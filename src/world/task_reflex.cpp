#include "world/task.hpp"

// stage 1: echo, shuffled echo, delayed echo, go/no-go, beat. one step is one chance to
// answer except in delayed echo, where a trial is cue, `delay` blank steps, then ask.

static int clamp_k(int k) {
    if (k < 2) {
        return 2;
    }
    return k > TASK_MAX_MOTORS ? TASK_MAX_MOTORS : k;
}

// lines before the first cue line; only the echo family uses an offset
static int cue_offset(const Task& task) {
    bool echo_family = task.kind == TaskKind::echo || task.kind == TaskKind::shuffled_echo ||
                       task.kind == TaskKind::delayed_echo;
    return echo_family ? task.settings.cue_offset : 0;
}

static void draw_cue(Task& task) {
    int cues = task.kind == TaskKind::go_no_go ? 2 : task.settings.k;
    task.cue = world_random_below(task.random, cues);
}

void reflex_reset(Task& task) {
    task.settings.k = clamp_k(task.settings.k);
    if (task.settings.delay < 0) {
        task.settings.delay = 0;
    }
    if (task.settings.period < 2) {
        task.settings.period = 2;
    }
    int k = task.settings.k;
    // echo family: cues and buttons may start past some lines that stay off (a second task
    // beside a first one, in the retention test)
    int offset = cue_offset(task);
    if (task.settings.cue_offset < 0 || offset + k + 1 > TASK_MAX_SINGLES) {
        task.settings.cue_offset = 0;
        offset = 0;
    }
    for (int i = 0; i < k; i++) {
        task.button_of[i] = offset + i;
    }
    // shuffled echo: which button answers which cue is drawn once per life
    if (task.kind == TaskKind::shuffled_echo) {
        for (int i = k - 1; i > 0; i--) {
            int j = world_random_below(task.random, i + 1);
            int swap = task.button_of[i];
            task.button_of[i] = task.button_of[j];
            task.button_of[j] = swap;
        }
    }
    task.phase = 0;
    draw_cue(task);
}

TaskShape reflex_shape(const Task& task) {
    TaskShape shape;
    int k = task.settings.k;
    int offset = cue_offset(task);
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
        shape.singles = offset + k;
        shape.motors = offset + k;
        break;
    case TaskKind::delayed_echo:
        shape.singles = offset + k + 1; // the cues, then the ask line
        shape.motors = offset + k;
        break;
    case TaskKind::go_no_go:
        shape.singles = 2; // go, no-go
        shape.motors = 2;  // press, rest
        break;
    case TaskKind::beat:
        shape.singles = 0;
        shape.motors = 2; // press, rest
        break;
    default:
        break;
    }
    return shape;
}

void reflex_sense(const Task& task, TaskSense& sense) {
    int offset = cue_offset(task);
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
    case TaskKind::go_no_go:
        sense.singles[offset + task.cue] = 1;
        break;
    case TaskKind::delayed_echo:
        if (task.phase == 0) {
            sense.singles[offset + task.cue] = 1;
        } else if (task.phase == task.settings.delay + 1) {
            sense.singles[offset + task.settings.k] = 1;
        }
        break;
    default:
        break;
    }
}

static bool beat_due(const Task& task) {
    return task.steps % task.settings.period == task.settings.period - 1;
}

int reflex_perfect_move(const Task& task) {
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
        return task.button_of[task.cue];
    case TaskKind::delayed_echo:
        return task.phase == task.settings.delay + 1 ? task.button_of[task.cue] : -1;
    case TaskKind::go_no_go:
        return task.cue == 0 ? 0 : -1;
    case TaskKind::beat:
        return beat_due(task) ? 0 : -1;
    default:
        return -1;
    }
}

// one answer to a cue: the matching button is good, another button is bad, no move is
// neither but still counts as a miss
static int judge_answer(Task& task, int move) {
    task.tally[1]++;
    if (move < 0) {
        return 0;
    }
    if (move == task.button_of[task.cue]) {
        task.tally[0]++;
        return 1;
    }
    return -1;
}

static int go_no_go_step(Task& task, int move) {
    bool press = move == 0;
    bool go = task.cue == 0;
    task.tally[1]++;
    if (press == go) {
        task.tally[0]++;
        return 1;
    }
    return -1;
}

static int beat_step(Task& task, int move) {
    bool press = move == 0;
    if (beat_due(task)) {
        task.tally[1]++;
        if (press) {
            task.tally[0]++;
            return 1;
        }
        return -1;
    }
    task.tally[3]++;
    if (!press) {
        task.tally[2]++;
        return 0;
    }
    return -1;
}

int reflex_step(Task& task, const TaskStepIn& in) {
    int valence = 0;
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
        valence = judge_answer(task, in.move);
        draw_cue(task);
        break;
    case TaskKind::delayed_echo:
        // presses before the ask line are ignored
        if (task.phase == task.settings.delay + 1) {
            valence = judge_answer(task, in.move);
            draw_cue(task);
            task.phase = 0;
        } else {
            task.phase++;
        }
        break;
    case TaskKind::go_no_go:
        valence = go_no_go_step(task, in.move);
        draw_cue(task);
        break;
    case TaskKind::beat:
        valence = beat_step(task, in.move);
        break;
    default:
        break;
    }
    return valence;
}
