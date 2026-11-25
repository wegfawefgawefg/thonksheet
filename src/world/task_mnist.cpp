#include "world/mnist_data.hpp"
#include "world/task.hpp"

#include <cstdio>
#include <cstdlib>

// mnist: one handwritten digit per step, shown as a bitmap on the sensor sheet for the whole
// step; ten buttons. good if the right button wins, bad otherwise (a tie or no spikes is a
// wrong answer). digits come in a seeded random order, a fresh order each pass. with
// mnist_test the digits are the 10,000 held-out ones and nothing is rewarded. score: right
// answers over digits. the good and bad lines are left unattached: a step's answer would
// otherwise be on the sheet while the next digit is shown. with mnist_pair two half-size
// digits share the sheet and there are two buttons (mnist_data.hpp): the second digit is
// drawn from the first's place in the order, so each pass pairs the digits afresh.

static const MnistSet& digit_set(const Task& task) {
    const Mnist* m = mnist_data();
    if (!m) {
        std::exit(1); // the loader said why
    }
    return task.settings.mnist_test ? m->test : m->train;
}

// the digits this phase uses: [first, first + count) of the set
static void digit_span(const Task& task, int& first, int& count) {
    int total = digit_set(task).count;
    first = task.settings.mnist_first;
    first = first < 0 ? 0 : (first >= total ? total - 1 : first);
    count = task.settings.mnist_count;
    count = count <= 0 || count > total - first ? total - first : count;
}

static void pick_digit(Task& task) {
    int first = 0, count = 0;
    digit_span(task, first, count);
    int pass = (int)(task.shown / count);
    int at = (int)(task.shown % count);
    task.image = first + mnist_order(at, count, task.order_seed + (uint64_t)pass);
    const MnistSet& set = digit_set(task);
    task.cue = set.labels[(size_t)task.image];
    int pair = task.settings.mnist_pair;
    if (pair != MNIST_PAIR_OFF) {
        uint64_t key =
            task.order_seed ^ ((uint64_t)pass << 40) ^ ((uint64_t)at * 0x2545f4914f6cdd1dull);
        task.partner = mnist_partner(set, first, count, task.image, pair, key);
        task.cue = mnist_pair_answer(task.cue, set.labels[(size_t)task.partner], pair);
    }
}

void mnist_reset(Task& task) {
    TaskSettings& s = task.settings;
    s.mnist_code = s.mnist_code < 0 || s.mnist_code >= MNIST_CODES ? 0 : s.mnist_code;
    task.order_seed = world_random_word(task.random);
    task.shown = 0;
    pick_digit(task);
}

void mnist_restart(Task& task, int test, int first, int count, int dx, int dy) {
    task.settings.mnist_test = test;
    task.settings.mnist_first = first;
    task.settings.mnist_count = count;
    task.settings.mnist_dx = dx;
    task.settings.mnist_dy = dy;
    task.shown = 0;
    for (int i = 0; i < 4; i++) {
        task.tally[i] = 0;
    }
    pick_digit(task);
}

TaskShape mnist_shape(const Task& task) {
    TaskShape shape;
    mnist_code_size(task.settings.mnist_code, shape.sheet_w, shape.sheet_h);
    shape.motors = MNIST_CLASSES;
    if (task.settings.mnist_pair != MNIST_PAIR_OFF) {
        shape.sheet_w = shape.sheet_h = MNIST_SIDE;
        shape.motors = 2;
    }
    shape.uses_reward = false;
    return shape;
}

void mnist_sense(const Task& task, TaskSense& sense) {
    const MnistSet& set = digit_set(task);
    const TaskSettings& s = task.settings;
    if (s.mnist_pair != MNIST_PAIR_OFF) {
        mnist_pair_encode(&set.pixels[(size_t)task.image * MNIST_PIXELS],
                          &set.pixels[(size_t)task.partner * MNIST_PIXELS], s.mnist_mix,
                          sense.sheet);
        return;
    }
    mnist_encode(&set.pixels[(size_t)task.image * MNIST_PIXELS], s.mnist_code, s.mnist_dx,
                 s.mnist_dy, sense.sheet);
}

int mnist_perfect_move(const Task& task) {
    return task.cue;
}

int mnist_step(Task& task, const TaskStepIn& in) {
    bool right = in.move == task.cue;
    task.tally[1]++;
    task.tally[0] += right;
    task.shown++;
    pick_digit(task);
    if (task.settings.mnist_test) {
        return 0;
    }
    return right ? 1 : -1;
}
