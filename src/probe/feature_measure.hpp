#pragma once

// The feature probe's measurement, shared by blobprobe feature and the viewer's readability
// tint. One tile's sensor cells carry two cues, A (every other cell) and B (the rest); a
// try is a short script of A and B over a few ticks, with a reading window and a class.
// Per neuron: spikes in the reading window under each class. A tile's number combines its
// 64 neurons' gaps between the two class means, in units of their try-to-try spread, less
// what chance alone gives. 1.0 is easy to read, under 0.3 is lost.
//   which   cue A or cue B, on for the whole try (the plain case, for comparison)
//   order   A then B, or B then A; read only after both are off
//   xor     A and B each on or off; exactly one, against both or neither
//   gap     the tile blinks twice with a short or a long gap; read after the second blink,
//           the same number of ticks after it either way
//   hold    cue A or cue B for six ticks, then nothing; read hold_gap ticks later

#include "blob/blob.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

enum class Feature { which, order, order_xor, gap, hold, count };

inline const char* const feature_names[(int)Feature::count] = {"which", "order", "xor", "gap",
                                                                "hold"};

inline bool feature_from_name(const char* name, Feature& feature) {
    for (int i = 0; i < (int)Feature::count; i++) {
        if (std::strcmp(name, feature_names[i]) == 0) {
            feature = (Feature)i;
            return true;
        }
    }
    return false;
}

// cue A is every other sensor cell of the tile, cue B the rest
inline void feature_set_cues(Blob& blob, int cue_tile, bool a_on, bool b_on) {
    uint64_t cells = blob.sensor[cue_tile];
    int index = 0;
    while (cells) {
        int bit = __builtin_ctzll(cells);
        cells &= cells - 1;
        blob_sense(blob, cue_tile * TILE_NEURONS + bit, (index & 1) ? b_on : a_on);
        index++;
    }
}

// one try: which lines are on at each tick, where the reading window is, and its class
struct FeatureTry {
    int length = 0;
    int read_from = 0, read_to = 0;
    int label = 0;
    int a = 0, b = 0; // the draw: meaning depends on the feature
};

// hold_gap: how many quiet ticks pass between the cue going off and the reading (hold)
inline FeatureTry feature_draw_try(Feature feature, uint64_t random, int hold_gap) {
    FeatureTry t;
    t.a = (int)(random & 1);
    t.b = (int)((random >> 1) & 1);
    switch (feature) {
    case Feature::which:
        t.label = t.a;
        t.length = 20;
        t.read_from = 6;
        t.read_to = 20;
        break;
    case Feature::order:
        t.label = t.a;
        t.length = 30;
        t.read_from = 14;
        t.read_to = 30;
        break;
    case Feature::order_xor:
        t.label = t.a ^ t.b;
        t.length = 20;
        t.read_from = 6;
        t.read_to = 20;
        break;
    case Feature::hold:
        t.label = t.a;
        t.read_from = 6 + hold_gap;
        t.read_to = t.read_from + 8;
        t.length = t.read_to;
        break;
    case Feature::gap: {
        t.label = t.a;
        int gap = t.a ? 9 : 3;
        t.length = 26;
        t.read_from = 6 + gap + 1;
        t.read_to = t.read_from + 8;
        break;
    }
    case Feature::count:
        break;
    }
    return t;
}

inline void feature_lines_at(Feature feature, const FeatureTry& t, int tick, bool& a_on,
                             bool& b_on) {
    a_on = b_on = false;
    switch (feature) {
    case Feature::which:
        a_on = t.label == 0;
        b_on = t.label == 1;
        break;
    case Feature::order: {
        bool first = tick < 6, second = tick >= 8 && tick < 14;
        a_on = t.label == 0 ? first : second;
        b_on = t.label == 0 ? second : first;
        break;
    }
    case Feature::order_xor:
        a_on = t.a != 0;
        b_on = t.b != 0;
        break;
    case Feature::hold:
        a_on = t.label == 0 && tick < 6;
        b_on = t.label == 1 && tick < 6;
        break;
    case Feature::gap: {
        int gap = t.a ? 9 : 3;
        bool blink = tick < 3 || (tick >= 3 + gap && tick < 6 + gap);
        a_on = b_on = blink;
        break;
    }
    case Feature::count:
        break;
    }
}

// per class, per neuron: spikes in the reading window summed over tries, and squared
struct FeatureSums {
    int neurons = 0;
    std::vector<double> sum, squares;
    int64_t seen[2] = {0, 0};
};

inline void feature_sums_begin(FeatureSums& s, int neurons) {
    s = FeatureSums{};
    s.neurons = neurons;
    s.sum.assign(2 * (size_t)neurons, 0.0);
    s.squares.assign(2 * (size_t)neurons, 0.0);
}

// one finished try's spike counts (one per neuron) under its class
inline void feature_sums_add(FeatureSums& s, const int* count, int label) {
    s.seen[label]++;
    for (int n = 0; n < s.neurons; n++) {
        s.sum[(size_t)(label * s.neurons + n)] += count[n];
        s.squares[(size_t)(label * s.neurons + n)] += (double)count[n] * count[n];
    }
}

// a tile's number: its neurons' gaps combined, less what chance alone gives
inline double feature_tile_readable(const FeatureSums& s, int tile) {
    double n0 = (double)(s.seen[0] ? s.seen[0] : 1), n1 = (double)(s.seen[1] ? s.seen[1] : 1);
    double chance = 1.0 / n0 + 1.0 / n1;
    double total = 0;
    for (int k = 0; k < TILE_NEURONS; k++) {
        int n = tile * TILE_NEURONS + k;
        double m0 = s.sum[(size_t)n] / n0, m1 = s.sum[(size_t)(s.neurons + n)] / n1;
        double v0 = s.squares[(size_t)n] / n0 - m0 * m0;
        double v1 = s.squares[(size_t)(s.neurons + n)] / n1 - m1 * m1;
        double noise = (v0 + v1) / 2.0;
        if (noise > 0) {
            total += (m1 - m0) * (m1 - m0) / noise - chance;
        }
    }
    return total > 0 ? std::sqrt(total) : 0.0;
}

// add this tick's spikes in every non-sensor neuron to the try's counts (sensor cells are
// driven by the world, so they do not count)
inline void feature_count_tick(const Blob& blob, int* count) {
    for (int tile = 0; tile < blob.tile_count; tile++) {
        uint64_t spikes = blob.spikes[blob.now][tile] & ~blob.sensor[tile];
        while (spikes) {
            count[tile * TILE_NEURONS + __builtin_ctzll(spikes)]++;
            spikes &= spikes - 1;
        }
    }
}
