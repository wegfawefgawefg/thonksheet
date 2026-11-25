#include "blob/random_words.hpp"
#include "table/table.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// birth, death and the command line for the table sheet

struct TableField {
    const char* name;
    int TableGenome::*field;
};

static const TableField fields[] = {
    {"inputs", &TableGenome::inputs},
    {"counter_bits", &TableGenome::counter_bits},
    {"reach", &TableGenome::reach},
    {"far_percent", &TableGenome::far_percent},
    {"far_reach_tiles", &TableGenome::far_reach_tiles},
    {"far_delay_per_tile", &TableGenome::far_delay_per_tile},
    {"local_delay_min", &TableGenome::local_delay_min},
    {"local_delay_max", &TableGenome::local_delay_max},
    {"init_fire_shift", &TableGenome::init_fire_shift},
    {"noise_shift", &TableGenome::noise_shift},
    {"rate_mode", &TableGenome::rate_mode},
    {"sector_tiles", &TableGenome::sector_tiles},
    {"target_per_1024", &TableGenome::target_per_1024},
    {"bar_period_shift", &TableGenome::bar_period_shift},
    {"rate_fire_shift", &TableGenome::rate_fire_shift},
    {"rate_quiet_shift", &TableGenome::rate_quiet_shift},
    {"rate_rest_only", &TableGenome::rate_rest_only},
    {"learning_on", &TableGenome::learning_on},
    {"learn_mode", &TableGenome::learn_mode},
    {"grade", &TableGenome::grade},
    {"baseline", &TableGenome::baseline},
    {"reward_shift", &TableGenome::reward_shift},
    {"rise_extra_shift", &TableGenome::rise_extra_shift},
    {"bad_rise_shift", &TableGenome::bad_rise_shift},
    {"recent_ticks", &TableGenome::recent_ticks},
    {"average_shift", &TableGenome::average_shift},
    {"grow_shift", &TableGenome::grow_shift},
    {"report", &TableGenome::report},
    {"report_ticks", &TableGenome::report_ticks},
};

