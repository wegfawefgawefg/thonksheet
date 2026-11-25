#pragma once

#include <cstdint>
#include <vector>

// the plain linear reader (linear_fit.cpp) split by version, for the invariance battery:
// fitted on trials of the rewarded versions and scored twice, on later trials of the same
// versions ("same": is the pattern readable at all) and on trials of the versions it never
// saw ("held": does what was read carry over). ridge least squares, one output per class,
// largest wins, so chance is 1 / classes. floats are fine here; this is a probe.

struct SplitData {
    int features = 0, classes = 2;
    std::vector<uint8_t> x; // trial-major: `features` numbers per trial
    std::vector<int> label, version;
    std::vector<uint8_t> held;
};

// rows in time order: the taught trials' first 60% fit, the next 15% pick the ridge, the
// last 25% are "same"; every held trial is "held". features standardized by the fit rows.
struct SplitPrep {
    int n = 0, d = 0, classes = 2;
    std::vector<int> fit, check, same, held;
    std::vector<double> z;      // n x d
    std::vector<double> y;      // fit x classes, one-hot less its mean
    std::vector<double> y_mean; // classes
};

struct SplitScore {
    double same = 0, held = 0, check = 0;
    std::vector<double> by_version; // share right per version, on its test rows
};

void split_prepare(const SplitData& data, SplitPrep& prep);
// a reader of the given features (every feature if empty)
SplitScore split_fit(const SplitData& data, const SplitPrep& prep, const std::vector<int>& features,
                     int versions);
