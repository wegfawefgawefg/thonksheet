// blobrun: one blob, one task, one long life. Score per window to csv, late minus early,
// then the same task with learning off and with coin-flip motors. Without the core it runs
// the stand-in only: coin flips and a scripted perfect player.

#include "run/run_flags.hpp"
#include "world/plug.hpp"
#include "world/plug_print.hpp"
#include "world/standin.hpp"
#include "world/task.hpp"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <vector>

// who drives the motor lines
enum class Driver { blob, learn_off, coin, perfect, count };
static const char* const driver_names[(int)Driver::count] = {"blob", "learn_off", "coin",
                                                             "perfect"};

// stopgap until the genome has a learning switch: make every learning chance 2^-30
static void genome_learning_off(Genome& g) {
    g.predict_shift = 30;
    g.reward_shift = 30;
    g.grow_shift = 30;
    g.prune_shift = 30;
    g.far_redraw_shift = 30;
    g.surprise_boost = 0;
}

static bool real_blob(Driver driver) {
    return driver == Driver::blob || driver == Driver::learn_off;
}

static Blob* birth(Driver driver, const Genome& genome, uint64_t seed) {
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        return blob_birth(genome, seed);
    }
#endif
    (void)driver;
    return standin_birth(genome, seed);
}

static void death(Driver driver, Blob* blob) {
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        blob_free(blob);
        return;
    }
#endif
    (void)driver;
    standin_free(blob);
}

static void tick(Driver driver, Blob& blob, const Plug& plug, int perfect_move) {
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        blob_tick(blob);
        return;
    }
#endif
    StandinPlay play = driver == Driver::perfect ? StandinPlay::perfect : StandinPlay::coin;
    standin_tick(blob, plug, play, perfect_move);
}

static void reward(Driver driver, Blob& blob, int valence) {
#ifdef BLOB_HAS_CORE
    if (real_blob(driver)) {
        blob_reward(blob, valence);
    }
#endif
    (void)driver;
    (void)blob;
    (void)valence;
}

// one whole life: the blob is born once and never reset; the task runs `steps` steps
static bool run_life(Driver driver, const RunFlags& flags, std::vector<double>& scores) {
    Genome genome = flags.genome;
    if (driver == Driver::learn_off) {
        genome_learning_off(genome);
    }
    Blob* blob = birth(driver, genome, flags.seed);
    Task task;
    task_reset(task, flags.task, flags.task_settings, flags.task_seed);
    Plug plug;
    if (!plug_attach(plug, *blob, task_shape(task), flags.plug)) {
        death(driver, blob);
        return false;
    }
    TaskSense sense;
    int ticks = flags.plug.ticks_per_step;

    for (int64_t step = 0; step < flags.steps; step++) {
        task_sense(task, sense);
        plug_set_lines(plug, sense);
        int perfect_move = task_perfect_move(task);

        // the blob ticks several times per step; motor spikes are counted over all of them
        TaskStepIn in;
        in.sensed_neurons = plug.sensed_neurons;
        for (int t = 0; t < ticks; t++) {
            plug_write(plug, *blob);
            tick(driver, *blob, plug, perfect_move);
            if (t == 0) {
                in.surprise = (int)blob->stats.surprise;
            }
            plug_count(plug, *blob);
        }
        in.move = plug_choice(plug);

        int valence = task_step(task, in);
        if (valence != 0) {
            reward(driver, *blob, valence);
            plug_reward(plug, valence);
            // optional quiet ticks so good/bad lands on this step's activity, not the next
            plug_quiet_lines(plug);
            for (int t = 0; t < flags.plug.rest_ticks; t++) {
                plug_write(plug, *blob);
                tick(driver, *blob, plug, -1);
            }
        }
        if ((step + 1) % flags.window == 0) {
            scores.push_back(task_window_score(task));
        }
    }
    death(driver, blob);
    return true;
}

