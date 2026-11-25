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
// The measurement itself is in feature_measure.hpp, shared with the viewer's tint.

#include "blob/blob.hpp"
#include "probe/feature_measure.hpp"
#include "probe/linear_fit.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

int probe_feature(int argc, char** argv) {
    Genome genome;
    apply_genome_args(argc, argv, genome);
    const char* name = text_arg(argc, argv, "--feature", "which");
    Feature feature = Feature::which;
    if (!feature_from_name(name, feature)) {
        std::fprintf(stderr, "blobprobe feature: --feature is which, order, xor, gap or hold\n");
        return 1;
    }
    int hold_gap = int_arg(argc, argv, "--hold-gap", 20);
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
    FeatureSums sums;
    feature_sums_begin(sums, neurons);
    std::vector<int> count((size_t)neurons);
    uint64_t state = 0x2545F4914F6CDD1Dull ^ seed;

    int64_t ticks_done = 0;
    int measured = 0;
    while (measured < tries) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        FeatureTry t = feature_draw_try(feature, state >> 20, hold_gap);
        bool counting = ticks_done >= settle;
        for (int n = 0; counting && n < neurons; n++) {
            count[(size_t)n] = 0;
        }
        for (int tick = 0; tick < t.length + rest; tick++) {
            bool a_on = false, b_on = false;
            if (tick < t.length) {
                feature_lines_at(feature, t, tick, a_on, b_on);
            }
            feature_set_cues(*blob, cue_tile, a_on, b_on);
            blob_tick(*blob);
            ticks_done++;
            if (counting && tick >= t.read_from && tick < t.read_to) {
                feature_count_tick(*blob, count.data());
            }
        }
        if (!counting) {
            continue;
        }
        measured++;
        if (linear) {
            for (int n = 0; n < neurons; n++) {
                int c = count[(size_t)n];
                kept.counts.push_back((uint8_t)(c < 255 ? c : 255));
            }
            kept.label.push_back(t.label);
        }
        feature_sums_add(sums, count.data(), t.label);
    }

    // a tile's number, averaged over the tiles at each distance from the cue
    const int max_distance = 5;
    double readable[max_distance] = {};
    int tiles_at[max_distance] = {};
    for (int tile = 0; tile < tiles; tile++) {
        int dx = std::abs(tile % blob->tiles_w - cue_x);
        int dy = std::abs(tile / blob->tiles_w - cue_y);
        int distance = dx > dy ? dx : dy;
        if (distance < max_distance) {
            readable[distance] += feature_tile_readable(sums, tile);
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
