#include "blob/critic.hpp"
#include "blob/random_words.hpp"
#include "blob/tick_steps.hpp"

// the actor: the readout's button cells, taught by the critic's surprise instead of the
// world's good and bad, for the press made a step ago and the few before it (fading). and
// the actor's own experiments: now and then a random press.
//
// two forms (genome actor_form):
// 0 the error rule, as learn_readout: on bad the pressed button's cells lose their active
//   sources and the other buttons' cells gain them (balanced); on good nothing, unless
//   readout_good, when the pressed gain and the others lose.
// 1 reinforce: the pressed button's cells alone, gaining after good and losing after bad.

static int clamp_int(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// move the cells of one tile's word for kept step `age`
static void move_cells(Blob& blob, CriticState& c, int t, uint64_t cells, int age, int sign,
                       int shift) {
    c.touched[t] |= cells;
    for (; cells; cells &= cells - 1) {
        critic_move_cell(blob, c, t * TILE_NEURONS + __builtin_ctzll(cells), age, sign, shift);
    }
}

void actor_learn(Blob& blob, CriticState& c, int sign) {
    const Genome& g = blob.genome;
    if (!blob.motor_attached) {
        return;
    }
    int words = c.words;
    for (int t = 0; t < words; t++) {
        c.touched[t] = 0;
    }
    int ages = clamp_int(g.actor_trace_steps, 1, CRITIC_STEPS - 1);
    ages = ages < c.steps ? ages : (int)c.steps;
    for (int a = 1; a <= ages; a++) {
        const uint64_t* acted = c.acted + (std::size_t)critic_slot(c, a) * words;
        int shift = g.readout_shift + (a - 1) * g.actor_fade;
        int64_t pressed_cells = 0, other_cells = 0;
        for (int t = 0; t < blob.tile_count; t++) {
            pressed_cells += __builtin_popcountll(acted[t] & blob.motor_attached[t]);
            other_cells += __builtin_popcountll(blob.motor_attached[t] & ~acted[t]);
        }
        if (pressed_cells == 0) {
            continue; // no move that step: nothing to credit
        }
        // balance: the other buttons move less, by their number over the pressed button's
        int other_shift = shift;
        if (g.readout_balance) {
            for (int e = 0; (pressed_cells << e) < other_cells; e++) {
                other_shift++;
            }
        }
        for (int t = 0; t < blob.tile_count; t++) {
            uint64_t pressed = acted[t] & blob.motor_attached[t];
            uint64_t other = blob.motor_attached[t] & ~acted[t];
            int pressed_sign = 0, other_sign = 0;
            if (g.actor_form == 1) {
                pressed_sign = sign;
            } else if (sign < 0 || g.readout_good) {
                pressed_sign = sign;
                other_sign = -sign;
            }
            if (pressed_sign != 0) {
                move_cells(blob, c, t, pressed, a, pressed_sign, shift);
            }
            if (other_sign != 0) {
                move_cells(blob, c, t, other, a, other_sign, other_shift);
            }
        }
    }
    for (int t = 0; t < blob.tile_count; t++) {
        for (uint64_t w = c.touched[t]; w; w &= w - 1) {
            evidence_rewire(blob, t * TILE_NEURONS + __builtin_ctzll(w));
        }
    }
}

void actor_explore(Blob& blob, CriticState& c) {
    const Genome& g = blob.genome;
    c.explored = 0;
    if (g.explore_shift <= 0 || !blob.motor_attached) {
        return;
    }
    // fewer experiments where the critic thinks things are going well
    int shift = g.explore_shift;
    if (g.explore_scale > 0) {
        int above =
            (int)(((int64_t)c.level * 65536 - c.level_mean) / (65536 * (int64_t)g.explore_scale));
        shift += clamp_int(above, 0, 16);
    }
    if (!(sparse_word(blob.rng, shift) & 1)) {
        return;
    }
    // a random button: hold the last press's cells quiet for a step (another button wins),
    // or every other button's (the same one again)
    const uint64_t* acted = c.acted + (std::size_t)c.head * c.words;
    bool same = rng_next(blob.rng) & 1;
    int hold = clamp_int(c.ticks, 1, 255);
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t quiet = blob.motor_attached[t] & (same ? ~acted[t] : acted[t]);
        for (; quiet; quiet &= quiet - 1) {
            int n = t * TILE_NEURONS + __builtin_ctzll(quiet);
            blob.cooldown[n] = (uint8_t)(blob.cooldown[n] > hold ? blob.cooldown[n] : hold);
        }
    }
    c.explored = 1;
}
