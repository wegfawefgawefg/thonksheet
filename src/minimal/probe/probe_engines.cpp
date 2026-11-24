#include "minimal/probe/probe.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// either engine through one door, and the per-tick and end-of-life measurements.

Blob* probe_birth(const ProbeArgs& args, const Genome& genome, uint64_t seed) {
    if (args.engine == ProbeEngine::minimal) {
        return minimal_birth(genome, args.mg, seed);
    }
    return blob_birth(genome, seed);
}

void probe_free(const ProbeArgs& args, Blob* blob) {
    if (args.engine == ProbeEngine::minimal) {
        minimal_free(blob);
    } else {
        blob_free(blob);
    }
}

void probe_tick(const ProbeArgs& args, Blob& blob) {
    if (args.engine == ProbeEngine::minimal) {
        minimal_tick(blob);
    } else {
        blob_tick(blob);
    }
}

void probe_reward(const ProbeArgs& args, Blob& blob, int valence) {
    if (args.engine == ProbeEngine::minimal) {
        minimal_reward(blob, valence);
    } else {
        blob_reward(blob, valence);
    }
}

void probe_set_learning(const ProbeArgs& args, Blob& blob, int on) {
    if (args.engine == ProbeEngine::minimal) {
        minimal_of(blob).mg.learning_on = on;
    } else {
        blob.genome.learning_on = on;
    }
}

const char* probe_engine_name(const ProbeArgs& args) {
    return args.engine == ProbeEngine::minimal ? "minimal" : "full";
}

double probe_bytes_per_neuron(const ProbeArgs& args, const Blob& blob) {
    double n = (double)blob.neuron_count;
    if (args.engine == ProbeEngine::minimal) {
        return (double)minimal_bytes(minimal_of(blob)) / n;
    }
    // every array blob_birth allocates (see src/blob/birth.cpp)
    double tiles = (double)blob.tile_count;
    double bytes = 8.0 * blob.spike_stride * HISTORY;         // spike ring
    bytes += 8.0 * 8.0 * (tiles + 1);                         // 8 per-tile words
    bytes += 4.0 * tiles * LOCAL_SLOTS + 4.0 * n * FAR_SLOTS; // source tiles
    bytes += n * SLOTS;                                       // slot delays
    bytes += 2.0 * 8.0 * n * SLOTS;                           // w0, w1
    bytes += n * (2 + 2 + 1 + 1 + 1 + 2);                     // potential .. fan_in
    bytes += 4.0 * tiles + 18.0 * blob.sector_count;          // sectors
    return bytes / n;
}

void recorder_start(Recorder& r, const Blob& blob) {
    r = Recorder{};
    r.count.assign((size_t)blob.neuron_count, 0);
    r.driven.assign((size_t)blob.neuron_count, 0);
}

void recorder_add(Recorder& r, const ProbeArgs& args, const Blob& blob) {
    const uint64_t* map = blob.spikes[blob.now];
    const uint64_t* noise = nullptr;
    if (args.engine == ProbeEngine::minimal) {
        const MinimalBlob& m = minimal_of(blob);
        noise = m.noise + (size_t)m.now * m.stride;
    }
    uint32_t total = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t f = map[t] & ~blob.sensor[t];
        uint64_t nz = noise ? noise[t] : 0;
        total += (uint32_t)__builtin_popcountll(f);
        while (f) {
            int b = __builtin_ctzll(f);
            f &= f - 1;
            int n = t * TILE_NEURONS + b;
            r.count[(size_t)n]++;
            r.driven[(size_t)n] += (uint32_t) !((nz >> b) & 1);
        }
    }
    r.fired.push_back((double)total);
    r.ticks++;
}

