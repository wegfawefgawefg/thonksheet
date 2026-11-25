#include "gpu/gpu_batch.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

// the compute pipelines (a life's ticks, a life's birth), the groups' buffers and
// descriptor sets, and one dispatch.

constexpr int BINDINGS = 17;
constexpr int MAX_GROUPS = 256;

static uint64_t mask_words(const GpuShape& s) {
    return (uint64_t)s.tiles * s.slots * 64;
}
static uint64_t ev_words(const GpuShape& s) {
    return s.ev ? (uint64_t)s.ev_rows * s.slots * s.ev_bits : 0;
}
static uint64_t far_words(const GpuShape& s) {
    return (uint64_t)s.tiles * (s.slots - 9) * 64;
}
static uint64_t ring_words(const GpuShape& s) {
    return 2ull * GPU_RING * (uint64_t)s.stride;
}

uint64_t gpu_life_bytes(const GpuShape& s) {
    return mask_words(s) * 8 + (uint64_t)s.tiles * 64 + ring_words(s) * 8 +
           ((uint64_t)s.tiles + 1) * 8 + sizeof(GpuLife) + sizeof(GpuGenome) +
           (uint64_t)s.max_windows * 8 + ev_words(s) * 8 + (s.ev ? (uint64_t)s.ev_rows : 0) +
           (s.delay ? mask_words(s) : 0) + far_words(s) * 4;
}

int gpu_group_size(const GpuContext& g, const GpuShape& s) {
    // the masks are the largest buffer; keep each under the binding limit and 1 GB
    uint64_t limit = g.limits.maxStorageBufferRange;
    limit = limit < (1ull << 30) ? limit : (1ull << 30);
    uint64_t largest = mask_words(s) * 8 > ev_words(s) * 8 ? mask_words(s) * 8 : ev_words(s) * 8;
    uint64_t lives = limit / largest;
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
    VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT, 0, 8};
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
// tiles across, host worlds (a shader with a fixed workgroup size ignores the first, and one
// without host worlds the last)
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
    const GpuShape& s = b.shape;
    int32_t values[16] = {s.workgroup, s.tiles,    s.stride,  s.tiles_w, s.host,     s.slots,
                          s.delay,     s.ev,       s.ev_bits, s.ev_rows, s.in_words, s.motor_cells,
                          s.tables,    s.counters, s.entries, s.pos};
    VkSpecializationMapEntry entries[16];
    for (uint32_t i = 0; i < 16; i++) {
        entries[i] = VkSpecializationMapEntry{i, i * 4, 4};
    }
    VkSpecializationInfo spec{16, entries, sizeof(values), values};
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

int gpu_ev_rows(const uint64_t* sensor, const uint64_t* motor, int tiles, int ev_on_max) {
    int rows = 0;
    for (int t = 0; t < tiles; t++) {
        uint64_t kind = ev_on_max == 2 ? ~0ull : motor[t];
        rows += __builtin_popcountll(kind & ~sensor[t]);
    }
    return rows;
}

bool gpu_batch_open(GpuContext& g, GpuBatch& b, const GpuShape& shape, const GpuParams& params,
                    const std::vector<uint64_t>& world_words, const char* life_spv,
                    const char* birth_spv, const uint64_t* sensor, const uint64_t* motor,
                    int ev_on_max) {
    b.shape = shape;
    b.params = params;
    if (!make_layout(g, b) || !make_pipeline(g, b, life_spv, shape.wave, b.pipeline) ||
        (birth_spv && !make_pipeline(g, b, birth_spv, 0, b.birth))) {
        return false;
    }
    // evidence rows: which neurons, the row before each tile, the motor cells
    const size_t tiles1 = (size_t)shape.tiles + 1;
    std::vector<uint64_t> meta(3 * tiles1, 0);
    if (shape.ev) {
        b.ev_row.assign((size_t)shape.tiles * 64, -1);
        int row = 0;
        for (int t = 0; t < shape.tiles; t++) {
            uint64_t kind = (ev_on_max == 2 ? ~0ull : motor[t]) & ~sensor[t];
            meta[(size_t)t] = kind;
            meta[tiles1 + (size_t)t] = (uint64_t)row;
            meta[2 * tiles1 + (size_t)t] = motor[t];
            for (int bit = 0; bit < 64; bit++) {
                b.ev_row[(size_t)t * 64 + (size_t)bit] = ((kind >> bit) & 1) ? row++ : -1;
            }
        }
    }
    return gpu_buffer(g, b.world, world_words.size() * 8, false) &&
           gpu_upload(g, b.world, 0, world_words.data(), world_words.size() * 8) &&
           gpu_buffer(g, b.ev_meta, meta.size() * 8, false) &&
           gpu_upload(g, b.ev_meta, 0, meta.data(), meta.size() * 8);
}

