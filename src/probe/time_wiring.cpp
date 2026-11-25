// blobprobe timewire --life "<blobrun flags>" [--born 1] [--trials N] [--top N] [--carry D]
//     [--out PATH] [--g.name value ...]
// idea 166, point 4 (docs/results-2026-11-25-timerule.md): what the cells wired. Lives the
// life as blobprobe transfer does (--born 1: none; --life-scramble, --life-quiet as there),
// then stops every rule (learning_on 0) and shows delayed echo again from a fresh task seed,
// counting each cell's spikes per step. From those counts:
//   - a fresh ridge reader of the cue from the hidden cells alone, at each step of the trial
//     (the cue step, each blank, the ask): how long the cue stays readable
//   - each hidden cell's d' for the cue at each step, and over the blanks together ("carry")
//   - the reader at the ask step leans on some cells (|weight| on standardised counts); for
//     its top N, and for every hidden cell, where their connections come from: the cue line
//     the cell prefers (+) or the other (-), the ask line, other lines (good, bad), motor
//     cells, hidden cells that carry the same cue through the blanks (|d'| >= D, same sign),
//     the other cue, neither; inhibitory cells apart. With the mean slot delay of each.
// --out writes one row per hidden cell, so a script can follow the same cells across sheets
// (born, lived, churn).

#include "blob/blob.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"
#include "probe/linear_fit.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);
bool parse_life(const char* text, const Genome& genome, uint64_t seed, RunFlags& flags);
Blob* probe_sheet(int argc, char** argv, const Genome& genome, uint64_t seed);

// where a connection comes from, seen from the cell it feeds
enum Source {
    CUE_SAME,
    CUE_OTHER,
    ASK,
    LINE_OTHER,
    MOTOR,
    CARRY_SAME,
    CARRY_OTHER,
    HIDDEN,
    INHIBIT,
    SOURCES
};
static const char* const source_names[SOURCES] = {"cue+",   "cue-",   "ask",    "line", "motor",
                                                  "carry+", "carry-", "hidden", "inhib"};

struct Counts {
    int phases = 0;              // steps per trial: cue, blanks, ask
    std::vector<int> cue;        // per trial
    std::vector<uint8_t> spikes; // trial x phase x neuron
};

// delayed echo with every rule stopped, from a fresh task seed; spikes per cell per step
static void record(Blob& blob, const RunFlags& life, int trials, Counts& c) {
    RunFlags f = life;
    f.task_seed = life.seed + 200;
    static TaskTicker tt;
    tt = TaskTicker{};
    if (!ticker_start(tt, blob, f)) {
        std::fprintf(stderr, "blobprobe timewire: the plug does not fit\n");
        std::exit(1);
    }
    int n = blob.neuron_count;
    c.phases = f.task_settings.delay + 2;
    c.spikes.assign((size_t)trials * c.phases * n, 0);
    c.cue.assign((size_t)trials, 0);
    std::vector<int> step(n);
    int trial = -1, phase = 0;
    while (true) {
        ticker_before_tick(tt, blob);
        if (tt.tick_in_step == 0) {
            phase = tt.task.phase;
            if (phase == 0) {
                trial++;
                if (trial == trials) {
                    break;
                }
                c.cue[(size_t)trial] = tt.task.cue;
            }
        }
        blob_tick(blob);
        const uint64_t* fired = blob.spikes[blob.now];
        for (int t = 0; t < blob.tile_count; t++) {
            for (uint64_t m = fired[t]; m; m &= m - 1) {
                step[t * TILE_NEURONS + __builtin_ctzll(m)]++;
            }
        }
        if (!ticker_after_tick(tt, blob)) {
            continue;
        }
        if (trial >= 0 && phase < c.phases) {
            uint8_t* row = &c.spikes[((size_t)trial * c.phases + phase) * n];
            for (int i = 0; i < n; i++) {
                row[i] = (uint8_t)(step[i] > 255 ? 255 : step[i]);
            }
        }
        for (int& s : step) {
            s = 0;
        }
    }
    plug_detach(tt.plug, blob);
}

static double d_prime(const std::vector<double>& x, const std::vector<int>& cue) {
    double s[2] = {0, 0}, q[2] = {0, 0}, k[2] = {0, 0};
    for (size_t r = 0; r < x.size(); r++) {
        int a = cue[r] != 0;
        s[a] += x[r], q[a] += x[r] * x[r], k[a] += 1;
    }
    if (k[0] < 2 || k[1] < 2) {
        return 0;
    }
    double m0 = s[0] / k[0], m1 = s[1] / k[1];
    double v = 0.5 * (q[0] / k[0] - m0 * m0 + q[1] / k[1] - m1 * m1);
    return (m1 - m0) / std::sqrt(v + 1e-3);
}

