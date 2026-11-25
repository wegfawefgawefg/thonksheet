#include "blob/random_words.hpp"
#include "vm/vm.hpp"

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <type_traits>

// building a vm from the shell genome (sheet size, sensor and motor lattices, the learning
// switch) and its own numbers, and freeing it.

static_assert(std::is_standard_layout_v<VmBlob>, "the shell must convert back");
static_assert(offsetof(VmBlob, shell) == 0, "the shell must be first");

static void* alloc_lines(size_t bytes) {
    size_t rounded = (bytes + 63) & ~(size_t)63;
    void* p = std::aligned_alloc(64, rounded ? rounded : 64);
    std::memset(p, 0, rounded ? rounded : 64);
    return p;
}

static uint64_t* words(size_t n) {
    return (uint64_t*)alloc_lines(n * sizeof(uint64_t));
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static void sanitize(VmGenome& vg) {
    vg.entries = clamp_int(vg.entries, 16, 1 << 20);
    vg.active = clamp_int(vg.active, 1, vg.entries / 2);
    vg.x_words = clamp_int(vg.x_words, 1, 1000);
    vg.x_bits = clamp_int(vg.x_bits, 1, 64);
    vg.addr_x_shift = clamp_int(vg.addr_x_shift, 0, 12);
    vg.addr_m_shift = clamp_int(vg.addr_m_shift, 0, 12);
    vg.addr_o_shift = clamp_int(vg.addr_o_shift, 0, 12);
    vg.set_shift = clamp_int(vg.set_shift, 1, 30);
    vg.clr_shift = clamp_int(vg.clr_shift, 1, 30);
    vg.m_target = clamp_int(vg.m_target, 0, 1024);
    vg.out_noise_shift = clamp_int(vg.out_noise_shift, 1, 30);
    vg.out_bar_bits = clamp_int(vg.out_bar_bits, 1, 15);
    vg.credit_ticks = clamp_int(vg.credit_ticks, 1, 1 << 20);
    vg.ev_max = clamp_int(vg.ev_max, 1, 127);
    vg.write_keep_shift = clamp_int(vg.write_keep_shift, 0, 12);
    vg.write_ev = clamp_int(vg.write_ev, 0, vg.ev_max);
    vg.write_sample = clamp_int(vg.write_sample, 1, vg.entries);
    vg.inner_shift = clamp_int(vg.inner_shift, 0, 30);
    vg.event_set_shift = clamp_int(vg.event_set_shift, 1, 30);
    vg.event_clr_shift = clamp_int(vg.event_clr_shift, 1, 30);
}

// a word with each bit set at density 2^-shift (shift 0: all ones)
static uint64_t density_word(uint64_t* rng, int shift) {
    return sparse_word(rng, shift);
}

// the lattices, as the other engines lay them out: every stride-th cell of each region
static void set_kinds(VmBlob& v) {
    Blob& b = v.shell;
    const Genome& g = b.genome;
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
        } else if (is_motor) {
            b.motor[t] |= bit;
        }
    }
}

// count and list the three kinds of cell
static void list_cells(VmBlob& v) {
    Blob& b = v.shell;
    for (int pass = 0; pass < 2; pass++) {
        v.sensor_cells = v.motor_cells = v.free_cells = 0;
        for (int n = 0; n < b.neuron_count; n++) {
            uint64_t bit = 1ull << (n % TILE_NEURONS);
            int t = n / TILE_NEURONS;
            if (b.sensor[t] & bit) {
                if (pass) {
                    v.sensor_cell[v.sensor_cells] = n;
                }
                v.sensor_cells++;
            } else if (b.motor[t] & bit) {
                if (pass) {
                    v.motor_cell[v.motor_cells] = n;
                }
                v.motor_cells++;
            } else {
                if (pass) {
                    v.free_cell[v.free_cells] = n;
                }
                v.free_cells++;
            }
        }
        if (!pass) {
            v.sensor_cell = (int32_t*)alloc_lines(sizeof(int32_t) * (size_t)(v.sensor_cells + 1));
            v.motor_cell = (int32_t*)alloc_lines(sizeof(int32_t) * (size_t)(v.motor_cells + 1));
            v.free_cell = (int32_t*)alloc_lines(sizeof(int32_t) * (size_t)(v.free_cells + 1));
        }
    }
}

