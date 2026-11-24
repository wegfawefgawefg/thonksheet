#pragma once

// The pulse in a record of neurons fired per tick: the lag at which the count best repeats.
// Shared by blobprobe rhythm and blobrun's end-of-life numbers. Probes may use floats.

#include <vector>

struct Rhythm {
    double mean = 0;     // neurons fired per tick
    double variance = 0; // of that count
    int period = 0;      // ticks; 0 when there is no peak
    double strength = 0; // how alike the count is to itself one period later (1 = exact)
};

// strength under this counts as no rhythm
constexpr double RHYTHM_NONE = 0.05;

inline Rhythm rhythm_find(const std::vector<double>& fired, int max_lag) {
    Rhythm r;
    int ticks = (int)fired.size();
    if (ticks < 2) {
        return r;
    }
    max_lag = max_lag + 2 < ticks ? max_lag : ticks - 2;
    for (double f : fired) {
        r.mean += f;
    }
    r.mean /= ticks;
    for (double f : fired) {
        r.variance += (f - r.mean) * (f - r.mean);
    }
    r.variance /= ticks;

    // how alike the count is to itself `lag` ticks later
    std::vector<double> alike((size_t)max_lag + 2, 0.0);
    for (int lag = 1; lag <= max_lag + 1; lag++) {
        double sum = 0;
        for (int t = 0; t + lag < ticks; t++) {
            sum += (fired[(size_t)t] - r.mean) * (fired[(size_t)(t + lag)] - r.mean);
        }
        alike[(size_t)lag] = r.variance > 0 ? sum / ((ticks - lag) * r.variance) : 0;
    }

    // the period is the strongest lag that is a peak of its own
    for (int lag = 3; lag <= max_lag; lag++) {
        double a = alike[(size_t)lag];
        if (a > alike[(size_t)lag - 1] && a >= alike[(size_t)lag + 1] && a > r.strength) {
            r.strength = a;
            r.period = lag;
        }
    }
    return r;
}
