// blobrun --reach and --speed: measurements with no task, through the same driver calls as
// every life, so they work for whatever engine blobrun drives.

#include "probe/carry_measure.hpp"
#include "run/life_protocols.hpp"

#include <chrono>
#include <malloc.h>
#include <string>

static Plug no_plug; // nothing attached; the stand-in tick only looks at its motor lines

// reach: blobprobe carry's measure (probe/carry_measure.hpp) with the cue tile at the sensor
// region's corner. blocks of 20 ticks, the first 6 of each not counted, one cue on per block
// by the same coin as the probe, so --seed 1 matches `blobprobe carry --seed 1`.
static bool reach_life(Driver driver, const RunFlags& flags, double* readable, int& distances,
                       int& reach) {
    const int block = 20, skip = 6;
    Genome genome = flags.genome;
    if (driver == Driver::learn_off) {
        genome_learning_off(genome);
    }
    Blob* blob = birth(driver, genome, flags.seed);
    CarryMeasure m;
    carry_begin(m, *blob, genome.sensor_x / 8, genome.sensor_y / 8);
    uint64_t state = 0x2545F4914F6CDD1Dull ^ flags.seed;
    int settle_blocks = flags.reach_settle / block;
    for (int b = 0; b < settle_blocks + flags.reach_blocks; b++) {
        state ^= state << 13;
        state ^= state >> 7;
        state ^= state << 17;
        int which = (int)(state >> 40) & 1;
        carry_set_cue(*blob, m.cue_tile, which);
        carry_block_start(m);
        for (int t = 0; t < block; t++) {
            tick(driver, *blob, no_plug, -1);
            if (t >= skip && b >= settle_blocks) {
                carry_count_tick(m, *blob);
            }
        }
        if (b >= settle_blocks) {
            carry_block_end(m, *blob, which);
        }
    }
    distances = carry_readable(m, *blob, readable, reach);
    death(driver, blob);
    return true;
}

int protocol_reach(const RunFlags& flags) {
    std::printf("reach: cue A or B on one corner tile's sensor cells, %d ticks to settle, %d "
                "blocks of 20 ticks, no reward\n",
                flags.reach_settle, flags.reach_blocks);
    FILE* sum = summary_open(flags.out);
    if (!sum) {
        return 1;
    }
    double readable[(int)Driver::count][CARRY_MAX_DISTANCE] = {};
    int distances = 0;
    bool ran[(int)Driver::count] = {};
    std::printf("%-10s distance in tiles, 0 to 7; reach = farthest at %.1f or more\n", "",
                CARRY_LOST);
    for (int d = 0; d <= (int)Driver::learn_off; d++) {
        bool want[(int)Driver::count];
        drivers_wanted(flags, want);
        if (!want[d] || !real_blob((Driver)d)) {
            continue;
        }
        int reach = 0;
        reach_life((Driver)d, flags, readable[d], distances, reach);
        ran[d] = true;
        std::printf("%-10s", driver_names[d]);
        for (int k = 0; k < distances; k++) {
            std::printf(" %5.2f", readable[d][k]);
            std::fprintf(sum, "%s,readable_%d,%.6g\n", driver_names[d], k, readable[d][k]);
        }
        std::printf("   reach %d\n", reach);
        std::fprintf(sum, "%s,reach,%d\n", driver_names[d], reach);
    }
    std::fclose(sum);
    if (!ran[(int)Driver::blob]) {
        std::printf("reach needs a real blob; nothing ran\n");
        return 0;
    }
    FILE* f = std::fopen(flags.out.c_str(), "w");
    if (f) {
        std::fprintf(f, "distance");
        for (int d = 0; d < (int)Driver::count; d++) {
            std::fprintf(f, ran[d] ? ",%s" : "", driver_names[d]);
        }
        std::fprintf(f, "\n");
        for (int k = 0; k < distances; k++) {
            std::fprintf(f, "%d", k);
            for (int d = 0; d < (int)Driver::count; d++) {
                if (ran[d]) {
                    std::fprintf(f, ",%.6f", readable[d][k]);
                }
            }
            std::fprintf(f, "\n");
        }
        std::fclose(f);
    }
    std::printf("csv: %s\n", flags.out.c_str());
    return 0;
}

// heap bytes in use, from the allocator
static double heap_bytes() {
    struct mallinfo2 info = mallinfo2();
    return (double)info.uordblks + (double)info.hblkhd;
}

static uint64_t speed_random(uint64_t& state) {
    state ^= state << 13;
    state ^= state >> 7;
    state ^= state << 17;
    return state;
}

// speed: one blob per size with random bits on every sensor cell (1 in 8, as blobprobe
// rhythm), learning on, ticked for --speed-ms; bytes per neuron is the heap the birth took
int protocol_speed(const RunFlags& flags) {
    bool want[(int)Driver::count];
    drivers_wanted(flags, want);
    if (!want[(int)Driver::blob]) {
        std::printf("speed needs a real blob; nothing ran\n");
        return 0;
    }
    FILE* sum = summary_open(flags.out);
    if (!sum) {
        return 1;
    }
    std::printf("speed: one core, random input, learning on, %d ms per size\n", flags.speed_ms);
    const int sizes[3] = {8, 16, 32};
    for (int size : sizes) {
        Genome genome = flags.genome;
        genome.tiles_w = size;
        genome.tiles_h = size;
        double before = heap_bytes();
        Blob* blob = birth(Driver::blob, genome, flags.seed);
        double bytes = (heap_bytes() - before) / blob->neuron_count;
        uint64_t state = 0x9E3779B97F4A7C15ull ^ flags.seed;
        int64_t ticks = 0;
        auto start = std::chrono::steady_clock::now();
        double seconds = 0;
        while (seconds * 1000.0 < flags.speed_ms || ticks < 10) {
            for (int i = 0; i < 16; i++) {
                for (int t = 0; t < blob->tile_count; t++) {
                    uint64_t on = speed_random(state) & speed_random(state) & speed_random(state);
                    blob->sense_attached[t] = blob->sensor[t];
                    blob->sense_in[t] = blob->sensor[t] & on;
                }
                tick(Driver::blob, *blob, no_plug, -1);
                ticks++;
            }
            seconds =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        }
        double per_s = (double)ticks / seconds;
        std::printf("%2dx%-2d tiles %7d neurons  %9.0f ticks/s  %8.3g neuron updates/s  %6.0f "
                    "bytes/neuron\n",
                    size, size, blob->neuron_count, per_s, per_s * blob->neuron_count, bytes);
        std::fprintf(sum, "blob,ticks_per_s_%dx%d,%.6g\n", size, size, per_s);
        std::fprintf(sum, "blob,bytes_per_neuron_%dx%d,%.6g\n", size, size, bytes);
        death(Driver::blob, blob);
    }
    std::fclose(sum);
    return 0;
}
