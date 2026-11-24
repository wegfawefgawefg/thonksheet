#pragma once

#include <cstdint>

// the world's own dice. separate from the blob's generator so a task repeats exactly for a
// given task seed, whatever the blob does.

struct WorldRandom {
    uint64_t state = 0;
};

inline uint64_t world_random_word(WorldRandom& r) {
    // splitmix64
    r.state += 0x9e3779b97f4a7c15ull;
    uint64_t z = r.state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

inline void world_random_seed(WorldRandom& r, uint64_t seed) {
    r.state = seed;
    world_random_word(r);
}

// a whole number in [0, n)
inline int world_random_below(WorldRandom& r, int n) {
    return (int)((world_random_word(r) >> 32) * (uint64_t)n >> 32);
}
