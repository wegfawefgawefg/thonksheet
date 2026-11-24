// fake_blob_tick: made-up activity written into the Blob fields the viewer reads. Each
// sector has a mood (healthy, dying, saturating, cycling) so each kind of trouble can be
// looked at on purpose.

#include "fake_blob.hpp"

#include <algorithm>
#include <bit>
#include <cmath>

constexpr int CYCLE = 24;

// the chance this neuron fires now, from its sector's mood
static float fire_chance(const FakeBlob& f, int n, int x, int y, int mood, uint64_t tick) {
    const Blob& b = f.blob;
    const float t = static_cast<float>(tick);
    const uint64_t bit = 1ull << (n % TILE_NEURONS);
    const int tile = n / TILE_NEURONS;
    if (mood == MOOD_CYCLING) {
        return (tick + f.phase[n]) % CYCLE == 0 ? 0.95f : 0.002f;
    }
    if (mood == MOOD_SATURATING) {
        return 0.85f;
    }
    if (f.pacemaker[n]) {
        return 1.0f; // fires whenever its cooldown lets it
    }
    // sensors: a bar sweeping across the sensor region, as if something were plugged in
    const Genome& g = b.genome;
    if (b.sensor[tile] & bit) {
        const int bar = 2 * (static_cast<int>(tick / 6) % std::max(1, g.sensor_w / 2));
        return x - g.sensor_x == bar ? 0.7f : 0.01f;
    }
    // everything else: sparse noise lifted by two slow travelling waves
    const float w1 = std::sin(0.23f * x + 0.11f * y - 0.07f * t);
    const float w2 = std::sin(-0.09f * x + 0.19f * y - 0.045f * t + 1.3f);
    const float wave = std::max(0.0f, w1 + w2 - 0.6f);
    float rate = 0.012f + 0.09f * wave * wave;
    if (b.inhibitory[tile] & bit) {
        rate *= 1.6f;
    }
    if (mood == MOOD_DYING) {
        rate *= std::exp(-t / 1500.0f);
    }
    return rate;
}

// grow, prune, strengthen and weaken a few weights at random so the histograms move
static void fake_learning(FakeBlob& f, int n) {
    Blob& b = f.blob;
    const int k = static_cast<int>(fake_random(f.rng) % LOCAL_SLOTS);
    if (b.local_tile[(n / TILE_NEURONS) * LOCAL_SLOTS + k] >= static_cast<uint32_t>(b.tile_count)) {
        return;
    }
    uint64_t& w0 = b.w0[n * SLOTS + k];
    uint64_t& w1 = b.w1[n * SLOTS + k];
    const uint64_t bit = 1ull << (fake_random(f.rng) % TILE_NEURONS);
    const uint64_t roll = fake_random(f.rng) % 64;
    const bool empty = ((w0 | w1) & bit) == 0;
    const bool weak = (w0 & ~w1 & bit) != 0;
    if (roll < 2 && empty && b.fan_in[n] < b.genome.fan_in_max) {
        w0 |= bit;
        b.fan_in[n] += 1;
        b.stats.grown += 1;
    } else if (roll < 4 && weak) {
        w0 &= ~bit;
        b.fan_in[n] -= 1;
        b.stats.pruned += 1;
    } else if (roll < 12 && weak && b.good_ticks > 0) {
        w0 &= ~bit;
        w1 |= bit;
        b.stats.strengthened += 1;
    } else if (roll < 12 && (w1 & bit) && b.bad_ticks > 0) {
        w1 &= ~bit;
        w0 |= bit;
        b.stats.weakened += 1;
    }
}

