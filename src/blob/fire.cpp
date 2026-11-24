#include <cstddef>
#include <cstring>
#if defined(__AVX512F__)
#include <immintrin.h>
#endif

#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"

// integrate and fire. the hot loop: per neuron, 11 slots of AND + popcount.

constexpr int POTENTIAL_MIN = -2048;
constexpr int POTENTIAL_MAX = 16000;

// add one local slot's drive to the 64 neurons of a tile: excitation, inhibition, branch
// bonus. e and i are the source tile's excitatory and inhibitory spikes, the same for every
// neuron (all delays equal). the loop runs across the 64 neurons, so it vectorizes
// (AVX-512 VPOPCNTQ on this machine).
static inline void add_slot_shared(int32_t* __restrict drive, uint64_t e, uint64_t i,
                                   const uint64_t* __restrict a0, const uint64_t* __restrict a1,
                                   int gain, int bt, int bonus, int sl) {
    for (int b = 0; b < TILE_NEURONS; b++) {
        int excite = __builtin_popcountll(e & a0[b]) + (__builtin_popcountll(e & a1[b]) << sl);
        int inhibit = __builtin_popcountll(i & a0[b]) + (__builtin_popcountll(i & a1[b]) << sl);
        drive[b] += excite - gain * inhibit + (excite >= bt ? bonus : 0);
    }
}

// the same, when each neuron read its own word s[b] (its own delay) from one source tile
static inline void add_slot_own(int32_t* __restrict drive, const uint64_t* __restrict s,
                                uint64_t inh, const uint64_t* __restrict a0,
                                const uint64_t* __restrict a1, int gain, int bt, int bonus,
                                int sl) {
    for (int b = 0; b < TILE_NEURONS; b++) {
        uint64_t e = s[b] & ~inh, i = s[b] & inh;
        int excite = __builtin_popcountll(e & a0[b]) + (__builtin_popcountll(e & a1[b]) << sl);
        int inhibit = __builtin_popcountll(i & a0[b]) + (__builtin_popcountll(i & a1[b]) << sl);
        drive[b] += excite - gain * inhibit + (excite >= bt ? bonus : 0);
    }
}

// far slots: each neuron reads its own source word, excitatory senders only
static inline void add_far_slot(int32_t* __restrict drive, const uint64_t* __restrict s,
                                const uint64_t* __restrict a0, const uint64_t* __restrict a1,
                                int bt, int bonus, int sl) {
    for (int b = 0; b < TILE_NEURONS; b++) {
        int excite =
            __builtin_popcountll(s[b] & a0[b]) + (__builtin_popcountll(s[b] & a1[b]) << sl);
        drive[b] += excite + (excite >= bt ? bonus : 0);
    }
}

// each neuron's word from one source tile at its own delay: words[b] = column[delay[b]],
// where column holds that tile's 16 ring maps by delay. with AVX-512 the column is two
// registers and the pick is one permute per 8 neurons; otherwise one select pass per delay
// value in the genome's range (that vectorizes too, where a plain table lookup does not).
static inline void pick_by_delay(uint64_t* __restrict words, const uint64_t* column,
                                 const uint8_t* __restrict delay, int delay_min, int delay_max) {
#if defined(__AVX512F__)
    (void)delay_min, (void)delay_max;
    __m512i low = _mm512_loadu_si512(column);
    __m512i high = _mm512_loadu_si512(column + 8);
    for (int b = 0; b < TILE_NEURONS; b += 8) {
        __m512i index = _mm512_cvtepu8_epi64(_mm_loadl_epi64((const __m128i*)(delay + b)));
        _mm512_store_si512(words + b, _mm512_permutex2var_epi64(low, index, high));
    }
#else
    for (int b = 0; b < TILE_NEURONS; b++) {
        words[b] = 0;
    }
    for (int d = delay_min; d <= delay_max; d++) {
        for (int b = 0; b < TILE_NEURONS; b++) {
            words[b] = delay[b] == d ? column[d] : words[b];
        }
    }
#endif
}

// 64 flags of 0 or 1, one per byte, into one word (bit b = flag b). eight bytes at a time:
// the multiply moves byte i's low bit to bit 56 + i with no carries.
static inline uint64_t pack_flags(const uint8_t* flags) {
    uint64_t word = 0;
    for (int j = 0; j < 8; j++) {
        uint64_t x;
        std::memcpy(&x, flags + 8 * j, 8);
        word |= ((x * 0x0102040810204080ull) >> 56) << (8 * j);
    }
    return word;
}

// one word into 64 flags of 0 or 1
static inline void unpack_flags(uint64_t word, uint8_t* flags) {
    for (int j = 0; j < 8; j++) {
        uint64_t spread = ((word >> (8 * j)) & 0xff) * 0x0101010101010101ull;
        uint64_t picked = spread & 0x8040201008040201ull;
        uint64_t x = ((picked + 0x7f7f7f7f7f7f7f7full) >> 7) & 0x0101010101010101ull;
        std::memcpy(flags + 8 * j, &x, 8);
    }
}

