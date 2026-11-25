#include "blob/learn_rule.hpp"

#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"
#include "blob/weight_levels.hpp"

#include <cstdio>
#include <cstdlib>

// idea 120: applying a rule table (learn_rule.hpp). for one neuron and one slot, the source
// facts are five words over the slot's 64 sources; a cube's mask is the AND of the facts it
// cares about (or their complements), then a sparse random word for its chance, then the
// counters (or the connection levels) under the mask move. neuron and moment facts pick the
// group of cubes, so a neuron whose group is empty costs nothing. it runs on the reward
// moment and on neurons' own spikes, after the tick's integrate-and-fire, never inside it.

static const uint64_t* map_at(const Blob& blob, int age) {
    return blob.spikes[(blob.now - age) & (HISTORY - 1)];
}

static const uint64_t* noise_at(const Blob& blob, int age) {
    return blob.noise_ring + (size_t)((blob.now - age) & (HISTORY - 1)) * blob.spike_stride;
}

// what a neuron's update needs from the moment
struct RuleMoment {
    bool reward;
    int window;            // reward: the source fact counts ages 1..window, one chance each
    const uint64_t* fired; // per tile, fact 0 as one word: the window's OR, or blob.recent
    int extra_shift;       // balance: other buttons' cells move less
    uint32_t up, down;     // levels or counter steps moved, for the stats
};

// the sources whose counters are above zero
static uint64_t positive_bits(const int8_t* c) {
    uint64_t w = 0;
    for (int b = 0; b < TILE_NEURONS; b++) {
        w |= (uint64_t)(c[b] > 0) << b;
    }
    return w;
}

// counters under `mask` one step toward zero
static void evidence_decay(int8_t* c, uint64_t mask) {
    while (mask) {
        int b = __builtin_ctzll(mask);
        mask &= mask - 1;
        c[b] = (int8_t)(c[b] > 0 ? c[b] - 1 : (c[b] < 0 ? c[b] + 1 : 0));
    }
}

// move the pairs in `hit` of one slot by `action`
static void rule_act(Blob& blob, int n, int k, int8_t* row, int action, uint64_t hit, int* fan,
                     RuleMoment& m) {
    if (!row) {
        // direct: the connection's level is the counter; a new one only under the cap
        uint64_t& a0 = blob.w0[blob_weight_index(n, k)];
        uint64_t& a1 = blob.w1[blob_weight_index(n, k)];
        uint64_t had = a0 | a1;
        if (action == rule_up) {
            if (*fan >= blob.genome.fan_in_max) {
                hit &= had;
            }
            m.up += __builtin_popcountll(level_up(a0, a1, hit));
        } else {
            m.down += __builtin_popcountll(level_down(a0, a1, hit));
        }
        *fan += __builtin_popcountll((a0 | a1) & ~had) - __builtin_popcountll(had & ~(a0 | a1));
        return;
    }
    if (action == rule_up) {
        evidence_add(blob, n, k, hit, 0);
        m.up += __builtin_popcountll(hit);
    } else if (action == rule_down) {
        evidence_add(blob, n, k, 0, hit);
        m.down += __builtin_popcountll(hit);
    } else {
        evidence_decay(row + k * TILE_NEURONS, hit);
        m.down += __builtin_popcountll(hit);
    }
}

// facts 3 and 4 of a slot as it stands: connected, and counter above zero (or strong)
static void slot_state(const Blob& blob, int64_t w, const int8_t* counters, int top,
                       uint64_t* fact) {
    fact[3] = blob.w0[w] | blob.w1[w];
    fact[4] = !top ? 0 : (counters ? positive_bits(counters) : blob.w1[w]);
}

// the pairs of a slot a cube applies to, before fact 0 and the chance
static uint64_t cube_mask(const RuleCube& c, const uint64_t* fact, uint64_t usable) {
    uint64_t mask = usable;
    for (int f = 1; f < RULE_SOURCE_FACTS; f++) {
        if ((c.care >> f) & 1) {
            mask &= ((c.value >> f) & 1) ? fact[f] : ~fact[f];
        }
    }
    return mask;
}

