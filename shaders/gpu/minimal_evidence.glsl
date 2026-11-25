// evidence counters (src/minimal/minimal_evidence.cpp), EV 1. every possible source of a
// counter neuron has a small hidden counter, offset binary in ev_bits bits, bitsliced: plane
// i of a slot is one word holding bit i of its 64 sources' counters. a reward better than
// average moves up the counters of the sources a graded neuron heard before it fired (worse:
// down); a neuron quiet through the window moves the other way; co-activity bumps one. the
// mask is then read off the counters. each neuron touches only its own counters and masks,
// so neurons move in parallel, each from its own keyed stream.
//
// rows: the batch numbers the neurons that may have counters (every non-sensor neuron if any
// life has ev_on 2, else the motor cells); a life uses the rows of its own counter neurons.
// needs from the including file: g_ev_base, g_line_base, s_sensor and minimal_learn.glsl.

const uint SITE_EV_GRADE = 7u;
const uint SITE_EV_MOVE = 8u;

uint g_ev_base, g_line_base;

// this life's counter neurons of tile t
uint64_t ev_mine(int t, Genome G) {
    if (EV == 0 || G.ev_on == 0) {
        return 0UL;
    }
    uint64_t kind = G.ev_on == 2 ? ~0UL : ev_meta[2 * (TILES + 1) + t];
    return kind & ~s_sensor[t];
}

int ev_row(int t, int b) {
    return int(ev_meta[TILES + 1 + t]) + bc64(ev_meta[t] & ((1UL << b) - 1UL));
}

uint ev_at(int row, int k) {
    return g_ev_base + uint((row * SLOTS + k) * EV_BITS);
}

// +1 on the counters selected by x, saturating at the top
void count_up(uint p, int bits, uint64_t x) {
    uint64_t top = ~0UL;
    for (int i = 0; i < bits; i++) {
        top &= ev_planes[p + uint(i)];
    }
    uint64_t carry = x & ~top;
    for (int i = 0; i < bits && carry != 0UL; i++) {
        uint64_t next = ev_planes[p + uint(i)] & carry;
        ev_planes[p + uint(i)] ^= carry;
        carry = next;
    }
}

// -1 on the counters selected by x, saturating at zero
void count_down(uint p, int bits, uint64_t x) {
    uint64_t any = 0UL;
    for (int i = 0; i < bits; i++) {
        any |= ev_planes[p + uint(i)];
    }
    uint64_t borrow = x & any;
    for (int i = 0; i < bits && borrow != 0UL; i++) {
        uint64_t next = ~ev_planes[p + uint(i)] & borrow;
        ev_planes[p + uint(i)] ^= borrow;
        borrow = next;
    }
}

// which of the 64 counters are >= value
uint64_t at_least(uint p, int bits, int value) {
    uint64_t gt = 0UL, eq = ~0UL;
    for (int i = bits - 1; i >= 0; i--) {
        uint64_t plane = ev_planes[p + uint(i)];
        if (((value >> i) & 1) != 0) {
            eq &= plane;
        } else {
            gt |= eq & plane;
            eq &= ~plane;
        }
    }
    return gt | eq;
}

int count_at_least(int row, int t, int b, int bits, int value) {
    int total = 0;
    for (int k = 0; k < SLOTS; k++) {
        total += bc64(at_least(ev_at(row, k), bits, value) & allowed(t, b, k));
    }
    return total;
}

// the words the read-off picks from, made again from the counters
// and the mask (no arrays: arrays of slot words here took the whole shader to 256 registers
// and spills). lo: the read-off's line.
const int PICK_TRIM = 0;  // wired ties: what a trim takes away
const int PICK_FILL = 1;  // unwired ties: what a fill adds
uint64_t ev_above(int row, int t, int b, int k, int bits, int lo) {
    int top = (1 << bits) - 1;
    return lo < top ? at_least(ev_at(row, k), bits, lo + 1) & allowed(t, b, k) : 0UL;
}
uint64_t ev_tie(int row, int t, int b, int k, int bits, int lo) {
    return at_least(ev_at(row, k), bits, lo) & allowed(t, b, k) & ~ev_above(row, t, b, k, bits, lo);
}
uint64_t pick_word(int which, int row, int t, int b, int k, int bits, int lo) {
    uint64_t m = masks[mask_at(t, b, k)];
    uint64_t at = at_least(ev_at(row, k), bits, lo) & allowed(t, b, k);
    uint64_t above = ev_above(row, t, b, k, bits, lo);
    return which == PICK_TRIM ? m & ~above : at & ~above & ~m;
}

