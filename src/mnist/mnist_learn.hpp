#pragma once

#include <cstdint>
#include <vector>

// The plain learners blobmnist measures the sheet against: a linear classifier (softmax
// regression by SGD) and a two-layer network (one hidden layer, plain backprop). Floats are
// fine here; this is the yardstick, not the blob.

// rows of small whole-number features (bits, grey pixels, spike counts) and their labels
struct Rows {
    int n = 0, d = 0;
    std::vector<uint8_t> v; // n x d
    std::vector<uint8_t> label;
};

void rows_add(Rows& rows, const uint8_t* x, int label);

// each feature centred and scaled over the training rows; a feature that never varies is
// dropped (scale 0). mode 0: by its own spread; 1: all by the mean spread; 2: by its own
// spread softened by the mean one, so a rare feature is not blown up
struct Scale {
    std::vector<float> mean, inv;
};
Scale scale_fit(const Rows& train, int mode = 0);

struct Linear {
    int d = 0, c = 0;
    std::vector<float> w; // (d + 1) x c, the last row the bias
};

struct TwoLayer {
    int d = 0, h = 0, c = 0;
    std::vector<float> w1, b1; // d x h, h
    std::vector<float> w2, b2; // h x c, c
};

// passes over the training rows in a seeded random order, step size lr / (1 + pass)
void linear_train(Linear& m, const Rows& train, const Scale& s, int classes, int passes, float lr,
                  uint64_t seed);
double linear_accuracy(const Linear& m, const Rows& rows, const Scale& s);

// relu hidden layer, softmax out, one example at a time
void two_layer_train(TwoLayer& m, const Rows& train, const Scale& s, int classes, int hidden,
                     int passes, float lr, uint64_t seed);
double two_layer_accuracy(const TwoLayer& m, const Rows& rows, const Scale& s);

// a seeded random order of [0, n)
void shuffled(std::vector<int>& order, int n, uint64_t seed);
// seconds on a steady clock since some fixed point
double now_seconds();
