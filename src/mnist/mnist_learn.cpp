#include "mnist/mnist_learn.hpp"

#include <chrono>
#include <cmath>

void rows_add(Rows& rows, const uint8_t* x, int label) {
    rows.v.insert(rows.v.end(), x, x + rows.d);
    rows.label.push_back((uint8_t)label);
    rows.n++;
}

Scale scale_fit(const Rows& train, int mode) {
    Scale s;
    int d = train.d;
    std::vector<double> sum((size_t)d, 0.0), sq((size_t)d, 0.0);
    for (int r = 0; r < train.n; r++) {
        const uint8_t* x = &train.v[(size_t)r * d];
        for (int j = 0; j < d; j++) {
            sum[(size_t)j] += x[j];
            sq[(size_t)j] += (double)x[j] * x[j];
        }
    }
    s.mean.assign((size_t)d, 0.0f);
    s.inv.assign((size_t)d, 0.0f);
    std::vector<double> var((size_t)d, 0.0);
    double mean_var = 0;
    for (int j = 0; j < d; j++) {
        double m = train.n ? sum[(size_t)j] / train.n : 0.0;
        var[(size_t)j] = train.n ? sq[(size_t)j] / train.n - m * m : 0.0;
        s.mean[(size_t)j] = (float)m;
        mean_var += var[(size_t)j] / (d > 0 ? d : 1);
    }
    for (int j = 0; j < d; j++) {
        double v = var[(size_t)j];
        double by = mode == 1 ? mean_var : (mode == 2 ? v + mean_var : v);
        s.inv[(size_t)j] = v > 1e-6 && by > 1e-9 ? (float)(1.0 / std::sqrt(by)) : 0.0f;
    }
    return s;
}

static void expand(const Rows& rows, int r, const Scale& s, float* out) {
    const uint8_t* x = &rows.v[(size_t)r * rows.d];
    for (int j = 0; j < rows.d; j++) {
        out[j] = ((float)x[j] - s.mean[(size_t)j]) * s.inv[(size_t)j];
    }
}

void shuffled(std::vector<int>& order, int n, uint64_t seed) {
    order.resize((size_t)n);
    for (int i = 0; i < n; i++) {
        order[(size_t)i] = i;
    }
    uint64_t z = seed * 0x9e3779b97f4a7c15ull + 1;
    for (int i = n - 1; i > 0; i--) {
        z ^= z << 13;
        z ^= z >> 7;
        z ^= z << 17;
        int j = (int)(z % (uint64_t)(i + 1));
        int t = order[(size_t)i];
        order[(size_t)i] = order[(size_t)j];
        order[(size_t)j] = t;
    }
}

double now_seconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// softmax in place; returns nothing, p sums to 1
static void softmax(float* p, int c) {
    float top = p[0];
    for (int k = 1; k < c; k++) {
        top = p[k] > top ? p[k] : top;
    }
    float sum = 0;
    for (int k = 0; k < c; k++) {
        p[k] = std::exp(p[k] - top);
        sum += p[k];
    }
    for (int k = 0; k < c; k++) {
        p[k] /= sum;
    }
}

static void linear_scores(const Linear& m, const float* x, float* out) {
    int c = m.c;
    for (int k = 0; k < c; k++) {
        out[k] = m.w[(size_t)m.d * c + k];
    }
    for (int j = 0; j < m.d; j++) {
        if (x[j] == 0.0f) {
            continue;
        }
        const float* wj = &m.w[(size_t)j * c];
        for (int k = 0; k < c; k++) {
            out[k] += x[j] * wj[k];
        }
    }
}

static int arg_max(const float* p, int c) {
    int best = 0;
    for (int k = 1; k < c; k++) {
        best = p[k] > p[best] ? k : best;
    }
    return best;
}

void linear_train(Linear& m, const Rows& train, const Scale& s, int classes, int passes, float lr,
                  uint64_t seed) {
    m.d = train.d;
    m.c = classes;
    m.w.assign((size_t)(m.d + 1) * classes, 0.0f);
    std::vector<float> x((size_t)m.d), p((size_t)classes);
    std::vector<int> order;
    for (int pass = 0; pass < passes; pass++) {
        shuffled(order, train.n, seed + (uint64_t)pass);
        float step = lr / (1.0f + pass);
        for (int r : order) {
            expand(train, r, s, x.data());
            linear_scores(m, x.data(), p.data());
            softmax(p.data(), classes);
            p[train.label[(size_t)r]] -= 1.0f; // gradient of the loss on the scores
            for (int j = 0; j < m.d; j++) {
                if (x[(size_t)j] == 0.0f) {
                    continue;
                }
                float* wj = &m.w[(size_t)j * classes];
                for (int k = 0; k < classes; k++) {
                    wj[k] -= step * p[(size_t)k] * x[(size_t)j];
                }
            }
            float* bias = &m.w[(size_t)m.d * classes];
            for (int k = 0; k < classes; k++) {
                bias[k] -= step * p[(size_t)k];
            }
        }
    }
}

