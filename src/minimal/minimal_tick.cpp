#include "blob/random_words.hpp"
#include "minimal/minimal.hpp"
#include "minimal/minimal_steps.hpp"

#include <cstring>
#if defined(__AVX512F__)
#include <immintrin.h>
#endif

// one tick: popcount and fire, per-neuron threshold regulation, wiring by co-activity.

// a local slot where every neuron reads the same word: excitatory minus gain x inhibitory.
// the loop runs across the 64 neurons with the source word broadcast, so it vectorizes
// (AVX-512 VPOPCNTQ here).
static inline void add_shared(int32_t* __restrict drive, uint64_t e, uint64_t i,
                              const uint64_t* __restrict mk, int gain) {
    for (int b = 0; b < TILE_NEURONS; b++) {
        drive[b] += __builtin_popcountll(e & mk[b]) - gain * __builtin_popcountll(i & mk[b]);
    }
}

// a slot where each neuron reads its own word (its own delay or its own far tile)
static inline void add_own(int32_t* __restrict drive, const uint64_t* __restrict words,
                           uint64_t inh, const uint64_t* __restrict mk, int gain) {
    for (int b = 0; b < TILE_NEURONS; b++) {
        uint64_t e = words[b] & ~inh, i = words[b] & inh;
        drive[b] += __builtin_popcountll(e & mk[b]) - gain * __builtin_popcountll(i & mk[b]);
    }
}

// each neuron's word by its own delay: words[b] = column[delay[b]] (column holds one source
// tile's maps by age, 16 words). with AVX-512 one permute per 8 neurons, as the full engine
// does; otherwise one select pass per delay value, which vectorizes too.
static inline void pick_by_delay(uint64_t* __restrict words, const uint64_t* column,
                                 const uint8_t* __restrict delay, int delay_max) {
#if defined(__AVX512F__)
    (void)delay_max;
    __m512i low = _mm512_load_si512(column);
    __m512i high = _mm512_load_si512(column + 8);
    for (int b = 0; b < TILE_NEURONS; b += 8) {
        __m512i index = _mm512_cvtepu8_epi64(_mm_loadl_epi64((const __m128i*)(delay + b)));
        _mm512_store_si512(words + b, _mm512_permutex2var_epi64(low, index, high));
    }
#else
    for (int b = 0; b < TILE_NEURONS; b++) {
        words[b] = column[1];
    }
    for (int d = 2; d <= delay_max; d++) {
        for (int b = 0; b < TILE_NEURONS; b++) {
            words[b] = delay[b] == d ? column[d] : words[b];
        }
    }
#endif
}

// drive against threshold, one bit per neuron of tile t
static inline uint64_t compare(const MinimalBlob& m, const int32_t* drive, int t) {
    const uint8_t* __restrict thr = m.threshold + (int64_t)t * TILE_NEURONS;
    uint64_t would = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        would |= (uint64_t)(drive[b] >= (int32_t)thr[b]) << b;
    }
    return would;
}

// fire one tile, the plain engine: 9 local slots, all read from the previous map
static uint64_t would_fire_plain(const MinimalBlob& m, const uint64_t* prev, int t) {
    const uint64_t* inh = m.shell.inhibitory;
    const int gain = m.mg.inhibit_gain;
    alignas(64) int32_t drive[TILE_NEURONS] = {};
    for (int k = 0; k < MIN_SLOTS; k++) {
        uint32_t src = m.local_tile[t * MIN_SLOTS + k];
        uint64_t s = prev[src];
        if (s) {
            add_shared(drive, s & ~inh[src], s & inh[src],
                       m.mask + ((int64_t)t * MIN_SLOTS + k) * TILE_NEURONS, gain);
        }
    }
    return compare(m, drive, t);
}

