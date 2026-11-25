#include "blob/critic.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"

#include <cstdlib>

// the critic: what the step's activity was, how high the critic cells fired, the surprise,
// and the critic's own learning. see critic.hpp.

constexpr int CRITIC_WINDOW = HISTORY - 1; // ticks of a step counted, at most

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// the critic cells: the motor region's cells at half the motor stride both ways, between
// the buttons' lattice cells. none when the stride is 1 (every cell there is a motor cell).
// critic_wide: every cell of the motor region that is not a motor cell (3 in 4 of them).
// with critic_opponent, every other block of the motor lattice (a checkerboard) counts against
static void mark_cells(const Blob& blob, uint64_t* cells, uint64_t* minus) {
    const Genome& g = blob.genome;
    int s = g.motor_stride, off = g.motor_stride / 2;
    if (s < 2) {
        return;
    }
    for (int y = g.motor_y; y < g.motor_y + g.motor_h && y < blob.tiles_h * 8; y++) {
        for (int x = g.motor_x; x < g.motor_x + g.motor_w && x < blob.tiles_w * 8; x++) {
            bool between = (x - g.motor_x) % s == off && (y - g.motor_y) % s == off;
            if (!between && !g.critic_wide) {
                continue;
            }
            int n = blob_neuron_at(blob, x, y);
            uint64_t bit = 1ull << (n % TILE_NEURONS);
            int t = n / TILE_NEURONS;
            if ((blob.sensor[t] | blob.motor[t]) & bit) {
                continue;
            }
            cells[t] |= bit;
            bool odd = (((x - g.motor_x) / s + (y - g.motor_y) / s) & 1) != 0;
            minus[t] |= g.critic_opponent && odd ? bit : 0;
        }
    }
}

CriticState& critic_state(Blob& blob) {
    if (blob.critic) {
        return *blob.critic;
    }
    CriticState* c = new CriticState{};
    c->words = blob.tile_count + 1;
    size_t w = (size_t)c->words;
    c->cells = (uint64_t*)std::calloc(w, sizeof(uint64_t));
    c->planes = (uint64_t*)std::calloc(w * CRITIC_STEPS * CRITIC_PLANES, sizeof(uint64_t));
    c->acted = (uint64_t*)std::calloc(w * CRITIC_STEPS, sizeof(uint64_t));
    c->touched = (uint64_t*)std::calloc(w, sizeof(uint64_t));
    c->minus = (uint64_t*)std::calloc(w, sizeof(uint64_t));
    mark_cells(blob, c->cells, c->minus);
    blob.critic = c;
    // the evidence rows are made once, for motor and critic cells together; rows made
    // before the critic existed would leave its cells out, so they are made again
    if (blob.readout_row) {
        std::free(blob.readout_row);
        std::free(blob.readout_evidence);
        std::free(blob.readout_keep);
        blob.readout_row = nullptr;
        blob.readout_evidence = nullptr;
        blob.readout_keep = nullptr;
    }
    return *c;
}

void critic_free(Blob& blob) {
    CriticState* c = blob.critic;
    if (!c) {
        return;
    }
    std::free(c->cells);
    std::free(c->planes);
    std::free(c->acted);
    std::free(c->touched);
    std::free(c->minus);
    delete c;
    blob.critic = nullptr;
}

// every neuron's spikes over the last `ticks` ticks as a 4-bit count, one bit plane each
static void count_planes(const Blob& blob, int ticks, int words, uint64_t* planes) {
    for (int t = 0; t < words; t++) {
        uint64_t c0 = 0, c1 = 0, c2 = 0, c3 = 0;
        for (int age = 0; age < ticks; age++) {
            uint64_t carry = blob.spikes[(blob.now - age) & (HISTORY - 1)][t];
            uint64_t s = c0 ^ carry;
            carry &= c0;
            c0 = s;
            s = c1 ^ carry;
            carry &= c1;
            c1 = s;
            s = c2 ^ carry;
            carry &= c2;
            c2 = s;
            c3 ^= carry;
        }
        planes[t] = c0;
        planes[words + t] = c1;
        planes[2 * words + t] = c2;
        planes[3 * words + t] = c3;
    }
}

// the critic cells' spikes, from a step's count planes; the opponent half's count against
static int level_of(const CriticState& c, const uint64_t* planes) {
    int level = 0;
    for (int p = 0; p < CRITIC_PLANES; p++) {
        for (int t = 0; t < c.words; t++) {
            uint64_t plane = planes[p * c.words + t];
            level += __builtin_popcountll(plane & c.cells[t] & ~c.minus[t]) << p;
            level -= __builtin_popcountll(plane & c.minus[t]) << p;
        }
    }
    return level;
}

int critic_level(Blob& blob) {
    CriticState& c = critic_state(blob);
    uint64_t* scratch = (uint64_t*)std::calloc((size_t)c.words * CRITIC_PLANES, sizeof(uint64_t));
    count_planes(blob, CRITIC_WINDOW, c.words, scratch);
    int level = level_of(c, scratch);
    std::free(scratch);
    return level;
}

