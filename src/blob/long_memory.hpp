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
    // freezer: lock depth per evidence counter, laid out as readout_evidence
    uint8_t* lock = nullptr;
};

// after the fire step wrote spikes[now]: integrator cells count and fire, and clear on a
// reward broadcast this tick
void long_memory_after_fire(Blob& blob);
// one evidence counter step with the freezer: returns the new value
int8_t long_memory_evidence_step(Blob& blob, int64_t at, int8_t value, int direction, int top);
void long_memory_free(Blob& blob);
