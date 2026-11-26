#include "blob/output_state.hpp"

#include "blob/hop_state.hpp"
#include "blob/tick_steps.hpp"

#include <cstdlib>
#include <cstring>

// the blob's own output (genome out_on; idea 182, docs/results-2026-11-26-output-inside.md).
// the readout the engine had is motor cells that spike: each wires a fixed number of
// sources, one bit each, picked by hidden counters. this one keeps the counters and drops
// the wiring: the counters are the weights. a perceptron with a margin, in bit planes.
//
// state. each motor cell of a button keeps a signed counter per source it can hear (its 9
// local tiles and its far tiles; itself left out; no inhibitory source on a far slot), as
// out_bits bit planes a slot: 11 x out_bits words a cell, the one-hop rule's layout
// (hop_planes.cpp). all start at zero.
//
// read. at the end of a step a source is "on" if it fired in the last out_window ticks:
// one word a tile, an OR of spike maps. a cell's vote is the sum of its counters over the
// sources that are on: an AND and a popcount a plane a slot, shifted by the plane, the sign
// plane taken off. a line's vote is the sum of its cells' votes. the world reads one
// number a line (blob.out_vote, asked for with blob.out_read) and takes the highest; no
// motor cell's spike is read.
//
// learn. the world names the right line at the verdict (blob_output_verdict), as it does
// for the one-hop rule. a wrong line whose vote is within out_margin steps' worth of the
// right line's, or over it, steps down: every cell of it takes one count off each source
// that was on (out_top: only the highest such line). then the right line steps up the same
// way. one step's worth is the number
// of (cell, source on) pairs of the two lines, halved: what a step would move the vote by.
// a right line that wins by more than the margin moves nothing. every event counts, no
// chance is drawn, nothing is re-picked.
//
// teach. the lines that stepped are the one-hop rule's errors (hop_teach 6): "lower" for
// the wrong lines that were too close, "higher" for the right line, nothing for the rest
// and nothing at all on a step won clear. out_fine > 0 gives the cells their own zone.
//
// pay (out_pay 1). instead of the rule's hashed wires, a line's error goes back along the
// output's own sources: a line told "higher" pays each source that was on and has a
// counter above zero in one of its cells, and charges each with a counter below zero; a
// line told "lower" the reverse. tallies are bit-sliced counts a tile; a cell paid more
// often than charged is told "more", the reverse "less"
// (docs/results-2026-11-26-credit-rules.md, pay1).

static int plane_count(const Genome& g) {
    return g.out_bits < 4 ? 4 : (g.out_bits > 16 ? 16 : g.out_bits);
}

static OutputState& make(Blob& blob) {
    if (!blob.output) {
        OutputState* o = (OutputState*)std::calloc(1, sizeof(OutputState));
        o->active = (uint64_t*)std::calloc((size_t)blob.tile_count + 1, sizeof(uint64_t));
        blob.output = o;
    }
    return *blob.output;
}

void output_free(Blob& blob) {
    std::free(blob.motor_line);
    blob.motor_line = nullptr;
    if (!blob.output) {
        return;
    }
    std::free(blob.output->cell);
    std::free(blob.output->counter);
    std::free(blob.output->active);
    std::free(blob.output->tally);
    std::free(blob.output);
    blob.output = nullptr;
}

// the sources of slot k a motor cell may count
static uint64_t countable(const Blob& blob, int n, int k, uint32_t src) {
    uint64_t u = src == (uint32_t)blob.tile_count ? 0 : ~0ull;
    u &= k == 4 ? ~(1ull << (n % TILE_NEURONS)) : ~0ull;
    u &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
    u &= blob.genome.out_hidden ? ~blob.sensor[src] : ~0ull;
    return u;
}

