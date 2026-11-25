#pragma once

#include "world/task.hpp"

#include <cstdint>

// What blobprobe predict asks a frozen sheet (idea 162): things about the world that are
// not on its input lines at one instant, so a reader has to find them in what the sheet
// carries from step to step.
//   catch, track            which way the paddle (marker) moved into this step: left,
//                           still, right (the lines show where it is, not where it was)
//   forage                  the animal's last step: up, down, left, right, none
//   cartpole, pendulum,     the last push: left, right, none (the number lines show the
//   mountain car            state; the push shows only as a change between two steps)
//   chain                   the last press (k buttons, none); the stage is on a line
//   delayed echo            time since the cue (the phase, delay + 2 classes) and the cue
//   echo, shuffled echo     the last step's cue (cues are drawn afresh each step)
//   predict-next            the next symbol, and the symbol before the one shown
//   beat                    the step's place in the period
// Moves in the read come from a fixed coin, not from the sheet, so every arm sees the same
// world.

constexpr int READ_MAX_LABELS = 2;

struct ReadLabels {
    int count = 0;
    const char* name[READ_MAX_LABELS] = {};
    int classes[READ_MAX_LABELS] = {};
    int value[READ_MAX_LABELS] = {};
};

struct ReadMemory {
    int64_t steps = 0; // steps ended
    int place = 0;     // catch, track: the paddle or marker at the last step's start
    int move = -1;     // the last step's move
    int cue = 0;       // the last step's cue
    int next[2] = {};  // predict-next: the "next" label one and two steps back
};

// the labels of the step about to be sensed; false while there are none yet (or never, for
// a kind this probe does not read)
bool read_labels(const Task& task, const ReadMemory& m, ReadLabels& out);
// once the step's move is chosen and before task_step: what the next step's labels need
void read_remember(const Task& task, ReadMemory& m, int move);
