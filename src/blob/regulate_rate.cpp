#include "blob/regulate_rate.hpp"

#include <cstdlib>

// idea 175: a rate control that does not fight the rest (docs/results-2026-11-26-rate-control.md).
// the standard control (regulate.cpp) walks every cell's threshold up on a spike and down by
// chance each tick, and steps each sector's bias toward the target every tick. that jitters
// a read, takes back a loop that stays lit, and pins cells at the cap. the pieces here replace
// parts of it; tick_regulate picks them by genome number:
//   rate_band_shift   the dead band (this file)
//   threshold_reg_until, threshold_late_shift   a critical period (regulate.cpp, no state)
//   sector_gain       inhibition in proportion to the sector's own activity (this file)

constexpr int THRESHOLD_FLOOR = 1;
constexpr int COUNT_MAX = 65535;

struct RateState {
    uint16_t* count = nullptr; // per neuron: spikes in its window so far (dead band)
    int16_t* slow = nullptr;   // per sector: the slow part of the bias (sector_gain)
    uint32_t* recip = nullptr; // per sector: 2^20 / target, so the gain needs no divide
};

static RateState& rate_of(Blob& blob) {
    if (!blob.rate) {
        blob.rate = (RateState*)std::calloc(1, sizeof(RateState));
    }
    return *blob.rate;
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// a cell's share of the target over one window of 2^k ticks, in spikes
static int window_target(const Genome& g, int k) {
    int64_t t = ((int64_t)g.sector_target_per_1024 << k) >> 10;
    return (int)(t < 1 ? 1 : (t > COUNT_MAX ? COUNT_MAX : t));
}

void regulate_band(Blob& blob) {
    const Genome& g = blob.genome;
    RateState& r = rate_of(blob);
    int k = clamp_int(g.rate_band_shift, 1, 24);
    int target = window_target(g, k);
    if (!r.count) {
        // made on first use (also when switched on after a life): every cell starts its first
        // window at the target, so a part window does not look silent
        r.count = (uint16_t*)std::malloc(sizeof(uint16_t) * (size_t)blob.neuron_count);
        for (int n = 0; n < blob.neuron_count; n++) {
            r.count[n] = (uint16_t)target;
        }
    }
    int lo = target >> clamp_int(g.rate_band_low, 0, 16);
    int64_t hi64 = (int64_t)target << clamp_int(g.rate_band_high, 0, 16);
    int hi = (int)(hi64 > COUNT_MAX - 1 ? COUNT_MAX - 1 : hi64);
    int step = g.rate_band_step;
    uint64_t mask = (1ull << k) - 1;
    uint64_t tick = blob.stats.tick;
    const uint64_t* fired = blob.spikes[blob.now];
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t fixed = blob.sensor[t] | blob.pacemaker[t];
        uint16_t* c = r.count + (size_t)t * TILE_NEURONS;
        // count this tick's spikes
        for (uint64_t m = fired[t] & ~fixed; m; m &= m - 1) {
            int b = __builtin_ctzll(m);
            c[b] = c[b] < COUNT_MAX ? (uint16_t)(c[b] + 1) : c[b];
        }
        // each tile closes its window on its own tick, so the steps are spread out
        if (((tick + (uint64_t)t) & mask) != 0) {
            continue;
        }
        int16_t* thr = blob.threshold + (size_t)t * TILE_NEURONS;
        for (uint64_t m = ~fixed; m; m &= m - 1) {
            int b = __builtin_ctzll(m);
            int v = thr[b];
            if (c[b] < lo) {
                v = v - step < THRESHOLD_FLOOR ? THRESHOLD_FLOOR : v - step;
            } else if (c[b] > hi) {
                v = v + step > g.threshold_cap ? (v > g.threshold_cap ? v : g.threshold_cap)
                                                : v + step;
            }
            thr[b] = (int16_t)v;
            c[b] = 0;
        }
    }
}

void regulate_sector_gain(Blob& blob, int sector, int act, int step) {
    const Genome& g = blob.genome;
    RateState& r = rate_of(blob);
    if (!r.slow) {
        r.slow = (int16_t*)std::calloc((size_t)blob.sector_count, sizeof(int16_t));
        r.recip = (uint32_t*)std::calloc((size_t)blob.sector_count, sizeof(uint32_t));
        for (int s = 0; s < blob.sector_count; s++) {
            // the bias the sector had is where the slow part starts
            r.slow[s] = blob.sector_bias[s];
            uint32_t target = blob.sector_target[s];
            r.recip[s] = target > 0 ? (uint32_t)((1u << 20) / target) : 0;
        }
    }
    // the slow part: the usual step toward the target, with its dead band of an eighth
    int target = (int)blob.sector_target[sector];
    int band = target / 8;
    int slow = r.slow[sector];
    if (step && act > target + band && slow < g.sector_bias_max) {
        slow++;
    } else if (step && act < target - band && slow > -g.sector_bias_max) {
        slow--;
    }
    r.slow[sector] = (int16_t)slow;
    // the fast part: inhibition in proportion to the activity over the target, none under it
    int over = act - target;
    int64_t p = over > 0 ? ((int64_t)over * g.sector_gain * r.recip[sector]) >> 20 : 0;
    int top = 4 * g.sector_gain;
    p = p > top ? top : p;
    blob.sector_bias[sector] = (int16_t)(slow + (int)p);
}

void rate_free(Blob& blob) {
    if (!blob.rate) {
        return;
    }
    std::free(blob.rate->count);
    std::free(blob.rate->slow);
    std::free(blob.rate->recip);
    std::free(blob.rate);
    blob.rate = nullptr;
}