static void fire_tile(FakeBlob& f, int tile, uint64_t* map) {
    Blob& b = f.blob;
    const int mood = f.mood[b.tile_sector[tile]];
    uint64_t word = 0;
    for (int bit = 0; bit < TILE_NEURONS; ++bit) {
        const int n = tile * TILE_NEURONS + bit;
        if (b.cooldown[n] > 0) {
            b.cooldown[n] -= 1;
            b.potential[n] = static_cast<int16_t>(b.potential[n] >> 1);
            continue;
        }
        const int x = blob_neuron_x(b, n);
        const int y = blob_neuron_y(b, n);
        const float p = fire_chance(f, n, x, y, mood, b.stats.tick);
        if (fake_chance(f.rng) < p) {
            word |= 1ull << bit;
            b.cooldown[n] = b.cooldown_len[n];
            b.potential[n] = 0;
            if (fake_random(f.rng) % 16 == 0 && b.threshold[n] < 40 && !f.pacemaker[n]) {
                b.threshold[n] += 1;
            }
            fake_learning(f, n);
        } else {
            const int target = static_cast<int>(p * 30.0f * (b.threshold[n] + 1));
            b.potential[n] = static_cast<int16_t>((b.potential[n] + target) / 2);
            if (fake_random(f.rng) % 512 == 0 && b.threshold[n] > b.genome.threshold_min) {
                b.threshold[n] -= 1;
            }
        }
    }
    map[tile] = word;
}

// smoothed spikes per sector, and a bias that leans against being off target
static void regulate(FakeBlob& f) {
    Blob& b = f.blob;
    const uint64_t* map = b.spikes[b.now];
    std::vector<uint32_t> count(b.sector_count, 0), tiles(b.sector_count, 0);
    for (int t = 0; t < b.tile_count; ++t) {
        count[b.tile_sector[t]] += std::popcount(map[t]);
        tiles[b.tile_sector[t]] += 1;
    }
    for (int s = 0; s < b.sector_count; ++s) {
        uint32_t& a = b.sector_activity[s];
        a = a - (a >> 4) + ((count[s] * 256) >> 4);
        const uint32_t target = static_cast<uint32_t>(b.genome.sector_target_per_1024) * tiles[s] *
                                TILE_NEURONS * 256 / 1024;
        const int max = b.genome.sector_bias_max;
        if (a > target + target / 8 && b.sector_bias[s] < max) {
            b.sector_bias[s] += 1;
        } else if (a + target / 8 < target && b.sector_bias[s] > -max) {
            b.sector_bias[s] -= 1;
        }
    }
}

// sensor guesses: mostly right where the bar is, so surprise stays low but not zero
static void predict(FakeBlob& f) {
    Blob& b = f.blob;
    const uint64_t* map = b.spikes[b.now];
    uint32_t wrong = 0;
    for (int t = 0; t < b.tile_count; ++t) {
        const uint64_t actual = map[t] & b.sensor[t];
        const uint64_t miss = fake_random(f.rng) & fake_random(f.rng) & fake_random(f.rng);
        b.predicted[t] = (actual ^ (miss & b.sensor[t]));
        wrong += std::popcount(b.predicted[t] ^ actual);
        b.sector_surprise[b.tile_sector[t]] = b.sector_surprise[b.tile_sector[t]] * 15 / 16 +
                                              std::popcount(b.predicted[t] ^ actual) * 16;
    }
    b.stats.surprise = wrong;
    b.surprise_high =
        static_cast<int>(wrong) * 1024 > b.genome.surprise_high_per_1024 * b.neuron_count;
}

void fake_blob_tick(FakeBlob& f) {
    Blob& b = f.blob;
    b.stats.grown = b.stats.pruned = b.stats.strengthened = b.stats.weakened = 0;

    // fire into the next spike map
    const int next = (b.now + 1) & (HISTORY - 1);
    uint64_t* map = b.spikes[next];
    for (int t = 0; t < b.tile_count; ++t) {
        fire_tile(f, t, map);
    }
    map[b.tile_count] = 0;
    b.now = next;

    // "fired recently": the OR of the last few maps
    const int recent = std::clamp(b.genome.recent_ticks, 1, HISTORY);
    uint32_t fired = 0;
    for (int t = 0; t < b.tile_count; ++t) {
        uint64_t any = 0;
        for (int age = 0; age < recent; ++age) {
            any |= b.spikes[(b.now - age) & (HISTORY - 1)][t];
        }
        b.recent[t] = any;
        fired += std::popcount(map[t]);
    }
    b.stats.fired = fired;

    regulate(f);
    predict(f);

    // good and bad now and then
    b.good_ticks = std::max(0, b.good_ticks - 1);
    b.bad_ticks = std::max(0, b.bad_ticks - 1);
    if (fake_random(f.rng) % 600 == 0) {
        fake_blob_reward(f, fake_random(f.rng) % 3 == 0 ? -1 : 1);
    }
    b.stats.tick += 1;
}