static double mean(const std::vector<double>& v, size_t from, size_t to) {
    double sum = 0.0;
    for (size_t i = from; i < to; i++) {
        sum += v[i];
    }
    return to > from ? sum / (double)(to - from) : 0.0;
}

static bool write_csv(const std::string& path, const RunFlags& flags, const bool* ran,
                      const std::vector<double>* scores, size_t windows) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) {
        std::fprintf(stderr, "blobrun: cannot write %s\n", path.c_str());
        return false;
    }
    std::fprintf(f, "window,end_step");
    for (int d = 0; d < (int)Driver::count; d++) {
        if (ran[d]) {
            std::fprintf(f, ",%s", driver_names[d]);
        }
    }
    std::fprintf(f, "\n");
    for (size_t w = 0; w < windows; w++) {
        std::fprintf(f, "%zu,%lld", w, (long long)((w + 1) * flags.window));
        for (int d = 0; d < (int)Driver::count; d++) {
            if (ran[d]) {
                std::fprintf(f, ",%.6f", scores[d][w]);
            }
        }
        std::fprintf(f, "\n");
    }
    std::fclose(f);
    return true;
}

int main(int argc, char** argv) {
    RunFlags flags;
    if (!run_flags_parse(argc, argv, flags)) {
        return 1;
    }
    if (flags.out.empty()) {
        flags.out = std::string("out/") + task_name(flags.task) + "-s" +
                    std::to_string(flags.seed) + ".csv";
    }

    // which lives to run: the blob and its learning-off twin need the core
    bool want[(int)Driver::count] = {};
#ifdef BLOB_HAS_CORE
    want[(int)Driver::blob] = true;
    want[(int)Driver::learn_off] = flags.baselines;
#else
    std::printf("no core linked: running the stand-in only (coin flips, perfect player)\n");
#endif
    want[(int)Driver::coin] = flags.baselines || !want[(int)Driver::blob];
    want[(int)Driver::perfect] = true;

    std::printf("task %s  seed %llu  task seed %llu  steps %lld  ticks/step %d  window %d\n",
                task_name(flags.task), (unsigned long long)flags.seed,
                (unsigned long long)flags.task_seed, (long long)flags.steps,
                flags.plug.ticks_per_step, flags.window);
    if (flags.show_plug) {
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

    std::vector<double> scores[(int)Driver::count];
    bool ran[(int)Driver::count] = {};
    for (int d = 0; d < (int)Driver::count; d++) {
        if (!want[d]) {
            continue;
        }
        auto start = std::chrono::steady_clock::now();
        if (!run_life((Driver)d, flags, scores[d])) {
            return 1;
        }
        double secs =
            std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        ran[d] = true;
        if (real_blob((Driver)d)) {
            double ticks = (double)flags.steps * flags.plug.ticks_per_step;
            std::printf("%-9s life took %.1f s, %.0f ticks/s\n", driver_names[d], secs,
                        ticks / secs);
        }
    }

    size_t windows = (size_t)(flags.steps / flags.window);
    if (!write_csv(flags.out, flags, ran, scores, windows)) {
        return 1;
    }

    // early = first `edge` windows, late = last `edge` windows
    size_t edge = flags.edge > 0 ? (size_t)flags.edge : windows / 10;
    edge = edge < 1 ? 1 : edge;
    edge = edge > windows ? windows : edge;
    std::printf("%zu windows of %d steps, early/late = %zu windows each\n", windows, flags.window,
                edge);
    std::printf("%-10s %10s %10s %12s\n", "", "early", "late", "late-early");
    for (int d = 0; d < (int)Driver::count; d++) {
        if (!ran[d]) {
            continue;
        }
        double early = mean(scores[d], 0, edge);
        double late = mean(scores[d], windows - edge, windows);
        std::printf("%-10s %10.4f %10.4f %+12.4f\n", driver_names[d], early, late, late - early);
    }
    std::printf("csv: %s\n", flags.out.c_str());
    return 0;
}
