#pragma once

#include "blob/blob.hpp"
#include "world/plug.hpp"
#include "world/task.hpp"

#include <cstdint>

// The reference learner (docs/bench.md, "One setting for everything"): an ordinary network
// held to the same rule as our designs. One size, one recipe, the same stream through the
// same plug, only good and bad, nothing changed between tasks. Floats are allowed here: it
// is a yardstick, not a candidate for the hardware.
//
//  - it sees, once per task step, the sensor cells the plug wrote on the step's first tick,
//    as bits (the plug holds a step's lines for the whole step, so that is all it shows;
//    the good and bad lines of the last reward are on in those ticks too).
//  - it gives one score per button and samples a button from a softmax (temperature 1),
//    then fires that button's motor cells on every tick of the step.
//  - it learns only from good and bad: REINFORCE with a running-average baseline, by
//    backprop, one update per reward (Adam or plain SGD), on the step just judged.
//  - mlp: inputs -> 128 relu -> buttons. gru: inputs -> GRU of 128 -> buttons, its state
//    never reset, backprop through the last `window` steps.
//  - teach 1 is the labelled extra, not the reference: cross-entropy on the right button
//    (the perfect player's move) every step that has one; good and bad are ignored.
//
// It stands behind the Blob shell like the other engines: the shell is the first member of
// NnBlob. It reads the plug for which cells belong to which button and for the step clock
// (a step starts when no motor spike has been counted yet), which is why its tick takes it.

constexpr int NN_HIDDEN = 128;
constexpr int NN_OUT = TASK_MAX_MOTORS; // buttons a task may have; a task uses the first few
constexpr int NN_WINDOW_MAX = 32;       // steps of history kept for backprop through time
constexpr int NN_MAX_INPUTS = 1024;     // sensor lattice cells

enum class NnKind { mlp, gru };

// "--n.name value" overrides one. The defaults are the one fixed setting (chosen on seeds
// 1-4 of echo, exclusive-or and delayed echo, docs/results-2026-11-25-fixed-net-reference.md)
struct NnSettings {
    int adam = 1;                // 1 Adam, 0 plain SGD
    float lr = 0.0001f;          // step size
    float temperature = 1.0f;    // softmax over the scores
    float baseline_rate = 0.01f; // running average of reward follows over about 1/this
    int window = 8;              // gru: steps of backprop through time
    int teach = 0;               // 1: told the right button (cross-entropy), not good/bad
};

// one decision's record, kept for backprop
struct NnStep {
    int active_count = 0;
    int16_t active[NN_MAX_INPUTS] = {}; // input cells on
    float h_prev[NN_HIDDEN] = {};       // gru: state before the step
    float gate_r[NN_HIDDEN] = {};
    float gate_z[NN_HIDDEN] = {};
    float cand[NN_HIDDEN] = {};  // gru: candidate n; mlp: hidden after relu
    float rec_n[NN_HIDDEN] = {}; // gru: U_n h + b_n, before the reset gate
    float h[NN_HIDDEN] = {};     // gru: state after the step; mlp: same as cand
};

struct NnBlob {
    Blob shell; // stays first
    NnKind kind = NnKind::mlp;
    NnSettings s;
    float lr = 0; // 0 for the learning-off twin

    // input cell of each neuron, -1 if not a sensor lattice cell
    int32_t* input_of = nullptr;
    int inputs = 0;

    // parameters, their gradients and Adam's two moments, one block each. offsets into it:
    // mlp  w_in [inputs][H], b_in [H], w_out [H][OUT], b_out [OUT]
    // gru  w_in [inputs][3H], b_in [3H], w_rec [H][3H], b_rec [3H], w_out, b_out
    // (gate order r, z, n, as torch)
    int params = 0;
    float* param = nullptr;
    float* grad = nullptr;
    float* adam_m = nullptr;
    float* adam_v = nullptr;
    float adam_b1t = 1, adam_b2t = 1; // beta^t, kept by multiplying
    int w_in = 0, b_in = 0, w_rec = 0, b_rec = 0, w_out = 0, b_out = 0;

    // history: steps[(head - age) mod window], count of them filled
    NnStep* steps = nullptr;
    int head = 0, filled = 0;
    float h[NN_HIDDEN] = {}; // gru state, never reset

    // the last decision
    int buttons = 0;
    float prob[NN_OUT] = {};
    int action = -1;
    bool acted = false; // a decision not yet judged
    float baseline = 0;

    uint64_t rng = 0;
    uint64_t decisions = 0, updates = 0;
};

Blob* nn_birth(const Genome& genome, NnKind kind, const NnSettings& s, uint64_t seed);
void nn_free(Blob* blob);
// one tick: decide on a step's first tick, fire the chosen button's cells every tick
void nn_tick(Blob& blob, const Plug& plug, int perfect_move);
// good (+1) or bad (-1) for the step just ended
void nn_reward(Blob& blob, int valence);
// "--n.name value" overrides one setting
void apply_nn_args(int argc, char** argv, NnSettings& s);

inline NnBlob& nn_of(Blob& blob) {
    return *reinterpret_cast<NnBlob*>(&blob);
}

// the passes (nn_mlp.cpp, nn_gru.cpp): forward one step into steps[head], fill logits;
// backward from d logits on the newest step, adding into grad
void mlp_forward(NnBlob& nb, NnStep& st, float* logits);
void mlp_backward(NnBlob& nb, const NnStep& st, const float* dlogits);
void gru_forward(NnBlob& nb, NnStep& st, float* logits);
void gru_backward(NnBlob& nb, const float* dlogits);

// shared numbers (nn_math.cpp): exp from basic float operations only, so a run gives the
// same bits on any machine
float nn_exp(float x);
float nn_sigmoid(float x);
float nn_tanh(float x);
// one optimizer step (Adam or SGD) over every parameter, then the gradient is cleared
void nn_update(NnBlob& nb);
