// blobprobe reader: level (a) of the function battery. Is the answer readable from the
// sheet as born? Runs a task through the plug with its own timing on a frozen sheet
// (learning off), keeps every non-sensor neuron's spike count over each answer step, and
// fits a plain linear reader (ridge least squares, one output per button, the largest
// wins). Beside it the same reader on the input lines themselves, every line at every step
// of the trial: what a single layer of threshold units on the raw input could do.
//
// Trials in time order: the first 60% of taught trials fit, the next 15% pick the ridge,
// the last 25% are the test. Trials on held-out values (--task number --fn-hold H) are
// never fitted on and are scored apart: filling in between taught values. One line out:
//
//   reader LABEL classes C trials N test T held H sheet A sheet_held A inputs A inputs_held A
//
// blobrun's task, plug, seed and --g. flags all work. Own flags: --trials N (answer steps
// kept, default 3000), --settle N (ticks before keeping any, default 20000), --label text.
// The kernel is built in blocks of neurons, so a 32x32-tile sheet (65,536 neurons) fits.

#include "blob/blob.hpp"
#include "run/run_flags.hpp"
#include "run/task_ticker.hpp"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

int int_arg(int argc, char** argv, const char* name, int fallback);
const char* text_arg(int argc, char** argv, const char* name, const char* fallback);

static const char* const reader_only[] = {"--trials", "--settle", "--label"};
constexpr int READER_RIDGES = 5;
static const double ridge_scale[READER_RIDGES] = {0.001, 0.01, 0.1, 1.0, 10.0};

struct ReaderRows {
    int n = 0, d = 0, classes = 2;
    std::vector<uint8_t> x; // n x d
    std::vector<int> label;
    std::vector<uint8_t> held;
    std::vector<int> fit, pick, test, held_rows;
};

// standardized by the fit rows; a feature that never varies there is dropped
static void standardize(const ReaderRows& r, std::vector<int>& keep, std::vector<float>& mean,
                        std::vector<float>& scale) {
    std::vector<double> sum((size_t)r.d, 0.0), sq((size_t)r.d, 0.0);
    for (int i : r.fit) {
        const uint8_t* x = &r.x[(size_t)i * r.d];
        for (int j = 0; j < r.d; j++) {
            sum[(size_t)j] += x[j];
            sq[(size_t)j] += (double)x[j] * x[j];
        }
    }
    double n = (double)r.fit.size();
    for (int j = 0; j < r.d; j++) {
        double m = sum[(size_t)j] / n, v = sq[(size_t)j] / n - m * m;
        if (v > 1e-9) {
            keep.push_back(j);
            mean.push_back((float)m);
            scale.push_back((float)(1.0 / std::sqrt(v)));
        }
    }
}

// the kernel K = Z Z^T over every row, built a block of features at a time
static std::vector<double> kernel(const ReaderRows& r, int& features) {
    std::vector<int> keep;
    std::vector<float> mean, scale;
    standardize(r, keep, mean, scale);
    features = (int)keep.size();
    int n = r.n;
    std::vector<double> k((size_t)n * n, 0.0);
    const int block = 512;
    std::vector<float> z;
    for (int f0 = 0; f0 < features; f0 += block) {
        int bw = features - f0 < block ? features - f0 : block;
        z.assign((size_t)n * bw, 0.0f);
        for (int i = 0; i < n; i++) {
            const uint8_t* x = &r.x[(size_t)i * r.d];
            for (int j = 0; j < bw; j++) {
                int f = f0 + j;
                z[(size_t)i * bw + j] =
                    ((float)x[keep[(size_t)f]] - mean[(size_t)f]) * scale[(size_t)f];
            }
        }
        for (int i = 0; i < n; i++) {
            const float* zi = &z[(size_t)i * bw];
            for (int j = 0; j <= i; j++) {
                const float* zj = &z[(size_t)j * bw];
                float s = 0.0f;
                for (int c = 0; c < bw; c++) {
                    s += zi[c] * zj[c];
                }
                k[(size_t)i * n + j] += s;
            }
        }
    }
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < i; j++) {
            k[(size_t)j * n + i] = k[(size_t)i * n + j];
        }
    }
    return k;
}

