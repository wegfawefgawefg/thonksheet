#pragma once

#include <cstdint>

#include "blob/genome.hpp"

// The blob's whole state, as flat arrays. See docs/design.md for what each mechanism is.
// This header is the contract between the core, the viewer and the tasks. Only the core
// owner edits it.

constexpr int TILE_NEURONS = 64; // 8x8 cells, one spike word
constexpr int LOCAL_SLOTS = 9;   // own tile and the 8 around it
constexpr int FAR_SLOTS = 2;
constexpr int SLOTS = LOCAL_SLOTS + FAR_SLOTS;
constexpr int HISTORY = 16; // spike maps kept, a power of two

struct BlobStats {
    uint64_t tick = 0;
    // counts for the latest tick
    uint32_t fired = 0;
    uint32_t surprise = 0; // sensor neurons whose prediction was wrong
    uint32_t grown = 0, pruned = 0, strengthened = 0, weakened = 0;
};

struct Blob {
    Genome genome;
    int tiles_w = 0, tiles_h = 0;
    int tile_count = 0;   // tiles_w * tiles_h
    int neuron_count = 0; // tile_count * 64
    int sector_count = 0;

    // spikes[h] is one spike map: tile_count + 1 words, the last word always zero.
    // spikes[now] is the latest tick; older maps are at (now - age) & (HISTORY - 1).
    uint64_t* spikes[HISTORY] = {};
    int now = 0;
    uint64_t* recent = nullptr; // OR of the last genome.recent_ticks maps, tile_count + 1

    // fixed at birth, one word per tile (tile_count + 1, last word zero)
    uint64_t* inhibitory = nullptr;
    uint64_t* sensor = nullptr;
    uint64_t* motor = nullptr;

    // written by the world before each tick: which sensor neurons fire, and which sensor
    // lines a task has attached (unattached lines are not learned from)
    uint64_t* sense_in = nullptr;
    uint64_t* sense_attached = nullptr;
    uint64_t* predicted = nullptr; // what the sensor neurons would have done by themselves

    // wiring. local_tile[tile * 9 + k] is the source tile of local slot k (tile_count means
    // the zero word). far_tile[neuron * 2 + k] likewise; far_delay is in ticks (2..15).
    uint32_t* local_tile = nullptr;
    uint32_t* far_tile = nullptr;
    uint8_t* far_delay = nullptr;

    // weights: two bit planes per slot, at w0[blob_weight_index(neuron, slot)], same for w1.
    // slot-major inside a tile: ((tile * SLOTS + slot) * 64 + bit), so the 64 neurons of a
    // tile sit side by side for each slot, which is what SIMD and GPUs want.
    // (w1, w0) = 00 none, 01 strength 1, 10 strength 4, 11 strength 5.
    uint64_t* w0 = nullptr;
    uint64_t* w1 = nullptr;

    // per neuron
    int16_t* potential = nullptr;
    int16_t* threshold = nullptr;
    uint8_t* leak_shift = nullptr;
    uint8_t* cooldown = nullptr;
    uint8_t* cooldown_len = nullptr;
    uint16_t* fan_in = nullptr; // connections this neuron has

    // per sector
    uint32_t* tile_sector = nullptr;    // which sector a tile is in, per tile
    int16_t* sector_bias = nullptr;     // added to thresholds in the sector
    uint32_t* sector_activity = nullptr; // smoothed spikes per tick, x256
    uint32_t* sector_surprise = nullptr; // smoothed wrong predictions per tick, x256

    // broadcasts
    int good_ticks = 0; // ticks left of the "that was good" signal
    int bad_ticks = 0;
    int surprise_high = 0; // 1 while surprise is above the genome's line

    // added by the core after the first draft of this header
    uint64_t* pacemaker = nullptr;     // fixed at birth, one word per tile (tile_count + 1)
    uint32_t* sector_target = nullptr; // wanted sector_activity, same units (x256)
    uint32_t surprise_level = 0; // smoothed wrong predictions per 1,024 attached lines, x256

    uint64_t rng[4] = {};
    BlobStats stats;
};

// birth and death
Blob* blob_birth(const Genome& genome, uint64_t seed);
void blob_free(Blob* blob);

// one tick: integrate, fire, regulate, learn. the world writes sense_in before calling.
void blob_tick(Blob& blob);

// broadcast good (+1) or bad (-1). a task also raises its good/bad sensor line.
void blob_reward(Blob& blob, int valence);

// cell (x, y) on the sheet to a neuron index, and back
inline int blob_neuron_at(const Blob& blob, int x, int y) {
    return ((y / 8) * blob.tiles_w + (x / 8)) * TILE_NEURONS + (y % 8) * 8 + (x % 8);
}
inline int blob_neuron_x(const Blob& blob, int neuron) {
    return ((neuron / TILE_NEURONS) % blob.tiles_w) * 8 + (neuron % TILE_NEURONS) % 8;
}
inline int blob_neuron_y(const Blob& blob, int neuron) {
    return ((neuron / TILE_NEURONS) / blob.tiles_w) * 8 + (neuron % TILE_NEURONS) / 8;
}

// did this neuron fire on the latest tick
inline bool blob_fired(const Blob& blob, int neuron) {
    return (blob.spikes[blob.now][neuron / TILE_NEURONS] >> (neuron % TILE_NEURONS)) & 1;
}

// make a sensor neuron fire on the next tick, and mark its line as attached
inline void blob_sense(Blob& blob, int neuron, bool on) {
    uint64_t bit = 1ull << (neuron % TILE_NEURONS);
    blob.sense_attached[neuron / TILE_NEURONS] |= bit;
    if (on) {
        blob.sense_in[neuron / TILE_NEURONS] |= bit;
    } else {
        blob.sense_in[neuron / TILE_NEURONS] &= ~bit;
    }
}

// where one neuron's mask word for one slot lives in w0 and w1
inline int64_t blob_weight_index(int neuron, int slot) {
    return ((int64_t)(neuron / TILE_NEURONS) * SLOTS + slot) * TILE_NEURONS +
           neuron % TILE_NEURONS;
}

// strength of the connection in one slot from source bit `bit`: 0, 1, 4 or 5
inline int blob_weight(const Blob& blob, int neuron, int slot, int bit) {
    int low = (blob.w0[blob_weight_index(neuron, slot)] >> bit) & 1;
    int high = (blob.w1[blob_weight_index(neuron, slot)] >> bit) & 1;
    return low + 4 * high;
}
