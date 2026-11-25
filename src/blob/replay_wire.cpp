#include "blob/random_words.hpp"
#include "blob/replay.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

#include <cstdlib>
#include <cstring>

// reverse replay (idea 62): the hidden counters of inner neurons and how they are read off
// into real connections.
//
// every neuron has a signed 6-bit counter per possible source (11 slots x 64, one byte each:
// 704 bytes per neuron; bit-sliced it would be 6 words per slot, 528). a row starts at the
// wiring the neuron has when it is first touched, born strength x 4 (1 -> 4, 4 -> 16,
// 5 -> 20), so reading it off unchanged gives the born wiring back exactly. reading off keeps
// the neuron's count of connections fixed at what it had then: the highest counters win, and
// at the cut a source that is wired now beats one that is not (so one step on an unwired
// source does not evict anything). nothing at zero or below is wired, so a neuron can end up
// with fewer. strength: counter 1..9 is 1, 10..19 is 4, 20 and up is 5.
//
// replay_strong_budget: the count of strong connections is kept fixed too. of the wired
// sources the highest counters, as many as the neuron had strong ones, are strength 4 (ties
// to what is strong now), the rest 1, and nothing is 5, so the walk cannot add drive.

constexpr int COUNTER_TOP = 31;
constexpr int BORN_SCALE = 4;

static size_t round64(size_t n) {
    return (n + 63) & ~(size_t)63;
}

ReplayState* replay_state(Blob& blob) {
    if (blob.replay) {
        return blob.replay;
    }
    size_t neurons = (size_t)blob.neuron_count;
    size_t stride = (size_t)blob.tile_count + 1;
    size_t planes = (size_t)blob.tile_count * SLOTS * TILE_NEURONS;
    size_t sizes[] = {
        round64(sizeof(ReplayState)),
        round64(neurons * REPLAY_ROW),
        round64(neurons * sizeof(uint16_t)),
        round64(neurons * sizeof(uint16_t)),
        round64(planes * 8),
        round64(planes * 8),
        round64(HISTORY * stride * 8),
        round64(HISTORY * stride * 8),
        round64(HISTORY * stride * 8),
        round64(HISTORY * stride * 8),
        round64(stride * 8),
    };
    size_t total = 0;
    for (size_t s : sizes) {
        total += s;
    }
    uint8_t* block = (uint8_t*)std::calloc(total, 1);
    uint8_t* at[11];
    size_t off = 0;
    for (int i = 0; i < 11; i++) {
        at[i] = block + off;
        off += sizes[i];
    }
    ReplayState* r = (ReplayState*)at[0];
    r->evidence = (int8_t*)at[1];
    r->keep = (uint16_t*)at[2];
    r->strong = (uint16_t*)at[3];
    r->born_w0 = (uint64_t*)at[4];
    r->born_w1 = (uint64_t*)at[5];
    r->walk[0] = (uint64_t*)at[6];
    r->walk[1] = (uint64_t*)at[7];
    r->next[0] = (uint64_t*)at[8];
    r->next[1] = (uint64_t*)at[9];
    r->touched = (uint64_t*)at[10];
    std::memcpy(r->born_w0, blob.w0, planes * 8);
    std::memcpy(r->born_w1, blob.w1, planes * 8);
    blob.replay = r;
    return r;
}

// the counter row of neuron n, filled from its wiring on first use
static int8_t* counter_row(Blob& blob, ReplayState& r, int n) {
    int8_t* c = r.evidence + (size_t)n * REPLAY_ROW;
    if (r.keep[n] == 0) {
        for (int k = 0; k < SLOTS; k++) {
            for (int b = 0; b < TILE_NEURONS; b++) {
                c[k * TILE_NEURONS + b] = (int8_t)(BORN_SCALE * blob_weight(blob, n, k, b));
            }
        }
        int have = count_fan_in(blob, n), strong = 0;
        for (int k = 0; k < SLOTS; k++) {
            strong += __builtin_popcountll(blob.w1[blob_weight_index(n, k)]);
        }
        r.keep[n] = (uint16_t)(have > 0 ? have : 1);
        r.strong[n] = (uint16_t)strong;
    }
    return c;
}

void replay_counters_add(Blob& blob, ReplayState& r, int n, int slot, uint64_t up,
                         uint64_t down) {
    int8_t* c = counter_row(blob, r, n) + slot * TILE_NEURONS;
    r.up += __builtin_popcountll(up);
    r.down += __builtin_popcountll(down);
    while (up) {
        int b = __builtin_ctzll(up);
        up &= up - 1;
        c[b] = (int8_t)(c[b] < COUNTER_TOP ? c[b] + 1 : COUNTER_TOP);
    }
    while (down) {
        int b = __builtin_ctzll(down);
        down &= down - 1;
        c[b] = (int8_t)(c[b] > -COUNTER_TOP ? c[b] - 1 : -COUNTER_TOP);
    }
}

// take up to `room` bits of `pool`, slots from a random start, low bits first
static void take_bits(Blob& blob, uint64_t* pool, uint64_t* wired, int& room) {
    int start = (int)rng_below(blob.rng, SLOTS);
    for (int j = 0; j < SLOTS && room > 0; j++) {
        int k = (start + j) % SLOTS;
        uint64_t m = pool[k];
        while (m && room > 0) {
            uint64_t bit = m & (~m + 1);
            m &= m - 1;
            wired[k] |= bit;
            room--;
        }
        pool[k] = m;
    }
}

