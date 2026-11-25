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
    GPU_HOST_WORLD = 4, // any task: the world runs on the host, one exchange per step
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
    // birth only (minimal_birth.comp)
    int32_t budget, threshold_min, threshold_max, inhibitory_percent;
    // the grown-back pieces: slots in use (9 + far slots), delays, evidence counters
    int32_t slots, delay_max, far_percent, far_reach, far_delay_per_tile, delay_percent;
    int32_t ev_on, ev_bits, ev_init, ev_read, ev_spikes, ev_fired_min;
    int32_t ev_quiet, ev_quiet_shift, ev_shift, ev_inh, ev_cowire, pad2;
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
    int32_t block; // host worlds: the world table block of this life's world
    int32_t button_of[GPU_MAX_MOTORS];
    int32_t motor_count[GPU_MAX_MOTORS];
    // host worlds: buttons, and the lines attached (bit 0 good, 1 bad, 31 any task line)
    int32_t motor_lines;
    uint32_t attached;
    int32_t pad[2];
};

// host worlds (gpu_host.hpp): what crosses once per task step, per life. in: the reward of
// the step just answered and the lines of the next; out: the next step's motor counts.
struct GpuExchangeIn {
    int32_t valence; // the last step's good (+1) or bad (-1), 0 none
    int32_t run;     // 0: the life is over, its workgroup does nothing
    int32_t acted;   // the button the world says was pressed (-1 none)
    int32_t pad;
};
struct GpuExchangeOut {
    int32_t motor_count[GPU_MAX_MOTORS];
    int32_t surprise; // after the step's first tick (0 for the minimal engine)
    int32_t ticks;    // ticks this dispatch ran
    int32_t pad[2];
};

static_assert(sizeof(GpuParams) == 64, "mirrors minimal_layout.glsl");
static_assert(sizeof(GpuGenome) == 152, "mirrors minimal_layout.glsl");
static_assert(sizeof(GpuLife) == 232, "mirrors minimal_layout.glsl");
static_assert(sizeof(GpuExchangeIn) == 16, "mirrors minimal_layout.glsl");
static_assert(sizeof(GpuExchangeOut) == 80, "mirrors minimal_layout.glsl");

// one world's table: sensor, then MAX_LINES line masks, then MAX_MOTORS button masks, each
// tiles + 1 words. host worlds put one block per world side by side.
constexpr int GPU_WORLD_ROWS = 1 + GPU_MAX_LINES + GPU_MAX_MOTORS;
