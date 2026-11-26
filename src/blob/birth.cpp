#include "blob/birth_shape.hpp"
#include "blob/blob.hpp"
#include "blob/fire_dynamics.hpp"
#include "blob/long_memory.hpp"
#include "blob/critic.hpp"
#include "blob/random_words.hpp"
#include "blob/regulate_rate.hpp"
#include "blob/tag_store.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

#include <cstdlib>
#include <cstring>

// building a blob from a genome and a seed, and freeing it.

// zeroed memory, 64-byte aligned, rounded up to whole cache lines
static void* alloc_lines(size_t bytes) {
    size_t rounded = (bytes + 63) & ~(size_t)63;
    if (rounded == 0) {
        rounded = 64;
    }
    void* p = std::aligned_alloc(64, rounded);
    std::memset(p, 0, rounded);
    return p;
}

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// a value in [lo, hi]
static int draw_between(uint64_t* rng, int lo, int hi) {
    if (hi <= lo) {
        return lo;
    }
    return lo + (int)rng_below(rng, (uint32_t)(hi - lo + 1));
}

// keep the genome inside what the data layout can hold
static void sanitize_genome(Genome& g) {
    g.tiles_w = clamp_int(g.tiles_w, 1, 4096);
    g.tiles_h = clamp_int(g.tiles_h, 1, 4096);
    g.inhibitory_percent = clamp_int(g.inhibitory_percent, 0, 100);
    g.threshold_min = clamp_int(g.threshold_min, 0, 1000);
    g.threshold_max = clamp_int(g.threshold_max, g.threshold_min, 1000);
    g.leak_shift_min = clamp_int(g.leak_shift_min, 1, 15);
    g.leak_shift_max = clamp_int(g.leak_shift_max, g.leak_shift_min, 15);
    g.cooldown_min = clamp_int(g.cooldown_min, 0, 255);
    g.cooldown_max = clamp_int(g.cooldown_max, g.cooldown_min, 255);
    g.pacemaker_period_min = clamp_int(g.pacemaker_period_min, 1, 256);
    g.pacemaker_period_max = clamp_int(g.pacemaker_period_max, g.pacemaker_period_min, 256);
    g.far_reach_tiles = clamp_int(g.far_reach_tiles, 1, 64);
    g.sensor_stride = clamp_int(g.sensor_stride, 1, 1 << 20);
    g.motor_stride = clamp_int(g.motor_stride, 1, 1 << 20);
    g.local_delay_min = clamp_int(g.local_delay_min, 1, HISTORY - 1);
    g.local_delay_max = clamp_int(g.local_delay_max, g.local_delay_min, HISTORY - 1);
    g.fan_in_max = clamp_int(g.fan_in_max, 1, SLOTS * TILE_NEURONS);
    g.sector_tiles = clamp_int(g.sector_tiles, 1, 4096);
    g.recent_ticks = clamp_int(g.recent_ticks, 1, 8);
    g.reward_ticks = clamp_int(g.reward_ticks, 1, 1000);
    g.sector_smooth_shift = clamp_int(g.sector_smooth_shift, 0, 12);
    g.threshold_cap = clamp_int(g.threshold_cap, g.threshold_max, 30000);
    g.strong_log = clamp_int(g.strong_log, 0, 4);
    g.flip_count = clamp_int(g.flip_count, 1, FLIP_MAX);
    g.flip_window_ticks = clamp_int(g.flip_window_ticks, 1, 1 << 30);
    g.flip_average_shift = clamp_int(g.flip_average_shift, 0, 16);
    g.noise_trace_ticks = clamp_int(g.noise_trace_ticks, 1, HISTORY);
    g.reward_average_shift = clamp_int(g.reward_average_shift, 0, 16);
}

int count_fan_in(const Blob& blob, int neuron) {
    int count = 0;
    for (int k = 0; k < SLOTS; k++) {
        int64_t w = blob_weight_index(neuron, k);
        count += __builtin_popcountll(connected_bits(blob.w0[w], blob.w1[w]));
    }
    return count;
}

