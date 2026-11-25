#include "world/task_function_parts.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// the function battery: one trial is a blank step, the steps that show the input, and an
// answer step with the ask line on (the only step whose press counts). the right button is
// good, another bad, no move a miss. inputs are drawn uniformly when that already gives
// every answer equally often; otherwise an answer is drawn first, uniformly among those
// some input has, then an input with that answer, uniformly. points with N = 2 and parity
// is both-not-either exactly: the same lines, the same draws, the same csv.

static const char* const fn_names[(int)FnName::none + 1] = {
    "and",   "or",    "parity",   "majority", "exactly", "table", "table-lin", "table-nonlin",
    "count", "first", "identity", "step",     "tent",    "sine",  "cosine",    "square",
    "sum",   "diff",  "larger",   "product",  "compare", "same",  "recall",    "none",
};

const char* fn_name(FnName fn) {
    return fn_names[(int)fn];
}

bool fn_from_name(const char* name, FnName& fn) {
    for (int i = 0; i < (int)FnName::none; i++) {
        if (std::strcmp(name, fn_names[i]) == 0) {
            fn = (FnName)i;
            return true;
        }
    }
    return false;
}

// which functions each kind takes; the first is its default
static bool fn_in_kind(TaskKind kind, FnName fn) {
    int f = (int)fn;
    switch (kind) {
    case TaskKind::points:
        return f <= (int)FnName::first;
    case TaskKind::number:
        return f >= (int)FnName::identity && f <= (int)FnName::square;
    case TaskKind::two_numbers:
        return f >= (int)FnName::sum && f <= (int)FnName::compare;
    case TaskKind::past:
        return fn == FnName::same || fn == FnName::parity || fn == FnName::majority ||
               fn == FnName::recall;
    default:
        return false;
    }
}

static FnName fn_default(TaskKind kind) {
    switch (kind) {
    case TaskKind::points:
        return FnName::parity;
    case TaskKind::number:
        return FnName::identity;
    case TaskKind::two_numbers:
        return FnName::sum;
    default:
        return FnName::same;
    }
}

static bool is_function_kind(TaskKind kind) {
    return kind == TaskKind::points || kind == TaskKind::number || kind == TaskKind::two_numbers ||
           kind == TaskKind::past;
}

static bool fail(const char* text) {
    std::fprintf(stderr, "blobrun: %s\n", text);
    return false;
}

bool function_settings_ok(TaskKind kind, const TaskSettings& settings) {
    if (!is_function_kind(kind)) {
        return true;
    }
    const FnSettings& s = settings.fn;
    FnName fn = s.fn == FnName::none ? fn_default(kind) : s.fn;
    if (!fn_in_kind(kind, fn)) {
        std::fprintf(stderr, "blobrun: task %s has no function '%s'\n", task_name(kind),
                     fn_name(fn));
        return false;
    }
    bool number_kind = kind == TaskKind::number || kind == TaskKind::two_numbers;
    if (kind == TaskKind::points && (s.lines < 1 || s.lines > 10)) {
        return fail("points: --fn-lines is 1 to 10");
    }
    if (kind == TaskKind::points && fn == FnName::exactly && (s.k < 0 || s.k > s.lines)) {
        return fail("points: --fn-k is 0 to the number of lines");
    }
    if (kind == TaskKind::number && (s.lines < 2 || s.lines > 64)) {
        return fail("number: --fn-lines is 2 to 64");
    }
    if (kind == TaskKind::two_numbers && (s.lines < 2 || s.lines > 32)) {
        return fail("two-numbers: --fn-lines is 2 to 32");
    }
    if (number_kind && (s.bands < 2 || s.bands > FN_MAX_CLASSES - 1)) {
        return fail("number kinds: --fn-bands is 2 to 16");
    }
    if (s.hold < 0 || (s.hold > 0 && kind != TaskKind::number)) {
        return fail("--fn-hold is for --task number only");
    }
    if (kind == TaskKind::past) {
        int items = fn == FnName::same || fn == FnName::recall ? 2 : s.items;
        if (items < 2 || items > 4) {
            return fail("past: --fn-items is 2 to 4");
        }
        if (s.gap < 0 || 1 + items + (items - 1) * s.gap > TASK_MAX_TRIAL) {
            return fail("past: --fn-gap too long for a trial of 16 steps");
        }
    }
    return true;
}

