#include "layout.hpp"

#include <algorithm>
#include <cmath>

constexpr float PAD = 6.0f;
constexpr float SIDE_W = 440.0f;

Layout layout_for(float w, float h) {
    Layout l;
    const float left_w = std::max(300.0f, w - SIDE_W);
    l.side = Rect{left_w, 0, w - left_w, h};

    // the raster along the bottom of the left part
    const float raster_h = std::round(h * 0.28f);
    l.raster = Rect{PAD, h - raster_h + l.label, left_w - 2 * PAD, raster_h - l.label - PAD};

    // the sheet: the biggest square in the top left
    const float top_h = h - raster_h;
    const float sheet = std::floor(std::min(top_h - l.label - PAD, left_w * 0.56f));
    l.sheet = Rect{PAD, PAD + l.label, sheet, sheet};

    // the middle column: the recurrence square, then the lines
    const float mid_x = l.sheet.x + sheet + 2 * PAD;
    const float mid_w = left_w - mid_x - PAD;
    const float recur = std::floor(std::min(mid_w, top_h * 0.45f));
    l.recur = Rect{mid_x, PAD + l.label, recur, recur};
    const float lines_y = l.recur.y + recur + PAD;
    l.lines = Rect{mid_x - PAD, lines_y, mid_w + 2 * PAD, top_h - lines_y - PAD};
    return l;
}

bool rect_contains(const Rect& r, float x, float y) {
    return x >= r.x && y >= r.y && x < r.x + r.w && y < r.y + r.h;
}

SheetPlace sheet_place(const Rect& r, int cells_w, int cells_h, const ViewSettings& v) {
    SheetPlace p;
    float fit = std::min(r.w / std::max(1, cells_w), r.h / std::max(1, cells_h));
    if (fit >= 1.0f) {
        fit = std::floor(fit); // whole pixels per cell keeps every cell the same size
    }
    p.cell_px = fit * v.zoom;
    const float centre_x = cells_w * 0.5f + v.pan_x;
    const float centre_y = cells_h * 0.5f + v.pan_y;
    p.origin_x = std::round(r.x + r.w * 0.5f - centre_x * p.cell_px);
    p.origin_y = std::round(r.y + r.h * 0.5f - centre_y * p.cell_px);
    return p;
}

int sheet_pick(const SheetPlace& p, int tiles_w, int tiles_h, float x, float y) {
    const int cx = static_cast<int>(std::floor((x - p.origin_x) / p.cell_px));
    const int cy = static_cast<int>(std::floor((y - p.origin_y) / p.cell_px));
    if (cx < 0 || cy < 0 || cx >= tiles_w * 8 || cy >= tiles_h * 8) {
        return -1;
    }
    return ((cy / 8) * tiles_w + cx / 8) * 64 + (cy % 8) * 8 + cx % 8;
}

int raster_total_rows(int tile_count, const ViewSettings& v) {
    return v.raster_rows == 0 ? tile_count * 64 : tile_count;
}

void raster_rows(const Rect& r, int tile_count, const ViewSettings& v, float* first,
                 float* per_px) {
    const float total = static_cast<float>(raster_total_rows(tile_count, v));
    const float count = v.raster_count > 0.0f ? std::min(v.raster_count, total) : total;
    *first = std::clamp(v.raster_first, 0.0f, std::max(0.0f, total - count));
    *per_px = count / std::max(1.0f, r.h);
}
