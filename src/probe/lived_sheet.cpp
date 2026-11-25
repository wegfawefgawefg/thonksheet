// A sheet that has lived a task life first, for probes that ask what the inside learned
// (action-feedback tags, docs/results-2026-11-25-tags-and-margin.md).
//
// blobprobe feature ... --life "<blobrun flags>": the feature probe on the sheet left after
// that life (learning numbers from the probe's --g. flags, so tags can be on in the life;
// the probe's own learning_on is put back after it).
//
// blobprobe transfer --life "<blobrun flags>" [--born 1] --then TASK [--then-k N]
//     [--then-steps N] [--window N] [--g.name value ...]
// lives the life (none with --born 1), then freezes the inside (learn mode 5, no tags, no
// margin rule), puts every motor cell's wiring back as born and wipes the readout's counters,
// and lets that fresh readout learn TASK on the same plug. Prints the fresh readout's
// windows, its mean over the first quarter and its late score (last tenth).

#include "blob/blob.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

// blobrun flags from one string, split on spaces; the blob and task seed are the probe's
static bool parse_life(const char* text, const Genome& genome, uint64_t seed, RunFlags& flags) {
    static std::vector<std::string> words;
    words.clear();
    std::string w;
    for (const char* p = text;; p++) {
        if (*p == ' ' || *p == '\0') {
            if (!w.empty()) {
                words.push_back(w);
            }
            w.clear();
            if (*p == '\0') {
                break;
            }
            continue;
        }
        w += *p;
    }
    std::vector<char*> argv = {(char*)"blobprobe"};
    for (std::string& s : words) {
        argv.push_back(s.data());
    }
    flags = RunFlags{};
    flags.genome = genome;
    apply_genome_args((int)argv.size(), argv.data(), flags.genome);
    if (!run_flags_parse((int)argv.size(), argv.data(), flags)) {
        return false;
    }
    flags.seed = seed;
    flags.task_seed = seed;
    return true;
}

// one task for `steps` steps on a blob already born, as blobrun's life loop does it.
// returns the window scores
static std::vector<double> live(Blob& blob, const RunFlags& flags, int64_t steps, int window) {
    static TaskTicker tt;
    tt = TaskTicker{};
    std::vector<double> scores;
    if (!ticker_start(tt, blob, flags)) {
        return scores;
    }
    int64_t step = 0;
    while (step < steps) {
        ticker_before_tick(tt, blob);
        blob_tick(blob);
        if (!ticker_after_tick(tt, blob)) {
            continue;
        }
        if (tt.valence != 0) {
            plug_mark_acted(tt.plug, blob, tt.move);
            plug_mark_vote(tt.plug, blob, tt.move);
            plug_hop_lines(tt.plug, blob, tt.move, tt.right_move, tt.in.pattern,
                           tt.in.pattern_tied, tt.task.pattern_graded);
            blob_reward(blob, tt.valence);
        }
        step++;
        if (step % window == 0) {
            scores.push_back(task_window_score(tt.task));
        }
    }
    plug_detach(tt.plug, blob);
    return scores;
}

static double mean_of(const std::vector<double>& v, size_t from, size_t to) {
    double sum = 0;
    for (size_t i = from; i < to && i < v.size(); i++) {
        sum += v[i];
    }
    return to > from ? sum / (double)(to - from) : 0;
}

// born, or born and lived (--life). on a lived sheet the genome's learning numbers are the
// probe's again afterwards
Blob* probe_sheet(int argc, char** argv, const Genome& genome, uint64_t seed) {
    const char* life = text_arg(argc, argv, "--life", nullptr);
    if (!life || int_arg(argc, argv, "--born", 0)) {
        return blob_birth(genome, seed);
    }
    RunFlags flags;
    if (!parse_life(life, genome, seed, flags)) {
        std::exit(1);
    }
    flags.genome.learning_on = 1;
    Blob* blob = blob_birth(flags.genome, seed);
    std::vector<double> s = live(*blob, flags, flags.steps, flags.window);
    size_t edge = s.size() / 10 > 0 ? s.size() / 10 : 1;
    std::fprintf(stderr,
                 "life: late %.3f; tags: %u verdicts moved %u neurons; margin: %u of %u "
                 "goods thin\n",
                 mean_of(s, s.size() - edge, s.size()), blob->tag_events, blob->tag_count,
                 blob->margin_thin, blob->margin_goods);
    blob->genome = genome;
    return blob;
}

// the inside frozen; motor cells as born, the readout's counters gone
static void fresh_readout(Blob& blob, uint64_t seed) {
    Blob* born = blob_birth(blob.genome, seed);
    for (int t = 0; t < blob.tile_count; t++) {
        uint64_t cells = blob.motor[t];
        while (cells) {
            int n = t * TILE_NEURONS + __builtin_ctzll(cells);
            cells &= cells - 1;
            for (int k = 0; k < SLOTS; k++) {
                int64_t w = blob_weight_index(n, k);
                blob.w0[w] = born->w0[w];
                blob.w1[w] = born->w1[w];
            }
            for (int k = 0; k < FAR_SLOTS; k++) {
                blob.far_tile[n * FAR_SLOTS + k] = born->far_tile[n * FAR_SLOTS + k];
            }
            blob.fan_in[n] = born->fan_in[n];
        }
    }
    blob_free(born);
    std::free(blob.readout_row);
    std::free(blob.readout_evidence);
    std::free(blob.readout_keep);
    blob.readout_row = nullptr;
    blob.readout_evidence = nullptr;
    blob.readout_keep = nullptr;
    blob.genome.learn_mode = 5;
    blob.genome.tag_mode = 0;
    blob.genome.margin_percent = 0;
}

int probe_transfer(int argc, char** argv) {
    Genome genome;
    apply_genome_args(argc, argv, genome);
    uint64_t seed = (uint64_t)int_arg(argc, argv, "--seed", 1);
    const char* life = text_arg(argc, argv, "--life", nullptr);
    if (!life) {
        std::fprintf(stderr, "blobprobe transfer: --life \"<blobrun flags>\" is needed\n");
        return 1;
    }
    RunFlags then;
    if (!parse_life(life, genome, seed, then)) {
        return 1;
    }
    Blob* blob = probe_sheet(argc, argv, then.genome, seed);
    const char* task = text_arg(argc, argv, "--then", nullptr);
    if (task && !task_from_name(task, then.task)) {
        std::fprintf(stderr, "blobprobe transfer: no task '%s'\n", task);
        return 1;
    }
    then.task_settings.k = int_arg(argc, argv, "--then-k", then.task_settings.k);
    then.task_seed = seed + 100;
    int64_t steps = int_arg(argc, argv, "--then-steps", 20000);
    int window = int_arg(argc, argv, "--window", 1000);
    fresh_readout(*blob, seed);
    std::vector<double> s = live(*blob, then, steps, window);
    size_t edge = s.size() / 10 > 0 ? s.size() / 10 : 1;
    std::printf("transfer %s %s seed %llu: first quarter %.4f late %.4f\n", task_name(then.task),
                int_arg(argc, argv, "--born", 0) ? "born" : "lived", (unsigned long long)seed,
                mean_of(s, 0, s.size() / 4 > 0 ? s.size() / 4 : 1),
                mean_of(s, s.size() - edge, s.size()));
    std::printf("windows:");
    for (double v : s) {
        std::printf(" %.3f", v);
    }
    std::printf("\n");
    blob_free(blob);
    return 0;
}