void critic_move_cell(Blob& blob, CriticState& c, int n, int age, int sign, int shift) {
    const uint64_t* planes = c.planes + (size_t)critic_slot(c, age) * CRITIC_PLANES * c.words;
    uint64_t self = 1ull << (n % TILE_NEURONS);
    for (int k = 0; k < SLOTS; k++) {
        uint32_t src = slot_source(blob, n, k);
        if (src == (uint32_t)blob.tile_count) {
            continue;
        }
        // as the readout: far slots count every source as excitatory and never wire an
        // inhibitory one; "more" strengthens excitatory sources and weakens inhibitory ones
        uint64_t kind = k < LOCAL_SLOTS ? blob.inhibitory[src] : 0;
        uint64_t allowed = k == 4 ? ~self : ~0ull;
        allowed &= k >= LOCAL_SLOTS ? ~blob.inhibitory[src] : ~0ull;
        // plane p is worth 2^p spikes, so it moves with 2^p times the chance
        for (int p = 0; p < CRITIC_PLANES; p++) {
            uint64_t bits = planes[p * c.words + src] & allowed;
            if (!bits) {
                continue;
            }
            uint64_t hit = bits & sparse_word(blob.rng, shift - p);
            if (!hit) {
                continue;
            }
            uint64_t more = hit & ~kind, less = hit & kind;
            evidence_add(blob, n, k, sign > 0 ? more : less, sign > 0 ? less : more);
        }
    }
}

// the critic learns from the surprise: the steps before this one led somewhere better (or
// worse) than its level said, so their active sources gain (or lose) evidence in every
// critic cell, fading with each step further back
static void critic_learn(Blob& blob, CriticState& c, int sign) {
    const Genome& g = blob.genome;
    int ages = clamp_int(g.critic_trace_steps, 1, CRITIC_STEPS - 1);
    ages = ages < c.steps ? ages : (int)c.steps;
    for (int t = 0; t < blob.tile_count; t++) {
        for (uint64_t cells = c.cells[t]; cells; cells &= cells - 1) {
            int bit = __builtin_ctzll(cells);
            int n = t * TILE_NEURONS + bit;
            int s = ((c.minus[t] >> bit) & 1) ? -sign : sign;
            for (int a = 1; a <= ages; a++) {
                critic_move_cell(blob, c, n, a, s, g.critic_shift + (a - 1) * g.critic_fade);
            }
            evidence_rewire(blob, n);
        }
    }
}

bool critic_step(Blob& blob, int valence) {
    const Genome& g = blob.genome;
    if (!g.critic_on || !g.learning_on) {
        return false;
    }
    CriticState& c = critic_state(blob);
    c.ticks = (int)(blob.stats.tick - c.last_tick);
    c.last_tick = blob.stats.tick;

    // this step's activity and press become the newest kept step
    c.head = (c.head + 1) % CRITIC_STEPS;
    uint64_t* planes = c.planes + (size_t)c.head * CRITIC_PLANES * c.words;
    count_planes(blob, clamp_int(c.ticks, 1, CRITIC_WINDOW), c.words, planes);
    uint64_t* acted = c.acted + (size_t)c.head * c.words;
    for (int t = 0; t < c.words; t++) {
        acted[t] = blob.motor_acted ? blob.motor_acted[t] : 0;
    }
    c.vote[c.head][0] = blob.vote_pressed;
    c.vote[c.head][1] = blob.vote_runner;
    c.level_before = c.level;
    c.level = level_of(c, planes);

    // the step before is judged now: its reward, and how good the place it led to looks
    c.sign = 0;
    if (c.steps >= 1) {
        int64_t r = (int64_t)c.valence_before * 65536 - (g.critic_center ? c.reward_mean : 0);
        c.surprise = (int)((g.critic_reward * r) / 65536) + c.level - c.level_before;
        c.surprise = c.oracle ? g.critic_reward * c.valence_before : c.surprise;
        c.sign = c.surprise > g.critic_margin ? 1 : (c.surprise < -g.critic_margin ? -1 : 0);
        if (c.sign != 0) {
            critic_learn(blob, c, c.sign);
            actor_learn(blob, c, c.sign);
        }
        // the inner rule of modes 0, 1 and 6 hears the surprise too
        if (c.sign > 0) {
            blob.good_ticks = g.reward_ticks;
        } else if (c.sign < 0) {
            blob.bad_ticks = g.reward_ticks;
        }
    }
    int shift = clamp_int(g.critic_average_shift, 0, 16);
    c.reward_mean += ((int64_t)valence * 65536 - c.reward_mean) >> shift;
    c.level_mean += ((int64_t)c.level * 65536 - c.level_mean) >> shift;
    c.valence_before = valence;
    c.steps++;
    actor_explore(blob, c);
    return true;
}
