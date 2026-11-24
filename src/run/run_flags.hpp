#pragma once

#include "blob/genome.hpp"
#include "world/plug.hpp"
#include "world/task.hpp"

#include <cstdint>
#include <string>

// what one blobrun call does with its blob (docs/bench.md, the suite)
enum class Protocol {
    life,      // one task for the whole life (the default)
    retention, // task A, then task B, then A again for a short retest
    idle,      // task A, then everything unplugged for a while, then A again
    capacity,  // echo with 2, 4, 8 and 16 cues, each a separate life
    reach,     // how far a cue can be read across the sheet, no reward
    speed,     // ticks per second at three sizes, and bytes per neuron
};

// blobrun's command line, parsed into plain settings
struct RunFlags {
    TaskKind task = TaskKind::echo;
    TaskSettings task_settings;
    PlugSettings plug;
    Genome genome;
    uint64_t seed = 1;      // the blob's seed
    uint64_t task_seed = 0; // the task's own seed; 0 means "same as seed"
    int64_t steps = 20000;  // life length in task steps
    int window = 200;       // task steps per score window
    int edge = 0;           // windows averaged at each end; 0 means a tenth of them
    bool baselines = true;  // also run learning-off and coin flips
    bool show_plug = false; // print where the lines sit, then run
    std::string out;        // csv path; empty means out/<task>-s<seed>.csv

    // life protocols (see Protocol)
    Protocol protocol = Protocol::life;
    TaskKind task_b = TaskKind::echo; // retention: the second task
    int64_t retest = 0;               // retention, idle: steps of the retest; 0 = steps / 4
    int64_t idle_ticks = 0;           // idle: ticks with everything unplugged
    bool frozen_readout = false;      // only connections into motor neurons learn
    int alive_ticks = 16384;          // end of life: ticks counted for rate, silent, stuck
    int rhythm_ticks = 16384;         // end of life: idle ticks recorded for the rhythm
    int reach_settle = 30000;         // reach: ticks before measuring
    int reach_blocks = 4000;          // reach: blocks of 20 ticks measured
    int speed_ms = 2000;              // speed: time spent at each size
};

// false after printing a message (or the usage) to stderr
bool run_flags_parse(int argc, char** argv, RunFlags& flags);
