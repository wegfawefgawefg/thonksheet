#pragma once

#include "world/world_random.hpp"

#include <cstdint>

// Tasks from docs/curriculum.md. A task never sees the blob: it fills a TaskSense each step,
// takes the move the plug read off the motor lines, and answers good (+1), bad (-1) or
// nothing (0). Each task keeps its own score per window of steps.

enum class TaskKind {
    echo,            // one of k cues is on; press the matching button
    shuffled_echo,   // the same, with the cue-to-button match drawn per life
    delayed_echo,    // cue, a few blank steps, then an ask line; answer the cue
    go_no_go,        // press on the go cue, hold still on the no-go cue
    beat,            // press every `period` steps, no cue
    sequence,        // a repeating pattern on the lines, no reward; score is prediction
    catch_dot,       // move a paddle under a falling dot
    cartpole,        // balance a pole on a cart
    order,           // two cues come on one after the other; answer which came first
    both_not_either, // two cues; one button when exactly one is on, the other otherwise
    gap,             // one cue blinks twice; answer whether the gap was short or long
    place,           // a small shape at one of several places; answer which shape
    mnist,           // a handwritten digit on the sheet; press its button (task_mnist.cpp)
    count,
};

constexpr int TASK_MAX_SINGLES = 32;
constexpr int TASK_MAX_SHEET = 1024; // mnist's 28x28
constexpr int TASK_MAX_NUMBERS = 4;
constexpr int TASK_MAX_MOTORS = 16;
constexpr int TASK_MAX_PATTERN = 64;
constexpr int TASK_MAX_TRIAL = 16; // steps in one trial of a feature task

// what a task needs plugged in
struct TaskShape {
    int singles = 0;              // on/off lines
    int sheet_w = 0, sheet_h = 0; // a bitmap, kept as a sheet
    int numbers = 0;              // numbers turned into threshold lines
    int number_lines = 0;         // lines per number
    int32_t number_lo[TASK_MAX_NUMBERS] = {};
    int32_t number_hi[TASK_MAX_NUMBERS] = {};
    int motors = 0;
    bool uses_reward = true; // false leaves the good and bad lines unattached
};

// one step's input as the task sees it
struct TaskSense {
    uint8_t singles[TASK_MAX_SINGLES] = {};
    uint8_t sheet[TASK_MAX_SHEET] = {}; // row-major, sheet_w * sheet_h
    int32_t numbers[TASK_MAX_NUMBERS] = {};
};

// what comes back from the blob's side after one step
struct TaskStepIn {
    int move = -1;          // motor line that won, -1 for no move
    int surprise = 0;       // wrong sensor predictions on the step's first tick
    int sensed_neurons = 0; // attached sensor neurons that surprise is counted over
};

struct TaskSettings {
    int k = 2;      // echo family: number of cues and buttons
    int delay = 2;  // delayed echo: blank steps between cue and ask
    int period = 4; // beat: press every this many steps
    int seq_lines = 8;
    int seq_length = 4; // steps before the sequence repeats
    int catch_w = 6, catch_h = 6;
    int cue_offset = 0; // echo family: cues and buttons start at this line; lines before stay off
    int order_gap = 1;  // order: steps the first cue is on alone before the second joins it
    int gap_short = 1, gap_long = 3; // gap: blank steps between the two blinks
    int place_size = 4;              // place: the sheet is size x size lines
    int place_hold = 3;              // place: places never rewarded; only these are scored
    // cartpole reward: 0 sparse (good every 25 steps up, bad on a fall), 1 alive (good on
    // every step up, bad on a fall), 2 lean (every step: good if the pole and cart ended
    // nearer upright-and-centred than they began or are already there, bad if further)
    int cart_reward = 0;
    // mnist: how a digit becomes bits (MnistCode in mnist_data.hpp); the held-out test
    // digits, unrewarded; digits [first, first + count) of the set (count 0: the rest); every
    // digit moved by (dx, dy) pixels
    int mnist_code = 0;
    int mnist_test = 0;
    int mnist_first = 0, mnist_count = 0;
    int mnist_dx = 0, mnist_dy = 0;
};

