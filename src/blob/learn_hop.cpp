#include "blob/hop_state.hpp"
#include "blob/random_words.hpp"
#include "blob/weight_levels.hpp"

#include <cstdlib>
#include <cstring>

// the error sent one hop, to every hidden cell (genome hop_on; docs/results-2026-11-25-hop.md).
// direct feedback alignment (Nokland 2016) with a mark per connection (e-prop, Bellec et al.
// 2020), in bits, online, with no chain backwards and no second pass.
//
// error lines. at each verdict the world names the motor lines the blob pressed and those that
// were right (blob_hop_lines). on bad, a pressed line that was not right should have been
// lower (-1); with hop_teach 2 a right line that was not pressed should have been higher (+1);
// with hop_good, on good, the pressed line should have been higher. nothing else is known.
// hop_teach 3 (docs/results-2026-11-25-hop-real.md): every verdict teaches from the answer
// alone; a line's error is its target (1 right, 0 not) less its running rate of being right,
// in 1/65536ths, so what the readout pressed plays no part (direct random target projection,
// Frenkel et al. 2021, centred).
//
// one hop. each error line reaches a hidden excitatory cell with chance 2^-hop_wire_shift,
// with a sign; both come from a hash of the cell and the line, so the wires cost no memory.
// the cell's teaching bit is the sign of the sum of its wires times the errors: fire more for
// what was just shown, or fire less. a sum of zero teaches nothing.
//
// marks. a cell is "near" on a tick when its potential is at most hop_near eighths of its bar
// under the bar (bar = threshold + sector bias), whether or not it fired; with hop_fired the
// cells that fired count as near too. one word per tile per tick, in a ring beside the spike
// ring, is all that is stored. a connection is marked when its source was on (at that slot's
// own delay) on a tick, within hop_window ticks of the verdict, when its cell was near: the
// mark is read off the two rings at the verdict, never kept per connection.
//
// move. each marked source's hidden counter (the tag store, tag_store.cpp) steps toward the
// teaching bit with chance 2^-hop_shift (excitatory sources up for "more", inhibitory ones
// down). the cell rewires its top counters at its born counts (finding 38) only when a step
// crossed the line between wired and unwired, or strong and weak: each cell keeps bounds on
// its counters (lowest wired, highest unwired, lowest strong, highest weak, per kind), so a
// re-pick happens only when it can change something.
//
// idea 164: the input lines' prediction errors as more error lines (blob_hop_errors, from
// run/predict.cpp), lines HOP_PRED_FIRST and on, taught on the next tick beside the buttons'
// (or alone: hop_teach 0 sends no button errors). with places (blob_hop_error_places) a
// prediction line reaches only the cells within `reach` cells of its sensor group, still with
// a hashed sign (the predictive-coding form); without, the hashed wires as for the buttons.
//
// idea 178 (docs/results-2026-11-26-every-event.md), each off by default: hop_bits keeps the
// counters as bit planes wide enough to step on every marked input (hop_shift 0;
// hop_planes.cpp); hop_teach 4, 5 take the output's own signed error per line
// (hop_lines.cpp, which holds every button error); hop_who lets inhibitory, pacemaker and
// motor cells learn too.

enum { LOW_WIRED = 0, HIGH_UNWIRED = 2, LOW_STRONG = 4, HIGH_WEAK = 6 };

static HopState& hop_make(Blob& blob) {
    if (!blob.hop) {
        HopState* h = (HopState*)std::calloc(1, sizeof(HopState));
        size_t words = (size_t)blob.tile_count + 1;
        h->near = (uint64_t*)std::calloc(HISTORY * words, sizeof(uint64_t));
        h->bounds = (int8_t*)std::calloc((size_t)blob.neuron_count * 8, 1);
        h->fresh = (uint8_t*)std::calloc((size_t)blob.neuron_count, 1);
        h->salt = rng_next(blob.rng);
        blob.hop = h;
    }
    return *blob.hop;
}

