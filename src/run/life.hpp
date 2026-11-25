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
};

struct LifeResult {
    std::vector<double> scores; // one per window, all phases in order
    // phase p owns scores [first_window[p], first_window[p + 1])
    int first_window[LIFE_MAX_PHASES + 1] = {};
    int64_t ticks = 0; // ticks lived over the phases (not the end measurements)
    double seconds = 0;
    LifeEnd end; // room and aliveness, for real blobs only
    // extra scores per window, when the task has them (task_extra_name names them; the
    // task is the last phase's, valid until the next life)
    int extras = 0;
    std::vector<double> extra[TASK_MAX_EXTRA];
    const Task* extra_task = nullptr;
};

// --task for --steps, as blobrun has always run
LifePhase life_phase_main(const RunFlags& flags);

// false (with a message) if a task's lines do not fit the blob
bool life_run(Driver driver, const RunFlags& flags, const LifePhase* phases, int count,
              LifeResult& out);
