#include "bench/bench_modes.hpp"
#include "blob/blob.hpp"
#include "blob/random_words.hpp"

#include <cstdio>
#include <cstring>

// stage 0: the blob left running. three phases of `--ticks` each: idle (nothing attached),
// random input on every sensor line, and input that starts and stops (unplugged between).
// prints one line per window and writes per-sector activity to a csv.

struct WindowCounts {
    int64_t ticks = 0;
    int64_t fired = 0;                 // non-sensor spikes
    int64_t fired_sq = 0;              // sum of squared per-tick counts, for the fano factor
    int64_t saturated_ticks = 0;       // ticks with over a quarter of the free neurons firing
    int64_t silent_ticks = 0;          // ticks with no free neuron firing
    int64_t overlap_and[HISTORY] = {}; // spike map against the map `lag` ticks earlier
    int64_t overlap_or[HISTORY] = {};
    int64_t grown = 0, pruned = 0, strengthened = 0, weakened = 0, surprise = 0;
    int64_t* sector_spikes = nullptr;
    int* sector_free = nullptr; // free (non-sensor) neurons per sector
};

static int free_neurons(const Blob& blob) {
    int count = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        count += TILE_NEURONS - __builtin_popcountll(blob.sensor[t]);
    }
    return count;
}

static void count_tick(const Blob& blob, WindowCounts& w, int free_count) {
    const uint64_t* cur = blob.spikes[blob.now];
    int fired = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        int f = __builtin_popcountll(cur[t] & ~blob.sensor[t]);
        fired += f;
        w.sector_spikes[blob.tile_sector[t]] += f;
    }
    for (int lag = 1; lag < HISTORY; lag++) {
        const uint64_t* old = blob.spikes[(blob.now - lag) & (HISTORY - 1)];
        int64_t a = 0, o = 0;
        for (int t = 0; t < blob.tile_count; t++) {
            a += __builtin_popcountll(cur[t] & old[t]);
            o += __builtin_popcountll(cur[t] | old[t]);
        }
        w.overlap_and[lag] += a;
        w.overlap_or[lag] += o;
    }
    w.ticks++;
    w.fired += fired;
    w.fired_sq += (int64_t)fired * fired;
    w.saturated_ticks += fired * 4 > free_count ? 1 : 0;
    w.silent_ticks += fired == 0 ? 1 : 0;
    w.grown += blob.stats.grown;
    w.pruned += blob.stats.pruned;
    w.strengthened += blob.stats.strengthened;
    w.weakened += blob.stats.weakened;
    w.surprise += blob.stats.surprise;
}

// weight levels over the whole blob: counts of strength 1, 4 and 5
static void weight_levels(const Blob& blob, int64_t* ones, int64_t* fours, int64_t* fives) {
    *ones = *fours = *fives = 0;
    int64_t words = (int64_t)blob.neuron_count * SLOTS;
    for (int64_t i = 0; i < words; i++) {
        *ones += __builtin_popcountll(blob.w0[i] & ~blob.w1[i]);
        *fours += __builtin_popcountll(~blob.w0[i] & blob.w1[i]);
        *fives += __builtin_popcountll(blob.w0[i] & blob.w1[i]);
    }
}

static void report_window(const Blob& blob, WindowCounts& w, const char* phase, int free_count,
                          FILE* csv) {
    // per sector: spikes per tick per 1,024 of its free neurons
    int lo = 1 << 30, hi = 0, dead = 0, bias_lo = 1 << 30, bias_hi = -(1 << 30);
    for (int s = 0; s < blob.sector_count; s++) {
        int free_in_sector = w.sector_free[s];
        if (free_in_sector == 0) {
            continue; // all sensor: fires only from the world
        }
        int rate = (int)(w.sector_spikes[s] * 1024 / (w.ticks * free_in_sector));
        lo = rate < lo ? rate : lo;
        hi = rate > hi ? rate : hi;
        dead += w.sector_spikes[s] == 0 ? 1 : 0;
        int bias = blob.sector_bias[s];
        bias_lo = bias < bias_lo ? bias : bias_lo;
        bias_hi = bias > bias_hi ? bias : bias_hi;
        if (csv) {
            std::fprintf(csv, "%s,%llu,%d,%d,%d\n", phase, (unsigned long long)blob.stats.tick, s,
                         rate, blob.sector_bias[s]);
        }
    }
    // the most repeated lag (2..15); lag 1 is listed apart since cooldown shapes it
    int best_lag = 2;
    int64_t best = 0;
    for (int lag = 2; lag < HISTORY; lag++) {
        int64_t o = w.overlap_or[lag] ? w.overlap_and[lag] * 1000 / w.overlap_or[lag] : 0;
        if (o > best) {
            best = o, best_lag = lag;
        }
    }
    int64_t lag1 = w.overlap_or[1] ? w.overlap_and[1] * 1000 / w.overlap_or[1] : 0;
    int64_t ones, fours, fives;
    weight_levels(blob, &ones, &fours, &fives);
    int64_t thr = 0;
    for (int n = 0; n < blob.neuron_count; n++) {
        thr += blob.threshold[n];
    }
    // fano factor of the per-tick count: about 1 for independent firing, large for bursts
    double mean = (double)w.fired / w.ticks;
    double fano = mean > 0 ? ((double)w.fired_sq / w.ticks - mean * mean) / mean : 0;
    std::printf("%-7s %9llu %5lld %5d %5d %4d %6.1f %5lld %5lld %5lld %3d %5lld %6.1f %4d %4d "
                "%5.1f %5.2f %5.2f %6lld %6lld %7lld %7lld %6lld\n",
                phase, (unsigned long long)blob.stats.tick,
                (long long)(w.fired * 1024 / (w.ticks * free_count)), lo, hi, dead, fano,
                (long long)w.saturated_ticks, (long long)w.silent_ticks, (long long)lag1, best_lag,
                (long long)best, (double)thr / blob.neuron_count, bias_lo, bias_hi,
                (double)ones / blob.neuron_count, (double)fours / blob.neuron_count,
                (double)fives / blob.neuron_count, (long long)w.grown, (long long)w.pruned,
                (long long)w.strengthened, (long long)w.weakened, (long long)w.surprise);
    std::fflush(stdout);
}