bool draw_far_tile(Blob& blob, int tile, uint32_t* far_tile, uint8_t* far_delay) {
    const Genome& g = blob.genome;
    int tx = tile % blob.tiles_w, ty = tile / blob.tiles_w;
    int reach = g.far_reach_tiles;
    int longest = blob.tiles_w > blob.tiles_h ? blob.tiles_w : blob.tiles_h;
    for (int attempt = 0; attempt < 8; attempt++) {
        // distance: mostly near reach, with a long tail
        int d = 2 + (int)rng_below(blob.rng, (uint32_t)reach);
        while ((rng_next(blob.rng) & 3) == 0 && d < longest) {
            d += 1 + (int)rng_below(blob.rng, (uint32_t)reach);
        }
        // a point on the square ring at that distance
        int side = (int)rng_below(blob.rng, 4);
        int p = (int)rng_below(blob.rng, (uint32_t)(2 * d)) - d;
        int dx = 0, dy = 0;
        if (side == 0) {
            dx = p, dy = -d;
        } else if (side == 1) {
            dx = d, dy = p;
        } else if (side == 2) {
            dx = -p, dy = d;
        } else {
            dx = -d, dy = -p;
        }
        int x = tx + dx, y = ty + dy;
        if (x < 0 || y < 0 || x >= blob.tiles_w || y >= blob.tiles_h) {
            continue;
        }
        *far_tile = (uint32_t)(y * blob.tiles_w + x);
        *far_delay = (uint8_t)clamp_int(d * g.far_delay_per_tile, 2, HISTORY - 1);
        return true;
    }
    return false;
}

static void set_kinds(Blob& blob) {
    const Genome& g = blob.genome;
    for (int n = 0; n < blob.neuron_count; n++) {
        int x = blob_neuron_x(blob, n), y = blob_neuron_y(blob, n);
        int t = n / TILE_NEURONS;
        uint64_t bit = 1ull << (n % TILE_NEURONS);
        bool in_sensor_rect = x >= g.sensor_x && x < g.sensor_x + g.sensor_w && y >= g.sensor_y &&
                              y < g.sensor_y + g.sensor_h;
        bool is_sensor = in_sensor_rect && (x - g.sensor_x) % g.sensor_stride == 0 &&
                         (y - g.sensor_y) % g.sensor_stride == 0;
        bool in_motor_rect = x >= g.motor_x && x < g.motor_x + g.motor_w && y >= g.motor_y &&
                             y < g.motor_y + g.motor_h;
        bool is_motor = in_motor_rect && (x - g.motor_x) % g.motor_stride == 0 &&
                        (y - g.motor_y) % g.motor_stride == 0;
        if (is_sensor) {
            blob.sensor[t] |= bit;
        }
        if (is_motor && !is_sensor) {
            blob.motor[t] |= bit;
        }
        bool inhib = !is_sensor && (int)rng_below(blob.rng, 100) < g.inhibitory_percent;
        if (inhib) {
            blob.inhibitory[t] |= bit;
        }
        bool pace = !is_sensor && !inhib && (int)rng_below(blob.rng, 1024) < g.pacemakers_per_1024;
        if (pace) {
            blob.pacemaker[t] |= bit;
        }

        blob.threshold[n] = (int16_t)draw_between(blob.rng, g.threshold_min, g.threshold_max);
        blob.leak_shift[n] = (uint8_t)draw_between(blob.rng, g.leak_shift_min, g.leak_shift_max);
        blob.cooldown_len[n] = (uint8_t)draw_between(blob.rng, g.cooldown_min, g.cooldown_max);
        if (pace) {
            blob.threshold[n] = 0;
            int period = draw_between(blob.rng, g.pacemaker_period_min, g.pacemaker_period_max);
            blob.cooldown_len[n] = (uint8_t)clamp_int(period - 1, 0, 255);
            blob.cooldown[n] = (uint8_t)rng_below(blob.rng, (uint32_t)period);
        }
        blob.potential[n] = (int16_t)rng_below(blob.rng, (uint32_t)blob.threshold[n] + 1);
    }
}

