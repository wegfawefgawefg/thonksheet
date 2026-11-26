#include "blob/hop_state.hpp"
#include "blob/random_words.hpp"
#include "blob/weight_levels.hpp"

#include <cstdlib>

// the one-hop rule's counters as bit planes (genome hop_bits; idea 178,
// docs/results-2026-11-26-every-event.md). the rule's ladder found that the counter has to
// step on every marked input, not with a small chance, and then needs 9 to 12 bits. a byte
// per source would be stepped one source at a time; planes step a whole slot at once.
//
// layout. a neuron has SLOTS x hop_bits words: for each slot, plane 0 holds the lowest bit
// of its 64 sources' counters, the last plane the sign (two's complement). 12 bits: 132
// words a neuron against the byte store's 88.
//
// step. one count up for a word of sources is a ripple carry down the planes (an AND and an
// XOR a plane); one down the same with a borrow. sources at the counter's ends are masked
// out first. no branch per source, no random word.
//
// read-off. the cell wires its highest counters at its born counts (finding 38), as
// tag_rewire does. the highest `want` of a set are found a plane at a time from the top
// (a radix select: a popcount per slot per plane), so no counter is ever read out as a
// number. at the line the sources already wired stay first and the rest are drawn by
// chance, drawn only when there is a real choice.

int hop_plane_count(const Genome& g) {
    return g.hop_bits <= 0 ? 0 : (g.hop_bits < 4 ? 4 : (g.hop_bits > 16 ? 16 : g.hop_bits));
}

void hop_planes_free(HopState& h) {
    std::free(h.planes);
    std::free(h.plane_made);
    std::free(h.dirty);
    h.planes = nullptr;
    h.plane_made = nullptr;
    h.dirty = nullptr;
}

uint64_t* hop_plane_row(Blob& blob, HopState& h, int n) {
    int planes = hop_plane_count(blob.genome);
    size_t row = (size_t)SLOTS * (size_t)planes;
    if (!h.planes) {
        h.planes = (uint64_t*)std::calloc((size_t)blob.neuron_count * row, sizeof(uint64_t));
        h.plane_made = (uint8_t*)std::calloc((size_t)blob.neuron_count, 1);
        h.dirty = (uint64_t*)std::calloc((size_t)blob.tile_count + 1, sizeof(uint64_t));
    }
    uint64_t* p = h.planes + (size_t)n * row;
    if (h.plane_made[n]) {
        return p;
    }
    // each counter starts at the strength of its connection: 0, 1, 4 or 5
    for (int k = 0; k < SLOTS; k++) {
        int64_t w = blob_weight_index(n, k);
        p[k * planes] = blob.w0[w];
        p[k * planes + 2] = blob.w1[w];
    }
    tag_row(blob, n); // the born counts it keeps (tag_keep, tag_strong)
    h.plane_made[n] = 1;
    return p;
}

int hop_planes_step(uint64_t* p, int planes, uint64_t up, uint64_t down) {
    // at the top: sign 0, every other plane 1. at the bottom (minus the top): sign 1, plane
    // 0 on, the rest 0
    uint64_t top = ~p[planes - 1], bottom = p[planes - 1] & p[0];
    for (int i = 0; i < planes - 1; i++) {
        top &= p[i];
    }
    for (int i = 1; i < planes - 1; i++) {
        bottom &= ~p[i];
    }
    up &= ~top;
    down &= ~bottom;
    uint64_t carry = up, borrow = down;
    for (int i = 0; i < planes; i++) {
        uint64_t next = p[i] & carry;
        p[i] ^= carry;
        carry = next;
    }
    for (int i = 0; i < planes; i++) {
        uint64_t next = ~p[i] & borrow;
        p[i] ^= borrow;
        borrow = next;
    }
    return __builtin_popcountll(up) + __builtin_popcountll(down);
}

