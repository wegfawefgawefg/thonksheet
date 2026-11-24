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
    mg.credit_ticks = clamp_int(mg.credit_ticks, 1, MIN_HISTORY - 2);
    mg.avg_shift = clamp_int(mg.avg_shift, 0, 16);
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

// every non-sensor neuron gets `budget` random local sources. no structure.
static void wire(MinimalBlob& m) {
    const Blob& b = m.shell;
    for (int n = 0; n < b.neuron_count; n++) {
        if ((b.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1) {
            continue;
        }
        int have = 0;
        for (int tries = 0; have < m.mg.budget && tries < 100000; tries++) {
            int k = (int)rng_below(m.rng, MIN_SLOTS);
            uint64_t bit = 1ull << rng_below(m.rng, 64);
            uint64_t& word = m.mask[min_index(n, k)];
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
    m->local_tile = (uint32_t*)alloc_lines(sizeof(uint32_t) * b.tile_count * MIN_SLOTS);
    m->mask = (uint64_t*)alloc_lines(sizeof(uint64_t) * n * MIN_SLOTS);
    if (m->mg.drop_mode == 1) {
        m->used = (uint64_t*)alloc_lines(sizeof(uint64_t) * n * MIN_SLOTS);
    }
    m->threshold = (uint8_t*)alloc_lines(n);

    set_kinds(*m);
    set_local_tiles(*m);
    wire(*m);
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
                      m->threshold};
    for (void* p : arrays) {
        std::free(p);
    }
    delete m;
}

int minimal_fan_in(const MinimalBlob& m, int n) {
    int count = 0;
    for (int k = 0; k < MIN_SLOTS; k++) {
        count += __builtin_popcountll(m.mask[min_index(n, k)]);
    }
    return count;
}

size_t minimal_bytes(const MinimalBlob& m) {
    size_t n = (size_t)m.shell.neuron_count;
    size_t bytes = 2 * sizeof(uint64_t) * (size_t)m.stride * MIN_HISTORY;
    bytes += sizeof(uint32_t) * (size_t)m.shell.tile_count * MIN_SLOTS;
    bytes += sizeof(uint64_t) * n * MIN_SLOTS * (m.used ? 2 : 1);
    bytes += n; // thresholds
    // the per-tile words the engine reads from the shell: sensor, inhibitory, sense_in
    bytes += 3 * sizeof(uint64_t) * (size_t)(m.shell.tile_count + 1);
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
