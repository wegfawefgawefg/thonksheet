#include "blob/random_words.hpp"
#include "memory/memory.hpp"

#include <bit>

// the code of what is sensed, length buckets, and a frame: the slot of every table, the
// sums per button, and the button to drive.

void memory_code_of_input(MemoryBlob& m, uint64_t* out) {
    for (int w = 0; w < MEM_FIELD_WORDS; w++) {
        out[w] = 0;
    }
    bool parity = m.mg.code == 0;
    for (int iw = 0; iw < m.in_words; iw++) {
        uint64_t on = m.in_now[iw];
        while (on) {
            int c = iw * 64 + std::countr_zero(on);
            on &= on - 1;
            const uint64_t* row = m.rows + (size_t)c * MEM_FIELD_WORDS;
            for (int w = 0; w < MEM_FIELD_WORDS; w++) {
                out[w] = parity ? out[w] ^ row[w] : out[w] | row[w];
            }
        }
    }
}

// 0..3 exactly, then 4 buckets per doubling
int memory_level(int64_t ticks) {
    if (ticks < 4) {
        return ticks < 0 ? 0 : (int)ticks;
    }
    int e = 63 - std::countl_zero((uint64_t)ticks); // 2..
    int mantissa = (int)((ticks >> (e - 2)) & 3);
    int level = 4 * (e - 1) + mantissa;
    return level < MEM_LEVELS ? level : MEM_LEVELS - 1;
}

static bool cell_attached(const MemoryBlob& m, int cell) {
    int n = m.motor_neuron[cell];
    return (m.shell.motor_attached[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

// buttons with an attached cell, and whether any attached cell has no button yet
static uint32_t attached_units(const MemoryBlob& m, bool* unassigned) {
    uint32_t mask = 0;
    *unassigned = false;
    for (int c = 0; c < m.motor_cells; c++) {
        if (!cell_attached(m, c)) {
            continue;
        }
        if (m.unit_of[c] >= 0) {
            mask |= 1u << m.unit_of[c];
        } else {
            *unassigned = true;
        }
    }
    return mask;
}

// a random attached cell with no button, kept until it is pressed or the reward redraws it
static void check_new_cell(MemoryBlob& m) {
    if (m.new_cell >= 0 && m.unit_of[m.new_cell] < 0 && cell_attached(m, m.new_cell)) {
        return;
    }
    int count = 0;
    for (int c = 0; c < m.motor_cells; c++) {
        count += m.unit_of[c] < 0 && cell_attached(m, c);
    }
    m.new_cell = -1;
    if (count == 0) {
        return;
    }
    int pick = (int)rng_below(m.rng, (uint32_t)count);
    for (int c = 0; c < m.motor_cells; c++) {
        if (m.unit_of[c] < 0 && cell_attached(m, c) && pick-- == 0) {
            m.new_cell = c;
            return;
        }
    }
}

// the largest sum among attached buttons, ties by the draw made at the last reward. a new
// button counts as a sum of 0.
static int choose(MemoryBlob& m, uint32_t attached, bool unassigned) {
    if (m.explore_unit >= 0 && ((attached >> m.explore_unit) & 1)) {
        return m.explore_unit;
    }
    if (m.explore_unit == -1 && unassigned) {
        return -1;
    }
    int best = -2;
    int64_t best_score = 0;
    for (int u = 0; u < m.units; u++) {
        if (!((attached >> u) & 1)) {
            continue;
        }
        int64_t score = (int64_t)m.sum[u] * 256 + m.tie_key[u];
        if (best == -2 || score > best_score) {
            best = u;
            best_score = score;
        }
    }
    if (unassigned) {
        int64_t score = m.tie_key[MEM_UNITS];
        if (best == -2 || score > best_score) {
            best = -1;
        }
    }
    return best;
}

void memory_new_frame(MemoryBlob& m) {
    const MemoryGenome& mg = m.mg;
    const uint64_t* field[MEM_FIELDS] = {};
    field[0] = m.code_now;
    field[1] = m.levels + (size_t)m.level_now * MEM_FIELD_WORDS;
    for (int j = 0; j < MEM_DEPTH_MAX; j++) {
        field[2 + 2 * j] = m.past_code[j];
        field[3 + 2 * j] = m.levels + (size_t)m.past_level[j] * MEM_FIELD_WORDS;
    }

    int64_t index = m.frame_count++;
    int ring = (int)(index & (MEM_FRAMES - 1));
    uint16_t* slots = m.frame_slot + (size_t)ring * mg.tables;
    for (int u = 0; u < MEM_UNITS; u++) {
        m.sum[u] = 0;
    }
    int bits = mg.table_bits;
    for (int t = 0; t < mg.tables; t++) {
        const uint16_t* pos = m.pos + (size_t)t * bits;
        uint32_t slot = 0;
        for (int i = 0; i < bits; i++) {
            const uint64_t* f = field[pos[i] >> 10];
            int bit = pos[i] & 1023;
            slot |= (uint32_t)((f[bit >> 6] >> (bit & 63)) & 1) << i;
        }
        slots[t] = (uint16_t)slot;
        const int8_t* c = m.counters + ((size_t)t << bits | slot) * MEM_UNITS;
        for (int u = 0; u < m.units; u++) {
            m.sum[u] += c[u];
        }
    }

    bool unassigned = false;
    uint32_t attached = attached_units(m, &unassigned);
    if (unassigned) {
        check_new_cell(m);
        unassigned = m.new_cell >= 0;
    }
    m.drive = choose(m, attached, unassigned);
    MemoryFrame& f = m.frames[ring];
    f.first_tick = m.tick;
    f.last_tick = m.tick;
    f.unit = m.drive;
    m.stats.frames++;
}