// fire one tile with far slots or delays: each slot reads the map its delay back (delay 1 is
// the previous tick). far slots hold excitatory senders only.
static uint64_t would_fire(const MinimalBlob& m, int next, int t) {
    const uint64_t* inh = m.shell.inhibitory;
    const int gain = m.mg.inhibit_gain;
    const int slots = m.slots;
    const uint64_t* prev = m.spikes + (size_t)((next - 1) & (MIN_HISTORY - 1)) * m.stride;
    alignas(64) int32_t drive[TILE_NEURONS] = {};
    alignas(64) uint64_t words[TILE_NEURONS];
    for (int k = 0; k < MIN_SLOTS; k++) {
        uint32_t src = m.local_tile[t * MIN_SLOTS + k];
        const uint64_t* __restrict mk = m.mask + ((int64_t)t * slots + k) * TILE_NEURONS;
        if (!m.delay) {
            uint64_t s = prev[src];
            if (s) {
                add_shared(drive, s & ~inh[src], s & inh[src], mk, gain);
            }
            continue;
        }
        // each neuron picks its own age of this source tile
        alignas(64) uint64_t column[MIN_DELAY_MAX + 1] = {};
        uint64_t any = 0;
        for (int d = 1; d <= m.mg.delay_max; d++) {
            column[d] = m.spikes[(size_t)((next - d) & (MIN_HISTORY - 1)) * m.stride + src];
            any |= column[d];
        }
        if (!any) {
            continue;
        }
        pick_by_delay(words, column, m.delay + ((int64_t)t * slots + k) * TILE_NEURONS,
                      m.mg.delay_max);
        add_own(drive, words, inh[src], mk, gain);
    }
    // far slots: each neuron its own tile (and delay)
    for (int k = MIN_SLOTS; k < slots; k++) {
        const uint32_t* far =
            m.far_tile + ((int64_t)t * (slots - MIN_SLOTS) + (k - MIN_SLOTS)) * TILE_NEURONS;
        const uint64_t* __restrict mk = m.mask + ((int64_t)t * slots + k) * TILE_NEURONS;
        if (!m.delay) {
            for (int b = 0; b < TILE_NEURONS; b++) {
                words[b] = prev[far[b]];
            }
        } else {
            const uint8_t* dl = m.delay + ((int64_t)t * slots + k) * TILE_NEURONS;
            for (int b = 0; b < TILE_NEURONS; b++) {
                words[b] =
                    m.spikes[(size_t)((next - dl[b]) & (MIN_HISTORY - 1)) * m.stride + far[b]];
            }
        }
        for (int b = 0; b < TILE_NEURONS; b++) {
            drive[b] += __builtin_popcountll(words[b] & mk[b]);
        }
    }
    return compare(m, drive, t);
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
static void cowire(MinimalBlob& m, const uint64_t* fired) {
    for (int t = 0; t < m.shell.tile_count; t++) {
        uint64_t pick = fired[t] & ~m.shell.sensor[t] & sparse_word(m.rng, m.mg.cowire_shift);
        while (pick) {
            int n = t * TILE_NEURONS + __builtin_ctzll(pick);
            pick &= pick - 1;
            if (m.ev_tiles && m.mg.ev_cowire && m.ev_row[n] >= 0) {
                min_evidence_cowire(m, n); // evidence: a counter goes up, the mask follows
                continue;
            }
            m.stats.cowired += min_gain_active(m, n, 0) ? 1 : 0;
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
    uint64_t* out = minimal_map(m, next);
    uint64_t* noise_out = minimal_noise_map(m, next);

    const uint64_t* prev = minimal_map(m, m.now);
    const bool plain = m.slots == MIN_SLOTS && !m.delay;
    uint32_t fired_count = 0, noise_count = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t would = plain ? would_fire_plain(m, prev, t) : would_fire(m, next, t);
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
        cowire(m, out);
    }
    if (mg.learning_on && m.used) {
        min_mark_used(m, out);
    }

    // the shell's copy of the newest map, for the plug
    blob.now = (blob.now + 1) & (HISTORY - 1);
    std::memcpy(blob.spikes[blob.now], out, sizeof(uint64_t) * (size_t)(blob.tile_count + 1));
    blob.stats.tick++;
    blob.stats.fired = fired_count;
    m.stats.noise_spikes = noise_count;
}
