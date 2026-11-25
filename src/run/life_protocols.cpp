#include "run/life_protocols.hpp"

#include "run/life.hpp"

#include <cstring>
#include <filesystem>

void genome_readout_only(Genome& genome) {
    // through the genome's own names, so this builds whether or not the core has the field
    char program[] = "blobrun", name[] = "--g.learn_mode", value[] = "1";
    char* argv[] = {program, name, value};
    apply_genome_args(3, argv, genome);
}

static std::string stem_of(const std::string& csv_path) {
    size_t dot = csv_path.rfind(".csv");
    return dot != std::string::npos && dot + 4 == csv_path.size() ? csv_path.substr(0, dot)
                                                                  : csv_path;
}

static void make_parent(const std::string& path) {
    std::filesystem::path p(path);
    if (p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
}

FILE* summary_open(const std::string& csv_path) {
    std::string path = stem_of(csv_path) + "-sum.csv";
    make_parent(path);
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) {
        std::fprintf(stderr, "blobrun: cannot write %s\n", path.c_str());
        return nullptr;
    }
    std::fprintf(f, "driver,key,value\n");
    return f;
}

static double mean(const std::vector<double>& v, size_t from, size_t to) {
    double sum = 0.0;
    for (size_t i = from; i < to; i++) {
        sum += v[i];
    }
    return to > from ? sum / (double)(to - from) : 0.0;
}

// windows averaged for "early" and "late": --edge, or a tenth of them, at least one
static size_t edge_of(const RunFlags& flags, size_t windows) {
    size_t edge = flags.edge > 0 ? (size_t)flags.edge : windows / 10;
    edge = edge < 1 ? 1 : edge;
    return edge > windows ? windows : edge;
}

// one column per driver; a phase column only for lives of more than one phase
static bool write_csv(const std::string& path, const RunFlags& flags, const bool* ran,
                      const LifeResult* results, int phases) {
    make_parent(path);
    FILE* f = std::fopen(path.c_str(), "w");
    if (!f) {
        std::fprintf(stderr, "blobrun: cannot write %s\n", path.c_str());
        return false;
    }
    const LifeResult* shape = nullptr;
    std::fprintf(f, phases > 1 ? "window,end_step,phase" : "window,end_step");
    for (int d = 0; d < (int)Driver::count; d++) {
        if (ran[d]) {
            std::fprintf(f, ",%s", driver_names[d]);
            shape = &results[d];
        }
    }
    std::fprintf(f, "\n");
    size_t windows = shape ? shape->scores.size() : 0;
    int phase = 0;
    for (size_t w = 0; w < windows; w++) {
        std::fprintf(f, "%zu,%lld", w, (long long)((w + 1) * flags.window));
        if (phases > 1) {
            while (phase + 1 < phases && (int)w >= shape->first_window[phase + 1]) {
                phase++;
            }
            std::fprintf(f, ",%d", phase);
        }
        for (int d = 0; d < (int)Driver::count; d++) {
            if (ran[d]) {
                std::fprintf(f, ",%.6f", results[d].scores[w]);
            }
        }
        std::fprintf(f, "\n");
    }
    std::fclose(f);
    return true;
}

static LifeResult results[(int)Driver::count];

static bool run_lives(const RunFlags& flags, const LifePhase* phases, int count, bool* ran) {
    bool want[(int)Driver::count];
    drivers_wanted(flags, want);
    for (int d = 0; d < (int)Driver::count; d++) {
        ran[d] = false;
        if (!want[d]) {
            continue;
        }
        if (!life_run((Driver)d, flags, phases, count, results[d])) {
            return false;
        }
        ran[d] = true;
        if (engine_driver((Driver)d)) {
            const LifeResult& r = results[d];
            std::printf("%-9s life took %.1f s, %.0f ticks/s\n", driver_names[d], r.seconds,
                        (double)r.ticks / r.seconds);
        }
    }
    return true;
}

static void put(FILE* sum, int driver, const char* key, double value) {
    std::fprintf(sum, "%s,%s,%.6g\n", driver_names[driver], key, value);
}

// --task for the whole life. writes `csv`, and early/late under `prefix` in the summary.
static bool life_plain(const RunFlags& flags, const std::string& csv, FILE* sum, const char* prefix,
                       bool end_numbers, double* late_out) {
    LifePhase phase = life_phase_main(flags);
    bool ran[(int)Driver::count];
    if (!run_lives(flags, &phase, 1, ran) || !write_csv(csv, flags, ran, results, 1)) {
        return false;
    }
    size_t windows = (size_t)(flags.steps / flags.window);
    size_t edge = edge_of(flags, windows);
    std::printf("%zu windows of %d steps, early/late = %zu windows each\n", windows, flags.window,
                edge);
    std::printf("%-10s %10s %10s %12s\n", "", "early", "late", "late-early");
    std::string key(prefix);
    for (int d = 0; d < (int)Driver::count; d++) {
        if (!ran[d]) {
            continue;
        }
        const std::vector<double>& s = results[d].scores;
        double early = mean(s, 0, edge);
        double late = mean(s, windows - edge, windows);
        std::printf("%-10s %10.4f %10.4f %+12.4f\n", driver_names[d], early, late, late - early);
        put(sum, d, (key + "early").c_str(), early);
        put(sum, d, (key + "late").c_str(), late);
        put(sum, d, (key + "late_minus_early").c_str(), late - early);
        if (late_out) {
            late_out[d] = late;
        }
    }
    for (int d = 0; d < (int)Driver::count; d++) {
        if (ran[d] && results[d].end.measured) {
            life_end_print(results[d].end, driver_names[d], stdout);
            if (end_numbers) {
                put(sum, d, "ticks_per_s", (double)results[d].ticks / results[d].seconds);
                life_end_write(results[d].end, driver_names[d], sum);
            }
        }
    }
    return true;
}

