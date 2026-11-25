// The views' pipelines. Each draws quads made in the vertex shader from gl_VertexIndex
// (no vertex input) into the screen with dynamic rendering. One descriptor set per slot:
// the tape, the kinds, the marks.

#include "renderer/vulkan_internal.hpp"

#include <cstdio>
#include <fstream>
#include <string>

namespace vk {

static VkShaderModule load_shader(Context& g, const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        std::fprintf(stderr, "blobview: no shader %s\n", path.c_str());
        return VK_NULL_HANDLE;
    }
    const size_t size = static_cast<size_t>(file.tellg());
    std::vector<uint32_t> code((size + 3) / 4);
    file.seekg(0);
    file.read(reinterpret_cast<char*>(code.data()), static_cast<std::streamsize>(size));
    VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
    info.codeSize = size;
    info.pCode = code.data();
    VkShaderModule module = VK_NULL_HANDLE;
    check(vkCreateShaderModule(g.device, &info, nullptr, &module), "shader module");
    return module;
}

static VkPipeline make_pipeline(Context& g, const char* dir, const char* vert, const char* frag,
                                bool blend) {
    const VkShaderModule vs = load_shader(g, std::string(dir) + "/" + vert + ".spv");
    const VkShaderModule fs = load_shader(g, std::string(dir) + "/" + frag + ".spv");
    if (vs == VK_NULL_HANDLE || fs == VK_NULL_HANDLE) {
        return VK_NULL_HANDLE;
    }
    VkPipelineShaderStageCreateInfo stages[2]{};
    for (int i = 0; i < 2; ++i) {
        stages[i] =
            VkPipelineShaderStageCreateInfo{VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
        stages[i].stage = i == 0 ? VK_SHADER_STAGE_VERTEX_BIT : VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[i].module = i == 0 ? vs : fs;
        stages[i].pName = "main";
    }
    VkPipelineVertexInputStateCreateInfo vertex{
        VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO};
    VkPipelineInputAssemblyStateCreateInfo assembly{
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO};
    assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkPipelineViewportStateCreateInfo viewport{
        VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO};
    viewport.viewportCount = 1;
    viewport.scissorCount = 1;
    VkPipelineRasterizationStateCreateInfo raster{
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
    raster.polygonMode = VK_POLYGON_MODE_FILL;
    raster.cullMode = VK_CULL_MODE_NONE;
    raster.lineWidth = 1.0f;
    VkPipelineMultisampleStateCreateInfo multisample{
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO};
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    VkPipelineDepthStencilStateCreateInfo depth{
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO};
    VkPipelineColorBlendAttachmentState b{};
    b.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                       VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    if (blend) {
        b.blendEnable = VK_TRUE;
        b.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        b.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        b.colorBlendOp = VK_BLEND_OP_ADD;
        b.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        b.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        b.alphaBlendOp = VK_BLEND_OP_ADD;
    }
    VkPipelineColorBlendStateCreateInfo blending{
        VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO};
    blending.attachmentCount = 1;
    blending.pAttachments = &b;
    const VkDynamicState dynamic_states[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic{VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
    dynamic.dynamicStateCount = 2;
    dynamic.pDynamicStates = dynamic_states;
    const VkFormat color = screen_format;
    VkPipelineRenderingCreateInfo rendering{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};
    rendering.colorAttachmentCount = 1;
    rendering.pColorAttachmentFormats = &color;

    VkGraphicsPipelineCreateInfo info{VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO};
    info.pNext = &rendering;
    info.stageCount = 2;
    info.pStages = stages;
    info.pVertexInputState = &vertex;
    info.pInputAssemblyState = &assembly;
    info.pViewportState = &viewport;
    info.pRasterizationState = &raster;
    info.pMultisampleState = &multisample;
    info.pDepthStencilState = &depth;
    info.pColorBlendState = &blending;
    info.pDynamicState = &dynamic;
    info.layout = g.pipeline_layout;
    VkPipeline pipeline = VK_NULL_HANDLE;
    check(vkCreateGraphicsPipelines(g.device, VK_NULL_HANDLE, 1, &info, nullptr, &pipeline),
          "graphics pipeline");
    vkDestroyShaderModule(g.device, vs, nullptr);
    vkDestroyShaderModule(g.device, fs, nullptr);
    return pipeline;
}

// four storage buffers: the tape (0), the kinds (1), the marks (2), the overlay (3)
constexpr int binding_count = 4;

static bool make_layouts(Context& g) {
    VkDescriptorSetLayoutBinding bindings[binding_count];
    for (int i = 0; i < binding_count; ++i) {
        bindings[i] = VkDescriptorSetLayoutBinding{};
        bindings[i].binding = static_cast<uint32_t>(i);
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkDescriptorSetLayoutCreateInfo set_info{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    set_info.bindingCount = binding_count;
    set_info.pBindings = bindings;
    VkPushConstantRange push{VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
                             sizeof(ViewPush)};
    VkPipelineLayoutCreateInfo layout_info{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    layout_info.setLayoutCount = 1;
    layout_info.pSetLayouts = &g.set_layout;
    layout_info.pushConstantRangeCount = 1;
    layout_info.pPushConstantRanges = &push;
    VkDescriptorPoolSize size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, binding_count * slot_count};
    VkDescriptorPoolCreateInfo pool_info{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    pool_info.maxSets = slot_count;
    pool_info.poolSizeCount = 1;
    pool_info.pPoolSizes = &size;
    if (!check(vkCreateDescriptorSetLayout(g.device, &set_info, nullptr, &g.set_layout),
               "set layout") ||
        !check(vkCreatePipelineLayout(g.device, &layout_info, nullptr, &g.pipeline_layout),
               "pipeline layout") ||
        !check(vkCreateDescriptorPool(g.device, &pool_info, nullptr, &g.descriptors),
               "descriptor pool")) {
        return false;
    }
    for (Slot& s : g.slots) {
        VkDescriptorSetAllocateInfo alloc{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
        alloc.descriptorPool = g.descriptors;
        alloc.descriptorSetCount = 1;
        alloc.pSetLayouts = &g.set_layout;
        if (!check(vkAllocateDescriptorSets(g.device, &alloc, &s.set), "descriptor set")) {
            return false;
        }
    }
    return true;
}

bool init_pipelines(Context& g, const char* shader_dir) {
    if (!make_layouts(g)) {
        return false;
    }
    g.sheet = make_pipeline(g, shader_dir, "view.vert", "sheet.frag", false);
    g.raster = make_pipeline(g, shader_dir, "view.vert", "raster.frag", false);
    g.recur = make_pipeline(g, shader_dir, "view.vert", "recur.frag", false);
    g.marks = make_pipeline(g, shader_dir, "marks.vert", "marks.frag", true);
    return g.sheet != VK_NULL_HANDLE && g.raster != VK_NULL_HANDLE && g.recur != VK_NULL_HANDLE &&
           g.marks != VK_NULL_HANDLE;
}

void shutdown_pipelines(Context& g) {
    for (VkPipeline p : {g.sheet, g.raster, g.recur, g.marks}) {
        vkDestroyPipeline(g.device, p, nullptr);
    }
    vkDestroyDescriptorPool(g.device, g.descriptors, nullptr);
    vkDestroyPipelineLayout(g.device, g.pipeline_layout, nullptr);
    vkDestroyDescriptorSetLayout(g.device, g.set_layout, nullptr);
}

} // namespace vk
