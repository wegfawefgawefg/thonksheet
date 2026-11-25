#include "blob/random_words.hpp"
#include "blob/tag_store.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
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
//
// the read-off (docs/results-2026-11-25-compete2.md). re-picking the top counters costs four
// passes over a cell's 704 counters, and almost every update leaves the pick as it was. so
// each learner keeps 4 bytes of bounds on its excitatory counters (lowest wired, highest
// unwired, lowest strong, highest weak) and re-picks only when a step crosses one, as the
// one-hop rule does. the bounds are kept loose in the safe direction, so the wiring is the
// re-pick's own at every update. a re-pick that changes nothing still draws chance for its
// ties; the cheap path draws the same, so a life is byte-identical either way.
// compete_full 1 re-picks on every update (the old read-off); 2 does that and stops the
// program if the bounds ever missed a change.
//
// compete_churn 1, the churn control: a winner moves as many counters up, toward as many
// excitatory sources drawn at random among those it may wire, instead of those that were on.

struct CompeteState {
    int8_t* bounds = nullptr; // per neuron: lowest wired, highest unwired, lowest strong,
                              // highest weak (excitatory sources), and the draws a re-pick
                              // that changes nothing takes (STALE: the bounds are not made)
    uint64_t* sum = nullptr;  // per neuron: a hash of its wiring when the bounds were made
};

enum { LOW_WIRED = 0, HIGH_UNWIRED = 1, LOW_STRONG = 2, HIGH_WEAK = 3, DRAWS = 4, BOUNDS = 5 };
constexpr int8_t STALE = -1;

static CompeteState& compete_make(Blob& blob) {
    if (!blob.compete) {
        CompeteState* c = (CompeteState*)std::calloc(1, sizeof(CompeteState));
        size_t n = (size_t)blob.neuron_count;
        c->bounds = (int8_t*)std::malloc(n * BOUNDS);
        c->sum = (uint64_t*)std::calloc(n, sizeof(uint64_t));
        for (size_t i = 0; i < n; i++) {
            c->bounds[i * BOUNDS + DRAWS] = STALE;
        }
        blob.compete = c;
    }
    return *blob.compete;
}

void compete_free(Blob& blob) {
    if (!blob.compete) {
        return;
    }
    std::free(blob.compete->bounds);
    std::free(blob.compete->sum);
    std::free(blob.compete);
    blob.compete = nullptr;
}

void compete_stale(Blob& blob, int n) {
    blob.compete->bounds[(size_t)n * BOUNDS + DRAWS] = STALE;
}

// which cells of tile t learn: hidden ones, excitatory unless compete_inh
static uint64_t learners(const Blob& blob, int t) {
    uint64_t not_hidden = blob.sensor[t] | blob.motor[t] | blob.pacemaker[t];
    uint64_t kind = blob.genome.compete_inh ? 0 : blob.inhibitory[t];
    return ~not_hidden & ~kind;
}

// the sources of slot k a cell may move: excitatory, not itself (as tag_rewire's)
static uint64_t usable(const Blob& blob, int n, int k, uint32_t src) {
    uint64_t self = 1ull << (n % TILE_NEURONS);
    return (k == 4 ? ~self : ~0ull) & ~blob.inhibitory[src];
}

// a hash of a cell's wiring and sources: anyone else who rewired it makes the bounds stale
static uint64_t wiring_sum(const Blob& blob, int n) {
    uint64_t h = 0x9e3779b97f4a7c15ull;
    for (int k = 0; k < SLOTS; k++) {
        int64_t w = blob_weight_index(n, k);
        h = (h ^ blob.w0[w]) * 0xbf58476d1ce4e5b9ull;
        h = (h ^ blob.w1[w] ^ ((uint64_t)slot_source(blob, n, k) << 32)) * 0x94d049bb133111ebull;
    }
    return h ^ (h >> 31);
}

