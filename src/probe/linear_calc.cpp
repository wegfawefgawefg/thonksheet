// blobprobe calc: the reference reader for the calculation battery (ideas 133, 134). Runs a
// calc task through the plug on the frozen sheet (learning off), with the task's own
// timing, and keeps every non-sensor neuron's spike count over each ask step, the answer's
// bits, the trial's input lines and whether its case is held out. Then a ridge reader per
// output bit (on if its score is over one half) is fitted on seen cases only and scored on
// later trials of seen cases and on every held-out trial: bits right and whole answers
// right. Two readers: the whole sheet, and the input lines alone (every step's lines at
// once), which says whether a bit is a plain weighted sum of the inputs at all. For trials
// of more than one showing step (programs) a third reader asks the sheet, at the ask step,
// for the lines of the step before it: is the last instruction still there.
// blobrun's task, plug, seed and --g. flags all work. Own flags: --trials N (ask steps
// kept, default 3000), --settle N (ticks before keeping any, default 30000), --learning
// 0|1, --label text, --out csv (reader,key,value lines).

#include "blob/blob.hpp"
#include "probe/linear_fit.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

static const char* const calc_only[] = {"--trials", "--settle", "--learning", "--label",
                                        "--out"};

struct CalcRows {
    int d = 0;                 // features per row
    std::vector<double> x;     // rows x d
    std::vector<uint8_t> held;
};

struct CalcScore {
    double bits[2] = {}, whole[2] = {}; // [0] seen (later trials), [1] held out
    std::vector<double> bit[2];
    int n[2] = {};
};

static double dot(const double* a, const double* b, int n) {
    double s = 0;
    for (int i = 0; i < n; i++) {
        s += a[i] * b[i];
    }
    return s;
}

