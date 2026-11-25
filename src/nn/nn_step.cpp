#include "nn/nn.hpp"

#include <bit>
#include <cstdio>

// the reference network's tick and reward: one decision per task step, sampled from a
// softmax, held as spikes on the chosen button's motor cells; one update per judged step

static uint64_t next_word(uint64_t& s) {
    s += 0x9e3779b97f4a7c15ull;
    uint64_t z = s;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

// the input cells on this tick, written by the plug
static void gather(NnBlob& nb, NnStep& st) {
    const Blob& b = nb.shell;
    st.active_count = 0;
    for (int t = 0; t < b.tile_count; t++) {
        uint64_t on = b.sense_in[t] & b.sense_attached[t] & b.sensor[t];
        while (on) {
            int n = t * TILE_NEURONS + std::countr_zero(on);
            on &= on - 1;
            int i = nb.input_of[n];
            if (i >= 0) {
                st.active[st.active_count++] = (int16_t)i;
            }
        }
    }
}

// d loss / d scores is pushed back through the network, then one optimizer step
static void learn_from(NnBlob& nb, const float* dlogits) {
    if (nb.kind == NnKind::mlp) {
        mlp_backward(nb, nb.steps[nb.head], dlogits);
    } else {
        gru_backward(nb, dlogits);
    }
    nn_update(nb);
}

static void decide(NnBlob& nb, int buttons, int perfect_move) {
    int window = nb.s.window;
    nb.head = (nb.head + 1) % window;
    nb.filled = nb.filled < window ? nb.filled + 1 : window;
    NnStep& st = nb.steps[nb.head];
    gather(nb, st);
    float logits[NN_OUT];
    if (nb.kind == NnKind::mlp) {
        mlp_forward(nb, st, logits);
    } else {
        gru_forward(nb, st, logits);
    }

    // softmax over the task's buttons
    float t = nb.s.temperature > 0 ? nb.s.temperature : 1.0f;
    float top = logits[0];
    for (int o = 1; o < buttons; o++) {
        top = logits[o] > top ? logits[o] : top;
    }
    float sum = 0;
    for (int o = 0; o < buttons; o++) {
        nb.prob[o] = nn_exp((logits[o] - top) / t);
        sum += nb.prob[o];
    }
    for (int o = 0; o < buttons; o++) {
        nb.prob[o] /= sum;
    }
    nb.buttons = buttons;

    // sample before any update, so the teaching below does not change this step's answer
    float u = (float)(next_word(nb.rng) >> 40) * (1.0f / 16777216.0f);
    int a = buttons - 1;
    float run = 0;
    for (int o = 0; o < buttons; o++) {
        run += nb.prob[o];
        if (u < run) {
            a = o;
            break;
        }
    }
    nb.action = a;
    nb.acted = true;
    nb.decisions++;

    // the labelled extra: cross-entropy on the right button
    if (nb.s.teach && nb.lr > 0 && perfect_move >= 0 && perfect_move < buttons) {
        float d[NN_OUT] = {};
        for (int o = 0; o < buttons; o++) {
            d[o] = (nb.prob[o] - (o == perfect_move ? 1.0f : 0.0f)) / t;
        }
        learn_from(nb, d);
    }
}

void nn_tick(Blob& blob, const Plug& plug, int perfect_move) {
    NnBlob& nb = nn_of(blob);
    blob.now = (blob.now + 1) & (HISTORY - 1);
    uint64_t* map = blob.spikes[blob.now];
    uint32_t surprise = 0;
    bool plugged = false;
    // sensor cells fire as the world says. it predicts nothing, so every sensed bit is a
    // surprise (the sequence row reads this)
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t sensed = blob.sense_in[t] & blob.sensor[t];
        blob.predicted[t] = 0;
        surprise += std::popcount(sensed & blob.sense_attached[t]);
        plugged = plugged || (blob.sense_attached[t] & blob.sensor[t]) != 0;
        map[t] = sensed;
    }
    map[blob.tile_count] = 0;

    int lines = plug.motor_lines;
    if (plugged && lines > 0) {
        // a step starts when the plug has counted no motor spike yet: it clears its counts
        // when it reads an answer, and this network fires every tick of a step
        bool start = true;
        for (int line = 0; line < lines; line++) {
            start = start && plug.motor_count[line] == 0;
        }
        if (start) {
            if (lines > NN_OUT && nb.decisions == 0) {
                std::fprintf(stderr,
                             "nn: %d buttons, the network has %d; the rest are never "
                             "pressed\n",
                             lines, NN_OUT);
            }
            decide(nb, lines < NN_OUT ? lines : NN_OUT, perfect_move);
        }
        if (nb.action >= 0 && nb.action < lines) {
            const int* cells = nullptr;
            int count = plug_motor_cells(plug, nb.action, &cells);
            for (int k = 0; k < count; k++) {
                map[cells[k] / TILE_NEURONS] |= 1ull << (cells[k] % TILE_NEURONS);
            }
        }
    }
    uint32_t fired = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        fired += std::popcount(map[t]);
    }
    blob.stats.tick++;
    blob.stats.fired = fired;
    blob.stats.surprise = surprise;
}

void nn_reward(Blob& blob, int valence) {
    NnBlob& nb = nn_of(blob);
    if (!nb.acted) {
        return;
    }
    nb.acted = false;
    if (nb.s.teach || nb.lr <= 0) {
        return;
    }
    // REINFORCE: loss -(r - b) log p(a), b the running average of r
    float r = (float)valence;
    float advantage = r - nb.baseline;
    nb.baseline += nb.s.baseline_rate * (r - nb.baseline);
    float t = nb.s.temperature > 0 ? nb.s.temperature : 1.0f;
    float d[NN_OUT] = {};
    for (int o = 0; o < nb.buttons; o++) {
        d[o] = advantage * (nb.prob[o] - (o == nb.action ? 1.0f : 0.0f)) / t;
    }
    learn_from(nb, d);
}
