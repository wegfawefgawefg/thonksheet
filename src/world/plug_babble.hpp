#pragma once

#include "blob/blob.hpp"
#include "world/plug.hpp"
#include "world/world_random.hpp"

#include <cstdint>

// Two things the world does around the plug, both off by default.
//
// Babbling (idea 60): now and then, for one whole task step, the world urges one button
// picked uniformly at random: each tick, each of that button's motor cells is urged to fire
// with a set chance (blob.urge). The engines fire an urged cell on top of its inputs and
// learn from it like any other spike. The button is never chosen from the right answer.
//
// Balance (idea 61): whichever of good and bad has been commoner lately is dropped with the
// chance that makes the two arrive equally often. The task still scores every step; only
// the blob's signal is thinned.

struct BabbleSettings {
    int chance = 0;       // steps babbled per 1,024; 0 is off
    int fade = 0;         // 1: the chance is scaled by the recent share of bad answers
    int fire = 64;        // chance per 256 that an urged cell is urged on a tick
    int balance = 0;      // 1: thin the commoner of good and bad
    int memory_shift = 6; // running rates forget with weight 2^-shift per step
    int report = 0;       // 1: write signal counts even when both are off
};

struct Babble {
    BabbleSettings settings;
    WorldRandom random;
    int button = -1; // urged this step, -1 none

    // running rates per step, x65536: good and bad as the task gave them, and the share of
    // bad among rewarded steps that were not babbled (what fade reads)
    int32_t good_rate = 0, bad_rate = 0;
    int32_t bad_share = 32768;

    // whole-life counts
    int64_t steps = 0, babbled = 0, obeyed = 0; // obeyed: the urged button won
    int64_t good_raw = 0, bad_raw = 0, good_sent = 0, bad_sent = 0;
    // this window's tally[0..1] of babbled steps, to score the window without them
    int64_t window_right = 0, window_tries = 0;
};

inline bool babble_on(const BabbleSettings& s) {
    return s.chance > 0 || s.balance || s.report;
}

void babble_start(Babble& bb, const BabbleSettings& settings, uint64_t task_seed);
// a step's first tick: pick whether to babble and which of `buttons`
void babble_step_begin(Babble& bb, int buttons);
// every choosing tick, after plug_write: the urged cells for this tick
void babble_urge(Babble& bb, const Plug& plug, Blob& blob);
// the step is over: no cell is urged until the next babbled step
void babble_step_end(Babble& bb, const Plug& plug, Blob& blob);
// the task's answer to the step: counts it, and returns the valence the blob should get
// (0 when balance drops it). move is what the plug read; right/tries are the task's
// tally[0..1] moved by this step.
int babble_signal(Babble& bb, int valence, int move, int64_t right, int64_t tries);
// the window's right/tries with babbled steps left out; starts a new window
double babble_window_clean(Babble& bb, int64_t right, int64_t tries);
