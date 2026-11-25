#include "probe/linear_fit.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

// ridge least squares on standardized spike counts. trials are split in time order: the
// first three quarters train (of which the last fifth picks the ridge strength), the last
// quarter is the test. a tile or a 3x3 block is fitted from the feature-by-feature products
// (primal); the whole sheet, with more neurons than trials, from the trial-by-trial ones
// (dual).

constexpr int RIDGE_STEPS = 6;
static const double ridge_scale[RIDGE_STEPS] = {0.0003, 0.003, 0.03, 0.3, 3.0, 30.0};

struct Standard {
    int n = 0, d = 0;
    int fit = 0, train = 0; // rows [0, fit) fit, [fit, train) pick the ridge, [train, n) test
    std::vector<double> z;  // n x d, standardized by the fit rows
    std::vector<double> zt; // d x fit: each neuron's fit column, contiguous
    std::vector<double> y;  // fit x classes, one-hot less its mean
    std::vector<double> y_mean;
};

static void standardize(const LinearData& data, Standard& s) {
    s.n = (int)data.label.size();
    s.d = data.neurons;
    s.train = s.n * 3 / 4;
    s.fit = s.train * 4 / 5;
    int c = data.classes;
    std::vector<double> mean((size_t)s.d, 0.0), sd((size_t)s.d, 0.0);
    for (int r = 0; r < s.fit; r++) {
        const uint8_t* x = &data.counts[(size_t)r * s.d];
        for (int j = 0; j < s.d; j++) {
            mean[(size_t)j] += x[j];
            sd[(size_t)j] += (double)x[j] * x[j];
        }
    }
    for (int j = 0; j < s.d; j++) {
        double m = mean[(size_t)j] / s.fit;
        double v = sd[(size_t)j] / s.fit - m * m;
        mean[(size_t)j] = m;
        // a neuron that never varies is left out (all zeros)
        sd[(size_t)j] = v > 1e-9 ? 1.0 / std::sqrt(v) : 0.0;
    }
    s.z.assign((size_t)s.n * s.d, 0.0);
    s.zt.assign((size_t)s.d * s.fit, 0.0);
    for (int r = 0; r < s.n; r++) {
        const uint8_t* x = &data.counts[(size_t)r * s.d];
        double* z = &s.z[(size_t)r * s.d];
        for (int j = 0; j < s.d; j++) {
            z[j] = (x[j] - mean[(size_t)j]) * sd[(size_t)j];
            if (r < s.fit) {
                s.zt[(size_t)j * s.fit + r] = z[j];
            }
        }
    }
    s.y_mean.assign((size_t)c, 0.0);
    for (int r = 0; r < s.fit; r++) {
        s.y_mean[(size_t)data.label[(size_t)r]] += 1.0 / s.fit;
    }
    s.y.assign((size_t)s.fit * c, 0.0);
    for (int r = 0; r < s.fit; r++) {
        for (int k = 0; k < c; k++) {
            s.y[(size_t)r * c + k] = (data.label[(size_t)r] == k ? 1.0 : 0.0) - s.y_mean[(size_t)k];
        }
    }
}

static double dot(const double* a, const double* b, int n) {
    double sum = 0;
    for (int i = 0; i < n; i++) {
        sum += a[i] * b[i];
    }
    return sum;
}

// solve (a + ridge I) x = b in place for `cols` right-hand sides; a is m x m, kept intact
static bool ridge_solve(const std::vector<double>& a, int m, double ridge, const double* b,
                        int cols, std::vector<double>& x) {
    std::vector<double> l(a);
    for (int i = 0; i < m; i++) {
        l[(size_t)i * m + i] += ridge;
    }
    // cholesky, lower triangle
    for (int j = 0; j < m; j++) {
        double* lj = &l[(size_t)j * m];
        double diag = lj[j] - dot(lj, lj, j);
        if (diag <= 0) {
            return false;
        }
        diag = std::sqrt(diag);
        lj[j] = diag;
        for (int i = j + 1; i < m; i++) {
            double* li = &l[(size_t)i * m];
            li[j] = (li[j] - dot(li, lj, j)) / diag;
        }
    }
    x.assign((size_t)m * cols, 0.0);
    std::vector<double> v((size_t)m);
    for (int c = 0; c < cols; c++) {
        for (int i = 0; i < m; i++) {
            const double* li = &l[(size_t)i * m];
            double sum = b[(size_t)i * cols + c];
            for (int k = 0; k < i; k++) {
                sum -= li[k] * v[(size_t)k];
            }
            v[(size_t)i] = sum / li[i];
        }
        for (int i = m - 1; i >= 0; i--) {
            double sum = v[(size_t)i];
            for (int k = i + 1; k < m; k++) {
                sum -= l[(size_t)k * m + i] * x[(size_t)k * cols + c];
            }
            x[(size_t)i * cols + c] = sum / l[(size_t)i * m + i];
        }
    }
    return true;
}

