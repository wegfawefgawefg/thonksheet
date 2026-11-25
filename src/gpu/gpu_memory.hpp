#pragma once

#include "blob/blob.hpp"
#include "gpu/gpu_batch.hpp"
#include "gpu/gpu_flags.hpp"
#include "gpu/gpu_host.hpp"
#include "memory/memory.hpp"

#include <cstdint>
#include <string>
#include <vector>

// The memory engine (src/memory/, idea 123) on the GPU, with host worlds: one workgroup is one
// life, its threads share the tables of a frame and of a reward; the rest of a tick (events,
// the code, the choice, the next-input store, the random draws) runs in the CPU engine's
// order on one thread, so a life is the CPU's bit for bit. shaders/gpu/memory_life.comp.
// Births stay on the CPU (memory_birth) and are copied up.
//
// A batch uses the minimal engine's groups and bindings with other contents (the shapes are
// fixed by binding number in shaders/gpu/memory_layout.glsl):
//   genomes GpuMemGenome, lives GpuMemLife, masks the counters (int8), thresholds the table
//   bits (uint), rings the code rows, inhibitory the level codes, scores the frames' slots
//   (uint), ev_planes the frames, ev_lines the next-input tags, delays the next-input words,
//   far_tiles each motor cell's button (int), ev_meta (per batch) the input tiles and the
//   motor cells.

struct GpuMemGenome {
    int32_t learning_on, code, tables, table_bits;
    int32_t counter_max, good_step, bad_step, others_up;
    int32_t credit_ticks, trace_ticks, trace_across, across_ticks;
    int32_t explore_max, explore_min, avg_shift, predict;
    int32_t predict_bits, pad0, pad1, pad2;
};

struct GpuMemFrame {
    int64_t first_tick, last_tick;
    int32_t unit, pad;
};

// one life's small state: MemoryBlob's, the shell's predicted words, and the ticker's
struct GpuMemLife {
    int64_t tick, now_start, frame_count, last_reward_tick;
    uint64_t predict_key;
    uint64_t rng[4];
    uint64_t code_now[MEM_FIELD_WORDS];
    uint64_t past_code[MEM_DEPTH_MAX * MEM_FIELD_WORDS];
    uint64_t past_in_hash[MEM_DEPTH_MAX];
    uint64_t in_now[MEM_IN_WORDS];
    uint64_t predicted[MEM_IN_WORDS]; // the shell's predicted map at each input tile
    int32_t past_level[MEM_DEPTH_MAX];
    int32_t level_now, drive, units, explore_unit;
    int32_t new_cell, frame_due, good_avg, predict_have;
    int32_t tie_key[MEM_UNITS + 1];
    // the ticker, as GpuLife's
    int32_t tick_in_step, rest_left, good_left, bad_left;
    int32_t block, motor_lines;
    uint32_t attached;
    int32_t surprise; // after the step's first tick
    int32_t motor_count[GPU_MAX_MOTORS];
    int32_t pad;
};

static_assert(sizeof(GpuMemGenome) == 80, "mirrors memory_layout.glsl");
static_assert(sizeof(GpuMemFrame) == 24, "mirrors memory_layout.glsl");
static_assert(sizeof(GpuMemLife) == 1984, "mirrors memory_layout.glsl");

// a memory genome the shader runs as written; false with the reason
bool gpu_memory_supported(const MemoryGenome& yg, std::string& why);
// the batch from the first life's shell: shape, worlds' tables, the pipeline. sizes are the
// largest any life asks for.
bool gpu_memory_open(GpuContext& g, GpuBatch& b, const GpuFlags& f, std::vector<HostWorld>& worlds,
                     const std::vector<GpuLifeSpec>& lives, const char* spv);
// births on the CPU (up to --birth-threads at once), copied up a group at a time
bool gpu_memory_upload(GpuContext& g, GpuBatch& b, const GpuFlags& f,
                       const std::vector<HostWorld>& worlds, const std::vector<GpuLifeSpec>& lives);
// bytes one life takes on the device
uint64_t gpu_memory_life_bytes(const GpuShape& s);
// life-ticks lived, summed over the batch
int64_t gpu_memory_ticks(GpuContext& g, GpuBatch& b);
// one life's whole state against the CPU engine's (after the same steps); false with the
// first difference in `why`
bool gpu_memory_matches(GpuContext& g, GpuBatch& b, GpuGroup& group, int index, const Blob& cpu,
                        std::string& why);
