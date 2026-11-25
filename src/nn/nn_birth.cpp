#include "nn/nn.hpp"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <type_traits>

// building the reference network behind the Blob shell, and freeing it

static_assert(std::is_standard_layout_v<NnBlob>, "the shell must convert back");
static_assert(offsetof(NnBlob, shell) == 0, "the shell must be first");

static uint64_t* words(int n) {
    return (uint64_t*)std::calloc((size_t)n, sizeof(uint64_t));
}

static float* floats(int n) {
    return (float*)std::calloc((size_t)n, sizeof(float));
}

static uint64_t splitmix(uint64_t& s) {
    s += 0x9e3779b97f4a7c15ull;
    uint64_t z = s;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

// torch's default: every weight and bias uniform in +-bound
static void fill_uniform(NnBlob& nb, int from, int count, float bound) {
    for (int i = 0; i < count; i++) {
        float u = (float)(splitmix(nb.rng) >> 40) * (1.0f / 16777216.0f);
        nb.param[from + i] = (2.0f * u - 1.0f) * bound;
    }
}

// sensor and motor lattices as the engines lay them out: every stride-th cell of a region
static void set_masks(Blob& b) {
    const Genome& g = b.genome;
    for (int n = 0; n < b.neuron_count; n++) {
        int x = blob_neuron_x(b, n), y = blob_neuron_y(b, n);
        uint64_t bit = 1ull << (n % TILE_NEURONS);
        bool sensor = x >= g.sensor_x && x < g.sensor_x + g.sensor_w && y >= g.sensor_y &&
                      y < g.sensor_y + g.sensor_h && (x - g.sensor_x) % g.sensor_stride == 0 &&
                      (y - g.sensor_y) % g.sensor_stride == 0;
        bool motor = x >= g.motor_x && x < g.motor_x + g.motor_w && y >= g.motor_y &&
                     y < g.motor_y + g.motor_h && (x - g.motor_x) % g.motor_stride == 0 &&
                     (y - g.motor_y) % g.motor_stride == 0;
        if (sensor) {
            b.sensor[n / TILE_NEURONS] |= bit;
        }
        if (motor && !sensor) {
            b.motor[n / TILE_NEURONS] |= bit;
        }
    }
}

Blob* nn_birth(const Genome& genome, NnKind kind, const NnSettings& s, uint64_t seed) {
    NnBlob* nb = new NnBlob{};
    Blob& b = nb->shell;
    b.genome = genome;
    b.tiles_w = genome.tiles_w;
    b.tiles_h = genome.tiles_h;
    b.tile_count = genome.tiles_w * genome.tiles_h;
    b.neuron_count = b.tile_count * TILE_NEURONS;
    int n = b.tile_count + 1;
    for (int h = 0; h < HISTORY; h++) {
        b.spikes[h] = words(n);
    }
    b.recent = words(n);
    b.inhibitory = words(n);
    b.sensor = words(n);
    b.motor = words(n);
    b.sense_in = words(n);
    b.sense_attached = words(n);
    b.predicted = words(n);
    set_masks(b);

    nb->kind = kind;
    nb->s = s;
    nb->s.window = kind == NnKind::mlp ? 1 : nb->s.window;
    nb->s.window =
        nb->s.window < 1 ? 1 : (nb->s.window > NN_WINDOW_MAX ? NN_WINDOW_MAX : nb->s.window);
    nb->lr = genome.learning_on ? s.lr : 0.0f;
    nb->rng = seed ^ 0x4e6574776f726b00ull; // "Network"

    nb->input_of = (int32_t*)std::malloc(sizeof(int32_t) * (size_t)b.neuron_count);
    for (int i = 0; i < b.neuron_count; i++) {
        bool sensor = (b.sensor[i / TILE_NEURONS] >> (i % TILE_NEURONS)) & 1;
        nb->input_of[i] = sensor && nb->inputs < NN_MAX_INPUTS ? nb->inputs++ : -1;
    }

    // parameter block
    const int H = NN_HIDDEN;
    int gates = kind == NnKind::gru ? 3 : 1;
    int at = 0;
    nb->w_in = at;
    at += nb->inputs * gates * H;
    nb->b_in = at;
    at += gates * H;
    nb->w_rec = at;
    at += kind == NnKind::gru ? H * gates * H : 0;
    nb->b_rec = at;
    at += kind == NnKind::gru ? gates * H : 0;
    nb->w_out = at;
    at += H * NN_OUT;
    nb->b_out = at;
    at += NN_OUT;
    nb->params = at;
    nb->param = floats(at);
    nb->grad = floats(at);
    nb->adam_m = floats(at);
    nb->adam_v = floats(at);
    if (kind == NnKind::gru) {
        fill_uniform(*nb, 0, nb->w_out, 1.0f / std::sqrt((float)H));
    } else {
        float bound = 1.0f / std::sqrt((float)(nb->inputs > 0 ? nb->inputs : 1));
        fill_uniform(*nb, nb->w_in, nb->w_out, bound);
    }
    fill_uniform(*nb, nb->w_out, H * NN_OUT + NN_OUT, 1.0f / std::sqrt((float)H));
    nb->steps = new NnStep[nb->s.window];
    return &b;
}

void nn_free(Blob* blob) {
    if (!blob) {
        return;
    }
    NnBlob* nb = &nn_of(*blob);
    for (int h = 0; h < HISTORY; h++) {
        std::free(blob->spikes[h]);
    }
    std::free(blob->recent);
    std::free(blob->inhibitory);
    std::free(blob->sensor);
    std::free(blob->motor);
    std::free(blob->sense_in);
    std::free(blob->sense_attached);
    std::free(blob->predicted);
    std::free(nb->input_of);
    std::free(nb->param);
    std::free(nb->grad);
    std::free(nb->adam_m);
    std::free(nb->adam_v);
    delete[] nb->steps;
    delete nb;
}

void apply_nn_args(int argc, char** argv, NnSettings& s) {
    for (int i = 0; i + 1 < argc; i++) {
        if (std::strncmp(argv[i], "--n.", 4) != 0) {
            continue;
        }
        const char* name = argv[i] + 4;
        const char* value = argv[i + 1];
        if (std::strcmp(name, "adam") == 0) {
            s.adam = (int)std::strtol(value, nullptr, 10);
        } else if (std::strcmp(name, "lr") == 0) {
            s.lr = std::strtof(value, nullptr);
        } else if (std::strcmp(name, "temperature") == 0) {
            s.temperature = std::strtof(value, nullptr);
        } else if (std::strcmp(name, "baseline_rate") == 0) {
            s.baseline_rate = std::strtof(value, nullptr);
        } else if (std::strcmp(name, "window") == 0) {
            s.window = (int)std::strtol(value, nullptr, 10);
        } else if (std::strcmp(name, "teach") == 0) {
            s.teach = (int)std::strtol(value, nullptr, 10);
        } else {
            std::fprintf(stderr, "unknown reference network setting: %s\n", name);
            std::exit(1);
        }
    }
}