// solve (a + ridge I) x = b for `cols` right-hand sides by cholesky; a is m x m
static bool solve(const std::vector<double>& a, int m, double ridge, const std::vector<double>& b,
                  int cols, std::vector<double>& x) {
    std::vector<double> l(a);
    for (int i = 0; i < m; i++) {
        l[(size_t)i * m + i] += ridge;
    }
    for (int j = 0; j < m; j++) {
        double* lj = &l[(size_t)j * m];
        double diag = lj[j];
        for (int c = 0; c < j; c++) {
            diag -= lj[c] * lj[c];
        }
        if (diag <= 0) {
            return false;
        }
        diag = std::sqrt(diag);
        lj[j] = diag;
        for (int i = j + 1; i < m; i++) {
            double* li = &l[(size_t)i * m];
            double s = li[j];
            for (int c = 0; c < j; c++) {
                s -= li[c] * lj[c];
            }
            li[j] = s / diag;
        }
    }
    x.assign((size_t)m * cols, 0.0);
    std::vector<double> v((size_t)m);
    for (int c = 0; c < cols; c++) {
        for (int i = 0; i < m; i++) {
            double s = b[(size_t)i * cols + c];
            for (int k = 0; k < i; k++) {
                s -= l[(size_t)i * m + k] * v[(size_t)k];
            }
            v[(size_t)i] = s / l[(size_t)i * m + i];
        }
        for (int i = m - 1; i >= 0; i--) {
            double s = v[(size_t)i];
            for (int k = i + 1; k < m; k++) {
                s -= l[(size_t)k * m + i] * x[(size_t)k * cols + c];
            }
            x[(size_t)i * cols + c] = s / l[(size_t)i * m + i];
        }
    }
    return true;
}

// share right over `rows`, from the kernel against the fit rows and the dual weights
static double accuracy(const ReaderRows& r, const std::vector<double>& k,
                       const std::vector<double>& alpha, const std::vector<double>& y_mean,
                       const std::vector<int>& rows) {
    int c = r.classes, right = 0;
    std::vector<double> score((size_t)c);
    for (int row : rows) {
        for (int q = 0; q < c; q++) {
            score[(size_t)q] = y_mean[(size_t)q];
        }
        for (size_t i = 0; i < r.fit.size(); i++) {
            double kv = k[(size_t)row * r.n + r.fit[i]];
            for (int q = 0; q < c; q++) {
                score[(size_t)q] += kv * alpha[i * c + q];
            }
        }
        int best = 0;
        for (int q = 1; q < c; q++) {
            best = score[(size_t)q] > score[(size_t)best] ? q : best;
        }
        right += best == r.label[(size_t)row];
    }
    return rows.empty() ? -1.0 : (double)right / (double)rows.size();
}

// fit on the fit rows, pick the ridge on the pick rows; share right on test and held rows
static void fit_rows(const ReaderRows& r, double& test_acc, double& held_acc) {
    int features = 0;
    std::vector<double> k = kernel(r, features);
    int m = (int)r.fit.size(), c = r.classes;
    std::vector<double> kf((size_t)m * m), y((size_t)m * c, 0.0), y_mean((size_t)c, 0.0);
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < m; j++) {
            kf[(size_t)i * m + j] = k[(size_t)r.fit[(size_t)i] * r.n + r.fit[(size_t)j]];
        }
        y_mean[(size_t)r.label[(size_t)r.fit[(size_t)i]]] += 1.0 / m;
    }
    for (int i = 0; i < m; i++) {
        for (int q = 0; q < c; q++) {
            y[(size_t)i * c + q] =
                (r.label[(size_t)r.fit[(size_t)i]] == q ? 1.0 : 0.0) - y_mean[(size_t)q];
        }
    }
    double best = -1;
    test_acc = held_acc = -1;
    std::vector<double> alpha;
    for (int s = 0; s < READER_RIDGES; s++) {
        double ridge = ridge_scale[s] * (features > 0 ? features : 1);
        if (!solve(kf, m, ridge, y, c, alpha)) {
            continue;
        }
        double val = accuracy(r, k, alpha, y_mean, r.pick);
        if (val > best) {
            best = val;
            test_acc = accuracy(r, k, alpha, y_mean, r.test);
            held_acc = accuracy(r, k, alpha, y_mean, r.held_rows);
        }
    }
}

