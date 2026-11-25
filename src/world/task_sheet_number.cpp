#include "world/task_script.hpp"

// number and parts-and-wholes on a sheet (tier 9). the sheet is a bitmap of lines, so
// neighbours stay neighbours on the sensor region.
//
// how-many    a 7x7 sheet and the ask. 1 to k items, none touching another (not even at a
//             corner), shown with the ask for one step. press button n - 1. items are
//             single cells; with `varied` they are rectangles of 1 to 3 by 1 to 3 cells
//             whose total area is always 6 to 9 cells, so brightness does not give n.
// more-fewer  the same sheet: one display, then another with the ask. button 1 if the
//             second has more items, 0 if fewer (never equal).
// parts       a figure made of two smaller ones from a set of k (a bar down, a bar across,
//             a corner, each 3 cells). plain: a 6x3 sheet, one part in the left half and
//             one in the right at fixed spots; press the button of the ordered pair
//             (k * k buttons). with `varied`: a 6x6 sheet, the two parts anywhere, touching
//             along an edge, never overlapping; press the button of the unordered pair
//             (k (k + 1) / 2 buttons). a figure that two other parts could also make is
//             drawn again.

constexpr int COUNT_SIDE = 7;
constexpr int AREA_LO = 6, AREA_HI = 9;
constexpr int PARTS_W = 6;
constexpr int PART_KINDS = 3;

// each part's three cells (dx, dy)
static const int part_cells[PART_KINDS][3][2] = {
    {{0, 0}, {0, 1}, {0, 2}}, // bar down
    {{0, 0}, {1, 0}, {2, 0}}, // bar across
    {{0, 0}, {0, 1}, {1, 1}}, // corner
};
// where each part sits in its half of the plain sheet
static const int part_home[PART_KINDS][2] = {{1, 0}, {0, 1}, {1, 0}};

static int parts_h(const Task& task) {
    return task.settings.varied ? PARTS_W : 3;
}

void sheet_number_reset(Task& task) {
    TaskSettings& s = task.settings;
    if (task.kind == TaskKind::parts) {
        s.k = s.k > PART_KINDS ? PART_KINDS : s.k;
    } else {
        s.k = s.k > 5 ? 5 : s.k; // at most 5 items fit the area band
    }
}

TaskShape sheet_number_shape(const Task& task) {
    TaskShape shape;
    int k = task.settings.k;
    shape.singles = 1; // ask
    if (task.kind == TaskKind::parts) {
        shape.sheet_w = PARTS_W;
        shape.sheet_h = parts_h(task);
        shape.motors = task.settings.varied ? k * (k + 1) / 2 : k * k;
    } else {
        shape.sheet_w = COUNT_SIDE;
        shape.sheet_h = COUNT_SIDE;
        shape.motors = task.kind == TaskKind::how_many ? k : 2;
    }
    return shape;
}

// --- how many items ---

// n items onto one step's sheet; false if they did not fit (draw again)
static bool place_items(Task& task, int step, int n) {
    WorldRandom& r = task.random;
    int w[5] = {1, 1, 1, 1, 1}, h[5] = {1, 1, 1, 1, 1};
    if (task.settings.varied) {
        int area = 0;
        do {
            area = 0;
            for (int i = 0; i < n; i++) {
                w[i] = 1 + world_random_below(r, 3);
                h[i] = 1 + world_random_below(r, 3);
                area += w[i] * h[i];
            }
        } while (area < AREA_LO || area > AREA_HI);
    }
    // taken: cells of an item and the ring around it
    uint8_t taken[COUNT_SIDE * COUNT_SIDE] = {};
    for (int i = 0; i < n; i++) {
        bool placed = false;
        for (int tries = 0; tries < 200 && !placed; tries++) {
            int x0 = world_random_below(r, COUNT_SIDE - w[i] + 1);
            int y0 = world_random_below(r, COUNT_SIDE - h[i] + 1);
            bool free = true;
            for (int y = y0; y < y0 + h[i] && free; y++) {
                for (int x = x0; x < x0 + w[i] && free; x++) {
                    free = !taken[y * COUNT_SIDE + x];
                }
            }
            if (!free) {
                continue;
            }
            for (int y = y0 - 1; y <= y0 + h[i]; y++) {
                for (int x = x0 - 1; x <= x0 + w[i]; x++) {
                    if (x >= 0 && y >= 0 && x < COUNT_SIDE && y < COUNT_SIDE) {
                        taken[y * COUNT_SIDE + x] = 1;
                    }
                }
            }
            for (int y = y0; y < y0 + h[i]; y++) {
                for (int x = x0; x < x0 + w[i]; x++) {
                    script_sheet_on(task, step, y * COUNT_SIDE + x);
                }
            }
            placed = true;
        }
        if (!placed) {
            return false;
        }
    }
    return true;
}

static void clear_sheet(Task& task, int step) {
    for (int i = 0; i < SCRIPT_SHEET_WORDS; i++) {
        task.script.sheet[step][i] = 0;
    }
}

