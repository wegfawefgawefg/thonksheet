#pragma once

#include <cstdint>

// The function battery (docs/curriculum.md, "The function battery"): worlds where the right
// button is a fixed function of what was shown, graded in difficulty. Four task kinds share
// one trial shape (a blank step, the steps that show it, and an answer step with the ask
// line on) and one way of drawing what is shown (task_function.cpp):
//
//   points       N cue lines, each on or off; the answer a boolean function of them, or
//                how many are on, or which is the first on
//   number       a value x shown as one lit line of L (place code); the answer is which of
//                K bands f(x) falls in
//   two-numbers  two values the same way; the answer the band of their sum, difference,
//                larger, product, or which is larger
//   past         two lines A and B; each item shows one of them, items `gap` blank steps
//                apart; the answer a function of the last two or three items

// every function name, across the four kinds; each kind takes only its own
enum class FnName : uint8_t {
    // points
    and_all,
    or_any,
    parity,   // also past: an odd number of B among the items
    majority, // also past: more B than A among the items
    exactly,  // exactly `k` lines on
    table,    // a truth table drawn per life from the task seed
    table_lin,
    table_nonlin,
    count, // how many are on (N + 1 buttons)
    first, // the index of the first line on (N buttons)
    // number
    identity,
    step,
    tent,
    sine,
    cosine,
    square,
    // two numbers
    sum,
    diff,
    larger,
    product,
    compare,
    // past
    same,
    recall,
    none,
};

constexpr int FN_MAX_INPUTS = 1024; // points: 2^10 patterns; two numbers: 32 x 32 pairs
constexpr int FN_MAX_CLASSES = 17;
constexpr uint8_t FN_NO_CLASS = 255; // an input never shown (a tie, no line on)

struct FnSettings {
    FnName fn = FnName::none; // none: the kind's first function
    int lines = 2;            // points: N cue lines; number kinds: L lines per number
    int bands = 2;            // number kinds: K bands, one button each
    int k = 1;                // points, exactly: how many on
    int hold = 0;             // number: x values never rewarded; the only ones scored
    int gap = 0;              // past: blank steps between items
    int items = 3;            // past: items for parity and majority (same, recall: 2)
};

struct FnState {
    int inputs = 0;  // patterns, values or pairs that can be shown
    int classes = 0; // buttons
    uint8_t cls[FN_MAX_INPUTS] = {};
    // inputs sorted by class: class c owns member[first[c] .. first[c + 1])
    uint16_t member[FN_MAX_INPUTS] = {};
    int first[FN_MAX_CLASSES + 1] = {};
    bool uniform = false;       // drawing inputs uniformly already gives every answer equally
    bool threshold_unit = true; // points: a single threshold unit can do it (perceptron check)
    int x = 0;                  // the input shown this trial
};

const char* fn_name(FnName fn);
bool fn_from_name(const char* name, FnName& fn);
