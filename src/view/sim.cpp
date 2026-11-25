#include "sim.hpp"

#include <chrono>
#include <cstdio>

bool sim_birth(Sim& sim, const SimSettings& settings) {
    sim.settings = settings;
    sim.engine = settings.engine.engine;
    sim.rng = settings.seed * 0x9e3779b97f4a7c15ull + 7;
    sim.input_was_on = false;
    sim.task.on = false;
    sim.counts = EngineCounts{};
    sim.probe = SimProbe{};
    if (!engine_built(sim.engine)) {
        return false;
    }
    if (sim.engine == ENGINE_FAKE) {
        if (settings.task || settings.probe.feature >= 0) {
            std::fprintf(stderr, "blobview: --task and --probe run a real engine, not --fake\n");
            return false;
        }
        sim.fake_blob = fake_blob_birth(settings.engine.genome, settings.seed, settings.scene);
    } else {
        sim.blob = engine_birth(settings.engine, settings.seed, true);
        if (sim.blob == nullptr) {
            return false;
        }
    }
    if (settings.task && (!sim_task_start(sim) || !twin_birth(sim.twin, settings))) {
        return false;
    }
    if (settings.probe.feature >= 0) {
        sim_probe_ask(sim, settings.probe.feature);
    }
    tape_init(sim.tape, sim_blob(sim).tile_count);
    sim.shared.ticks = 0;
    return true;
}

void sim_free(Sim& sim) {
    twin_free(sim.twin);
    if (sim.fake_blob != nullptr) {
        fake_blob_free(sim.fake_blob);
        sim.fake_blob = nullptr;
    }
    engine_free(sim.engine, sim.blob);
    sim.blob = nullptr;
}

const Blob& sim_blob(const Sim& sim) {
    return sim.fake_blob != nullptr ? sim.fake_blob->blob : *sim.blob;
}

// the tick's counts for the lines over time: the engine's own, and the reward lines from
// the broadcast (full engine) or the plug's good and bad lines (any engine)
static LinePoint line_point(Sim& sim) {
    const Blob& b = sim_blob(sim);
    LinePoint p;
    engine_line_point(sim.engine, b, sim.counts, p);
    const Plug& plug = sim.task.ticker.plug;
    const bool task = sim.task.on && sim.task.plugged;
    p.v[LINE_GOOD] = b.good_ticks > 0 || (task && plug.good_left > 0) ? 1.0f : 0.0f;
    p.v[LINE_BAD] = b.bad_ticks > 0 || (task && plug.bad_left > 0) ? 1.0f : 0.0f;
    return p;
}

bool sim_tick(Sim& sim) {
    const int reward = sim.shared.reward.exchange(0);
    bool answered = false;
    if (sim.fake_blob != nullptr) {
        if (reward != 0) {
            fake_blob_reward(*sim.fake_blob, reward);
        }
        fake_blob_tick(*sim.fake_blob);
    } else {
        if (reward != 0) {
            engine_reward(sim.engine, *sim.blob, reward);
        }
        // the world's side: the readability probe, the task through the plug, random bits,
        // or nothing
        const bool probing = sim_probe_on(sim);
        if (probing) {
            sim_probe_before_tick(sim);
        } else if (sim.task.on) {
            sim_task_before_tick(sim);
        } else {
            sim_random_input(sim, sim.shared.input_on.load(std::memory_order_relaxed));
        }
        engine_tick(sim.engine, *sim.blob);
        if (probing) {
            sim_probe_after_tick(sim);
        } else if (sim.task.on) {
            answered = sim_task_after_tick(sim);
        }
    }
    tape_record(sim.tape, sim_blob(sim), line_point(sim));
    sim.shared.ticks.store(sim.tape.ticks, std::memory_order_relaxed);
    return answered;
}

void sim_fill(Sim& sim, ViewFrame& out, int selected) {
    // the whole-blob histograms cost more, so they come every 6th fill
    const bool heavy = sim.fills % 6 == 0;
    ++sim.fills;
    tape_copy_new(out.tape, sim.tape);
    frame_fill(out, sim_blob(sim), selected, heavy, sim.engine);
    sim_task_fill(sim, out.task, out.scores);
    if (sim.task.on) {
        out.readout = sim.task.readout.view;
    } else {
        out.readout = ReadoutView{};
    }
    if (sim.blob != nullptr) {
        readout_fill_selected(sim.task.readout, *sim.blob, out.selected);
    }
    sim_probe_fill(sim, out);
}

static void sim_loop(Sim& sim) {
    using clock = std::chrono::steady_clock;
    SimShared& sh = sim.shared;
    clock::time_point next_slow = clock::now();
    while (!sh.quit.load()) {
        // a readability measurement asked for from the panel
        const int measure = sh.measure.exchange(-1);
        if (measure >= 0) {
            sim_probe_ask(sim, measure);
        }

        // tick or not, by the mode
        // a measurement asked for runs flat out whatever the mode, so it never takes minutes
        const bool measuring = (sim.probe.on || sim.probe.pending >= 0) && !sim.probe.live &&
                               sim.settings.probe.feature < 0;
        const int mode = measuring ? RUN_FULL : sh.mode.load();
        bool tick = false, task_step = false;
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
        } else if (sh.task_steps.load() > 0) {
            tick = task_step = true; // ticks until the step's answer is in
        }
        if (tick && sim_tick(sim) && task_step) {
            sh.task_steps.fetch_sub(1);
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
    if (sim.task.on) {
        twin_start(sim.twin);
    }
}

void sim_stop(Sim& sim) {
    sim.shared.quit = true;
    if (sim.thread.joinable()) {
        sim.thread.join();
    }
    twin_stop(sim.twin);
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

bool sim_restart(Sim& sim, bool learning_on) {
    sim_stop(sim);
    SimSettings settings = sim.settings;
    settings.engine.genome.learning_on = learning_on ? 1 : 0;
    settings.engine.table.learning_on = learning_on ? 1 : 0;
    sim_free(sim);
    sim.shared.steps = 0;
    sim.shared.task_steps = 0;
    if (!sim_birth(sim, settings)) {
        return false;
    }
    sim_start(sim);
    return true;
}

double sim_run_capture(Sim& sim, int64_t ticks) {
    // the twin keeps up on its own thread, as in the window, and is waited for at the end
    const auto start = std::chrono::steady_clock::now();
    if (sim.task.on) {
        twin_start(sim.twin);
    }
    for (int64_t i = 0; i < ticks; ++i) {
        const int measure = sim.shared.measure.exchange(-1);
        if (measure >= 0) {
            sim_probe_ask(sim, measure);
        }
        sim_tick(sim);
    }
    const double secs =
        std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    if (sim.task.on) {
        while (sim.twin.done.load() < sim.task.view.step) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        twin_stop(sim.twin);
    }
    return secs;
}