static void set_local_tiles(Blob& blob) {
    for (int t = 0; t < blob.tile_count; t++) {
        int tx = t % blob.tiles_w, ty = t / blob.tiles_w;
        for (int k = 0; k < LOCAL_SLOTS; k++) {
            int x = tx + k % 3 - 1, y = ty + k / 3 - 1;
            bool inside = x >= 0 && y >= 0 && x < blob.tiles_w && y < blob.tiles_h;
            blob.local_tile[t * LOCAL_SLOTS + k] =
                inside ? (uint32_t)(y * blob.tiles_w + x) : (uint32_t)blob.tile_count;
        }
    }
}

// remove random connections until the neuron is under the cap
void enforce_fan_in(Blob& blob, int n, int cap) {
    int count = count_fan_in(blob, n);
    while (count > cap) {
        int k = (int)rng_below(blob.rng, SLOTS);
        int64_t w = blob_weight_index(n, k);
        uint64_t conn = connected_bits(blob.w0[w], blob.w1[w]);
        if (conn == 0) {
            continue;
        }
        uint64_t bit = 1ull << pick_bit(blob.rng, conn);
        blob.w0[w] &= ~bit;
        blob.w1[w] &= ~bit;
        count--;
    }
    blob.fan_in[n] = (uint16_t)count;
}

// wire_pixel_shift: a hidden excitatory cell also wires the sensor cells of local slot k
// with chance 2^-shift, strong as usual (docs/results-2026-11-25-hop-real.md)
static void more_pixels(Blob& blob, int n, int k) {
    const Genome& g = blob.genome;
    int t = n / TILE_NEURONS;
    uint64_t bit = 1ull << (n % TILE_NEURONS);
    uint32_t src = blob.local_tile[t * LOCAL_SLOTS + k];
    if ((blob.sensor[t] | blob.motor[t] | blob.pacemaker[t] | blob.inhibitory[t]) & bit) {
        return;
    }
    int64_t w = blob_weight_index(n, k);
    uint64_t extra =
        blob.sensor[src] & sparse_word(blob.rng, g.wire_pixel_shift) & ~(blob.w0[w] | blob.w1[w]);
    uint64_t strong = extra & sparse_word(blob.rng, g.strong_shift);
    blob.w0[w] |= extra & ~strong;
    blob.w1[w] |= strong;
}

static void wire(Blob& blob) {
    const Genome& g = blob.genome;
    for (int n = 0; n < blob.neuron_count; n++) {
        int t = n / TILE_NEURONS;
        uint64_t self = 1ull << (n % TILE_NEURONS);
        // local slots: a delay each (the smaller of two draws, so short ones are common)
        for (int k = 0; k < LOCAL_SLOTS; k++) {
            int d1 = draw_between(blob.rng, g.local_delay_min, g.local_delay_max);
            int d2 = draw_between(blob.rng, g.local_delay_min, g.local_delay_max);
            blob.slot_delay[blob_weight_index(n, k)] = (uint8_t)(d1 < d2 ? d1 : d2);
            if (blob.local_tile[t * LOCAL_SLOTS + k] == (uint32_t)blob.tile_count) {
                continue;
            }
            uint64_t conn = sparse_word(blob.rng, g.local_connect_shift);
            if (k == 4) {
                conn &= ~self;
            }
            uint64_t strong = conn & sparse_word(blob.rng, g.strong_shift);
            blob.w0[blob_weight_index(n, k)] = conn & ~strong;
            blob.w1[blob_weight_index(n, k)] = strong;
            if (g.wire_pixel_shift > 0) {
                more_pixels(blob, n, k);
            }
        }
        // far slots: some excitatory neurons, excitatory sources only, all weak
        bool excitatory = !((blob.inhibitory[t] >> (n % TILE_NEURONS)) & 1);
        for (int k = 0; k < FAR_SLOTS; k++) {
            int f = n * FAR_SLOTS + k;
            uint8_t* delay = &blob.slot_delay[blob_weight_index(n, LOCAL_SLOTS + k)];
            blob.far_tile[f] = (uint32_t)blob.tile_count;
            *delay = 2;
            if (!excitatory || (int)rng_below(blob.rng, 100) >= g.far_percent) {
                continue;
            }
            if (!draw_far_tile(blob, t, &blob.far_tile[f], delay)) {
                continue;
            }
            uint64_t senders = ~blob.inhibitory[blob.far_tile[f]];
            blob.w0[blob_weight_index(n, LOCAL_SLOTS + k)] =
                senders & sparse_word(blob.rng, g.far_connect_shift);
        }
        for (int k = 0; g.wire_deep_percent > 0 && k < SLOTS; k++) {
            uint64_t off = deep_blocked(blob, n, slot_source(blob, n, k));
            blob.w0[blob_weight_index(n, k)] &= ~off;
            blob.w1[blob_weight_index(n, k)] &= ~off;
        }
        enforce_fan_in(blob, n, g.fan_in_max);
    }
}

