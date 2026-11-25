#include "nn/nn.hpp"

// gru: inputs (bits) -> a GRU of 128 -> one score per button, as torch.nn.GRU:
//   r = sig(W_r x + b_ir + U_r h + b_hr)
//   z = sig(W_z x + b_iz + U_z h + b_hz)
//   n = tanh(W_n x + b_in + r * (U_n h + b_hn))
//   h' = (1 - z) * n + z * h
// the state carries across steps and is never reset. backprop runs from the newest step's
// scores back through the last `window` steps kept in the ring.

void gru_forward(NnBlob& nb, NnStep& st, float* logits) {
    const int H = NN_HIDDEN, G = 3 * NN_HIDDEN;
    const float* w_in = nb.param + nb.w_in;
    const float* b_in = nb.param + nb.b_in;
    const float* w_rec = nb.param + nb.w_rec;
    const float* b_rec = nb.param + nb.b_rec;
    const float* w_out = nb.param + nb.w_out;
    const float* b_out = nb.param + nb.b_out;
    float ax[3 * NN_HIDDEN], ah[3 * NN_HIDDEN];
    for (int g = 0; g < G; g++) {
        ax[g] = b_in[g];
        ah[g] = b_rec[g];
    }
    for (int k = 0; k < st.active_count; k++) {
        const float* row = w_in + (int64_t)st.active[k] * G;
        for (int g = 0; g < G; g++) {
            ax[g] += row[g];
        }
    }
    for (int k = 0; k < H; k++) {
        float hk = nb.h[k];
        st.h_prev[k] = hk;
        if (hk == 0.0f) {
            continue;
        }
        const float* row = w_rec + k * G;
        for (int g = 0; g < G; g++) {
            ah[g] += hk * row[g];
        }
    }
    for (int j = 0; j < H; j++) {
        float r = nn_sigmoid(ax[j] + ah[j]);
        float z = nn_sigmoid(ax[H + j] + ah[H + j]);
        float n = nn_tanh(ax[2 * H + j] + r * ah[2 * H + j]);
        st.gate_r[j] = r;
        st.gate_z[j] = z;
        st.cand[j] = n;
        st.rec_n[j] = ah[2 * H + j];
        st.h[j] = (1.0f - z) * n + z * st.h_prev[j];
        nb.h[j] = st.h[j];
    }
    for (int o = 0; o < NN_OUT; o++) {
        logits[o] = b_out[o];
    }
    for (int j = 0; j < H; j++) {
        const float* row = w_out + j * NN_OUT;
        float hj = st.h[j];
        for (int o = 0; o < NN_OUT; o++) {
            logits[o] += hj * row[o];
        }
    }
}

void gru_backward(NnBlob& nb, const float* dlogits) {
    const int H = NN_HIDDEN, G = 3 * NN_HIDDEN;
    const float* w_rec = nb.param + nb.w_rec;
    const float* w_out = nb.param + nb.w_out;
    float* g_in = nb.grad + nb.w_in;
    float* gb_in = nb.grad + nb.b_in;
    float* g_rec = nb.grad + nb.w_rec;
    float* gb_rec = nb.grad + nb.b_rec;
    float* g_out = nb.grad + nb.w_out;
    float* gb_out = nb.grad + nb.b_out;
    int window = nb.s.window;
    const NnStep& newest = nb.steps[nb.head];

    // the scores read the newest state
    float dh[NN_HIDDEN];
    for (int o = 0; o < NN_OUT; o++) {
        gb_out[o] += dlogits[o];
    }
    for (int j = 0; j < H; j++) {
        float* grow = g_out + j * NN_OUT;
        const float* row = w_out + j * NN_OUT;
        float back = 0.0f;
        for (int o = 0; o < NN_OUT; o++) {
            grow[o] += newest.h[j] * dlogits[o];
            back += row[o] * dlogits[o];
        }
        dh[j] = back;
    }

    // back through the kept steps, newest first
    int depth = nb.filled < window ? nb.filled : window;
    for (int age = 0; age < depth; age++) {
        const NnStep& st = nb.steps[(nb.head - age + window) % window];
        float dax[3 * NN_HIDDEN], dah[3 * NN_HIDDEN], dprev[NN_HIDDEN];
        for (int j = 0; j < H; j++) {
            float r = st.gate_r[j], z = st.gate_z[j], n = st.cand[j];
            float dn = dh[j] * (1.0f - z);
            float dz = dh[j] * (st.h_prev[j] - n);
            dprev[j] = dh[j] * z;
            float dan = dn * (1.0f - n * n);
            float dr = dan * st.rec_n[j];
            float dar = dr * r * (1.0f - r);
            float daz = dz * z * (1.0f - z);
            dax[j] = dar;
            dax[H + j] = daz;
            dax[2 * H + j] = dan;
            dah[j] = dar;
            dah[H + j] = daz;
            dah[2 * H + j] = dan * r;
        }
        for (int g = 0; g < G; g++) {
            gb_in[g] += dax[g];
            gb_rec[g] += dah[g];
        }
        for (int k = 0; k < st.active_count; k++) {
            float* row = g_in + (int64_t)st.active[k] * G;
            for (int g = 0; g < G; g++) {
                row[g] += dax[g];
            }
        }
        for (int k = 0; k < H; k++) {
            float hk = st.h_prev[k];
            const float* row = w_rec + k * G;
            float back = 0.0f;
            for (int g = 0; g < G; g++) {
                back += row[g] * dah[g];
            }
            dprev[k] += back;
            if (hk == 0.0f) {
                continue;
            }
            float* grow = g_rec + k * G;
            for (int g = 0; g < G; g++) {
                grow[g] += hk * dah[g];
            }
        }
        for (int j = 0; j < H; j++) {
            dh[j] = dprev[j];
        }
    }
}
