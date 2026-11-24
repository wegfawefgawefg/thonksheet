// blobprobe linear: the reference reader for idea 58. Runs a real task through the plug, on
// the frozen sheet (learning off unless --learning 1), with the task's own timing, and keeps
// every neuron's spike count over each scored step (sensor cells left out, as in the
// feature probe). Then a plain linear reader is fitted to those counts and scored on held-
// out trials: share right, the task's own units. blobrun's task, plug, seed and --g. flags
// all work. Own flags: --trials N (scored steps kept, default 4000), --settle N (ticks run
// before keeping any, default 30000), --learning 0|1, --label text.

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

static const char* const linear_only[] = {"--trials", "--settle", "--learning", "--label"};

// the tile of the first cell of a motor line, and of the first cue line
static void plug_tiles(const Plug& plug, const Blob& blob, int& cue_tile,
                       std::vector<int>& motor_tiles) {
    int first = plug.sensor_lines > plug.first_single ? plug.first_single : 0;
    cue_tile = plug.sensor_neuron[first * PLUG_MAX_GROUP] / TILE_NEURONS;
    int group = plug_motor_group(plug);
    for (int line = 0; line < plug.motor_lines; line++) {
        for (int k = 0; k < group; k++) {
            int t = plug.motor_neuron[line * PLUG_MAX_GROUP + k] / TILE_NEURONS;
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
    if (!learning) {
        flags.genome.learning_on = 0;
    }
    if (flags.frozen_readout) {
        flags.genome.learn_mode = 1;
    }
    if (flags.task_seed == 0) {
        flags.task_seed = flags.seed;
    }

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
    int64_t ticks = 0;
    int kept = 0;
    while (kept < trials) {
        if (tt.tick_in_step == 0 && tt.rest_left == 0) {
            std::memset(count.data(), 0, count.size());
        }
        ticker_before_tick(tt, *blob);
        blob_tick(*blob);
        ticks++;
        for (int t = 0; t < blob->tile_count; t++) {
            uint64_t spikes = blob->spikes[blob->now][t] & ~blob->sensor[t];
            while (spikes) {
                uint8_t& c = count[(size_t)(t * TILE_NEURONS + __builtin_ctzll(spikes))];
                c = c < 255 ? c + 1 : c;
                spikes &= spikes - 1;
            }
        }
        int64_t tries_before = tt.task.tally[1];
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        if (tt.valence != 0) {
            blob_reward(*blob, tt.valence);
        }
        bool scored = tt.task.tally[1] > tries_before;
        if (ticks < settle || !scored || tt.right_move < 0) {
            continue;
        }
        data.counts.insert(data.counts.end(), count.begin(), count.end());
        data.label.push_back(tt.right_move);
        kept++;
    }
    int cue_tile = 0;
    std::vector<int> motor_tiles;
    plug_tiles(tt.plug, *blob, cue_tile, motor_tiles);
    linear_report(data, label, cue_tile, motor_tiles);
    blob_free(blob);
    return 0;
}
