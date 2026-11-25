// frame_fill: a ViewFrame from a Blob, using only the fields in blob.hpp. Runs on the sim
// thread while the blob is between ticks.

#include "blob/blob.hpp"
#include "engine.hpp"
#include "view_frame.hpp"

#include <algorithm>
#include <bit>
#include <cstdlib>

static void hist_clear(Histogram& h, int first, int width) {
    h = Histogram{};
    h.first = first;
    h.width = std::max(1, width);
    h.low = 1 << 30;
    h.high = -(1 << 30);
}

static void hist_add(Histogram& h, int value, uint32_t count = 1) {
    const int bin = std::clamp((value - h.first) / h.width, 0, HIST_BINS - 1);
    h.counts[bin] += count;
    h.total += count;
    h.low = std::min(h.low, value);
    h.high = std::max(h.high, value);
}

// what every engine's shell has: the sheet's size and the kinds of cell. sectors only where
// the engine keeps them in the shell (the full engine and the fake)
static void fill_shape(ViewFrame& f, const Blob& b) {
    f.tiles_w = b.tiles_w;
    f.tiles_h = b.tiles_h;
    f.tile_count = b.tile_count;
    f.neuron_count = b.neuron_count;
    f.tick = b.stats.tick;
    f.inhibitory.assign(b.inhibitory, b.inhibitory + b.tile_count);
    f.sensor.assign(b.sensor, b.sensor + b.tile_count);
    f.motor.assign(b.motor, b.motor + b.tile_count);
    const bool sectors = b.tile_sector != nullptr && b.sector_activity != nullptr;
    f.sector_count = sectors ? b.sector_count : 0;
    f.sector_tiles = sectors ? b.genome.sector_tiles : 1;
    if (sectors) {
        f.tile_sector.assign(b.tile_sector, b.tile_sector + b.tile_count);
    } else {
        f.tile_sector.assign(b.tile_count, 0);
    }
}

// activity against target per sector; the target is the genome's rate times the sector's
// neurons, in the same units as sector_activity (spikes per tick)
static void fill_sectors(ViewFrame& f, const Blob& b) {
    f.sectors.assign(b.sector_count, SectorView{});
    for (int t = 0; t < b.tile_count; ++t) {
        f.sectors[b.tile_sector[t]].tiles += 1;
    }
    for (int s = 0; s < b.sector_count; ++s) {
        SectorView& v = f.sectors[s];
        v.activity = b.sector_activity[s] / 256.0f;
        v.surprise = b.sector_surprise[s] / 256.0f;
        v.bias = b.sector_bias[s];
        v.target = b.genome.sector_target_per_1024 * v.tiles * TILE_NEURONS / 1024.0f;
    }
    f.good_ticks = b.good_ticks;
    f.bad_ticks = b.bad_ticks;
    f.surprise_high = b.surprise_high;
}

static int slot_source(const Blob& b, int neuron, int slot) {
    if (slot < LOCAL_SLOTS) {
        return static_cast<int>(b.local_tile[(neuron / TILE_NEURONS) * LOCAL_SLOTS + slot]);
    }
    return static_cast<int>(b.far_tile[neuron * FAR_SLOTS + slot - LOCAL_SLOTS]);
}

