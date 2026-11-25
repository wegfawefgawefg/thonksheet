#include "world/plug_babble.hpp"

// babbling and balance (ideas 60 and 61). all integer; the world's own dice, seeded from the
// task seed, so the blob's generator is untouched and a run repeats exactly.

void babble_start(Babble& bb, const BabbleSettings& settings, uint64_t task_seed) {
    bb = Babble{};
    bb.settings = settings;
    world_random_seed(bb.random, task_seed ^ 0x426162626c650000ull); // "Babble"
}

void babble_step_begin(Babble& bb, int buttons) {
    bb.button = -1;
    const BabbleSettings& s = bb.settings;
    if (s.chance <= 0 || buttons <= 0) {
        return;
    }
    // chance per 1,024 steps, x65536 so fade can scale it by the share of bad
    int64_t chance = (int64_t)s.chance * 64;
    if (s.fade) {
        chance = chance * bb.bad_share >> 16;
    }
    if (world_random_below(bb.random, 65536) < chance) {
        bb.button = world_random_below(bb.random, buttons);
    }
}

static void clear_group(const Plug& plug, Blob& blob, int line) {
    int group = plug_motor_group(plug);
    for (int k = 0; k < group; k++) {
        int n = plug.motor_neuron[line * PLUG_MAX_GROUP + k];
        blob.urge[n / TILE_NEURONS] &= ~(1ull << (n % TILE_NEURONS));
    }
}

void babble_urge(Babble& bb, const Plug& plug, Blob& blob) {
    if (bb.button < 0 || !blob.urge) {
        return;
    }
    clear_group(plug, blob, bb.button);
    int group = plug_motor_group(plug);
    for (int k = 0; k < group; k++) {
        if ((int)(world_random_word(bb.random) & 255) >= bb.settings.fire) {
            continue;
        }
        int n = plug.motor_neuron[bb.button * PLUG_MAX_GROUP + k];
        blob.urge[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
    }
}

void babble_step_end(Babble& bb, const Plug& plug, Blob& blob) {
    if (bb.button >= 0 && blob.urge) {
        clear_group(plug, blob, bb.button);
    }
}

// a running rate toward `hit` (one per step), x65536
static void rate_toward(int32_t& rate, bool hit, int shift) {
    rate += ((hit ? 65536 : 0) - rate) >> shift;
}

int babble_signal(Babble& bb, int valence, int move, int64_t right, int64_t tries) {
    const BabbleSettings& s = bb.settings;
    bb.steps++;
    if (bb.button >= 0) {
        bb.babbled++;
        bb.obeyed += move == bb.button;
        bb.window_right += right;
        bb.window_tries += tries;
    } else {
        bb.still += move < 0;
    }
    rate_toward(bb.good_rate, valence > 0, s.memory_shift);
    rate_toward(bb.bad_rate, valence < 0, s.memory_shift);
    // fade reads how the blob does on its own answers
    if (valence != 0 && bb.button < 0) {
        rate_toward(bb.bad_share, valence < 0, s.memory_shift);
    }
    bb.good_raw += valence > 0;
    bb.bad_raw += valence < 0;

    int sent = valence;
    if (s.balance && valence != 0) {
        // the commoner signal is kept with chance rarer / commoner
        int32_t mine = valence > 0 ? bb.good_rate : bb.bad_rate;
        int32_t other = valence > 0 ? bb.bad_rate : bb.good_rate;
        if (mine > other && world_random_below(bb.random, mine) >= other) {
            sent = 0;
        }
    }
    bb.good_sent += sent > 0;
    bb.bad_sent += sent < 0;
    return sent;
}

double babble_window_clean(Babble& bb, int64_t right, int64_t tries) {
    int64_t r = right - bb.window_right, n = tries - bb.window_tries;
    bb.window_right = 0;
    bb.window_tries = 0;
    return n > 0 ? (double)r / (double)n : 0.0;
}
