#pragma once

#include "gpu/gpu_batch.hpp"
#include "gpu/gpu_cpu.hpp"
#include "gpu/gpu_flags.hpp"
#include "gpu/gpu_host.hpp"

#include <vector>

// the pieces of a blobgpu run that main.cpp calls in order

// every life's genome runs on the GPU as written, and every life shares the first one's sheet
bool gpu_run_supported(const std::vector<GpuLifeSpec>& lives);
// the world from the first life, the pipeline and the shared tables
bool gpu_run_open(GpuContext& g, GpuBatch& b, const GpuFlags& f,
                  const std::vector<GpuLifeSpec>& lives, const char* life_spv,
                  const char* birth_spv);
// born on the GPU (or the CPU, --birth cpu) a group at a time. check: the CPU lives are kept.
// worlds: host worlds (each life's world table and buttons), or null
bool gpu_run_upload(GpuContext& g, GpuBatch& b, const GpuFlags& f,
                    const std::vector<GpuLifeSpec>& lives, std::vector<CpuLife>* check,
                    const std::vector<HostWorld>* worlds = nullptr);
// every life's window scores, read back
bool gpu_run_scores(GpuContext& g, GpuBatch& b, std::vector<std::vector<double>>& scores);
// the check after a dispatch: each CPU life of the group runs the same ticks, then the whole
// state is compared. false (with a message) on the first difference.
bool gpu_run_check(GpuContext& g, GpuBatch& b, GpuGroup& group, std::vector<CpuLife>& cpu,
                   int ticks, bool host = false);
// the CPU engine, one life after another on this thread. ticks lived and seconds spent
// ticking (birth not counted) come back.
void cpu_run(const GpuFlags& f, const std::vector<GpuLifeSpec>& lives,
             const std::vector<HostWorld>& worlds, std::vector<std::vector<double>>& scores,
             int64_t& ticks, double& seconds);
