#include "panels/palette.hpp"
#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>

// the picked neuron's numbers and its spikes on the tape
void neuron_section(State& s) {
    const ViewFrame& f = s.frame;
    const Selected& n = f.selected;
    ImGui::SeparatorText("neuron");
    if (n.neuron < 0) {
        ImGui::TextColored(DIM, "click a cell on the sheet or a row on the raster");
        return;
    }
    ImGui::Text("#%d  cell %d,%d  tile %d bit %d ", n.neuron, n.x, n.y, n.tile, n.bit);
    ImGui::SameLine();
    ImGui::TextColored(kind_colors[n.kind], "%s", kind_names[n.kind]);
    ImGui::Text("threshold %d %+d bias = %d   potential %d", n.threshold, n.sector_bias,
                n.threshold + n.sector_bias, n.potential);
    ImGui::Text("leak >>%d   cooldown %d of %d   sector %d", n.leak_shift, n.cooldown,
                n.cooldown_len, n.sector);
    int by_strength[6] = {}, inhibitory = 0, far = 0, far_targets = 0;
    for (const Link& l : n.sources) {
        by_strength[l.strength] += 1;
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
    for (int k = 0; k < 2; ++k) {
        ImGui::SameLine();
        if (n.far_tile[k] >= 0) {
            ImGui::TextColored(VIOLET, "tile %d, %d ticks", n.far_tile[k], n.far_delay[k]);
        } else {
            ImGui::TextColored(DIM, "empty");
        }
    }
    // sources by how many ticks their spikes take to arrive
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
