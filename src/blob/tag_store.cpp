#include "blob/tag_store.hpp"

#include "blob/random_words.hpp"
#include "blob/weight_levels.hpp"

#include <cstdlib>

// hidden counters for every neuron (tag_mode, learn_tags.cpp), the same store the readout's
// motor cells have (learn_evidence.cpp), and the read-off that keeps a neuron's shape: it
// wires exactly as many excitatory and as many inhibitory sources as it was born with, and as
// many of each strong, the highest counters winning. at a tie the sources already wired (or strong)
// stay first and the rest are drawn by chance, so the counts hold and ties do not churn.
//
// tag_counts (idea 155, to see what the fixed counts do): 1 keeps the two kinds' counts but
// reads strength off the counter (1..3 at 1, 4..7 at 4, 8 and up at 5), so drive can grow;
// 2 also keeps one count for both kinds and wires nothing at zero or below, the walk's
// read-off (replay_wire.cpp) on these counters.

constexpr int TAG_ROW = SLOTS * TILE_NEURONS;

void tag_store_make(Blob& blob) {
    if (blob.tag_evidence) {
        return;
    }
    size_t neurons = (size_t)blob.neuron_count;
    size_t words = (size_t)blob.tile_count + 1;
    blob.tag_evidence = (int8_t*)std::calloc(neurons * TAG_ROW, 1);
    blob.tag_keep = (uint16_t*)std::calloc(2 * neurons, sizeof(uint16_t));
    blob.tag_strong = (uint16_t*)std::calloc(2 * neurons, sizeof(uint16_t));
    blob.tag_made = (uint8_t*)std::calloc(neurons, 1);
    blob.tag_plus = (uint64_t*)std::calloc(words, sizeof(uint64_t));
    blob.tag_minus = (uint64_t*)std::calloc(words, sizeof(uint64_t));
    blob.tag_feedback = (int16_t*)std::calloc(neurons, sizeof(int16_t));
    blob.tag_scratch = (uint64_t*)std::calloc(2 * words, sizeof(uint64_t));
}

// which sources of slot k are inhibitory (far slots carry excitatory ones only)
static uint64_t slot_kind(const Blob& blob, int n, int k) {
    uint32_t src = slot_source(blob, n, k);
    return k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
}

// a neuron's counters, made on first use from its wiring: each counter starts at the
// strength of its connection (0, 1, 4 or 5). the counts it keeps are those it has now,
// connections and strong ones, separately for excitatory and inhibitory sources, so its
// balance of excitation and inhibition is the one it was born with
static int8_t* tag_row(Blob& blob, int n) {
    int8_t* c = blob.tag_evidence + (size_t)n * TAG_ROW;
    if (blob.tag_made[n]) {
        return c;
    }
    int keep[2] = {0, 0}, strong[2] = {0, 0};
    for (int k = 0; k < SLOTS; k++) {
        int64_t w = blob_weight_index(n, k);
        uint64_t kind = slot_kind(blob, n, k);
        uint64_t wired = connected_bits(blob.w0[w], blob.w1[w]);
        keep[0] += __builtin_popcountll(wired & ~kind);
        keep[1] += __builtin_popcountll(wired & kind);
        strong[0] += __builtin_popcountll(blob.w1[w] & ~kind);
        strong[1] += __builtin_popcountll(blob.w1[w] & kind);
        for (int b = 0; b < TILE_NEURONS; b++) {
            c[k * TILE_NEURONS + b] = (int8_t)blob_weight(blob, n, k, b);
        }
    }
    for (int i = 0; i < 2; i++) {
        blob.tag_keep[2 * n + i] = (uint16_t)keep[i];
        blob.tag_strong[2 * n + i] = (uint16_t)strong[i];
    }
    blob.tag_made[n] = 1;
    return c;
}

void tag_add(Blob& blob, int n, int slot, uint64_t up, uint64_t down) {
    int8_t* c = tag_row(blob, n) + slot * TILE_NEURONS;
    int bits = blob.genome.tag_bits;
    int top = (1 << ((bits < 2 ? 2 : (bits > 8 ? 8 : bits)) - 1)) - 1;
    uint64_t both = up & down;
    up &= ~both;
    down &= ~both;
    while (up) {
        int b = __builtin_ctzll(up);
        up &= up - 1;
        c[b] = (int8_t)(c[b] < top ? c[b] + 1 : top);
    }
    while (down) {
        int b = __builtin_ctzll(down);
        down &= down - 1;
        c[b] = (int8_t)(c[b] > -top ? c[b] - 1 : -top);
    }
}