// exact bounds after a re-pick, and the draws a re-pick that changes nothing takes: one per
// pick (wired, then strong, for each kind) that wants some and has enough sources for them
static void bounds_make(Blob& blob, int n) {
    int8_t* bd = blob.compete->bounds + (size_t)n * BOUNDS;
    const int8_t* c = tag_row(blob, n);
    int8_t b[4] = {127, -128, 127, -128};
    int usable_count[2] = {0, 0};
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        uint64_t kind = k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
        uint64_t u = usable(blob, n, k, src) | kind;
        u &= k == 4 ? ~(1ull << (n % TILE_NEURONS)) : ~0ull;
        usable_count[0] += __builtin_popcountll(u & ~kind);
        usable_count[1] += __builtin_popcountll(u & kind);
        int64_t w = blob_weight_index(n, k);
        uint64_t wired = connected_bits(blob.w0[w], blob.w1[w]), strong = blob.w1[w];
        for (uint64_t m = u & ~kind; m; m &= m - 1) {
            int i = __builtin_ctzll(m);
            int8_t v = c[k * TILE_NEURONS + i];
            if (!((wired >> i) & 1)) {
                b[HIGH_UNWIRED] = v > b[HIGH_UNWIRED] ? v : b[HIGH_UNWIRED];
                continue;
            }
            b[LOW_WIRED] = v < b[LOW_WIRED] ? v : b[LOW_WIRED];
            int at = (strong >> i) & 1 ? LOW_STRONG : HIGH_WEAK;
            b[at] = at == LOW_STRONG ? (v < b[at] ? v : b[at]) : (v > b[at] ? v : b[at]);
        }
    }
    int draws = 0;
    for (int i = 0; i < 2; i++) {
        int keep = blob.tag_keep[2 * n + i], strong = blob.tag_strong[2 * n + i];
        int wired = keep < usable_count[i] ? keep : usable_count[i];
        draws += (keep > 0 && keep <= usable_count[i]) + (strong > 0 && strong <= wired);
    }
    for (int i = 0; i < 4; i++) {
        bd[i] = b[i];
    }
    bd[DRAWS] = (int8_t)draws;
    blob.compete->sum[n] = wiring_sum(blob, n);
}

// one counter step for the bits of `up` and `down` in slot k; true if a step may change what
// the cell wires (an unwired counter past the lowest wired, a wired one under the highest
// unwired, the same for strong and weak)
static bool step_slot(Blob& blob, int n, int k, uint64_t up, uint64_t down, int8_t* bd) {
    int8_t* c = tag_row(blob, n) + k * TILE_NEURONS;
    int bits = blob.genome.tag_bits;
    int top = (1 << ((bits < 2 ? 2 : (bits > 8 ? 8 : bits)) - 1)) - 1;
    int64_t w = blob_weight_index(n, k);
    uint64_t wired = connected_bits(blob.w0[w], blob.w1[w]), strong = blob.w1[w];
    bool cross = false;
    for (; up; up &= up - 1) {
        int b = __builtin_ctzll(up);
        if (c[b] >= top) {
            continue;
        }
        int8_t v = ++c[b];
        if (!((wired >> b) & 1)) {
            cross |= v > bd[LOW_WIRED];
            bd[HIGH_UNWIRED] = v > bd[HIGH_UNWIRED] ? v : bd[HIGH_UNWIRED];
        } else if (!((strong >> b) & 1)) {
            cross |= v > bd[LOW_STRONG];
            bd[HIGH_WEAK] = v > bd[HIGH_WEAK] ? v : bd[HIGH_WEAK];
        }
    }
    for (; down; down &= down - 1) {
        int b = __builtin_ctzll(down);
        if (c[b] <= -top) {
            continue;
        }
        int8_t v = --c[b];
        if (!((wired >> b) & 1)) {
            continue;
        }
        cross |= v < bd[HIGH_UNWIRED];
        bd[LOW_WIRED] = v < bd[LOW_WIRED] ? v : bd[LOW_WIRED];
        if ((strong >> b) & 1) {
            cross |= v < bd[HIGH_WEAK];
            bd[LOW_STRONG] = v < bd[LOW_STRONG] ? v : bd[LOW_STRONG];
        }
    }
    return cross;
}

