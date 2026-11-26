#pragma once

#include "blob/blob.hpp"
#include "blob/tag_store.hpp"

// the one-hop rule's state (learn_hop.cpp), shared with its error lines (hop_lines.cpp) and
// its wide counters (hop_planes.cpp). core-only.

enum { HOP_PRED_FIRST = 64 }; // prediction error line l is wire line HOP_PRED_FIRST + l
constexpr uint32_t HOP_COIN_RING = 256; // hop_coin 3: the verdicts whose errors are kept
constexpr int HOP_KEPT = 16;            // idea 183: steps whose rings are kept (a power of two)
constexpr int HOP_PRED_HORIZONS = 4;    // idea 183: horizons whose errors can teach

struct HopState {
    uint64_t* near = nullptr; // HISTORY x (tile_count + 1) words, at the spike ring's index
    int8_t* bounds = nullptr; // per neuron 8: lowest wired, highest unwired, lowest strong,
                              // highest weak, each for excitatory and inhibitory sources
    uint8_t* fresh = nullptr; // per neuron: the bounds are up to date
    uint64_t salt = 0;
    uint64_t pressed = 0, right = 0; // the lines of the step just judged
    int lines = 0;
    int pending = 0;
    uint64_t counts[7] = {}; // see blob_hop_counts
    int32_t rate[64] = {};   // hop_teach 3: each line's running rate of being right, x65536
    // idea 164: the prediction errors waiting for the next tick, and the lines' places
    int8_t pred_error[HOP_MAX_PRED] = {};
    int pred_lines = 0, pred_pending = 0, pred_reach = 0;
    int16_t pred_x[HOP_MAX_PRED] = {}, pred_y[HOP_MAX_PRED] = {};
    // idea 178. hop_teach 4, 5: each line's spikes in the step just judged (blob_hop_votes)
    int32_t vote[64] = {};
    int votes = 0;
    // hop_coin 3: the last verdicts' errors, the lines told "higher" and those told "lower"
    uint64_t* ring_up = nullptr;
    uint64_t* ring_down = nullptr;
    uint32_t ring_count = 0;
    // hop_bits: the counters as bit planes, per neuron SLOTS x hop_bits words (plane 0 the
    // lowest bit, the last the sign); made: the row was filled from the wiring; dirty: the
    // cells whose counters moved since their last re-pick (hop_rewire_every)
    uint64_t* planes = nullptr;
    uint8_t* plane_made = nullptr;
    uint64_t* dirty = nullptr;
    int8_t* balance = nullptr; // hop_balance: per neuron, verdicts taught "more" less "less"
    // idea 183 (hop_pred.cpp). pred_first: the waiting prediction errors are of this
    // horizon; its lines are wire lines HOP_PRED_FIRST + pred_first x HOP_MAX_PRED and on
    int pred_first = 0;
    // the last HOP_KEPT steps' rings, for errors that come back more than a step later:
    // per step HISTORY spike maps then HISTORY near maps, each tile_count + 1 words
    uint64_t* kept = nullptr;
    uint8_t kept_now[HOP_KEPT] = {}; // each kept step's ring index of its last tick
    int64_t kept_steps = 0;
    // hop_coin 3 on prediction lines: the last steps' errors of each horizon
    int8_t* pred_ring = nullptr;
    uint32_t pred_ring_count[HOP_PRED_HORIZONS] = {};
    uint64_t pred_seen = 0, pred_sent = 0; // lines graded, lines that taught
};

// which sources of slot k a neuron may wire (as tag_rewire), and which are inhibitory
inline uint64_t hop_usable(const Blob& blob, int n, int k, uint32_t src) {
    uint64_t u = src == (uint32_t)blob.tile_count ? 0 : ~0ull;
    u &= k == 4 ? ~(1ull << (n % TILE_NEURONS)) : ~0ull;
    u &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
    u &= ~deep_blocked(blob, n, src);
    return u;
}

inline uint64_t hop_kind_of(const Blob& blob, int k, uint32_t src) {
    return k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
}

// the button lines' errors of the verdict just given, into line[] and sign[] (up to 64);
// returns how many. the coin controls are applied here (hop_lines.cpp)
int hop_button_errors(Blob& blob, HopState& h, int valence, int* line, int* sign);

// the prediction lines (hop_pred.cpp). the waiting prediction errors as lines after the
// first `count`, their coin controls applied; returns the new count
int hop_pred_errors(Blob& blob, HopState& h, int* line, int* sign, int count);
void hop_pred_free(HopState& h);

// hop_bits (hop_planes.cpp). how many planes the genome asks for (0: the byte counters)
int hop_plane_count(const Genome& g);
// a neuron's counters, SLOTS x planes words, made from its wiring on first use
uint64_t* hop_plane_row(Blob& blob, HopState& h, int n);
// one count up for the sources in `up`, one down for those in `down`, stopping at the
// counter's ends, in one slot's planes; returns how many moved
int hop_planes_step(uint64_t* p, int planes, uint64_t up, uint64_t down);
// the neuron wires its top counters at its born counts; true if its wiring changed
bool hop_planes_rewire(Blob& blob, HopState& h, int n);
void hop_planes_free(HopState& h);
