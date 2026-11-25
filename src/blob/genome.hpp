#pragma once

// The numbers a blob is born from. Plain integers only. A "shift" is a chance of 2^-shift.
// Defaults are a first guess for a 4,096-neuron blob; the core owner tunes them until one
// blob visibly learns. Later a search varies them.
struct Genome {
    // sheet size in tiles (a tile is 8x8 cells = 64 neurons)
    int tiles_w = 8;
    int tiles_h = 8;

    // sensor and motor regions, in cells. inside each rect only every stride-th cell both
    // ways is a sensor (or motor) neuron, a lattice, so each sits among ordinary neurons.
    // sensor neurons are all excitatory; motor neurons are ordinary in every other way.
    int sensor_x = 0, sensor_y = 0, sensor_w = 32, sensor_h = 32;
    int sensor_stride = 2;
    int motor_x = 0, motor_y = 32, motor_w = 32, motor_h = 32; // shares an edge with the input area
    int motor_stride = 2;

    // kinds and spreads (each neuron draws its own value inside the range)
    int inhibitory_percent = 25;
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
    int inhibit_gain = 5;
    int branch_threshold = 3; // excitation in one slot that earns the bonus
    int branch_bonus = 2;
    int noise_shift = 10; // spontaneous firing chance per neuron per tick

    // regulation
    int sector_tiles = 2;            // a sector is this many tiles on a side
    int sector_target_per_1024 = 32; // wanted spikes per 1,024 neurons per tick; keep it
                                     // at the per-neuron balance, 1024 * 2^(up - down)
    int sector_bias_max = 16;
    int threshold_up_shift = 4;   // on firing, raise own threshold with this chance
    int threshold_down_shift = 9; // every tick, lower own threshold with this chance
    int threshold_cap = 64;       // regulation never raises a threshold past this
    int sector_smooth_shift = 4;  // sector activity is smoothed over about 2^this ticks

    // learning (all changes are one weight level, taken by chance)
    int learning_on = 1;       // 0: no weight, wiring or far-redraw change at all (regulation runs)
    int recent_ticks = 6;      // how long "fired recently" lasts (1..8)
    int predict_shift = 3;     // sensor prediction: on a wrong guess, move the active sources
    int reward_shift = 4;      // good/bad broadcast on recently co-active pairs (bad stops at 1)
    int reward_ticks = 4;      // how long one good/bad event keeps acting
    int grow_shift = 5;        // on firing, chance to grow one new connection
    int prune_shift = 6;       // on firing, chance to weaken idle weak connections
    int prune_bit_shift = 4;   // then each idle weak connection goes with this chance
    int far_redraw_shift = 14; // per tick, chance an empty far slot is redrawn
    int surprise_boost = 1;    // shifts taken off the chances while surprise is high
    int surprise_high_per_1024 = 80;

    // switches for ablation runs. 1 is the design; 0 takes that one mechanism out and
    // changes nothing else. (other mechanisms switch off through their own numbers:
    // pacemakers_per_1024 0, far_percent 0, inhibitory_percent 0, branch_bonus 0,
    // surprise_boost 0, local_delay_min = max = 1, cooldown_min = max = 0.)
    int sector_reg_on = 1;    // sector bias moves toward the target (0: bias stays 0)
    int threshold_reg_on = 1; // per-neuron threshold up on firing, down by chance
    int noise_on = 1;         // spontaneous firing
    int predict_on = 1;       // sensor prediction learning
    int good_on = 1;          // the good broadcast strengthens co-active pairs
    int bad_on = 1;           // the bad broadcast weakens them
    int grow_on = 1;
    int prune_on = 1;
    int far_redraw_on = 1;
    int strong_log = 2; // a w1 bit counts 2^this: 2 gives strengths 0,1,4,5; 1 gives 0,1,2,3

