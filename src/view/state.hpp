#pragma once
// Everything the viewer is doing right now, in one plain struct. The blob itself lives on
// the sim thread (sim.hpp); what is drawn is `frame`, its latest snapshot. The renderer and
// the window are locals in main, as in Mans In My House.

#include "layout.hpp"
#include "sim.hpp"
#include "view_frame.hpp"

#include <string>

union SDL_Event;

// the command line (cli.cpp); also drives headless captures
struct CliOptions {
    bool fake = true;
    FakeScene scene = FAKE_MIXED;
    int tiles_w = 0, tiles_h = 0; // 0: the genome's
    uint64_t seed = 1;
    // capture: no window; run `until` ticks, draw one frame, save a png, quit
    std::string capture;
    int64_t until = 2000;
    // a task through the world plug, with blobrun's flags (src/run/run_flags.cpp)
    bool task = false;
    bool twin = true;
    RunFlags run;
    int width = 1600, height = 1000;
    int select = -1; // neuron picked at the start
    bool validation = false;
    int frames = 0; // window: quit after this many frames; 0 runs until closed
    int mode = RUN_FULL;
    int slow_tps = 20;
    bool input = false;
    int raster_rows = -1; // -1: per neuron up to 16,384 neurons, per tile above
    bool fullscreen = false;
};

// rates for the controls panel, measured twice a second
struct Clock {
    double last_time = 0;
    uint64_t last_ticks = 0;
    int frames = 0;
    float ticks_per_second = 0;
    float fps = 0;
};

// right-button drags on the sheet and the raster
struct Drag {
    bool sheet = false, raster = false;
    float x = 0, y = 0;
};

struct State {
    CliOptions options;
    Sim sim;
    ViewFrame frame;
    ViewSettings view;
    Layout layout;
    Clock clock;
    Drag drag;
    int mode = RUN_FULL; // the run mode as the viewer last set it; the sim reads sim.shared.mode
    int frames = 0;
    bool resized = false; // the window's pixel size changed
    bool running = true;
    bool learning = true; // genome.learning_on for the next restart (task panel)
};

// cli.cpp
bool parse_cli(CliOptions& o, int argc, char** argv);
SimSettings sim_settings(const CliOptions& o);
void set_mode(State& s, int mode);

// inputs.cpp: keys, clicks, wheel, drags
void handle_event(State& s, const SDL_Event& e);

// capture.cpp
struct Renderer;
bool save_capture(Renderer& renderer, const std::string& path);

// panels/: every ImGui window, and the views' titles
void draw_panels(State& s);
void lines_panel(State& s);
void neuron_section(State& s);
void sectors_section(State& s);
void histograms_section(State& s);
void task_panel(State& s);
void task_labels(State& s);
