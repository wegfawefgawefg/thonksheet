#pragma once

#include "blob/blob.hpp"

#include <cstddef>
#include <cstdint>

// The minimal engine. Three pieces and one switchable extra:
//  1. a neuron is a popcount over a one-bit mask of its 576 local sources (own tile and the
//     8 around it, last tick only), excitatory minus gain x inhibitory, against a threshold.
//  2. each neuron raises its own threshold by chance when it fires and lowers it by a much
//     smaller chance every tick. nothing else keeps activity alive.
//  3. noise is the experiment: spontaneous spikes are a random word ORed in at firing, so the
//     engine knows which spikes were noise. on a reward better than the running average, a
//     neuron whose latest spike was noise connects to a source that was active just before
//     it; on worse, it disconnects from one. a fixed budget per neuron: every gain costs a
//     loss (a random connection, or the least recently useful one).
//  4. (switchable) wiring by co-activity: a firing neuron by chance swaps one connection for
//     a source that was just active.
// Grown back one piece at a time, each off by default (docs/results-2026-11-24-minimal-grow.md):
//  - far slots: up to 2 extra source tiles per neuron, drawn at birth with a long tail,
//    excitatory senders only, inside the same budget.
//  - delays: each slot reads the map 1..delay_max ticks back, one small number per neuron
//    per slot; far slots read later the further away they are.
//
// It stands behind the same Blob shell the plug and tasks use, as world/standin does: the
// shell holds sense_in, the sensor/motor/inhibitory masks and a copy of the latest spike map.
// The shell is the first member of MinimalBlob, so a Blob* from minimal_birth converts back.

constexpr int MIN_SLOTS = 9;   // own tile and the 8 around it
constexpr int MIN_FAR_MAX = 2; // far slots a neuron may have, after the local ones
constexpr int MIN_SLOTS_MAX = MIN_SLOTS + MIN_FAR_MAX;
constexpr int MIN_DELAY_MAX = 15; // longest slot delay in ticks
constexpr int MIN_HISTORY = 32;   // spike and noise maps kept, a power of two

// plain integers; a "shift" is a chance of 2^-shift. "--m.name value" overrides one.
struct MinimalGenome {
    int learning_on = 1;  // 0: no wiring change at all (regulation and noise still run)
    int budget = 48;      // connections per neuron, set at birth
    int budget_on = 1;    // 0: gains and losses are not paired, fan-in drifts
    int inhibit_gain = 4; // an inhibitory spike counts this many excitatory ones
    int threshold_min = 1, threshold_max = 4; // drawn at birth
    int threshold_cap = 255;
    int up_shift = 4;      // on firing, raise own threshold with this chance
    int down_shift = 9;    // every tick, lower own threshold with this chance
    int noise_shift = 9;   // spontaneous spike chance per neuron per tick
    int credit_ticks = 20; // a noise spike this recent is graded by a reward (1..31-delay_max)
    int learn_shift = 0;   // chance a graded neuron makes one swap, at the largest surprise
    int avg_shift = 5;     // reward average follows over about 2^this rewards
    int grade_mode = 0;    // 0 noise spikes only, 1 every recent spike (the ablation)
    int drop_mode = 0;     // what a gain costs: 0 a random connection, 1 least recently useful
    int cowire_on = 1;     // wiring by co-activity
    int cowire_shift = 6;  // on firing, chance to swap one connection for a just-active source
    // co-activity wiring only while a sensor line is attached (0: always)
    int cowire_needs_input = 1;
    int far_slots = 0;          // 0..2 far slots per neuron
    int far_percent = 15;       // share of non-sensor neurons that get each far slot
    int far_reach = 3;          // typical far distance in tiles (2 and up, long tail)
    int delay_max = 1;          // slot delays drawn in 1..this; 1 is the one-tick reading
    int far_delay_per_tile = 1; // far slot delay: distance x this, capped at delay_max
};

struct MinimalStats {
    uint64_t gained = 0, lost = 0; // connections, over the whole life
    uint64_t graded = 0;           // neuron grades from rewards
    uint64_t cowired = 0;          // swaps from co-activity
    uint64_t noise_spikes = 0;     // this tick
};

struct MinimalBlob {
    Blob shell; // stays first
    MinimalGenome mg;
    int stride = 0; // words from one map to the next (tile_count + 1, rounded to 8)
    int now = 0;    // ring index of the newest map

    // MIN_HISTORY maps each, one word per tile plus a zero word. noise holds the spikes that
    // happened only because of the noise word.
    uint64_t* spikes = nullptr;
    uint64_t* noise = nullptr;

    int slots = MIN_SLOTS; // local slots and far slots per neuron

    // local_tile[tile * 9 + k]: source tile of slot k (tile_count = off the sheet, the zero word)
    uint32_t* local_tile = nullptr;
    // far_tile[(tile * far_slots + j) * 64 + bit]: source tile of far slot j of neuron
    // (tile, bit), tile_count if it has none. null without far slots.
    uint32_t* far_tile = nullptr;
    // mask[(tile * slots + k) * 64 + bit]: which of slot k's 64 sources neuron (tile, bit) hears
    uint64_t* mask = nullptr;
    // delay[(tile * slots + k) * 64 + bit]: ticks back slot k reads (1..delay_max). null when
    // delay_max is 1.
    uint8_t* delay = nullptr;
    // same shape, drop_mode 1 only: connection was active just before its target fired, since
    // that neuron's last sweep (a clock approximation of least recently useful)
    uint64_t* used = nullptr;
    uint8_t* threshold = nullptr;

    int32_t reward_avg = 0; // running average of reward, x256
    uint64_t rng[4] = {};
    MinimalStats stats;
};

Blob* minimal_birth(const Genome& genome, const MinimalGenome& mg, uint64_t seed);
void minimal_free(Blob* blob);
// one tick: fire, regulate, wire by co-activity. the world writes sense_in before calling.
void minimal_tick(Blob& blob);
// a reward arrives (+1 good, -1 bad): grade the neurons whose recent spike was noise
void minimal_reward(Blob& blob, int valence);

inline MinimalBlob& minimal_of(Blob& blob) {
    return *reinterpret_cast<MinimalBlob*>(&blob);
}
inline const MinimalBlob& minimal_of(const Blob& blob) {
    return *reinterpret_cast<const MinimalBlob*>(&blob);
}

// ring maps by index
inline uint64_t* minimal_map(MinimalBlob& m, int index) {
    return m.spikes + (size_t)(index & (MIN_HISTORY - 1)) * m.stride;
}
inline uint64_t* minimal_noise_map(MinimalBlob& m, int index) {
    return m.noise + (size_t)(index & (MIN_HISTORY - 1)) * m.stride;
}

// connections neuron n has
int minimal_fan_in(const MinimalBlob& m, int n);
// bytes the engine itself allocates (not the shell)
size_t minimal_bytes(const MinimalBlob& m);

// "--m.name value" overrides one number
void apply_minimal_args(int argc, char** argv, MinimalGenome& mg);