// one neuron: every slot, every cube of its group
static void rule_neuron(Blob& blob, const RuleTable& rule, int g, int n, RuleMoment& m) {
    bool direct = rule.readoff == readoff_direct;
    int8_t* row = direct ? nullptr : evidence_row(blob, n);
    if (!direct && !row) {
        return;
    }
    // the neuron's last spike, ages back: now on its own spike, in the window on a reward
    int t = n / TILE_NEURONS, bit = n % TILE_NEURONS;
    int last = m.reward ? HISTORY : 0;
    for (int a = 1; m.reward && a <= m.window; a++) {
        if ((map_at(blob, a)[t] >> bit) & 1) {
            last = a;
            break;
        }
    }
    const RuleCube* cubes = rule.cube[g];
    int count = rule.cubes[g];
    uint64_t self = 1ull << bit;
    int fan = blob.fan_in[n];
    bool moved = false;
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        int64_t w = blob_weight_index(n, k);
        uint64_t inh = blob.inhibitory[src];
        uint64_t usable = k == 4 ? ~self : ~0ull;
        usable &= k >= LOCAL_SLOTS ? ~inh : ~0ull;
        uint64_t fact[RULE_SOURCE_FACTS];
        fact[0] = m.fired[src];
        int age = last + blob.slot_delay[w];
        fact[1] = age < HISTORY ? map_at(blob, age)[src] : 0;
        fact[2] = k < LOCAL_SLOTS ? inh : 0;
        slot_state(blob, w, row ? row + k * TILE_NEURONS : nullptr, rule.needs_top[g], fact);
        for (int i = 0; i < count; i++) {
            const RuleCube& c = cubes[i];
            uint64_t mask = cube_mask(c, fact, usable);
            int shift = rule.shift[c.chance] + m.extra_shift;
            // fact 0 off: once. on, at a reward: one chance per tick the source fired
            if (!(c.value & 1) || !m.reward) {
                uint64_t hit = mask & ((c.value & 1) ? fact[0] : ~fact[0]);
                if (hit) {
                    hit &= sparse_word(blob.rng, shift);
                }
                if (hit) {
                    rule_act(blob, n, k, row, c.action, hit, &fan, m);
                    moved = true;
                }
                continue;
            }
            // each pair matched its entry at the moment; a pair that a move takes out of the
            // entry (a strong link weakened) gets no more chances from it
            uint64_t now[RULE_SOURCE_FACTS] = {fact[0], fact[1], fact[2], fact[3], fact[4]};
            for (int a = 1; mask && a <= m.window; a++) {
                uint64_t hit = mask & map_at(blob, a)[src];
                if (hit) {
                    hit &= sparse_word(blob.rng, shift);
                }
                if (hit) {
                    rule_act(blob, n, k, row, c.action, hit, &fan, m);
                    moved = true;
                    slot_state(blob, w, row ? row + k * TILE_NEURONS : nullptr, rule.needs_top[g],
                               now);
                    mask &= cube_mask(c, now, usable);
                }
            }
        }
    }
    if (!moved) {
        return;
    }
    if (direct) {
        blob.fan_in[n] = (uint16_t)fan;
    } else {
        evidence_rewire_as(blob, n, rule.readoff == readoff_graded ? 2 : 1);
    }
}

// the neurons the rule may touch in a tile
static uint64_t scope_word(const Blob& blob, int t, int scope) {
    uint64_t button = blob.motor_attached ? blob.motor_attached[t] : 0;
    uint64_t w = ~blob.sensor[t];
    return scope == 1 ? w & button : (scope == 2 ? w & ~button : w);
}

