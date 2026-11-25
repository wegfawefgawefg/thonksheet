// blobmnist baselines: the plain learners on the same bits the sheet is shown, timed on this
// machine, one thread. A linear classifier, a two-layer network with 100 hidden units trained
// by backprop for one pass, and a fixed random binary projection of the bits read by the
// same linear classifier (the honest rival of a never-taught sheet plus a readout). The
// linear classifier on grey pixels is there for the usual reference number.

#include "mnist/mnist_learn.hpp"
#include "mnist/mnist_run.hpp"
#include "world/mnist_data.hpp"

#include <cstdio>
#include <cstring>

// the digits [first, first + count) of a set, encoded and moved
static void encode_rows(const MnistSet& set, int first, int count, int code, int dx, int dy,
                        Rows& rows) {
    int w = 0, h = 0;
    mnist_code_size(code, w, h);
    rows = Rows{};
    rows.d = w * h;
    std::vector<uint8_t> bits((size_t)rows.d);
    for (int i = first; i < first + count; i++) {
        mnist_encode(&set.pixels[(size_t)i * MNIST_PIXELS], code, dx, dy, bits.data());
        rows_add(rows, bits.data(), set.labels[(size_t)i]);
    }
}

static void grey_rows(const MnistSet& set, int first, int count, int dx, int dy, Rows& rows) {
    rows = Rows{};
    rows.d = MNIST_PIXELS;
    std::vector<uint8_t> px((size_t)MNIST_PIXELS);
    for (int i = first; i < first + count; i++) {
        const uint8_t* p = &set.pixels[(size_t)i * MNIST_PIXELS];
        for (int y = 0; y < MNIST_SIDE; y++) {
            for (int x = 0; x < MNIST_SIDE; x++) {
                int sx = x - dx, sy = y - dy;
                bool in = sx >= 0 && sx < MNIST_SIDE && sy >= 0 && sy < MNIST_SIDE;
                px[(size_t)(y * MNIST_SIDE + x)] = in ? p[sy * MNIST_SIDE + sx] : 0;
            }
        }
        rows_add(rows, px.data(), set.labels[(size_t)i]);
    }
}

// a fixed random binary projection: each unit hears `fan` random lines with random signs and
// fires above its median over the first training rows (no labels used)
struct Projection {
    int units = 0, fan = 0;
    std::vector<int> line;
    std::vector<int8_t> sign;
    std::vector<int> threshold;
};

static int unit_sum(const Projection& p, int u, const uint8_t* x) {
    int sum = 0;
    for (int k = 0; k < p.fan; k++) {
        sum += p.sign[(size_t)u * p.fan + k] * x[p.line[(size_t)u * p.fan + k]];
    }
    return sum;
}

static void projection_make(Projection& p, const Rows& train, int units, int fan, uint64_t seed) {
    p.units = units;
    p.fan = fan;
    std::vector<int> order;
    p.line.resize((size_t)units * fan);
    p.sign.resize((size_t)units * fan);
    uint64_t z = seed * 0x9e3779b97f4a7c15ull + 3;
    for (size_t i = 0; i < p.line.size(); i++) {
        z ^= z << 13;
        z ^= z >> 7;
        z ^= z << 17;
        p.line[i] = (int)((z >> 8) % (uint64_t)train.d);
        p.sign[i] = (z & 1) ? 1 : -1;
    }
    p.threshold.assign((size_t)units, 0);
    int sample = train.n < 2000 ? train.n : 2000;
    std::vector<int> sums((size_t)sample);
    for (int u = 0; u < units; u++) {
        for (int r = 0; r < sample; r++) {
            sums[(size_t)r] = unit_sum(p, u, &train.v[(size_t)r * train.d]);
        }
        // the median: the unit is on for about half the digits
        std::vector<int> sorted = sums;
        for (size_t a = 1; a < sorted.size(); a++) {
            int v = sorted[a];
            size_t b = a;
            for (; b > 0 && sorted[b - 1] > v; b--) {
                sorted[b] = sorted[b - 1];
            }
            sorted[b] = v;
        }
        p.threshold[(size_t)u] = sorted.empty() ? 0 : sorted[sorted.size() / 2];
    }
}

static void project_rows(const Projection& p, const Rows& in, Rows& out) {
    out = Rows{};
    out.d = p.units;
    std::vector<uint8_t> h((size_t)p.units);
    for (int r = 0; r < in.n; r++) {
        const uint8_t* x = &in.v[(size_t)r * in.d];
        for (int u = 0; u < p.units; u++) {
            h[(size_t)u] = unit_sum(p, u, x) > p.threshold[(size_t)u];
        }
        rows_add(out, h.data(), in.label[(size_t)r]);
    }
}

