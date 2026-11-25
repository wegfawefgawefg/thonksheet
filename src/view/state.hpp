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
    MinimalGenome minimal; // --engine minimal's numbers (--m.NAME)
    TableGenome table;     // --table's numbers (--t.NAME)
    ProbeSettings probe;   // --probe and the readability measurement
    int measure = -1;      // capture: a feature measured at the end
    int width = 1600, height = 1000;
    int select = -1;        // neuron picked at the start
    int select_button = -1; // or a cell of this button
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
    bool learning = true;  // genome.learning_on for the next restart (task panel)
    int readout_pick = -1; // the readout cell last picked with V, an index into its cells
};

// cli.cpp
bool parse_cli(CliOptions& o, int argc, char** argv);
SimSettings sim_settings(const CliOptions& o);
void set_mode(State& s, int mode);

// inputs.cpp: keys, clicks, wheel, drags
void handle_event(State& s, const SDL_Event& e);
// pick a cell of the readout: the next one after the last picked (step 1), or a given one
void pick_readout_cell(State& s, int index);
// the readability tint: the next feature (off after the last), measured if it has not been
void cycle_tint(State& s);
void measure_now(State& s, int feature);

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
void readout_section(State& s);
void readability_section(State& s);
void readability_labels(State& s); // each tile's number on the sheet, when tiles are big
// panels/task_world.cpp: the task panel's world picture and line strip
float world_picture_width(const TaskView& t, float height);
void world_picture(const TaskView& t, float width, float height);
void line_strip(const TaskView& t, float height);
