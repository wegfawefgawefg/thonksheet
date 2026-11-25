#pragma once

#include "blob/blob.hpp"

#include <cstdint>
#include <vector>

// blobmnist ladder (docs/results-2026-11-25-output-gap.md): the sheet's response to each digit
// is recorded once, frozen, and readers that add one handicap at a time are run on the
// recording, from a float reader fit offline to a copy of the engine's own output rule.
// Floats are fine here; this is the yardstick, not the blob.

// spike counts per neuron per digit, as recorded
struct Record {
    int n = 0, d = 0;
    std::vector<uint8_t> c20;   // n x d: spikes over the digit's ticks (what a reader reads)
    std::vector<uint8_t> c15;   // n x d: over its last 15 ticks (the engine's learning window)
    std::vector<uint8_t> label;
};

// the motor cells as the engine has them: which button, what each can hear, as born
struct Cells {
    int buttons = 0, count = 0;
    std::vector<int> neuron, button;    // per cell
    std::vector<int> tile;              // count x SLOTS: source tile, -1 for an empty slot
    std::vector<uint64_t> usable;       // count x SLOTS: sources the engine may wire
    std::vector<uint64_t> inhib;        // count x SLOTS: inhibitory sources among them
    std::vector<int8_t> born;           // count x SLOTS x 64: born strength (0, 1, 4, 5)
    std::vector<int> keep;              // born connection count (the budget)
    std::vector<uint8_t> against;       // an odd column of the motor lattice: the opponent
                                        // half, as the engine's readout_opponent has it
    double fire_share = 0;              // share of digits a motor cell fires on, as recorded
    int inhibit_gain = 5;
};

constexpr int LADDER_CHECKS = 5;
constexpr int64_t LADDER_CHECK_AT[LADDER_CHECKS] = {1000, 5000, 20000, 60000, 120000};

struct Ladder {
    Record train, test;
    Cells cells;
    std::vector<int> inner; // the non-sensor neurons (rungs 1 to 4 read these)
    std::vector<std::vector<int>> later; // the stream's order on passes 2, 3, ...
    uint64_t seed = 1;
};

// one online reader's test accuracy at each check, and its buttons' presses at the last one
struct Curve {
    double acc[LADDER_CHECKS] = {};
    int quiet_buttons[LADDER_CHECKS] = {}; // buttons pressed on fewer than 1% of test digits
    double ties[LADDER_CHECKS] = {};       // share of test digits with no single winner
    int64_t goods = 0, close_goods = 0;    // training goods, and those won by a close vote
};

// the record index of the i-th digit of the online stream: the recorded order first, then a
// fresh seeded order for each later pass (as the task does)
int ladder_stream(const Ladder& L, int64_t i);
// the unique arg max of k scores, -1 on a tie
int ladder_pick(const double* score, int k);
void ladder_print(const char* name, const Curve& c);

// rungs 1 to 4: readers of every non-sensor cell (mnist_ladder_linear.cpp)
double rung1_offline(const Ladder& L);
void rung2_online_float(const Ladder& L, Curve& c);
// center 1: each cell's count less its running mean (in integers), as the float readers do
void rung3_integer_told(const Ladder& L, int center, Curve& c);
// what moves on a good (bad always: the pressed button loses, the others gain a sixteenth)
enum GoodHalf {
    GOOD_NONE = 0,          // nothing
    GOOD_ALL = 1,           // the pressed button gains, the others lose a sixteenth (engine)
    GOOD_ALL_CLOSE = 2,     // the same, only when the vote was close (the gate)
    GOOD_PRESSED = 3,       // the pressed button gains, the others do not move
    GOOD_PRESSED_CLOSE = 4, // the same, only when the vote was close
    GOOD_RIVAL_CLOSE = 5,   // on a close vote the pressed button gains and the runner-up loses
};
// a close vote: the winner's lead under 20% of its own score (the engine's margin_percent 20)
bool ladder_close(const double* score, int k, int pick, int& runner);
// nonneg 1: weights never go below zero
void rung4_integer_good_bad(const Ladder& L, int good, int nonneg, int center, Curve& c);

// rungs 5 and 6: a button is the vote of its motor cells (mnist_ladder_cells.cpp)
// vote: 0 the sum of the cells' drives, 1 each cell's drive over its threshold, in its own
// spread (a stand-in for a spike count), 2 a count of cells over their threshold
void rung5_cells(const Ladder& L, int vote, int good, Curve& c);
// rung 6 and the fixes tried on it before they go into the engine
struct Rung6 {
    int form = 1;     // 0 counters as graded weights, 1 one-bit wiring of the top counters
    int good = GOOD_ALL;
    int keep_mul = 1; // the budget: this many times the born connection count
    int halve = 0;    // 1: a counter reaching the clamp halves the cell's whole row
    int strong = 0;   // 1: the top quarter of the wired sources count 4 (the w1 plane)
    int dale = 0;     // 1: only excitatory sources may be wired (readout_evidence 3)
    int opponent = 0; // 1: every second cell of a button counts against it and learns the
                      // other way, so one-bit excitatory wiring can say "not this button"
};
void rung6_counters(const Ladder& L, const Rung6& opt, Curve& c);

int mnist_ladder(int argc, char** argv);
