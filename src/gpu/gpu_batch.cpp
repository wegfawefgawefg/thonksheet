#include "gpu/gpu_batch.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

// the compute pipelines (a life's ticks, a life's birth), the groups' buffers and
// descriptor sets, and one dispatch.

constexpr int BINDINGS = 9;
constexpr int MAX_GROUPS = 256;

static uint64_t mask_words(const GpuShape& s) {
    return (uint64_t)s.tiles * 9 * 64;
}
static uint64_t ring_words(const GpuShape& s) {
    return 2ull * GPU_RING * (uint64_t)s.stride;
}

uint64_t gpu_life_bytes(const GpuShape& s) {
    return mask_words(s) * 8 + (uint64_t)s.tiles * 64 + ring_words(s) * 8 +
           ((uint64_t)s.tiles + 1) * 8 + sizeof(GpuLife) + sizeof(GpuGenome) +
           (uint64_t)s.max_windows * 8;
}

int gpu_group_size(const GpuContext& g, const GpuShape& s) {
    // the masks are the largest buffer; keep each under the binding limit and 1 GB
    uint64_t limit = g.limits.maxStorageBufferRange;
    limit = limit < (1ull << 30) ? limit : (1ull << 30);
    uint64_t lives = limit / (mask_words(s) * 8);
    lives = lives > 65535 ? 65535 : lives;
    return lives < 1 ? 1 : (int)lives;
}

static bool read_file(const char* path, std::vector<uint32_t>& out) {
    FILE* f = std::fopen(path, "rb");
    if (!f) {
        std::fprintf(stderr, "blobgpu: cannot read %s\n", path);
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    long bytes = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize((size_t)bytes / 4);
    size_t got = std::fread(out.data(), 4, out.size(), f);
    std::fclose(f);
    return got == out.size();
}

// the descriptor set layout every pipeline shares, the pipeline layout and the pool
static bool make_layout(GpuContext& g, GpuBatch& b) {
    VkDescriptorSetLayoutBinding bindings[BINDINGS];
    for (int i = 0; i < BINDINGS; i++) {
        bindings[i] = VkDescriptorSetLayoutBinding{(uint32_t)i, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                                                   1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr};
    }
    VkDescriptorSetLayoutCreateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set_info.bindingCount = BINDINGS;
    set_info.pBindings = bindings;
    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, 4};
    VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &b.set_layout;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push;
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, BINDINGS * MAX_GROUPS};
    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.maxSets = MAX_GROUPS;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &size;
    return gpu_ok(g, vkCreateDescriptorSetLayout(g.device, &set_info, nullptr, &b.set_layout),
                  "set layout") &&
           gpu_ok(g, vkCreatePipelineLayout(g.device, &layout_info, nullptr, &b.layout),
                  "pipeline layout") &&
           gpu_ok(g, vkCreateDescriptorPool(g.device, &pool_info, nullptr, &b.pool),
                  "descriptor pool");
}

