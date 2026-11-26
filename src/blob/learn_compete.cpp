#include "blob/hop_state.hpp"
#include "blob/random_words.hpp"
#include "blob/tag_store.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
                              // highest weak (excitatory sources), the draws a re-pick that
                              // changes nothing takes (STALE: the bounds are not made), and
                              // the last re-pick's lines (tag_rewire_hinted), as guesses
    uint64_t* sum = nullptr;  // per neuron: a hash of its wiring when the bounds were made
    // counted competition (compete_window, at the end of this file): spikes per neuron in
    // the window, and per tile which neurons fired in it (then which were on) and which
    // learners fired
    uint8_t* count = nullptr;
    uint64_t* seen = nullptr;
    uint64_t* pool = nullptr;
};

enum { LOW_WIRED = 0, HIGH_UNWIRED = 1, LOW_STRONG = 2, HIGH_WEAK = 3, DRAWS = 4, PICK = 5 };
constexpr int BOUNDS = PICK + 12;
constexpr int8_t STALE = -1;

static CompeteState& compete_make(Blob& blob) {
    if (!blob.compete) {
        CompeteState* c = (CompeteState*)std::calloc(1, sizeof(CompeteState));
        size_t n = (size_t)blob.neuron_count;
        c->bounds = (int8_t*)std::calloc(n * BOUNDS, 1);
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
    std::free(blob.compete->count);
    std::free(blob.compete->seen);
    std::free(blob.compete->pool);
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

// the bounds right after a re-pick, from where its lines fell: the lowest wired counter is
// the line; the highest unwired is the line if some at the line were left out, else under it
// (a bound loose in the safe direction). and the draws a re-pick that changes nothing takes:
// those this one took, since the counts and the sources are the same
static void bounds_make(Blob& blob, int n) {
    int8_t* bd = blob.compete->bounds + (size_t)n * BOUNDS;
    const int8_t* p = bd + PICK;
    for (int j = 0; j < 2; j++) {
        int line = p[3 * j];
        bd[2 * j] = (int8_t)line;                                                 // lowest in
        bd[2 * j + 1] = (int8_t)(p[3 * j + 1] || line == -128 ? line : line - 1); // highest out
    }
    bd[DRAWS] = (int8_t)(p[2] + p[5] + p[8] + p[11]);
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

// compete_store 1 (idea 188): the same move on the one-hop rule's bit planes, so the two
// rules share one store and one re-pick. a winner's active sources step up a word at a
// time; a silent source above zero (no sign bit, some bit on) steps down by chance
static void move_cell_planes(Blob& blob, int n, const uint64_t* use, const uint64_t* active,
                             bool won) {
    const Genome& g = blob.genome;
    HopState& h = *blob.hop;
    int planes = hop_plane_count(g);
    uint64_t* row = hop_plane_row(blob, h, n);
    int moved = 0;
    for (int k = 0; k < SLOTS; k++) {
        if (slot_source(blob, n, k) == (uint32_t)blob.tile_count) {
            continue;
        }
        uint64_t* p = row + k * planes;
        if (!won) {
            moved += hop_planes_step(p, planes, 0, active[k]);
            continue;
        }
        uint64_t some = 0;
        for (int j = 0; j < planes - 1; j++) {
            some |= p[j];
        }
        uint64_t silent = use[k] & ~active[k] & sparse_word(blob.rng, g.compete_down_shift);
        moved += hop_planes_step(p, planes, active[k], silent & some & ~p[planes - 1]);
    }
    if (moved) {
        hop_planes_rewire(blob, h, n);
    }
}

// one cell moves toward (or, losing, away from) the sources that were active on this tick,
// or with `window` set, those that fired in the window (one word per tile)
static void move_cell(Blob& blob, int n, int read_now, bool won, const uint64_t* window) {
    const Genome& g = blob.genome;
    CompeteState& cs = *blob.compete;
    int8_t* bd = cs.bounds + (size_t)n * BOUNDS;
    // the bounds hold while this rule alone moved the counters and nobody touched the wiring
    bool bounded = g.compete_full != 1 && g.tag_counts == 0 && !g.hop_on;
    bool cheap = bounded && bd[DRAWS] != STALE && cs.sum[n] == wiring_sum(blob, n);
    const int8_t* row = blob.tag_evidence + (std::size_t)n * SLOTS * TILE_NEURONS;
    bool made = blob.tag_made[n] != 0;
    uint64_t use[SLOTS], active[SLOTS];
    int on = 0;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        bool none = src == (uint32_t)blob.tile_count;
        use[k] = none ? 0 : usable(blob, n, k, src);
        active[k] = none ? 0 : (window ? window[src] : slot_read(blob, read_now, n, k)) & use[k];
        on += __builtin_popcountll(active[k]);
    }
    if (won && g.compete_churn) {
        churn_draw(blob, use, on, active);
    }
    if (g.compete_store && g.hop_on && blob.hop && hop_plane_count(g) > 0) {
        move_cell_planes(blob, n, use, active, won);
        return;
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
    if (cheap && !cross && g.compete_full != 2) {
        // a re-pick here would keep the wiring; it would still draw for its ties
        for (int d = 0; d < bd[DRAWS]; d++) {
            rng_below(blob.rng, SLOTS);
        }
        return;
    }
    if (!bounded) {
        tag_rewire(blob, n);
        return;
    }
    uint64_t before = g.compete_full == 2 ? wiring_sum(blob, n) : 0;
    tag_rewire_hinted(blob, n, bd + PICK);
    if (g.compete_full == 2 && cheap && !cross && wiring_sum(blob, n) != before) {
        // the check: the re-pick changed the wiring where the bounds said it would not
        std::fprintf(stderr, "compete_full 2: neuron %d rewired where the bounds said no\n", n);
        std::abort();
    }
    bounds_make(blob, n);
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

static void compete_counted(Blob& blob);

void learn_compete(Blob& blob, int read_now) {
    const Genome& g = blob.genome;
    tag_store_make(blob);
    compete_make(blob);
    if (g.compete_window > 0) {
        compete_counted(blob);
        return;
    }
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
            move_cell(blob, t * TILE_NEURONS + b, read_now, true, nullptr);
        }
        while (lost) {
            int b = __builtin_ctzll(lost);
            lost &= lost - 1;
            move_cell(blob, t * TILE_NEURONS + b, read_now, false, nullptr);
        }
    }
}

// counted competition (genome compete_window; docs/results-2026-11-26-steady.md, idea 174).
// the rule decides on a window's whole count instead of on single spikes: every tick each
// neuron's spikes are counted (a byte; 5 bits hold a 20-tick digit, 6 an 80-tick one). on
// the window's last tick a source is "on" if it fired at least compete_window_on times (1 if
// 0) in the window, one bit per neuron; each tile's tile_winners (4 if 0) learners that
// fired most, at least once, are its winners (ties by a rotation drawn once per tile); each
// moves with chance 2^-compete_shift toward every usable source that was on, as a spike's
// winner does toward those on at its spike. then the counts start again. windows are
// counted on the blob's tick, so with the window equal to a step's ticks and no rest they
// are the steps.

static uint64_t rotate_right(uint64_t x, int r) {
    return r == 0 ? x : (x >> r) | (x << (64 - r));
}

// the `want` cells of `pool` with the highest counts
static uint64_t top_counts(Blob& blob, int t, uint64_t pool, int want) {
    const uint8_t* count = blob.compete->count + (size_t)t * TILE_NEURONS;
    uint64_t win = 0;
    while (want > 0 && pool) {
        int best = 0;
        uint64_t at = 0;
        for (uint64_t m = pool; m; m &= m - 1) {
            int b = __builtin_ctzll(m);
            if (count[b] > best) {
                best = count[b];
                at = 0;
            }
            at |= (uint64_t)(count[b] == best) << b;
        }
        if (__builtin_popcountll(at) <= want) {
            win |= at;
            want -= __builtin_popcountll(at);
            pool &= ~at;
            continue;
        }
        // more tied than wanted: the first `want` after a random rotation
        int r = (int)(rng_next(blob.rng) & 63);
        uint64_t rot = rotate_right(at, r), pick = 0;
        for (; want > 0; want--) {
            pick |= rot & (~rot + 1);
            rot &= rot - 1;
        }
        win |= rotate_right(pick, (64 - r) & 63);
    }
    return win;
}

static void compete_counted(Blob& blob) {
    const Genome& g = blob.genome;
    CompeteState& cs = *blob.compete;
    if (!cs.count) {
        cs.count = (uint8_t*)std::calloc((size_t)blob.neuron_count, 1);
        cs.seen = (uint64_t*)std::calloc((size_t)blob.tile_count + 1, sizeof(uint64_t));
        cs.pool = (uint64_t*)std::calloc((size_t)blob.tile_count + 1, sizeof(uint64_t));
    }
    const uint64_t* fired = blob.spikes[blob.now];
    for (int t = 0; t < blob.tile_count; t++) {
        cs.seen[t] |= fired[t];
        for (uint64_t m = fired[t]; m; m &= m - 1) {
            uint8_t& c = cs.count[(size_t)t * TILE_NEURONS + __builtin_ctzll(m)];
            c = c < 255 ? c + 1 : c;
        }
    }
    if ((int)(blob.stats.tick % (uint64_t)g.compete_window) != g.compete_window - 1) {
        return;
    }
    int want = g.tile_winners > 0 ? g.tile_winners : 4;
    // the learners that fired at all are the pool; then "on" is the count at its minimum
    int on_min = g.compete_window_on > 1 ? g.compete_window_on : 1;
    for (int t = 0; t < blob.tile_count; t++) {
        cs.pool[t] = cs.seen[t] & learners(blob, t);
        uint64_t on = 0;
        for (uint64_t m = cs.seen[t]; m; m &= m - 1) {
            int b = __builtin_ctzll(m);
            on |= (uint64_t)(cs.count[(size_t)t * TILE_NEURONS + b] >= on_min) << b;
        }
        cs.seen[t] = on;
    }
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t won = top_counts(blob, t, cs.pool[t], want);
        won &= sparse_word(blob.rng, g.compete_shift);
        for (; won; won &= won - 1) {
            move_cell(blob, t * TILE_NEURONS + __builtin_ctzll(won), blob.now, true, cs.seen);
        }
    }
    std::memset(cs.count, 0, (size_t)blob.neuron_count);
    std::memset(cs.seen, 0, sizeof(uint64_t) * ((size_t)blob.tile_count + 1));
}