void hop_free(Blob& blob) {
    if (!blob.hop) {
        return;
    }
    std::free(blob.hop->near);
    std::free(blob.hop->bounds);
    std::free(blob.hop->fresh);
    std::free(blob.hop->ring_up);
    std::free(blob.hop->ring_down);
    hop_planes_free(*blob.hop);
    std::free(blob.hop);
    blob.hop = nullptr;
}

void hop_stale(Blob& blob, int n) {
    blob.hop->fresh[n] = 0;
}

void blob_hop_lines(Blob& blob, uint64_t pressed, uint64_t right, int lines) {
    if (!blob.genome.hop_on) {
        return;
    }
    HopState& h = hop_make(blob);
    h.pressed = pressed;
    h.right = right;
    h.lines = lines < 64 ? lines : 64;
    h.pending = 1;
}

void blob_hop_errors(Blob& blob, const int8_t* error, int count) {
    if (!blob.genome.hop_on || !blob.genome.learning_on) {
        return;
    }
    HopState& h = hop_make(blob);
    h.pred_lines = count < HOP_MAX_PRED ? count : HOP_MAX_PRED;
    std::memcpy(h.pred_error, error, (size_t)h.pred_lines);
    h.pred_pending = 1;
}

void blob_hop_error_places(Blob& blob, const int16_t* x, const int16_t* y, int count, int reach) {
    if (!blob.genome.hop_on || !blob.genome.learning_on) {
        return;
    }
    HopState& h = hop_make(blob);
    count = count < HOP_MAX_PRED ? count : HOP_MAX_PRED;
    std::memcpy(h.pred_x, x, (size_t)count * sizeof(int16_t));
    std::memcpy(h.pred_y, y, (size_t)count * sizeof(int16_t));
    h.pred_reach = reach;
}

void blob_hop_counts(const Blob& blob, uint64_t out[7]) {
    for (int i = 0; i < 7; i++) {
        out[i] = blob.hop ? blob.hop->counts[i] : 0;
    }
}

// which cells of tile t learn: hidden excitatory ones; with hop_who also inhibitory (1),
// pacemaker (2) and motor (4) cells. sensor cells never
static uint64_t learners(const Blob& blob, int t) {
    int who = blob.genome.hop_who;
    uint64_t out = blob.sensor[t];
    out |= who & 1 ? 0 : blob.inhibitory[t];
    out |= who & 2 ? 0 : blob.pacemaker[t];
    out |= who & 4 ? 0 : blob.motor[t];
    return ~out;
}

void hop_after_fire(Blob& blob) {
    const Genome& g = blob.genome;
    HopState& h = hop_make(blob);
    int64_t stride = (int64_t)blob.tile_count + 1;
    uint64_t* out = h.near + (int64_t)blob.now * stride;
    const uint64_t* fired = blob.spikes[blob.now];
    int eighths = g.hop_near < 0 ? 0 : (g.hop_near > 8 ? 8 : g.hop_near);
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t learn = learners(blob, t);
        if (!learn) {
            out[t] = 0;
            continue;
        }
        int bias = blob.sector_bias[blob.tile_sector[t]];
        const int16_t* pot = blob.potential + t * TILE_NEURONS;
        const int16_t* thr = blob.threshold + t * TILE_NEURONS;
        // eight cells at a time: a flag byte each, gathered into the word by a multiply
        uint64_t near = 0;
        for (int j = 0; j < 8; j++) {
            uint64_t bytes = 0;
            for (int i = 0; i < 8; i++) {
                int b = 8 * j + i;
                int bar = thr[b] + bias;
                int band = bar > 0 ? (bar * eighths) >> 3 : 0;
                bytes |= (uint64_t)(pot[b] >= bar - band) << (8 * i);
            }
            near |= ((bytes * 0x0102040810204080ull) >> 56) << (8 * j);
        }
        near &= ~fired[t];
        near |= g.hop_fired ? fired[t] : 0;
        out[t] = near & learn;
    }
    out[blob.tile_count] = 0;
}

