// blobrun: one blob, one task, one long life. Score per window to csv, late minus early,
// then the same task with learning off and with coin-flip motors. Without the core it runs
// the stand-in only: coin flips and a scripted perfect player. The life protocols of the
// suite (retention, idle, capacity, reach, speed) are in life_protocols.cpp.

#include "run/drive.hpp"
#include "run/life_protocols.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"
#include "world/plug.hpp"
#include "world/plug_print.hpp"
#include "world/standin.hpp"
#include "world/task.hpp"

#include <cstdio>
#include <string>

// who drives the motor lines (drive.hpp)
const char* const driver_names[(int)Driver::count] = {"blob", "learn_off", "coin", "perfect"};

// the learning-off twin: same genome and seed, nothing changes its wiring
void genome_learning_off(Genome& g) {
    g.learning_on = 0;
}

bool real_blob(Driver driver) {
    return driver == Driver::blob || driver == Driver::learn_off;
}

Blob* birth(Driver driver, const Genome& genome, uint64_t seed) {
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        return blob_birth(genome, seed);
    }
#endif
    (void)driver;
    return standin_birth(genome, seed);
}

void death(Driver driver, Blob* blob) {
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        blob_free(blob);
        return;
    }
#endif
    (void)driver;
    standin_free(blob);
}

void tick(Driver driver, Blob& blob, const Plug& plug, int perfect_move) {
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        blob_tick(blob);
        return;
    }
#endif
    StandinPlay play = driver == Driver::perfect ? StandinPlay::perfect : StandinPlay::coin;
    standin_tick(blob, plug, play, perfect_move);
}

void reward(Driver driver, Blob& blob, int valence) {
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        blob_reward(blob, valence);
    }
#endif
    (void)driver;
    (void)blob;
    (void)valence;
}

int main(int argc, char** argv) {
    RunFlags flags;
    apply_genome_args(argc, argv, flags.genome);
    if (!run_flags_parse(argc, argv, flags)) {
        return 1;
    }
    if (flags.out.empty()) {
        // protocols other than one plain life are named before the task
        const char* const protocol_names[] = {"", "retention-", "idle-", "capacity-", "", ""};
        std::string what = protocol_names[(int)flags.protocol];
        what += task_name(flags.task);
        what = flags.protocol == Protocol::reach ? "reach" : what;
        what = flags.protocol == Protocol::speed ? "speed" : what;
        flags.out = "out/" + what + "-s" + std::to_string(flags.seed) + ".csv";
    }
    if (flags.frozen_readout) {
        genome_readout_only(flags.genome);
    }
#ifndef BLOB_HAS_CORE
    std::printf("no core linked: running the stand-in only (coin flips, perfect player)\n");
#endif

    bool has_task = flags.protocol != Protocol::reach && flags.protocol != Protocol::speed;
    if (has_task) {
        std::printf("task %s  seed %llu  task seed %llu  steps %lld  ticks/step %d  window %d\n",
                    task_name(flags.task), (unsigned long long)flags.seed,
                    (unsigned long long)flags.task_seed, (long long)flags.steps,
                    flags.plug.ticks_per_step, flags.window);
    }
    if (flags.show_plug && has_task) {
        // the layout depends only on the genome and task shape, so a stand-in shows it
        Blob* shell = standin_birth(flags.genome, flags.seed);
        Task task;
        task_reset(task, flags.task, flags.task_settings, flags.task_seed);
        Plug plug;
        if (plug_attach(plug, *shell, task_shape(task), flags.plug)) {
            plug_print(plug, *shell, stdout);
        }
        standin_free(shell);
    }

    return protocol_run(flags);
}
