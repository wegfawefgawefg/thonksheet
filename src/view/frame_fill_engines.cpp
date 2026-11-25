// The picked neuron and the histograms for the engines that keep their wiring outside the
// Blob shell: the minimal engine (a one-bit mask per possible source) and table neurons
// (K inputs addressing a table of counters). Runs on the sim thread between ticks.

#include "engine.hpp"
#include "minimal/minimal_steps.hpp"
#include "view_frame.hpp"

#include <algorithm>

// --- the minimal engine ---

void fill_selected_minimal(Selected& s, const Blob& b, int n) {
    fill_selected_place(s, b, n);
    const MinimalBlob& m = minimal_of(b);
    s.threshold = m.threshold[n];
    s.fan_in = minimal_fan_in(m, n);
    // sources: every set bit of every slot's mask, with the slot's delay
    for (int k = 0; k < m.slots; ++k) {
        const uint32_t src = min_source_tile(m, n, k);
        if (src >= static_cast<uint32_t>(b.tile_count)) {
            continue;
        }
        for (uint64_t bits = m.mask[min_index(m, n, k)]; bits != 0; bits &= bits - 1) {
            Link l = make_link(b, static_cast<int>(src) * TILE_NEURONS + __builtin_ctzll(bits), 4,
                               k >= MIN_SLOTS);
            l.delay = static_cast<uint8_t>(min_delay(m, n, k));
            s.sources.push_back(l);
        }
        if (k >= MIN_SLOTS) {
            s.far_tile[k - MIN_SLOTS] = static_cast<int>(src);
            s.far_delay[k - MIN_SLOTS] = min_delay(m, n, k);
        }
    }
    // targets through local slots: neurons in tiles whose local slot k is this tile
    const int tile = n / TILE_NEURONS;
    const int bit = n % TILE_NEURONS;
    for (int t = 0; t < b.tile_count; ++t) {
        for (int k = 0; k < MIN_SLOTS; ++k) {
            if (static_cast<int>(m.local_tile[t * MIN_SLOTS + k]) != tile) {
                continue;
            }
            for (int target = t * TILE_NEURONS; target < (t + 1) * TILE_NEURONS; ++target) {
                if ((m.mask[min_index(m, target, k)] >> bit) & 1) {
                    s.targets.push_back(make_link(b, target, 4, false));
                }
            }
        }
    }
    s.targets_known = m.slots == MIN_SLOTS; // far targets are not looked for
}

void fill_histograms_minimal(ViewFrame& f, const Blob& b) {
    const MinimalBlob& m = minimal_of(b);
    Histogram& th = f.threshold;
    Histogram& fi = f.fan_in;
    th = Histogram{};
    fi = Histogram{};
    th.width = std::max(1, (m.mg.threshold_cap + HIST_BINS) / HIST_BINS);
    fi.width = std::max(1, (m.mg.budget * 2 + HIST_BINS) / HIST_BINS);
    th.low = fi.low = 1 << 30;
    th.high = fi.high = -(1 << 30);
    for (int n = 0; n < b.neuron_count; ++n) {
        const int values[2] = {m.threshold[n], minimal_fan_in(m, n)};
        Histogram* hs[2] = {&th, &fi};
        for (int i = 0; i < 2; ++i) {
            Histogram& h = *hs[i];
            const int bin = std::clamp(values[i] / h.width, 0, HIST_BINS - 1);
            h.counts[bin] += 1;
            h.total += 1;
            h.low = std::min(h.low, values[i]);
            h.high = std::max(h.high, values[i]);
        }
    }
}

// --- table neurons ---

void fill_selected_table(Selected& s, const Blob& b, int n) {
    fill_selected_place(s, b, n);
    const TableBlob& tb = *reinterpret_cast<const TableBlob*>(&b);
    s.sector = static_cast<int>(tb.tile_sector[s.tile]);
    s.table_inputs = tb.inputs;
    s.table_bits = tb.bits;
    s.table_bar = tb.bar[tb.tile_sector[s.tile]];
    s.table_entry =
        tb.address[static_cast<int64_t>(b.now & (TABLE_ADDR_HISTORY - 1)) * b.neuron_count + n];
    const int entries = 1 << tb.inputs;
    s.table.resize(entries);
    for (int e = 0; e < entries; ++e) {
        s.table[e] = static_cast<uint8_t>(table_counter(tb, n, e));
    }
    // the inputs: each one other neuron's spike at its own delay
    s.table_input_on.resize(tb.inputs);
    for (int j = 0; j < tb.inputs; ++j) {
        const uint32_t e = tb.input[static_cast<int64_t>(n) * tb.inputs + j];
        const int src = static_cast<int>(e >> 4);
        s.table_input_on[j] = static_cast<uint8_t>((s.table_entry >> j) & 1);
        if (src >= b.neuron_count) {
            s.sources.push_back(Link{0, 0, 0, 0, 0, 0}); // off the sheet: strength 0, not drawn
            continue;
        }
        const int dx = std::abs(blob_neuron_x(b, src) - s.x);
        const int dy = std::abs(blob_neuron_y(b, src) - s.y);
        Link l = make_link(b, src, s.table_input_on[j] ? 5 : 1, std::max(dx, dy) > tb.tg.reach);
        l.delay = static_cast<uint8_t>(e & 15);
        s.sources.push_back(l);
        s.fan_in += 1;
    }
    // targets: every neuron with an input from this one
    const int64_t inputs = static_cast<int64_t>(b.neuron_count) * tb.inputs;
    for (int64_t i = 0; i < inputs; ++i) {
        if (static_cast<int>(tb.input[i] >> 4) == n) {
            s.targets.push_back(make_link(b, static_cast<int>(i / tb.inputs), 4, false));
        }
    }
}
