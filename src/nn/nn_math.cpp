#include "nn/nn.hpp"

#include <bit>
#include <cmath>

// exp by range reduction and a short series, from + - * / only (the build turns off fused
// multiply-add for this folder), so both machines give the same bits
float nn_exp(float x) {
    x = x > 88.0f ? 88.0f : (x < -87.0f ? -87.0f : x);
    float k = x * 1.44269504f;
    int n = (int)(k >= 0 ? k + 0.5f : k - 0.5f);
    float r = x - (float)n * 0.693145751953125f - (float)n * 1.428606765330187e-06f;
    float p = 1.0f / 720.0f;
    p = p * r + 1.0f / 120.0f;
    p = p * r + 1.0f / 24.0f;
    p = p * r + 1.0f / 6.0f;
    p = p * r + 0.5f;
    p = p * r + 1.0f;
    p = p * r + 1.0f;
    float scale = std::bit_cast<float>((int32_t)((n + 127) << 23));
    return p * scale;
}

float nn_sigmoid(float x) {
    return 1.0f / (1.0f + nn_exp(-x));
}

float nn_tanh(float x) {
    return 2.0f / (1.0f + nn_exp(-2.0f * x)) - 1.0f;
}

// Adam (beta 0.9, 0.999, eps 1e-8, torch's defaults) or plain SGD, then clear the gradient
void nn_update(NnBlob& nb) {
    if (!nb.s.adam) {
        for (int i = 0; i < nb.params; i++) {
            nb.param[i] -= nb.lr * nb.grad[i];
            nb.grad[i] = 0.0f;
        }
        nb.updates++;
        return;
    }
    nb.adam_b1t *= 0.9f;
    nb.adam_b2t *= 0.999f;
    float c1 = 1.0f / (1.0f - nb.adam_b1t);
    float c2 = 1.0f / (1.0f - nb.adam_b2t);
    float lr = nb.lr;
    float* p = nb.param;
    float* g = nb.grad;
    float* m = nb.adam_m;
    float* v = nb.adam_v;
    for (int i = 0; i < nb.params; i++) {
        float gi = g[i];
        m[i] = 0.9f * m[i] + 0.1f * gi;
        v[i] = 0.999f * v[i] + 0.001f * gi * gi;
        p[i] -= lr * (m[i] * c1) / (std::sqrt(v[i] * c2) + 1e-8f);
        g[i] = 0.0f;
    }
    nb.updates++;
}
