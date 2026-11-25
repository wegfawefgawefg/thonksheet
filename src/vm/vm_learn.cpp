#include "blob/random_words.hpp"
#include "vm/vm.hpp"

#include <cstring>

// what an outcome does to the bank: evidence on the entries that just fired, one entry
// rewritten to store the answer, and the operations of the entries since the last outcome
// thinned on bad.

static bool cell_on(const uint64_t* words, int n) {
    return words && ((words[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1);
}

// per motor cell: +1 toward, -1 away, 0 not a button cell. on bad the pressed button's cells
// go away and the other buttons' come toward; on good the reverse (used by ev_good and by
// writing). returns the number of buttons' cells that are not pressed.
static int outcome_delta(VmBlob& v, int valence) {
    const Blob& b = v.shell;
    int others = 0;
    for (int j = 0; j < v.motor_cells; j++) {
        int n = v.motor_cell[j];
        int d = 0;
        if (cell_on(b.motor_attached, n)) {
            bool pressed = cell_on(b.motor_acted, n);
            d = pressed ? valence : -valence;
            others += !pressed;
        }
        v.delta[j] = (int8_t)d;
    }
    return others;
}

static int8_t clamp_ev(int value, int max) {
    return (int8_t)(value > max ? max : (value < -max ? -max : value));
}

// evidence: entries that fired in the last credit_ticks ticks move by the delta
static void move_evidence(VmBlob& v) {
    int cells = v.motor_cells;
    int max = v.vg.ev_max;
    for (int e = 0; e < v.vg.entries; e++) {
        uint32_t stamp = v.last_fired[e];
        if (stamp == 0 || v.now - stamp >= (uint32_t)v.vg.credit_ticks) {
            continue;
        }
        int8_t* row = v.ev + (size_t)e * cells;
        for (int j = 0; j < cells; j++) {
            if (v.delta[j]) {
                row[j] = clamp_ev(row[j] + v.delta[j], max);
            }
        }
        v.stats.ev_moves++;
    }
}

// inner: on bad, entries that fired since the last outcome lose mask bits by chance
static void thin_operations(VmBlob& v) {
    int mw = v.m_words;
    for (int e = 0; e < v.vg.entries; e++) {
        if (v.last_fired[e] <= v.last_outcome) {
            continue;
        }
        uint64_t* s = v.set + (size_t)e * mw;
        uint64_t* c = v.clr + (size_t)e * mw;
        for (int w = 0; w < mw; w++) {
            s[w] &= ~sparse_word(v.rng, v.vg.inner_shift);
            c[w] &= ~sparse_word(v.rng, v.vg.inner_shift);
        }
        v.stats.inner_hits++;
    }
}

// keep about `want` of the on bits of words[0..n): each with chance want / on
static void sample_on_bits(VmBlob& v, const uint64_t* from, uint64_t* to, int n, int want) {
    int on = 0;
    for (int w = 0; w < n; w++) {
        on += __builtin_popcountll(from[w]);
    }
    for (int w = 0; w < n; w++) {
        uint64_t kept = 0;
        for (uint64_t bits = from[w]; bits; bits &= bits - 1) {
            if (on <= want || (int)rng_below(v.rng, (uint32_t)on) < want) {
                kept |= bits & (~bits + 1);
            }
        }
        to[w] = kept;
    }
}

void vm_address_from_state(VmBlob& v, int e, bool m_part) {
    uint64_t* a = v.addr + (size_t)e * v.a_words;
    const VmGenome& vg = v.vg;
    sample_on_bits(v, vm_x(v), a, v.x_words, (v.x_words * 64) >> vg.addr_x_shift);
    if (m_part) {
        sample_on_bits(v, vm_m(v), a + v.x_words, v.m_words, v.free_cells >> vg.addr_m_shift);
    } else {
        std::memset(a + v.x_words, 0, sizeof(uint64_t) * (size_t)v.m_words);
    }
    if (vg.addr_o) {
        sample_on_bits(v, vm_o(v), a + v.x_words + v.m_words, v.o_words,
                       v.motor_cells >> vg.addr_o_shift);
    }
    int xs = 0;
    for (int w = 0; w < v.x_words; w++) {
        xs += __builtin_popcountll(a[w]);
    }
    v.x_size[e] = (uint16_t)xs;
}

// the quietest of a few sampled entries, never one that fired this tick
int vm_quietest_entry(VmBlob& v) {
    int best = -1;
    uint32_t best_stamp = 0;
    for (int i = 0; i < v.vg.write_sample; i++) {
        int e = (int)rng_below(v.rng, (uint32_t)v.vg.entries);
        uint32_t stamp = v.last_fired[e];
        if (stamp == v.now) {
            continue;
        }
        if (best < 0 || stamp < best_stamp) {
            best = e;
            best_stamp = stamp;
        }
    }
    return best;
}

// writing: one quiet entry now answers this state. its address is a sample of the state's
// on bits; its evidence is the outcome's delta at write_ev (the other buttons share it)
static void write_entry(VmBlob& v, int others) {
    int e = vm_quietest_entry(v);
    if (e < 0) {
        return;
    }
    vm_address_from_state(v, e, true);
    int cells = v.motor_cells;
    int8_t* row = v.ev + (size_t)e * cells;
    // pressed-cell strength, and the share each other button's cell gets
    int pressed_cells = 0;
    for (int j = 0; j < cells; j++) {
        pressed_cells += v.delta[j] != 0 && cell_on(v.shell.motor_acted, v.motor_cell[j]);
    }
    int share = others > 0 && pressed_cells > 0 ? v.vg.write_ev * pressed_cells / others : 0;
    share = share > 0 ? share : 1;
    for (int j = 0; j < cells; j++) {
        bool pressed = cell_on(v.shell.motor_acted, v.motor_cell[j]);
        int d = v.delta[j];
        row[j] = (int8_t)(d == 0 ? 0 : (pressed ? d * v.vg.write_ev : d * share));
    }
    if (v.vg.write_effect) {
        vm_random_effect(v, e, v.vg.set_shift, v.vg.clr_shift);
    } else {
        std::memset(v.set + (size_t)e * v.m_words, 0, sizeof(uint64_t) * (size_t)v.m_words);
        std::memset(v.clr + (size_t)e * v.m_words, 0, sizeof(uint64_t) * (size_t)v.m_words);
    }
    v.last_fired[e] = v.now; // a new entry counts as just used
    v.stats.writes++;
}

void vm_reward(Blob& blob, int valence) {
    VmBlob& v = vm_of(blob);
    if (!v.vg.learning_on || valence == 0) {
        v.last_outcome = v.now;
        return;
    }
    int sign = valence > 0 ? 1 : -1;
    int others = outcome_delta(v, sign);
    // evidence moves on bad only, unless ev_good
    if (sign < 0 || v.vg.ev_good) {
        move_evidence(v);
    }
    if (sign < 0 && v.vg.inner_on) {
        thin_operations(v);
    }
    if ((sign < 0 && v.vg.write_on_bad) || (sign > 0 && v.vg.write_on_good)) {
        write_entry(v, others);
    }
    v.last_outcome = v.now;
}

// an entry whose operation sets at least one bit: only such an entry counts as an input's
// detector, so an answer entry covering the same input does not stop the event's mark
static bool writes_something(const VmBlob& v, int e) {
    const uint64_t* s = v.set + (size_t)e * v.m_words;
    for (int w = 0; w < v.m_words; w++) {
        if (s[w]) {
            return true;
        }
    }
    return false;
}

// events: the input changed, and no firing entry is a detector of it (an entry whose address
// covers this input: at least 3/4 of its X part is on now, and that part is at least half
// of what is on). a quiet entry becomes one, with a fresh code as its operation.
void vm_events(VmBlob& v) {
    const uint64_t* x = vm_x(v);
    int changed = 0, on = 0;
    for (int w = 0; w < v.x_words; w++) {
        changed += __builtin_popcountll(x[w] ^ v.x_last[w]);
        on += __builtin_popcountll(x[w]);
        v.x_last[w] = x[w];
    }
    if (!v.vg.learning_on || !v.vg.event_on || changed == 0 || on < v.vg.event_min_bits) {
        return;
    }
    for (int k = 0; k < v.vg.active; k++) {
        int e = v.winners[k];
        int size = v.x_size[e];
        if (4 * v.match_x[e] >= 3 * size && 2 * size >= on && writes_something(v, e)) {
            return; // already known
        }
    }
    int e = vm_quietest_entry(v);
    if (e < 0) {
        return;
    }
    vm_address_from_state(v, e, v.vg.event_m != 0);
    std::memset(v.ev + (size_t)e * v.motor_cells, 0, (size_t)v.motor_cells);
    vm_random_effect(v, e, v.vg.event_set_shift, v.vg.event_clr_shift);
    // counts as just used; with refraction not as firing on the tick before, so it acts
    v.last_fired[e] = v.vg.refract && v.now > 2 ? v.now - 2 : v.now;
    v.stats.events++;
}
