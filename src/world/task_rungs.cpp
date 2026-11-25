#include "world/task.hpp"

// the first rungs of acting, for the actor-critic job, written to the same definitions as
// idea 63's bandit and chain (another branch); drop this file for those when they land.
//
// bandit  k buttons and one line that is always on (it carries nothing). one button, drawn
//         per life, pays good bandit_good% of the time and bad otherwise; the others pay
//         good bandit_poor% of the time. no move is neither. score: share of steps the best
//         button was pressed.
// chain   k buttons; A and B (different, drawn per life). stage 0: pressing A moves to
//         stage 1, anything else stays; no reward either way. stage 1: B is good, another
//         button bad, no move nothing; then back to stage 0. so the first press is only
//         ever paid one step later. two lines show the stage. score: chains paid x 2 /
//         steps (1.0 is A, B, A, B, ...).

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void rung_reset(Task& task) {
    TaskSettings& s = task.settings;
    s.k = clamp_int(s.k, 2, TASK_MAX_MOTORS);
    s.bandit_good = clamp_int(s.bandit_good, 0, 100);
    s.bandit_poor = clamp_int(s.bandit_poor, 0, 100);
    task.phase = 0;
    // the bandit's best arm is button_of[0]; the chain's A and B are button_of[0] and [1]
    task.button_of[0] = world_random_below(task.random, s.k);
    task.button_of[1] = (task.button_of[0] + 1 + world_random_below(task.random, s.k - 1)) % s.k;
}

TaskShape rung_shape(const Task& task) {
    TaskShape shape;
    shape.motors = task.settings.k;
    shape.singles = task.kind == TaskKind::chain ? 2 : 1;
    return shape;
}

void rung_sense(const Task& task, TaskSense& sense) {
    sense.singles[task.kind == TaskKind::chain ? task.phase : 0] = 1;
}

int rung_perfect_move(const Task& task) {
    return task.button_of[task.kind == TaskKind::chain ? task.phase : 0];
}

int rung_step(Task& task, const TaskStepIn& in) {
    task.tally[1]++;
    if (task.kind == TaskKind::bandit) {
        bool best = in.move == task.button_of[0];
        task.tally[0] += best;
        if (in.move < 0) {
            return 0;
        }
        int percent = best ? task.settings.bandit_good : task.settings.bandit_poor;
        return world_random_below(task.random, 100) < percent ? 1 : -1;
    }
    if (task.phase == 0) {
        task.phase = in.move == task.button_of[0] ? 1 : 0;
        return 0;
    }
    task.phase = 0;
    if (in.move < 0) {
        return 0;
    }
    if (in.move == task.button_of[1]) {
        task.tally[0] += 2;
        return 1;
    }
    return -1;
}
