// blobprobe invariance: level (a) of the invariance battery (docs/curriculum.md,
// "Invariance"). Runs a battery task (or `place`) through the plug on a sheet as born
// (learning off unless --learning 1, no reward), keeps every neuron's spike count over each
// answered step, then fits a plain linear reader on trials of the rewarded versions and
// scores it on later trials of those versions ("same") and on trials of the held-out
// versions ("held"). Readers: the task's own input lines (what a reader of the raw input
// gets: if this carries over, the task is too easy), the whole sheet, the motors' view (the
// 3x3 tiles around every motor tile), and every tile alone. Any engine: --engine minimal,
// --table. blobrun's task, plug, seed and --g./--m./--t. flags all work.
// Own flags: --trials N (answered trials kept, default 4000), --settle N (ticks before
// keeping any, default 30000), --learning 0|1, --label text, --sum PATH (key,value csv),
// --show 1 (print each shape in each version and one moving stretch, then stop), --sheet 0
// (fit the input lines only).

#include "blob/blob.hpp"
#include "probe/linear_split.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"
#include "table/table.hpp"
#ifdef BLOB_HAS_MINIMAL
#include "minimal/minimal.hpp"
#endif

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);
void inv_show(const RunFlags& flags);

static const char* const own_flags[] = {"--trials", "--settle", "--learning", "--label",
                                        "--sum",    "--show",   "--sheet"};

enum class Engine { blob, minimal, table };

static Blob* engine_birth(Engine e, int argc, char** argv, const RunFlags& f, bool learning) {
    if (e == Engine::table) {
        TableGenome tg;
        table_genome_args(argc, argv, tg);
        tg.learning_on = learning ? tg.learning_on : 0;
        return table_birth(f.genome, tg, f.seed);
    }
#ifdef BLOB_HAS_MINIMAL
    if (e == Engine::minimal) {
        MinimalGenome mg;
        apply_minimal_args(argc, argv, mg);
        mg.learning_on = learning ? mg.learning_on : 0;
        return minimal_birth(f.genome, mg, f.seed);
    }
#endif
    Genome g = f.genome;
    g.learning_on = learning ? g.learning_on : 0;
    return blob_birth(g, f.seed);
}

static void engine_tick(Engine e, Blob& b) {
#ifdef BLOB_HAS_MINIMAL
    if (e == Engine::minimal) {
        minimal_tick(b);
        return;
    }
#endif
    if (e == Engine::table) {
        table_tick(b);
    } else {
        blob_tick(b);
    }
}

static void engine_reward(Engine e, Blob& b, int valence) {
#ifdef BLOB_HAS_MINIMAL
    if (e == Engine::minimal) {
        minimal_reward(b, valence);
        return;
    }
#endif
    if (e == Engine::table) {
        table_reward(b, valence);
    } else {
        blob_reward(b, valence);
    }
}

static void engine_free(Engine e, Blob* b) {
#ifdef BLOB_HAS_MINIMAL
    if (e == Engine::minimal) {
        minimal_free(b);
        return;
    }
#endif
    if (e == Engine::table) {
        table_free(b);
    } else {
        blob_free(b);
    }
}

// the 3x3 tiles around every tile a motor cell sits in
static std::vector<int> motors_view(const Plug& plug, const Blob& blob) {
    std::vector<uint8_t> in((size_t)blob.tile_count, 0);
    int w = blob.tiles_w, h = blob.tiles_h;
    for (int line = 0; line < plug.motor_lines; line++) {
        const int* cells = nullptr;
        int count = plug_motor_cells(plug, line, &cells);
        for (int k = 0; k < count; k++) {
            int t = cells[k] / TILE_NEURONS;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int x = t % w + dx, y = t / w + dy;
                    if (x >= 0 && x < w && y >= 0 && y < h) {
                        in[(size_t)(y * w + x)] = 1;
                    }
                }
            }
        }
    }
    std::vector<int> out;
    for (int t = 0; t < blob.tile_count; t++) {
        for (int k = 0; in[(size_t)t] && k < TILE_NEURONS; k++) {
            out.push_back(t * TILE_NEURONS + k);
        }
    }
    return out;
}

