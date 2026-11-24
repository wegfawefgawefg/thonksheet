#pragma once
// The Vulkan state behind device.hpp, after Mans In My House's renderer/vulkan_internal.hpp:
// buffers and images with their memory, submission slots taken in turn, and one Context
// holding all of it.

#include "renderer/device.hpp"
#include "renderer/marks.hpp"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace vk {

struct Buffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize capacity = 0;
    void* mapped = nullptr; // every buffer here is host-visible and stays mapped
};

struct Image {
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkImageView view = VK_NULL_HANDLE;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED; // as the last recorded command left it
    int w = 0, h = 0;
};

// One submission's command buffer and fence, and the buffers the views read from. A slot
// is written again only once its last submission has finished.
struct Slot {
    VkCommandBuffer command = VK_NULL_HANDLE;
    VkFence fence = VK_NULL_HANDLE;
    VkSemaphore acquired = VK_NULL_HANDLE; // the swapchain image its blit goes into
    bool pending = false;
    VkDescriptorSet set = VK_NULL_HANDLE;
    Buffer tape;           // the tape's spike words, as many rows as the ViewFrame keeps
    Buffer marks;          // the picked neuron's marks
    uint64_t uploaded = 0; // tape rows this slot's buffer has
    int mark_count = 0;
};
inline constexpr int slot_count = 2;

// mirrors the push block in shaders/common.glsl
struct ViewPush {
    float rect[4];
    float extent[2];
    float origin[2];
    float cell_px;
    float fade;
    uint32_t cells_w, cells_h, tiles_w, tile_count;
    uint32_t tape_len, newest_row, rows_valid, flags;
    uint32_t sector_tiles, mode;
    int32_t selected;
    uint32_t word_step;
};

inline constexpr VkFormat screen_format = VK_FORMAT_R8G8B8A8_UNORM;

struct Context {
    VkInstance instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    VkPhysicalDevice physical = VK_NULL_HANDLE;
    VkPhysicalDeviceMemoryProperties memory{};
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t family = 0;
    VkCommandPool pool = VK_NULL_HANDLE;
    Renderer* owner = nullptr; // its `lost` is set on a lost or hung device

    Slot slots[slot_count];
    int slot = 0;
    VkCommandBuffer command = VK_NULL_HANDLE; // the slot being recorded

    // the views' pipelines (vulkan_pipeline.cpp) and what they read
    VkDescriptorSetLayout set_layout = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptors = VK_NULL_HANDLE;
    VkPipeline sheet = VK_NULL_HANDLE, raster = VK_NULL_HANDLE, recur = VK_NULL_HANDLE,
               marks = VK_NULL_HANDLE;
    Buffer kinds;                     // per tile: inhibitory, sensor, motor
    int tile_count = 0, tape_len = 0; // the shape the buffers are for
    std::vector<Mark> mark_list;

    // everything is drawn here first
    Image screen;

    // the window's
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkExtent2D extent{};
    std::vector<VkImage> swap_images;
    std::vector<VkSemaphore> finished; // one per swapchain image: its blit is done
    bool swap_stale = true;
};

inline Context& ctx(Renderer& r) {
    return *static_cast<Context*>(r.gpu);
}
inline Slot& now(Context& g) {
    return g.slots[g.slot];
}

// vulkan_resources.cpp
bool check(VkResult result, const char* what);
// as check, and a lost device or a wait that timed out marks the renderer lost
bool check_alive(Context& g, VkResult result, const char* what);
uint32_t memory_type(Context& g, uint32_t bits, VkMemoryPropertyFlags want);
// a host-visible buffer of at least `size` bytes, kept if big enough
bool buffer(Context& g, Buffer& b, VkDeviceSize size, VkBufferUsageFlags usage);
void destroy_buffer(Context& g, Buffer& b);
bool image(Context& g, Image& img, int w, int h, VkFormat format, VkImageUsageFlags usage);
void destroy_image(Context& g, Image& img);
void transition(Context& g, Image& img, VkImageLayout layout);
void transition_raw(Context& g, VkImage img, VkImageLayout from, VkImageLayout to);
bool wait(Context& g);  // every slot's last submission finished
bool begin(Context& g); // the next slot, once free, recording
bool finish(Context& g, VkSemaphore signal = VK_NULL_HANDLE, VkSemaphore waits = VK_NULL_HANDLE);

// vulkan_context.cpp
bool init(Renderer& r, bool validation);
void shutdown(Renderer& r);
bool swapchain(Renderer& r);
// the screen blitted onto a swapchain image, inside the slot being recorded, then submitted
// and presented
bool present(Renderer& r);

// vulkan_pipeline.cpp
bool init_pipelines(Context& g, const char* shader_dir);
void shutdown_pipelines(Context& g);

// vulkan_views.cpp
void upload_views(Context& g, const ViewFrame& f, const ViewSettings& v);
void record_views(Context& g, const ViewFrame& f, const Layout& l, const ViewSettings& v);

// vulkan_imgui.cpp
bool imgui_vulkan_init(Context& g);
void imgui_vulkan_shutdown();
void imgui_vulkan_record(Context& g);

} // namespace vk
