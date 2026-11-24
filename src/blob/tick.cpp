#include "blob/blob.hpp"
#include "blob/tick_steps.hpp"

// one tick: integrate and fire, regulate, learn.

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
        int boost = blob.surprise_high ? g.surprise_boost : 0;
        if (g.predict_on) {
            learn_predict(blob, read_now, boost);
        }
        if (blob.good_ticks > 0 && g.good_on) {
            learn_reward(blob, +1, boost);
        }
        if (blob.bad_ticks > 0 && g.bad_on) {
            learn_reward(blob, -1, boost);
        }
        learn_wiring(blob, boost);
    }
    if (blob.good_ticks > 0) {
        blob.good_ticks--;
    }
    if (blob.bad_ticks > 0) {
        blob.bad_ticks--;
    }
    blob.stats.tick++;
}
