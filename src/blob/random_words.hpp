#pragma once

#include <cstdint>

// xoshiro256** and the sparse random words every chance in the blob is made from.
// state is four words; a blob keeps its own in Blob::rng so a run is repeatable.

inline uint64_t rotl64(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

inline uint64_t rng_next(uint64_t* s) {
    uint64_t result = rotl64(s[1] * 5, 7) * 9;
    uint64_t t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = rotl64(s[3], 45);
    return result;
}

// fill the four state words from one seed (splitmix64)
inline void rng_seed(uint64_t* s, uint64_t seed) {
    for (int k = 0; k < 4; k++) {
        seed += 0x9e3779b97f4a7c15ull;
        uint64_t z = seed;
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
        s[k] = z ^ (z >> 31);
    }
}

// a number in [0, n), n > 0. only used at birth and in the bench, never per bit.
inline uint32_t rng_below(uint64_t* s, uint32_t n) {
    return (uint32_t)(((rng_next(s) >> 32) * n) >> 32);
}

// a word whose bits are each set with chance 2^-shift.
// shift <= 6: several random words ANDed. shift > 6: one draw that gates the word with
// chance 2^-(shift-6) and then sets one random bit, which keeps the expected rate exact at
// one draw per word (at most one bit set, which at these rates almost never matters).
inline uint64_t sparse_word(uint64_t* s, int shift) {
    if (shift <= 0) {
        return ~0ull;
    }
    if (shift <= 6) {
        uint64_t w = rng_next(s);
        for (int k = 1; k < shift; k++) {
            w &= rng_next(s);
        }
        return w;
    }
    uint64_t r = rng_next(s);
    uint64_t gate = (1ull << (shift - 6)) - 1;
    if (shift - 6 >= 58) {
        gate = (1ull << 57) - 1;
    }
    if ((r & gate) != 0) {
        return 0;
    }
    return 1ull << (r >> 58);
}

// pick one set bit of a nonzero word, roughly uniformly
inline int pick_bit(uint64_t* s, uint64_t word) {
    int turn = (int)(rng_next(s) >> 58);
    uint64_t rolled = turn ? rotl64(word, 64 - turn) : word;
    return (__builtin_ctzll(rolled) + turn) & 63;
}
