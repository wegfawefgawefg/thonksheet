#include "blob/random_words.hpp"
#include "minimal/minimal.hpp"
#include "minimal/minimal_steps.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

// building a minimal blob from the shell genome (sheet, sensor and motor regions, learning
// switch) and its own genome, and freeing it.

static_assert(std::is_standard_layout_v<MinimalBlob>, "the shell must convert back");
static_assert(offsetof(MinimalBlob, shell) == 0, "the shell must be first");

static void* alloc_lines(size_t bytes) {
    size_t rounded = (bytes + 63) & ~(size_t)63;
    void* p = std::aligned_alloc(64, rounded ? rounded : 64);
    std::memset(p, 0, rounded ? rounded : 64);
    return p;
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void sanitize(MinimalGenome& mg) {
    mg.budget = clamp_int(mg.budget, 1, MIN_SLOTS * TILE_NEURONS - 1);
    mg.inhibit_gain = clamp_int(mg.inhibit_gain, 0, 64);
    mg.threshold_min = clamp_int(mg.threshold_min, 1, 255);
    mg.threshold_max = clamp_int(mg.threshold_max, mg.threshold_min, 255);
    mg.threshold_cap = clamp_int(mg.threshold_cap, mg.threshold_max, 255);
    mg.far_slots = clamp_int(mg.far_slots, 0, MIN_FAR_MAX);
    mg.far_percent = clamp_int(mg.far_percent, 0, 100);
    mg.far_reach = clamp_int(mg.far_reach, 1, 64);
    mg.delay_max = clamp_int(mg.delay_max, 1, MIN_DELAY_MAX);
    mg.far_delay_per_tile = clamp_int(mg.far_delay_per_tile, 0, 16);
    mg.delay_percent = clamp_int(mg.delay_percent, 0, 100);
    // the oldest map learning reads is credit_ticks - 1 + delay_max ticks back
    mg.credit_ticks = clamp_int(mg.credit_ticks, 1, MIN_HISTORY - 1 - mg.delay_max);
    mg.avg_shift = clamp_int(mg.avg_shift, 0, 16);
    mg.ev_on = clamp_int(mg.ev_on, 0, 2);
    mg.ev_bits = clamp_int(mg.ev_bits, 2, 8);
    mg.ev_init = clamp_int(mg.ev_init, 0, 255);
    mg.ev_read = clamp_int(mg.ev_read, 0, 2);
    mg.ev_fired_min = clamp_int(mg.ev_fired_min, 1, 8);
    mg.ev_shift = clamp_int(mg.ev_shift, 0, 30);
    mg.ev_quiet_shift = clamp_int(mg.ev_quiet_shift, 0, 30);
}

// sensor and motor lattices and inhibitory senders, as the full engine lays them out
static void set_kinds(MinimalBlob& m) {
    Blob& b = m.shell;
    const Genome& g = b.genome;
    const MinimalGenome& mg = m.mg;
    for (int n = 0; n < b.neuron_count; n++) {
        int x = blob_neuron_x(b, n), y = blob_neuron_y(b, n);
        int t = n / TILE_NEURONS;
        uint64_t bit = 1ull << (n % TILE_NEURONS);
        bool in_sensor = x >= g.sensor_x && x < g.sensor_x + g.sensor_w && y >= g.sensor_y &&
                         y < g.sensor_y + g.sensor_h;
        bool is_sensor = in_sensor && (x - g.sensor_x) % g.sensor_stride == 0 &&
                         (y - g.sensor_y) % g.sensor_stride == 0;
        bool in_motor = x >= g.motor_x && x < g.motor_x + g.motor_w && y >= g.motor_y &&
                        y < g.motor_y + g.motor_h;
        bool is_motor = in_motor && (x - g.motor_x) % g.motor_stride == 0 &&
                        (y - g.motor_y) % g.motor_stride == 0;
        if (is_sensor) {
            b.sensor[t] |= bit;
        }
        if (is_motor && !is_sensor) {
            b.motor[t] |= bit;
        }
        if (!is_sensor && (int)rng_below(m.rng, 100) < g.inhibitory_percent) {
            b.inhibitory[t] |= bit;
        }
        int spread = mg.threshold_max - mg.threshold_min + 1;
        m.threshold[n] = (uint8_t)(mg.threshold_min + (int)rng_below(m.rng, (uint32_t)spread));
    }
}

static void set_local_tiles(MinimalBlob& m) {
    const Blob& b = m.shell;
    for (int t = 0; t < b.tile_count; t++) {
        int tx = t % b.tiles_w, ty = t / b.tiles_w;
        for (int k = 0; k < MIN_SLOTS; k++) {
            int x = tx + k % 3 - 1, y = ty + k / 3 - 1;
            bool on = x >= 0 && y >= 0 && x < b.tiles_w && y < b.tiles_h;
            m.local_tile[t * MIN_SLOTS + k] =
                on ? (uint32_t)(y * b.tiles_w + x) : (uint32_t)b.tile_count;
        }
    }
}

// a far tile for a neuron in `tile`: distance mostly 2..reach+1 with a long tail (as the full
// engine draws it), a point on the square ring at that distance. false if it fell off the
// sheet 8 times running; the slot then stays empty.
static bool draw_far(MinimalBlob& m, int tile, uint32_t& far, int& distance) {
    const Blob& b = m.shell;
    int tx = tile % b.tiles_w, ty = tile / b.tiles_w;
    int reach = m.mg.far_reach;
    int longest = b.tiles_w > b.tiles_h ? b.tiles_w : b.tiles_h;
    for (int attempt = 0; attempt < 8; attempt++) {
        int d = 2 + (int)rng_below(m.rng, (uint32_t)reach);
        while ((rng_next(m.rng) & 3) == 0 && d < longest) {
            d += 1 + (int)rng_below(m.rng, (uint32_t)reach);
        }
        int side = (int)rng_below(m.rng, 4);
        int p = (int)rng_below(m.rng, (uint32_t)(2 * d)) - d;
        int dx = side == 0 ? p : (side == 1 ? d : (side == 2 ? -p : -d));
        int dy = side == 0 ? -d : (side == 1 ? p : (side == 2 ? d : -p));
        int x = tx + dx, y = ty + dy;
        if (x < 0 || y < 0 || x >= b.tiles_w || y >= b.tiles_h) {
            continue;
        }
        far = (uint32_t)(y * b.tiles_w + x);
        distance = d;
        return true;
    }
    return false;
}

// far tiles and slot delays. local delays: the smaller of two draws in 1..delay_max, so short
// ones are common; a far slot's delay is its distance x far_delay_per_tile (at least 1).
static void set_far_and_delays(MinimalBlob& m) {
    const Blob& b = m.shell;
    const MinimalGenome& mg = m.mg;
    const int far_slots = m.slots - MIN_SLOTS;
    for (int n = 0; n < b.neuron_count; n++) {
        int t = n / TILE_NEURONS;
        bool sensor = (b.sensor[t] >> (n % TILE_NEURONS)) & 1;
        if (m.delay) {
            for (int k = 0; k < MIN_SLOTS; k++) {
                int d1 = 1 + (int)rng_below(m.rng, (uint32_t)mg.delay_max);
                int d2 = 1 + (int)rng_below(m.rng, (uint32_t)mg.delay_max);
                bool drawn = (int)rng_below(m.rng, 100) < mg.delay_percent;
                m.delay[min_index(m, n, k)] = (uint8_t)(drawn ? (d1 < d2 ? d1 : d2) : 1);
            }
        }
        for (int j = 0; j < far_slots; j++) {
            uint32_t& far =
                m.far_tile[((int64_t)t * far_slots + j) * TILE_NEURONS + n % TILE_NEURONS];
            far = (uint32_t)b.tile_count;
            int distance = 1;
            if (!sensor && (int)rng_below(m.rng, 100) < mg.far_percent) {
                draw_far(m, t, far, distance);
            }
            if (m.delay) {
                m.delay[min_index(m, n, MIN_SLOTS + j)] =
                    (uint8_t)clamp_int(distance * mg.far_delay_per_tile, 1, mg.delay_max);
            }
        }
    }
}

// every non-sensor neuron gets `budget` random sources over its slots. no structure.
static void wire(MinimalBlob& m) {
    const Blob& b = m.shell;
    for (int n = 0; n < b.neuron_count; n++) {
        if ((b.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1) {
            continue;
        }
        int have = 0;
        for (int tries = 0; have < m.mg.budget && tries < 100000; tries++) {
            int k = (int)rng_below(m.rng, (uint32_t)m.slots);
            uint64_t bit = 1ull << rng_below(m.rng, 64);
            uint64_t& word = m.mask[min_index(m, n, k)];
            if ((min_allowed(m, n, k) & bit) && !(word & bit)) {
                word |= bit;
                have++;
            }
        }
    }
}

Blob* minimal_birth(const Genome& genome, const MinimalGenome& mg, uint64_t seed) {
    MinimalBlob* m = new MinimalBlob{};
    Blob& b = m->shell;
    b.genome = genome;
    m->mg = mg;
    sanitize(m->mg);
    m->mg.learning_on = genome.learning_on && mg.learning_on;
    rng_seed(m->rng, seed ^ 0x4d696e696d616c00ull); // "Minimal"

    b.tiles_w = genome.tiles_w;
    b.tiles_h = genome.tiles_h;
    b.tile_count = b.tiles_w * b.tiles_h;
    b.neuron_count = b.tile_count * TILE_NEURONS;
    size_t map_bytes = sizeof(uint64_t) * (size_t)(b.tile_count + 1);
    size_t n = (size_t)b.neuron_count;

    // the shell: what the plug and tasks touch
    for (int h = 0; h < HISTORY; h++) {
        b.spikes[h] = (uint64_t*)alloc_lines(map_bytes);
    }
    b.recent = (uint64_t*)alloc_lines(map_bytes);
    b.inhibitory = (uint64_t*)alloc_lines(map_bytes);
    b.sensor = (uint64_t*)alloc_lines(map_bytes);
    b.motor = (uint64_t*)alloc_lines(map_bytes);
    b.sense_in = (uint64_t*)alloc_lines(map_bytes);
    b.sense_attached = (uint64_t*)alloc_lines(map_bytes);
    b.predicted = (uint64_t*)alloc_lines(map_bytes);

    // the engine
    m->stride = (b.tile_count + 1 + 7) & ~7;
    m->spikes = (uint64_t*)alloc_lines(sizeof(uint64_t) * m->stride * MIN_HISTORY);
    m->noise = (uint64_t*)alloc_lines(sizeof(uint64_t) * m->stride * MIN_HISTORY);
    m->slots = MIN_SLOTS + m->mg.far_slots;
    size_t slots = (size_t)m->slots;
    m->local_tile = (uint32_t*)alloc_lines(sizeof(uint32_t) * b.tile_count * MIN_SLOTS);
    if (m->mg.far_slots > 0) {
        m->far_tile = (uint32_t*)alloc_lines(sizeof(uint32_t) * n * (size_t)m->mg.far_slots);
    }
    m->mask = (uint64_t*)alloc_lines(sizeof(uint64_t) * n * slots);
    if (m->mg.delay_max > 1) {
        m->delay = (uint8_t*)alloc_lines(n * slots);
    }
    if (m->mg.drop_mode == 1) {
        m->used = (uint64_t*)alloc_lines(sizeof(uint64_t) * n * slots);
    }
    m->threshold = (uint8_t*)alloc_lines(n);

    set_kinds(*m);
    set_local_tiles(*m);
    set_far_and_delays(*m);
    wire(*m);
    if (m->mg.ev_on) {
        // evidence counters: rows for every neuron that might have them, filled from the wiring
        m->ev_tiles = (uint64_t*)alloc_lines(map_bytes);
        m->ev_row = (int32_t*)alloc_lines(sizeof(int32_t) * n);
        int rows = 0;
        for (int t = 0; t < b.tile_count; t++) {
            uint64_t kind = m->mg.ev_on == 2 ? ~0ull : b.motor[t];
            rows += __builtin_popcountll(kind & ~b.sensor[t]);
        }
        m->ev_rows = rows;
        m->ev_planes =
            (uint64_t*)alloc_lines(sizeof(uint64_t) * (size_t)rows * slots * m->mg.ev_bits);
        m->ev_line = (uint8_t*)alloc_lines((size_t)rows);
        min_evidence_birth(*m);
    }
    return &b;
}

void minimal_free(Blob* blob) {
    if (!blob) {
        return;
    }
    MinimalBlob* m = &minimal_of(*blob);
    for (int h = 0; h < HISTORY; h++) {
        std::free(blob->spikes[h]);
    }
    void* arrays[] = {blob->recent,   blob->inhibitory,     blob->sensor,    blob->motor,
                      blob->sense_in, blob->sense_attached, blob->predicted, m->spikes,
                      m->noise,       m->local_tile,        m->mask,         m->used,
                      m->threshold,   m->far_tile,          m->delay,        m->ev_tiles,
                      m->ev_row,      m->ev_planes,         m->ev_line};
    for (void* p : arrays) {
        std::free(p);
    }
    delete m;
}

int minimal_fan_in(const MinimalBlob& m, int n) {
    int count = 0;
    for (int k = 0; k < m.slots; k++) {
        count += __builtin_popcountll(m.mask[min_index(m, n, k)]);
    }
    return count;
}

size_t minimal_bytes(const MinimalBlob& m) {
    size_t n = (size_t)m.shell.neuron_count;
    size_t bytes = 2 * sizeof(uint64_t) * (size_t)m.stride * MIN_HISTORY;
    bytes += sizeof(uint32_t) * (size_t)m.shell.tile_count * MIN_SLOTS;
    bytes += sizeof(uint64_t) * n * (size_t)m.slots * (m.used ? 2 : 1);
    bytes += sizeof(uint32_t) * n * (size_t)(m.slots - MIN_SLOTS); // far tiles
    bytes += m.delay ? n * (size_t)m.slots : 0;                    // delays
    bytes += n;                                                    // thresholds
    // the per-tile words the engine reads from the shell: sensor, inhibitory, sense_in
    bytes += 3 * sizeof(uint64_t) * (size_t)(m.shell.tile_count + 1);
    if (m.ev_planes) {
        // evidence: the counters (packed: bits per source), a line per row, the row index
        bytes += sizeof(uint64_t) * (size_t)m.ev_rows * (size_t)m.slots * (size_t)m.mg.ev_bits;
        bytes += (size_t)m.ev_rows + sizeof(uint32_t) * n;
        bytes += sizeof(uint64_t) * (size_t)(m.shell.tile_count + 1);
    }
    return bytes;
}

struct MinimalField {
    const char* name;
    int MinimalGenome::*field;
};

static const MinimalField fields[] = {
    {"learning_on", &MinimalGenome::learning_on},
    {"budget", &MinimalGenome::budget},
    {"budget_on", &MinimalGenome::budget_on},
    {"inhibit_gain", &MinimalGenome::inhibit_gain},
    {"threshold_min", &MinimalGenome::threshold_min},
    {"threshold_max", &MinimalGenome::threshold_max},
    {"threshold_cap", &MinimalGenome::threshold_cap},
    {"up_shift", &MinimalGenome::up_shift},
    {"down_shift", &MinimalGenome::down_shift},
    {"noise_shift", &MinimalGenome::noise_shift},
    {"credit_ticks", &MinimalGenome::credit_ticks},
    {"learn_shift", &MinimalGenome::learn_shift},
    {"avg_shift", &MinimalGenome::avg_shift},
    {"grade_mode", &MinimalGenome::grade_mode},
    {"drop_mode", &MinimalGenome::drop_mode},
    {"cowire_on", &MinimalGenome::cowire_on},
    {"cowire_shift", &MinimalGenome::cowire_shift},
    {"cowire_needs_input", &MinimalGenome::cowire_needs_input},
    {"far_slots", &MinimalGenome::far_slots},
    {"far_percent", &MinimalGenome::far_percent},
    {"far_reach", &MinimalGenome::far_reach},
    {"delay_max", &MinimalGenome::delay_max},
    {"far_delay_per_tile", &MinimalGenome::far_delay_per_tile},
    {"delay_percent", &MinimalGenome::delay_percent},
    {"noise_sign", &MinimalGenome::noise_sign},
    {"noise_pair", &MinimalGenome::noise_pair},
    {"ev_on", &MinimalGenome::ev_on},
    {"ev_bits", &MinimalGenome::ev_bits},
    {"ev_init", &MinimalGenome::ev_init},
    {"ev_read", &MinimalGenome::ev_read},
    {"ev_spikes", &MinimalGenome::ev_spikes},
    {"ev_fired_min", &MinimalGenome::ev_fired_min},
    {"ev_quiet", &MinimalGenome::ev_quiet},
    {"ev_quiet_shift", &MinimalGenome::ev_quiet_shift},
    {"ev_shift", &MinimalGenome::ev_shift},
    {"ev_inh", &MinimalGenome::ev_inh},
    {"ev_cowire", &MinimalGenome::ev_cowire},
};

void apply_minimal_args(int argc, char** argv, MinimalGenome& mg) {
    for (int i = 0; i + 1 < argc; i++) {
        if (std::strncmp(argv[i], "--m.", 4) != 0) {
            continue;
        }
        const char* name = argv[i] + 4;
        bool known = false;
        for (const MinimalField& f : fields) {
            if (std::strcmp(f.name, name) == 0) {
                mg.*f.field = (int)std::strtol(argv[i + 1], nullptr, 10);
                known = true;
            }
        }
        if (!known) {
            std::fprintf(stderr, "unknown minimal genome field: %s\n", name);
            std::exit(1);
        }
    }
}
