#pragma once

#include <cstdint>

// The calculation battery (ideas 133 and 134, docs/curriculum.md "Calculation"). The answer
// is a pattern: several yes/no outputs at once. Each output bit has two motor lines, an
// "off" group (line 2b) and an "on" group (line 2b + 1); the one with more spikes over the
// step is the bit's answer, a tie is no answer (wrong). Everything is integers.
//
// One trial: a blank step, the steps that show it, and the last of them also has the ask
// line on. Only the answer on the ask step counts. Some input cases are held out for the
// whole life: never rewarded, scored apart, so "seen" against "held out" says whether a
// rule was learned or a table of the cases it was taught.

enum class CalcOp {
    copy,       // out = a (the control: every output bit is one input line)
    add,        // a + b, N + 1 bits out
    sub,        // a - b mod 2^N
    mul,        // a * b, 2N bits out
    inc,        // a + 1, N + 1 bits out
    compare,    // a > b, one bit out
    and_bits,   // a & b
    or_bits,    // a | b
    xor_bits,   // a ^ b
    not_bits,   // ~a
    shl,        // a << s mod 2^N, s shown in binary
    rotl,       // a turned left by s, s shown in binary
    count_ones, // how many bits of a are on, in binary
    parity,     // whether an odd number of bits of a are on, one bit out
    alu4,       // op (2 bits) + a + b: add, sub, and, xor; N bits out
    alu8,       // op (3 bits) + a + b: add, sub, and, or, xor, not a, a << 1, a >> 1
    program,    // one instruction per step on a register of N bits; out the register
    max2,       // the larger of a and b
    sort3,      // three N-bit numbers, out in rising order
    kth,        // a list of `list` numbers and k in binary; out the k-th
    turn,       // a point on a size x size grid, place-coded; out turned a quarter
    mirror,     // the point mirrored left to right
    move,       // the point moved by a shown offset, wrapping
    scale,      // the point scaled from the corner by a shown factor 1..size-1
    turn3d,     // a point in a size^3 cube turned a quarter about z
    move3d,     // a point in the cube moved by a shown offset, wrapping
    angle,      // a point on a (2r+1)^2 grid turned by one of 8 angles, rounded
    turn3,      // three points on the grid, each turned a quarter
    count,
};

constexpr int CALC_MAX_OUT = 30;     // output bits (two motor lines each)
constexpr int CALC_MAX_LINES = 32;   // sensor lines, the ask line included
constexpr int CALC_MAX_STEPS = 8;    // steps that show a trial (programs)
constexpr int CALC_EXACT_CASES = 4096; // up to this many cases the held-out set is exact

struct CalcSettings {
    CalcOp op = CalcOp::add;
    int bits = 2;     // N: operand width (register width for programs)
    int hold = 4;     // cases held out, in sixteenths (0: none)
    int grade = 0;    // 0: one good or bad for the whole answer; 1: a verdict per output bit
    int length = 3;   // program: instructions
    int size = 4;     // geometry: grid side (angle: radius)
    int list = 4;     // kth: numbers in the list
};

struct CalcState {
    int lines = 0;    // sensor lines, the ask line (line 0) included
    int out_bits = 0; // output bits
    uint64_t cases = 0;   // input cases (ids 0 .. cases - 1, some invalid for scale/angle)
    uint64_t salt = 0;    // per life: which cases are held out
    uint64_t held_exact[CALC_EXACT_CASES / 64] = {};
    // the trial under way: lines on at each step, its answer and case
    int steps = 0;
    uint32_t step_lines[CALC_MAX_STEPS + 1] = {};
    uint64_t answer = 0;
    uint64_t case_id = 0;
    bool held = false;
    // tallies for the window: [0] seen, [1] held out
    int64_t trials[2] = {}, whole[2] = {}, bits[2] = {};
    int64_t bit_right[2][CALC_MAX_OUT] = {};
};

// one blobrun flag for the battery. 1 used it, 0 not a calc flag, -1 bad value (message
// printed). in task_calc_flags.cpp, so every program that parses blobrun's flags has it
struct TaskSettings;
int calc_parse_flag(const char* flag, const char* value, TaskSettings& settings);
const char* calc_op_name(CalcOp op);
extern const char* const calc_flags_usage;
