// The lines over time (their own window under the recurrence square) and the histograms
// (a section of the side window).

#include "panels/palette.hpp"
#include "state.hpp"

#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <imgui.h>

static const ImVec4 line_colors[LINE_COUNT] = {
    AMBER,                          // fired
    PINK,                           // surprise
    GREEN,                          // grown
    ImVec4(0.54f, 0.58f, 0.65f, 1), // pruned
    ImVec4(0.91f, 0.93f, 0.95f, 1), // strengthened
    BLUE,                           // weakened
    GREEN,                          // good
    ALARM,                          // bad
};

// one series from a ring of LinePoints, oldest first, scaled from 0 to its own top (or
// floor_top, if higher)
static void plot_series(int line, const LinePoint* ring, int count, int oldest, float height,
                        float floor_top) {
    float top = 0, sum = 0;
    for (int i = 0; i < count; ++i) {
        top = std::max(top, ring[i].v[line]);
        sum += ring[i].v[line];
    }
    const float newest = count > 0 ? ring[(oldest + count - 1) % count].v[line] : 0.0f;
    char overlay[96];
    std::snprintf(overlay, sizeof overlay, "%s  now %.0f  mean %.2f  top %.0f", line_names[line],
                  newest, count > 0 ? sum / count : 0.0f, top);
    ImGui::PushStyleColor(ImGuiCol_PlotLines, line_colors[line]);
    ImGui::PushID(line);
    ImGui::PlotLines("##line", &ring[0].v[line], count, oldest, overlay, 0.0f,
                     std::max({top * 1.1f, floor_top, 1.0f}), ImVec2(-1, height),
                     sizeof(LinePoint));
    ImGui::PopID();
    ImGui::PopStyleColor();
}

void lines_panel(State& s) {
    const Rect& r = s.layout.lines;
    ImGui::SetNextWindowPos(ImVec2(r.x, r.y));
    ImGui::SetNextWindowSize(ImVec2(r.w, r.h));
    ImGui::Begin("lines", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    const Tape& t = s.frame.tape;
    ImGui::Text("over time");
    ImGui::SameLine();
    ImGui::Checkbox("long history", &s.view.lines_long);
    ImGui::SameLine();
    if (s.view.lines_long) {
        ImGui::TextColored(DIM, "(a point per %d ticks)", LONG_STRIDE);
    } else {
        ImGui::TextColored(DIM, "(a point per tick)");
    }
    const LinePoint* ring = nullptr;
    int count = 0, oldest = 0;
    if (s.view.lines_long && t.long_points > 0) {
        ring = t.long_lines.data();
        count = static_cast<int>(std::min<uint64_t>(t.long_points, LONG_POINTS));
        oldest = t.long_points > LONG_POINTS ? static_cast<int>(t.long_points % LONG_POINTS) : 0;
    } else if (!s.view.lines_long && t.ticks > 0) {
        ring = t.lines.data();
        count = static_cast<int>(std::min<uint64_t>(t.ticks, t.len));
        oldest = t.ticks > static_cast<uint64_t>(t.len) ? static_cast<int>(t.ticks % t.len) : 0;
    }
    if (ring != nullptr) {
        const float room = ImGui::GetContentRegionAvail().y;
        const float height = std::max(20.0f, room / static_cast<float>(LINE_COUNT) -
                                                 ImGui::GetStyle().ItemSpacing.y);
        // fired is drawn against twice the target, so a dying blob sits low
        float target = 0;
        for (const SectorView& sector : s.frame.sectors) {
            target += sector.target;
        }
        for (int line = 0; line < LINE_COUNT; ++line) {
            plot_series(line, ring, count, oldest, height, line == LINE_FIRED ? 2 * target : 0);
        }
    }
    ImGui::End();
}

// a histogram with its range under it
static void plot_histogram(const char* name, const Histogram& h, const char* unit) {
    float counts[HIST_BINS];
    int last = 0;
    for (int i = 0; i < HIST_BINS; ++i) {
        counts[i] = static_cast<float>(h.counts[i]);
        if (h.counts[i] > 0) {
            last = i;
        }
    }
    char overlay[96];
    std::snprintf(overlay, sizeof overlay, "%s  %d..%d %s", name, h.low, h.high, unit);
    ImGui::PushID(name);
    ImGui::PlotHistogram("##histogram", counts, last + 1, 0, overlay, 0.0f, FLT_MAX,
                         ImVec2(-1, 44));
    ImGui::PopID();
}

void histograms_section(State& s) {
    const ViewFrame& f = s.frame;
    ImGui::SeparatorText("histograms");

    // weight levels, counted over every possible source in every live slot
    const uint64_t connected = f.weight_levels[1] + f.weight_levels[2] + f.weight_levels[3];
    const char* level_names[4] = {"0", "1", "4", "5"};
    ImGui::Text("weights: %llu links (%llu local, %llu far)",
                static_cast<unsigned long long>(connected),
                static_cast<unsigned long long>(f.local_links),
                static_cast<unsigned long long>(f.far_links));
    for (int i = 1; i < 4; ++i) {
        const float share = connected > 0 ? static_cast<float>(f.weight_levels[i]) / connected : 0;
        ImGui::Text("at %s  %8llu  %3.0f%%", level_names[i],
                    static_cast<unsigned long long>(f.weight_levels[i]), 100.0f * share);
        ImGui::SameLine();
        ImGui::ProgressBar(share, ImVec2(-1, 12), "");
    }
    ImGui::TextColored(DIM, "0 (empty, could connect): %llu",
                       static_cast<unsigned long long>(f.weight_levels[0]));

    plot_histogram("spikes per neuron (bins 0,1,2-3,4-7..)", f.rate, "");
    plot_histogram("threshold", f.threshold, "");
    plot_histogram("fan-in", f.fan_in, "links");
    plot_histogram("length", f.length, "cells");
}
