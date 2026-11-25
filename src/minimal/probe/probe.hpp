#pragma once

#include "blob/blob.hpp"
#include "minimal/minimal.hpp"
#include "run/run_flags.hpp"

#include <cstdint>
#include <vector>

// minimalprobe: measurements of the minimal engine beside the full one, driven the same way.
// probes may use floats; the engines do not.

enum class ProbeEngine { full, minimal };

struct ProbeArgs {
    ProbeEngine engine = ProbeEngine::minimal;
    MinimalGenome mg;
    RunFlags flags; // task, plug, genome, seed, steps: the same flags as blobrun
    int64_t idle_ticks = 2000000;
    int64_t test_steps = 2000;
    int64_t tail_ticks = 65536; // ticks recorded at the end of a life
    int input = 1;              // alive: random bits on the sensors (1) or nothing (0)
    int warmup = 30000;
    int tiles = 0; // speed: one size only
    int seconds_ms = 2000;
    int64_t teach_steps = 0; // linear: steps learned before the sheet is frozen and read
    int trials = 4000;       // linear: scored steps kept for the reader
};

// argv after the mode word; false after printing a message
bool probe_args(int argc, char** argv, ProbeArgs& args);

// either engine through one door
Blob* probe_birth(const ProbeArgs& args, const Genome& genome, uint64_t seed);
void probe_free(const ProbeArgs& args, Blob* blob);
void probe_tick(const ProbeArgs& args, Blob& blob);
void probe_reward(const ProbeArgs& args, Blob& blob, int valence);
void probe_set_learning(const ProbeArgs& args, Blob& blob, int on);
const char* probe_engine_name(const ProbeArgs& args);
// bytes per neuron the engine allocates (its own arrays, not the plug's)
double probe_bytes_per_neuron(const ProbeArgs& args, const Blob& blob);

// what a stretch of ticks looked like: per tick totals and per neuron counts
struct Recorder {
    std::vector<double> fired;    // spikes per tick, sensors excluded
    std::vector<uint32_t> count;  // per neuron spikes
    std::vector<uint32_t> driven; // per neuron spikes that were not noise (minimal only)
    int64_t ticks = 0;
};
void recorder_start(Recorder& r, const Blob& blob);
void recorder_add(Recorder& r, const ProbeArgs& args, const Blob& blob);
// rate against target, Fano, pulse period and strength, silent and stuck neurons
void recorder_print(const Recorder& r, const ProbeArgs& args, const Blob& blob, const char* label);
// connections, thresholds: what the wiring looks like now
void print_wiring(const ProbeArgs& args, const Blob& blob, const char* label);

int probe_alive(ProbeArgs& args);
int probe_life(ProbeArgs& args);
int probe_idle(ProbeArgs& args);
int probe_speed(ProbeArgs& args);
int probe_trace(ProbeArgs& args);
int probe_linear(ProbeArgs& args);
