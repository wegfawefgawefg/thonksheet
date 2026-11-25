#pragma once

#include "world/task_calc.hpp"

#include <cstdint>

// the members of the calculation battery as plain functions of a case id (calc_ops.cpp)

struct CalcLayout {
    int lines = 0;      // sensor lines, the ask line included
    int out_bits = 0;   // answer bits
    uint64_t cases = 0; // case ids (some may be invalid)
    int steps = 2;      // steps of a trial, the blank one first
};

struct CalcTrial {
    int steps = 0;
    uint32_t lines[CALC_MAX_STEPS + 1] = {}; // lines on at each step; ask on the last
    uint64_t answer = 0;
};

CalcLayout calc_layout(const CalcSettings& s);
// false if id is not a valid case of this member (a shift past the width, a point scaled
// off the grid, a point outside the turning disc, a shift instruction with a value)
bool calc_case(const CalcSettings& s, uint64_t id, CalcTrial& t);
