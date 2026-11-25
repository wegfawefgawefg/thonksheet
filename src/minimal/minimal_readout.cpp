#include "blob/random_words.hpp"
#include "minimal/minimal.hpp"
#include "minimal/minimal_steps.hpp"

// the readout (idea 58's winner, ported for idea 65): an error rule at the buttons' motor
// cells, the perceptron rule in bits. the world marks which cells belong to a button
// (motor_attached) and which belong to the button just pressed (motor_acted, an efference
// copy), the way it writes sense_in. on bad, the pressed button's cells lose evidence for the
// sources they heard over the last readout_ticks ticks and the other buttons' cells gain it;
// on good nothing moves (readout_good 1: the mirror of bad). each tick a source was heard is
// its own chance of 2^-readout_shift, so a source heard often moves often. sign-aware:
// gaining raises an excitatory source's counter and lowers an inhibitory one's. the counters
// are minimal_evidence.cpp's (ev_bits wide, wired sources born ev_init above zero); after a
// move the cell's mask is read off them (the top `budget`). balance: the other buttons' cells
// move at a chance lowered by their number over the pressed button's (to a power of two), so
// with many buttons the gains do not outweigh the loss. "heard" respects slot delays and far
// slots: it is what the cell's slot read on that tick.

// move cell n's counters for what it heard in the window: gain or lose
static void move_cell(MinimalBlob& m, int n, bool gain, int shift) {
    if (m.ev_row[n] < 0) {
        return;
    }
    bool moved = false;
    for (int k = 0; k < m.slots; k++) {
        uint64_t allowed = min_allowed(m, n, k);
        if (!allowed) {
            continue;
        }
        uint64_t inh = m.shell.inhibitory[min_source_tile(m, n, k)];
        // readout_inh 0: inhibitory sources keep their counters (only excitatory ones move)
        allowed &= m.mg.readout_inh ? ~0ull : ~inh;
        for (int a = 0; a < m.mg.readout_ticks; a++) {
            uint64_t heard = min_heard(m, a, n, k) & allowed;
            if (!heard) {
                continue;
            }
            uint64_t chance = heard & sparse_word(m.rng, shift);
            if (!chance) {
                continue;
            }
            uint64_t more = chance & ~inh, less = chance & inh;
            min_evidence_move(m, n, k, gain ? more : less, gain ? less : more);
            moved = true;
        }
    }
    if (moved) {
        min_evidence_read(m, n);
    }
}

void min_readout_reward(MinimalBlob& m, int valence) {
    const Blob& b = m.shell;
    const MinimalGenome& mg = m.mg;
    if (!b.motor_attached || !b.motor_acted || valence == 0 || (valence > 0 && !mg.readout_good)) {
        return;
    }
    bool good = valence > 0;
    int other_shift = mg.readout_shift;
    if (mg.readout_balance) {
        int64_t pressed = 0, other = 0;
        for (int t = 0; t < b.tile_count; t++) {
            uint64_t acted = b.motor_acted[t] & b.motor_attached[t];
            pressed += __builtin_popcountll(acted);
            other += __builtin_popcountll(b.motor_attached[t] & ~acted);
        }
        for (int e = 0; pressed > 0 && (pressed << e) < other; e++) {
            other_shift++;
        }
    }
    // pressed cells move with the valence, the other buttons' cells against it
    for (int t = 0; t < b.tile_count; t++) {
        uint64_t pressed = b.motor_acted[t] & b.motor_attached[t];
        uint64_t other = b.motor_attached[t] & ~b.motor_acted[t];
        while (pressed) {
            int n = t * TILE_NEURONS + __builtin_ctzll(pressed);
            pressed &= pressed - 1;
            move_cell(m, n, good, mg.readout_shift);
        }
        while (other) {
            int n = t * TILE_NEURONS + __builtin_ctzll(other);
            other &= other - 1;
            move_cell(m, n, !good, other_shift);
        }
    }
}
