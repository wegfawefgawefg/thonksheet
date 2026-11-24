#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bench/bench_modes.hpp"

// blobbench: tick speed, the stage 0 idle run, and the small learning check.
//
//   blobbench [speed] [--seconds-ms 400]
//   blobbench stage0 [--ticks 1000000] [--tiles 8] [--seed 1]
//   blobbench learn [--learn 1] [--cues 2] [--trials 20000] [--seed 1] ...

int64_t arg_int(int argc, char** argv, const char* name, int64_t fallback) {
    for (int i = 0; i + 1 < argc; i++) {
        if (std::strcmp(argv[i], name) == 0) {
            return std::strtoll(argv[i + 1], nullptr, 10);
        }
    }
    return fallback;
}

double now_seconds() {
    auto t = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration<double>(t).count();
}

Genome genome_for_sheet(int tiles) {
    Genome g;
    g.tiles_w = tiles;
    g.tiles_h = tiles;
    // the genome's input lattice and motor area, shrunk to fit small sheets
    int cells = tiles * 8;
    int sensor_side = cells / 2 < g.sensor_w ? cells / 2 : g.sensor_w;
    int motor_side = cells / 2 < g.motor_w ? cells / 2 : g.motor_w;
    int motor_at = cells - motor_side < g.motor_x ? cells - motor_side : g.motor_x;
    g.sensor_w = g.sensor_h = sensor_side;
    g.motor_x = g.motor_y = motor_at;
    g.motor_w = g.motor_h = motor_side;
    return g;
}

int main(int argc, char** argv) {
    if (argc >= 2 && std::strcmp(argv[1], "stage0") == 0) {
        return run_stage0(argc - 2, argv + 2);
    }
    if (argc >= 2 && std::strcmp(argv[1], "learn") == 0) {
        return run_learn_check(argc - 2, argv + 2);
    }
    if (argc >= 2 && std::strcmp(argv[1], "speed") == 0) {
        return run_speed(argc - 2, argv + 2);
    }
    if (argc >= 2 && argv[1][0] != '-') {
        std::fprintf(stderr, "usage: blobbench [speed|stage0|learn] [--name value ...]\n");
        return 1;
    }
    return run_speed(argc - 1, argv + 1);
}
