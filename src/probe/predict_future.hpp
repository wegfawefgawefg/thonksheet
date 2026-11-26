#pragma once

#include <cstdint>
#include <vector>

// blobprobe predict --future H,H: how much of the input's future a linear reader finds in
// the frozen sheet, the ceiling the predictor cells (idea 162) could reach with a better
// output. Rows are consecutive read steps: x the features of each step (spike counts, or the
// lines themselves), `lines` the predicted lines of each step (good and bad, then the task's).
// One ridge reader per line of step + h, thresholded at one half; share of line-steps right
// on the test quarter, beside "same as now" and each line's more common value on those rows.
// Idea 183 adds two harder scores of the same reader on the same rows: balanced (the mean of
// the shares right on on- and on off-line-steps) and changed (the share right on line-steps
// whose value differs from h steps before).
struct FutureScore {
    double right = 0, same = 0, common = 0;
    double balanced = 0, changed = 0;
};

FutureScore future_fit(const std::vector<uint8_t>& x, int d, const std::vector<uint8_t>& lines,
                       int count, int rows, int h);