// retention: A, B, A again. idle: A, unplugged, A again.
static bool life_phases(const RunFlags& flags, FILE* sum) {
    LifePhase phases[3];
    phases[0] = life_phase_main(flags);
    phases[2] = phases[0];
    phases[2].steps = flags.retest;
    const char* names[4];
    if (flags.protocol == Protocol::retention) {
        // B's cues and buttons come after A's, so one learner can hold both
        phases[1] = phases[0];
        phases[1].task = flags.task_b;
        phases[1].settings.cue_offset = flags.task_settings.cue_offset + flags.task_settings.k;
        phases[1].task_seed = flags.task_seed + 1;
        std::printf("retention: %s for %lld steps, %s (cue offset %d) for %lld, %s again for "
                    "%lld\n",
                    task_name(flags.task), (long long)flags.steps, task_name(flags.task_b),
                    phases[1].settings.cue_offset, (long long)flags.steps, task_name(flags.task),
                    (long long)flags.retest);
        const char* r[4] = {"a_end", "a_retest_start", "a_retest_end", "b_end"};
        std::memcpy(names, r, sizeof(names));
    } else {
        phases[1] = LifePhase{};
        phases[1].idle_ticks = flags.idle_ticks;
        std::printf("idle: %s for %lld steps, %lld ticks unplugged, %s again for %lld steps\n",
                    task_name(flags.task), (long long)flags.steps, (long long)flags.idle_ticks,
                    task_name(flags.task), (long long)flags.retest);
        const char* r[4] = {"before", "after_start", "after_end", nullptr};
        std::memcpy(names, r, sizeof(names));
    }
    bool ran[(int)Driver::count];
    if (!run_lives(flags, phases, 3, ran) || !write_csv(flags.out, flags, ran, results, 3)) {
        return false;
    }

    std::printf("windows of %d steps; end = mean of the phase's last tenth, start = its first "
                "window\n",
                flags.window);
    std::printf("%-10s %12s %12s %12s %12s\n", "", names[0], names[1], names[2],
                names[3] ? names[3] : "");
    for (int d = 0; d < (int)Driver::count; d++) {
        if (!ran[d]) {
            continue;
        }
        const LifeResult& r = results[d];
        const std::vector<double>& s = r.scores;
        size_t a0 = (size_t)r.first_window[0], a1 = (size_t)r.first_window[1];
        size_t b1 = (size_t)r.first_window[2];
        size_t c0 = (size_t)r.first_window[2], c1 = (size_t)r.first_window[3];
        double v[4] = {mean(s, a1 - edge_of(flags, a1 - a0), a1), c1 > c0 ? s[c0] : 0.0,
                       mean(s, c1 - edge_of(flags, c1 - c0), c1),
                       mean(s, b1 - edge_of(flags, b1 - a1), b1)};
        std::printf("%-10s %12.4f %12.4f %12.4f", driver_names[d], v[0], v[1], v[2]);
        if (names[3]) {
            std::printf(" %12.4f", v[3]);
        }
        std::printf("\n");
        for (int k = 0; k < 4; k++) {
            if (names[k]) {
                put(sum, d, names[k], v[k]);
            }
        }
    }
    for (int d = 0; d < (int)Driver::count; d++) {
        if (ran[d] && results[d].end.measured) {
            life_end_print(results[d].end, driver_names[d], stdout);
        }
    }
    return true;
}