bool linear_ridge_solve(const std::vector<double>& a, int m, double ridge, const double* b,
                        int cols, std::vector<double>& x) {
    return ridge_solve(a, m, ridge, b, cols, x);
}

// share right over rows [from, to), with weights w (features x classes) on `features`
static double accuracy(const LinearData& data, const Standard& s, const std::vector<int>& features,
                       const std::vector<double>& w, int from, int to) {
    int c = data.classes;
    int right = 0;
    std::vector<double> score((size_t)c);
    for (int r = from; r < to; r++) {
        const double* z = &s.z[(size_t)r * s.d];
        for (int k = 0; k < c; k++) {
            score[(size_t)k] = s.y_mean[(size_t)k];
        }
        for (size_t f = 0; f < features.size(); f++) {
            double v = z[features[f]];
            for (int k = 0; k < c; k++) {
                score[(size_t)k] += v * w[f * c + k];
            }
        }
        int best = 0;
        for (int k = 1; k < c; k++) {
            best = score[(size_t)k] > score[(size_t)best] ? k : best;
        }
        right += best == data.label[(size_t)r];
    }
    return to > from ? (double)right / (to - from) : 0.0;
}

// a reader of the given neurons: pick the ridge on the held-back train rows, then score
// the test rows
static double fit_primal(const LinearData& data, const Standard& s,
                         const std::vector<int>& features) {
    int m = (int)features.size();
    int c = data.classes;
    std::vector<double> a((size_t)m * m), b((size_t)m * c, 0.0);
    for (int i = 0; i < m; i++) {
        const double* zi = &s.zt[(size_t)features[(size_t)i] * s.fit];
        for (int j = 0; j <= i; j++) {
            double v = dot(zi, &s.zt[(size_t)features[(size_t)j] * s.fit], s.fit);
            a[(size_t)i * m + j] = v;
            a[(size_t)j * m + i] = v;
        }
        for (int r = 0; r < s.fit; r++) {
            for (int k = 0; k < c; k++) {
                b[(size_t)i * c + k] += zi[r] * s.y[(size_t)r * c + k];
            }
        }
    }
    double best_val = -1, best_test = 0;
    std::vector<double> w;
    for (int step = RIDGE_STEPS - 1; step >= 0; step--) {
        if (!ridge_solve(a, m, ridge_scale[step] * s.fit, b.data(), c, w)) {
            continue;
        }
        double val = accuracy(data, s, features, w, s.fit, s.train);
        if (val > best_val) {
            best_val = val;
            best_test = accuracy(data, s, features, w, s.train, s.n);
        }
    }
    return best_test;
}

