#include "blob/blob.hpp"
#include "blob/critic.hpp"
#include "blob/tick_steps.hpp"

#include <cstdlib>

// readout_evidence (idea 58, candidate 5): evidence before commitment. each motor cell keeps
// a small hidden counter per possible source (11 slots x 64 bits, one signed byte each,
// clamped to readout_evidence_bits). the error rule moves counters instead of connections,
// one step per chance, so weak evidence adds up over many tries before anything is wired.
// after each update the cell's real connections are the sources whose counters are in the
// top of its budget (the count it was born with), all at one strength; nothing with a
// counter of zero or less is wired. counters live only for motor cells and are touched only
// on reward, so the tick does not slow.

constexpr int EVIDENCE_ROW = SLOTS * TILE_NEURONS;

// the counter row of a motor cell, made on first use: counters start at the strength the
// cell was born with, and its budget is its connection count then
static int8_t* evidence_row(Blob& blob, int n) {
    if (!blob.readout_row) {
        int rows = 0;
        blob.readout_row = (int32_t*)std::malloc(sizeof(int32_t) * (size_t)blob.neuron_count);
        for (int m = 0; m < blob.neuron_count; m++) {
            bool motor = (blob.motor[m / TILE_NEURONS] >> (m % TILE_NEURONS)) & 1;
            motor = motor || critic_cell(blob, m); // the critic's cells read the same way
            blob.readout_row[m] = motor ? rows++ : -1;
        }
        blob.readout_evidence = (int8_t*)std::calloc((size_t)rows * EVIDENCE_ROW, 1);
        blob.readout_keep = (uint16_t*)std::calloc((size_t)rows, sizeof(uint16_t));
    }
    int row = blob.readout_row[n];
    if (row < 0) {
        return nullptr;
    }
    int8_t* c = blob.readout_evidence + (size_t)row * EVIDENCE_ROW;
    if (blob.readout_keep[row] == 0) {
        for (int k = 0; k < SLOTS; k++) {
            for (int b = 0; b < TILE_NEURONS; b++) {
                c[k * TILE_NEURONS + b] = (int8_t)blob_weight(blob, n, k, b);
            }
        }
        int have = count_fan_in(blob, n);
        blob.readout_keep[row] = (uint16_t)(have > 0 ? have : 1);
    }
    return c;
}

// one step up for the bits in `up`, one down for those in `down`, within the clamp
void evidence_add(Blob& blob, int n, int slot, uint64_t up, uint64_t down) {
    int8_t* c = evidence_row(blob, n);
    if (!c) {
        return;
    }
    int bits = blob.genome.readout_evidence_bits;
    int top = (1 << ((bits < 2 ? 2 : (bits > 8 ? 8 : bits)) - 1)) - 1;
    c += slot * TILE_NEURONS;
    while (up) {
        int b = __builtin_ctzll(up);
        up &= up - 1;
        c[b] = (int8_t)(c[b] < top ? c[b] + 1 : top);
    }
    while (down) {
        int b = __builtin_ctzll(down);
        down &= down - 1;
        c[b] = (int8_t)(c[b] > -top ? c[b] - 1 : -top);
    }
}

// wire the top of the budget: the highest counters, at least 1, at most `keep` of them
void evidence_rewire(Blob& blob, int n) {
    int8_t* c = evidence_row(blob, n);
    if (!c) {
        return;
    }
    int keep = blob.readout_keep[blob.readout_row[n]];
    uint64_t self = 1ull << (n % TILE_NEURONS);
    // usable sources: a slot with a tile, not itself, no inhibitory source on a far slot
    uint64_t usable[SLOTS];
    int histogram[128] = {};
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        uint64_t u = src == (uint32_t)blob.tile_count ? 0 : ~0ull;
        u &= k == 4 ? ~self : ~0ull;
        u &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        usable[k] = u;
        for (int b = 0; b < TILE_NEURONS; b++) {
            int v = c[k * TILE_NEURONS + b];
            if (((u >> b) & 1) && v > 0) {
                histogram[v]++;
            }
        }
    }
    // the lowest counter that still keeps the count within the budget
    int cut = 127, total = 0;
    while (cut > 1 && total + histogram[cut] <= keep) {
        total += histogram[cut];
        cut--;
    }
    if (total + histogram[cut] <= keep) {
        total += histogram[cut];
    } else {
        cut++;
    }
    bool strong = blob.genome.readout_evidence_strong != 0;
    for (int k = 0; k < SLOTS; k++) {
        uint64_t wired = 0;
        for (int b = 0; b < TILE_NEURONS; b++) {
            wired |= (uint64_t)(c[k * TILE_NEURONS + b] >= cut) << b;
        }
        wired &= usable[k];
        int64_t w = blob_weight_index(n, k);
        blob.w0[w] = strong ? 0 : wired;
        blob.w1[w] = strong ? wired : 0;
    }
    blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
}
