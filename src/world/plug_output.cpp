#include "world/plug.hpp"

// the blob's own output (genome out_on, src/blob/learn_output.cpp). the world's part is
// plain writes and reads of the blob's fields, as for sense_in: it names each button's motor
// cells at attach, asks for the votes on a step's last tick (blob.out_read, set by the
// ticker), and takes the vote a line in place of the motor cells' spike counts. a tie goes
// to the lowest line: tasks that give no verdict for "no move" would leave a newborn output,
// every vote zero, unanswered and untaught for ever.

void plug_output_lines(const Plug& plug, Blob& blob, bool on) {
    for (int line = 0; line < plug.motor_lines; line++) {
        const int* cells = nullptr;
        int count = plug_motor_cells(plug, line, &cells);
        for (int k = 0; k < count; k++) {
            blob_motor_line(blob, cells[k], on ? line : 255);
        }
    }
}

void plug_output_votes(Plug& plug, const Blob& blob) {
    if (!blob.genome.out_on || blob.out_lines <= 0) {
        return;
    }
    for (int line = 0; line < plug.motor_lines; line++) {
        plug.motor_count[line] = line < blob.out_lines ? blob.out_vote[line] : 0;
    }
    plug.signed_vote = true;
    plug.tie_low = true;
}
