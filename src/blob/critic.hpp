#pragma once

#include "blob/blob.hpp"

#include <cstddef>
#include <cstdint>

// Actor and critic (genome critic_on), after Barto, Sutton and Anderson 1983. Core-only,
// except critic_level, which a probe may read.
//
// The critic is a group of ordinary cells: the motor region's cells at half the motor stride
// (between the motor lattice's cells, so they hear the same tiles the buttons hear). Its
// level is their spike count over a step. It learns by the readout's evidence counters:
// when a step turns out better than its level said, the sources active a step ago (and a
// few steps before that, fading) gain evidence in every critic cell; worse, they lose it.
//
// The surprise is reward (against its running mean) + level now - level a step ago, in
// spikes. Beyond the margin it is good or bad, and that, not the world's raw good or bad,
// teaches the actor: the readout's button cells, through the same counters, for the press
// made a step ago and the few before it (learn_actor.cpp). With margin_percent set, the
// actor's good half for a kept step runs only when that step's vote was close (the
// pressed button's lead under margin_percent of its spikes): the margin gate of
// learn_margin.cpp, moved to the actor (learn_margin itself never runs under a critic).
//
// Everything is integers and chance. A step is whatever the world calls one: it calls
// blob_reward once per step, valence 0 included, and marks the pressed button first.

constexpr int CRITIC_STEPS = 8;  // steps of activity and presses kept
constexpr int CRITIC_PLANES = 4; // a source's spikes in one step, as a 4-bit count (0..15)

struct CriticState {
    int words = 0;       // tile_count + 1
    uint64_t* cells = 0; // the critic cells
    // per kept step: the count planes of every neuron's spikes in its last 15 ticks, and the
    // pressed button's cells. the newest is at `head`
    uint64_t* planes = 0;  // CRITIC_STEPS x CRITIC_PLANES x words
    uint64_t* acted = 0;   // CRITIC_STEPS x words
    uint64_t* touched = 0; // scratch: button cells the actor moved this step
    uint64_t* minus = 0;   // critic_opponent: the critic cells that count against
    int head = 0;
    int64_t steps = 0;      // steps seen
    uint64_t last_tick = 0; // blob tick at the last step's end
    int ticks = 0;          // ticks in the last step

    int level = 0, level_before = 0; // critic spikes this step and the step before
    int valence_before = 0;          // the world's valence for the step before
    int64_t reward_mean = 0;         // running mean of valence, x65536
    int64_t level_mean = 0;          // running mean of level, x65536
    int surprise = 0;                // the last surprise, in spikes
    int sign = 0;                    // and what it counted as: +1, -1 or 0
    int explored = 0;                // 1 if this step's press was forced at random
    // per kept step: the vote (pressed button's spikes, the runner-up's), as the world
    // marked it (plug_mark_vote, only while margin_percent is set)
    int32_t vote[CRITIC_STEPS][2] = {};
    // a diagnostic, never a candidate: set by a probe, the surprise is the step before's
    // valence alone (critic_reward x valence: no level, no mean), so a probe that hands in a
    // clean valence from the world's true state drives the actor with a clean surprise
    int oracle = 0;
    uint8_t judged[CRITIC_STEPS] = {}; // per kept step: 1 if the world gave it good or bad
};

// the critic cells: set if `neuron` is one (false before the first step)
inline bool critic_cell(const Blob& blob, int neuron) {
    return blob.critic &&
           ((blob.critic->cells[neuron / TILE_NEURONS] >> (neuron % TILE_NEURONS)) & 1);
}

// one step's end (from blob_reward). true if the critic took the reward: the caller then
// leaves the raw valence alone. false when critic_on or learning_on is 0.
bool critic_step(Blob& blob, int valence);
// the critic's state, made if missing (for probes: the oracle diagnostic)
CriticState& critic_state(Blob& blob);
// the critic cells' spikes over the last 15 ticks, for any blob (born wiring if the critic
// never learned); the mask is made if missing. for probes
int critic_level(Blob& blob);
void critic_free(Blob& blob);

// learn_actor.cpp: the actor's update for one surprise, and the random press for the next
// step
void actor_learn(Blob& blob, CriticState& c, int sign);
void actor_explore(Blob& blob, CriticState& c);
// shared with the critic: move one cell's evidence for the sources active in kept step
// `age` (0 newest), `sign` +1 more, -1 less, chance 2^-shift per spike. no rewire
void critic_move_cell(Blob& blob, CriticState& c, int n, int age, int sign, int shift);
// the kept step `age` steps before the newest (0 newest)
inline int critic_slot(const CriticState& c, int age) {
    return (c.head - age + CRITIC_STEPS * 4) % CRITIC_STEPS;
}
