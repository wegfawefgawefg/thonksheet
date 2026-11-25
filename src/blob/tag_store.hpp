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
// the same, faster with guesses: pick[3 j] is a guess at where pick j's line is (j: excitatory
// wired, excitatory strong, inhibitory wired, inhibitory strong) and comes back as the line;
// pick[3 j + 1] is 1 if sources at the line were left out, pick[3 j + 2] 1 if it drew chance.
// the wiring and the chance drawn are tag_rewire's whatever the guesses
void tag_rewire_hinted(Blob& blob, int n, int8_t* pick);
