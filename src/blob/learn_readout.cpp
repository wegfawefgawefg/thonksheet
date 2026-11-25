#include "blob/blob.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

// learn modes 5 and 6 (idea 58): an error rule at the motor cells, the perceptron rule in
// bits. on bad, the cells of the button that was pressed lose their recently active inputs
// and the cells of the other buttons gain those same inputs; on good nothing moves (unless
// readout_good). "active" is a source's spikes over the readout_trace_ticks ticks before the
// reward, the end of the step that chose; with readout_counts each of those ticks is its own
// chance, so a source that fired often moves more often. sign-aware: gaining strengthens
// excitatory sources and weakens inhibitory ones, losing the reverse. every change is one
// level, by chance, as elsewhere.
//
// readout_center: the ridge reader centres its inputs, and so does this. a source moves by
// (its spikes this window) - (its spikes in the last rewarded window), tick by tick, which
// is zero on average, so the activity every trial shares no longer swamps the update.
//
// which cells are "pressed" and "other": with readout_know_button the world marks them
// (motor_attached, motor_acted: an efference copy of the action taken, one bit per cell).
// without it a motor cell knows only itself: "pressed" is every motor cell that fired in the
// window, "other" every motor cell that did not.

// the spike map `age` ticks before the newest
static const uint64_t* map_at(const Blob& blob, int age) {
    return blob.spikes[(blob.now - age) & (HISTORY - 1)];
}

// remove one connection whose source was idle in the window (budget payment)
static bool drop_idle(Blob& blob, int n, const uint64_t* trace) {
    int start = (int)rng_below(blob.rng, SLOTS);
    for (int j = 0; j < SLOTS; j++) {
        int k = (start + j) % SLOTS;
        int64_t w = blob_weight_index(n, k);
        uint64_t idle = connected_bits(blob.w0[w], blob.w1[w]) & ~trace[slot_source(blob, n, k)];
        if (!idle) {
            continue;
        }
        uint64_t bit = 1ull << pick_bit(blob.rng, idle);
        blob.w0[w] &= ~bit;
        blob.w1[w] &= ~bit;
        return true;
    }
    return false;
}

// wire in one weak connection from a source that was idle in the window (budget refill)
static bool add_idle(Blob& blob, int n, const uint64_t* trace) {
    uint64_t self = 1ull << (n % TILE_NEURONS);
    int start = (int)rng_below(blob.rng, SLOTS);
    for (int j = 0; j < SLOTS; j++) {
        int k = (start + j) % SLOTS;
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        int64_t w = blob_weight_index(n, k);
        uint64_t free = ~connected_bits(blob.w0[w], blob.w1[w]) & ~trace[src];
        free &= k == 4 ? ~self : ~0ull;
        free &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        if (!free) {
            continue;
        }
        blob.w0[w] |= 1ull << pick_bit(blob.rng, free);
        return true;
    }
    return false;
}

// pooling: give a motor cell both far slots, drawn as at birth (an empty one stays empty
// only if no tile fits)
static void open_far_slots(Blob& blob, int n) {
    for (int k = 0; k < FAR_SLOTS; k++) {
        uint32_t* far = &blob.far_tile[n * FAR_SLOTS + k];
        if (*far != (uint32_t)blob.tile_count) {
            continue;
        }
        draw_far_tile(blob, n / TILE_NEURONS, far,
                      &blob.slot_delay[blob_weight_index(n, LOCAL_SLOTS + k)]);
    }
}

// move one motor cell's active inputs: gain (+1) or lose (-1). returns levels moved
static uint32_t move_inputs(Blob& blob, int n, int sign, const uint64_t* trace, int ages,
                            int shift) {
    const Genome& g = blob.genome;
    int reps = g.readout_counts ? ages : 1;
    bool center = g.readout_center && g.readout_counts && blob.readout_last;
    int64_t stride = (int64_t)blob.tile_count + 1;
    bool evidence = g.readout_evidence != 0;
    uint64_t self = 1ull << (n % TILE_NEURONS);
    uint32_t moved = 0;
    int born = 0, gone = 0;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        // far slots count every source as excitatory and never wire an inhibitory one
        uint64_t kind = k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
        uint64_t allowed = k == 4 ? ~self : ~0ull;
        allowed &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        uint64_t& a0 = blob.w0[blob_weight_index(n, k)];
        uint64_t& a1 = blob.w1[blob_weight_index(n, k)];
        for (int r = 0; r < reps; r++) {
            uint64_t active = g.readout_counts ? map_at(blob, 1 + r)[src] : trace[src];
            // centred: against the same tick of the last rewarded window. a source that fired
            // now and not then moves with the sign, one that fired then and not now against
            // it, one that did the same both times not at all
            uint64_t last = center ? blob.readout_last[(int64_t)r * stride + src] : 0;
            uint64_t chance = (active ^ last) & allowed & sparse_word(blob.rng, shift);
            if (!chance) {
                continue;
            }
            uint64_t with = chance & active, against = chance & last;
            // excitatory sources go the way of the sign, inhibitory ones the other way
            uint64_t more = (with & ~kind) | (against & kind);
            uint64_t less = (with & kind) | (against & ~kind);
            uint64_t up = sign > 0 ? more : less;
            uint64_t down = sign > 0 ? less : more;
            if (evidence) {
                evidence_add(blob, n, k, up, down);
                moved += __builtin_popcountll(up | down);
                continue;
            }
            uint64_t had = connected_bits(a0, a1);
            if (!g.readout_new) {
                up &= had;
            }
            moved += __builtin_popcountll(level_up(a0, a1, up));
            moved += __builtin_popcountll(level_down(a0, a1, down));
            uint64_t now = connected_bits(a0, a1);
            born += __builtin_popcountll(now & ~had);
            gone += __builtin_popcountll(had & ~now);
        }
    }
    if (evidence) {
        evidence_rewire(blob, n);
        return moved;
    }
    // the budget: the count of connections stays where it was
    if (g.readout_budget) {
        for (; born > gone && drop_idle(blob, n, trace); born--) {
        }
        for (; gone > born && add_idle(blob, n, trace); gone--) {
        }
    }
    blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
    return moved;
}

