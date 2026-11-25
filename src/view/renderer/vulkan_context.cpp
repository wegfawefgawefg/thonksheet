// The instance, device and queue; the window's swapchain and the present. No window: no
// surface, no swapchain, and nothing ever reaches a display.

#include "renderer/vulkan_internal.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <algorithm>
#include <cstdio>

namespace vk {

static VKAPI_ATTR VkBool32 VKAPI_CALL on_message(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                 VkDebugUtilsMessageTypeFlagsEXT,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                 void*) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        std::fprintf(stderr, "vulkan: %s\n", data->pMessage);
    }
    return VK_FALSE;
}

static bool create_instance(Context& g, SDL_Window* window, bool validation) {
    std::vector<const char*> extensions;
    if (window != nullptr) {
        Uint32 count = 0;
        const char* const* names = SDL_Vulkan_GetInstanceExtensions(&count);
        extensions.assign(names, names + count);
    }
    const char* layer = "VK_LAYER_KHRONOS_validation";
    if (validation) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "blobview";
    app.apiVersion = VK_API_VERSION_1_3;
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pApplicationInfo = &app;
    info.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    info.ppEnabledExtensionNames = extensions.data();
    info.enabledLayerCount = validation ? 1 : 0;
    info.ppEnabledLayerNames = &layer;
    if (!check(vkCreateInstance(&info, nullptr, &g.instance), "create instance")) {
        return false;
    }
    if (validation) {
        auto create = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(g.instance, "vkCreateDebugUtilsMessengerEXT"));
        VkDebugUtilsMessengerCreateInfoEXT m{
            VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
        m.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        m.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        m.pfnUserCallback = on_message;
        if (create != nullptr) {
            create(g.instance, &m, nullptr, &g.messenger);
        }
    }
    return true;
}

// a discrete GPU first, then integrated, then anything (lavapipe); Vulkan 1.3 for dynamic
// rendering and synchronization2
static bool pick_device(Renderer& r, Context& g) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(g.instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(g.instance, &count, devices.data());
    int best = -1;
    for (VkPhysicalDevice d : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(d, &props);
        if (props.apiVersion < VK_API_VERSION_1_3) {
            continue;
        }
        uint32_t qcount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qcount, nullptr);
        std::vector<VkQueueFamilyProperties> families(qcount);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qcount, families.data());
        for (uint32_t f = 0; f < qcount; ++f) {
            VkBool32 present = VK_TRUE;
            if (g.surface != VK_NULL_HANDLE) {
                vkGetPhysicalDeviceSurfaceSupportKHR(d, f, g.surface, &present);
            }
            if ((families[f].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 || !present) {
                continue;
            }
            const int score = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU     ? 3
                              : props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU ? 2
                                                                                           : 1;
            if (score > best) {
                best = score;
                g.physical = d;
                g.family = f;
                r.gpu_name = props.deviceName;
            }
            break;
        }
    }
    if (g.physical == VK_NULL_HANDLE) {
        std::fprintf(stderr, "blobview: no Vulkan 1.3 device\n");
        return false;
    }
    vkGetPhysicalDeviceMemoryProperties(g.physical, &g.memory);
    return true;
}

static bool create_device(Context& g) {
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue.queueFamilyIndex = g.family;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;
    const char* swapchain_ext = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
    VkPhysicalDeviceVulkan13Features v13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    v13.dynamicRendering = VK_TRUE;
    v13.synchronization2 = VK_TRUE;
    VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features.pNext = &v13;
    VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    info.pNext = &features;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue;
    info.enabledExtensionCount = g.surface != VK_NULL_HANDLE ? 1 : 0;
    info.ppEnabledExtensionNames = &swapchain_ext;
    if (!check(vkCreateDevice(g.physical, &info, nullptr, &g.device), "create device")) {
        return false;
    }
    vkGetDeviceQueue(g.device, g.family, 0, &g.queue);
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = g.family;
    return check(vkCreateCommandPool(g.device, &pool, nullptr, &g.pool), "command pool");
}

static bool create_slots(Context& g) {
    for (Slot& s : g.slots) {
        VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
        alloc.commandPool = g.pool;
        alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        alloc.commandBufferCount = 1;
        VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
        VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
        if (!check(vkAllocateCommandBuffers(g.device, &alloc, &s.command), "command buffer") ||
            !check(vkCreateFence(g.device, &fence, nullptr, &s.fence), "fence") ||
            !check(vkCreateSemaphore(g.device, &sem, nullptr, &s.acquired), "semaphore")) {
            return false;
        }
    }
    return true;
}

bool init(Renderer& r, bool validation) {
    Context* state = new Context;
    r.gpu = state;
    Context& g = *state;
    g.owner = &r;
    if (!create_instance(g, r.window, validation)) {
        return false;
    }
    if (r.window != nullptr &&
        !SDL_Vulkan_CreateSurface(r.window, g.instance, nullptr, &g.surface)) {
        std::fprintf(stderr, "blobview: no Vulkan surface: %s\n", SDL_GetError());
        return false;
    }
    if (!pick_device(r, g) || !create_device(g) || !create_slots(g)) {
        return false;
    }
    std::printf("blobview: %s\n", r.gpu_name.c_str());
    return true;
}

static void destroy_swapchain(Context& g) {
    for (VkSemaphore s : g.finished) {
        vkDestroySemaphore(g.device, s, nullptr);
    }
    g.finished.clear();
    g.swap_images.clear();
    if (g.swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(g.device, g.swapchain, nullptr);
        g.swapchain = VK_NULL_HANDLE;
    }
}