// a new device buffer of zeros
static bool zeros_new(GpuContext& g, GpuBuffer& buf, uint64_t bytes) {
    if (!gpu_buffer(g, buf, bytes, false) || !gpu_begin(g)) {
        return false;
    }
    vkCmdFillBuffer(g.command, buf.buffer, 0, VK_WHOLE_SIZE, 0);
    return gpu_submit(g);
}

// one dispatch of a pipeline over a group (push: ticks, first life)
static bool dispatch(GpuContext& g, GpuBatch& b, GpuGroup& group, VkPipeline pipeline,
                     uint32_t workgroups, int32_t ticks, int32_t first = 0) {
    if (!gpu_begin(g)) {
        return false;
    }
    int32_t push[2] = {ticks, first};
    vkCmdBindPipeline(g.command, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(g.command, VK_PIPELINE_BIND_POINT_COMPUTE, b.layout, 0, 1, &group.set,
                            0, nullptr);
    vkCmdPushConstants(g.command, b.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), push);
    vkCmdDispatch(g.command, workgroups, 1, 1);
    return gpu_submit(g);
}

// a new device buffer holding these bytes
static bool upload_new(GpuContext& g, GpuBuffer& buf, const void* data, uint64_t bytes) {
    return gpu_buffer(g, buf, bytes, false) && gpu_upload(g, buf, 0, data, bytes);
}

bool gpu_buffer_new(GpuContext& g, GpuBuffer& buf, const void* data, uint64_t bytes) {
    bytes = bytes < 8 ? 8 : bytes;
    return data ? upload_new(g, buf, data, bytes) : zeros_new(g, buf, bytes);
}

bool gpu_group_exchange(GpuContext& g, GpuBatch& b, GpuGroup& gr) {
    const size_t xn = b.shape.host ? (size_t)gr.count : 1, tiles1 = (size_t)b.shape.tiles + 1;
    bool ok = gpu_buffer(g, gr.xin, sizeof(GpuExchangeIn) * xn, true) &&
              gpu_buffer(g, gr.xwords, 8 * tiles1 * xn, true) &&
              gpu_buffer_read_back(g, gr.xout, sizeof(GpuExchangeOut) * xn);
    if (ok) {
        std::memset(gr.xin.mapped, 0, sizeof(GpuExchangeIn) * xn);
        std::memset(gr.xwords.mapped, 0, 8 * tiles1 * xn);
        std::memset(gr.xout.mapped, 0, sizeof(GpuExchangeOut) * xn);
    }
    return ok;
}

bool gpu_group_bind(GpuContext& g, GpuBatch& b, GpuGroup& gr) {
    VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc.descriptorPool = b.pool;
    alloc.descriptorSetCount = 1;
    alloc.pSetLayouts = &b.set_layout;
    if (!gpu_ok(g, vkAllocateDescriptorSets(g.device, &alloc, &gr.set), "descriptor set")) {
        return false;
    }
    GpuBuffer* order[BINDINGS] = {
        &gr.params, &b.world,       &gr.genomes,  &gr.lives,  &gr.masks,    &gr.thresholds,
        &gr.rings,  &gr.inhibitory, &gr.scores,   &gr.xin,    &gr.xwords,   &gr.xout,
        &b.ev_meta, &gr.ev_planes,  &gr.ev_lines, &gr.delays, &gr.far_tiles};
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
    return true;
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
    // the exchange: host-visible, written and read in place between dispatches
    ok = ok && gpu_group_exchange(g, b, gr);
    // the grown-back pieces: zeros, or what birth fills
    size_t ew = (size_t)ev_words(s);
    ok = ok && zeros_new(g, gr.ev_planes, ew ? ew * n * 8 : 8) &&
         zeros_new(g, gr.ev_lines, s.ev ? (size_t)s.ev_rows * n : 8) &&
         zeros_new(g, gr.delays, s.delay ? mw * n : 8) &&
         zeros_new(g, gr.far_tiles, s.slots > 9 ? (size_t)far_words(s) * n * 4 : 8);
    if (!ok || !gpu_group_bind(g, b, gr)) {
        return false;
    }
    // the birth shader: one thread per life, 64 lives a workgroup
    return !born_here || dispatch(g, b, gr, b.birth, (uint32_t)(count + 63) / 64, 0);
}

