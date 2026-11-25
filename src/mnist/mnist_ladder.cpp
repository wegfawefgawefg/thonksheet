// blobmnist ladder: where the 0.2 between a linear reader of the sheet and the sheet's own
// output goes (docs/results-2026-11-25-output-gap.md). The sheet, frozen, is shown 60,000
// training digits (the task's stream) and the 10,000 test digits once; each neuron's spike
// count per digit is kept (over all 20 ticks, and over the last 15, the window the engine
// learns from). Then readers run on that recording, each one handicap closer to the engine:
//   1 float softmax, offline, 10 passes, every non-sensor cell (the ceiling's reader)
//   2 the same, online, one pass, checked as it goes
//   3 integer weights, online, told the right button (the perceptron)
//   4 the same, told only good or bad: 4n nothing moves on good, 4u the pressed button
//     gains on good (the engine's readout_good 1), 4g only when the vote was close
//   5 each button is its 102 motor cells, each hearing only its own 11 slots (sensor cells
//     included), integer weights, the engine's good and bad rule: 5s the sum of the cells'
//     drives, 5v a count of cells over their own threshold
//   6 evidence counters at the engine's width (8 bits) and step chances, learning from the
//     15-tick window: 6c counters used as graded weights, 6b one-bit wiring of the top
//     counters at the born count (the engine's rule), 6g the same with the close-vote gate
// Online readers go through the stream twice (120,000 digits) and are tested on all test
// digits at 1,000, 5,000, 20,000, 60,000 and 120,000.
//   --life N        first N digits with no reward under the genome's own rules (competitive
//                   learning), then frozen: the lived sheet
//   --record N      training digits recorded (default 60000)
//   --test-count N  test digits (default 10000)
//   --rungs LIST    which readers (default 1,2,3,4n,4u,4g,5s,5v,6c,6b,6g); the fixes screened
//                   on rung 6 before they went into the engine: 6k2, 6k4 (two and four times
//                   the budget), 6h (a counter at the clamp halves the row), 6s (top quarter
//                   at strength 4), 6d (excitatory sources only), and with the gate 6gh,
//                   6gk2, 6gs, 6ghk2
// the rest are blobrun's flags.

#include "mnist/mnist_ladder.hpp"
#include "mnist/mnist_run.hpp"
#include "blob/tick_steps.hpp"
#include "world/mnist_data.hpp"

#include <cstdio>
#include <cstring>

static const char* const ladder_own[] = {"--life", "--record", "--test-count", "--rungs",
                                         "--settle"};

int ladder_stream(const Ladder& L, int64_t i) {
    int64_t pass = i / L.train.n, at = i % L.train.n;
    if (pass == 0) {
        return (int)at;
    }
    const std::vector<int>& order = L.later[(size_t)((pass - 1) % (int64_t)L.later.size())];
    return order[(size_t)at];
}

int ladder_pick(const double* score, int k) {
    int best = 0;
    bool tie = false;
    for (int j = 1; j < k; j++) {
        if (score[j] > score[best]) {
            best = j;
            tie = false;
        } else if (score[j] == score[best]) {
            tie = true;
        }
    }
    return tie ? -1 : best;
}

void ladder_print(const char* name, const Curve& c) {
    std::printf("%-44s", name);
    for (int k = 0; k < LADDER_CHECKS; k++) {
        std::printf(" %7.4f", c.acc[k]);
    }
    std::printf("   quiet %d %d %d %d %d  ties %.3f\n", c.quiet_buttons[0], c.quiet_buttons[1],
                c.quiet_buttons[2], c.quiet_buttons[3], c.quiet_buttons[4],
                c.ties[LADDER_CHECKS - 1]);
    std::fflush(stdout);
}

