// The ImGui side: the views' titles, the controls and the health line, and the side window
// that holds them with the neuron, sectors and histograms (their own files).

#include "panels/palette.hpp"
#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>

// a title just above a view
static void title(const Rect& r, const char* text) {
    ImGui::GetBackgroundDrawList()->AddText(ImVec2(r.x, r.y - 16), IM_COL32(170, 176, 190, 255),
                                            text);
}

static void view_titles(State& s) {
    const ViewFrame& f = s.frame;
    char text[200];
    std::snprintf(text, sizeof text, "sheet  %d x %d cells   %s", f.tiles_w * 8, f.tiles_h * 8,
                  s.view.zoom > 1.0f ? "(zoomed: Z resets)" : "");
    title(s.layout.sheet, text);
    const int shown = static_cast<int>(std::min<uint64_t>(f.tape.ticks, f.tape.len));
    float first = 0, per_px = 1;
    raster_rows(s.layout.raster, f.tile_count, s.view, &first, &per_px);
    std::snprintf(text, sizeof text,
                  "raster  last %d ticks, now at the right   rows %d..%d (a row per %s, %.1f per "
                  "pixel)",
                  shown, static_cast<int>(first),
                  static_cast<int>(first + per_px * s.layout.raster.h) - 1,
                  s.view.raster_rows == 0 ? "neuron" : "tile", per_px);
    title(s.layout.raster, text);
    if (s.layout.recur.w > 16) {
        std::snprintf(text, sizeof text, "recurrence  last %d ticks",
                      std::min(shown, std::max(8, s.view.recur_ticks)));
        title(s.layout.recur, text);
    }
    task_labels(s);
    if (s.mode == RUN_TURBO) {
        const Rect& r = s.layout.sheet;
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(r.x + 20, r.y + 20),
                                                IM_COL32(242, 184, 75, 255),
                                                "full speed: nothing drawn (F to draw again)");
    }
}

static void mode_button(State& s, const char* label, int mode) {
    if (ImGui::RadioButton(label, s.mode == mode)) {
        set_mode(s, mode);
    }
    ImGui::SameLine();
}

static void controls(State& s) {
    const ViewFrame& f = s.frame;
    SimShared& sh = s.sim.shared;
    ImGui::Text("thonksheet   %s", f.real ? "real blob" : "fake blob");
    if (!f.real) {
        ImGui::SameLine();
        ImGui::TextColored(DIM, "(%s)", fake_scene_names[s.sim.settings.scene]);
    }
    ImGui::Text("%d neurons, %d tiles (%dx%d), %d sectors", f.neuron_count, f.tile_count, f.tiles_w,
                f.tiles_h, f.sector_count);
    ImGui::Text("tick %llu   %.0f ticks/s   %.0f fps",
                static_cast<unsigned long long>(sh.ticks.load()), s.clock.ticks_per_second,
                s.clock.fps);
    mode_button(s, "pause", RUN_PAUSED);
    mode_button(s, "slow", RUN_SLOW);
    mode_button(s, "run", RUN_FULL);
    mode_button(s, "full, undrawn", RUN_TURBO);
    ImGui::NewLine();
    if (ImGui::Button("step (E)")) {
        set_mode(s, RUN_PAUSED);
        sh.steps += 1;
    }
    ImGui::SameLine();
    int tps = sh.slow_tps.load();
    ImGui::SetNextItemWidth(150);
    if (ImGui::SliderInt("slow ticks/s", &tps, 1, 1000, "%d", ImGuiSliderFlags_Logarithmic)) {
        sh.slow_tps = tps;
    }
    if (s.sim.task.on) {
        // the task: a whole step, the sheet marks, and learning on or off for a new life
        if (ImGui::Button("task step (W)")) {
            set_mode(s, RUN_PAUSED);
            sh.task_steps += 1;
        }
        ImGui::SameLine();
        ImGui::Checkbox("task marks", &s.view.task_marks);
        ImGui::SameLine();
        ImGui::Checkbox("learning", &s.learning);
        ImGui::SameLine();
        if (ImGui::Button("restart")) {
            sim_restart(s.sim, s.learning);
        }
        ImGui::SameLine();
        ImGui::TextColored(DIM, "(a new life)");
    }
    if (ImGui::Button("good")) {
        sh.reward = 1;
    }
    ImGui::SameLine();
    if (ImGui::Button("bad")) {
        sh.reward = -1;
    }
    ImGui::SameLine();
    bool input = sh.input_on.load();
    if (f.real && !s.sim.task.on && ImGui::Checkbox("random input", &input)) {
        sh.input_on = input;
    }
    ImGui::Checkbox("tiles (T)", &s.view.tile_lines);
    ImGui::SameLine();
    ImGui::Checkbox("sectors (G)", &s.view.sector_lines);
    ImGui::SameLine();
    ImGui::Checkbox("sources", &s.view.sources);
    ImGui::SameLine();
    ImGui::Checkbox("targets", &s.view.targets);
    ImGui::SetNextItemWidth(120);
    ImGui::SliderInt("fade ticks", &s.view.fade, 1, 16);
    ImGui::SameLine();
    const char* rows[2] = {"neuron", "tile"};
    ImGui::SetNextItemWidth(90);
    if (ImGui::Combo("raster row (R)", &s.view.raster_rows, rows, 2)) {
        s.view.raster_first = s.view.raster_count = 0;
    }
}

