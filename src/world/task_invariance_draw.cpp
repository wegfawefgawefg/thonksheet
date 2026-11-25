#include "world/task_invariance.hpp"

// pictures for the invariance battery, all integer.
//
// the 2D shapes: 4x4 templates of 7 pixels, each chiral (its 8 turns and mirror images all
// differ) and none a turn or mirror image of another, so turning one never gives another.
// shapes 0 and 1 share 5 pixels and differ in two: in detail, not in where their weight
// sits. most pairs of shapes can be told apart at a size, turn or place never taught by a
// plain linear reader of the lines, which makes the task no test of invariance; this pair
// cannot (docs/results-2026-11-25-invariance-battery.md, "Redesigns").
//   0 .###   1 ...#   2 ##..   3 ####
//     ##..     ####     .#..     .#..
//     .#..     .#..     .##.     .#..
//     .#..     .#..     ..##     .#..
//
// a pose is drawn by asking, for every line of the canvas, which template pixel it falls in:
// undo the turn (a table of 16 angles, x1024), undo the mirror, divide by the pixel size.
// quarter turns about a line corner are exact; other angles are nearest-pixel.
//
// the 3D objects (view): 8 points each on a 4x4x4 grid, x across, y down, z into the sheet,
// seen straight on after turning about the vertical axis; each point is a 2x2 block. all
// three have a post at x = 0, z = 0. object 0 has an arm along x at the top, object 1 the
// same arm along z, so object 0 seen from the front looks like object 1 seen from the side,
// but for one point at a different height. object 2 has the arm along x at the foot.

static const char* const templates[4] = {
    ".#####...#...#..",
    "...#####.#...#..",
    "##...#...##...##",
    "####.#...#...#..",
};

// cos of k/16 of a turn, x1024; sin(k) is cos(k - 4)
static const int cos16[16] = {1024,  946,  724,  392,  0, -392, -724, -946,
                              -1024, -946, -724, -392, 0, 392,  724,  946};

static int cos_of(int a) {
    return cos16[a & 15];
}
static int sin_of(int a) {
    return cos16[(a - 4) & 15];
}

static int floor_div(int a, int b) {
    int q = a / b;
    return (a % b != 0 && ((a < 0) != (b < 0))) ? q - 1 : q;
}

static const int object_points[3][8][3] = {
    {{0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, {1, 0, 0}, {2, 0, 0}, {3, 0, 0}, {3, 1, 1}},
    {{0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, {0, 0, 1}, {0, 0, 2}, {0, 0, 3}, {1, 2, 3}},
    {{0, 0, 0}, {0, 1, 0}, {0, 2, 0}, {0, 3, 0}, {1, 3, 0}, {2, 3, 0}, {3, 3, 0}, {3, 2, 1}},
};

bool inv_template_on(int shape, int tx, int ty) {
    if (tx < 0 || tx > 3 || ty < 0 || ty > 3) {
        return false;
    }
    return templates[shape & 3][ty * 4 + tx] == '#';
}

static void put_block(uint8_t* sheet, int w, int h, int x, int y) {
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            int px = x + dx, py = y + dy;
            if (px >= 0 && px < w && py >= 0 && py < h) {
                sheet[py * w + px] = 1;
            }
        }
    }
}

// lines lit of a 2x2 cell (sub position sx, sy) at each density
static bool density_on(int density, int sx, int sy) {
    switch (density) {
    case 1:
        return sx == 0 && sy == 0;
    case 2:
        return sx == sy;
    case 3:
        return !(sx == 1 && sy == 0);
    default:
        return true;
    }
}

static void draw_template(const InvPose& p, uint8_t* sheet, int w, int h) {
    int pixel = 512 * p.size4; // a template pixel in half lines x 1024
    int c = cos_of(p.angle), s = sin_of(p.angle);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int u = 2 * x + 1 - p.cx2, v = 2 * y + 1 - p.cy2;
            // undo the turn, then the mirror
            int pu = u * c + v * s, pv = -u * s + v * c;
            pu = p.mirror ? -pu : pu;
            int fu = pu + 2 * pixel, fv = pv + 2 * pixel;
            int tx = floor_div(fu, pixel), ty = floor_div(fv, pixel);
            if (!inv_template_on(p.shape, tx, ty)) {
                continue;
            }
            int sx = (fu - tx * pixel) * 2 / pixel, sy = (fv - ty * pixel) * 2 / pixel;
            if (density_on(p.density, sx, sy)) {
                sheet[y * w + x] = 1;
            }
        }
    }
    // clutter: 2x2 blocks on a 6x6 grid of them centred on the shape
    int left = p.cx2 / 2 - 6, top = p.cy2 / 2 - 6;
    for (int i = 0; i < p.clutter && i < INV_MAX_CLUTTER; i++) {
        int b = p.clutter_at[i];
        put_block(sheet, w, h, left + 2 * (b % 6), top + 2 * (b / 6));
    }
}

static void draw_object(const InvPose& p, uint8_t* sheet, int w, int h) {
    int c = cos_of(p.angle), s = sin_of(p.angle);
    for (int i = 0; i < 8; i++) {
        const int* q = object_points[p.shape % 3][i];
        // half units about the grid's middle: -3, -1, 1, 3; one half unit is one line
        int hx = 2 * q[0] - 3, hy = 2 * q[1] - 3, hz = 2 * q[2] - 3;
        int u = hx * c + hz * s;
        int px2 = p.cx2 + floor_div(2 * u + 512, 1024);
        int py2 = p.cy2 + 2 * hy;
        put_block(sheet, w, h, floor_div(px2 - 1, 2), floor_div(py2 - 1, 2));
    }
}

void inv_draw(const InvPose& pose, int is_view, uint8_t* sheet, int canvas_w, int canvas_h) {
    if (is_view) {
        draw_object(pose, sheet, canvas_w, canvas_h);
    } else {
        draw_template(pose, sheet, canvas_w, canvas_h);
    }
}

// the tunes: three notes in an order, then all three together, so every note was last on
// at the same moment and only the order tells the tunes apart
static const int tune_order[4][3] = {{0, 1, 2}, {2, 1, 0}, {1, 0, 2}, {0, 2, 1}};

int inv_tune_ticks(int note) {
    return 4 * note;
}

uint32_t inv_tune_notes(int tune, int note, int t) {
    if (t < 0 || note < 1) {
        return 0;
    }
    int segment = t / note;
    if (segment < 3) {
        return 1u << tune_order[tune & 3][segment];
    }
    return segment == 3 ? 7u : 0u;
}
