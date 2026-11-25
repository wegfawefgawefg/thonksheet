#pragma once

#include "gpu/gpu_lives.hpp"
#include "run/run_flags.hpp"

#include <string>
#include <vector>

// blobgpu's command line, and the settings file that makes a batch a population: one row per
// life, a column per number. Every blobrun flag that shapes a life (--task, --k, --steps,
// --ticks, --window, --sensor-side, --g.name, --m.name ...) is read by blobrun's own parser.

enum class GpuDevice { gpu, lavapipe, cpu };

struct GpuFlags {
    int lives = 1;
    uint64_t seed_from = 1; // life i has seed seed_from + i
    int tiles = 0;          // 0: the genome's own sheet
    bool twin = false;      // also run each life with learning off
    bool check = false;     // step the CPU engine alongside and compare every tick
    bool validate = false;  // Vulkan validation layers
    GpuDevice device = GpuDevice::gpu;
    int workgroup = 256;               // threads per life
    int group = 0;                     // lives per dispatch at most (0: as many as fit)
    bool birth_gpu = true;             // births on the GPU (false: on the CPU, copied up)
    int birth_threads = 2;             // CPU threads for births
    int wave = 0;                      // subgroup size to ask for, 0 the driver's
    int dispatch_ms = 50;              // aim for submits about this long
    int max_ticks = 4096;              // never more ticks than this in one dispatch
    int check_ticks = 16;              // ticks per dispatch while checking
    std::string out;                   // one row per life
    std::string windows;               // one row per life per window
    std::string settings;              // csv of settings in
    std::string shader;                // the .spv to run; empty is the built one
    bool host = false;                 // host worlds even for an Echo-family task (gpu_host.hpp)
    std::string tasks;                 // host worlds: a list of worlds, one batch of lives each
    int host_threads = 4;              // CPU threads stepping the host worlds (at most 4)
    RunFlags run;                      // the task and plug, as blobrun reads them
    std::vector<std::string> run_args; // the blobrun flags as given, for --tasks
};

// the settings file's columns and rows, kept to be echoed into the output
struct GpuSettings {
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;
};

bool gpu_flags_parse(int argc, char** argv, GpuFlags& f, Genome& genome, MinimalGenome& mg);
// the lives of the batch: from the settings file if given, else seeds seed_from.. with the
// command line's genomes. twins come after all the learning lives. with several worlds the
// whole list repeats per world: life (world * (twin ? 2 : 1) + is_twin) * rows + row.
bool gpu_lives_list(const GpuFlags& f, const Genome& genome, const MinimalGenome& mg, int worlds,
                    GpuSettings& settings, std::vector<GpuLifeSpec>& lives);
