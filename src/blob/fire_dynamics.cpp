#include "blob/fire_dynamics.hpp"

#include "blob/random_words.hpp"

#include <cstdlib>
#include <cstring>

// ideas 66-75: tiring and warming synapses, tiring neurons, a slow second potential,
// rebound, up and down states, any-branch, veto, coincidence and winner-in-the-tile neurons.
// docs/results-2026-11-25-family-dynamics.md

constexpr int POTENTIAL_MIN = -2048;
constexpr int POTENTIAL_MAX = 16000;
constexpr int SLOW_LIMIT = 16000;
constexpr int TIRE_MAX = 16000; // x16 threshold steps

DynamicsOn dynamics_on(const Genome& g) {
    DynamicsOn on;
    on.warm = g.syn_warm_ticks > 0;
    on.sent = g.syn_tire_ticks > 0 || on.warm;
    on.split = g.any_branch_percent > 0 || g.veto_percent > 0 || g.coincide_percent > 0;
    on.neuron = g.tire_jump > 0 || g.slow_leak_shift > 0 || g.rebound_level > 0 ||
                g.updown_drop > 0 || g.tile_winners > 0;
    return on;
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// zeroed memory, 64-byte aligned, whole cache lines
static void* zeroed(size_t bytes) {
    size_t rounded = (bytes + 63) & ~(size_t)63;
    void* p = std::aligned_alloc(64, rounded ? rounded : 64);
    std::memset(p, 0, rounded ? rounded : 64);
    return p;
}

// one word per tile: each neuron is of the kind with chance percent / 100
static uint64_t* draw_kind(Blob& blob, uint64_t* rng, int percent) {
    uint64_t* kind = (uint64_t*)zeroed(sizeof(uint64_t) * (blob.tile_count + 1));
    for (int t = 0; t < blob.tile_count; t++) {
        for (int b = 0; b < TILE_NEURONS; b++) {
            if ((int)rng_below(rng, 100) < percent) {
                kind[t] |= 1ull << b;
            }
        }
    }
    return kind;
}

void dynamics_birth(Blob& blob, uint64_t seed) {
    Genome& g = blob.genome;
    g.syn_tire_ticks = clamp_int(g.syn_tire_ticks, 0, HISTORY - 1);
    g.syn_tire_keep_shift = clamp_int(g.syn_tire_keep_shift, 0, 63);
    g.syn_warm_ticks = clamp_int(g.syn_warm_ticks, 0, HISTORY - 1);
    g.syn_warm_shift = clamp_int(g.syn_warm_shift, 0, 63);
    g.tire_jump = clamp_int(g.tire_jump, 0, 1000);
    g.tire_shift = clamp_int(g.tire_shift, 0, 12);
    g.slow_leak_shift = clamp_int(g.slow_leak_shift, 0, 14);
    g.slow_share_shift = clamp_int(g.slow_share_shift, 0, 14);
    g.rebound_level = clamp_int(g.rebound_level, 0, 2000);
    g.rebound_ticks = clamp_int(g.rebound_ticks, 0, 255);
    g.updown_drop = clamp_int(g.updown_drop, 0, 1000);
    g.any_branch_percent = clamp_int(g.any_branch_percent, 0, 100);
    g.veto_percent = clamp_int(g.veto_percent, 0, 100);
    g.coincide_percent = clamp_int(g.coincide_percent, 0, 100);
    g.tile_winners = clamp_int(g.tile_winners, 0, TILE_NEURONS);

    size_t n = (size_t)blob.neuron_count;
    size_t ring_words = (size_t)blob.spike_stride * HISTORY;
    DynamicsOn on = dynamics_on(g);
    if (on.sent) {
        blob.sent = (uint64_t*)zeroed(sizeof(uint64_t) * ring_words);
    }
    if (on.warm) {
        blob.warm = (uint64_t*)zeroed(sizeof(uint64_t) * ring_words);
    }
    if (g.tire_jump > 0) {
        blob.tire = (int16_t*)zeroed(sizeof(int16_t) * n);
    }
    if (g.slow_leak_shift > 0) {
        blob.slow = (int16_t*)zeroed(sizeof(int16_t) * n);
    }
    if (g.rebound_level > 0) {
        blob.rebound = (uint8_t*)zeroed(n);
    }
    if (g.updown_drop > 0) {
        blob.up = (uint64_t*)zeroed(sizeof(uint64_t) * (blob.tile_count + 1));
    }
    // kinds come from their own stream so the blob's own draws are the same as without
    uint64_t rng[4];
    rng_seed(rng, seed ^ 0x6479a7c3e1f05b2dull);
    if (g.any_branch_percent > 0) {
        blob.any_branch = draw_kind(blob, rng, g.any_branch_percent);
    }
    if (g.veto_percent > 0) {
        blob.veto = draw_kind(blob, rng, g.veto_percent);
    }
    if (g.coincide_percent > 0) {
        blob.coincide = draw_kind(blob, rng, g.coincide_percent);
    }
}

void split_start(const Blob& blob, int t, SlotSplit& x) {
    for (int b = 0; b < TILE_NEURONS; b++) {
        x.best[b] = 0;
        x.inhibit[b] = 0;
        x.active[b] = 0;
    }
    uint64_t veto = blob.veto ? blob.veto[t] : 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        x.veto[b] = (int32_t)((veto >> b) & 1);
    }
}

