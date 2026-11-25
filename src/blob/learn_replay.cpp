#include "blob/random_words.hpp"
#include "blob/replay.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

#include <cstring>

// reverse replay (idea 62): credit walks backward through the recorded ticks.
//
// when the readout takes an error step (learn_readout.cpp: on bad the pressed button's cells
// should have been less active, the other buttons' cells more), the same sign is walked back
// through the spike ring. a walked spike is (neuron, age, sign). its drivers are the sources
// connected to it that fired at age + the slot's delay: exactly the spikes its fire step
// read. they are the next step's walked spikes, with the same sign through an excitatory
// source and the other sign through an inhibitory one. each walked spike past the motor cells
// moves its neuron's own hidden counters (replay_wire.cpp) for the sources that were active
// one delay earlier still: more active wanted means excitatory sources gain and inhibitory
// ones lose, less wanted the reverse. with replay_new the active sources that are not wired
// gain too, so the walk can grow connections. each step past the first keeps a walked spike
// with chance 2^-replay_thin, so deep credit is gentler.
//
// only spikes are walked: a cell that did not fire left nothing to walk back from, so "should
// have fired more" walks from the spikes it did make (replay_quiet: from every tick of the
// window, its active connected sources counting as "would have helped"). sensor cells are
// driven by the world and motor cells are the readout's, so neither is walked into or moved.
// the walk only follows the ring: a driver older than 15 ticks is lost (counted as clipped).
// everything is an AND of a mask with an older spike map, per walked spike per slot.

// the spike map `age` ticks before the newest
static const uint64_t* map_at(const Blob& blob, int age) {
    return blob.spikes[(blob.now - age) & (HISTORY - 1)];
}

static void clear_maps(uint64_t* maps, int64_t stride) {
    std::memset(maps, 0, sizeof(uint64_t) * HISTORY * (size_t)stride);
}

// the motor cells: the readout's error, with its sign, at every tick they fired
static void seed_motor(Blob& blob, ReplayState& r, int valence, int ages) {
    const Genome& g = blob.genome;
    int64_t stride = (int64_t)blob.tile_count + 1;
    // balance, as in the readout: the other buttons' cells are thinned by their number over
    // the pressed button's, rounded up to a power of two
    int thin = 0;
    if (g.readout_balance) {
        int64_t pressed = 0, other = 0;
        for (int t = 0; t < blob.tile_count; t++) {
            uint64_t acted = blob.motor_acted[t] & blob.motor_attached[t];
            pressed += __builtin_popcountll(acted);
            other += __builtin_popcountll(blob.motor_attached[t] & ~acted);
        }
        for (; pressed > 0 && (pressed << thin) < other; thin++) {
        }
    }
    // on bad the pressed cells should have been less active (sign 1), the others more (0)
    int pressed_sign = valence < 0 ? 1 : 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t pressed = blob.motor_acted[t] & blob.motor_attached[t];
        uint64_t other = blob.motor_attached[t] & ~blob.motor_acted[t];
        if (!(pressed | other)) {
            continue;
        }
        for (int a = 1; a <= ages; a++) {
            uint64_t fired = map_at(blob, a)[t];
            uint64_t o = g.replay_quiet ? other : other & fired;
            o &= o && thin ? sparse_word(blob.rng, thin) : ~0ull;
            uint64_t mine = pressed & fired, theirs = o;
            // the control: the same spikes, each with a sign drawn at random
            if (g.replay_shuffle && (mine | theirs)) {
                uint64_t all = mine | theirs, coin = rng_next(blob.rng);
                mine = all & coin;
                theirs = all & ~coin;
            }
            r.walk[pressed_sign][a * stride + t] |= mine;
            r.walk[1 - pressed_sign][a * stride + t] |= theirs;
        }
    }
}

// the drivers of one walked spike into `next`
static void step_back(Blob& blob, ReplayState& r, int n, int age, int sign) {
    int64_t stride = (int64_t)blob.tile_count + 1;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        int64_t w = blob_weight_index(n, k);
        int older = age + blob.slot_delay[w];
        if (older > HISTORY - 1) {
            r.clipped++;
            continue;
        }
        uint64_t drove = map_at(blob, older)[src] & connected_bits(blob.w0[w], blob.w1[w]);
        drove &= ~blob.sensor[src] & ~blob.motor[src];
        if (!drove) {
            continue;
        }
        uint64_t kind = k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
        r.next[sign][older * stride + src] |= drove & ~kind;
        r.next[1 - sign][older * stride + src] |= drove & kind;
    }
}

// one walked spike moves its neuron's counters for the sources active one delay earlier
static void teach(Blob& blob, ReplayState& r, int n, int age, int sign) {
    const Genome& g = blob.genome;
    uint64_t self = 1ull << (n % TILE_NEURONS);
    bool moved = false;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        int64_t w = blob_weight_index(n, k);
        int older = age + blob.slot_delay[w];
        if (older > HISTORY - 1) {
            continue;
        }
        uint64_t active = map_at(blob, older)[src];
        active &= k == 4 ? ~self : ~0ull;
        active &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        active &= g.replay_new ? ~0ull : connected_bits(blob.w0[w], blob.w1[w]);
        if (!active) {
            continue;
        }
        uint64_t chance = active & sparse_word(blob.rng, g.replay_shift);
        if (!chance) {
            continue;
        }
        uint64_t kind = k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
        uint64_t more = chance & ~kind, less = chance & kind;
        replay_counters_add(blob, r, n, k, sign == 0 ? more : less, sign == 0 ? less : more);
        moved = true;
    }
    if (moved) {
        r.touched[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
    }
}

