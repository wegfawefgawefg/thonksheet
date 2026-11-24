#pragma once

#include "blob/genome.hpp"
#include "world/plug.hpp"
#include "world/task.hpp"

#include <cstdint>
#include <string>

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
    std::string engine = "blob"; // blob (the full engine) or minimal
};

// false after printing a message (or the usage) to stderr
bool run_flags_parse(int argc, char** argv, RunFlags& flags);