static void fill_count(Task& task, int step, int n) {
    while (!place_items(task, step, n)) {
        clear_sheet(task, step);
    }
}

// --- parts ---

// the part's cells as a mask over a sheet of PARTS_W columns, false if off the sheet
static bool part_mask(int part, int x, int y, int rows, uint64_t& mask) {
    mask = 0;
    for (int c = 0; c < 3; c++) {
        int cx = x + part_cells[part][c][0], cy = y + part_cells[part][c][1];
        if (cx < 0 || cy < 0 || cx >= PARTS_W || cy >= rows) {
            return false;
        }
        mask |= 1ull << (cy * PARTS_W + cx);
    }
    return true;
}

// can `figure` be made of parts p and q
static bool made_of(uint64_t figure, int p, int q, int rows) {
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < PARTS_W; x++) {
            uint64_t mp = 0;
            if (!part_mask(p, x, y, rows, mp) || (mp & figure) != mp) {
                continue;
            }
            uint64_t rest = figure & ~mp;
            for (int y2 = 0; y2 < rows; y2++) {
                for (int x2 = 0; x2 < PARTS_W; x2++) {
                    uint64_t mq = 0;
                    if (part_mask(q, x2, y2, rows, mq) && mq == rest) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

static int pair_index(int p, int q, int kinds) {
    if (p > q) {
        int swap = p;
        p = q;
        q = swap;
    }
    return p * kinds - p * (p - 1) / 2 + (q - p);
}

// two parts touching along an edge, as a mask; 0 if this draw failed
static uint64_t touching_figure(Task& task, int p, int q, int rows) {
    WorldRandom& r = task.random;
    uint64_t mp = 0, mq = 0;
    if (!part_mask(p, world_random_below(r, PARTS_W), world_random_below(r, rows), rows, mp)) {
        return 0;
    }
    if (!part_mask(q, world_random_below(r, PARTS_W), world_random_below(r, rows), rows, mq)) {
        return 0;
    }
    if (mp & mq) {
        return 0;
    }
    // edge neighbours of p's cells, kept inside the sheet's columns
    uint64_t left_edge = 0, right_edge = 0;
    for (int y = 0; y < rows; y++) {
        left_edge |= 1ull << (y * PARTS_W);
        right_edge |= 1ull << (y * PARTS_W + PARTS_W - 1);
    }
    uint64_t near =
        (mp << PARTS_W) | (mp >> PARTS_W) | ((mp & ~right_edge) << 1) | ((mp & ~left_edge) >> 1);
    return (near & mq) ? (mp | mq) : 0;
}

static void parts_trial(Task& task, int step) {
    WorldRandom& r = task.random;
    int kinds = task.settings.k, rows = parts_h(task);
    int p = world_random_below(r, kinds), q = world_random_below(r, kinds);
    if (!task.settings.varied) {
        uint64_t mp = 0, mq = 0;
        part_mask(p, part_home[p][0], part_home[p][1], rows, mp);
        part_mask(q, part_home[q][0] + 3, part_home[q][1], rows, mq);
        task.script.sheet[step][0] = mp | mq;
        task.script.want[step] = (int8_t)(p * kinds + q);
        return;
    }
    uint64_t figure = 0;
    for (int tries = 0; tries < 100000 && !figure; tries++) {
        figure = touching_figure(task, p, q, rows);
        for (int a = 0; a < kinds && figure; a++) {
            for (int b = a; b < kinds && figure; b++) {
                bool same_pair = pair_index(a, b, kinds) == pair_index(p, q, kinds);
                if (!same_pair && made_of(figure, a, b, rows)) {
                    figure = 0; // another pair makes the same figure
                }
            }
        }
    }
    task.script.sheet[step][0] = figure;
    task.script.want[step] = (int8_t)pair_index(p, q, kinds);
}

void sheet_number_trial(Task& task) {
    WorldRandom& r = task.random;
    int k = task.settings.k;
    uint8_t flags = SCRIPT_SCORED | SCRIPT_REWARDED | SCRIPT_PROBE;
    script_add(task, 0, -1, 0); // the blank that parts trials
    switch (task.kind) {
    case TaskKind::how_many: {
        int n = 1 + world_random_below(r, k);
        int at = script_add(task, line_bit(0), n - 1, flags);
        fill_count(task, at, n);
        break;
    }
    case TaskKind::more_fewer: {
        int a = 1 + world_random_below(r, k), b = a;
        while (b == a) {
            b = 1 + world_random_below(r, k);
        }
        int first = script_add(task, 0, -1, 0);
        fill_count(task, first, a);
        int second = script_add(task, line_bit(0), b > a ? 1 : 0, flags);
        fill_count(task, second, b);
        break;
    }
    case TaskKind::parts: {
        int at = script_add(task, line_bit(0), 0, flags);
        parts_trial(task, at);
        break;
    }
    default:
        break;
    }
}
