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

    // barely correct (Baldassi et al. 2007; learn_margin.cpp), learn modes 5 and 6 with
    // readout_evidence. on good, when the pressed button won by less than margin_percent of
    // its own spikes, its cells deepen the counters of their wired, active, excitatory sources
    // (margin_win), and the runner-up's cells push active excitatory sources down
    // (margin_rival 1: unwired ones only, hidden; 2: every active one, sign-aware) with
    // chance 2^-(readout_shift + margin_rival_shift). margin_win 2: the stop-learning gate
    // only; the readout's own move on good (readout_good 1) runs only on a thin vote.
    // 0 is off and changes nothing.
    int margin_percent = 0;
    int margin_win = 1;
    int margin_rival = 0;
    int margin_rival_shift = 1;
    // action-feedback tags (AGREL / BrainProp; learn_tags.cpp), learn modes 5 and 6. at each
    // verdict the chosen button's cells send feedback back along their wired, active inputs
    // (summed, signed by the edge: an inhibitory source gets the opposite sign); neurons whose
    // feedback reaches tag_min are tagged. tag_hops 2 sends it one step further, thinned by
    // 2^-tag_thin per edge. the verdict against a running mean then moves the tagged
    // neurons' hidden counters for their active inputs: toward on better, away on worse, with
    // chance 2^-tag_shift; each neuron wires its top counters at its born count and born
    // strong count. tag_mode 0 off (changes nothing), 1 feedback from the chosen button only,
    // 2 the chosen button's feedback less the other buttons'.
    int tag_mode = 0;
    int tag_hops = 1;
    int tag_thin = 1;
    int tag_min = 1;
    int tag_shift = 4;
    int tag_far = 1;     // 1: feedback also runs back along far slots
    int tag_inh = 1;     // 1: inhibitory neurons are tagged (opposite sign) and learn; 0: never
    int tag_avg_shift = 5; // the running mean of rewards spans about 2^this verdicts
    int tag_bits = 6;    // counter size, signed (2..8)
    int tag_verdict = 0; // 0 against the running mean, as above; 1 every bad moves, no good does
    // wiring at birth (ideas 19-21, 77-80, 83; birth_shape.cpp). all 0 is today's wiring and
    // draws exactly what it drew before. a shaped neuron keeps today's expected count of local
    // connections; only where they come from changes. "doubling": 2x likelier.
    // 19: in, middle, out groups by bit; the favoured flows are 2^this likelier
    int wire_layers = 0;
    // 21: halvings of the chance per 4 cells of distance
    int wire_fine = 0;
    // 20: listen to this many of the 9 local tiles (own tile always)
    int wire_slots = 0;
    // 77: where A listens to B, B adds A with this chance (count kept)
    int wire_mirror_percent = 0;
    // 78: share of hub neurons; a hub has 2^shift times the connections in, and is 2^shift
    // times likelier to be picked as a source
    int wire_hub_per_1024 = 0;
    int wire_hub_shift = 2;
    // 79: doublings per 8 cells upstream; direction 0..15 in sixteenths of a turn (0 flows
    // toward +x, 4 toward +y); turn: sixteenths of a turn the flow turns across the width
    int wire_stream = 0;
    int wire_stream_dir = 0;
    int wire_stream_turn = 0;
    // 83: 1 a tile's far links land on one tile per slot; 2 a map, one shift per sector, so
    // neighbouring tiles land near each other. jitter: tiles of scatter around the landing
    int wire_far_topo = 0;
    int wire_far_jitter = 0;
    // kinds by place. >0: inhibitory neurons listen wider (own tile 2^-this as likely). <0:
    // inhibition stays local (inhibitory neurons favour their own tile, and are 2^this less
    // likely as sources in the neighbours)
    int wire_inhib_reach = 0;
    // 80: no local wiring across the middle row; this percent of the far half's far slots are
    // fed from the same place on the near half; the far half sends back to 1 in 8 of the near
    // half's far links
    int wire_two_sheets = 0;
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
    // idea 120: the learning rule as a table, loaded with --rule-file (learn_rule.hpp).
    // 0 off. 1 the table is the only learning, on every neuron but sensor cells. 2 the learn
    // mode runs as usual and the table too, on neurons that are not button cells (the inside).
    // 3 the table is the only learning, on button cells only (the sheet frozen)
    int rule_on = 0;

    // ideas 66-75, what a connection does over a few ticks and what a neuron computes
    // (fire_dynamics.cpp, docs/results-2026-11-25-family-dynamics.md). 0 is off for every one,
    // and off changes nothing.
    int syn_tire_ticks = 0;      // 66: a spike from a source that fired in the last N ticks...
    int syn_tire_keep_shift = 1; //     ...reaches its targets only with chance 2^-this
    int syn_warm_ticks = 0;      // 67: a spike from a source that fired in the last N ticks...
    int syn_warm_shift = 0;      //     ...counts twice, with chance 2^-this
    int tire_jump = 0;           // 68: threshold steps added on firing, fading...
    int tire_shift = 4;          //     ...by 2^-this of what is left per tick
    int slow_leak_shift = 0;     // 69: a second potential leaking by 2^-this per tick...
    int slow_share_shift = 6;    //     ...of which 2^-this is added to the first at the compare
    int rebound_level = 0;       // 70: a potential below minus this (inhibited)...
    int rebound_ticks = 4;       //     ...leaves the threshold lower for this many ticks...
    int rebound_drop = 4;        //     ...by this much
    int updown_drop = 0;         // 71: a neuron that fired is "up": its threshold is this much
    int updown_floor = 0;        //     lower until its potential falls below this
    int any_branch_percent = 0;  // 72: share of neurons driven by their best slot, not the sum
    int veto_percent = 0;        // 73: share of neurons where one inhibitory spike in a slot
                                 //     cancels that slot's excitation
    int coincide_percent = 0;    // 74: share of neurons that fire only when two slots with
                                 //     different delays both reach branch_threshold
    int tile_winners = 0;        // 75: at most this many neurons of a tile keep their spike,
                                 //     the ones furthest over threshold
    int syn_sensors = 1;         // 66, 67: 1 sensor cells' spikes tire and warm too; 0 not
    // long memory (long_memory.cpp). 0 is off and changes nothing.
    int hold_percent = 0;         // integrator cells: share of excitatory inner neurons
    int hold_cap = 15;            // their onset count stops here
    int readout_lock_levels = 0;  // the freezer on readout evidence: lock depth, up to 3
    int readout_lock_shift = 4;   // a push at the end of the range deepens with 2^-this
    int readout_lock_hold = 3;    // a move against depth d goes with 2^-(this * d)
    int readout_decay = 0;        // per motor-cell update, this many random counters step
                                  // toward 0 (locked ones do not): forgetting makes room
    int readout_decay_idle = 0;   // 1: only sources idle in the graded window decay
    int readout_lock_from = 0;    // a push agreeing with a counter this strong deepens a
                                  // lock (0: only at the end of the range)
    int readout_lock_fade = 0;    // a decay pick on a lock lowers it with 2^-this (0 never)
    int time_percent = 0;         // time cells: share of excitatory inner neurons
    int time_levels = 14;         // their log bins, 16 ticks up to 2^(4 + this)

    // actor and critic (learn_critic.cpp, learn_actor.cpp; Barto, Sutton and Anderson 1983).
    // the world calls blob_reward every step; a group of critic cells beside the motor
    // lattice learns to fire more where things go well, and the change in their spike
    // count plus the reward is the surprise that teaches the readout instead of raw good
    // and bad. needs learn mode 5 and readout_evidence. 0 is off and changes nothing.
    int critic_on = 0;
    int critic_reward = 16;        // level, in spikes, that one good or bad is worth
    int critic_margin = 4;         // a surprise this close to zero is neither good nor bad
    int critic_center = 1;         // 1: reward counts against its running mean
    int critic_average_shift = 8;  // that mean is over about 2^this steps
    int critic_shift = 4;          // critic chance per source spike per update, 2^-this
    int critic_trace_steps = 4;    // steps back a surprise reaches, for the critic (1..8)
    int critic_fade = 1;           // each step back makes the chance 2^-this smaller
    int actor_form = 0;            // 0 error rule (readout_good decides the good half),
                                   // 1 reinforce: the pressed button alone, both ways
    int actor_trace_steps = 4;     // steps back a surprise reaches, for the actor (1..8)
    int actor_fade = 1;            // each step back makes the chance 2^-this smaller
    int explore_shift = 0;         // chance per step, 2^-this, of a random press; 0 never
    int explore_scale = 0;         // the chance halves per this many spikes the critic's
                                   // level stands above its mean; 0 keeps it fixed
    int critic_opponent = 1;       // 1: half the critic cells (a checkerboard) learn the other
                                   // way and count against, so what all cells share cancels
    int critic_wide = 0;           // 1: every non-motor cell of the motor region is a critic
                                   // cell (768 by default), not only those between (256)

    // one inner rule (idea 155; learn_tags.cpp, tag_store.cpp): the tags above with the
    // pieces of reverse replay, each behind its own number. the defaults are the tags as
    // they were, and change nothing.
    int tag_exact = 0;   // 1: spike-exact (the walk's credit): a button cell's feedback goes
                         // only along an edge whose source fired one slot delay before one of
                         // the cell's spikes in the step, and a tagged neuron moves counters
                         // only for sources that fired one delay before its own spikes
    int tag_grow = 1;    // 0: only wired sources' counters move, so nothing new is wired in
    int tag_counts = 0;  // 0: born counts of excitatory, inhibitory and strong connections
                         // kept; 1: the two kinds' counts kept, strength from the counter
                         // (drive can grow); 2: one count for both kinds, strength from the
                         // counter, nothing at zero wired (the walk's read-off)
    int tag_shuffle = 0; // control: each button cell sends feedback with a random role
                         // (pressed or other), as many pressed roles expected as real
    int tag_random = 0;  // control: as many neurons as were tagged, drawn at random among the
                         // learners that fired in the step, carry the tags instead
    // competitive learning (learn_compete.cpp, docs/results-2026-11-25-compete.md), any learn
    // mode, no reward anywhere. a hidden cell (not sensor, motor or pacemaker; excitatory
    // unless compete_inh) that fires moves its hidden counters (the tag store) toward what
    // made it fire: each excitatory source its slot read active on that spike gains a step,
    // each silent one whose counter is above zero loses one with chance 2^-compete_down_shift;
    // then it wires its top counters at its born counts, so its input stays the same size.
    // compete_on 2: also, with tile_winners, cells over threshold that lost the tile's
    // competition push their active sources down. 0 is off and changes nothing.
    int compete_on = 0;
    int compete_shift = 3;      // a spike (or a loss) moves the cell with chance 2^-this
    int compete_down_shift = 2;
    int compete_inh = 0;

    // the error sent one hop (direct feedback alignment in bits, Nokland 2016, with marks as
    // in e-prop; learn_hop.cpp, docs/results-2026-11-25-hop.md), any learn mode. the world
    // names the lines pressed and right at each verdict; each error line is wired to a hidden
    // excitatory cell with chance 2^-hop_wire_shift, its sign drawn from a hash of the cell
    // and the line (no memory). a cell's teaching bit is the sign of its wires times the
    // errors: fire more, or less. its marked sources (on while the cell was near its bar in
    // the last hop_window ticks) move hidden counters (the tag store) by that sign with
    // chance 2^-hop_shift, and the cell rewires its top counters at its born counts, only
    // when a counter crossed a wired one. 0 is off and changes nothing.
    int hop_on = 0;
    int hop_teach = 1;      // 1: told good or bad (bad: the pressed line should be lower);
                            // 2: told the right line too (it should have been higher);
                            // 3: every verdict, good or bad, teaches from the answer alone:
                            // a line's error is its target (1 right, 0 not) less its running
                            // rate of being right, so what the readout pressed plays no part
    int hop_good = 0;       // 1: on good, the pressed line should have been higher
    int hop_wire_shift = 1; // each error line reaches a cell with chance 2^-this
    int hop_near = 2;       // "near": potential within this many eighths of the bar under it
    int hop_fired = 1;      // 1: a cell that fired is near too; 0: only those that did not
    int hop_window = 12;    // ticks before the verdict whose marks count (1..15)
    int hop_shift = 3;      // chance per marked source per verdict, 2^-this
    int hop_coin = 0;       // control: every error goes to a random line instead; 2: to
                            // distinct random lines, so none cancel (two-line tasks)
    int hop_rate_shift = 8; // hop_teach 3: the running rates span about 2^this verdicts
    // wider and deeper cells (docs/results-2026-11-25-hop-real.md, idea 163). 0 is off and
    // changes nothing. "hidden excitatory": not sensor, motor, pacemaker or inhibitory.
    int wire_pixel_shift = 0;  // >0: at birth a hidden excitatory cell also wires each sensor
                               // cell of its 9 local tiles with chance 2^-this (strong as usual)
    int wire_deep_percent = 0; // share of hidden excitatory cells (by a hash of the cell) that
                               // hear no sensor cell: none at birth, none after any rewire
};

// "--g.name value" on a command line overrides one genome number, for tuning runs
void apply_genome_args(int argc, char** argv, Genome& g);
