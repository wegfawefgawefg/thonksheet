#include "gpu/gpu_vulkan.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

// headless compute Vulkan, after the viewer's renderer and Mans In My House's: plain
// functions over one context struct.

static int message_count = 0;

static VKAPI_ATTR VkBool32 VKAPI_CALL on_message(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                 VkDebugUtilsMessageTypeFlagsEXT,
                                                 const VkDebugUtilsMessengerCallbackDataEXT* data,
                                                 void*) {
    if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        message_count++;
        std::fprintf(stderr, "vulkan: %s\n", data->pMessage);
    }
    return VK_FALSE;
}

int gpu_messages() {
    return message_count;
}

bool gpu_ok(GpuContext& g, VkResult result, const char* what) {
    if (result == VK_SUCCESS) {
        return true;
    }
    if (result == VK_ERROR_DEVICE_LOST || result == VK_TIMEOUT) {
        g.lost = true;
        std::fprintf(stderr, "blobgpu: %s: %s, stopping\n", what,
                     result == VK_TIMEOUT ? "the GPU did not finish in time" : "device lost");
        return false;
    }
    std::fprintf(stderr, "blobgpu: %s failed (%d)\n", what, (int)result);
    return false;
}

static bool create_instance(GpuContext& g, bool validation) {
    VkApplicationInfo app{VK_STRUCTURE_TYPE_APPLICATION_INFO};
    app.pApplicationName = "blobgpu";
    app.apiVersion = VK_API_VERSION_1_3;
    const char* layer = "VK_LAYER_KHRONOS_validation";
    const char* debug_ext = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    VkInstanceCreateInfo info{VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
    info.pApplicationInfo = &app;
    info.enabledLayerCount = validation ? 1 : 0;
    info.ppEnabledLayerNames = &layer;
    info.enabledExtensionCount = validation ? 1 : 0; // no surface extensions, ever
    info.ppEnabledExtensionNames = &debug_ext;
    if (!gpu_ok(g, vkCreateInstance(&info, nullptr, &g.instance), "create instance")) {
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
        if (create == nullptr || create(g.instance, &m, nullptr, &g.messenger) != VK_SUCCESS) {
            std::fprintf(stderr, "blobgpu: validation asked for but not available\n");
            return false;
        }
    }
    return true;
}

// software: the CPU device (lavapipe). otherwise a discrete GPU. a compute queue either way.
static bool pick_device(GpuContext& g, bool software) {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(g.instance, &count, nullptr);
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(g.instance, &count, devices.data());
    for (VkPhysicalDevice d : devices) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(d, &props);
        bool cpu = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU;
        bool discrete = props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        if ((software && !cpu) || (!software && !discrete) ||
            props.apiVersion < VK_API_VERSION_1_3) {
            continue;
        }
        uint32_t qcount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qcount, nullptr);
        std::vector<VkQueueFamilyProperties> families(qcount);
        vkGetPhysicalDeviceQueueFamilyProperties(d, &qcount, families.data());
        for (uint32_t f = 0; f < qcount; f++) {
            if (families[f].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                g.physical = d;
                g.family = f;
                g.name = props.deviceName;
                g.limits = props.limits;
                g.software = cpu;
                vkGetPhysicalDeviceMemoryProperties(d, &g.memory);
                return true;
            }
        }
    }
    std::fprintf(stderr, "blobgpu: no %s Vulkan 1.3 device with a compute queue\n",
                 software ? "software (lavapipe)" : "discrete");
    return false;
}