// co-activity's pick: heard last tick and not wired (min_pick_one over candidates FRESH)
bool fresh_pick(inout Rng r, int t, int b, out int slot, out int bit) {
    slot = 0;
    bit = 0;
    int total = 0;
    for (int k = 0; k < SLOTS; k++) {
        total += bc64(candidates(FRESH, 0, t, b, k));
    }
    if (total == 0) {
        return false;
    }
    int at = int(rng_below(r, uint(total)));
    for (int k = 0; k < SLOTS; k++) {
        uint64_t w = candidates(FRESH, 0, t, b, k);
        int c = bc64(w);
        if (at < c) {
            slot = k;
            bit = nth_bit(w, at);
            return true;
        }
        at -= c;
    }
    return false;
}

// rebuild neuron (t, b)'s mask from its counters
void read_off(int t, int b, int row, Genome G, inout Rng r) {
    const int bits = G.ev_bits, top = (1 << bits) - 1;
    if (G.ev_read == 2) {
        for (int k = 0; k < SLOTS; k++) {
            masks[mask_at(t, b, k)] = at_least(ev_at(row, k), bits, (top >> 1) + 1) & allowed(t, b, k);
        }
        return;
    }
    if (G.ev_read == 1) {
        // the line steps once toward the budget, then the mask is everything at or above it
        uint at = g_line_base + uint(row);
        int line = int(ev_lines[at]);
        int have = count_at_least(row, t, b, bits, line);
        int moved = have > G.budget && line < top ? line + 1
                    : have < G.budget && line > 0 ? line - 1
                                                  : line;
        ev_lines[at] = uint8_t(moved);
        for (int k = 0; k < SLOTS; k++) {
            masks[mask_at(t, b, k)] = at_least(ev_at(row, k), bits, moved) & allowed(t, b, k);
        }
        return;
    }
    // the highest value with at least `budget` counters at or above it
    int lo = 0, hi = top;
    while (lo < hi) {
        int mid = (lo + hi + 1) / 2;
        if (count_at_least(row, t, b, bits, mid) >= G.budget) {
            lo = mid;
        } else {
            hi = mid - 1;
        }
    }
    // the sure ones and the ties that were wired, then trim or fill the ties at random
    int sure = 0, wired = 0;
    for (int k = 0; k < SLOTS; k++) {
        uint64_t above = ev_above(row, t, b, k, bits, lo);
        uint64_t kept = ev_tie(row, t, b, k, bits, lo) & masks[mask_at(t, b, k)];
        masks[mask_at(t, b, k)] = above | kept;
        sure += bc64(above);
        wired += bc64(kept);
    }
    // one of trim (take wired ties away) or fill (wire free ties) runs; either flips the bit.
    // the words it picks from are kept in one array: lavapipe crashed when they were made
    // again from the counters for each pick
    const int need = G.budget - sure, which = wired > need ? PICK_TRIM : PICK_FILL;
    uint64_t pool[SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        pool[k] = pick_word(which, row, t, b, k, bits, lo);
    }
    for (; wired != need; wired += which == PICK_TRIM ? -1 : 1) {
        int total = 0;
        for (int k = 0; k < SLOTS; k++) {
            total += bc64(pool[k]);
        }
        if (total == 0) {
            break;
        }
        int at = int(rng_below(r, uint(total)));
        for (int k = 0; k < SLOTS; k++) {
            int c = bc64(pool[k]);
            if (at >= 0 && at < c) {
                uint64_t flip = 1UL << nth_bit(pool[k], at);
                pool[k] ^= flip;
                masks[mask_at(t, b, k)] ^= flip;
            }
            at -= c;
        }
    }
}

