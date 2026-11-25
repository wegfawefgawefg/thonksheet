// walk the owner's vvm (a copy of thonksheet-library/repos/vvm, unchanged) and measure its
// loops: for input none / one held / two taking turns every 8 ticks, 600 ticks after 100 to
// settle, from 5 starting states: distinct ops, the top op's share, the final cycle period.
// not part of the build (it links his library). from a release build of his copy in DIR:
//   g++ -std=c++20 -O2 -IDIR/include src/vm/vvm/vvm_walk.cpp DIR/build/libvvm_core.a -o vvm_walk
//   ./vvm_walk OPS HARD(0/1) [checkpoint or -] [refractory ticks]
#include "vvm/model.hpp"

#include <cmath>
#include <cstdio>
#include <span>
#include <cstdlib>
#include <map>
#include <random>
#include <set>
#include <vector>

static std::vector<float> unit(std::mt19937& rng, size_t d) {
    std::normal_distribution<float> n(0.f, 1.f);
    std::vector<float> v(d);
    float s = 0;
    for (auto& x : v) {
        x = n(rng);
        s += x * x;
    }
    s = std::sqrt(s);
    for (auto& x : v) x /= s;
    return v;
}

int main(int argc, char** argv) {
    vvm::Config c;
    c.state_dim = 256;
    c.num_ops = (size_t)std::atoi(argv[1]);
    c.candidate_count = 8;
    c.sample_retrieval = std::atoi(argv[2]) == 0;
    c.hard_refractory_ticks = argc > 4 ? (size_t)std::atoi(argv[4]) : 0;
    vvm::Model model(c);
    if (argc > 3 && argv[3][0] != '-') {
        model.load_checkpoint(argv[3]);
    }
    const char* names[3] = {"no input", "constant input", "input switching every 8"};
    for (int mode = 0; mode < 3; mode++) {
        double distinct_sum = 0, top_sum = 0, period_sum = 0;
        int cycles = 0;
        std::set<size_t> ever;
        for (int trial = 0; trial < 5; trial++) {
            std::mt19937 rng(1000 + trial);
            std::vector<float> state = unit(rng, c.state_dim);
            std::vector<float> a = unit(rng, c.state_dim), b = unit(rng, c.state_dim);
            std::vector<size_t> ops;
            for (int t = 0; t < 700; t++) {
                std::span<const float> in;
                if (mode == 1) in = a;
                if (mode == 2) in = ((t / 8) % 2) ? b : a;
                auto tick = model.tick(state, rng, (size_t)t, in);
                if (t >= 100) ops.push_back(tick.chosen_op);
            }
            std::map<size_t, int> count;
            for (size_t o : ops) count[o]++, ever.insert(o);
            int top = 0;
            for (auto& [o, n] : count) top = n > top ? n : top;
            distinct_sum += (double)count.size();
            top_sum += (double)top / ops.size();
            // smallest period p (<= 64) that repeats over the last 128 ticks
            int period = 0;
            for (int p = 1; p <= 64 && !period; p++) {
                bool ok = true;
                for (size_t t = ops.size() - 128 + p; t < ops.size() && ok; t++) ok = ops[t] == ops[t - p];
                if (ok) period = p;
            }
            if (period) cycles++, period_sum += period;
        }
        std::printf("%-26s distinct ops %.1f of %zu, top op share %.2f, ops ever used %zu, "
                    "settled into a cycle %d of 5 (mean period %.1f)\n",
                    names[mode], distinct_sum / 5, c.num_ops, top_sum / 5, ever.size(), cycles,
                    cycles ? period_sum / cycles : 0.0);
    }
    return 0;
}
