// The task: its panel under the middle column (what it is, the step, the last tries, the
// score per window over the whole life against the learning-off twin and coin flips), and
// the names of its lines on the sheet.

#include "panels/palette.hpp"
#include "state.hpp"

#include <algorithm>
#include <cstdio>
#include <imgui.h>

// a colour per motor line (what was right, what the blob did); dark for "hold still"
static ImU32 move_color(int move) {
    static const ImU32 colors[4] = {IM_COL32(74, 163, 255, 255), IM_COL32(242, 184, 75, 255),
                                    IM_COL32(176, 140, 255, 255), IM_COL32(255, 107, 154, 255)};
    return move < 0 ? IM_COL32(52, 56, 66, 255) : colors[move % 4];
}

static ImU32 color(const ImVec4& c, float alpha = 1.0f) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(c.x, c.y, c.z, c.w * alpha));
}

static const TaskTry& try_at(const TaskView& t, int64_t age) {
    return t.tries[(t.tries_total - 1 - age) % TASK_TRIES];
}

// the last tries, newest at the right: what was right on top, what the blob did under it,
// and a bar for good or bad
static void tries_strip(const TaskView& t) {
    const float w = ImGui::GetContentRegionAvail().x;
    const float cell = 9.0f;
    const int count = static_cast<int>(std::min<int64_t>(t.tries_total, (int)(w / cell)));
    const ImVec2 at = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    for (int i = 0; i < count; ++i) {
        const TaskTry& tr = try_at(t, i);
        const float x = at.x + w - (i + 1) * cell;
        dl->AddRectFilled(ImVec2(x, at.y), ImVec2(x + cell - 2, at.y + cell - 1),
                          move_color(tr.right));
        dl->AddRectFilled(ImVec2(x, at.y + cell), ImVec2(x + cell - 2, at.y + 2 * cell - 1),
                          move_color(tr.move));
        const ImU32 bar = tr.valence > 0   ? color(GREEN)
                          : tr.valence < 0 ? color(ALARM)
                                           : IM_COL32(52, 56, 66, 255);
        dl->AddRectFilled(ImVec2(x, at.y + 2 * cell + 1), ImVec2(x + cell - 2, at.y + 2 * cell + 4),
                          bar);
    }
    ImGui::Dummy(ImVec2(w, 2 * cell + 5));
}

// a list squeezed into `width` pixels: the mean of the windows under each pixel
static float bucket(const std::vector<float>& v, int px, int width) {
    const size_t n = v.size();
    const size_t from = n * px / width;
    const size_t to = std::max(from + 1, n * (px + 1) / width);
    float sum = 0;
    for (size_t i = from; i < to && i < n; ++i) {
        sum += v[i];
    }
    return sum / static_cast<float>(std::min(to, n) - from);
}

static float mean_of(const std::vector<float>& v) {
    float sum = 0;
    for (float x : v) {
        sum += x;
    }
    return v.empty() ? 0.0f : sum / static_cast<float>(v.size());
}

// the mean of the k windows around each one (fewer at the ends)
static void smooth(const std::vector<float>& v, int k, std::vector<float>& out) {
    const int n = static_cast<int>(v.size());
    out.assign(n, 0.0f);
    double sum = 0;
    int lo = 0, hi = 0; // the windows summed: [lo, hi)
    for (int i = 0; i < n; ++i) {
        const int want_lo = std::max(0, i - k / 2);
        const int want_hi = std::min(n, i + k / 2 + 1);
        while (hi < want_hi) {
            sum += v[hi++];
        }
        while (lo < want_lo) {
            sum -= v[lo++];
        }
        out[i] = static_cast<float>(sum / (hi - lo));
    }
}

static void dashed(ImDrawList* dl, float x0, float x1, float y, ImU32 c) {
    for (float x = x0; x < x1; x += 10.0f) {
        dl->AddLine(ImVec2(x, y), ImVec2(std::min(x + 5.0f, x1), y), c, 1.5f);
    }
}

// one list as a line across the plot (points too while there are few)
static void series(ImDrawList* dl, const std::vector<float>& v, int total, ImVec2 at, ImVec2 size,
                   float top, ImU32 c, float thick) {
    if (v.empty() || total <= 0) {
        return;
    }
    // x by window index over the whole life so far, so every list lines up
    const int n = static_cast<int>(v.size());
    const int width = static_cast<int>(size.x);
    const auto y_of = [&](float s) {
        return at.y + size.y * (1.0f - std::clamp(s / top, 0.0f, 1.0f));
    };
    if (total > width) {
        const int px_end = std::max(1, width * n / total);
        ImVec2 prev;
        for (int px = 0; px < px_end; ++px) {
            const ImVec2 p(at.x + px, y_of(bucket(v, px, px_end)));
            if (px > 0) {
                dl->AddLine(prev, p, c, thick);
            }
            prev = p;
        }
        return;
    }
    const float step = size.x / std::max(1, total);
    ImVec2 prev;
    for (int i = 0; i < n; ++i) {
        const ImVec2 p(at.x + (i + 0.5f) * step, y_of(v[i]));
        if (i > 0) {
            dl->AddLine(prev, p, c, thick);
        }
        if (total <= 60) {
            dl->AddCircleFilled(p, thick + 1.0f, c);
        }
        prev = p;
    }
}

