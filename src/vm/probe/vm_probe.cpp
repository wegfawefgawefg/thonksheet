// vmprobe: look inside the vm engine on a task. One life learns for --steps steps (learning
// on throughout, as in blobrun), then --test-steps more are recorded tick by tick: which
// entries fire, the M bits at the end of each step, and where each trial is. Printed:
//  - the score over the recorded steps, how many distinct entries fired
//  - per position in a trial: how alike the winners and M are between trials with the same
//    answer and with different answers (Jaccard, 1 = identical)
//  - how the blank part of a trial is held: a fixed point (one winner set repeated), a
//    cycle (sets repeat with a short period) or a chain (sets keep changing)
//  - --show N trials as walks through the bank
//
//   vmprobe --task delayed-echo --delay 2 --steps 20000 --test-steps 2000 --show 4
//   (blobrun's task and plug flags, --v.name value for vm numbers)

#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <bit>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

struct StepRecord {
    int phase = 0, cue = 0, right = -1, move = -1, valence = 0;
    int first_tick = 0;      // index into the tick records
    std::vector<uint64_t> m; // M at the step's last tick
};

struct Recording {
    int K = 0;
    std::vector<int32_t> winners; // K per tick, sorted
    std::vector<StepRecord> steps;
};

static double jaccard_sets(const int32_t* a, const int32_t* b, int K) {
    int i = 0, j = 0, both = 0;
    while (i < K && j < K) {
        if (a[i] == b[j]) {
            both++, i++, j++;
        } else if (a[i] < b[j]) {
            i++;
        } else {
            j++;
        }
    }
    return (double)both / (2 * K - both);
}

static double jaccard_bits(const std::vector<uint64_t>& a, const std::vector<uint64_t>& b) {
    int both = 0, any = 0;
    for (size_t w = 0; w < a.size(); w++) {
        both += std::popcount(a[w] & b[w]);
        any += std::popcount(a[w] | b[w]);
    }
    return any ? (double)both / any : 1.0;
}

// trials: runs of steps from phase 0 to the step before the next phase 0
static std::vector<std::pair<int, int>> trials_of(const Recording& r) {
    std::vector<std::pair<int, int>> out;
    int start = -1;
    for (int s = 0; s < (int)r.steps.size(); s++) {
        if (r.steps[s].phase == 0) {
            if (start >= 0) {
                out.push_back({start, s});
            }
            start = s;
        }
    }
    return out;
}

static void alike_by_position(const Recording& r) {
    auto trials = trials_of(r);
    if (trials.size() < 4) {
        std::printf("too few trials to compare\n");
        return;
    }
    int len = trials[0].second - trials[0].first;
    int n = std::min<int>((int)trials.size(), 120);
    std::printf("\nhow alike trials are, by step in the trial (winners at the step's last tick; "
                "M then)\n  step   winners same/diff   M same/diff\n");
    for (int p = 0; p < len; p++) {
        double ws = 0, wd = 0, ms = 0, md = 0;
        int ns = 0, nd = 0;
        for (int a = 0; a < n; a++) {
            for (int b = a + 1; b < n; b++) {
                if (trials[a].second - trials[a].first != len ||
                    trials[b].second - trials[b].first != len) {
                    continue;
                }
                const StepRecord& sa = r.steps[trials[a].first + p];
                const StepRecord& sb = r.steps[trials[b].first + p];
                const StepRecord& ea = r.steps[trials[a].second - 1];
                const StepRecord& eb = r.steps[trials[b].second - 1];
                bool same = ea.cue == eb.cue;
                // the step's last tick is the tick before the next step's first
                int ta = r.steps[trials[a].first + p + 1].first_tick - 1;
                int tb = r.steps[trials[b].first + p + 1].first_tick - 1;
                double wj =
                    jaccard_sets(&r.winners[(size_t)ta * r.K], &r.winners[(size_t)tb * r.K], r.K);
                double mj = jaccard_bits(sa.m, sb.m);
                if (same) {
                    ws += wj, ms += mj, ns++;
                } else {
                    wd += wj, md += mj, nd++;
                }
            }
        }
        std::printf("  %4d   %6.3f / %6.3f     %6.3f / %6.3f\n", p, ns ? ws / ns : 0,
                    nd ? wd / nd : 0, ns ? ms / ns : 0, nd ? md / nd : 0);
    }
}

// within each trial's blank ticks (after the first step, before the last): distinct winner
// sets, and whether the last 20 ticks are one set, a short cycle, or a chain
static void hold_kind(const Recording& r) {
    auto trials = trials_of(r);
    int fixed = 0, cycle = 0, chain = 0, none = 0;
    double distinct_sum = 0;
    int cycle_period_sum = 0;
    for (auto [first, end] : trials) {
        if (end - first < 3) {
            none++;
            continue;
        }
        int t0 = r.steps[first + 1].first_tick;
        int t1 = r.steps[end - 1].first_tick; // the ask step's first tick
        int n = t1 - t0;
        std::vector<int> label(n);
        std::vector<int> seen; // first tick of each distinct set
        for (int t = 0; t < n; t++) {
            int found = -1;
            for (int s = 0; s < (int)seen.size(); s++) {
                if (std::memcmp(&r.winners[(size_t)(t0 + t) * r.K],
                                &r.winners[(size_t)(t0 + seen[s]) * r.K],
                                sizeof(int32_t) * r.K) == 0) {
                    found = s;
                    break;
                }
            }
            if (found < 0) {
                found = (int)seen.size();
                seen.push_back(t);
            }
            label[t] = found;
        }
        distinct_sum += (double)seen.size();
        int tail = std::min(n, 20);
        int period = 0;
        for (int p = 1; p <= tail / 2 && !period; p++) {
            bool ok = true;
            for (int t = n - tail + p; t < n && ok; t++) {
                ok = label[t] == label[t - p];
            }
            period = ok ? p : 0;
        }
        if (period == 1) {
            fixed++;
        } else if (period > 1) {
            cycle++;
            cycle_period_sum += period;
        } else {
            chain++;
        }
    }
    int judged = fixed + cycle + chain;
    if (!judged) {
        std::printf("\nno blank steps in these trials\n");
        return;
    }
    std::printf("\nblank part of %d trials: distinct winner sets %.1f per trial; last 20 ticks: "
                "fixed point %d, cycle %d (mean period %.1f), chain %d\n",
                judged, distinct_sum / judged, fixed, cycle,
                cycle ? (double)cycle_period_sum / cycle : 0.0, chain);
}