// the whole sheet: more neurons than trials, so solve over trials and map back
static double fit_dual(const LinearData& data, const Standard& s) {
    int n = s.fit, c = data.classes;
    std::vector<double> g((size_t)n * n);
    for (int i = 0; i < n; i++) {
        const double* zi = &s.z[(size_t)i * s.d];
        for (int j = 0; j <= i; j++) {
            double v = dot(zi, &s.z[(size_t)j * s.d], s.d);
            g[(size_t)i * n + j] = v;
            g[(size_t)j * n + i] = v;
        }
    }
    std::vector<int> all((size_t)s.d);
    for (int j = 0; j < s.d; j++) {
        all[(size_t)j] = j;
    }
    double best_val = -1, best_test = 0;
    std::vector<double> alpha, w((size_t)s.d * c);
    for (int step = RIDGE_STEPS - 1; step >= 0; step--) {
        if (!ridge_solve(g, n, ridge_scale[step] * s.d, s.y.data(), c, alpha)) {
            continue;
        }
        for (int j = 0; j < s.d; j++) {
            const double* col = &s.zt[(size_t)j * s.fit];
            for (int k = 0; k < c; k++) {
                double sum = 0;
                for (int r = 0; r < n; r++) {
                    sum += col[r] * alpha[(size_t)r * c + k];
                }
                w[(size_t)j * c + k] = sum;
            }
        }
        double val = accuracy(data, s, all, w, s.fit, s.train);
        if (val > best_val) {
            best_val = val;
            best_test = accuracy(data, s, all, w, s.train, s.n);
        }
    }
    return best_test;
}

static void tile_neurons(int tile, std::vector<int>& out) {
    for (int k = 0; k < 64; k++) {
        out.push_back(tile * 64 + k);
    }
}

static void print_grid(const char* title, const std::vector<double>& acc, int w, int h,
                       int cue_tile, const std::vector<int>& motor_tiles) {
    std::printf("  %s (c cue tile, m motor tile)\n", title);
    for (int y = 0; y < h; y++) {
        std::printf("   ");
        for (int x = 0; x < w; x++) {
            int t = y * w + x;
            char mark = ' ';
            for (int m : motor_tiles) {
                mark = m == t ? 'm' : mark;
            }
            mark = t == cue_tile ? 'c' : mark;
            std::printf(" %4.2f%c", acc[(size_t)t], mark);
        }
        std::printf("\n");
    }
}

void linear_report(const LinearData& data, const char* label, int cue_tile,
                   const std::vector<int>& motor_tiles) {
    Standard s;
    standardize(data, s);
    int w = data.tiles_w, h = data.tiles_h, tiles = w * h;
    std::vector<double> tile_acc((size_t)tiles), block_acc((size_t)tiles);
    for (int t = 0; t < tiles; t++) {
        std::vector<int> own, block;
        tile_neurons(t, own);
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int x = t % w + dx, y = t / w + dy;
                if (x >= 0 && x < w && y >= 0 && y < h) {
                    tile_neurons(y * w + x, block);
                }
            }
        }
        tile_acc[(size_t)t] = fit_primal(data, s, own);
        block_acc[(size_t)t] = fit_primal(data, s, block);
    }
    // the motor cells' own views together: the union of the 3x3 blocks around motor tiles
    std::vector<uint8_t> in_view((size_t)tiles, 0);
    for (int m : motor_tiles) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                int x = m % w + dx, y = m / w + dy;
                if (x >= 0 && x < w && y >= 0 && y < h) {
                    in_view[(size_t)(y * w + x)] = 1;
                }
            }
        }
    }
    std::vector<int> view;
    for (int t = 0; t < tiles; t++) {
        if (in_view[(size_t)t]) {
            tile_neurons(t, view);
        }
    }
    double view_acc = view.empty() ? 0.0 : fit_primal(data, s, view);
    double whole = fit_dual(data, s);
    double best_tile = 0, best_block = 0;
    for (int t = 0; t < tiles; t++) {
        best_tile = tile_acc[(size_t)t] > best_tile ? tile_acc[(size_t)t] : best_tile;
        best_block = block_acc[(size_t)t] > best_block ? block_acc[(size_t)t] : best_block;
    }
    char view_text[16] = "-";
    if (!view.empty()) {
        std::snprintf(view_text, sizeof(view_text), "%.3f", view_acc);
    }
    std::printf("%-28s trials %d (test %d)  whole sheet %.3f  motors' view %s  best 3x3 %.3f  "
                "best tile %.3f\n",
                label, s.n, s.n - s.train, whole, view_text, best_block, best_tile);
    print_grid("one tile", tile_acc, w, h, cue_tile, motor_tiles);
    print_grid("3x3 tiles around it (a motor cell's local view)", block_acc, w, h, cue_tile,
               motor_tiles);
    std::fflush(stdout);
}
