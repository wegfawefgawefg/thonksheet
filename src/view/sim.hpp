#pragma once
// The blob and the thread that ticks it. The sim thread owns the blob outright and ticks it
// flat out (or slowly, or not at all). The viewer never touches the blob; it asks for a
// snapshot and copies it out.
//
// Locking, all of it: one mutex, `shared.lock`, guards `shared.frame` and
// `shared.frame_ready`. The sim thread takes it only to fill the frame when asked; the viewer
// takes it only to copy the frame out. Everything else the two sides tell each other is a
// single atomic number.

#include "fake_blob.hpp"
#include "view_frame.hpp"

#include <atomic>
#include <mutex>
#include <thread>

enum RunMode {
    RUN_PAUSED,
    RUN_SLOW,  // slow_tps ticks per second
    RUN_FULL,  // flat out, drawn
    RUN_TURBO, // flat out, nothing drawn but the tick rate
};

struct SimShared {
    std::mutex lock;
    ViewFrame frame;
    bool frame_ready = false;

    // asks from the viewer
    std::atomic<bool> want_frame{false};
    std::atomic<bool> quit{false};
    std::atomic<int> mode{RUN_FULL};
    std::atomic<int> slow_tps{20};
    std::atomic<int> steps{0}; // single steps asked for while paused
    std::atomic<int> selected{-1};
    std::atomic<int> reward{0};        // +1 good, -1 bad, taken once
    std::atomic<bool> input_on{false}; // random bits on the sensor neurons (real blob)

    // told back
    std::atomic<uint64_t> ticks{0};
};

struct SimSettings {
    bool fake = true;
    FakeScene scene = FAKE_MIXED;
    Genome genome;
    uint64_t seed = 1;
};

struct Sim {
    SimSettings settings;
    Blob* blob = nullptr;          // the real one (BLOB_HAS_CORE)
    FakeBlob* fake_blob = nullptr; // or the fake
    Tape tape;                     // every tick's spikes; the sim thread's own
    uint64_t rng = 1;
    int fills = 0;
    std::thread thread;
    SimShared shared;
};

bool sim_birth(Sim& sim, const SimSettings& settings);
void sim_free(Sim& sim);
const Blob& sim_blob(const Sim& sim);

// one tick and its tape row. Only the sim thread calls this once it is started.
void sim_tick(Sim& sim);
// the tape's new rows and a fresh snapshot of everything else into `out`
void sim_fill(Sim& sim, ViewFrame& out, int selected);

void sim_start(Sim& sim);
void sim_stop(Sim& sim);
// viewer side: copy the latest snapshot into `to` if there is a new one; ask for the next
bool sim_take(Sim& sim, ViewFrame& to);