int mnist_baselines(int argc, char** argv) {
    const Mnist* data = mnist_data();
    if (!data) {
        return 1;
    }
    int code = 0;
    if (!mnist_code_from_name(text_flag(argc, argv, "--mnist-code", "t14"), code)) {
        std::fprintf(stderr, "blobmnist: --mnist-code is t14, g14 or t28\n");
        return 1;
    }
    bool val = int_flag(argc, argv, "--val", 0) != 0;
    int train_count = int_flag(argc, argv, "--train", val ? 50000 : 60000);
    int hidden = int_flag(argc, argv, "--hidden", 100);
    int units = int_flag(argc, argv, "--proj", 4096);
    int fan = int_flag(argc, argv, "--proj-fan", 32);
    int passes = int_flag(argc, argv, "--passes", 3);
    float lr = (float)int_flag(argc, argv, "--lr-1000", 3) / 1000.0f;
    float lr2 = (float)int_flag(argc, argv, "--lr2-1000", 3) / 1000.0f;
    uint64_t seed = (uint64_t)int_flag(argc, argv, "--seed", 1);
    bool grey = int_flag(argc, argv, "--grey", 1) != 0;
    const MnistSet& eval_set = val ? data->train : data->test;
    int eval_first = val ? 50000 : 0, eval_count = val ? 10000 : data->test.count;

    std::printf("baselines  code %s  trained on %d training digits  scored on %s (%d digits)\n",
                mnist_code_name(code), train_count, val ? "validation 50000-59999" : "test",
                eval_count);
    std::printf("%-34s %8s %8s %8s %9s\n", "learner", "test", "moved 1", "moved 2", "train s");
    Rows train, test;
    std::vector<Rows> moved(2 * SHIFT_DIRECTIONS);
    // fills train, test and the moved tests with one encoding
    auto make = [&](bool as_grey) {
        if (as_grey) {
            grey_rows(data->train, 0, train_count, 0, 0, train);
            grey_rows(eval_set, eval_first, eval_count, 0, 0, test);
        } else {
            encode_rows(data->train, 0, train_count, code, 0, 0, train);
            encode_rows(eval_set, eval_first, eval_count, code, 0, 0, test);
        }
        for (int s = 0; s < 2; s++) {
            for (int dir = 0; dir < SHIFT_DIRECTIONS; dir++) {
                int dx = 0, dy = 0;
                shift_direction(dir, s + 1, dx, dy);
                Rows& r = moved[(size_t)(s * SHIFT_DIRECTIONS + dir)];
                if (as_grey) {
                    grey_rows(eval_set, eval_first, eval_count, dx, dy, r);
                } else {
                    encode_rows(eval_set, eval_first, eval_count, code, dx, dy, r);
                }
            }
        }
    };
    auto report = [&](const char* name, double t0, auto score) {
        double secs = now_seconds() - t0;
        double acc = score(test);
        double m[2] = {0, 0};
        for (int s = 0; s < 2; s++) {
            for (int dir = 0; dir < SHIFT_DIRECTIONS; dir++) {
                m[s] += score(moved[(size_t)(s * SHIFT_DIRECTIONS + dir)]) / SHIFT_DIRECTIONS;
            }
        }
        std::printf("%-34s %8.4f %8.4f %8.4f %9.1f\n", name, acc, m[0], m[1], secs);
        std::fflush(stdout);
    };
    char name[64];
    if (grey) {
        make(true);
        Scale s = scale_fit(train);
        Linear lin;
        double t0 = now_seconds();
        linear_train(lin, train, s, MNIST_CLASSES, passes, lr, seed);
        std::snprintf(name, sizeof name, "linear, grey pixels, %d passes", passes);
        report(name, t0, [&](const Rows& r) { return linear_accuracy(lin, r, s); });
    }
    make(false);
    {
        Scale s = scale_fit(train);
        Linear lin;
        double t0 = now_seconds();
        linear_train(lin, train, s, MNIST_CLASSES, passes, lr, seed);
        std::snprintf(name, sizeof name, "linear, %d passes", passes);
        report(name, t0, [&](const Rows& r) { return linear_accuracy(lin, r, s); });
    }
    {
        Scale s = scale_fit(train);
        TwoLayer net;
        double t0 = now_seconds();
        two_layer_train(net, train, s, MNIST_CLASSES, hidden, 1, lr2, seed);
        std::snprintf(name, sizeof name, "two-layer, %d hidden, 1 pass", hidden);
        report(name, t0, [&](const Rows& r) { return two_layer_accuracy(net, r, s); });
    }
    {
        double t0 = now_seconds();
        Projection p;
        projection_make(p, train, units, fan, seed);
        Rows ptrain;
        project_rows(p, train, ptrain);
        Scale s = scale_fit(ptrain);
        Linear lin;
        linear_train(lin, ptrain, s, MNIST_CLASSES, passes, lr, seed);
        std::snprintf(name, sizeof name, "random projection %d x %d, linear", units, fan);
        report(name, t0, [&](const Rows& r) {
            Rows pr;
            project_rows(p, r, pr);
            return linear_accuracy(lin, pr, s);
        });
    }
    return 0;
}