void tick_fire(Blob& blob, uint64_t* out) {
    const Genome& g = blob.genome;
    const uint64_t* inh = blob.inhibitory;
    const int gain = g.inhibit_gain, bt = g.branch_threshold, bonus = g.branch_bonus;
    const int noise_shift = g.noise_shift;
    const int sl = g.strong_log;
    // every local delay the same: one word per slot for the whole tile
    const bool shared = g.local_delay_min == g.local_delay_max;

    // where in the ring block each delay reads: delay d is d ticks before the new map
    const uint64_t* ring = blob.spikes[0];
    int32_t offset[HISTORY];
    for (int d = 0; d < HISTORY; d++) {
        offset[d] = ((blob.now + 1 - d) & (HISTORY - 1)) * blob.spike_stride;
    }

    alignas(64) int32_t drive[TILE_NEURONS];
    alignas(64) uint64_t words[TILE_NEURONS];
    for (int t = 0; t < blob.tile_count; t++) {
        int64_t tile_base = (int64_t)t * SLOTS * TILE_NEURONS;
        const uint64_t* tile_w0 = blob.w0 + tile_base;
        const uint64_t* tile_w1 = blob.w1 + tile_base;
        const uint8_t* tile_delay = blob.slot_delay + tile_base;
        for (int b = 0; b < TILE_NEURONS; b++) {
            drive[b] = 0;
        }
        // the 9 local slots
        for (int k = 0; k < LOCAL_SLOTS; k++) {
            uint32_t src = blob.local_tile[t * LOCAL_SLOTS + k];
            const uint64_t* a0 = tile_w0 + k * TILE_NEURONS;
            const uint64_t* a1 = tile_w1 + k * TILE_NEURONS;
            if (shared) {
                uint64_t s = ring[offset[g.local_delay_min] + src];
                add_slot_shared(drive, s & ~inh[src], s & inh[src], a0, a1, gain, bt, bonus, sl);
                continue;
            }
            // each neuron picks its own age of this source tile
            alignas(64) uint64_t column[HISTORY];
            for (int d = 0; d < HISTORY; d++) {
                column[d] = ring[offset[d] + src];
            }
            pick_by_delay(words, column, tile_delay + k * TILE_NEURONS, g.local_delay_min,
                          g.local_delay_max);
            add_slot_own(drive, words, inh[src], a0, a1, gain, bt, bonus, sl);
        }
        // the 2 far slots: each neuron its own tile and delay
        for (int k = 0; k < FAR_SLOTS; k++) {
            int slot = LOCAL_SLOTS + k;
            const uint8_t* delay = tile_delay + slot * TILE_NEURONS;
            const uint32_t* far = blob.far_tile + (int64_t)t * TILE_NEURONS * FAR_SLOTS + k;
            for (int b = 0; b < TILE_NEURONS; b++) {
                words[b] = ring[offset[delay[b]] + far[b * FAR_SLOTS]];
            }
            add_far_slot(drive, words, tile_w0 + slot * TILE_NEURONS, tile_w1 + slot * TILE_NEURONS,
                         bt, bonus, sl);
        }

        // leak, add, compare
        int bias = blob.sector_bias[blob.tile_sector[t]];
        int16_t* __restrict pot = blob.potential + t * TILE_NEURONS;
        const int16_t* __restrict thr = blob.threshold + t * TILE_NEURONS;
        const uint8_t* __restrict leak = blob.leak_shift + t * TILE_NEURONS;
        uint8_t* __restrict cool = blob.cooldown + t * TILE_NEURONS;
        alignas(64) uint8_t would_flags[TILE_NEURONS], rested_flags[TILE_NEURONS];
        for (int b = 0; b < TILE_NEURONS; b++) {
            int p = pot[b];
            p = p - (p >> leak[b]) + drive[b];
            p = p < POTENTIAL_MIN ? POTENTIAL_MIN : (p > POTENTIAL_MAX ? POTENTIAL_MAX : p);
            pot[b] = (int16_t)p;
            would_flags[b] = p >= thr[b] + bias ? 1 : 0;
            rested_flags[b] = cool[b] == 0 ? 1 : 0;
        }
        uint64_t would = pack_flags(would_flags);
        uint64_t rested = pack_flags(rested_flags);

        // sensors fire as the world says; their own guess is kept as the prediction
        uint64_t sensor = blob.sensor[t];
        blob.predicted[t] = would & sensor;
        uint64_t noise = g.noise_on ? sparse_word(blob.rng, noise_shift) : 0;
        uint64_t inner = (would | noise | blob.pacemaker[t]) & rested & ~sensor;
        uint64_t fired = inner | (blob.sense_in[t] & sensor);
        out[t] = fired;

        // reset the fired, count down the rest
        const uint8_t* __restrict cool_len = blob.cooldown_len + t * TILE_NEURONS;
        alignas(64) uint8_t fired_flags[TILE_NEURONS];
        unpack_flags(fired, fired_flags);
        for (int b = 0; b < TILE_NEURONS; b++) {
            int c = cool[b];
            cool[b] = (uint8_t)(fired_flags[b] ? cool_len[b] : (c > 0 ? c - 1 : 0));
            pot[b] = fired_flags[b] ? (int16_t)0 : pot[b];
        }
    }
    out[blob.tile_count] = 0;
}
