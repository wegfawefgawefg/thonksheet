#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"

// learn mode 2, a rival: random search on the weight planes. flip a few bits, live for a
// window, keep the flips if the reward summed over the window beat the running mean of
// past windows, otherwise flip them back. nothing else learns in this mode.

// flip one bit of one plane; flipping again undoes it
static void flip_bit(Blob& blob, int64_t word, int plane, int bit) {
    uint64_t* w = plane ? blob.w1 : blob.w0;
    w[word] ^= 1ull << bit;
}

// a neuron whose inputs can matter: not a sensor (a sensor fires as the world says), or
// with scope 1 a motor neuron
static int draw_target(Blob& blob) {
    for (;;) {
        int n = (int)rng_below(blob.rng, (uint32_t)blob.neuron_count);
        uint64_t bit = 1ull << (n % TILE_NEURONS);
        int t = n / TILE_NEURONS;
        uint64_t allowed = blob.genome.flip_scope ? blob.motor[t] : ~blob.sensor[t];
        if (allowed & bit) {
            return n;
        }
    }
}

static void start_trial(Blob& blob) {
    FlipTrial& f = blob.flips;
    f.count = blob.genome.flip_count;
    for (int i = 0; i < f.count; i++) {
        int n = draw_target(blob);
        int slot = (int)rng_below(blob.rng, SLOTS);
        int bit = (int)rng_below(blob.rng, TILE_NEURONS);
        if (slot == 4 && bit == n % TILE_NEURONS) {
            bit = (bit + 1) % TILE_NEURONS; // never wire a neuron to itself
        }
        f.word[i] = blob_weight_index(n, slot);
        f.bit[i] = (uint8_t)bit;
        f.plane[i] = (uint8_t)(rng_next(blob.rng) & 1);
        flip_bit(blob, f.word[i], f.plane[i], f.bit[i]);
    }
}

static void undo_trial(Blob& blob) {
    FlipTrial& f = blob.flips;
    for (int i = f.count - 1; i >= 0; i--) {
        flip_bit(blob, f.word[i], f.plane[i], f.bit[i]);
    }
}

void learn_flips(Blob& blob) {
    const Genome& g = blob.genome;
    FlipTrial& f = blob.flips;
    if (!f.started) {
        // the first window has no flips; it only measures
        f.started = 1;
        f.count = 0;
        f.score = 0;
        f.ticks_left = g.flip_window_ticks - 1;
        return;
    }
    if (f.ticks_left > 0) {
        f.ticks_left--;
        return;
    }
    // a window is over: judge the live trial against the mean of past windows
    int32_t score = f.score * 256;
    if (f.count == 0) {
        f.reference = score;
    } else {
        if (score > f.reference) {
            f.kept++;
        } else {
            undo_trial(blob);
            f.undone++;
        }
        f.reference += (score - f.reference) >> g.flip_average_shift;
    }
    f.score = 0;
    start_trial(blob);
    f.ticks_left = g.flip_window_ticks - 1;
}
