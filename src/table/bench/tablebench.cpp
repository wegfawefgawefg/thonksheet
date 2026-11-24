// tablebench: ticks per second of the table sheet on one core, at several sizes.
// random bits on every sensor cell (1 in 8), good or bad every 20 ticks, so the learning
// path does real work. --t.name value sets table numbers, --g.name value sheet numbers.
//   tablebench                 4,096 / 65,536 / 1,048,576 neurons
//   tablebench --tiles 32      one size (tiles per side)
//   tablebench --seconds 2     time per size

#include "blob/random_words.hpp"
#include "table/table.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static double now_seconds() {
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

static int arg_int(int argc, char** argv, const char* name, int fallback) {
    for (int i = 1; i + 1 < argc; i++) {
        if (std::strcmp(argv[i], name) == 0) {
            return std::atoi(argv[i + 1]);
        }
    }
    return fallback;
}

static void feed_random(Blob& blob, uint64_t* rng) {
    for (int t = 0; t < blob.tile_count; t++) {
        if (!blob.sensor[t]) {
            continue;
        }
        blob.sense_attached[t] |= blob.sensor[t];
        blob.sense_in[t] = blob.sensor[t] & sparse_word(rng, 3);
    }
}

static void bench(int tiles, double seconds, int argc, char** argv) {
    Genome g;
    apply_genome_args(argc, argv, g);
    g.tiles_w = tiles;
    g.tiles_h = tiles;
    TableGenome tg;
    table_genome_args(argc, argv, tg);
    Blob* blob = table_birth(g, tg, 1);
    TableBlob& tb = table_of(*blob);
    uint64_t rng[4];
    rng_seed(rng, 99);
    for (int i = 0; i < 200; i++) {
        feed_random(*blob, rng);
        table_tick(*blob);
    }
    int64_t ticks = 0;
    double start = now_seconds(), elapsed = 0;
    while (elapsed < seconds) {
        for (int i = 0; i < 20; i++) {
            feed_random(*blob, rng);
            table_tick(*blob);
        }
        table_reward(*blob, (rng_next(rng) & 1) ? 1 : -1);
        ticks += 20;
        elapsed = now_seconds() - start;
    }
    double tps = (double)ticks / elapsed;
    std::printf("%10d %3d %12.0f %16.3e %10.1f %10.1f\n", blob->neuron_count, tb.inputs, tps,
                tps * blob->neuron_count, table_bytes_per_neuron(tb),
                100.0 * blob->stats.fired / blob->neuron_count);
    table_free(blob);
}

int main(int argc, char** argv) {
    double seconds = arg_int(argc, argv, "--seconds", 1);
    int only = arg_int(argc, argv, "--tiles", 0);
    std::printf("%10s %3s %12s %16s %10s %10s\n", "neurons", "K", "ticks/s", "neuron-upd/s",
                "bytes/n", "fired %");
    int sizes[] = {8, 32, 128}; // 4,096 / 65,536 / 1,048,576 neurons
    for (int tiles : sizes) {
        if (only == 0 || only == tiles) {
            bench(tiles, seconds, argc, argv);
        }
    }
    if (only != 0 && only != 8 && only != 32 && only != 128) {
        bench(only, seconds, argc, argv);
    }
    return 0;
}