// the `want` highest counters among `cand` into `out`; at the line, `prefer` first, then
// the rest by chance
static void select_top(Blob& blob, const uint64_t* row, int planes, const uint64_t* cand,
                       const uint64_t* prefer, int want, uint64_t* out) {
    uint64_t at[SLOTS];
    int have = 0;
    for (int k = 0; k < SLOTS; k++) {
        at[k] = cand[k];
        out[k] = 0;
        have += __builtin_popcountll(cand[k]);
    }
    if (want >= have) {
        for (int k = 0; k < SLOTS; k++) {
            out[k] = cand[k];
        }
        return;
    }
    if (want <= 0) {
        return;
    }
    // from the top plane down: `at` are the sources whose higher bits are the line's. if
    // enough of them have this bit, the line has it and the others fall out; if not, those
    // that have it are above the line and are taken
    int need = want;
    for (int i = planes - 1; i >= 0; i--) {
        uint64_t ones[SLOTS];
        int c = 0;
        for (int k = 0; k < SLOTS; k++) {
            uint64_t x = row[k * planes + i];
            ones[k] = at[k] & (i == planes - 1 ? ~x : x); // the sign plane counts the other way
            c += __builtin_popcountll(ones[k]);
        }
        if (c >= need) {
            for (int k = 0; k < SLOTS; k++) {
                at[k] = ones[k];
            }
            continue;
        }
        need -= c;
        for (int k = 0; k < SLOTS; k++) {
            out[k] |= ones[k];
            at[k] &= ~ones[k];
        }
    }
    // at the line: preferred first, then the others; chance only where some must be left
    for (int pass = 0; pass < 2 && need > 0; pass++) {
        uint64_t pool[SLOTS];
        int count = 0;
        for (int k = 0; k < SLOTS; k++) {
            pool[k] = at[k] & (pass == 0 ? prefer[k] : ~prefer[k]);
            count += __builtin_popcountll(pool[k]);
        }
        if (count <= need) {
            for (int k = 0; k < SLOTS; k++) {
                out[k] |= pool[k];
            }
            need -= count;
            continue;
        }
        for (; need > 0; need--) {
            // one of the pool, evenly: the r-th set bit over the slots
            int r = (int)rng_below(blob.rng, (uint32_t)count);
            int k = 0;
            for (; r >= __builtin_popcountll(pool[k]); k++) {
                r -= __builtin_popcountll(pool[k]);
            }
            uint64_t m = pool[k];
            for (; r > 0; r--) {
                m &= m - 1;
            }
            uint64_t bit = m & (~m + 1);
            pool[k] &= ~bit;
            out[k] |= bit;
            count--;
        }
    }
}

bool hop_planes_rewire(Blob& blob, HopState& h, int n) {
    int planes = hop_plane_count(blob.genome);
    const uint64_t* row = hop_plane_row(blob, h, n);
    uint64_t usable[2][SLOTS], wired[SLOTS], strong_now[SLOTS], wire[2][SLOTS], strong[2][SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        uint64_t u = hop_usable(blob, n, k, src);
        uint64_t kind = hop_kind_of(blob, k, src);
        usable[0][k] = u & ~kind;
        usable[1][k] = u & kind;
        int64_t w = blob_weight_index(n, k);
        wired[k] = connected_bits(blob.w0[w], blob.w1[w]);
        strong_now[k] = blob.w1[w];
    }
    // excitatory and inhibitory sources each keep their own counts, wired and strong
    for (int i = 0; i < 2; i++) {
        select_top(blob, row, planes, usable[i], wired, blob.tag_keep[2 * n + i], wire[i]);
        select_top(blob, row, planes, wire[i], strong_now, blob.tag_strong[2 * n + i], strong[i]);
    }
    bool changed = false;
    for (int k = 0; k < SLOTS; k++) {
        int64_t w = blob_weight_index(n, k);
        uint64_t s = strong[0][k] | strong[1][k];
        uint64_t weak = (wire[0][k] | wire[1][k]) & ~s;
        changed |= blob.w1[w] != s || blob.w0[w] != weak;
        blob.w1[w] = s;
        blob.w0[w] = weak;
    }
    if (changed) {
        blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
    }
    return changed;
}
