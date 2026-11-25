// blobprobe feature: does the sheet hold features nobody designed in?
//
// The carry probe asks whether "which cue is on" can be read from a tile's neurons. This
// asks the same of things no single input line carries, with no reward anywhere:
//   which   cue A or cue B, on for the whole try (the plain case, for comparison)
//   order   A then B, or B then A; read only after both are off
//   xor     A and B each on or off; exactly one, against both or neither
//   gap     the tile blinks twice with a short or a long gap; read after the second blink,
//           the same number of ticks after it either way
//   hold    cue A or cue B for six ticks, then nothing; read --hold-gap ticks later. How
//           long the sheet keeps a thing in mind
// For every neuron: the gap between its mean spikes in the reading window under the two
// classes, in units of its try-to-try spread. A tile's number combines its 64 neurons, less
// what chance alone gives. 1.0 is easy to read, under 0.3 is lost. It is what an ideal
// reader of that tile could get, so it is an upper bound on what any readout can learn.

#include "blob/blob.hpp"
#include "probe/linear_fit.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

enum class Feature { which, order, order_xor, gap, hold };

// hold: how many quiet ticks pass between the cue going off and the reading
static int hold_gap = 20;

// cue A is every other sensor cell of the tile, cue B the rest
static void set_cues(Blob& blob, int cue_tile, bool a_on, bool b_on) {
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
struct Try {
    int length = 0;
    int read_from = 0, read_to = 0;
    int label = 0;
    int a = 0, b = 0; // the draw: meaning depends on the feature
};

static Try draw_try(Feature feature, uint64_t random) {
    Try t;
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
    }
    return t;
}

static void lines_at(Feature feature, const Try& t, int tick, bool& a_on, bool& b_on) {
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
    }
}

int probe_feature(int argc, char** argv) {
    Genome genome;
    apply_genome_args(argc, argv, genome);
    const char* name = text_arg(argc, argv, "--feature", "which");
    Feature feature = Feature::which;
    if (std::strcmp(name, "order") == 0) {
        feature = Feature::order;
    } else if (std::strcmp(name, "xor") == 0) {
        feature = Feature::order_xor;
    } else if (std::strcmp(name, "gap") == 0) {
        feature = Feature::gap;
    } else if (std::strcmp(name, "hold") == 0) {
        feature = Feature::hold;
        hold_gap = int_arg(argc, argv, "--hold-gap", 20);
    } else if (std::strcmp(name, "which") != 0) {
        std::fprintf(stderr, "blobprobe feature: --feature is which, order, xor, gap or hold\n");
        return 1;
    }
    int tries = int_arg(argc, argv, "--tries", 4000);
    int settle = int_arg(argc, argv, "--settle", 30000); // ticks of the same input first
    int rest = int_arg(argc, argv, "--rest", 10);        // quiet ticks between tries
    int cue_x = int_arg(argc, argv, "--cue-x", 1);
    int cue_y = int_arg(argc, argv, "--cue-y", 1);
    uint64_t seed = (uint64_t)int_arg(argc, argv, "--seed", 1);
    const char* label = text_arg(argc, argv, "--label", "run");
    // --linear 1: also fit a plain linear reader to the same counts (idea 58)
    bool linear = int_arg(argc, argv, "--linear", 0) != 0;
    LinearData kept;

    Blob* blob = blob_birth(genome, seed);
    int cue_tile = cue_y * blob->tiles_w + cue_x;
    int tiles = blob->tile_count;
    int neurons = blob->neuron_count;
    std::vector<double> sum(2 * (size_t)neurons, 0.0), squares(2 * (size_t)neurons, 0.0);
    std::vector<int> count((size_t)neurons);
    int64_t seen[2] = {0, 0};
    uint64_t state = 0x2545F4914F6CDD1Dull ^ seed;

    int64_t ticks_done = 0;
    int measured = 0;
    while (measured < tries) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        Try t = draw_try(feature, state >> 20);
        bool counting = ticks_done >= settle;
        for (int n = 0; counting && n < neurons; n++) {
            count[(size_t)n] = 0;
        }
        for (int tick = 0; tick < t.length + rest; tick++) {
            bool a_on = false, b_on = false;
            if (tick < t.length) {
                lines_at(feature, t, tick, a_on, b_on);
            }
            set_cues(*blob, cue_tile, a_on, b_on);
            blob_tick(*blob);
            ticks_done++;
            if (!counting || tick < t.read_from || tick >= t.read_to) {
                continue;
            }
            for (int tile = 0; tile < tiles; tile++) {
                // sensor cells are driven by the world, so they do not count
                uint64_t spikes = blob->spikes[blob->now][tile] & ~blob->sensor[tile];
                while (spikes) {
                    count[(size_t)(tile * TILE_NEURONS + __builtin_ctzll(spikes))]++;
                    spikes &= spikes - 1;
                }
            }
        }
        if (!counting) {
            continue;
        }
        measured++;
        seen[t.label]++;
        if (linear) {
            for (int n = 0; n < neurons; n++) {
                int c = count[(size_t)n];
                kept.counts.push_back((uint8_t)(c < 255 ? c : 255));
            }
            kept.label.push_back(t.label);
        }
        for (int n = 0; n < neurons; n++) {
            sum[(size_t)(t.label * neurons + n)] += count[(size_t)n];
            squares[(size_t)(t.label * neurons + n)] += (double)count[(size_t)n] * count[(size_t)n];
        }
    }

    // a tile's number: its neurons' gaps combined, less what chance alone gives
    const int max_distance = 5;
    double readable[max_distance] = {};
    int tiles_at[max_distance] = {};
    double n0 = (double)(seen[0] ? seen[0] : 1), n1 = (double)(seen[1] ? seen[1] : 1);
    double chance = 1.0 / n0 + 1.0 / n1;
    for (int tile = 0; tile < tiles; tile++) {
        double total = 0;
        for (int k = 0; k < TILE_NEURONS; k++) {
            int n = tile * TILE_NEURONS + k;
            double m0 = sum[(size_t)n] / n0, m1 = sum[(size_t)(neurons + n)] / n1;
            double v0 = squares[(size_t)n] / n0 - m0 * m0;
            double v1 = squares[(size_t)(neurons + n)] / n1 - m1 * m1;
            double noise = (v0 + v1) / 2.0;
            if (noise > 0) {
                total += (m1 - m0) * (m1 - m0) / noise - chance;
            }
        }
        int dx = std::abs(tile % blob->tiles_w - cue_x);
        int dy = std::abs(tile / blob->tiles_w - cue_y);
        int distance = dx > dy ? dx : dy;
        if (distance < max_distance) {
            readable[distance] += total > 0 ? std::sqrt(total) : 0.0;
            tiles_at[distance]++;
        }
    }
    std::printf("%-7s %-26s", name, label);
    for (int d = 0; d < max_distance && tiles_at[d] > 0; d++) {
        std::printf(" %6.2f", readable[d] / tiles_at[d]);
    }
    std::printf("\n");
    if (linear) {
        kept.neurons = neurons;
        kept.tiles_w = blob->tiles_w;
        kept.tiles_h = blob->tiles_h;
        kept.classes = 2;
        linear_report(kept, label, cue_tile, {});
    }
    blob_free(blob);
    return 0;
}