// keep this window's maps as the next update's reference (readout_center)
static void keep_window(Blob& blob, int ages) {
    int64_t stride = (int64_t)blob.tile_count + 1;
    for (int r = 0; r < ages; r++) {
        const uint64_t* map = map_at(blob, 1 + r);
        for (int64_t t = 0; t < stride; t++) {
            blob.readout_last[(int64_t)r * stride + t] = map[t];
        }
    }
}

void learn_readout(Blob& blob, int valence) {
    const Genome& g = blob.genome;
    if (valence == 0) {
        return;
    }
    if (valence > 0 && !g.readout_good) {
        if (g.readout_center && blob.readout_last) {
            int ages = g.readout_trace_ticks;
            keep_window(blob, ages < 1 ? 1 : (ages > HISTORY - 1 ? HISTORY - 1 : ages));
        }
        return;
    }
    // what was active: the OR of the window's maps
    int ages = g.readout_trace_ticks;
    ages = ages < 1 ? 1 : (ages > HISTORY - 1 ? HISTORY - 1 : ages);
    uint64_t* trace = blob.active_trace;
    for (int t = 0; t <= blob.tile_count; t++) {
        uint64_t r = 0;
        for (int age = 1; age <= ages; age++) {
            r |= map_at(blob, age)[t];
        }
        trace[t] = r;
    }
    bool marked = g.readout_know_button && blob.motor_attached && blob.motor_acted;
    // readout_balance: the other buttons' cells move less, by their number over the
    // pressed button's (rounded up to a power of two), so with k buttons the gains do not
    // outweigh the losses k - 1 to 1. no change with two buttons
    int other_shift = g.readout_shift;
    if (g.readout_balance && marked) {
        int64_t pressed_cells = 0, other_cells = 0;
        for (int t = 0; t < blob.tile_count; t++) {
            uint64_t acted = blob.motor_acted[t] & blob.motor_attached[t];
            pressed_cells += __builtin_popcountll(acted);
            other_cells += __builtin_popcountll(blob.motor_attached[t] & ~acted);
        }
        int e = 0;
        for (; pressed_cells > 0 && (pressed_cells << e) < other_cells; e++) {
            other_shift++;
        }
        // readout_balance 2: the nearest power of two instead of the next one up, so with ten
        // buttons the nine others move at 1/8, not 1/16, and the counters do not drain (mnist)
        int64_t below = e > 0 ? pressed_cells << (e - 1) : 0;
        if (g.readout_balance == 2 && below > 0 && other_cells * other_cells < 2 * below * below) {
            other_shift--;
        }
    }
    // pressed cells move with the valence, the other cells against it
    uint32_t up = 0, down = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t pressed =
            marked ? blob.motor_acted[t] & blob.motor_attached[t] : blob.motor[t] & trace[t];
        uint64_t other =
            marked ? blob.motor_attached[t] & ~blob.motor_acted[t] : blob.motor[t] & ~trace[t];
        if (g.readout_far && marked) {
            uint64_t cells = blob.motor_attached[t];
            while (cells) {
                open_far_slots(blob, t * TILE_NEURONS + __builtin_ctzll(cells));
                cells &= cells - 1;
            }
        }
        while (pressed) {
            int n = t * TILE_NEURONS + __builtin_ctzll(pressed);
            pressed &= pressed - 1;
            (valence > 0 ? up : down) +=
                move_inputs(blob, n, valence, trace, ages, g.readout_shift);
        }
        while (other) {
            int n = t * TILE_NEURONS + __builtin_ctzll(other);
            other &= other - 1;
            (valence > 0 ? down : up) += move_inputs(blob, n, -valence, trace, ages, other_shift);
        }
    }
    if (g.readout_center && blob.readout_last) {
        keep_window(blob, ages);
    }
    blob.stats.strengthened += up;
    blob.stats.weakened += down;
}

// idea 162: the same step on another moment's maps (blob.hpp). the maps are swapped in for
// the call and put back, so nothing else sees them
void blob_readout_verdict(Blob& blob, int valence, uint64_t* const* maps, int now, int shift) {
    if (!blob.genome.learning_on || valence == 0) {
        return;
    }
    uint64_t* own[HISTORY];
    int own_now = blob.now, own_shift = blob.genome.readout_shift;
    for (int i = 0; i < HISTORY; i++) {
        own[i] = blob.spikes[i];
        blob.spikes[i] = maps[i];
    }
    blob.now = now;
    blob.genome.readout_shift = shift;
    learn_readout(blob, valence);
    for (int i = 0; i < HISTORY; i++) {
        blob.spikes[i] = own[i];
    }
    blob.now = own_now;
    blob.genome.readout_shift = own_shift;
}
