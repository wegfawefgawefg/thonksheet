// The picked neuron: where it is and what kind, then what its engine keeps for it (the full
// engine: threshold, bias, potential, leak, cooldown and weighted sources; the minimal
// engine: threshold and its one-bit sources; table neurons: the inputs and the table they
// address), and its own spikes over the last 256 ticks.

#include "panels/palette.hpp"
#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>

static void sources_by_delay(const Selected& n) {
    int by_delay[16] = {};
    for (const Link& l : n.sources) {
        by_delay[std::min<int>(l.delay, 15)] += 1;
    }
    ImGui::Text("delays:");
    for (int d = 0; d < 16; ++d) {
        if (by_delay[d] > 0) {
            ImGui::SameLine();
            ImGui::Text("%d at %d", by_delay[d], d);
        }
    }
    ImGui::SameLine();
    ImGui::TextColored(DIM, "(sources, ticks)");
}

static void far_slots(const Selected& n) {
    for (int k = 0; k < 2; ++k) {
        ImGui::SameLine();
        if (n.far_tile[k] >= 0) {
            ImGui::TextColored(VIOLET, "tile %d, %d ticks", n.far_tile[k], n.far_delay[k]);
        } else {
            ImGui::TextColored(DIM, "empty");
        }
    }
}

static void full_numbers(const Selected& n) {
    ImGui::Text("threshold %d %+d bias = %d   potential %d", n.threshold, n.sector_bias,
                n.threshold + n.sector_bias, n.potential);
    ImGui::Text("leak >>%d   cooldown %d of %d   sector %d", n.leak_shift, n.cooldown,
                n.cooldown_len, n.sector);
    int by_strength[6] = {}, inhibitory = 0, far = 0, far_targets = 0;
    for (const Link& l : n.sources) {
        by_strength[std::min<int>(l.strength, 5)] += 1;
        inhibitory += l.inhibitory;
        far += l.far;
    }
    for (const Link& l : n.targets) {
        far_targets += l.far;
    }
    ImGui::Text("fan-in %d: %d at 1, %d at 4, %d at 5;", n.fan_in, by_strength[1], by_strength[4],
                by_strength[5]);
    ImGui::SameLine();
    ImGui::TextColored(BLUE, "%d inh", inhibitory);
    ImGui::SameLine();
    ImGui::TextColored(VIOLET, "%d far", far);
    ImGui::Text("targets %d (%d far)   far slots:", static_cast<int>(n.targets.size()),
                far_targets);
    far_slots(n);
    sources_by_delay(n);
}

static void minimal_numbers(const Selected& n) {
    int inhibitory = 0, far = 0;
    for (const Link& l : n.sources) {
        inhibitory += l.inhibitory;
        far += l.far;
    }
    ImGui::Text("threshold %d   fan-in %d (one bit each);", n.threshold, n.fan_in);
    ImGui::SameLine();
    ImGui::TextColored(BLUE, "%d inh", inhibitory);
    ImGui::SameLine();
    ImGui::TextColored(VIOLET, "%d far", far);
    ImGui::Text("targets %d%s   far slots:", static_cast<int>(n.targets.size()),
                n.targets_known ? "" : " (local only)");
    far_slots(n);
    sources_by_delay(n);
}

// the table: one square per entry, amber where the counter is at or above the bar (fires),
// blue below it, brighter the further from the bar; the entry used on the newest tick ringed
static void table_picture(const Selected& n) {
    const int entries = static_cast<int>(n.table.size());
    if (entries == 0) {
        return;
    }
    const int side_x = 1 << ((n.table_inputs + 1) / 2);
    const int side_y = std::max(1, entries / side_x);
    const float w = ImGui::GetContentRegionAvail().x;
    const float cell = std::clamp(std::min(w / side_x, 160.0f / side_y), 3.0f, 24.0f);
    const int top = (1 << n.table_bits) - 1;
    const ImVec2 at = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int e = 0; e < entries; ++e) {
        const ImVec2 p(at.x + (e % side_x) * cell, at.y + (e / side_x) * cell);
        const int v = n.table[e];
        const bool fires = v >= n.table_bar;
        const float far = fires ? static_cast<float>(v - n.table_bar + 1) / (top - n.table_bar + 1)
                                : static_cast<float>(n.table_bar - v) / std::max(1, n.table_bar);
        const ImVec4 c = fires ? AMBER : BLUE;
        const float k = 0.2f + 0.8f * std::clamp(far, 0.0f, 1.0f);
        dl->AddRectFilled(p, ImVec2(p.x + cell - 1, p.y + cell - 1),
                          ImGui::GetColorU32(ImVec4(c.x * k, c.y * k, c.z * k, 1.0f)));
        if (e == n.table_entry) {
            dl->AddRect(ImVec2(p.x - 1, p.y - 1), ImVec2(p.x + cell, p.y + cell),
                        IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);
        }
    }
    ImGui::Dummy(ImVec2(side_x * cell, side_y * cell + 2));
}