// a ridge reader of cue from the given cells' counts at one phase (dual form): fit on the
// first 60% of trials, the ridge picked on the next 15%, share right on the last 25%. the
// weights on standardised counts go to `w` (one per cell)
static double ridge_read(const Counts& c, const std::vector<int>& cells, int phase, int n,
                         std::vector<double>* w) {
    int trials = (int)c.cue.size(), d = (int)cells.size();
    int fit = trials * 60 / 100, check = trials * 75 / 100;
    std::vector<double> z((size_t)trials * d), mu((size_t)d), sd((size_t)d);
    for (int j = 0; j < d; j++) {
        double s = 0, q = 0;
        for (int r = 0; r < fit; r++) {
            double v = c.spikes[((size_t)r * c.phases + phase) * n + cells[(size_t)j]];
            s += v, q += v * v;
        }
        mu[(size_t)j] = s / fit;
        sd[(size_t)j] = std::sqrt(q / fit - mu[(size_t)j] * mu[(size_t)j]) + 1e-3;
    }
    for (int r = 0; r < trials; r++) {
        for (int j = 0; j < d; j++) {
            double v = c.spikes[((size_t)r * c.phases + phase) * n + cells[(size_t)j]];
            z[(size_t)r * d + j] = (v - mu[(size_t)j]) / sd[(size_t)j];
        }
    }
    std::vector<double> k((size_t)fit * fit), y((size_t)fit);
    double trace = 0;
    for (int a = 0; a < fit; a++) {
        y[(size_t)a] = c.cue[(size_t)a] ? 1.0 : -1.0;
        for (int b = 0; b <= a; b++) {
            double s = 0;
            for (int j = 0; j < d; j++) {
                s += z[(size_t)a * d + j] * z[(size_t)b * d + j];
            }
            k[(size_t)a * fit + b] = k[(size_t)b * fit + a] = s;
        }
        trace += k[(size_t)a * fit + a];
    }
    double best = -1, best_test = 0;
    for (double scale : {0.01, 0.1, 1.0, 10.0}) {
        std::vector<double> alpha;
        if (!linear_ridge_solve(k, fit, scale * trace / fit, y.data(), 1, alpha)) {
            continue;
        }
        std::vector<double> wj((size_t)d, 0.0);
        for (int a = 0; a < fit; a++) {
            for (int j = 0; j < d; j++) {
                wj[(size_t)j] += alpha[(size_t)a] * z[(size_t)a * d + j];
            }
        }
        int right[2] = {0, 0};
        for (int r = fit; r < trials; r++) {
            double s = 0;
            for (int j = 0; j < d; j++) {
                s += wj[(size_t)j] * z[(size_t)r * d + j];
            }
            right[r >= check] += (s > 0) == (c.cue[(size_t)r] != 0);
        }
        double on_check = (double)right[0] / (check - fit);
        if (on_check > best) {
            best = on_check;
            best_test = (double)right[1] / (trials - check);
            if (w) {
                *w = wj;
            }
        }
    }
    return best_test;
}

struct CellWiring {
    double count[SOURCES] = {};
    double delay[SOURCES] = {};
};

// where cell n's connections come from; `pref` the cue it prefers (+1 cue 1, -1 cue 0)
static CellWiring wiring_of(const Blob& blob, const Plug& plug, int n, int pref,
                            const std::vector<int>& line_of, const std::vector<double>& carry,
                            double carry_min) {
    CellWiring cw;
    int k_cues = plug.motor_lines;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        int64_t w = blob_weight_index(n, k);
        int delay = blob.slot_delay[w];
        for (uint64_t m = connected_bits(blob.w0[w], blob.w1[w]); m; m &= m - 1) {
            int b = __builtin_ctzll(m);
            int s = (int)src * TILE_NEURONS + b;
            int kind = HIDDEN;
            int line = line_of[(size_t)s];
            if ((blob.inhibitory[src] >> b) & 1) {
                kind = INHIBIT;
            } else if (line >= plug.first_single && line < plug.first_single + k_cues) {
                int cue_sign = line - plug.first_single ? 1 : -1;
                kind = cue_sign == pref ? CUE_SAME : CUE_OTHER;
            } else if (line == plug.first_single + k_cues) {
                kind = ASK;
            } else if (line >= 0 || ((blob.sensor[src] >> b) & 1)) {
                kind = LINE_OTHER;
            } else if ((blob.motor[src] >> b) & 1) {
                kind = MOTOR;
            } else if (std::fabs(carry[(size_t)s]) >= carry_min) {
                kind = (carry[(size_t)s] > 0 ? 1 : -1) == pref ? CARRY_SAME : CARRY_OTHER;
            }
            cw.count[kind] += 1;
            cw.delay[kind] += delay;
        }
    }
    return cw;
}

