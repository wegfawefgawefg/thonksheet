#include "blob/blob.hpp"
#include "blob/learn_rule.hpp"
#include "blob/long_memory.hpp"
#include "blob/output_state.hpp"
#include "blob/tick_steps.hpp"

// one tick: integrate and fire, regulate, learn.

// the learning step for the genome's learn mode (see genome.hpp)
static void tick_learn(Blob& blob, int read_now) {
    const Genome& g = blob.genome;
    int mode = g.learn_mode;
    int boost = blob.surprise_high ? g.surprise_boost : 0;
    // competitive learning on the hidden cells, beside whatever the learn mode does
    if (g.compete_on) {
        learn_compete(blob, read_now);
    }
    // the error sent one hop to every hidden cell, beside whatever the learn mode does
    if (g.hop_on) {
        learn_hop(blob);
    }
    // the blob's own output moves on the verdict just given (after the rule read its errors)
    if (g.out_on) {
        learn_output(blob);
    }
    // idea 120: the rule table, alone (1, 3) or beside the learn mode on the inside (2)
    if (g.rule_on) {
        learn_rule(blob, blob.pending_valence, g.rule_on == 3 ? 1 : (g.rule_on == 2 ? 2 : 0));
        if (g.rule_on != 2) {
            return;
        }
    }
    if (mode == 2) {
        learn_flips(blob);
        return;
    }
    if (mode == 5 || mode == 6) {
        // tags first: the feedback runs along the wiring that made the answer
        if (g.tag_mode && blob.pending_valence != 0) {
            learn_tags(blob, blob.pending_valence);
        }
        learn_replay(blob, blob.pending_valence); // idea 62; returns at once while off
        // barely correct: a good won by a thin vote; with margin_win 2 a comfortable good
        // skips the readout's own move on good (stop learning)
        bool thin = true;
        if (g.margin_percent && blob.pending_valence > 0) {
            thin = learn_margin(blob);
        }
        // hop_who 4: the motor cells learn by the one-hop rule, not the readout's
        // out_on: the motor cells' spikes are not the output, so their rule is skipped too
        bool hop_motor = (g.hop_on && (g.hop_who & 4)) || g.out_on;
        if ((thin || g.margin_win != 2) && !hop_motor) {
            learn_readout(blob, blob.pending_valence);
        }
        if (mode == 5) {
            return;
        }
    }
    // readout: no prediction learning, everything else only into motor neurons
    bool motor_only = mode == 1;
    if (g.predict_on && !motor_only) {
        learn_predict(blob, read_now, boost);
    }
    if (mode != 3) {
        learn_lose_shift(blob, boost);
        if (blob.good_ticks > 0 && g.good_on) {
            learn_reward(blob, +1, boost, motor_only);
        }
        if (blob.bad_ticks > 0 && g.bad_on) {
            learn_reward(blob, -1, boost, motor_only);
        }
    }
    if ((mode == 3 || mode == 4) && blob.pending_valence != 0) {
        learn_noise(blob, blob.pending_valence);
    }
    learn_wiring(blob, boost, motor_only);
}

void blob_tick(Blob& blob) {
    const Genome& g = blob.genome;
    int read_now = blob.now;
    int next = (blob.now + 1) & (HISTORY - 1);
    blob.stats.grown = blob.stats.pruned = 0;
    blob.stats.strengthened = blob.stats.weakened = 0;

    tick_fire(blob, blob.spikes[next]);
    blob.now = next;
    long_memory_after_fire(blob);
    if (g.hop_on && g.learning_on) {
        hop_after_fire(blob); // the near maps only feed learning
    }
    if (g.out_on && blob.out_read) {
        output_read(blob); // the world asked for the blob's own votes on this tick
    }

    // "fired recently": OR of the last recent_ticks maps
    for (int t = 0; t <= blob.tile_count; t++) {
        uint64_t r = 0;
        for (int age = 0; age < g.recent_ticks; age++) {
            r |= blob.spikes[(next - age) & (HISTORY - 1)][t];
        }
        blob.recent[t] = r;
    }

    tick_regulate(blob);

    if (g.learning_on) {
        tick_learn(blob, read_now);
    }
    blob.pending_valence = 0;
    if (blob.good_ticks > 0) {
        blob.good_ticks--;
    }
    if (blob.bad_ticks > 0) {
        blob.bad_ticks--;
    }
    blob.stats.tick++;
}
