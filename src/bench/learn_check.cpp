#include <cstdio>

#include "bench/bench_modes.hpp"
#include "blob/blob.hpp"
#include "blob/random_words.hpp"

// the core's own small check that learning does something. one of K cues is held on a few
// sensor lines; K groups of motor neurons; good when the group that matches the cue fires
// most over the window, bad otherwise. which group matches which cue is shuffled per seed,
// and the lines and groups are random cells inside their regions, so nothing born-in can be
// right. the same genome and seed run with learning off is the baseline.

constexpr int MAX_CUES = 8;
constexpr int MAX_LINES = 64;

struct EchoSetup {
    int cues = 2;
    int lines_per_cue = 8;
    int group_size = 8;
    int window = 20; // ticks the cue is on and motors are counted
    int gap = 10;    // quiet ticks after each trial
    int cue_line[MAX_CUES][MAX_LINES];
    int group_neuron[MAX_CUES][MAX_LINES];
    int answer[MAX_CUES]; // motor group that is right for each cue
    // cells the lines and groups are drawn from (whole sheet unless narrowed)
    int sensor_box[4] = {0, 0, 1 << 20, 1 << 20}; // x0, y0, x1, y1
    int motor_box[4] = {0, 0, 1 << 20, 1 << 20};
};

// distinct random neurons of one kind (sensor or motor word set)
static void pick_neurons(const Blob& blob, const uint64_t* kind, const int* box, uint64_t* rng,
                         int count, int* out, uint64_t* taken) {
    int found = 0;
    while (found < count) {
        int n = (int)rng_below(rng, (uint32_t)blob.neuron_count);
        uint64_t bit = 1ull << (n % TILE_NEURONS);
        int t = n / TILE_NEURONS;
        if (!(kind[t] & bit) || (taken[t] & bit)) {
            continue;
        }
        int x = blob_neuron_x(blob, n), y = blob_neuron_y(blob, n);
        if (x < box[0] || y < box[1] || x >= box[2] || y >= box[3]) {
            continue;
        }
        taken[t] |= bit;
        out[found++] = n;
    }
}

static void setup_echo(const Blob& blob, EchoSetup& e, uint64_t seed) {
    uint64_t rng[4];
    rng_seed(rng, seed * 31337 + 5);
    uint64_t* taken = new uint64_t[blob.tile_count + 1]();
    for (int c = 0; c < e.cues; c++) {
        pick_neurons(blob, blob.sensor, e.sensor_box, rng, e.lines_per_cue, e.cue_line[c],
                     taken);
        pick_neurons(blob, blob.motor, e.motor_box, rng, e.group_size, e.group_neuron[c],
                     taken);
        e.answer[c] = c;
    }
    // shuffle which group answers which cue
    for (int c = e.cues - 1; c > 0; c--) {
        int j = (int)rng_below(rng, (uint32_t)(c + 1));
        int keep = e.answer[c];
        e.answer[c] = e.answer[j];
        e.answer[j] = keep;
    }
    delete[] taken;
}

static void set_cue(Blob& blob, const EchoSetup& e, int cue) {
    for (int c = 0; c < e.cues; c++) {
        for (int l = 0; l < e.lines_per_cue; l++) {
            blob_sense(blob, e.cue_line[c][l], c == cue);
        }
    }
}

// how well each tile tells cue 0 from cue 1: spikes per neuron during each cue's windows
struct CueSpread {
    int64_t* spikes[2] = {};
    int64_t ticks[2] = {};
};

static void count_spread(const Blob& blob, CueSpread& spread, int cue) {
    if (!spread.spikes[0] || cue > 1) {
        return;
    }
    const uint64_t* now = blob.spikes[blob.now];
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t w = now[t];
        while (w) {
            spread.spikes[cue][t * TILE_NEURONS + __builtin_ctzll(w)]++;
            w &= w - 1;
        }
    }
    spread.ticks[cue]++;
}

