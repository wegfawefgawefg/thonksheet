// blobprobe ratehealth --life "<blobrun flags>" [--measure-steps N] [--gap N] [--back-steps N]
//     [--bin N] [--g.name value ...]
// idea 175 (docs/results-2026-11-26-rate-control.md): the sheet's health at the end of a life
// and through a change. Lives the life (put every genome number on the probe's own line, so
// the sheet keeps learning the same way after it), then goes on with the same task, learning
// on, in three parts:
//   - measure (--measure-steps, default 820: 16,400 ticks at 20 a step): each cell's spikes
//     counted. rate per 1,024 cell-ticks; silent (no spike) and stuck (spiking on over a
//     quarter of the ticks) shares as blobrun counts them, for all non-sensor cells and apart
//     for plain hidden cells, integrator and time cells, and motor cells; thresholds of hidden
//     cells at the cap and at the floor; the sector bias, mean and shares at either limit.
//   - gap (--gap, default 2,000 ticks): every line unplugged, no reward.
//   - back (--back-steps, default 100): the same task again.
// Through the last 2,000 ticks of measure, the gap and back: the rate per --bin ticks
// (default 100), the hidden cells' mean threshold and the mean sector bias at each bin's end.
// "jump": the rate in the first bin back over the rate of the last 1,000 ticks before the gap.

#include "blob/blob.hpp"
#include "blob/long_memory.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cstdio>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);
bool parse_life(const char* text, const Genome& genome, uint64_t seed, RunFlags& flags);
Blob* probe_sheet(int argc, char** argv, const Genome& genome, uint64_t seed);

// the kinds of non-sensor cell the health is split into
enum Kind { PLAIN, MEMORY, MOTOR, KINDS };
static const char* const kind_names[KINDS] = {"hidden", "memory", "motor"};

static bool bit_of(const uint64_t* mask, int n) {
    return mask && ((mask[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1);
}

static Kind kind_of(const Blob& blob, int n) {
    if (bit_of(blob.motor, n)) {
        return MOTOR;
    }
    const BlobLongMemory* lm = blob.long_memory;
    if (lm && (bit_of(lm->cell, n) || bit_of(lm->time_cell, n))) {
        return MEMORY;
    }
    return PLAIN;
}

struct Bins {
    int size = 100;
    int64_t ticks = 0, spikes = 0; // the open bin
    std::vector<double> rate, thr, bias;
};

// non-sensor spikes of the latest tick
static int fired_inner(const Blob& blob) {
    int f = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        f += __builtin_popcountll(blob.spikes[blob.now][t] & ~blob.sensor[t]);
    }
    return f;
}

static int inner_count(const Blob& blob) {
    int c = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        c += TILE_NEURONS - __builtin_popcountll(blob.sensor[t]);
    }
    return c;
}

static void bin_tick(Bins& b, const Blob& blob, int inner) {
    b.spikes += fired_inner(blob);
    if (++b.ticks < b.size) {
        return;
    }
    double thr = 0, cells = 0, bias = 0;
    for (int n = 0; n < blob.neuron_count; n++) {
        if (!bit_of(blob.sensor, n) && !bit_of(blob.pacemaker, n)) {
            thr += blob.threshold[n];
            cells++;
        }
    }
    for (int s = 0; s < blob.sector_count; s++) {
        bias += blob.sector_bias[s];
    }
    b.rate.push_back((double)b.spikes * 1024.0 / ((double)b.ticks * inner));
    b.thr.push_back(cells > 0 ? thr / cells : 0);
    b.bias.push_back(blob.sector_count ? bias / blob.sector_count : 0);
    b.ticks = b.spikes = 0;
}

// the task for `steps` steps, as blobrun's life loop does it; counts per cell when `count`
static void run_task(Blob& blob, const RunFlags& flags, int64_t steps, std::vector<uint32_t>* count,
                     Bins* bins, int64_t bins_from_tick, int inner) {
    static TaskTicker tt;
    tt = TaskTicker{};
    if (!ticker_start(tt, blob, flags)) {
        std::fprintf(stderr, "blobprobe ratehealth: the plug does not fit\n");
        std::exit(1);
    }
    int64_t step = 0, tick = 0;
    while (step < steps) {
        ticker_before_tick(tt, blob);
        blob_tick(blob);
        if (count) {
            for (int t = 0; t < blob.tile_count; t++) {
                for (uint64_t m = blob.spikes[blob.now][t]; m; m &= m - 1) {
                    (*count)[(size_t)t * TILE_NEURONS + __builtin_ctzll(m)]++;
                }
            }
        }
        if (bins && tick >= bins_from_tick) {
            bin_tick(*bins, blob, inner);
        }
        tick++;
        if (!ticker_after_tick(tt, blob)) {
            continue;
        }
        if (tt.valence != 0) {
            plug_mark_acted(tt.plug, blob, tt.move);
            plug_mark_vote(tt.plug, blob, tt.move);
            plug_hop_lines(tt.plug, blob, tt.move, tt.right_move, tt.in.pattern,
                           tt.in.pattern_tied, tt.task.pattern_graded);
            blob_reward(blob, tt.valence);
        }
        step++;
    }
    plug_detach(tt.plug, blob);
}

