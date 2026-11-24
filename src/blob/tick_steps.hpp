#pragma once

#include "blob/blob.hpp"

#include <cstdint>

// the parts of one tick, in the order blob_tick runs them. core-only, not part of the
// contract with the viewer and the tasks.

// integrate and fire: reads spikes[now] (and older maps for far slots), writes the new
// spike map into `out` and the sensor guesses into blob.predicted.
void tick_fire(Blob& blob, uint64_t* out);

// sector bias and per-neuron thresholds, from the newest map (spikes[now])
void tick_regulate(Blob& blob);

// sensor prediction learning. `read_now` is the ring index the fire step read from.
void learn_predict(Blob& blob, int read_now, int boost);

// good and bad broadcast on recently co-active pairs. motor_only: only connections into
// motor neurons move (learn mode 1, the readout rival).
void learn_reward(Blob& blob, int valence, int boost, bool motor_only);

// growth toward recent neighbours, pruning of idle weak links, far slot redraw
void learn_wiring(Blob& blob, int boost, bool motor_only);

// learn mode 2: random weight-bit flips kept when the reward over a window beat the average
void learn_flips(Blob& blob);

// learn modes 3, 4: a reward graded against the running mean moves the recently active
// inputs of neurons whose recent spike was noise
void learn_noise(Blob& blob, int valence);

// connections a neuron has, over all slots
int count_fan_in(const Blob& blob, int neuron);

// draw a far source tile and its delay for a neuron in `tile`; false if none fits
bool draw_far_tile(Blob& blob, int tile, uint32_t* far_tile, uint8_t* far_delay);

// the source tile of a slot (tile_count when there is none)
inline uint32_t slot_source(const Blob& blob, int n, int k) {
    if (k < LOCAL_SLOTS) {
        return blob.local_tile[(n / TILE_NEURONS) * LOCAL_SLOTS + k];
    }
    return blob.far_tile[n * FAR_SLOTS + k - LOCAL_SLOTS];
}

// the source word a slot read on the tick that read ring index `read_now` as its newest map
inline uint64_t slot_read(const Blob& blob, int read_now, int n, int k) {
    int delay = blob.slot_delay[blob_weight_index(n, k)];
    return blob.spikes[(read_now + 1 - delay) & (HISTORY - 1)][slot_source(blob, n, k)];
}

// a shift made smaller while surprise is high, never below 0
inline int boosted(int shift, int boost) {
    return shift > boost ? shift - boost : 0;
}
