#include "blob/random_words.hpp"
#include "table/table.hpp"

#include <cstdlib>

// long memory on the table sheet (docs/results-2026-11-25-long-memory.md): a timeline of
// slow maps (idea 131), integrator cells that hold without leaking (catalogue 99, 180), and
// a freezer on table counters (idea 130). each is off at 0 and then touches nothing.

static int levels_of(const TableBlob& tb) {
    int l = tb.tg.timeline_levels;
    return l < 0 ? 0 : (l > TABLE_MAX_LEVELS ? TABLE_MAX_LEVELS : l);
}

// a random sensor cell within `reach` of (x, y), or anywhere if none is near; -1 if none
static int sensor_near(TableBlob& tb, int x, int y) {
    const Blob& b = tb.shell;
    int reach = tb.tg.reach;
    for (int tries = 0; tries < 64; tries++) {
        int sx = x + (int)rng_below(tb.rng, (uint32_t)(2 * reach + 1)) - reach;
        int sy = y + (int)rng_below(tb.rng, (uint32_t)(2 * reach + 1)) - reach;
        if (sx < 0 || sy < 0 || sx >= b.tiles_w * 8 || sy >= b.tiles_h * 8) {
            continue;
        }
        int m = blob_neuron_at(b, sx, sy);
        if ((b.sensor[m / TILE_NEURONS] >> (m % TILE_NEURONS)) & 1) {
            return m;
        }
    }
    for (int tries = 0; tries < 4096; tries++) {
        int m = (int)rng_below(tb.rng, (uint32_t)b.neuron_count);
        if ((b.sensor[m / TILE_NEURONS] >> (m % TILE_NEURONS)) & 1) {
            return m;
        }
    }
    return -1;
}

void table_memory_birth(TableBlob& tb) {
    Blob& b = tb.shell;
    const TableGenome& tg = tb.tg;
    size_t words = (size_t)b.tile_count + 1;
    int levels = levels_of(tb);
    if (levels > 0) {
        tb.level = (uint64_t*)std::calloc(words * (size_t)levels, sizeof(uint64_t));
        tb.level_acc = (uint64_t*)std::calloc(words * (size_t)levels, sizeof(uint64_t));
    }
    if (tg.hold_percent > 0) {
        tb.hold_cell = (uint64_t*)std::calloc(words, sizeof(uint64_t));
        tb.held = (uint64_t*)std::calloc(words, sizeof(uint64_t));
        tb.hold_source = (uint32_t*)std::calloc((size_t)b.neuron_count, sizeof(uint32_t));
        tb.hold_count = (uint8_t*)std::calloc((size_t)b.neuron_count, 1);
        for (int n = 0; n < b.neuron_count; n++) {
            uint64_t bit = 1ull << (n % TILE_NEURONS);
            int t = n / TILE_NEURONS;
            if ((b.sensor[t] | b.motor[t]) & bit) {
                continue;
            }
            if ((int)rng_below(tb.rng, 100) >= tg.hold_percent) {
                continue;
            }
            int s = sensor_near(tb, blob_neuron_x(b, n), blob_neuron_y(b, n));
            if (s < 0) {
                continue;
            }
            tb.hold_cell[t] |= bit;
            tb.hold_source[n] = (uint32_t)s;
        }
    }
    if (tg.lock_levels > 0) {
        int entries = 1 << tb.inputs;
        tb.lock_words = entries * 2 > 64 ? entries * 2 / 64 : 1;
        tb.lock = (uint64_t*)std::calloc((size_t)b.neuron_count * (size_t)tb.lock_words,
                                         sizeof(uint64_t));
    }
}

void table_memory_free(TableBlob& tb) {
    std::free(tb.level);
    std::free(tb.level_acc);
    std::free(tb.hold_cell);
    std::free(tb.held);
    std::free(tb.hold_source);
    std::free(tb.hold_count);
    std::free(tb.lock);
}

uint32_t table_memory_input(TableBlob& tb, uint32_t source, uint32_t delay, int x, int y) {
    int levels = levels_of(tb);
    if (levels == 0 || (int)rng_below(tb.rng, 100) >= tb.tg.timeline_percent) {
        return (source << 5) | delay;
    }
    uint32_t code = 16u + rng_below(tb.rng, (uint32_t)levels);
    if (tb.tg.timeline_source) {
        // the slow maps of busy inner cells are all ones; a sensor cell's are not
        int s = sensor_near(tb, x, y);
        source = s < 0 ? source : (uint32_t)s;
    }
    return (source << 5) | code;
}