// the churn control: `count` sources drawn at random, none twice, among a cell's usable ones
static void churn_draw(Blob& blob, const uint64_t* u, int count, uint64_t* out) {
    int total = 0;
    for (int k = 0; k < SLOTS; k++) {
        out[k] = 0;
        total += __builtin_popcountll(u[k]);
    }
    count = count < total ? count : total;
    for (int got = 0; got < count;) {
        int r = (int)rng_below(blob.rng, (uint32_t)total), k = 0;
        while (r >= __builtin_popcountll(u[k])) {
            r -= __builtin_popcountll(u[k++]);
        }
        uint64_t m = u[k];
        for (; r > 0; r--) {
            m &= m - 1;
        }
        uint64_t bit = m & (~m + 1);
        got += (out[k] & bit) == 0;
        out[k] |= bit;
    }
}

// the old read-off checked: the re-pick must keep the wiring whenever the bounds said so
static void check_repick(Blob& blob, int n, bool cross) {
    uint64_t before = wiring_sum(blob, n);
    tag_rewire(blob, n);
    if (!cross && wiring_sum(blob, n) != before) {
        std::fprintf(stderr, "compete_full 2: neuron %d rewired where the bounds said no\n", n);
        std::abort();
    }
}

// one cell moves toward (or, losing, away from) the sources that were active on this tick
static void move_cell(Blob& blob, int n, int read_now, bool won) {
    const Genome& g = blob.genome;
    CompeteState& cs = *blob.compete;
    int8_t* bd = cs.bounds + (size_t)n * BOUNDS;
    // the bounds hold while this rule alone moved the counters and nobody touched the wiring
    bool cheap = g.compete_full != 1 && g.tag_counts == 0 && !g.hop_on && bd[DRAWS] != STALE &&
                 cs.sum[n] == wiring_sum(blob, n);
    const int8_t* row = blob.tag_evidence + (std::size_t)n * SLOTS * TILE_NEURONS;
    bool made = blob.tag_made[n] != 0;
    uint64_t use[SLOTS], active[SLOTS];
    int on = 0;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        bool none = src == (uint32_t)blob.tile_count;
        use[k] = none ? 0 : usable(blob, n, k, src);
        active[k] = none ? 0 : slot_read(blob, read_now, n, k) & use[k];
        on += __builtin_popcountll(active[k]);
    }
    if (won && g.compete_churn) {
        churn_draw(blob, use, on, active);
    }
    bool cross = false;
    for (int k = 0; k < SLOTS; k++) {
        if (slot_source(blob, n, k) == (uint32_t)blob.tile_count) {
            continue;
        }
        if (!won) {
            cross |= cheap ? step_slot(blob, n, k, 0, active[k], bd)
                           : (tag_add(blob, n, k, 0, active[k]), true);
            continue;
        }
        // a row not made yet starts at the connection strengths: wired is above zero. only
        // the silent sources the draw picks need their counter looked at
        uint64_t silent = use[k] & ~active[k];
        uint64_t pick = silent & sparse_word(blob.rng, g.compete_down_shift);
        int64_t w = blob_weight_index(n, k);
        uint64_t positive = 0;
        for (uint64_t m = made ? pick : 0; m; m &= m - 1) {
            int b = __builtin_ctzll(m);
            positive |= (uint64_t)(row[k * TILE_NEURONS + b] > 0) << b;
        }
        positive = made ? positive : connected_bits(blob.w0[w], blob.w1[w]);
        uint64_t down = pick & positive;
        cross |= cheap ? step_slot(blob, n, k, active[k], down, bd)
                       : (tag_add(blob, n, k, active[k], down), true);
        made = true;
    }
    if (g.compete_full == 2 && cheap) {
        check_repick(blob, n, cross);
    } else if (cross || !cheap) {
        tag_rewire(blob, n);
    } else {
        // a re-pick here would keep the wiring; it would still draw for its ties
        for (int d = 0; d < bd[DRAWS]; d++) {
            rng_below(blob.rng, SLOTS);
        }
        return;
    }
    if (g.compete_full != 1 && g.tag_counts == 0 && !g.hop_on) {
        bounds_make(blob, n);
    }
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
    compete_make(blob);
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
