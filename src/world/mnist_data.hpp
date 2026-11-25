#pragma once

#include <cstdint>
#include <vector>

// MNIST digits (scripts/get_mnist.sh puts the four idx files in data/mnist/), and how a
// digit becomes bits on the sensor sheet. Plain integers; no floats.

constexpr int MNIST_SIDE = 28;
constexpr int MNIST_PIXELS = MNIST_SIDE * MNIST_SIDE;
constexpr int MNIST_CLASSES = 10;

struct MnistSet {
    int count = 0;
    std::vector<uint8_t> pixels; // count x 784, row-major, 0 is blank, 255 full ink
    std::vector<uint8_t> labels;
};

struct Mnist {
    MnistSet train, test;
};

// the digits, read once per process from $THONK_MNIST or data/mnist. null, with a message on
// stderr, if the files are missing or malformed
const Mnist* mnist_data();

// how a 28x28 grey digit becomes bits:
//   t14  2x2 pixels averaged to 14x14, on where the average is at least half ink (196 lines)
//   g14  14x14 at four grey levels: each pixel is a 2x2 group of lines, a thermometer lit at
//        averages of 1/8, 3/8, 5/8, 7/8 ink, laid as one 28x28 sheet (784 lines)
//   t28  every pixel, on at half ink (784 lines)
enum MnistCode { MNIST_T14 = 0, MNIST_G14 = 1, MNIST_T28 = 2, MNIST_CODES = 3 };
const char* mnist_code_name(int code);
bool mnist_code_from_name(const char* name, int& code);
void mnist_code_size(int code, int& w, int& h);
// one digit to w x h bits (row-major), moved right by dx and down by dy pixels of the 28x28
// image first; blank comes in at the edge
void mnist_encode(const uint8_t* pixels, int code, int dx, int dy, uint8_t* out);

// pairs (docs/results-2026-11-25-hop-real.md): a task a straight-line reader fails. two
// digits at half size (2x2 averaged, on at half ink) on one 28x28 sheet. mix 0: side by side,
// rows 7-20, the first in columns 0-13, the second in 14-27; mix 1: interleaved, the first on
// even columns and the second on odd ones, each half-size row shown twice. the answer:
// MNIST_PAIR_SUM the parity of the sum of the two labels (0 even, 1 odd); MNIST_PAIR_SAME 1
// if the two are the same class, 0 if not
enum MnistPair { MNIST_PAIR_OFF = 0, MNIST_PAIR_SUM = 1, MNIST_PAIR_SAME = 2 };
void mnist_pair_encode(const uint8_t* a, const uint8_t* b, int mix, uint8_t* out);
int mnist_pair_answer(int label_a, int label_b, int pair);
// the second digit for the digit `image` of [first, first + count) of a set, drawn from a key:
// any digit for the sum; for same, half the keys draw one of the same class, half another
int mnist_partner(const MnistSet& set, int first, int count, int image, int pair, uint64_t key);

// position i of a seeded random order of [0, n): a permutation for each seed
int mnist_order(int i, int n, uint64_t seed);
