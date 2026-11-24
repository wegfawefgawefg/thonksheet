// blobprobe carry: how far across the sheet can a cue be read from activity?
//
// One tile's sensor cells are split into two halves, cue A and cue B. In every block of
// ticks exactly one of them is on, so the total input never changes: only which cue it is.
// No reward. For every neuron we count its spikes per block and ask how well A blocks and
// B blocks can be told apart (the gap between the two means in units of the block-to-block
// spread). A tile's number combines its neurons: 1.0 means the cue is easy to read there,
// under 0.3 it is lost in noise. Tiles are grouped by how many tiles they are from the cue.
// The measure itself is in carry_measure.hpp, shared with blobrun --reach.

#include "blob/blob.hpp"
#include "probe/carry_measure.hpp"

#include <cstdio>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

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
    CarryMeasure m;
    carry_begin(m, *blob, cue_x, cue_y);
    uint64_t state = 0x2545F4914F6CDD1Dull ^ seed;

    int settle_blocks = settle / block;
    for (int b = 0; b < settle_blocks + blocks; b++) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        int which = (int)(state >> 40) & 1;
        carry_set_cue(*blob, m.cue_tile, which);
        carry_block_start(m);
        for (int tick = 0; tick < block; tick++) {
            blob_tick(*blob);
            if (tick >= skip && b >= settle_blocks) {
                carry_count_tick(m, *blob);
            }
        }
        if (b >= settle_blocks) {
            carry_block_end(m, *blob, which);
        }
    }

    // how readable the cue is at each distance in tiles
    double readable[CARRY_MAX_DISTANCE] = {};
    int reach = 0;
    int distances = carry_readable(m, *blob, readable, reach);
    std::printf("%-26s", label);
    for (int d = 0; d < distances; d++) {
        std::printf(" %5.2f", readable[d]);
    }
    std::printf("   reach %d\n", reach);
    blob_free(blob);
    return 0;
}