// the frozen sheet shown `count` digits of whatever the task streams next
static void record_digits(Sheet& s, int count, Record& r) {
    TaskTicker& tt = *s.tt;
    Blob& b = *s.blob;
    r.d = b.neuron_count;
    int late = s.flags.plug.ticks_per_step - 15;
    for (int i = 0; i < count; i++) {
        size_t at = r.c20.size();
        r.c20.resize(at + (size_t)r.d, 0);
        r.c15.resize(at + (size_t)r.d, 0);
        uint8_t* c20 = &r.c20[at];
        uint8_t* c15 = &r.c15[at];
        for (int tick = 0;; tick++) {
            ticker_before_tick(tt, b);
            blob_tick(b);
            s.ticks++;
            for (int t = 0; t < b.tile_count; t++) {
                uint64_t spikes = b.spikes[b.now][t];
                while (spikes) {
                    int n = t * TILE_NEURONS + __builtin_ctzll(spikes);
                    spikes &= spikes - 1;
                    c20[n] = c20[n] < 255 ? c20[n] + 1 : c20[n];
                    c15[n] += tick >= late ? 1 : 0;
                }
            }
            if (ticker_after_tick(tt, b)) {
                break;
            }
        }
        r.label.push_back((uint8_t)tt.right_move);
        r.n++;
        tt.rest_left = 0;
    }
}

static void record_test(Sheet& s, int count, Record& r) {
    TaskTicker& tt = *s.tt;
    Task saved = tt.task;
    mnist_restart(tt.task, s.eval_test, s.eval_first, s.eval_count, 0, 0);
    record_digits(s, count, r);
    tt.task = saved;
}

static void find_cells(const Sheet& s, Cells& c) {
    const Blob& b = *s.blob;
    const Plug& plug = s.tt->plug;
    c = Cells{};
    c.buttons = plug.motor_lines;
    c.inhibit_gain = b.genome.inhibit_gain;
    for (int line = 0; line < plug.motor_lines; line++) {
        const int* cells = nullptr;
        int count = plug_motor_cells(plug, line, &cells);
        for (int k = 0; k < count; k++) {
            int n = cells[k];
            c.neuron.push_back(n);
            c.button.push_back(line);
            uint64_t self = 1ull << (n % TILE_NEURONS);
            for (int slot = 0; slot < SLOTS; slot++) {
                uint32_t src = slot_source(b, n, slot);
                bool none = src == (uint32_t)b.tile_count;
                uint64_t u = none ? 0 : ~0ull;
                u &= slot == 4 ? ~self : ~0ull;
                u &= slot >= LOCAL_SLOTS && !none ? ~b.inhibitory[src] : ~0ull;
                c.tile.push_back(none ? -1 : (int)src);
                c.usable.push_back(u);
                c.inhib.push_back(!none && slot < LOCAL_SLOTS ? b.inhibitory[src] & u : 0);
                for (int bit = 0; bit < TILE_NEURONS; bit++) {
                    c.born.push_back((int8_t)blob_weight(b, n, slot, bit));
                }
            }
            int have = count_fan_in(b, n);
            c.keep.push_back(have > 0 ? have : 1);
        }
    }
    c.count = (int)c.neuron.size();
}

// rung 6 and the fixes screened on it: form, gate, budget multiple, halve, strong, dale
struct Variant6 {
    const char* key;
    const char* label;
    Rung6 opt;
};
static const Variant6 variants6[] = {
    {"6c", "6c counters as graded weights, vote", {0, 0, 1, 0, 0, 0}},
    {"6b", "6b one-bit top counters (engine)", {1, 0, 1, 0, 0, 0}},
    {"6g", "6g one-bit top counters, close-vote gate", {1, 1, 1, 0, 0, 0}},
    {"6k2", "6k2 6b, twice the budget", {1, 0, 2, 0, 0, 0}},
    {"6k4", "6k4 6b, four times the budget", {1, 0, 4, 0, 0, 0}},
    {"6h", "6h 6b, a full counter halves the row", {1, 0, 1, 1, 0, 0}},
    {"6s", "6s 6b, top quarter strong (4)", {1, 0, 1, 0, 1, 0}},
    {"6d", "6d 6b, excitatory sources only", {1, 0, 1, 0, 0, 1}},
    {"6gh", "6gh gate and halving", {1, 1, 1, 1, 0, 0}},
    {"6gk2", "6gk2 gate and twice the budget", {1, 1, 2, 0, 0, 0}},
    {"6gs", "6gs gate and strong quarter", {1, 1, 1, 0, 1, 0}},
    {"6ghk2", "6ghk2 gate, halving, twice the budget", {1, 1, 2, 1, 0, 0}},
};

static bool wanted(const char* list, const char* rung) {
    if (!list) {
        return true;
    }
    size_t len = std::strlen(rung);
    for (const char* p = list; *p;) {
        const char* end = std::strchr(p, ',');
        size_t n = end ? (size_t)(end - p) : std::strlen(p);
        if (n == len && std::strncmp(p, rung, len) == 0) {
            return true;
        }
        p = end ? end + 1 : p + n;
    }
    return false;
}

