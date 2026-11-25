#include "blob/random_words.hpp"
#include "minimal/minimal.hpp"
#include "minimal/minimal_steps.hpp"

// evidence counters (idea 59). every possible source of a counter neuron has a small hidden
// counter, offset binary in ev_bits bits (2^(bits-1) is zero), stored bitsliced: plane i of a
// slot is one word holding bit i of its 64 sources' counters, so moving every heard source's
// counter at once is a ripple of word ops. a reward better than average moves up the counters
// of the sources a graded neuron heard before it fired (worse: down); a neuron quiet through
// the window moves the other way. the mask is then read off the counters: the top `budget`,
// ties kept as wired (read 0); above a line the neuron moves toward the budget (read 1); or
// every counter above zero (read 2, no budget). the tick never sees the counters.

static uint64_t* planes_of(MinimalBlob& m, int n, int k) {
    return m.ev_planes + ((int64_t)m.ev_row[n] * m.slots + k) * m.mg.ev_bits;
}
static const uint64_t* planes_of(const MinimalBlob& m, int n, int k) {
    return m.ev_planes + ((int64_t)m.ev_row[n] * m.slots + k) * m.mg.ev_bits;
}

// +1 on the counters selected by x, saturating at the top
static void count_up(uint64_t* p, int bits, uint64_t x) {
    uint64_t top = ~0ull;
    for (int i = 0; i < bits; i++) {
        top &= p[i];
    }
    uint64_t carry = x & ~top;
    for (int i = 0; i < bits && carry; i++) {
        uint64_t next = p[i] & carry;
        p[i] ^= carry;
        carry = next;
    }
}

// -1 on the counters selected by x, saturating at zero
static void count_down(uint64_t* p, int bits, uint64_t x) {
    uint64_t any = 0;
    for (int i = 0; i < bits; i++) {
        any |= p[i];
    }
    uint64_t borrow = x & any;
    for (int i = 0; i < bits && borrow; i++) {
        uint64_t next = ~p[i] & borrow;
        p[i] ^= borrow;
        borrow = next;
    }
}

// which of the 64 counters are >= value
static uint64_t at_least(const uint64_t* p, int bits, int value) {
    uint64_t gt = 0, eq = ~0ull;
    for (int i = bits - 1; i >= 0; i--) {
        if ((value >> i) & 1) {
            eq &= p[i];
        } else {
            gt |= eq & p[i];
            eq &= ~p[i];
        }
    }
    return gt | eq;
}

static int count_at_least(const MinimalBlob& m, int n, const uint64_t* allowed, int value,
                          uint64_t* out) {
    int total = 0;
    for (int k = 0; k < m.slots; k++) {
        out[k] = at_least(planes_of(m, n, k), m.mg.ev_bits, value) & allowed[k];
        total += __builtin_popcountll(out[k]);
    }
    return total;
}