static void split(ReaderRows& r) {
    std::vector<int> taught;
    for (int i = 0; i < r.n; i++) {
        (r.held[(size_t)i] ? r.held_rows : taught).push_back(i);
    }
    int t = (int)taught.size();
    for (int i = 0; i < t; i++) {
        std::vector<int>& to = i < t * 60 / 100 ? r.fit : (i < t * 75 / 100 ? r.pick : r.test);
        to.push_back(taught[(size_t)i]);
    }
}

int probe_reader(int argc, char** argv) {
    std::vector<char*> rest = {argv[0]};
    for (int i = 2; i < argc; i++) {
        bool mine = false;
        for (const char* name : reader_only) {
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
    int trials = int_arg(argc, argv, "--trials", 3000);
    int64_t settle = int_arg(argc, argv, "--settle", 20000);
    const char* label = text_arg(argc, argv, "--label", "run");
    flags.genome.learning_on = 0;

    Blob* blob = blob_birth(flags.genome, flags.seed);
    static TaskTicker tt;
    tt = TaskTicker{};
    if (!ticker_start(tt, *blob, flags)) {
        blob_free(blob);
        return 1;
    }
    // the input lines of every step of a trial, good and bad left out
    int lines = tt.plug.sensor_lines - tt.plug.first_single;
    std::vector<uint8_t> shown((size_t)TASK_MAX_TRIAL * lines, 0);
    int input_d = TASK_MAX_TRIAL * lines;
    ReaderRows sheet, inputs;
    sheet.d = blob->neuron_count;
    inputs.d = input_d;
    sheet.classes = inputs.classes = tt.plug.motor_lines;
    std::vector<uint8_t> count((size_t)blob->neuron_count, 0);
    int64_t ticks = 0;
    bool held = false;
    int kept = 0;
    while (kept < trials) {
        if (tt.tick_in_step == 0 && tt.rest_left == 0) {
            std::memset(count.data(), 0, count.size());
            int phase = tt.task.phase < TASK_MAX_TRIAL ? tt.task.phase : 0;
            if (phase == 0) {
                std::memset(shown.data(), 0, shown.size());
            }
            held = tt.task.settings.fn.hold > 0 && tt.task.scored;
            ticker_before_tick(tt, *blob);
            for (int l = 0; l < lines; l++) {
                shown[(size_t)phase * lines + l] = tt.plug.line_on[tt.plug.first_single + l];
            }
        } else {
            ticker_before_tick(tt, *blob);
        }
        bool resting = tt.resting;
        blob_tick(*blob);
        ticks++;
        for (int t = 0; !resting && t < blob->tile_count; t++) {
            uint64_t spikes = blob->spikes[blob->now][t] & ~blob->sensor[t];
            while (spikes) {
                uint8_t& c = count[(size_t)(t * TILE_NEURONS + __builtin_ctzll(spikes))];
                c = c < 255 ? c + 1 : c;
                spikes &= spikes - 1;
            }
        }
        if (!ticker_after_tick(tt, *blob)) {
            continue;
        }
        if (tt.valence != 0) {
            blob_reward(*blob, tt.valence);
        }
        if (ticks < settle || tt.right_move < 0) {
            continue;
        }
        sheet.x.insert(sheet.x.end(), count.begin(), count.end());
        inputs.x.insert(inputs.x.end(), shown.begin(), shown.end());
        for (ReaderRows* r : {&sheet, &inputs}) {
            r->label.push_back(tt.right_move);
            r->held.push_back(held);
            r->n++;
        }
        kept++;
    }
    blob_free(blob);
    split(sheet);
    split(inputs);
    double sheet_test = 0, sheet_held = 0, input_test = 0, input_held = 0;
    fit_rows(sheet, sheet_test, sheet_held);
    fit_rows(inputs, input_test, input_held);
    std::printf("reader %s classes %d trials %d test %d held %d sheet %.4f sheet_held %.4f inputs "
                "%.4f inputs_held %.4f\n",
                label, sheet.classes, sheet.n, (int)sheet.test.size(), (int)sheet.held_rows.size(),
                sheet_test, sheet_held, input_test, input_held);
    return 0;
}