// a fixed random wire from error line `line` to cell n: +1, -1 or 0 (none). a prediction
// line with places reaches the cells within pred_reach of its group (any sign)
static int wire(const Blob& blob, const HopState& h, int shift, int n, int line) {
    if (line >= HOP_PRED_FIRST && h.pred_reach > 0) {
        int dx = blob_neuron_x(blob, n) - h.pred_x[line - HOP_PRED_FIRST];
        int dy = blob_neuron_y(blob, n) - h.pred_y[line - HOP_PRED_FIRST];
        if (dx > h.pred_reach || -dx > h.pred_reach || dy > h.pred_reach || -dy > h.pred_reach) {
            return 0;
        }
        shift = 0;
    }
    uint64_t z = h.salt ^ ((uint64_t)n * 0x9e3779b97f4a7c15ull) ^ ((uint64_t)(line + 1) << 40);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    z ^= z >> 31;
    if (shift > 0 && (z & ((1ull << shift) - 1)) != 0) {
        return 0;
    }
    return (z >> 63) ? 1 : -1;
}

// exact bounds from the counters and the wiring
static void bounds_make(Blob& blob, int n, int8_t* bd) {
    const int8_t* c = tag_row(blob, n);
    for (int i = 0; i < 2; i++) {
        bd[LOW_WIRED + i] = 127;
        bd[HIGH_UNWIRED + i] = -128;
        bd[LOW_STRONG + i] = 127;
        bd[HIGH_WEAK + i] = -128;
    }
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        uint64_t u = hop_usable(blob, n, k, src);
        uint64_t kind = hop_kind_of(blob, k, src);
        int64_t w = blob_weight_index(n, k);
        uint64_t wired = connected_bits(blob.w0[w], blob.w1[w]), strong = blob.w1[w];
        for (uint64_t m = u; m; m &= m - 1) {
            int b = __builtin_ctzll(m);
            int8_t v = c[k * TILE_NEURONS + b];
            int i = (int)((kind >> b) & 1);
            if (!((wired >> b) & 1)) {
                bd[HIGH_UNWIRED + i] = v > bd[HIGH_UNWIRED + i] ? v : bd[HIGH_UNWIRED + i];
                continue;
            }
            bd[LOW_WIRED + i] = v < bd[LOW_WIRED + i] ? v : bd[LOW_WIRED + i];
            if ((strong >> b) & 1) {
                bd[LOW_STRONG + i] = v < bd[LOW_STRONG + i] ? v : bd[LOW_STRONG + i];
            } else {
                bd[HIGH_WEAK + i] = v > bd[HIGH_WEAK + i] ? v : bd[HIGH_WEAK + i];
            }
        }
    }
    blob.hop->fresh[n] = 1;
}

