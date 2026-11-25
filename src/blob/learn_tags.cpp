#include "blob/random_words.hpp"
#include "blob/tag_store.hpp"
#include "blob/weight_levels.hpp"

#include <cstring>

// action-feedback tags (AGREL, Roelfsema and van Ooyen 2005; AuGMEnT, Rombouts et al. 2015;
// BrainProp, Pozzi et al. 2020): credit for the inside of the sheet, from the action taken.
//
// at a verdict, the cells of the button that was pressed (the world marks them, as for the
// readout) send feedback back along their own wired inputs to the sources that were active
// in the step (the last readout_trace_ticks maps). each such edge adds the sender's sign to
// the source's feedback, flipped when the source is inhibitory (it pushed against the
// answer). a neuron whose summed feedback reaches tag_min gets a plus tag, one at -tag_min a
// minus tag: a bit each, kept until the next verdict. tag_hops 2: every tagged neuron sends
// its own sign on, one step further back, each edge with chance 2^-tag_thin. tag_mode 2: the
// other buttons' cells send the opposite sign, scaled so all buttons weigh the same.
//
// then the verdict, against a running mean of rewards (x256, as the noise rule): how far it
// is from the mean is the chance the event moves anything; better than expected moves a plus-
// tagged neuron's counters toward its active inputs (excitatory up, inhibitory down) and a
// minus-tagged one's away; worse does the reverse. chance 2^-tag_shift per active source.
// each moved neuron rewires from its counters (tag_store.cpp). sensor cells and the motor
// lattice are never tagged: the world drives the first, the readout owns the second.
//
// the feedback is computed at the verdict from the spike ring, which in these worlds is the
// tick right after the decision; a tag lasts until the next verdict.
//
// one inner rule (idea 155): the pieces of reverse replay (learn_replay.cpp) behind genome
// numbers. tag_exact: the walk's spike-exact credit. a cell's feedback goes back only along
// the edges whose source fired one slot delay before one of the cell's own spikes in the
// step (the spikes its fire step read), and a tagged neuron moves the counters of the sources
// that fired one delay before its own spikes, not of everything active in the step.
// tag_grow 0: only wired sources move. tag_counts (tag_store.cpp): which counts a neuron
// keeps. two controls: tag_shuffle gives each button cell a random role, tag_random hands
// the tags to as many neurons drawn at random among those that fired.

// the spike map `age` ticks before the newest
static const uint64_t* map_at(const Blob& blob, int age) {
    return blob.spikes[(blob.now - age) & (HISTORY - 1)];
}

// the step's window, as the readout counts it
static int window_ages(const Genome& g) {
    int ages = g.readout_trace_ticks;
    return ages < 1 ? 1 : (ages > HISTORY - 1 ? HISTORY - 1 : ages);
}

// tag_exact: the ages in the window at which neuron n fired, a bit each
static uint32_t fired_ages(const Blob& blob, int n) {
    int t = n / TILE_NEURONS, b = n % TILE_NEURONS, ages = window_ages(blob.genome);
    uint32_t m = 0;
    for (int a = 1; a <= ages; a++) {
        m |= (uint32_t)((map_at(blob, a)[t] >> b) & 1) << a;
    }
    return m;
}

// tag_exact: the sources of slot k that fired one slot delay before one of the spikes in
// `fired` (a driver older than the ring is lost)
static uint64_t exact_sources(const Blob& blob, int n, int k, uint32_t fired) {
    uint32_t src = slot_source(blob, n, k);
    int delay = blob.slot_delay[blob_weight_index(n, k)];
    uint64_t d = 0;
    for (; fired; fired &= fired - 1) {
        int older = __builtin_ctz(fired) + delay;
        d |= older < HISTORY ? map_at(blob, older)[src] : 0;
    }
    return d;
}

// neurons that may carry a tag in tile t
static uint64_t learners(const Blob& blob, int t) {
    uint64_t m = ~blob.sensor[t] & ~blob.motor[t];
    return blob.genome.tag_inh ? m : m & ~blob.inhibitory[t];
}

