// The readout (the buttons' cells, where learning happens with the working rule) and the
// readability probe, as sections of the side window.
//
// readout: per button its colour, cells, spikes this step and over the last answered step,
// whether it won, and what the last change did (cells that gained and lost evidence). A map
// of the buttons' cells, green where the last change raised a cell's evidence and red where
// it lowered it; a click on it picks that cell. The picked cell's evidence in numbers; the
// picture of it is on the sheet.
//
// readability: per feature, how readable it is from each tile (the probe's number) by
// distance from the cue tile, and which one tints the sheet.

#include "button_colors.hpp"
#include "panels/palette.hpp"
#include "renderer/overlay.hpp"
#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>

static ImVec4 button_color(int b) {
    float rgb[3];
    button_rgb(b, rgb);
    return ImVec4(rgb[0], rgb[1], rgb[2], 1.0f);
}

static const char* keep_words(int keep) {
    return keep == KEEP_COUNTERS    ? "evidence counters per source"
           : keep == KEEP_STRENGTHS ? "connection strengths per source (no counters)"
                                    : "nothing per source";
}

// one row per button: swatch and name, spikes now and last step, won, the last change
static void button_rows(const ViewFrame& f) {
    const ReadoutView& r = f.readout;
    const TaskView& t = f.task;
    int most = 1;
    for (const ReadoutButton& b : r.buttons) {
        most = std::max({most, b.spikes, b.last_spikes});
    }
    for (int i = 0; i < static_cast<int>(r.buttons.size()); ++i) {
        const ReadoutButton& b = r.buttons[i];
        ImGui::PushID(i);
        ImGui::ColorButton("##c", button_color(i), ImGuiColorEditFlags_NoTooltip, ImVec2(12, 12));
        ImGui::SameLine();
        ImGui::TextColored(button_color(i), "%-12s", i < t.motors ? t.motor_names[i] : "button");
        ImGui::SameLine();
        ImGui::Text("%3d %3d", b.spikes, b.last_spikes);
        ImGui::SameLine();
        // the last step's spikes as a bar, so the winner shows at a glance
        const ImVec2 at = ImGui::GetCursorScreenPos();
        const float w = 50.0f * b.last_spikes / most;
        ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(at.x, at.y + 3),
                                                  ImVec2(at.x + w, at.y + 11),
                                                  ImGui::GetColorU32(button_color(i)));
        ImGui::Dummy(ImVec2(52, 12));
        ImGui::SameLine();
        ImGui::TextColored(b.won ? AMBER : DIM, b.won ? "won" : "   ");
        if (r.change_step >= 0 && r.keep != KEEP_NONE) {
            ImGui::SameLine();
            ImGui::TextColored(GREEN, "%3d up", b.gained);
            ImGui::SameLine();
            ImGui::TextColored(ALARM, "%3d down", b.lost);
        }
        ImGui::PopID();
    }
}

// the buttons' cells where they sit: each in its button's colour, with the last change on it
// (green raised, red lowered). a click picks the nearest cell
static void cell_map(State& s) {
    const ReadoutView& r = s.frame.readout;
    if (r.cells.empty()) {
        return;
    }
    int x0 = 1 << 20, y0 = 1 << 20, x1 = 0, y1 = 0;
    for (const ReadoutCell& c : r.cells) {
        x0 = std::min<int>(x0, c.x);
        y0 = std::min<int>(y0, c.y);
        x1 = std::max<int>(x1, c.x);
        y1 = std::max<int>(y1, c.y);
    }
    const float avail = ImGui::GetContentRegionAvail().x;
    const float px =
        std::clamp(std::min(avail / (x1 - x0 + 1), 110.0f / (y1 - y0 + 1)), 2.0f, 16.0f);
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const ImVec2 size((x1 - x0 + 1) * px, (y1 - y0 + 1) * px);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(at, ImVec2(at.x + size.x, at.y + size.y), IM_COL32(16, 18, 24, 255));
    for (const ReadoutCell& c : r.cells) {
        const ImVec2 p(at.x + (c.x - x0) * px, at.y + (c.y - y0) * px);
        const float in = std::max(1.0f, px - 1.0f);
        dl->AddRectFilled(
            p, ImVec2(p.x + in, p.y + in),
            ImGui::GetColorU32(ImVec4(button_color(c.button).x, button_color(c.button).y,
                                      button_color(c.button).z, 0.55f)));
        if (c.up != c.down && px >= 4.0f) {
            const ImVec4 mark = c.up > c.down ? GREEN : ALARM;
            const float m = in * 0.3f;
            dl->AddRectFilled(ImVec2(p.x + m, p.y + m), ImVec2(p.x + in - m, p.y + in - m),
                              ImGui::GetColorU32(mark));
        }
        if (c.x == s.frame.selected.x && c.y == s.frame.selected.y) {
            dl->AddRect(ImVec2(p.x - 1, p.y - 1), ImVec2(p.x + in + 1, p.y + in + 1),
                        IM_COL32(255, 255, 255, 255), 0.0f, 0, 1.5f);
        }
    }
    ImGui::InvisibleButton("##cells", ImVec2(std::max(1.0f, size.x), std::max(1.0f, size.y)));
    if (ImGui::IsItemClicked()) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        float best = 1e9f;
        int pick = -1;
        for (int i = 0; i < static_cast<int>(r.cells.size()); ++i) {
            const float dx = at.x + (r.cells[i].x - x0 + 0.5f) * px - m.x;
            const float dy = at.y + (r.cells[i].y - y0 + 0.5f) * px - m.y;
            if (dx * dx + dy * dy < best) {
                best = dx * dx + dy * dy;
                pick = i;
            }
        }
        pick_readout_cell(s, pick);
    }
}