// one counter step for the bits of `up` and `down` in slot k; true if a step may change
// what the cell wires (the bounds are kept loose in the safe direction)
static bool step_slot(Blob& blob, int n, int k, uint32_t src, uint64_t up, uint64_t down,
                      int8_t* bd, uint32_t& moved) {
    int8_t* c = tag_row(blob, n) + k * TILE_NEURONS;
    int bits = blob.genome.tag_bits;
    int top = (1 << ((bits < 2 ? 2 : (bits > 8 ? 8 : bits)) - 1)) - 1;
    int64_t w = blob_weight_index(n, k);
    uint64_t wired = connected_bits(blob.w0[w], blob.w1[w]), strong = blob.w1[w];
    uint64_t kind = hop_kind_of(blob, k, src);
    bool cross = false;
    for (; up; up &= up - 1) {
        int b = __builtin_ctzll(up);
        if (c[b] >= top) {
            continue;
        }
        int8_t v = ++c[b];
        moved++;
        int i = (int)((kind >> b) & 1);
        if (!((wired >> b) & 1)) {
            cross |= v > bd[LOW_WIRED + i];
            bd[HIGH_UNWIRED + i] = v > bd[HIGH_UNWIRED + i] ? v : bd[HIGH_UNWIRED + i];
        } else if (!((strong >> b) & 1)) {
            cross |= v > bd[LOW_STRONG + i];
            bd[HIGH_WEAK + i] = v > bd[HIGH_WEAK + i] ? v : bd[HIGH_WEAK + i];
        }
    }
    for (; down; down &= down - 1) {
        int b = __builtin_ctzll(down);
        if (c[b] <= -top) {
            continue;
        }
        int8_t v = --c[b];
        moved++;
        int i = (int)((kind >> b) & 1);
        if (!((wired >> b) & 1)) {
            continue;
        }
        cross |= v < bd[HIGH_UNWIRED + i];
        bd[LOW_WIRED + i] = v < bd[LOW_WIRED + i] ? v : bd[LOW_WIRED + i];
        if ((strong >> b) & 1) {
            cross |= v < bd[HIGH_WEAK + i];
            bd[LOW_STRONG + i] = v < bd[LOW_STRONG + i] ? v : bd[LOW_STRONG + i];
        }
    }
    return cross;
}

// the same move on bit-plane counters (hop_bits): a slot's marked sources step together, and
// the cell re-picks its top counters at once, or with hop_rewire_every when its turn comes
static void move_cell_planes(Blob& blob, HopState& h, int n, bool more, uint32_t ages,
                             int planes) {
    const Genome& g = blob.genome;
    uint64_t* row = hop_plane_row(blob, h, n);
    int moved = 0;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        int delay = blob.slot_delay[blob_weight_index(n, k)];
        uint64_t mark = 0;
        for (uint32_t a = ages; a; a &= a - 1) {
            int age = __builtin_ctz(a);
            if (age + delay > HISTORY - 1) {
                break; // older than the ring keeps
            }
            mark |= blob.spikes[(blob.now - age - delay) & (HISTORY - 1)][src];
        }
        mark &= hop_usable(blob, n, k, src) & sparse_word(blob.rng, g.hop_shift);
        uint64_t kind = hop_kind_of(blob, k, src);
        uint64_t excite = mark & ~kind, inhibit = mark & kind;
        moved += hop_planes_step(row + k * planes, planes, more ? excite : inhibit,
                                 more ? inhibit : excite);
    }
    h.counts[2] += (uint64_t)moved;
    if (moved == 0) {
        return;
    }
    if (g.hop_rewire_every > 0) {
        h.dirty[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
        return;
    }
    h.counts[3] += hop_planes_rewire(blob, h, n);
}

// one taught cell: its marked sources move toward firing more (more) or less. ages: the
// bits of the window ticks on which the cell was near (bit a: a ticks before the newest map)
static void move_cell(Blob& blob, int n, bool more, uint32_t ages) {
    const Genome& g = blob.genome;
    HopState& h = *blob.hop;
    int planes = hop_plane_count(g);
    if (planes > 0) {
        move_cell_planes(blob, h, n, more, ages, planes);
        return;
    }
    int8_t* bd = h.bounds + (size_t)n * 8;
    tag_row(blob, n);
    if (!h.fresh[n]) {
        bounds_make(blob, n, bd);
    }
    bool cross = false;
    uint32_t moved = 0;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        int delay = blob.slot_delay[blob_weight_index(n, k)];
        uint64_t mark = 0;
        for (uint32_t a = ages; a; a &= a - 1) {
            int age = __builtin_ctz(a);
            if (age + delay > HISTORY - 1) {
                break; // older than the ring keeps
            }
            mark |= blob.spikes[(blob.now - age - delay) & (HISTORY - 1)][src];
        }
        mark &= hop_usable(blob, n, k, src);
        if (!mark) {
            continue;
        }
        mark &= sparse_word(blob.rng, g.hop_shift);
        if (!mark) {
            continue;
        }
        uint64_t kind = hop_kind_of(blob, k, src);
        uint64_t excite = mark & ~kind, inhibit = mark & kind;
        cross |= step_slot(blob, n, k, src, more ? excite : inhibit, more ? inhibit : excite, bd,
                           moved);
    }
    h.counts[2] += moved;
    if (cross) {
        tag_rewire(blob, n); // marks the bounds stale; made again on the next move
        h.counts[3]++;
    }
}

