#pragma once

#include "blob/blob.hpp"
#include "gpu/gpu_batch.hpp"
#include "minimal/minimal.hpp"
#include "run/run_flags.hpp"

#include <cstdint>
#include <string>
#include <vector>

// From CPU births to what the shader reads. Birth stays on the CPU (it draws from the life's
// own stream in a fixed order); the born state is copied out as it is. The world tables come
// from the real plug laid on a born sheet, and each life's task from the real task_reset.

// one life of a batch: its seed and its genomes
struct GpuLifeSpec {
    uint64_t seed = 1;
    Genome genome;
    MinimalGenome mg;
};

// the batch's world: task and plug numbers, sensor, line and button masks
struct GpuWorld {
    GpuParams params{};
    std::vector<uint64_t> words; // sensor, line masks, motor masks (gpu_layout.hpp)
    GpuShape shape;
};

// a minimal genome the shader runs as written; false with the reason if it is not
bool gpu_genome_supported(const MinimalGenome& mg, std::string& why);
// the world from a born life (the plug is attached to it and detached again)
bool gpu_world_build(GpuWorld& w, Blob& blob, const RunFlags& flags, std::string& why);
// a born life, copied out for upload (its task started from task_seed)
void gpu_life_pack(GpuLifeData& out, const MinimalBlob& m, const RunFlags& flags,
                   uint64_t task_seed);
