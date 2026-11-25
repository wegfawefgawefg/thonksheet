#pragma once

#include "world/task_function.hpp"
#include "world/task_long.hpp"
#include "world/task_invariance.hpp"
#include "world/task_calc.hpp"
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
    // tiers 9 and 10 of docs/task-map.md (docs/curriculum.md, "The rest of the map"); all run
    // on the trial script in task_script.cpp
    same,         // two items side by side: the same or different
    same_then,    // two items one after the other on the same lines: the same or different
    odd_one,      // three or four items, all alike but one: press the odd one's slot
    bigger,       // two magnitudes: is the first the bigger
    analogy,      // A is to B as C is to which candidate
    how_many,     // how many items are lit on a sheet, wherever and whatever their size
    more_fewer,   // two sheets one after the other: more or fewer items than last time
    grammar,      // a string from a Reber grammar, one symbol a step: allowed or not
    brackets,     // a string of ( and ): balanced or not
    parts,        // a figure made of two smaller ones: answer the pair
    noisy_echo,   // echo where the right answer is rewarded only most of the time
    rare_cue,     // echo where one cue comes only one try in N
    noisy_lines,  // echo on patterns with a share of their lines flipped at random
    context,      // a context line says which of two rules is in force
    switch_rule,  // echo whose rule changes every N tries, with no signal
    copy,         // a sequence of cues, then pressed back in order
    recall,       // a marked cue, other cues in between, then asked for the marked one
    n_back,       // press "match" when this cue equals the one n steps ago
    predict_next, // no teacher: a stream with structure; how well is the next input predicted
    fill_in,      // no teacher: patterns with a part sometimes blanked; is it filled in
    odd_event,    // no teacher: a regular stream with rare breaks; is the break noticed
    points,          // function battery: a boolean function of N cue lines (task_function.hpp)
    number,          // function battery: the band f(x) falls in, x as one lit line of L
    two_numbers,     // function battery: the band of f(x1, x2)
    past,            // function battery: a function of the last two or three items shown
    // more worlds (idea 63): the rungs between echo and cartpole
    bandit,        // no cue; one of k buttons pays good most of the time, drawn per life
    switch_bandit, // the same, and the paying button changes every switch_every steps
    chain,         // press A, then B, to be paid; anything else starts over
    track,         // move a marker along a line onto a target
    forage,        // a small wrapped grid around the animal; walk onto the food
    mountain_car,  // rock a car out of a valley (the classic set)
    pendulum,      // swing a pendulum up and hold it there (the classic set)
    hold,            // long memory (task_long.hpp): a cue held over a long blank gap
    pairs,           // a stream of new pattern-button pairs, old ones re-asked by age
    once,            // a pattern shown once with its answer, asked much later
    rare,            // patterns asked one step in a thousand among common ones
    drift,           // fixed pairs; half change button late in life
    invariance,      // the invariance battery: a shape in versions never rewarded (task_invariance)
    calc,            // the calculation battery: the answer is a pattern (task_calc.hpp)
    count,
};

constexpr int TASK_MAX_SINGLES = 32;
constexpr int TASK_MAX_SHEET = 1024; // mnist's 28x28
constexpr int TASK_MAX_NUMBERS = 4;
constexpr int TASK_MAX_MOTORS = 16;
constexpr int TASK_MAX_PATTERN = 64;
constexpr int TASK_MAX_TRIAL = 16; // steps in one trial of a feature task
constexpr int SCRIPT_MAX = 24;     // steps in one trial of a tier 9 or 10 task
constexpr int SCRIPT_SHEET_WORDS = 4;
constexpr int TASK_MAX_EXTRA = 64; // extra scores per window (calc)

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
    // an answer that is a pattern (idea 134): this many output bits, motors = 2 per bit,
    // line 2b "off" and 2b + 1 "on". 0: one button wins, as before
    int pattern_bits = 0;
};

// one step's input as the task sees it
struct TaskSense {
    uint8_t singles[TASK_MAX_SINGLES] = {};
    uint8_t sheet[TASK_MAX_SHEET] = {}; // row-major, sheet_w * sheet_h
    int32_t numbers[TASK_MAX_NUMBERS] = {};
    // fill-in: single lines shown off whose true value the task knows, one bit per line
    uint32_t hidden = 0, hidden_on = 0;
};

