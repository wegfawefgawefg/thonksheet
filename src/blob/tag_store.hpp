#pragma once

#include "blob/blob.hpp"
#include "blob/tick_steps.hpp"

// hidden counters per possible source for every neuron, for the action-feedback tags
// (tag_store.cpp). core-only.

// make the counter store, the tag maps and the feedback scratch, once
void tag_store_make(Blob& blob);
// a neuron's counter row (SLOTS x 64), made from its wiring on first use
int8_t* tag_row(Blob& blob, int n);
// one counter step up for the sources in `up`, one down for those in `down`, in one slot
void tag_add(Blob& blob, int n, int slot, uint64_t up, uint64_t down);
// wire the top counters at the neuron's born connection count and born strong count
void tag_rewire(Blob& blob, int n);

// wire_deep_percent (docs/results-2026-11-25-hop-real.md): a hidden excitatory cell that
// hears no sensor cell, by a hash of the cell number. the sources it may not wire: the
// sensor cells of tile `src` (0 for every other cell)
inline uint64_t deep_blocked(const Blob& blob, int n, uint32_t src) {
    int p = blob.genome.wire_deep_percent;
    if (p <= 0 || src == (uint32_t)blob.tile_count) {
        return 0;
    }
    int t = n / TILE_NEURONS;
    uint64_t bit = 1ull << (n % TILE_NEURONS);
    if ((blob.sensor[t] | blob.motor[t] | blob.pacemaker[t] | blob.inhibitory[t]) & bit) {
        return 0;
    }
    uint64_t z = (uint64_t)n * 0x9e3779b97f4a7c15ull + 0x64656570ull;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    z ^= z >> 31;
    return (int)(z % 100) < p ? blob.sensor[src] : 0;
}