static void set_sectors(Blob& blob) {
    const Genome& g = blob.genome;
    int sw = (blob.tiles_w + g.sector_tiles - 1) / g.sector_tiles;
    int sh = (blob.tiles_h + g.sector_tiles - 1) / g.sector_tiles;
    blob.sector_count = sw * sh;
    blob.tile_sector = (uint32_t*)alloc_lines(sizeof(uint32_t) * (blob.tile_count + 1));
    blob.sector_bias = (int16_t*)alloc_lines(sizeof(int16_t) * blob.sector_count);
    blob.sector_activity = (uint32_t*)alloc_lines(sizeof(uint32_t) * blob.sector_count);
    blob.sector_surprise = (uint32_t*)alloc_lines(sizeof(uint32_t) * blob.sector_count);
    blob.sector_target = (uint32_t*)alloc_lines(sizeof(uint32_t) * blob.sector_count);
    for (int t = 0; t < blob.tile_count; t++) {
        int tx = t % blob.tiles_w, ty = t / blob.tiles_w;
        int s = (ty / g.sector_tiles) * sw + tx / g.sector_tiles;
        blob.tile_sector[t] = (uint32_t)s;
        // sensor neurons fire only from the world, so they do not count toward the target
        int free_neurons = TILE_NEURONS - __builtin_popcountll(blob.sensor[t]);
        blob.sector_target[s] += (uint32_t)(free_neurons * g.sector_target_per_1024 * 256 / 1024);
    }
    for (int s = 0; s < blob.sector_count; s++) {
        blob.sector_activity[s] = blob.sector_target[s];
    }
}

Blob* blob_birth(const Genome& genome, uint64_t seed) {
    Blob* blob = new Blob{};
    blob->genome = genome;
    sanitize_genome(blob->genome);
    rng_seed(blob->rng, seed);

    blob->tiles_w = blob->genome.tiles_w;
    blob->tiles_h = blob->genome.tiles_h;
    blob->tile_count = blob->tiles_w * blob->tiles_h;
    blob->neuron_count = blob->tile_count * TILE_NEURONS;
    size_t map_bytes = sizeof(uint64_t) * (blob->tile_count + 1);
    size_t n = (size_t)blob->neuron_count;

    // the ring is one block so a delay is just an offset; maps start on cache lines
    blob->spike_stride = (blob->tile_count + 1 + 7) & ~7;
    uint64_t* ring = (uint64_t*)alloc_lines(sizeof(uint64_t) * blob->spike_stride * HISTORY);
    for (int h = 0; h < HISTORY; h++) {
        blob->spikes[h] = ring + (size_t)h * blob->spike_stride;
    }
    blob->recent = (uint64_t*)alloc_lines(map_bytes);
    blob->noise_ring = (uint64_t*)alloc_lines(sizeof(uint64_t) * blob->spike_stride * HISTORY);
    blob->noise_trace = (uint64_t*)alloc_lines(map_bytes);
    blob->active_trace = (uint64_t*)alloc_lines(map_bytes);
    blob->motor_attached = (uint64_t*)alloc_lines(map_bytes);
    blob->motor_acted = (uint64_t*)alloc_lines(map_bytes);
    blob->readout_last = (uint64_t*)alloc_lines(map_bytes * HISTORY);
    blob->inhibitory = (uint64_t*)alloc_lines(map_bytes);
    blob->sensor = (uint64_t*)alloc_lines(map_bytes);
    blob->motor = (uint64_t*)alloc_lines(map_bytes);
    blob->pacemaker = (uint64_t*)alloc_lines(map_bytes);
    blob->sense_in = (uint64_t*)alloc_lines(map_bytes);
    blob->sense_attached = (uint64_t*)alloc_lines(map_bytes);
    blob->predicted = (uint64_t*)alloc_lines(map_bytes);
    blob->urge = (uint64_t*)alloc_lines(map_bytes);
    blob->local_tile = (uint32_t*)alloc_lines(sizeof(uint32_t) * blob->tile_count * LOCAL_SLOTS);
    blob->far_tile = (uint32_t*)alloc_lines(sizeof(uint32_t) * n * FAR_SLOTS);
    blob->slot_delay = (uint8_t*)alloc_lines(n * SLOTS);
    blob->w0 = (uint64_t*)alloc_lines(sizeof(uint64_t) * n * SLOTS);
    blob->w1 = (uint64_t*)alloc_lines(sizeof(uint64_t) * n * SLOTS);
    blob->potential = (int16_t*)alloc_lines(sizeof(int16_t) * n);
    blob->threshold = (int16_t*)alloc_lines(sizeof(int16_t) * n);
    blob->leak_shift = (uint8_t*)alloc_lines(n);
    blob->cooldown = (uint8_t*)alloc_lines(n);
    blob->cooldown_len = (uint8_t*)alloc_lines(n);
    blob->fan_in = (uint16_t*)alloc_lines(sizeof(uint16_t) * n);

    set_kinds(*blob);
    set_local_tiles(*blob);
    // shaped wiring (birth_shape.cpp) only when one of its numbers is set, so a default
    // genome draws exactly what it always drew
    if (wire_shape_on(blob->genome)) {
        wire_shaped(*blob);
    } else {
        wire(*blob);
    }
    set_sectors(*blob);
    dynamics_birth(*blob, seed);
    return blob;
}

