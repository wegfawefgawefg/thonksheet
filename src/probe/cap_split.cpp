// blobprobe capsplit --life "<blobrun flags>" [--seed N] [--g.name value ...]
//     [--cap-motor N] [--cap-hidden N] [--cap-memory N] [--hold-motor 1] [--hold-hidden 1]
//     [--look-steps N] [--trace N]
// idea 180 (docs/results-2026-11-26-cap-default.md): the threshold cap split by kind of cell,
// and what the output and the inside do step by step. Lives the life as blobrun does (put
// every genome number on the probe's own line; give --g.threshold_cap 1000 there and the caps
// here), and after every tick:
//   --cap-KIND N     thresholds of that kind are cut back to N. the walk only steps up by
//                    one, so this is exactly a cap of N for those cells. kinds: motor, memory
//                    (integrator and time cells), hidden (every other cell that walks)
//   --hold-KIND 1    thresholds of that kind are put back as born (no walk on them)
// Prints the window scores and the late score (blobrun's), a line per window (rate, each
// button's mean threshold), and over the last --look-steps steps (default 2,000):
//   - health by kind: cells, spikes per 1,024 cell-ticks, silent, stuck (over a quarter of
//     the ticks), mean and highest threshold, share at 64 or more
//   - the presses: for each right answer (the chain: A on stage 0, B on stage 1) how often
//     each button was pressed, and each button's mean spikes in the step
//   - with up to 4 right answers: how alike the hidden cells' spike counts are between the
//     first two (cosine), and how many hidden cells fire 4 times more on one than the other
//   - the chain: which buttons A and B are
//   --trace N: the last N steps one by one (right answer, press, verdict, each button's spikes)

#include "blob/blob.hpp"
#include "blob/long_memory.hpp"
#include "blob/regulate_rate.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);
bool parse_life(const char* text, const Genome& genome, uint64_t seed, RunFlags& flags);

enum CapKind { CAP_HIDDEN, CAP_MEMORY, CAP_MOTOR, CAP_KINDS };
static const char* const cap_names[CAP_KINDS] = {"hidden", "memory", "motor"};
constexpr int ROWS = 4, LINES = 16;

