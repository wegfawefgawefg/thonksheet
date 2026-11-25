#pragma once

#include "blob/blob.hpp"
#include "gpu/gpu_batch.hpp"
#include "run/run_flags.hpp"
#include "world/plug.hpp"
#include "world/task.hpp"

#include <cstdint>
#include <string>
#include <vector>

// Host worlds: the sheet steps on the GPU, the world (the real task and plug code of
// src/world) on the CPU, and once per task step they exchange what each side needs. Any task
// runs this way with no shader change. Per life and step:
//
//   GPU -> host  the step's motor counts per button
//   host         the plug's choice, task_step (good, bad or nothing), the window score, then
//                the next step's task_sense and plug lines, written out as sensor words
//   host -> GPU  the reward and the pressed button, and the next step's sensor words
//
// The GPU applies the reward at the start of its next dispatch, before the rest ticks: the
// same point in the life as blobrun's loop (src/run/life.cpp), so a life is the CPU's bit for
// bit. Good and bad lines and rest ticks are counted on the GPU (plug_reward, plug_write).

// one world of a batch: a task, its settings and the plug laid on the batch's sheet
struct HostWorld {
    std::string name; // as given (--task or one entry of --tasks)
    RunFlags flags;   // the task, its settings, steps and window; the plug
    Plug plug;        // the layout, read only (each thread steps a copy)
    int motor_lines = 0;
    uint32_t attached = 0; // lines attached: bit 0 good, 1 bad, 31 any task line
};

// a life's world on the host
struct HostLife {
    Task task;
    int world = 0;
    int64_t step = 0;
    int valence = 0, move = -1; // the step just answered
    bool done = false;
    std::vector<double> scores; // one per window
};

// time spent per part, summed over the life of the batch
struct HostTimes {
    double card = 0;     // dispatches, submit to fence
    double exchange = 0; // counts out of, words and rewards into the mapped buffers
    double world = 0;    // the host worlds: choice, task step, sense, plug lines
    int64_t exchanges = 0;
};

// the worlds of a batch: one from the command line's task, or one per entry of a --tasks
// list ("echo:k=8,order,gap,catch:catch-size=4/4": each key=value is that blobrun flag)
bool host_worlds_parse(const std::string& tasks, const std::vector<std::string>& run_args,
                       const RunFlags& base, std::vector<HostWorld>& worlds);
// lay each world's plug on a born sheet and fill its table block (GPU_WORLD_ROWS x tiles + 1
// words: sensor, good and bad lines, buttons). false with the reason.
bool host_worlds_build(std::vector<HostWorld>& worlds, Blob& born, std::vector<uint64_t>& words,
                       std::string& why);
// a newborn life's world (its task seeded as blobrun seeds it, task seed = seed)
void host_life_start(HostLife& h, int world, uint64_t seed, const HostWorld& w);

// what the host keeps per thread: a copy of each world's plug, a shell to write lines onto
struct HostScratch {
    std::vector<Plug> plugs;
    Blob shell;
    std::vector<uint64_t> sense_in, attached;
    TaskSense sense;
    std::vector<uint64_t> words; // lives x (tiles + 1)
    std::vector<GpuExchangeIn> xin;
    std::vector<GpuExchangeOut> xout;
};
void host_scratch_open(HostScratch& s, const std::vector<HostWorld>& worlds, int tiles);

// the host's half of one exchange for lives [from, to) of a group (indices into the group):
// read the counts, answer the step (answer false: the first exchange, nothing to answer),
// write the next step's words. times add up in `t`.
void host_exchange(const std::vector<HostWorld>& worlds, std::vector<HostLife>& host,
                   GpuGroup& group, int from, int to, bool answer, int tiles, HostScratch& s,
                   HostTimes& t);
