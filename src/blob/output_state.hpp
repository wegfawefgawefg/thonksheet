#pragma once

#include "blob/blob.hpp"

// the blob's own output (genome out_on, learn_output.cpp). core-only.

constexpr int OUT_MAX_LINES = 64;
constexpr int OUT_PAY_PLANES = 8; // out_pay: a tally counts to 255

struct OutputState {
    int lines = 0, cells = 0, planes = 0;
    uint32_t named = 0;                    // blob.motor_line_set when the cells were listed
    int32_t* cell = nullptr;               // the motor cells, line by line
    int32_t first[OUT_MAX_LINES + 1] = {}; // line l's cells: cell[first[l] .. first[l + 1])
    uint64_t* counter = nullptr;           // per cell SLOTS x planes words (plane 0 the lowest
                                           // bit of the slot's 64 counters, the last the sign)
    uint64_t* active = nullptr;            // tile_count + 1 words: the sources that fired in
                                           // the window of the step last read
    int64_t vote[OUT_MAX_LINES] = {};      // that step's vote per line
    int64_t size[OUT_MAX_LINES] = {};      // and what one step would add to it
    int8_t step[OUT_MAX_LINES] = {};       // the verdict's move per line: +1 up, -1 down
    int8_t teach[OUT_MAX_LINES] = {};      // the error per line for the cells (hop_teach 6)
    int pending = 0;                       // a verdict's move not yet made
    uint64_t counts[4] = {};               // see blob_output_counts
    uint64_t* tally = nullptr;             // out_pay: per tile, paid then charged, bit-sliced
};

// hop_teach 6: the error lines of the verdict just given, into line[] and sign[]
int output_errors(const Blob& blob, int* line, int* sign);
// out_pay: the lines' errors sent back along the output's own sources, by the sign of each
// counter: the cells paid on balance into more[], those charged into less[] (a word a tile)
void output_pay(Blob& blob, const int* line, const int* sign, int count, uint64_t* more,
                uint64_t* less);
// after the fire step of a tick the world asked a read on (blob.out_read): the votes of the
// step ending now into blob.out_vote
void output_read(Blob& blob);
// the verdict's move, in the learning step
void learn_output(Blob& blob);
void output_free(Blob& blob);
