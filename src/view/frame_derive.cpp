// frame_derive: numbers the viewer works out from the tape it already has, so the sim
// thread never pays for them: firing rates, silent and stuck neurons, and how much each
// tick looks like the ticks before it.

#include "blob/blob.hpp"
#include "view_frame.hpp"

#include <algorithm>
#include <bit>

static int rows_valid(const Tape& t) {
    return static_cast<int>(std::min<uint64_t>(t.ticks, static_cast<uint64_t>(t.len)));
}

static const uint64_t* tape_row(const Tape& t, int age) {
    const uint64_t row = (t.ticks - 1 - age) % static_cast<uint64_t>(t.len);
    return &t.words[row * t.row_words];
}

// spikes per neuron over the tape; rates go in log2 bins: 0, 1, 2-3, 4-7, ...
static void derive_rates(ViewFrame& f, std::vector<uint16_t>& counts) {
    const Tape& t = f.tape;
    const int rows = rows_valid(t);
    counts.assign(f.neuron_count, 0);
    for (int age = 0; age < rows; ++age) {
        const uint64_t* words = tape_row(t, age);
        for (int w = 0; w < t.row_words; ++w) {
            for (uint64_t bits = words[w]; bits != 0; bits &= bits - 1) {
                counts[w * TILE_NEURONS + std::countr_zero(bits)] += 1;
            }
        }
    }
    f.rate = Histogram{};
    f.rate.low = 1 << 30;
    f.silent = 0;
    f.stuck = 0;
    uint64_t sum = 0;
    for (int n = 0; n < f.neuron_count; ++n) {
        const int c = counts[n];
        const int bin = c == 0 ? 0 : 1 + std::bit_width(static_cast<unsigned>(c)) - 1;
        f.rate.counts[std::min(bin, HIST_BINS - 1)] += 1;
        f.rate.total += 1;
        f.rate.low = std::min(f.rate.low, c);
        f.rate.high = std::max(f.rate.high, c);
        f.silent += c == 0 ? 1 : 0;
        f.stuck += rows > 0 && c * 4 > rows ? 1 : 0;
        sum += c;
    }
    f.mean_rate = rows > 0 && f.neuron_count > 0
                      ? static_cast<float>(sum) / (static_cast<float>(rows) * f.neuron_count)
                      : 0.0f;
    const int n = f.selected.neuron;
    f.selected_spikes = n >= 0 && n < f.neuron_count ? counts[n] : 0;
}

// overlap (shared spikes over all spikes) of each tick with the tick `lag` before it,
// averaged over the newest 256 ticks. A peak at some lag means activity repeats with that
// period; high everywhere means it is stuck in one pattern.
static void derive_lags(ViewFrame& f) {
    const Tape& t = f.tape;
    const int rows = rows_valid(t);
    const int span = std::min(256, rows - LAG_MAX);
    const int step = std::max(1, t.row_words / 256);
    for (int lag = 1; lag <= LAG_MAX; ++lag) {
        uint64_t shared = 0, either = 0;
        for (int age = 0; age < span; ++age) {
            const uint64_t* a = tape_row(t, age);
            const uint64_t* b = tape_row(t, age + lag);
            for (int w = 0; w < t.row_words; w += step) {
                shared += std::popcount(a[w] & b[w]);
                either += std::popcount(a[w] | b[w]);
            }
        }
        f.lag_similarity[lag - 1] =
            either > 0 ? static_cast<float>(shared) / static_cast<float>(either) : 0.0f;
    }
}

void frame_derive(ViewFrame& frame) {
    static std::vector<uint16_t> counts; // viewer thread only; kept to save the allocation
    if (frame.tape.len == 0 || frame.neuron_count == 0) {
        return;
    }
    derive_rates(frame, counts);
    derive_lags(frame);
}
