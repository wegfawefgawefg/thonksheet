#include "blob/long_memory.hpp"

#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"

#include <cstdlib>

// long memory in the full engine: integrator cells and the evidence freezer (see the header)

static bool has_bit(const uint64_t* mask, int n) {
    return (mask[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

static BlobLongMemory& memory_of(Blob& blob) {
    if (!blob.long_memory) {
        blob.long_memory = (BlobLongMemory*)std::calloc(1, sizeof(BlobLongMemory));
    }
    return *blob.long_memory;
}

// a random sensor cell within 8 cells of neuron n, or anywhere if none is near; -1 if none
static int sensor_near(Blob& blob, int n) {
    int x = blob_neuron_x(blob, n), y = blob_neuron_y(blob, n);
    int w = blob.tiles_w * 8, h = blob.tiles_h * 8;
    for (int tries = 0; tries < 64; tries++) {
        int sx = x + (int)rng_below(blob.rng, 17) - 8;
        int sy = y + (int)rng_below(blob.rng, 17) - 8;
        if (sx < 0 || sy < 0 || sx >= w || sy >= h) {
            continue;
        }
        int m = blob_neuron_at(blob, sx, sy);
        if (has_bit(blob.sensor, m)) {
            return m;
        }
    }
    for (int tries = 0; tries < 4096; tries++) {
        int m = (int)rng_below(blob.rng, (uint32_t)blob.neuron_count);
        if (has_bit(blob.sensor, m)) {
            return m;
        }
    }
    return -1;
}

// pick the cells: of the excitatory neurons that are neither sensor nor motor, hold_percent
// become integrator cells and time_percent time cells, each with a sensor cell near it. a
// time cell also draws its bin (0..time_levels-1) and which onset it times (last or the
// one before)
static void make_cells(Blob& blob, BlobLongMemory& lm) {
    const Genome& g = blob.genome;
    size_t words = (size_t)blob.tile_count + 1;
    size_t count = (size_t)blob.neuron_count;
    lm.cell = (uint64_t*)std::calloc(words, sizeof(uint64_t));
    lm.time_cell = (uint64_t*)std::calloc(words, sizeof(uint64_t));
    lm.source = (uint32_t*)std::calloc(count, sizeof(uint32_t));
    lm.count = (uint8_t*)std::calloc(count, 1);
    lm.bin = (uint8_t*)std::calloc(count, 1);
    lm.since_last = (uint32_t*)std::calloc(count, sizeof(uint32_t));
    lm.since_prev = (uint32_t*)std::calloc(count, sizeof(uint32_t));
    int levels = g.time_levels < 1 ? 1 : (g.time_levels > 27 ? 27 : g.time_levels);
    for (int n = 0; n < blob.neuron_count; n++) {
        if (has_bit(blob.sensor, n) || has_bit(blob.motor, n) || has_bit(blob.inhibitory, n)) {
            continue;
        }
        int r = (int)rng_below(blob.rng, 100);
        bool hold = r < g.hold_percent;
        bool time = !hold && g.time_percent > 0 && r < g.hold_percent + g.time_percent;
        if (!hold && !time) {
            continue;
        }
        int s = sensor_near(blob, n);
        if (s < 0) {
            continue;
        }
        uint64_t bit = 1ull << (n % TILE_NEURONS);
        lm.source[n] = (uint32_t)s;
        if (hold) {
            lm.cell[n / TILE_NEURONS] |= bit;
            continue;
        }
        lm.time_cell[n / TILE_NEURONS] |= bit;
        int level = (int)rng_below(blob.rng, (uint32_t)levels);
        int before = (int)(rng_next(blob.rng) & 1);
        lm.bin[n] = (uint8_t)(level | (before << 7));
    }
    // every sensor cell starts long silent
    for (int n = 0; n < blob.neuron_count; n++) {
        lm.since_last[n] = lm.since_prev[n] = 1u << 30;
    }
}

// time cells: ticks since each sensor cell's last onset and the onset before it; a time
// cell fires while its count lies in its bin, [2^(4 + level), 2^(5 + level)) ticks
static void time_tick(Blob& blob, BlobLongMemory& lm) {
    uint64_t* now = blob.spikes[blob.now];
    const uint64_t* before = blob.spikes[(blob.now - 1) & (HISTORY - 1)];
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t sensors = blob.sensor[t];
        uint64_t onset = now[t] & ~before[t];
        while (sensors) {
            int bit = __builtin_ctzll(sensors);
            sensors &= sensors - 1;
            int n = t * TILE_NEURONS + bit;
            uint32_t last = lm.since_last[n], prev = lm.since_prev[n];
            last += last < (1u << 30);
            prev += prev < (1u << 30);
            if ((onset >> bit) & 1) {
                prev = last;
                last = 0;
            }
            lm.since_last[n] = last;
            lm.since_prev[n] = prev;
        }
    }
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t cells = lm.time_cell[t];
        if (!cells) {
            continue;
        }
        uint64_t fire = 0, all = cells;
        while (cells) {
            int bit = __builtin_ctzll(cells);
            cells &= cells - 1;
            int n = t * TILE_NEURONS + bit;
            uint32_t s = lm.source[n];
            int b = lm.bin[n];
            uint32_t since = (b & 128) ? lm.since_prev[s] : lm.since_last[s];
            uint32_t low = 1u << (4 + (b & 31));
            fire |= (uint64_t)(since >= low && since < 2 * low) << bit;
        }
        now[t] = (now[t] & ~all) | fire;
    }
}

void long_memory_after_fire(Blob& blob) {
    if (blob.genome.hold_percent <= 0 && blob.genome.time_percent <= 0) {
        return;
    }
    BlobLongMemory& lm = memory_of(blob);
    if (!lm.cell) {
        make_cells(blob, lm);
    }
    if (blob.genome.time_percent > 0) {
        time_tick(blob, lm);
    }
    int cap =
        blob.genome.hold_cap < 1 ? 1 : (blob.genome.hold_cap > 255 ? 255 : blob.genome.hold_cap);
    bool clear = blob.pending_valence != 0;
    uint64_t* now = blob.spikes[blob.now];
    const uint64_t* before = blob.spikes[(blob.now - 1) & (HISTORY - 1)];
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t cells = lm.cell[t];
        if (!cells) {
            continue;
        }
        uint64_t held = 0, all = cells;
        while (cells) {
            int bit = __builtin_ctzll(cells);
            cells &= cells - 1;
            int n = t * TILE_NEURONS + bit;
            uint32_t s = lm.source[n];
            bool onset = has_bit(now, (int)s) && !has_bit(before, (int)s);
            int c = clear ? 0 : lm.count[n];
            c = c + (onset ? 1 : 0) > cap ? cap : c + (onset ? 1 : 0);
            lm.count[n] = (uint8_t)c;
            held |= (uint64_t)(c > 0) << bit;
        }
        now[t] = (now[t] & ~all) | held;
    }
}

