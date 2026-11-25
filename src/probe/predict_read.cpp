// blobprobe predict: does a life with idea 162's predictor cells leave the inside knowing
// more? The sheet lives (blobrun's task, plug, seed and --g. flags, the --predict flags
// included, rewards delivered as blobrun delivers them), is frozen, and then watches the same
// world again with moves from a fixed coin (so every arm sees the same world). A fresh ridge
// reader (linear_fit.cpp, whole sheet) is fitted to each step's spike counts and asked
// things that are not on the input lines at one instant (predict_labels.hpp). Beside it the
// same reader on the hidden cells only (no sensor or motor cell), on the task's lines of
// this step, on this step's and the last step's lines (a perfect one-step memory), on every
// line of this step (good, bad and the extra lines too: the error lines can carry a change),
// and the most common answer. Firing rates are printed too: a busier sheet reads better for free.
// Own flags:
//   --life N       steps lived before the freeze (0: the sheet as born)
//   --shuffle 1    the control: the life is recorded, then a fresh sheet of the same seed
//                  lives the same frames (lines, good and bad, presses) in a shuffled
//                  order, with no task and no reward; it is that sheet that is read
//   --trials N     steps kept in the read (default 3000), after --settle N (default 300)
//   --label text, --out csv (label,question,key,value lines, appended)
// Docs: docs/results-2026-11-25-predict.md; the battery is scripts/predict.sh.

#include "blob/blob.hpp"
#include "probe/linear_fit.hpp"
#include "probe/predict_labels.hpp"
#include "run/predict.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

static const char* const own_flags[] = {"--life",   "--shuffle", "--trials",
                                        "--settle", "--label",   "--out"};

static TaskTicker ticker;
static Predictor predictor;

// a life's frames, for the shuffled control: each step's lines (good and bad first, as the
// plug numbers them, up to the extra lines) and the step's press
struct Frames {
    int lines = 0;
    std::vector<uint8_t> bits;
    std::vector<int> move;
};

static void start(Blob& blob, const RunFlags& flags) {
    ticker = TaskTicker{};
    if (!ticker_start(ticker, blob, flags) || !predict_attach(predictor, ticker, blob, flags)) {
        std::exit(1);
    }
}

// the life as blobrun lives it; frames kept when asked
static void live(Blob& blob, const RunFlags& flags, int64_t steps, Frames* keep) {
    start(blob, flags);
    TaskTicker& tt = ticker;
    if (keep) {
        keep->lines = tt.plug.first_extra;
    }
    for (int64_t step = 0; step < steps;) {
        bool first = tt.tick_in_step == 0 && tt.rest_left == 0;
        predict_before_tick(predictor, tt, blob);
        ticker_before_tick(tt, blob);
        if (first && keep) {
            for (int l = 0; l < keep->lines; l++) {
                keep->bits.push_back(tt.plug.line_on[l]);
            }
        }
        blob_tick(blob);
        predict_after_tick(predictor, tt, blob);
        if (!ticker_after_tick(tt, blob)) {
            continue;
        }
        predict_step_end(predictor, blob, tt.in.move);
        if (keep) {
            keep->move.push_back(tt.in.move);
        }
        if (tt.valence != 0 || blob.genome.critic_on) {
            plug_mark_acted(tt.plug, blob, tt.move);
            plug_mark_vote(tt.plug, blob, tt.move);
            blob_reward(blob, tt.valence);
        }
        step++;
    }
    predict_print(predictor.score, "life", stdout);
    plug_detach(tt.plug, blob);
}

