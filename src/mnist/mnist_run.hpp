#pragma once

#include "blob/blob.hpp"
#include "mnist/mnist_learn.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cstdint>
#include <vector>

// blobmnist: MNIST through the sheet, and the plain learners it is measured against.
// docs/results-2026-11-25-mnist.md; scripts/mnist.sh runs it all.

int int_flag(int argc, char** argv, const char* name, int fallback);
const char* text_flag(int argc, char** argv, const char* name, const char* fallback);

// the shift test: a digit moved by `by` pixels in one of four directions
constexpr int SHIFT_DIRECTIONS = 4;
void shift_direction(int dir, int by, int& dx, int& dy);

constexpr int MNIST_PRESS_KINDS = 11;

// one blob with the mnist task plugged in, driven a digit at a time
struct Sheet {
    RunFlags flags;
    Blob* blob = nullptr;
    TaskTicker* tt = nullptr;
    int gap = 0;         // quiet ticks after every digit, train and test alike
    int train_first = 0; // the training stream: digits [first, first + count)
    int train_count = 0;
    int eval_test = 1; // scored on the test set (1) or the validation slice (0)
    int eval_first = 0, eval_count = 0;
    int64_t ticks = 0;
    std::vector<int>* images = nullptr;      // when set, each digit's index in its set is appended
    std::vector<int>* partners = nullptr;    // and with --pair, the second digit's
    int64_t presses[MNIST_PRESS_KINDS] = {}; // per button, then "no move", since last cleared
    // idea 155, a control: the blob is told good or bad by a coin instead of the answer
    int coin_reward = 0;
    uint64_t coin_state = 0x636f696eull;
    // the inside-learning control (mnist_shuffle.cpp): while on, each digit's pixels are
    // shuffled by a permutation drawn fresh for that digit (same ink, no structure)
    int shuffle = 0;
    int teach = 0; // --life-teach: the life is judged (the readout and the hop rule learn)
    uint64_t shuffle_rng = 0;
    std::vector<int> shuffle_cells, shuffle_perm;
};

// shuffle on: draw this digit's permutation (at the digit's start), and move the pixel
// cells' bits by it (after every write of the lines)
void shuffle_draw(Sheet& s);
void shuffle_apply(Sheet& s);

// what a reward-free life did to the inside (mnist_wiring.cpp): the wiring as born, a life
// in chunks with a line on activity and wiring after each, and those two lines alone
void wiring_snapshot(const Sheet& s, std::vector<uint64_t>& born);
void life_run(Sheet& s, int64_t digits, int chunks, const std::vector<uint64_t>& born);
void activity_report(const Sheet& s, const Rows& rows, const char* label);
void wiring_report(const Sheet& s, const std::vector<uint64_t>& born, const char* label);

// own flags are read by the caller; everything else is blobrun's (task, plug, --g. numbers)
bool sheet_start(Sheet& s, int argc, char** argv, const char* const* own, int own_count);
void sheet_free(Sheet& s);

// the sheet's inner wiring by co-activity (learn mode 0: prediction, growth, pruning), with
// no reward, for `digits` training digits; then back to the genome's own learn mode
void sheet_pretrain(Sheet& s, int64_t digits);
// run `digits` steps of the training stream; learn: rewards reach the blob. returns right
// answers. counts: each digit's spike counts over its ticks appended, every neuron
int64_t sheet_train(Sheet& s, int64_t digits, bool learn, Rows* counts);
// the evaluation digits (moved by dx, dy) with learning frozen; the training stream carries
// on afterwards where it was. returns the share right; counts as above
double sheet_eval(Sheet& s, int dx, int dy, int count, Rows* counts);

int mnist_baselines(int argc, char** argv);
int mnist_online(int argc, char** argv);
int mnist_ceiling(int argc, char** argv);
