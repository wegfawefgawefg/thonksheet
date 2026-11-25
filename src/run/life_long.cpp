// blobrun --long: one life per driver on a long-memory world (task_long.hpp), scored per
// bucket (an age, teaching, common or rare, kept or changed). The csv has a row per window
// per driver; the summary has each bucket over the whole life, before and after the split
// (drift_at for drift, half the life otherwise), and for pairs and once an estimate of how
// many pairs are held at once.

#include "run/drive.hpp"
#include "run/life_protocols.hpp"
#include "run/task_ticker.hpp"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

static TaskTicker ticker; // large; one per process

struct LongTotals {
    int64_t right[LONG_BUCKETS] = {}, asked[LONG_BUCKETS] = {};
};

static double rate(int64_t right, int64_t asked) {
    return asked > 0 ? (double)right / (double)asked : -1.0;
}

// one driver's life. writes its window rows to `csv`; fills the totals before and after the
// split step
static bool long_life(Driver d, const RunFlags& flags, FILE* csv, LongTotals& before,
                      LongTotals& after, double& seconds, int64_t& ticks) {
    Genome genome = flags.genome;
    if (d == Driver::learn_off) {
        genome_learning_off(genome);
    }
    Blob* blob = birth(d, genome, flags.seed);
    TaskTicker& tt = ticker;
    tt = TaskTicker{};
    if (!ticker_start(tt, *blob, flags)) {
        death(d, blob);
        return false;
    }
    const LongSettings& ls = tt.task.settings.longs;
    int64_t split = flags.task == TaskKind::drift ? ls.drift_at : flags.steps / 2;
    LongTotals at_split;
    bool split_taken = false;
    auto start = std::chrono::steady_clock::now();
    int64_t step = 0, tenth = flags.steps / 10 > 0 ? flags.steps / 10 : 1;
    ticks = 0;
    while (step < flags.steps) {
        int perfect_move = ticker_before_tick(tt, *blob);
        tick(d, *blob, tt.plug, perfect_move);
        ticks++;
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        if (tt.valence != 0) {
            plug_mark_acted(tt.plug, *blob, tt.move);
            reward(d, *blob, tt.valence);
        }
        step++;
        LongState& st = tt.task.longs;
        if (step == split) {
            for (int j = 0; j < LONG_BUCKETS; j++) {
                at_split.right[j] = st.right[j];
                at_split.asked[j] = st.asked[j];
            }
            split_taken = true;
        }
        if (step % flags.window == 0) {
            std::fprintf(csv, "%lld,%lld,%s,%.4f", (long long)(step / flags.window - 1),
                         (long long)step, driver_names[(int)d], task_window_score(tt.task));
            for (int j = 0; j < LONG_BUCKETS; j++) {
                if (long_bucket_name(tt.task, j)[0] == '\0') {
                    continue;
                }
                double r = rate(st.wright[j], st.wasked[j]);
                r < 0 ? (void)std::fprintf(csv, ",") : (void)std::fprintf(csv, ",%.4f", r);
            }
            std::fprintf(csv, "\n");
            long_window_clear(tt.task);
        }
        if (engine_driver(d) && flags.steps >= 1000000 && step % tenth == 0) {
            double s =
                std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            std::printf("  %s %lld%% %.0f s\n", driver_names[(int)d],
                        (long long)(step * 100 / flags.steps), s);
            std::fflush(stdout);
        }
    }
    seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    const LongState& st = tt.task.longs;
    for (int j = 0; j < LONG_BUCKETS; j++) {
        before.right[j] = split_taken ? at_split.right[j] : st.right[j];
        before.asked[j] = split_taken ? at_split.asked[j] : st.asked[j];
        after.right[j] = st.right[j] - before.right[j];
        after.asked[j] = st.asked[j] - before.asked[j];
    }
    // the buttons' motor cells left with no connection at all (engines with a fan-in count)
    if (real_blob(d) && blob->fan_in && blob->motor_attached) {
        int dark = 0, cells = 0;
        for (int n = 0; n < blob->neuron_count; n++) {
            if ((blob->motor_attached[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1) {
                cells++;
                dark += blob->fan_in[n] == 0;
            }
        }
        std::printf("%-10s button cells with no connection at the end: %d of %d\n",
                    driver_names[(int)d], dark, cells);
    }
    // evidence counters (readout_evidence) at either end of their range: a full readout
    if (real_blob(d) && blob->readout_evidence && blob->readout_row) {
        int bits = blob->genome.readout_evidence_bits;
        int top = (1 << ((bits < 2 ? 2 : (bits > 8 ? 8 : bits)) - 1)) - 1;
        int64_t rows = 0, at_end = 0, nonzero = 0;
        for (int n = 0; n < blob->neuron_count; n++) {
            rows += blob->readout_row[n] >= 0;
        }
        int64_t all = rows * SLOTS * TILE_NEURONS;
        for (int64_t i = 0; i < all; i++) {
            int v = blob->readout_evidence[i];
            at_end += v >= top || v <= -top;
            nonzero += v != 0;
        }
        std::printf("%-10s evidence counters at an end of their range: %.3f, not zero: %.3f\n",
                    driver_names[(int)d], (double)at_end / (double)(all > 0 ? all : 1),
                    (double)nonzero / (double)(all > 0 ? all : 1));
    }
    death(d, blob);
    return true;
}

// pairs held at once: the forgetting curve above chance, integrated over age, in pairs
static double pairs_held(const Task& task, const LongTotals& all) {
    const LongSettings& ls = task.settings.longs;
    double chance = 1.0 / ls.buttons;
    double held = 0.0, last_age = 0.0, last_r = -1.0;
    for (int j = 0; j < LONG_AGES; j++) {
        if (all.asked[j] == 0) {
            break;
        }
        double n = (double)all.asked[j];
        double above = (double)all.right[j] / n - chance;
        // an age counts only when it is more than two standard errors above chance, so a
        // wide age band does not turn noise into hundreds of pairs
        double noise = 2.0 * std::sqrt(chance * (1.0 - chance) / n);
        double r = above > noise ? above / (1.0 - chance) : 0.0;
        double age = (double)long_age(ls, j);
        // flat before the first age, straight lines between the ones measured
        held += (age - last_age) * (last_r < 0 ? r : (r + last_r) / 2.0);
        last_age = age;
        last_r = r;
    }
    return held / ls.pair_every;
}

int protocol_long(const RunFlags& flags) {
    bool want[(int)Driver::count];
    drivers_wanted(flags, want);
    // coin flips are cheap and are the floor of every table; --no-baselines only drops the
    // learning-off twin, which costs as much as the life itself
    want[(int)Driver::coin] = true;
    std::filesystem::path p(flags.out);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    FILE* csv = std::fopen(flags.out.c_str(), "w");
    FILE* sum = summary_open(flags.out);
    if (!csv || !sum) {
        std::fprintf(stderr, "blobrun: cannot write %s\n", flags.out.c_str());
        return 1;
    }
    Task shape_task;
    task_reset(shape_task, flags.task, flags.task_settings, flags.task_seed);
    std::fprintf(csv, "window,end_step,driver,score");
    for (int j = 0; j < LONG_BUCKETS; j++) {
        const char* name = long_bucket_name(shape_task, j);
        if (name[0]) {
            std::fprintf(csv, ",%s", name);
        }
    }
    std::fprintf(csv, "\n");
    bool pairs_like = flags.task == TaskKind::pairs || flags.task == TaskKind::once;
    if (pairs_like) {
        std::printf("ages:");
        for (int j = 0; j < LONG_AGES; j++) {
            std::printf(" age%d=%lld", j, (long long)long_age(shape_task.settings.longs, j));
        }
        std::printf(" steps\n");
    }
    std::printf("%-10s %-8s %8s %8s %8s %10s\n", "driver", "bucket", "all", "before", "after",
                "asked");
    for (int d = 0; d < (int)Driver::count; d++) {
        if (!want[d]) {
            continue;
        }
        LongTotals before, after;
        double seconds = 0;
        int64_t ticks = 0;
        if (!long_life((Driver)d, flags, csv, before, after, seconds, ticks)) {
            return 1;
        }
        std::fflush(csv);
        const char* dn = driver_names[d];
        if (engine_driver((Driver)d)) {
            std::printf("%-10s life took %.1f s, %.0f ticks/s\n", dn, seconds, ticks / seconds);
            std::fprintf(sum, "%s,ticks_per_s,%.0f\n", dn, ticks / seconds);
        }
        LongTotals all;
        for (int j = 0; j < LONG_BUCKETS; j++) {
            const char* name = long_bucket_name(shape_task, j);
            all.right[j] = before.right[j] + after.right[j];
            all.asked[j] = before.asked[j] + after.asked[j];
            if (!name[0]) {
                continue;
            }
            double a = rate(all.right[j], all.asked[j]);
            double b = rate(before.right[j], before.asked[j]);
            double c = rate(after.right[j], after.asked[j]);
            std::printf("%-10s %-8s %8.4f %8.4f %8.4f %10lld\n", dn, name, a, b, c,
                        (long long)all.asked[j]);
            std::fprintf(sum, "%s,%s_all,%.6g\n%s,%s_before,%.6g\n%s,%s_after,%.6g\n", dn, name, a,
                         dn, name, b, dn, name, c);
            std::fprintf(sum, "%s,%s_asked,%lld\n", dn, name, (long long)all.asked[j]);
        }
        if (pairs_like) {
            double held = pairs_held(shape_task, all);
            std::printf("%-10s pairs held at once (curve above chance, integrated): %.1f\n", dn,
                        held);
            std::fprintf(sum, "%s,held,%.6g\n", dn, held);
        }
    }
    std::fclose(csv);
    std::fclose(sum);
    std::printf("csv: %s\n", flags.out.c_str());
    return 0;
}
