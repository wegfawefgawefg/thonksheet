#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

// learning that needs a reason: sensor prediction errors, and the good/bad broadcast.

// sensor prediction. on attached lines only: a miss strengthens the excitatory sources that
// were active and weakens the inhibitory ones; a false alarm does the opposite. lines that
// guessed right are left alone, so a held line cannot saturate its inputs.
void learn_predict(Blob& blob, int read_now, int boost) {
    const Genome& g = blob.genome;
    const uint64_t* inh = blob.inhibitory;
    int shift = boosted(g.predict_shift, boost);
    uint32_t up_count = 0, down_count = 0;

    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t lines = blob.sensor[t] & blob.sense_attached[t];
        if (!lines) {
            continue;
        }
        uint64_t miss = blob.sense_in[t] & ~blob.predicted[t] & lines;
        uint64_t false_alarm = blob.predicted[t] & ~blob.sense_in[t] & lines;
        uint64_t wrong = miss | false_alarm;
        while (wrong) {
            int b = __builtin_ctzll(wrong);
            wrong &= wrong - 1;
            int n = t * TILE_NEURONS + b;
            bool was_miss = (miss >> b) & 1;
            for (int k = 0; k < SLOTS; k++) {
                uint64_t s = slot_read(blob, read_now, n, k);
                uint64_t kind = inh[slot_source(blob, n, k)];
                uint64_t chance = s & sparse_word(blob.rng, shift);
                uint64_t& a0 = blob.w0[blob_weight_index(n, k)];
                uint64_t& a1 = blob.w1[blob_weight_index(n, k)];
                uint64_t more = was_miss ? chance & ~kind : chance & kind;
                uint64_t less = was_miss ? chance & kind : chance & ~kind;
                up_count += __builtin_popcountll(level_up_connected(a0, a1, more));
                down_count += __builtin_popcountll(level_down(a0, a1, less));
            }
            blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
        }
    }
    blob.stats.strengthened += up_count;
    blob.stats.weakened += down_count;
}

// good (+1) or bad (-1): every connection whose source and target both fired recently moves
// one level, by chance. nothing per synapse is stored; "recent" is the OR of the last maps.
void learn_reward(Blob& blob, int valence, int boost) {
    const Genome& g = blob.genome;
    const uint64_t* recent = blob.recent;
    int shift = boosted(g.reward_shift, boost);
    uint32_t moved = 0;

    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t post = recent[t];
        while (post) {
            int b = __builtin_ctzll(post);
            post &= post - 1;
            int n = t * TILE_NEURONS + b;
            for (int k = 0; k < SLOTS; k++) {
                uint32_t src = slot_source(blob, n, k);
                uint64_t& a0 = blob.w0[blob_weight_index(n, k)];
                uint64_t& a1 = blob.w1[blob_weight_index(n, k)];
                uint64_t pairs = recent[src] & connected_bits(a0, a1);
                if (!pairs) {
                    continue;
                }
                uint64_t chance = pairs & sparse_word(blob.rng, shift);
                if (valence > 0) {
                    moved += __builtin_popcountll(level_up(a0, a1, chance));
                } else {
                    moved += __builtin_popcountll(level_down(a0, a1, chance));
                }
            }
            if (valence < 0) {
                blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
            }
        }
    }
    if (valence > 0) {
        blob.stats.strengthened += moved;
    } else {
        blob.stats.weakened += moved;
    }
}
