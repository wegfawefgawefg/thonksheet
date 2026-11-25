// tableprobe: idea 57, why noise-graded learning on the table sheet lands on the wrong
// answer. one table life on a reflex task, with the noise-graded reward rule run here (a copy
// of table_reward's noise modes, byte-identical to it when no --p. switch is set), and every
// graded flip counted: where it was, which way noise flipped it, whether it changed the
// answer, and which way its counter moved. docs/results-2026-11-24-noise-sign.md.
//
//   tableprobe --task echo --k 2 --t.inputs 4 --t.bad_rise_shift 0 --t.reward_shift 0
//       --t.learn_mode 1 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000
//   --p.detail N      print the first N rewards in full
//   --p.motor_only 1  grade only flips on the motor cells the plug reads
//   --p.causal_only 1 grade only those flips that changed the answer (an oracle, not a rule)
//   --p.cue_baseline 1  a running average of reward per cue (an oracle, not a rule)
//   --p.pair 1        also grade unflipped neurons at the noise chance (as --t.learn_mode 3)
//   --p.classes M     grade flips only in these classes (bits: 1 read motor right group,
//                     2 read motor wrong group, 4 motor not read, 8 inner; default 15)
//   --p.ways W        grade only flips that made a neuron fire (1), silenced it (2), both (3)
//   --p.flips 0       grade no flipped neuron; with --p.pair 1 only the unflipped sample is
//                     graded: the plain rule on a random sample (mode 1 sign) or its opposite
//                     (mode 2 sign), with no noise in it at all

#include "blob/random_words.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"
#include "table/table.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

struct ProbeSwitches {
    int detail = 0;
    int motor_only = 0;
    int causal_only = 0;
    int cue_baseline = 0;
    int pair = 0;
    int classes = 15; // bit per class graded: read right, read wrong, motor unread, inner
    int ways = 3;     // 1 flips that made a neuron fire, 2 flips that silenced one, 3 both
    int flips = 1;    // 0: flipped neurons are not graded at all (with pair: only the sample)
};

// classes of graded neuron
enum { C_RIGHT, C_WRONG, C_MOTOR_UNREAD, C_INNER, C_COUNT };
static const char* const class_names[C_COUNT] = {
    "read motor, right group", "read motor, wrong group", "motor, not read", "inner"};

// counts over a span of steps
struct Tally {
    int64_t steps = 0, good = 0, bad = 0, none = 0;
    int64_t press[4][TASK_MAX_MOTORS + 1] = {}; // [cue][button or none]
    int64_t graded[C_COUNT][2] = {};            // [class][0 noise made it fire, 1 silenced]
    int64_t moved[C_COUNT][2][2] = {}; // [class][good event?][0 toward flip, 1 toward table]
    // read motor cells only: [causal?][useful?]. useful: up for the right group, down for the
    // wrong one
    int64_t motor_moves[2][2] = {};
    int64_t causal_graded = 0, motor_graded = 0;
    int64_t events_acted[2] = {}; // [good?]
};

struct Probe {
    ProbeSwitches sw;
    int k = 2;
    int group = 9;
    int count[TASK_MAX_MOTORS] = {}; // motor spikes this step, as the plug counts them
    // which read group a neuron is in, -1 if none
    std::vector<int8_t> group_of;
    int32_t cue_average[TASK_MAX_MOTORS] = {};
    Tally span, life;
    int64_t rewards = 0;
};

// the plug's vote: the largest count, a tie or silence is no move
static int vote(const int* c, int k) {
    int best = -1, best_count = 0;
    bool tie = false;
    for (int line = 0; line < k; line++) {
        if (c[line] > best_count) {
            best = line;
            best_count = c[line];
            tie = false;
        } else if (c[line] == best_count && c[line] > 0) {
            tie = true;
        }
    }
    return tie ? -1 : best;
}

// would the answer have differed had noise not flipped this read motor cell on that tick
static bool flip_changed_answer(const Probe& p, int g, bool did, int answer) {
    int c[TASK_MAX_MOTORS];
    std::memcpy(c, p.count, sizeof(c));
    c[g] += did ? -1 : +1;
    return vote(c, p.k) != answer;
}

