// blobprobe rhythm: does the blob's activity pulse, and with what period?
//
// Runs one blob with nothing plugged in, or with random bits on every sensor cell, records
// how many neurons fired on each tick, and looks for the lag at which that count best
// repeats. Probes may use floats; the blob itself does not.

#include "blob/blob.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

static uint64_t probe_random(uint64_t& state) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

// random bits on every sensor cell, each on with chance 1 in 8 (what blobview --input does)
static void random_input(Blob& blob, uint64_t& state) {
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t on = probe_random(state) & probe_random(state) & probe_random(state);
        blob.sense_attached[t] = blob.sensor[t];
        blob.sense_in[t] = blob.sensor[t] & on;
    }
}

int probe_rhythm(int argc, char** argv) {
    Genome genome;
    apply_genome_args(argc, argv, genome);
    int tiles = int_arg(argc, argv, "--tiles", 0);
    if (tiles > 0) {
        genome.tiles_w = tiles;
        genome.tiles_h = tiles;
    }
    int input = int_arg(argc, argv, "--input", 1);
    int warmup = int_arg(argc, argv, "--warmup", 30000);
    int ticks = int_arg(argc, argv, "--ticks", 65536);
    int max_lag = int_arg(argc, argv, "--max-lag", 300);
    uint64_t seed = (uint64_t)int_arg(argc, argv, "--seed", 1);
    const char* label = text_arg(argc, argv, "--label", "run");

    Blob* blob = blob_birth(genome, seed);
    uint64_t state = 0x9E3779B97F4A7C15ull ^ seed;
    std::vector<double> fired((size_t)ticks);

    // let regulation settle, then record
    for (int t = 0; t < warmup + ticks; t++) {
        if (input) {
            random_input(*blob, state);
        }
        blob_tick(*blob);
        if (t >= warmup) {
            fired[(size_t)(t - warmup)] = (double)blob->stats.fired;
        }
    }

    // mean, spread, and how alike the count is to itself `lag` ticks later
    double mean = 0;
    for (double f : fired) {
        mean += f;
    }
    mean /= ticks;
    double variance = 0;
    for (double f : fired) {
        variance += (f - mean) * (f - mean);
    }
    variance /= ticks;
    std::vector<double> alike((size_t)max_lag + 2, 0.0);
    for (int lag = 1; lag <= max_lag + 1; lag++) {
        double sum = 0;
        for (int t = 0; t + lag < ticks; t++) {
            sum += (fired[(size_t)t] - mean) * (fired[(size_t)(t + lag)] - mean);
        }
        alike[(size_t)lag] = variance > 0 ? sum / ((ticks - lag) * variance) : 0;
    }

    // the period is the strongest lag that is a peak of its own
    int period = 0;
    double strength = 0;
    for (int lag = 3; lag <= max_lag; lag++) {
        double a = alike[(size_t)lag];
        if (a > alike[(size_t)lag - 1] && a >= alike[(size_t)lag + 1] && a > strength) {
            strength = a;
            period = lag;
        }
    }

    double per_1024 = mean * 1024.0 / blob->neuron_count;
    double fano = mean > 0 ? variance / mean : 0;
    if (strength < 0.05) {
        std::printf("%-28s neurons %6d  rate %5.1f per 1024  fano %6.1f  no rhythm (best %.2f)\n",
                    label, blob->neuron_count, per_1024, fano, strength);
    } else {
        std::printf("%-28s neurons %6d  rate %5.1f per 1024  fano %6.1f  period %3d ticks  "
                    "strength %.2f\n",
                    label, blob->neuron_count, per_1024, fano, period, strength);
    }
    blob_free(blob);
    return 0;
}
