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
};

// distinct random neurons of one kind (sensor or motor word set)
static void pick_neurons(const Blob& blob, const uint64_t* kind, uint64_t* rng, int count,
                         int* out, uint64_t* taken) {
    int found = 0;
    while (found < count) {
        int n = (int)rng_below(rng, (uint32_t)blob.neuron_count);
        uint64_t bit = 1ull << (n % TILE_NEURONS);
        int t = n / TILE_NEURONS;
        if (!(kind[t] & bit) || (taken[t] & bit)) {
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
        pick_neurons(blob, blob.sensor, rng, e.lines_per_cue, e.cue_line[c], taken);
        pick_neurons(blob, blob.motor, rng, e.group_size, e.group_neuron[c], taken);
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

// one trial: returns 1 when the right group fired most
static int run_trial(Blob& blob, const EchoSetup& e, uint64_t* rng, int64_t* motor_spikes) {
    int cue = (int)rng_below(rng, (uint32_t)e.cues);
    int counts[MAX_CUES] = {};
    set_cue(blob, e, cue);
    for (int i = 0; i < e.window; i++) {
        blob_tick(blob);
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
    int tiles = (int)arg_int(argc, argv, "--tiles", 4);
    int64_t trials = arg_int(argc, argv, "--trials", 20000);
    int64_t report = arg_int(argc, argv, "--report", 2000);
    uint64_t seed = (uint64_t)arg_int(argc, argv, "--seed", 1);
    int learn_only = (int)arg_int(argc, argv, "--learn", -1); // -1 runs both
    int layout = (int)arg_int(argc, argv, "--layout", 1);
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
        for (int64_t i = 0; i < windows * report; i++) {
            hits[learn][i / report] += run_trial(*blob, e, rng, &spikes[learn][i / report]);
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
