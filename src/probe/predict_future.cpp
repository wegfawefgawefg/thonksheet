#include "probe/predict_future.hpp"

#include "probe/linear_fit.hpp"

#include <cmath>

// blobprobe predict --future (predict_future.hpp): one ridge reader per line, all solved at
// once over the same kernel. rows in time order: the first three quarters train (of which
// the last fifth picks the ridge strength), the last quarter is the test

static double dot(const double* a, const double* b, int n) {
    double sum = 0;
    for (int i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

FutureScore future_fit(const std::vector<uint8_t>& x, int d, const std::vector<uint8_t>& lines,
                       int count, int rows, int h) {
    FutureScore out;
    int n = rows - h;
    int train = n * 3 / 4, fit = train * 4 / 5;
    if (fit < 10 || count < 1) {
        return out;
    }
    // standardize by the fit rows; a feature that never varies is left out
    std::vector<double> mean((size_t)d, 0.0), scale((size_t)d, 0.0);
    for (int r = 0; r < fit; r++) {
        for (int j = 0; j < d; j++) {
            double v = x[(size_t)r * d + j];
            mean[(size_t)j] += v;
            scale[(size_t)j] += v * v;
        }
    }
    for (int j = 0; j < d; j++) {
        double m = mean[(size_t)j] / fit, var = scale[(size_t)j] / fit - m * m;
        mean[(size_t)j] = m;
        scale[(size_t)j] = var > 1e-9 ? 1.0 / std::sqrt(var) : 0.0;
    }
    std::vector<double> z((size_t)n * d);
    for (int r = 0; r < n; r++) {
        for (int j = 0; j < d; j++) {
            z[(size_t)r * d + j] = (x[(size_t)r * d + j] - mean[(size_t)j]) * scale[(size_t)j];
        }
    }
    // targets: each line h steps on, less its fit mean
    std::vector<double> y_mean((size_t)count, 0.0), y((size_t)fit * count);
    for (int r = 0; r < fit; r++) {
        for (int l = 0; l < count; l++) {
            y_mean[(size_t)l] += lines[(size_t)(r + h) * count + l] / (double)fit;
        }
    }
    for (int r = 0; r < fit; r++) {
        for (int l = 0; l < count; l++) {
            y[(size_t)r * count + l] = lines[(size_t)(r + h) * count + l] - y_mean[(size_t)l];
        }
    }
    std::vector<double> g((size_t)fit * fit);
    for (int i = 0; i < fit; i++) {
        for (int j = 0; j <= i; j++) {
            double v = dot(&z[(size_t)i * d], &z[(size_t)j * d], d);
            g[(size_t)i * fit + j] = g[(size_t)j * fit + i] = v;
        }
    }
    // share of line-steps right over rows [from, to) with weights w (d x count)
    // idea 183, beside it: the mean of the shares right on on- and on off-line-steps
    // (balanced: lines are mostly off), and the share right on line-steps whose value
    // differs from h steps before (changed: what "same as now" always gets wrong)
    double balanced = 0, changed = 0;
    auto share = [&](const std::vector<double>& w, int from, int to) {
        int64_t right = 0, on = 0, on_right = 0, moved = 0, moved_right = 0;
        std::vector<double> p((size_t)count);
        for (int r = from; r < to; r++) {
            for (int l = 0; l < count; l++) {
                p[(size_t)l] = y_mean[(size_t)l];
            }
            const double* zr = &z[(size_t)r * d];
            for (int j = 0; j < d; j++) {
                for (int l = 0; l < count && zr[j] != 0; l++) {
                    p[(size_t)l] += zr[j] * w[(size_t)j * count + l];
                }
            }
            for (int l = 0; l < count; l++) {
                bool later = lines[(size_t)(r + h) * count + l] != 0;
                bool ok = (p[(size_t)l] > 0.5) == later;
                bool differs = later != (lines[(size_t)r * count + l] != 0);
                right += ok;
                on += later;
                on_right += later && ok;
                moved += differs;
                moved_right += differs && ok;
            }
        }
        int64_t total = (int64_t)(to - from) * count, off = total - on;
        balanced = ((on ? (double)on_right / (double)on : 0.0) +
                    (off ? (double)(right - on_right) / (double)off : 0.0)) /
                   2;
        changed = moved ? (double)moved_right / (double)moved : 0.0;
        return to > from ? (double)right / (double)total : 0.0;
    };
    static const double scales[6] = {0.0003, 0.003, 0.03, 0.3, 3.0, 30.0};
    double best_val = -1;
    std::vector<double> alpha, w((size_t)d * count);
    for (double s : scales) {
        if (!linear_ridge_solve(g, fit, s * d, y.data(), count, alpha)) {
            continue;
        }
        for (int j = 0; j < d; j++) {
            for (int l = 0; l < count; l++) {
                double sum = 0;
                for (int r = 0; r < fit; r++) {
                    sum += z[(size_t)r * d + j] * alpha[(size_t)r * count + l];
                }
                w[(size_t)j * count + l] = sum;
            }
        }
        double val = share(w, fit, train);
        if (val > best_val) {
            best_val = val;
            out.right = share(w, train, n);
            out.balanced = balanced;
            out.changed = changed;
        }
    }
    // baselines on the same test rows: the line h steps before, the line's more common value
    int64_t same = 0, common = 0;
    for (int l = 0; l < count; l++) {
        bool on = y_mean[(size_t)l] > 0.5;
        for (int r = train; r < n; r++) {
            uint8_t later = lines[(size_t)(r + h) * count + l] != 0;
            same += later == (lines[(size_t)r * count + l] != 0);
            common += later == on;
        }
    }
    double cells = (double)(n - train) * count;
    out.same = same / cells;
    out.common = common / cells;
    return out;
}
