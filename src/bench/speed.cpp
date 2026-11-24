#include <cstdio>

#include "bench/bench_modes.hpp"
#include "blob/blob.hpp"
#include "blob/random_words.hpp"

// ticks per second at several sheet sizes, one core, learning off and on.
// "on" also feeds random input to every sensor line and rewards every 50 ticks, so the
// learning paths do real work.

static void feed_random(Blob& blob, uint64_t* rng) {
    for (int t = 0; t < blob.tile_count; t++) {
        if (!blob.sensor[t]) {
            continue;
        }
        blob.sense_attached[t] |= blob.sensor[t];
        blob.sense_in[t] = blob.sensor[t] & sparse_word(rng, 3);
    }
}

static double ticks_per_second(int tiles, bool learn, double seconds, int argc, char** argv) {
    Genome g = genome_for_sheet(tiles);
    apply_genome_args(argc, argv, g);
    g.learning_on = learn ? 1 : 0;
    Blob* blob = blob_birth(g, 1);
    uint64_t rng[4];
    rng_seed(rng, 99);

    // settle first, then time whole batches until the time is used up
    for (int i = 0; i < 2000; i++) {
        if (learn) {
            feed_random(*blob, rng);
        }
        blob_tick(*blob);
    }
    int64_t ticks = 0;
    double start = now_seconds(), elapsed = 0;
    while (elapsed < seconds) {
        for (int i = 0; i < 200; i++) {
            if (learn) {
                feed_random(*blob, rng);
                if ((ticks + i) % 50 == 0) {
                    blob_reward(*blob, (rng_next(rng) & 1) ? 1 : -1);
                }
            }
            blob_tick(*blob);
        }
        ticks += 200;
        elapsed = now_seconds() - start;
    }
    blob_free(blob);
    return (double)ticks / elapsed;
}

int run_speed(int argc, char** argv) {
    double seconds = (double)arg_int(argc, argv, "--seconds-ms", 400) / 1000.0;
    int sizes[] = {4, 8, 16, 32}; // tiles per side: 1,024 / 4,096 / 16,384 / 65,536 neurons
    std::printf("%8s %6s %14s %18s\n", "neurons", "learn", "ticks/s", "neuron-updates/s");
    int only_tiles = (int)arg_int(argc, argv, "--tiles", 0);
    int only_learn = (int)arg_int(argc, argv, "--learn", -1);
    for (int tiles : sizes) {
        if (only_tiles && tiles != only_tiles) {
            continue;
        }
        for (int learn = 0; learn < 2; learn++) {
            if (only_learn >= 0 && learn != only_learn) {
                continue;
            }
            double tps = ticks_per_second(tiles, learn, seconds, argc, argv);
            int neurons = tiles * tiles * 64;
            std::printf("%8d %6s %14.0f %18.3e\n", neurons, learn ? "on" : "off", tps,
                        tps * neurons);
            std::fflush(stdout);
        }
    }
    return 0;
}
