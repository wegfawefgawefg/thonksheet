// The readout as the viewer watches it (readout_watch.hpp). Reads each engine's arrays,
// never writes them.

#include "readout_watch.hpp"

#include "engine.hpp"
#include "minimal/minimal_steps.hpp"
#include "world/plug.hpp"

#include <algorithm>
#include <cstring>

static int clamp_byte(int v) {
    return std::clamp(v, -127, 127);
}

// the tile slot k of neuron n reads, tile_count for none
static int source_tile(int engine, const Blob& b, int n, int k) {
    if (engine == ENGINE_MINIMAL) {
        const MinimalBlob& m = minimal_of(b);
        return k < m.slots ? static_cast<int>(min_source_tile(m, n, k)) : b.tile_count;
    }
    if (engine != ENGINE_BLOB && engine != ENGINE_FAKE) {
        return b.tile_count;
    }
    if (k < LOCAL_SLOTS) {
        return static_cast<int>(b.local_tile[(n / TILE_NEURONS) * LOCAL_SLOTS + k]);
    }
    return static_cast<int>(b.far_tile[n * FAR_SLOTS + k - LOCAL_SLOTS]);
}

// the full engine: evidence counters on a motor cell when the rule keeps them (made on the
// rule's first use; until then they equal the born strengths), else connection strengths
static int full_keep(const Blob& b, int n, int8_t* value, int8_t* wired, int* top, int* budget) {
    const bool motor = (b.motor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
    const bool counters = b.genome.readout_evidence != 0 && motor;
    const int8_t* row = nullptr;
    if (counters && b.readout_row != nullptr && b.readout_row[n] >= 0 &&
        b.readout_keep[b.readout_row[n]] > 0) {
        row = b.readout_evidence + static_cast<int64_t>(b.readout_row[n]) * KEEP_SOURCES;
        *budget = b.readout_keep[b.readout_row[n]];
    }
    const int bits = std::clamp(b.genome.readout_evidence_bits, 2, 8);
    *top = counters ? (1 << (bits - 1)) - 1 : 5;
    const int kind = counters ? KEEP_COUNTERS : KEEP_STRENGTHS;
    if (row != nullptr && wired == nullptr) {
        std::memcpy(value, row, KEEP_SOURCES); // the counters alone: one copy
        return kind;
    }
    // a slot's two weight words at once (blob_weight per bit is slow at this count)
    for (int k = 0; k < SLOTS; ++k) {
        const int64_t at = blob_weight_index(n, k);
        const uint64_t w0 = b.w0[at], w1 = b.w1[at];
        int8_t* v = value + k * TILE_NEURONS;
        for (int bit = 0; bit < TILE_NEURONS; ++bit) {
            const int w = static_cast<int>((w0 >> bit) & 1) + 4 * static_cast<int>((w1 >> bit) & 1);
            v[bit] = static_cast<int8_t>(w);
            if (wired != nullptr) {
                wired[k * TILE_NEURONS + bit] = w > 0 ? 1 : 0;
            }
        }
    }
    if (row != nullptr) {
        std::memcpy(value, row, KEEP_SOURCES);
    }
    return kind;
}

// the minimal engine: bitsliced counters (offset binary) where it keeps them, else the mask
static int minimal_keep(const Blob& b, int n, int8_t* value, int8_t* wired, int* top, int* budget) {
    const MinimalBlob& m = minimal_of(b);
    const bool counters = m.ev_row != nullptr && m.ev_row[n] >= 0;
    const int bits = m.mg.ev_bits;
    for (int k = 0; k < SLOTS; ++k) {
        const uint64_t mask = k < m.slots ? m.mask[min_index(m, n, k)] : 0;
        const uint64_t* planes =
            counters && k < m.slots
                ? m.ev_planes + (static_cast<int64_t>(m.ev_row[n]) * m.slots + k) * bits
                : nullptr;
        for (int bit = 0; bit < TILE_NEURONS; ++bit) {
            int v = static_cast<int>((mask >> bit) & 1);
            if (planes != nullptr) {
                v = 0;
                for (int i = 0; i < bits; ++i) {
                    v |= static_cast<int>((planes[i] >> bit) & 1) << i;
                }
                v -= 1 << (bits - 1);
            }
            if (wired != nullptr) {
                wired[k * TILE_NEURONS + bit] = static_cast<int8_t>((mask >> bit) & 1);
            }
            value[k * TILE_NEURONS + bit] = static_cast<int8_t>(clamp_byte(v));
        }
    }
    *top = counters ? 1 << (bits - 1) : 1;
    *budget = m.mg.budget;
    return counters ? KEEP_COUNTERS : KEEP_STRENGTHS;
}

static int keep_row(int engine, const Blob& b, int n, int8_t* value, int8_t* wired, int* top,
                    int* budget) {
    *top = 1;
    *budget = 0;
    if (engine == ENGINE_BLOB) {
        return full_keep(b, n, value, wired, top, budget);
    }
    if (engine == ENGINE_MINIMAL) {
        return minimal_keep(b, n, value, wired, top, budget);
    }
    return KEEP_NONE;
}

void readout_watch_start(ReadoutWatch& w, int engine, const Blob& blob, const Plug& plug) {
    w = ReadoutWatch{};
    w.engine = engine;
    w.view.on = true;
    w.view.buttons.assign(plug.motor_lines, ReadoutButton{});
    w.step_spikes.assign(plug.motor_lines, 0);
    for (int line = 0; line < plug.motor_lines; ++line) {
        const int* cells = nullptr;
        const int count = plug_motor_cells(plug, line, &cells);
        w.view.buttons[line].cells = count;
        for (int k = 0; k < count; ++k) {
            w.cells.push_back(cells[k]);
            w.button.push_back(static_cast<uint8_t>(line));
            ReadoutCell c;
            c.x = static_cast<int16_t>(blob_neuron_x(blob, cells[k]));
            c.y = static_cast<int16_t>(blob_neuron_y(blob, cells[k]));
            c.button = static_cast<uint8_t>(line);
            w.view.cells.push_back(c);
        }
    }
    w.row.assign(KEEP_SOURCES, 0);
    w.wired.assign(KEEP_SOURCES, 0);
    int top = 1, budget = 0;
    w.view.keep = w.cells.empty() ? KEEP_NONE
                                  : keep_row(engine, blob, w.cells[0], w.row.data(), w.wired.data(),
                                             &top, &budget);
}

void readout_watch_tick(ReadoutWatch& w, const Blob& blob) {
    // a word per button per tile it has cells in: one AND and popcount each
    if (w.words.empty() && !w.cells.empty()) {
        for (size_t i = 0; i < w.cells.size(); ++i) {
            const int tile = w.cells[i] / TILE_NEURONS;
            const uint64_t bit = 1ull << (w.cells[i] % TILE_NEURONS);
            bool found = false;
            for (ReadoutWord& word : w.words) {
                if (word.tile == tile && word.button == w.button[i]) {
                    word.cells |= bit;
                    found = true;
                }
            }
            if (!found) {
                w.words.push_back(ReadoutWord{tile, w.button[i], bit});
            }
        }
    }
    const uint64_t* spikes = blob.spikes[blob.now];
    for (const ReadoutWord& word : w.words) {
        w.step_spikes[word.button] += __builtin_popcountll(spikes[word.tile] & word.cells);
    }
    for (size_t b = 0; b < w.view.buttons.size(); ++b) {
        w.view.buttons[b].spikes = w.step_spikes[b];
    }
}

void readout_watch_answer(ReadoutWatch& w, int move) {
    for (size_t b = 0; b < w.view.buttons.size(); ++b) {
        ReadoutButton& rb = w.view.buttons[b];
        rb.last_spikes = w.step_spikes[b];
        rb.won = static_cast<int>(b) == move;
        rb.spikes = 0;
        w.step_spikes[b] = 0;
    }
}

void readout_watch_reward(ReadoutWatch& w, int64_t step, int valence, int move, bool watch) {
    if (w.view.keep == KEEP_NONE) {
        return;
    }
    if (!watch) {
        w.stale = true; // flat out undrawn: nothing compared, the next one a baseline
        w.pending = false;
        return;
    }
    w.pending = true;
    w.pending_step = step;
    w.pending_valence = valence;
    w.pending_move = move;
}

void readout_watch_settle(ReadoutWatch& w, const Blob& blob) {
    if (!w.pending) {
        return;
    }
    w.pending = false;
    // the first time, or after rewards that were not watched: only a new baseline
    const size_t size = w.cells.size() * KEEP_SOURCES;
    const bool baseline = w.stale || w.before.size() != size;
    w.stale = false;
    w.before.resize(size);
    w.diff.resize(size);
    // compare every cell's row with the baseline, which then becomes the row. a cell's up and
    // down are counted as drive: a local inhibitory source's counter going up takes drive
    // away, as the rule means it ("losing" an input)
    std::vector<int16_t> up(w.cells.size(), 0), down(w.cells.size(), 0);
    bool any = false;
    int top = 1, budget = 0;
    for (size_t i = 0; i < w.cells.size(); ++i) {
        const int n = w.cells[i];
        w.view.keep = keep_row(w.engine, blob, n, w.row.data(), nullptr, &top, &budget);
        int8_t* was = &w.before[i * KEEP_SOURCES];
        int8_t* diff = &w.diff[i * KEEP_SOURCES];
        if (std::memcmp(was, w.row.data(), KEEP_SOURCES) == 0) {
            std::memset(diff, 0, KEEP_SOURCES); // unchanged (every cell, on a good step)
            continue;
        }
        for (int k = 0; k < SLOTS; ++k) {
            const int src = source_tile(w.engine, blob, n, k);
            const uint64_t inh =
                k < LOCAL_SLOTS && src < blob.tile_count ? blob.inhibitory[src] : 0;
            for (int bit = 0; bit < TILE_NEURONS; ++bit) {
                const int j = k * TILE_NEURONS + bit;
                const int d = w.row[j] - was[j];
                const int drive = (inh >> bit) & 1 ? -d : d;
                up[i] = static_cast<int16_t>(up[i] + (drive > 0 ? drive : 0));
                down[i] = static_cast<int16_t>(down[i] + (drive < 0 ? -drive : 0));
                diff[j] = static_cast<int8_t>(clamp_byte(d));
                was[j] = w.row[j];
            }
        }
        any = any || up[i] > 0 || down[i] > 0;
    }
    if (!any || baseline) {
        return;
    }
    // a change: keep it as the last one
    w.moved.swap(w.diff);
    ReadoutView& v = w.view;
    for (ReadoutButton& b : v.buttons) {
        b.gained = b.lost = b.up = b.down = 0;
    }
    for (size_t i = 0; i < w.cells.size(); ++i) {
        ReadoutButton& b = v.buttons[w.button[i]];
        v.cells[i].up = up[i];
        v.cells[i].down = down[i];
        b.up += up[i];
        b.down += down[i];
        b.gained += up[i] > down[i] ? 1 : 0;
        b.lost += down[i] > up[i] ? 1 : 0;
    }
    v.change_step = w.pending_step;
    v.change_valence = w.pending_valence;
    v.change_pressed = w.pending_move;
    ++v.changes;
}

void readout_fill_selected(const ReadoutWatch& w, const Blob& blob, Selected& s) {
    s.evidence.clear();
    s.keep = KEEP_NONE;
    s.button = -1;
    const int n = s.neuron;
    if (n < 0 || n >= blob.neuron_count) {
        return;
    }
    int8_t value[KEEP_SOURCES], wired[KEEP_SOURCES];
    s.keep = keep_row(w.engine, blob, n, value, wired, &s.keep_top, &s.keep_budget);
    if (s.keep == KEEP_NONE) {
        return;
    }
    // the readout cell it is, if any, for what moved at the last change
    const int8_t* moved = nullptr;
    for (size_t i = 0; i < w.cells.size(); ++i) {
        if (w.cells[i] == n) {
            s.button = w.button[i];
            if (w.moved.size() == w.cells.size() * KEEP_SOURCES) {
                moved = &w.moved[i * KEEP_SOURCES];
            }
        }
    }
    // a sensor cell keeps nothing worth a picture
    if (s.kind == KIND_SENSOR) {
        s.keep = KEEP_NONE;
        return;
    }
    for (int k = 0; k < SLOTS; ++k) {
        const int src = source_tile(w.engine, blob, n, k);
        if (src >= blob.tile_count) {
            continue;
        }
        for (int bit = 0; bit < TILE_NEURONS; ++bit) {
            const int j = k * TILE_NEURONS + bit;
            const int8_t mv = moved != nullptr ? moved[j] : 0;
            if (value[j] == 0 && !wired[j] && mv == 0) {
                continue;
            }
            const int source = src * TILE_NEURONS + bit;
            EvidenceCell e;
            e.x = static_cast<int16_t>(blob_neuron_x(blob, source));
            e.y = static_cast<int16_t>(blob_neuron_y(blob, source));
            e.value = value[j];
            e.wired = static_cast<uint8_t>(wired[j]);
            e.moved = mv;
            e.far = k >= LOCAL_SLOTS ? 1 : 0;
            s.evidence.push_back(e);
        }
    }
}