// a curve over lags 1..64 scaled from lo to hi, with a tick every 8 lags, the zero line
// when it is in range, and the period marked
static void lag_plot(const char* name, const float* values, float lo, float hi, int period,
                     const ImVec4& c) {
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const ImVec2 size(ImGui::GetContentRegionAvail().x, 40);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(at, ImVec2(at.x + size.x, at.y + size.y), IM_COL32(22, 25, 33, 255));
    const auto x_of = [&](int lag) { return at.x + size.x * (lag - 0.5f) / LAG_MAX; };
    const auto y_of = [&](float v) {
        return at.y + size.y * (1.0f - std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f));
    };
    for (int lag = 8; lag <= LAG_MAX; lag += 8) {
        dl->AddLine(ImVec2(x_of(lag), at.y), ImVec2(x_of(lag), at.y + 4),
                    IM_COL32(90, 96, 110, 255));
    }
    if (lo < 0.0f && hi > 0.0f) {
        dl->AddLine(ImVec2(at.x, y_of(0)), ImVec2(at.x + size.x, y_of(0)),
                    IM_COL32(60, 66, 80, 255));
    }
    if (period > 0) {
        for (int lag = period; lag <= LAG_MAX; lag += period) {
            dl->AddLine(ImVec2(x_of(lag), at.y), ImVec2(x_of(lag), at.y + size.y),
                        IM_COL32(242, 184, 75, lag == period ? 150 : 60));
        }
    }
    const ImU32 line = ImGui::ColorConvertFloat4ToU32(c);
    for (int lag = 2; lag <= LAG_MAX; ++lag) {
        dl->AddLine(ImVec2(x_of(lag - 1), y_of(values[lag - 2])),
                    ImVec2(x_of(lag), y_of(values[lag - 1])), line, 1.5f);
    }
    dl->AddText(ImVec2(at.x + 4, at.y + size.y - 15), IM_COL32(150, 156, 170, 255), name);
    ImGui::Dummy(size);
}

// the alarms: dead, stuck, repeating. Red when out of a sane range.
static void health(State& s) {
    const ViewFrame& f = s.frame;
    const float n = static_cast<float>(std::max(1, f.neuron_count));
    const float silent = 100.0f * f.silent / n;
    const float stuck = 100.0f * f.stuck / n;
    const int shown = static_cast<int>(std::min<uint64_t>(f.tape.ticks, f.tape.len));
    ImGui::SeparatorText("health");
    ImGui::TextColored(silent > 20.0f ? ALARM : GREEN, "silent %.1f%%", silent);
    ImGui::SameLine();
    ImGui::TextColored(stuck > 2.0f ? ALARM : GREEN, "stuck on %.1f%%", stuck);
    ImGui::SameLine();
    const float per_1024 = f.mean_rate * 1024.0f;
    const bool rate_bad = per_1024 < 4.0f || per_1024 > 300.0f;
    ImGui::TextColored(rate_bad ? ALARM : GREEN, "%.1f spikes per 1024 per tick", per_1024);
    ImGui::PushStyleColor(ImGuiCol_Text, DIM);
    ImGui::TextWrapped("over the last %d ticks; silent: never fired, stuck: fired on over a "
                       "quarter of them",
                       shown);
    ImGui::PopStyleColor();
    // the shortest lag (after 1) that repeats nearly as well as the best one: the period
    float top = 0;
    for (int lag = 2; lag <= LAG_MAX; ++lag) {
        top = std::max(top, f.lag_similarity[lag - 1]);
    }
    int best = 2;
    while (best < LAG_MAX && f.lag_similarity[best - 1] < 0.9f * top) {
        ++best;
    }
    const float chance = f.mean_rate / std::max(0.001f, 2.0f - f.mean_rate);
    const float peak = f.lag_similarity[best - 1];
    const bool cyclic = peak > 0.25f && peak > 4.0f * chance;
    ImGui::TextColored(cyclic ? ALARM : GREEN, "repeat: lag %d overlaps %.2f", best, peak);
    ImGui::SameLine();
    ImGui::TextColored(DIM, "(chance %.3f, next tick %.2f)", chance, f.lag_similarity[0]);

    // the rhythm: the fired count against itself, and the spike overlap, by lag
    const float strength = f.period > 0 ? f.fired_lag[f.period - 1] : 0.0f;
    if (f.period > 0) {
        ImGui::TextColored(strength > 0.5f ? AMBER : GREEN, "rhythm: pulses every %d ticks",
                           f.period);
        ImGui::SameLine();
        ImGui::TextColored(DIM, "(fired count r %.2f at that lag)", strength);
    } else {
        ImGui::TextColored(GREEN, "rhythm: none clear");
    }
    lag_plot("fired count vs itself, lag 1..64 (-1..1)", f.fired_lag, -1.0f, 1.0f, f.period, AMBER);
    float low = 1, high = 0;
    for (float v : f.lag_similarity) {
        low = std::min(low, v);
        high = std::max(high, v);
    }
    char name[96];
    std::snprintf(name, sizeof name, "spike overlap by lag (%.3f..%.3f)", low, high);
    lag_plot(name, f.lag_similarity, low, std::max(high, low + 0.001f), f.period,
             cyclic ? ALARM : DIM);
}

void draw_panels(State& s) {
    view_titles(s);
    if (s.mode != RUN_TURBO) {
        lines_panel(s);
        task_panel(s);
    }
    const Rect& r = s.layout.side;
    ImGui::SetNextWindowPos(ImVec2(r.x, r.y));
    ImGui::SetNextWindowSize(ImVec2(r.w, r.h));
    ImGui::Begin("side", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    controls(s);
    if (s.mode != RUN_TURBO) {
        health(s);
        neuron_section(s);
        sectors_section(s);
        histograms_section(s);
    }
    ImGui::End();
}
