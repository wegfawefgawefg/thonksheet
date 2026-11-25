#include "world/task_script.hpp"

// parts and wholes in time (tier 9): strings shown one symbol a step, then the ask.
// button 0 if the string is allowed, 1 if not; half of all strings are allowed.
//
// grammar   Reber's grammar: lines B T P S X V E and the ask. an allowed string is a walk
//           through the grammar (each fork a coin flip), at most `length` symbols long. a
//           broken one is an allowed string with one inner symbol changed to another inner
//           symbol so that it is no longer allowed: same length, same start and end.
// brackets  lines ( and ) and the ask. every string has `length` symbols, half of each,
//           starts with ( and ends with ). allowed: balanced (no prefix closes more than it
//           opened). counts, length, first and last symbol are the same for both answers;
//           only the order says.

enum ReberSymbol { RB, RT, RP, RS, RX, RV, RE, REBER_SYMBOLS };

// the grammar as (state, symbol) -> next state; -1 not allowed, 7 the end
static int reber_next(int state, int symbol) {
    static const int next[7][REBER_SYMBOLS] = {
        // B   T   P   S   X   V   E
        {1, -1, -1, -1, -1, -1, -1}, // 0 start
        {-1, 2, 3, -1, -1, -1, -1},  // 1
        {-1, -1, -1, 2, 4, -1, -1},  // 2
        {-1, 3, -1, -1, -1, 5, -1},  // 3
        {-1, -1, -1, 6, 3, -1, -1},  // 4
        {-1, -1, 4, -1, -1, 6, -1},  // 5
        {-1, -1, -1, -1, -1, -1, 7}, // 6
    };
    return state >= 0 && state < 7 ? next[state][symbol] : -1;
}

static bool reber_allowed(const int* s, int len) {
    int state = 0;
    for (int i = 0; i < len; i++) {
        state = reber_next(state, s[i]);
        if (state < 0) {
            return false;
        }
    }
    return state == 7;
}

// a walk through the grammar, at most `most` symbols; returns its length
static int reber_walk(WorldRandom& r, int* out, int most) {
    for (;;) {
        int state = 0, len = 0;
        while (state != 7 && len < most) {
            int options[REBER_SYMBOLS] = {}, count = 0;
            for (int sym = 0; sym < REBER_SYMBOLS; sym++) {
                if (reber_next(state, sym) >= 0) {
                    options[count++] = sym;
                }
            }
            int sym = options[world_random_below(r, count)];
            out[len++] = sym;
            state = reber_next(state, sym);
        }
        if (state == 7) {
            return len;
        }
    }
}

static bool balanced(const int* s, int len) {
    int depth = 0;
    for (int i = 0; i < len; i++) {
        depth += s[i] == 0 ? 1 : -1;
        if (depth < 0) {
            return false;
        }
    }
    return depth == 0;
}

void grammar_reset(Task& task) {
    TaskSettings& s = task.settings;
    if (task.kind == TaskKind::grammar) {
        s.length = s.length < 5 ? 5 : (s.length > SCRIPT_MAX - 2 ? SCRIPT_MAX - 2 : s.length);
    } else {
        s.length = s.length < 4 ? 4 : (s.length > SCRIPT_MAX - 2 ? SCRIPT_MAX - 2 : s.length);
        s.length -= s.length % 2;
    }
}

TaskShape grammar_shape(const Task& task) {
    TaskShape shape;
    shape.singles = task.kind == TaskKind::grammar ? REBER_SYMBOLS + 1 : 3;
    shape.motors = 2;
    return shape;
}

void grammar_trial(Task& task) {
    WorldRandom& r = task.random;
    int s[SCRIPT_MAX];
    int len = 0, ask = 0;
    bool allowed = world_random_below(r, 2) == 0;
    if (task.kind == TaskKind::grammar) {
        ask = REBER_SYMBOLS;
        len = reber_walk(r, s, task.settings.length);
        while (!allowed) {
            int at = 1 + world_random_below(r, len - 2);
            int was = s[at];
            s[at] = RT + world_random_below(r, RE - RT); // an inner symbol
            if (s[at] != was && !reber_allowed(s, len)) {
                break;
            }
            s[at] = was;
        }
    } else {
        ask = 2;
        len = task.settings.length;
        do {
            // ( first, ) last, the middle a shuffle of the rest
            for (int i = 0; i < len; i++) {
                s[i] = i < len / 2 ? 0 : 1;
            }
            for (int i = len - 2; i > 1; i--) {
                int j = 1 + world_random_below(r, i);
                int swap = s[i];
                s[i] = s[j];
                s[j] = swap;
            }
        } while (balanced(s, len) != allowed);
    }
    script_add(task, 0, -1, 0); // the blank that parts trials
    for (int i = 0; i < len; i++) {
        script_add(task, line_bit(s[i]), -1, 0);
    }
    script_add(task, line_bit(ask), allowed ? 0 : 1,
               SCRIPT_SCORED | SCRIPT_REWARDED | SCRIPT_PROBE);
}
