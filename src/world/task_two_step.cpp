#include "world/task.hpp"

// two-step: a trial is two presses. step 0 shows one of two cue lines; step 1 shows only a
// "second" line. right is: the cue's button first, then the other button. nothing is said
// after the first press; one good or bad comes after the second, for the pair. so the
// first press is credited late, and the second has to remember the cue (or the first press).
// no move on both presses is neither good nor bad; it still counts as a miss.

constexpr int TWO_STEP_SECOND_LINE = 2;

void two_step_reset(Task& task) {
    task.phase = 0;
    task.first_move = -1;
    task.cue = world_random_below(task.random, 2);
}

TaskShape two_step_shape(const Task&) {
    TaskShape shape;
    shape.singles = 3; // cue 0, cue 1, second
    shape.motors = 2;
    return shape;
}

void two_step_sense(const Task& task, TaskSense& sense) {
    if (task.phase == 0) {
        sense.singles[task.cue] = 1;
    } else {
        sense.singles[TWO_STEP_SECOND_LINE] = 1;
    }
}

int two_step_perfect_move(const Task& task) {
    return task.phase == 0 ? task.cue : 1 - task.cue;
}

int two_step_step(Task& task, const TaskStepIn& in) {
    if (task.phase == 0) {
        task.first_move = in.move;
        task.phase = 1;
        return 0;
    }
    bool right = task.first_move == task.cue && in.move == 1 - task.cue;
    bool idle = task.first_move < 0 && in.move < 0;
    task.tally[1]++;
    task.tally[0] += right;
    task.phase = 0;
    task.first_move = -1;
    task.cue = world_random_below(task.random, 2);
    return right ? 1 : (idle ? 0 : -1);
}
