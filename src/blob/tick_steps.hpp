#pragma once

#include <cstdint>

#include "blob/blob.hpp"

// the parts of one tick, in the order blob_tick runs them. core-only, not part of the
// contract with the viewer and the tasks.

// integrate and fire: reads spikes[now] (and older maps for far slots), writes the new
// spike map into `out` and the sensor guesses into blob.predicted.
void tick_fire(Blob& blob, uint64_t* out);

// sector bias and per-neuron thresholds, from the newest map (spikes[now])
void tick_regulate(Blob& blob);

// sensor prediction learning. `read_now` is the ring index the fire step read from.
void learn_predict(Blob& blob, int read_now, int boost);

// good and bad broadcast on recently co-active pairs
void learn_reward(Blob& blob, int valence, int boost);

// growth toward recent neighbours, pruning of idle weak links, far slot redraw
void learn_wiring(Blob& blob, int boost);

// connections a neuron has, over all slots
int count_fan_in(const Blob& blob, int neuron);

// draw a far source tile and its delay for a neuron in `tile`; false if none fits
bool draw_far_tile(Blob& blob, int tile, uint32_t* far_tile, uint8_t* far_delay);

// a shift made smaller while surprise is high, never below 0
inline int boosted(int shift, int boost) {
    return shift > boost ? shift - boost : 0;
}
