#include "sim.hpp"

#include <chrono>
#include <cstdio>

bool sim_birth(Sim& sim, const SimSettings& settings) {
    sim.settings = settings;
    sim.rng = settings.seed * 0x9e3779b97f4a7c15ull + 7;
    if (settings.fake) {
        sim.fake_blob = fake_blob_birth(settings.genome, settings.seed, settings.scene);
    } else {
#if BLOB_HAS_CORE
        sim.blob = blob_birth(settings.genome, settings.seed);
#else
        std::fprintf(stderr, "blobview: built without the core; only --fake runs\n");
        return false;
#endif
    }
    tape_init(sim.tape, sim_blob(sim).tile_count);
    return true;
}

void sim_free(Sim& sim) {
    if (sim.fake_blob != nullptr) {
        fake_blob_free(sim.fake_blob);
        sim.fake_blob = nullptr;
    }
#if BLOB_HAS_CORE
    if (sim.blob != nullptr) {
        blob_free(sim.blob);
        sim.blob = nullptr;
    }
#endif
}

const Blob& sim_blob(const Sim& sim) {
    return sim.fake_blob != nullptr ? sim.fake_blob->blob : *sim.blob;
}

#if BLOB_HAS_CORE
// the world's side for a real blob: random bits on the sensor lines (1 in 8), or nothing
// attached at all
static void feed_real(Sim& sim) {
    Blob& b = *sim.blob;
    const bool on = sim.shared.input_on.load(std::memory_order_relaxed);
    for (int t = 0; t < b.tile_count; ++t) {
        uint64_t r = ~0ull;
        for (int i = 0; i < 3; ++i) {
            r &= fake_random(sim.rng);
        }
        b.sense_attached[t] = on ? b.sensor[t] : 0;
        b.sense_in[t] = on ? r & b.sensor[t] : 0;
    }
}
#endif

void sim_tick(Sim& sim) {
    const int reward = sim.shared.reward.exchange(0);
    if (sim.fake_blob != nullptr) {
        if (reward != 0) {
            fake_blob_reward(*sim.fake_blob, reward);
        }
        fake_blob_tick(*sim.fake_blob);
    } else {
#if BLOB_HAS_CORE
        if (reward != 0) {
            blob_reward(*sim.blob, reward);
        }
        feed_real(sim);
        blob_tick(*sim.blob);
#endif
    }
    tape_record(sim.tape, sim_blob(sim));
    sim.shared.ticks.store(sim.tape.ticks, std::memory_order_relaxed);
}

void sim_fill(Sim& sim, ViewFrame& out, int selected) {
    // the whole-blob histograms cost more, so they come every 6th fill
    const bool heavy = sim.fills % 6 == 0;
    ++sim.fills;
    tape_copy_new(out.tape, sim.tape);
    frame_fill(out, sim_blob(sim), selected, heavy, sim.fake_blob == nullptr);
}

static void sim_loop(Sim& sim) {
    using clock = std::chrono::steady_clock;
    SimShared& sh = sim.shared;
    clock::time_point next_slow = clock::now();
    while (!sh.quit.load()) {
        // tick or not, by the mode
        const int mode = sh.mode.load();
        bool tick = false;
        if (mode == RUN_FULL || mode == RUN_TURBO) {
            tick = true;
        } else if (mode == RUN_SLOW) {
            const clock::time_point now = clock::now();
            if (now >= next_slow) {
                tick = true;
                const int tps = std::max(1, sh.slow_tps.load());
                next_slow += std::chrono::microseconds(1000000 / tps);
                if (now - next_slow > std::chrono::seconds(1)) {
                    next_slow = now; // fell far behind: do not race to catch up
                }
            }
        } else if (sh.steps.load() > 0) {
            sh.steps.fetch_sub(1);
            tick = true;
        }
        if (tick) {
            sim_tick(sim);
        }

        // a snapshot when the viewer asks for one
        if (sh.want_frame.exchange(false)) {
            std::lock_guard<std::mutex> hold(sh.lock);
            sim_fill(sim, sh.frame, sh.selected.load());
            sh.frame_ready = true;
        }

        // idle without spinning while paused or slow
        if (!tick) {
            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    }
}

void sim_start(Sim& sim) {
    sim.shared.quit = false;
    sim.thread = std::thread(sim_loop, std::ref(sim));
}

void sim_stop(Sim& sim) {
    sim.shared.quit = true;
    if (sim.thread.joinable()) {
        sim.thread.join();
    }
}

bool sim_take(Sim& sim, ViewFrame& to) {
    SimShared& sh = sim.shared;
    bool took = false;
    {
        std::lock_guard<std::mutex> hold(sh.lock);
        if (sh.frame_ready) {
            frame_copy(to, sh.frame);
            sh.frame_ready = false;
            took = true;
        }
    }
    sh.want_frame = true;
    return took;
}
