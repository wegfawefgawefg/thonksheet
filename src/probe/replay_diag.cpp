// blobprobe replay: one life with reverse replay (idea 62) and what the walk did. Runs a task
// through the plug like blobrun (blobrun's task, plug, seed, --steps, --window and --g.
// flags), marking the pressed button before each reward as blobrun does, and prints per
// window: the score, error walks, spikes walked at each depth (per walk), how many named both
// signs, counter steps up and down, rewires that changed a connection; and at the end how far
// the inner wiring has moved from birth.

#include "blob/blob.hpp"
#include "blob/replay.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cstdio>
#include <cstring>
#include <vector>

int probe_replay(int argc, char** argv) {
    std::vector<char*> rest = {argv[0]};
    for (int i = 2; i < argc; i++) {
        rest.push_back(argv[i]);
    }
    RunFlags flags;
    apply_genome_args((int)rest.size(), rest.data(), flags.genome);
    if (!run_flags_parse((int)rest.size(), rest.data(), flags)) {
        return 1;
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
    int depth = flags.genome.replay_depth;
    depth = depth > REPLAY_DEPTH_MAX ? REPLAY_DEPTH_MAX : depth;
    ReplayState last = {};
    std::printf("%7s %6s %6s", "step", "score", "walks");
    for (int d = 0; d <= depth; d++) {
        std::printf("   depth%d", d);
    }
    std::printf(" %9s %9s %9s %9s\n", "conflict", "up", "down", "rewired");
    int64_t step = 0;
    while (step < flags.steps) {
        ticker_before_tick(tt, *blob);
        blob_tick(*blob);
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        if (tt.valence != 0) {
            plug_mark_acted(tt.plug, *blob, tt.move);
            blob_reward(*blob, tt.valence);
        }
        step++;
        if (step % flags.window != 0) {
            continue;
        }
        double score = task_window_score(tt.task);
        const ReplayState* r = blob->replay;
        if (!r) {
            std::printf("%7lld %6.3f (no replay state)\n", (long long)step, score);
            continue;
        }
        int64_t walks = r->walks - last.walks;
        double per = walks > 0 ? 1.0 / (double)walks : 0.0;
        std::printf("%7lld %6.3f %6lld", (long long)step, score, (long long)walks);
        for (int d = 0; d <= depth; d++) {
            std::printf(" %8.1f", (double)(r->walked[d] - last.walked[d]) * per);
        }
        std::printf(" %9.1f %9.1f %9.1f %9.1f\n", (double)(r->conflicts - last.conflicts) * per,
                    (double)(r->up - last.up) * per, (double)(r->down - last.down) * per,
                    (double)(r->rewired - last.rewired) * per);
        last = *r;
    }
    int64_t gained = 0, lost = 0, connections = 0, inner = 0, rows = 0;
    replay_changed(*blob, gained, lost);
    for (int n = 0; n < blob->neuron_count; n++) {
        int t = n / TILE_NEURONS, b = n % TILE_NEURONS;
        if (((blob->sensor[t] | blob->motor[t]) >> b) & 1) {
            continue;
        }
        inner++;
        connections += blob->fan_in[n];
        rows += blob->replay && blob->replay->keep[n] ? 1 : 0;
    }
    std::printf("inner neurons %lld, %.1f connections each; %lld have counters; since birth "
                "%lld connections gained, %lld lost (%.1f%% of them)\n",
                (long long)inner, (double)connections / (double)(inner ? inner : 1),
                (long long)rows, (long long)gained, (long long)lost,
                100.0 * (double)lost / (double)(connections ? connections : 1));
    // where the wiring moved: connections gained per inner neuron, by tile row (the cues sit
    // in the top half, the motor cells in the bottom half)
    const ReplayState* r = blob->replay;
    if (r) {
        std::printf("gained per inner neuron by tile row:");
        for (int ty = 0; ty < blob->tiles_h; ty++) {
            int64_t row_gained = 0, row_inner = 0;
            for (int tx = 0; tx < blob->tiles_w; tx++) {
                int t = ty * blob->tiles_w + tx;
                uint64_t in = ~blob->sensor[t] & ~blob->motor[t];
                row_inner += __builtin_popcountll(in);
                for (int k = 0; k < SLOTS; k++) {
                    for (int b = 0; b < TILE_NEURONS; b++) {
                        int64_t w = blob_weight_index(t * TILE_NEURONS + b, k);
                        uint64_t was = r->born_w0[w] | r->born_w1[w];
                        uint64_t is = blob->w0[w] | blob->w1[w];
                        row_gained += ((in >> b) & 1) ? __builtin_popcountll(is & ~was) : 0;
                    }
                }
            }
            std::printf(" %.2f", (double)row_gained / (double)(row_inner ? row_inner : 1));
        }
        std::printf("\n");
    }
    blob_free(blob);
    return 0;
}
