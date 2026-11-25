#include "world/standin.hpp"

#include "world/world_random.hpp"

#include <bit>
#include <cstdlib>

static uint64_t* words(int n) {
    return (uint64_t*)std::calloc((size_t)n, sizeof(uint64_t));
}

// set the bits of every cell inside a rectangle of the sheet
static void mark_region(const Blob& blob, uint64_t* mask, int x0, int y0, int w, int h) {
    for (int y = y0; y < y0 + h; y++) {
        for (int x = x0; x < x0 + w; x++) {
            if (x < 0 || y < 0 || x >= blob.tiles_w * 8 || y >= blob.tiles_h * 8) {
                continue;
            }
            int n = blob_neuron_at(blob, x, y);
            mask[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
        }
    }
}

Blob* standin_birth(const Genome& genome, uint64_t seed) {
    Blob* blob = new Blob{};
    blob->genome = genome;
    blob->tiles_w = genome.tiles_w;
    blob->tiles_h = genome.tiles_h;
    blob->tile_count = genome.tiles_w * genome.tiles_h;
    blob->neuron_count = blob->tile_count * TILE_NEURONS;
    int n = blob->tile_count + 1;
    for (int h = 0; h < HISTORY; h++) {
        blob->spikes[h] = words(n);
    }
    blob->recent = words(n);
    blob->inhibitory = words(n);
    blob->sensor = words(n);
    blob->motor = words(n);
    blob->sense_in = words(n);
    blob->sense_attached = words(n);
    blob->predicted = words(n);
    mark_region(*blob, blob->sensor, genome.sensor_x, genome.sensor_y, genome.sensor_w,
                genome.sensor_h);
    mark_region(*blob, blob->motor, genome.motor_x, genome.motor_y, genome.motor_w, genome.motor_h);
    blob->rng[0] = seed ^ 0x5374616e64696e00ull; // "Standin"
    return blob;
}

void standin_free(Blob* blob) {
    if (!blob) {
        return;
    }
    for (int h = 0; h < HISTORY; h++) {
        std::free(blob->spikes[h]);
    }
    std::free(blob->recent);
    std::free(blob->inhibitory);
    std::free(blob->sensor);
    std::free(blob->motor);
    std::free(blob->sense_in);
    std::free(blob->sense_attached);
    std::free(blob->predicted);
    delete blob;
}

void standin_tick(Blob& blob, const Plug& plug, StandinPlay play, int perfect_move) {
    WorldRandom random{blob.rng[0]};
    blob.now = (blob.now + 1) & (HISTORY - 1);
    uint64_t* map = blob.spikes[blob.now];
    uint32_t surprise = 0;
    uint32_t fired = 0;
    for (int t = 0; t < blob.tile_count; t++) {
        // sensor neurons fire exactly as the world says
        uint64_t sensed = blob.sense_in[t] & blob.sensor[t];
        uint64_t predicted = sensed;
        uint64_t motor = 0;
        if (play == StandinPlay::coin) {
            predicted = world_random_word(random) & blob.sensor[t];
            motor = world_random_word(random) & world_random_word(random) & blob.motor[t];
        }
        blob.predicted[t] = predicted;
        surprise += std::popcount((predicted ^ sensed) & blob.sense_attached[t] & blob.sensor[t]);
        map[t] = sensed | motor;
    }
    // a pattern answer (idea 134): perfect_move is the answer's bits; each bit's right
    // group fires
    for (int b = 0; play == StandinPlay::perfect && perfect_move >= 0 &&
                    b < plug.shape.pattern_bits;
         b++) {
        const int* cells = nullptr;
        int group = plug_motor_cells(plug, 2 * b + ((perfect_move >> b) & 1), &cells);
        for (int k = 0; k < group; k++) {
            map[cells[k] / TILE_NEURONS] |= 1ull << (cells[k] % TILE_NEURONS);
        }
    }
    if (play == StandinPlay::perfect && perfect_move >= 0 && perfect_move < plug.motor_lines &&
        plug.shape.pattern_bits == 0) {
        const int* cells = nullptr;
        int group = plug_motor_cells(plug, perfect_move, &cells);
        for (int k = 0; k < group; k++) {
            int n = cells[k];
            map[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
        }
    }
    for (int t = 0; t < blob.tile_count; t++) {
        fired += std::popcount(map[t]);
    }
    blob.rng[0] = random.state;
    blob.stats.tick++;
    blob.stats.fired = fired;
    blob.stats.surprise = surprise;
}
