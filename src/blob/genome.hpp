#pragma once

// The numbers a blob is born from. Plain integers only. A "shift" is a chance of 2^-shift.
// Defaults are a first guess for a 4,096-neuron blob; the core owner tunes them until one
// blob visibly learns. Later a search varies them.
struct Genome {
    // sheet size in tiles (a tile is 8x8 cells = 64 neurons)
    int tiles_w = 8;
    int tiles_h = 8;

    // sensor and motor regions, in cells. sensor neurons are all excitatory.
    int sensor_x = 0, sensor_y = 0, sensor_w = 16, sensor_h = 16;
    int motor_x = 48, motor_y = 48, motor_w = 16, motor_h = 16;

    // kinds and spreads (each neuron draws its own value inside the range)
    int inhibitory_percent = 20;
    int threshold_min = 4, threshold_max = 12;
    int leak_shift_min = 1, leak_shift_max = 4;
    int cooldown_min = 1, cooldown_max = 4;
    int pacemakers_per_1024 = 4; // born with threshold 0, so they tick by themselves

    // wiring at birth
    int local_connect_shift = 3; // each possible local source is wired in with this chance
    int strong_shift = 2;        // a wired source starts strong (4) instead of weak (1)
    int far_percent = 15;        // share of excitatory neurons that get a far slot
    int far_reach_tiles = 3;     // typical far distance; the draw has a long tail
    int far_delay_per_tile = 1;  // ticks of delay per tile of distance (2..15 total)
    int fan_in_max = 96;         // cap on connections per neuron

    // integration
    int inhibit_gain = 2;
    int branch_threshold = 3; // excitation in one slot that earns the bonus
    int branch_bonus = 2;
    int noise_shift = 10; // spontaneous firing chance per neuron per tick

    // regulation
    int sector_tiles = 2;             // a sector is this many tiles on a side
    int sector_target_per_1024 = 40;  // wanted spikes per 1,024 neurons per tick
    int sector_bias_max = 16;
    int threshold_up_shift = 4;    // on firing, raise own threshold with this chance
    int threshold_down_shift = 9;  // every tick, lower own threshold with this chance

    // learning (all changes are one weight level, taken by chance)
    int recent_ticks = 6;        // how long "fired recently" lasts (1..8)
    int predict_shift = 3;       // sensor prediction: strengthen right, weaken wrong
    int reward_shift = 2;        // good/bad broadcast acting on recently co-active pairs
    int reward_ticks = 4;        // how long one good/bad event keeps acting
    int grow_shift = 5;          // on firing, chance to grow one new connection
    int prune_shift = 6;         // on firing, chance to weaken idle weak connections
    int far_redraw_shift = 14;   // per tick, chance an empty far slot is redrawn
    int surprise_boost = 1;      // shifts taken off the chances while surprise is high
    int surprise_high_per_1024 = 80;
};
