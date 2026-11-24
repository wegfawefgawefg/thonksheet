#include <cstddef>
#include <cstring>

#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"

// integrate and fire. the hot loop: per neuron, 11 slots of AND + popcount.

constexpr int POTENTIAL_MIN = -2048;
constexpr int POTENTIAL_MAX = 16000;

// add one slot's drive to the 64 neurons of a tile: excitation, inhibition, branch bonus.
// e and i are the source tile's excitatory and inhibitory spikes; the loop runs across the
// 64 neurons, so it vectorizes (AVX-512 VPOPCNTQ on this machine).
static inline void add_slot(int32_t* __restrict drive, uint64_t e, uint64_t i,
                            const uint64_t* __restrict a0, const uint64_t* __restrict a1,
                            int gain, int bt, int bonus) {
    for (int b = 0; b < TILE_NEURONS; b++) {
        int excite = __builtin_popcountll(e & a0[b]) + 4 * __builtin_popcountll(e & a1[b]);
        int inhibit = __builtin_popcountll(i & a0[b]) + 4 * __builtin_popcountll(i & a1[b]);
        drive[b] += excite - gain * inhibit + (excite >= bt ? bonus : 0);
    }
}

// far slots: each neuron reads its own source word, excitatory senders only
static inline void add_far_slot(int32_t* __restrict drive, const uint64_t* __restrict s,
                                const uint64_t* __restrict a0, const uint64_t* __restrict a1,
                                int bt, int bonus) {
    for (int b = 0; b < TILE_NEURONS; b++) {
        int excite = __builtin_popcountll(s[b] & a0[b]) + 4 * __builtin_popcountll(s[b] & a1[b]);
        drive[b] += excite + (excite >= bt ? bonus : 0);
    }
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
    const uint64_t* prev = blob.spikes[blob.now];
    const uint64_t* inh = blob.inhibitory;
    const int gain = g.inhibit_gain, bt = g.branch_threshold, bonus = g.branch_bonus;
    const int noise_shift = g.noise_shift;

    // the map each far delay reads: delay d means d ticks before the new map
    const uint64_t* delayed[HISTORY];
    for (int d = 0; d < HISTORY; d++) {
        delayed[d] = blob.spikes[(blob.now + 1 - d) & (HISTORY - 1)];
    }

    alignas(64) int32_t drive[TILE_NEURONS];
    alignas(64) uint64_t far_words[TILE_NEURONS];
    for (int t = 0; t < blob.tile_count; t++) {
        const uint64_t* tile_w0 = blob.w0 + (int64_t)t * SLOTS * TILE_NEURONS;
        const uint64_t* tile_w1 = blob.w1 + (int64_t)t * SLOTS * TILE_NEURONS;
        for (int b = 0; b < TILE_NEURONS; b++) {
            drive[b] = 0;
        }
        // the 9 local slots: one source word each, shared by the whole tile
        for (int k = 0; k < LOCAL_SLOTS; k++) {
            uint32_t src = blob.local_tile[t * LOCAL_SLOTS + k];
            add_slot(drive, prev[src] & ~inh[src], prev[src] & inh[src],
                     tile_w0 + k * TILE_NEURONS, tile_w1 + k * TILE_NEURONS, gain, bt, bonus);
        }
        for (int k = 0; k < FAR_SLOTS; k++) {
            for (int b = 0; b < TILE_NEURONS; b++) {
                int f = (t * TILE_NEURONS + b) * FAR_SLOTS + k;
                far_words[b] = delayed[blob.far_delay[f]][blob.far_tile[f]];
            }
            int slot = LOCAL_SLOTS + k;
            add_far_slot(drive, far_words, tile_w0 + slot * TILE_NEURONS,
                         tile_w1 + slot * TILE_NEURONS, bt, bonus);
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
        uint64_t noise = sparse_word(blob.rng, noise_shift);
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