// the buttons' cells, line by line, from the world's naming (blob.motor_line). a naming
// that changed since they were listed is a fresh output: the counters start again at zero
static void list_cells(Blob& blob, OutputState& o) {
    if (o.cell && o.named == blob.motor_line_set) {
        return;
    }
    o.named = blob.motor_line_set;
    int count[OUT_MAX_LINES + 1] = {};
    o.lines = o.cells = o.pending = 0;
    for (int n = 0; n < blob.neuron_count; n++) {
        int l = blob.motor_line[n];
        if (l < OUT_MAX_LINES) {
            count[l]++;
            o.cells++;
            o.lines = l + 1 > o.lines ? l + 1 : o.lines;
        }
    }
    o.first[0] = 0;
    for (int l = 0; l < OUT_MAX_LINES; l++) {
        o.first[l + 1] = o.first[l] + count[l];
        count[l] = 0;
    }
    o.cell = (int32_t*)std::realloc(o.cell, (size_t)(o.cells + 1) * sizeof(int32_t));
    for (int n = 0; n < blob.neuron_count; n++) {
        int l = blob.motor_line[n];
        if (l < OUT_MAX_LINES) {
            o.cell[o.first[l] + count[l]++] = n;
        }
    }
    o.planes = plane_count(blob.genome);
    std::free(o.counter);
    o.counter = (uint64_t*)std::calloc((size_t)(o.cells + 1) * SLOTS * (size_t)o.planes,
                                       sizeof(uint64_t));
}

void output_read(Blob& blob) {
    const Genome& g = blob.genome;
    blob.out_read = 0;
    blob.out_lines = 0;
    if (!blob.motor_line) {
        return;
    }
    OutputState& o = make(blob);
    list_cells(blob, o);
    int planes = o.planes;
    int window = g.out_window < 1 ? 1 : (g.out_window > HISTORY - 1 ? HISTORY - 1 : g.out_window);
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t on = 0;
        for (int age = 0; age < window; age++) {
            on |= blob.spikes[(blob.now - age) & (HISTORY - 1)][t];
        }
        o.active[t] = on;
    }
    o.active[blob.tile_count] = 0;
    const uint64_t* all = o.counter;
    for (int l = 0; l < o.lines; l++) {
        int64_t v = 0, size = 0;
        for (int i = o.first[l]; i < o.first[l + 1]; i++) {
            int n = o.cell[i];
            const uint64_t* row = all + (size_t)i * SLOTS * (size_t)planes;
            for (int k = 0; k < SLOTS; k++) {
                uint32_t src = slot_source(blob, n, k);
                uint64_t on = o.active[src] & countable(blob, n, k, src);
                if (!on) {
                    continue;
                }
                const uint64_t* p = row + k * planes;
                size += __builtin_popcountll(on);
                for (int j = 0; j < planes - 1; j++) {
                    v += (int64_t)__builtin_popcountll(p[j] & on) << j;
                }
                v -= (int64_t)__builtin_popcountll(p[planes - 1] & on) << (planes - 1);
            }
        }
        o.vote[l] = v;
        o.size[l] = size;
        blob.out_vote[l] = (int32_t)(v > (1 << 30) ? (1 << 30) : (v < -(1 << 30) ? -(1 << 30) : v));
    }
    blob.out_lines = o.lines;
}

// which lines are too close at this margin: wrong ones -1, then the right ones +1
static bool zone(const OutputState& o, uint64_t right, int pairs, int margin, bool top,
                 int8_t* out) {
    auto close = [&](int low, int high) {
        return o.vote[low] + margin * ((o.size[low] + o.size[high]) / 2) > o.vote[high];
    };
    bool any = false;
    std::memset(out, 0, OUT_MAX_LINES);
    if (pairs) {
        // a pattern answer: each right line stands against the other line of its pair
        for (int l = 0; l < o.lines; l++) {
            int rival = l ^ 1;
            if (((right >> l) & 1) && rival < o.lines && !((right >> rival) & 1) && close(rival, l)) {
                out[rival] = -1;
                out[l] = 1;
                any = true;
            }
        }
        return any;
    }
    int best = -1;
    for (int l = 0; l < o.lines; l++) {
        best = ((right >> l) & 1) && (best < 0 || o.vote[l] > o.vote[best]) ? l : best;
    }
    int high = -1;
    for (int l = 0; best >= 0 && l < o.lines; l++) {
        if (!((right >> l) & 1) && close(l, best)) {
            out[l] = -1;
            any = true;
            high = high < 0 || o.vote[l] > o.vote[high] ? l : high;
        }
    }
    // out_top: only the highest of the wrong lines that were too close
    for (int l = 0; top && l < o.lines; l++) {
        out[l] = l == high ? -1 : 0;
    }
    for (int l = 0; any && l < o.lines; l++) {
        out[l] = ((right >> l) & 1) ? 1 : out[l];
    }
    return any;
}