static bool read_whole(const char* value, int& out) {
    char* end = nullptr;
    long v = std::strtol(value, &end, 10);
    if (end == value || *end != '\0') {
        std::fprintf(stderr, "blobrun: '%s' is not a whole number\n", value);
        return false;
    }
    out = (int)v;
    return true;
}

bool function_flag(const char* flag, const char* value, TaskSettings& settings, bool& ok) {
    FnSettings& s = settings.fn;
    ok = true;
    if (std::strcmp(flag, "--fn") == 0) {
        ok = fn_from_name(value, s.fn);
        if (!ok) {
            std::fprintf(stderr, "blobrun: no function '%s'\n", value);
        }
        return true;
    }
    static const char* const names[] = {"--fn-lines", "--fn-bands", "--fn-k",
                                        "--fn-hold",  "--fn-gap",   "--fn-items"};
    int* fields[] = {&s.lines, &s.bands, &s.k, &s.hold, &s.gap, &s.items};
    for (int i = 0; i < 6; i++) {
        if (std::strcmp(flag, names[i]) == 0) {
            ok = read_whole(value, *fields[i]);
            return true;
        }
    }
    return false;
}

// --- drawing ---

static uint32_t bit(int line) {
    return 1u << line;
}

// classes known: sort inputs by class, and see whether uniform inputs are already balanced
static void sort_members(FnState& f) {
    int count[FN_MAX_CLASSES] = {};
    bool excluded = false;
    for (int x = 0; x < f.inputs; x++) {
        if (f.cls[x] == FN_NO_CLASS) {
            excluded = true;
        } else {
            count[f.cls[x]]++;
        }
    }
    f.first[0] = 0;
    for (int c = 0; c < f.classes; c++) {
        f.first[c + 1] = f.first[c] + count[c];
    }
    int at[FN_MAX_CLASSES];
    for (int c = 0; c < f.classes; c++) {
        at[c] = f.first[c];
    }
    for (int x = 0; x < f.inputs; x++) {
        if (f.cls[x] != FN_NO_CLASS) {
            f.member[at[f.cls[x]]++] = (uint16_t)x;
        }
    }
    f.uniform = !excluded;
    int size = 0;
    for (int c = 0; c < f.classes; c++) {
        if (count[c] > 0 && size > 0 && count[c] != size) {
            f.uniform = false;
        }
        size = count[c] > 0 ? count[c] : size;
    }
}

static int draw_input(Task& task) {
    FnState& f = task.fn;
    WorldRandom& r = task.random;
    if (f.uniform) {
        // points and past: a coin per line or item, in order (as both-not-either draws)
        if (task.kind == TaskKind::points || task.kind == TaskKind::past) {
            int bits = 0;
            while ((1 << bits) < f.inputs) {
                bits++;
            }
            int x = 0;
            for (int i = 0; i < bits; i++) {
                x |= world_random_below(r, 2) << i;
            }
            return x;
        }
        return world_random_below(r, f.inputs);
    }
    int nonempty = 0;
    for (int c = 0; c < f.classes; c++) {
        nonempty += f.first[c + 1] > f.first[c];
    }
    int pick = world_random_below(r, nonempty);
    for (int c = 0; c < f.classes; c++) {
        int size = f.first[c + 1] - f.first[c];
        if (size > 0 && pick-- == 0) {
            return f.member[f.first[c] + world_random_below(r, size)];
        }
    }
    return f.member[0];
}

static int past_items(const Task& task) {
    FnName fn = task.settings.fn.fn;
    return fn == FnName::same || fn == FnName::recall ? 2 : task.settings.fn.items;
}

