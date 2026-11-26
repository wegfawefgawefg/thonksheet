#include "world/plug.hpp"

// the lines of the step just judged, for the blob's one-hop error rule (genome hop_on,
// src/blob/learn_hop.cpp). like plug_mark_acted, written by the world before the reward;
// the blob ignores it unless the genome asks, and its genome decides how much of it to use
// (hop_teach 1 uses only the pressed line). buttons: the pressed line and the right one. a
// pattern answer (idea 134): for each graded bit, the group that won (none on a tie) and
// the group that was right.
void plug_hop_lines(const Plug& plug, Blob& blob, int move, int right, uint64_t pattern,
                    uint64_t tied, uint64_t graded) {
    if (!blob.genome.hop_on && !blob.genome.out_on) {
        return;
    }
    uint64_t pressed = 0, want = 0;
    if (plug.shape.pattern_bits > 0) {
        for (int b = 0; 2 * b + 1 < plug.motor_lines && b < 32; b++) {
            if (!((graded >> b) & 1)) {
                continue;
            }
            if (!((tied >> b) & 1)) {
                pressed |= 1ull << (2 * b + (int)((pattern >> b) & 1));
            }
            if (right >= 0) {
                want |= 1ull << (2 * b + ((right >> b) & 1));
            }
        }
    } else {
        pressed = move >= 0 && move < 64 ? 1ull << move : 0;
        want = right >= 0 && right < 64 ? 1ull << right : 0;
    }
    // the blob's own output (genome out_on) hears which lines were right
    blob_output_verdict(blob, want, plug.shape.pattern_bits > 0, pressed);
    blob_hop_lines(blob, pressed, want, plug.motor_lines);
    if (plug.shape.pattern_bits == 0) {
        // each line's spikes in the step, for the rule's own-error forms (hop_teach 4, 5)
        blob_hop_votes(blob, plug.last_count, plug.motor_lines);
    }
}