static void print_health(const Blob& blob, const std::vector<uint32_t>& count, int64_t ticks) {
    int64_t fired = 0;
    int cells[KINDS] = {}, silent[KINDS] = {}, stuck[KINDS] = {};
    int all = 0, all_silent = 0, all_stuck = 0, hidden = 0, at_cap = 0, at_floor = 0;
    for (int n = 0; n < blob.neuron_count; n++) {
        if (bit_of(blob.sensor, n)) {
            continue;
        }
        Kind k = kind_of(blob, n);
        uint32_t s = count[(size_t)n];
        fired += s;
        all++, cells[k]++;
        bool off = s == 0, on = (int64_t)s * 4 > ticks;
        all_silent += off, silent[k] += off;
        all_stuck += on, stuck[k] += on;
        if (k == PLAIN && !bit_of(blob.pacemaker, n)) {
            hidden++;
            at_cap += blob.threshold[n] >= blob.genome.threshold_cap;
            at_floor += blob.threshold[n] <= 1;
        }
    }
    int top = blob.genome.sector_bias_max, plus = 0, minus = 0;
    double bias = 0;
    for (int s = 0; s < blob.sector_count; s++) {
        bias += blob.sector_bias[s];
        plus += blob.sector_bias[s] >= top;
        minus += blob.sector_bias[s] <= -top;
    }
    double rate = (double)fired * 1024.0 / ((double)ticks * (all ? all : 1));
    std::printf("health: rate %.2f per 1024 cell-ticks (target %d), silent %.4f, stuck %.4f; "
                "hidden thresholds at cap %.4f, at floor %.4f; sector bias mean %.2f, at +max "
                "%.3f, at -max %.3f\n",
                rate, blob.genome.sector_target_per_1024, (double)all_silent / (all ? all : 1),
                (double)all_stuck / (all ? all : 1), (double)at_cap / (hidden ? hidden : 1),
                (double)at_floor / (hidden ? hidden : 1),
                blob.sector_count ? bias / blob.sector_count : 0,
                (double)plus / (blob.sector_count ? blob.sector_count : 1),
                (double)minus / (blob.sector_count ? blob.sector_count : 1));
    for (int k = 0; k < KINDS; k++) {
        if (!cells[k]) {
            continue;
        }
        std::printf("kind %s: %d cells, silent %.4f, stuck %.4f\n", kind_names[k], cells[k],
                    (double)silent[k] / cells[k], (double)stuck[k] / cells[k]);
    }
}

int probe_ratehealth(int argc, char** argv) {
    uint64_t seed = (uint64_t)int_arg(argc, argv, "--seed", 5);
    Genome genome;
    apply_genome_args(argc, argv, genome);
    RunFlags life;
    const char* life_text = text_arg(argc, argv, "--life", nullptr);
    if (!life_text || !parse_life(life_text, genome, seed, life)) {
        std::fprintf(stderr, "blobprobe ratehealth: --life \"<blobrun flags>\" needed\n");
        return 1;
    }
    Blob* blob = probe_sheet(argc, argv, life.genome, seed);
    int64_t measure = int_arg(argc, argv, "--measure-steps", 820);
    int64_t gap = int_arg(argc, argv, "--gap", 2000);
    int64_t back = int_arg(argc, argv, "--back-steps", 100);
    Bins bins;
    bins.size = int_arg(argc, argv, "--bin", 100);
    int inner = inner_count(*blob);
    int64_t per_step = life.plug.ticks_per_step;
    int64_t measure_ticks = measure * per_step;
    // the bins start 2,000 ticks before the gap (or with the part, if it is shorter)
    int64_t from = measure_ticks > 2000 ? measure_ticks - 2000 : 0;
    std::vector<uint32_t> count((size_t)blob->neuron_count, 0);
    run_task(*blob, life, measure, &count, &bins, from, inner);
    print_health(*blob, count, measure_ticks);
    size_t before_end = bins.rate.size();
    blob_detach_all(*blob);
    for (int64_t t = 0; t < gap; t++) {
        blob_tick(*blob);
        bin_tick(bins, *blob, inner);
    }
    size_t gap_end = bins.rate.size();
    RunFlags again = life;
    again.task_seed = seed + 200;
    run_task(*blob, again, back, nullptr, &bins, 0, inner);
    // the last 1,000 ticks before the gap, against the first bin back
    double before = 0;
    int nb = 0;
    for (size_t i = before_end; i > 0 && nb * bins.size < 1000; i--, nb++) {
        before += bins.rate[i - 1];
    }
    before = nb ? before / nb : 0;
    double first = gap_end < bins.rate.size() ? bins.rate[gap_end] : 0;
    double gap_rate = 0;
    for (size_t i = before_end; i < gap_end; i++) {
        gap_rate += bins.rate[i];
    }
    gap_rate = gap_end > before_end ? gap_rate / (double)(gap_end - before_end) : 0;
    std::printf("change: rate before %.2f, in the gap %.2f, first bin back %.2f (jump %.2f); "
                "thresholds before %.2f, at the gap's end %.2f; bias before %.2f, at the gap's "
                "end %.2f\n",
                before, gap_rate, first, before > 0 ? first / before : 0,
                before_end ? bins.thr[before_end - 1] : 0, gap_end ? bins.thr[gap_end - 1] : 0,
                before_end ? bins.bias[before_end - 1] : 0, gap_end ? bins.bias[gap_end - 1] : 0);
    std::printf("bins (%d ticks; part, rate, hidden threshold mean, sector bias mean):\n",
                bins.size);
    for (size_t i = 0; i < bins.rate.size(); i++) {
        const char* part = i < before_end ? "on" : (i < gap_end ? "gap" : "back");
        std::printf("bin %zu %s %.2f %.2f %.2f\n", i, part, bins.rate[i], bins.thr[i], bins.bias[i]);
    }
    blob_free(blob);
    return 0;
}
