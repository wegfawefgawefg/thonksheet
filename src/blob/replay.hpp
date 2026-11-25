#pragma once

#include "blob/blob.hpp"

#include <cstdint>

// reverse replay (idea 62): what learn_replay.cpp keeps. one malloc block, this header first
// and the arrays right after it, so blob_free frees it with one call.

constexpr int REPLAY_DEPTH_MAX = 8;
constexpr int REPLAY_ROW = SLOTS * TILE_NEURONS; // counters per neuron, one byte each

struct ReplayState {
    int8_t* evidence;  // per neuron a signed counter per possible source (REPLAY_ROW)
    uint16_t* keep;    // per neuron the connections it keeps; 0 until its row is first used
    uint16_t* strong;  // per neuron its strong connections then (replay_strong_budget)
    uint64_t* born_w0; // the wiring at birth (the first tick), as the two weight planes
    uint64_t* born_w1;
    // walked spikes, HISTORY maps of tile_count + 1 words, indexed by age: [0] should have
    // been more active, [1] should have been less. `next` is the step being built
    uint64_t* walk[2];
    uint64_t* next[2];
    uint64_t* touched; // neurons whose counters moved in this walk, tile_count + 1
    int stopped;       // replay_until has passed
    // counts over the life (blobprobe replay prints them)
    int64_t walks;
    int64_t walked[REPLAY_DEPTH_MAX + 1]; // spikes walked at each depth; 0 is the motor cells
    int64_t conflicts;                    // spikes named with both signs; they cancel
    int64_t clipped;                      // slot reads older than the ring holds
    int64_t up, down;                     // counter steps
    int64_t rewired;                      // rewires that changed a connection
};

// the state, made on first use; the wiring as it is then is kept as the born wiring
ReplayState* replay_state(Blob& blob);

// one counter step up for the bits in `up`, one down for `down`, for neuron n's slot
void replay_counters_add(Blob& blob, ReplayState& r, int n, int slot, uint64_t up,
                         uint64_t down);

// wire neuron n from its counters: the `keep` highest, ties going to what is wired now;
// strength from the counter. true if a connection changed
bool replay_rewire(Blob& blob, ReplayState& r, int n);

// connections of neurons that are neither sensor nor motor cells, gained and lost since birth
void replay_changed(const Blob& blob, int64_t& gained, int64_t& lost);