uint64_t split_finish(const Blob& blob, int t, const SlotSplit& x, int32_t* drive) {
    if (blob.any_branch) {
        uint64_t any = blob.any_branch[t];
        for (int b = 0; b < TILE_NEURONS; b++) {
            drive[b] = ((any >> b) & 1) ? x.best[b] - x.inhibit[b] : drive[b];
        }
    }
    if (!blob.coincide) {
        return ~0ull;
    }
    // two or more different delays among the active slots
    uint64_t met = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        uint32_t a = x.active[b];
        met |= (uint64_t)((a & (a - 1)) != 0) << b;
    }
    return met | ~blob.coincide[t];
}

// 64 flags (0 or 1, one per int) into one word
static uint64_t pack_ints(const int32_t* flags) {
    uint64_t word = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        word |= (uint64_t)flags[b] << b;
    }
    return word;
}

uint64_t dynamics_compare(Blob& blob, int t, const int32_t* __restrict drive, int bias,
                          int32_t* __restrict margin) {
    const Genome& g = blob.genome;
    int base = t * TILE_NEURONS;
    int16_t* __restrict pot = blob.potential + base;
    const int16_t* __restrict thr = blob.threshold + base;
    const uint8_t* __restrict leak = blob.leak_shift + base;
    alignas(64) int32_t seen[TILE_NEURONS], bar[TILE_NEURONS], flag[TILE_NEURONS];
    for (int b = 0; b < TILE_NEURONS; b++) {
        int p = pot[b];
        p = p - (p >> leak[b]) + drive[b];
        p = p < POTENTIAL_MIN ? POTENTIAL_MIN : (p > POTENTIAL_MAX ? POTENTIAL_MAX : p);
        pot[b] = (int16_t)p;
        seen[b] = p;
        bar[b] = thr[b] + bias;
    }
    // 69: the slow sum leaks far less and adds a share of itself
    if (blob.slow) {
        int16_t* __restrict slow = blob.slow + base;
        int ls = g.slow_leak_shift, share = g.slow_share_shift;
        for (int b = 0; b < TILE_NEURONS; b++) {
            int s = slow[b];
            s = s - (s >> ls) + drive[b];
            s = s < -SLOW_LIMIT ? -SLOW_LIMIT : (s > SLOW_LIMIT ? SLOW_LIMIT : s);
            slow[b] = (int16_t)s;
            seen[b] += s >> share;
        }
    }
    // 68: recent firing raises the bar
    if (blob.tire) {
        const int16_t* __restrict tire = blob.tire + base;
        for (int b = 0; b < TILE_NEURONS; b++) {
            bar[b] += tire[b] >> 4;
        }
    }
    // 70: held down, then let go: easier for a few ticks
    if (blob.rebound) {
        uint8_t* __restrict r = blob.rebound + base;
        int level = -g.rebound_level, drop = g.rebound_drop, ticks = g.rebound_ticks;
        for (int b = 0; b < TILE_NEURONS; b++) {
            int held = pot[b] < level;
            int left = r[b];
            bar[b] -= (!held && left > 0) ? drop : 0;
            r[b] = (uint8_t)(held ? ticks : (left > 0 ? left - 1 : 0));
        }
    }
    // 71: up neurons have the lower bar until the potential falls below the floor
    if (blob.up) {
        uint64_t up = blob.up[t];
        int drop = g.updown_drop, floor = g.updown_floor;
        for (int b = 0; b < TILE_NEURONS; b++) {
            int is_up = (int)((up >> b) & 1);
            bar[b] -= is_up * drop;
            flag[b] = pot[b] < floor;
        }
        blob.up[t] = up & ~pack_ints(flag);
    }
    for (int b = 0; b < TILE_NEURONS; b++) {
        margin[b] = seen[b] - bar[b];
        flag[b] = margin[b] >= 0;
    }
    return pack_ints(flag);
}

