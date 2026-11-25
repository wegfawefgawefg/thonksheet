#include "world/task.hpp"

// three worlds a lookup of the recent past should not pass (idea 135's scoring; idea 149):
//
// distract: delayed echo with distracting events in the gap. a trial is the cue (one of 2
//   lines), then `distract` steps each lighting one of 4 other lines (never the same one
//   twice in a row, so each is an event), then an ask line. answer the cue. presses before
//   the ask are ignored.
// serial-add: two binary numbers arrive a bit pair per step, least significant first, as one
//   of 4 lines (00, 01, 10, 11); a fifth line is on with the first pair of each number, where
//   the carry is 0. every step, press the sum bit (button 0 or 1): a XOR b XOR carry. the
//   carry is a held value that is computed, never shown. a pair is 01 or 10 (the carry passes
//   through) with chance add_bias / 16, so the carry often comes from far back.
// majority: 5 lines each on by chance 1/2, and an ask line always on; press 1 when 3 or more
//   are on. maj_hold of the 32 patterns are drawn once per life and never rewarded; only
//   those are scored (the rest are rewarded and tallied apart), as in place.

constexpr int DISTRACT_LINES = 4;
constexpr int ADD_BITS = 8; // bits per number
constexpr int MAJ_LINES = 5;

static int pair_draw(Task& task) {
    bool pass = (int)world_random_below(task.random, 16) < task.settings.add_bias;
    int r = world_random_below(task.random, 2);
    return pass ? (r ? 1 : 2) : (r ? 3 : 0); // bit 0 is a, bit 1 is b
}

static void majority_draw(Task& task) {
    task.shape = world_random_below(task.random, 1 << MAJ_LINES);
    task.scored = (task.held_out >> task.shape) & 1;
    int on = 0;
    for (int i = 0; i < MAJ_LINES; i++) {
        on += (task.shape >> i) & 1;
    }
    task.cue = on >= 3;
}

static void distract_trial(Task& task) {
    task.phase = 0;
    task.cue = world_random_below(task.random, 2);
    int last = -1;
    int n = task.settings.distract;
    for (int i = 0; i < n && i < TASK_MAX_TRIAL; i++) {
        int d = world_random_below(task.random, DISTRACT_LINES - (last >= 0));
        d += last >= 0 && d >= last;
        task.trial_lines[i] = (uint32_t)d;
        last = d;
    }
    task.trial_len = n + 2;
}

void beyond_reset(Task& task) {
    TaskSettings& s = task.settings;
    s.distract = s.distract < 0 ? 0 : (s.distract > TASK_MAX_TRIAL ? TASK_MAX_TRIAL : s.distract);
    s.add_bias = s.add_bias < 0 ? 0 : (s.add_bias > 16 ? 16 : s.add_bias);
    s.maj_hold = s.maj_hold < 1 ? 1 : (s.maj_hold > 31 ? 31 : s.maj_hold);
    switch (task.kind) {
    case TaskKind::distract:
        distract_trial(task);
        break;
    case TaskKind::serial_add:
        task.phase = 0;
        task.first_move = 0; // the carry into this bit
        task.shape = pair_draw(task);
        break;
    case TaskKind::majority: {
        int left = s.maj_hold;
        while (left > 0) {
            int p = world_random_below(task.random, 1 << MAJ_LINES);
            if (!((task.held_out >> p) & 1)) {
                task.held_out |= 1ull << p;
                left--;
            }
        }
        majority_draw(task);
        break;
    }
    default:
        break;
    }
}

TaskShape beyond_shape(const Task& task) {
    TaskShape shape;
    shape.motors = 2;
    switch (task.kind) {
    case TaskKind::distract:
        shape.singles = 2 + DISTRACT_LINES + 1; // cues, distractors, ask
        break;
    case TaskKind::serial_add:
        shape.singles = 4 + 1; // the pair, start
        break;
    case TaskKind::majority:
        shape.singles = MAJ_LINES + 1; // the lines, ask
        break;
    default:
        break;
    }
    return shape;
}

void beyond_sense(const Task& task, TaskSense& sense) {
    switch (task.kind) {
    case TaskKind::distract:
        if (task.phase == 0) {
            sense.singles[task.cue] = 1;
        } else if (task.phase == task.trial_len - 1) {
            sense.singles[2 + DISTRACT_LINES] = 1;
        } else {
            sense.singles[2 + task.trial_lines[task.phase - 1]] = 1;
        }
        break;
    case TaskKind::serial_add:
        sense.singles[task.shape] = 1;
        sense.singles[4] = task.phase == 0;
        break;
    case TaskKind::majority:
        for (int i = 0; i < MAJ_LINES; i++) {
            sense.singles[i] = (task.shape >> i) & 1;
        }
        sense.singles[MAJ_LINES] = 1;
        break;
    default:
        break;
    }
}

static int sum_bit(const Task& task) {
    int a = task.shape & 1, b = (task.shape >> 1) & 1;
    return a ^ b ^ task.first_move;
}

int beyond_perfect_move(const Task& task) {
    switch (task.kind) {
    case TaskKind::distract:
        return task.phase == task.trial_len - 1 ? task.cue : -1;
    case TaskKind::serial_add:
        return sum_bit(task);
    case TaskKind::majority:
        return task.cue;
    default:
        return -1;
    }
}

static int judge(Task& task, int move, int right, bool scored) {
    bool ok = move == right;
    int64_t* t = task.tally + (scored ? 0 : 2);
    t[1]++;
    t[0] += ok;
    return move < 0 ? 0 : (ok ? 1 : -1);
}

int beyond_step(Task& task, const TaskStepIn& in) {
    switch (task.kind) {
    case TaskKind::distract: {
        if (task.phase < task.trial_len - 1) {
            task.phase++;
            return 0;
        }
        int valence = judge(task, in.move, task.cue, true);
        distract_trial(task);
        return valence;
    }
    case TaskKind::serial_add: {
        int valence = judge(task, in.move, sum_bit(task), true);
        int a = task.shape & 1, b = (task.shape >> 1) & 1;
        task.first_move = (a + b + task.first_move) >= 2;
        task.phase = (task.phase + 1) % ADD_BITS;
        if (task.phase == 0) {
            task.first_move = 0;
        }
        task.shape = pair_draw(task);
        return valence;
    }
    case TaskKind::majority: {
        // held-out patterns are scored and never rewarded
        bool held = task.scored;
        int valence = judge(task, in.move, task.cue, held);
        majority_draw(task);
        return held ? 0 : valence;
    }
    default:
        return 0;
    }
}
