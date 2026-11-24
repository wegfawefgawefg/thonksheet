#include "panels/palette.hpp"
#include "state.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <imgui.h>

// each sector against its target: a map (blue under, grey on target, red over), then bars
void sectors_section(State& s) {
    const ViewFrame& f = s.frame;
    ImGui::SeparatorText("sectors: activity / target, bias");
    if (f.sectors.empty() || f.tiles_w == 0) {
        return;
    }
    const int st = std::max(1, f.sector_tiles);
    const int sw = (f.tiles_w + st - 1) / st;
    const int sh = (f.tiles_h + st - 1) / st;
    const float side = std::min(150.0f, 150.0f * sw / std::max(sw, sh));
    const float cell = side / std::max(sw, sh);
    const ImVec2 at = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    float low = 1e9f, high = 0, sum = 0;
    int bias_low = 1 << 20, bias_high = -(1 << 20);
    for (int i = 0; i < static_cast<int>(f.sectors.size()); ++i) {
        const SectorView& v = f.sectors[i];
        const float ratio = v.activity / std::max(0.01f, v.target);
        low = std::min(low, ratio);
        high = std::max(high, ratio);
        sum += ratio;
        bias_low = std::min(bias_low, v.bias);
        bias_high = std::max(bias_high, v.bias);
        // log2 of the ratio, -2..2, to blue..grey..red
        const float l = std::clamp(std::log2(std::max(ratio, 1e-3f)) / 2.0f, -1.0f, 1.0f);
        ImVec4 col = l < 0 ? ImVec4(0.45f + 0.45f * l, 0.48f + 0.15f * l, 0.55f + 0.45f * -l, 1)
                           : ImVec4(0.45f + 0.55f * l, 0.48f - 0.3f * l, 0.55f - 0.4f * l, 1);
        if (v.activity <= 0.0f) {
            col = ImVec4(0.0f, 0.0f, 0.0f, 1.0f); // dead
        }
        const float x = at.x + (i % sw) * cell;
        const float y = at.y + (i / sw) * cell;
        dl->AddRectFilled(ImVec2(x, y), ImVec2(x + cell - 1, y + cell - 1),
                          ImGui::GetColorU32(col));
        if (cell >= 34) {
            char text[32];
            std::snprintf(text, sizeof text, "%.1fx", ratio);
            dl->AddText(ImVec2(x + 3, y + 2), IM_COL32(235, 238, 245, 255), text);
            std::snprintf(text, sizeof text, "%+d", v.bias);
            dl->AddText(ImVec2(x + 3, y + 16), IM_COL32(200, 205, 215, 255), text);
        }
    }
    ImGui::Dummy(ImVec2(side, sh * cell));
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("activity / target");
    ImGui::Text("  mean %.2fx", sum / std::max<size_t>(1, f.sectors.size()));
    ImGui::Text("  low  %.2fx", low);
    ImGui::Text("  high %.2fx", high);
    ImGui::Text("bias %+d .. %+d", bias_low, bias_high);
    ImGui::TextColored(DIM, "blue under, red over,");
    ImGui::TextColored(DIM, "black dead");
    ImGui::Text("good %d  bad %d  surprise %s", f.good_ticks, f.bad_ticks,
                f.surprise_high ? "high" : "low");
    ImGui::EndGroup();
}
