#include "minimal/probe/probe.hpp"
#include "run/task_ticker.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

// the probe's runs: a fresh blob left alone, a long task life, teach-idle-retest, speed.

static uint64_t probe_random(uint64_t& state) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

// random bits on every sensor cell, each on with chance 1 in 8 (as blobprobe rhythm does)
static void random_input(Blob& blob, uint64_t& state) {
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t on = probe_random(state) & probe_random(state) & probe_random(state);
        blob.sense_attached[t] = blob.sensor[t];
        blob.sense_in[t] = blob.sensor[t] & on;
    }
}

int probe_alive(ProbeArgs& args) {
    Blob* blob = probe_birth(args, args.flags.genome, args.flags.seed);
    uint64_t state = 0x9E3779B97F4A7C15ull ^ args.flags.seed;
    Recorder rec;
    recorder_start(rec, *blob);
    for (int64_t t = 0; t < args.warmup + args.tail_ticks; t++) {
        if (args.input) {
            random_input(*blob, state);
        }
        probe_tick(args, *blob);
        if (t >= args.warmup) {
            recorder_add(rec, args, *blob);
        }
    }
    recorder_print(rec, args, *blob, args.input ? "random input" : "no input");
    probe_free(args, blob);
    return 0;
}

// presses over a stretch: [0] no move, [1 + b] button b (up to 16), and how many were bad
struct Presses {
    int64_t move[17] = {};
    int64_t bad = 0, steps = 0;
};

// run `steps` task steps; record the last `tail` ticks; return the task's score over them
static double run_steps(const ProbeArgs& args, Blob& blob, TaskTicker& tt, int64_t steps,
                        Recorder* rec, int64_t tail, Presses* presses = nullptr) {
    task_window_score(tt.task);
    int64_t ticks_total = steps * (tt.ticks_per_step + 0);
    int64_t tick = 0, step = 0;
    while (step < steps) {
        ticker_before_tick(tt, blob);
        probe_tick(args, blob);
        if (rec && tick >= ticks_total - tail) {
            recorder_add(*rec, args, blob);
        }
        tick++;
        if (!ticker_after_tick(tt, blob)) {
            continue;
        }
        if (presses) {
            int b = tt.move < 0 ? 0 : (tt.move < 16 ? 1 + tt.move : 16);
            presses->move[b]++;
            presses->bad += tt.valence < 0 ? 1 : 0;
            presses->steps++;
        }
        if (tt.valence != 0) {
            plug_mark_acted(tt.plug, blob, tt.move); // for the readout, as blobrun does
            probe_reward(args, blob, tt.valence);
        }
        step++;
    }
    return task_window_score(tt.task);
}

int probe_life(ProbeArgs& args) {
    const RunFlags& f = args.flags;
    Blob* blob = probe_birth(args, f.genome, f.seed);
    TaskTicker tt;
    if (!ticker_start(tt, *blob, f)) {
        return 1;
    }
    int64_t late = f.steps / 10;
    run_steps(args, *blob, tt, f.steps - late, nullptr, 0);
    Recorder rec;
    recorder_start(rec, *blob);
    Presses presses;
    double score = run_steps(args, *blob, tt, late, &rec, args.tail_ticks, &presses);
    std::printf("life: task %s, %lld steps of %d ticks, seed %llu, late score %.3f\n",
                task_name(f.task), (long long)f.steps, f.plug.ticks_per_step,
                (unsigned long long)f.seed, score);
    // the last tenth: which buttons it pressed, and how often it was told bad
    double all = (double)(presses.steps ? presses.steps : 1);
    std::printf("  late presses: none %.2f", presses.move[0] / all);
    for (int b = 1; b < 17; b++) {
        if (presses.move[b]) {
            std::printf("  button %d %.2f", b - 1, presses.move[b] / all);
        }
    }
    std::printf("  bad %.2f\n", presses.bad / all);
    recorder_print(rec, args, *blob, "end of life");
    print_wiring(args, *blob, "end of life");
    probe_free(args, blob);
    return 0;
}

// which connections exist, one bit per possible source
static std::vector<uint64_t> snapshot(const ProbeArgs& args, const Blob& blob) {
    if (args.engine == ProbeEngine::minimal) {
        const MinimalBlob& m = minimal_of(blob);
        size_t words = (size_t)blob.neuron_count * m.slots;
        return std::vector<uint64_t>(m.mask, m.mask + words);
    }
    size_t words = (size_t)blob.neuron_count * SLOTS;
    std::vector<uint64_t> v(words);
    for (size_t i = 0; i < words; i++) {
        v[i] = blob.w0[i] | blob.w1[i];
    }
    return v;
}

