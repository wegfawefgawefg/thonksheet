#include "imgui_layer.hpp"

#include "renderer/vulkan_internal.hpp"

#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <imgui.h>

static bool windowed = false;

// dark panels on the same near-black as the views, square corners
static void set_style() {
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 0.0f;
    s.FrameRounding = 2.0f;
    s.WindowBorderSize = 0.0f;
    s.WindowPadding = ImVec2(8, 6);
    s.ItemSpacing = ImVec2(6, 4);
    s.Colors[ImGuiCol_WindowBg] = ImVec4(0.055f, 0.063f, 0.086f, 1.0f);
    s.Colors[ImGuiCol_ChildBg] = ImVec4(0.043f, 0.051f, 0.071f, 1.0f);
    s.Colors[ImGuiCol_FrameBg] = ImVec4(0.11f, 0.12f, 0.16f, 1.0f);
    s.Colors[ImGuiCol_Header] = ImVec4(0.16f, 0.18f, 0.23f, 1.0f);
    s.Colors[ImGuiCol_Button] = ImVec4(0.16f, 0.18f, 0.23f, 1.0f);
    s.Colors[ImGuiCol_ButtonActive] = ImVec4(0.95f, 0.72f, 0.29f, 0.8f);
    s.Colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.72f, 0.29f, 1.0f);
    s.Colors[ImGuiCol_SliderGrab] = ImVec4(0.95f, 0.72f, 0.29f, 0.8f);
    s.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.95f, 0.72f, 0.29f, 1.0f);
    s.Colors[ImGuiCol_PlotLines] = ImVec4(0.95f, 0.72f, 0.29f, 1.0f);
}

bool init_imgui_layer(SDL_Window* window, Renderer& renderer) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = nullptr; // no imgui.ini wherever it was started from
    set_style();
    windowed = window != nullptr;
    if (windowed && !ImGui_ImplSDL3_InitForVulkan(window)) {
        return false;
    }
    return vk::imgui_vulkan_init(vk::ctx(renderer));
}

void shutdown_imgui_layer() {
    vk::imgui_vulkan_shutdown();
    if (windowed) {
        ImGui_ImplSDL3_Shutdown();
    }
    ImGui::DestroyContext();
}

void imgui_new_frame(float headless_w, float headless_h) {
    ImGui_ImplVulkan_NewFrame();
    if (windowed) {
        ImGui_ImplSDL3_NewFrame();
    } else {
        ImGuiIO& io = ImGui::GetIO();
        io.DisplaySize = ImVec2(headless_w, headless_h);
        io.DeltaTime = 1.0f / 60.0f;
    }
    ImGui::NewFrame();
}

void imgui_process_event(const SDL_Event& event) {
    ImGui_ImplSDL3_ProcessEvent(&event);
}

bool imgui_want_capture_mouse() {
    return ImGui::GetIO().WantCaptureMouse;
}

bool imgui_want_capture_keyboard() {
    return ImGui::GetIO().WantCaptureKeyboard;
}
