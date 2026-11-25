// memprobe: two worlds the suite does not have, for the memory engine (idea 123), driven
// through the same plug and Blob shell as blobrun.
//
//   memprobe pairs --n 256 [--on 8] [--buttons 16]   N random line patterns, each with its
//                     own button drawn at random: how many cue-button pairs it holds
//   memprobe hold --distractors 2 [--k 2]            delayed echo where other lines blink
//                     in the gap: how many events a cue survives
//   memprobe bytes                                   the store's size for the numbers given
// common: --steps --window --seed --sensor-side --motor-side --ticks, --y.name value, and
// --off 1 for the learning-off twin. Prints early and late right/tries.

#include "blob/random_words.hpp"
#include "memory/memory.hpp"
#include "world/plug.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

struct ProbeFlags {
    const char* mode = "pairs";
    int n = 256, on = 8, buttons = 16;
    int distractors = 1, k = 2;
    int64_t steps = 0;
    int window = 1000;
    uint64_t seed = 1;
    int off = 0;
    PlugSettings plug;
};

static void parse(int argc, char** argv, ProbeFlags& f) {
    f.plug.ticks_per_step = 20;
    f.plug.sensor_side = 2;
    f.plug.motor_side = 3;
    if (argc > 1) {
        f.mode = argv[1];
    }
    for (int i = 2; i + 1 < argc; i += 2) {
        const char* a = argv[i];
        int64_t v = std::strtoll(argv[i + 1], nullptr, 10);
        if (!std::strcmp(a, "--n")) f.n = (int)v;
        else if (!std::strcmp(a, "--on")) f.on = (int)v;
        else if (!std::strcmp(a, "--buttons")) f.buttons = (int)v;
        else if (!std::strcmp(a, "--distractors")) f.distractors = (int)v;
        else if (!std::strcmp(a, "--k")) f.k = (int)v;
        else if (!std::strcmp(a, "--steps")) f.steps = v;
        else if (!std::strcmp(a, "--window")) f.window = (int)v;
        else if (!std::strcmp(a, "--seed")) f.seed = (uint64_t)v;
        else if (!std::strcmp(a, "--off")) f.off = (int)v;
        else if (!std::strcmp(a, "--sensor-side")) f.plug.sensor_side = (int)v;
        else if (!std::strcmp(a, "--motor-side")) f.plug.motor_side = (int)v;
        else if (!std::strcmp(a, "--ticks")) f.plug.ticks_per_step = (int)v;
        else if (std::strncmp(a, "--y.", 4) != 0) {
            std::fprintf(stderr, "memprobe: unknown flag %s\n", a);
            std::exit(1);
        }
    }
}

// one task step through the plug: lines on, ticks, the press, the reward
static int world_step(Plug& plug, Blob& blob, const uint8_t* lines, int answer) {
    TaskSense sense;
    for (int i = 0; i < plug.shape.singles; i++) {
        sense.singles[i] = lines[i];
    }
    for (int i = 0; i < plug.shape.sheet_w * plug.shape.sheet_h; i++) {
        sense.sheet[i] = lines[i];
    }
    plug_set_lines(plug, sense);
    for (int t = 0; t < plug.settings.ticks_per_step; t++) {
        plug_write(plug, blob);
        memory_tick(blob);
        plug_count(plug, blob);
    }
    int move = plug_choice(plug);
    if (answer < -1) {
        return move; // a step nobody answers
    }
    int valence = move < 0 ? 0 : (move == answer ? 1 : -1);
    if (valence) {
        plug_reward(plug, valence);
        plug_mark_acted(plug, blob, move);
        memory_reward(blob, valence);
    }
    return move;
}

struct Tally {
    std::vector<double> windows;
    int64_t right = 0, tries = 0, in_window = 0;
};

static void tally(Tally& t, bool right, int window) {
    t.right += right;
    t.tries++;
    if (++t.in_window == window) {
        t.windows.push_back((double)t.right / (double)t.tries);
        t.right = t.tries = t.in_window = 0;
    }
}

static void report(const ProbeFlags& f, const Tally& t, const char* what) {
    size_t n = t.windows.size();
    size_t edge = n / 10 > 0 ? n / 10 : 1;
    double early = 0, late = 0;
    for (size_t i = 0; i < edge && i < n; i++) {
        early += t.windows[i] / (double)edge;
        late += t.windows[n - 1 - i] / (double)edge;
    }
    std::printf("%s seed %llu %s early %.4f late %.4f windows %zu\n", f.mode,
                (unsigned long long)f.seed, what, early, late, n);
}

