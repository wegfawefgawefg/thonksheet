#pragma once

#include "gpu/gpu_cpu.hpp"
#include "gpu/gpu_flags.hpp"
#include "gpu/gpu_host.hpp"

#include <vector>

// a blobgpu run with host worlds (gpu_host.hpp), the pieces main.cpp calls in order

struct HostRunStats {
    HostTimes times;
    int64_t ticks = 0;  // life-ticks, summed
    double seconds = 0; // the loop of exchanges, wall clock
    double longest = 0; // the longest dispatch
    int dispatches = 0;
};

// the worlds' tables from the first life's sheet, the pipelines (HOST 1)
bool host_run_open(GpuContext& g, GpuBatch& b, const GpuFlags& f, std::vector<HostWorld>& worlds,
                   const std::vector<GpuLifeSpec>& lives, const char* life_spv,
                   const char* birth_spv);
// every life to its last step, one exchange per step. cpu: check every step against these
bool host_run(GpuContext& g, GpuBatch& b, const GpuFlags& f, const std::vector<HostWorld>& worlds,
              const std::vector<GpuLifeSpec>& lives, std::vector<std::vector<double>>& scores,
              std::vector<CpuLife>* cpu, HostRunStats& out);
