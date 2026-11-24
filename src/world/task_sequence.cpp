#include "world/task.hpp"

// stage 2, task 6: a repeating pattern plays on `seq_lines` lines, one pattern step per task
// step. no motors, no reward. the score is how many attached sensor neurons the blob
// predicted right on the first tick of each step, when the input changes. each pattern step
// line is on in half the pattern steps, so predicting all-off, all-on or coin flips scores
// 0.5, "same as last step" scores 0.5 or less, and knowing the pattern scores 1.

void sequence_reset(Task& task) {
    if (task.settings.seq_lines < 1) {
        task.settings.seq_lines = 1;
    }
    if (task.settings.seq_lines > TASK_MAX_SINGLES) {
        task.settings.seq_lines = TASK_MAX_SINGLES;
    }
    if (task.settings.seq_length < 2) {
        task.settings.seq_length = 2;
    }
    if (task.settings.seq_length > TASK_MAX_PATTERN) {
        task.settings.seq_length = TASK_MAX_PATTERN;
    }
    // each line is on in exactly half the pattern steps (rounded), in a drawn order, so no
    // line is constant and no fixed guess beats chance
    int lines = task.settings.seq_lines;
    int length = task.settings.seq_length;
    int order[TASK_MAX_PATTERN];
    for (int line = 0; line < lines; line++) {
        for (int s = 0; s < length; s++) {
            order[s] = s;
        }
        for (int s = length - 1; s > 0; s--) {
            int j = world_random_below(task.random, s + 1);
            int swap = order[s];
            order[s] = order[j];
            order[j] = swap;
        }
        for (int s = 0; s < length / 2; s++) {
            task.pattern[order[s]] |= 1u << line;
        }
    }
}

TaskShape sequence_shape(const Task& task) {
    TaskShape shape;
    shape.singles = task.settings.seq_lines;
    shape.motors = 0;
    shape.uses_reward = false;
    return shape;
}

void sequence_sense(const Task& task, TaskSense& sense) {
    uint32_t bits = task.pattern[task.steps % task.settings.seq_length];
    for (int line = 0; line < task.settings.seq_lines; line++) {
        sense.singles[line] = (bits >> line) & 1;
    }
}

int sequence_step(Task& task, const TaskStepIn& in) {
    int wrong = in.surprise < in.sensed_neurons ? in.surprise : in.sensed_neurons;
    task.tally[0] += in.sensed_neurons - wrong;
    task.tally[1] += in.sensed_neurons;
    return 0;
}