// table_reward's noise modes (1, 2, 3), with the probe's switches and counts. with no switch
// set it draws the same random words in the same order, so the life is the same.
static void probe_reward(Probe& p, TableBlob& tb, const TaskTicker& tt) {
    const TableGenome& tg = tb.tg;
    Blob& b = tb.shell;
    int valence = tt.valence;
    int cue = tt.cue, right = tt.right_move, answer = tt.move;
    bool good_event = valence > 0;
    int32_t& average = p.sw.cue_baseline ? p.cue_average[cue] : tb.reward_average;
    int32_t before = average;
    int32_t delta = valence * 256 - average;
    average += delta >> tg.average_shift;
    if (p.sw.cue_baseline) {
        tb.reward_average += (valence * 256 - tb.reward_average) >> tg.average_shift;
    }
    bool acted = false;
    bool toward = delta > 0;
    if (delta != 0) {
        toward = tg.learn_mode == 2 ? !toward : toward;
        int32_t size = delta > 0 ? delta : -delta;
        acted = !(size < 256 && (int32_t)(rng_next(tb.rng) & 255) >= size);
    }
    bool detail = p.rewards < p.sw.detail;
    if (detail) {
        std::printf("reward %lld: cue %d right %d answer %d valence %+d average %+.2f delta %+.2f"
                    " %s  counts",
                    (long long)p.rewards, cue, right, answer, valence, before / 256.0,
                    delta / 256.0, acted ? "acts" : "skipped");
        for (int i = 0; i < p.k; i++) {
            std::printf(" %d", p.count[i]);
        }
        std::printf("\n");
    }
    p.rewards++;
    if (!acted) {
        return;
    }
    p.span.events_acted[good_event]++;
    int rise_shift = toward ? tg.rise_extra_shift : tg.rise_extra_shift + tg.bad_rise_shift;
    for (int age = 0; age < tg.recent_ticks; age++) {
        int r = (b.now - age) & (HISTORY - 1);
        const uint64_t* map = b.spikes[r];
        const uint64_t* flips = tb.flips[r];
        const uint8_t* address =
            &tb.address[(int64_t)(r & (TABLE_ADDR_HISTORY - 1)) * b.neuron_count];
        for (int t = 0; t < b.tile_count; t++) {
            uint64_t graded = ~b.sensor[t] & sparse_word(tb.rng, tg.reward_shift) & flips[t];
            if (p.sw.pair || tg.learn_mode == 3) {
                graded |= ~b.sensor[t] & ~flips[t] &
                          sparse_word(tb.rng, tg.noise_shift + tg.reward_shift);
            }
            uint64_t may_rise = graded ? sparse_word(tb.rng, rise_shift) : 0;
            while (graded) {
                int bit = __builtin_ctzll(graded);
                graded &= graded - 1;
                int n = t * TILE_NEURONS + bit;
                bool did = (map[t] >> bit) & 1;
                bool flipped = (flips[t] >> bit) & 1;
                int g = p.group_of[n];
                int cls = g < 0 ? ((b.motor[t] >> bit) & 1 ? C_MOTOR_UNREAD : C_INNER)
                                : (g == right ? C_RIGHT : C_WRONG);
                bool causal = g >= 0 && flipped && flip_changed_answer(p, g, did, answer);
                if (p.sw.motor_only && g < 0) {
                    continue;
                }
                if (p.sw.causal_only && !causal) {
                    continue;
                }
                if (flipped && !p.sw.flips) {
                    continue;
                }
                if (flipped && (!((p.sw.classes >> cls) & 1) || !((p.sw.ways >> !did) & 1))) {
                    continue;
                }
                if (flipped) {
                    p.span.graded[cls][did ? 0 : 1]++;
                    p.span.motor_graded += g >= 0;
                    p.span.causal_graded += causal;
                }
                int entry = address[n];
                bool fire = toward ? did : !did;
                if (fire && !((may_rise >> bit) & 1)) {
                    continue;
                }
                bool moved = table_step(tb, n, entry, fire ? +1 : -1);
                tb.stats.learned += moved;
                if (!moved || !flipped) {
                    continue;
                }
                // toward the flip: the counter moves toward what noise made it do
                bool toward_flip = fire == did;
                p.span.moved[cls][good_event][toward_flip ? 0 : 1]++;
                if (g >= 0) {
                    bool useful = fire == (g == right);
                    p.span.motor_moves[causal][useful]++;
                }
                if (detail && g >= 0) {
                    std::printf("   group %d (%s) %s by noise at age %d, %s, counter %s\n", g,
                                g == right ? "right" : "wrong", did ? "fired" : "silenced", age,
                                causal ? "changed the answer" : "answer unchanged",
                                fire ? "up" : "down");
                }
            }
        }
    }
}

static void add_tally(Tally& to, const Tally& from) {
    const int64_t* f = (const int64_t*)&from;
    int64_t* t = (int64_t*)&to;
    for (size_t i = 0; i < sizeof(Tally) / sizeof(int64_t); i++) {
        t[i] += f[i];
    }
}

static void print_tally(const Tally& s, int k, const char* title) {
    std::printf("%s: %lld steps, good %lld bad %lld no move %lld\n", title, (long long)s.steps,
                (long long)s.good, (long long)s.bad, (long long)s.none);
    for (int c = 0; c < k; c++) {
        int64_t all = 0;
        for (int m = 0; m <= k; m++) {
            all += s.press[c][m];
        }
        std::printf("  cue %d presses:", c);
        for (int m = 0; m < k; m++) {
            std::printf(" b%d %.2f", m, all ? (double)s.press[c][m] / all : 0.0);
        }
        std::printf(" none %.2f\n", all ? (double)s.press[c][k] / all : 0.0);
    }
    std::printf("  rewards that acted: good %lld bad %lld\n", (long long)s.events_acted[1],
                (long long)s.events_acted[0]);
    std::printf("  graded flips by class (fired by noise / silenced by noise), counters moved on"
                " good (toward flip / toward table), on bad (toward flip / toward table):\n");
    for (int c = 0; c < C_COUNT; c++) {
        std::printf("    %-24s %9lld / %8lld   good %8lld / %8lld   bad %8lld / %8lld\n",
                    class_names[c], (long long)s.graded[c][0], (long long)s.graded[c][1],
                    (long long)s.moved[c][1][0], (long long)s.moved[c][1][1],
                    (long long)s.moved[c][0][0], (long long)s.moved[c][0][1]);
    }
    std::printf("  read motor flips graded %lld, of them changed the answer %lld\n",
                (long long)s.motor_graded, (long long)s.causal_graded);
    std::printf("  read motor counter moves: answer unchanged useful %lld harmful %lld;"
                " answer changed useful %lld harmful %lld\n",
                (long long)s.motor_moves[0][1], (long long)s.motor_moves[0][0],
                (long long)s.motor_moves[1][1], (long long)s.motor_moves[1][0]);
}