// lock depths, laid out as readout_evidence (a row per motor cell)
static uint8_t* lock_array(Blob& blob) {
    BlobLongMemory& lm = memory_of(blob);
    if (!lm.lock) {
        int motors = 0;
        for (int t = 0; t < blob.tile_count; t++) {
            motors += __builtin_popcountll(blob.motor[t]);
        }
        lm.lock = (uint8_t*)std::calloc((size_t)motors * SLOTS * TILE_NEURONS, 1);
    }
    return lm.lock;
}

void long_memory_evidence_decay(Blob& blob, int n, int64_t row_at, int8_t* row) {
    const Genome& g = blob.genome;
    uint8_t* lock = g.readout_lock_levels > 0 ? lock_array(blob) : nullptr;
    for (int i = 0; i < g.readout_decay; i++) {
        int k = (int)rng_below(blob.rng, SLOTS * TILE_NEURONS);
        if (lock && lock[row_at + k]) {
            // a lock is spared; with readout_lock_fade it loses a level now and then, so
            // only evidence that keeps being confirmed stays frozen
            int fade = g.readout_lock_fade;
            if (fade > 0 && (rng_next(blob.rng) & ((1ull << (fade > 62 ? 62 : fade)) - 1)) == 0) {
                lock[row_at + k]--;
            }
            continue;
        }
        // readout_decay_idle: a source active in the window just graded keeps its evidence
        if (g.readout_decay_idle) {
            uint32_t src = slot_source(blob, n, k / TILE_NEURONS);
            if ((blob.active_trace[src] >> (k % TILE_NEURONS)) & 1) {
                continue;
            }
        }
        int v = row[k];
        row[k] = (int8_t)(v > 0 ? v - 1 : (v < 0 ? v + 1 : 0));
    }
}

int8_t long_memory_evidence_step(Blob& blob, int64_t at, int8_t value, int direction, int top) {
    const Genome& g = blob.genome;
    if (g.readout_lock_levels <= 0) {
        int next = value + direction;
        return (int8_t)(next > top ? top : (next < -top ? -top : next));
    }
    uint8_t* lock = lock_array(blob);
    int depth = lock[at];
    // confirming evidence: the push agrees with a counter at least lock_from strong (0: only
    // at the end of the range). it deepens the lock now and then
    int from = g.readout_lock_from > 0 && g.readout_lock_from < top ? g.readout_lock_from : top;
    bool agree = (direction > 0 && value >= from) || (direction < 0 && value <= -from);
    if (agree) {
        int most = g.readout_lock_levels > 3 ? 3 : g.readout_lock_levels;
        if (depth < most && (rng_next(blob.rng) & ((1ull << g.readout_lock_shift) - 1)) == 0) {
            lock[at] = (uint8_t)(depth + 1);
        }
        bool end = (direction > 0 && value >= top) || (direction < 0 && value <= -top);
        return end ? value : (int8_t)(value + direction);
    }
    if (depth > 0) {
        int shift = g.readout_lock_hold * depth;
        shift = shift > 62 ? 62 : shift;
        if ((rng_next(blob.rng) & ((1ull << shift) - 1)) != 0) {
            return value;
        }
        lock[at] = 0;
    }
    return (int8_t)(value + direction);
}

void long_memory_free(Blob& blob) {
    if (!blob.long_memory) {
        return;
    }
    BlobLongMemory& lm = *blob.long_memory;
    std::free(lm.cell);
    std::free(lm.source);
    std::free(lm.count);
    std::free(lm.time_cell);
    std::free(lm.bin);
    std::free(lm.since_last);
    std::free(lm.since_prev);
    std::free(lm.lock);
    std::free(blob.long_memory);
    blob.long_memory = nullptr;
}