static void new_trial(Task& task) {
    FnState& f = task.fn;
    const FnSettings& s = task.settings.fn;
    uint32_t* steps = task.trial_lines;
    for (int i = 0; i < TASK_MAX_TRIAL; i++) {
        steps[i] = 0;
    }
    task.phase = 0;
    f.x = draw_input(task);
    task.cue = f.cls[f.x];
    task.scored = task.kind != TaskKind::number || s.hold == 0 || ((task.held_out >> f.x) & 1);
    int n = 1; // step 0 is the blank that parts trials
    switch (task.kind) {
    case TaskKind::points:
        steps[n++] = (uint32_t)f.x | bit(s.lines); // the cue lines with the ask after them
        break;
    case TaskKind::number:
    case TaskKind::two_numbers:
        steps[n++] = bit(0); // the ask; the sheet shows the value(s) on this step
        break;
    case TaskKind::past: {
        int items = past_items(task);
        for (int j = 0; j < items; j++) {
            n += j > 0 ? s.gap : 0;
            steps[n++] = bit((f.x >> j) & 1 ? FN_PAST_B : FN_PAST_A);
        }
        steps[n - 1] |= bit(FN_PAST_ASK);
        break;
    }
    default:
        break;
    }
    task.trial_len = n;
}

void function_reset(Task& task) {
    FnSettings& s = task.settings.fn;
    if (s.fn == FnName::none || !fn_in_kind(task.kind, s.fn)) {
        s.fn = fn_default(task.kind);
    }
    if (task.kind == TaskKind::past) {
        s.items = past_items(task);
    }
    switch (task.kind) {
    case TaskKind::points:
        points_classes(task);
        break;
    case TaskKind::number:
        number_classes(task);
        break;
    case TaskKind::two_numbers:
        pair_classes(task);
        break;
    case TaskKind::past:
        past_classes(task);
        break;
    default:
        break;
    }
    sort_members(task.fn);
    if (task.kind == TaskKind::number && s.hold > 0) {
        number_hold(task);
    }
    new_trial(task);
}

TaskShape function_shape(const Task& task) {
    TaskShape shape;
    shape.motors = task.fn.classes;
    switch (task.kind) {
    case TaskKind::points:
        shape.singles = task.settings.fn.lines + 1; // the cue lines, then the ask
        break;
    case TaskKind::past:
        shape.singles = 3; // A, B, ask
        break;
    case TaskKind::number:
    case TaskKind::two_numbers:
        shape.singles = 1; // the ask
        shape.sheet_w = number_sheet_w(task);
        shape.sheet_h = number_sheet_h(task);
        break;
    default:
        break;
    }
    return shape;
}

void function_sense(const Task& task, TaskSense& sense) {
    uint32_t lines = task.trial_lines[task.phase];
    for (int i = 0; i < TASK_MAX_SINGLES; i++) {
        sense.singles[i] = (lines >> i) & 1;
    }
    bool answering = task.phase == task.trial_len - 1;
    if (answering && (task.kind == TaskKind::number || task.kind == TaskKind::two_numbers)) {
        number_sheet(task, sense);
    }
}

int function_perfect_move(const Task& task) {
    return task.phase == task.trial_len - 1 ? task.cue : -1;
}

int function_step(Task& task, const TaskStepIn& in) {
    if (task.phase < task.trial_len - 1) {
        task.phase++; // presses before the ask are ignored
        return 0;
    }
    bool right = in.move == task.cue;
    int valence = in.move < 0 ? 0 : (right ? 1 : -1);
    if (task.scored) {
        task.tally[1]++;
        task.tally[0] += right;
    } else {
        // number with held-out values: a taught value, rewarded, counted apart
        task.tally[3]++;
        task.tally[2] += right;
    }
    // held-out values are never rewarded
    if (task.kind == TaskKind::number && task.settings.fn.hold > 0 && task.scored) {
        valence = 0;
    }
    new_trial(task);
    return valence;
}
