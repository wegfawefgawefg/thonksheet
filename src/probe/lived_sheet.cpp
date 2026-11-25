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
//
// idea 155 adds, for transfer and for lives of several tasks:
//   --then-flags "<blobrun flags>"  the second task in full (task, its settings, plug), in
//                                   place of the life's flags with --then TASK
//   --then-perm 1                   the buttons take one another's cells (a derangement drawn
//                                   from the seed): the same cues, other cue-to-button pairs
//   --then-move L                   cue line L (0 is the first) moves to the free sensor group
//                                   nearest its own: a line the life never showed
//   --mix "<blobrun flags>" --mix-every N   the life alternates its own task and this one
//                                   every N steps on the same blob, each restart a fresh task
//                                   (task seed + segment), the readout learning throughout

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

// changes to the plug of the second task (transfer)
struct PlugEdit {
    int perm = 0;  // buttons take one another's cells
    int move = -1; // this cue line moves to a free sensor group
    uint64_t seed = 1;
};

// --then-perm: line L reads the cells line perm[L] had, perm a derangement
static void permute_buttons(Plug& plug, uint64_t seed) {
    int k = plug.motor_lines;
    if (plug.settings.motor_spread <= 0 || k < 2) {
        std::fprintf(stderr, "blobprobe transfer: --then-perm needs --motor-spread\n");
        std::exit(1);
    }
    uint64_t r = seed * 0x9e3779b97f4a7c15ull + 77;
    int perm[PLUG_MAX_MOTOR_LINES];
    for (bool fixed = true; fixed;) {
        for (int i = 0; i < k; i++) {
            perm[i] = i;
        }
        for (int i = k - 1; i > 0; i--) {
            r = r * 6364136223846793005ull + 1442695040888963407ull;
            int j = (int)((r >> 33) % (uint64_t)(i + 1));
            int t = perm[i];
            perm[i] = perm[j];
            perm[j] = t;
        }
        fixed = false;
        for (int i = 0; i < k; i++) {
            fixed = fixed || perm[i] == i;
        }
    }
    static int first[PLUG_MAX_MOTOR_LINES + 1], cells[PLUG_MAX_SPREAD];
    int used = 0;
    for (int line = 0; line < k; line++) {
        first[line] = used;
        for (int c = plug.spread_first[perm[line]]; c < plug.spread_first[perm[line] + 1]; c++) {
            cells[used++] = plug.spread_neuron[c];
        }
    }
    first[k] = used;
    for (int line = 0; line <= k; line++) {
        plug.spread_first[line] = first[line];
    }
    for (int c = 0; c < used; c++) {
        plug.spread_neuron[c] = cells[c];
    }
    std::fprintf(stderr, "then-perm:");
    for (int i = 0; i < k; i++) {
        std::fprintf(stderr, " %d<-%d", i, perm[i]);
    }
    std::fprintf(stderr, "\n");
}

// --then-move: the cue line's cells move to the free sensor group nearest its own
static void move_line(Plug& plug, Blob& blob, int cue) {
    int line = plug.first_single + cue;
    const Genome& g = blob.genome;
    int side = plug.settings.sensor_side, pitch = side * g.sensor_stride;
    int group = plug_sensor_group(plug);
    int best_x = -1, best_y = -1, best = 1 << 30;
    for (int y = g.sensor_y; y + pitch <= g.sensor_y + g.sensor_h; y += pitch) {
        for (int x = g.sensor_x; x + pitch <= g.sensor_x + g.sensor_w; x += pitch) {
            bool taken = false;
            for (int l = 0; l < plug.sensor_lines; l++) {
                taken = taken || (plug.sensor_cell_x[l] == x && plug.sensor_cell_y[l] == y);
            }
            int dx = x - plug.sensor_cell_x[line], dy = y - plug.sensor_cell_y[line];
            if (!taken && dx * dx + dy * dy < best) {
                best = dx * dx + dy * dy;
                best_x = x;
                best_y = y;
            }
        }
    }
    if (best_x < 0) {
        std::fprintf(stderr, "blobprobe transfer: no free sensor group\n");
        std::exit(1);
    }
    for (int k = 0; k < group; k++) {
        blob_detach(blob, plug.sensor_neuron[line * PLUG_MAX_GROUP + k]);
    }
    for (int k = 0; k < group; k++) {
        int dy = k / side, dx = k % side;
        int n = blob_neuron_at(blob, best_x + dx * g.sensor_stride, best_y + dy * g.sensor_stride);
        plug.sensor_neuron[line * PLUG_MAX_GROUP + k] = n;
        blob_sense(blob, n, false);
    }
    std::fprintf(stderr, "then-move: cue line %d from cell (%d, %d) to (%d, %d)\n", cue,
                 plug.sensor_cell_x[line], plug.sensor_cell_y[line], best_x, best_y);
    plug.sensor_cell_x[line] = best_x;
    plug.sensor_cell_y[line] = best_y;
}

// one task for `steps` steps on a blob already born, as blobrun's life loop does it.
// returns the window scores
static std::vector<double> live(Blob& blob, const RunFlags& flags, int64_t steps, int window,
                                const PlugEdit& edit = PlugEdit{}) {
    static TaskTicker tt;
    tt = TaskTicker{};
    std::vector<double> scores;
    if (!ticker_start(tt, blob, flags)) {
        return scores;
    }
    if (edit.perm) {
        permute_buttons(tt.plug, edit.seed);
    }
    if (edit.move >= 0) {
        move_line(tt.plug, blob, edit.move);
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
    std::vector<double> s;
    const char* mix = text_arg(argc, argv, "--mix", nullptr);
    if (mix) {
        // two tasks taking turns on one blob
        RunFlags other;
        if (!parse_life(mix, genome, seed, other)) {
            std::exit(1);
        }
        int64_t every = int_arg(argc, argv, "--mix-every", 1000);
        std::vector<double> last[2];
        for (int64_t done = 0, seg = 0; done < flags.steps; done += every, seg++) {
            RunFlags f = seg % 2 == 0 ? flags : other;
            f.task_seed = seed + (uint64_t)seg;
            int64_t n = every < flags.steps - done ? every : flags.steps - done;
            last[seg % 2] = live(*blob, f, n, flags.window < n ? flags.window : (int)n);
            s.insert(s.end(), last[seg % 2].begin(), last[seg % 2].end());
        }
        std::fprintf(stderr, "mix: last segment of the life's task %.3f, of the other %.3f\n",
                     mean_of(last[0], 0, last[0].size()), mean_of(last[1], 0, last[1].size()));
    } else {
        s = live(*blob, flags, flags.steps, flags.window);
    }
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
    blob.genome.replay_depth = 0;
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
    const char* then_flags = text_arg(argc, argv, "--then-flags", nullptr);
    if (then_flags && !parse_life(then_flags, genome, seed, then)) {
        return 1;
    }
    then.task_seed = seed + 100;
    int64_t steps = int_arg(argc, argv, "--then-steps", 20000);
    int window = int_arg(argc, argv, "--window", 1000);
    PlugEdit edit;
    edit.perm = int_arg(argc, argv, "--then-perm", 0);
    edit.move = int_arg(argc, argv, "--then-move", -1);
    edit.seed = seed;
    fresh_readout(*blob, seed);
    std::vector<double> s = live(*blob, then, steps, window, edit);
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