// what comes back from the blob's side after one step
struct TaskStepIn {
    int move = -1;          // motor line that won, -1 for no move
    int surprise = 0;       // wrong sensor predictions on the step's first tick
    int sensed_neurons = 0; // attached sensor neurons that surprise is counted over
    // fill-in: over the step's ticks, the hidden lines' sensor neurons whose own prediction
    // was on, split by the hidden truth (task_ticker.cpp)
    int fill_hit = 0, fill_on = 0, fill_false = 0, fill_off = 0;
    // pattern tasks: each bit's answer, and the bits whose two groups tied (no answer)
    uint64_t pattern = 0;
    uint64_t pattern_tied = 0;
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

    // tiers 9 and 10 (task_script.cpp and the files it lists)
    int item_lines = 4;     // lines per item (relation, noisy lines, fill-in), levels (bigger)
    int item_on = 1;        // lines on per item (relation)
    int hold = 0;           // relation, analogy: items never rewarded; only trials on them score
    int varied = 0;         // how-many, more-fewer: items of mixed size with matched total area
    int length = 8;         // grammar, brackets: string length (most); copy: most cues
    int right_pct = 80;     // noisy echo: chance in 100 that the reward tells the truth
    int rare = 10;          // rare cue: one try in N; odd event: one step in N
    int flip_pct = 10;      // noisy lines: chance in 100 that a line is flipped
    int context_early = 0;  // context: 1 shows the context a step before the cue
    // switch rule: tries between changes (0: 500); switching bandit: steps per block (0: 1000)
    int switch_every = 0;
    int fill = 1;           // recall: other cues between the marked one and the ask
    int back = 1;           // n-back: how far back
    int markov_order = 1;   // predict-next: the next symbol depends on the last 1 or 2
    int diff_max = 0;       // bigger: largest difference shown, 0 any
    FnSettings fn; // the function battery (task_function.hpp)
    // cart_reward 3, relative: good when nearer upright than the recent average
    int cart_start = 0;  // cartpole starts: 0 near upright (+-0.05), 1 wide (task_control.cpp)
    // cartpole: 1 semi-implicit Euler, the default (a push moves the pole this step, so a
    // step's reward judges its own press); 0 the old Euler step (--cart-integrator euler)
    int cart_semi = 1;
    // more worlds (task_bandit.cpp, task_move.cpp, task_classic.cpp)
    int bandit_good = 80, bandit_poor = 20; // percent chance a press pays: best arm, the rest
    int chain_cue = 1;                      // chain: a line shows the stage (0: nothing does)
    int track_size = 5;                     // track: cells on the line
    int track_reward = 0;                   // track: 0 each step nearer/further, 1 arrival only
    int forage_size = 5;                    // forage: the wrapped grid is size x size (odd)
    int forage_warmer = 1;                  // forage: each step nearer is good, further bad
    int classic_reward = 0;                 // mountain car, pendulum: 0 shaped, 1 goal only
    LongSettings longs; // hold, pairs, once, rare, drift (task_long.hpp)
    InvarianceSettings inv; // the invariance battery
    CalcSettings calc; // the calculation battery
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
    FnState fn;               // the function battery

    // more worlds: the paying arm, steps into the block or episode, positions
    int best = 0;
    int64_t timer = 0;
    int marker = 0, target = 0;   // track
    int food_dx = 0, food_dy = 0; // forage: food relative to the animal
    int64_t pos = 0, vel = 0;     // mountain car, pendulum: 16.16 fixed point
    int64_t off_average = 0;      // cartpole relative reward: recent mean distance from home

    // window score, meaning depends on the kind (see task_window_score)
    int64_t tally[4] = {};

