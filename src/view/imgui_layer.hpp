#pragma once
// Dear ImGui, with the same calls as Gubsy's src/imgui_layer.hpp (Mans In My House uses
// those). Gubsy's own layer draws through an SDL_Renderer; this one draws through the
// viewer's Vulkan renderer. Without a window (captures) the display size is set by hand.

#include <SDL3/SDL.h>

struct Renderer;

// window may be null (headless): then pass the size
bool init_imgui_layer(SDL_Window* window, Renderer& renderer);
void shutdown_imgui_layer();
// headless_w/h: the display size when there is no window; ignored with one
void imgui_new_frame(float headless_w = 0, float headless_h = 0);
void imgui_process_event(const SDL_Event& event);
bool imgui_want_capture_mouse();
bool imgui_want_capture_keyboard();
// (the draw data is recorded by renderer_frame)
