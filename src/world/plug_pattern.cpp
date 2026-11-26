#include "world/plug.hpp"

// idea 134: an answer that is a pattern. each output bit is a pair of motor lines, voted the
// way buttons are: the group with more spikes over the step wins. what the world can tell
// the motor cells afterwards is the same kind of mark the buttons get (plug_mark_acted):
// which cells belong to the outputs being graded, and which of them won.

void plug_pattern(Plug& plug, uint64_t& bits, uint64_t& tied) {
    bits = 0;
    tied = 0;
    for (int b = 0; 2 * b + 1 < plug.motor_lines; b++) {
        int off = plug.motor_count[2 * b], on = plug.motor_count[2 * b + 1];
        bits |= (uint64_t)(on > off) << b;
        tied |= (uint64_t)(on == off && !plug.tie_low) << b;
    }
    plug_clear_counts(plug);
}

static void mark_line(const Plug& plug, Blob& blob, int line, bool acted) {
    const int* cells = nullptr;
    int count = plug_motor_cells(plug, line, &cells);
    for (int k = 0; k < count; k++) {
        blob_motor_attach(blob, cells[k], true);
        if (acted) {
            blob_motor_acted(blob, cells[k]);
        }
    }
}

void plug_mark_pattern(const Plug& plug, Blob& blob, uint64_t bits, uint64_t tied,
                       uint64_t graded) {
    plug_mark_motors(plug, blob, false); // clears every mark, the pressed ones too
    for (int b = 0; 2 * b + 1 < plug.motor_lines; b++) {
        if (!((graded >> b) & 1)) {
            continue;
        }
        bool tie = (tied >> b) & 1;
        int won = (bits >> b) & 1;
        mark_line(plug, blob, 2 * b, !tie && won == 0);
        mark_line(plug, blob, 2 * b + 1, !tie && won == 1);
    }
}