static bool create_device(GpuContext& g) {
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo queue{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queue.queueFamilyIndex = g.family;
    queue.queueCount = 1;
    queue.pQueuePriorities = &priority;
    VkPhysicalDeviceVulkan12Features v12{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    v12.storageBuffer8BitAccess = VK_TRUE;
    v12.shaderInt8 = VK_TRUE;
    v12.shaderBufferInt64Atomics = VK_TRUE;
    v12.shaderSharedInt64Atomics = VK_TRUE;
    VkPhysicalDeviceVulkan13Features v13{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    v13.maintenance4 = VK_TRUE; // the workgroup size is a specialization constant
    v13.subgroupSizeControl = VK_TRUE;
    v12.pNext = &v13;
    VkPhysicalDeviceFeatures2 features{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
    features.pNext = &v12;
    features.features.shaderInt64 = VK_TRUE;
    VkDeviceCreateInfo info{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    info.pNext = &features;
    info.queueCreateInfoCount = 1;
    info.pQueueCreateInfos = &queue;
    if (!gpu_ok(g, vkCreateDevice(g.physical, &info, nullptr, &g.device), "create device")) {
        return false;
    }
    vkGetDeviceQueue(g.device, g.family, 0, &g.queue);
    VkCommandPoolCreateInfo pool{VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
    pool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool.queueFamilyIndex = g.family;
    if (!gpu_ok(g, vkCreateCommandPool(g.device, &pool, nullptr, &g.pool), "command pool")) {
        return false;
    }
    VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc.commandPool = g.pool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    return gpu_ok(g, vkAllocateCommandBuffers(g.device, &alloc, &g.command), "command buffer") &&
           gpu_ok(g, vkCreateFence(g.device, &fence, nullptr, &g.fence), "fence");
}

bool gpu_open(GpuContext& g, bool software, bool validation) {
    if (!create_instance(g, validation) || !pick_device(g, software) || !create_device(g)) {
        return false;
    }
    return gpu_buffer(g, g.staging, 64ull << 20, true);
}

void gpu_close(GpuContext& g) {
    if (g.device != VK_NULL_HANDLE) {
        if (!g.lost) {
            vkDeviceWaitIdle(g.device);
        }
        gpu_free(g, g.staging);
        vkDestroyFence(g.device, g.fence, nullptr);
        vkDestroyCommandPool(g.device, g.pool, nullptr);
        vkDestroyDevice(g.device, nullptr);
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
    g = GpuContext{};
}

static uint32_t memory_type(GpuContext& g, uint32_t bits, VkMemoryPropertyFlags want) {
    for (uint32_t i = 0; i < g.memory.memoryTypeCount; i++) {
        if ((bits & (1u << i)) && (g.memory.memoryTypes[i].propertyFlags & want) == want) {
            return i;
        }
    }
    return UINT32_MAX;
}

static bool make_buffer(GpuContext& g, GpuBuffer& b, VkDeviceSize size, bool host, bool cached);

bool gpu_buffer(GpuContext& g, GpuBuffer& b, VkDeviceSize size, bool host) {
    return make_buffer(g, b, size, host, false);
}

bool gpu_buffer_read_back(GpuContext& g, GpuBuffer& b, VkDeviceSize size) {
    return make_buffer(g, b, size, true, true);
}

static bool make_buffer(GpuContext& g, GpuBuffer& b, VkDeviceSize size, bool host, bool cached) {
    VkBufferCreateInfo info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    info.size = size;
    info.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!gpu_ok(g, vkCreateBuffer(g.device, &info, nullptr, &b.buffer), "create buffer")) {
        return false;
    }
    VkMemoryRequirements req;
    vkGetBufferMemoryRequirements(g.device, b.buffer, &req);
    VkMemoryPropertyFlags want =
        host ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
             : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VkMemoryAllocateInfo alloc{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    alloc.allocationSize = req.size;
    alloc.memoryTypeIndex = memory_type(g, req.memoryTypeBits, want);
    uint32_t cached_type =
        memory_type(g, req.memoryTypeBits, want | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
    alloc.memoryTypeIndex =
        cached && cached_type != UINT32_MAX ? cached_type : alloc.memoryTypeIndex;
    if (alloc.memoryTypeIndex == UINT32_MAX) {
        std::fprintf(stderr, "blobgpu: no memory type for a buffer\n");
        return false;
    }
    if (!gpu_ok(g, vkAllocateMemory(g.device, &alloc, nullptr, &b.memory), "allocate memory") ||
        !gpu_ok(g, vkBindBufferMemory(g.device, b.buffer, b.memory, 0), "bind memory")) {
        return false;
    }
    b.size = size;
    if (host && !gpu_ok(g, vkMapMemory(g.device, b.memory, 0, size, 0, &b.mapped), "map")) {
        return false;
    }
    return true;
}

void gpu_free(GpuContext& g, GpuBuffer& b) {
    if (b.buffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(g.device, b.buffer, nullptr);
    }
    if (b.memory != VK_NULL_HANDLE) {
        vkFreeMemory(g.device, b.memory, nullptr);
    }
    b = GpuBuffer{};
}

// everything before (earlier submits, host writes) is seen by what comes after
static void memory_barrier(VkCommandBuffer command, VkPipelineStageFlags to, VkAccessFlags reads) {
    VkMemoryBarrier barrier{VK_STRUCTURE_TYPE_MEMORY_BARRIER};
    barrier.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
    barrier.dstAccessMask = reads;
    vkCmdPipelineBarrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, to, 0, 1, &barrier, 0,
                         nullptr, 0, nullptr);
}

static bool begin_on(GpuContext& g, VkCommandBuffer command) {
    if (g.lost) {
        return false;
    }
    VkCommandBufferBeginInfo info{VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (!gpu_ok(g, vkResetCommandBuffer(command, 0), "reset commands") ||
        !gpu_ok(g, vkBeginCommandBuffer(command, &info), "begin commands")) {
        return false;
    }
    memory_barrier(command, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                   VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT);
    return true;
}

static bool submit_on(GpuContext& g, VkCommandBuffer command, VkFence fence) {
    memory_barrier(command, VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_READ_BIT);
    if (!gpu_ok(g, vkEndCommandBuffer(command), "end commands")) {
        return false;
    }
    VkSubmitInfo submit{VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &command;
    return gpu_ok(g, vkResetFences(g.device, 1, &fence), "reset fence") &&
           gpu_ok(g, vkQueueSubmit(g.queue, 1, &submit, fence), "submit");
}

static bool wait_on(GpuContext& g, VkFence fence) {
    return gpu_ok(g, vkWaitForFences(g.device, 1, &fence, VK_TRUE, g.wait_ms * 1000000ull), "wait");
}

bool gpu_begin(GpuContext& g) {
    return begin_on(g, g.command);
}

bool gpu_submit(GpuContext& g) {
    return submit_on(g, g.command, g.fence) && wait_on(g, g.fence);
}

bool gpu_slot_open(GpuContext& g, GpuSlot& s) {
    VkCommandBufferAllocateInfo alloc{VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
    alloc.commandPool = g.pool;
    alloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc.commandBufferCount = 1;
    VkFenceCreateInfo fence{VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
    return gpu_ok(g, vkAllocateCommandBuffers(g.device, &alloc, &s.command), "command buffer") &&
           gpu_ok(g, vkCreateFence(g.device, &fence, nullptr, &s.fence), "fence");
}

void gpu_slot_close(GpuContext& g, GpuSlot& s) {
    if (s.pending && !g.lost) {
        wait_on(g, s.fence);
    }
    if (s.fence != VK_NULL_HANDLE) {
        vkDestroyFence(g.device, s.fence, nullptr);
    }
    if (s.command != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(g.device, g.pool, 1, &s.command);
    }
    s = GpuSlot{};
}

bool gpu_slot_begin(GpuContext& g, GpuSlot& s) {
    return begin_on(g, s.command);
}

bool gpu_slot_submit(GpuContext& g, GpuSlot& s) {
    s.pending = submit_on(g, s.command, s.fence);
    return s.pending;
}

bool gpu_slot_wait(GpuContext& g, GpuSlot& s) {
    bool ok = wait_on(g, s.fence);
    s.pending = false;
    return ok;
}

// copy through the staging buffer, a chunk at a time
static bool copy_chunk(GpuContext& g, VkBuffer from, VkDeviceSize from_offset, VkBuffer to,
                       VkDeviceSize to_offset, VkDeviceSize bytes) {
    if (!gpu_begin(g)) {
        return false;
    }
    VkBufferCopy region{from_offset, to_offset, bytes};
    vkCmdCopyBuffer(g.command, from, to, 1, &region);
    return gpu_submit(g);
}

bool gpu_upload(GpuContext& g, GpuBuffer& dst, VkDeviceSize offset, const void* data,
                VkDeviceSize bytes) {
    const uint8_t* p = (const uint8_t*)data;
    for (VkDeviceSize done = 0; done < bytes;) {
        VkDeviceSize n = bytes - done < g.staging.size ? bytes - done : g.staging.size;
        std::memcpy(g.staging.mapped, p + done, n);
        if (!copy_chunk(g, g.staging.buffer, 0, dst.buffer, offset + done, n)) {
            return false;
        }
        done += n;
    }
    return true;
}

bool gpu_download(GpuContext& g, GpuBuffer& src, VkDeviceSize offset, void* data,
                  VkDeviceSize bytes) {
    uint8_t* p = (uint8_t*)data;
    for (VkDeviceSize done = 0; done < bytes;) {
        VkDeviceSize n = bytes - done < g.staging.size ? bytes - done : g.staging.size;
        if (!copy_chunk(g, src.buffer, offset + done, g.staging.buffer, 0, n)) {
            return false;
        }
        std::memcpy(p + done, g.staging.mapped, n);
        done += n;
    }
    return true;
}