static void table_numbers(const Selected& n) {
    const int entry = n.table_entry;
    const int value = entry >= 0 && entry < static_cast<int>(n.table.size()) ? n.table[entry] : 0;
    ImGui::Text("%d inputs, %d-bit counters, sector %d bar %d", n.table_inputs, n.table_bits,
                n.sector, n.table_bar);
    ImGui::Text("newest tick: entry %d, counter %d:", entry, value);
    ImGui::SameLine();
    ImGui::TextColored(value >= n.table_bar ? AMBER : BLUE, "%s",
                       value >= n.table_bar ? "fires" : "quiet");
    // the inputs: which neuron, its delay, and whether it was on for that entry
    ImGui::Text("inputs:");
    for (int j = 0; j < static_cast<int>(n.table_input_on.size()); ++j) {
        ImGui::SameLine();
        const bool on = n.table_input_on[j] != 0;
        if (j < static_cast<int>(n.sources.size()) && n.sources[j].strength > 0) {
            const Link& l = n.sources[j];
            ImGui::TextColored(on ? AMBER : DIM, "%d,%d+%d", l.x, l.y, l.delay);
        } else {
            ImGui::TextColored(DIM, "off-sheet");
        }
        if (j % 4 == 3 && j + 1 < static_cast<int>(n.table_input_on.size())) {
            ImGui::Text("       ");
        }
    }
    ImGui::Text("targets %d", static_cast<int>(n.targets.size()));
    ImGui::TextColored(DIM, "the table: amber fires, blue quiet, ringed the entry just used");
    table_picture(n);
}

void neuron_section(State& s) {
    const ViewFrame& f = s.frame;
    const Selected& n = f.selected;
    if (!ImGui::CollapsingHeader("neuron", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    if (n.neuron < 0) {
        ImGui::TextColored(DIM, "click a cell on the sheet or a row on the raster");
        return;
    }
    ImGui::Text("#%d  cell %d,%d  tile %d bit %d ", n.neuron, n.x, n.y, n.tile, n.bit);
    ImGui::SameLine();
    ImGui::TextColored(kind_colors[n.kind], "%s", kind_names[n.kind]);
    if (f.engine == ENGINE_MINIMAL) {
        minimal_numbers(n);
    } else if (f.engine == ENGINE_TABLE) {
        table_numbers(n);
    } else {
        full_numbers(n);
    }
    const int shown = static_cast<int>(std::min<uint64_t>(f.tape.ticks, f.tape.len));
    ImGui::Text("fired %d times in the last %d ticks", f.selected_spikes, shown);

    // its own raster: the last 256 ticks as ticks on a line, now at the right
    const int span = std::min(shown, 256);
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const float w = ImGui::GetContentRegionAvail().x;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(at, ImVec2(at.x + w, at.y + 14), IM_COL32(11, 13, 18, 255));
    const ImU32 c = ImGui::GetColorU32(kind_colors[n.kind]);
    for (int age = 0; age < span; ++age) {
        if (tape_fired(f.tape, age, n.neuron)) {
            const float x = at.x + w - (age + 1) * w / span;
            dl->AddRectFilled(ImVec2(x, at.y), ImVec2(x + std::max(1.0f, w / span), at.y + 14), c);
        }
    }
    ImGui::Dummy(ImVec2(w, 16));
}
