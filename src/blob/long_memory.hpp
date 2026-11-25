#pragma once

#include "blob/blob.hpp"

#include <cstdint>

// Long memory in the full engine (docs/results-2026-11-25-long-memory.md). Made on first
// use, so a blob with every switch at 0 never allocates it and runs exactly as before.
//
// integrator cells (genome hold_percent, hold_cap; catalogue 99, 180): a share of the
// excitatory inner neurons each count onsets of one sensor cell near them, with no leak,
// and fire on every tick while the count is above 0. good or bad clears them. Their spike
// word is set after the fire step, so the rest of the tick sees them as ordinary spikes.
//
// the freezer on the evidence readout (genome readout_lock_levels, _shift, _hold; idea
// 130): a counter pushed further at the end of its range deepens a lock; a move against a
// lock of depth d goes through with chance 2^-(hold * d) and opens it.

struct BlobLongMemory {
    // integrator cells
    uint64_t* cell = nullptr;   // mask per tile
    uint32_t* source = nullptr; // per neuron: the sensor cell it counts
    uint8_t* count = nullptr;   // per neuron
    // time cells (genome time_percent, time_levels; idea 131 as log-spaced time cells,
    // catalogue 62): per sensor cell, ticks since its last onset and since the one before;
    // a time cell fires while its source's count lies in its bin [2^(4+k), 2^(5+k)) ticks
    uint64_t* time_cell = nullptr; // mask per tile
    uint8_t* bin = nullptr;        // per neuron: k, and bit 7 for "the onset before last"
    uint32_t* since_last = nullptr;
    uint32_t* since_prev = nullptr;
    // freezer: lock depth per evidence counter, laid out as readout_evidence
    uint8_t* lock = nullptr;
};

// after the fire step wrote spikes[now]: integrator cells count and fire, and clear on a
// reward broadcast this tick
void long_memory_after_fire(Blob& blob);
// one evidence counter step with the freezer: returns the new value
int8_t long_memory_evidence_step(Blob& blob, int64_t at, int8_t value, int direction, int top);
// forgetting as room (genome readout_decay): on each update of a motor cell, that many of
// its counters, picked at random, step once toward 0, unless the freezer holds them
void long_memory_evidence_decay(Blob& blob, int n, int64_t row_at, int8_t* row);
void long_memory_free(Blob& blob);