// per tile: mean over its neurons of |rate under cue 0 - rate under cue 1|, spikes per
// 1,000 ticks. S marks a tile with cue lines, M one with motor group neurons.
static void print_spread(const Blob& blob, const EchoSetup& e, const CueSpread& spread) {
    std::printf("cue 0 vs cue 1, |rate difference| per tile (spikes per 1000 ticks):\n");
    for (int ty = 0; ty < blob.tiles_h; ty++) {
        for (int tx = 0; tx < blob.tiles_w; tx++) {
            int t = ty * blob.tiles_w + tx;
            int64_t sum = 0;
            for (int b = 0; b < TILE_NEURONS; b++) {
                int n = t * TILE_NEURONS + b;
                if ((blob.sensor[t] >> b) & 1) {
                    continue;
                }
                int64_t r0 = spread.spikes[0][n] * 1000 / (spread.ticks[0] ? spread.ticks[0] : 1);
                int64_t r1 = spread.spikes[1][n] * 1000 / (spread.ticks[1] ? spread.ticks[1] : 1);
                sum += r0 > r1 ? r0 - r1 : r1 - r0;
            }
            char mark = ' ';
            for (int c = 0; c < e.cues; c++) {
                for (int l = 0; l < e.lines_per_cue; l++) {
                    mark = e.cue_line[c][l] / TILE_NEURONS == t ? 'S' : mark;
                }
                for (int m = 0; m < e.group_size; m++) {
                    mark = e.group_neuron[c][m] / TILE_NEURONS == t ? 'M' : mark;
                }
            }
            std::printf("%5lld%c", (long long)(sum / TILE_NEURONS), mark);
        }
        std::printf("\n");
    }
}

// one trial: returns 1 when the right group fired most
static int run_trial(Blob& blob, const EchoSetup& e, uint64_t* rng, int64_t* motor_spikes,
                     CueSpread& spread) {
    int cue = (int)rng_below(rng, (uint32_t)e.cues);
    int counts[MAX_CUES] = {};
    set_cue(blob, e, cue);
    for (int i = 0; i < e.window; i++) {
        blob_tick(blob);
        count_spread(blob, spread, cue);
        for (int g = 0; g < e.cues; g++) {
            for (int m = 0; m < e.group_size; m++) {
                counts[g] += blob_fired(blob, e.group_neuron[g][m]) ? 1 : 0;
            }
        }
    }
    // the winner, ties broken by chance
    int best = 0, ties = 0, winner = 0;
    for (int g = 0; g < e.cues; g++) {
        *motor_spikes += counts[g];
        if (counts[g] > best || g == 0) {
            best = counts[g], ties = 1, winner = g;
        } else if (counts[g] == best) {
            ties++;
            if (rng_below(rng, (uint32_t)ties) == 0) {
                winner = g;
            }
        }
    }
    int hit = winner == e.answer[cue] ? 1 : 0;
    blob_reward(blob, hit ? 1 : -1);
    set_cue(blob, e, -1);
    for (int i = 0; i < e.gap; i++) {
        blob_tick(blob);
    }
    return hit;
}