// ridge per output bit through the trials' kernel (dual form), fitted on seen rows
// [0, fit), the ridge picked on seen rows [fit, train), scored on seen rows from `train`
// and on every held row
static CalcScore fit_bits(const CalcRows& rows, const std::vector<uint64_t>& answer,
                          int out_bits) {
    std::vector<int> seen, held;
    for (size_t r = 0; r < rows.held.size(); r++) {
        (rows.held[r] ? held : seen).push_back((int)r);
    }
    int train = (int)seen.size() * 3 / 4, fit = train * 4 / 5, d = rows.d;
    // standardize by the fit rows
    std::vector<double> mean((size_t)d, 0), scale((size_t)d, 0);
    for (int i = 0; i < fit; i++) {
        const double* x = &rows.x[(size_t)seen[(size_t)i] * d];
        for (int j = 0; j < d; j++) {
            mean[(size_t)j] += x[j];
            scale[(size_t)j] += x[j] * x[j];
        }
    }
    for (int j = 0; j < d; j++) {
        double m = mean[(size_t)j] / fit, v = scale[(size_t)j] / fit - m * m;
        mean[(size_t)j] = m;
        scale[(size_t)j] = v > 1e-9 ? 1.0 / std::sqrt(v) : 0.0;
    }
    int n = (int)rows.held.size();
    std::vector<double> z((size_t)n * d);
    for (int r = 0; r < n; r++) {
        for (int j = 0; j < d; j++) {
            z[(size_t)r * d + j] = (rows.x[(size_t)r * d + j] - mean[(size_t)j]) * scale[(size_t)j];
        }
    }
    // kernel of every row with the fit rows
    std::vector<double> k((size_t)n * fit);
    for (int r = 0; r < n; r++) {
        for (int i = 0; i < fit; i++) {
            k[(size_t)r * fit + i] = dot(&z[(size_t)r * d], &z[(size_t)seen[(size_t)i] * d], d);
        }
    }
    std::vector<double> gram((size_t)fit * fit), y((size_t)fit * out_bits), y_mean(out_bits, 0);
    for (int i = 0; i < fit; i++) {
        for (int j = 0; j < fit; j++) {
            gram[(size_t)i * fit + j] = k[(size_t)seen[(size_t)i] * fit + j];
        }
        for (int b = 0; b < out_bits; b++) {
            y_mean[(size_t)b] += (double)((answer[(size_t)seen[(size_t)i]] >> b) & 1) / fit;
        }
    }
    for (int i = 0; i < fit; i++) {
        for (int b = 0; b < out_bits; b++) {
            double v = (double)((answer[(size_t)seen[(size_t)i]] >> b) & 1);
            y[(size_t)i * out_bits + b] = v - y_mean[(size_t)b];
        }
    }
    // score rows with weights alpha: per bit right, whole right
    auto score = [&](const std::vector<double>& alpha, const std::vector<int>& which, int from,
                     std::vector<double>& per_bit, double& whole) {
        per_bit.assign((size_t)out_bits, 0.0);
        whole = 0;
        int count = 0;
        for (size_t w = (size_t)from; w < which.size(); w++) {
            int r = which[w];
            bool all = true;
            for (int b = 0; b < out_bits; b++) {
                double s = y_mean[(size_t)b];
                for (int i = 0; i < fit; i++) {
                    s += k[(size_t)r * fit + i] * alpha[(size_t)i * out_bits + b];
                }
                bool right = (s > 0.5) == (bool)((answer[(size_t)r] >> b) & 1);
                per_bit[(size_t)b] += right;
                all = all && right;
            }
            whole += all;
            count++;
        }
        for (double& v : per_bit) {
            v = count > 0 ? v / count : 0.0;
        }
        whole = count > 0 ? whole / count : 0.0;
        return count;
    };
    static const double ridges[] = {0.0003, 0.003, 0.03, 0.3, 3.0, 30.0};
    CalcScore out;
    double best = -1;
    std::vector<double> alpha;
    for (double ridge : ridges) {
        if (!ridge_solve(gram, fit, ridge * d, y.data(), out_bits, alpha)) {
            continue;
        }
        std::vector<int> val(seen.begin(), seen.begin() + train);
        std::vector<double> per_bit;
        double whole = 0;
        score(alpha, val, fit, per_bit, whole);
        double mean_bit = 0;
        for (double v : per_bit) {
            mean_bit += v / out_bits;
        }
        if (mean_bit <= best) {
            continue;
        }
        best = mean_bit;
        out.n[0] = score(alpha, seen, train, out.bit[0], out.whole[0]);
        out.n[1] = score(alpha, held, 0, out.bit[1], out.whole[1]);
        for (int h = 0; h < 2; h++) {
            out.bits[h] = 0;
            for (double v : out.bit[h]) {
                out.bits[h] += v / out_bits;
            }
        }
    }
    return out;
}

static void report(FILE* csv, const char* label, const char* reader, const CalcScore& s,
                   int out_bits) {
    std::printf("%-20s %-12s bits seen %.3f held %.3f   whole seen %.3f held %.3f   (%d, %d "
                "trials)\n",
                label, reader, s.bits[0], s.bits[1], s.whole[0], s.whole[1], s.n[0], s.n[1]);
    const char* names[2] = {"seen", "held"};
    for (int h = 0; h < 2; h++) {
        std::printf("%-20s %-12s per bit %s:", "", "", names[h]);
        for (int b = 0; b < out_bits; b++) {
            std::printf(" %.2f", b < (int)s.bit[h].size() ? s.bit[h][(size_t)b] : 0.0);
        }
        std::printf("\n");
    }
    if (!csv) {
        return;
    }
    for (int h = 0; h < 2; h++) {
        std::fprintf(csv, "%s,bits_%s,%.6g\n", reader, names[h], s.bits[h]);
        std::fprintf(csv, "%s,whole_%s,%.6g\n", reader, names[h], s.whole[h]);
        for (int b = 0; b < out_bits && b < (int)s.bit[h].size(); b++) {
            std::fprintf(csv, "%s,bit%d_%s,%.6g\n", reader, b, names[h], s.bit[h][(size_t)b]);
        }
    }
}