// whole-blob histograms: weight levels, thresholds, fan-in, and connection lengths (the
// lengths from a sample of at most 4,096 neurons, so big sheets stay cheap)
static void fill_histograms(ViewFrame& f, const Blob& b) {
    for (uint64_t& w : f.weight_levels) {
        w = 0;
    }
    f.local_links = 0;
    f.far_links = 0;
    int tmin = 1 << 30, tmax = -(1 << 30);
    for (int n = 0; n < b.neuron_count; ++n) {
        tmin = std::min(tmin, static_cast<int>(b.threshold[n]));
        tmax = std::max(tmax, static_cast<int>(b.threshold[n]));
    }
    hist_clear(f.threshold, tmin, (tmax - tmin) / HIST_BINS + 1);
    hist_clear(f.fan_in, 0, (b.genome.fan_in_max + HIST_BINS) / HIST_BINS);
    hist_clear(f.length, 0, 1);
    const int stride = std::max(1, b.neuron_count / 4096);
    for (int n = 0; n < b.neuron_count; ++n) {
        hist_add(f.threshold, b.threshold[n]);
        hist_add(f.fan_in, b.fan_in[n]);
        const int x = blob_neuron_x(b, n);
        const int y = blob_neuron_y(b, n);
        for (int k = 0; k < SLOTS; ++k) {
            const int src = slot_source(b, n, k);
            if (src >= b.tile_count) {
                continue;
            }
            const uint64_t w0 = b.w0[blob_weight_index(n, k)];
            const uint64_t w1 = b.w1[blob_weight_index(n, k)];
            const uint64_t any = w0 | w1;
            const int connected = std::popcount(any);
            f.weight_levels[0] += TILE_NEURONS - connected;
            f.weight_levels[1] += std::popcount(w0 & ~w1);
            f.weight_levels[2] += std::popcount(~w0 & w1);
            f.weight_levels[3] += std::popcount(w0 & w1);
            (k < LOCAL_SLOTS ? f.local_links : f.far_links) += connected;
            if (n % stride != 0) {
                continue;
            }
            // length in cells, the larger of the two offsets
            for (uint64_t bits = any; bits != 0; bits &= bits - 1) {
                const int source = src * TILE_NEURONS + std::countr_zero(bits);
                const int dx = std::abs(blob_neuron_x(b, source) - x);
                const int dy = std::abs(blob_neuron_y(b, source) - y);
                hist_add(f.length, std::max(dx, dy));
            }
        }
    }
}

// the one place a slot's delay is read
static int slot_delay(const Blob& b, int neuron, int slot) {
    return b.slot_delay[blob_weight_index(neuron, slot)];
}

Link make_link(const Blob& b, int neuron, int strength, bool far) {
    Link l;
    l.x = static_cast<int16_t>(blob_neuron_x(b, neuron));
    l.y = static_cast<int16_t>(blob_neuron_y(b, neuron));
    l.strength = static_cast<uint8_t>(strength);
    l.far = far ? 1 : 0;
    l.inhibitory = (b.inhibitory[neuron / TILE_NEURONS] >> (neuron % TILE_NEURONS)) & 1;
    return l;
}

// who `n` listens to, and who listens to `n`
static void fill_links(Selected& s, const Blob& b, int n) {
    s.sources.clear();
    s.targets.clear();
    for (int k = 0; k < SLOTS; ++k) {
        const int src = slot_source(b, n, k);
        if (src >= b.tile_count) {
            continue;
        }
        const uint64_t any = b.w0[blob_weight_index(n, k)] | b.w1[blob_weight_index(n, k)];
        for (uint64_t bits = any; bits != 0; bits &= bits - 1) {
            const int bit = std::countr_zero(bits);
            Link l =
                make_link(b, src * TILE_NEURONS + bit, blob_weight(b, n, k, bit), k >= LOCAL_SLOTS);
            l.delay = static_cast<uint8_t>(slot_delay(b, n, k));
            s.sources.push_back(l);
        }
    }
    // targets through local slots: neurons in tiles whose local slot k is this tile
    const int tile = n / TILE_NEURONS;
    const int bit = n % TILE_NEURONS;
    for (int t = 0; t < b.tile_count; ++t) {
        for (int k = 0; k < LOCAL_SLOTS; ++k) {
            if (static_cast<int>(b.local_tile[t * LOCAL_SLOTS + k]) != tile) {
                continue;
            }
            for (int m = t * TILE_NEURONS; m < (t + 1) * TILE_NEURONS; ++m) {
                const int w = blob_weight(b, m, k, bit);
                if (w > 0) {
                    s.targets.push_back(make_link(b, m, w, false));
                }
            }
        }
    }
    // targets through far slots: every neuron's two far tiles
    for (int m = 0; m < b.neuron_count; ++m) {
        for (int k = 0; k < FAR_SLOTS; ++k) {
            if (static_cast<int>(b.far_tile[m * FAR_SLOTS + k]) != tile) {
                continue;
            }
            const int w = blob_weight(b, m, LOCAL_SLOTS + k, bit);
            if (w > 0) {
                s.targets.push_back(make_link(b, m, w, true));
            }
        }
    }
}