// the `want` highest counters among `cand` (none under `floor`) into `out`; at the line,
// `prefer` first, then the rest by chance
static void select_top(Blob& blob, const int8_t* c, const uint64_t* cand, const uint64_t* prefer,
                       int want, int floor, uint64_t* out) {
    int histogram[256] = {};
    for (int k = 0; k < SLOTS; k++) {
        uint64_t m = cand[k];
        while (m) {
            int b = __builtin_ctzll(m);
            m &= m - 1;
            histogram[c[k * TILE_NEURONS + b] + 128]++;
        }
    }
    // the line: the lowest value whose sources, with all above it, reach `want`
    // (fewer than `want` at or above the floor: take them all)
    int line = floor, above = 0, need = 0;
    for (int v = 127; v >= floor; v--) {
        if (above + histogram[v + 128] >= want || v == floor) {
            line = v;
            need = above + histogram[v + 128] >= want ? want - above : histogram[v + 128];
            break;
        }
        above += histogram[v + 128];
    }
    uint64_t at[SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        uint64_t gt = 0, eq = 0;
        uint64_t m = cand[k];
        while (m) {
            int b = __builtin_ctzll(m);
            m &= m - 1;
            int v = c[k * TILE_NEURONS + b];
            gt |= (uint64_t)(v > line) << b;
            eq |= (uint64_t)(v == line) << b;
        }
        out[k] = gt;
        at[k] = eq;
    }
    // at the line: preferred first, then the others, each from a random slot onward
    for (int pass = 0; pass < 2 && need > 0; pass++) {
        int start = (int)rng_below(blob.rng, SLOTS);
        for (int j = 0; j < SLOTS && need > 0; j++) {
            int k = (start + j) % SLOTS;
            uint64_t pool = at[k] & (pass == 0 ? prefer[k] : ~prefer[k]);
            int have = __builtin_popcountll(pool);
            if (have <= need) {
                out[k] |= pool;
                at[k] &= ~pool;
                need -= have;
                continue;
            }
            while (need > 0) {
                uint64_t bit = 1ull << pick_bit(blob.rng, pool);
                pool &= ~bit;
                out[k] |= bit;
                at[k] &= ~bit;
                need--;
            }
        }
    }
}

void tag_rewire(Blob& blob, int n) {
    const int8_t* c = tag_row(blob, n);
    uint64_t self = 1ull << (n % TILE_NEURONS);
    uint64_t usable[2][SLOTS], wired[SLOTS], strong_now[SLOTS], wire[2][SLOTS], strong[2][SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        uint64_t u = src == (uint32_t)blob.tile_count ? 0 : ~0ull;
        u &= k == 4 ? ~self : ~0ull;
        u &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        uint64_t kind = slot_kind(blob, n, k);
        usable[0][k] = u & ~kind;
        usable[1][k] = u & kind;
        int64_t w = blob_weight_index(n, k);
        wired[k] = connected_bits(blob.w0[w], blob.w1[w]);
        strong_now[k] = blob.w1[w];
    }
    int counts = blob.genome.tag_counts;
    if (counts == 2) {
        // one count for both kinds, nothing at zero or below
        uint64_t both[SLOTS];
        for (int k = 0; k < SLOTS; k++) {
            both[k] = usable[0][k] | usable[1][k];
            wire[1][k] = 0;
        }
        select_top(blob, c, both, wired, blob.tag_keep[2 * n] + blob.tag_keep[2 * n + 1], 1,
                   wire[0]);
    }
    // excitatory and inhibitory sources each keep their own counts
    for (int i = 0; i < 2; i++) {
        if (counts != 2) {
            select_top(blob, c, usable[i], wired, blob.tag_keep[2 * n + i], -128, wire[i]);
        }
        if (counts == 0) {
            select_top(blob, c, wire[i], strong_now, blob.tag_strong[2 * n + i], -128, strong[i]);
        }
    }
    for (int k = 0; k < SLOTS; k++) {
        int64_t w = blob_weight_index(n, k);
        uint64_t on = wire[0][k] | wire[1][k];
        if (counts == 0) {
            uint64_t s = strong[0][k] | strong[1][k];
            blob.w1[w] = s;
            blob.w0[w] = on & ~s;
            continue;
        }
        // strength from the counter
        uint64_t four = 0, five = 0;
        for (uint64_t m = on; m; m &= m - 1) {
            int b = __builtin_ctzll(m);
            int v = c[k * TILE_NEURONS + b];
            four |= (uint64_t)(v >= 4) << b;
            five |= (uint64_t)(v >= 8) << b;
        }
        blob.w1[w] = four;
        blob.w0[w] = (on & ~four) | five;
    }
    blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
}
