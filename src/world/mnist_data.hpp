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

// position i of a seeded random order of [0, n): a permutation for each seed
int mnist_order(int i, int n, uint64_t seed);
