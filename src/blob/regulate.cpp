#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"

// regulation: sector bias (fast) and per-neuron thresholds (slow).

constexpr int THRESHOLD_FLOOR = 1;

// smooth one sector's counts and move its bias toward the target
static void regulate_one_sector(Blob& blob, int sector, int spikes, int wrong) {
    const Genome& g = blob.genome;
    int act = (int)blob.sector_activity[sector];
    act += ((spikes << 8) - act) >> g.sector_smooth_shift;
    blob.sector_activity[sector] = (uint32_t)(act < 0 ? 0 : act);
    int sur = (int)blob.sector_surprise[sector];
    sur += ((wrong << 8) - sur) >> g.sector_smooth_shift;
    blob.sector_surprise[sector] = (uint32_t)(sur < 0 ? 0 : sur);

    // a dead band of an eighth of the target either side, then one step
    if (!g.sector_reg_on) {
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

    for (int s = 0; s < blob.sector_count; s++) {
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
        regulate_one_sector(blob, s, spikes, wrong);
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

// a neuron that fires gets a little harder to fire; every neuron slowly gets easier.
// sensors and pacemakers keep theirs.
static void regulate_thresholds(Blob& blob) {
    const Genome& g = blob.genome;
    const uint64_t* fired = blob.spikes[blob.now];
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t fixed = blob.sensor[t] | blob.pacemaker[t];
        uint64_t up = fired[t] & ~fixed & sparse_word(blob.rng, g.threshold_up_shift);
        uint64_t down = ~fixed & sparse_word(blob.rng, g.threshold_down_shift) & ~up;
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

void tick_regulate(Blob& blob) {
    regulate_sectors(blob);
    if (blob.genome.threshold_reg_on) {
        regulate_thresholds(blob);
    }
}