// rebuild neuron n's mask from its counters
static void read_off(MinimalBlob& m, int n) {
    const MinimalGenome& mg = m.mg;
    const int slots = m.slots;
    const int top = (1 << mg.ev_bits) - 1;
    uint64_t allowed[MIN_SLOTS_MAX], next[MIN_SLOTS_MAX], above[MIN_SLOTS_MAX];
    for (int k = 0; k < slots; k++) {
        allowed[k] = min_allowed(m, n, k);
    }
    if (mg.ev_read == 2) {
        count_at_least(m, n, allowed, (top >> 1) + 1, next);
    } else if (mg.ev_read == 1) {
        // the line steps once toward the budget, then the mask is everything at or above it
        uint8_t& line = m.ev_line[m.ev_row[n]];
        int have = count_at_least(m, n, allowed, line, next);
        int moved = have > mg.budget && line < top ? line + 1
                    : have < mg.budget && line > 0 ? line - 1
                                                   : line;
        if (moved != line) {
            line = (uint8_t)moved;
            count_at_least(m, n, allowed, line, next);
        }
    } else {
        // the highest value with at least `budget` counters at or above it
        int lo = 0, hi = top;
        while (lo < hi) {
            int mid = (lo + hi + 1) / 2;
            if (count_at_least(m, n, allowed, mid, next) >= mg.budget) {
                lo = mid;
            } else {
                hi = mid - 1;
            }
        }
        int sure = lo < top ? count_at_least(m, n, allowed, lo + 1, above) : 0;
        if (lo == top) {
            for (int k = 0; k < slots; k++) {
                above[k] = 0;
            }
        }
        count_at_least(m, n, allowed, lo, next);
        // ties at the line: keep the wired ones first, then fill or trim at random
        uint64_t tie_wired[MIN_SLOTS_MAX], tie_free[MIN_SLOTS_MAX];
        int wired = 0;
        for (int k = 0; k < slots; k++) {
            uint64_t tie = next[k] & ~above[k];
            tie_wired[k] = tie & m.mask[min_index(m, n, k)];
            tie_free[k] = tie & ~tie_wired[k];
            wired += __builtin_popcountll(tie_wired[k]);
        }
        int need = mg.budget - sure;
        int slot = 0, bit = 0;
        for (; wired > need && min_pick_one(m.rng, tie_wired, slots, slot, bit); wired--) {
            tie_wired[slot] &= ~(1ull << bit);
        }
        for (; wired < need && min_pick_one(m.rng, tie_free, slots, slot, bit); wired++) {
            tie_free[slot] &= ~(1ull << bit);
            tie_wired[slot] |= 1ull << bit;
        }
        for (int k = 0; k < slots; k++) {
            next[k] = above[k] | tie_wired[k];
        }
    }
    for (int k = 0; k < slots; k++) {
        uint64_t& word = m.mask[min_index(m, n, k)];
        m.stats.gained += (uint64_t)__builtin_popcountll(next[k] & ~word);
        m.stats.lost += (uint64_t)__builtin_popcountll(word & ~next[k]);
        word = next[k];
    }
}

// move every counter of neuron n whose source is set in x, up or down, then read the mask
// (ev_inh 1: inhibitory sources the other way, since wiring them works against firing)
static void move(MinimalBlob& m, int n, const uint64_t* x, bool up) {
    for (int k = 0; k < m.slots; k++) {
        uint64_t sel = x[k] & min_allowed(m, n, k);
        if (!sel) {
            continue;
        }
        uint64_t flip = m.mg.ev_inh ? sel & m.shell.inhibitory[min_source_tile(m, n, k)] : 0;
        uint64_t* p = planes_of(m, n, k);
        count_up(p, m.mg.ev_bits, up ? sel & ~flip : flip);
        count_down(p, m.mg.ev_bits, up ? flip : sel & ~flip);
    }
    m.stats.ev_moved++;
    read_off(m, n);
}

void min_evidence_birth(MinimalBlob& m) {
    const Blob& b = m.shell;
    const MinimalGenome& mg = m.mg;
    const int bits = mg.ev_bits, zero = 1 << (bits - 1), top = (1 << bits) - 1;
    const int wired_value = zero + mg.ev_init > top ? top : zero + mg.ev_init;
    int row = 0;
    for (int n = 0; n < b.neuron_count; n++) {
        int t = n / TILE_NEURONS;
        uint64_t bit = 1ull << (n % TILE_NEURONS);
        bool mine = !(b.sensor[t] & bit) && (mg.ev_on == 2 || (b.motor[t] & bit));
        m.ev_row[n] = mine ? row : -1;
        if (!mine) {
            continue;
        }
        m.ev_tiles[t] |= bit;
        m.ev_line[row] = (uint8_t)wired_value;
        row++;
        // every counter at zero, the wired sources at ev_init above it
        for (int k = 0; k < m.slots; k++) {
            uint64_t* p = planes_of(m, n, k);
            uint64_t wired = m.mask[min_index(m, n, k)];
            for (int i = 0; i < bits; i++) {
                uint64_t base = ((zero >> i) & 1) ? ~0ull : 0;
                uint64_t set = ((wired_value >> i) & 1) ? ~0ull : 0;
                p[i] = (base & ~wired) | (set & wired);
            }
        }
    }
}

