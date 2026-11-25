// blobprobe looptrace --life "<blobrun flags of delayed echo, k 2>" [--born 1]
//     [--read-delay D] [--calib N] [--trials N] [--carry C] [--loop-strong 1] [--out PATH]
//     [--after "<--g.name value ...>"] [--set-cool N] [--set-leak N] [--quiet-pace 1]
//     [--g.name value ...]
// idea 172 (docs/results-2026-11-26-loops.md): what ends the learned loop. Lives the life as
// blobprobe transfer does, makes after_life's changes (the wiring stays the life's), stops
// every rule (regulation runs) and shows delayed echo with D blank steps (default 16) from a
// fresh task seed:
//   - calibration (--calib trials, default 600): each hidden excitatory cell's d' for the
//     cue at the first blank. |d'| >= C/100 (default 50) makes it a loop cell of the cue it
//     prefers. --loop-strong 1 then makes every wired input a loop cell takes from a loop cell
//     of the same cue strong (4): the loop with all the strength it could have.
//   - the read (--trials, default 1200): at each step, a fresh ridge reader of the cue from all
//     hidden cells, and the loop's own vote (spikes of cue-1 loop cells less cue-0 loop cells,
//     cut at the middle of the two means of the first half, scored on the second half).
//   - tick by tick through the trial: the share of lit loop cells (those of the cue shown)
//     firing, of the rival loop (the other cue), other hidden excitatory and inhibitory cells;
//     the lit cells' mean threshold and sector bias; the share of lit cells over their bar
//     that did not fire although rested (the tile's winners cut them); the share of lit
//     cells whose tile spent all its winners on that tick.
// --out writes the tick table as csv.

#include "blob/blob.hpp"
#include "blob/tick_steps.hpp"
#include "probe/time_read.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);
bool parse_life(const char* text, const Genome& genome, uint64_t seed, RunFlags& flags);
Blob* probe_sheet(int argc, char** argv, const Genome& genome, uint64_t seed);

// a cell's role while a trial runs: loop cells by the cue they hold, others by kind
enum Role { LOOP0, LOOP1, OTHER, INHIB, NOT_HIDDEN };
// what is summed per tick of the trial
enum Stat { LIT, RIVAL, HIDDEN, INHIBIT, LIT_THR, RIVAL_THR, LIT_BIAS, LIT_CUT, LIT_FULL, STATS };
static const char* const stat_names[STATS] = {"lit",  "rival", "hidden", "inhib", "lit_thr",
                                              "rival_thr", "lit_bias", "lit_cut", "lit_full"};

struct TickSums {
    int ticks = 0;            // per trial, at most
    std::vector<double> sum;  // tick x stat
    std::vector<double> seen; // trials that reached the tick
    std::vector<int> phase;   // the step each tick belongs to
};

// one tick's stats for one trial of cue `cue`. role words: one per tile per role
static void tick_stats(const Blob& blob, const std::vector<uint64_t>& role, int cue, double* s,
                       double* counts) {
    const uint64_t* fired = blob.spikes[blob.now];
    int tiles = blob.tile_count, k = blob.genome.tile_winners;
    for (int t = 0; t < tiles; t++) {
        uint64_t lit = role[(size_t)(cue ? LOOP1 : LOOP0) * tiles + t];
        uint64_t rival = role[(size_t)(cue ? LOOP0 : LOOP1) * tiles + t];
        uint64_t other = role[(size_t)OTHER * tiles + t], inh = role[(size_t)INHIB * tiles + t];
        s[LIT] += __builtin_popcountll(fired[t] & lit);
        s[RIVAL] += __builtin_popcountll(fired[t] & rival);
        s[HIDDEN] += __builtin_popcountll(fired[t] & other);
        s[INHIBIT] += __builtin_popcountll(fired[t] & inh);
        int bias = blob.sector_bias[blob.tile_sector[t]];
        bool full = k > 0 && __builtin_popcountll(fired[t] & ~blob.sensor[t]) >= k;
        for (uint64_t m = lit | rival; m; m &= m - 1) {
            int b = __builtin_ctzll(m), n = t * TILE_NEURONS + b;
            bool is_lit = (lit >> b) & 1;
            s[is_lit ? LIT_THR : RIVAL_THR] += blob.threshold[n];
            if (!is_lit) {
                continue;
            }
            s[LIT_BIAS] += bias;
            bool over = blob.potential[n] >= blob.threshold[n] + bias;
            s[LIT_CUT] += over && !((fired[t] >> b) & 1) && blob.cooldown[n] == 0;
            s[LIT_FULL] += full;
        }
    }
    // per-cell means: divide by the class sizes
    for (int st = 0; st < STATS; st++) {
        s[st] /= counts[st] > 0 ? counts[st] : 1;
    }
}

