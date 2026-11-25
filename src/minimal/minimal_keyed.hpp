#pragma once

#include "blob/random_words.hpp"

#include <cstdint>

// keyed random words (--m.rng_mode 1), so the GPU port (src/gpu/, shaders/gpu/) can make the
// same words in any order. rng_mode 0 is the engine as it always was: one xoshiro stream per
// life, drawn in a fixed order. rng_mode 1: every draw site starts a fresh xoshiro stream from
// a key made of the life, the tick, what is drawn and where (a tile or a neuron, and an age),
// and draws from it in the same order the plain code does. birth always uses the life's own
// stream. shaders/gpu/minimal_random.glsl makes the same words; change both together.

// what is drawn (the top byte of the key)
enum MinDrawSite : uint32_t {
    MIN_DRAW_NOISE = 1,      // per tile: the noise word
    MIN_DRAW_REGULATE = 2,   // per tile: threshold up word, then down word
    MIN_DRAW_COWIRE = 3,     // per tile: which firing neurons swap by co-activity
    MIN_DRAW_COWIRE_PICK = 4, // per neuron: the swap itself
    MIN_DRAW_GRADE = 5,      // per tile and age: which graded neurons move
    MIN_DRAW_GRADE_PICK = 6, // per neuron and age: the move itself
};

// the splitmix64 finalizer, a bijection
inline uint64_t min_mix64(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

// the key of one draw site. where < 2^40, age < 256.
inline uint64_t min_draw_key(uint64_t life, uint64_t tick, uint32_t site, uint32_t where,
                             uint32_t age) {
    uint64_t z = min_mix64(life + tick * 0x9e3779b97f4a7c15ull);
    return z ^ ((uint64_t)site << 56) ^ ((uint64_t)age << 48) ^ (uint64_t)where;
}

// rng_mode 1: restart the stream `s` at this draw site. rng_mode 0: leave it alone.
inline void min_reseed(uint64_t* s, int rng_mode, uint64_t life, uint64_t tick, uint32_t site,
                       uint32_t where, uint32_t age) {
    if (rng_mode) {
        rng_seed(s, min_draw_key(life, tick, site, where, age));
    }
}
