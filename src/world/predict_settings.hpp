#pragma once

#include <cstdint>
#include <cstdio>

// Idea 162, every line is a target: the flags and the skill tally. The predictor itself is
// src/run/predict.hpp; these parts sit in the world library so every program that reads
// blobrun's flags has them (world/predict_flags.cpp).

constexpr int PREDICT_MAX_HORIZONS = 4;
constexpr int PREDICT_MAX_TARGETS = 128;
constexpr int PREDICT_RING = 16; // steps kept, more than the longest horizon

struct PredictSettings {
    int horizons = 0;                       // 0: no predictor cells
    int horizon[PREDICT_MAX_HORIZONS] = {}; // ascending: the first is the shortest
    int press = 0;                          // --predict-press 1: the last press comes back as lines
    int errors = 0; // --predict-errors 1: the shortest horizon's errors come back as lines
    int cells = 4;  // --predict-cells: most motor cells per group
    int shift = -1; // --predict-shift: chance of a verdict's move, 2^-shift; -1 the readout's
    int64_t score_from = -1; // --predict-score-from: first step tallied; -1 the last quarter
};

inline bool predict_wanted(const PredictSettings& s) {
    return s.horizons > 0 || s.press || s.errors;
}

// one horizon's tally over the scored steps, per line-step
struct PredictTally {
    int64_t total = 0, right = 0, tied = 0;
    int64_t same = 0; // "same as now": the line's value h steps before was right
    int64_t changed = 0, changed_right = 0; // line-steps whose value differs from h before
    int64_t on = 0, on_right = 0, off_right = 0;
    int64_t same_on = 0; // line-steps on now and h before (the "same as now" hits on on-steps)
    int64_t steps = 0;
    int64_t line_on[PREDICT_MAX_TARGETS] = {}; // for the most-common-value baseline
};

struct PredictScore {
    int horizons = 0, targets = 0, cells_per_group = 0;
    int horizon[PREDICT_MAX_HORIZONS] = {};
    PredictTally tally[PREDICT_MAX_HORIZONS];
    int64_t verdicts = 0; // verdict calls (one per horizon per step with a wrong bit)
};

// --predict, --predict-press, --predict-errors, --predict-cells, --predict-shift,
// --predict-score-from: 1 a flag of ours, 0 not ours, -1 a bad value (after a message)
int predict_parse_flag(const char* flag, const char* value, PredictSettings& s);
extern const char* const predict_flags_usage;

// the skill table: per horizon, the share of line-steps right beside "same as now" (the
// line h steps before), the most common value of each line, the ties, and the right share
// on line-steps that changed, on lines that were on, and on lines that were off. then the
// balanced share (the mean of the right shares on on- and off-line-steps, so a line that is
// nearly always off cannot be predicted for free) for the predictor, "same" and "common"
// (which is about 0.5 when every line is mostly off)
void predict_print(const PredictScore& s, const char* who, FILE* out);
// the same as summary lines "who,pred_h<h>_<key>,value"
void predict_write_sum(const PredictScore& s, const char* who, FILE* sum);