// delayed echo, every rule stopped, from `task_seed`: spikes per cell per step into c, and
// (when sums is not null) the per-tick stats of the roles
static void record(Blob& blob, const RunFlags& rec, int trials, Counts& c,
                   const std::vector<uint64_t>* role, const int* class_size, TickSums* sums) {
    static TaskTicker tt;
    tt = TaskTicker{};
    if (!ticker_start(tt, blob, rec)) {
        std::fprintf(stderr, "blobprobe looptrace: the plug does not fit\n");
        std::exit(1);
    }
    int n = blob.neuron_count;
    c.phases = rec.task_settings.delay + 2;
    c.spikes.assign((size_t)trials * c.phases * n, 0);
    c.cue.assign((size_t)trials, 0);
    if (sums) {
        sums->ticks = c.phases * tt.ticks_per_step;
        sums->sum.assign((size_t)sums->ticks * STATS, 0.0);
        sums->seen.assign((size_t)sums->ticks, 0.0);
        sums->phase.assign((size_t)sums->ticks, 0);
    }
    std::vector<int> step((size_t)n, 0);
    int trial = -1, phase = 0, tick = 0;
    double s[STATS];
    while (true) {
        ticker_before_tick(tt, blob);
        if (!tt.resting && tt.tick_in_step == 0) {
            phase = tt.task.phase;
            if (phase == 0) {
                trial++;
                tick = 0;
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
                step[(size_t)(t * TILE_NEURONS + __builtin_ctzll(m))]++;
            }
        }
        if (sums && trial >= 0 && !tt.resting && tick < sums->ticks) {
            int cue = c.cue[(size_t)trial];
            // class sizes: the lit and rival classes are the loop of the cue shown and of
            // the other
            double sized[STATS];
            for (int st = 0; st < STATS; st++) {
                s[st] = 0;
            }
            sized[LIT] = sized[LIT_THR] = sized[LIT_BIAS] = sized[LIT_CUT] = sized[LIT_FULL] =
                class_size[cue ? LOOP1 : LOOP0];
            sized[RIVAL] = sized[RIVAL_THR] = class_size[cue ? LOOP0 : LOOP1];
            sized[HIDDEN] = class_size[OTHER];
            sized[INHIBIT] = class_size[INHIB];
            tick_stats(blob, *role, cue, s, sized);
            for (int st = 0; st < STATS; st++) {
                sums->sum[(size_t)tick * STATS + st] += s[st];
            }
            sums->seen[(size_t)tick] += 1;
            sums->phase[(size_t)tick] = phase;
            tick++;
        }
        if (!ticker_after_tick(tt, blob)) {
            continue;
        }
        if (trial >= 0 && phase < c.phases) {
            uint8_t* row = &c.spikes[((size_t)trial * c.phases + phase) * n];
            for (int i = 0; i < n; i++) {
                row[i] = (uint8_t)(step[(size_t)i] > 255 ? 255 : step[(size_t)i]);
            }
        }
        for (int& v : step) {
            v = 0;
        }
    }
    plug_detach(tt.plug, blob);
}

