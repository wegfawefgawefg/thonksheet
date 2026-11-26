#pragma once

#include <cstdint>
#include <vector>

// spikes per cell per step of delayed-echo trials, and a fresh ridge reader of the cue from
// them (time_wiring.cpp; used by blobprobe timewire and looptrace)
struct Counts {
    int phases = 0;              // steps per trial: cue, blanks, ask
    std::vector<int> cue;        // per trial
    std::vector<uint8_t> spikes; // trial x phase x neuron
};

// a ridge reader of the cue from the given cells' counts at one phase: fit on the first 60%
// of trials, the ridge picked on the next 15%, share right on the last 25%. the weights on
// standardised counts go to `w` (one per cell) when it is not null
double ridge_read(const Counts& c, const std::vector<int>& cells, int phase, int n,
                  std::vector<double>* w);