static void print_set(const Recording& r, int tick, const char* label) {
    std::printf("    %s", label);
    for (int k = 0; k < r.K; k++) {
        std::printf(" %d", r.winners[(size_t)tick * r.K + k]);
    }
    std::printf("\n");
}

static void show_trials(const Recording& r, int count) {
    auto trials = trials_of(r);
    for (int i = 0; i < count && i < (int)trials.size(); i++) {
        auto [first, end] = trials[i];
        const StepRecord& ask = r.steps[end - 1];
        std::printf("\ntrial %d: answer %d, pressed %d, %s\n", i, ask.right, ask.move,
                    ask.valence > 0 ? "good" : (ask.valence < 0 ? "bad" : "no outcome"));
        for (int s = first; s < end; s++) {
            int t0 = r.steps[s].first_tick;
            int t1 = r.steps[s + 1].first_tick - 1;
            std::printf("  step %d (phase %d)\n", s - first, r.steps[s].phase);
            print_set(r, t0, "first tick:");
            if (t1 - t0 > 2) {
                print_set(r, t0 + 1, "second:    ");
                print_set(r, t0 + 2, "third:     ");
            }
            print_set(r, t1, "last tick: ");
        }
    }
}

int main(int argc, char** argv) {
    int64_t test_steps = 2000;
    int show = 3;
    std::vector<char*> rest = {argv[0]};
    for (int i = 1; i < argc; i++) {
        if (i + 1 < argc && std::strcmp(argv[i], "--test-steps") == 0) {
            test_steps = std::atoll(argv[++i]);
        } else if (i + 1 < argc && std::strcmp(argv[i], "--show") == 0) {
            show = std::atoi(argv[++i]);
        } else {
            rest.push_back(argv[i]);
        }
    }
    RunFlags flags;
    if (!run_flags_parse((int)rest.size(), rest.data(), flags)) {
        return 1;
    }
    VmGenome vg;
    apply_vm_args(argc, argv, vg);
    Blob* blob = vm_birth(flags.genome, vg, flags.seed);
    VmBlob& v = vm_of(*blob);
    TaskTicker tt;
    if (!ticker_start(tt, *blob, flags)) {
        return 1;
    }
    Recording rec;
    rec.K = v.vg.active;
    int64_t right = 0, asked = 0;
    int64_t total = flags.steps + test_steps;
    int64_t step = 0;
    std::vector<uint8_t> used((size_t)v.vg.entries, 0);
    bool step_open = false;
    while (step < total) {
        bool recording = step >= flags.steps;
        if (recording && !step_open && tt.tick_in_step == 0 && tt.rest_left == 0) {
            StepRecord sr;
            sr.phase = tt.task.phase;
            sr.first_tick = (int)(rec.winners.size() / rec.K);
            rec.steps.push_back(sr);
            step_open = true;
        }
        ticker_before_tick(tt, *blob);
        vm_tick(*blob);
        if (recording) {
            std::vector<int32_t> w(v.winners, v.winners + rec.K);
            std::sort(w.begin(), w.end());
            rec.winners.insert(rec.winners.end(), w.begin(), w.end());
            for (int32_t e : w) {
                used[(size_t)e] = 1;
            }
        }
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        if (recording && step_open) {
            StepRecord& sr = rec.steps.back();
            sr.cue = tt.cue;
            sr.right = tt.right_move;
            sr.move = tt.move;
            sr.valence = tt.valence;
            sr.m.assign(vm_m(v), vm_m(v) + v.m_words);
            step_open = false;
            if (tt.right_move >= 0) {
                asked++;
                right += tt.move == tt.right_move;
            }
        }
        if (tt.valence != 0) {
            plug_mark_acted(tt.plug, *blob, tt.move);
            vm_reward(*blob, tt.valence);
        }
        step++;
    }
    // a closing record so every step has a successor
    StepRecord end;
    end.first_tick = (int)(rec.winners.size() / rec.K);
    end.phase = 0;
    rec.steps.push_back(end);

    int distinct = 0;
    for (uint8_t u : used) {
        distinct += u;
    }
    std::printf("vmprobe %s: %lld learning steps, %lld recorded; right %.3f of %lld asks\n",
                task_name(flags.task), (long long)flags.steps, (long long)test_steps,
                asked ? (double)right / asked : 0.0, (long long)asked);
    std::printf("entries: %d in the bank, %d fired while recording; writes so far %llu; M on "
                "%u of %d\n",
                v.vg.entries, distinct, (unsigned long long)v.stats.writes, v.stats.m_on,
                v.free_cells);
    alike_by_position(rec);
    hold_kind(rec);
    show_trials(rec, show);
    vm_free(blob);
    return 0;
}
