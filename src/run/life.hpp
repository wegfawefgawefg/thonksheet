#pragma once

#include "run/drive.hpp"
#include "run/life_end.hpp"
#include "run/run_flags.hpp"
#include "world/task.hpp"

#include <cstdint>
#include <vector>

// One whole life as a few phases in order: a task for some steps, or everything unplugged
// for some ticks. The blob is born once and never reset between phases. A plain blobrun
// life is one phase; the retention and idle tests are three.

constexpr int LIFE_MAX_PHASES = 4;

struct LifePhase {
    TaskKind task = TaskKind::echo;
    TaskSettings settings;
    uint64_t task_seed = 1;
    int64_t steps = 0;      // task steps; 0 makes this an unplugged phase
    int64_t idle_ticks = 0; // ticks with every line off and unattached
    // being shown the answer and damage (life_shown_damage.cpp)
    int shown_ticks = 0;   // the right button's cells made to fire for the last N ticks of a step
    bool withhold = false; // no good or bad reaches the blob
    int damage_pct = 0;    // at the phase's start, silence this share (in 100) of non-sensor cells
    int damage_tile = 0;   // at the phase's start, silence the non-sensor cells of the cue tile
};

struct LifeResult {
    std::vector<double> scores; // one per window, all phases in order
    // phase p owns scores [first_window[p], first_window[p + 1])
    int first_window[LIFE_MAX_PHASES + 1] = {};
    int64_t ticks = 0; // ticks lived over the phases (not the end measurements)
    double seconds = 0;
    LifeEnd end; // room and aliveness, for real blobs only
};

// --task for --steps, as blobrun has always run
LifePhase life_phase_main(const RunFlags& flags);

// the silencing of a damage phase, and the forced firing of a shown one (life_shown_damage.cpp)
void life_damage(Blob& blob, const Plug& plug, const LifePhase& phase, uint64_t seed);
void life_force(Blob& blob, const Plug& plug, int button);
void life_hooks_free(Blob& blob);

// false (with a message) if a task's lines do not fit the blob
bool life_run(Driver driver, const RunFlags& flags, const LifePhase* phases, int count,
              LifeResult& out);
