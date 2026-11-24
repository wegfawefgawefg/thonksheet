#include "blob/blob.hpp"
#include "blob/tick_steps.hpp"

// one tick: integrate and fire, regulate, learn.

// the learning step for the genome's learn mode (see genome.hpp)
static void tick_learn(Blob& blob, int read_now) {
    const Genome& g = blob.genome;
    int mode = g.learn_mode;
    int boost = blob.surprise_high ? g.surprise_boost : 0;
    if (mode == 2) {
        learn_flips(blob);
        return;
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
