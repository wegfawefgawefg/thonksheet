// The renderer's face: up, down, and a new size. ImGui's part is brought up after it and
// taken down before it (imgui_layer.cpp).

#include "renderer/vulkan_internal.hpp"

#include <cstdio>

static bool make_screen(Renderer& r, int w, int h) {
    vk::Context& g = vk::ctx(r);
    r.w = w;
    r.h = h;
    return vk::image(g, g.screen, w, h, vk::screen_format,
                     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
}

bool renderer_create(Renderer& r, SDL_Window* window, int w, int h, bool validation) {
    r.window = window;
    if (!vk::init(r, validation)) {
        return false;
    }
    vk::Context& g = vk::ctx(r);
    return make_screen(r, w, h) && vk::init_pipelines(g, BLOBVIEW_SHADER_DIR);
}

void renderer_destroy(Renderer& r) {
    if (r.gpu == nullptr) {
        return;
    }
    vk::Context& g = vk::ctx(r);
    if (g.device != VK_NULL_HANDLE) {
        if (!r.lost) {
            vkDeviceWaitIdle(g.device);
        }
        vk::shutdown_pipelines(g);
    }
    vk::shutdown(r);
}

bool renderer_resize(Renderer& r, int w, int h) {
    if (w == r.w && h == r.h) {
        return true;
    }
    vk::Context& g = vk::ctx(r);
    g.swap_stale = true;
    return vk::wait(g) && make_screen(r, w, h);
}

void renderer_idle(Renderer& r) {
    vk::Context& g = vk::ctx(r);
    if (!r.lost && g.device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(g.device);
    }
}
