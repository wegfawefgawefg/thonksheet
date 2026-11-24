// Dear ImGui's Vulkan backend, drawing into the screen image like everything else.

#include "renderer/vulkan_internal.hpp"

#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

namespace vk {

static VkFormat imgui_format = screen_format;

bool imgui_vulkan_init(Context& g) {
    ImGui_ImplVulkan_InitInfo info{};
    info.ApiVersion = VK_API_VERSION_1_3;
    info.Instance = g.instance;
    info.PhysicalDevice = g.physical;
    info.Device = g.device;
    info.QueueFamily = g.family;
    info.Queue = g.queue;
    info.DescriptorPoolSize = 64;
    info.MinImageCount = 2;
    info.ImageCount = slot_count;
    info.UseDynamicRendering = true;
    info.PipelineInfoMain.PipelineRenderingCreateInfo =
        VkPipelineRenderingCreateInfoKHR{VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR};
    info.PipelineInfoMain.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    info.PipelineInfoMain.PipelineRenderingCreateInfo.pColorAttachmentFormats = &imgui_format;
    return ImGui_ImplVulkan_Init(&info);
}

void imgui_vulkan_shutdown() {
    ImGui_ImplVulkan_Shutdown();
}

// the frame's ImGui draw data, inside the rendering already begun
void imgui_vulkan_record(Context& g) {
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), g.command);
}

} // namespace vk
