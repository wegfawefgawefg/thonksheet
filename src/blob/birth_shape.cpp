#include "blob/birth_shape.hpp"

#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

#include <cstdlib>
#include <cstring>

// shaped wiring at birth: the wire_* genome numbers. every possible local source of a neuron
// gets a weight (a power of two in quarter steps, from geometry and kind only, never from
// what a cell will be used for); the neuron then wires each source with a chance in
// proportion, scaled so its expected count of local connections is today's. birth only:
// the tick reads the same slots and words as ever.

static int clamp_to(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int draw_in(uint64_t* rng, int lo, int hi) {
    return hi <= lo ? lo : lo + (int)rng_below(rng, (uint32_t)(hi - lo + 1));
}

bool wire_shape_on(const Genome& g) {
    return g.wire_layers || g.wire_fine || g.wire_slots || g.wire_mirror_percent ||
           g.wire_hub_per_1024 || g.wire_stream || g.wire_far_topo || g.wire_inhib_reach ||
           g.wire_two_sheets;
}

static void sanitize_shape(Genome& g) {
    g.wire_layers = clamp_to(g.wire_layers, 0, 8);
    g.wire_fine = clamp_to(g.wire_fine, 0, 16);
    g.wire_slots = clamp_to(g.wire_slots, 0, LOCAL_SLOTS);
    g.wire_mirror_percent = clamp_to(g.wire_mirror_percent, 0, 100);
    g.wire_hub_per_1024 = clamp_to(g.wire_hub_per_1024, 0, 1024);
    g.wire_hub_shift = clamp_to(g.wire_hub_shift, 0, 4);
    g.wire_stream = clamp_to(g.wire_stream, -8, 8);
    g.wire_far_topo = clamp_to(g.wire_far_topo, 0, 2);
    g.wire_far_jitter = clamp_to(g.wire_far_jitter, 0, 8);
    g.wire_inhib_reach = clamp_to(g.wire_inhib_reach, -8, 8);
    g.wire_two_sheets = clamp_to(g.wire_two_sheets, 0, 100);
}

// what birth decides once for the whole sheet
struct Shape {
    uint64_t* hub = nullptr;     // one word per tile (tile_count + 1)
    uint32_t* landing = nullptr; // wire_far_topo: per tile per far slot, tile_count if none
    int half = 0;                // wire_two_sheets: first row of the far half
};

// 2^(e/4), e in quarter doublings, clamped to +-12 doublings; 1.0 is 2^20
static uint64_t quarter_pow2(int e) {
    static const uint64_t mant[4] = {256, 304, 362, 431};
    e = clamp_to(e, -48, 48);
    return mant[e & 3] << ((e >> 2) + 12);
}

// a 16-way compass, (cos, sin) x 16
static const int compass_x[16] = {16, 15, 11, 6, 0, -6, -11, -15, -16, -15, -11, -6, 0, 6, 11, 15};
static const int compass_y[16] = {0, 6, 11, 15, 16, 15, 11, 6, 0, -6, -11, -15, -16, -15, -11, -6};

static bool bit_of(const uint64_t* words, int n) {
    return (words[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

static int tile_distance(const Blob& blob, int a, int b) {
    int dx = std::abs(a % blob.tiles_w - b % blob.tiles_w);
    int dy = std::abs(a / blob.tiles_w - b / blob.tiles_w);
    return dx > dy ? dx : dy;
}

static uint32_t tile_at(const Blob& blob, int x, int y) {
    return (uint32_t)(clamp_to(y, 0, blob.tiles_h - 1) * blob.tiles_w +
                      clamp_to(x, 0, blob.tiles_w - 1));
}

static int jitter(Blob& blob, int j) {
    return j > 0 ? (int)rng_below(blob.rng, (uint32_t)(2 * j + 1)) - j : 0;
}

// chance w * target / total for one source, in 2^-32 units, capped below 1
static uint64_t share_of(uint64_t w, uint64_t target_q32, uint64_t total) {
    unsigned __int128 p = (unsigned __int128)w * target_q32 / (total ? total : 1);
    return p > 0xffffffffull ? 0xffffffffull : (uint64_t)p;
}

static void prepare(Blob& blob, Shape& s) {
    const Genome& g = blob.genome;
    s.hub = (uint64_t*)std::calloc((size_t)blob.tile_count + 1, sizeof(uint64_t));
    s.landing = (uint32_t*)std::calloc((size_t)blob.tile_count * FAR_SLOTS, sizeof(uint32_t));
    s.half = blob.tiles_h / 2;
    if (g.wire_hub_per_1024) {
        for (int n = 0; n < blob.neuron_count; n++) {
            if (!bit_of(blob.sensor, n) && (int)rng_below(blob.rng, 1024) < g.wire_hub_per_1024) {
                s.hub[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
            }
        }
    }
    for (int i = 0; i < blob.tile_count * FAR_SLOTS; i++) {
        s.landing[i] = (uint32_t)blob.tile_count;
    }
    if (g.wire_far_topo == 1) {
        // each tile, each far slot: one landing tile for all its neurons
        for (int t = 0; t < blob.tile_count; t++) {
            for (int k = 0; k < FAR_SLOTS; k++) {
                uint8_t delay = 2;
                draw_far_tile(blob, t, &s.landing[t * FAR_SLOTS + k], &delay);
            }
        }
    } else if (g.wire_far_topo == 2) {
        // each sector, each far slot: one shift, so the sector maps onto a shifted copy
        int st = g.sector_tiles;
        for (int sy = 0; sy < blob.tiles_h; sy += st) {
            for (int sx = 0; sx < blob.tiles_w; sx += st) {
                int corner = sy * blob.tiles_w + sx;
                for (int k = 0; k < FAR_SLOTS; k++) {
                    uint32_t to = (uint32_t)blob.tile_count;
                    uint8_t delay = 2;
                    if (!draw_far_tile(blob, corner, &to, &delay)) {
                        continue;
                    }
                    int dx = (int)to % blob.tiles_w - sx, dy = (int)to / blob.tiles_w - sy;
                    for (int ty = sy; ty < sy + st && ty < blob.tiles_h; ty++) {
                        for (int tx = sx; tx < sx + st && tx < blob.tiles_w; tx++) {
                            // off the sheet: the shift turns back on that axis
                            int x = tx + dx, y = ty + dy;
                            x = (x < 0 || x >= blob.tiles_w) ? tx - dx : x;
                            y = (y < 0 || y >= blob.tiles_h) ? ty - dy : y;
                            s.landing[(ty * blob.tiles_w + tx) * FAR_SLOTS + k] =
                                tile_at(blob, x, y);
                        }
                    }
                }
            }
        }
    }
}

// which local slots a neuron may use: on the sheet, on its own half, and a drawn subset
static uint32_t usable_slots(Blob& blob, const Shape& s, int t) {
    const Genome& g = blob.genome;
    int ty = t / blob.tiles_w;
    uint32_t usable = 0;
    int others[LOCAL_SLOTS], other_count = 0;
    for (int k = 0; k < LOCAL_SLOTS; k++) {
        uint32_t st = blob.local_tile[t * LOCAL_SLOTS + k];
        if (st == (uint32_t)blob.tile_count) {
            continue;
        }
        if (g.wire_two_sheets && ((int)st / blob.tiles_w >= s.half) != (ty >= s.half)) {
            continue;
        }
        usable |= 1u << k;
        if (k != 4) {
            others[other_count++] = k;
        }
    }
    if (g.wire_slots == 0 || g.wire_slots - 1 >= other_count) {
        return usable;
    }
    // own tile plus wire_slots - 1 others (a partial shuffle)
    uint32_t chosen = usable & (1u << 4);
    for (int i = 0; i < g.wire_slots - 1; i++) {
        int j = i + (int)rng_below(blob.rng, (uint32_t)(other_count - i));
        int k = others[j];
        others[j] = others[i];
        others[i] = k;
        chosen |= 1u << k;
    }
    return chosen;
}

// the exponent (quarter doublings) for source m heard by neuron n through local slot k
static int source_weight(const Blob& blob, const Shape& s, int n, int m, int k) {
    const Genome& g = blob.genome;
    int e = 0;
    int dx = blob_neuron_x(blob, m) - blob_neuron_x(blob, n);
    int dy = blob_neuron_y(blob, m) - blob_neuron_y(blob, n);
    if (g.wire_layers && wire_layer_favoured(wire_group(blob, m), wire_group(blob, n), k == 4)) {
        e += 4 * g.wire_layers;
    }
    if (g.wire_fine) {
        int ax = std::abs(dx), ay = std::abs(dy);
        int d = ax > ay ? ax + ay / 2 : ay + ax / 2;
        e -= g.wire_fine * d;
    }
    if (g.wire_stream) {
        int turn = blob_neuron_x(blob, n) * g.wire_stream_turn / (blob.tiles_w * 8);
        int dir = (g.wire_stream_dir + turn) & 15;
        int along = compass_x[dir] * dx + compass_y[dir] * dy; // cells x 16
        e -= g.wire_stream * along / 32;                       // upstream sources likelier
    }
    if (bit_of(s.hub, m)) {
        e += 4 * g.wire_hub_shift;
    }
    if (g.wire_inhib_reach > 0 && bit_of(blob.inhibitory, n) && k == 4) {
        e -= 4 * g.wire_inhib_reach;
    }
    if (g.wire_inhib_reach < 0) {
        if (bit_of(blob.inhibitory, n) && k == 4) {
            e -= 4 * g.wire_inhib_reach;
        }
        if (bit_of(blob.inhibitory, m) && k != 4) {
            e += 4 * g.wire_inhib_reach;
        }
    }
    return e;
}

static void wire_local(Blob& blob, const Shape& s, int n) {
    const Genome& g = blob.genome;
    int t = n / TILE_NEURONS;
    uint32_t usable = usable_slots(blob, s, t);
    uint64_t weight[LOCAL_SLOTS * TILE_NEURONS];
    uint64_t total = 0;
    int on_sheet = 0;
    for (int k = 0; k < LOCAL_SLOTS; k++) {
        uint32_t st = blob.local_tile[t * LOCAL_SLOTS + k];
        if (st == (uint32_t)blob.tile_count) {
            continue;
        }
        on_sheet += k == 4 ? TILE_NEURONS - 1 : TILE_NEURONS;
        if (!((usable >> k) & 1)) {
            continue;
        }
        for (int b = 0; b < TILE_NEURONS; b++) {
            int m = (int)st * TILE_NEURONS + b;
            uint64_t w = m == n ? 0 : quarter_pow2(source_weight(blob, s, n, m, k));
            weight[k * TILE_NEURONS + b] = w;
            total += w;
        }
    }
    // today's expected count; a hub listens to 2^hub_shift times as many
    uint64_t target = ((uint64_t)on_sheet << 32) >> clamp_to(g.local_connect_shift, 0, 32);
    if (bit_of(s.hub, n)) {
        target <<= g.wire_hub_shift;
    }
    for (int k = 0; k < LOCAL_SLOTS; k++) {
        int64_t wi = blob_weight_index(n, k);
        blob.w0[wi] = blob.w1[wi] = 0;
        if (!((usable >> k) & 1)) {
            continue;
        }
        uint64_t conn = 0;
        for (int b = 0; b < TILE_NEURONS; b++) {
            uint64_t p = share_of(weight[k * TILE_NEURONS + b], target, total);
            if ((rng_next(blob.rng) >> 32) < p) {
                conn |= 1ull << b;
            }
        }
        uint64_t strong = conn & sparse_word(blob.rng, g.strong_shift);
        blob.w0[wi] = conn & ~strong;
        blob.w1[wi] = strong;
    }
}

// a far slot's sources: excitatory cells of the far tile; with layers, its out group likelier
static uint64_t far_mask(Blob& blob, uint32_t src) {
    const Genome& g = blob.genome;
    uint64_t senders = ~blob.inhibitory[src];
    if (!g.wire_layers) {
        return senders & sparse_word(blob.rng, g.far_connect_shift);
    }
    uint64_t weight[TILE_NEURONS], total = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        bool out = wire_group(blob, (int)src * TILE_NEURONS + b) == GROUP_OUT;
        weight[b] = ((senders >> b) & 1) ? quarter_pow2(out ? 4 * g.wire_layers : 0) : 0;
        total += weight[b];
    }
    uint64_t target =
        ((uint64_t)__builtin_popcountll(senders) << 32) >> clamp_to(g.far_connect_shift, 0, 32);
    uint64_t mask = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        if ((rng_next(blob.rng) >> 32) < share_of(weight[b], target, total)) {
            mask |= 1ull << b;
        }
    }
    return mask;
}

// a far draw that stays on one half of the sheet (wire_two_sheets)
static bool draw_far_in_half(Blob& blob, const Shape& s, int t, bool far_half, uint32_t* src) {
    for (int attempt = 0; attempt < 8; attempt++) {
        uint8_t delay = 2;
        if (draw_far_tile(blob, t, src, &delay) &&
            ((int)*src / blob.tiles_w >= s.half) == far_half) {
            return true;
        }
    }
    return false;
}

// which tile far slot k of neuron n listens to, or false for none
static bool far_source(Blob& blob, const Shape& s, int n, int k, uint32_t* src) {
    const Genome& g = blob.genome;
    int t = n / TILE_NEURONS, tx = t % blob.tiles_w, ty = t / blob.tiles_w;
    bool excitatory = !bit_of(blob.inhibitory, n);
    if (g.wire_two_sheets) {
        bool far_half = ty >= s.half;
        if (far_half && (int)rng_below(blob.rng, 100) < g.wire_two_sheets) {
            // fed forward from the same place on the near half
            int y = clamp_to(ty - s.half + jitter(blob, 1), 0, s.half - 1);
            *src = tile_at(blob, tx + jitter(blob, 1), y);
            return true;
        }
        if (!excitatory || (int)rng_below(blob.rng, 100) >= g.far_percent) {
            return false;
        }
        if (!far_half && rng_below(blob.rng, 8) == 0) {
            // sent back from the same place on the far half
            int y = clamp_to(ty + s.half + jitter(blob, 1), s.half, blob.tiles_h - 1);
            *src = tile_at(blob, tx + jitter(blob, 1), y);
            return true;
        }
        return draw_far_in_half(blob, s, t, far_half, src);
    }
    if (!excitatory || (int)rng_below(blob.rng, 100) >= g.far_percent) {
        return false;
    }
    uint32_t land = s.landing[t * FAR_SLOTS + k];
    if (g.wire_far_topo && land != (uint32_t)blob.tile_count) {
        int j = g.wire_far_jitter;
        *src = tile_at(blob, (int)land % blob.tiles_w + jitter(blob, j),
                       (int)land / blob.tiles_w + jitter(blob, j));
        return true;
    }
    uint8_t delay = 2;
    return draw_far_tile(blob, t, src, &delay);
}

static int fan_in_cap(const Blob& blob, const Shape& s, int n) {
    int cap = blob.genome.fan_in_max;
    if (bit_of(s.hub, n)) {
        cap <<= blob.genome.wire_hub_shift;
    }
    return cap < SLOTS * TILE_NEURONS ? cap : SLOTS * TILE_NEURONS;
}

// where A listens to B, B adds A with some chance; then B drops connections that are not
// returned until it has its old count, so only reciprocity changes
static void mirror_links(Blob& blob) {
    const Genome& g = blob.genome;
    for (int n = 0; n < blob.neuron_count; n++) {
        int t = n / TILE_NEURONS;
        uint64_t me = 1ull << (n % TILE_NEURONS);
        int before = count_fan_in(blob, n);
        uint64_t returned[LOCAL_SLOTS] = {}, added[LOCAL_SLOTS] = {};
        for (int k = 0; k < LOCAL_SLOTS; k++) {
            uint32_t st = blob.local_tile[t * LOCAL_SLOTS + k];
            if (st == (uint32_t)blob.tile_count) {
                continue;
            }
            // the cells of the source tile that listen to n: their slot 8 - k is n's tile
            uint64_t listen = 0;
            for (int b = 0; b < TILE_NEURONS; b++) {
                int64_t w = blob_weight_index((int)st * TILE_NEURONS + b, LOCAL_SLOTS - 1 - k);
                if (connected_bits(blob.w0[w], blob.w1[w]) & me) {
                    listen |= 1ull << b;
                }
            }
            if (k == 4) {
                listen &= ~me;
            }
            int64_t wi = blob_weight_index(n, k);
            uint64_t missing = listen & ~connected_bits(blob.w0[wi], blob.w1[wi]);
            uint64_t add = 0;
            while (missing) {
                uint64_t bit = missing & -missing;
                missing &= missing - 1;
                if ((int)rng_below(blob.rng, 100) < g.wire_mirror_percent) {
                    add |= bit;
                }
            }
            blob.w0[wi] |= add;
            added[k] = add;
            returned[k] = listen;
        }
        // back to the old count, dropping only links nobody returns
        int count = count_fan_in(blob, n);
        for (int guard = 0; count > before && guard < 100000; guard++) {
            int k = (int)rng_below(blob.rng, LOCAL_SLOTS);
            int64_t wi = blob_weight_index(n, k);
            uint64_t drop = connected_bits(blob.w0[wi], blob.w1[wi]) & ~returned[k] & ~added[k];
            if (!drop) {
                continue;
            }
            uint64_t bit = 1ull << pick_bit(blob.rng, drop);
            blob.w0[wi] &= ~bit;
            blob.w1[wi] &= ~bit;
            count--;
        }
        blob.fan_in[n] = (uint16_t)count;
    }
}

void wire_shaped(Blob& blob) {
    sanitize_shape(blob.genome);
    const Genome& g = blob.genome;
    Shape s;
    prepare(blob, s);
    for (int n = 0; n < blob.neuron_count; n++) {
        // local delays as today: the smaller of two draws, so short ones are common
        for (int k = 0; k < LOCAL_SLOTS; k++) {
            int d1 = draw_in(blob.rng, g.local_delay_min, g.local_delay_max);
            int d2 = draw_in(blob.rng, g.local_delay_min, g.local_delay_max);
            blob.slot_delay[blob_weight_index(n, k)] = (uint8_t)(d1 < d2 ? d1 : d2);
        }
        wire_local(blob, s, n);
        for (int k = 0; k < FAR_SLOTS; k++) {
            int f = n * FAR_SLOTS + k;
            int64_t wi = blob_weight_index(n, LOCAL_SLOTS + k);
            blob.far_tile[f] = (uint32_t)blob.tile_count;
            blob.slot_delay[wi] = 2;
            uint32_t src = 0;
            if (!far_source(blob, s, n, k, &src)) {
                continue;
            }
            blob.far_tile[f] = src;
            int d = tile_distance(blob, n / TILE_NEURONS, (int)src);
            blob.slot_delay[wi] = (uint8_t)clamp_to(d * g.far_delay_per_tile, 2, HISTORY - 1);
            blob.w0[wi] = far_mask(blob, src);
        }
        enforce_fan_in(blob, n, fan_in_cap(blob, s, n));
    }
    if (g.wire_mirror_percent) {
        mirror_links(blob);
    }
    std::free(s.hub);
    std::free(s.landing);
}