// score per window over the whole life: the blob, the learning-off twin, and dashed lines
// at the twin's mean (learning off) and the coin flips' mean (chance)
static void score_plot(const TaskView& t, const TaskScores& sc, float height) {
    const ImVec2 at = ImGui::GetCursorScreenPos();
    const ImVec2 size(ImGui::GetContentRegionAvail().x - 34, height);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 plot(at.x + 30, at.y);
    dl->AddRectFilled(plot, ImVec2(plot.x + size.x, plot.y + size.y), IM_COL32(22, 25, 33, 255));

    // the scale: shares run 0..1; cartpole's steps per fall run to the highest seen
    float top = 1.0f;
    if (!t.ratio_score) {
        for (const std::vector<float>* v : {&sc.blob, &sc.twin, &sc.coin}) {
            for (float x : *v) {
                top = std::max(top, x * 1.1f);
            }
        }
    }
    for (int i = 0; i <= 4; ++i) {
        const float y = plot.y + size.y * i / 4.0f;
        dl->AddLine(ImVec2(plot.x, y), ImVec2(plot.x + size.x, y), IM_COL32(48, 53, 64, 255));
        char label[16];
        std::snprintf(label, sizeof label, t.ratio_score ? "%.2f" : "%.0f", top * (4 - i) / 4.0f);
        dl->AddText(ImVec2(at.x, y - 7), color(DIM), label);
    }

    const int total = static_cast<int>(std::max({sc.blob.size(), sc.twin.size(), sc.coin.size()}));
    if (total == 0) {
        char text[96];
        std::snprintf(text, sizeof text, "the first score comes at step %d", t.window);
        dl->AddText(ImVec2(plot.x + 10, plot.y + size.y * 0.5f - 7), color(DIM), text);
    }
    const auto y_of = [&](float s) {
        return plot.y + size.y * (1.0f - std::clamp(s / top, 0.0f, 1.0f));
    };
    if (!sc.coin.empty()) {
        dashed(dl, plot.x, plot.x + size.x, y_of(mean_of(sc.coin)), color(DIM));
    }
    if (!sc.twin.empty()) {
        dashed(dl, plot.x, plot.x + size.x, y_of(mean_of(sc.twin)), color(BLUE, 0.8f));
    }
    // with many windows the raw scores are drawn faint and a running mean over them bold
    const int k = total > 60 ? std::max(3, total / 40) : 1;
    if (k == 1) {
        series(dl, sc.twin, total, plot, size, top, color(BLUE, 0.7f), 1.0f);
        series(dl, sc.blob, total, plot, size, top, color(AMBER), 2.0f);
    } else {
        static std::vector<float> twin, blob; // viewer thread only
        smooth(sc.twin, k, twin);
        smooth(sc.blob, k, blob);
        series(dl, sc.twin, total, plot, size, top, color(BLUE, 0.25f), 1.0f);
        series(dl, sc.blob, total, plot, size, top, color(AMBER, 0.3f), 1.0f);
        series(dl, twin, total, plot, size, top, color(BLUE, 0.9f), 1.5f);
        series(dl, blob, total, plot, size, top, color(AMBER), 2.5f);
    }

    // what the x axis covers
    if (total > 0) {
        char text[96];
        if (k > 1) {
            std::snprintf(text, sizeof text, "%d windows of %d steps (bold: mean of %d)", total,
                          t.window, k);
        } else {
            std::snprintf(text, sizeof text, "%d windows of %d steps", total, t.window);
        }
        const ImVec2 ts = ImGui::CalcTextSize(text);
        dl->AddText(ImVec2(plot.x + size.x - ts.x - 4, plot.y + size.y - ts.y - 2), color(DIM),
                    text);
    }
    ImGui::Dummy(ImVec2(size.x + 30, size.y + 2));
}

static void legend(const TaskView& t, const TaskScores& sc) {
    const auto last = [](const std::vector<float>& v) { return v.empty() ? 0.0f : v.back(); };
    ImGui::TextColored(AMBER, "blob %.2f", last(sc.blob));
    ImGui::SameLine();
    if (t.twin) {
        ImGui::TextColored(BLUE, "learning off %.2f (mean %.2f)", last(sc.twin), mean_of(sc.twin));
        ImGui::SameLine();
    }
    ImGui::TextColored(DIM, "chance %.2f", mean_of(sc.coin));
}

