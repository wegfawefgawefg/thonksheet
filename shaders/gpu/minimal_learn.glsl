// the swaps that keep a neuron on its budget (src/minimal/minimal_learn.cpp), for the plain
// engine: 9 local slots, every slot read one tick back. each touches only its own neuron's
// masks, so neurons move in parallel; its draws come from the neuron's own keyed stream.
// needs from the including file: g_mask_base, g_ring_base, g_now, local_tile().

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

// the r-th set bit of a word
int nth_bit(uint64_t word, int r) {
    for (int i = 0; i < r; i++) {
        word &= word - 1UL;
    }
    return lsb64(word);
}

// one set bit, uniformly, over 9 slot words (min_pick_one). -1 if all are empty.
int pick_one(inout Rng r, uint64_t words[9], out int bit) {
    int total = 0;
    for (int k = 0; k < 9; k++) {
        total += bc64(words[k]);
    }
    bit = 0;
    if (total == 0) {
        return -1;
    }
    int at = int(rng_below(r, uint(total)));
    for (int k = 0; k < 9; k++) {
        int c = bc64(words[k]);
        if (at < c) {
            bit = nth_bit(words[k], at);
            return k;
        }
        at -= c;
    }
    return -1;
}

// connect to one source heard at the spike `age` ticks ago that is not wired yet; under the
// budget pay with one wired source that was quiet then (min_gain_active, drop_one)
void gain_active(int t, int b, int age, inout Rng r, int budget_on) {
    uint base = g_mask_base + uint(t * 9 * 64 + b);
    uint64_t h[9], w[9];
    for (int k = 0; k < 9; k++) {
        h[k] = heard(age, t, k);
        w[k] = h[k] & ~masks[base + k * 64] & allowed(t, b, k);
    }
    int bit;
    int slot = pick_one(r, w, bit);
    if (slot < 0) {
        return;
    }
    masks[base + slot * 64] |= 1UL << bit;
    if (budget_on == 0) {
        return;
    }
    for (int k = 0; k < 9; k++) {
        w[k] = masks[base + k * 64] & ~h[k];
    }
    slot = pick_one(r, w, bit);
    if (slot >= 0) {
        masks[base + slot * 64] &= ~(1UL << bit);
    }
}

// disconnect from one wired source heard then; under the budget a random unwired source that
// was quiet then takes its place (min_lose_active)
void lose_active(int t, int b, int age, inout Rng r, int budget_on) {
    uint base = g_mask_base + uint(t * 9 * 64 + b);
    uint64_t h[9], w[9];
    for (int k = 0; k < 9; k++) {
        h[k] = heard(age, t, k);
        w[k] = h[k] & masks[base + k * 64];
    }
    int bit;
    int slot = pick_one(r, w, bit);
    if (slot < 0) {
        return;
    }
    masks[base + slot * 64] &= ~(1UL << bit);
    if (budget_on == 0) {
        return;
    }
    for (int k = 0; k < 9; k++) {
        w[k] = ~h[k] & ~masks[base + k * 64] & allowed(t, b, k);
    }
    slot = pick_one(r, w, bit);
    if (slot >= 0) {
        masks[base + slot * 64] |= 1UL << bit;
    }
}
