#include "world/task_function_parts.hpp"

// points: N cue lines, each on or off, shown together with the ask. two buttons for the
// boolean functions (button 1 when the function is true), N + 1 for count, N for first.
// past: two lines A and B; each item lights one of them for a step, items `gap` blank
// steps apart, the last with the ask. button 1 when the function of the items is true.

static int ones(int x) {
    return __builtin_popcount((unsigned)x);
}

// can one threshold unit (a weighted sum against a bar) give this two-class table? a
// perceptron on +-1 inputs, at most 2,000 passes: if it gets every input right it can. a
// table it does not get in time is called not, which for N <= 4 is exact
static bool threshold_unit_gets(const uint8_t* cls, int lines) {
    int inputs = 1 << lines;
    int w[16] = {}, b = 0;
    for (int pass = 0; pass < 2000; pass++) {
        int wrong = 0;
        for (int x = 0; x < inputs; x++) {
            int target = cls[x] ? 1 : -1;
            int sum = b;
            for (int i = 0; i < lines; i++) {
                sum += (x >> i) & 1 ? w[i] : -w[i];
            }
            if (sum * target > 0) {
                continue;
            }
            wrong++;
            b += target;
            for (int i = 0; i < lines; i++) {
                w[i] += (x >> i) & 1 ? target : -target;
            }
        }
        if (wrong == 0) {
            return true;
        }
    }
    return false;
}

static bool constant(const uint8_t* cls, int inputs) {
    for (int x = 1; x < inputs; x++) {
        if (cls[x] != cls[0]) {
            return false;
        }
    }
    return true;
}

// any table but the two constant ones
static void draw_any_table(Task& task) {
    FnState& f = task.fn;
    do {
        for (int x = 0; x < f.inputs; x++) {
            f.cls[x] = (uint8_t)world_random_below(task.random, 2);
        }
    } while (constant(f.cls, f.inputs));
}

// a random threshold unit: weights +-1..4 on every line, the bar at the sum of one random
// input, so both answers occur
static void draw_lin_table(Task& task, int lines) {
    FnState& f = task.fn;
    do {
        int w[16];
        for (int i = 0; i < lines; i++) {
            int size = 1 + world_random_below(task.random, 4);
            w[i] = world_random_below(task.random, 2) ? size : -size;
        }
        auto sum = [&](int x) {
            int s = 0;
            for (int i = 0; i < lines; i++) {
                s += (x >> i) & 1 ? w[i] : 0;
            }
            return s;
        };
        int bar = sum(world_random_below(task.random, f.inputs));
        for (int x = 0; x < f.inputs; x++) {
            f.cls[x] = sum(x) >= bar;
        }
    } while (constant(f.cls, f.inputs));
}

void points_classes(Task& task) {
    const FnSettings& s = task.settings.fn;
    FnState& f = task.fn;
    int n = s.lines;
    f.inputs = 1 << n;
    f.classes = 2;
    switch (s.fn) {
    case FnName::table:
        draw_any_table(task);
        break;
    case FnName::table_lin:
        draw_lin_table(task, n);
        break;
    case FnName::table_nonlin:
        // tables a threshold unit cannot give; for N = 2 that is exclusive-or or its negation
        for (int tries = 0; tries < 100000; tries++) {
            draw_any_table(task);
            if (!threshold_unit_gets(f.cls, n)) {
                break;
            }
        }
        break;
    case FnName::count:
        f.classes = n + 1;
        break;
    case FnName::first:
        f.classes = n;
        break;
    default:
        break;
    }
    for (int x = 0; x < f.inputs; x++) {
        int on = ones(x);
        switch (s.fn) {
        case FnName::and_all:
            f.cls[x] = on == n;
            break;
        case FnName::or_any:
            f.cls[x] = on > 0;
            break;
        case FnName::parity:
            f.cls[x] = on & 1;
            break;
        case FnName::majority:
            f.cls[x] = 2 * on > n; // half on, with N even, is not a majority
            break;
        case FnName::exactly:
            f.cls[x] = on == s.k;
            break;
        case FnName::count:
            f.cls[x] = (uint8_t)on;
            break;
        case FnName::first:
            f.cls[x] = x ? (uint8_t)__builtin_ctz((unsigned)x) : FN_NO_CLASS;
            break;
        default:
            break; // the tables are drawn above
        }
    }
    f.threshold_unit = f.classes == 2 && threshold_unit_gets(f.cls, n);
}

void past_classes(Task& task) {
    const FnSettings& s = task.settings.fn;
    FnState& f = task.fn;
    int items = s.items;
    f.inputs = 1 << items;
    f.classes = 2;
    // bit j of x is item j, oldest first; 1 is B
    for (int x = 0; x < f.inputs; x++) {
        int b = ones(x);
        switch (s.fn) {
        case FnName::same:
            f.cls[x] = (x & 1) == ((x >> 1) & 1);
            break;
        case FnName::parity:
            f.cls[x] = b & 1;
            break;
        case FnName::majority:
            f.cls[x] = 2 * b > items;
            break;
        case FnName::recall:
            f.cls[x] = x & 1;
            break;
        default:
            break;
        }
    }
}
