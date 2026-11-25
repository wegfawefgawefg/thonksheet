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
};

// what one life holds on the device, host side, ready to upload
struct GpuLifeData {
    GpuGenome genome;
    GpuLife life;
    std::vector<uint64_t> mask;       // tiles * 9 * 64
    std::vector<uint8_t> threshold;   // tiles * 64
    std::vector<uint64_t> ring;       // 2 * 32 * stride: spikes then noise
    std::vector<uint64_t> inhibitory; // tiles + 1
};

struct GpuGroup {
    int first = 0, count = 0; // lives [first, first + count) of the batch
    GpuBuffer params, genomes, lives, masks, thresholds, rings, inhibitory, scores;
    VkDescriptorSet set = VK_NULL_HANDLE;
    bool done = false;
};

struct GpuBatch {
    GpuShape shape;
    GpuParams params; // lives is set per group
    GpuBuffer world;
    std::vector<GpuGroup> groups;
    VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
    VkPipelineLayout layout = VK_NULL_HANDLE;
    VkDescriptorPool pool = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
};

// lives per group for this device and shape
int gpu_group_size(const GpuContext& g, const GpuShape& shape);
// bytes one life takes on the device
uint64_t gpu_life_bytes(const GpuShape& shape);

// pipeline and the shared world table. spv: path to minimal_life.comp.spv
bool gpu_batch_open(GpuContext& g, GpuBatch& b, const GpuShape& shape, const GpuParams& params,
                    const std::vector<uint64_t>& world_words, const char* spv);
// a group's buffers, filled from its lives (lives[0] is the group's first)
bool gpu_group_add(GpuContext& g, GpuBatch& b, int first, const GpuLifeData* lives, int count);
// step one group by up to `ticks` ticks (one submit, waited for)
bool gpu_group_step(GpuContext& g, GpuBatch& b, GpuGroup& group, int ticks);
// read back a group's life states
bool gpu_group_lives(GpuContext& g, GpuGroup& group, GpuLife* out);
// read back one life of a group, whole
bool gpu_group_read(GpuContext& g, const GpuBatch& b, GpuGroup& group, int index, GpuLifeData& out);
bool gpu_group_scores(GpuContext& g, const GpuBatch& b, GpuGroup& group, uint32_t* out);
void gpu_batch_close(GpuContext& g, GpuBatch& b);
