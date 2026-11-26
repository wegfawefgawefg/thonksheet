#pragma once

#include "mnist/mnist_run.hpp"

#include <cstdint>
#include <vector>

// the outside outputs of blobmnist ceiling --life-outside (mnist_outside.cpp, and the
// ladder's forms in mnist_outside_cells.cpp). measuring sticks, no part of the blob.

struct OutsideCells; // --outside-form 4

struct Outside {
    std::vector<int> cols;  // the non-sensor cells
    std::vector<float> v;   // lines x cols
    std::vector<float> bias, x;
    int lines = 0;
    float lr = 0.3f, fine = 0.1f;
    int coin = 0, form = 0, margin = 8;
    std::vector<int16_t> w; // form 3: lines x cols
    std::vector<int8_t> ring; // the last 256 digits' error signs
    int64_t ring_count = 0;
    uint64_t rng = 0x6f757473696465ull;
    int64_t right = 0, seen = 0;
    OutsideCells* cells = nullptr;
};

// form 4: the ladder from the whole-number output down toward the engine's readout
OutsideCells* outside_cells_make(const Sheet& s, int argc, char** argv);
void outside_cells_free(OutsideCells* c);
// one digit: each line's error sign into sign[], the step taken; returns the line it answered
int outside_cells_digit(const Sheet& s, OutsideCells& c, const uint8_t* counts, int label,
                        int8_t* sign);
