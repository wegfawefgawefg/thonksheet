// The selected neuron on the sheet. Sources get a ring round their cell: white for an
// excitatory source, blue for an inhibitory one, violet for a far one with a line to the
// neuron. Targets get a dot in the middle of their cell. Strength shows as brightness and
// width: weak (1) dim and thin, strong (4, 5) bright and thick.

#include "renderer/marks.hpp"

#include "layout.hpp"
#include "view_frame.hpp"

#include <algorithm>

static const float WHITE[3] = {0.95f, 0.96f, 0.98f};
static const float BLUE[3] = {0.290f, 0.639f, 1.000f};
static const float VIOLET[3] = {0.690f, 0.549f, 1.000f};
static const float GREY[3] = {0.80f, 0.82f, 0.86f};
static const float GREEN[3] = {0.373f, 0.827f, 0.553f};
static const float RED[3] = {1.0f, 0.35f, 0.35f};
static const float PINK[3] = {1.000f, 0.420f, 0.604f};

static float strength_alpha(int strength) {
    return strength >= 5 ? 1.0f : strength >= 4 ? 0.8f : 0.38f;
}

static float strength_width(int strength) {
    return strength >= 5 ? 2.0f : strength >= 4 ? 1.5f : 1.0f;
}

static void push(std::vector<Mark>& marks, float x0, float y0, float x1, float y1, const float* rgb,
                 float alpha, float type, float width) {
    Mark m{};
    m.a[0] = x0;
    m.a[1] = y0;
    m.a[2] = x1;
    m.a[3] = y1;
    m.color[0] = rgb[0];
    m.color[1] = rgb[1];
    m.color[2] = rgb[2];
    m.color[3] = alpha;
    m.p[0] = type;
    m.p[1] = width;
    marks.push_back(m);
}

// the task's lines: a ring round each group (good green, bad red, the task's own lines
// white, motor lines pink), filled while a sensor line is on; the motor line that gave the
// last answer filled green or red by how it went. Round the whole sheet, a frame in the
// last answer's colour that fades over one step.
static void task_marks(std::vector<Mark>& marks, const ViewFrame& f) {
    const TaskView& t = f.task;
    const TaskTry& last = t.tries[(t.tries_total + TASK_TRIES - 1) % TASK_TRIES];
    const float* answer = last.valence > 0 ? GREEN : last.valence < 0 ? RED : GREY;
    for (const TaskLine& l : t.lines) {
        const float* rgb = l.kind == TASK_LINE_GOOD    ? GREEN
                           : l.kind == TASK_LINE_BAD   ? RED
                           : l.kind == TASK_LINE_MOTOR ? PINK
                                                       : WHITE;
        if (l.on && l.kind == TASK_LINE_MOTOR) {
            push(marks, l.x0, l.y0, l.x1, l.y1, answer, 0.35f, 0, 0.0f);
            push(marks, l.x0, l.y0, l.x1, l.y1, answer, 1.0f, 1, 3.0f);
            continue;
        }
        if (l.on) {
            push(marks, l.x0, l.y0, l.x1, l.y1, rgb, 0.22f, 0, 0.0f);
        }
        push(marks, l.x0, l.y0, l.x1, l.y1, rgb, l.on ? 1.0f : 0.55f, 1, l.on ? 2.5f : 1.5f);
    }
    const float age = static_cast<float>(f.tick - t.answer_tick);
    const float span = static_cast<float>(std::max(1, t.ticks_per_step));
    if (t.tries_total > 0 && last.valence != 0 && age < span) {
        const float cw = static_cast<float>(f.tiles_w * 8);
        const float ch = static_cast<float>(f.tiles_h * 8);
        push(marks, 0.5f, 0.5f, cw - 0.5f, ch - 0.5f, answer, 0.9f * (1.0f - age / span), 1, 5.0f);
    }
}

void build_marks(std::vector<Mark>& marks, const ViewFrame& f, const ViewSettings& v) {
    marks.clear();
    if (f.task.on && v.task_marks) {
        task_marks(marks, f);
    }
    const Selected& s = f.selected;
    if (s.neuron < 0) {
        return;
    }
    const float cx = s.x + 0.5f;
    const float cy = s.y + 0.5f;

    // far lines first, so rings and dots sit on top of them
    for (const Link& l : s.sources) {
        if (v.sources && l.far) {
            push(marks, l.x + 0.5f, l.y + 0.5f, cx, cy, VIOLET,
                 0.25f + 0.5f * strength_alpha(l.strength), 2, strength_width(l.strength));
        }
    }
    for (const Link& l : s.targets) {
        if (v.targets && l.far) {
            push(marks, cx, cy, l.x + 0.5f, l.y + 0.5f, VIOLET, 0.3f, 2, 1.0f);
        }
    }
    if (v.targets) {
        for (const Link& l : s.targets) {
            push(marks, l.x + 0.3f, l.y + 0.3f, l.x + 0.7f, l.y + 0.7f, l.far ? VIOLET : GREY,
                 strength_alpha(l.strength), 0, 0.0f);
        }
    }
    if (v.sources) {
        for (const Link& l : s.sources) {
            const float* rgb = l.far ? VIOLET : l.inhibitory ? BLUE : WHITE;
            push(marks, l.x, l.y, l.x + 1.0f, l.y + 1.0f, rgb, strength_alpha(l.strength), 1,
                 strength_width(l.strength));
        }
    }
    // the neuron itself: a thick white ring and a dark one inside it so it shows on anything
    push(marks, s.x - 0.25f, s.y - 0.25f, s.x + 1.25f, s.y + 1.25f, WHITE, 1.0f, 1, 2.5f);
    const float dark[3] = {0.0f, 0.0f, 0.0f};
    push(marks, s.x, s.y, s.x + 1.0f, s.y + 1.0f, dark, 0.9f, 1, 1.0f);
}