static void move_name(int move, char* out, int size) {
    if (move < 0) {
        std::snprintf(out, size, "nothing");
    } else {
        std::snprintf(out, size, "motor %d", move);
    }
}

static const char* answer_word(int valence) {
    return valence > 0 ? "good" : valence < 0 ? "bad" : "nothing";
}

void task_panel(State& s) {
    const TaskView& t = s.frame.task;
    const Rect& r = s.layout.task;
    if (!t.on || r.w <= 0) {
        return;
    }
    ImGui::SetNextWindowPos(ImVec2(r.x, r.y));
    ImGui::SetNextWindowSize(ImVec2(r.w, r.h));
    ImGui::Begin("task", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar);
    ImGui::Text("task %s", t.name);
    ImGui::SameLine();
    ImGui::TextColored(t.learning_on ? GREEN : ALARM,
                       t.learning_on ? "learning on" : "learning off");
    ImGui::SameLine();
    ImGui::Text("  step %lld  (tick %d of %d)", static_cast<long long>(t.step), t.tick_in_step,
                t.ticks_per_step);
    ImGui::PushStyleColor(ImGuiCol_Text, DIM);
    ImGui::TextWrapped("%s", t.settings);
    ImGui::PopStyleColor();

    // the last try in words, and the share of the last 100
    if (t.tries_total > 0) {
        const TaskTry& last = try_at(t, 0);
        int good = 0, bad = 0;
        const int n = static_cast<int>(std::min<int64_t>(t.tries_total, 100));
        for (int i = 0; i < n; ++i) {
            good += try_at(t, i).valence > 0 ? 1 : 0;
            bad += try_at(t, i).valence < 0 ? 1 : 0;
        }
        char did[16], right[16];
        move_name(last.move, did, sizeof did);
        move_name(last.right, right, sizeof right);
        if (t.has_cue) {
            ImGui::Text("last: cue %d, answered %s:", last.cue, did);
        } else {
            ImGui::Text("last: wanted %s, did %s:", right, did);
        }
        ImGui::SameLine();
        ImGui::TextColored(last.valence > 0   ? GREEN
                           : last.valence < 0 ? ALARM
                                              : DIM,
                           "%s", answer_word(last.valence));
        ImGui::SameLine();
        ImGui::TextColored(DIM, "  last %d:", n);
        ImGui::SameLine();
        ImGui::TextColored(GREEN, "%d", good);
        ImGui::SameLine();
        ImGui::TextColored(ALARM, "%d", bad);
        ImGui::SameLine();
        ImGui::TextColored(DIM, "%d still", n - good - bad);
    } else {
        ImGui::TextColored(DIM, "no answer yet");
    }
    ImGui::TextColored(DIM, "tries, newest right: wanted / did / good or bad   key:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.29f, 0.64f, 1.0f, 1), "0");
    ImGui::SameLine();
    ImGui::TextColored(AMBER, "1");
    ImGui::SameLine();
    ImGui::TextColored(DIM, "still");
    tries_strip(t);

    const float room = ImGui::GetContentRegionAvail().y - ImGui::GetTextLineHeightWithSpacing() - 6;
    score_plot(t, s.frame.scores, std::max(40.0f, room));
    legend(t, s.frame.scores);
    ImGui::End();
}

// each named line's name just above its group on the sheet, on a dark patch so it reads
// over firing cells
void task_labels(State& s) {
    const ViewFrame& f = s.frame;
    if (!f.task.on || !s.view.task_marks || s.mode == RUN_TURBO) {
        return;
    }
    const Rect& r = s.layout.sheet;
    const SheetPlace p = sheet_place(r, f.tiles_w * 8, f.tiles_h * 8, s.view);
    ImDrawList* dl = ImGui::GetBackgroundDrawList();
    dl->PushClipRect(ImVec2(r.x, r.y), ImVec2(r.x + r.w, r.y + r.h), true);
    for (const TaskLine& l : f.task.lines) {
        if (l.label[0] == '\0') {
            continue;
        }
        const ImVec4 c = l.kind == TASK_LINE_GOOD    ? GREEN
                         : l.kind == TASK_LINE_BAD   ? ALARM
                         : l.kind == TASK_LINE_MOTOR ? PINK
                                                     : ImVec4(0.92f, 0.94f, 0.96f, 1.0f);
        const ImVec2 size = ImGui::CalcTextSize(l.label);
        const ImVec2 at(p.origin_x + l.x0 * p.cell_px, p.origin_y + l.y0 * p.cell_px - size.y - 3);
        dl->AddRectFilled(ImVec2(at.x - 2, at.y - 1), ImVec2(at.x + size.x + 2, at.y + size.y + 1),
                          IM_COL32(11, 13, 18, 210));
        dl->AddText(at, color(c), l.label);
    }
    dl->PopClipRect();
}