// the picked cell's evidence in numbers
static void picked_cell(const ViewFrame& f) {
    const Selected& n = f.selected;
    if (n.neuron < 0 || n.keep == KEEP_NONE) {
        ImGui::TextColored(DIM, "pick a button's cell (V, or click the map or the sheet) to see "
                                "its evidence over the sheet");
        return;
    }
    int wired = 0, pro = 0, con = 0, up = 0, down = 0, best = 0, worst = 0;
    for (const EvidenceCell& e : n.evidence) {
        wired += e.wired;
        pro += e.value > 0 ? 1 : 0;
        con += e.value < 0 ? 1 : 0;
        best = std::max<int>(best, e.value);
        worst = std::min<int>(worst, e.value);
        up += e.moved > 0 ? e.moved : 0;
        down += e.moved < 0 ? -e.moved : 0;
    }
    if (n.button >= 0) {
        ImGui::TextColored(button_color(n.button), "cell %d,%d of %s", n.x, n.y,
                           n.button < f.task.motors ? f.task.motor_names[n.button] : "a button");
    } else {
        ImGui::Text("cell %d,%d (not a button's)", n.x, n.y);
    }
    ImGui::SameLine();
    if (n.keep_budget > 0) {
        ImGui::Text("wired %d of %d", wired, n.keep_budget);
    } else {
        ImGui::Text("wired %d", wired);
    }
    if (n.keep == KEEP_COUNTERS) {
        ImGui::TextColored(GREEN, "for %d (top %+d)", pro, best);
        ImGui::SameLine();
        ImGui::TextColored(ALARM, "against %d (low %+d)", con, worst);
        ImGui::SameLine();
        ImGui::TextColored(DIM, "range %+d..%+d", -n.keep_top, n.keep_top);
    }
    if (up + down > 0) {
        ImGui::Text("last change moved it: %d up, %d down", up, down);
    }
    ImGui::PushStyleColor(ImGuiCol_Text, DIM);
    ImGui::TextWrapped("on the sheet: green for, red against, by strength; white rings the "
                       "sources it has wired; dots the sources the last change moved (white "
                       "up, black down)");
    ImGui::PopStyleColor();
}