// replay_strong_budget: the top r.strong[n] wired counters at 4, the rest at 1
static bool set_strong_budget(Blob& blob, ReplayState& r, int n, const int8_t* c,
                              const uint64_t* wired, const uint64_t* now) {
    int histogram[COUNTER_TOP + 1] = {};
    for (int k = 0; k < SLOTS; k++) {
        for (int b = 0; b < TILE_NEURONS; b++) {
            int v = c[k * TILE_NEURONS + b];
            histogram[v > 0 ? v : 0] += (int)((wired[k] >> b) & 1);
        }
    }
    int want = r.strong[n], above = 0, cut = COUNTER_TOP;
    while (cut > 1 && above + histogram[cut] < want) {
        above += histogram[cut];
        cut--;
    }
    int room = want - above;
    uint64_t strong[SLOTS], tie_now[SLOTS], tie_new[SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        uint64_t over = 0, at = 0;
        for (int b = 0; b < TILE_NEURONS; b++) {
            int v = c[k * TILE_NEURONS + b];
            over |= (uint64_t)(v > cut) << b;
            at |= (uint64_t)(v == cut) << b;
        }
        int64_t w = blob_weight_index(n, k);
        strong[k] = want > 0 ? over & wired[k] : 0;
        tie_now[k] = want > 0 ? at & wired[k] & blob.w1[w] : 0;
        tie_new[k] = want > 0 ? at & wired[k] & ~blob.w1[w] : 0;
    }
    take_bits(blob, tie_now, strong, room);
    take_bits(blob, tie_new, strong, room);
    bool changed = false;
    for (int k = 0; k < SLOTS; k++) {
        int64_t w = blob_weight_index(n, k);
        changed = changed || wired[k] != now[k];
        blob.w0[w] = wired[k] & ~strong[k];
        blob.w1[w] = strong[k];
    }
    blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
    return changed;
}

bool replay_rewire(Blob& blob, ReplayState& r, int n) {
    int8_t* c = counter_row(blob, r, n);
    int keep = r.keep[n];
    uint64_t self = 1ull << (n % TILE_NEURONS);
    // usable sources: a slot with a tile, not itself, no inhibitory source on a far slot
    uint64_t usable[SLOTS], now[SLOTS];
    int histogram[COUNTER_TOP + 1] = {};
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        uint64_t u = src == (uint32_t)blob.tile_count ? 0 : ~0ull;
        u &= k == 4 ? ~self : ~0ull;
        u &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        usable[k] = u;
        int64_t w = blob_weight_index(n, k);
        now[k] = connected_bits(blob.w0[w], blob.w1[w]);
        for (int b = 0; b < TILE_NEURONS; b++) {
            int v = c[k * TILE_NEURONS + b];
            if (((u >> b) & 1) && v > 0) {
                histogram[v]++;
            }
        }
    }
    // the cut: everything above it is wired; at it, what fits in the rest of the budget
    int above = 0, cut = COUNTER_TOP;
    while (cut > 1 && above + histogram[cut] < keep) {
        above += histogram[cut];
        cut--;
    }
    int room = keep - above;
    uint64_t wired[SLOTS], tie_now[SLOTS], tie_new[SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        uint64_t over = 0, at = 0;
        for (int b = 0; b < TILE_NEURONS; b++) {
            int v = c[k * TILE_NEURONS + b];
            over |= (uint64_t)(v > cut) << b;
            at |= (uint64_t)(v == cut) << b;
        }
        wired[k] = over & usable[k];
        tie_now[k] = at & usable[k] & now[k];
        tie_new[k] = at & usable[k] & ~now[k];
    }
    take_bits(blob, tie_now, wired, room);
    take_bits(blob, tie_new, wired, room);
    // strengths from the counters
    bool changed = false;
    if (blob.genome.replay_strong_budget) {
        return set_strong_budget(blob, r, n, c, wired, now);
    }
    for (int k = 0; k < SLOTS; k++) {
        uint64_t s4 = 0, s5 = 0;
        for (int b = 0; b < TILE_NEURONS; b++) {
            int v = c[k * TILE_NEURONS + b];
            s4 |= (uint64_t)(v >= 10) << b;
            s5 |= (uint64_t)(v >= 20) << b;
        }
        uint64_t one = wired[k] & ~s4, four = wired[k] & s4 & ~s5, five = wired[k] & s5;
        int64_t w = blob_weight_index(n, k);
        changed = changed || wired[k] != now[k];
        blob.w0[w] = one | five;
        blob.w1[w] = four | five;
    }
    blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
    return changed;
}

void replay_changed(const Blob& blob, int64_t& gained, int64_t& lost) {
    gained = lost = 0;
    const ReplayState* r = blob.replay;
    if (!r) {
        return;
    }
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t inner = ~blob.sensor[t] & ~blob.motor[t];
        for (int k = 0; k < SLOTS; k++) {
            for (int b = 0; b < TILE_NEURONS; b++) {
                if (!((inner >> b) & 1)) {
                    continue;
                }
                int64_t w = blob_weight_index(t * TILE_NEURONS + b, k);
                uint64_t was = connected_bits(r->born_w0[w], r->born_w1[w]);
                uint64_t is = connected_bits(blob.w0[w], blob.w1[w]);
                gained += __builtin_popcountll(is & ~was);
                lost += __builtin_popcountll(was & ~is);
            }
        }
    }
}
