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
// idea 184 (docs/results-2026-11-26-motor-apart.md): motor cells regulated apart from the
// rest (motor_apart; the end of this file). the output's motor cells share a sector's bias
// with the hidden cells, fire at several times their share under the output rules, pin the
// bias, and starve the hidden cells.

constexpr int THRESHOLD_FLOOR = 1;
constexpr int COUNT_MAX = 65535;

struct RateState {
    uint16_t* count = nullptr; // per neuron: spikes in its window so far (dead band)
    int16_t* slow = nullptr;   // per sector: the slow part of the bias (sector_gain)
    uint32_t* recip = nullptr; // per sector: 2^20 / target, so the gain needs no divide
    // motor_apart, per sector: the motor cells' own bias (3), smoothed count and target
    int16_t* motor_bias = nullptr;
    uint32_t* motor_activity = nullptr;
    uint32_t* motor_target = nullptr;
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
        // with rate_band_motor the motor cells walk instead (regulate.cpp)
        uint64_t fixed =
            blob.sensor[t] | blob.pacemaker[t] | (g.rate_band_motor ? blob.motor[t] : 0);
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

void motor_apart_start(Blob& blob) {
    RateState& r = rate_of(blob);
    if (r.motor_target) {
        return;
    }
    const Genome& g = blob.genome;
    size_t sectors = (size_t)blob.sector_count;
    r.motor_bias = (int16_t*)std::calloc(sectors, sizeof(int16_t));
    r.motor_activity = (uint32_t*)std::calloc(sectors, sizeof(uint32_t));
    r.motor_target = (uint32_t*)std::calloc(sectors, sizeof(uint32_t));
    int motor_rate =
        g.motor_target_per_1024 > 0 ? g.motor_target_per_1024 : g.sector_target_per_1024;
    for (int s = 0; s < blob.sector_count; s++) {
        blob.sector_target[s] = 0;
    }
    // as birth counts a sector's target, with the motor cells in a count of their own
    for (int t = 0; t < blob.tile_count; t++) {
        uint32_t s = blob.tile_sector[t];
        uint64_t motor = blob.motor[t] & ~blob.sensor[t];
        int rest = TILE_NEURONS - __builtin_popcountll(blob.sensor[t] | motor);
        blob.sector_target[s] += (uint32_t)(rest * g.sector_target_per_1024 * 256 / 1024);
        r.motor_target[s] += (uint32_t)(__builtin_popcountll(motor) * motor_rate * 256 / 1024);
    }
    for (int s = 0; s < blob.sector_count; s++) {
        blob.sector_activity[s] = blob.sector_target[s];
        r.motor_activity[s] = r.motor_target[s];
    }
}

void regulate_motor_bias(Blob& blob, int sector, int spikes, int step) {
    const Genome& g = blob.genome;
    RateState& r = *blob.rate;
    int act = (int)r.motor_activity[sector];
    act += ((spikes << 8) - act) >> g.sector_smooth_shift;
    r.motor_activity[sector] = (uint32_t)(act < 0 ? 0 : act);
    if (!g.sector_reg_on || !step) {
        return;
    }
    // the sector's own rule: a dead band of an eighth of the target, then one step
    int target = (int)r.motor_target[sector];
    int band = target / 8;
    int top = g.motor_bias_max > 0 ? g.motor_bias_max : g.sector_bias_max;
    int bias = r.motor_bias[sector];
    if (act > target + band && bias < top) {
        bias++;
    } else if (act < target - band && bias > -top) {
        bias--;
    }
    r.motor_bias[sector] = (int16_t)bias;
}

uint64_t motor_apart_compare(const Blob& blob, int t, uint64_t would, int bias, int32_t* margin) {
    uint64_t motor = blob.motor[t];
    const RateState* r = blob.rate;
    int own = blob.genome.motor_apart == 3 && r && r->motor_bias
                  ? r->motor_bias[blob.tile_sector[t]]
                  : 0;
    int shift = bias - own;
    if (!motor || !shift) {
        return would;
    }
    // every cell of the tile compared again and the motor cells' answers kept: no branch
    // per cell
    alignas(64) int32_t flag[TILE_NEURONS];
    if (margin) {
        for (int b = 0; b < TILE_NEURONS; b++) {
            margin[b] += (int32_t)((motor >> b) & 1) * shift;
            flag[b] = margin[b] >= 0;
        }
    } else {
        const int16_t* __restrict pot = blob.potential + t * TILE_NEURONS;
        const int16_t* __restrict thr = blob.threshold + t * TILE_NEURONS;
        for (int b = 0; b < TILE_NEURONS; b++) {
            flag[b] = pot[b] >= thr[b] + own;
        }
    }
    uint64_t over = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        over |= (uint64_t)flag[b] << b;
    }
    return (would & ~motor) | (over & motor);
}

// the motor cells' own bias in a sector (motor_apart 3), for probes; 0 while there is none
int motor_apart_bias(const Blob& blob, int sector) {
    const RateState* r = blob.rate;
    return r && r->motor_bias ? r->motor_bias[sector] : 0;
}

void rate_free(Blob& blob) {
    if (!blob.rate) {
        return;
    }
    std::free(blob.rate->motor_bias);
    std::free(blob.rate->motor_activity);
    std::free(blob.rate->motor_target);
    std::free(blob.rate->count);
    std::free(blob.rate->slow);
    std::free(blob.rate->recip);
    std::free(blob.rate);
    blob.rate = nullptr;
}
