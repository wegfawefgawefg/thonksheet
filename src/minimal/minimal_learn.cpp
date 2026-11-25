#include "blob/random_words.hpp"
#include "minimal/minimal.hpp"
#include "minimal/minimal_steps.hpp"

#if defined(__BMI2__)
#include <immintrin.h>
#endif

// the reward rule (noise is the experiment, against a running average of reward) and the
// swaps that keep every neuron on its budget.

// the r-th set bit of a word (r < popcount)
static int nth_bit(uint64_t word, int r) {
#if defined(__BMI2__)
    return __builtin_ctzll(_pdep_u64(1ull << r, word));
#else
    for (int i = 0; i < r; i++) {
        word &= word - 1;
    }
    return __builtin_ctzll(word);
#endif
}

// one set bit, uniformly, over a neuron's slot words. false if all are empty.
bool min_pick_one(uint64_t* rng, const uint64_t* words, int slots, int& slot, int& bit) {
    int counts[MIN_SLOTS_MAX];
    int total = 0;
    for (int k = 0; k < slots; k++) {
        counts[k] = __builtin_popcountll(words[k]);
        total += counts[k];
    }
    if (total == 0) {
        return false;
    }
    int r = (int)rng_below(rng, (uint32_t)total);
    for (int k = 0; k < slots; k++) {
        if (r < counts[k]) {
            slot = k;
            bit = nth_bit(words[k], r);
            return true;
        }
        r -= counts[k];
    }
    return false;
}

// drop one connection that was quiet at the spike `age` ticks ago: a random one, or
// (drop_mode 1) one not marked used since the neuron's last sweep; when all are marked, sweep
static void drop_one(MinimalBlob& m, int n, int age) {
    const int slots = m.slots;
    uint64_t idle[MIN_SLOTS_MAX];
    for (int k = 0; k < slots; k++) {
        idle[k] = m.mask[min_index(m, n, k)] & ~min_heard(m, age, n, k);
    }
    if (m.used) {
        uint64_t stale[MIN_SLOTS_MAX];
        uint64_t any = 0;
        for (int k = 0; k < slots; k++) {
            stale[k] = idle[k] & ~m.used[min_index(m, n, k)];
            any |= stale[k];
        }
        if (!any) {
            for (int k = 0; k < slots; k++) {
                m.used[min_index(m, n, k)] = 0;
            }
        } else {
            for (int k = 0; k < slots; k++) {
                idle[k] = stale[k];
            }
        }
    }
    int slot = 0, bit = 0;
    if (min_pick_one(m.rng, idle, slots, slot, bit)) {
        m.mask[min_index(m, n, slot)] &= ~(1ull << bit);
        m.stats.lost++;
    }
}

bool min_gain_active(MinimalBlob& m, int n, int age) {
    const int slots = m.slots;
    uint64_t fresh[MIN_SLOTS_MAX];
    for (int k = 0; k < slots; k++) {
        fresh[k] = min_heard(m, age, n, k) & ~m.mask[min_index(m, n, k)] & min_allowed(m, n, k);
    }
    int slot = 0, bit = 0;
    if (!min_pick_one(m.rng, fresh, slots, slot, bit)) {
        return false;
    }
    m.mask[min_index(m, n, slot)] |= 1ull << bit;
    m.stats.gained++;
    if (m.mg.budget_on) {
        drop_one(m, n, age);
    }
    return true;
}

bool min_lose_active(MinimalBlob& m, int n, int age) {
    const int slots = m.slots;
    uint64_t wired[MIN_SLOTS_MAX];
    for (int k = 0; k < slots; k++) {
        wired[k] = min_heard(m, age, n, k) & m.mask[min_index(m, n, k)];
    }
    int slot = 0, bit = 0;
    if (!min_pick_one(m.rng, wired, slots, slot, bit)) {
        return false;
    }
    m.mask[min_index(m, n, slot)] &= ~(1ull << bit);
    m.stats.lost++;
    if (!m.mg.budget_on) {
        return true;
    }
    // the budget: a random unwired source that was quiet takes its place
    uint64_t other[MIN_SLOTS_MAX];
    for (int k = 0; k < slots; k++) {
        other[k] = ~min_heard(m, age, n, k) & ~m.mask[min_index(m, n, k)] & min_allowed(m, n, k);
    }
    if (min_pick_one(m.rng, other, slots, slot, bit)) {
        m.mask[min_index(m, n, slot)] |= 1ull << bit;
        m.stats.gained++;
    }
    return true;
}

