// blobprobe carry: how far across the sheet can a cue be read from activity?
//
// One tile's sensor cells are split into two halves, cue A and cue B. In every block of
// ticks exactly one of them is on, so the total input never changes: only which cue it is.
// No reward. For every neuron we count its spikes per block and ask how well A blocks and
// B blocks can be told apart (the gap between the two means in units of the block-to-block
// spread). A tile's number combines its neurons: 1.0 means the cue is easy to read there,
// under 0.3 it is lost in noise. Tiles are grouped by how many tiles they are from the cue.

#include "blob/blob.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

// cue 0 is every other sensor cell of the tile, cue 1 the rest
static void set_cue(Blob& blob, int cue_tile, int which) {
    uint64_t cells = blob.sensor[cue_tile];
    int index = 0;
    while (cells) {
        int bit = __builtin_ctzll(cells);
        cells &= cells - 1;
        blob_sense(blob, cue_tile * TILE_NEURONS + bit, (index & 1) == which);
        index++;
    }
}

int probe_carry(int argc, char** argv) {
    Genome genome;
    apply_genome_args(argc, argv, genome);
    int block = int_arg(argc, argv, "--block", 20);
    int blocks = int_arg(argc, argv, "--blocks", 4000);
    int settle = int_arg(argc, argv, "--settle", 30000); // ticks before measuring, cue cycling
    int skip = int_arg(argc, argv, "--skip", 6);         // ticks at a block's start not counted
    int cue_x = int_arg(argc, argv, "--cue-x", 0);
    int cue_y = int_arg(argc, argv, "--cue-y", 0);
    uint64_t seed = (uint64_t)int_arg(argc, argv, "--seed", 1);
    const char* label = text_arg(argc, argv, "--label", "run");

    Blob* blob = blob_birth(genome, seed);
    int cue_tile = cue_y * blob->tiles_w + cue_x;
    int tiles = blob->tile_count;
    int neurons = blob->neuron_count;
    // per neuron, per cue: sum and sum of squares of spikes per block
    std::vector<double> sum(2 * (size_t)neurons, 0.0), squares(2 * (size_t)neurons, 0.0);
    std::vector<int> count((size_t)neurons);
    int64_t seen[2] = {0, 0};
    uint64_t state = 0x2545F4914F6CDD1Dull ^ seed;

    int settle_blocks = settle / block;
    for (int b = 0; b < settle_blocks + blocks; b++) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        int which = (int)(state >> 40) & 1;
        set_cue(*blob, cue_tile, which);
        for (int n = 0; n < neurons; n++) {
            count[(size_t)n] = 0;
        }
        for (int tick = 0; tick < block; tick++) {
            blob_tick(*blob);
            if (tick < skip || b < settle_blocks) {
                continue;
            }
            for (int t = 0; t < tiles; t++) {
                // sensor cells are driven by the world, so they do not count as carrying
                uint64_t spikes = blob->spikes[blob->now][t] & ~blob->sensor[t];
                while (spikes) {
                    count[(size_t)(t * TILE_NEURONS + __builtin_ctzll(spikes))]++;
                    spikes &= spikes - 1;
                }
            }
        }
        if (b < settle_blocks) {
            continue;
        }
        seen[which]++;
        for (int n = 0; n < neurons; n++) {
            sum[(size_t)(which * neurons + n)] += count[(size_t)n];
            squares[(size_t)(which * neurons + n)] += (double)count[(size_t)n] * count[(size_t)n];
        }
    }

    // how readable the cue is in each tile: its neurons' gaps combined, with the part that
    // chance alone would give taken off; then averaged over tiles at each distance
    const int max_distance = 16;
    double readable[max_distance] = {};
    int tiles_at[max_distance] = {};
    double n0 = (double)(seen[0] ? seen[0] : 1), n1 = (double)(seen[1] ? seen[1] : 1);
    double chance = 1.0 / n0 + 1.0 / n1;
    for (int t = 0; t < tiles; t++) {
        double total = 0;
        for (int k = 0; k < TILE_NEURONS; k++) {
            int n = t * TILE_NEURONS + k;
            double m0 = sum[(size_t)n] / n0, m1 = sum[(size_t)(neurons + n)] / n1;
            double v0 = squares[(size_t)n] / n0 - m0 * m0;
            double v1 = squares[(size_t)(neurons + n)] / n1 - m1 * m1;
            double noise = (v0 + v1) / 2.0;
            if (noise > 0) {
                total += (m1 - m0) * (m1 - m0) / noise - chance;
            }
        }
        double gap = total > 0 ? std::sqrt(total) : 0.0;
        int dx = std::abs(t % blob->tiles_w - cue_x), dy = std::abs(t / blob->tiles_w - cue_y);
        int distance = dx > dy ? dx : dy;
        if (distance < max_distance) {
            readable[distance] += gap;
            tiles_at[distance]++;
        }
    }
    int reach = 0;
    std::printf("%-26s", label);
    for (int d = 0; d < max_distance && tiles_at[d] > 0; d++) {
        double r = readable[d] / tiles_at[d];
        std::printf(" %5.2f", r);
        if (r >= 0.3) {
            reach = d;
        }
    }
    std::printf("   reach %d\n", reach);
    blob_free(blob);
    return 0;
}