static double d_prime(const Counts& c, int phase, int n, int cell) {
    double s[2] = {0, 0}, q[2] = {0, 0}, k[2] = {0, 0};
    for (size_t r = 0; r < c.cue.size(); r++) {
        double x = c.spikes[(r * c.phases + phase) * n + cell];
        int a = c.cue[r] != 0;
        s[a] += x, q[a] += x * x, k[a] += 1;
    }
    if (k[0] < 2 || k[1] < 2) {
        return 0;
    }
    double m0 = s[0] / k[0], m1 = s[1] / k[1];
    double v = 0.5 * (q[0] / k[0] - m0 * m0 + q[1] / k[1] - m1 * m1);
    return (m1 - m0) / std::sqrt(v + 1e-3);
}

// the loop's own vote at one step: cue-1 loop spikes less cue-0 loop spikes (each per cell),
// cut at the middle of the two cue means of the first half, share right on the second half
static double vote_read(const Counts& c, const std::vector<int>& loop1,
                        const std::vector<int>& loop0, int phase, int n) {
    size_t trials = c.cue.size(), half = trials / 2;
    std::vector<double> v(trials, 0.0);
    for (size_t r = 0; r < trials; r++) {
        const uint8_t* row = &c.spikes[(r * c.phases + phase) * n];
        double a = 0, b = 0;
        for (int i : loop1) {
            a += row[i];
        }
        for (int i : loop0) {
            b += row[i];
        }
        v[r] = (loop1.empty() ? 0 : a / loop1.size()) - (loop0.empty() ? 0 : b / loop0.size());
    }
    double m[2] = {0, 0}, k[2] = {0, 0};
    for (size_t r = 0; r < half; r++) {
        m[c.cue[r] != 0] += v[r], k[c.cue[r] != 0] += 1;
    }
    double cut = 0.5 * (m[0] / (k[0] > 0 ? k[0] : 1) + m[1] / (k[1] > 0 ? k[1] : 1));
    int right = 0;
    for (size_t r = half; r < trials; r++) {
        right += v[r] == cut ? (int)(r & 1) : ((v[r] > cut) == (c.cue[r] != 0));
    }
    return trials > half ? (double)right / (double)(trials - half) : 0;
}

