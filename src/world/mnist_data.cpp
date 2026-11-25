#include "world/mnist_data.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

// idx files: a big-endian header (magic, counts, sizes), then the bytes

static uint32_t big_endian(const uint8_t* p) {
    return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

static bool read_file(const char* path, std::vector<uint8_t>& out) {
    FILE* f = std::fopen(path, "rb");
    if (!f) {
        return false;
    }
    std::fseek(f, 0, SEEK_END);
    long size = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    out.resize(size > 0 ? (size_t)size : 0);
    bool ok = size > 0 && std::fread(out.data(), 1, out.size(), f) == out.size();
    std::fclose(f);
    return ok;
}

static bool load_set(const char* dir, const char* images, const char* labels, MnistSet& set) {
    char path[1024];
    std::vector<uint8_t> img, lab;
    std::snprintf(path, sizeof path, "%s/%s", dir, images);
    if (!read_file(path, img) || img.size() < 16 || big_endian(&img[0]) != 0x803 ||
        big_endian(&img[8]) != MNIST_SIDE || big_endian(&img[12]) != MNIST_SIDE) {
        std::fprintf(stderr, "mnist: cannot read %s (run scripts/get_mnist.sh)\n", path);
        return false;
    }
    std::snprintf(path, sizeof path, "%s/%s", dir, labels);
    if (!read_file(path, lab) || lab.size() < 8 || big_endian(&lab[0]) != 0x801) {
        std::fprintf(stderr, "mnist: cannot read %s (run scripts/get_mnist.sh)\n", path);
        return false;
    }
    int count = (int)big_endian(&img[4]);
    if ((int)big_endian(&lab[4]) != count || img.size() != 16 + (size_t)count * MNIST_PIXELS ||
        lab.size() != 8 + (size_t)count) {
        std::fprintf(stderr, "mnist: %s and its labels do not match\n", images);
        return false;
    }
    set.count = count;
    set.pixels.assign(img.begin() + 16, img.end());
    set.labels.assign(lab.begin() + 8, lab.end());
    return true;
}

const Mnist* mnist_data() {
    static Mnist data;
    static int state = 0; // 0 not tried, 1 loaded, -1 failed
    if (state == 0) {
        const char* dir = std::getenv("THONK_MNIST");
        dir = dir && dir[0] ? dir : "data/mnist";
        bool ok = load_set(dir, "train-images-idx3-ubyte", "train-labels-idx1-ubyte", data.train) &&
                  load_set(dir, "t10k-images-idx3-ubyte", "t10k-labels-idx1-ubyte", data.test);
        state = ok ? 1 : -1;
    }
    return state == 1 ? &data : nullptr;
}

static const char* const code_names[MNIST_CODES] = {"t14", "g14", "t28"};

const char* mnist_code_name(int code) {
    return code >= 0 && code < MNIST_CODES ? code_names[code] : "?";
}

bool mnist_code_from_name(const char* name, int& code) {
    for (int i = 0; i < MNIST_CODES; i++) {
        if (std::strcmp(name, code_names[i]) == 0) {
            code = i;
            return true;
        }
    }
    return false;
}

void mnist_code_size(int code, int& w, int& h) {
    w = h = code == MNIST_T14 ? 14 : 28;
}

// the pixel at (x, y) of the moved digit
static int moved(const uint8_t* pixels, int dx, int dy, int x, int y) {
    x -= dx;
    y -= dy;
    bool inside = x >= 0 && x < MNIST_SIDE && y >= 0 && y < MNIST_SIDE;
    return inside ? pixels[y * MNIST_SIDE + x] : 0;
}

void mnist_encode(const uint8_t* pixels, int code, int dx, int dy, uint8_t* out) {
    if (code == MNIST_T28) {
        for (int y = 0; y < MNIST_SIDE; y++) {
            for (int x = 0; x < MNIST_SIDE; x++) {
                out[y * MNIST_SIDE + x] = moved(pixels, dx, dy, x, y) >= 128;
            }
        }
        return;
    }
    for (int py = 0; py < 14; py++) {
        for (int px = 0; px < 14; px++) {
            // sum of the 2x2 block, 0..1020
            int sum = 0;
            for (int k = 0; k < 4; k++) {
                sum += moved(pixels, dx, dy, 2 * px + (k & 1), 2 * py + (k >> 1));
            }
            if (code == MNIST_T14) {
                out[py * 14 + px] = sum >= 512;
                continue;
            }
            // thermometer: line k of the pixel's 2x2 group is on at (2k + 1) / 8 ink
            for (int k = 0; k < 4; k++) {
                int line = (2 * py + (k >> 1)) * 28 + 2 * px + (k & 1);
                out[line] = sum * 8 >= (2 * k + 1) * 1020;
            }
        }
    }
}

// a 4-round feistel permutation of the next even power of two at or above n, walked until
// it lands inside [0, n)
static uint32_t feistel(uint32_t v, int half, uint64_t seed) {
    uint32_t mask = (1u << half) - 1;
    uint32_t left = v >> half, right = v & mask;
    for (int round = 0; round < 4; round++) {
        uint64_t z = (uint64_t)right * 0x9e3779b97f4a7c15ull + seed +
                     (uint64_t)round * 0xbf58476d1ce4e5b9ull;
        z = (z ^ (z >> 31)) * 0x94d049bb133111ebull;
        z ^= z >> 29;
        uint32_t next = left ^ ((uint32_t)z & mask);
        left = right;
        right = next;
    }
    return left << half | right;
}

int mnist_order(int i, int n, uint64_t seed) {
    int half = 1;
    while ((1ll << (2 * half)) < n) {
        half++;
    }
    uint32_t v = (uint32_t)i;
    do {
        v = feistel(v, half, seed);
    } while ((int)v >= n);
    return (int)v;
}