// shown: the task with the answer shown and no good or bad, then a test with neither.
// damage: the task, then part of the sheet silenced and the task again.
static bool life_shown_or_damage(const RunFlags& flags, FILE* sum) {
    LifePhase phases[2];
    phases[0] = life_phase_main(flags);
    phases[1] = phases[0];
    phases[1].steps = flags.retest;
    bool shown = flags.protocol == Protocol::shown;
    if (shown) {
        phases[0].shown_ticks = flags.shown_ticks;
        phases[0].withhold = true;
        phases[1].withhold = true;
        std::printf("shown: %s for %lld steps with the right button's cells made to fire for "
                    "the last %d ticks of each answer step, no good or bad; then %lld steps "
                    "with no help and no good or bad\n",
                    task_name(flags.task), (long long)flags.steps, flags.shown_ticks,
                    (long long)flags.retest);
    } else {
        phases[1].damage_pct = flags.damage_pct;
        phases[1].damage_tile = flags.damage_tile;
        std::printf("damage: %s for %lld steps, then %s silenced, then %lld steps more\n",
                    task_name(flags.task), (long long)flags.steps,
                    flags.damage_tile ? "the cue tile's inner cells" : "part of the sheet",
                    (long long)flags.retest);
    }
    bool ran[(int)Driver::count];
    if (!run_lives(flags, phases, 2, ran) || !write_csv(flags.out, flags, ran, results, 2)) {
        return false;
    }
    const char* r_shown[4] = {"teach_end", "test_start", "test_mean", "test_end"};
    const char* r_damage[4] = {"before", "after_start", "after_end", nullptr};
    const char* const* names = shown ? r_shown : r_damage;
    std::printf("windows of %d steps; end = mean of the phase's last tenth, start = its first "
                "window, mean = all its windows\n",
                flags.window);
    std::printf("%-10s %12s %12s %12s %12s\n", "", names[0], names[1], names[2],
                names[3] ? names[3] : "");
    for (int d = 0; d < (int)Driver::count; d++) {
        if (!ran[d]) {
            continue;
        }
        const LifeResult& r = results[d];
        const std::vector<double>& s = r.scores;
        size_t a0 = (size_t)r.first_window[0], a1 = (size_t)r.first_window[1];
        size_t b1 = (size_t)r.first_window[2];
        double a_end = mean(s, a1 - edge_of(flags, a1 - a0), a1);
        double b_start = b1 > a1 ? s[a1] : 0.0;
        double b_end = mean(s, b1 - edge_of(flags, b1 - a1), b1);
        double v[4] = {a_end, b_start, shown ? mean(s, a1, b1) : b_end, b_end};
        std::printf("%-10s %12.4f %12.4f %12.4f", driver_names[d], v[0], v[1], v[2]);
        if (names[3]) {
            std::printf(" %12.4f", v[3]);
        }
        std::printf("\n");
        for (int k = 0; k < 4; k++) {
            if (names[k]) {
                put(sum, d, names[k], v[k]);
            }
        }
    }
    return true;
}

// echo with 2, 4, 8, 16 cues, a life each. a k counts as held when its late score is at least
// halfway from chance (1/k) to perfect. the k=2 life's room and aliveness go in the summary.
static bool life_capacity(const RunFlags& flags, FILE* sum) {
    const int ks[4] = {2, 4, 8, 16};
    double late[4][(int)Driver::count] = {};
    bool echo_family = flags.task == TaskKind::echo || flags.task == TaskKind::shuffled_echo ||
                       flags.task == TaskKind::delayed_echo;
    std::string stem = stem_of(flags.out);
    for (int i = 0; i < 4; i++) {
        RunFlags f = flags;
        f.task = echo_family ? flags.task : TaskKind::echo;
        f.task_settings.k = ks[i];
        std::printf("== %s k=%d\n", task_name(f.task), ks[i]);
        char prefix[16];
        std::snprintf(prefix, sizeof(prefix), "k%d_", ks[i]);
        std::string csv = stem + "-k" + std::to_string(ks[i]) + ".csv";
        if (!life_plain(f, csv, sum, prefix, i == 0, late[i])) {
            return false;
        }
    }
    bool want[(int)Driver::count];
    drivers_wanted(flags, want);
    std::printf("capacity: largest k whose late score is at least (1/k + 1) / 2\n");
    for (int d = 0; d < (int)Driver::count; d++) {
        if (!want[d]) {
            continue;
        }
        int held = 0;
        for (int i = 0; i < 4; i++) {
            double line = (1.0 / ks[i] + 1.0) / 2.0;
            held = late[i][d] >= line ? ks[i] : held;
        }
        std::printf("%-10s %d\n", driver_names[d], held);
        put(sum, d, "capacity", held);
    }
    return true;
}

int protocol_run(const RunFlags& flags) {
    if (flags.protocol == Protocol::reach) {
        return protocol_reach(flags);
    }
    if (flags.protocol == Protocol::speed) {
        return protocol_speed(flags);
    }
    FILE* sum = summary_open(flags.out);
    if (!sum) {
        return 1;
    }
    bool ok = false;
    switch (flags.protocol) {
    case Protocol::life:
        ok = life_plain(flags, flags.out, sum, "", true, nullptr);
        break;
    case Protocol::retention:
    case Protocol::idle:
        ok = life_phases(flags, sum);
        break;
    case Protocol::capacity:
        ok = life_capacity(flags, sum);
        break;
    case Protocol::shown:
    case Protocol::damage:
        ok = life_shown_or_damage(flags, sum);
        break;
    case Protocol::reach:
    case Protocol::speed:
        break;
    }
    std::fclose(sum);
    if (ok && flags.protocol == Protocol::capacity) {
        std::printf("csv: %s-k<k>.csv\n", stem_of(flags.out).c_str());
    } else if (ok) {
        std::printf("csv: %s\n", flags.out.c_str());
    }
    return ok ? 0 : 1;
}
