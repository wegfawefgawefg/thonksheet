#pragma once
// Where everything goes on the window, and what the views are set to show. The sheet,
// raster and recurrence are drawn by our own pipelines into their rects; the ImGui panels
// fill the rest and never cover them.
//
//   +-----------+------------+---------+
//   |           | recurrence | controls|
//   |  sheet    +------------+ neuron  |
//   |           | lines      | sectors |
//   +-----------+------------+ histos  |
//   | raster          | task |         |
//   +------------------------+---------+
//
// The task panel is there only with --task; otherwise the raster takes the whole width.

struct Rect {
    float x = 0, y = 0, w = 0, h = 0;
};

struct Layout {
    Rect sheet, recur, raster; // our pipelines
    Rect lines, side, task;    // ImGui windows
    float label = 18;          // room for a view's title above it
};

struct ViewSettings {
    bool tile_lines = true;
    bool sector_lines = true;
    bool sources = true;
    bool targets = true;
    int fade = 6;        // ticks a spike takes to fade on the sheet
    int raster_rows = 0; // 0 a row per neuron, 1 a row per tile
    float zoom = 1.0f;   // sheet zoom; 1 fits the whole sheet
    float pan_x = 0.0f;  // sheet centre offset, in cells
    float pan_y = 0.0f;
    float raster_first = 0.0f; // first raster row shown
    float raster_count = 0.0f; // rows shown; 0 shows them all
    int recur_ticks = 256;     // ticks across the recurrence square
    bool lines_long = false;   // lines over the long history instead of the tape
    bool task_marks = true;    // the task's lines and their names on the sheet
    bool buttons = true;       // each button's cells in its own colour
    bool evidence = true;      // a picked cell's evidence over the sheet
    int tint = -1;             // the feature whose readability tints the tiles, -1 none
};

// where cell (0, 0) lands and how many pixels a cell is
struct SheetPlace {
    float origin_x = 0, origin_y = 0, cell_px = 1;
};

Layout layout_for(float w, float h, bool task);
bool rect_contains(const Rect& r, float x, float y);
SheetPlace sheet_place(const Rect& r, int cells_w, int cells_h, const ViewSettings& v);
// the neuron under a pixel, or -1
int sheet_pick(const SheetPlace& p, int tiles_w, int tiles_h, float x, float y);

// raster rows: how many there are, and the first shown and rows per pixel
int raster_total_rows(int tile_count, const ViewSettings& v);
void raster_rows(const Rect& r, int tile_count, const ViewSettings& v, float* first, float* per_px);