// one neuron sends `sign` back to its wired, active sources
static void send_back(Blob& blob, int n, int sign, const uint64_t* trace, int thin) {
    const Genome& g = blob.genome;
    uint64_t self = 1ull << (n % TILE_NEURONS);
    uint32_t fired = g.tag_exact ? fired_ages(blob, n) : 0;
    for (int k = 0; k < SLOTS; k++) {
        if (k >= LOCAL_SLOTS && !g.tag_far) {
            break;
        }
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        int64_t w = blob_weight_index(n, k);
        uint64_t from = g.tag_exact ? exact_sources(blob, n, k, fired) : trace[src];
        uint64_t reach = connected_bits(blob.w0[w], blob.w1[w]) & from;
        reach &= learners(blob, (int)src) & (k == 4 ? ~self : ~0ull);
        if (thin > 0) {
            reach &= sparse_word(blob.rng, thin);
        }
        uint64_t kind = k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
        int16_t* fb = blob.tag_feedback + (int64_t)src * TILE_NEURONS;
        while (reach) {
            int b = __builtin_ctzll(reach);
            reach &= reach - 1;
            fb[b] = (int16_t)(fb[b] + (((kind >> b) & 1) ? -sign : sign));
        }
    }
}

// every neuron of `plus` sends +1 and of `minus` -1 (words per tile)
static void send_all(Blob& blob, const uint64_t* plus, const uint64_t* minus, const uint64_t* trace,
                     int thin) {
    for (int t = 0; t < blob.tile_count; t++) {
        for (int s = 0; s < 2; s++) {
            uint64_t m = s == 0 ? plus[t] : minus[t];
            while (m) {
                int n = t * TILE_NEURONS + __builtin_ctzll(m);
                m &= m - 1;
                send_back(blob, n, s == 0 ? 1 : -1, trace, thin);
            }
        }
    }
}

// feedback at or past +-line into tag words, and the feedback cleared
static void read_tags(Blob& blob, int line, uint64_t* plus, uint64_t* minus) {
    for (int t = 0; t < blob.tile_count; t++) {
        const int16_t* fb = blob.tag_feedback + (int64_t)t * TILE_NEURONS;
        uint64_t p = 0, m = 0;
        for (int b = 0; b < TILE_NEURONS; b++) {
            p |= (uint64_t)(fb[b] >= line) << b;
            m |= (uint64_t)(fb[b] <= -line) << b;
        }
        plus[t] = p;
        minus[t] = m;
    }
    std::memset(blob.tag_feedback, 0, sizeof(int16_t) * (size_t)blob.neuron_count);
}

// the chosen button's feedback, and in tag_mode 2 the others' against it
static void tag_from_buttons(Blob& blob, const uint64_t* trace) {
    const Genome& g = blob.genome;
    int64_t pressed_cells = 0, other_cells = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t acted = blob.motor_acted[t] & blob.motor_attached[t];
        pressed_cells += __builtin_popcountll(acted);
        other_cells += __builtin_popcountll(blob.motor_attached[t] & ~acted);
    }
    int weight = 1;
    if (g.tag_mode == 2 && pressed_cells > 0) {
        weight = (int)((other_cells + pressed_cells / 2) / pressed_cells);
        weight = weight < 1 ? 1 : weight;
    }
    if (g.tag_shuffle) {
        // the control: every button cell draws its role, pressed with the pressed share of
        // cells; in tag_mode 1 only the cells drawn as pressed send
        uint32_t cells = (uint32_t)(pressed_cells + other_cells);
        for (int t = 0; t < blob.tile_count && pressed_cells > 0; t++) {
            uint64_t m = blob.motor_attached[t];
            while (m) {
                int n = t * TILE_NEURONS + __builtin_ctzll(m);
                m &= m - 1;
                bool pressed = (int64_t)rng_below(blob.rng, cells) < pressed_cells;
                if (pressed || g.tag_mode == 2) {
                    send_back(blob, n, pressed ? weight : -1, trace, 0);
                }
            }
        }
        read_tags(blob, g.tag_min < 1 ? 1 : g.tag_min, blob.tag_plus, blob.tag_minus);
        return;
    }
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t acted = blob.motor_acted[t] & blob.motor_attached[t];
        uint64_t other = g.tag_mode == 2 ? blob.motor_attached[t] & ~acted : 0;
        while (acted) {
            int n = t * TILE_NEURONS + __builtin_ctzll(acted);
            acted &= acted - 1;
            send_back(blob, n, weight, trace, 0);
        }
        while (other) {
            int n = t * TILE_NEURONS + __builtin_ctzll(other);
            other &= other - 1;
            send_back(blob, n, -1, trace, 0);
        }
    }
    read_tags(blob, g.tag_min < 1 ? 1 : g.tag_min, blob.tag_plus, blob.tag_minus);
}

