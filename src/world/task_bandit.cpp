#include "world/task.hpp"

// more worlds (idea 63), the simplest reinforcement problems: no cue tells the answer.
//
// bandit         k buttons and one line that is always on (it carries nothing). one button,
//                drawn per life, pays good bandit_good% of the time and bad otherwise; the
//                others pay good bandit_poor% of the time. no move is neither. score: share
//                of steps the best button was pressed.
// switch-bandit  the same, but every switch_every steps the paying button moves to another
//                one. score: share of steps on the best button, over the second half of
//                each block only (has it let go of the old answer and found the new one).
// chain          k buttons; A and B (different, drawn per life). stage 0: pressing A moves
//                to stage 1, anything else stays; no reward either way. stage 1: B is good,
//                another button bad, no move nothing; then back to stage 0. so the first
//                press is only ever paid one step later. chain_cue 1: two lines show the
//                stage; 0: one line always on and the stage must be remembered. score:
//                chains paid x 2 / steps (1.0 is A, B, A, B, ...).

static int clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// a percent chance, in integers
static bool chance(Task& task, int percent) {
    return world_random_below(task.random, 100) < percent;
}

static void new_best(Task& task) {
    int k = task.settings.k;
    if (task.kind == TaskKind::switch_bandit && task.steps > 0) {
        // another button than the one paying now
        task.best = (task.best + 1 + world_random_below(task.random, k - 1)) % k;
        return;
    }
    task.best = world_random_below(task.random, k);
}

void bandit_reset(Task& task) {
    TaskSettings& s = task.settings;
    s.k = clamp(s.k, 2, TASK_MAX_MOTORS);
    s.bandit_good = clamp(s.bandit_good, 0, 100);
    s.bandit_poor = clamp(s.bandit_poor, 0, 100);
    s.switch_every = s.switch_every == 0 ? 1000 : (s.switch_every < 2 ? 2 : s.switch_every);
    task.timer = 0;
    task.phase = 0;
    if (task.kind == TaskKind::chain) {
        // A and B, two different buttons
        task.button_of[0] = world_random_below(task.random, s.k);
        task.button_of[1] =
            (task.button_of[0] + 1 + world_random_below(task.random, s.k - 1)) % s.k;
        return;
    }
    new_best(task);
}

TaskShape bandit_shape(const Task& task) {
    TaskShape shape;
    shape.motors = task.settings.k;
    // the chain's two stage lines, or one line that is always on
    shape.singles = task.kind == TaskKind::chain && task.settings.chain_cue ? 2 : 1;
    return shape;
}

void bandit_sense(const Task& task, TaskSense& sense) {
    if (task.kind == TaskKind::chain && task.settings.chain_cue) {
        sense.singles[task.phase] = 1;
        return;
    }
    sense.singles[0] = 1;
}

int bandit_perfect_move(const Task& task) {
    if (task.kind == TaskKind::chain) {
        return task.button_of[task.phase];
    }
    return task.best;
}

static int pull(Task& task, int move) {
    if (move < 0) {
        return 0;
    }
    task.episodes += task.kind == TaskKind::bandit;
    bool paid =
        chance(task, move == task.best ? task.settings.bandit_good : task.settings.bandit_poor);
    task.wins += paid && task.kind == TaskKind::bandit;
    return paid ? 1 : -1;
}

static int chain_step(Task& task, int move) {
    task.tally[1]++;
    if (task.phase == 0) {
        task.phase = move == task.button_of[0] ? 1 : 0;
        return 0;
    }
    task.phase = 0;
    if (move < 0) {
        return 0;
    }
    if (move == task.button_of[1]) {
        task.tally[0] += 2;
        task.episodes++;
        return 1;
    }
    return -1;
}

int bandit_step(Task& task, const TaskStepIn& in) {
    if (task.kind == TaskKind::chain) {
        return chain_step(task, in.move);
    }
    // switching: only the second half of each block is scored
    bool scored = task.kind == TaskKind::bandit || task.timer >= task.settings.switch_every / 2;
    if (scored) {
        task.tally[1]++;
        task.tally[0] += in.move == task.best;
    }
    int valence = pull(task, in.move);
    if (task.kind == TaskKind::switch_bandit && ++task.timer >= task.settings.switch_every) {
        task.timer = 0;
        task.episodes++;
        new_best(task);
    }
    return valence;
}
