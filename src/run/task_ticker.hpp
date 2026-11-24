#pragma once

#include "run/run_flags.hpp"
#include "world/plug.hpp"
#include "world/plug_babble.hpp"
#include "world/task.hpp"

#include <cstdint>

// One task step spread over blob ticks, so a caller can stop between any two ticks (the
// viewer's single step) or run whole steps (blobrun). The caller ticks the blob itself and
// broadcasts the reward, since only it knows what kind of blob it has:
//
//   int perfect = ticker_before_tick(tt, blob);  // senses on a step's first tick
//   tick the blob
//   if (ticker_after_tick(tt, blob)) {           // the step's answer is in
//       if (tt.valence != 0) reward the blob with tt.valence
//   }
//
// A step is ticks_per_step ticks that sense and choose; after a rewarded step come
// rest_ticks quiet ticks, which belong to that step but run before the next one senses.
struct TaskTicker {
    Task task;
    Plug plug;
    TaskSense sense;
    TaskStepIn in;
    int ticks_per_step = 8;
    int rest_ticks = 0;
    int tick_in_step = 0; // choosing ticks done in this step
    int rest_left = 0;    // quiet ticks still owed by the last step
    bool resting = false; // the tick under way is one of them
    int perfect_move = -1;

    // the step that answered last
    int cue = 0;         // task.cue while it was sensed
    int right_move = -1; // what the perfect player would have done
    int move = -1;       // what the blob did (-1 no move)
    int valence = 0;     // as the blob gets it (balance may have dropped it)

    Babble babble; // ideas 60 and 61; does nothing when off
};

// a fresh task on a blob that is already born. false if the plug does not fit.
bool ticker_start(TaskTicker& tt, Blob& blob, const RunFlags& flags);
// before each blob tick: a new step senses, then the lines go onto the blob. returns the
// move a scripted perfect player would make this tick (-1 none, and in rest ticks).
int ticker_before_tick(TaskTicker& tt, Blob& blob);
// after each blob tick: counts motor spikes; true on the tick a step's answer is in (the
// task has moved on, move and valence are set, the plug's good/bad line is raised).
bool ticker_after_tick(TaskTicker& tt, const Blob& blob);
