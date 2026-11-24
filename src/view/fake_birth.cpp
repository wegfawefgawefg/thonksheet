// fake_blob_birth: a Blob laid out like docs/design.md (tiles, 11 slots, 2-bit weights,
// kinds, sectors), with wiring drawn at random from the genome's numbers.

#include "fake_blob.hpp"

#include <algorithm>
#include <bit>
#include <cmath>

const char* const fake_scene_names[FAKE_SCENE_COUNT] = {
    "mixed", "healthy", "dying", "saturating", "cycling",
};

uint64_t fake_random(uint64_t& state) {
    // splitmix64
    uint64_t z = (state += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

float fake_chance(uint64_t& state) {
    return static_cast<float>(fake_random(state) >> 40) * (1.0f / 16777216.0f);
}

static int draw_range(uint64_t& rng, int lo, int hi) {
    return lo +
           static_cast<int>(fake_random(rng) % static_cast<uint64_t>(std::max(1, hi - lo + 1)));
}

static bool in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return x >= rx && y >= ry && x < rx + rw && y < ry + rh;
}

// point every Blob array at its vector
static void allocate(FakeBlob& f) {
    Blob& b = f.blob;
    const int words = b.tile_count + 1;
    const int n = b.neuron_count;
    f.spike_store.assign(static_cast<size_t>(HISTORY) * words, 0);
    for (int h = 0; h < HISTORY; ++h) {
        b.spikes[h] = &f.spike_store[static_cast<size_t>(h) * words];
    }
    b.spike_stride = words;
    f.word_store.assign(static_cast<size_t>(7) * words, 0);
    b.recent = &f.word_store[0 * words];
    b.inhibitory = &f.word_store[1 * words];
    b.sensor = &f.word_store[2 * words];
    b.motor = &f.word_store[3 * words];
    b.sense_in = &f.word_store[4 * words];
    b.sense_attached = &f.word_store[5 * words];
    b.predicted = &f.word_store[6 * words];
    f.weight_store.assign(static_cast<size_t>(2) * n * SLOTS, 0);
    b.w0 = &f.weight_store[0];
    b.w1 = &f.weight_store[static_cast<size_t>(n) * SLOTS];
    f.local_store.assign(static_cast<size_t>(b.tile_count) * LOCAL_SLOTS, 0);
    f.far_store.assign(static_cast<size_t>(n) * FAR_SLOTS, 0);
    f.far_delay.assign(static_cast<size_t>(n) * SLOTS, 1); // every slot has a delay; local ones 1
    b.local_tile = f.local_store.data();
    b.far_tile = f.far_store.data();
    b.slot_delay = f.far_delay.data();
    f.potential.assign(n, 0);
    f.threshold.assign(n, 0);
    f.leak_shift.assign(n, 0);
    f.cooldown.assign(n, 0);
    f.cooldown_len.assign(n, 0);
    f.fan_in.assign(n, 0);
    f.pacemaker.assign(n, 0);
    f.phase.assign(n, 0);
    b.potential = f.potential.data();
    b.threshold = f.threshold.data();
    b.leak_shift = f.leak_shift.data();
    b.cooldown = f.cooldown.data();
    b.cooldown_len = f.cooldown_len.data();
    b.fan_in = f.fan_in.data();
    // sector arrays: tile_sector, then activity and surprise
    f.sector_store.assign(static_cast<size_t>(b.tile_count) + 2 * b.sector_count, 0);
    b.tile_sector = &f.sector_store[0];
    b.sector_activity = &f.sector_store[b.tile_count];
    b.sector_surprise = &f.sector_store[b.tile_count + b.sector_count];
    f.sector_bias.assign(b.sector_count, 0);
    b.sector_bias = f.sector_bias.data();
}

static void place_kinds(FakeBlob& f) {
    Blob& b = f.blob;
    const Genome& g = b.genome;
    for (int n = 0; n < b.neuron_count; ++n) {
        const int x = blob_neuron_x(b, n);
        const int y = blob_neuron_y(b, n);
        const uint64_t bit = 1ull << (n % TILE_NEURONS);
        const int t = n / TILE_NEURONS;
        // sensors: a lattice, every sensor_stride-th cell both ways inside the sensor rect, all
        // excitatory
        const int ss = std::max(1, g.sensor_stride);
        const bool lattice = (x - g.sensor_x) % ss == 0 && (y - g.sensor_y) % ss == 0;
        if (in_rect(x, y, g.sensor_x, g.sensor_y, g.sensor_w, g.sensor_h) && lattice) {
            b.sensor[t] |= bit;
        } else if (static_cast<int>(fake_random(f.rng) % 100) < g.inhibitory_percent) {
            b.inhibitory[t] |= bit;
        }
        // motor cells: a lattice through the motor rect the same way, among ordinary neurons
        const int ms = std::max(1, g.motor_stride);
        const bool motor_lattice = (x - g.motor_x) % ms == 0 && (y - g.motor_y) % ms == 0;
        if (in_rect(x, y, g.motor_x, g.motor_y, g.motor_w, g.motor_h) && motor_lattice) {
            b.motor[t] |= bit;
        }
    }
}

// sectors of sector_tiles x sector_tiles tiles; in the mixed scene three of them misbehave
static void place_sectors(FakeBlob& f) {
    Blob& b = f.blob;
    const int st = std::max(1, b.genome.sector_tiles);
    const int sw = (b.tiles_w + st - 1) / st;
    for (int t = 0; t < b.tile_count; ++t) {
        b.tile_sector[t] = static_cast<uint32_t>((t / b.tiles_w) / st * sw + (t % b.tiles_w) / st);
    }
    const int sh = (b.tiles_h + st - 1) / st;
    const FakeMood whole[FAKE_SCENE_COUNT] = {MOOD_HEALTHY, MOOD_HEALTHY, MOOD_DYING,
                                              MOOD_SATURATING, MOOD_CYCLING};
    f.mood.assign(b.sector_count, whole[f.scene]);
    if (f.scene == FAKE_MIXED && b.sector_count >= 4) {
        f.mood[sw - 1] = MOOD_DYING;                   // top right
        f.mood[(sh - 1) * sw] = MOOD_SATURATING;       // bottom left
        f.mood[(sh / 2) * sw + sw / 2] = MOOD_CYCLING; // middle
    }
}

// local slot k is the tile at (dx, dy) = (k % 3 - 1, k / 3 - 1); off the sheet is the zero word
static void wire_local(FakeBlob& f) {
    Blob& b = f.blob;
    for (int t = 0; t < b.tile_count; ++t) {
        const int tx = t % b.tiles_w;
        const int ty = t / b.tiles_w;
        for (int k = 0; k < LOCAL_SLOTS; ++k) {
            const int x = tx + k % 3 - 1;
            const int y = ty + k / 3 - 1;
            const bool on = x >= 0 && y >= 0 && x < b.tiles_w && y < b.tiles_h;
            b.local_tile[t * LOCAL_SLOTS + k] =
                static_cast<uint32_t>(on ? y * b.tiles_w + x : b.tile_count);
        }
    }
}

// far slots: some excitatory neurons reach a tile at a long-tailed distance
static void wire_far(FakeBlob& f) {
    Blob& b = f.blob;
    const Genome& g = b.genome;
    for (int n = 0; n < b.neuron_count; ++n) {
        const int t = n / TILE_NEURONS;
        const uint64_t bit = 1ull << (n % TILE_NEURONS);
        for (int k = 0; k < FAR_SLOTS; ++k) {
            b.far_tile[n * FAR_SLOTS + k] = static_cast<uint32_t>(b.tile_count);
            const int percent = k == 0 ? g.far_percent : g.far_percent / 4;
            if ((b.inhibitory[t] & bit) || static_cast<int>(fake_random(f.rng) % 100) >= percent) {
                continue;
            }
            const float reach = g.far_reach_tiles * -std::log(std::max(1e-4f, fake_chance(f.rng)));
            const float angle = fake_chance(f.rng) * 6.2831853f;
            const int fx =
                t % b.tiles_w + static_cast<int>(std::lround((1.5f + reach) * std::cos(angle)));
            const int fy =
                t / b.tiles_w + static_cast<int>(std::lround((1.5f + reach) * std::sin(angle)));
            if (fx < 0 || fy < 0 || fx >= b.tiles_w || fy >= b.tiles_h) {
                continue;
            }
            const int distance =
                std::max(std::abs(fx - t % b.tiles_w), std::abs(fy - t / b.tiles_w));
            b.far_tile[n * FAR_SLOTS + k] = static_cast<uint32_t>(fy * b.tiles_w + fx);
            b.slot_delay[blob_weight_index(n, LOCAL_SLOTS + k)] =
                static_cast<uint8_t>(std::clamp(1 + distance * g.far_delay_per_tile, 2, 15));
        }
    }
}

// weights: each possible source wired with chance 2^-local_connect_shift (far: 1/4), and a
// wired one strong (4, or 5 half the time) with chance 2^-strong_shift
static void wire_weights(FakeBlob& f) {
    Blob& b = f.blob;
    const Genome& g = b.genome;
    for (int n = 0; n < b.neuron_count; ++n) {
        int fan_in = 0;
        for (int k = 0; k < SLOTS; ++k) {
            const int src =
                k < LOCAL_SLOTS
                    ? static_cast<int>(b.local_tile[(n / TILE_NEURONS) * LOCAL_SLOTS + k])
                    : static_cast<int>(b.far_tile[n * FAR_SLOTS + k - LOCAL_SLOTS]);
            if (src >= b.tile_count) {
                continue;
            }
            uint64_t wired = ~0ull;
            const int shift = k < LOCAL_SLOTS ? g.local_connect_shift : 2;
            for (int i = 0; i < shift; ++i) {
                wired &= fake_random(f.rng);
            }
            uint64_t strong = wired;
            for (int i = 0; i < g.strong_shift; ++i) {
                strong &= fake_random(f.rng);
            }
            const uint64_t five = strong & fake_random(f.rng);
            b.w1[blob_weight_index(n, k)] = strong;
            b.w0[blob_weight_index(n, k)] = (wired & ~strong) | five;
            fan_in += std::popcount(wired);
        }
        b.fan_in[n] = static_cast<uint16_t>(fan_in);
    }
}

// thresholds, leaks and cooldowns drawn per neuron; a few pacemakers; the cycle phase
static void draw_neurons(FakeBlob& f) {
    Blob& b = f.blob;
    const Genome& g = b.genome;
    for (int n = 0; n < b.neuron_count; ++n) {
        b.threshold[n] = static_cast<int16_t>(draw_range(f.rng, g.threshold_min, g.threshold_max));
        b.leak_shift[n] =
            static_cast<uint8_t>(draw_range(f.rng, g.leak_shift_min, g.leak_shift_max));
        b.cooldown_len[n] = static_cast<uint8_t>(draw_range(f.rng, g.cooldown_min, g.cooldown_max));
        if (static_cast<int>(fake_random(f.rng) % 1024) < g.pacemakers_per_1024) {
            b.threshold[n] = 0;
            b.cooldown_len[n] = static_cast<uint8_t>(draw_range(f.rng, 6, 40));
            f.pacemaker[n] = 1;
        }
        const int x = blob_neuron_x(b, n);
        const int y = blob_neuron_y(b, n);
        f.phase[n] = static_cast<uint8_t>((2 * x + y + fake_random(f.rng) % 3) % 24);
    }
}

FakeBlob* fake_blob_birth(const Genome& genome, uint64_t seed, FakeScene scene) {
    FakeBlob* f = new FakeBlob;
    f->scene = scene;
    f->rng = seed * 0x2545f4914f6cdd1dull + 1;
    Blob& b = f->blob;
    b.genome = genome;
    b.tiles_w = genome.tiles_w;
    b.tiles_h = genome.tiles_h;
    b.tile_count = b.tiles_w * b.tiles_h;
    b.neuron_count = b.tile_count * TILE_NEURONS;
    const int st = std::max(1, genome.sector_tiles);
    b.sector_count = ((b.tiles_w + st - 1) / st) * ((b.tiles_h + st - 1) / st);
    allocate(*f);
    place_kinds(*f);
    place_sectors(*f);
    wire_local(*f);
    wire_far(*f);
    wire_weights(*f);
    draw_neurons(*f);
    for (int i = 0; i < 4; ++i) {
        b.rng[i] = fake_random(f->rng);
    }
    return f;
}

void fake_blob_free(FakeBlob* fake) {
    delete fake;
}

void fake_blob_reward(FakeBlob& fake, int valence) {
    if (valence > 0) {
        fake.blob.good_ticks = fake.blob.genome.reward_ticks;
    } else if (valence < 0) {
        fake.blob.bad_ticks = fake.blob.genome.reward_ticks;
    }
}