void table_genome_args(int argc, char** argv, TableGenome& tg) {
    for (int i = 0; i + 1 < argc; i++) {
        if (std::strncmp(argv[i], "--t.", 4) != 0) {
            continue;
        }
        const char* name = argv[i] + 4;
        bool known = false;
        for (const TableField& f : fields) {
            if (std::strcmp(f.name, name) == 0) {
                tg.*f.field = (int)std::strtol(argv[i + 1], nullptr, 10);
                known = true;
            }
        }
        if (!known) {
            std::fprintf(stderr, "unknown table genome field: %s\n", name);
            std::exit(1);
        }
    }
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void* zeroed(size_t bytes) {
    return std::calloc(bytes, 1);
}

// set the bits of every cell inside a rectangle of the sheet
static void mark_region(const Blob& blob, uint64_t* mask, int x0, int y0, int w, int h) {
    for (int y = y0; y < y0 + h; y++) {
        for (int x = x0; x < x0 + w; x++) {
            if (x < 0 || y < 0 || x >= blob.tiles_w * 8 || y >= blob.tiles_h * 8) {
                continue;
            }
            int n = blob_neuron_at(blob, x, y);
            mask[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
        }
    }
}

// the same lattice rule as the blob: every stride-th cell of a region
static void mark_lattice(const Blob& blob, uint64_t* mask, int x0, int y0, int w, int h,
                         int stride) {
    stride = stride < 1 ? 1 : stride;
    for (int y = y0; y < y0 + h; y += stride) {
        for (int x = x0; x < x0 + w; x += stride) {
            mark_region(blob, mask, x, y, 1, 1);
        }
    }
}

static bool on_sheet(const Blob& blob, int x, int y) {
    return x >= 0 && y >= 0 && x < blob.tiles_w * 8 && y < blob.tiles_h * 8;
}

// one input for the neuron at (x, y): mostly a nearby cell at a short delay, sometimes a far
// one whose delay grows with distance. returns (source << 4) | delay.
static uint32_t draw_input(TableBlob& tb, int x, int y) {
    Blob& b = tb.shell;
    const TableGenome& tg = tb.tg;
    for (int tries = 0; tries < 16; tries++) {
        int sx = x, sy = y, delay = 1;
        if ((int)rng_below(tb.rng, 100) < tg.far_percent) {
            // ring of r tiles, r with a long tail
            int r = 2;
            while (r < 32 && rng_below(tb.rng, (uint32_t)tg.far_reach_tiles + 1) != 0) {
                r++;
            }
            int along = (int)rng_below(tb.rng, (uint32_t)(2 * r + 1)) - r;
            int side = (rng_next(tb.rng) & 1) ? r : -r;
            bool swap = rng_next(tb.rng) & 1;
            sx += 8 * (swap ? along : side) + (int)rng_below(tb.rng, 8) - 4;
            sy += 8 * (swap ? side : along) + (int)rng_below(tb.rng, 8) - 4;
            delay = clamp_int(1 + r * tg.far_delay_per_tile, 2, HISTORY - 1);
        } else {
            int span = 2 * tg.reach + 1;
            sx += (int)rng_below(tb.rng, (uint32_t)span) - tg.reach;
            sy += (int)rng_below(tb.rng, (uint32_t)span) - tg.reach;
            int dspan = tg.local_delay_max - tg.local_delay_min + 1;
            delay = tg.local_delay_min + (int)rng_below(tb.rng, (uint32_t)dspan);
        }
        if (on_sheet(b, sx, sy)) {
            return ((uint32_t)blob_neuron_at(b, sx, sy) << 4) | (uint32_t)delay;
        }
    }
    return ((uint32_t)b.tile_count * TILE_NEURONS << 4) | 1u; // the zero word
}

static void fill_tables(TableBlob& tb) {
    // counters start at the middle: just above (fires) with chance 2^-init_fire_shift
    int mid = 1 << (tb.bits - 1);
    int entries = 1 << tb.inputs;
    for (int n = 0; n < tb.shell.neuron_count; n++) {
        uint64_t fire = 0;
        for (int a = 0; a < entries; a++) {
            if ((a & 63) == 0) {
                fire = sparse_word(tb.rng, tb.tg.init_fire_shift);
            }
            uint64_t value = (uint64_t)(((fire >> (a & 63)) & 1) ? mid : mid - 1);
            int at = a * tb.bits;
            tb.table[(int64_t)n * tb.words_per_neuron + (at >> 6)] |= value << (at & 63);
        }
    }
}

Blob* table_birth(const Genome& genome, const TableGenome& tg_in, uint64_t seed) {
    TableBlob* tb = new TableBlob{};
    Blob& b = tb->shell;
    TableGenome& tg = tb->tg;
    tg = tg_in;
    tg.inputs = clamp_int(tg.inputs, 1, TABLE_MAX_INPUTS);
    tg.counter_bits = tg.counter_bits >= 4 ? 4 : (tg.counter_bits >= 2 ? 2 : 1);
    tg.recent_ticks = clamp_int(tg.recent_ticks, 1, TABLE_ADDR_HISTORY);
    tg.reach = clamp_int(tg.reach, 1, 64);
    tg.far_reach_tiles = clamp_int(tg.far_reach_tiles, 1, 32);
    tg.local_delay_min = clamp_int(tg.local_delay_min, 1, HISTORY - 1);
    tg.local_delay_max = clamp_int(tg.local_delay_max, tg.local_delay_min, HISTORY - 1);
    tb->inputs = tg.inputs;
    tb->bits = tg.counter_bits;
    tb->words_per_plane = tg.inputs > 6 ? 1 << (tg.inputs - 6) : 1;
    int table_bits = (1 << tg.inputs) * tb->bits;
    tb->words_per_neuron = table_bits > 64 ? table_bits / 64 : 1;

    b.genome = genome;
    b.tiles_w = genome.tiles_w;
    b.tiles_h = genome.tiles_h;
    b.tile_count = b.tiles_w * b.tiles_h;
    b.neuron_count = b.tile_count * TILE_NEURONS;
    int words = b.tile_count + 1;
    size_t map_bytes = sizeof(uint64_t) * (size_t)words;
    b.spike_stride = words;
    uint64_t* block = (uint64_t*)zeroed(map_bytes * HISTORY);
    uint64_t* flip_block = (uint64_t*)zeroed(map_bytes * HISTORY);
    for (int h = 0; h < HISTORY; h++) {
        b.spikes[h] = block + (size_t)h * words;
        tb->flips[h] = flip_block + (size_t)h * words;
    }
    b.recent = (uint64_t*)zeroed(map_bytes);
    b.inhibitory = (uint64_t*)zeroed(map_bytes);
    b.sensor = (uint64_t*)zeroed(map_bytes);
    b.motor = (uint64_t*)zeroed(map_bytes);
    b.sense_in = (uint64_t*)zeroed(map_bytes);
    b.sense_attached = (uint64_t*)zeroed(map_bytes);
    b.predicted = (uint64_t*)zeroed(map_bytes);
    b.urge = (uint64_t*)zeroed(map_bytes);
    mark_lattice(b, b.sensor, genome.sensor_x, genome.sensor_y, genome.sensor_w, genome.sensor_h,
                 genome.sensor_stride);
    mark_lattice(b, b.motor, genome.motor_x, genome.motor_y, genome.motor_w, genome.motor_h,
                 genome.motor_stride);
    for (int t = 0; t < b.tile_count; t++) {
        b.motor[t] &= ~b.sensor[t];
    }
    rng_seed(tb->rng, seed ^ 0x5461626c65000000ull); // "Table"

    int64_t n_all = b.neuron_count;
    tb->input = (uint32_t*)zeroed(sizeof(uint32_t) * (size_t)(n_all * tb->inputs));
    tb->table = (uint64_t*)zeroed(sizeof(uint64_t) * (size_t)n_all * (size_t)tb->words_per_neuron);
    tb->address = (uint8_t*)zeroed((size_t)n_all * TABLE_ADDR_HISTORY);
    for (int n = 0; n < b.neuron_count; n++) {
        int x = blob_neuron_x(b, n), y = blob_neuron_y(b, n);
        for (int j = 0; j < tb->inputs; j++) {
            tb->input[(int64_t)n * tb->inputs + j] = draw_input(*tb, x, y);
        }
    }
    fill_tables(*tb);

    // sectors, each bar starting at the middle
    tg.sector_tiles = clamp_int(tg.sector_tiles, 1, 1024);
    tb->sectors_w = (b.tiles_w + tg.sector_tiles - 1) / tg.sector_tiles;
    int sectors_h = (b.tiles_h + tg.sector_tiles - 1) / tg.sector_tiles;
    tb->sector_count = tb->sectors_w * sectors_h;
    tb->tile_sector = (uint32_t*)zeroed(sizeof(uint32_t) * (size_t)words);
    tb->bar = (uint8_t*)zeroed((size_t)tb->sector_count);
    tb->sector_free = (uint32_t*)zeroed(sizeof(uint32_t) * (size_t)tb->sector_count);
    tb->sector_fired = (uint32_t*)zeroed(sizeof(uint32_t) * (size_t)tb->sector_count);
    for (int t = 0; t < b.tile_count; t++) {
        int s =
            (t / b.tiles_w / tg.sector_tiles) * tb->sectors_w + (t % b.tiles_w) / tg.sector_tiles;
        tb->tile_sector[t] = (uint32_t)s;
        tb->sector_free[s] += (uint32_t)__builtin_popcountll(~b.sensor[t]);
    }
    for (int s = 0; s < tb->sector_count; s++) {
        tb->bar[s] = (uint8_t)(1 << (tb->bits - 1));
    }
    if (tg.report) {
        tb->fire_count = (uint32_t*)zeroed(sizeof(uint32_t) * (size_t)n_all);
        tb->visited = (uint64_t*)zeroed(sizeof(uint64_t) * (size_t)n_all * tb->words_per_plane);
    }
    return &b;
}

void table_free(Blob* blob) {
    if (!blob) {
        return;
    }
    TableBlob& tb = table_of(*blob);
    if (tb.tg.report) {
        table_report(tb, "end");
    }
    Blob& b = tb.shell;
    std::free(b.spikes[0]);
    std::free(tb.flips[0]);
    std::free(b.recent);
    std::free(b.inhibitory);
    std::free(b.sensor);
    std::free(b.motor);
    std::free(b.sense_in);
    std::free(b.sense_attached);
    std::free(b.predicted);
    std::free(b.urge);
    std::free(tb.input);
    std::free(tb.table);
    std::free(tb.address);
    std::free(tb.fire_count);
    std::free(tb.visited);
    std::free(tb.tile_sector);
    std::free(tb.bar);
    std::free(tb.sector_free);
    std::free(tb.sector_fired);
    delete &tb;
}

double table_bytes_per_neuron(const TableBlob& tb) {
    double input = 4.0 * tb.inputs;
    double table = 8.0 * tb.words_per_neuron;
    double address = TABLE_ADDR_HISTORY;
    double rings = 2.0 * HISTORY / 8.0; // spike and flip rings, one bit each per tick
    return input + table + address + rings;
}