int probe_calc(int argc, char** argv) {
    std::vector<char*> rest = {argv[0]};
    for (int i = 2; i < argc; i++) {
        bool mine = false;
        for (const char* name : calc_only) {
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
    if (flags.task != TaskKind::calc) {
        std::fprintf(stderr, "blobprobe calc: needs --task calc\n");
        return 1;
    }
    int trials = int_arg(argc, argv, "--trials", 3000);
    int64_t settle = int_arg(argc, argv, "--settle", 30000);
    const char* label = text_arg(argc, argv, "--label", "run");
    std::string out_path = text_arg(argc, argv, "--out", "");
    if (!int_arg(argc, argv, "--learning", 0)) {
        flags.genome.learning_on = 0;
    }

    Blob* blob = blob_birth(flags.genome, flags.seed);
    static TaskTicker tt;
    tt = TaskTicker{};
    if (!ticker_start(tt, *blob, flags)) {
        blob_free(blob);
        return 1;
    }
    int out_bits = tt.task.calc.out_bits, lines = tt.task.calc.lines;
    int steps = tt.task.calc.steps;
    CalcRows sheet, inputs;
    // the answer, and the lines of the step before the ask (multi-step trials: can the
    // sheet still read the last instruction when it is asked)
    std::vector<uint64_t> answers, before;
    uint64_t prev_lines = 0;
    sheet.d = blob->neuron_count;
    inputs.d = steps * lines;
    std::vector<double> count((size_t)blob->neuron_count, 0.0);
    std::vector<double> shown((size_t)inputs.d, 0.0);
    uint64_t answer = 0;
    bool asking = false, held = false;
    int64_t ticks = 0;
    int kept = 0;
    while (kept < trials) {
        // a step's first tick: is it the ask step, and what does the trial show
        if (tt.tick_in_step == 0 && tt.rest_left == 0) {
            std::fill(count.begin(), count.end(), 0.0);
            const CalcState& c = tt.task.calc;
            asking = tt.task.phase == c.steps - 1;
            answer = c.answer;
            held = c.held;
            prev_lines = steps > 2 ? c.step_lines[steps - 2] >> 1 : 0;
            for (int s = 0; s < steps; s++) {
                for (int l = 0; l < lines; l++) {
                    shown[(size_t)(s * lines + l)] = (double)((c.step_lines[s] >> l) & 1);
                }
            }
        }
        bool resting = tt.rest_left > 0;
        ticker_before_tick(tt, *blob);
        blob_tick(*blob);
        ticks++;
        for (int t = 0; !resting && t < blob->tile_count; t++) {
            uint64_t spikes = blob->spikes[blob->now][t] & ~blob->sensor[t];
            while (spikes) {
                count[(size_t)(t * TILE_NEURONS + __builtin_ctzll(spikes))] += 1.0;
                spikes &= spikes - 1;
            }
        }
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        if (tt.valence != 0) {
            blob_reward(*blob, tt.valence);
        }
        if (ticks < settle || !asking) {
            continue;
        }
        sheet.x.insert(sheet.x.end(), count.begin(), count.end());
        inputs.x.insert(inputs.x.end(), shown.begin(), shown.end());
        for (CalcRows* r : {&sheet, &inputs}) {
            r->held.push_back(held);
        }
        answers.push_back(answer);
        before.push_back(prev_lines);
        kept++;
    }
    blob_free(blob);

    FILE* csv = out_path.empty() ? nullptr : std::fopen(out_path.c_str(), "w");
    if (csv) {
        std::fprintf(csv, "reader,key,value\n");
    }
    report(csv, label, "inputs", fit_bits(inputs, answers, out_bits), out_bits);
    report(csv, label, "sheet", fit_bits(sheet, answers, out_bits), out_bits);
    if (steps > 2) {
        report(csv, label, "sheet_prev", fit_bits(sheet, before, lines - 1), lines - 1);
    }
    if (csv) {
        std::fclose(csv);
    }
    std::fflush(stdout);
    return 0;
}
