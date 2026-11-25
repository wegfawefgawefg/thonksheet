#pragma once

#include "run/task_ticker.hpp"

#include <cstdint>
#include <vector>

// The shuffled-input control for lives on the toys and the calculation battery
// (docs/results-2026-11-25-hop-real.md). Each step the sheet is shown the input lines of
// another step, drawn at random from the last 256 the task showed, while the world grades
// the step it really set. The cases look as they always do; which one is shown says
// nothing about the answer. (On MNIST the control shuffles a digit's pixels instead,
// mnist_shuffle.cpp.)
struct Scramble {
    int on = 0;
    uint64_t rng = 0;
    int filled = 0, next = 0, show = 0;
    std::vector<uint8_t> ring; // cases x the plug's sensor lines
};

void scramble_start(Scramble& s, int on, uint64_t seed);
// after ticker_before_tick: on a step's first tick keep the case the task shows and pick one
// to show instead; on every tick of the step write that case onto the sensor cells
void scramble_tick(Scramble& s, const TaskTicker& tt, Blob& blob);
