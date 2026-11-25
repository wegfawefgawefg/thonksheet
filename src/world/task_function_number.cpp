#include "world/task_function_parts.hpp"

// number: x in 0..L-1 lights one line of L (place code). the lines sit on a sheet 8 wide,
// in a snake (left to right, then right to left on the next row), so x and x + 1 are always
// neighbours. f(x) is worked out at the centre of x's share of [0, 1) in units of 1/4096,
// u = (2x + 1) * 4096 / 2L, all in integers; the answer is the band of K that f falls in.
//
//   identity  u              step    0 below the middle, else the top (always 2 buttons)
//   tent      up then down   sine    one turn, 0.5 + 0.5 sin   cosine  the same, cos
//   square    u^2
//
// two-numbers: x1 and x2 the same way, x1's rows above x2's. sum, diff (x1 - x2, signed),
// larger (the larger value), product: the band of the result over its whole range.
// compare: button 0 if x1 is larger, 1 if x2 is; equal pairs are never shown.

constexpr int FN_ONE = 4096;

// round(2047 sin(2 pi i / 128))
static const int16_t sine_table[128] = {
    0,     100,   201,   300,   399,   497,   594,   690,   783,   875,   965,   1052,  1137,
    1219,  1299,  1375,  1447,  1517,  1582,  1644,  1702,  1756,  1805,  1850,  1891,  1927,
    1959,  1986,  2008,  2025,  2037,  2045,  2047,  2045,  2037,  2025,  2008,  1986,  1959,
    1927,  1891,  1850,  1805,  1756,  1702,  1644,  1582,  1517,  1447,  1375,  1299,  1219,
    1137,  1052,  965,   875,   783,   690,   594,   497,   399,   300,   201,   100,   0,
    -100,  -201,  -300,  -399,  -497,  -594,  -690,  -783,  -875,  -965,  -1052, -1137, -1219,
    -1299, -1375, -1447, -1517, -1582, -1644, -1702, -1756, -1805, -1850, -1891, -1927, -1959,
    -1986, -2008, -2025, -2037, -2045, -2047, -2045, -2037, -2025, -2008, -1986, -1959, -1927,
    -1891, -1850, -1805, -1756, -1702, -1644, -1582, -1517, -1447, -1375, -1299, -1219, -1137,
    -1052, -965,  -875,  -783,  -690,  -594,  -497,  -399,  -300,  -201,  -100,
};

// f at x's centre, in [0, 4096)
static int value_at(FnName fn, int x, int lines) {
    int u = (2 * x + 1) * FN_ONE / (2 * lines);
    int turn = u * 128 / FN_ONE; // u as an index into one turn of the sine table
    switch (fn) {
    case FnName::step:
        return u < FN_ONE / 2 ? 0 : FN_ONE - 1;
    case FnName::tent: {
        int d = 2 * u - FN_ONE;
        return FN_ONE - (d < 0 ? -d : d);
    }
    case FnName::sine:
        return 2048 + sine_table[turn & 127];
    case FnName::cosine:
        return 2048 + sine_table[(turn + 32) & 127];
    case FnName::square:
        return u * u / FN_ONE;
    default:
        return u;
    }
}

static int band(int value, int range, int bands) {
    int b = (int)((int64_t)value * bands / range);
    return b < bands ? b : bands - 1;
}

void number_classes(Task& task) {
    const FnSettings& s = task.settings.fn;
    FnState& f = task.fn;
    f.inputs = s.lines;
    f.classes = s.fn == FnName::step ? 2 : s.bands;
    for (int x = 0; x < f.inputs; x++) {
        f.cls[x] = (uint8_t)band(value_at(s.fn, x, s.lines), FN_ONE, f.classes);
    }
}

void pair_classes(Task& task) {
    const FnSettings& s = task.settings.fn;
    FnState& f = task.fn;
    int L = s.lines;
    f.inputs = L * L;
    f.classes = s.fn == FnName::compare ? 2 : s.bands;
    for (int x1 = 0; x1 < L; x1++) {
        for (int x2 = 0; x2 < L; x2++) {
            int c = 0;
            switch (s.fn) {
            case FnName::sum:
                c = band(x1 + x2, 2 * L - 1, f.classes);
                break;
            case FnName::diff:
                c = band(x1 - x2 + L - 1, 2 * L - 1, f.classes);
                break;
            case FnName::larger:
                c = band(x1 > x2 ? x1 : x2, L, f.classes);
                break;
            case FnName::product:
                c = band(x1 * x2, (L - 1) * (L - 1) + 1, f.classes);
                break;
            case FnName::compare:
                c = x1 == x2 ? FN_NO_CLASS : (x1 > x2 ? 0 : 1);
                break;
            default:
                break;
            }
            f.cls[x1 * L + x2] = (uint8_t)c;
        }
    }
}

// held-out values sit inside a band: both neighbours are taught and in the same band, so
// filling in between taught values gives the right answer. never two held side by side.
void number_hold(Task& task) {
    FnState& f = task.fn;
    int L = f.inputs;
    int candidate[64], count = 0;
    for (int x = 1; x + 1 < L; x++) {
        if (f.cls[x - 1] == f.cls[x] && f.cls[x + 1] == f.cls[x]) {
            candidate[count++] = x;
        }
    }
    for (int i = count - 1; i > 0; i--) {
        int j = world_random_below(task.random, i + 1);
        int swap = candidate[i];
        candidate[i] = candidate[j];
        candidate[j] = swap;
    }
    int held = 0;
    for (int i = 0; i < count && held < task.settings.fn.hold; i++) {
        int x = candidate[i];
        uint64_t near = (1ull << (x - 1)) | (1ull << (x + 1));
        if (!(task.held_out & near)) {
            task.held_out |= 1ull << x;
            held++;
        }
    }
}

int number_sheet_w(const Task& task) {
    int L = task.settings.fn.lines;
    return L < FN_SHEET_WIDTH ? L : FN_SHEET_WIDTH;
}

static int rows_per_number(const Task& task) {
    int w = number_sheet_w(task);
    return (task.settings.fn.lines + w - 1) / w;
}

int number_sheet_h(const Task& task) {
    return rows_per_number(task) * (task.kind == TaskKind::two_numbers ? 2 : 1);
}

static void light(const Task& task, TaskSense& sense, int value, int first_row) {
    int w = number_sheet_w(task);
    int row = value / w, col = value % w;
    col = row & 1 ? w - 1 - col : col;
    sense.sheet[(first_row + row) * w + col] = 1;
}

void number_sheet(const Task& task, TaskSense& sense) {
    int x = task.fn.x;
    if (task.kind == TaskKind::number) {
        light(task, sense, x, 0);
        return;
    }
    int L = task.settings.fn.lines;
    light(task, sense, x / L, 0);
    light(task, sense, x % L, rows_per_number(task));
}
