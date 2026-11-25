#pragma once

#include <cstdint>

// What the shader reads and writes, as plain structs. Each mirrors a block in
// shaders/gpu/minimal_layout.glsl (std430); change both together.
//
// Lives are laid out side by side, life-major: life L's masks are words
// [L * tiles * 9 * 64, (L + 1) * tiles * 9 * 64) in the same order as the CPU engine
// (((tile * 9 + slot) * 64 + bit)), so one life's block is a straight copy of
// MinimalBlob::mask. Likewise thresholds (one byte per neuron), the spike and noise rings
// (32 maps of `stride` words each, spikes then noise) and the inhibitory words.
// One workgroup steps one life; a dispatch steps every life of a group by up to `ticks`.

constexpr int GPU_MAX_LINES = 32;  // sensor lines, good and bad included (a bit each)
constexpr int GPU_MAX_MOTORS = 16; // motor lines (buttons)
constexpr int GPU_RING = 32;       // MIN_HISTORY

enum GpuTaskKind : int32_t {
    GPU_ECHO = 0,
    GPU_SHUFFLED_ECHO = 1,
    GPU_DELAYED_ECHO = 2,
    GPU_BOTH_NOT_EITHER = 3,
};

// the world, the same for every life in a batch
struct GpuParams {
    int32_t lives; // in this group
    int32_t task;  // GpuTaskKind
    int32_t k, cue_offset, delay;
    int32_t first_single; // plug: first task line
    int32_t motor_lines;  // buttons
    int32_t ticks_per_step, rest_ticks, reward_line_ticks;
    int32_t window, steps, max_windows;
    uint32_t attached; // sensor lines attached, a bit each
    int32_t uses_reward;
    int32_t pad;
};

// the minimal genome's numbers the tick and the reward read, per life
struct GpuGenome {
    int32_t learning_on, budget_on, inhibit_gain, threshold_cap;
    int32_t up_shift, down_shift, noise_shift, credit_ticks;
    int32_t learn_shift, avg_shift, grade_mode, cowire_on;
    int32_t cowire_shift, cowire_needs_input, noise_sign, pad;
};

// one life's small state: the engine's clock and reward average, the task, the ticker
struct GpuLife {
    uint64_t key;   // MinimalBlob::rng_key
    uint64_t tick;  // ticks lived (Blob::stats.tick)
    uint64_t world; // the task's WorldRandom state
    int32_t now, reward_avg;
    int32_t cue, phase, trial; // trial: both-not-either's answer-step lines
    int32_t tick_in_step, rest_left, good_left, bad_left;
    uint32_t line_on; // task lines on, a bit per line
    int32_t step, right, chances, windows, done;
    int32_t pad;
    int32_t button_of[GPU_MAX_MOTORS];
    int32_t motor_count[GPU_MAX_MOTORS];
};

static_assert(sizeof(GpuParams) == 64, "mirrors minimal_layout.glsl");
static_assert(sizeof(GpuGenome) == 64, "mirrors minimal_layout.glsl");
static_assert(sizeof(GpuLife) == 216, "mirrors minimal_layout.glsl");
