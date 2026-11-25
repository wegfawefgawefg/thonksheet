#pragma once

#include "blob/blob.hpp"
#include "world/task.hpp"

#include <cstdint>

// The world plug: puts a task's sensor lines onto the genome's sensor region and reads its
// motor lines off the motor region. Every line is a square group of side x side cells. Lines
// are packed onto a grid of such groups, left to right in shelves, so a bitmap keeps its
// geometry: pixel (px, py) is the group at (px, py) from the bitmap's corner.
//
// Sensor order: good, bad (always the two top-left groups, so they stay put across tasks),
// then the task's on/off lines, then its bitmap, then its numbers, each number a row of
// threshold lines. A task that uses no reward leaves good and bad unattached.

constexpr int PLUG_MAX_SENSOR_LINES = 1024; // mnist's 28x28 and good, bad
constexpr int PLUG_MAX_MOTOR_LINES = 64;
constexpr int PLUG_MAX_SIDE = 4;
constexpr int PLUG_MAX_GROUP = PLUG_MAX_SIDE * PLUG_MAX_SIDE;
constexpr int PLUG_GOOD_LINE = 0;
constexpr int PLUG_BAD_LINE = 1;
constexpr int PLUG_MAX_SPREAD = 4096; // motor cells over all buttons with --motor-spread

struct PlugSettings {
    int sensor_side = 2;       // a sensor line drives side x side neurons
    int motor_side = 2;        // a motor line is read from side x side neurons
    int ticks_per_step = 8;    // blob ticks per task step
    int reward_line_ticks = 2; // ticks the good or bad line stays on after a reward
    int rest_ticks = 0;        // quiet ticks after a rewarded step (task lines off)
    // tiles from the cue lines to the motor lines; 0 packs both from the facing edges of
    // their regions (1 tile apart with the default genome)
    int distance = 0;
    // how a number becomes lines: 0 bar (lines light from the left as the value rises, so a
    // low value is only an absence), 1 place (only the line whose band holds the value)
    int number_code = 0;
    // idea 58: N motor cells per button, spread over the motor region (plug_spread.cpp);
    // 0 is one side x side group per button. even: spread from nearest to farthest rather
    // than the nearest N
    int motor_spread = 0;
    int motor_spread_even = 0;
};

struct Plug {
    PlugSettings settings;
    TaskShape shape;

    // sensor lines, in the order above. line_cell_x/y is the group's top-left cell.
    int sensor_lines = 0;
    int first_single = 0, first_sheet = 0, first_number = 0;
    int sensor_cell_x[PLUG_MAX_SENSOR_LINES] = {};
    int sensor_cell_y[PLUG_MAX_SENSOR_LINES] = {};
    int sensor_neuron[PLUG_MAX_SENSOR_LINES * PLUG_MAX_GROUP] = {};
    uint8_t attached[PLUG_MAX_SENSOR_LINES] = {};
    uint8_t line_on[PLUG_MAX_SENSOR_LINES] = {};
    int sensed_neurons = 0; // attached neurons, good and bad included when attached

    // motor lines
    int motor_lines = 0;
    int motor_cell_x[PLUG_MAX_MOTOR_LINES] = {};
    int motor_cell_y[PLUG_MAX_MOTOR_LINES] = {};
    int motor_neuron[PLUG_MAX_MOTOR_LINES * PLUG_MAX_GROUP] = {};
    int motor_count[PLUG_MAX_MOTOR_LINES] = {}; // spikes this step

    // ticks left on the good and bad lines
    int good_left = 0, bad_left = 0;

    // --motor-spread: line L's cells are spread_neuron[spread_first[L] .. spread_first[L + 1])
    int spread_first[PLUG_MAX_MOTOR_LINES + 1] = {};
    int spread_neuron[PLUG_MAX_SPREAD] = {};
};

// lay the task's lines onto the blob and attach them. false (with a message on stderr) if
// they do not fit the genome's regions or land on neurons that are not sensor/motor.
bool plug_attach(Plug& plug, Blob& blob, const TaskShape& shape, const PlugSettings& settings);
// take the lines back out: the blob stops learning from them
void plug_detach(Plug& plug, Blob& blob);

// --motor-spread: deal the motor lattice out to the buttons (plug_spread.cpp)
bool plug_spread_motors(Plug& plug, const Blob& blob);
// the motor cells line `line` is read from, spread or one group; returns how many
int plug_motor_cells(const Plug& plug, int line, const int** cells);

// the buttons' motor cells marked on the blob (on) or cleared, and the pressed button's
// cells after a step (move -1: none). only learn modes 5 and 6 read them (idea 58).
void plug_mark_motors(const Plug& plug, Blob& blob, bool on);
void plug_mark_acted(const Plug& plug, Blob& blob, int move);

// task values to line bits, once per step
void plug_set_lines(Plug& plug, const TaskSense& sense);
// all task lines off (rest ticks); good and bad keep their own timers
void plug_quiet_lines(Plug& plug);
// write every attached line onto the blob; call once before each tick
void plug_write(Plug& plug, Blob& blob);
// add this tick's motor spikes; call once after each tick
void plug_count(Plug& plug, const Blob& blob);
// the motor line with the most spikes this step, or -1 on a tie or no spikes. clears counts.
int plug_choice(Plug& plug);
void plug_clear_counts(Plug& plug);
// raise the good (+1) or bad (-1) line for the next reward_line_ticks ticks
void plug_reward(Plug& plug, int valence);

// cells per line
inline int plug_sensor_group(const Plug& plug) {
    return plug.settings.sensor_side * plug.settings.sensor_side;
}
inline int plug_motor_group(const Plug& plug) {
    return plug.settings.motor_side * plug.settings.motor_side;
}

// an answer that is a pattern (idea 134, plug_pattern.cpp): output bit b is read from motor
// lines 2b ("off") and 2b + 1 ("on"), whichever had more spikes over the step. a tie is no
// answer (its bit in `tied`). clears the counts, as plug_choice does.
void plug_pattern(Plug& plug, uint64_t& bits, uint64_t& tied);
// the marks for one good or bad on a pattern answer: only the cells of the output bits in
// `graded` are marked as buttons' cells, and of each, the group that won (none on a tie) as
// pressed. the readout's error step then touches those outputs alone.
void plug_mark_pattern(const Plug& plug, Blob& blob, uint64_t bits, uint64_t tied,
                       uint64_t graded);
