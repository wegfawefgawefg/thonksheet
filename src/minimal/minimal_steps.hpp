#pragma once

#include "minimal/minimal.hpp"
#include "minimal/minimal_keyed.hpp"

#include <cstdint>

// engine-only helpers, shared by the tick, the reward and birth.
//
// learning names a spike by its age (0 = the newest map). "the sources active just before
// it" are, for slot k, the map `delay` ticks older than the spike, at the slot's source tile:
// exactly what the neuron read when it fired.

// rng_mode 1: the next words come from this draw site's own stream (minimal_keyed.hpp)
inline void min_site(MinimalBlob& m, uint32_t site, uint32_t where, uint32_t age) {
    min_reseed(m.rng, m.mg.rng_mode, m.rng_key, m.shell.stats.tick, site, where, age);
}

// the mask (and delay) word of neuron n, slot k
inline int64_t min_index(const MinimalBlob& m, int n, int k) {
    return ((int64_t)(n / TILE_NEURONS) * m.slots + k) * TILE_NEURONS + n % TILE_NEURONS;
}

// the tile slot k of neuron n reads (tile_count = none, the zero word)
inline uint32_t min_source_tile(const MinimalBlob& m, int n, int k) {
    int t = n / TILE_NEURONS;
    if (k < MIN_SLOTS) {
        return m.local_tile[t * MIN_SLOTS + k];
    }
    int far_slots = m.slots - MIN_SLOTS;
    return m.far_tile[((int64_t)t * far_slots + (k - MIN_SLOTS)) * TILE_NEURONS + n % TILE_NEURONS];
}

// ticks back slot k of neuron n reads
inline int min_delay(const MinimalBlob& m, int n, int k) {
    return m.delay ? m.delay[min_index(m, n, k)] : 1;
}

// what slot k of neuron n heard when the neuron fired `age` ticks ago
inline uint64_t min_heard(const MinimalBlob& m, int age, int n, int k) {
    int index = (m.now - age - min_delay(m, n, k)) & (MIN_HISTORY - 1);
    return m.spikes[(size_t)index * m.stride + min_source_tile(m, n, k)];
}

// sources slot k may ever hold: none off the sheet, never the neuron itself, and on a far
// slot only excitatory senders
inline uint64_t min_allowed(const MinimalBlob& m, int n, int k) {
    uint32_t src = min_source_tile(m, n, k);
    if (src == (uint32_t)m.shell.tile_count) {
        return 0;
    }
    if (k >= MIN_SLOTS) {
        return ~m.shell.inhibitory[src];
    }
    return k == 4 ? ~(1ull << (n % TILE_NEURONS)) : ~0ull;
}

// one set bit, uniformly, over `slots` words. false if all are empty.
bool min_pick_one(uint64_t* rng, const uint64_t* words, int slots, int& slot, int& bit);

// connect n to one source it heard at its spike `age` ticks ago that is not yet wired;
// under the budget, pay with one connection whose source was quiet then. true if it connected.
bool min_gain_active(MinimalBlob& m, int n, int age);
// disconnect n from one connected source it heard at that spike; under the budget, take a
// random unwired source that was quiet then in its place.
bool min_lose_active(MinimalBlob& m, int n, int age);
// drop_mode 1: mark the connections heard by each neuron that fired on the newest tick
void min_mark_used(MinimalBlob& m, const uint64_t* fired);

// evidence counters (minimal_evidence.cpp)
// allocate and set them from the birth wiring: wired sources ev_init above zero
void min_evidence_birth(MinimalBlob& m);
// a reward better (or worse) than average, at chance shift: move the counters, read the masks
void min_evidence_reward(MinimalBlob& m, bool better, int shift);
// co-activity on counter neuron n (ev_cowire 1): one source heard last tick goes up one
void min_evidence_cowire(MinimalBlob& m, int n);
// counters at their lowest and highest value, over every counter neuron (for the probes)
void min_evidence_ends(const MinimalBlob& m, double& low, double& high);