bool gpu_group_step(GpuContext& g, GpuBatch& b, GpuGroup& group, int ticks) {
    return dispatch(g, b, group, b.pipeline, (uint32_t)group.count, ticks);
}

bool gpu_group_step_part(GpuContext& g, GpuBatch& b, GpuGroup& group, int ticks, int first,
                         int count) {
    return dispatch(g, b, group, b.pipeline, (uint32_t)count, ticks, first);
}

bool gpu_group_step_async(GpuContext& g, GpuBatch& b, GpuGroup& group, int ticks, int first,
                          int count, GpuSlot& slot) {
    if (!gpu_slot_begin(g, slot)) {
        return false;
    }
    int32_t push[2] = {ticks, first};
    vkCmdBindPipeline(slot.command, VK_PIPELINE_BIND_POINT_COMPUTE, b.pipeline);
    vkCmdBindDescriptorSets(slot.command, VK_PIPELINE_BIND_POINT_COMPUTE, b.layout, 0, 1,
                            &group.set, 0, nullptr);
    vkCmdPushConstants(slot.command, b.layout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(push), push);
    vkCmdDispatch(slot.command, (uint32_t)count, 1, 1);
    return gpu_slot_submit(g, slot);
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
    uint64_t ew = ev_words(s), fw = far_words(s);
    out.ev_planes.resize(ew);
    out.ev_lines.resize(s.ev ? (size_t)s.ev_rows : 0);
    out.delays.resize(s.delay ? mw : 0);
    out.far_tiles.resize(fw);
    bool ok = (!ew || gpu_download(g, group.ev_planes, i * ew * 8, out.ev_planes.data(), ew * 8)) &&
              (!s.ev || gpu_download(g, group.ev_lines, i * s.ev_rows, out.ev_lines.data(),
                                     (uint64_t)s.ev_rows)) &&
              (!s.delay || gpu_download(g, group.delays, i * mw, out.delays.data(), mw)) &&
              (!fw || gpu_download(g, group.far_tiles, i * fw * 4, out.far_tiles.data(), fw * 4));
    return ok && gpu_download(g, group.lives, i * sizeof(GpuLife), &out.life, sizeof(GpuLife)) &&
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
        GpuBuffer* all[] = {&gr.params, &gr.genomes,    &gr.lives,    &gr.masks,  &gr.thresholds,
                            &gr.rings,  &gr.inhibitory, &gr.scores,   &gr.xin,    &gr.xwords,
                            &gr.xout,   &gr.ev_planes,  &gr.ev_lines, &gr.delays, &gr.far_tiles};
        for (GpuBuffer* buf : all) {
            gpu_free(g, *buf);
        }
    }
    gpu_free(g, b.world);
    gpu_free(g, b.ev_meta);
    vkDestroyPipeline(g.device, b.pipeline, nullptr);
    if (b.birth != VK_NULL_HANDLE) {
        vkDestroyPipeline(g.device, b.birth, nullptr);
    }
    vkDestroyDescriptorPool(g.device, b.pool, nullptr);
    vkDestroyPipelineLayout(g.device, b.layout, nullptr);
    vkDestroyDescriptorSetLayout(g.device, b.set_layout, nullptr);
    b = GpuBatch{};
}