// every group of the moment (bad fixed) over the neurons of one tile. `recent` is fact 5,
// `noise` the last-spike-was-noise word for neurons that are not button cells
static void rule_tile(Blob& blob, const RuleTable& rule, int t, uint64_t neurons, int high,
                      uint64_t recent, uint64_t noise, int other_shift, RuleMoment& m) {
    uint64_t button = blob.motor_attached ? blob.motor_attached[t] : 0;
    uint64_t acted = blob.motor_acted ? blob.motor_acted[t] : 0;
    uint64_t marked = (button & acted) | (~button & noise);
    for (int low = 0; low < 8; low++) {
        int g = high | low;
        if (!rule.cubes[g]) {
            continue;
        }
        uint64_t these = neurons & ((low & 1) ? recent : ~recent);
        these &= (low & 2) ? button : ~button;
        these &= (low & 4) ? marked : ~marked;
        m.extra_shift = (low & 6) == 2 ? other_shift : 0;
        while (these) {
            int n = t * TILE_NEURONS + __builtin_ctzll(these);
            these &= these - 1;
            rule_neuron(blob, rule, g, n, m);
        }
    }
}

// balance, as learn_readout does it: the other buttons' cells move less by their number over
// the pressed button's, rounded up to a power of two
static int balance_shift(const Blob& blob) {
    if (!blob.motor_attached || !blob.motor_acted) {
        return 0;
    }
    int64_t pressed = 0, other = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t acted = blob.motor_acted[t] & blob.motor_attached[t];
        pressed += __builtin_popcountll(acted);
        other += __builtin_popcountll(blob.motor_attached[t] & ~acted);
    }
    int e = 0;
    while (pressed > 0 && (pressed << e) < other) {
        e++;
    }
    return e;
}

static void rule_reward(Blob& blob, const RuleTable& rule, int valence, int scope) {
    int window = rule.window;
    uint64_t* trace = blob.rule_words;
    for (int t = 0; t <= blob.tile_count; t++) {
        uint64_t r = 0;
        for (int a = 1; a <= window; a++) {
            r |= map_at(blob, a)[t];
        }
        trace[t] = r;
    }
    RuleMoment m{true, window, trace, 0, 0, 0};
    int other_shift = rule.balance ? balance_shift(blob) : 0;
    int high = 8 | (valence < 0 ? 16 : 0);
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t neurons = scope_word(blob, t, scope);
        // which neurons' latest spike in the window was noise
        uint64_t seen = 0, noise = 0;
        for (int a = 1; a <= window; a++) {
            uint64_t first = map_at(blob, a)[t] & ~seen;
            noise |= first & noise_at(blob, a)[t];
            seen |= first;
        }
        rule_tile(blob, rule, t, neurons, high, trace[t], noise, other_shift, m);
    }
    blob.stats.strengthened += m.up;
    blob.stats.weakened += m.down;
}

static void rule_spikes(Blob& blob, const RuleTable& rule, int scope) {
    const Genome& g = blob.genome;
    RuleMoment m{false, 0, blob.recent, 0, 0, 0};
    int high = blob.rule_last_valence < 0 ? 16 : 0;
    const uint64_t* fired = map_at(blob, 0);
    const uint64_t* noise = noise_at(blob, 0);
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t neurons = fired[t] & scope_word(blob, t, scope);
        if (!neurons) {
            continue;
        }
        // fired in the ticks before this spike
        uint64_t before = 0;
        for (int a = 1; a <= g.recent_ticks; a++) {
            before |= map_at(blob, a)[t];
        }
        rule_tile(blob, rule, t, neurons, high, before, noise[t], 0, m);
    }
    blob.stats.strengthened += m.up;
    blob.stats.weakened += m.down;
}

void learn_rule(Blob& blob, int valence, int scope) {
    const RuleTable& rule = rule_table();
    if (!rule.loaded) {
        std::fprintf(stderr, "genome rule_on is set but no --rule-file or --rule-hex was given\n");
        std::exit(1);
    }
    if (!blob.rule_words) {
        blob.rule_words = (uint64_t*)std::calloc((size_t)blob.tile_count + 1, sizeof(uint64_t));
    }
    if (valence != 0 && rule.any_reward) {
        rule_reward(blob, rule, valence, scope);
    }
    if (valence != 0) {
        blob.rule_last_valence = valence;
    }
    if (rule.any_spike) {
        rule_spikes(blob, rule, scope);
    }
}