// where the picked neuron is and what kind it is, for any engine; the rest starts empty
void fill_selected_place(Selected& s, const Blob& b, int n) {
    std::vector<Link> sources = std::move(s.sources), targets = std::move(s.targets);
    std::vector<EvidenceCell> evidence = std::move(s.evidence);
    std::vector<uint8_t> table = std::move(s.table), input_on = std::move(s.table_input_on);
    s = Selected{};
    s.sources = std::move(sources);
    s.targets = std::move(targets);
    s.evidence = std::move(evidence);
    s.table = std::move(table);
    s.table_input_on = std::move(input_on);
    s.sources.clear();
    s.targets.clear();
    s.evidence.clear();
    s.table.clear();
    s.table_input_on.clear();
    s.neuron = n;
    s.x = blob_neuron_x(b, n);
    s.y = blob_neuron_y(b, n);
    s.tile = n / TILE_NEURONS;
    s.bit = n % TILE_NEURONS;
    const uint64_t mask = 1ull << s.bit;
    s.kind = (b.sensor[s.tile] & mask)       ? KIND_SENSOR
             : (b.motor[s.tile] & mask)      ? KIND_MOTOR
             : (b.inhibitory[s.tile] & mask) ? KIND_INHIBITORY
                                             : KIND_EXCITATORY;
}

static void fill_selected(Selected& s, const Blob& b, int n) {
    fill_selected_place(s, b, n);
    s.sector = static_cast<int>(b.tile_sector[s.tile]);
    s.threshold = b.threshold[n];
    s.potential = b.potential[n];
    s.leak_shift = b.leak_shift[n];
    s.cooldown = b.cooldown[n];
    s.cooldown_len = b.cooldown_len[n];
    s.fan_in = b.fan_in[n];
    s.sector_bias = b.sector_bias[s.sector];
    for (int k = 0; k < FAR_SLOTS; ++k) {
        const int t = static_cast<int>(b.far_tile[n * FAR_SLOTS + k]);
        s.far_tile[k] = t < b.tile_count ? t : -1;
        s.far_delay[k] = slot_delay(b, n, LOCAL_SLOTS + k);
    }
    fill_links(s, b, n);
}

void frame_fill(ViewFrame& frame, const Blob& blob, int selected, bool heavy, int engine) {
    frame.real = engine != ENGINE_FAKE;
    frame.engine = engine;
    frame.lines_known = engine_lines(static_cast<Engine>(engine));
    fill_shape(frame, blob);
    const bool full = engine == ENGINE_FAKE || engine == ENGINE_BLOB;
    frame.has = full                       ? HAS_SECTORS | HAS_WEIGHTS | HAS_THRESHOLDS | HAS_FAN_IN
                : engine == ENGINE_MINIMAL ? HAS_THRESHOLDS | HAS_FAN_IN
                                           : 0u;
    if (full) {
        fill_sectors(frame, blob);
    } else {
        frame.sectors.clear();
    }
    if (heavy && full) {
        fill_histograms(frame, blob);
    } else if (heavy && engine == ENGINE_MINIMAL) {
        fill_histograms_minimal(frame, blob);
    }
    if (selected < 0 || selected >= blob.neuron_count) {
        frame.selected = Selected{};
    } else if (full) {
        fill_selected(frame.selected, blob, selected);
    } else if (engine == ENGINE_MINIMAL) {
        fill_selected_minimal(frame.selected, blob, selected);
    } else if (engine == ENGINE_TABLE) {
        fill_selected_table(frame.selected, blob, selected);
    }
}