// what slot k of neuron (t, b) heard at the spikes it is graded for (graded: its latest spike
// if noise, its latest spike, or every spike, by ev_spikes), or on the last tick (quiet)
uint64_t ev_heard(int t, int b, int k, bool graded, Genome G) {
    if (!graded) {
        return heard(0, t, b, k);
    }
    const uint noise_base = g_ring_base + uint(RING * STRIDE);
    uint64_t bit = 1UL << b, x = 0UL;
    bool seen = false;
    for (int a = 0; a < G.credit_ticks; a++) {
        uint index = uint(((g_now - a) & (RING - 1)) * STRIDE + t);
        bool spiked = (rings[g_ring_base + index] & bit) != 0UL;
        bool kind = G.ev_spikes == 0 ? (rings[noise_base + index] & bit) != 0UL : spiked;
        if (kind && (G.ev_spikes == 2 || !seen)) {
            x |= heard(a, t, b, k);
        }
        seen = seen || spiked;
    }
    return x;
}

// move every counter of neuron (t, b) whose source it heard, up or down, then read the mask
// (ev_inh 1: inhibitory sources the other way)
void ev_move(int t, int b, bool graded, bool up, Genome G, inout Rng r) {
    const int row = ev_row(t, b);
    for (int k = 0; k < SLOTS; k++) {
        uint64_t sel = ev_heard(t, b, k, graded, G) & allowed(t, b, k);
        if (sel == 0UL) {
            continue;
        }
        uint64_t flip = G.ev_inh != 0 ? sel & s_inh[source_tile(t, b, k)] : 0UL;
        uint p = ev_at(row, k);
        count_up(p, G.ev_bits, up ? sel & ~flip : flip);
        count_down(p, G.ev_bits, up ? flip : sel & ~flip);
    }
    read_off(t, b, row, G, r);
}

// co-activity on a counter neuron (ev_cowire 1): one source heard last tick goes up one
void ev_cowire(int t, int b, Genome G, inout Rng r) {
    int slot = 0, bit = 0;
    if (!fresh_pick(r, t, b, slot, bit)) {
        return;
    }
    int row = ev_row(t, b);
    count_up(ev_at(row, slot), G.ev_bits, 1UL << bit);
    read_off(t, b, row, G, r);
}

// a reward better (or worse) than average, at chance shift: per tile, which counter neurons
// move (their latest spike if noise, ev_spikes 0; their latest spike, 1; every spike, 2) and
// which quiet ones move the other way; then each of them, per neuron
void ev_reward(uint64_t key, uint64_t tick, Genome G, bool up, int shift, int tid, int WG) {
    const uint noise_base = g_ring_base + uint(RING * STRIDE);
    barrier(); // grade() is done with the shared words
    for (int t = tid; t < TILES; t += WG) {
        uint64_t mine = ev_mine(t, G), graded = 0UL, quiet = 0UL;
        if (mine != 0UL) {
            uint64_t seen = 0UL, at[8];
            for (int c = 0; c < 8; c++) {
                at[c] = 0UL;
            }
            for (int a = 0; a < G.credit_ticks; a++) {
                uint index = uint(((g_now - a) & (RING - 1)) * STRIDE + t);
                uint64_t spikes = rings[g_ring_base + index];
                uint64_t kind = G.ev_spikes == 0 ? rings[noise_base + index] : spikes;
                graded |= G.ev_spikes == 2 ? kind : kind & ~seen;
                seen |= spikes;
                for (int c = 7; c > 0; c--) {
                    at[c] |= at[c - 1] & spikes;
                }
                at[0] |= spikes;
            }
            uint64_t fired = at[G.ev_fired_min - 1];
            Rng r = rng_at(key, tick, SITE_EV_GRADE, uint(t), 0u);
            graded &= mine & fired & sparse_word(r, shift + G.ev_shift);
            if (G.ev_quiet != 0) {
                uint64_t low = G.ev_quiet == 2 ? ~fired : ~seen;
                quiet = mine & low & sparse_word(r, shift + G.ev_shift + G.ev_quiet_shift);
            }
        }
        s_words[0][t] = graded;
        s_words[1][t] = quiet;
    }
    barrier();
    for (int n = tid; n < N; n += WG) {
        int t = tile_of(n), b = n & 63;
        if (uniform64(s_words[0][t] | s_words[1][t]) == 0UL) {
            continue;
        }
        // graded, or quiet through the window (what it heard last, the other way)
        uint64_t bit = 1UL << b;
        bool graded = (s_words[0][t] & bit) != 0UL;
        if (graded || (s_words[1][t] & bit) != 0UL) {
            Rng r = rng_at(key, tick, SITE_EV_MOVE, uint(n), graded ? 0u : 1u);
            ev_move(t, b, graded, graded ? up : !up, G, r);
        }
    }
}
