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
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"
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
    std::atomic<int> steps{0};      // single ticks asked for while paused
    std::atomic<int> task_steps{0}; // whole task steps asked for while paused
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
    bool task = false; // run.task through the world plug (real blob only)
    bool twin = true;  // also run the learning-off twin on a second thread
    RunFlags run;      // the task, plug and window, as blobrun takes them
};

// the task side of the sim thread (sim_task.cpp)
struct SimTask {
    bool on = false;
    TaskTicker ticker;
    TaskView view;             // all but the lists, kept up to date tick by tick
    std::vector<float> scores; // per window over the life
};

// the learning-off twin and the coin-flip stand-in, run on their own thread up to the
// main blob's step count, for the score plot's reference lines (sim_twin.cpp)
struct Twin {
    RunFlags run;
    bool learn_off = true; // false: coin flips only
    Blob* blob = nullptr;  // same genome and seed, learning_on = 0
    Blob* coin = nullptr;  // the stand-in
    TaskTicker ticker, coin_ticker;
    int64_t steps = 0;
    std::atomic<int64_t> target{0}; // the main blob's steps
    std::atomic<int64_t> done{0};   // steps both have run
    std::atomic<bool> quit{false};
    std::mutex lock; // guards the score lists
    std::vector<float> scores, coin_scores;
    std::thread thread;
};

struct Sim {
    SimSettings settings;
    Blob* blob = nullptr;          // the real one (BLOB_HAS_CORE)
    FakeBlob* fake_blob = nullptr; // or the fake
    Tape tape;                     // every tick's spikes; the sim thread's own
    SimTask task;
    Twin twin;
    uint64_t rng = 1;
    bool input_was_on = false;
    int fills = 0;
    std::thread thread;
    SimShared shared;
};

bool sim_birth(Sim& sim, const SimSettings& settings);
void sim_free(Sim& sim);
const Blob& sim_blob(const Sim& sim);

// one tick and its tape row; true when a task step was answered on it. Only the sim thread
// calls this once it is started.
bool sim_tick(Sim& sim);
// the tape's new rows and a fresh snapshot of everything else into `out`
void sim_fill(Sim& sim, ViewFrame& out, int selected);

void sim_start(Sim& sim);
void sim_stop(Sim& sim);
// viewer side: copy the latest snapshot into `to` if there is a new one; ask for the next
bool sim_take(Sim& sim, ViewFrame& to);
// a new life with the same settings and learning on or off, ticking again at once
bool sim_restart(Sim& sim, bool learning_on);
// a capture's ticks, on the calling thread (the twin on its own, waited for). returns the
// seconds the ticks took.
double sim_run_capture(Sim& sim, int64_t ticks);

// sim_task.cpp: the task through the world plug, on the sim thread
bool sim_task_start(Sim& sim);
void sim_task_before_tick(Sim& sim);
bool sim_task_after_tick(Sim& sim); // true when a step was answered
void sim_task_fill(Sim& sim, TaskView& out, TaskScores& scores);
// random bits on every sensor line (--input), through blob_sense
void sim_random_input(Sim& sim, bool on);

// sim_twin.cpp
bool twin_birth(Twin& twin, const SimSettings& settings);
void twin_free(Twin& twin);
void twin_start(Twin& twin);
void twin_stop(Twin& twin);
