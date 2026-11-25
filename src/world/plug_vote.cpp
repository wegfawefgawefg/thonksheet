#include "world/plug.hpp"

// the step's vote, for the blob's barely-correct rule (genome margin_percent,
// src/blob/learn_margin.cpp): the pressed button's spikes, the runner-up's, and which cells
// are the runner-up's. like plug_mark_acted, an efference copy written by the world; the
// blob ignores it unless the genome asks.
void plug_mark_vote(const Plug& plug, Blob& blob, int move) {
    if (!blob.genome.margin_percent || move < 0 || move >= plug.motor_lines) {
        return;
    }
    int runner = -1, runner_count = -1;
    for (int line = 0; line < plug.motor_lines; line++) {
        if (line != move && plug.last_count[line] > runner_count) {
            runner = line;
            runner_count = plug.last_count[line];
        }
    }
    const int* cells = nullptr;
    int count = runner >= 0 ? plug_motor_cells(plug, runner, &cells) : 0;
    blob_mark_vote(blob, plug.last_count[move], runner_count < 0 ? 0 : runner_count, cells, count);
}