// phase input: 0 idle, 1 random on every line, 2 random in bursts with unplugged gaps
static void feed(Blob& blob, int phase, uint64_t* rng, int64_t* burst_left, bool* plugged) {
    if (phase == 0) {
        return;
    }
    if (phase == 2 && --*burst_left <= 0) {
        *plugged = !*plugged;
        *burst_left = 200 + rng_below(rng, 5000);
    }
    for (int t = 0; t < blob.tile_count; t++) {
        if (!blob.sensor[t]) {
            continue;
        }
        if (phase == 2 && !*plugged) {
            blob.sense_attached[t] = 0;
            blob.sense_in[t] = 0;
            continue;
        }
        blob.sense_attached[t] |= blob.sensor[t];
        blob.sense_in[t] = blob.sensor[t] & sparse_word(rng, 3);
    }
}

int run_stage0(int argc, char** argv) {
    int64_t phase_ticks = arg_int(argc, argv, "--ticks", 1000000);
    int64_t window = arg_int(argc, argv, "--window", 50000);
    int tiles = (int)arg_int(argc, argv, "--tiles", 8);
    uint64_t seed = (uint64_t)arg_int(argc, argv, "--seed", 1);
    int csv_on = (int)arg_int(argc, argv, "--csv", 1);

    Genome g = genome_for_sheet(tiles);
    apply_genome_args(argc, argv, g);
    Blob* blob = blob_birth(g, seed);
    int free_count = free_neurons(*blob);
    uint64_t rng[4];
    rng_seed(rng, seed * 7919 + 1);
    FILE* csv = csv_on ? std::fopen("stage0_sectors.csv", "w") : nullptr;
    if (csv) {
        std::fprintf(csv, "phase,tick,sector,spikes_per_tick_per_1024,bias\n");
    }

    std::printf("neurons %d, sectors %d, target %d spikes per tick per 1024\n", blob->neuron_count,
                blob->sector_count, g.sector_target_per_1024);
    std::printf("%-7s %9s %5s %5s %5s %4s %6s %5s %5s %5s %3s %5s %6s %4s %4s %5s %5s %5s %6s "
                "%6s %7s %7s %6s\n",
                "phase", "tick", "rate", "seclo", "sechi", "dead", "fano", "satur", "silnt", "lag1",
                "lag", "ovl", "thr", "bilo", "bihi", "w1", "w4", "w5", "grown", "pruned", "strong",
                "weak", "surpr");
    const char* names[] = {"idle", "random", "onoff"};
    WindowCounts w;
    w.sector_spikes = new int64_t[blob->sector_count]();
    w.sector_free = new int[blob->sector_count]();
    for (int t = 0; t < blob->tile_count; t++) {
        w.sector_free[blob->tile_sector[t]] += TILE_NEURONS - __builtin_popcountll(blob->sensor[t]);
    }
    int64_t burst_left = 1;
    bool plugged = false;
    for (int phase = 0; phase < 3; phase++) {
        for (int64_t i = 0; i < phase_ticks; i++) {
            feed(*blob, phase, rng, &burst_left, &plugged);
            blob_tick(*blob);
            count_tick(*blob, w, free_count);
            if (w.ticks == window) {
                report_window(*blob, w, names[phase], free_count, csv);
                int64_t* keep = w.sector_spikes;
                int* keep_free = w.sector_free;
                w = WindowCounts{};
                w.sector_spikes = keep;
                w.sector_free = keep_free;
                std::memset(keep, 0, sizeof(int64_t) * blob->sector_count);
            }
        }
    }
    if (csv) {
        std::fclose(csv);
    }
    delete[] w.sector_spikes;
    delete[] w.sector_free;
    blob_free(blob);
    return 0;
}