// timeline: after the newest map is written. level k (0-based) spans 2^(5+k) ticks.
// OR: the gathering map of level 0 takes every tick; when level k's span ends, its map is
// what it gathered, which joins level k + 1's gathering. sampled: when level k's span ends
// it takes the map level k - 1 holds (level 0 takes the newest map), top level first.
static void timeline_tick(TableBlob& tb) {
    const Blob& b = tb.shell;
    int levels = levels_of(tb);
    if (levels == 0) {
        return;
    }
    int64_t words = (int64_t)b.tile_count + 1;
    const uint64_t* map = b.spikes[b.now];
    uint64_t t = b.stats.tick;
    if (tb.tg.timeline_mode == 2) {
        for (int k = levels - 1; k >= 0; k--) {
            if (t % (1ull << (5 + k)) != 0) {
                continue;
            }
            const uint64_t* from = k == 0 ? map : tb.level + (k - 1) * words;
            uint64_t* to = tb.level + k * words;
            for (int64_t w = 0; w < words; w++) {
                to[w] = from[w];
            }
        }
        return;
    }
    for (int64_t w = 0; w < words; w++) {
        tb.level_acc[w] |= map[w];
    }
    for (int k = 0; k < levels; k++) {
        if (t % (1ull << (5 + k)) != 0) {
            break; // a longer span cannot end where a shorter one does not
        }
        uint64_t* acc = tb.level_acc + k * words;
        uint64_t* out = tb.level + k * words;
        uint64_t* up = k + 1 < levels ? tb.level_acc + (k + 1) * words : nullptr;
        for (int64_t w = 0; w < words; w++) {
            out[w] = acc[w];
            if (up) {
                up[w] |= acc[w];
            }
            acc[w] = 0;
        }
    }
}

// integrator cells: count each onset of the source (fired now, not the tick before) and
// fire, from the next tick, while the count is above 0
static void hold_tick(TableBlob& tb) {
    if (!tb.hold_cell) {
        return;
    }
    const Blob& b = tb.shell;
    const uint64_t* now = b.spikes[b.now];
    const uint64_t* before = b.spikes[(b.now - 1) & (HISTORY - 1)];
    int cap = tb.tg.hold_cap < 1 ? 1 : (tb.tg.hold_cap > 255 ? 255 : tb.tg.hold_cap);
    for (int t = 0; t < b.tile_count; t++) {
        uint64_t cells = tb.hold_cell[t];
        uint64_t held = 0;
        while (cells) {
            int bit = __builtin_ctzll(cells);
            cells &= cells - 1;
            int n = t * TILE_NEURONS + bit;
            uint32_t s = tb.hold_source[n];
            bool on = (now[s / TILE_NEURONS] >> (s % TILE_NEURONS)) & 1;
            bool was = (before[s / TILE_NEURONS] >> (s % TILE_NEURONS)) & 1;
            int c = tb.hold_count[n] + (on && !was ? 1 : 0);
            c = c > cap ? cap : c;
            tb.hold_count[n] = (uint8_t)c;
            held |= (uint64_t)(c > 0) << bit;
        }
        tb.held[t] = held;
    }
}

void table_memory_tick(TableBlob& tb) {
    timeline_tick(tb);
    hold_tick(tb);
}

void table_memory_reward(TableBlob& tb) {
    if (!tb.hold_cell) {
        return;
    }
    const Blob& b = tb.shell;
    for (int t = 0; t < b.tile_count; t++) {
        uint64_t cells = tb.hold_cell[t];
        while (cells) {
            tb.hold_count[t * TILE_NEURONS + __builtin_ctzll(cells)] = 0;
            cells &= cells - 1;
        }
        tb.held[t] = 0;
    }
}

// the freezer's lock depth of one entry (2 bits)
static int lock_get(const TableBlob& tb, int n, int entry) {
    int at = entry * 2;
    return (int)((tb.lock[(int64_t)n * tb.lock_words + (at >> 6)] >> (at & 63)) & 3);
}

static void lock_set(TableBlob& tb, int n, int entry, int depth) {
    int at = entry * 2;
    uint64_t& w = tb.lock[(int64_t)n * tb.lock_words + (at >> 6)];
    w = (w & ~(3ull << (at & 63))) | ((uint64_t)depth << (at & 63));
}

bool table_step_locked(TableBlob& tb, int n, int entry, int direction) {
    if (!tb.lock) {
        return table_step(tb, n, entry, direction);
    }
    const TableGenome& tg = tb.tg;
    int top = (1 << tb.bits) - 1;
    int value = table_counter(tb, n, entry);
    int depth = lock_get(tb, n, entry);
    bool further = (direction > 0 && value == top) || (direction < 0 && value == 0);
    if (further) {
        // confirming evidence at the end of the range: deepen the lock now and then
        int most = tg.lock_levels > 3 ? 3 : tg.lock_levels;
        if (depth < most && (rng_next(tb.rng) & ((1ull << tg.lock_shift) - 1)) == 0) {
            lock_set(tb, n, entry, depth + 1);
        }
        return false;
    }
    if (depth > 0) {
        // against the lock: rarely, and then it opens
        int shift = tg.lock_hold * depth;
        shift = shift > 62 ? 62 : shift;
        if ((rng_next(tb.rng) & ((1ull << shift) - 1)) != 0) {
            return false;
        }
        lock_set(tb, n, entry, 0);
    }
    return table_step(tb, n, entry, direction);
}
