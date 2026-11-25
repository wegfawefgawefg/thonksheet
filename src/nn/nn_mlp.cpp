#include "nn/nn.hpp"

// mlp: inputs (bits) -> 128 relu -> one score per button. matrices are stored input-major
// ([in][out]) so a pass adds whole rows, which keeps every sum in one fixed order.

void mlp_forward(NnBlob& nb, NnStep& st, float* logits) {
    const int H = NN_HIDDEN;
    const float* w_in = nb.param + nb.w_in;
    const float* b_in = nb.param + nb.b_in;
    const float* w_out = nb.param + nb.w_out;
    const float* b_out = nb.param + nb.b_out;
    float a[NN_HIDDEN];
    for (int j = 0; j < H; j++) {
        a[j] = b_in[j];
    }
    for (int k = 0; k < st.active_count; k++) {
        const float* row = w_in + (int64_t)st.active[k] * H;
        for (int j = 0; j < H; j++) {
            a[j] += row[j];
        }
    }
    for (int j = 0; j < H; j++) {
        st.cand[j] = a[j] > 0.0f ? a[j] : 0.0f;
        st.h[j] = st.cand[j];
    }
    for (int o = 0; o < NN_OUT; o++) {
        logits[o] = b_out[o];
    }
    for (int j = 0; j < H; j++) {
        float hj = st.cand[j];
        if (hj == 0.0f) {
            continue;
        }
        const float* row = w_out + j * NN_OUT;
        for (int o = 0; o < NN_OUT; o++) {
            logits[o] += hj * row[o];
        }
    }
}

void mlp_backward(NnBlob& nb, const NnStep& st, const float* dlogits) {
    const int H = NN_HIDDEN;
    const float* w_out = nb.param + nb.w_out;
    float* g_in = nb.grad + nb.w_in;
    float* gb_in = nb.grad + nb.b_in;
    float* g_out = nb.grad + nb.w_out;
    float* gb_out = nb.grad + nb.b_out;
    for (int o = 0; o < NN_OUT; o++) {
        gb_out[o] += dlogits[o];
    }
    float dh[NN_HIDDEN];
    for (int j = 0; j < H; j++) {
        float hj = st.cand[j];
        float* grow = g_out + j * NN_OUT;
        const float* row = w_out + j * NN_OUT;
        float back = 0.0f;
        for (int o = 0; o < NN_OUT; o++) {
            grow[o] += hj * dlogits[o];
            back += row[o] * dlogits[o];
        }
        dh[j] = hj > 0.0f ? back : 0.0f;
    }
    for (int j = 0; j < H; j++) {
        gb_in[j] += dh[j];
    }
    for (int k = 0; k < st.active_count; k++) {
        float* row = g_in + (int64_t)st.active[k] * H;
        for (int j = 0; j < H; j++) {
            row[j] += dh[j];
        }
    }
}