// pull the --p.name value switches out of argv
static int take_switches(int argc, char** argv, ProbeSwitches& sw) {
    int out = 1;
    for (int i = 1; i < argc; i++) {
        if (std::strncmp(argv[i], "--p.", 4) == 0 && i + 1 < argc) {
            const char* name = argv[i] + 4;
            int v = std::atoi(argv[++i]);
            if (!std::strcmp(name, "detail")) {
                sw.detail = v;
            } else if (!std::strcmp(name, "motor_only")) {
                sw.motor_only = v;
            } else if (!std::strcmp(name, "causal_only")) {
                sw.causal_only = v;
            } else if (!std::strcmp(name, "cue_baseline")) {
                sw.cue_baseline = v;
            } else if (!std::strcmp(name, "pair")) {
                sw.pair = v;
            } else if (!std::strcmp(name, "flips")) {
                sw.flips = v;
            } else if (!std::strcmp(name, "classes")) {
                sw.classes = v;
            } else if (!std::strcmp(name, "ways")) {
                sw.ways = v;
            } else {
                std::fprintf(stderr, "tableprobe: no switch %s\n", argv[i - 1]);
                std::exit(1);
            }
            continue;
        }
        argv[out++] = argv[i];
    }
    return out;
}

int main(int argc, char** argv) {
    Probe p;
    argc = take_switches(argc, argv, p.sw);
    RunFlags flags;
    apply_genome_args(argc, argv, flags.genome);
    TableGenome tg;
    table_genome_args(argc, argv, tg);
    if (!run_flags_parse(argc, argv, flags)) {
        return 1;
    }
    if (tg.learn_mode < 1 || tg.learn_mode > 3) {
        std::fprintf(stderr, "tableprobe: --t.learn_mode 1, 2 or 3\n");
        return 1;
    }
    Blob* blob = table_birth(flags.genome, tg, flags.seed);
    TableBlob& tb = table_of(*blob);
    TaskTicker tt;
    if (!ticker_start(tt, *blob, flags)) {
        return 1;
    }
    p.k = tt.plug.motor_lines;
    p.group = plug_motor_group(tt.plug);
    p.group_of.assign((size_t)blob->neuron_count, -1);
    for (int line = 0; line < p.k; line++) {
        for (int q = 0; q < p.group; q++) {
            p.group_of[tt.plug.motor_neuron[line * PLUG_MAX_GROUP + q]] = (int8_t)line;
        }
    }
    std::printf("task %s k %d seed %llu, button of cue:", task_name(flags.task), p.k,
                (unsigned long long)flags.seed);
    for (int c = 0; c < p.k; c++) {
        std::printf(" %d", tt.task.button_of[c]);
    }
    std::printf("\n");

    int64_t step = 0;
    while (step < flags.steps) {
        ticker_before_tick(tt, *blob);
        table_tick(*blob);
        for (int line = 0; line < p.k; line++) {
            for (int q = 0; q < p.group; q++) {
                p.count[line] += blob_fired(*blob, tt.plug.motor_neuron[line * PLUG_MAX_GROUP + q]);
            }
        }
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        Tally& s = p.span;
        s.steps++;
        s.good += tt.valence > 0;
        s.bad += tt.valence < 0;
        s.none += tt.valence == 0;
        if (tt.cue < 4) {
            s.press[tt.cue][tt.move < 0 ? p.k : tt.move]++;
        }
        if (tt.valence != 0 && tg.learning_on) {
            probe_reward(p, tb, tt);
        }
        std::memset(p.count, 0, sizeof(p.count));
        step++;
        if (step % flags.window == 0) {
            double score = task_window_score(tt.task);
            if (step % (flags.window * 10) == 0 || step <= flags.window * 10) {
                std::printf("\n== steps to %lld, window score %.3f, average %+.2f\n",
                            (long long)step, score, tb.reward_average / 256.0);
                print_tally(s, p.k, "last window");
            } else {
                std::printf("steps to %lld, window score %.3f\n", (long long)step, score);
            }
            add_tally(p.life, s);
            p.span = Tally{};
        }
    }
    std::printf("\n");
    print_tally(p.life, p.k, "whole life");
    table_free(blob);
    return 0;
}