void dynamics_after(Blob& blob, int t, uint64_t fired) {
    const Genome& g = blob.genome;
    if (blob.up) {
        blob.up[t] |= fired & ~blob.sensor[t];
    }
    if (blob.tire) {
        int16_t* tire = blob.tire + t * TILE_NEURONS;
        int round = (1 << g.tire_shift) - 1;
        int jump = g.tire_jump * 16;
        for (int b = 0; b < TILE_NEURONS; b++) {
            int v = tire[b];
            v -= (v + round) >> g.tire_shift;
            v += ((fired >> b) & 1) ? jump : 0;
            tire[b] = (int16_t)(v > TIRE_MAX ? TIRE_MAX : v);
        }
    }
}

// how many of the spikes are at least `bar` over threshold
static int count_over(uint64_t spikes, const int32_t* margin, int bar) {
    int count = 0;
    while (spikes) {
        int b = __builtin_ctzll(spikes);
        spikes &= spikes - 1;
        count += margin[b] >= bar ? 1 : 0;
    }
    return count;
}

uint64_t dynamics_winners(uint64_t spikes, const int32_t* margin, int k) {
    if (__builtin_popcountll(spikes) <= k) {
        return spikes;
    }
    // the lowest bar that leaves at most k: binary search between the margins present
    int lo = 1 << 30, hi = -(1 << 30);
    for (uint64_t s = spikes; s; s &= s - 1) {
        int m = margin[__builtin_ctzll(s)];
        lo = m < lo ? m : lo;
        hi = m > hi ? m : hi;
    }
    hi++; // nothing is over hi: count 0
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (count_over(spikes, margin, mid) <= k) {
            hi = mid;
        } else {
            lo = mid + 1;
        }
    }
    uint64_t kept = 0;
    for (uint64_t s = spikes; s; s &= s - 1) {
        int b = __builtin_ctzll(s);
        kept |= margin[b] >= lo ? 1ull << b : 0;
    }
    return kept;
}

void dynamics_sent(Blob& blob, int next) {
    const Genome& g = blob.genome;
    size_t stride = (size_t)blob.spike_stride;
    const uint64_t* now = blob.spikes[next];
    uint64_t* sent = blob.sent + next * stride;
    uint64_t* warm = blob.warm ? blob.warm + next * stride : nullptr;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t tired = 0, warmed = 0;
        for (int age = 1; age <= g.syn_tire_ticks; age++) {
            tired |= blob.spikes[(next - age) & (HISTORY - 1)][t];
        }
        for (int age = 1; age <= g.syn_warm_ticks; age++) {
            warmed |= blob.spikes[(next - age) & (HISTORY - 1)][t];
        }
        uint64_t s = now[t];
        if (tired) {
            s &= ~(tired & ~sparse_word(blob.rng, g.syn_tire_keep_shift));
        }
        sent[t] = s;
        if (warm) {
            warm[t] = s & warmed & sparse_word(blob.rng, g.syn_warm_shift);
        }
    }
    sent[blob.tile_count] = 0;
    if (warm) {
        warm[blob.tile_count] = 0;
    }
}
