#pragma once

#include "blob/blob.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>

// The vm engine (idea 135, the owner's vvm in bits). One state, a long bit vector, and a bank
// of entries found by content. Each tick the state picks the entries whose address it
// matches best (k winners by popcount), and their operations are applied to the state: set
// these bits, clear those. The new state is the next address. No program counter, no
// separate memory.
//
// The state has three parts, one after the other in `state`:
//   X  the world's part, overwritten each tick: every sensor cell that is on ORs a fixed
//      random code of a few bits into X.
//   M  the machine's own bits, one per sheet cell that is neither sensor nor motor. a bit
//      keeps its value until an entry sets or clears it.
//   O  one bit per motor cell, voted each tick by the active entries' output evidence.
//
// Learning, all local to the entries that fired:
//   evidence  each entry keeps a small signed counter per motor cell. on bad, entries that
//             fired in the last credit_ticks ticks move away from the pressed button's cells
//             and toward the other buttons' (finding 15's error rule).
//   writing   on an outcome, the quietest of a few sampled entries is rewritten: its address
//             becomes a sample of the current state's on bits, its evidence the answer that
//             outcome implies, its operation a fresh random one. one-shot storage.
//   inner     on bad, entries that fired since the last outcome lose bits of their set and
//             clear masks (drift toward doing nothing); on good they keep them.
//   events    no reward needed: when the input changes to something no firing entry knows,
//             a quiet entry is rewritten to recognise this input (its address is the input's
//             on bits) and its operation becomes a fresh random code: set a few M bits, clear
//             a few. so every new event leaves its own mark in M, and older marks fade one
//             event at a time, not one tick at a time.
//
// It stands behind the same Blob shell the plug uses: the shell's spike map shows the
// sensor cells as sensed, the motor cells as O and every other cell as its M bit.

// plain integers; a "shift" is a chance or density of 2^-shift. "--v.name value" sets one.
struct VmGenome {
    int learning_on = 1;
    int entries = 2048;       // operations in the bank
    int active = 16;          // entries that fire each tick (k winners)
    int x_words = 16;         // size of the world's part, in 64-bit words
    int x_bits = 16;          // code bits per sensor cell
    int addr_x_shift = 3;     // address density over X
    int addr_m_shift = 5;     // address density over M
    int addr_o = 0;           // 1: addresses also see O (the machine's own last output)
    int addr_o_shift = 3;     // address density over O
    int set_shift = 9;        // set-mask density over M at birth
    int clr_shift = 7;        // clear-mask density over M at birth
    int m_target = 128;       // wanted M bits on per 1,024
    int out_noise_shift = 4;  // a motor cell also fires by chance
    int out_bar_bits = 9;     // a cell fires when its vote beats a random number this wide
    int credit_ticks = 20;    // evidence: entries this recent are graded by an outcome
    int ev_max = 31;          // evidence saturates at +-this
    int ev_good = 0;          // 1: on good, the pressed cells gain and the others lose too
    int write_on_bad = 1;     // one-shot writing after a bad outcome
    int write_on_good = 1;    // ... and after a good one
    int write_keep_shift = 1; // a written address keeps each on bit with this chance
    int write_ev = 8;         // evidence a written entry starts with
    int write_sample = 16;    // entries sampled to find the quietest
    int write_effect = 0;     // 1: a written entry gets a fresh random operation, 0: none
    // inner rule on M effects. off: as specified it strips the event entries' codes (they
    // fire before every outcome, right or wrong), and memory goes with them
    int inner_on = 0;
    int inner_shift = 3;     // on bad, each mask bit of an eligible entry drops by chance
    int event_on = 1;        // unsupervised event entries
    int event_min_bits = 8;  // an input with fewer X bits on is not an event
    int event_set_shift = 4; // an event's code: set-mask density over M
    int event_clr_shift = 1; // and clear-mask density (half of M: older marks fade by half)
    // 1: an event entry's address also samples M (its context), and an input in a new
    // context is a new event: transitions become (input, state) -> a new mark
    int event_m = 0;
    // the last this-many words of M form a ring that slides one bit along every tick: a
    // permutation, so a mark keeps all its bits and its place tells how long ago it was made
    int m_slide_words = 0;
    // refraction, as in production systems: an entry's operation acts only on the tick it
    // starts to fire, not again while it keeps firing. 0: every tick it is a winner
    int refract = 1;
};