// every walked spike in `maps`, after cancelling spikes named with both signs: step back
// from it (into r.next) or teach its neuron. returns how many
static int64_t each_walked(Blob& blob, ReplayState& r, uint64_t** maps, bool teaching) {
    int64_t stride = (int64_t)blob.tile_count + 1, count = 0;
    for (int a = 1; a < HISTORY; a++) {
        for (int t = 0; t < blob.tile_count; t++) {
            uint64_t& more = maps[0][a * stride + t];
            uint64_t& less = maps[1][a * stride + t];
            uint64_t both = more & less;
            r.conflicts += __builtin_popcountll(both);
            more &= ~both;
            less &= ~both;
            for (int s = 0; s < 2; s++) {
                uint64_t m = s == 0 ? more : less;
                count += __builtin_popcountll(m);
                while (m) {
                    int n = t * TILE_NEURONS + __builtin_ctzll(m);
                    if (teaching) {
                        teach(blob, r, n, a, s);
                    } else {
                        step_back(blob, r, n, a, s);
                    }
                    m &= m - 1;
                }
            }
        }
    }
    return count;
}

static void walk(Blob& blob, ReplayState& r, int valence) {
    const Genome& g = blob.genome;
    int64_t stride = (int64_t)blob.tile_count + 1;
    int depth = g.replay_depth < REPLAY_DEPTH_MAX ? g.replay_depth : REPLAY_DEPTH_MAX;
    int ages = g.readout_trace_ticks;
    ages = ages < 1 ? 1 : (ages > HISTORY - 1 ? HISTORY - 1 : ages);
    clear_maps(r.walk[0], stride);
    clear_maps(r.walk[1], stride);
    seed_motor(blob, r, valence, ages);
    r.walks++;
    for (int d = 1; d <= depth; d++) {
        clear_maps(r.next[0], stride);
        clear_maps(r.next[1], stride);
        r.walked[d - 1] += each_walked(blob, r, r.walk, false);
        // past the first step, each walked spike goes on with chance 2^-replay_thin
        if (d >= 2 && g.replay_thin > 0) {
            for (int s = 0; s < 2; s++) {
                for (int64_t i = stride; i < HISTORY * stride; i++) {
                    uint64_t& m = r.next[s][i];
                    m &= m ? sparse_word(blob.rng, g.replay_thin) : 0;
                }
            }
        }
        uint64_t* swap[2] = {r.walk[0], r.walk[1]};
        r.walk[0] = r.next[0];
        r.walk[1] = r.next[1];
        r.next[0] = swap[0];
        r.next[1] = swap[1];
        int64_t taught = each_walked(blob, r, r.walk, true);
        if (d == depth) {
            r.walked[d] += taught;
        }
    }
    // read the moved counters off into connections
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t m = r.touched[t];
        r.touched[t] = 0;
        while (m) {
            r.rewired += replay_rewire(blob, r, t * TILE_NEURONS + __builtin_ctzll(m)) ? 1 : 0;
            m &= m - 1;
        }
    }
}

// replay_until has passed: the walk stops; with replay_reset the motor cells go back to
// their born wiring and the readout's counters start over (so a fresh readout can be tried
// on the walked sheet), and with 2 all learning stops
static void stop(Blob& blob, ReplayState& r) {
    const Genome& g = blob.genome;
    r.stopped = 1;
    if (g.replay_reset < 1) {
        return;
    }
    int rows = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t m = blob.motor[t];
        rows += __builtin_popcountll(m);
        while (m) {
            int n = t * TILE_NEURONS + __builtin_ctzll(m);
            m &= m - 1;
            for (int k = 0; k < SLOTS; k++) {
                int64_t w = blob_weight_index(n, k);
                blob.w0[w] = r.born_w0[w];
                blob.w1[w] = r.born_w1[w];
            }
            blob.fan_in[n] = (uint16_t)count_fan_in(blob, n);
        }
    }
    if (blob.readout_keep) {
        std::memset(blob.readout_keep, 0, sizeof(uint16_t) * (size_t)rows);
    }
    if (g.replay_reset >= 2) {
        blob.genome.learning_on = 0;
    }
}

void learn_replay(Blob& blob, int valence) {
    const Genome& g = blob.genome;
    if (g.replay_depth <= 0 && g.replay_until <= 0) {
        return;
    }
    ReplayState* r = replay_state(blob);
    if (r->stopped) {
        return;
    }
    if (g.replay_until > 0 && (int64_t)blob.stats.tick >= g.replay_until) {
        stop(blob, *r);
        return;
    }
    if (g.replay_depth <= 0 || valence == 0 || (valence > 0 && !g.readout_good)) {
        return;
    }
    if (!blob.motor_attached || !blob.motor_acted) {
        return;
    }
    walk(blob, *r, valence);
}
