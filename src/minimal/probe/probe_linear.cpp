#include "minimal/probe/probe.hpp"
#include "probe/linear_fit.hpp"
#include "run/task_ticker.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

// minimalprobe linear: blobprobe linear (idea 58's reference reader) for either engine. a
// real task through the plug, the task's own timing; every non-sensor neuron's spike count
// over each scored step; a ridge reader fitted on the first three quarters of the trials and
// scored on the last (whole sheet, each tile, each 3x3 block, the motors' view). learning is
// on for the first --teach-steps steps (default 0: the sheet as born) and off from then on,
// through --settle ticks and the --trials kept steps.

int probe_linear(ProbeArgs& args) {
    const RunFlags& f = args.flags;
    Blob* blob = probe_birth(args, f.genome, f.seed);
    TaskTicker tt;
    if (!ticker_start(tt, *blob, f)) {
        probe_free(args, blob);
        return 1;
    }
    // teach, with the reward and the pressed button as blobrun gives them
    for (int64_t step = 0; step < args.teach_steps;) {
        ticker_before_tick(tt, *blob);
        probe_tick(args, *blob);
        if (ticker_after_tick(tt, *blob)) {
            if (tt.valence != 0) {
                plug_mark_acted(tt.plug, *blob, tt.move);
                probe_reward(args, *blob, tt.valence);
            }
            step++;
        }
    }
    probe_set_learning(args, *blob, 0);

    LinearData data;
    data.neurons = blob->neuron_count;
    data.tiles_w = blob->tiles_w;
    data.tiles_h = blob->tiles_h;
    data.classes = tt.plug.motor_lines;
    std::vector<uint8_t> count((size_t)blob->neuron_count, 0);
    int64_t ticks = 0;
    int kept = 0;
    while (kept < args.trials) {
        if (tt.tick_in_step == 0 && tt.rest_left == 0) {
            std::memset(count.data(), 0, count.size());
        }
        bool resting = tt.rest_left > 0;
        ticker_before_tick(tt, *blob);
        probe_tick(args, *blob);
        ticks++;
        for (int t = 0; !resting && t < blob->tile_count; t++) {
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
        bool scored = tt.task.tally[1] > tries_before;
        if (ticks < args.warmup || !scored || tt.right_move < 0) {
            continue;
        }
        data.counts.insert(data.counts.end(), count.begin(), count.end());
        data.label.push_back(tt.right_move);
        kept++;
    }
    // the cue tile and the buttons' tiles, marked on the grids
    int first = tt.plug.sensor_lines > tt.plug.first_single ? tt.plug.first_single : 0;
    int cue_tile = tt.plug.sensor_neuron[first * PLUG_MAX_GROUP] / TILE_NEURONS;
    std::vector<int> motor_tiles;
    for (int line = 0; line < tt.plug.motor_lines; line++) {
        const int* cells = nullptr;
        int group = plug_motor_cells(tt.plug, line, &cells);
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
    char label[96];
    std::snprintf(label, sizeof(label), "%s %s s%llu", probe_engine_name(args), task_name(f.task),
                  (unsigned long long)f.seed);
    linear_report(data, label, cue_tile, motor_tiles);
    probe_free(args, blob);
    return 0;
}