static void put(FILE* sum, const char* key, double v) {
    if (sum) {
        std::fprintf(sum, "%s,%.6g\n", key, v);
    }
}

static void report(const char* name, const SplitScore& s, FILE* sum, const char* key) {
    std::printf("  %-26s same %.3f  held %.3f\n", name, s.same, s.held);
    std::string k(key);
    put(sum, (k + "_same").c_str(), s.same);
    put(sum, (k + "_held").c_str(), s.held);
}

int probe_invariance(int argc, char** argv) {
    std::vector<char*> rest = {argv[0]};
    for (int i = 2; i < argc; i++) {
        bool mine = false;
        for (const char* name : own_flags) {
            mine = mine || (std::strcmp(argv[i], name) == 0 && i + 1 < argc);
        }
        if (mine) {
            i++;
            continue;
        }
        rest.push_back(argv[i]);
    }
    RunFlags flags;
    apply_genome_args((int)rest.size(), rest.data(), flags.genome);
    if (!run_flags_parse((int)rest.size(), rest.data(), flags)) {
        return 1;
    }
    if (int_arg(argc, argv, "--show", 0)) {
        inv_show(flags);
        return 0;
    }
    int trials = int_arg(argc, argv, "--trials", 4000);
    int64_t settle = int_arg(argc, argv, "--settle", 30000);
    bool learning = int_arg(argc, argv, "--learning", 0) != 0;
    const char* label = text_arg(argc, argv, "--label", "run");
    const char* sum_path = text_arg(argc, argv, "--sum", "");
    Engine engine =
        flags.table ? Engine::table : (flags.engine == "minimal" ? Engine::minimal : Engine::blob);

    Blob* blob = engine_birth(engine, argc, argv, flags, learning);
    static TaskTicker tt;
    tt = TaskTicker{};
    if (!ticker_start(tt, *blob, flags)) {
        engine_free(engine, blob);
        return 1;
    }
    // input lines: the task's own lines (not good and bad), each tick of the trial, in bins
    // of 2 ticks, over the last 4 steps (a time-kind trial; a picture trial is 2 and the rest
    // is blank)
    int first = tt.plug.first_single, lines = tt.plug.sensor_lines - first;
    int span = 4 * tt.ticks_per_step, bin = 2;
    int bins = (span + bin - 1) / bin;
    std::vector<uint8_t> history((size_t)span * lines, 0); // ring, tick-major
    int64_t ticks = 0;

    SplitData neurons, input;
    neurons.features = blob->neuron_count;
    input.features = lines * bins;
    neurons.classes = input.classes = tt.plug.motor_lines;
    std::vector<uint8_t> count((size_t)blob->neuron_count, 0);
    int versions = 0, kept = 0;
    int version = 0;
    bool held = false;
    while (kept < trials) {
        if (tt.tick_in_step == 0 && tt.rest_left == 0) {
            std::memset(count.data(), 0, count.size());
        }
        bool resting = tt.rest_left > 0;
        ticker_before_tick(tt, *blob);
        uint8_t* now = &history[(size_t)(ticks % span) * lines];
        for (int l = 0; l < lines; l++) {
            now[l] = resting ? 0 : tt.plug.line_on[first + l];
        }
        engine_tick(engine, *blob);
        ticks++;
        for (int t = 0; !resting && t < blob->tile_count; t++) {
            uint64_t spikes = blob->spikes[blob->now][t] & ~blob->sensor[t];
            while (spikes) {
                uint8_t& c = count[(size_t)(t * TILE_NEURONS + __builtin_ctzll(spikes))];
                c = c < 255 ? c + 1 : c;
                spikes &= spikes - 1;
            }
        }
        // the version is the trial's until its answer is in
        bool is_trial = task_version(tt.task, version, held);
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        if (learning && tt.valence != 0) {
            plug_mark_acted(tt.plug, *blob, tt.move);
            engine_reward(engine, *blob, tt.valence);
        }
        if (ticks < settle || !is_trial || tt.right_move < 0) {
            continue;
        }
        versions = version + 1 > versions ? version + 1 : versions;
        for (SplitData* d : {&neurons, &input}) {
            d->label.push_back(tt.right_move);
            d->version.push_back(version);
            d->held.push_back(held);
        }
        neurons.x.insert(neurons.x.end(), count.begin(), count.end());
        for (int l = 0; l < lines; l++) {
            for (int b = 0; b < bins; b++) {
                int on = 0;
                for (int k = 0; k < bin && b * bin + k < span; k++) {
                    int64_t at = ticks - span + b * bin + k;
                    on += at >= 0 ? history[(size_t)(at % span) * lines + l] : 0;
                }
                input.x.push_back((uint8_t)on);
            }
        }
        kept++;
    }

    FILE* sum = sum_path[0] ? std::fopen(sum_path, "w") : nullptr;
    if (sum) {
        std::fprintf(sum, "key,value\n");
    }
    int held_trials = 0;
    for (uint8_t h : neurons.held) {
        held_trials += h;
    }
    std::printf("%s: %d trials (%d on held-out versions), %d classes, chance %.3f\n", label, kept,
                held_trials, neurons.classes, 1.0 / neurons.classes);
    put(sum, "trials", kept);
    put(sum, "held_trials", held_trials);

    SplitPrep prep;
    split_prepare(input, prep);
    report("input lines", split_fit(input, prep, {}, versions), sum, "input");
    // --sheet 0: the input lines only (a quick check of a task design)
    if (!int_arg(argc, argv, "--sheet", 1)) {
        if (sum) {
            std::fclose(sum);
        }
        engine_free(engine, blob);
        return 0;
    }
    split_prepare(neurons, prep);
    SplitScore whole = split_fit(neurons, prep, {}, versions);
    report("whole sheet", whole, sum, "whole");
    report("motors' view", split_fit(neurons, prep, motors_view(tt.plug, *blob), versions), sum,
           "view");
    std::vector<SplitScore> tiles((size_t)blob->tile_count);
    int best = 0;
    for (int t = 0; t < blob->tile_count; t++) {
        std::vector<int> own;
        for (int k = 0; k < TILE_NEURONS; k++) {
            own.push_back(t * TILE_NEURONS + k);
        }
        tiles[(size_t)t] = split_fit(neurons, prep, own, versions);
        best = tiles[(size_t)t].check > tiles[(size_t)best].check ? t : best;
    }
    report("best tile (by check rows)", tiles[(size_t)best], sum, "tile");
    std::printf("  whole sheet by version (* held out):");
    for (int v = 0; v < versions; v++) {
        bool is_held = false;
        for (size_t r = 0; r < neurons.version.size() && !is_held; r++) {
            is_held = neurons.version[r] == v && neurons.held[r];
        }
        std::printf(" %d%s %.2f", v, is_held ? "*" : "", whole.by_version[(size_t)v]);
    }
    std::printf("\n");
    for (int which = 0; which < 2; which++) {
        std::printf("  one tile, %s:\n", which ? "held" : "same");
        for (int y = 0; y < blob->tiles_h; y++) {
            std::printf("   ");
            for (int x = 0; x < blob->tiles_w; x++) {
                const SplitScore& s = tiles[(size_t)(y * blob->tiles_w + x)];
                std::printf(" %4.2f", which ? s.held : s.same);
            }
            std::printf("\n");
        }
    }
    if (sum) {
        std::fclose(sum);
    }
    std::fflush(stdout);
    engine_free(engine, blob);
    return 0;
}
