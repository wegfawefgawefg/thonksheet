#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

// lose-shift and its two neighbours (idea 53), all on motor cells only:
// - lose-shift: on bad, motor cells that just fired get their threshold raised a few steps.
//   threshold regulation lowers it again by chance, so it wears off slowly.
// - win-stay: on good, the same cells get their threshold lowered a few steps. regulation
//   raises it again as they fire, so it wears off fast.
// - bad cut: while bad is on, a strength-1 connection between recently co-active cells may
//   be removed, but only when its target is a recently fired motor cell.
// with every number at 0 nothing here runs and no random word is drawn.

constexpr int SHIFT_THRESHOLD_FLOOR = 1; // the floor regulate.cpp keeps

// motor cells of tile t that fired at least `fires` times (1..3) in the spike ring (the last
// 16 ticks): a saturating two-bit count kept as three bit planes
static uint64_t motor_fired_often(const Blob& blob, int t, int fires) {
    uint64_t motor = blob.motor[t];
    uint64_t once = 0, twice = 0, thrice = 0;
    for (int h = 0; h < HISTORY; h++) {
        uint64_t m = blob.spikes[h][t] & motor;
        thrice |= twice & m;
        twice |= once & m;
        once |= m;
    }
    return fires >= 3 ? thrice : (fires == 2 ? twice : once);
}

// move the thresholds of those cells by `steps` (positive: harder to fire)
static void shift_motor_thresholds(Blob& blob, int steps) {
    const Genome& g = blob.genome;
    for (int t = 0; t < blob.tile_count; t++) {
        if (!blob.motor[t]) {
            continue;
        }
        uint64_t cells = motor_fired_often(blob, t, g.lose_shift_fires);
        while (cells) {
            int n = t * TILE_NEURONS + __builtin_ctzll(cells);
            cells &= cells - 1;
            int v = blob.threshold[n] + steps;
            v = v > g.threshold_cap ? g.threshold_cap : v;
            v = v < SHIFT_THRESHOLD_FLOOR ? SHIFT_THRESHOLD_FLOOR : v;
            blob.threshold[n] = (int16_t)v;
        }
    }
}

// bad on strength-1 connections into recently fired motor cells, same chance as the bad
// broadcast (which itself never goes below strength 1)
static void bad_cut(Blob& blob, int boost) {
    const Genome& g = blob.genome;
    const uint64_t* recent = blob.recent;
    int shift = boosted(g.reward_shift, boost);
    uint32_t cut = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t post = recent[t] & blob.motor[t];
        while (post) {
            int n = t * TILE_NEURONS + __builtin_ctzll(post);
            post &= post - 1;
            uint32_t moved = 0;
            for (int k = 0; k < SLOTS; k++) {
                uint64_t& a0 = blob.w0[blob_weight_index(n, k)];
                uint64_t& a1 = blob.w1[blob_weight_index(n, k)];
                uint64_t pairs = recent[slot_source(blob, n, k)] & weak_bits(a0, a1);
                if (!pairs) {
                    continue;
                }
                moved += (uint32_t)__builtin_popcountll(
                    level_down(a0, a1, pairs & sparse_word(blob.rng, shift)));
            }
            if (moved) {
                blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
                cut += moved;
            }
        }
    }
    blob.stats.weakened += cut;
}

void learn_lose_shift(Blob& blob, int boost) {
    const Genome& g = blob.genome;
    // once per reward event: pending_valence is set by blob_reward and cleared after the tick
    if (blob.pending_valence < 0 && g.lose_shift_steps > 0) {
        shift_motor_thresholds(blob, g.lose_shift_steps);
    }
    if (blob.pending_valence > 0 && g.win_stay_steps > 0) {
        shift_motor_thresholds(blob, -g.win_stay_steps);
    }
    if (blob.bad_ticks > 0 && g.bad_on && g.bad_cut_on) {
        bad_cut(blob, boost);
    }
}
