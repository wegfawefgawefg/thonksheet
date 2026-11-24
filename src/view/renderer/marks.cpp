// The selected neuron on the sheet. Sources get a ring round their cell: white for an
// excitatory source, blue for an inhibitory one, violet for a far one with a line to the
// neuron. Targets get a dot in the middle of their cell. Strength shows as brightness and
// width: weak (1) dim and thin, strong (4, 5) bright and thick.

#include "renderer/marks.hpp"

#include "layout.hpp"
#include "view_frame.hpp"

static const float WHITE[3] = {0.95f, 0.96f, 0.98f};
static const float BLUE[3] = {0.290f, 0.639f, 1.000f};
static const float VIOLET[3] = {0.690f, 0.549f, 1.000f};
static const float GREY[3] = {0.80f, 0.82f, 0.86f};

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

void build_marks(std::vector<Mark>& marks, const ViewFrame& f, const ViewSettings& v) {
    marks.clear();
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