int run_learn_check(int argc, char** argv) {
    int tiles = (int)arg_int(argc, argv, "--tiles", 8);
    int64_t trials = arg_int(argc, argv, "--trials", 20000);
    int64_t report = arg_int(argc, argv, "--report", 2000);
    uint64_t seed = (uint64_t)arg_int(argc, argv, "--seed", 1);
    int learn_only = (int)arg_int(argc, argv, "--learn", -1); // -1 runs both
    int layout = (int)arg_int(argc, argv, "--layout", 0);
    int show_spread = (int)arg_int(argc, argv, "--spread", 0);
    EchoSetup e;
    e.cues = (int)arg_int(argc, argv, "--cues", 2);
    e.lines_per_cue = (int)arg_int(argc, argv, "--lines", 8);
    e.group_size = (int)arg_int(argc, argv, "--group", 8);
    e.window = (int)arg_int(argc, argv, "--window", 20);
    e.gap = (int)arg_int(argc, argv, "--gap", 10);

    Genome g = genome_for_sheet(tiles);
    int cells = tiles * 8;
    if (layout == 1) {
        // sensor along the top tile row, motor along the row below it
        g.sensor_x = 0, g.sensor_y = 0, g.sensor_w = cells, g.sensor_h = 8;
        g.motor_x = 0, g.motor_y = 8, g.motor_w = cells, g.motor_h = 8;
    } else if (layout == 2) {
        // one empty tile row between them
        g.sensor_x = 0, g.sensor_y = 0, g.sensor_w = cells, g.sensor_h = 8;
        g.motor_x = 0, g.motor_y = 16, g.motor_w = cells, g.motor_h = 8;
    }
    apply_genome_args(argc, argv, g);
    if (layout == 3) {
        // genome areas, but lines from the 16x16 sensor corner nearest the motor area and
        // groups from the 16x16 motor corner nearest the sensor area
        int sx1 = g.sensor_x + g.sensor_w, sy1 = g.sensor_y + g.sensor_h;
        int box_s[4] = {sx1 - 16, sy1 - 16, sx1, sy1};
        int box_m[4] = {g.motor_x, g.motor_y, g.motor_x + 16, g.motor_y + 16};
        for (int k = 0; k < 4; k++) {
            e.sensor_box[k] = box_s[k];
            e.motor_box[k] = box_m[k];
        }
    }

    int64_t windows = trials / report;
    int64_t* hits[2] = {new int64_t[windows](), new int64_t[windows]()};
    int64_t* spikes[2] = {new int64_t[windows](), new int64_t[windows]()};
    for (int learn = 0; learn < 2; learn++) {
        if (learn_only >= 0 && learn != learn_only) {
            continue;
        }
        g.learning_on = learn;
        Blob* blob = blob_birth(g, seed);
        setup_echo(*blob, e, seed);
        uint64_t rng[4];
        rng_seed(rng, seed * 104729 + 3);
        CueSpread spread;
        if (show_spread) {
            spread.spikes[0] = new int64_t[blob->neuron_count]();
            spread.spikes[1] = new int64_t[blob->neuron_count]();
        }
        for (int64_t i = 0; i < windows * report; i++) {
            // the spread is measured over the last window only
            if (show_spread && i == (windows - 1) * report) {
                for (int n = 0; n < blob->neuron_count; n++) {
                    spread.spikes[0][n] = spread.spikes[1][n] = 0;
                }
                spread.ticks[0] = spread.ticks[1] = 0;
            }
            hits[learn][i / report] +=
                run_trial(*blob, e, rng, &spikes[learn][i / report], spread);
        }
        if (show_spread) {
            std::printf("learning %s, last window\n", learn ? "on" : "off");
            print_spread(*blob, e, spread);
            delete[] spread.spikes[0];
            delete[] spread.spikes[1];
        }
        blob_free(blob);
    }

    std::printf("echo: %d cues, %d lines each, groups of %d, window %d, gap %d, layout %d, "
                "%d neurons, seed %llu\n",
                e.cues, e.lines_per_cue, e.group_size, e.window, e.gap, layout, tiles * tiles * 64,
                (unsigned long long)seed);
    std::printf("%8s %10s %10s %12s %12s\n", "trials", "hit% on", "hit% off", "motor/trial on",
                "off");
    for (int64_t w = 0; w < windows; w++) {
        std::printf("%8lld %10.1f %10.1f %12.1f %12.1f\n", (long long)((w + 1) * report),
                    100.0 * hits[1][w] / report, 100.0 * hits[0][w] / report,
                    (double)spikes[1][w] / report, (double)spikes[0][w] / report);
    }
    for (int k = 0; k < 2; k++) {
        delete[] hits[k];
        delete[] spikes[k];
    }
    return 0;
}
