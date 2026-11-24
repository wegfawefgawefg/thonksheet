#pragma once

// The numbers a blob is born from. Plain integers only. A "shift" is a chance of 2^-shift.
// Defaults are a first guess for a 4,096-neuron blob; the core owner tunes them until one
// blob visibly learns. Later a search varies them.
struct Genome {
    // sheet size in tiles (a tile is 8x8 cells = 64 neurons)
    int tiles_w = 8;
    int tiles_h = 8;

    // sensor and motor regions, in cells. sensor neurons are all excitatory. inside the
    // sensor rect only every sensor_stride-th cell both ways is a sensor (a lattice), so
    // every sensor cell sits among ordinary neurons that hear it.
    int sensor_x = 0, sensor_y = 0, sensor_w = 32, sensor_h = 32;
    int sensor_stride = 2;
    int motor_x = 40, motor_y = 40, motor_w = 16, motor_h = 16;

    // kinds and spreads (each neuron draws its own value inside the range)
    int inhibitory_percent = 20;
    int threshold_min = 4, threshold_max = 12;
    int leak_shift_min = 1, leak_shift_max = 4;
    int cooldown_min = 1, cooldown_max = 4;
    int pacemakers_per_1024 = 4; // born with threshold 0, so they tick by themselves
    int pacemaker_period_min = 4, pacemaker_period_max = 16; // ticks between beats

    // wiring at birth
    int local_connect_shift = 3; // each possible local source is wired in with this chance
    int strong_shift = 2;        // a wired source starts strong (4) instead of weak (1)
    int far_percent = 15;        // share of excitatory neurons that get a far slot
    int far_reach_tiles = 3;     // typical far distance; the draw has a long tail
    int far_delay_per_tile = 1;  // ticks of delay per tile of distance (2..15 total)
    int local_delay_min = 1;     // local slot delays, drawn per neuron per slot, skewed short.
    int local_delay_max = 4;     // min = max = 1 is the old one-tick local reading
    int far_connect_shift = 2;   // each excitatory bit of a far tile is wired with this chance
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
    int threshold_cap = 64;        // regulation never raises a threshold past this
    int sector_smooth_shift = 4;   // sector activity is smoothed over about 2^this ticks

    // learning (all changes are one weight level, taken by chance)
    int learning_on = 1; // 0: no weight, wiring or far-redraw change at all (regulation runs)
    int recent_ticks = 6;        // how long "fired recently" lasts (1..8)
    int predict_shift = 3;       // sensor prediction: strengthen right, weaken wrong
    int reward_shift = 2;        // good/bad broadcast acting on recently co-active pairs
    int reward_ticks = 4;        // how long one good/bad event keeps acting
    int grow_shift = 5;          // on firing, chance to grow one new connection
    int prune_shift = 6;         // on firing, chance to weaken idle weak connections
    int prune_bit_shift = 4;     // then each idle weak connection goes with this chance
    int far_redraw_shift = 14;   // per tick, chance an empty far slot is redrawn
    int surprise_boost = 1;      // shifts taken off the chances while surprise is high
    int surprise_high_per_1024 = 80;
};