void shutdown(Renderer& r) {
    if (r.gpu == nullptr) {
        return;
    }
    Context& g = ctx(r);
    if (g.device != VK_NULL_HANDLE) {
        if (!r.lost) {
            vkDeviceWaitIdle(g.device);
        }
        for (Slot& s : g.slots) {
            destroy_buffer(g, s.tape);
            destroy_buffer(g, s.marks);
            destroy_buffer(g, s.overlay);
            vkDestroyFence(g.device, s.fence, nullptr);
            vkDestroySemaphore(g.device, s.acquired, nullptr);
        }
        destroy_buffer(g, g.kinds);
        destroy_image(g, g.screen);
        destroy_swapchain(g);
        vkDestroyCommandPool(g.device, g.pool, nullptr);
        vkDestroyDevice(g.device, nullptr);
    }
    if (g.surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(g.instance, g.surface, nullptr);
    }
    if (g.messenger != VK_NULL_HANDLE) {
        auto destroy = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(g.instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (destroy != nullptr) {
            destroy(g.instance, g.messenger, nullptr);
        }
    }
    if (g.instance != VK_NULL_HANDLE) {
        vkDestroyInstance(g.instance, nullptr);
    }
    delete &g;
    r.gpu = nullptr;
}

// FIFO: the viewer's frame rate is the display's; the sim runs on its own thread anyway
bool swapchain(Renderer& r) {
    Context& g = ctx(r);
    if (!g.swap_stale) {
        return true;
    }
    if (!wait(g)) {
        return false;
    }
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g.physical, g.surface, &caps);
    uint32_t n = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(g.physical, g.surface, &n, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(n);
    vkGetPhysicalDeviceSurfaceFormatsKHR(g.physical, g.surface, &n, formats.data());
    VkSurfaceFormatKHR format = formats.empty() ? VkSurfaceFormatKHR{} : formats[0];
    for (const VkSurfaceFormatKHR& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM) {
            format = f; // UNORM, so the palette's bytes reach the screen untouched
            break;
        }
    }
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        extent.width = std::clamp(static_cast<uint32_t>(r.w), caps.minImageExtent.width,
                                  caps.maxImageExtent.width);
        extent.height = std::clamp(static_cast<uint32_t>(r.h), caps.minImageExtent.height,
                                   caps.maxImageExtent.height);
    }
    uint32_t count = std::max(3u, caps.minImageCount);
    if (caps.maxImageCount > 0) {
        count = std::min(count, caps.maxImageCount);
    }
    VkSwapchainCreateInfoKHR info{VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR};
    info.surface = g.surface;
    info.minImageCount = count;
    info.imageFormat = format.format;
    info.imageColorSpace = format.colorSpace;
    info.imageExtent = extent;
    info.imageArrayLayers = 1;
    info.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    info.preTransform = caps.currentTransform;
    info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    info.clipped = VK_TRUE;
    info.oldSwapchain = g.swapchain;
    VkSwapchainKHR made = VK_NULL_HANDLE;
    if (!check(vkCreateSwapchainKHR(g.device, &info, nullptr, &made), "create swapchain")) {
        return false;
    }
    destroy_swapchain(g);
    g.swapchain = made;
    g.extent = extent;
    vkGetSwapchainImagesKHR(g.device, g.swapchain, &n, nullptr);
    g.swap_images.resize(n);
    vkGetSwapchainImagesKHR(g.device, g.swapchain, &n, g.swap_images.data());
    g.finished.assign(n, VK_NULL_HANDLE);
    VkSemaphoreCreateInfo sem{VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
    for (VkSemaphore& s : g.finished) {
        if (!check(vkCreateSemaphore(g.device, &sem, nullptr, &s), "present semaphore")) {
            return false;
        }
    }
    g.swap_stale = false;
    return true;
}

bool present(Renderer& r) {
    Context& g = ctx(r);
    const VkSemaphore acquired = now(g).acquired;
    uint32_t index = 0;
    VkResult got = vkAcquireNextImageKHR(g.device, g.swapchain, 5'000'000'000ull, acquired,
                                         VK_NULL_HANDLE, &index);
    if (got == VK_ERROR_OUT_OF_DATE_KHR) {
        g.swap_stale = true;
        return finish(g); // the frame's drawing still goes in; nothing shown this once
    }
    if (got != VK_SUBOPTIMAL_KHR && !check_alive(g, got, "acquire image")) {
        return false;
    }
    // the screen blitted over the whole window image (they are the same size but for a
    // resize in flight)
    transition(g, g.screen, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    const VkImage dest = g.swap_images[index];
    transition_raw(g, dest, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    VkImageBlit blit{};
    blit.srcSubresource = blit.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
    blit.srcOffsets[1] = {g.screen.w, g.screen.h, 1};
    blit.dstOffsets[1] = {static_cast<int>(g.extent.width), static_cast<int>(g.extent.height), 1};
    vkCmdBlitImage(g.command, g.screen.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dest,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_NEAREST);
    transition_raw(g, dest, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
    if (!finish(g, g.finished[index], acquired)) {
        return false;
    }
    VkPresentInfoKHR info{VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    info.waitSemaphoreCount = 1;
    info.pWaitSemaphores = &g.finished[index];
    info.swapchainCount = 1;
    info.pSwapchains = &g.swapchain;
    info.pImageIndices = &index;
    const VkResult shown = vkQueuePresentKHR(g.queue, &info);
    if (shown == VK_ERROR_OUT_OF_DATE_KHR || shown == VK_SUBOPTIMAL_KHR) {
        g.swap_stale = true;
        return true;
    }
    return check_alive(g, shown, "present");
}

} // namespace vk
