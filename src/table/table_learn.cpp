#include "blob/random_words.hpp"
#include "table/table.hpp"

// learning on the table sheet. only counters that were just used ever move, so one context
// cannot overwrite another.

// good (+1) or bad (-1). for each of the last recent_ticks ticks, graded neurons have the
// counter they used that tick moved one step toward what they did (good) or away from it
// (bad), each with chance 2^-reward_shift.
//   mode 0: graded are the neurons that fired (grade 0), also quiet ones that had an input
//           on (grade 1), every neuron (grade 2), or every neuron on good and only the ones
//           that fired on bad (grade 3).
//   mode 1: noise is the experiment. only neurons whose output noise flipped that tick are
//           graded, and against the running average of reward, not its sign.
//   mode 2: the same with the sign turned over: better than average moves a flipped
//           neuron back toward its table's answer, worse moves it toward what noise did.
//           (mode 1 settles on the wrong answer here; see the results doc.)
//   mode 3: mode 1 with the flip's average taken out: an unflipped neuron is graded too,
//           with the same chance a flip has, toward its table's answer on better than
//           average. a flip that did not change the answer then nets to zero in each
//           context, where in mode 1 it is pushed toward the flip in contexts that do better
//           than the average and toward the table in those that do worse, which leads to the
//           wrong answer (docs/results-2026-11-24-noise-sign.md).
// with baseline (always in mode 1) the direction is reward minus its running average, and
// the chance shrinks in proportion to that difference.
void table_reward(Blob& blob, int valence) {
    TableBlob& tb = table_of(blob);
    const TableGenome& tg = tb.tg;
    // integrator cells clear on any good or bad, learning or not: it is activity, not wiring
    if (valence != 0) {
        table_memory_reward(tb);
    }
    if (!tg.learning_on || valence == 0) {
        return;
    }
    if (tg.decay > 0) {
        table_memory_decay(tb);
    }
    Blob& b = tb.shell;
    bool noise_mode = tg.learn_mode >= 1 && tg.learn_mode <= 3;
    bool toward = valence > 0;
    int shift = tg.reward_shift;
    if (noise_mode || tg.baseline) {
        int32_t delta = valence * 256 - tb.reward_average;
        tb.reward_average += delta >> tg.average_shift;
        if (delta == 0) {
            return;
        }
        toward = delta > 0;
        toward = tg.learn_mode == 2 ? !toward : toward;
        // the whole event acts with chance |delta| / 256 (one draw per reward), so a neuron
        // whose firing did not matter drifts by zero on average
        int32_t size = delta > 0 ? delta : -delta;
        if (size < 256 && (int32_t)(rng_next(tb.rng) & 255) >= size) {
            return;
        }
    }
    bool bad_fired_only = tg.grade == 3 && !toward;
    // on bad, a quiet neuron is pushed toward fire only with this extra chance
    int rise_shift = toward ? tg.rise_extra_shift : tg.rise_extra_shift + tg.bad_rise_shift;
    for (int age = 0; age < tg.recent_ticks; age++) {
        int r = (b.now - age) & (HISTORY - 1);
        const uint64_t* map = b.spikes[r];
        const uint64_t* flips = tb.flips[r];
        const uint8_t* address =
            &tb.address[(int64_t)(r & (TABLE_ADDR_HISTORY - 1)) * b.neuron_count];
        for (int t = 0; t < b.tile_count; t++) {
            uint64_t graded = ~b.sensor[t] & sparse_word(tb.rng, shift);
            if (bad_fired_only) {
                graded &= map[t];
            }
            if (noise_mode) {
                graded &= flips[t];
            }
            if (tg.learn_mode == 3) {
                graded |= ~b.sensor[t] & ~flips[t] &
                          sparse_word(tb.rng, tg.noise_shift + tg.reward_shift);
            }
            // a step toward fire also needs this, so blind grading drifts toward quiet
            uint64_t may_rise = graded ? sparse_word(tb.rng, rise_shift) : 0;
            while (graded) {
                int bit = __builtin_ctzll(graded);
                graded &= graded - 1;
                int n = t * TILE_NEURONS + bit;
                bool did = (map[t] >> bit) & 1;
                int entry = address[n];
                if (!noise_mode && !did && (tg.grade == 0 || (tg.grade == 1 && entry == 0))) {
                    continue;
                }
                bool fire = toward ? did : !did;
                if (fire && !((may_rise >> bit) & 1)) {
                    continue;
                }
                tb.stats.learned += table_step_locked(tb, n, entry, fire ? +1 : -1);
            }
        }
    }
}

// which inputs changed the neuron's answer at any of its recent addresses
static uint32_t inputs_that_mattered(const TableBlob& tb, int n) {
    uint32_t mattered = 0;
    for (int s = 0; s < TABLE_ADDR_HISTORY; s++) {
        int a = tb.address[(int64_t)s * tb.shell.neuron_count + n];
        bool out = table_output(tb, n, a);
        for (int j = 0; j < tb.inputs; j++) {
            if (table_output(tb, n, a ^ (1 << j)) != out) {
                mattered |= 1u << j;
            }
        }
    }
    return mattered;
}

// a nearby neuron that fired in the last recent_ticks ticks, or -1
static int recently_active_near(TableBlob& tb, int n) {
    const Blob& b = tb.shell;
    int x = blob_neuron_x(b, n), y = blob_neuron_y(b, n);
    int reach = tb.tg.reach;
    for (int tries = 0; tries < 4; tries++) {
        int sx = x + (int)rng_below(tb.rng, (uint32_t)(2 * reach + 1)) - reach;
        int sy = y + (int)rng_below(tb.rng, (uint32_t)(2 * reach + 1)) - reach;
        if (sx < 0 || sy < 0 || sx >= b.tiles_w * 8 || sy >= b.tiles_h * 8) {
            continue;
        }
        int m = blob_neuron_at(b, sx, sy);
        uint64_t recent = 0;
        for (int age = 0; age < tb.tg.recent_ticks; age++) {
            recent |= b.spikes[(b.now - age) & (HISTORY - 1)][m / TILE_NEURONS];
        }
        if ((recent >> (m % TILE_NEURONS)) & 1) {
            return m;
        }
    }
    return -1;
}

// growth by co-activity: a picked neuron swaps one input that changed none of its recent
// answers for a nearby neuron that fired recently. its table is left as it is.
void table_grow(TableBlob& tb) {
    Blob& b = tb.shell;
    uint32_t all = (1u << tb.inputs) - 1;
    for (int t = 0; t < b.tile_count; t++) {
        uint64_t picks = ~b.sensor[t] & sparse_word(tb.rng, tb.tg.grow_shift);
        while (picks) {
            int n = t * TILE_NEURONS + __builtin_ctzll(picks);
            picks &= picks - 1;
            uint32_t dead = ~inputs_that_mattered(tb, n) & all;
            if (!dead) {
                continue;
            }
            int j = pick_bit(tb.rng, dead);
            int m = recently_active_near(tb, n);
            if (m < 0) {
                continue;
            }
            int dspan = tb.tg.local_delay_max - tb.tg.local_delay_min + 1;
            int delay = tb.tg.local_delay_min + (int)rng_below(tb.rng, (uint32_t)dspan);
            tb.input[(int64_t)n * tb.inputs + j] = ((uint32_t)m << 5) | (uint32_t)delay;
            tb.stats.grown++;
        }
    }
}
