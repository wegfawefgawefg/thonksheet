#pragma once

#include "blob/blob.hpp"
#include "gpu/gpu_batch.hpp"
#include "gpu/gpu_lives.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cstdint>
#include <string>
#include <vector>

// The same lives on the CPU engine, stepped exactly as blobrun's life loop steps them
// (src/run/life.cpp): the reference the GPU is checked against, and the CPU side of the speed
// table (--engine cpu).

struct CpuLife {
    Blob* blob = nullptr;
    TaskTicker* ticker = nullptr;
    RunFlags flags;
    int64_t step = 0;
    int64_t ticks = 0;
    bool done = false;
    std::vector<double> scores; // one per window
};

bool cpu_life_start(CpuLife& c, const GpuLifeSpec& spec, const RunFlags& flags);
// up to `ticks` ticks, fewer if the life ends. returns the ticks run.
int64_t cpu_life_ticks(CpuLife& c, int64_t ticks);
void cpu_life_free(CpuLife& c);

// the CPU life against the GPU's copy of it: spike and noise maps of the last `recent`
// ticks, then the whole ring, masks, thresholds, reward average and task. false with the
// first difference in `why`.
bool cpu_life_matches(const CpuLife& c, const GpuLifeData& gpu, int recent, std::string& why);