double linear_accuracy(const Linear& m, const Rows& rows, const Scale& s) {
    std::vector<float> x((size_t)m.d), p((size_t)m.c);
    int right = 0;
    for (int r = 0; r < rows.n; r++) {
        expand(rows, r, s, x.data());
        linear_scores(m, x.data(), p.data());
        right += arg_max(p.data(), m.c) == rows.label[(size_t)r];
    }
    return rows.n ? (double)right / rows.n : 0.0;
}

// hidden activations (relu) and output scores
static void two_layer_forward(const TwoLayer& m, const float* x, float* hid, float* out) {
    for (int u = 0; u < m.h; u++) {
        hid[u] = m.b1[(size_t)u];
    }
    for (int j = 0; j < m.d; j++) {
        if (x[j] == 0.0f) {
            continue;
        }
        const float* wj = &m.w1[(size_t)j * m.h];
        for (int u = 0; u < m.h; u++) {
            hid[u] += x[j] * wj[u];
        }
    }
    for (int u = 0; u < m.h; u++) {
        hid[u] = hid[u] > 0 ? hid[u] : 0;
    }
    for (int k = 0; k < m.c; k++) {
        out[k] = m.b2[(size_t)k];
    }
    for (int u = 0; u < m.h; u++) {
        const float* wu = &m.w2[(size_t)u * m.c];
        for (int k = 0; k < m.c; k++) {
            out[k] += hid[u] * wu[k];
        }
    }
}

void two_layer_train(TwoLayer& m, const Rows& train, const Scale& s, int classes, int hidden,
                     int passes, float lr, uint64_t seed) {
    m.d = train.d;
    m.h = hidden;
    m.c = classes;
    m.w1.assign((size_t)m.d * hidden, 0.0f);
    m.b1.assign((size_t)hidden, 0.0f);
    m.w2.assign((size_t)hidden * classes, 0.0f);
    m.b2.assign((size_t)classes, 0.0f);
    // he initialisation from a seeded generator (sum of uniforms, roughly normal)
    uint64_t z = seed * 0x9e3779b97f4a7c15ull + 7;
    auto uniform = [&z]() {
        z ^= z << 13;
        z ^= z >> 7;
        z ^= z << 17;
        return (float)((z >> 11) * (1.0 / 9007199254740992.0)) - 0.5f;
    };
    float a1 = std::sqrt(2.0f / m.d) * 3.46f, a2 = std::sqrt(2.0f / hidden) * 3.46f;
    for (float& w : m.w1) {
        w = uniform() * a1;
    }
    for (float& w : m.w2) {
        w = uniform() * a2;
    }
    std::vector<float> x((size_t)m.d), hid((size_t)hidden), p((size_t)classes), dh((size_t)hidden);
    std::vector<int> order;
    for (int pass = 0; pass < passes; pass++) {
        shuffled(order, train.n, seed + 100 + (uint64_t)pass);
        float step = lr / (1.0f + pass);
        for (int r : order) {
            expand(train, r, s, x.data());
            two_layer_forward(m, x.data(), hid.data(), p.data());
            softmax(p.data(), classes);
            p[train.label[(size_t)r]] -= 1.0f;
            // back through the output layer
            for (int u = 0; u < hidden; u++) {
                float* wu = &m.w2[(size_t)u * classes];
                float g = 0;
                for (int k = 0; k < classes; k++) {
                    g += wu[k] * p[(size_t)k];
                    wu[k] -= step * p[(size_t)k] * hid[(size_t)u];
                }
                dh[(size_t)u] = hid[(size_t)u] > 0 ? g : 0;
            }
            for (int k = 0; k < classes; k++) {
                m.b2[(size_t)k] -= step * p[(size_t)k];
            }
            // and through the hidden layer
            for (int j = 0; j < m.d; j++) {
                if (x[(size_t)j] == 0.0f) {
                    continue;
                }
                float* wj = &m.w1[(size_t)j * hidden];
                for (int u = 0; u < hidden; u++) {
                    wj[u] -= step * dh[(size_t)u] * x[(size_t)j];
                }
            }
            for (int u = 0; u < hidden; u++) {
                m.b1[(size_t)u] -= step * dh[(size_t)u];
            }
        }
    }
}

double two_layer_accuracy(const TwoLayer& m, const Rows& rows, const Scale& s) {
    std::vector<float> x((size_t)m.d), hid((size_t)m.h), p((size_t)m.c);
    int right = 0;
    for (int r = 0; r < rows.n; r++) {
        expand(rows, r, s, x.data());
        two_layer_forward(m, x.data(), hid.data(), p.data());
        right += arg_max(p.data(), m.c) == rows.label[(size_t)r];
    }
    return rows.n ? (double)right / rows.n : 0.0;
}
