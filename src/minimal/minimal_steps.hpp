#pragma once

#include "minimal/minimal.hpp"

#include <cstdint>

// engine-only helpers, shared by the tick, the reward and birth.

// what neuron n hears in slot k: the word of `map` at the slot's source tile
inline uint64_t min_source(const MinimalBlob& m, const uint64_t* map, int n, int k) {
    return map[m.local_tile[(n / TILE_NEURONS) * MIN_SLOTS + k]];
}

// the mask word of neuron n, slot k
inline int64_t min_index(int n, int k) {
    return ((int64_t)(n / TILE_NEURONS) * MIN_SLOTS + k) * TILE_NEURONS + n % TILE_NEURONS;
}

// sources slot k may ever hold: none off the sheet, never the neuron itself
inline uint64_t min_allowed(const MinimalBlob& m, int n, int k) {
    int t = n / TILE_NEURONS;
    if (m.local_tile[t * MIN_SLOTS + k] == (uint32_t)m.shell.tile_count) {
        return 0;
    }
    return k == 4 ? ~(1ull << (n % TILE_NEURONS)) : ~0ull;
}

// connect n to one source that was active in `before` and is not yet wired; under the
// budget, pay with one connection whose source was not active. true if it connected.
bool min_gain_active(MinimalBlob& m, int n, const uint64_t* before);
// disconnect n from one connected source that was active in `before`; under the budget,
// take a random unwired source that was not active in its place.
bool min_lose_active(MinimalBlob& m, int n, const uint64_t* before);
// drop_mode 1: mark the connections that were active just before each neuron that fired
void min_mark_used(MinimalBlob& m, const uint64_t* before, const uint64_t* fired);
