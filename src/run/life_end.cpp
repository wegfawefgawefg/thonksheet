#include "run/life_end.hpp"

#include "probe/rhythm_find.hpp"

#include <bit>

constexpr int RHYTHM_MAX_LAG = 300;

void alive_count_tick(AliveCount& count, const Blob& blob) {
    if (count.spikes.empty()) {
        count.spikes.assign((size_t)blob.neuron_count, 0);
    }
    const uint64_t* map = blob.spikes[blob.now];
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t spikes = map[t];
        count.fired += std::popcount(spikes);
        while (spikes) {
            count.spikes[(size_t)(t * TILE_NEURONS + std::countr_zero(spikes))]++;
            spikes &= spikes - 1;
        }
    }
    count.ticks++;
}

int fired_now(const Blob& blob) {
    int fired = 0;
    const uint64_t* map = blob.spikes[blob.now];
    for (int t = 0; t < blob.tile_count; t++) {
        fired += std::popcount(map[t]);
    }
    return fired;
}

void life_end_room(LifeEnd& end, const Blob& blob) {
    end.measured = true;
    int neurons = blob.neuron_count;
    if (neurons <= 0) {
        return;
    }
    if (blob.w0 && blob.w1) {
        // (w1, w0): 01 strength 1, 10 strength 4, 11 strength 5
        int64_t at[3] = {0, 0, 0};
        int64_t words = (int64_t)blob.tile_count * SLOTS * TILE_NEURONS;
        for (int64_t i = 0; i < words; i++) {
            uint64_t w0 = blob.w0[i], w1 = blob.w1[i];
            at[0] += std::popcount(w0 & ~w1);
            at[1] += std::popcount(~w0 & w1);
            at[2] += std::popcount(w0 & w1);
        }
        int64_t total = at[0] + at[1] + at[2];
        end.connections_per_neuron = (double)total / neurons;
        for (int k = 0; k < 3; k++) {
            end.share_strength[k] = total > 0 ? (double)at[k] / (double)total : 0.0;
        }
    }
    if (blob.fan_in) {
        int at_cap = 0;
        for (int n = 0; n < neurons; n++) {
            at_cap += blob.fan_in[n] >= blob.genome.fan_in_max;
        }
        end.share_at_cap = (double)at_cap / neurons;
    }
    if (blob.threshold) {
        int at_cap = 0;
        for (int n = 0; n < neurons; n++) {
            at_cap += blob.threshold[n] >= blob.genome.threshold_cap;
        }
        end.share_threshold_cap = (double)at_cap / neurons;
    }
}

void life_end_alive(LifeEnd& end, const AliveCount& count, const Blob& blob) {
    end.measured = true;
    end.target_per_1024 = blob.genome.sector_target_per_1024;
    if (count.ticks <= 0 || count.spikes.empty()) {
        return;
    }
    end.rate_per_1024 = (double)count.fired * 1024.0 / ((double)count.ticks * blob.neuron_count);
    int counted = 0, silent = 0, stuck = 0;
    for (int n = 0; n < blob.neuron_count; n++) {
        bool sensor = blob.sensor && ((blob.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1);
        if (sensor) {
            continue;
        }
        counted++;
        uint32_t s = count.spikes[(size_t)n];
        silent += s == 0;
        stuck += (int64_t)s * 4 > count.ticks;
    }
    end.share_silent = counted > 0 ? (double)silent / counted : 0.0;
    end.share_stuck = counted > 0 ? (double)stuck / counted : 0.0;
}

void life_end_rhythm(LifeEnd& end, const std::vector<double>& fired_per_tick) {
    if (fired_per_tick.size() < 4 * (size_t)RHYTHM_MAX_LAG) {
        return;
    }
    Rhythm r = rhythm_find(fired_per_tick, RHYTHM_MAX_LAG);
    end.rhythm_period = r.strength < RHYTHM_NONE ? 0 : r.period;
    end.rhythm_strength = r.strength;
}

void life_end_print(const LifeEnd& end, const char* driver, FILE* f) {
    if (!end.measured) {
        return;
    }
    std::fprintf(f,
                 "%-9s room: %.1f connections/neuron, strength 1/4/5 %.2f %.2f %.2f, at the "
                 "cap %.2f, threshold at cap %.2f\n",
                 driver, end.connections_per_neuron, end.share_strength[0], end.share_strength[1],
                 end.share_strength[2], end.share_at_cap, end.share_threshold_cap);
    std::fprintf(f,
                 "%-9s alive: %.1f spikes per 1024 per tick (target %.0f), silent %.3f, stuck "
                 "%.3f, rhythm %d ticks strength %.2f\n",
                 driver, end.rate_per_1024, end.target_per_1024, end.share_silent, end.share_stuck,
                 end.rhythm_period, end.rhythm_strength);
}

void life_end_write(const LifeEnd& end, const char* driver, FILE* f) {
    if (!end.measured) {
        return;
    }
    std::fprintf(f, "%s,connections_per_neuron,%.6g\n", driver, end.connections_per_neuron);
    std::fprintf(f, "%s,share_strength_1,%.6g\n", driver, end.share_strength[0]);
    std::fprintf(f, "%s,share_strength_4,%.6g\n", driver, end.share_strength[1]);
    std::fprintf(f, "%s,share_strength_5,%.6g\n", driver, end.share_strength[2]);
    std::fprintf(f, "%s,share_at_cap,%.6g\n", driver, end.share_at_cap);
    std::fprintf(f, "%s,share_threshold_cap,%.6g\n", driver, end.share_threshold_cap);
    std::fprintf(f, "%s,rate_per_1024,%.6g\n", driver, end.rate_per_1024);
    std::fprintf(f, "%s,target_per_1024,%.6g\n", driver, end.target_per_1024);
    double over = end.target_per_1024 > 0 && end.rate_per_1024 >= 0
                      ? end.rate_per_1024 / end.target_per_1024
                      : -1;
    std::fprintf(f, "%s,rate_over_target,%.6g\n", driver, over);
    std::fprintf(f, "%s,share_silent,%.6g\n", driver, end.share_silent);
    std::fprintf(f, "%s,share_stuck,%.6g\n", driver, end.share_stuck);
    std::fprintf(f, "%s,rhythm_period,%d\n", driver, end.rhythm_period);
    std::fprintf(f, "%s,rhythm_strength,%.6g\n", driver, end.rhythm_strength);
}
