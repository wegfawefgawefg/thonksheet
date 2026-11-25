#include "vm/vm.hpp"

#include <bit>
#include <cstdlib>

// one line per tick while vm.trace is set: the tick, how many bits are on in X, M and O, and
// the winners sorted by entry number

static int by_number(const void* a, const void* b) {
    return *(const int32_t*)a - *(const int32_t*)b;
}

void vm_trace_tick(VmBlob& v) {
    int x_on = 0, o_on = 0;
    const uint64_t* x = vm_x(v);
    const uint64_t* o = vm_o(v);
    for (int w = 0; w < v.x_words; w++) {
        x_on += std::popcount(x[w]);
    }
    for (int w = 0; w < v.o_words; w++) {
        o_on += std::popcount(o[w]);
    }
    int32_t sorted[256];
    int K = v.vg.active < 256 ? v.vg.active : 256;
    for (int k = 0; k < K; k++) {
        sorted[k] = v.winners[k];
    }
    std::qsort(sorted, (size_t)K, sizeof(int32_t), by_number);
    std::fprintf(v.trace, "t %u x %d m %u o %d w", v.now, x_on, v.stats.m_on, o_on);
    for (int k = 0; k < K; k++) {
        std::fprintf(v.trace, " %d", sorted[k]);
    }
    std::fprintf(v.trace, "\n");
}
