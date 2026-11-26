#include "blob/hop_state.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"

#include <cstdlib>
#include <cstring>

// the one-hop rule's error lines from predictions (learn_hop.cpp has the rule, hop_lines.cpp
// the buttons' lines). idea 164 sent the next-step predictor's wrong answers; idea 183
// (docs/results-2026-11-26-predict-every-event.md) adds what the every-event rule needs:
//
// a fine zone (hop_pred_fine). an answer is a pair of motor-cell groups, "off" and "on",
// and the group with more spikes in the step answers. the answer's miss is the wrong
// group's share of the pair's spikes (a silent pair misses by a half). with hop_pred_fine
// F > 0 a line teaches, toward what the line turned out to be, when its miss is F 32nds or
// more: 16 is "wrong or tied", 3 is the ladder's 0.1, and a line that was right by more
// than the zone teaches nothing. 0 is idea 164's error: wrong answers only, a tie says off.
//
// far horizons. an answer about h steps ahead is graded h steps after the cells made it,
// when the blob's rings no longer hold that step. blob_hop_keep_step copies the two rings
// (spikes, near) at each step's end, HOP_KEPT steps of them; blob_hop_predicted with
// back > 1 puts the kept step's rings in the blob's place for the length of one verdict, so
// the marks are "this source was on while the cell was near, in the step the answer was
// made". back 1 waits for the next tick and uses the live rings, as idea 164 did. each
// horizon has its own wire lines (HOP_PRED_FIRST + horizon x HOP_MAX_PRED + line).
//
// coins (hop_coin). 1: each error on a prediction line drawn at random (idea 164's). 3: the
// errors of another step of the same horizon, drawn from the last 256, so they keep their
// counts and pairings and lose only their tie to what was shown.

static size_t kept_stride(const Blob& blob) {
    return (size_t)blob.tile_count + 1;
}

void hop_pred_free(HopState& h) {
    std::free(h.kept);
    std::free(h.pred_ring);
    h.kept = nullptr;
    h.pred_ring = nullptr;
}

void blob_hop_keep_step(Blob& blob) {
    if (!blob.genome.hop_on || !blob.genome.learning_on || !blob.hop) {
        return;
    }
    HopState& h = *blob.hop;
    size_t stride = kept_stride(blob), step = 2 * HISTORY * stride;
    if (!h.kept) {
        h.kept = (uint64_t*)std::calloc(HOP_KEPT * step, sizeof(uint64_t));
    }
    int slot = (int)(h.kept_steps & (HOP_KEPT - 1));
    uint64_t* at = h.kept + (size_t)slot * step;
    for (int j = 0; j < HISTORY; j++) {
        std::memcpy(at + (size_t)j * stride, blob.spikes[j], stride * sizeof(uint64_t));
    }
    std::memcpy(at + HISTORY * stride, h.near, HISTORY * stride * sizeof(uint64_t));
    h.kept_now[slot] = (uint8_t)blob.now;
    h.kept_steps++;
}

// hop_coin 3: this step's errors of the horizon go into its ring, another step's come out
static void ring_swap(Blob& blob, HopState& h) {
    int index = h.pred_first < HOP_PRED_HORIZONS ? h.pred_first : HOP_PRED_HORIZONS - 1;
    size_t row = HOP_MAX_PRED;
    if (!h.pred_ring) {
        h.pred_ring = (int8_t*)std::calloc(HOP_PRED_HORIZONS * HOP_COIN_RING * row, 1);
    }
    int8_t* ring = h.pred_ring + (size_t)index * HOP_COIN_RING * row;
    uint32_t& count = h.pred_ring_count[index];
    uint32_t kept = count < HOP_COIN_RING ? count : HOP_COIN_RING;
    int8_t now[HOP_MAX_PRED];
    std::memcpy(now, h.pred_error, row);
    if (kept > 0) {
        std::memcpy(h.pred_error, ring + (size_t)rng_below(blob.rng, kept) * row, row);
    }
    std::memcpy(ring + (size_t)(count % HOP_COIN_RING) * row, now, row);
    count++;
}

