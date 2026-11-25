#include "blob/random_words.hpp"
#include "memory/memory.hpp"

// a reward: which button was pressed (found from the world's motor_acted marks), the
// counters of the frames since the last reward, and the draw for the next decision.

// the cells marked as pressed are one button: the button they share, or a new one
static int pressed_unit(MemoryBlob& m) {
    const Blob& b = m.shell;
    int unit = -2;
    bool mixed = false;
    int marked = 0;
    for (int c = 0; c < m.motor_cells; c++) {
        int n = m.motor_neuron[c];
        if (!((b.motor_acted[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1)) {
            continue;
        }
        marked++;
        int u = m.unit_of[c];
        if (unit == -2) {
            unit = u;
        } else if (u != unit) {
            mixed = true;
        }
    }
    if (marked == 0) {
        return -1;
    }
    if (!mixed && unit >= 0) {
        return unit;
    }
    if (m.units >= MEM_UNITS) {
        return -1; // no room for another button: nothing to credit
    }
    int fresh = m.units++;
    for (int c = 0; c < m.motor_cells; c++) {
        int n = m.motor_neuron[c];
        if ((b.motor_acted[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1) {
            m.unit_of[c] = (int8_t)fresh;
        }
    }
    return fresh;
}

static void move_counter(int8_t& c, int by, int max) {
    int v = c + by;
    c = (int8_t)(v > max ? max : (v < -max ? -max : v));
}

// one slot, one button, one reward
static void credit_slot(MemoryBlob& m, int8_t* slot, int unit, int valence) {
    const MemoryGenome& mg = m.mg;
    if (valence > 0) {
        move_counter(slot[unit], mg.good_step, mg.counter_max);
    } else {
        move_counter(slot[unit], -mg.bad_step, mg.counter_max);
        if (mg.others_up) {
            for (int u = 0; u < m.units; u++) {
                if (u != unit) {
                    move_counter(slot[u], 1, mg.counter_max);
                }
            }
        }
    }
    if (mg.fade_shift && (rng_next(m.rng) & ((1ull << mg.fade_shift) - 1)) == 0) {
        for (int u = 0; u < MEM_UNITS; u++) {
            slot[u] = (int8_t)(slot[u] / 2);
        }
    }
    m.stats.credited++;
}

static void credit(MemoryBlob& m, int pressed, int valence) {
    const MemoryGenome& mg = m.mg;
    // frames before the last reward (trace_across 2) are moved by how much better or worse
    // this reward is than usual: towards its sign with chance |reward - mean| / 2
    int mean = 2 * m.good_avg - 256; // x256, -256..256
    int surprise = valence * 256 - mean;
    int older_dir = surprise > 0 ? 1 : -1;
    int older_chance = (surprise < 0 ? -surprise : surprise) / 2; // out of 256
    // the frames to credit, newest first, with the button and direction of each
    int ring[MEM_FRAMES];
    int unit[MEM_FRAMES];
    int dir[MEM_FRAMES];
    int count = 0;
    int64_t oldest = m.frame_count > MEM_FRAMES ? m.frame_count - MEM_FRAMES : 0;
    for (int64_t i = m.frame_count - 1; i >= oldest; i--) {
        const MemoryFrame& f = m.frames[i & (MEM_FRAMES - 1)];
        bool before = f.last_tick <= m.last_reward_tick;
        bool too_old = f.last_tick < m.tick - (before ? mg.across_ticks : mg.trace_ticks);
        if ((before && !mg.trace_across) || too_old) {
            break;
        }
        int u = f.last_tick > m.tick - mg.credit_ticks ? pressed : f.unit;
        int d = valence;
        if (before && mg.trace_across == 2) {
            if ((int)(rng_next(m.rng) & 255) >= older_chance) {
                continue;
            }
            d = older_dir;
        }
        if (u < 0) {
            continue;
        }
        ring[count] = (int)(i & (MEM_FRAMES - 1));
        unit[count] = u;
        dir[count] = d;
        count++;
    }
    // each (slot, button) of a table moves once per reward, however many frames used it
    for (int t = 0; t < mg.tables; t++) {
        for (int k = 0; k < count; k++) {
            uint16_t slot = m.frame_slot[(size_t)ring[k] * mg.tables + t];
            bool seen = false;
            for (int j = 0; j < k && !seen; j++) {
                seen = unit[j] == unit[k] &&
                       m.frame_slot[(size_t)ring[j] * mg.tables + t] == slot;
            }
            if (seen) {
                continue;
            }
            int8_t* c = m.counters + ((size_t)t << mg.table_bits | slot) * MEM_UNITS;
            credit_slot(m, c, unit[k], dir[k]);
        }
    }
}

void memory_reward(Blob& blob, int valence) {
    MemoryBlob& m = memory_of(blob);
    const MemoryGenome& mg = m.mg;
    if (valence == 0) {
        return;
    }
    int pressed = pressed_unit(m);
    int good = valence > 0 ? 256 : 0;
    m.good_avg += (good - m.good_avg) >> mg.avg_shift;
    if (mg.learning_on) {
        credit(m, pressed, valence);
    }
    m.last_reward_tick = m.tick;

    // the next decision: tie order, and maybe a random button (more often while it goes badly)
    for (int u = 0; u <= MEM_UNITS; u++) {
        m.tie_key[u] = (uint8_t)rng_next(m.rng);
    }
    int explore = mg.explore_min + ((mg.explore_max - mg.explore_min) * (256 - m.good_avg) >> 8);
    m.explore_unit = -2;
    if ((int)(rng_next(m.rng) & 255) < explore) {
        // among the known buttons and one new one; an unattached pick falls back to the sums
        int pick = (int)rng_below(m.rng, (uint32_t)m.units + 1);
        m.explore_unit = pick == m.units ? -1 : pick;
        m.stats.explored++;
    }
    m.new_cell = -1; // a fresh one is drawn when needed
    m.frame_due = true;
}
