#pragma once
// The viewer's renderer, laid out like Mans In My House's src/renderer: device.hpp is the
// face, vulkan_internal.hpp the Vulkan state, vulkan_*.cpp the parts. One device draws the
// views and the ImGui panels into its own screen image; with a window that image is blitted
// onto it, without one (captures) it is read back. Nothing here ever needs a display.

#include <cstdint>
#include <string>
#include <vector>

struct SDL_Window;
struct ViewFrame;
struct Layout;
struct ViewSettings;

struct Renderer {
    SDL_Window* window = nullptr; // null: headless
    void* gpu = nullptr;          // the vk::Context
    int w = 0, h = 0;             // the screen image
    bool lost = false;            // the device was lost or hung: stop drawing and leave
    std::string gpu_name;
};

bool renderer_create(Renderer& r, SDL_Window* window, int w, int h, bool validation);
void renderer_destroy(Renderer& r);
// wait for everything submitted (before taking ImGui down)
void renderer_idle(Renderer& r);
// a new screen size (the window's pixels); the swapchain follows on the next frame
bool renderer_resize(Renderer& r, int w, int h);
// one frame: the views (unless `views` is false) and ImGui's draw data into the screen,
// then onto the window if there is one. False when the device is lost.
bool renderer_frame(Renderer& r, const ViewFrame& f, const Layout& l, const ViewSettings& v,
                    bool views);
// the screen's pixels as RGBA rows, after everything drawn so far
bool renderer_read(Renderer& r, std::vector<std::uint8_t>& rgba);
