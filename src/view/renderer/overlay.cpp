// The sheet's per-cell colours (overlay.hpp): the readability tint by tile, the buttons'
// cells in their colours, and a picked cell's evidence over the cells it could hear.

#include "renderer/overlay.hpp"

#include "button_colors.hpp"
#include "layout.hpp"
#include "view_frame.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

static uint32_t pack(const float* rgb, int how) {
    const auto byte = [](float c) {
        return static_cast<uint32_t>(std::clamp(c, 0.0f, 1.0f) * 255.0f + 0.5f);
    };
    return byte(rgb[0]) | byte(rgb[1]) << 8 | byte(rgb[2]) << 16 |
           static_cast<uint32_t>(how & 255) << 24;
}

// log scale: 0.3 (lost) is about 0.1, 1 (easy) 0.3, 3 about 0.6, 8 and up 1
float readable_shade(float value) {
    return std::clamp(std::log2(1.0f + std::max(0.0f, value)) / std::log2(9.0f), 0.0f, 1.0f);
}

// dark violet through teal to pale yellow
void readable_rgb(float shade, float* rgb) {
    static const float stops[3][3] = {
        {0.25f, 0.12f, 0.50f}, {0.10f, 0.70f, 0.70f}, {1.00f, 0.95f, 0.45f}};
    const float s = std::clamp(shade, 0.0f, 1.0f) * 2.0f;
    const int i = std::min(1, static_cast<int>(s));
    const float t = s - static_cast<float>(i);
    for (int c = 0; c < 3; ++c) {
        rgb[c] = stops[i][c] + (stops[i + 1][c] - stops[i][c]) * t;
    }
}

static void tint_tiles(std::vector<uint32_t>& cells, const ViewFrame& f, const Readability& r) {
    const int cw = f.tiles_w * 8;
    for (int t = 0; t < f.tile_count && t < static_cast<int>(r.tile.size()); ++t) {
        const float shade = readable_shade(r.tile[t]);
        float rgb[3];
        readable_rgb(shade, rgb);
        const uint32_t word = pack(rgb, static_cast<int>(18.0f + 66.0f * shade));
        const int x0 = (t % f.tiles_w) * 8, y0 = (t / f.tiles_w) * 8;
        for (int y = y0; y < y0 + 8; ++y) {
            for (int x = x0; x < x0 + 8; ++x) {
                cells[static_cast<size_t>(y) * cw + x] = word;
            }
        }
    }
}

static void button_cells(std::vector<uint32_t>& cells, const ViewFrame& f) {
    const int cw = f.tiles_w * 8;
    for (const ReadoutCell& c : f.readout.cells) {
        float rgb[3];
        button_rgb(c.button, rgb);
        cells[static_cast<size_t>(c.y) * cw + c.x] = pack(rgb, 128);
    }
}

// green for evidence for a source, red against; how strong against the cell's own largest
// value, but never against less than a quarter of the counter's range, so a young cell's
// few small steps look small
static void evidence_cells(std::vector<uint32_t>& cells, const ViewFrame& f) {
    const Selected& s = f.selected;
    const int cw = f.tiles_w * 8;
    int most = std::max(1, s.keep_top / 4);
    for (const EvidenceCell& e : s.evidence) {
        most = std::max(most, std::abs(static_cast<int>(e.value)));
    }
    const float top = static_cast<float>(most);
    static const float FOR[3] = {0.30f, 0.98f, 0.48f};
    static const float AGAINST[3] = {1.00f, 0.28f, 0.28f};
    for (const EvidenceCell& e : s.evidence) {
        if (e.value == 0) {
            continue;
        }
        const float strength = std::min(1.0f, std::abs(e.value) / top);
        cells[static_cast<size_t>(e.y) * cw + e.x] =
            pack(e.value > 0 ? FOR : AGAINST, static_cast<int>(30.0f + 85.0f * strength));
    }
}

bool build_overlay(std::vector<uint32_t>& cells, const ViewFrame& f, const ViewSettings& v) {
    const bool tint = v.tint >= 0 && v.tint < READ_FEATURES && !f.readable[v.tint].tile.empty();
    const bool buttons = v.buttons && f.task.on && !f.readout.cells.empty();
    const bool evidence = v.evidence && f.selected.neuron >= 0 && !f.selected.evidence.empty();
    if (!tint && !buttons && !evidence) {
        return false;
    }
    cells.assign(static_cast<size_t>(f.tiles_w) * 8 * f.tiles_h * 8, 0u);
    if (tint) {
        tint_tiles(cells, f, f.readable[v.tint]);
    }
    if (buttons) {
        button_cells(cells, f);
    }
    if (evidence) {
        evidence_cells(cells, f);
    }
    return true;
}
