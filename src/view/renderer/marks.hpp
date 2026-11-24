#pragma once
// Marks over the sheet for the picked neuron, built on the CPU each frame and drawn by
// shaders/marks.vert as one instance each.

#include <vector>

struct ViewFrame;
struct ViewSettings;

struct Mark {
    float a[4];     // rect (x0, y0, x1, y1) or line (from, to), in cells
    float color[4]; // straight alpha
    float p[4];     // type (0 filled, 1 ring, 2 line), width in pixels
};

constexpr int MARKS_MAX = 8192;

void build_marks(std::vector<Mark>& marks, const ViewFrame& f, const ViewSettings& v);
