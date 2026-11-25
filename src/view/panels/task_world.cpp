// The task panel's view of the world, for any task: the bitmap as the world sees it, each
// number with its lit lines, and the strip of the last steps (which on/off lines were on,
// what was wanted, what was done, good or bad). Everything comes from the plug and the
// task's TaskSense, so a new task kind shows up here with nothing added.

#include "button_colors.hpp"
#include "panels/palette.hpp"
#include "state.hpp"

#include <algorithm>
#include <cstdio>
#include <imgui.h>

static ImU32 button_color(int b, float alpha = 1.0f) {
    if (b < 0) {
        return IM_COL32(52, 56, 66, 255); // held still
    }
    float rgb[3];
    button_rgb(b, rgb);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(rgb[0], rgb[1], rgb[2], alpha));
}

static const TaskTry& try_at(const TaskView& t, int64_t age) {
    return t.tries[(t.tries_total - 1 - age) % TASK_TRIES];
}

float world_picture_width(const TaskView& t, float height) {
    if (t.sheet_w * t.sheet_h > 0) {
        const float px = std::clamp((height - 18.0f) / std::max(1, t.sheet_h), 3.0f, 14.0f);
        return std::max(70.0f, px * t.sheet_w + 4.0f);
    }
    return t.numbers > 0 ? 150.0f : 0.0f;
}

// the bitmap, a square per pixel, lit where the world has it on; or the numbers, each a row
// of its lines (lit where on) and its place in its range
void world_picture(const TaskView& t, float width, float height) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 at = ImGui::GetCursorScreenPos();
    if (t.sheet_w * t.sheet_h > 0) {
        ImGui::TextColored(DIM, "bitmap %dx%d", t.sheet_w, t.sheet_h);
        const float top = ImGui::GetCursorScreenPos().y;
        const float px = std::clamp((height - 18.0f) / std::max(1, t.sheet_h), 3.0f, 14.0f);
        dl->AddRectFilled(ImVec2(at.x, top),
                          ImVec2(at.x + px * t.sheet_w + 2, top + px * t.sheet_h + 2),
                          IM_COL32(22, 25, 33, 255));
        for (int y = 0; y < t.sheet_h; ++y) {
            for (int x = 0; x < t.sheet_w; ++x) {
                const int i = y * t.sheet_w + x;
                if (i < 256 && t.sheet[i]) {
                    const ImVec2 p(at.x + 1 + x * px, top + 1 + y * px);
                    dl->AddRectFilled(p, ImVec2(p.x + px - 1, p.y + px - 1),
                                      IM_COL32(235, 240, 245, 255));
                }
            }
        }
        ImGui::Dummy(ImVec2(width, px * t.sheet_h + 4));
        return;
    }
    for (int i = 0; i < t.numbers; ++i) {
        const long long span = static_cast<long long>(t.number_hi[i]) - t.number_lo[i];
        const float place =
            span != 0 ? static_cast<float>(t.number[i] - t.number_lo[i]) / static_cast<float>(span)
                      : 0.0f;
        ImGui::TextColored(DIM, "%s", t.number_names[i]);
        ImGui::SameLine();
        ImGui::Text("%.2f", place);
        // its lines, lit where on, and a tick at its place in [lo, hi]
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const int lines = std::max(1, std::min(t.number_lines, 32));
        const float cell = (width - 4) / lines;
        for (int j = 0; j < lines; ++j) {
            const bool on = (t.number_on[i] >> j) & 1;
            dl->AddRectFilled(ImVec2(p.x + j * cell, p.y),
                              ImVec2(p.x + (j + 1) * cell - 1, p.y + 8),
                              on ? IM_COL32(235, 240, 245, 255) : IM_COL32(40, 44, 54, 255));
        }
        const float x = p.x + std::clamp(place, 0.0f, 1.0f) * (width - 4);
        dl->AddLine(ImVec2(x, p.y - 2), ImVec2(x, p.y + 11), ImGui::GetColorU32(AMBER), 2.0f);
        ImGui::Dummy(ImVec2(width, 12));
    }
}

// the last steps, newest at the right: a row per on/off line (lit while it was on), then
// what was wanted, what was done, and a bar for good or bad
void line_strip(const TaskView& t, float height) {
    const int rows = std::min(t.singles, 16);
    const float name_w = 70.0f;
    const float w = ImGui::GetContentRegionAvail().x - name_w;
    const float cell = 9.0f;
    const float row_h = std::clamp((height - 2 * cell - 6) / std::max(1, rows), 3.0f, 9.0f);
    const int count = static_cast<int>(std::min<int64_t>(t.tries_total, (int)(w / cell)));
    const ImVec2 at = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 dim = ImGui::GetColorU32(DIM);
    const float x_end = at.x + name_w + w;
    // the names down the left, where they fit
    for (int r = 0; r < rows && row_h >= 8.0f; ++r) {
        dl->AddText(nullptr, 11.0f, ImVec2(at.x, at.y + r * row_h - 1), dim, t.single_names[r]);
    }
    const float base = at.y + rows * row_h + 2;
    dl->AddText(nullptr, 11.0f, ImVec2(at.x, base - 1), dim, "wanted");
    dl->AddText(nullptr, 11.0f, ImVec2(at.x, base + cell - 1), dim, "did");
    for (int i = 0; i < count; ++i) {
        const TaskTry& tr = try_at(t, i);
        const float x = x_end - (i + 1) * cell;
        for (int r = 0; r < rows; ++r) {
            const bool on = (tr.lines >> r) & 1;
            dl->AddRectFilled(ImVec2(x, at.y + r * row_h),
                              ImVec2(x + cell - 2, at.y + (r + 1) * row_h - 1),
                              on ? IM_COL32(230, 234, 240, 255) : IM_COL32(30, 33, 42, 255));
        }
        dl->AddRectFilled(ImVec2(x, base), ImVec2(x + cell - 2, base + cell - 1),
                          button_color(tr.right));
        dl->AddRectFilled(ImVec2(x, base + cell), ImVec2(x + cell - 2, base + 2 * cell - 1),
                          button_color(tr.move));
        const ImU32 bar = tr.valence > 0   ? ImGui::GetColorU32(GREEN)
                          : tr.valence < 0 ? ImGui::GetColorU32(ALARM)
                                           : IM_COL32(52, 56, 66, 255);
        dl->AddRectFilled(ImVec2(x, base + 2 * cell + 1), ImVec2(x + cell - 2, base + 2 * cell + 4),
                          bar);
    }
    ImGui::Dummy(ImVec2(name_w + w, base - at.y + 2 * cell + 5));
}
