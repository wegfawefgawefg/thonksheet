#pragma once

// How well a cue can be read from activity, tile by tile. Shared by blobprobe carry and
// blobrun --reach. One tile's sensor cells are split into cue A (every other cell) and cue
// B (the rest). Per neuron: spikes per block under A and under B; a tile's number combines
// its neurons' gaps between the two means, in units of their block-to-block spread, less
// what chance alone would give. 1.0 is easy to read, under CARRY_LOST it is lost.

#include "blob/blob.hpp"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

constexpr double CARRY_LOST = 0.3;
constexpr int CARRY_MAX_DISTANCE = 16;

struct CarryMeasure {
    int cue_x = 0, cue_y = 0, cue_tile = 0;
    std::vector<double> sum, squares; // per cue, per neuron: spikes per block and squared
    std::vector<int> count;           // spikes per neuron in the block under way
    int64_t seen[2] = {0, 0};         // blocks measured under each cue
};

inline void carry_begin(CarryMeasure& m, const Blob& blob, int cue_x, int cue_y) {
    m = CarryMeasure{};
    m.cue_x = cue_x;
    m.cue_y = cue_y;
    m.cue_tile = cue_y * blob.tiles_w + cue_x;
    m.sum.assign(2 * (size_t)blob.neuron_count, 0.0);
    m.squares.assign(2 * (size_t)blob.neuron_count, 0.0);
    m.count.assign((size_t)blob.neuron_count, 0);
}

// cue 0 is every other sensor cell of the tile, cue 1 the rest
inline void carry_set_cue(Blob& blob, int cue_tile, int which) {
    uint64_t cells = blob.sensor[cue_tile];
    int index = 0;
    while (cells) {
        int bit = __builtin_ctzll(cells);
        cells &= cells - 1;
        blob_sense(blob, cue_tile * TILE_NEURONS + bit, (index & 1) == which);
        index++;
    }
}

inline void carry_block_start(CarryMeasure& m) {
    for (int& c : m.count) {
        c = 0;
    }
}

// the latest tick's spikes; sensor cells are driven by the world, so they do not count
inline void carry_count_tick(CarryMeasure& m, const Blob& blob) {
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t spikes = blob.spikes[blob.now][t] & ~blob.sensor[t];
        while (spikes) {
            m.count[(size_t)(t * TILE_NEURONS + __builtin_ctzll(spikes))]++;
            spikes &= spikes - 1;
        }
    }
}

inline void carry_block_end(CarryMeasure& m, const Blob& blob, int which) {
    int neurons = blob.neuron_count;
    m.seen[which]++;
    for (int n = 0; n < neurons; n++) {
        double c = m.count[(size_t)n];
        m.sum[(size_t)(which * neurons + n)] += c;
        m.squares[(size_t)(which * neurons + n)] += c * c;
    }
}

// readable[d] for each distance d in tiles from the cue tile (as far as the sheet goes);
// returns how many distances were filled. reach is the farthest still readable.
inline int carry_readable(const CarryMeasure& m, const Blob& blob, double* readable, int& reach) {
    int neurons = blob.neuron_count;
    double sum_at[CARRY_MAX_DISTANCE] = {};
    int tiles_at[CARRY_MAX_DISTANCE] = {};
    double n0 = (double)(m.seen[0] ? m.seen[0] : 1), n1 = (double)(m.seen[1] ? m.seen[1] : 1);
    double chance = 1.0 / n0 + 1.0 / n1;
    for (int t = 0; t < blob.tile_count; t++) {
        double total = 0;
        for (int k = 0; k < TILE_NEURONS; k++) {
            int n = t * TILE_NEURONS + k;
            double m0 = m.sum[(size_t)n] / n0, m1 = m.sum[(size_t)(neurons + n)] / n1;
            double v0 = m.squares[(size_t)n] / n0 - m0 * m0;
            double v1 = m.squares[(size_t)(neurons + n)] / n1 - m1 * m1;
            double noise = (v0 + v1) / 2.0;
            if (noise > 0) {
                total += (m1 - m0) * (m1 - m0) / noise - chance;
            }
        }
        double gap = total > 0 ? std::sqrt(total) : 0.0;
        int dx = std::abs(t % blob.tiles_w - m.cue_x), dy = std::abs(t / blob.tiles_w - m.cue_y);
        int distance = dx > dy ? dx : dy;
        if (distance < CARRY_MAX_DISTANCE) {
            sum_at[distance] += gap;
            tiles_at[distance]++;
        }
    }
    reach = 0;
    int filled = 0;
    for (int d = 0; d < CARRY_MAX_DISTANCE && tiles_at[d] > 0; d++) {
        readable[d] = sum_at[d] / tiles_at[d];
        if (readable[d] >= CARRY_LOST) {
            reach = d;
        }
        filled = d + 1;
    }
    return filled;
}
