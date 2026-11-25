#pragma once

#include "gpu/gpu_layout.hpp"
#include "gpu/gpu_vulkan.hpp"

#include <cstdint>
#include <vector>

// A batch of lives on the device, in groups. A group is as many lives as fit one set of
// buffers (a storage buffer binding is at most 4 GB on the GPU, 128 MB on lavapipe); one
// dispatch steps one group. The per-batch table (sensor, line and button masks) is shared
// by every group.

struct GpuShape {
    int tiles = 64;      // tiles in a sheet
    int tiles_w = 8;     // tiles across
    int stride = 72;     // ring map stride in words (the CPU engine's)
    int max_windows = 0; // score windows per life
    int workgroup = 256; // threads per life
    int wave = 0;        // subgroup size asked of the driver (0: its own choice)
    int host = 0;        // 1: host worlds, one task step a dispatch (gpu_host.hpp)
    // the grown-back pieces, batch-wide (each life uses what its genome says inside them)
    int slots = 9;   // slots per neuron in the layout: 9 local and the most far slots
    int delay = 0;   // 1: slot delays
    int ev = 0;      // 1: evidence counters
    int ev_bits = 1; // planes per slot: the widest counters of the batch
    int ev_rows = 1; // rows of counters per life
};

// what one life holds on the device, host side, ready to upload
struct GpuLifeData {
    GpuGenome genome;
    GpuLife life;
    std::vector<uint64_t> mask;       // tiles * slots * 64; empty: born on the GPU
    std::vector<uint8_t> threshold;   // tiles * 64
    std::vector<uint64_t> ring;       // read back only (a newborn ring is zeros)
    std::vector<uint64_t> inhibitory; // tiles + 1
    // read back only: evidence planes and lines, delays, far tiles, in the batch's layout
    std::vector<uint64_t> ev_planes;
    std::vector<uint8_t> ev_lines, delays;
    std::vector<uint32_t> far_tiles;
};

struct GpuGroup {
    int first = 0, count = 0; // lives [first, first + count) of the batch
    GpuBuffer params, genomes, lives, masks, thresholds, rings, inhibitory, scores;
    // host worlds: the exchange, host-visible and mapped (tiny placeholders otherwise)
    GpuBuffer xin, xwords, xout;
    // the grown-back pieces (tiny placeholders when the batch has none)
    GpuBuffer ev_planes, ev_lines, delays, far_tiles;
    VkDescriptorSet set = VK_NULL_HANDLE;
    bool done = false;
};

struct GpuBatch {
    GpuShape shape;
    GpuParams params; // lives is set per group
    GpuBuffer world;
    // evidence: per batch, the rows' neurons per tile, the row before each tile, the motor
    // cells (tiles + 1 words each); and on the host, each neuron's row (-1 none)
    GpuBuffer ev_meta;
    std::vector<int32_t> ev_row;
    std::vector<GpuGroup> groups;
    VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE; // minimal_life.comp: ticks
    VkPipeline birth = VK_NULL_HANDLE;    // minimal_birth.comp
};

// lives per group for this device and shape
int gpu_group_size(const GpuContext& g, const GpuShape& shape);
// bytes one life takes on the device
uint64_t gpu_life_bytes(const GpuShape& shape);

// pipelines and the shared world table, from minimal_life.comp.spv and minimal_birth.comp.spv.
// sensor and motor: the sheet's cell words (tiles + 1 each), for the evidence rows
bool gpu_batch_open(GpuContext& g, GpuBatch& b, const GpuShape& shape, const GpuParams& params,
                    const std::vector<uint64_t>& world_words, const char* life_spv,
                    const char* birth_spv, const uint64_t* sensor = nullptr,
                    const uint64_t* motor = nullptr, int ev_on_max = 0);
// rows of evidence counters a batch needs: every non-sensor neuron (ev_on 2) or the motor cells
int gpu_ev_rows(const uint64_t* sensor, const uint64_t* motor, int tiles, int ev_on_max);
// a group's buffers, filled from its lives (lives[0] is the group's first). lives whose masks
// are empty are born on the GPU (all of a group or none).
bool gpu_group_add(GpuContext& g, GpuBatch& b, int first, const GpuLifeData* lives, int count);
// step one group by up to `ticks` ticks (one submit, waited for)
bool gpu_group_step(GpuContext& g, GpuBatch& b, GpuGroup& group, int ticks);
// the same for `count` of its lives from `first` (host worlds split a group's step)
bool gpu_group_step_part(GpuContext& g, GpuBatch& b, GpuGroup& group, int ticks, int first,
                         int count);
// the same, submitted on a slot of its own and not waited for (gpu_slot_wait)
bool gpu_group_step_async(GpuContext& g, GpuBatch& b, GpuGroup& group, int ticks, int first,
                          int count, GpuSlot& slot);
// read back a group's life states
bool gpu_group_lives(GpuContext& g, GpuGroup& group, GpuLife* out);
// read back one life of a group, whole
bool gpu_group_read(GpuContext& g, const GpuBatch& b, GpuGroup& group, int index, GpuLifeData& out);
bool gpu_group_scores(GpuContext& g, const GpuBatch& b, GpuGroup& group, uint32_t* out);
void gpu_batch_close(GpuContext& g, GpuBatch& b);
