#pragma once

#include <cstdint>
#include <vector>

// a plain linear reader as a reference point (idea 58): how well can a fitted linear
// readout of spike counts tell the answer, in the task's own units (share right).
// ridge least squares, one output per class, the largest wins. floats are fine here; this
// is a probe, not the blob.

struct LinearData {
    int neurons = 0;
    int tiles_w = 0, tiles_h = 0;
    int classes = 2;
    std::vector<uint8_t> counts; // trial-major: neurons counts per trial
    std::vector<int> label;      // one per trial, 0..classes-1
};

// fit on the first three quarters of the trials, score on the last quarter, and print:
// the whole sheet, every tile alone, and every 3x3 block of tiles (what a motor cell in the
// middle tile can see through its local slots). cue_tile and motor_tiles are marked.
void linear_report(const LinearData& data, const char* label, int cue_tile,
                   const std::vector<int>& motor_tiles);

// solve (a + ridge I) x = b for `cols` right-hand sides (b and x are m x cols, row-major);
// a is m x m and kept intact. false if not positive definite. (linear_fit.cpp)
bool ridge_solve(const std::vector<double>& a, int m, double ridge, const double* b, int cols,
                 std::vector<double>& x);
