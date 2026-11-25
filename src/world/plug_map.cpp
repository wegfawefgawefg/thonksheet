#include "world/plug.hpp"

#include <cstring>

// --bitmap-map logpolar: the task's bitmap is laid on the sensor sheet as 16 angles (across)
// by 8 rings (down) about the bitmap's middle, ring edges growing about 1.44 times each, the
// way the eye's map from retina to cortex goes. a shape grown about the middle then slides
// down the rings (2x is about 2 rings, 3x about 3) and a shape turned about the middle
// slides along the angles (a quarter turn is 4 lines, wrapping round). every task pixel
// within 10 lines of the middle belongs to the one ring and angle its centre falls in; a
// mapped line is on when any of its pixels is. a ring and angle with no pixel (near the
// middle, where the rings are thinner than a pixel) takes the pixel under its own middle.
// all integer, so a run repeats exactly on any machine.

// 4 x (ring's outer edge in lines)^2 x 100: edges 0.75 1.1 1.6 2.3 3.3 4.8 6.9 10
static const int ring_edge[PLUG_MAP_RINGS] = {225, 484, 1024, 2116, 4356, 9216, 19044, 40000};
// each ring's middle radius in lines x 1024
static const int ring_middle[PLUG_MAP_RINGS] = {512, 932, 1362, 1966, 2816, 4076, 5888, 8509};
// cos of k/16 of a turn, x1024; sin(k) is cos(k - 4)
static const int cos16[16] = {1024,  946,  724,  392,  0, -392, -724, -946,
                              -1024, -946, -724, -392, 0, 392,  724,  946};

bool plug_map_from_name(const char* name, int& map) {
    if (std::strcmp(name, "plain") == 0) {
        map = 0;
        return true;
    }
    if (std::strcmp(name, "logpolar") == 0) {
        map = 1;
        return true;
    }
    return false;
}

static int floor_div(int a, int b) {
    int q = a / b;
    return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}

void plug_map_prepare(Plug& plug) {
    plug.map_src_w = plug.map_src_h = 0;
    int w = plug.shape.sheet_w, h = plug.shape.sheet_h;
    if (plug.settings.bitmap_map != 1 || w * h == 0 || w * h > TASK_MAX_SHEET) {
        return;
    }
    plug.map_src_w = w;
    plug.map_src_h = h;
    std::memset(plug.map_from, 0, sizeof(plug.map_from));
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            // offsets from the middle in half lines
            int u = 2 * x + 1 - w, v = 2 * y + 1 - h;
            int q = (u * u + v * v) * 100;
            int ring = 0;
            while (ring < PLUG_MAP_RINGS && q >= ring_edge[ring]) {
                ring++;
            }
            if (ring == PLUG_MAP_RINGS) {
                continue;
            }
            int angle = 0, best = -(1 << 30);
            for (int a = 0; a < PLUG_MAP_ANGLES; a++) {
                int along = u * cos16[a] + v * cos16[(a - 4) & 15];
                if (along > best) {
                    best = along;
                    angle = a;
                }
            }
            int p = y * w + x;
            plug.map_from[ring * PLUG_MAP_ANGLES + angle][p / 64] |= 1ull << (p % 64);
        }
    }
    // empty bins: the pixel under the bin's middle
    for (int ring = 0; ring < PLUG_MAP_RINGS; ring++) {
        for (int a = 0; a < PLUG_MAP_ANGLES; a++) {
            uint64_t* from = plug.map_from[ring * PLUG_MAP_ANGLES + a];
            bool empty = true;
            for (int k = 0; k < TASK_MAX_SHEET / 64; k++) {
                empty = empty && from[k] == 0;
            }
            if (!empty) {
                continue;
            }
            int x = floor_div(w * 512 + ring_middle[ring] * cos16[a] / 1024, 1024);
            int y = floor_div(h * 512 + ring_middle[ring] * cos16[(a - 4) & 15] / 1024, 1024);
            if (x >= 0 && x < w && y >= 0 && y < h) {
                int p = y * w + x;
                from[p / 64] |= 1ull << (p % 64);
            }
        }
    }
    plug.shape.sheet_w = PLUG_MAP_ANGLES;
    plug.shape.sheet_h = PLUG_MAP_RINGS;
}

void plug_map_lines(Plug& plug, const TaskSense& sense) {
    uint64_t on[TASK_MAX_SHEET / 64] = {};
    for (int p = 0; p < plug.map_src_w * plug.map_src_h; p++) {
        on[p / 64] |= (uint64_t)(sense.sheet[p] != 0) << (p % 64);
    }
    for (int line = 0; line < PLUG_MAP_RINGS * PLUG_MAP_ANGLES; line++) {
        bool lit = false;
        for (int k = 0; k < TASK_MAX_SHEET / 64; k++) {
            lit = lit || (plug.map_from[line][k] & on[k]) != 0;
        }
        plug.line_on[plug.first_sheet + line] = lit;
    }
}