// one compute pipeline from a .spv. specialization: workgroup size, tiles, ring stride,
// tiles across (a shader with a fixed workgroup size ignores the first)
static bool make_pipeline(GpuContext& g, GpuBatch& b, const char* spv, int wave, VkPipeline& out) {
    std::vector<uint32_t> code;
    if (!read_file(spv, code)) {
        return false;
    }
    VkShaderModuleCreateInfo module_info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    module_info.codeSize = code.size() * 4;
    module_info.pCode = code.data();
    VkShaderModule module = VK_NULL_HANDLE;
    if (!gpu_ok(g, vkCreateShaderModule(g.device, &module_info, nullptr, &module), "shader")) {
        return false;
    }
    int32_t values[4] = {b.shape.workgroup, b.shape.tiles, b.shape.stride, b.shape.tiles_w};
    VkSpecializationMapEntry entries[4] = {{0, 0, 4}, {1, 4, 4}, {2, 8, 4}, {3, 12, 4}};
    VkSpecializationInfo spec{4, entries, sizeof(values), values};
    VkComputePipelineCreateInfo info{VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
    info.stage =
        VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
    info.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    info.stage.module = module;
    info.stage.pName = "main";
    info.stage.pSpecializationInfo = &spec;
    VkPipelineShaderStageRequiredSubgroupSizeCreateInfo size{
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO};
    size.requiredSubgroupSize = (uint32_t)wave;
    info.stage.pNext = wave > 0 ? &size : nullptr;
    info.layout = b.layout;
    bool ok = gpu_ok(g, vkCreateComputePipelines(g.device, VK_NULL_HANDLE, 1, &info, nullptr, &out),
                     "compute pipeline");
    vkDestroyShaderModule(g.device, module, nullptr);
    return ok;
}

bool gpu_batch_open(GpuContext& g, GpuBatch& b, const GpuShape& shape, const GpuParams& params,
                    const std::vector<uint64_t>& world_words, const char* life_spv,
                    const char* birth_spv) {
    b.shape = shape;
    b.params = params;
    if (!make_layout(g, b) || !make_pipeline(g, b, life_spv, shape.wave, b.pipeline) ||
        !make_pipeline(g, b, birth_spv, 0, b.birth)) {
        return false;
    }
    return gpu_buffer(g, b.world, world_words.size() * 8, false) &&
           gpu_upload(g, b.world, 0, world_words.data(), world_words.size() * 8);
}

// a new device buffer of zeros
static bool zeros_new(GpuContext& g, GpuBuffer& buf, uint64_t bytes) {
    if (!gpu_buffer(g, buf, bytes, false) || !gpu_begin(g)) {
        return false;
    }
    vkCmdFillBuffer(g.command, buf.buffer, 0, VK_WHOLE_SIZE, 0);
    return gpu_submit(g);
}

// one dispatch of a pipeline over a group
static bool dispatch(GpuContext& g, GpuBatch& b, GpuGroup& group, VkPipeline pipeline,
                     uint32_t workgroups, int32_t push) {
    if (!gpu_begin(g)) {
        return false;
    }
    vkCmdBindPipeline(g.command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(g.command, VK_PIPELINE_BIND_POINT_COMPUTE, b.layout, 0, 1, &group.set,
                            0, nullptr);
    vkCmdPushConstants(g.command, b.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, 4, &push);
    vkCmdDispatch(g.command, workgroups, 1, 1);
    return gpu_submit(g);
}

// a new device buffer holding these bytes
static bool upload_new(GpuContext& g, GpuBuffer& buf, const void* data, uint64_t bytes) {
    return gpu_buffer(g, buf, bytes, false) && gpu_upload(g, buf, 0, data, bytes);
}

bool gpu_group_add(GpuContext& g, GpuBatch& b, int first, const GpuLifeData* lives, int count) {
    if ((int)b.groups.size() >= MAX_GROUPS) {
        std::fprintf(stderr, "blobgpu: more than %d groups\n", MAX_GROUPS);
        return false;
    }
    b.groups.push_back(GpuGroup{});
    GpuGroup& gr = b.groups.back();
    gr.first = first;
    gr.count = count;
    const GpuShape& s = b.shape;
    GpuParams p = b.params;
    p.lives = count;
    size_t mw = (size_t)mask_words(s), rw = (size_t)ring_words(s), tiles1 = (size_t)s.tiles + 1;
    size_t nt = (size_t)s.tiles * 64;
    size_t windows = s.max_windows > 0 ? (size_t)s.max_windows : 1;
    size_t n = (size_t)count;

    // every life's part of each buffer, side by side
    std::vector<GpuGenome> genomes(n);
    std::vector<GpuLife> states(n);
    for (size_t i = 0; i < n; i++) {
        genomes[i] = lives[i].genome;
        states[i] = lives[i].life;
    }
    bool ok = upload_new(g, gr.params, &p, sizeof(p)) &&
              upload_new(g, gr.genomes, genomes.data(), sizeof(GpuGenome) * n) &&
              upload_new(g, gr.lives, states.data(), sizeof(GpuLife) * n) &&
              zeros_new(g, gr.rings, rw * n * 8) && zeros_new(g, gr.scores, windows * 8 * n);
    const bool born_here = lives[0].mask.empty();
    if (born_here) {
        // born on the GPU below, from zeros
        ok = ok && zeros_new(g, gr.masks, mw * n * 8) && zeros_new(g, gr.thresholds, nt * n) &&
             zeros_new(g, gr.inhibitory, tiles1 * n * 8);
    } else {
        // born on the CPU: copied in as they are
        std::vector<uint64_t> masks(mw * n), inhibitory(tiles1 * n);
        std::vector<uint8_t> thresholds(nt * n);
        for (size_t i = 0; i < n; i++) {
            std::memcpy(&masks[mw * i], lives[i].mask.data(), mw * 8);
            std::memcpy(&inhibitory[tiles1 * i], lives[i].inhibitory.data(), tiles1 * 8);
            std::memcpy(&thresholds[nt * i], lives[i].threshold.data(), nt);
        }
        ok = ok && upload_new(g, gr.masks, masks.data(), masks.size() * 8) &&
             upload_new(g, gr.thresholds, thresholds.data(), thresholds.size()) &&
             upload_new(g, gr.inhibitory, inhibitory.data(), inhibitory.size() * 8);
    }
    if (!ok) {
        return false;
    }
    VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc.descriptorPool = b.pool;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &b.set_layout;
    if (!gpu_ok(g, vkAllocateDescriptorSets(g.device, &alloc, &gr.set), "descriptor set")) {
        return false;
    }
    GpuBuffer* order[BINDINGS] = {&gr.params,     &b.world,  &gr.genomes,    &gr.lives, &gr.masks,
                                  &gr.thresholds, &gr.rings, &gr.inhibitory, &gr.scores};
    VkDescriptorBufferInfo infos[BINDINGS];
    VkWriteDescriptorSet writes[BINDINGS];
    for (int i = 0; i < BINDINGS; i++) {
        infos[i] = VkDescriptorBufferInfo{order[i]->buffer, 0, VK_WHOLE_SIZE};
        writes[i] = VkWriteDescriptorSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        writes[i].dstSet = gr.set;
        writes[i].dstBinding = (uint32_t)i;
        writes[i].descriptorCount = 1;
        writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        writes[i].pBufferInfo = &infos[i];
    }
    vkUpdateDescriptorSets(g.device, BINDINGS, writes, 0, nullptr);
    // the birth shader: one thread per life, 64 lives a workgroup
    return !born_here || dispatch(g, b, gr, b.birth, (uint32_t)(count + 63) / 64, 0);
}

bool gpu_group_step(GpuContext& g, GpuBatch& b, GpuGroup& group, int ticks) {
    return dispatch(g, b, group, b.pipeline, (uint32_t)group.count, ticks);
}

bool gpu_group_lives(GpuContext& g, GpuGroup& group, GpuLife* out) {
    return gpu_download(g, group.lives, 0, out, sizeof(GpuLife) * (uint64_t)group.count);
}

bool gpu_group_read(GpuContext& g, const GpuBatch& b, GpuGroup& group, int index,
                    GpuLifeData& out) {
    const GpuShape& s = b.shape;
    uint64_t mw = mask_words(s), rw = ring_words(s), tiles1 = (uint64_t)s.tiles + 1;
    uint64_t i = (uint64_t)index;
    out.mask.resize(mw);
    out.threshold.resize((size_t)s.tiles * 64);
    out.ring.resize(rw);
    out.inhibitory.resize(tiles1);
    return gpu_download(g, group.lives, i * sizeof(GpuLife), &out.life, sizeof(GpuLife)) &&
           gpu_download(g, group.masks, i * mw * 8, out.mask.data(), mw * 8) &&
           gpu_download(g, group.thresholds, i * s.tiles * 64, out.threshold.data(),
                        (uint64_t)s.tiles * 64) &&
           gpu_download(g, group.rings, i * rw * 8, out.ring.data(), rw * 8) &&
           gpu_download(g, group.inhibitory, i * tiles1 * 8, out.inhibitory.data(), tiles1 * 8);
}

bool gpu_group_scores(GpuContext& g, const GpuBatch& b, GpuGroup& group, uint32_t* out) {
    uint64_t windows = b.shape.max_windows > 0 ? (uint64_t)b.shape.max_windows : 1;
    return gpu_download(g, group.scores, 0, out, windows * 8 * (uint64_t)group.count);
}

void gpu_batch_close(GpuContext& g, GpuBatch& b) {
    if (g.device == VK_NULL_HANDLE) {
        return;
    }
    if (!g.lost) {
        vkDeviceWaitIdle(g.device);
    }
    for (GpuGroup& gr : b.groups) {
        GpuBuffer* all[] = {&gr.params,     &gr.genomes, &gr.lives,      &gr.masks,
                            &gr.thresholds, &gr.rings,   &gr.inhibitory, &gr.scores};
        for (GpuBuffer* buf : all) {
            gpu_free(g, *buf);
        }
    }
    gpu_free(g, b.world);
    vkDestroyPipeline(g.device, b.pipeline, nullptr);
    vkDestroyPipeline(g.device, b.birth, nullptr);
    vkDestroyDescriptorPool(g.device, b.pool, nullptr);
    vkDestroyPipelineLayout(g.device, b.layout, nullptr);
    vkDestroyDescriptorSetLayout(g.device, b.set_layout, nullptr);
    b = GpuBatch{};
}