void learn_hop(Blob& blob) {
    const Genome& g = blob.genome;
    HopState& h = hop_make(blob);
    bool buttons = h.pending && blob.pending_valence != 0;
    if (!buttons && !h.pred_pending) {
        return;
    }
    h.pending = buttons ? 0 : h.pending;
    int valence = blob.pending_valence;
    // the error lines: the buttons' (hop_teach 0: none), then the predictions'
    int line[HOP_PRED_FIRST + HOP_MAX_PRED], sign[HOP_PRED_FIRST + HOP_MAX_PRED], count = 0;
    if (buttons) {
        count = hop_button_errors(blob, h, valence, line, sign);
    }
    int from_pred = count;
    for (int l = 0; h.pred_pending && l < h.pred_lines; l++) {
        if (h.pred_error[l] != 0) {
            line[count] = HOP_PRED_FIRST + l;
            sign[count++] = h.pred_error[l];
        }
    }
    for (int i = from_pred; g.hop_coin && i < count; i++) {
        line[i] = HOP_PRED_FIRST + (int)rng_below(blob.rng, (uint32_t)h.pred_lines);
    }
    h.pred_pending = 0;
    if (count == 0) {
        return;
    }
    tag_store_make(blob);
    int window = g.hop_window < 1 ? 1 : (g.hop_window > HISTORY - 1 ? HISTORY - 1 : g.hop_window);
    int64_t stride = (int64_t)blob.tile_count + 1;
    uint32_t taught = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        // the window's near maps for this tile, and the cells near at least once
        uint64_t near[HISTORY], any = 0;
        for (int a = 1; a <= window; a++) {
            near[a] = h.near[((blob.now - a) & (HISTORY - 1)) * stride + t];
            any |= near[a];
            h.counts[4] += __builtin_popcountll(near[a] & ~blob.spikes[(blob.now - a) & (HISTORY - 1)][t]);
            h.counts[5] += __builtin_popcountll(near[a] & blob.spikes[(blob.now - a) & (HISTORY - 1)][t]);
        }
        h.counts[6] += (uint64_t)window * __builtin_popcountll(learners(blob, t));
        for (; any; any &= any - 1) {
            int b = __builtin_ctzll(any);
            int n = t * TILE_NEURONS + b;
            int sum = 0;
            for (int i = 0; i < count; i++) {
                sum += sign[i] * wire(blob, h, g.hop_wire_shift, n, line[i]);
            }
            if (sum == 0) {
                continue;
            }
            uint32_t ages = 0;
            for (int a = 1; a <= window; a++) {
                ages |= (uint32_t)((near[a] >> b) & 1) << a;
            }
            // hop_loose: one mark a verdict, a source on at any tick of the window
            ages = g.hop_loose ? (uint32_t)((2u << window) - 2u) : ages;
            move_cell(blob, n, sum > 0, ages);
            taught++;
        }
    }
    h.counts[0]++;
    h.counts[1] += taught;
    // hop_rewire_every: the cells whose counters moved re-pick together, every so many
    // verdicts that taught
    if (g.hop_rewire_every > 0 && h.dirty && h.counts[0] % (uint64_t)g.hop_rewire_every == 0) {
        for (int t = 0; t < blob.tile_count; t++) {
            for (uint64_t m = h.dirty[t]; m; m &= m - 1) {
                h.counts[3] += hop_planes_rewire(blob, h, t * TILE_NEURONS + __builtin_ctzll(m));
            }
            h.dirty[t] = 0;
        }
    }
}
