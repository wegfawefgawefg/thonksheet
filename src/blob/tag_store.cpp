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
int8_t* tag_row(Blob& blob, int n) {
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

// the counters of one slot at or above v, as a word: one compare of 64 bytes, then eight
// multiplies gather a flag byte each into the bits (as hop_after_fire does)
typedef int8_t SlotBytes __attribute__((vector_size(TILE_NEURONS)));
static inline uint64_t at_least(const int8_t* c, int v) {
    if (v > 127) {
        return 0;
    }
    if (v < -127) {
        return ~0ull;
    }
    SlotBytes x, line;
    __builtin_memcpy(&x, c, sizeof x);
    for (int b = 0; b < TILE_NEURONS; b++) {
        line[b] = (int8_t)v;
    }
    SlotBytes flags = (SlotBytes)(x >= line) & 1;
    uint64_t words[TILE_NEURONS / 8], m = 0;
    __builtin_memcpy(words, &flags, sizeof words);
    for (int j = 0; j < TILE_NEURONS / 8; j++) {
        m |= ((words[j] * 0x0102040810204080ull) >> 56) << (8 * j);
    }
    return m;
}

// how many of `cand` have a counter at or above v
static int count_at_least(const int8_t* c, const uint64_t* cand, int v) {
    int n = 0;
    for (int k = 0; k < SLOTS; k++) {
        n += cand[k] ? __builtin_popcountll(cand[k] & at_least(c + k * TILE_NEURONS, v)) : 0;
    }
    return n;
}

// the `want` highest counters among `cand` (none under `floor`) into `out`; at the line,
// `prefer` first, then the rest by chance. `hint` (or NO_HINT) is a guess at the line, which
// only changes how fast it is found; the pick says where the line was, whether sources at
// the line were left out, and whether chance was drawn
constexpr int NO_HINT = -1000;
static void select_top(Blob& blob, const int8_t* c, const uint64_t* cand, const uint64_t* prefer,
                       int want, int floor, uint64_t* out, int hint, int8_t* pick) {
    // the line: the highest value whose sources, with all above it, reach `want` (fewer than
    // `want` at or above the floor: the floor, and take them all). counters stay within
    // [-top, top] once moved and start at 0..5, so only that range is searched: from the
    // hint up or down a step at a time, or by halving
    int bits = blob.genome.tag_bits;
    int top = (1 << ((bits < 2 ? 2 : (bits > 8 ? 8 : bits)) - 1)) - 1;
    int lo = floor > -top ? floor : -top, hi = top > 5 ? top : 5;
    int all = 0;
    for (int k = 0; k < SLOTS; k++) {
        all += __builtin_popcountll(cand[k]);
    }
    int line = floor, reach = floor > -top ? count_at_least(c, cand, floor) : all, above = -1;
    if (reach >= want && hint != NO_HINT) {
        line = hint < lo ? lo : (hint > hi ? hi : hint);
        reach = count_at_least(c, cand, line);
        if (reach >= want) {
            for (; line < hi; line++) {
                above = count_at_least(c, cand, line + 1);
                if (above < want) {
                    break;
                }
                reach = above;
            }
            above = line < hi ? above : -1;
        } else {
            while (reach < want) {
                above = reach;
                reach = count_at_least(c, cand, --line);
            }
        }
    } else if (reach >= want) {
        while (lo < hi) {
            int mid = lo + (hi - lo + 1) / 2;
            if (count_at_least(c, cand, mid) >= want) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }
        line = lo;
        reach = count_at_least(c, cand, line);
    }
    above = above >= 0 ? above : count_at_least(c, cand, line + 1);
    int need = reach >= want ? want - above : reach - above;
    pick[0] = (int8_t)(line < -128 ? -128 : (line > 127 ? 127 : line));
    pick[1] = (int8_t)(reach > want);
    pick[2] = (int8_t)(need > 0);
    uint64_t at[SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        uint64_t ge = cand[k] ? cand[k] & at_least(c + k * TILE_NEURONS, line) : 0;
        uint64_t gt = cand[k] ? cand[k] & at_least(c + k * TILE_NEURONS, line + 1) : 0;
        out[k] = gt;
        at[k] = ge & ~gt;
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
    tag_rewire_hinted(blob, n, nullptr);
}

void tag_rewire_hinted(Blob& blob, int n, int8_t* pick) {
    int8_t scratch[12];
    int8_t* p = pick ? pick : scratch;
    auto hint = [&](int j) { return pick ? (int)pick[3 * j] : NO_HINT; };
    const int8_t* c = tag_row(blob, n);
    uint64_t self = 1ull << (n % TILE_NEURONS);
    uint64_t usable[2][SLOTS], wired[SLOTS], strong_now[SLOTS], wire[2][SLOTS], strong[2][SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        uint64_t u = src == (uint32_t)blob.tile_count ? 0 : ~0ull;
        u &= k == 4 ? ~self : ~0ull;
        u &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        u &= ~deep_blocked(blob, n, src);
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
                   wire[0], hint(0), p);
    }
    // excitatory and inhibitory sources each keep their own counts
    for (int i = 0; i < 2; i++) {
        if (counts != 2) {
            select_top(blob, c, usable[i], wired, blob.tag_keep[2 * n + i], -128, wire[i],
                       hint(2 * i), p + 6 * i);
        }
        if (counts == 0) {
            select_top(blob, c, wire[i], strong_now, blob.tag_strong[2 * n + i], -128, strong[i],
                       hint(2 * i + 1), p + 6 * i + 3);
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
    if (blob.hop) {
        hop_stale(blob, n); // the one-hop rule keeps bounds on this neuron's counters
    }
    if (blob.compete) {
        compete_stale(blob, n); // so does competitive learning
    }
}