int probe_looptrace(int argc, char** argv) {
    Genome genome;
    apply_genome_args(argc, argv, genome);
    uint64_t seed = (uint64_t)int_arg(argc, argv, "--seed", 1);
    const char* life_text = text_arg(argc, argv, "--life", nullptr);
    RunFlags life;
    if (!life_text || !parse_life(life_text, genome, seed, life) ||
        life.task != TaskKind::delayed_echo || life.task_settings.k != 2) {
        std::fprintf(stderr, "blobprobe looptrace: --life \"<blobrun flags>\" of delayed echo, "
                             "k 2, is needed\n");
        return 1;
    }
    int calib = int_arg(argc, argv, "--calib", 600);
    int trials = int_arg(argc, argv, "--trials", 1200);
    double carry_min = int_arg(argc, argv, "--carry", 50) / 100.0;
    Blob* blob = probe_sheet(argc, argv, life.genome, seed);
    blob->genome.learning_on = 0;
    RunFlags rec = life;
    rec.task_seed = life.seed + 200;
    rec.task_settings.delay = int_arg(argc, argv, "--read-delay", 16);
    int n = blob->neuron_count, tiles = blob->tile_count;

    // calibration: the loop cells, by their d' at the first blank
    Counts c1;
    record(*blob, rec, calib, c1, nullptr, nullptr, nullptr);
    std::vector<uint64_t> role((size_t)NOT_HIDDEN * tiles, 0);
    std::vector<int> hidden, loop[2];
    int size[NOT_HIDDEN] = {0};
    for (int i = 0; i < n; i++) {
        int t = i / TILE_NEURONS, b = i % TILE_NEURONS;
        uint64_t not_hidden = blob->sensor[t] | blob->motor[t] | blob->pacemaker[t];
        if ((not_hidden >> b) & 1) {
            continue;
        }
        hidden.push_back(i);
        int r = OTHER;
        if ((blob->inhibitory[t] >> b) & 1) {
            r = INHIB;
        } else {
            double d = d_prime(c1, 1, n, i);
            r = std::fabs(d) >= carry_min ? (d > 0 ? LOOP1 : LOOP0) : OTHER;
        }
        role[(size_t)r * tiles + t] |= 1ull << b;
        size[r]++;
        if (r == LOOP0 || r == LOOP1) {
            loop[r == LOOP1].push_back(i);
        }
    }

    // --loop-strong: every wired input a loop cell takes from its own loop is strong
    int made_strong = 0;
    if (int_arg(argc, argv, "--loop-strong", 0)) {
        for (int side = 0; side < 2; side++) {
            const uint64_t* mine = &role[(size_t)(side ? LOOP1 : LOOP0) * tiles];
            for (int i : loop[side]) {
                for (int k = 0; k < SLOTS; k++) {
                    uint32_t src = slot_source(*blob, i, k);
                    if (src == (uint32_t)tiles) {
                        continue;
                    }
                    int64_t w = blob_weight_index(i, k);
                    uint64_t weak = blob->w0[w] & ~blob->w1[w] & mine[src];
                    made_strong += __builtin_popcountll(weak);
                    blob->w1[w] |= weak;
                    blob->w0[w] &= ~weak;
                }
            }
        }
    }

    // the read
    Counts c;
    TickSums sums;
    record(*blob, rec, trials, c, &role, size, &sums);
    std::printf("looptrace seed %llu: read delay %d, %d trials; loop cells %d (cue 0) %d (cue 1), "
                "other hidden %d, inhibitory %d; made strong %d\n",
                (unsigned long long)seed, rec.task_settings.delay, trials, size[LOOP0],
                size[LOOP1], size[OTHER], size[INHIB], made_strong);
    int ticks_per_step = sums.ticks / c.phases;
    int trace = 0;
    bool held = true;
    for (int p = 0; p < c.phases; p++) {
        double all = ridge_read(c, hidden, p, n, nullptr);
        double vote = vote_read(c, loop[1], loop[0], p, n);
        if (p >= 1 && p + 1 < c.phases) {
            held = held && all >= 0.75;
            trace += held;
        }
        // the step's mean of each tick stat
        double m[STATS] = {0};
        for (int t = p * ticks_per_step; t < (p + 1) * ticks_per_step; t++) {
            for (int st = 0; st < STATS; st++) {
                m[st] += sums.seen[(size_t)t] > 0 ? sums.sum[(size_t)t * STATS + st] /
                                                        sums.seen[(size_t)t] / ticks_per_step
                                                  : 0;
            }
        }
        std::printf("step %s %d: read %.3f vote %.3f; per 1000 cell-ticks lit %.1f rival %.1f "
                    "hidden %.1f inhib %.1f; lit thr %.2f rival thr %.2f bias %.2f; lit cut %.3f "
                    "full %.3f\n",
                    p == 0 ? "cue" : (p == c.phases - 1 ? "ask" : "blank"), p, all, vote,
                    1000 * m[LIT], 1000 * m[RIVAL], 1000 * m[HIDDEN], 1000 * m[INHIBIT],
                    m[LIT_THR], m[RIVAL_THR], m[LIT_BIAS], m[LIT_CUT], m[LIT_FULL]);
    }
    std::printf("trace: %d blanks read at 0.75 or more\n", trace);

    const char* out = text_arg(argc, argv, "--out", nullptr);
    FILE* f = out ? std::fopen(out, "w") : nullptr;
    if (f) {
        std::fprintf(f, "tick,phase");
        for (int st = 0; st < STATS; st++) {
            std::fprintf(f, ",%s", stat_names[st]);
        }
        std::fprintf(f, "\n");
        for (int t = 0; t < sums.ticks; t++) {
            std::fprintf(f, "%d,%d", t, sums.phase[(size_t)t]);
            for (int st = 0; st < STATS; st++) {
                double v = sums.seen[(size_t)t] > 0
                               ? sums.sum[(size_t)t * STATS + st] / sums.seen[(size_t)t]
                               : 0;
                std::fprintf(f, ",%.5f", v);
            }
            std::fprintf(f, "\n");
        }
        std::fclose(f);
    }
    blob_free(blob);
    return 0;
}