// one tagged neuron: `toward` makes it fire more under this input, else less
static void move_tagged(Blob& blob, int n, bool toward, const uint64_t* trace) {
    const Genome& g = blob.genome;
    uint64_t self = 1ull << (n % TILE_NEURONS);
    uint32_t fired = g.tag_exact ? fired_ages(blob, n) : 0;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        uint64_t kind = k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
        uint64_t allowed = k == 4 ? ~self : ~0ull;
        allowed &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        if (!g.tag_grow) {
            int64_t w = blob_weight_index(n, k);
            allowed &= connected_bits(blob.w0[w], blob.w1[w]);
        }
        uint64_t from = g.tag_exact ? exact_sources(blob, n, k, fired) : trace[src];
        uint64_t active = from & allowed & sparse_word(blob.rng, g.tag_shift);
        if (!active) {
            continue;
        }
        uint64_t more = active & ~kind, less = active & kind;
        tag_add(blob, n, k, toward ? more : less, toward ? less : more);
    }
    tag_rewire(blob, n);
}

// tag_random: the same numbers of plus and minus tags, on learners drawn at random among
// those that fired in the step
static void random_tags(Blob& blob, const uint64_t* trace) {
    int64_t plus = 0, minus = 0, pool = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        plus += __builtin_popcountll(blob.tag_plus[t]);
        minus += __builtin_popcountll(blob.tag_minus[t]);
        pool += __builtin_popcountll(learners(blob, t) & trace[t]);
        blob.tag_plus[t] = blob.tag_minus[t] = 0;
    }
    int64_t want = plus + minus < pool ? plus + minus : pool;
    for (int64_t i = 0; i < want;) {
        int n = (int)rng_below(blob.rng, (uint32_t)blob.neuron_count);
        int t = n / TILE_NEURONS;
        uint64_t bit = 1ull << (n % TILE_NEURONS);
        if (!(learners(blob, t) & trace[t] & bit) || ((blob.tag_plus[t] | blob.tag_minus[t]) & bit)) {
            continue;
        }
        (i < plus ? blob.tag_plus : blob.tag_minus)[t] |= bit;
        i++;
    }
}

void learn_tags(Blob& blob, int valence) {
    const Genome& g = blob.genome;
    if (!blob.motor_attached || !blob.motor_acted) {
        return;
    }
    tag_store_make(blob);
    // the verdict against what was expected
    int32_t gap = valence * 256 - blob.tag_average;
    blob.tag_average += gap >> g.tag_avg_shift;

    // what was active in the step that chose
    int ages = window_ages(g);
    uint64_t* trace = blob.active_trace;
    for (int t = 0; t <= blob.tile_count; t++) {
        uint64_t r = 0;
        for (int age = 1; age <= ages; age++) {
            r |= blob.spikes[(blob.now - age) & (HISTORY - 1)][t];
        }
        trace[t] = r;
    }

    tag_from_buttons(blob, trace);
    if (g.tag_hops >= 2) {
        // one step further back, from every tagged neuron, thinned
        uint64_t* plus2 = blob.tag_scratch;
        uint64_t* minus2 = blob.tag_scratch + blob.tile_count + 1;
        send_all(blob, blob.tag_plus, blob.tag_minus, trace, g.tag_thin);
        read_tags(blob, 1, plus2, minus2);
        for (int t = 0; t < blob.tile_count; t++) {
            uint64_t p = blob.tag_plus[t], m = blob.tag_minus[t];
            p |= plus2[t] & ~m;
            m |= minus2[t] & ~blob.tag_plus[t];
            blob.tag_plus[t] = p & ~m;
            blob.tag_minus[t] = m & ~p;
        }
    }

    if (g.tag_random) {
        random_tags(blob, trace);
    }

    // how far from expected is the chance anything moves. tag_verdict 1: an error rule
    // instead, as the readout's: every bad moves, no good does
    int32_t size = gap < 0 ? -gap : gap;
    if (g.tag_verdict == 1 && valence > 0) {
        return;
    }
    if (g.tag_verdict == 0 && (int32_t)rng_below(blob.rng, 512) >= size) {
        return;
    }
    bool better = g.tag_verdict == 1 ? false : gap > 0;
    uint32_t moved = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        for (int s = 0; s < 2; s++) {
            uint64_t m = s == 0 ? blob.tag_plus[t] : blob.tag_minus[t];
            while (m) {
                int n = t * TILE_NEURONS + __builtin_ctzll(m);
                m &= m - 1;
                move_tagged(blob, n, better == (s == 0), trace);
                moved++;
            }
        }
    }
    blob.tag_events++;
    blob.tag_count += moved;
}
