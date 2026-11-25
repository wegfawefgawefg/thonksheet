#pragma once

#include <cstdint>

// Long-horizon worlds (idea 129, docs/curriculum.md "Long memory"). Five kinds:
//   hold   a cue, hold_steps blank steps (with or without distractors), then the ask
//   pairs  a never-ending stream of new pattern-to-button pairs, each taught for a short
//          while, re-asked once at each of five ages; the score is a forgetting curve
//   once   a new pattern shown a single time with its answer (on hint lines), asked once
//          much later
//   rare   a few patterns asked every step, a few asked one step in rare_every
//   drift  a fixed set of pairs; at drift_at half of them change button
// Patterns are `on` lines lit out of `lines`; the button of a pattern is a fixed hash of it,
// so a pattern means the same thing whenever it comes back.

struct Task;
struct TaskShape;
struct TaskSense;
struct TaskStepIn;

constexpr int LONG_AGES = 5;    // pairs, once: ages base x 1, 10, 100, 1,000, 10,000 steps
constexpr int LONG_BUCKETS = 8; // what a step is scored into (see long_bucket_name)

struct LongSettings {
    // hold
    int hold_steps = 1;  // blank steps between the cue and the ask
    int hold_cues = 2;   // cues and buttons
    int hold_noise = 0;  // 0 none; 1 distractor lines flash in the gap; 2 the cue lines
                         // themselves flash in the gap, the real cue is marked by its own line
    int noise_shift = 2; // a distractor flashes on a gap step with chance 2^-this
    // pattern worlds
    int lines = 28, on = 6, buttons = 4;
    int pair_every = 400; // pairs, once: steps between new pairs
    int pair_live = 4;    // pairs: the newest this many are taught side by side
    int age_base = 100;   // pairs, once: the first age asked, in steps
    int probe_reward = 0; // pairs: 1 rewards the probes too (each probe then re-teaches)
    int rare_common = 8;  // rare: patterns asked often
    int rare_count = 4;   // rare: patterns asked one step in rare_every
    int rare_every = 1000;
    int drift_pairs = 8;
    int64_t drift_at = 100000; // drift: the step at which odd pairs change button
};

struct LongState {
    uint64_t universe = 0; // patterns there are: lines choose on
    uint64_t stride = 1;   // pair i is pattern (i * stride + offset) mod universe
    uint64_t offset = 0;
    uint64_t salt = 0;      // the button of a pattern is a hash of it with this
    uint32_t lines_now = 0; // the singles lit this step
    int answer = -1;        // the right button this step, -1 none (press ignored)
    int bucket = -1;        // where this step is scored, -1 nowhere
    bool rewarded = false;  // this step's press gets good or bad
    int64_t trial_step = 0; // hold: steps into the trial
    int64_t trial_len = 0;
    int64_t right[LONG_BUCKETS] = {}, asked[LONG_BUCKETS] = {};   // whole life
    int64_t wright[LONG_BUCKETS] = {}, wasked[LONG_BUCKETS] = {}; // this window
};

// the age in steps that bucket j of pairs and once stands for
int64_t long_age(const LongSettings& s, int j);
// a short name for bucket j of a kind's tables ("" if the kind does not use it)
const char* long_bucket_name(const Task& task, int j);

void long_reset(Task& task);
TaskShape long_shape(const Task& task);
void long_sense(const Task& task, TaskSense& sense);
int long_perfect_move(const Task& task);
int long_step(Task& task, const TaskStepIn& in);
// clear the window tallies after the protocol read them
void long_window_clear(Task& task);
// true for the five kinds here
bool long_kind(const Task& task);

// blobrun's flags for these worlds (task_long_flags.cpp): true if `flag` was one
bool long_flag(const char* flag, int64_t v, LongSettings& ls);
extern const char* const long_usage;