static void print_wiring(const char* label, const std::vector<CellWiring>& cells) {
    CellWiring sum;
    for (const CellWiring& c : cells) {
        for (int k = 0; k < SOURCES; k++) {
            sum.count[k] += c.count[k];
            sum.delay[k] += c.delay[k];
        }
    }
    double n = cells.empty() ? 1 : (double)cells.size();
    std::printf("%-12s connections per cell:", label);
    for (int k = 0; k < SOURCES; k++) {
        std::printf(" %s %.2f", source_names[k], sum.count[k] / n);
    }
    std::printf("\n%-12s mean slot delay:     ", label);
    for (int k = 0; k < SOURCES; k++) {
        std::printf(" %s %.2f", source_names[k],
                    sum.count[k] > 0 ? sum.delay[k] / sum.count[k] : 0);
    }
    std::printf("\n");
}

int probe_timewire(int argc, char** argv) {
    Genome genome;
    apply_genome_args(argc, argv, genome);
    uint64_t seed = (uint64_t)int_arg(argc, argv, "--seed", 1);
    const char* life_text = text_arg(argc, argv, "--life", nullptr);
    RunFlags life;
    if (!life_text || !parse_life(life_text, genome, seed, life) ||
        life.task != TaskKind::delayed_echo || life.task_settings.k != 2) {
        std::fprintf(stderr, "blobprobe timewire: --life \"<blobrun flags>\" of delayed echo, "
                             "k 2, is needed\n");
        return 1;
    }
    int trials = int_arg(argc, argv, "--trials", 2000);
    int top = int_arg(argc, argv, "--top", 64);
    double carry_min = int_arg(argc, argv, "--carry", 50) / 100.0;
    Blob* blob = probe_sheet(argc, argv, life.genome, seed);
    blob->genome.learning_on = 0;
    Counts c;
    record(*blob, life, trials, c);
    int n = blob->neuron_count;

    // the hidden cells, and which line each sensor cell carries
    std::vector<int> hidden, line_of((size_t)n, -1);
    for (int i = 0; i < n; i++) {
        int t = i / TILE_NEURONS, b = i % TILE_NEURONS;
        uint64_t not_hidden = blob->sensor[t] | blob->motor[t] | blob->pacemaker[t];
        if (!((not_hidden >> b) & 1)) {
            hidden.push_back(i);
        }
    }
    static TaskTicker tt;
    tt = TaskTicker{};
    ticker_start(tt, *blob, life);
    const Plug& plug = tt.plug;
    int group = plug_sensor_group(plug);
    for (int line = 0; line < plug.sensor_lines; line++) {
        for (int k = 0; plug.attached[line] && k < group; k++) {
            line_of[(size_t)plug.sensor_neuron[line * PLUG_MAX_GROUP + k]] = line;
        }
    }

    // d' per cell per phase, and over the blanks together
    std::vector<std::vector<double>> dp((size_t)c.phases, std::vector<double>((size_t)n, 0.0));
    std::vector<double> carry((size_t)n, 0.0), x(c.cue.size());
    for (int i : hidden) {
        for (int p = 0; p < c.phases; p++) {
            for (size_t r = 0; r < c.cue.size(); r++) {
                x[r] = c.spikes[(r * c.phases + p) * n + i];
            }
            dp[(size_t)p][(size_t)i] = d_prime(x, c.cue);
        }
        for (size_t r = 0; r < c.cue.size(); r++) {
            double s = 0;
            for (int p = 1; p + 1 < c.phases; p++) {
                s += c.spikes[(r * c.phases + p) * n + i];
            }
            x[r] = s;
        }
        carry[(size_t)i] = c.phases > 2 ? d_prime(x, c.cue) : 0.0;
    }

    // the hidden cells the readout can hear: in the 3x3 tiles round a button's cell
    std::vector<uint8_t> near_tile((size_t)blob->tile_count + 1, 0);
    for (int t = 0; t < blob->tile_count; t++) {
        for (int k = 0; (blob->motor_attached[t] != 0) && k < LOCAL_SLOTS; k++) {
            near_tile[blob->local_tile[t * LOCAL_SLOTS + k]] = 1;
        }
    }
    std::vector<int> near;
    for (int i : hidden) {
        if (near_tile[(size_t)(i / TILE_NEURONS)]) {
            near.push_back(i);
        }
    }

    // how long the cue stays readable, by every hidden cell and by those near the buttons;
    // the cells the reader of the last blank step leans on (the ask step also hears the
    // press, which carries the answer once the readout has it)
    int lean = c.phases > 2 ? c.phases - 2 : c.phases - 1;
    std::printf("timewire seed %llu: %d trials, %zu hidden cells, %zu near the buttons\n",
                (unsigned long long)seed, trials, hidden.size(), near.size());
    std::vector<double> w;
    for (int p = 0; p < c.phases; p++) {
        double all = ridge_read(c, hidden, p, n, p == lean ? &w : nullptr);
        double nr = near.empty() ? 0 : ridge_read(c, near, p, n, nullptr);
        int carriers = 0;
        for (int i : hidden) {
            carriers += std::fabs(dp[(size_t)p][(size_t)i]) >= carry_min;
        }
        std::printf("read %s %d: all %.3f near %.3f, %d cells |d'|>=%.2f\n",
                    p == 0 ? "cue" : (p == c.phases - 1 ? "ask" : "blank"), p, all, nr, carriers,
                    carry_min);
    }
    int carriers = 0;
    for (int i : hidden) {
        carriers += std::fabs(carry[(size_t)i]) >= carry_min;
    }
    std::printf("\ncarry over the blanks: %d cells\n", carriers);

    std::vector<int> order((size_t)hidden.size());
    for (size_t j = 0; j < order.size(); j++) {
        order[j] = (int)j;
    }
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return std::fabs(w[(size_t)a]) > std::fabs(w[(size_t)b]); });
    std::vector<CellWiring> top_cells, all_cells, carry_cells;
    std::vector<CellWiring> row_of((size_t)n);
    for (size_t j = 0; j < order.size(); j++) {
        int i = hidden[(size_t)order[j]];
        int pref = dp[(size_t)lean][(size_t)i] >= 0 ? 1 : -1;
        CellWiring cw = wiring_of(*blob, plug, i, pref, line_of, carry, carry_min);
        row_of[(size_t)i] = cw;
        if ((int)j < top) {
            top_cells.push_back(cw);
        }
        if (std::fabs(carry[(size_t)i]) >= carry_min) {
            CellWiring cc =
                wiring_of(*blob, plug, i, carry[(size_t)i] > 0 ? 1 : -1, line_of, carry, carry_min);
            carry_cells.push_back(cc);
        }
        all_cells.push_back(cw);
    }
    print_wiring("top", top_cells);
    print_wiring("carriers", carry_cells);
    print_wiring("all hidden", all_cells);

    const char* out = text_arg(argc, argv, "--out", nullptr);
    if (out) {
        FILE* f = std::fopen(out, "w");
        if (f) {
            std::fprintf(f, "neuron,rank,weight,carry");
            for (int p = 0; p < c.phases; p++) {
                std::fprintf(f, ",dp%d", p);
            }
            for (int k = 0; k < SOURCES; k++) {
                std::fprintf(f, ",n_%s", source_names[k]);
            }
            for (int k = 0; k < SOURCES; k++) {
                std::fprintf(f, ",d_%s", source_names[k]);
            }
            std::fprintf(f, "\n");
            for (size_t j = 0; j < order.size(); j++) {
                int i = hidden[(size_t)order[j]];
                std::fprintf(f, "%d,%zu,%.5f,%.4f", i, j, w[(size_t)order[j]], carry[(size_t)i]);
                for (int p = 0; p < c.phases; p++) {
                    std::fprintf(f, ",%.4f", dp[(size_t)p][(size_t)i]);
                }
                const CellWiring& cw = row_of[(size_t)i];
                for (int k = 0; k < SOURCES; k++) {
                    std::fprintf(f, ",%.0f", cw.count[k]);
                }
                for (int k = 0; k < SOURCES; k++) {
                    std::fprintf(f, ",%.0f", cw.delay[k]);
                }
                std::fprintf(f, "\n");
            }
            std::fclose(f);
        }
    }
    plug_detach(tt.plug, *blob);
    blob_free(blob);
    return 0;
}