// a fresh random operation for entry e: sparse set and clear masks over M
void vm_random_effect(VmBlob& v, int e, int set_shift, int clr_shift) {
    uint64_t* s = v.set + (size_t)e * v.m_words;
    uint64_t* c = v.clr + (size_t)e * v.m_words;
    for (int w = 0; w < v.m_words; w++) {
        uint64_t sw = density_word(v.rng, set_shift);
        uint64_t cw = density_word(v.rng, clr_shift) & ~sw;
        s[w] = sw;
        c[w] = cw;
    }
    // bits past the last free cell never exist
    int tail = v.free_cells % 64;
    if (tail) {
        uint64_t keep = (1ull << tail) - 1;
        s[v.m_words - 1] &= keep;
        c[v.m_words - 1] &= keep;
    }
}

static void random_address(VmBlob& v, int e) {
    uint64_t* a = v.addr + (size_t)e * v.a_words;
    int w = 0;
    for (int k = 0; k < v.x_words; k++) {
        a[w++] = density_word(v.rng, v.vg.addr_x_shift);
    }
    for (int k = 0; k < v.m_words; k++) {
        a[w++] = density_word(v.rng, v.vg.addr_m_shift);
    }
    for (int k = 0; v.vg.addr_o && k < v.o_words; k++) {
        a[w++] = density_word(v.rng, v.vg.addr_o_shift);
    }
    int xs = 0;
    for (int k = 0; k < v.x_words; k++) {
        xs += __builtin_popcountll(a[k]);
    }
    v.x_size[e] = (uint16_t)xs;
}

Blob* vm_birth(const Genome& genome, const VmGenome& vg_in, uint64_t seed) {
    VmBlob* v = (VmBlob*)alloc_lines(sizeof(VmBlob));
    new (v) VmBlob{};
    v->vg = vg_in;
    sanitize(v->vg);
    v->vg.learning_on = genome.learning_on && vg_in.learning_on;
    rng_seed(v->rng, seed ^ 0x766d000000000000ull); // "vm"

    Blob& b = v->shell;
    b.genome = genome;
    b.tiles_w = genome.tiles_w;
    b.tiles_h = genome.tiles_h;
    b.tile_count = b.tiles_w * b.tiles_h;
    b.neuron_count = b.tile_count * TILE_NEURONS;
    size_t n = (size_t)b.tile_count + 1;
    b.spike_stride = (int)n;
    b.spikes[0] = words(n * HISTORY);
    for (int h = 1; h < HISTORY; h++) {
        b.spikes[h] = b.spikes[0] + (size_t)h * n;
    }
    b.recent = words(n);
    b.inhibitory = words(n);
    b.sensor = words(n);
    b.motor = words(n);
    b.sense_in = words(n);
    b.sense_attached = words(n);
    b.predicted = words(n);
    b.motor_attached = words(n);
    b.motor_acted = words(n);
    set_kinds(*v);
    list_cells(*v);

    const VmGenome& vg = v->vg;
    v->x_words = vg.x_words;
    v->m_words = (v->free_cells + 63) / 64;
    v->o_words = (v->motor_cells + 63) / 64;
    v->a_words = v->x_words + v->m_words + (vg.addr_o ? v->o_words : 0);
    v->s_words = v->x_words + v->m_words + v->o_words;
    v->state = words((size_t)v->s_words);

    // the input code: each sensor cell lights x_bits fixed random bits of X
    int xbits = v->x_words * 64;
    v->x_code = (uint16_t*)alloc_lines(sizeof(uint16_t) * (size_t)v->sensor_cells * vg.x_bits);
    for (int i = 0; i < v->sensor_cells * vg.x_bits; i++) {
        v->x_code[i] = (uint16_t)rng_below(v->rng, (uint32_t)xbits);
    }

    size_t E = (size_t)vg.entries;
    v->addr = words(E * v->a_words);
    v->set = words(E * v->m_words);
    v->clr = words(E * v->m_words);
    v->ev = (int8_t*)alloc_lines(E * (size_t)v->motor_cells);
    v->last_fired = (uint32_t*)alloc_lines(sizeof(uint32_t) * E);
    v->match = (uint16_t*)alloc_lines(sizeof(uint16_t) * E);
    v->match_x = (uint16_t*)alloc_lines(sizeof(uint16_t) * E);
    v->x_size = (uint16_t*)alloc_lines(sizeof(uint16_t) * E);
    v->x_last = words((size_t)v->x_words);
    v->winners = (int32_t*)alloc_lines(sizeof(int32_t) * (size_t)vg.active);
    v->fresh = (uint8_t*)alloc_lines((size_t)vg.active);
    v->vote = (int16_t*)alloc_lines(sizeof(int16_t) * (size_t)(v->motor_cells + 1));
    v->delta = (int8_t*)alloc_lines((size_t)(v->motor_cells + 1));
    for (int e = 0; e < vg.entries; e++) {
        random_address(*v, e);
        vm_random_effect(*v, e, vg.set_shift, vg.clr_shift);
    }
    // M starts at its target density
    uint64_t* m = vm_m(*v);
    for (int i = 0; i < v->free_cells; i++) {
        if ((int)rng_below(v->rng, 1024) < vg.m_target) {
            m[i / 64] |= 1ull << (i % 64);
        }
    }
    return &v->shell;
}

