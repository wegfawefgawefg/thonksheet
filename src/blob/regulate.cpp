#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/regulate_rate.hpp"
#include "blob/tick_steps.hpp"

// regulation: sector bias (fast) and per-neuron thresholds (slow).

constexpr int THRESHOLD_FLOOR = 1;

// smooth one sector's counts and move its bias toward the target. `step` 0: the bias stays
// this tick (sector_step_shift)
static void regulate_one_sector(Blob& blob, int sector, int spikes, int wrong, int step) {
    const Genome& g = blob.genome;
    int act = (int)blob.sector_activity[sector];
    act += ((spikes << 8) - act) >> g.sector_smooth_shift;
    blob.sector_activity[sector] = (uint32_t)(act < 0 ? 0 : act);
    int sur = (int)blob.sector_surprise[sector];
    sur += ((wrong << 8) - sur) >> g.sector_smooth_shift;
    blob.sector_surprise[sector] = (uint32_t)(sur < 0 ? 0 : sur);

    // idea 175: inhibition in proportion on top of a slow part (regulate_rate.cpp)
    if (g.sector_gain > 0) {
        if (g.sector_reg_on) {
            regulate_sector_gain(blob, sector, act, step);
        }
        return;
    }

    // a dead band of an eighth of the target either side, then one step
    if (!g.sector_reg_on || !step) {
        return;
    }
    int target = (int)blob.sector_target[sector];
    int band = target / 8;
    int bias = blob.sector_bias[sector];
    if (act > target + band && bias < g.sector_bias_max) {
        bias++;
    } else if (act < target - band && bias > -g.sector_bias_max) {
        bias--;
    }
    blob.sector_bias[sector] = (int16_t)bias;
}

// count this tick's spikes and wrong guesses per sector, then regulate each sector
static void regulate_sectors(Blob& blob) {
    const Genome& g = blob.genome;
    const uint64_t* fired = blob.spikes[blob.now];
    int st = g.sector_tiles;
    int sectors_w = (blob.tiles_w + st - 1) / st;
    uint32_t total_fired = 0, total_wrong = 0, attached = 0;
    // idea 172: a slow bias steps only on a drawn tick, one bit per sector (no draw while off)
    uint64_t steps = ~0ull;

    for (int s = 0; s < blob.sector_count; s++) {
        if (g.sector_step_shift > 0 && (s & 63) == 0) {
            steps = sparse_word(blob.rng, g.sector_step_shift);
        }
        int x0 = (s % sectors_w) * st, y0 = (s / sectors_w) * st;
        int spikes = 0, wrong = 0;
        for (int y = y0; y < y0 + st && y < blob.tiles_h; y++) {
            for (int x = x0; x < x0 + st && x < blob.tiles_w; x++) {
                int t = y * blob.tiles_w + x;
                uint64_t sensor = blob.sensor[t];
                uint64_t lines = sensor & blob.sense_attached[t];
                int w = __builtin_popcountll((blob.predicted[t] ^ blob.sense_in[t]) & lines);
                spikes += __builtin_popcountll(fired[t] & ~sensor);
                wrong += w;
                total_fired += (uint32_t)__builtin_popcountll(fired[t]);
                total_wrong += (uint32_t)w;
                attached += (uint32_t)__builtin_popcountll(lines);
            }
        }
        regulate_one_sector(blob, s, spikes, wrong, (int)((steps >> (s & 63)) & 1));
    }

    blob.stats.fired = total_fired;
    blob.stats.surprise = total_wrong;
    // surprise per 1,024 attached lines, smoothed
    int rate = attached ? (int)((uint64_t)total_wrong * 1024 * 256 / attached) : 0;
    int level = (int)blob.surprise_level;
    level += (rate - level) >> 4;
    blob.surprise_level = (uint32_t)(level < 0 ? 0 : level);
    blob.surprise_high = blob.surprise_level > (uint32_t)g.surprise_high_per_1024 * 256 ? 1 : 0;
}

// a neuron that fires gets a little harder to fire (chance 2^-up_shift); every neuron slowly
// gets easier (2^-down_shift a tick). sensors and pacemakers keep theirs. motor_only: only
// motor cells walk (the same draws)
static void regulate_thresholds(Blob& blob, int up_shift, int down_shift, bool motor_only) {
    const Genome& g = blob.genome;
    const uint64_t* fired = blob.spikes[blob.now];
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t fixed = blob.sensor[t] | blob.pacemaker[t] | (motor_only ? ~blob.motor[t] : 0);
        uint64_t up = fired[t] & ~fixed & sparse_word(blob.rng, up_shift);
        uint64_t down = ~fixed & sparse_word(blob.rng, down_shift) & ~up;
        while (up) {
            int n = t * TILE_NEURONS + __builtin_ctzll(up);
            up &= up - 1;
            if (blob.threshold[n] < g.threshold_cap) {
                blob.threshold[n]++;
            }
        }
        while (down) {
            int n = t * TILE_NEURONS + __builtin_ctzll(down);
            down &= down - 1;
            if (blob.threshold[n] > THRESHOLD_FLOOR) {
                blob.threshold[n]--;
            }
        }
    }
}

// idea 175: per-cell thresholds walk as above; after a critical period (threshold_reg_until
// ticks) the walk stops, goes on 2^threshold_late_shift slower, or hands over to the dead band
// (rate_band_shift, which runs from birth when there is no critical period). with
// rate_band_motor the motor cells keep the walk whatever the others do
static void regulate_cells(Blob& blob) {
    const Genome& g = blob.genome;
    bool late = g.threshold_reg_until > 0 && blob.stats.tick >= (uint64_t)g.threshold_reg_until;
    bool band = g.rate_band_shift > 0 && (late || g.threshold_reg_until <= 0);
    if (band) {
        regulate_band(blob);
    } else if (!late) {
        regulate_thresholds(blob, g.threshold_up_shift, g.threshold_down_shift, false);
        return;
    } else if (g.threshold_late_shift > 0) {
        regulate_thresholds(blob, g.threshold_up_shift + g.threshold_late_shift,
                            g.threshold_down_shift + g.threshold_late_shift, false);
        return;
    }
    if (g.rate_band_motor) {
        regulate_thresholds(blob, g.threshold_up_shift, g.threshold_down_shift, true);
    }
}

void tick_regulate(Blob& blob) {
    regulate_sectors(blob);
    if (blob.genome.threshold_reg_on) {
        regulate_cells(blob);
    }
}
