// A frame: clear the screen, the views, the ImGui panels, then the window (or nothing, for
// a capture). And reading the screen back.

#include "renderer/vulkan_internal.hpp"

#include <cstring>

bool renderer_frame(Renderer& r, const ViewFrame& f, const Layout& l, const ViewSettings& v,
                    bool views) {
    vk::Context& g = vk::ctx(r);
    if (r.lost || (r.window != nullptr && !vk::swapchain(r)) || !vk::begin(g)) {
        return false;
    }
    if (views) {
        vk::upload_views(g, f, v);
    }

    // everything into the screen, in one rendering
    vk::transition(g, g.screen, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    VkRenderingAttachmentInfo color{VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO};
    color.imageView = g.screen.view;
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color.clearValue.color = VkClearColorValue{{0.043f, 0.051f, 0.071f, 1.0f}}; // #0b0d12
    VkRenderingInfo info{VK_STRUCTURE_TYPE_RENDERING_INFO};
    info.renderArea =
        VkRect2D{{0, 0}, {static_cast<uint32_t>(g.screen.w), static_cast<uint32_t>(g.screen.h)}};
    info.layerCount = 1;
    info.colorAttachmentCount = 1;
    info.pColorAttachments = &color;
    vkCmdBeginRendering(g.command, &info);
    if (views) {
        vk::record_views(g, f, l, v);
    }
    vk::imgui_vulkan_record(g);
    vkCmdEndRendering(g.command);

    // onto the window, or just submitted
    return r.window != nullptr ? vk::present(r) : vk::finish(g);
}

bool renderer_read(Renderer& r, std::vector<std::uint8_t>& rgba) {
    vk::Context& g = vk::ctx(r);
    vk::Buffer staging;
    const VkDeviceSize size = static_cast<VkDeviceSize>(g.screen.w) * g.screen.h * 4;
    if (r.lost || !vk::wait(g) || !vk::buffer(g, staging, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT) ||
        !vk::begin(g)) {
        return false;
    }
    vk::transition(g, g.screen, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    VkBufferImageCopy copy{};
    copy.imageSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    copy.imageExtent = {static_cast<uint32_t>(g.screen.w), static_cast<uint32_t>(g.screen.h), 1};
    vkCmdCopyImageToBuffer(g.command, g.screen.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                           staging.buffer, 1, &copy);
    const bool ok = vk::finish(g) && vk::wait(g);
    if (ok) {
        rgba.resize(size);
        std::memcpy(rgba.data(), staging.mapped, size);
    }
    vk::destroy_buffer(g, staging);
    return ok;
}
