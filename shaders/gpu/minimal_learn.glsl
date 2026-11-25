// the swaps that keep a neuron on its budget (src/minimal/minimal_learn.cpp), for the plain
// engine: 9 local slots, every slot read one tick back. each touches only its own neuron's
// masks, so neurons move in parallel; its draws come from the neuron's own keyed stream.
// needs from the including file: g_mask_base, g_ring_base, g_now, local_tile().
//
// a swap picks one bit over 9 slot words. the words are made again each time they are needed
// rather than kept in arrays: this path is rare, and arrays here would hold registers that
// the whole shader then pays for in the fire loop.

// what slot k of neuron (t, b) heard when it fired `age` ticks ago
uint64_t heard(int age, int t, int k) {
    int index = (g_now - age - 1) & (RING - 1);
    return rings[g_ring_base + index * STRIDE + local_tile(t, k)];
}

// sources slot k may hold: none off the sheet, never the neuron itself
uint64_t allowed(int t, int b, int k) {
    if (local_tile(t, k) == TILES) {
        return 0UL;
    }
    return k == 4 ? ~(1UL << b) : ~0UL;
}

// which candidates a swap picks from, slot k
const int FRESH = 0;  // heard and not wired: a gain
const int IDLE = 1;   // wired and not heard: what a gain costs
const int WIRED = 2;  // heard and wired: a loss
const int UNHEARD = 3; // neither: what replaces a loss
uint64_t candidates(int which, int age, int t, int b, int k) {
    uint64_t h = heard(age, t, k);
    uint64_t m = masks[g_mask_base + uint((t * 9 + k) * 64 + b)];
    if (which == FRESH) {
        return h & ~m & allowed(t, b, k);
    }
    if (which == IDLE) {
        return m & ~h;
    }
    if (which == WIRED) {
        return h & m;
    }
    return ~h & ~m & allowed(t, b, k);
}

// the r-th set bit of a word
int nth_bit(uint64_t word, int r) {
    for (int i = 0; i < r; i++) {
        word &= word - 1UL;
    }
    return lsb64(word);
}

// one candidate, uniformly over the 9 slots (min_pick_one), flipped in the mask. false if
// there were none.
bool flip_one(inout Rng r, int which, int age, int t, int b) {
    int total = 0;
    for (int k = 0; k < 9; k++) {
        total += bc64(candidates(which, age, t, b, k));
    }
    if (total == 0) {
        return false;
    }
    int at = int(rng_below(r, uint(total)));
    for (int k = 0; k < 9; k++) {
        uint64_t w = candidates(which, age, t, b, k);
        int c = bc64(w);
        if (at < c) {
            masks[g_mask_base + uint((t * 9 + k) * 64 + b)] ^= 1UL << nth_bit(w, at);
            return true;
        }
        at -= c;
    }
    return false;
}

// connect to one source heard at the spike `age` ticks ago that is not wired yet; under the
// budget pay with one wired source that was quiet then (min_gain_active, drop_one)
void gain_active(int t, int b, int age, inout Rng r, int budget_on) {
    if (flip_one(r, FRESH, age, t, b) && budget_on != 0) {
        flip_one(r, IDLE, age, t, b);
    }
}

// disconnect from one wired source heard then; under the budget a random unwired source that
// was quiet then takes its place (min_lose_active)
void lose_active(int t, int b, int age, inout Rng r, int budget_on) {
    if (flip_one(r, WIRED, age, t, b) && budget_on != 0) {
        flip_one(r, UNHEARD, age, t, b);
    }
}
