#include "minimal/probe/probe.hpp"
#include "run/task_ticker.hpp"

#include <cmath>
#include <cstdio>
#include <vector>

// how long a cue survives in activity after it goes off. delayed echo (k=2): teach for
// --steps steps (learning as the flags say), then freeze and run --test-steps steps. for every
// tick of a trial (cue step, blank steps, ask step) and every neuron, the spike chance under
// cue 0 and under cue 1; their gap in units of the spread, chance taken off and summed over
// neurons, is what an ideal reader of that group could get at that tick (as blobprobe carry
// does over tiles). 1.0 is easy to read; under 0.3 is lost.

// ideal-reader number over the neurons in `group` at trial tick `tick`
static double readable(const std::vector<uint32_t>& on, const int64_t* trials, int neurons,
                       int tick, const std::vector<uint8_t>& group) {
    double n0 = (double)(trials[0] ? trials[0] : 1), n1 = (double)(trials[1] ? trials[1] : 1);
    double chance = 1.0 / n0 + 1.0 / n1;
    double total = 0;
    for (int n = 0; n < neurons; n++) {
        if (!group[(size_t)n]) {
            continue;
        }
        size_t at = ((size_t)tick * neurons + (size_t)n) * 2;
        double p0 = on[at] / n0, p1 = on[at + 1] / n1;
        double spread = (p0 * (1 - p0) + p1 * (1 - p1)) / 2.0;
        if (spread > 0) {
            total += (p1 - p0) * (p1 - p0) / spread - chance;
        }
    }
    return total > 0 ? std::sqrt(total) : 0.0;
}

int probe_trace(ProbeArgs& args) {
    const RunFlags& f = args.flags;
    Blob* blob = probe_birth(args, f.genome, f.seed);
    TaskTicker tt;
    bool fits = ticker_start(tt, *blob, f);
    bool delayed = tt.task.kind == TaskKind::delayed_echo;
    if (!fits || (!delayed && tt.task.kind != TaskKind::echo)) {
        std::fprintf(stderr, "probe trace: needs --task delayed-echo or echo that fits\n");
        return 1;
    }
    // echo: a trial is one step, so this reads how far the cue reaches the motor neurons
    const int tps = tt.ticks_per_step;
    const int trial_ticks = delayed ? (tt.task.settings.delay + 2) * tps : tps;
    const int neurons = blob->neuron_count;

    // teach
    for (int64_t step = 0; step < f.steps;) {
        ticker_before_tick(tt, *blob);
        probe_tick(args, *blob);
        if (ticker_after_tick(tt, *blob)) {
            if (tt.valence != 0) {
                probe_reward(args, *blob, tt.valence);
            }
            step++;
        }
    }

    // measure frozen: spikes per (trial tick, neuron, cue)
    probe_set_learning(args, *blob, 0);
    std::vector<uint32_t> on((size_t)trial_ticks * neurons * 2, 0);
    int64_t trials[2] = {0, 0};
    int right = 0, asked = 0;
    for (int64_t step = 0; step < args.test_steps;) {
        ticker_before_tick(tt, *blob);
        int tick = tt.task.phase * tps + tt.tick_in_step;
        int cue = tt.task.cue & 1;
        bool resting = tt.resting;
        probe_tick(args, *blob);
        if (!resting && tick < trial_ticks) {
            const uint64_t* map = blob->spikes[blob->now];
            for (int t = 0; t < blob->tile_count; t++) {
                uint64_t s = map[t] & ~blob->sensor[t];
                while (s) {
                    int n = t * TILE_NEURONS + __builtin_ctzll(s);
                    s &= s - 1;
                    on[((size_t)tick * neurons + (size_t)n) * 2 + (size_t)cue]++;
                }
            }
            trials[cue] += tick == 0 ? 1 : 0;
        }
        if (ticker_after_tick(tt, *blob)) {
            if (tt.right_move >= 0) {
                asked++;
                right += tt.move == tt.right_move ? 1 : 0;
            }
            step++;
        }
    }

    // groups: every non-sensor neuron, and the motor neurons the answer is read from
    std::vector<uint8_t> all((size_t)neurons, 0), motor((size_t)neurons, 0);
    for (int n = 0; n < neurons; n++) {
        int t = n / TILE_NEURONS, b = n % TILE_NEURONS;
        all[(size_t)n] = !((blob->sensor[t] >> b) & 1);
        motor[(size_t)n] = (blob->motor[t] >> b) & 1;
    }
    std::printf("trace: %s, seed %llu, taught %lld steps, %lld + %lld trials, frozen score "
                "%.3f\n",
                probe_engine_name(args), (unsigned long long)f.seed, (long long)f.steps,
                (long long)trials[0], (long long)trials[1], asked ? (double)right / asked : 0.0);
    if (delayed) {
        std::printf("  cue ticks 0-%d, blank %d-%d, ask %d-%d\n", tps - 1, tps,
                    trial_ticks - tps - 1, trial_ticks - tps, trial_ticks - 1);
    }
    std::printf("  %-8s %8s %8s\n", "ticks", "sheet", "motor");
    const int bin = 4;
    int lost = -1; // first tick after the cue went off where the sheet no longer reads it
    for (int from = 0; from < trial_ticks; from += bin) {
        double s = 0, mo = 0;
        for (int tick = from; tick < from + bin && tick < trial_ticks; tick++) {
            double r = readable(on, trials, neurons, tick, all);
            s += r;
            mo += readable(on, trials, neurons, tick, motor);
            if (tick >= tps && lost < 0 && r < 0.3) {
                lost = tick;
            }
        }
        std::printf("  %3d-%-3d  %8.2f %8.2f\n", from, from + bin - 1, s / bin, mo / bin);
    }
    if (delayed) {
        std::printf("  the sheet reads the cue for %d ticks after it goes off (until under 0.3)\n",
                    lost < 0 ? trial_ticks - tps : lost - tps);
    }
    probe_free(args, blob);
    return 0;
}
