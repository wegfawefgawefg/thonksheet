// the shuffled-input control for blobprobe calc --life (docs/results-2026-11-25-compete2.md):
// in the life, each trial's input lines (the ask line, line 0, aside) are moved by a
// permutation drawn fresh for that trial, the same for all its steps. as many lines are on,
// at the same times, but which lines are on together no longer says anything that comes
// back. the read after the life shows real cases, as MNIST's --life-shuffle does.

#include "world/task_calc.hpp"

#include <cstdint>

static uint64_t next(uint64_t& s) {
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    return s;
}

void calc_shuffle_trial(CalcState& c, uint64_t& rng) {
    int order[32];
    int lines = c.lines < 32 ? c.lines : 32;
    for (int i = 0; i < lines; i++) {
        order[i] = i;
    }
    for (int i = lines - 1; i > 1; i--) {
        int j = 1 + (int)(next(rng) % (uint64_t)i); // 1..i
        int t = order[i];
        order[i] = order[j];
        order[j] = t;
    }
    for (int s = 0; s < c.steps && s <= CALC_MAX_STEPS; s++) {
        uint32_t in = c.step_lines[s], out = in & 1u;
        for (int i = 1; i < lines; i++) {
            out |= ((in >> i) & 1u) << order[i];
        }
        c.step_lines[s] = out;
    }
}