// fixed point for cartpole: 16 fractional bits
constexpr int64_t FIX_ONE = 1 << 16;

struct Task {
    TaskKind kind = TaskKind::echo;
    TaskSettings settings;
    WorldRandom random;
    int64_t steps = 0; // steps taken over the whole life

    // echo family and go/no-go
    int cue = 0;
    int phase = 0; // delayed echo: 0 cue, 1..delay blank, delay + 1 ask
    int button_of[TASK_MAX_MOTORS] = {};

    // sequence: one bit per line per pattern step
    uint32_t pattern[TASK_MAX_PATTERN] = {};

    // catch
    int dot_x = 0, dot_y = 0, paddle_x = 0;

    // cartpole, all in 16.16 fixed point
    int64_t cart_x = 0, cart_v = 0, pole_a = 0, pole_w = 0;
    int64_t alive = 0; // steps since the last fall

    // feature tasks: one trial is a short script, the singles on at each of its steps; the
    // last step is the one answered
    int trial_len = 0;
    uint32_t trial_lines[TASK_MAX_TRIAL] = {};
    int shape = 0, place = 0; // place: which shape, where (index into the places)
    uint64_t held_out = 0;    // place: one bit per place never rewarded
    bool scored = true;       // place: this trial is on a held-out place

    // window score, meaning depends on the kind (see task_window_score)
    int64_t tally[4] = {};

    // mnist: digits shown since the last restart, the order's seed, the digit on the sheet
    int64_t shown = 0;
    uint64_t order_seed = 0;
    int image = 0;
};

const char* task_name(TaskKind kind);
bool task_from_name(const char* name, TaskKind& kind);

// a fresh task for a new life. the blob is not touched.
void task_reset(Task& task, TaskKind kind, const TaskSettings& settings, uint64_t seed);
TaskShape task_shape(const Task& task);
void task_sense(const Task& task, TaskSense& sense);
// the right move for the current step, for the scripted perfect player (-1 = hold still)
int task_perfect_move(const Task& task);
// apply the move, advance one step, return +1, -1 or 0
int task_step(Task& task, const TaskStepIn& in);
// score over the steps since the last call, then start a new window
double task_window_score(Task& task);

// per-kind halves, in task_reflex.cpp, task_sequence.cpp, task_control.cpp and
// task_feature.cpp
void reflex_reset(Task& task);
TaskShape reflex_shape(const Task& task);
void reflex_sense(const Task& task, TaskSense& sense);
int reflex_perfect_move(const Task& task);
int reflex_step(Task& task, const TaskStepIn& in);

void sequence_reset(Task& task);
TaskShape sequence_shape(const Task& task);
void sequence_sense(const Task& task, TaskSense& sense);
int sequence_step(Task& task, const TaskStepIn& in);

void catch_reset(Task& task);
TaskShape catch_shape(const Task& task);
void catch_sense(const Task& task, TaskSense& sense);
int catch_perfect_move(const Task& task);
int catch_step(Task& task, const TaskStepIn& in);

void cartpole_reset(Task& task);
TaskShape cartpole_shape(const Task& task);
void cartpole_sense(const Task& task, TaskSense& sense);
int cartpole_perfect_move(const Task& task);
int cartpole_step(Task& task, const TaskStepIn& in);

void feature_reset(Task& task);
TaskShape feature_shape(const Task& task);
void feature_sense(const Task& task, TaskSense& sense);
int feature_perfect_move(const Task& task);
int feature_step(Task& task, const TaskStepIn& in);

void mnist_reset(Task& task);
TaskShape mnist_shape(const Task& task);
void mnist_sense(const Task& task, TaskSense& sense);
int mnist_perfect_move(const Task& task);
int mnist_step(Task& task, const TaskStepIn& in);
// a new phase on the same task: train or test digits, which of them, moved how; the order
// and the tallies start over
void mnist_restart(Task& task, int test, int first, int count, int dx, int dy);