static bool has_bit(const uint64_t* mask, int n) {
    return mask && ((mask[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1);
}

// -1: a cell the walk leaves alone (sensor, pacemaker)
static int cap_kind(const Blob& blob, int n) {
    if (has_bit(blob.sensor, n) || has_bit(blob.pacemaker, n)) {
        return -1;
    }
    if (has_bit(blob.motor, n)) {
        return CAP_MOTOR;
    }
    const BlobLongMemory* lm = blob.long_memory;
    if (lm && (has_bit(lm->cell, n) || has_bit(lm->time_cell, n))) {
        return CAP_MEMORY;
    }
    return CAP_HIDDEN;
}

struct Look {
    int64_t ticks = 0, steps = 0;
    std::vector<uint32_t> count;                // per cell, spikes over the look
    std::vector<uint32_t> row[ROWS];            // per cell, spikes on steps with this right answer
    int64_t pressed[LINES + 1][LINES + 1] = {}; // [right + 1][move + 1]
    int64_t spikes[LINES + 1][LINES] = {};      // [right + 1][line], the step's vote
    int64_t row_steps[LINES + 1] = {};
    // the chain (idea 184): the stage-0 steps that open a chain (after a stage-1 step), by
    // what was pressed on them: A, B, another button, nothing; and the chains paid
    int64_t opened[4] = {};
    int64_t paid = 0;
};

static void count_tick(const Blob& blob, std::vector<uint32_t>& into) {
    for (int t = 0; t < blob.tile_count; t++) {
        for (uint64_t m = blob.spikes[blob.now][t]; m; m &= m - 1) {
            into[(size_t)t * TILE_NEURONS + __builtin_ctzll(m)]++;
        }
    }
}

static double button_threshold(const Blob& blob, const Plug& plug, int line) {
    const int* cells = nullptr;
    int n = plug_motor_cells(plug, line, &cells);
    double sum = 0;
    for (int i = 0; i < n; i++) {
        sum += blob.threshold[cells[i]];
    }
    return n ? sum / n : 0;
}

static void print_health(const Blob& blob, const std::vector<int>& kind, const Look& look) {
    for (int k = 0; k < CAP_KINDS; k++) {
        int cells = 0, silent = 0, stuck = 0, high = 0, top = 0;
        double fired = 0, thr = 0;
        for (int n = 0; n < blob.neuron_count; n++) {
            if (kind[(size_t)n] != k) {
                continue;
            }
            uint32_t s = look.count[(size_t)n];
            cells++, fired += s, thr += blob.threshold[n];
            silent += s == 0, stuck += (int64_t)s * 4 > look.ticks;
            high += blob.threshold[n] >= 64;
            top = blob.threshold[n] > top ? blob.threshold[n] : top;
        }
        if (!cells) {
            continue;
        }
        std::printf("kind %s: %d cells, rate %.2f per 1024 cell-ticks, silent %.4f, stuck %.4f, "
                    "threshold mean %.2f, highest %d, at 64 or more %.4f\n",
                    cap_names[k], cells, fired * 1024.0 / ((double)look.ticks * cells),
                    (double)silent / cells, (double)stuck / cells, thr / cells, top,
                    (double)high / cells);
    }
    double bias = 0, own = 0;
    int plus = 0, minus = 0, own_plus = 0;
    int own_max = blob.genome.motor_bias_max > 0 ? blob.genome.motor_bias_max
                                                 : blob.genome.sector_bias_max;
    for (int s = 0; s < blob.sector_count; s++) {
        bias += blob.sector_bias[s];
        plus += blob.sector_bias[s] >= blob.genome.sector_bias_max;
        minus += blob.sector_bias[s] <= -blob.genome.sector_bias_max;
        own += motor_apart_bias(blob, s);
        own_plus += motor_apart_bias(blob, s) >= own_max;
    }
    double sectors = blob.sector_count ? blob.sector_count : 1;
    std::printf("sector bias mean %.2f, at +max %.3f (target %d per 1024)\n", bias / sectors,
                plus / sectors, blob.genome.sector_target_per_1024);
    // idea 184: the other limit, and the motor cells' own bias (motor_apart 3; else 0)
    std::printf("sector bias at -max %.3f; motor bias mean %.2f, at +max %.3f\n",
                minus / sectors, own / sectors, own_plus / sectors);
}

static void print_presses(const Look& look, int lines) {
    for (int r = 0; r <= lines && r <= LINES; r++) {
        if (!look.row_steps[r]) {
            continue;
        }
        std::printf("right %d: %lld steps; pressed", r - 1, (long long)look.row_steps[r]);
        for (int m = 0; m <= lines && m <= LINES; m++) {
            std::printf(" %d:%.3f", m - 1, (double)look.pressed[r][m] / (double)look.row_steps[r]);
        }
        std::printf("; spikes");
        for (int l = 0; l < lines && l < LINES; l++) {
            std::printf(" %d:%.1f", l, (double)look.spikes[r][l] / (double)look.row_steps[r]);
        }
        std::printf("\n");
    }
}

// hidden cells' counts on the first two right answers that came up
static void print_inside(const Blob& blob, const std::vector<int>& kind, const Look& look) {
    int a = -1, b = -1;
    for (int r = 0; r < ROWS; r++) {
        if (look.row_steps[r + 1] && a < 0) {
            a = r;
        } else if (look.row_steps[r + 1] && b < 0) {
            b = r;
        }
    }
    if (a < 0 || b < 0) {
        return;
    }
    double na = (double)look.row_steps[a + 1], nb = (double)look.row_steps[b + 1];
    double dot = 0, aa = 0, bb = 0, sa = 0, sb = 0;
    int cells = 0, only = 0;
    for (int n = 0; n < blob.neuron_count; n++) {
        if (kind[(size_t)n] != CAP_HIDDEN) {
            continue;
        }
        double x = look.row[a][(size_t)n] / na, y = look.row[b][(size_t)n] / nb;
        dot += x * y, aa += x * x, bb += y * y, sa += x, sb += y;
        cells++;
        only += (x > 4 * y || y > 4 * x) && x + y > 0.05;
    }
    std::printf("inside: hidden spikes a step on right %d %.2f, on right %d %.2f; alike (cosine) "
                "%.3f; cells firing 4 times more on one than the other %.4f of %d\n",
                a, sa, b, sb, aa > 0 && bb > 0 ? dot / std::sqrt(aa * bb) : 0,
                cells ? (double)only / cells : 0, cells);
}

int probe_capsplit(int argc, char** argv) {
    uint64_t seed = (uint64_t)int_arg(argc, argv, "--seed", 5);
    Genome genome;
    apply_genome_args(argc, argv, genome);
    RunFlags flags;
    const char* life_text = text_arg(argc, argv, "--life", nullptr);
    if (!life_text || !parse_life(life_text, genome, seed, flags)) {
        std::fprintf(stderr, "blobprobe capsplit: --life \"<blobrun flags>\" needed\n");
        return 1;
    }
    int cap[CAP_KINDS] = {int_arg(argc, argv, "--cap-hidden", 0),
                          int_arg(argc, argv, "--cap-memory", 0),
                          int_arg(argc, argv, "--cap-motor", 0)};
    int hold[CAP_KINDS] = {int_arg(argc, argv, "--hold-hidden", 0), 0,
                           int_arg(argc, argv, "--hold-motor", 0)};
    int64_t look_steps = int_arg(argc, argv, "--look-steps", 2000);
    int64_t trace = int_arg(argc, argv, "--trace", 0);

    Blob* blob = blob_birth(flags.genome, seed);
    std::vector<int> kind((size_t)blob->neuron_count);
    std::vector<int16_t> born((size_t)blob->neuron_count);
    bool edits = false;
    for (int n = 0; n < blob->neuron_count; n++) {
        kind[(size_t)n] = cap_kind(*blob, n);
        born[(size_t)n] = blob->threshold[n];
    }
    for (int k = 0; k < CAP_KINDS; k++) {
        edits = edits || cap[k] > 0 || hold[k];
    }

    static TaskTicker tt;
    tt = TaskTicker{};
    if (!ticker_start(tt, *blob, flags)) {
        std::fprintf(stderr, "blobprobe capsplit: the plug does not fit\n");
        return 1;
    }
    int lines = tt.plug.motor_lines;
    Look look;
    look.count.assign((size_t)blob->neuron_count, 0);
    for (int r = 0; r < ROWS; r++) {
        look.row[r].assign((size_t)blob->neuron_count, 0);
    }
    std::vector<uint32_t> step_count((size_t)blob->neuron_count, 0);
    std::vector<double> scores;
    int64_t look_from = flags.steps > look_steps ? flags.steps - look_steps : 0;
    int64_t window_fired = 0, window_ticks = 0;
    int inner = 0;
    for (int n = 0; n < blob->neuron_count; n++) {
        inner += !has_bit(blob->sensor, n);
    }

    // the life, as blobrun's loop (run/life.cpp)
    int64_t step = 0;
    int last_right = -2; // the step before's right answer (the chain: which stage it was)
    while (step < flags.steps) {
        ticker_before_tick(tt, *blob);
        blob_tick(*blob);
        if (edits) {
            for (int n = 0; n < blob->neuron_count; n++) {
                int k = kind[(size_t)n];
                if (k < 0) {
                    continue;
                }
                if (hold[k]) {
                    blob->threshold[n] = born[(size_t)n];
                } else if (cap[k] > 0 && blob->threshold[n] > cap[k]) {
                    blob->threshold[n] = (int16_t)cap[k];
                }
            }
        }
        window_ticks++;
        for (int t = 0; t < blob->tile_count; t++) {
            window_fired += __builtin_popcountll(blob->spikes[blob->now][t] & ~blob->sensor[t]);
        }
        bool looking = step >= look_from;
        if (looking) {
            look.ticks++;
            count_tick(*blob, look.count);
            count_tick(*blob, step_count);
        }
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        // with a critic the blob hears every step, valence 0 included (as run/life.cpp)
        if (tt.valence != 0 || flags.genome.critic_on) {
            if (tt.plug.shape.pattern_bits > 0) {
                plug_mark_pattern(tt.plug, *blob, tt.in.pattern, tt.in.pattern_tied,
                                  tt.task.pattern_graded);
            } else {
                plug_mark_acted(tt.plug, *blob, tt.move);
                plug_mark_vote(tt.plug, *blob, tt.move);
            }
            plug_hop_lines(tt.plug, *blob, tt.move, tt.right_move, tt.in.pattern,
                           tt.in.pattern_tied, tt.task.pattern_graded);
            blob_reward(*blob, tt.valence);
        }
        if (looking && tt.task.kind == TaskKind::chain) {
            int a = tt.task.button_of[0], b = tt.task.button_of[1];
            if (tt.right_move == a && last_right != a) {
                look.opened[tt.move == a ? 0 : tt.move == b ? 1 : tt.move >= 0 ? 2 : 3]++;
            }
            look.paid += tt.valence > 0;
        }
        last_right = tt.right_move;
        if (looking) {
            int r = tt.right_move + 1, m = tt.move + 1;
            if (r >= 0 && r <= LINES && m >= 0 && m <= LINES) {
                look.pressed[r][m]++;
                look.row_steps[r]++;
                for (int l = 0; l < lines && l < LINES; l++) {
                    look.spikes[r][l] += tt.plug.last_count[l];
                }
            }
            if (tt.right_move >= 0 && tt.right_move < ROWS) {
                for (size_t n = 0; n < step_count.size(); n++) {
                    look.row[tt.right_move][n] += step_count[n];
                }
            }
            step_count.assign(step_count.size(), 0);
            look.steps++;
            if (flags.steps - step <= trace) {
                std::printf("step %lld right %d pressed %d verdict %d spikes", (long long)step,
                            tt.right_move, tt.move, tt.valence);
                for (int l = 0; l < lines && l < LINES; l++) {
                    std::printf(" %d", tt.plug.last_count[l]);
                }
                std::printf("\n");
            }
        }
        step++;
        if (step % flags.window == 0) {
            scores.push_back(task_window_score(tt.task));
            std::printf("window %zu score %.6f rate %.2f button thresholds", scores.size(),
                        scores.back(),
                        (double)window_fired * 1024.0 / ((double)window_ticks * inner));
            for (int l = 0; l < lines && l < 4; l++) {
                std::printf(" %.1f", button_threshold(*blob, tt.plug, l));
            }
            std::printf("\n");
            window_fired = window_ticks = 0;
        }
    }
    size_t edge = scores.size() / 10 > 0 ? scores.size() / 10 : 1;
    double late = 0;
    for (size_t i = scores.size() - edge; i < scores.size(); i++) {
        late += scores[i];
    }
    std::printf("life: late %.6f over %zu windows; caps hidden %d memory %d motor %d (genome %d), "
                "hold hidden %d motor %d\n",
                scores.empty() ? 0 : late / (double)edge, scores.size(), cap[CAP_HIDDEN],
                cap[CAP_MEMORY], cap[CAP_MOTOR], blob->genome.threshold_cap, hold[CAP_HIDDEN],
                hold[CAP_MOTOR]);
    double all = 0;
    for (int n = 0; n < blob->neuron_count; n++) {
        all += has_bit(blob->sensor, n) ? 0 : look.count[(size_t)n];
    }
    std::printf("look: %lld steps, rate %.2f per 1024 cell-ticks (not sensor cells)\n",
                (long long)look.steps, all * 1024.0 / ((double)look.ticks * inner));
    print_health(*blob, kind, look);
    for (int l = 0; l < lines && l < LINES; l++) {
        const int* cells = nullptr;
        int n = plug_motor_cells(tt.plug, l, &cells), high = 0;
        for (int i = 0; i < n; i++) {
            high += blob->threshold[cells[i]] >= 64;
        }
        std::printf("button %d: %d cells, threshold mean %.2f, at 64 or more %.3f\n", l, n,
                    button_threshold(*blob, tt.plug, l), n ? (double)high / n : 0);
    }
    if (tt.task.kind == TaskKind::chain) {
        std::printf("chain: A %d B %d\n", tt.task.button_of[0], tt.task.button_of[1]);
        int64_t n = look.opened[0] + look.opened[1] + look.opened[2] + look.opened[3];
        double d = n ? (double)n : 1;
        std::printf("chain opened %lld times: A at once %.4f, B %.4f, another button %.4f, "
                    "nothing %.4f; chains paid x 2 / steps %.4f\n",
                    (long long)n, look.opened[0] / d, look.opened[1] / d, look.opened[2] / d,
                    look.opened[3] / d, look.steps ? 2.0 * look.paid / look.steps : 0);
    }
    print_presses(look, lines);
    print_inside(*blob, kind, look);
    plug_detach(tt.plug, *blob);
    blob_free(blob);
    return 0;
}