static uint64_t mix(uint64_t& state) {
    uint64_t z = (state += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

// the control's life: the frames in a shuffled order, no task, no reward
static void replay(Blob& blob, const RunFlags& flags, const Frames& f) {
    start(blob, flags);
    Plug& plug = ticker.plug;
    size_t n = f.move.size();
    std::vector<size_t> order(n);
    for (size_t i = 0; i < n; i++) {
        order[i] = i;
    }
    uint64_t state = flags.seed * 0x5851f42d4c957f2dull + 17;
    for (size_t i = n; i > 1; i--) {
        size_t j = (size_t)(mix(state) % i);
        size_t swap = order[i - 1];
        order[i - 1] = order[j];
        order[j] = swap;
    }
    int ticks = flags.plug.ticks_per_step;
    for (size_t at : order) {
        const uint8_t* bits = &f.bits[at * (size_t)f.lines];
        for (int l = plug.first_single; l < f.lines; l++) {
            plug.line_on[l] = bits[l];
        }
        plug.good_left = bits[PLUG_GOOD_LINE] ? plug.settings.reward_line_ticks : 0;
        plug.bad_left = bits[PLUG_BAD_LINE] ? plug.settings.reward_line_ticks : 0;
        if (predictor.on) {
            predict_step_begin(predictor, plug, blob);
        }
        for (int t = 0; t < ticks; t++) {
            plug_write(plug, blob);
            blob_tick(blob);
            if (predictor.on) {
                predict_count(predictor, blob);
            }
        }
        predict_step_end(predictor, blob, f.move[at]);
    }
    predict_print(predictor.score, "shuffled", stdout);
    plug_detach(plug, blob);
}

// what the read keeps per question
struct Rows {
    LinearData sheet, hidden, now, two, all;
    std::vector<int> label;
};

static double majority(const std::vector<int>& label, int classes) {
    size_t n = label.size(), train = n * 3 / 4;
    std::vector<int> count((size_t)classes, 0);
    for (size_t r = 0; r < train; r++) {
        count[(size_t)label[r]]++;
    }
    int best = 0;
    for (int c = 1; c < classes; c++) {
        best = count[(size_t)c] > count[(size_t)best] ? c : best;
    }
    size_t right = 0;
    for (size_t r = train; r < n; r++) {
        right += label[r] == best;
    }
    return n > train ? (double)right / (double)(n - train) : 0;
}

static double fit(LinearData& d, const std::vector<int>& label, int classes, bool whole) {
    d.label = label;
    d.classes = classes;
    return whole ? linear_whole(d) : linear_score(d);
}

int probe_predict(int argc, char** argv) {
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
    int64_t life = int_arg(argc, argv, "--life", 0);
    bool shuffle = int_arg(argc, argv, "--shuffle", 0) != 0;
    int trials = int_arg(argc, argv, "--trials", 3000);
    int settle = int_arg(argc, argv, "--settle", 300);
    const char* label = text_arg(argc, argv, "--label", "run");
    const char* out = text_arg(argc, argv, "--out", nullptr);
    if (flags.task_seed == 0) {
        flags.task_seed = flags.seed;
    }
    flags.steps = life;

    Blob* blob = blob_birth(flags.genome, flags.seed);
    if (life > 0 && shuffle) {
        Frames frames;
        live(*blob, flags, life, &frames);
        blob_free(blob);
        blob = blob_birth(flags.genome, flags.seed);
        replay(*blob, flags, frames);
    } else if (life > 0) {
        live(*blob, flags, life, nullptr);
    }
    blob->genome.learning_on = 0;
    std::fflush(stdout);

    // the read: the same world afresh, moves from a coin
    RunFlags rf = flags;
    rf.task_seed = flags.task_seed + 7919;
    start(*blob, rf);
    TaskTicker& tt = ticker;
    Plug& plug = tt.plug;
    int n = blob->neuron_count;
    std::vector<int> sheet_cells, hidden_cells;
    for (int c = 0; c < n; c++) {
        uint64_t bit = 1ull << (c % TILE_NEURONS);
        if (blob->sensor[c / TILE_NEURONS] & bit) {
            continue;
        }
        sheet_cells.push_back(c);
        if (!(blob->motor[c / TILE_NEURONS] & bit)) {
            hidden_cells.push_back(c);
        }
    }
    int task_lines = plug.first_extra - plug.first_single;
    Rows rows[READ_MAX_LABELS];
    ReadLabels first;
    std::vector<uint8_t> count((size_t)n, 0), last_lines((size_t)task_lines, 0);
    ReadMemory memory;
    uint64_t coin = rf.task_seed * 0x2545f4914f6cdd1dull + 3;
    int64_t spikes_sheet = 0, spikes_hidden = 0, ticks = 0;
    std::vector<uint8_t> is_hidden((size_t)n, 0);
    for (int c : hidden_cells) {
        is_hidden[(size_t)c] = 1;
    }
    int kept = 0;
    for (int64_t step = 0; kept < trials; step++) {
        ReadLabels labels;
        bool asked = read_labels(tt.task, memory, labels);
        if (asked && first.count == 0) {
            first = labels;
        }
        TaskSense sense;
        task_sense(tt.task, sense);
        plug_set_lines(plug, sense);
        if (predictor.on) {
            predict_step_begin(predictor, plug, *blob);
        }
        std::memset(count.data(), 0, count.size());
        for (int t = 0; t < tt.ticks_per_step; t++) {
            plug_write(plug, *blob);
            blob_tick(*blob);
            for (int c : sheet_cells) {
                if ((blob->spikes[blob->now][c / TILE_NEURONS] >> (c % TILE_NEURONS)) & 1) {
                    count[(size_t)c] += count[(size_t)c] < 255;
                    spikes_sheet++;
                    spikes_hidden += is_hidden[(size_t)c];
                }
            }
            ticks++;
            if (predictor.on) {
                predict_count(predictor, *blob);
            }
        }
        int buttons = plug.motor_lines;
        int move = (int)(mix(coin) % (uint64_t)(buttons + 1)) - 1;
        read_remember(tt.task, memory, move);
        TaskStepIn in;
        in.move = move;
        in.sensed_neurons = plug.sensed_neurons;
        plug_reward(plug, task_step(tt.task, in));
        predict_step_end(predictor, *blob, move);
        bool keep = asked && step >= settle;
        for (int q = 0; keep && q < labels.count; q++) {
            Rows& r = rows[q];
            for (int c : sheet_cells) {
                r.sheet.counts.push_back(count[(size_t)c]);
            }
            for (int c : hidden_cells) {
                r.hidden.counts.push_back(count[(size_t)c]);
            }
            for (int l = 0; l < task_lines; l++) {
                uint8_t v = plug.line_on[plug.first_single + l];
                r.now.counts.push_back(v);
                r.two.counts.push_back(v);
            }
            for (int l = 0; l < task_lines; l++) {
                r.two.counts.push_back(last_lines[(size_t)l]);
            }
            for (int l = 0; l < plug.sensor_lines; l++) {
                r.all.counts.push_back(plug.line_on[l]);
            }
            r.label.push_back(labels.value[q]);
        }
        kept += keep;
        for (int l = 0; l < task_lines; l++) {
            last_lines[(size_t)l] = plug.line_on[plug.first_single + l];
        }
    }
    double cells = (double)sheet_cells.size(), hidden = (double)hidden_cells.size();
    double rate_sheet = 1000.0 * (double)spikes_sheet / (cells * (double)ticks);
    double rate_hidden = 1000.0 * (double)spikes_hidden / (hidden * (double)ticks);
    FILE* csv = out ? std::fopen(out, "a") : nullptr;
    for (int q = 0; q < first.count; q++) {
        Rows& r = rows[q];
        r.sheet.neurons = (int)sheet_cells.size();
        r.hidden.neurons = (int)hidden_cells.size();
        r.now.neurons = task_lines;
        r.two.neurons = 2 * task_lines;
        r.all.neurons = plug.sensor_lines;
        int classes = first.classes[q];
        double values[8] = {fit(r.sheet, r.label, classes, true),
                            fit(r.hidden, r.label, classes, true),
                            fit(r.now, r.label, classes, false),
                            fit(r.two, r.label, classes, false),
                            fit(r.all, r.label, classes, false),
                            majority(r.label, classes),
                            rate_sheet,
                            rate_hidden};
        static const char* const keys[8] = {"sheet",     "hidden",   "lines_now",  "lines_two",
                                            "lines_all", "majority", "rate_sheet", "rate_hidden"};
        std::printf("%-24s %-11s", label, first.name[q]);
        for (int k = 0; k < 8; k++) {
            std::printf(" %s %.3f", keys[k], values[k]);
            if (csv) {
                std::fprintf(csv, "%s,%s,%s,%.6g\n", label, first.name[q], keys[k], values[k]);
            }
        }
        std::printf("\n");
    }
    if (csv) {
        std::fclose(csv);
    }
    blob_free(blob);
    return 0;
}
