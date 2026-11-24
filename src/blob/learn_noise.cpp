#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

// learn modes 3 and 4, a rival: noise is the experiment (REINFORCE at the synapse). the blob
// knows which spikes came from noise alone. a reward is compared with the running mean of
// rewards; better than the mean strengthens the recently active inputs of the neurons whose
// recent spike was noise, worse weakens them (never below strength 1, as with bad). how far
// the reward is from the mean sets the chance the event acts at all, so a blob that is
// nearly always right stops pushing on its good events. neurons that fired from their
// inputs get nothing from this rule.

// noise_learn_pair: the other half of the rule. a neuron that stayed quiet on a tick could
// have been fired by noise there, with the noise chance; grading only the ones that were
// leaves a push toward firing in every context that does better than the mean and away from
// it in every context that does worse, whether the spike mattered or not (idea 57). so a
// sample of quiet neuron-ticks, at the noise chance, is moved the other way.
static uint32_t learn_noise_pair(Blob& blob, bool better) {
    const Genome& g = blob.genome;
    uint32_t moved = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        // quiet on that tick and every later one, as a graded noise spike is the latest
        uint64_t quiet = 0, later = 0;
        for (int age = 0; age < g.noise_trace_ticks; age++) {
            int h = (blob.now - age) & (HISTORY - 1);
            later |= blob.spikes[h][t];
            quiet |= sparse_word(blob.rng, g.noise_shift) & ~later;
        }
        uint64_t post = quiet & ~blob.noise_trace[t] & ~blob.sensor[t];
        while (post) {
            int b = __builtin_ctzll(post);
            post &= post - 1;
            int n = t * TILE_NEURONS + b;
            for (int k = 0; k < SLOTS; k++) {
                uint32_t src = slot_source(blob, n, k);
                uint64_t& a0 = blob.w0[blob_weight_index(n, k)];
                uint64_t& a1 = blob.w1[blob_weight_index(n, k)];
                uint64_t pairs = blob.active_trace[src] & connected_bits(a0, a1);
                if (!pairs) {
                    continue;
                }
                uint64_t chance = pairs & sparse_word(blob.rng, g.noise_learn_shift);
                if (better) {
                    moved += __builtin_popcountll(level_down(a0, a1, chance & a1));
                } else {
                    moved += __builtin_popcountll(level_up(a0, a1, chance));
                }
            }
        }
    }
    return moved;
}

void learn_noise(Blob& blob, int valence) {
    const Genome& g = blob.genome;

    // reward against the running mean, both x256; the gap is at most 512
    int32_t gap = valence * 256 - blob.reward_average;
    blob.reward_average += gap >> g.reward_average_shift;
    int32_t size = gap < 0 ? -gap : gap;
    if ((int32_t)rng_below(blob.rng, 512) >= size) {
        return;
    }
    bool better = (gap > 0) == (g.noise_learn_sign >= 0);

    // over the last noise_trace_ticks maps: neurons whose latest spike was noise alone, and
    // every neuron that fired at all. newest first, so a later input-driven spike hides an
    // older noise spike.
    for (int t = 0; t <= blob.tile_count; t++) {
        uint64_t noise = 0, active = 0;
        for (int age = 0; age < g.noise_trace_ticks; age++) {
            int h = (blob.now - age) & (HISTORY - 1);
            noise |= blob.noise_ring[(int64_t)h * blob.spike_stride + t] & ~active;
            active |= blob.spikes[h][t];
        }
        blob.noise_trace[t] = noise;
        blob.active_trace[t] = active;
    }

    uint32_t moved = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t post = blob.noise_trace[t];
        while (post) {
            int b = __builtin_ctzll(post);
            post &= post - 1;
            int n = t * TILE_NEURONS + b;
            for (int k = 0; k < SLOTS; k++) {
                uint32_t src = slot_source(blob, n, k);
                uint64_t& a0 = blob.w0[blob_weight_index(n, k)];
                uint64_t& a1 = blob.w1[blob_weight_index(n, k)];
                uint64_t pairs = blob.active_trace[src] & connected_bits(a0, a1);
                if (!pairs) {
                    continue;
                }
                uint64_t chance = pairs & sparse_word(blob.rng, g.noise_learn_shift);
                if (better) {
                    moved += __builtin_popcountll(level_up(a0, a1, chance));
                } else {
                    moved += __builtin_popcountll(level_down(a0, a1, chance & a1));
                }
            }
        }
    }
    if (g.noise_learn_pair) {
        moved += learn_noise_pair(blob, better);
    }
    if (better) {
        blob.stats.strengthened += moved;
    } else {
        blob.stats.weakened += moved;
    }
}
