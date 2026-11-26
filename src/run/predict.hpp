#pragma once

#include "blob/blob.hpp"
#include "run/task_ticker.hpp"
#include "world/plug.hpp"
#include "world/predict_settings.hpp"

#include <cstdint>
#include <vector>

// Idea 162, every line is a target (docs/results-2026-11-25-predict.md). Beside the world's
// own buttons, the sheet answers, for every input line and each horizon h, "this line will
// be on h steps from now". Each answer is a pair of motor-cell groups voted the way a
// pattern bit is (plug_pattern.cpp): more spikes in the "on" group than the "off" group over
// the step says on. A tie says off (lines are mostly off; a pattern bit's tie is no answer),
// and the verdict treats the off group as the one that answered. When step t + h arrives its lines
// are the answers, and every wrong bit gets the working output rule's bad (learn_readout.cpp),
// applied to the activity of step t, which is kept in a ring of spike maps (the delay line that
// stands in for an eligibility trace). The world's reward still teaches the buttons, as before.
//
// Two more things ride on the same plug, each off by default: the sheet's own last press as
// input lines (one per button, the step after the press), and the shortest horizon's errors
// as input lines (per target: "said on, was off" and "said off, was on"), shown during the
// step whose lines they are about.
//
// The lines predicted (the targets): good and bad when the task uses them, then every line
// of the task's own. The extra lines above are not targets.
//
// A life loop drives it round the ticker (life.cpp):
//   predict_before_tick(p, tt, blob);   then ticker_before_tick, tick
//   predict_after_tick(p, tt, blob);    before ticker_after_tick
//   if (ticker_after_tick(...)) predict_step_end(p, blob, tt.in.move);

struct Predictor {
    bool on = false; // anything asked (predict_wanted) and attached
    PredictSettings s;
    int targets = 0;
    int target_line[PREDICT_MAX_TARGETS] = {}; // plug line of each target
    int bits = 0;                              // targets x horizons, bit = target * H + h
    int cells_per_group = 0;
    std::vector<int> cell;       // 2 x bits groups (off, on) x cells_per_group
    std::vector<uint16_t> count; // spikes per group this step
    int first_press = -1, first_error = -1, press_lines = 0;
    int last_move = -1;
    int64_t step = 0; // steps begun
    // the ring, one slot per step: line values at its start, the answers at its end
    // (0 off, 1 on, 2 a tie, which answers off), and the spike maps at its end
    uint8_t truth[PREDICT_RING][PREDICT_MAX_TARGETS] = {};
    uint8_t said[PREDICT_RING][PREDICT_MAX_HORIZONS][PREDICT_MAX_TARGETS] = {};
    std::vector<uint64_t> maps; // PREDICT_RING x HISTORY x words
    int map_now[PREDICT_RING] = {};
    int words = 0;
    int64_t score_from = 0;
    // the hook for idea 159: the shortest horizon's signed errors at this step's start, per
    // target: +1 the line came on and the answer was not on, -1 the answer was on and the
    // line was off, 0 right (or no answer due yet)
    int8_t error[PREDICT_MAX_TARGETS] = {};
    PredictScore score;
    std::vector<uint64_t> saved_attached, saved_acted; // the world's marks, kept round a verdict
    // idea 183: each ring step's group counts (PREDICT_RING x 2 x bits), kept when the
    // answers' errors teach the hidden cells (--predict-hop); hop_far: some horizon past 1 does
    std::vector<uint16_t> votes;
    bool hop_far = false;
};

// after ticker_start: lay the plug again with the extra lines, pick the predictor cells.
// does nothing (and turns p off) unless predict_wanted. false after a message
bool predict_attach(Predictor& p, TaskTicker& tt, Blob& blob, const RunFlags& flags);
// a step's first tick, before ticker_before_tick: the step's lines, then grade the answers
// now due (teaching the cells), tally, and set the extra lines
void predict_before_tick(Predictor& p, TaskTicker& tt, Blob& blob);
// after each blob tick, before ticker_after_tick: count the groups' spikes
void predict_after_tick(Predictor& p, const TaskTicker& tt, const Blob& blob);
// when ticker_after_tick says the step is answered: read the answers, keep the maps
void predict_step_end(Predictor& p, Blob& blob, int move);

// the same steps for a caller that writes the plug's lines itself (no task: the shuffled
// control in blobprobe predict). predict_step_begin wants the step's lines in the plug
void predict_step_begin(Predictor& p, Plug& plug, Blob& blob);
void predict_count(Predictor& p, const Blob& blob);

// the shortest horizon's signed errors (the idea 159 hook), PREDICT_MAX_TARGETS long
inline const int8_t* predict_errors(const Predictor& p) {
    return p.error;
}
