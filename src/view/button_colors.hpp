#pragma once
// A colour per button, the same on the sheet (its cells), in the task panel's strip and in
// the readout panel. Picked away from the kinds' colours (amber, blue, green, pink) and the
// far links' violet; past eight they go round the hue circle by the golden angle, so any
// number of buttons gets colours that differ.

#include <cmath>

inline void button_rgb(int i, float* rgb) {
    static const float fixed[8][3] = {
        {0.21f, 0.88f, 0.88f}, // cyan
        {1.00f, 0.31f, 0.85f}, // magenta
        {0.72f, 0.94f, 0.25f}, // lime
        {1.00f, 0.54f, 0.24f}, // orange
        {0.55f, 0.61f, 1.00f}, // periwinkle
        {0.96f, 0.94f, 0.35f}, // yellow
        {1.00f, 0.29f, 0.29f}, // red
        {0.62f, 0.96f, 0.82f}, // pale teal
    };
    if (i >= 0 && i < 8) {
        rgb[0] = fixed[i][0];
        rgb[1] = fixed[i][1];
        rgb[2] = fixed[i][2];
        return;
    }
    // hue by the golden angle, full saturation, bright
    const float h = std::fmod(0.11f + 0.618034f * static_cast<float>(i), 1.0f) * 6.0f;
    const float f = h - std::floor(h);
    const float v = 0.95f, p = 0.25f, q = v - (v - p) * f, t = p + (v - p) * f;
    const int sector = static_cast<int>(h) % 6;
    const float table[6][3] = {{v, t, p}, {q, v, p}, {p, v, t}, {p, q, v}, {t, p, v}, {v, p, q}};
    rgb[0] = table[sector][0];
    rgb[1] = table[sector][1];
    rgb[2] = table[sector][2];
}
