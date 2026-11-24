// blobview: watch a blob tick. The sim runs flat out on its own thread; this loop samples it
// and draws. Keys and flags: docs/notes-viewer.md and `blobview --help`.
//
//   blobview                                the window (never run it on a display you are using)
//   blobview --capture out.png --ticks 3000 no window at all: tick, draw one frame, save it

#include "imgui_layer.hpp"
#include "renderer/device.hpp"
#include "state.hpp"

#include <SDL3/SDL.h>
#include <cstdio>

// ticks per second and frames per second, twice a second
static void follow_clock(State& s) {
    Clock& c = s.clock;
    const double now = static_cast<double>(SDL_GetTicksNS()) / 1e9;
    const uint64_t ticks = s.sim.shared.ticks.load();
    ++c.frames;
    if (c.last_time == 0) {
        c.last_time = now;
        c.last_ticks = ticks;
    } else if (now - c.last_time >= 0.5) {
        c.ticks_per_second = static_cast<float>((ticks - c.last_ticks) / (now - c.last_time));
        c.fps = static_cast<float>(c.frames / (now - c.last_time));
        c.last_time = now;
        c.last_ticks = ticks;
        c.frames = 0;
    }
}

static int fail(const char* why) {
    std::fprintf(stderr, "blobview: %s\n", why);
    return 1;
}

int main(int argc, char** argv) {
    // init state: the options, the blob, what to show first
    State state;
    if (!parse_cli(state.options, argc, argv)) {
        return 1;
    }
    const CliOptions& o = state.options;
    const bool capturing = !o.capture.empty();
    if (!sim_birth(state.sim, sim_settings(o))) {
        return 1;
    }
    const int neurons = sim_blob(state.sim).neuron_count;
    state.view.raster_rows = o.raster_rows >= 0 ? o.raster_rows : neurons > 16384 ? 1 : 0;
    state.sim.shared.selected = o.select;
    state.sim.shared.slow_tps = o.slow_tps;
    state.sim.shared.input_on = o.input;
    set_mode(state, o.mode);

    // init window, renderer and input. A capture opens no window and never touches a display.
    SDL_Window* window = nullptr;
    int w = o.width, h = o.height;
    if (!capturing) {
        if (!SDL_Init(SDL_INIT_VIDEO)) { // video only: no audio, no gamepads
            return fail(SDL_GetError());
        }
        const SDL_WindowFlags flags = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE |
                                      (o.fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WindowFlags{0});
        window = SDL_CreateWindow("thonksheet", w, h, flags);
        if (window == nullptr) {
            return fail(SDL_GetError());
        }
        SDL_SetWindowMinimumSize(window, 1100, 700);
        SDL_GetWindowSizeInPixels(window, &w, &h);
    }
    Renderer renderer;
    if (!renderer_create(renderer, window, w, h, o.validation) ||
        !init_imgui_layer(window, renderer)) {
        return fail("no renderer");
    }
    state.layout = layout_for(static_cast<float>(w), static_cast<float>(h));

    // a capture's ticks run here, in order; otherwise the sim thread starts ticking
    if (capturing) {
        for (int i = 0; i < o.ticks; ++i) {
            sim_tick(state.sim);
        }
        sim_fill(state.sim, state.frame, o.select);
        frame_derive(state.frame);
    } else {
        sim_start(state.sim);
    }

    while (state.running) {
        // pump input
        SDL_Event e;
        while (window != nullptr && SDL_PollEvent(&e)) {
            imgui_process_event(e);
            handle_event(state, e);
        }
        if (state.resized) {
            SDL_GetWindowSizeInPixels(window, &w, &h);
            renderer_resize(renderer, w, h);
            state.layout = layout_for(static_cast<float>(w), static_cast<float>(h));
            state.resized = false;
        }

        // the sim's latest snapshot (none while it runs flat out undrawn), and what the
        // viewer works out from it
        if (!capturing && state.mode != RUN_TURBO && sim_take(state.sim, state.frame)) {
            frame_derive(state.frame);
        }

        // the panels
        imgui_new_frame(static_cast<float>(w), static_cast<float>(h));
        draw_panels(state);

        // draw and present; a lost or hung device ends the loop
        if (!renderer_frame(renderer, state.frame, state.layout, state.view,
                            state.mode != RUN_TURBO)) {
            std::fprintf(stderr, "blobview: drawing failed, leaving\n");
            break;
        }
        ++state.frames;

        // a capture is saved on its second frame (ImGui sizes its windows on the first)
        if (capturing && state.frames == 2) {
            save_capture(renderer, o.capture);
            state.running = false;
        }

        // rates; undrawn, the viewer idles so the sim has the machine
        follow_clock(state);
        if (state.mode == RUN_TURBO) {
            SDL_Delay(100);
        }
        if (o.frames > 0 && state.frames >= o.frames) {
            state.running = false;
        }
    }

    // shutdown
    sim_stop(state.sim);
    renderer_idle(renderer);
    shutdown_imgui_layer();
    renderer_destroy(renderer);
    sim_free(state.sim);
    if (window != nullptr) {
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    return renderer.lost ? 1 : 0;
}
