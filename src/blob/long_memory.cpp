#include "blob/long_memory.hpp"

#include "blob/random_words.hpp"

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

// pick the integrator cells: hold_percent of the excitatory neurons that are neither sensor
// nor motor, each with a sensor cell near it
static void make_cells(Blob& blob, BlobLongMemory& lm) {
    size_t words = (size_t)blob.tile_count + 1;
    lm.cell = (uint64_t*)std::calloc(words, sizeof(uint64_t));
    lm.source = (uint32_t*)std::calloc((size_t)blob.neuron_count, sizeof(uint32_t));
    lm.count = (uint8_t*)std::calloc((size_t)blob.neuron_count, 1);
    for (int n = 0; n < blob.neuron_count; n++) {
        if (has_bit(blob.sensor, n) || has_bit(blob.motor, n) || has_bit(blob.inhibitory, n)) {
            continue;
        }
        if ((int)rng_below(blob.rng, 100) >= blob.genome.hold_percent) {
            continue;
        }
        int s = sensor_near(blob, n);
        if (s < 0) {
            continue;
        }
        lm.cell[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
        lm.source[n] = (uint32_t)s;
    }
}

void long_memory_after_fire(Blob& blob) {
    if (blob.genome.hold_percent <= 0) {
        return;
    }
    BlobLongMemory& lm = memory_of(blob);
    if (!lm.cell) {
        make_cells(blob, lm);
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

void long_memory_evidence_decay(Blob& blob, int64_t row_at, int8_t* row) {
    const Genome& g = blob.genome;
    uint8_t* lock = g.readout_lock_levels > 0 ? lock_array(blob) : nullptr;
    for (int i = 0; i < g.readout_decay; i++) {
        int k = (int)rng_below(blob.rng, SLOTS * TILE_NEURONS);
        if (lock && lock[row_at + k]) {
            continue;
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
    bool further = (direction > 0 && value >= top) || (direction < 0 && value <= -top);
    if (further) {
        int most = g.readout_lock_levels > 3 ? 3 : g.readout_lock_levels;
        if (depth < most && (rng_next(blob.rng) & ((1ull << g.readout_lock_shift) - 1)) == 0) {
            lock[at] = (uint8_t)(depth + 1);
        }
        return value;
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
    std::free(lm.lock);
    std::free(blob.long_memory);
    blob.long_memory = nullptr;
}
