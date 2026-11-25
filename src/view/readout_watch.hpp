#pragma once
// The readout as the viewer watches it, on the sim thread: every button's cells (from the
// plug), their spikes per step, and what each cell keeps per possible source. Before each
// reward the cells' rows are copied; right after it they are compared, so the panel can
// say which cells gained and lost evidence on the last change and the picture of a picked
// cell can show what moved.
//
// What a cell keeps, by engine (KeepKind):
//   full engine, readout_evidence on   its evidence counters (blob.readout_evidence)
//   full engine otherwise              its connection strengths 0, 1, 4, 5
//   minimal engine, ev_on              its evidence counters (bitsliced, ev_planes)
//   minimal engine otherwise           its one-bit mask
//   table neurons                      nothing per source (the panel says so)

#include "blob/blob.hpp"
#include "view_frame.hpp"

#include <cstdint>
#include <vector>

struct Plug;

constexpr int KEEP_SOURCES = SLOTS * TILE_NEURONS; // 11 slots x 64 sources, every engine

// one button's cells in one tile, for counting their spikes a word at a time
struct ReadoutWord {
    int tile = 0;
    int button = 0;
    uint64_t cells = 0;
};

struct ReadoutWatch {
    std::vector<ReadoutWord> words;
    int engine = 0;
    std::vector<int> cells;         // neuron of each readout cell, button by button
    std::vector<uint8_t> button;    // per cell
    std::vector<int8_t> before;     // per cell, KEEP_SOURCES values as last seen
    std::vector<int8_t> moved;      // per cell, how much each value moved at the last change
    std::vector<int8_t> diff;       // scratch: the same, for the reward being compared
    std::vector<int8_t> row, wired; // scratch for one cell
    std::vector<int> step_spikes;   // per button, this step so far
    ReadoutView view;
    // a reward whose effect is still to be compared: the full engine learns from it on the
    // next tick, so the comparison waits for that tick
    bool pending = false;
    bool stale = false; // a reward went by unwatched: the next comparison is a baseline only
    int64_t pending_step = 0;
    int pending_valence = 0, pending_move = -1;
};

// the plug's buttons and their cells (after the plug is attached)
void readout_watch_start(ReadoutWatch& w, int engine, const Blob& blob, const Plug& plug);
// after every tick: the cells' spikes
void readout_watch_tick(ReadoutWatch& w, const Blob& blob);
// a step's answer is in: its spikes per button, who won
void readout_watch_answer(ReadoutWatch& w, int move);
// a reward was given (watch: the viewer is drawing). after the next tick (the full engine
// learns from a reward on the tick after it), readout_watch_settle compares every cell's
// row with how it was at the last comparison; anything that moved is the last change
void readout_watch_reward(ReadoutWatch& w, int64_t step, int valence, int move, bool watch);
void readout_watch_settle(ReadoutWatch& w, const Blob& blob);

// what neuron n keeps per possible source, into the picked neuron's picture (any neuron
// that keeps something; the moved values only for a readout cell)
void readout_fill_selected(const ReadoutWatch& w, const Blob& blob, Selected& s);
