#include "blob/random_words.hpp"
#include "memory/memory.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>

// building a memory blob from the shell genome (sheet, sensor and motor regions, learning
// switch) and its own genome, and freeing it.

static_assert(std::is_standard_layout_v<MemoryBlob>, "the shell must convert back");
static_assert(offsetof(MemoryBlob, shell) == 0, "the shell must be first");

static void* alloc_lines(size_t bytes) {
    size_t rounded = (bytes + 63) & ~(size_t)63;
    void* p = std::aligned_alloc(64, rounded ? rounded : 64);
    std::memset(p, 0, rounded ? rounded : 64);
    return p;
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void sanitize(MemoryGenome& mg) {
    mg.code = clamp_int(mg.code, 0, 1);
    mg.taps = clamp_int(mg.taps, 1, 256);
    mg.tables = clamp_int(mg.tables, 1, 4096);
    mg.table_bits = clamp_int(mg.table_bits, 1, MEM_TABLE_BITS_MAX);
    mg.past_bits = clamp_int(mg.past_bits, 0, mg.table_bits);
    mg.depth = clamp_int(mg.depth, 0, MEM_DEPTH_MAX);
    mg.now_only = clamp_int(mg.now_only, 0, 64);
    mg.counter_max = clamp_int(mg.counter_max, 1, 127);
    mg.good_step = clamp_int(mg.good_step, 0, 127);
    mg.bad_step = clamp_int(mg.bad_step, 0, 127);
    mg.credit_ticks = clamp_int(mg.credit_ticks, 1, 1 << 20);
    mg.trace_ticks = clamp_int(mg.trace_ticks, mg.credit_ticks, 1 << 20);
    mg.explore_max = clamp_int(mg.explore_max, 0, 256);
    mg.explore_min = clamp_int(mg.explore_min, 0, mg.explore_max);
    mg.avg_shift = clamp_int(mg.avg_shift, 0, 16);
    mg.level_span = clamp_int(mg.level_span, 1, MEM_LEVELS);
    mg.fade_shift = clamp_int(mg.fade_shift, 0, 30);
    mg.predict_bits = clamp_int(mg.predict_bits, 4, 20);
}

// every stride-th cell of a rectangle, as the other engines lay sensor and motor cells
static void mark_lattice(Blob& b, uint64_t* mask, int x0, int y0, int w, int h, int stride) {
    for (int y = y0; y < y0 + h; y += stride) {
        for (int x = x0; x < x0 + w; x += stride) {
            if (x < 0 || y < 0 || x >= b.tiles_w * 8 || y >= b.tiles_h * 8) {
                continue;
            }
            int n = blob_neuron_at(b, x, y);
            mask[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
        }
    }
}

static void shell_birth(Blob& b, const Genome& genome) {
    b.genome = genome;
    b.tiles_w = genome.tiles_w;
    b.tiles_h = genome.tiles_h;
    b.tile_count = genome.tiles_w * genome.tiles_h;
    b.neuron_count = b.tile_count * TILE_NEURONS;
    size_t map = sizeof(uint64_t) * (size_t)(b.tile_count + 1);
    for (int h = 0; h < HISTORY; h++) {
        b.spikes[h] = (uint64_t*)alloc_lines(map);
    }
    b.recent = (uint64_t*)alloc_lines(map);
    b.inhibitory = (uint64_t*)alloc_lines(map);
    b.sensor = (uint64_t*)alloc_lines(map);
    b.motor = (uint64_t*)alloc_lines(map);
    b.sense_in = (uint64_t*)alloc_lines(map);
    b.sense_attached = (uint64_t*)alloc_lines(map);
    b.predicted = (uint64_t*)alloc_lines(map);
    b.motor_attached = (uint64_t*)alloc_lines(map);
    b.motor_acted = (uint64_t*)alloc_lines(map);
    const Genome& g = genome;
    mark_lattice(b, b.sensor, g.sensor_x, g.sensor_y, g.sensor_w, g.sensor_h, g.sensor_stride);
    mark_lattice(b, b.motor, g.motor_x, g.motor_y, g.motor_w, g.motor_h, g.motor_stride);
}

// each code bit is the parity (or OR) of `taps` random sensor cells: so each cell's row has
// the code bits it takes part in
static void make_rows(MemoryBlob& m) {
    Blob& b = m.shell;
    int cells[MEM_IN_WORDS * 64];
    int count = 0;
    for (int t = 0; t < b.tile_count && m.in_words < MEM_IN_WORDS; t++) {
        if (!b.sensor[t]) {
            continue;
        }
        int w = m.in_words++;
        m.in_tile[w] = t;
        for (int bit = 0; bit < 64; bit++) {
            if ((b.sensor[t] >> bit) & 1) {
                cells[count++] = w * 64 + bit;
            }
        }
    }
    m.rows = (uint64_t*)alloc_lines(sizeof(uint64_t) * MEM_FIELD_WORDS * 64 * (size_t)m.in_words);
    if (count == 0) {
        return;
    }
    for (int j = 0; j < MEM_FIELD_BITS; j++) {
        for (int k = 0; k < m.mg.taps; k++) {
            int c = cells[rng_below(m.rng, (uint32_t)count)];
            m.rows[(size_t)c * MEM_FIELD_WORDS + j / 64] |= 1ull << (j % 64);
        }
    }
}

// level codes: a random half-full code, then each next bucket flips a fresh share of bits,
// so codes level_span buckets apart are unrelated
static void make_levels(MemoryBlob& m) {
    m.levels = (uint64_t*)alloc_lines(sizeof(uint64_t) * MEM_FIELD_WORDS * MEM_LEVELS);
    for (int w = 0; w < MEM_FIELD_WORDS; w++) {
        m.levels[w] = rng_next(m.rng);
    }
    int flips = MEM_FIELD_BITS / (2 * m.mg.level_span);
    for (int l = 1; l < MEM_LEVELS; l++) {
        uint64_t* prev = m.levels + (size_t)(l - 1) * MEM_FIELD_WORDS;
        uint64_t* code = m.levels + (size_t)l * MEM_FIELD_WORDS;
        std::memcpy(code, prev, sizeof(uint64_t) * MEM_FIELD_WORDS);
        for (int f = 0; f < flips; f++) {
            int j = (int)rng_below(m.rng, MEM_FIELD_BITS);
            code[j / 64] ^= 1ull << (j % 64);
        }
    }
}

// table t's field: kinds are now_only parts of "now only", then one per other field
static void make_tables(MemoryBlob& m) {
    const MemoryGenome& mg = m.mg;
    int others = 1 + 2 * mg.depth; // now's length, then each past event's code and length
    int kinds = mg.now_only + others;
    m.slots = 1 << mg.table_bits;
    m.pos = (uint16_t*)alloc_lines(sizeof(uint16_t) * (size_t)mg.tables * mg.table_bits);
    for (int t = 0; t < mg.tables; t++) {
        int kind = kinds > 0 ? t % kinds : 0;
        int field = kind < mg.now_only ? 0 : 1 + (kind - mg.now_only);
        int from_other = field == 0 ? 0 : mg.past_bits;
        for (int i = 0; i < mg.table_bits; i++) {
            int f = i < mg.table_bits - from_other ? 0 : field;
            int bit = (int)rng_below(m.rng, MEM_FIELD_BITS);
            m.pos[t * mg.table_bits + i] = (uint16_t)(f << 10 | bit);
        }
    }
    size_t counters = (size_t)mg.tables * m.slots * MEM_UNITS;
    m.counters = (int8_t*)alloc_lines(counters);
    m.frame_slot = (uint16_t*)alloc_lines(sizeof(uint16_t) * MEM_FRAMES * (size_t)mg.tables);
}

static void list_motors(MemoryBlob& m) {
    Blob& b = m.shell;
    for (int n = 0; n < b.neuron_count && m.motor_cells < MEM_MOTOR_CELLS; n++) {
        if ((b.motor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1) {
            m.unit_of[m.motor_cells] = -1;
            m.motor_neuron[m.motor_cells++] = n;
        }
    }
}

Blob* memory_birth(const Genome& genome, const MemoryGenome& mg, uint64_t seed) {
    MemoryBlob* m = (MemoryBlob*)alloc_lines(sizeof(MemoryBlob));
    new (m) MemoryBlob{};
    m->mg = mg;
    sanitize(m->mg);
    m->mg.learning_on = genome.learning_on && mg.learning_on;
    rng_seed(m->rng, seed ^ 0x4d656d6f72790000ull); // "Memory"
    shell_birth(m->shell, genome);
    make_rows(*m);
    make_levels(*m);
    make_tables(*m);
    list_motors(*m);
    if (m->mg.predict) {
        size_t entries = (size_t)1 << m->mg.predict_bits;
        m->predict_tag = (uint64_t*)alloc_lines(sizeof(uint64_t) * entries);
        m->predict_in = (uint64_t*)alloc_lines(sizeof(uint64_t) * entries * (size_t)m->in_words);
    }
    for (int u = 0; u <= MEM_UNITS; u++) {
        m->tie_key[u] = (uint8_t)rng_next(m->rng);
    }
    return &m->shell;
}

void memory_free(Blob* blob) {
    if (!blob) {
        return;
    }
    MemoryBlob& m = memory_of(*blob);
    Blob& b = m.shell;
    for (int h = 0; h < HISTORY; h++) {
        std::free(b.spikes[h]);
    }
    std::free(b.recent);
    std::free(b.inhibitory);
    std::free(b.sensor);
    std::free(b.motor);
    std::free(b.sense_in);
    std::free(b.sense_attached);
    std::free(b.predicted);
    std::free(b.motor_attached);
    std::free(b.motor_acted);
    std::free(m.rows);
    std::free(m.levels);
    std::free(m.pos);
    std::free(m.counters);
    std::free(m.frame_slot);
    std::free(m.predict_tag);
    std::free(m.predict_in);
    m.~MemoryBlob();
    std::free(&m);
}

size_t memory_bytes(const MemoryBlob& m) {
    size_t bytes = sizeof(MemoryBlob);
    bytes += sizeof(uint64_t) * MEM_FIELD_WORDS * 64 * (size_t)m.in_words; // rows
    bytes += sizeof(uint64_t) * MEM_FIELD_WORDS * MEM_LEVELS;              // level codes
    bytes += sizeof(uint16_t) * (size_t)m.mg.tables * m.mg.table_bits;     // table bits
    bytes += (size_t)m.mg.tables * m.slots * MEM_UNITS;                    // counters
    bytes += sizeof(uint16_t) * MEM_FRAMES * (size_t)m.mg.tables;          // frame slots
    if (m.predict_tag) {
        size_t entries = (size_t)1 << m.mg.predict_bits;
        bytes += sizeof(uint64_t) * entries * (1 + (size_t)m.in_words);
    }
    return bytes;
}

struct MemoryField {
    const char* name;
    int MemoryGenome::*field;
};

static const MemoryField fields[] = {
    {"learning_on", &MemoryGenome::learning_on},
    {"code", &MemoryGenome::code},
    {"taps", &MemoryGenome::taps},
    {"tables", &MemoryGenome::tables},
    {"table_bits", &MemoryGenome::table_bits},
    {"past_bits", &MemoryGenome::past_bits},
    {"depth", &MemoryGenome::depth},
    {"now_only", &MemoryGenome::now_only},
    {"counter_max", &MemoryGenome::counter_max},
    {"good_step", &MemoryGenome::good_step},
    {"bad_step", &MemoryGenome::bad_step},
    {"others_up", &MemoryGenome::others_up},
    {"credit_ticks", &MemoryGenome::credit_ticks},
    {"trace_ticks", &MemoryGenome::trace_ticks},
    {"explore_max", &MemoryGenome::explore_max},
    {"explore_min", &MemoryGenome::explore_min},
    {"avg_shift", &MemoryGenome::avg_shift},
    {"level_span", &MemoryGenome::level_span},
    {"fade_shift", &MemoryGenome::fade_shift},
    {"predict", &MemoryGenome::predict},
    {"predict_bits", &MemoryGenome::predict_bits},
};

void apply_memory_args(int argc, char** argv, MemoryGenome& mg) {
    for (int i = 0; i + 1 < argc; i++) {
        if (std::strncmp(argv[i], "--y.", 4) != 0) {
            continue;
        }
        const char* name = argv[i] + 4;
        bool known = false;
        for (const MemoryField& f : fields) {
            if (std::strcmp(f.name, name) == 0) {
                mg.*f.field = (int)std::strtol(argv[i + 1], nullptr, 10);
                known = true;
            }
        }
        if (!known) {
            std::fprintf(stderr, "unknown memory genome field: %s\n", name);
            std::exit(1);
        }
    }
}
