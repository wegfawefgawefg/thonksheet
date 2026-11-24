#pragma once
// A fake blob: a real Blob struct, filled and moved by the viewer itself, so the viewer can
// be built and looked at before the core exists, and so each kind of trouble the viewer must
// show (dying, saturating, cycling) can be shown on purpose. Nothing here is how the real
// blob works; the activity is drawn by chance from made-up rates.

#include "blob/blob.hpp"

#include <vector>

enum FakeScene {
    FAKE_MIXED,      // mostly healthy, one sector each dying, saturating, cycling
    FAKE_HEALTHY,    // sparse waves, a sweeping input bar, pacemakers
    FAKE_DYING,      // everything fades to silence
    FAKE_SATURATING, // everything fires as often as cooldown allows
    FAKE_CYCLING,    // one pattern repeating every 24 ticks
    FAKE_SCENE_COUNT,
};
extern const char* const fake_scene_names[FAKE_SCENE_COUNT];

enum FakeMood { MOOD_HEALTHY, MOOD_DYING, MOOD_SATURATING, MOOD_CYCLING };

struct FakeBlob {
    Blob blob; // its arrays point into the vectors below
    FakeScene scene = FAKE_MIXED;
    uint64_t rng = 0;
    std::vector<uint64_t> spike_store, word_store, weight_store;
    std::vector<uint32_t> local_store, far_store, sector_store;
    std::vector<int16_t> potential, threshold, sector_bias;
    std::vector<uint8_t> leak_shift, cooldown, cooldown_len, far_delay, pacemaker;
    std::vector<uint16_t> fan_in;
    std::vector<uint8_t> mood;  // per sector
    std::vector<uint8_t> phase; // per neuron, its place in the cycle
};

FakeBlob* fake_blob_birth(const Genome& genome, uint64_t seed, FakeScene scene);
void fake_blob_free(FakeBlob* fake);
void fake_blob_tick(FakeBlob& fake);
void fake_blob_reward(FakeBlob& fake, int valence);

// fake_birth.cpp, shared with fake_tick.cpp
uint64_t fake_random(uint64_t& state);
float fake_chance(uint64_t& state); // 0..1
