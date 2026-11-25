#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

#include <cstdlib>

// barely correct (Baldassi, Braunstein, Brunel and Zecchina 2007; the stop-learning gate of
// Brader, Senn and Fusi 2007). the readout's error rule moves nothing on good. their binary
// perceptron with hidden states has a third case: correct, but by a thin margin. then only
// the synapses that pushed the right way move, and only in their hidden state, deeper. here
// the margin is the step's vote: the pressed button's spikes against the runner-up's. on a
// good whose lead is under margin_percent of the pressed button's spikes:
//   margin_win: each pressed cell's wired, active, excitatory sources gain one counter step
//     (they helped; already wired, so nothing visible changes)
//   margin_rival 1: each runner-up cell's unwired, active, excitatory sources lose a step
//     (kept out more firmly: the 0/1-input form of the rule, hidden only)
//   margin_rival 2: each runner-up cell loses its active inputs as on bad, sign-aware, and
//     rewires (visible)
// chance per source per tick it fired: 2^-readout_shift for the winner, 2^-(readout_shift +
// margin_rival_shift) for the runner-up. a comfortable good moves nothing, as before.
//
// margin_win 2 is the stop-learning gate alone (Brader, Senn and Fusi): with readout_good 1
// the readout moves on good (the pressed button gains, the others lose) only when the vote
// was thin; a comfortable good moves nothing. returns whether the vote was thin.

void blob_mark_vote(Blob& blob, int pressed, int runner, const int* runner_cells, int count) {
    if (!blob.genome.margin_percent) {
        return;
    }
    int64_t words = (int64_t)blob.tile_count + 1;
    if (!blob.motor_runner) {
        blob.motor_runner = (uint64_t*)std::calloc((size_t)words, sizeof(uint64_t));
    }
    for (int64_t t = 0; t < words; t++) {
        blob.motor_runner[t] = 0;
    }
    for (int k = 0; k < count; k++) {
        int n = runner_cells[k];
        blob.motor_runner[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
    }
    blob.vote_pressed = pressed;
    blob.vote_runner = runner;
}

// the spike map `age` ticks before the newest
static const uint64_t* map_at(const Blob& blob, int age) {
    return blob.spikes[(blob.now - age) & (HISTORY - 1)];
}

// one cell: the winner deepens its helping sources; the runner-up pushes down by mode
static void margin_cell(Blob& blob, int n, int ages, bool winner, int shift) {
    const Genome& g = blob.genome;
    uint64_t self = 1ull << (n % TILE_NEURONS);
    int reps = g.readout_counts ? ages : 1;
    bool visible = !winner && g.margin_rival == 2;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        uint64_t kind = k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
        uint64_t allowed = k == 4 ? ~self : ~0ull;
        allowed &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        int64_t w = blob_weight_index(n, k);
        uint64_t wired = connected_bits(blob.w0[w], blob.w1[w]);
        for (int r = 0; r < reps; r++) {
            uint64_t active = 0;
            if (g.readout_counts) {
                active = map_at(blob, 1 + r)[src];
            } else {
                for (int age = 1; age <= ages; age++) {
                    active |= map_at(blob, age)[src];
                }
            }
            uint64_t chance = active & allowed & sparse_word(blob.rng, shift);
            if (!chance) {
                continue;
            }
            if (winner) {
                evidence_add(blob, n, k, chance & wired & ~kind, 0);
            } else if (g.margin_rival == 1) {
                evidence_add(blob, n, k, 0, chance & ~wired & ~kind);
            } else {
                evidence_add(blob, n, k, chance & kind, chance & ~kind);
            }
        }
    }
    if (visible) {
        evidence_rewire(blob, n);
    }
}

bool learn_margin(Blob& blob) {
    const Genome& g = blob.genome;
    bool marked = blob.motor_attached && blob.motor_acted && blob.motor_runner;
    if (!marked || !g.readout_evidence || blob.vote_pressed <= 0) {
        return true;
    }
    blob.margin_goods++;
    int64_t lead = (int64_t)blob.vote_pressed - blob.vote_runner;
    if (lead * 100 >= (int64_t)g.margin_percent * blob.vote_pressed) {
        return false;
    }
    blob.margin_thin++;
    if (g.margin_win == 2) {
        return true; // the gate only: the readout's own move on good does the work
    }
    int ages = g.readout_trace_ticks;
    ages = ages < 1 ? 1 : (ages > HISTORY - 1 ? HISTORY - 1 : ages);
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t win = g.margin_win ? blob.motor_acted[t] & blob.motor_attached[t] : 0;
        uint64_t rival = g.margin_rival ? blob.motor_runner[t] & blob.motor_attached[t] : 0;
        rival &= ~blob.motor_acted[t];
        while (win) {
            int n = t * TILE_NEURONS + __builtin_ctzll(win);
            win &= win - 1;
            margin_cell(blob, n, ages, true, g.readout_shift);
        }
        while (rival) {
            int n = t * TILE_NEURONS + __builtin_ctzll(rival);
            rival &= rival - 1;
            margin_cell(blob, n, ages, false, g.readout_shift + g.margin_rival_shift);
        }
    }
    return true;
}
