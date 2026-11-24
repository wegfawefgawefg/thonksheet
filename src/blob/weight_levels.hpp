#pragma once

#include <cstdint>

// one-level weight moves on a slot's two bit planes, for every bit in `mask` at once.
// code (w1, w0): 00 none, 01 = 1, 10 = 4, 11 = 5. each returns the bits that moved.

// up one level; 11 stays. 00 -> 01 makes a new connection.
inline uint64_t level_up(uint64_t& w0, uint64_t& w1, uint64_t mask) {
    uint64_t m = mask & ~(w0 & w1);
    uint64_t carry = w0 & m;
    w0 ^= m;
    w1 |= carry;
    return m;
}

// up one level, only on existing connections
inline uint64_t level_up_connected(uint64_t& w0, uint64_t& w1, uint64_t mask) {
    return level_up(w0, w1, mask & (w0 | w1));
}

// down one level; 00 stays. 01 -> 00 removes the connection.
inline uint64_t level_down(uint64_t& w0, uint64_t& w1, uint64_t mask) {
    uint64_t m = mask & (w0 | w1);
    uint64_t borrow = m & ~w0;
    w0 ^= m;
    w1 &= ~borrow;
    return m;
}

inline uint64_t connected_bits(uint64_t w0, uint64_t w1) {
    return w0 | w1;
}

// strength 1 only (01): the weak ones that churn may remove
inline uint64_t weak_bits(uint64_t w0, uint64_t w1) {
    return w0 & ~w1;
}