void min_mark_used(MinimalBlob& m, const uint64_t* fired) {
    for (int t = 0; t < m.shell.tile_count; t++) {
        uint64_t f = fired[t] & ~m.shell.sensor[t];
        while (f) {
            int n = t * TILE_NEURONS + __builtin_ctzll(f);
            f &= f - 1;
            for (int k = 0; k < m.slots; k++) {
                int64_t w = min_index(m, n, k);
                m.used[w] |= m.mask[w] & min_heard(m, 0, n, k);
            }
        }
    }
}

// the graded neurons: for each age inside the credit window, the neurons whose latest spike
// is at that age and was noise (grade_mode 1: any spike). each moves one connection by chance
// toward (better) or away from (worse) the sources it heard when it fired.
static void grade(MinimalBlob& m, bool better, int shift) {
    const MinimalGenome& mg = m.mg;
    for (int t = 0; t < m.shell.tile_count; t++) {
        // counter neurons learn through their counters, the readout's cells by its rule
        uint64_t elsewhere = (m.ev_tiles && mg.ev_on ? m.ev_tiles[t] : 0) | min_readout_cells(m, t);
        uint64_t not_sensor = ~m.shell.sensor[t];
        uint64_t seen = 0;
        for (int a = 0; a < mg.credit_ticks; a++) {
            uint64_t spikes = minimal_map(m, m.now - a)[t];
            uint64_t noise = minimal_noise_map(m, m.now - a)[t];
            uint64_t pick = (mg.grade_mode == 0 ? noise : spikes) & ~seen & not_sensor;
            if (mg.noise_pair && mg.grade_mode == 0) {
                // the other half (idea 57): neurons quiet on this tick and on every later one
                // in the window, at the noise chance, moved the other way
                uint64_t quiet = ~spikes & ~seen & not_sensor & sparse_word(m.rng, mg.noise_shift) &
                                 sparse_word(m.rng, shift) & ~elsewhere;
                while (quiet) {
                    int n = t * TILE_NEURONS + __builtin_ctzll(quiet);
                    quiet &= quiet - 1;
                    if (better) {
                        min_lose_active(m, n, a);
                    } else {
                        min_gain_active(m, n, a);
                    }
                }
            }
            seen |= spikes;
            if (!pick) {
                continue;
            }
            pick &= sparse_word(m.rng, shift) & ~elsewhere;
            while (pick) {
                int n = t * TILE_NEURONS + __builtin_ctzll(pick);
                pick &= pick - 1;
                m.stats.graded++;
                if (better) {
                    min_gain_active(m, n, a);
                } else {
                    min_lose_active(m, n, a);
                }
            }
        }
    }
}

void minimal_reward(Blob& blob, int valence) {
    MinimalBlob& m = minimal_of(blob);
    if (valence == 0) {
        return;
    }
    // reward against its running average; the size of the gap sets the chance of change
    int delta = (valence > 0 ? 256 : -256) - m.reward_avg;
    m.reward_avg += delta / (1 << m.mg.avg_shift);
    if (!m.mg.learning_on) {
        return;
    }
    if (delta != 0) {
        int size = delta > 0 ? delta : -delta;
        int width = 32 - __builtin_clz((unsigned)size); // 9 or 10 for a gap of a whole reward
        int extra = width >= 9 ? 0 : 9 - width;         // each halving of the gap halves the chance
        if (m.mg.inner_reward) {
            grade(m, (delta > 0) == (m.mg.noise_sign >= 0), m.mg.learn_shift + extra);
        }
        if (m.ev_tiles && m.mg.ev_on) {
            min_evidence_reward(m, delta > 0, m.mg.learn_shift + extra);
        }
    }
    // the readout grades against nothing: bad is bad
    if (m.mg.readout) {
        min_readout_reward(m, valence);
    }
}

void minimal_readout_only(MinimalGenome& mg) {
    mg.readout = 1;
    mg.inner_reward = 0;
    mg.cowire_on = 0;
    mg.ev_on = 0;
}
