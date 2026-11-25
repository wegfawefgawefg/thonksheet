#include "blob/random_words.hpp"
#include "blob/tag_store.hpp"
#include "blob/tick_steps.hpp"

#include <cstddef>
// competitive learning (Rumelhart and Zipser 1985) in bits, with no reward anywhere
// (genome compete_on; docs/results-2026-11-25-compete.md). a hidden cell that fires is a
// winner: it moves toward what made it fire. its hidden counters (the store the tags use,
// tag_store.cpp) gain a step for every excitatory source its slots read active on that
// spike, and a silent source whose counter is above zero loses one by chance, so a counter
// tracks how often its source is on when the cell fires. the cell then wires its top
// counters at the counts it was born with, excitatory and strong alike (finding 38): the
// input stays the same size, so moving toward one thing means leaving another. which cells
// win is the sheet's own business: local inhibition, the tile's winners (tile_winners), and
// threshold regulation, which raises a cell's threshold as it wins and lowers it while it is
// silent (the handicap that keeps cells from dying or taking everything). compete_on 2 adds
// the losers: with tile_winners, a cell over threshold that lost its tile moves away from the
// input. inhibitory sources are never moved, and cells keep their born balance of them.

// which cells of tile t learn: hidden ones, excitatory unless compete_inh
static uint64_t learners(const Blob& blob, int t) {
    uint64_t not_hidden = blob.sensor[t] | blob.motor[t] | blob.pacemaker[t];
    uint64_t kind = blob.genome.compete_inh ? 0 : blob.inhibitory[t];
    return ~not_hidden & ~kind;
}

// the counters of one slot above zero
static uint64_t above_zero(const int8_t* c) {
    uint64_t w = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        w |= (uint64_t)(c[b] > 0) << b;
    }
    return w;
}

// one cell moves toward (or, losing, away from) the sources that were active on this tick
static void move_cell(Blob& blob, int n, int read_now, bool won) {
    const Genome& g = blob.genome;
    uint64_t self = 1ull << (n % TILE_NEURONS);
    const int8_t* row = blob.tag_evidence + (std::size_t)n * SLOTS * TILE_NEURONS;
    bool made = blob.tag_made[n] != 0;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        uint64_t usable = (k == 4 ? ~self : ~0ull) & ~blob.inhibitory[src];
        uint64_t active = slot_read(blob, read_now, n, k) & usable;
        if (!won) {
            tag_add(blob, n, k, 0, active);
            continue;
        }
        // a row not made yet starts at the connection strengths: wired is above zero
        int64_t w = blob_weight_index(n, k);
        uint64_t positive = made ? above_zero(row + k * TILE_NEURONS) : blob.w0[w] | blob.w1[w];
        uint64_t silent = usable & ~active & positive & sparse_word(blob.rng, g.compete_down_shift);
        tag_add(blob, n, k, active, silent);
        made = true;
    }
    tag_rewire(blob, n);
}

// with tile_winners: cells still over their bar that did not fire, and had not fired lately
// (so the cut, not a cooldown, kept them quiet)
static uint64_t losers(const Blob& blob, int t) {
    int bias = blob.sector_bias[blob.tile_sector[t]];
    uint64_t over = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        int n = t * TILE_NEURONS + b;
        over |= (uint64_t)(blob.potential[n] >= blob.threshold[n] + bias) << b;
    }
    return over & ~blob.recent[t];
}

void learn_compete(Blob& blob, int read_now) {
    const Genome& g = blob.genome;
    tag_store_make(blob);
    const uint64_t* fired = blob.spikes[blob.now];
    bool lose = g.compete_on == 2 && g.tile_winners > 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t learn = learners(blob, t);
        uint64_t won = fired[t] & learn & sparse_word(blob.rng, g.compete_shift);
        uint64_t lost = 0;
        if (lose && __builtin_popcountll(fired[t] & ~blob.sensor[t]) >= g.tile_winners) {
            lost = losers(blob, t) & learn & sparse_word(blob.rng, g.compete_shift);
        }
        while (won) {
            int b = __builtin_ctzll(won);
            won &= won - 1;
            move_cell(blob, t * TILE_NEURONS + b, read_now, true);
        }
        while (lost) {
            int b = __builtin_ctzll(lost);
            lost &= lost - 1;
            move_cell(blob, t * TILE_NEURONS + b, read_now, false);
        }
    }
}