void min_evidence_reward(MinimalBlob& m, bool better, int shift) {
    const MinimalGenome& mg = m.mg;
    uint64_t graded_at[MIN_HISTORY];
    for (int t = 0; t < m.shell.tile_count; t++) {
        uint64_t mine = m.ev_tiles[t];
        if (!mine) {
            continue;
        }
        // which neurons are graded at each age, in the credit window: their latest spike if
        // it was noise (ev_spikes 0, as the plain rule), their latest spike (1), every spike (2)
        // at[c]: spiked at least c + 1 times in the window (counted up to 8)
        uint64_t seen = 0, graded = 0, at[8] = {};
        for (int a = 0; a < mg.credit_ticks; a++) {
            uint64_t spikes = minimal_map(m, m.now - a)[t];
            uint64_t kind = mg.ev_spikes == 0 ? minimal_noise_map(m, m.now - a)[t] : spikes;
            graded_at[a] = mg.ev_spikes == 2 ? kind : kind & ~seen;
            graded |= graded_at[a];
            seen |= spikes;
            for (int c = 7; c > 0; c--) {
                at[c] |= at[c - 1] & spikes;
            }
            at[0] |= spikes;
        }
        // fired: at least ev_fired_min spikes. quiet: none (ev_quiet 1) or fewer (2)
        uint64_t fired = at[mg.ev_fired_min - 1];
        min_site(m, MIN_DRAW_EV_GRADE, (uint32_t)t, 0);
        graded &= mine & fired & sparse_word(m.rng, shift + mg.ev_shift);
        uint64_t quiet = 0;
        if (mg.ev_quiet) {
            uint64_t low = mg.ev_quiet == 2 ? ~fired : ~seen;
            quiet = mine & low & sparse_word(m.rng, shift + mg.ev_shift + mg.ev_quiet_shift);
        }
        uint64_t x[MIN_SLOTS_MAX];
        while (graded) {
            int b = __builtin_ctzll(graded);
            graded &= graded - 1;
            int n = t * TILE_NEURONS + b;
            for (int k = 0; k < m.slots; k++) {
                x[k] = 0;
            }
            for (int a = 0; a < mg.credit_ticks; a++) {
                if ((graded_at[a] >> b) & 1) {
                    for (int k = 0; k < m.slots; k++) {
                        x[k] |= min_heard(m, a, n, k);
                    }
                }
            }
            m.stats.graded++;
            min_site(m, MIN_DRAW_EV_MOVE, (uint32_t)n, 0);
            move(m, n, x, better);
        }
        // quiet through the window: what it heard last, the other way
        while (quiet) {
            int n = t * TILE_NEURONS + __builtin_ctzll(quiet);
            quiet &= quiet - 1;
            for (int k = 0; k < m.slots; k++) {
                x[k] = min_heard(m, 0, n, k);
            }
            min_site(m, MIN_DRAW_EV_MOVE, (uint32_t)n, 1);
            move(m, n, x, !better);
        }
    }
}

void min_evidence_cowire(MinimalBlob& m, int n) {
    uint64_t fresh[MIN_SLOTS_MAX];
    for (int k = 0; k < m.slots; k++) {
        fresh[k] = min_heard(m, 0, n, k) & ~m.mask[min_index(m, n, k)] & min_allowed(m, n, k);
    }
    int slot = 0, bit = 0;
    if (!min_pick_one(m.rng, fresh, m.slots, slot, bit)) {
        return;
    }
    count_up(planes_of(m, n, slot), m.mg.ev_bits, 1ull << bit);
    read_off(m, n);
}

void min_evidence_ends(const MinimalBlob& m, double& low, double& high) {
    int64_t lows = 0, highs = 0, all = 0;
    const int bits = m.mg.ev_bits;
    for (int n = 0; n < m.shell.neuron_count; n++) {
        if (m.ev_row[n] < 0) {
            continue;
        }
        for (int k = 0; k < m.slots; k++) {
            const uint64_t* p = planes_of(m, n, k);
            uint64_t allowed = min_allowed(m, n, k);
            uint64_t any = 0, every = ~0ull;
            for (int i = 0; i < bits; i++) {
                any |= p[i];
                every &= p[i];
            }
            lows += __builtin_popcountll(~any & allowed);
            highs += __builtin_popcountll(every & allowed);
            all += __builtin_popcountll(allowed);
        }
    }
    low = all ? (double)lows / (double)all : 0;
    high = all ? (double)highs / (double)all : 0;
}