void readout_section(State& s) {
    const ViewFrame& f = s.frame;
    const ReadoutView& r = f.readout;
    if (!r.on ||
        !ImGui::CollapsingHeader("readout: the buttons' cells", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    const int cells = r.buttons.empty() ? 0 : r.buttons[0].cells;
    ImGui::TextColored(DIM, "%d cells a button, each keeps %s", cells, keep_words(r.keep));
    ImGui::TextColored(DIM, "spikes now, last step; last change: cells up, down");
    button_rows(f);
    if (r.keep == KEEP_NONE) {
        ImGui::TextColored(DIM, "no counters per source: no evidence to show");
    } else if (r.change_step < 0) {
        ImGui::TextColored(DIM, "no change yet (the rule moves on reward)");
    } else {
        const int p = r.change_pressed;
        ImGui::Text("last change: step %lld, %s, pressed", static_cast<long long>(r.change_step),
                    r.change_valence > 0 ? "good" : "bad");
        ImGui::SameLine();
        ImGui::TextColored(p >= 0 ? button_color(p) : DIM, "%s",
                           p < 0               ? "nothing"
                           : p < f.task.motors ? f.task.motor_names[p]
                                               : "?");
        ImGui::SameLine();
        ImGui::TextColored(DIM, "(%d changes)", r.changes);
    }
    cell_map(s);
    if (ImGui::Button("next cell (V)")) {
        pick_readout_cell(s, -1);
    }
    ImGui::SameLine();
    ImGui::Checkbox("colours", &s.view.buttons);
    ImGui::SameLine();
    ImGui::Checkbox("evidence (Q)", &s.view.evidence);
    picked_cell(f);
}

// the readability scale as a row of numbers, one per distance, each in its tint
static void distance_row(const Readability& r) {
    for (int d = 0; d < r.distances; ++d) {
        float rgb[3];
        readable_rgb(readable_shade(r.by_distance[d]), rgb);
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(rgb[0], rgb[1], rgb[2], 1.0f), "%5.2f", r.by_distance[d]);
    }
}

void readability_section(State& s) {
    const ViewFrame& f = s.frame;
    if (!f.real || !ImGui::CollapsingHeader("readability (the feature probe, no reward)",
                                            ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    ImGui::TextColored(DIM, "tile by tile, how well an ideal reader could tell the feature; "
                            "1 is easy, under 0.3 lost. by distance from the cue tile:");
    for (int i = 0; i < READ_FEATURES; ++i) {
        const Readability& r = f.readable[i];
        ImGui::PushID(i);
        if (ImGui::RadioButton(feature_names[i], s.view.tint == i)) {
            s.view.tint = s.view.tint == i ? -1 : i;
            if (s.view.tint == i && r.tile.empty() && f.reading != i) {
                measure_now(s, i);
            }
        }
        ImGui::SameLine(70);
        if (f.reading == i) {
            ImGui::TextColored(AMBER, "measuring %d of %d", r.tries, s.sim.settings.probe.tries);
        } else if (r.tile.empty()) {
            ImGui::TextColored(DIM, "not measured");
        } else {
            ImGui::TextColored(DIM, "%4d tries", r.tries);
        }
        distance_row(r);
        ImGui::PopID();
    }
    if (ImGui::Button("measure (X)")) {
        measure_now(s, s.view.tint >= 0 ? s.view.tint : 0);
    }
    ImGui::SameLine();
    if (ImGui::Button("tint off")) {
        s.view.tint = -1;
    }
    ImGui::SameLine();
    ImGui::TextColored(DIM, "C: next feature");
    ImGui::PushStyleColor(ImGuiCol_Text, DIM);
    ImGui::TextWrapped(s.sim.settings.probe.feature >= 0
                           ? "the probe is the world: it measures again and again, learning as "
                             "the genome says"
                           : "a measurement waits for the end of a task step, unplugs the task, "
                             "measures with learning off, and plugs it back");
    ImGui::PopStyleColor();
}

// each tile's number written on the sheet when tiles are big enough to hold it
void readability_labels(State& s) {
    const ViewFrame& f = s.frame;
    const int i = s.view.tint;
    if (i < 0 || i >= READ_FEATURES || f.readable[i].tile.empty() || s.mode == RUN_TURBO) {
        return;
    }
    const Rect& r = s.layout.sheet;
    const SheetPlace p = sheet_place(r, f.tiles_w * 8, f.tiles_h * 8, s.view);
    if (p.cell_px * 8.0f < 30.0f) {
        return;
    }
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->PushClipRect(ImVec2(r.x, r.y), ImVec2(r.x + r.w, r.y + r.h), true);
    const std::vector<float>& tile = f.readable[i].tile;
    for (int t = 0; t < static_cast<int>(tile.size()) && t < f.tile_count; ++t) {
        char text[16];
        std::snprintf(text, sizeof text, "%.1f", tile[t]);
        const ImVec2 at(p.origin_x + (t % f.tiles_w) * 8 * p.cell_px + 3,
                        p.origin_y + (t / f.tiles_w) * 8 * p.cell_px + 2);
        const ImVec2 size = ImGui::CalcTextSize(text);
        dl->AddRectFilled(ImVec2(at.x - 1, at.y), ImVec2(at.x + size.x + 1, at.y + size.y),
                          IM_COL32(11, 13, 18, 190));
        dl->AddText(at, IM_COL32(235, 238, 245, 255), text);
    }
    dl->PopClipRect();
}