void recorder_print(const Recorder& r, const ProbeArgs& args, const Blob& blob, const char* label) {
    size_t ticks = r.fired.size();
    if (ticks < 8) {
        return;
    }
    double mean = 0;
    for (double f : r.fired) {
        mean += f;
    }
    mean /= (double)ticks;
    double variance = 0;
    for (double f : r.fired) {
        variance += (f - mean) * (f - mean);
    }
    variance /= (double)ticks;

    // pulse: the lag with the strongest self-likeness that is a peak of its own
    int max_lag = 300;
    std::vector<double> alike((size_t)max_lag + 2, 0.0);
    for (int lag = 1; lag <= max_lag + 1 && (size_t)lag < ticks; lag++) {
        double sum = 0;
        for (size_t t = 0; t + (size_t)lag < ticks; t++) {
            sum += (r.fired[t] - mean) * (r.fired[t + (size_t)lag] - mean);
        }
        alike[(size_t)lag] = variance > 0 ? sum / ((double)(ticks - (size_t)lag) * variance) : 0;
    }
    int period = 0;
    double strength = 0;
    for (int lag = 3; lag <= max_lag; lag++) {
        double a = alike[(size_t)lag];
        if (a > alike[(size_t)lag - 1] && a >= alike[(size_t)lag + 1] && a > strength) {
            strength = a;
            period = lag;
        }
    }

    // silent: under 1/16 of the target rate. stuck: firing more than half the ticks.
    int free_neurons = 0, silent = 0, stuck = 0, driven_silent = 0;
    double target = args.engine == ProbeEngine::minimal
                        ? 1.0 / (double)(1 << (args.mg.down_shift - args.mg.up_shift))
                        : blob.genome.sector_target_per_1024 / 1024.0;
    for (int n = 0; n < blob.neuron_count; n++) {
        if ((blob.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1) {
            continue;
        }
        free_neurons++;
        double rate = (double)r.count[(size_t)n] / (double)ticks;
        silent += rate < target / 16 ? 1 : 0;
        stuck += rate > 0.5 ? 1 : 0;
        driven_silent += r.driven[(size_t)n] == 0 ? 1 : 0;
    }
    double per_1024 = mean * 1024.0 / free_neurons;
    std::vector<double> sorted = r.fired;
    std::sort(sorted.begin(), sorted.end());
    double scale = 1024.0 / free_neurons;
    std::printf("%s: %s, %zu ticks\n", label, probe_engine_name(args), ticks);
    std::printf("  rate %.1f per 1024 per tick (target %.1f)  fano %.1f\n", per_1024, target * 1024,
                variance / (mean > 0 ? mean : 1));
    std::printf("  per tick, per 1024: median %.1f  p90 %.1f  p99 %.1f  max %.1f\n",
                sorted[ticks / 2] * scale, sorted[ticks * 9 / 10] * scale,
                sorted[ticks * 99 / 100] * scale, sorted[ticks - 1] * scale);
    if (std::getenv("PROBE_TRACE")) {
        // the first ticks of the window, one count per tick, for a look by eye
        for (size_t t = 0; t < ticks && t < 600; t++) {
            std::printf("%d%c", (int)r.fired[t], t % 30 == 29 ? '\n' : ' ');
        }
        std::printf("\n");
    }
    if (strength < 0.05) {
        std::printf("  pulse none (best %.2f)\n", strength);
    } else {
        std::printf("  pulse period %d ticks, strength %.2f\n", period, strength);
    }
    std::printf("  silent %.1f%%  stuck %.1f%%", 100.0 * silent / free_neurons,
                100.0 * stuck / free_neurons);
    if (args.engine == ProbeEngine::minimal) {
        std::printf("  never driven (noise spikes only) %.1f%%",
                    100.0 * driven_silent / free_neurons);
    }
    std::printf("\n");
}

static void print_spread(const char* what, std::vector<int>& v) {
    if (v.empty()) {
        return;
    }
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    std::printf("  %-12s min %d  p10 %d  median %d  p90 %d  max %d\n", what, v[0], v[n / 10],
                v[n / 2], v[n * 9 / 10], v[n - 1]);
}

void print_wiring(const ProbeArgs& args, const Blob& blob, const char* label) {
    std::vector<int> fan, thr;
    int64_t connections = 0, top = 0;
    int at_cap = 0, thr_at_cap = 0;
    for (int n = 0; n < blob.neuron_count; n++) {
        if ((blob.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1) {
            continue;
        }
        if (args.engine == ProbeEngine::minimal) {
            const MinimalBlob& m = minimal_of(blob);
            int f = minimal_fan_in(m, n);
            fan.push_back(f);
            thr.push_back(m.threshold[n]);
            connections += f;
            thr_at_cap += m.threshold[n] >= m.mg.threshold_cap ? 1 : 0;
            continue;
        }
        int f = 0;
        for (int k = 0; k < SLOTS; k++) {
            uint64_t a0 = blob.w0[blob_weight_index(n, k)], a1 = blob.w1[blob_weight_index(n, k)];
            f += __builtin_popcountll(a0 | a1);
            top += __builtin_popcountll(a0 & a1);
        }
        fan.push_back(f);
        thr.push_back(blob.threshold[n]);
        connections += f;
        at_cap += f >= blob.genome.fan_in_max ? 1 : 0;
        thr_at_cap += blob.threshold[n] >= blob.genome.threshold_cap ? 1 : 0;
    }
    size_t free_neurons = fan.size();
    std::printf("%s: %s wiring\n", label, probe_engine_name(args));
    print_spread("connections", fan);
    print_spread("threshold", thr);
    std::printf("  thresholds at cap %.1f%%", 100.0 * thr_at_cap / (double)free_neurons);
    if (args.engine == ProbeEngine::full) {
        std::printf("  connections at top strength %.1f%%  neurons at fan-in cap %.1f%%",
                    100.0 * (double)top / (double)(connections ? connections : 1),
                    100.0 * at_cap / (double)free_neurons);
    } else {
        const MinimalBlob& m = minimal_of(blob);
        std::printf("  gained %llu  lost %llu  graded %llu  cowired %llu  reward avg %.2f",
                    (unsigned long long)m.stats.gained, (unsigned long long)m.stats.lost,
                    (unsigned long long)m.stats.graded, (unsigned long long)m.stats.cowired,
                    m.reward_avg / 256.0);
        if (m.slots > MIN_SLOTS) {
            // far slots: how many neurons have one, and what share of connections sit there
            int64_t far = 0, with = 0;
            for (int n = 0; n < blob.neuron_count; n++) {
                int t = n / TILE_NEURONS, far_slots = m.slots - MIN_SLOTS;
                bool any = false;
                for (int j = 0; j < far_slots; j++) {
                    int64_t i = ((int64_t)t * m.slots + MIN_SLOTS + j) * TILE_NEURONS + n % 64;
                    far += __builtin_popcountll(m.mask[i]);
                    any |= m.far_tile[((int64_t)t * far_slots + j) * TILE_NEURONS + n % 64] !=
                           (uint32_t)blob.tile_count;
                }
                with += any ? 1 : 0;
            }
            std::printf("\n  far: neurons with a far tile %.1f%%  connections in far slots %.1f%%",
                        100.0 * (double)with / blob.neuron_count,
                        100.0 * (double)far / (double)(connections ? connections : 1));
        }
    }
    std::printf("\n  bytes per neuron %.1f\n", probe_bytes_per_neuron(args, blob));
}
