// Buffers, images, layouts and submission slots. Every wait on the GPU has a timeout, so a
// hung device ends the viewer instead of the desktop.

#include "renderer/vulkan_internal.hpp"

#include <algorithm>
#include <cstdio>

namespace vk {

constexpr uint64_t fence_timeout_ns = 5'000'000'000ull;

bool check(VkResult result, const char* what) {
    if (result != VK_SUCCESS) {
        std::fprintf(stderr, "blobview: %s failed (%d)\n", what, static_cast<int>(result));
        return false;
    }
    return true;
}

bool check_alive(Context& g, VkResult result, const char* what) {
    if (result == VK_ERROR_DEVICE_LOST || result == VK_TIMEOUT) {
        std::fprintf(stderr, "blobview: %s: device lost or hung (%d), leaving\n", what,
                     static_cast<int>(result));
        g.owner->lost = true;
        return false;
    }
    return check(result, what);
}

uint32_t memory_type(Context& g, uint32_t bits, VkMemoryPropertyFlags want) {
    for (uint32_t i = 0; i < g.memory.memoryTypeCount; ++i) {
        if ((bits & (1u << i)) && (g.memory.memoryTypes[i].propertyFlags & want) == want) {
            return i;
        }
    }
    return UINT32_MAX;
}

bool buffer(Context& g, Buffer& b, VkDeviceSize size, VkBufferUsageFlags usage) {
    if (b.capacity >= size && b.buffer != VK_NULL_HANDLE) {
        return true;
    }
    destroy_buffer(g, b);
    b.capacity = std::max<VkDeviceSize>(size, 256);
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = b.capacity;
    info.usage = usage;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!check(vkCreateBuffer(g.device, &info, nullptr, &b.buffer), "create buffer")) {
        return false;
    }
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(g.device, b.buffer, &req);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex =
        memory_type(g, req.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    return check(vkAllocateMemory(g.device, &alloc, nullptr, &b.memory), "buffer memory") &&
           check(vkBindBufferMemory(g.device, b.buffer, b.memory, 0), "bind buffer") &&
           check(vkMapMemory(g.device, b.memory, 0, VK_WHOLE_SIZE, 0, &b.mapped), "map buffer");
}

void destroy_buffer(Context& g, Buffer& b) {
    if (b.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(g.device, b.buffer, nullptr);
        vkFreeMemory(g.device, b.memory, nullptr);
    }
    b = Buffer{};
}

bool image(Context& g, Image& img, int w, int h, VkFormat format, VkImageUsageFlags usage) {
    destroy_image(g, img);
    img.w = w;
    img.h = h;
    VkImageCreateInfo info{VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
    info.imageType = VK_IMAGE_TYPE_2D;
    info.format = format;
    info.extent = VkExtent3D{static_cast<uint32_t>(w), static_cast<uint32_t>(h), 1};
    info.mipLevels = 1;
    info.arrayLayers = 1;
    info.samples = VK_SAMPLE_COUNT_1_BIT;
    info.tiling = VK_IMAGE_TILING_OPTIMAL;
    info.usage = usage;
    info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (!check(vkCreateImage(g.device, &info, nullptr, &img.image), "create image")) {
        return false;
    }
    VkMemoryRequirements req;
    vkGetImageMemoryRequirements(g.device, img.image, &req);
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memory_type(g, req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (!check(vkAllocateMemory(g.device, &alloc, nullptr, &img.memory), "image memory") ||
        !check(vkBindImageMemory(g.device, img.image, img.memory, 0), "bind image")) {
        return false;
    }
    VkImageViewCreateInfo view{VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
    view.image = img.image;
    view.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view.format = format;
    view.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    return check(vkCreateImageView(g.device, &view, nullptr, &img.view), "image view");
}

void destroy_image(Context& g, Image& img) {
    if (img.image != VK_NULL_HANDLE) {
        vkDestroyImageView(g.device, img.view, nullptr);
        vkDestroyImage(g.device, img.image, nullptr);
        vkFreeMemory(g.device, img.memory, nullptr);
    }
    img = Image{};
}

void transition_raw(Context& g, VkImage img, VkImageLayout from, VkImageLayout to) {
    VkImageMemoryBarrier2 b{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
    b.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    b.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    b.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT;
    b.oldLayout = from;
    b.newLayout = to;
    b.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    b.image = img;
    b.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    VkDependencyInfo dep{VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
    dep.imageMemoryBarrierCount = 1;
    dep.pImageMemoryBarriers = &b;
    vkCmdPipelineBarrier2(g.command, &dep);
}

void transition(Context& g, Image& img, VkImageLayout layout) {
    if (img.layout == layout) {
        return;
    }
    transition_raw(g, img.image, img.layout, layout);
    img.layout = layout;
}

// a slot's last submission, finished
static bool settle(Context& g, Slot& s) {
    if (!s.pending) {
        return true;
    }
    const bool ok = check_alive(
        g, vkWaitForFences(g.device, 1, &s.fence, VK_TRUE, fence_timeout_ns), "wait fence");
    s.pending = !ok;
    return ok;
}

bool wait(Context& g) {
    for (Slot& s : g.slots) {
        if (!settle(g, s)) {
            return false;
        }
    }
    return true;
}

bool begin(Context& g) {
    g.slot = (g.slot + 1) % slot_count;
    Slot& s = now(g);
    if (!settle(g, s)) {
        return false;
    }
    g.command = s.command;
    vkResetCommandBuffer(g.command, 0);
    VkCommandBufferBeginInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    return check(vkBeginCommandBuffer(g.command, &info), "begin commands");
}

bool finish(Context& g, VkSemaphore signal, VkSemaphore waits) {
    if (!check(vkEndCommandBuffer(g.command), "end commands")) {
        return false;
    }
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &g.command;
    const VkPipelineStageFlags stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    if (waits != VK_NULL_HANDLE) {
        submit.waitSemaphoreCount = 1;
        submit.pWaitSemaphores = &waits;
        submit.pWaitDstStageMask = &stage;
    }
    if (signal != VK_NULL_HANDLE) {
        submit.signalSemaphoreCount = 1;
        submit.pSignalSemaphores = &signal;
    }
    Slot& s = now(g);
    vkResetFences(g.device, 1, &s.fence);
    if (!check_alive(g, vkQueueSubmit(g.queue, 1, &submit, s.fence), "submit")) {
        return false;
    }
    s.pending = true;
    return true;
}

} // namespace vk