int mnist_ladder(int argc, char** argv) {
    Sheet s;
    if (!sheet_start(s, argc, argv, ladder_own, (int)(sizeof ladder_own / sizeof *ladder_own))) {
        return 1;
    }
    int64_t life = int_flag(argc, argv, "--life", 0);
    int count = int_flag(argc, argv, "--record", 60000);
    int test_count = int_flag(argc, argv, "--test-count", 10000);
    int settle = int_flag(argc, argv, "--settle", 200);
    const char* rungs = text_flag(argc, argv, "--rungs", "1,2,3,4n,4u,4g,5s,5v,6c,6b,6g");
    double t0 = now_seconds();
    if (life > 0) {
        sheet_train(s, life, false, nullptr);
        std::printf("life: %lld digits with no reward, %.1f s\n", (long long)life,
                    now_seconds() - t0);
    }
    s.blob->genome.learning_on = 0; // frozen from here on
    sheet_train(s, settle, false, nullptr);
    Ladder L;
    L.seed = s.flags.seed;
    double a = now_seconds();
    record_digits(s, count, L.train);
    record_test(s, test_count, L.test);
    find_cells(s, L.cells);
    const Blob& b = *s.blob;
    for (int n = 0; n < b.neuron_count; n++) {
        if (!((b.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1)) {
            L.inner.push_back(n);
        }
    }
    int64_t fired = 0;
    for (int r = 0; r < L.train.n; r++) {
        for (int m = 0; m < L.cells.count; m++) {
            fired += L.train.c20[(size_t)r * L.train.d + L.cells.neuron[(size_t)m]] > 0;
        }
    }
    L.cells.fire_share = (double)fired / ((double)L.train.n * L.cells.count);
    for (int p = 1; p <= 2; p++) {
        L.later.emplace_back();
        shuffled(L.later.back(), L.train.n, L.seed * 7919 + (uint64_t)p);
    }
    std::printf("ladder  seed %llu  life %lld  recorded %d training and %d test digits in %.1f s"
                "  inner cells %zu  motor cells %d (each fires on %.3f of digits)\n",
                (unsigned long long)L.seed, (long long)life, L.train.n, L.test.n,
                now_seconds() - a, L.inner.size(), L.cells.count, L.cells.fire_share);
    std::printf("%-44s %7lld %7lld %7lld %7lld %7lld\n", "rung, reader", 1000LL, 5000LL, 20000LL,
                60000LL, 120000LL);
    std::fflush(stdout);
    if (wanted(rungs, "1")) {
        double r1 = rung1_offline(L);
        std::printf("%-44s %7.4f   (offline, fit on all %d)\n", "1  float softmax, offline", r1,
                    L.train.n);
        std::fflush(stdout);
    }
    Curve c;
    if (wanted(rungs, "2")) {
        rung2_online_float(L, c = Curve{});
        ladder_print("2  float softmax, online", c);
    }
    if (wanted(rungs, "3")) {
        rung3_integer_told(L, c = Curve{});
        ladder_print("3  integer, told the right button", c);
    }
    const char* r4[3] = {"4n integer, good/bad, nothing on good", "4u integer, good/bad, up on good",
                         "4g integer, good/bad, up on a close good"};
    for (int g = 0; g < 3; g++) {
        if (wanted(rungs, g == 0 ? "4n" : (g == 1 ? "4u" : "4g"))) {
            rung4_integer_good_bad(L, g, c = Curve{});
            ladder_print(r4[g], c);
        }
    }
    if (wanted(rungs, "5s")) {
        rung5_cells(L, 0, c = Curve{});
        ladder_print("5s motor cells' fields, sum of drives", c);
    }
    if (wanted(rungs, "5v")) {
        rung5_cells(L, 1, c = Curve{});
        ladder_print("5v motor cells' fields, vote of cells", c);
    }
    for (const Variant6& v : variants6) {
        if (wanted(rungs, v.key)) {
            rung6_counters(L, v.opt, c = Curve{});
            ladder_print(v.label, c);
        }
    }
    std::printf("total wall %.1f s\n", now_seconds() - t0);
    sheet_free(s);
    return 0;
}
