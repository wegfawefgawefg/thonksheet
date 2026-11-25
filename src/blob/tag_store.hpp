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
