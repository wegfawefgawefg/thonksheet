#include "mnist/mnist_run.hpp"

// the control for the inside-learning test (docs/results-2026-11-25-compete.md): a life on
// digits whose pixels are moved by a permutation drawn fresh for every digit. the sheet sees
// the same amount of ink, held for the same ticks, but no shape that comes back. done on
// the blob's sensor bits after the plug writes them, so the world code is untouched.

static uint64_t split_mix(uint64_t& state) {
    uint64_t z = (state += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

void shuffle_draw(Sheet& s) {
    const Plug& plug = s.tt->plug;
    if (s.shuffle_cells.empty()) {
        // the bitmap's cells, line by line (the digit's pixels)
        int group = plug_sensor_group(plug);
        for (int line = plug.first_sheet; line < plug.first_number; line++) {
            for (int k = 0; k < group; k++) {
                s.shuffle_cells.push_back(plug.sensor_neuron[line * PLUG_MAX_GROUP + k]);
            }
        }
        s.shuffle_rng = s.flags.seed * 0x2545f4914f6cdd1dull + 77;
    }
    int n = (int)s.shuffle_cells.size();
    s.shuffle_perm.resize((size_t)n);
    for (int i = 0; i < n; i++) {
        s.shuffle_perm[(size_t)i] = i;
    }
    for (int i = n - 1; i > 0; i--) {
        int j = (int)(split_mix(s.shuffle_rng) % (uint64_t)(i + 1));
        int t = s.shuffle_perm[(size_t)i];
        s.shuffle_perm[(size_t)i] = s.shuffle_perm[(size_t)j];
        s.shuffle_perm[(size_t)j] = t;
    }
}

void shuffle_apply(Sheet& s) {
    Blob& b = *s.blob;
    int n = (int)s.shuffle_cells.size();
    static std::vector<uint8_t> on;
    on.resize((size_t)n);
    for (int i = 0; i < n; i++) {
        int cell = s.shuffle_cells[(size_t)i];
        on[(size_t)i] = (uint8_t)((b.sense_in[cell / TILE_NEURONS] >> (cell % TILE_NEURONS)) & 1);
    }
    for (int i = 0; i < n; i++) {
        int cell = s.shuffle_cells[(size_t)s.shuffle_perm[(size_t)i]];
        uint64_t bit = 1ull << (cell % TILE_NEURONS);
        uint64_t& w = b.sense_in[cell / TILE_NEURONS];
        w = on[(size_t)i] ? w | bit : w & ~bit;
    }
}
