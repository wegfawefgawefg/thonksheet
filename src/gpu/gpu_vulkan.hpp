#pragma once

#include <cstdint>
#include <string>
#include <vulkan/vulkan.h>

// Headless Vulkan for compute only: an instance with no surface extensions, one device, one
// queue, one command buffer and fence. Nothing here can open a window or reach a display.
// Every wait has a timeout; a lost device or a wait that times out marks the context lost
// and every later call refuses, so the caller exits cleanly instead of piling work on.

struct GpuBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    void* mapped = nullptr; // host-visible buffers only
};

struct GpuContext {
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties memory{};
    VkPhysicalDeviceLimits limits{};
    VkDevice device = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t family = 0;
    VkCommandPool pool = VK_NULL_HANDLE;
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    GpuBuffer staging; // host-visible, for uploads and downloads in chunks
    std::string name;
    bool software = false; // lavapipe
    bool lost = false;
    uint64_t wait_ms = 10000; // a submit that takes longer than this counts as a hang
};

// software: lavapipe only (refuses a real GPU); otherwise a discrete GPU only.
// validation: the Khronos layer, every message printed and counted.
bool gpu_open(GpuContext& g, bool software, bool validation);
void gpu_close(GpuContext& g);
// validation messages seen so far (warnings and errors)
int gpu_messages();

// device-local storage buffer (host: host-visible and mapped instead)
bool gpu_buffer(GpuContext& g, GpuBuffer& b, VkDeviceSize size, bool host);
// host-visible and mapped, cached on the host if the device has such memory: for what the
// host reads back often
bool gpu_buffer_read_back(GpuContext& g, GpuBuffer& b, VkDeviceSize size);
void gpu_free(GpuContext& g, GpuBuffer& b);
bool gpu_upload(GpuContext& g, GpuBuffer& dst, VkDeviceSize offset, const void* data,
                VkDeviceSize bytes);
bool gpu_download(GpuContext& g, GpuBuffer& src, VkDeviceSize offset, void* data,
                  VkDeviceSize bytes);

// record into g.command between begin and submit; submit waits for the fence
bool gpu_begin(GpuContext& g);
bool gpu_submit(GpuContext& g);

// a command buffer and fence of its own, for work that is submitted and waited for later
// (host worlds keep the queue fed while the host answers). every wait has the timeout.
struct GpuSlot {
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    bool pending = false; // submitted and not waited for yet
};
bool gpu_slot_open(GpuContext& g, GpuSlot& s);
void gpu_slot_close(GpuContext& g, GpuSlot& s);
bool gpu_slot_begin(GpuContext& g, GpuSlot& s);
bool gpu_slot_submit(GpuContext& g, GpuSlot& s); // does not wait
bool gpu_slot_wait(GpuContext& g, GpuSlot& s);
// as gpu_check, and a lost device marks the context lost
bool gpu_ok(GpuContext& g, VkResult result, const char* what);