struct VmStats {
    uint64_t writes = 0;     // entries rewritten
    uint64_t ev_moves = 0;   // entry evidence rows moved
    uint64_t inner_hits = 0; // entries whose masks were thinned
    uint64_t events = 0;     // event entries written
    uint32_t m_on = 0;       // M bits on after the latest tick
};

struct VmBlob {
    Blob shell; // stays first
    VmGenome vg;

    // word counts of the parts and of an address (X, M, and O when addr_o)
    int x_words = 0, m_words = 0, o_words = 0, a_words = 0, s_words = 0;
    // cells: sensor_cell[i] is the neuron of sensor cell i, likewise motor and free (M)
    int sensor_cells = 0, motor_cells = 0, free_cells = 0;
    int32_t* sensor_cell = nullptr;
    int32_t* motor_cell = nullptr;
    int32_t* free_cell = nullptr;
    uint16_t* x_code = nullptr; // [sensor_cells * x_bits] bit positions in X

    uint64_t* state = nullptr;      // X | M | O, s_words
    uint64_t* addr = nullptr;       // [entries * a_words]
    uint64_t* set = nullptr;        // [entries * m_words]
    uint64_t* clr = nullptr;        // [entries * m_words]
    int8_t* ev = nullptr;           // [entries * motor_cells]
    uint32_t* last_fired = nullptr; // tick stamp per entry, 0 never
    uint16_t* match = nullptr;      // per entry, this tick: X part plus M part (plus O)
    uint16_t* match_x = nullptr;    // per entry, this tick: the X part alone
    uint16_t* x_size = nullptr;     // per entry: bits in the X part of its address
    uint64_t* x_last = nullptr;     // X on the previous tick
    int32_t* winners = nullptr;     // the active entries this tick, vg.active
    uint8_t* fresh = nullptr;       // per winner: 1 if it did not fire on the tick before
    int16_t* vote = nullptr;        // per motor cell
    int8_t* delta = nullptr;        // per motor cell, scratch for an outcome

    uint32_t now = 0;          // ticks lived, the stamp of the latest tick
    uint32_t last_outcome = 0; // tick of the latest reward
    uint64_t rng[4] = {};
    VmStats stats;

    FILE* trace = nullptr; // when set, one line per tick (vm_trace.cpp)
};

Blob* vm_birth(const Genome& genome, const VmGenome& vg, uint64_t seed);
void vm_free(Blob* blob);
// one tick: input code, match, winners, operations, output vote. the world writes sense_in.
void vm_tick(Blob& blob);
// an outcome arrives (+1 good, -1 bad); motor_acted names the pressed button's cells
void vm_reward(Blob& blob, int valence);

inline VmBlob& vm_of(Blob& blob) {
    return *reinterpret_cast<VmBlob*>(&blob);
}
inline const VmBlob& vm_of(const Blob& blob) {
    return *reinterpret_cast<const VmBlob*>(&blob);
}

inline uint64_t* vm_x(VmBlob& v) {
    return v.state;
}
inline uint64_t* vm_m(VmBlob& v) {
    return v.state + v.x_words;
}
inline uint64_t* vm_o(VmBlob& v) {
    return v.state + v.x_words + v.m_words;
}

// a fresh random operation (set and clear masks at these densities) for entry e
void vm_random_effect(VmBlob& v, int e, int set_shift, int clr_shift);
// an address for entry e sampled from the state's on bits, about as many bits per part as a
// random address has; m_part 0 leaves the M part empty (vm_learn.cpp)
void vm_address_from_state(VmBlob& v, int e, bool m_part);
// the quietest of a few sampled entries that did not fire this tick, -1 if none
int vm_quietest_entry(VmBlob& v);
// after the winners are known: write an event entry if the input is new (vm_learn.cpp)
void vm_events(VmBlob& v);
// bytes the engine itself allocates (not the shell)
size_t vm_bytes(const VmBlob& v);
// "--v.name value" overrides one number
void apply_vm_args(int argc, char** argv, VmGenome& vg);
// one trace line for the latest tick (vm_trace.cpp)
void vm_trace_tick(VmBlob& v);
