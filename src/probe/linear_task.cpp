// blobprobe linear: the reference reader for idea 58. Runs a real task through the plug, on
// the frozen sheet (learning off unless --learning 1), with the task's own timing, and keeps
// every neuron's spike count over each scored step (sensor cells left out, as in the
// feature probe). Then a plain linear reader is fitted to those counts and scored on held-
// out trials: share right, the task's own units. blobrun's task, plug, seed and --g. flags
// all work. Own flags: --trials N (scored steps kept, default 4000), --settle N (ticks run
// before keeping any, default 30000), --learning 0|1, --label text, --read-from N and
// --read-to N (count only those ticks of a step, from 0).
//
// The shortcut check (tiers 9 and 10): the same reader fitted to the task's own input lines
// over the last --history steps (default 16), and to how many lines were on at each of those
// steps (brightness, as one line per count 0..15, so any function of each step's count is
// open to it). A perfect memory with a linear reader; whatever it gets, a task does not
// need the sheet for. --lines-from N --lines-to N keep only those of the task's lines (from 0,
// the first after good and bad), to ask what one item alone tells.

#include "blob/blob.hpp"
#include "probe/linear_fit.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

static const char* const linear_only[] = {"--trials",  "--settle",     "--learning",
                                          "--label",   "--read-from",  "--read-to",
                                          "--history", "--lines-from", "--lines-to"};

// the tile of the first cell of a motor line, and of the first cue line
static void plug_tiles(const Plug& plug, const Blob& blob, int& cue_tile,
                       std::vector<int>& motor_tiles) {
    int first = plug.sensor_lines > plug.first_single ? plug.first_single : 0;
    cue_tile = plug.sensor_neuron[first * PLUG_MAX_GROUP] / TILE_NEURONS;
    for (int line = 0; line < plug.motor_lines; line++) {
        const int* cells = nullptr;
        int group = plug_motor_cells(plug, line, &cells);
        for (int k = 0; k < group; k++) {
            int t = cells[k] / TILE_NEURONS;
            bool seen = false;
            for (int m : motor_tiles) {
                seen = seen || m == t;
            }
            if (!seen) {
                motor_tiles.push_back(t);
            }
        }
    }
    (void)blob;
}