    // mnist: digits shown since the last restart, the order's seed, the digit on the sheet
    int64_t shown = 0;
    uint64_t order_seed = 0;
    int image = 0;
    // tiers 9 and 10: one trial as a script of steps (task_script.cpp)
    struct Script {
        int len = 0;
        uint32_t lines[SCRIPT_MAX] = {};
        uint64_t sheet[SCRIPT_MAX][SCRIPT_SHEET_WORDS] = {};
        int8_t want[SCRIPT_MAX] = {}; // the right button, -1: presses ignored
        uint8_t flags[SCRIPT_MAX] = {};
        uint32_t hidden[SCRIPT_MAX] = {}, hidden_on[SCRIPT_MAX] = {};
    } script;
    int64_t probe_marks = 0; // steps an offline reader should keep (blobprobe linear)
    int64_t extra[4] = {};   // per-window counts some kinds keep apart from tally
    uint64_t held_items = 0; // relation: one bit per item never rewarded
    int item_pool[64] = {};  // relation: the items (line bits), fill-in: the patterns
    int item_count = 0;
    int memory[16] = {};     // n-back, predict-next, odd event, switch: what came before
    int table_next[64] = {}; // predict-next: the likely next symbol; odd event: the cycle
    // over the whole life, never reset: episodes (falls, dots, arrivals) and wins among them
    int64_t episodes = 0, wins = 0;

    LongState longs; // hold, pairs, once, rare, drift

    InvarianceState inv; // the invariance battery
    // the calculation battery
    CalcState calc;
    // pattern tasks: the output bits the last step's good or bad is for
    uint64_t pattern_graded = 0;
    // extra scores of the last window (task_extra_name names them)
    int score_extras = 0;
    double score_extra[TASK_MAX_EXTRA] = {};
};

// script step flags
constexpr uint8_t SCRIPT_SCORED = 1;   // counts toward the score
constexpr uint8_t SCRIPT_REWARDED = 2; // good or bad follow the answer
constexpr uint8_t SCRIPT_PROBE = 4;    // an offline reader keeps this step (label: want)

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
// what an episode and a win are called for this kind ("falls", "dots" / "caught"); null for
// kinds that do not count them
const char* task_episode_name(TaskKind kind);
const char* task_win_name(TaskKind kind);
// place and the invariance battery: right / tries on the rewarded versions since the last
// window score (call it first); -1 for other tasks
double task_taught_score(const Task& task);

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
// tiers 9 and 10: the shared script runner (task_script.cpp)
bool script_kind(TaskKind kind);
void script_reset(Task& task);
TaskShape script_shape(const Task& task);
void script_sense(const Task& task, TaskSense& sense);
int script_perfect_move(const Task& task);
int script_step(Task& task, const TaskStepIn& in);
double script_window_score(Task& task);
// the function battery, in task_function*.cpp
void function_reset(Task& task);
TaskShape function_shape(const Task& task);
void function_sense(const Task& task, TaskSense& sense);
int function_perfect_move(const Task& task);
int function_step(Task& task, const TaskStepIn& in);
// the battery's --fn flags: true if `flag` is one (ok false on a bad value); and a check of
// the settings once the command line is read, false after a message
bool function_flag(const char* flag, const char* value, TaskSettings& settings, bool& ok);
bool function_settings_ok(TaskKind kind, const TaskSettings& settings);
void bandit_reset(Task& task);
TaskShape bandit_shape(const Task& task);
void bandit_sense(const Task& task, TaskSense& sense);
int bandit_perfect_move(const Task& task);
int bandit_step(Task& task, const TaskStepIn& in);

void move_reset(Task& task);
TaskShape move_shape(const Task& task);
void move_sense(const Task& task, TaskSense& sense);
int move_perfect_move(const Task& task);
int move_step(Task& task, const TaskStepIn& in);

void classic_reset(Task& task);
TaskShape classic_shape(const Task& task);
void classic_sense(const Task& task, TaskSense& sense);
int classic_perfect_move(const Task& task);
int classic_step(Task& task, const TaskStepIn& in);
void calc_reset(Task& task);
TaskShape calc_shape(const Task& task);
void calc_sense(const Task& task, TaskSense& sense);
// the right pattern, as bits (or -1 off the ask step)
int calc_perfect_move(const Task& task);
int calc_step(Task& task, const TaskStepIn& in);
double calc_window_score(Task& task);
// the name of extra score i, e.g. "whole_held" or "bit3_seen"
void task_extra_name(const Task& task, int i, char* out, int size);
