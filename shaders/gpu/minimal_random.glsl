// keyed random words: xoshiro256** started at a draw site's key, as rng_mode 1 of the CPU
// engine makes them (src/minimal/minimal_keyed.hpp, src/blob/random_words.hpp). change both
// together.

const uint SITE_NOISE = 1u;
const uint SITE_REGULATE = 2u;
const uint SITE_COWIRE = 3u;
const uint SITE_COWIRE_PICK = 4u;
const uint SITE_GRADE = 5u;
const uint SITE_GRADE_PICK = 6u;

const uint64_t GOLDEN = 0x9e3779b97f4a7c15UL;

struct Rng {
    uint64_t s0, s1, s2, s3;
};

uint64_t mix64(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9UL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebUL;
    return z ^ (z >> 31);
}

uint64_t rotl64(uint64_t x, int k) {
    return (x << k) | (x >> (64 - k));
}

// rng_seed: four splitmix64 words
Rng rng_seed(uint64_t seed) {
    Rng r;
    seed += GOLDEN;
    r.s0 = mix64(seed);
    seed += GOLDEN;
    r.s1 = mix64(seed);
    seed += GOLDEN;
    r.s2 = mix64(seed);
    seed += GOLDEN;
    r.s3 = mix64(seed);
    return r;
}

uint64_t rng_next(inout Rng r) {
    uint64_t result = rotl64(r.s1 * 5UL, 7) * 9UL;
    uint64_t t = r.s1 << 17;
    r.s2 ^= r.s0;
    r.s3 ^= r.s1;
    r.s1 ^= r.s2;
    r.s0 ^= r.s3;
    r.s2 ^= t;
    r.s3 = rotl64(r.s3, 45);
    return result;
}

// the stream of one draw site: life, tick, what, where (tile or neuron), age
Rng rng_at(uint64_t life, uint64_t tick, uint site, uint where, uint age) {
    uint64_t z = mix64(life + tick * GOLDEN);
    z ^= (uint64_t(site) << 56) ^ (uint64_t(age) << 48) ^ uint64_t(where);
    return rng_seed(z);
}

// a number in [0, n)
uint rng_below(inout Rng r, uint n) {
    return uint(((rng_next(r) >> 32) * uint64_t(n)) >> 32);
}

// a word whose bits are each set with chance 2^-shift (sparse_word)
uint64_t sparse_word(inout Rng r, int shift) {
    if (shift <= 0) {
        return ~0UL;
    }
    if (shift <= 6) {
        uint64_t w = rng_next(r);
        for (int k = 1; k < shift; k++) {
            w &= rng_next(r);
        }
        return w;
    }
    uint64_t v = rng_next(r);
    uint64_t gate = shift - 6 >= 58 ? (1UL << 57) - 1UL : (1UL << (shift - 6)) - 1UL;
    if ((v & gate) != 0UL) {
        return 0UL;
    }
    return 1UL << int(v >> 58);
}

int bc64(uint64_t x) {
    uvec2 v = unpackUint2x32(x);
    return bitCount(v.x) + bitCount(v.y);
}

int lsb64(uint64_t x) {
    uvec2 v = unpackUint2x32(x);
    return v.x != 0u ? findLSB(v.x) : 32 + findLSB(v.y);
}