int probe_linear(int argc, char** argv) {
    // own flags out, the rest to blobrun's parser (argv[1] is the word "linear")
    std::vector<char*> rest = {argv[0]};
    for (int i = 2; i < argc; i++) {
        bool mine = false;
        for (const char* name : linear_only) {
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
    int trials = int_arg(argc, argv, "--trials", 4000);
    int64_t settle = int_arg(argc, argv, "--settle", 30000);
    int learning = int_arg(argc, argv, "--learning", 0);
    const char* label = text_arg(argc, argv, "--label", "run");
    // only these ticks of a step are counted (0 .. ticks per step)
    int read_from = int_arg(argc, argv, "--read-from", 0);
    int read_to = int_arg(argc, argv, "--read-to", 1 << 30);
    if (!learning) {
        flags.genome.learning_on = 0;
    }
    if (flags.frozen_readout) {
        flags.genome.learn_mode = 1;
    }
    if (flags.task_seed == 0) {
        flags.task_seed = flags.seed;
    }

    int history = int_arg(argc, argv, "--history", 16);
    history = history < 1 ? 1 : history;

    Blob* blob = blob_birth(flags.genome, flags.seed);
    static TaskTicker tt;
    tt = TaskTicker{};
    if (!ticker_start(tt, *blob, flags)) {
        blob_free(blob);
        return 1;
    }
    LinearData data;
    data.neurons = blob->neuron_count;
    data.tiles_w = blob->tiles_w;
    data.tiles_h = blob->tiles_h;
    data.classes = tt.plug.motor_lines;
    std::vector<uint8_t> count((size_t)blob->neuron_count, 0);
    // the task's own lines over the last `history` steps, newest first
    const Plug& plug = tt.plug;
    int task_lines = plug.sensor_lines - plug.first_single;
    int lines_from = int_arg(argc, argv, "--lines-from", 0);
    int lines_to = int_arg(argc, argv, "--lines-to", task_lines);
    lines_from = lines_from < 0 ? 0 : lines_from;
    lines_to = lines_to > task_lines ? task_lines : lines_to;
    int used = lines_to > lines_from ? lines_to - lines_from : 0;
    std::vector<uint8_t> line_ring((size_t)(history * (used > 0 ? used : 1)), 0);
    const int counts = 16; // brightness: one feature per count, 0 to 15 or more
    std::vector<uint8_t> bright_ring((size_t)(history * counts), 0);
    LinearData lines_data, bright_data;
    lines_data.neurons = history * used;
    bright_data.neurons = history * counts;
    lines_data.classes = bright_data.classes = data.classes;
    int64_t probe_before = 0;
    int64_t ticks = 0;
    int kept = 0;
    while (kept < trials) {
        if (tt.tick_in_step == 0 && tt.rest_left == 0) {
            std::memset(count.data(), 0, count.size());
        }
        bool resting = tt.rest_left > 0;
        int in_step = tt.tick_in_step;
        ticker_before_tick(tt, *blob);
        if (!resting && in_step == 0 && used > 0) {
            // a new step's lines go in front of the rings
            std::memmove(&line_ring[(size_t)used], &line_ring[0], (size_t)(history - 1) * used);
            std::memmove(&bright_ring[counts], &bright_ring[0], (size_t)(history - 1) * counts);
            int on = 0;
            for (int i = 0; i < used; i++) {
                uint8_t v = plug.line_on[plug.first_single + lines_from + i];
                line_ring[(size_t)i] = v;
                on += v;
            }
            for (int c = 0; c < counts; c++) {
                bright_ring[(size_t)c] = (on < counts ? on : counts - 1) == c;
            }
        }
        blob_tick(*blob);
        ticks++;
        bool counted = !resting && in_step >= read_from && in_step < read_to;
        for (int t = 0; counted && t < blob->tile_count; t++) {
            uint64_t spikes = blob->spikes[blob->now][t] & ~blob->sensor[t];
            while (spikes) {
                uint8_t& c = count[(size_t)(t * TILE_NEURONS + __builtin_ctzll(spikes))];
                c = c < 255 ? c + 1 : c;
                spikes &= spikes - 1;
            }
        }
        int64_t tries_before = tt.task.tally[1];
        probe_before = tt.task.probe_marks;
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        if (tt.valence != 0) {
            blob_reward(*blob, tt.valence);
        }
        // the older tasks mark a scored step in tally[1]; tiers 9 and 10 in probe_marks
        bool scored = script_kind(tt.task.kind) ? tt.task.probe_marks > probe_before
                                                : tt.task.tally[1] > tries_before;
        if (ticks < settle || !scored || tt.right_move < 0) {
            continue;
        }
        data.counts.insert(data.counts.end(), count.begin(), count.end());
        data.label.push_back(tt.right_move);
        lines_data.counts.insert(lines_data.counts.end(), line_ring.begin(),
                                 line_ring.begin() + (size_t)lines_data.neurons);
        lines_data.label.push_back(tt.right_move);
        bright_data.counts.insert(bright_data.counts.end(), bright_ring.begin(), bright_ring.end());
        bright_data.label.push_back(tt.right_move);
        kept++;
    }
    int cue_tile = 0;
    std::vector<int> motor_tiles;
    plug_tiles(tt.plug, *blob, cue_tile, motor_tiles);
    linear_report(data, label, cue_tile, motor_tiles);
    if (used > 0) {
        std::printf("%-28s input lines %d..%d, last %d steps %.3f  brightness, last %d steps "
                    "%.3f\n",
                    label, lines_from, lines_to - 1, history, linear_score(lines_data), history,
                    linear_score(bright_data));
    }
    blob_free(blob);
    return 0;
}
