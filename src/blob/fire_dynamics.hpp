#pragma once

#include "blob/blob.hpp"

#include <cstdint>

// ideas 66-75: what a connection does over a few ticks and what a neuron computes. core-only.
// every piece is off at genome 0 and then the fire step takes its plain path, untouched.

// which pieces are on, read from the genome once per tick
struct DynamicsOn {
    bool sent = false;   // 66, 67: the hot loop reads blob.sent in place of the spike ring
    bool warm = false;   // 67: a second pass over the slots with blob.warm
    bool split = false;  // 72-74: per-slot pieces
    bool neuron = false; // 68-71, 75: the leak-add-compare step with extras
};
DynamicsOn dynamics_on(const Genome& g);

// birth: clamp the numbers and make the arrays the genome asks for
void dynamics_birth(Blob& blob, uint64_t seed);

// 72-74, filled slot by slot while a tile's drive is added up
struct SlotSplit {
    alignas(64) int32_t best[TILE_NEURONS];    // largest excitation (with bonus) of one slot
    alignas(64) int32_t inhibit[TILE_NEURONS]; // gain x inhibition over all slots
    alignas(64) uint32_t active[TILE_NEURONS]; // bit d: a slot read at delay d was active
    alignas(64) int32_t veto[TILE_NEURONS];    // 1 for this tile's veto neurons
};

// one slot, each neuron reading its own word s[b]; inh marks the inhibitory senders.
// branch-free so it vectorizes like the plain slot add
static inline void split_add_slot(int32_t* __restrict drive, SlotSplit& x,
                                  const uint64_t* __restrict s, uint64_t inh,
                                  const uint64_t* __restrict a0, const uint64_t* __restrict a1,
                                  const uint8_t* __restrict delay, int gain, int bt, int bonus,
                                  int sl) {
    int32_t* __restrict best = x.best;
    int32_t* __restrict inhibit_sum = x.inhibit;
    uint32_t* __restrict active = x.active;
    const int32_t* __restrict veto = x.veto;
    for (int b = 0; b < TILE_NEURONS; b++) {
        uint64_t e = s[b] & ~inh, i = s[b] & inh;
        int excite = __builtin_popcountll(e & a0[b]) + (__builtin_popcountll(e & a1[b]) << sl);
        int inhibit = __builtin_popcountll(i & a0[b]) + (__builtin_popcountll(i & a1[b]) << sl);
        int open = 1 - (veto[b] & (inhibit > 0)); // 0: vetoed
        int strong = excite >= bt;
        int up = open * (excite + strong * bonus);
        drive[b] += up - gain * inhibit;
        best[b] = up > best[b] ? up : best[b];
        inhibit_sum[b] += gain * inhibit;
        active[b] |= (uint32_t)(open & strong) << delay[b];
    }
}

void split_start(const Blob& blob, int t, SlotSplit& x);
// any-branch neurons take their best slot; returns the neurons allowed to fire by
// coincidence (all but coincidence neurons that lack it)
uint64_t split_finish(const Blob& blob, int t, const SlotSplit& x, int32_t* drive);

// 68-71: leak, add and compare with the extras; writes how far each neuron is over its
// threshold into margin (75 ranks by it) and returns the neurons over it
uint64_t dynamics_compare(Blob& blob, int t, const int32_t* drive, int bias, int32_t* margin);
// after the spike: tiring and the up state
void dynamics_after(Blob& blob, int t, uint64_t fired);
// 75: keep at most k of these spikes, those furthest over threshold
uint64_t dynamics_winners(uint64_t spikes, const int32_t* margin, int k);
// 66, 67: what the new map (ring index `next`) sends, once the whole map is made
void dynamics_sent(Blob& blob, int next);