int probe_idle(ProbeArgs& args) {
    const RunFlags& f = args.flags;
    Blob* blob = probe_birth(args, f.genome, f.seed);
    TaskTicker tt;
    if (!ticker_start(tt, *blob, f)) {
        return 1;
    }
    run_steps(args, *blob, tt, f.steps, nullptr, 0);

    // test frozen, so the test itself teaches nothing
    probe_set_learning(args, *blob, 0);
    double before = run_steps(args, *blob, tt, args.test_steps, nullptr, 0);
    probe_set_learning(args, *blob, 1);
    std::vector<uint64_t> kept = snapshot(args, *blob);

    // unplugged: nothing written, nothing rewarded, learning on
    plug_detach(tt.plug, *blob);
    blob_detach_all(*blob);
    Recorder rec;
    recorder_start(rec, *blob);
    for (int64_t t = 0; t < args.idle_ticks; t++) {
        probe_tick(args, *blob);
        if (t >= args.idle_ticks - args.tail_ticks) {
            recorder_add(rec, args, *blob);
        }
    }
    std::vector<uint64_t> now = snapshot(args, *blob);
    int64_t had = 0, still = 0;
    for (size_t i = 0; i < kept.size(); i++) {
        had += __builtin_popcountll(kept[i]);
        still += __builtin_popcountll(kept[i] & now[i]);
    }

    // plug back in and test frozen again
    if (!ticker_start(tt, *blob, f)) {
        return 1;
    }
    probe_set_learning(args, *blob, 0);
    double after = run_steps(args, *blob, tt, args.test_steps, nullptr, 0);
    std::printf("idle: %s, task %s, seed %llu, taught %lld steps, idle %lld ticks\n",
                probe_engine_name(args), task_name(f.task), (unsigned long long)f.seed,
                (long long)f.steps, (long long)args.idle_ticks);
    std::printf("  score before idle %.3f  after idle %.3f  connections kept %.1f%%\n", before,
                after, 100.0 * (double)still / (double)(had ? had : 1));
    recorder_print(rec, args, *blob, "end of idle");
    probe_free(args, blob);
    return 0;
}

// a square sheet, sensor and motor regions kept inside it (as blobbench does)
static Genome sheet_genome(const Genome& base, int tiles) {
    Genome g = base;
    g.tiles_w = g.tiles_h = tiles;
    int cells = tiles * 8;
    int sensor_side = cells / 2 < g.sensor_w ? cells / 2 : g.sensor_w;
    int motor_side = cells / 2 < g.motor_w ? cells / 2 : g.motor_w;
    int motor_at = cells - motor_side < g.motor_x ? cells - motor_side : g.motor_x;
    g.sensor_w = g.sensor_h = sensor_side;
    g.motor_x = g.motor_y = motor_at;
    g.motor_w = g.motor_h = motor_side;
    return g;
}

static double seconds_now() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

int probe_speed(ProbeArgs& args) {
    int sizes[] = {8, 32, 128}; // 4,096 / 65,536 / 1,048,576 neurons
    std::printf("%-8s %9s %6s %12s %16s %10s\n", "engine", "neurons", "learn", "ticks/s",
                "neuron-upd/s", "bytes/n");
    for (int tiles : sizes) {
        if (args.tiles && tiles != args.tiles) {
            continue;
        }
        for (int learn = 0; learn < 2; learn++) {
            Genome g = sheet_genome(args.flags.genome, tiles);
            g.learning_on = learn;
            Blob* blob = probe_birth(args, g, 1);
            uint64_t state = 0x9E3779B97F4A7C15ull;
            // settle, then time whole batches; random sensor input, a reward every 50 ticks
            int64_t ticks = 0;
            double start = 0, elapsed = 0;
            for (int64_t i = 0;; i++) {
                if (i == 200) {
                    start = seconds_now();
                }
                random_input(*blob, state);
                if (learn && i % 50 == 0) {
                    probe_reward(args, *blob, (probe_random(state) & 1) ? 1 : -1);
                }
                probe_tick(args, *blob);
                if (i >= 200) {
                    ticks++;
                    if ((ticks & 15) == 0) {
                        elapsed = seconds_now() - start;
                        if (elapsed * 1000.0 >= args.seconds_ms) {
                            break;
                        }
                    }
                }
            }
            double tps = (double)ticks / elapsed;
            std::printf("%-8s %9d %6s %12.0f %16.3e %10.1f\n", probe_engine_name(args),
                        blob->neuron_count, learn ? "on" : "off", tps, tps * blob->neuron_count,
                        probe_bytes_per_neuron(args, *blob));
            std::fflush(stdout);
            probe_free(args, blob);
        }
    }
    return 0;
}