void vm_free(Blob* blob) {
    if (!blob) {
        return;
    }
    VmBlob* v = &vm_of(*blob);
    Blob& b = v->shell;
    void* shell_arrays[] = {b.spikes[0],      b.recent,     b.inhibitory,     b.sensor,
                            b.motor,          b.sense_in,   b.sense_attached, b.predicted,
                            b.motor_attached, b.motor_acted};
    for (void* p : shell_arrays) {
        std::free(p);
    }
    void* own[] = {v->sensor_cell, v->motor_cell, v->free_cell, v->x_code, v->state,
                   v->addr,        v->set,        v->clr,       v->ev,     v->last_fired,
                   v->match,       v->winners,    v->vote,      v->delta,  v->match_x,
                   v->x_size,      v->x_last,     v->fresh};
    for (void* p : own) {
        std::free(p);
    }
    v->~VmBlob();
    std::free(v);
}

size_t vm_bytes(const VmBlob& v) {
    size_t E = (size_t)v.vg.entries;
    size_t bytes = 0;
    bytes += E * (size_t)(v.a_words + 2 * v.m_words) * sizeof(uint64_t); // address, set, clear
    bytes += E * (size_t)v.motor_cells;                                  // evidence
    bytes += E * (sizeof(uint32_t) + 3 * sizeof(uint16_t));              // stamp, matches, x size
    bytes += (size_t)v.s_words * sizeof(uint64_t);                       // the state
    bytes += (size_t)v.sensor_cells * v.vg.x_bits * sizeof(uint16_t);    // input code
    return bytes;
}

struct VmField {
    const char* name;
    int VmGenome::*field;
};

static const VmField fields[] = {
    {"learning_on", &VmGenome::learning_on},
    {"entries", &VmGenome::entries},
    {"active", &VmGenome::active},
    {"x_words", &VmGenome::x_words},
    {"x_bits", &VmGenome::x_bits},
    {"addr_x_shift", &VmGenome::addr_x_shift},
    {"addr_m_shift", &VmGenome::addr_m_shift},
    {"addr_o", &VmGenome::addr_o},
    {"addr_o_shift", &VmGenome::addr_o_shift},
    {"set_shift", &VmGenome::set_shift},
    {"clr_shift", &VmGenome::clr_shift},
    {"m_target", &VmGenome::m_target},
    {"homeostat", &VmGenome::homeostat},
    {"out_noise_shift", &VmGenome::out_noise_shift},
    {"out_bar_bits", &VmGenome::out_bar_bits},
    {"credit_ticks", &VmGenome::credit_ticks},
    {"ev_max", &VmGenome::ev_max},
    {"ev_good", &VmGenome::ev_good},
    {"write_on_bad", &VmGenome::write_on_bad},
    {"write_on_good", &VmGenome::write_on_good},
    {"write_keep_shift", &VmGenome::write_keep_shift},
    {"write_ev", &VmGenome::write_ev},
    {"write_sample", &VmGenome::write_sample},
    {"write_effect", &VmGenome::write_effect},
    {"inner_on", &VmGenome::inner_on},
    {"inner_shift", &VmGenome::inner_shift},
    {"event_on", &VmGenome::event_on},
    {"event_min_bits", &VmGenome::event_min_bits},
    {"event_set_shift", &VmGenome::event_set_shift},
    {"event_clr_shift", &VmGenome::event_clr_shift},
    {"event_m", &VmGenome::event_m},
    {"event_route", &VmGenome::event_route},
    {"m_slide_words", &VmGenome::m_slide_words},
    {"refract", &VmGenome::refract},
};

void apply_vm_args(int argc, char** argv, VmGenome& vg) {
    for (int i = 0; i + 1 < argc; i++) {
        if (std::strncmp(argv[i], "--v.", 4) != 0) {
            continue;
        }
        const char* name = argv[i] + 4;
        bool known = false;
        for (const VmField& f : fields) {
            if (std::strcmp(f.name, name) == 0) {
                vg.*f.field = (int)std::strtol(argv[i + 1], nullptr, 10);
                known = true;
            }
        }
        if (!known) {
            std::fprintf(stderr, "unknown vm number: %s\n", name);
            std::exit(1);
        }
    }
}