int hop_pred_errors(Blob& blob, HopState& h, int* line, int* sign, int count) {
    const Genome& g = blob.genome;
    if (!h.pred_pending) {
        return count;
    }
    h.pred_pending = 0;
    if (g.hop_coin == 3) {
        ring_swap(blob, h);
    }
    int first = HOP_PRED_FIRST + h.pred_first * HOP_MAX_PRED;
    int from = count;
    for (int l = 0; l < h.pred_lines; l++) {
        if (h.pred_error[l] != 0) {
            line[count] = first + l;
            sign[count++] = h.pred_error[l];
        }
    }
    for (int i = from; g.hop_coin && g.hop_coin != 3 && i < count; i++) {
        line[i] = first + (int)rng_below(blob.rng, (uint32_t)h.pred_lines);
    }
    return count;
}

// one verdict from the kept step that ended `back` steps ago: its rings stand in for the
// blob's while the rule reads its marks, and the errors waiting for the next tick are put
// aside. the fake "now" is one past the step's last tick, where the next tick would be
static void teach_back(Blob& blob, HopState& h, const int8_t* error, int count, int index,
                       int back) {
    if (!h.kept || back > h.kept_steps || back > HOP_KEPT) {
        return;
    }
    size_t stride = kept_stride(blob);
    int slot = (int)((h.kept_steps - back) & (HOP_KEPT - 1));
    uint64_t* at = h.kept + (size_t)slot * 2 * HISTORY * stride;
    uint64_t* live_spikes[HISTORY];
    for (int j = 0; j < HISTORY; j++) {
        live_spikes[j] = blob.spikes[j];
        blob.spikes[j] = at + (size_t)j * stride;
    }
    uint64_t* live_near = h.near;
    int live_now = blob.now, live_pending = h.pending;
    int8_t wait_error[HOP_MAX_PRED];
    int wait_lines = h.pred_lines, wait_pending = h.pred_pending, wait_first = h.pred_first;
    std::memcpy(wait_error, h.pred_error, HOP_MAX_PRED);

    h.near = at + HISTORY * stride;
    blob.now = (h.kept_now[slot] + 1) & (HISTORY - 1);
    h.pending = 0; // the buttons' verdict is about the live step
    std::memset(h.pred_error, 0, HOP_MAX_PRED);
    std::memcpy(h.pred_error, error, (size_t)count);
    h.pred_lines = count;
    h.pred_first = index;
    h.pred_pending = 1;
    learn_hop(blob);

    for (int j = 0; j < HISTORY; j++) {
        blob.spikes[j] = live_spikes[j];
    }
    h.near = live_near;
    blob.now = live_now;
    h.pending = live_pending;
    std::memcpy(h.pred_error, wait_error, HOP_MAX_PRED);
    h.pred_lines = wait_lines;
    h.pred_pending = wait_pending;
    h.pred_first = wait_first;
}

void blob_hop_predicted(Blob& blob, int index, int back, const uint16_t* off, const uint16_t* on,
                        const uint8_t* truth, int count) {
    const Genome& g = blob.genome;
    if (!g.hop_on || !g.learning_on || !blob.hop || index < 0 || index >= HOP_PRED_HORIZONS) {
        return;
    }
    HopState& h = *blob.hop;
    count = count < HOP_MAX_PRED ? count : HOP_MAX_PRED;
    int fine = g.hop_pred_fine > 32 ? 32 : g.hop_pred_fine;
    int8_t error[HOP_MAX_PRED];
    for (int l = 0; l < count; l++) {
        int was = truth[l] != 0;
        if (fine <= 0) {
            error[l] = (int8_t)(was - (on[l] > off[l]));
        } else {
            int wrong = was ? off[l] : on[l], total = on[l] + off[l];
            bool miss = total > 0 ? 32 * wrong >= fine * total : 16 >= fine;
            error[l] = (int8_t)(miss ? (was ? 1 : -1) : 0);
        }
        h.pred_sent += error[l] != 0;
    }
    h.pred_seen += (uint64_t)count;
    if (back > 1) {
        teach_back(blob, h, error, count, index, back);
        return;
    }
    std::memset(h.pred_error, 0, HOP_MAX_PRED);
    std::memcpy(h.pred_error, error, (size_t)count);
    h.pred_lines = count;
    h.pred_first = index;
    h.pred_pending = 1;
}

void blob_hop_pred_counts(const Blob& blob, uint64_t out[2]) {
    out[0] = blob.hop ? blob.hop->pred_seen : 0;
    out[1] = blob.hop ? blob.hop->pred_sent : 0;
}
