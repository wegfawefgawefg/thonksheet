#include "blob/random_words.hpp"
#include "minimal/minimal.hpp"
#include "minimal/minimal_steps.hpp"

#include <cstring>

// one tick: popcount and fire, per-neuron threshold regulation, wiring by co-activity.

// fire one tile: drive is excitatory minus gain x inhibitory over the 9 slot masks, read from
// the previous map. the loop runs across the 64 neurons of the tile with each source word
// broadcast, so it vectorizes (AVX-512 VPOPCNTQ here).
static uint64_t would_fire(const MinimalBlob& m, const uint64_t* prev, int t) {
    const uint64_t* inh = m.shell.inhibitory;
    const int gain = m.mg.inhibit_gain;
    alignas(64) int32_t drive[TILE_NEURONS] = {};
    for (int k = 0; k < MIN_SLOTS; k++) {
        uint32_t src = m.local_tile[t * MIN_SLOTS + k];
        uint64_t s = prev[src];
        if (!s) {
            continue;
        }
        uint64_t e = s & ~inh[src], i = s & inh[src];
        const uint64_t* __restrict mk = m.mask + ((int64_t)t * MIN_SLOTS + k) * TILE_NEURONS;
        for (int b = 0; b < TILE_NEURONS; b++) {
            drive[b] += __builtin_popcountll(e & mk[b]) - gain * __builtin_popcountll(i & mk[b]);
        }
    }
    const uint8_t* __restrict thr = m.threshold + (int64_t)t * TILE_NEURONS;
    uint64_t would = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        would |= (uint64_t)(drive[b] >= (int32_t)thr[b]) << b;
    }
    return would;
}

// a neuron that fires gets a little harder to fire; every neuron slowly gets easier
static void regulate(MinimalBlob& m, const uint64_t* fired) {
    const MinimalGenome& mg = m.mg;
    for (int t = 0; t < m.shell.tile_count; t++) {
        uint64_t fixed = m.shell.sensor[t];
        uint64_t up = fired[t] & ~fixed & sparse_word(m.rng, mg.up_shift);
        uint64_t down = ~fixed & ~up & sparse_word(m.rng, mg.down_shift);
        uint8_t* thr = m.threshold + (int64_t)t * TILE_NEURONS;
        while (up) {
            int b = __builtin_ctzll(up);
            up &= up - 1;
            thr[b] = (uint8_t)(thr[b] + (thr[b] < mg.threshold_cap));
        }
        while (down) {
            int b = __builtin_ctzll(down);
            down &= down - 1;
            thr[b] = (uint8_t)(thr[b] - (thr[b] > 1));
        }
    }
}

// a firing neuron, by chance, swaps one connection for a source it just heard
static void cowire(MinimalBlob& m, const uint64_t* prev, const uint64_t* fired) {
    for (int t = 0; t < m.shell.tile_count; t++) {
        uint64_t pick = fired[t] & ~m.shell.sensor[t] & sparse_word(m.rng, m.mg.cowire_shift);
        while (pick) {
            int n = t * TILE_NEURONS + __builtin_ctzll(pick);
            pick &= pick - 1;
            m.stats.cowired += min_gain_active(m, n, prev) ? 1 : 0;
        }
    }
}

// any sensor line attached: the world is plugged in
static bool plugged_in(const Blob& blob) {
    uint64_t any = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        any |= blob.sense_attached[t];
    }
    return any != 0;
}

void minimal_tick(Blob& blob) {
    MinimalBlob& m = minimal_of(blob);
    const MinimalGenome& mg = m.mg;
    const int next = (m.now + 1) & (MIN_HISTORY - 1);
    const uint64_t* prev = minimal_map(m, m.now);
    uint64_t* out = minimal_map(m, next);
    uint64_t* noise_out = minimal_noise_map(m, next);

    uint32_t fired_count = 0, noise_count = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t would = would_fire(m, prev, t);
        uint64_t sensor = blob.sensor[t];
        uint64_t noise = sparse_word(m.rng, mg.noise_shift) & ~sensor;
        uint64_t fired = ((would | noise) & ~sensor) | (blob.sense_in[t] & sensor);
        out[t] = fired;
        noise_out[t] = noise & ~would;
        fired_count += (uint32_t)__builtin_popcountll(fired);
        noise_count += (uint32_t)__builtin_popcountll(noise & ~would);
    }
    out[blob.tile_count] = 0;
    noise_out[blob.tile_count] = 0;
    m.now = next;

    regulate(m, out);
    if (mg.learning_on && mg.cowire_on && (!mg.cowire_needs_input || plugged_in(blob))) {
        cowire(m, prev, out);
    }
    if (mg.learning_on && m.used) {
        min_mark_used(m, prev, out);
    }

    // the shell's copy of the newest map, for the plug
    blob.now = (blob.now + 1) & (HISTORY - 1);
    std::memcpy(blob.spikes[blob.now], out, sizeof(uint64_t) * (size_t)(blob.tile_count + 1));
    blob.stats.tick++;
    blob.stats.fired = fired_count;
    m.stats.noise_spikes = noise_count;
}