void blob_output_verdict(Blob& blob, uint64_t right, int pairs) {
    const Genome& g = blob.genome;
    if (!g.out_on || !g.learning_on || !blob.output || blob.output->lines == 0) {
        return;
    }
    OutputState& o = *blob.output;
    o.pending = zone(o, right, pairs, g.out_margin, g.out_top != 0, o.step);
    zone(o, right, pairs, g.out_fine > 0 ? g.out_fine : g.out_margin, g.out_top == 1, o.teach);
    o.counts[0]++;
    o.counts[1] += (uint64_t)o.pending;
}

int output_errors(const Blob& blob, int* line, int* sign) {
    int count = 0;
    for (int l = 0; blob.output && l < blob.output->lines; l++) {
        if (blob.output->teach[l] != 0) {
            line[count] = l;
            sign[count++] = blob.output->teach[l];
        }
    }
    return count;
}

void learn_output(Blob& blob) {
    if (!blob.output || !blob.output->pending) {
        return;
    }
    OutputState& o = *blob.output;
    o.pending = 0;
    int planes = o.planes;
    uint64_t* all = o.counter;
    for (int l = 0; l < o.lines; l++) {
        if (o.step[l] == 0) {
            continue;
        }
        o.counts[2]++;
        for (int i = o.first[l]; i < o.first[l + 1]; i++) {
            int n = o.cell[i];
            uint64_t* row = all + (size_t)i * SLOTS * (size_t)planes;
            for (int k = 0; k < SLOTS; k++) {
                uint32_t src = slot_source(blob, n, k);
                uint64_t on = o.active[src] & countable(blob, n, k, src);
                if (on) {
                    hop_planes_step(row + k * planes, planes, o.step[l] > 0 ? on : 0,
                                    o.step[l] > 0 ? 0 : on);
                }
            }
        }
    }
}

// one more in a bit-sliced tally for each bit of `add`
static void tally_add(uint64_t* t, uint64_t add) {
    for (int j = 0; j < OUT_PAY_PLANES && add; j++) {
        uint64_t next = t[j] & add;
        t[j] ^= add;
        add = next;
    }
}

void output_pay(Blob& blob, const int* line, const int* sign, int count, uint64_t* more,
                uint64_t* less) {
    OutputState& o = *blob.output;
    size_t words = ((size_t)blob.tile_count + 1) * 2 * OUT_PAY_PLANES;
    if (!o.tally) {
        o.tally = (uint64_t*)std::calloc(words, sizeof(uint64_t));
    }
    std::memset(o.tally, 0, words * sizeof(uint64_t));
    int planes = o.planes;
    const uint64_t* all = o.counter;
    for (int e = 0; e < count; e++) {
        int l = line[e];
        if (l < 0 || l >= o.lines || sign[e] == 0) {
            continue;
        }
        for (int i = o.first[l]; i < o.first[l + 1]; i++) {
            int n = o.cell[i];
            const uint64_t* row = all + (size_t)i * SLOTS * (size_t)planes;
            for (int k = 0; k < SLOTS; k++) {
                uint32_t src = slot_source(blob, n, k);
                uint64_t on = o.active[src] & countable(blob, n, k, src);
                if (!on) {
                    continue;
                }
                const uint64_t* p = row + k * planes;
                uint64_t some = 0;
                for (int j = 0; j < planes; j++) {
                    some |= p[j];
                }
                uint64_t below = p[planes - 1] & on, above = some & ~p[planes - 1] & on;
                uint64_t* t = o.tally + (size_t)src * 2 * OUT_PAY_PLANES;
                tally_add(t, sign[e] > 0 ? above : below);
                tally_add(t + OUT_PAY_PLANES, sign[e] > 0 ? below : above);
            }
        }
    }
    // paid against charged, a tile at a time, from the top plane down
    for (int t = 0; t < blob.tile_count; t++) {
        const uint64_t* paid = o.tally + (size_t)t * 2 * OUT_PAY_PLANES;
        const uint64_t* charged = paid + OUT_PAY_PLANES;
        uint64_t same = ~0ull, up = 0, down = 0;
        for (int j = OUT_PAY_PLANES - 1; j >= 0; j--) {
            up |= same & paid[j] & ~charged[j];
            down |= same & charged[j] & ~paid[j];
            same &= ~(paid[j] ^ charged[j]);
        }
        more[t] = up;
        less[t] = down;
    }
}

void blob_output_counts(const Blob& blob, uint64_t out[4]) {
    for (int i = 0; i < 4; i++) {
        out[i] = blob.output ? blob.output->counts[i] : 0;
    }
}