static int pairs(const ProbeFlags& f, Blob& blob, uint64_t* rng) {
    TaskShape shape;
    shape.sheet_w = 8; // the sensor grid at 2x2 groups, under good and bad
    shape.sheet_h = 7;
    shape.motors = f.buttons;
    Plug plug;
    if (!plug_attach(plug, blob, shape, f.plug)) {
        return 1;
    }
    int lines = shape.sheet_w * shape.sheet_h;
    std::vector<uint8_t> pattern((size_t)f.n * lines, 0);
    std::vector<int> button((size_t)f.n);
    for (int p = 0; p < f.n; p++) {
        for (int j = 0; j < f.on;) {
            int l = (int)rng_below(rng, (uint32_t)lines);
            if (!pattern[(size_t)p * lines + l]) {
                pattern[(size_t)p * lines + l] = 1;
                j++;
            }
        }
        button[(size_t)p] = (int)rng_below(rng, (uint32_t)f.buttons);
    }
    int64_t steps = f.steps > 0 ? f.steps : 40000 + 100 * (int64_t)f.n;
    Tally t;
    for (int64_t s = 0; s < steps; s++) {
        int p = (int)rng_below(rng, (uint32_t)f.n);
        int move = world_step(plug, blob, &pattern[(size_t)p * lines], button[(size_t)p]);
        tally(t, move == button[(size_t)p], f.window);
    }
    char what[64];
    std::snprintf(what, sizeof what, "n %d on %d buttons %d", f.n, f.on, f.buttons);
    report(f, t, what);
    return 0;
}

// a trial: blank, the cue, then the gap (blank, a distractor line, blank, ...), then ask
static int hold(const ProbeFlags& f, Blob& blob, uint64_t* rng) {
    int distractor_lines = 4;
    TaskShape shape;
    shape.singles = f.k + distractor_lines + 1;
    shape.motors = f.k;
    Plug plug;
    if (!plug_attach(plug, blob, shape, f.plug)) {
        return 1;
    }
    int ask = f.k + distractor_lines;
    int64_t steps = f.steps > 0 ? f.steps : 40000;
    int per_trial = 3 + 2 * f.distractors + 1;
    Tally t;
    uint8_t lines[TASK_MAX_SINGLES] = {};
    for (int64_t s = 0; s + per_trial <= steps; s += per_trial) {
        int cue = (int)rng_below(rng, (uint32_t)f.k);
        std::memset(lines, 0, sizeof lines);
        world_step(plug, blob, lines, -2);
        lines[cue] = 1;
        world_step(plug, blob, lines, -2);
        lines[cue] = 0;
        for (int d = 0; d < f.distractors; d++) {
            world_step(plug, blob, lines, -2);
            int l = f.k + (int)rng_below(rng, (uint32_t)distractor_lines);
            lines[l] = 1;
            world_step(plug, blob, lines, -2);
            lines[l] = 0;
        }
        world_step(plug, blob, lines, -2);
        lines[ask] = 1;
        int move = world_step(plug, blob, lines, cue);
        tally(t, move == cue, f.window / per_trial > 0 ? f.window / per_trial : 1);
    }
    char what[64];
    std::snprintf(what, sizeof what, "k %d distractors %d", f.k, f.distractors);
    report(f, t, what);
    return 0;
}

int main(int argc, char** argv) {
    ProbeFlags f;
    parse(argc, argv, f);
    Genome genome;
    apply_genome_args(argc, argv, genome);
    MemoryGenome mg;
    apply_memory_args(argc, argv, mg);
    if (f.off) {
        genome.learning_on = 0;
    }
    Blob* blob = memory_birth(genome, mg, f.seed);
    uint64_t rng[4];
    rng_seed(rng, f.seed * 7919 + 17);
    int code = 0;
    if (!std::strcmp(f.mode, "pairs")) {
        code = pairs(f, *blob, rng);
    } else if (!std::strcmp(f.mode, "hold")) {
        code = hold(f, *blob, rng);
    } else if (!std::strcmp(f.mode, "bytes")) {
        const MemoryBlob& m = memory_of(*blob);
        std::printf("bytes %zu  counters %zu  tables %d  slots %d\n", memory_bytes(m),
                    (size_t)m.mg.tables * m.slots * MEM_UNITS, m.mg.tables, m.slots);
    } else {
        std::fprintf(stderr, "memprobe pairs|hold|bytes ...\n");
        code = 1;
    }
    memory_free(blob);
    return code;
}
