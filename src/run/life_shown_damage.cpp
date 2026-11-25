#include "run/life.hpp"
#include "world/world_random.hpp"

#include <cstdlib>

// the world's two hooks into every engine's fire step (blob.silenced, blob.forced): damage
// silences cells for the rest of the life, being shown the answer makes the right button's
// cells fire. the arrays are the world's, made on first use and freed before the blob dies.

static uint64_t* words_for(const Blob& blob) {
    return (uint64_t*)std::calloc((size_t)blob.tile_count + 1, sizeof(uint64_t));
}

void life_damage(Blob& blob, const Plug& plug, const LifePhase& phase, uint64_t seed) {
    if (!blob.silenced) {
        blob.silenced = words_for(blob);
    }
    // its own dice, so the same seed silences the same cells whatever the engine
    WorldRandom r;
    world_random_seed(r, seed * 0x9e3779b97f4a7c15ull + 0xda3a6e);
    for (int t = 0; t < blob.tile_count && phase.damage_pct > 0; t++) {
        for (int b = 0; b < TILE_NEURONS; b++) {
            bool sensor = (blob.sensor[t] >> b) & 1;
            if (!sensor && world_random_below(r, 100) < phase.damage_pct) {
                blob.silenced[t] |= 1ull << b;
            }
        }
    }
    if (phase.damage_tile) {
        // the tile holding the first cue line: every cell there that is not a sensor
        int first = plug.sensor_lines > plug.first_single ? plug.first_single : 0;
        int tile = plug.sensor_neuron[first * PLUG_MAX_GROUP] / TILE_NEURONS;
        blob.silenced[tile] |= ~blob.sensor[tile];
    }
}

void life_force(Blob& blob, const Plug& plug, int button) {
    if (!blob.forced) {
        blob.forced = words_for(blob);
    }
    for (int t = 0; t <= blob.tile_count; t++) {
        blob.forced[t] = 0;
    }
    if (button < 0 || button >= plug.motor_lines) {
        return;
    }
    const int* cells = nullptr;
    int count = plug_motor_cells(plug, button, &cells);
    for (int k = 0; k < count; k++) {
        blob.forced[cells[k] / TILE_NEURONS] |= 1ull << (cells[k] % TILE_NEURONS);
    }
}

void life_hooks_free(Blob& blob) {
    std::free(blob.silenced);
    std::free(blob.forced);
    blob.silenced = nullptr;
    blob.forced = nullptr;
}