    // learning mode, for the rivals the brain-style rule has to beat (docs/plan.md).
    // 0 the design. 1 readout: only connections into motor neurons learn (good/bad, growth,
    // pruning); nothing else changes. 2 random flips: flip a few weight bits, keep them if the
    // reward over the next window beat the running average, else undo; nothing else learns.
    // 3 noise is the experiment: good/bad grades only neurons whose recent spike was noise,
    // in place of the good/bad rule. 4 the same, alongside the good/bad rule.
    int learn_mode = 0;
    int flip_count = 4;           // mode 2: bits flipped per trial
    int flip_scope = 0;           // mode 2: 0 anywhere, 1 only connections into motor neurons
    int flip_window_ticks = 1000; // mode 2: ticks one trial lives
    int flip_average_shift = 3;   // mode 2: the reference is a running mean over ~2^this trials
    int noise_learn_shift = 2;    // modes 3, 4: chance per eligible connection
    int noise_trace_ticks = 16;   // modes 3, 4: how far back a noise spike counts (1..16)
    int reward_average_shift = 5; // modes 3, 4: running mean of rewards over ~2^this events

    // learn mode 5: an error rule at the motor cells, nothing else learns (idea 58,
    // learn_readout.cpp). 6: the same at the motor cells, and the design rule everywhere.
    int readout_shift = 6;        // chance per eligible input per update
    int readout_trace_ticks = 15; // the ticks before the reward that count as "active" (1..15)
    int readout_counts = 1;       // 1: one chance per tick a source fired; 0: one if it fired at all
    int readout_know_button = 1;  // 1: the world marks the pressed button's motor cells. 0: a
                                  // motor cell only knows whether it fired itself
    int readout_good = 0;         // 1: on good the pressed button gains and the others lose
    int readout_new = 1;          // 1: gaining may wire a source in that was not connected
    int readout_budget = 0;       // 1: a motor cell keeps its count of connections; a new
                                  // one costs an idle one, a lost one is replaced by an idle one
    int readout_center = 0;       // 1: a source moves by its spikes now less those in the last
                                  // rewarded window (needs readout_counts)
    int readout_evidence = 0;        // 1: the rule moves hidden counters per source; a cell
                                     // wires the top of its budget (learn_evidence.cpp)
    int readout_evidence_bits = 6;   // counter size, signed (2..8)
    int readout_evidence_strong = 1; // wired sources at strength 4 (0: at 1)
    int readout_balance = 0;         // 1: the other buttons move less, by their number
    int readout_far = 0;          // 1: a marked motor cell with an empty far slot gets one drawn,
                                  // so it hears two more tiles than its 9 local ones
    // lose-shift and neighbours (idea 53, learn_lose_shift.cpp). 0 is off and changes nothing.
    int lose_shift_steps = 0; // on bad, threshold steps added to motor cells that just fired
    int win_stay_steps = 0;   // on good, threshold steps taken off them
    int lose_shift_fires = 1; // "just fired": at least this many spikes (1..3) in the last 16 ticks
    int bad_cut_on = 0; // bad may remove strength-1 links, only into recently fired motor cells
    // idea 57 (docs/results-2026-11-24-noise-sign.md), modes 3 and 4 only.
    // noise_learn_sign -1 turns the rule over: better than the mean weakens, worse strengthens.
    // noise_learn_pair 1 takes the mean out of the rule: neurons that stayed quiet on a tick
    // are graded too, with the chance a noise spike has, and moved the other way.
    int noise_learn_sign = 1;
    int noise_learn_pair = 0;
    // reverse replay (idea 62, learn_replay.cpp), on top of learn mode 5 or 6. on each error
    // step the readout's sign walks back through the spike ring along edges that carried a
    // spike, moving hidden evidence counters of inner neurons. 0 everywhere is off.
    int replay_depth = 0; // steps back (layers of inner neurons taught), 0 off
    int replay_shift = 4; // chance per active source per walked spike of a counter step
    int replay_thin = 1;  // each step past the first keeps a walked spike with 2^-this
    int replay_new = 1;   // 1: active sources not yet connected gain too (the walk can grow)
    int replay_quiet = 0; // 1: cells that should have fired more walk from every tick of the
                          // window, not only from their spikes
    int replay_until = 0; // tick at which the walk stops for good (0: never)
    int replay_reset = 0; // at replay_until: 1 the motor cells go back to born wiring and the
                          // readout's counters are cleared; 2 the same, then all learning stops
    int replay_shuffle = 0; // control: 1 gives each motor spike walked a random sign
    int replay_strong_budget = 0; // 1: a walked neuron keeps its count of strong connections
                                  // too, and none goes to 5 (replay_wire.cpp)
};

// "--g.name value" on a command line overrides one genome number, for tuning runs
void apply_genome_args(int argc, char** argv, Genome& g);
