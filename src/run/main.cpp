// blobrun: one blob, one task, one long life. Score per window to csv, late minus early,
// then the same task with learning off and with coin-flip motors. Without the core it runs
// the stand-in only: coin flips and a scripted perfect player. The life protocols of the
// suite (retention, idle, capacity, reach, speed) are in life_protocols.cpp.

#include "run/drive.hpp"
#include "run/life_protocols.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"
#include "table/table.hpp"
#include "world/plug.hpp"
#include "world/plug_print.hpp"
#include "world/standin.hpp"
#include "world/task.hpp"

#include <cstdio>
#include <string>
#ifdef BLOB_HAS_MINIMAL
#include "minimal/minimal.hpp"
#endif
#ifdef BLOB_HAS_MEMORY
#include "memory/memory.hpp"
#endif
#ifdef BLOB_HAS_NN
#include "nn/nn.hpp"
#endif

// who drives the motor lines (drive.hpp)
// table and table_off are idea 26 (src/table/), picked with --table
const char* const driver_names[(int)Driver::count] = {"blob",    "learn_off", "coin",
                                                      "perfect", "table",     "table_off"};

// the table sheet's own numbers (--t.name value)
static TableGenome table_genome;

bool table_driver(Driver driver) {
    return driver == Driver::table || driver == Driver::table_off;
}

// the learning-off twin: same genome and seed, nothing changes its wiring
void genome_learning_off(Genome& g) {
    g.learning_on = 0;
}

bool real_blob(Driver driver) {
    return driver == Driver::blob || driver == Driver::learn_off;
}

// --engine minimal: src/minimal/ stands behind the same Blob shell
#ifdef BLOB_HAS_MINIMAL
static bool use_minimal = false;
static MinimalGenome minimal_genome;
#else
static const bool use_minimal = false;
#endif
// --engine memory: idea 123, src/memory/, a content-addressed store instead of a sheet
#ifdef BLOB_HAS_MEMORY
static bool use_memory = false;
static MemoryGenome memory_genome;
#else
static const bool use_memory = false;
#endif

// --engine mlp or gru: the fixed reference network (src/nn/) behind the same shell
#ifdef BLOB_HAS_NN
static bool use_nn = false;
static NnKind nn_kind = NnKind::mlp;
static NnSettings nn_settings;
#else
static const bool use_nn = false;
#endif

Blob* birth(Driver driver, const Genome& genome, uint64_t seed) {
    if (table_driver(driver)) {
        TableGenome tg = table_genome;
        tg.learning_on = driver == Driver::table ? tg.learning_on : 0;
        return table_birth(genome, tg, seed);
    }
#ifdef BLOB_HAS_MINIMAL
    if (use_minimal && real_blob(driver)) {
        return minimal_birth(genome, minimal_genome, seed);
    }
#endif
#ifdef BLOB_HAS_MEMORY
    if (use_memory && real_blob(driver)) {
        return memory_birth(genome, memory_genome, seed);
    }
#endif
#ifdef BLOB_HAS_NN
    if (use_nn && real_blob(driver)) {
        return nn_birth(genome, nn_kind, nn_settings, seed);
    }
#endif
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        return blob_birth(genome, seed);
    }
#endif
    (void)driver;
    return standin_birth(genome, seed);
}

void death(Driver driver, Blob* blob) {
    if (table_driver(driver)) {
        table_free(blob);
        return;
    }
#ifdef BLOB_HAS_MINIMAL
    if (use_minimal && real_blob(driver)) {
        minimal_free(blob);
        return;
    }
#endif
#ifdef BLOB_HAS_MEMORY
    if (use_memory && real_blob(driver)) {
        memory_free(blob);
        return;
    }
#endif
#ifdef BLOB_HAS_NN
    if (use_nn && real_blob(driver)) {
        nn_free(blob);
        return;
    }
#endif
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
    if (table_driver(driver)) {
        table_tick(blob);
        return;
    }
#ifdef BLOB_HAS_MINIMAL
    if (use_minimal && real_blob(driver)) {
        minimal_tick(blob);
        return;
    }
#endif
#ifdef BLOB_HAS_MEMORY
    if (use_memory && real_blob(driver)) {
        memory_tick(blob);
        return;
    }
#endif
#ifdef BLOB_HAS_NN
    if (use_nn && real_blob(driver)) {
        nn_tick(blob, plug, perfect_move);
        return;
    }
#endif
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
    if (table_driver(driver)) {
        table_reward(blob, valence);
        return;
    }
#ifdef BLOB_HAS_MINIMAL
    if (use_minimal && real_blob(driver)) {
        minimal_reward(blob, valence);
        return;
    }
#endif
#ifdef BLOB_HAS_MEMORY
    if (use_memory && real_blob(driver)) {
        memory_reward(blob, valence);
        return;
    }
#endif
#ifdef BLOB_HAS_NN
    if (use_nn && real_blob(driver)) {
        nn_reward(blob, valence);
        return;
    }
#endif
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        blob_reward(blob, valence);
    }
#endif
    (void)driver;
    (void)blob;
    (void)valence;
}

// which lives to run: the blob and its learning-off twin need the core (or another engine)
void drivers_wanted(const RunFlags& flags, bool* want) {
    for (int d = 0; d < (int)Driver::count; d++) {
        want[d] = false;
    }
#ifdef BLOB_HAS_CORE
    want[(int)Driver::blob] = true;
    want[(int)Driver::learn_off] = flags.baselines;
#endif
    if (use_minimal || use_memory || use_nn) {
        want[(int)Driver::blob] = true;
        want[(int)Driver::learn_off] = flags.baselines;
    }
    want[(int)Driver::coin] = flags.baselines || !want[(int)Driver::blob];
    want[(int)Driver::perfect] = true;
    if (flags.table) {
        // the table sheet stands in for the blob, with its own learning-off twin
        want[(int)Driver::blob] = false;
        want[(int)Driver::learn_off] = false;
        want[(int)Driver::table] = true;
        want[(int)Driver::table_off] = flags.baselines;
        want[(int)Driver::coin] = flags.baselines;
    }
}

int main(int argc, char** argv) {
    RunFlags flags;
    apply_genome_args(argc, argv, flags.genome);
    table_genome_args(argc, argv, table_genome);
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

#ifdef BLOB_HAS_MINIMAL
    use_minimal = flags.engine == "minimal";
    apply_minimal_args(argc, argv, minimal_genome);
#endif
#ifdef BLOB_HAS_MEMORY
    use_memory = flags.engine == "memory";
    apply_memory_args(argc, argv, memory_genome);
#endif
#ifdef BLOB_HAS_NN
    use_nn = flags.engine == "mlp" || flags.engine == "gru";
    nn_kind = flags.engine == "gru" ? NnKind::gru : NnKind::mlp;
    apply_nn_args(argc, argv, nn_settings);
#endif
    if (flags.engine != "blob" && !use_minimal && !use_memory && !use_nn) {
        std::fprintf(stderr, "blobrun: no engine '%s' in this build\n", flags.engine.c_str());
        return 1;
    }
    // the readout mode is the blob genome's learn_mode 1; other engines do not read it
    if (flags.frozen_readout && (flags.table || use_minimal || use_memory || use_nn)) {
        std::fprintf(stderr, "blobrun: --frozen-readout is the blob's learn_mode 1; this "
                             "engine has no readout mode\n");
        return 1;
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