void blob_free(Blob* blob) {
    if (!blob) {
        return;
    }
    critic_free(*blob);
    std::free(blob->spikes[0]); // the whole ring
    void* arrays[] = {
        blob->recent,      blob->inhibitory,      blob->sensor,          blob->motor,
        blob->pacemaker,   blob->sense_in,        blob->sense_attached,  blob->predicted,
        blob->local_tile,  blob->far_tile,        blob->slot_delay,      blob->w0,
        blob->w1,          blob->potential,       blob->threshold,       blob->leak_shift,
        blob->cooldown,    blob->cooldown_len,    blob->fan_in,          blob->tile_sector,
        blob->sector_bias, blob->sector_activity, blob->sector_surprise, blob->sector_target,
        blob->noise_ring,  blob->noise_trace,     blob->active_trace,    blob->urge,
        blob->motor_attached, blob->motor_acted, blob->readout_last,
        blob->readout_row,    blob->readout_evidence, blob->readout_keep,
        blob->motor_runner,   blob->tag_plus,     blob->tag_minus,    blob->tag_feedback,
        blob->tag_evidence,   blob->tag_keep,     blob->tag_strong,   blob->tag_made,
        blob->tag_scratch,
        blob->rule_words,
        blob->sent,           blob->warm,             blob->tire,
        blob->slow,           blob->rebound,          blob->up,
        blob->any_branch,     blob->veto,             blob->coincide,
    };
    for (void* p : arrays) {
        std::free(p);
    }
    std::free(blob->replay); // one block (blob/replay.hpp)
    hop_free(*blob);
    compete_free(*blob);
    rate_free(*blob);
    long_memory_free(*blob);
    delete blob;
}

void blob_reward(Blob& blob, int valence) {
    // critic_on: every step lands here, and the critic's surprise replaces the valence
    // with critic_keep_world the world's verdict still goes on below as without the critic
    if (critic_step(blob, valence) && !blob.genome.critic_keep_world) {
        return;
    }
    if (valence > 0) {
        blob.good_ticks = blob.genome.reward_ticks;
    } else if (valence < 0) {
        blob.bad_ticks = blob.genome.reward_ticks;
    }
    // the rival modes read these; mode 0 ignores them
    blob.flips.score += valence;
    blob.pending_valence = valence;
}
