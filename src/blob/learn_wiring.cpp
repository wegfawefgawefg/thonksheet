#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

// churn that needs no reason: growth toward recently active neighbours, pruning of idle
// weak connections, and redrawing empty far slots. strong connections are never touched.

// the source tile of a slot (tile_count when there is none)
static inline uint32_t slot_source(const Blob& blob, int n, int k) {
    int t = n / TILE_NEURONS;
    if (k < LOCAL_SLOTS) {
        return blob.local_tile[t * LOCAL_SLOTS + k];
    }
    return blob.far_tile[n * FAR_SLOTS + k - LOCAL_SLOTS];
}

// one new weak connection from a recently active source that is not yet wired in
static bool grow_one(Blob& blob, int n) {
    if (blob.fan_in[n] >= blob.genome.fan_in_max) {
        return false;
    }
    uint64_t self = 1ull << (n % TILE_NEURONS);
    int start = (int)rng_below(blob.rng, SLOTS);
    for (int j = 0; j < SLOTS; j++) {
        int k = start + j < SLOTS ? start + j : start + j - SLOTS;
        uint32_t src = slot_source(blob, n, k);
        int64_t w = blob_weight_index(n, k);
        uint64_t candidates = blob.recent[src] & ~connected_bits(blob.w0[w], blob.w1[w]);
        if (k == 4) {
            candidates &= ~self;
        }
        if (k >= LOCAL_SLOTS) {
            candidates &= ~blob.inhibitory[src];
        }
        if (!candidates) {
            continue;
        }
        blob.w0[w] |= 1ull << pick_bit(blob.rng, candidates);
        blob.fan_in[n]++;
        return true;
    }
    return false;
}

// weak connections whose source was not active lose their level by chance
static int prune_idle(Blob& blob, int n, int bit_shift) {
    int removed = 0;
    for (int k = 0; k < SLOTS; k++) {
        int64_t w = blob_weight_index(n, k);
        uint32_t src = slot_source(blob, n, k);
        uint64_t idle = weak_bits(blob.w0[w], blob.w1[w]) & ~blob.recent[src];
        if (!idle) {
            continue;
        }
        uint64_t gone = idle & sparse_word(blob.rng, bit_shift);
        blob.w0[w] &= ~gone;
        removed += __builtin_popcountll(gone);
    }
    blob.fan_in[n] = (uint16_t)(blob.fan_in[n] - removed);
    return removed;
}

// a far slot that was pruned empty gets a new source tile
static void redraw_far(Blob& blob, int n) {
    int t = n / TILE_NEURONS;
    for (int k = 0; k < FAR_SLOTS; k++) {
        int f = n * FAR_SLOTS + k;
        int64_t w = blob_weight_index(n, LOCAL_SLOTS + k);
        if (blob.far_tile[f] == (uint32_t)blob.tile_count) {
            continue; // born without this slot
        }
        if (connected_bits(blob.w0[w], blob.w1[w])) {
            continue;
        }
        draw_far_tile(blob, t, &blob.far_tile[f], &blob.far_delay[f]);
    }
}

void learn_wiring(Blob& blob, int boost) {
    const Genome& g = blob.genome;
    const uint64_t* fired = blob.spikes[blob.now];
    int grow_shift = boosted(g.grow_shift, boost);
    int prune_shift = boosted(g.prune_shift, boost);
    int redraw_shift = boosted(g.far_redraw_shift, boost);
    uint32_t grown = 0, pruned = 0;

    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t grow = fired[t] & sparse_word(blob.rng, grow_shift);
        uint64_t prune = fired[t] & sparse_word(blob.rng, prune_shift);
        uint64_t redraw = sparse_word(blob.rng, redraw_shift);
        while (grow) {
            int n = t * TILE_NEURONS + __builtin_ctzll(grow);
            grow &= grow - 1;
            grown += grow_one(blob, n) ? 1 : 0;
        }
        while (prune) {
            int n = t * TILE_NEURONS + __builtin_ctzll(prune);
            prune &= prune - 1;
            pruned += (uint32_t)prune_idle(blob, n, g.prune_bit_shift);
        }
        while (redraw) {
            int n = t * TILE_NEURONS + __builtin_ctzll(redraw);
            redraw &= redraw - 1;
            redraw_far(blob, n);
        }
    }
    blob.stats.grown = grown;
    blob.stats.pruned = pruned;
}
