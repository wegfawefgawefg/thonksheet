#include "probe/linear_split.hpp"

#include "probe/linear_fit.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <string>

constexpr int RIDGE_STEPS = 6;
static const double ridge_scale[RIDGE_STEPS] = {0.0003, 0.003, 0.03, 0.3, 3.0, 30.0};

void split_prepare(const SplitData& data, SplitPrep& p) {
    p = SplitPrep{};
    p.n = (int)data.label.size();
    p.d = data.features;
    p.classes = data.classes;
    std::vector<int> taught;
    for (int r = 0; r < p.n; r++) {
        (data.held[(size_t)r] ? p.held : taught).push_back(r);
    }
    int t = (int)taught.size();
    int fit_end = t * 60 / 100, check_end = t * 75 / 100;
    for (int i = 0; i < t; i++) {
        std::vector<int>& to = i < fit_end ? p.fit : (i < check_end ? p.check : p.same);
        to.push_back(taught[(size_t)i]);
    }
    int nf = (int)p.fit.size();
    std::vector<double> mean((size_t)p.d, 0.0), sq((size_t)p.d, 0.0);
    for (int r : p.fit) {
        const uint8_t* x = &data.x[(size_t)r * p.d];
        for (int j = 0; j < p.d; j++) {
            mean[(size_t)j] += x[j];
            sq[(size_t)j] += (double)x[j] * x[j];
        }
    }
    for (int j = 0; j < p.d; j++) {
        double m = nf ? mean[(size_t)j] / nf : 0.0;
        double v = nf ? sq[(size_t)j] / nf - m * m : 0.0;
        mean[(size_t)j] = m;
        // a feature that never varied while fitting is left out (all zeros), so a pixel or
        // neuron seen only in held-out versions carries no weight
        sq[(size_t)j] = v > 1e-9 ? 1.0 / std::sqrt(v) : 0.0;
    }
    p.z.assign((size_t)p.n * p.d, 0.0);
    for (int r = 0; r < p.n; r++) {
        const uint8_t* x = &data.x[(size_t)r * p.d];
        double* z = &p.z[(size_t)r * p.d];
        for (int j = 0; j < p.d; j++) {
            z[j] = (x[j] - mean[(size_t)j]) * sq[(size_t)j];
        }
    }
    int c = p.classes;
    p.y_mean.assign((size_t)c, 0.0);
    for (int r : p.fit) {
        p.y_mean[(size_t)data.label[(size_t)r]] += 1.0 / nf;
    }
    p.y.assign((size_t)nf * c, 0.0);
    for (int i = 0; i < nf; i++) {
        int label = data.label[(size_t)p.fit[(size_t)i]];
        for (int k = 0; k < c; k++) {
            p.y[(size_t)i * c + k] = (label == k ? 1.0 : 0.0) - p.y_mean[(size_t)k];
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

// the class a reader with weights w (m x classes) on `features` gives row r
static int answer(const SplitPrep& p, const std::vector<int>& features,
                  const std::vector<double>& w, int r) {
    int c = p.classes;
    const double* z = &p.z[(size_t)r * p.d];
    int best = 0;
    double best_score = -1e300;
    for (int k = 0; k < c; k++) {
        double score = p.y_mean[(size_t)k];
        for (size_t f = 0; f < features.size(); f++) {
            score += z[features[f]] * w[f * c + (size_t)k];
        }
        if (score > best_score) {
            best_score = score;
            best = k;
        }
    }
    return best;
}

static double share_right(const SplitData& data, const SplitPrep& p,
                          const std::vector<int>& features, const std::vector<double>& w,
                          const std::vector<int>& rows) {
    int right = 0;
    for (int r : rows) {
        right += answer(p, features, w, r) == data.label[(size_t)r];
    }
    return rows.empty() ? 0.0 : (double)right / (double)rows.size();
}

SplitScore split_fit(const SplitData& data, const SplitPrep& p, const std::vector<int>& features_in,
                     int versions) {
    std::vector<int> features = features_in;
    if (features.empty()) {
        for (int j = 0; j < p.d; j++) {
            features.push_back(j);
        }
    }
    int m = (int)features.size(), nf = (int)p.fit.size(), c = p.classes;
    // the fit rows on these features, row-major and column-major
    std::vector<double> rows((size_t)nf * m), cols((size_t)m * nf);
    for (int i = 0; i < nf; i++) {
        const double* z = &p.z[(size_t)p.fit[(size_t)i] * p.d];
        for (int f = 0; f < m; f++) {
            rows[(size_t)i * m + f] = z[features[(size_t)f]];
            cols[(size_t)f * nf + i] = z[features[(size_t)f]];
        }
    }
    bool primal = m <= nf;
    int size = primal ? m : nf;
    std::vector<double> a((size_t)size * size), b;
    for (int i = 0; i < size; i++) {
        const double* ri = primal ? &cols[(size_t)i * nf] : &rows[(size_t)i * m];
        for (int j = 0; j <= i; j++) {
            const double* rj = primal ? &cols[(size_t)j * nf] : &rows[(size_t)j * m];
            double v = dot(ri, rj, primal ? nf : m);
            a[(size_t)i * size + j] = v;
            a[(size_t)j * size + i] = v;
        }
    }
    if (primal) {
        b.assign((size_t)m * c, 0.0);
        for (int f = 0; f < m; f++) {
            for (int i = 0; i < nf; i++) {
                for (int k = 0; k < c; k++) {
                    b[(size_t)f * c + k] += cols[(size_t)f * nf + i] * p.y[(size_t)i * c + k];
                }
            }
        }
    }
    SplitScore best;
    best.check = -1;
    std::vector<double> x, w((size_t)m * c);
    for (int step = RIDGE_STEPS - 1; step >= 0; step--) {
        double ridge = ridge_scale[step] * (primal ? nf : m);
        if (!linear_ridge_solve(a, size, ridge, primal ? b.data() : p.y.data(), c, x)) {
            continue;
        }
        if (primal) {
            w = x;
        } else {
            for (int f = 0; f < m; f++) {
                for (int k = 0; k < c; k++) {
                    double sum = 0;
                    for (int i = 0; i < nf; i++) {
                        sum += cols[(size_t)f * nf + i] * x[(size_t)i * c + k];
                    }
                    w[(size_t)f * c + k] = sum;
                }
            }
        }
        double check = share_right(data, p, features, w, p.check);
        if (check <= best.check) {
            continue;
        }
        best.check = check;
        best.same = share_right(data, p, features, w, p.same);
        best.held = share_right(data, p, features, w, p.held);
        if (versions > 0) {
            std::vector<int> right((size_t)versions, 0), seen((size_t)versions, 0);
            for (const std::vector<int>* set : {&p.same, &p.held}) {
                for (int r : *set) {
                    int v = data.version[(size_t)r];
                    if (v >= 0 && v < versions) {
                        seen[(size_t)v]++;
                        right[(size_t)v] += answer(p, features, w, r) == data.label[(size_t)r];
                    }
                }
            }
            best.by_version.assign((size_t)versions, -1.0);
            for (int v = 0; v < versions; v++) {
                if (seen[(size_t)v]) {
                    best.by_version[(size_t)v] = (double)right[(size_t)v] / seen[(size_t)v];
                }
            }
        }
    }
    return best;
}

// the nearest taught pattern: every distinct fit row with how often each class came with
// it; a test row takes the classes of the fit patterns nearest to it (fewest differing
// counts, summed), ties shared. what a reader that only remembers what it saw would do
static double nearest_share(const SplitData& data, const SplitPrep& p,
                            const std::map<std::string, std::vector<int>>& seen,
                            const std::vector<int>& rows) {
    int d = data.features, c = p.classes;
    double right = 0;
    std::vector<int> votes((size_t)c, 0);
    for (int r : rows) {
        const uint8_t* x = &data.x[(size_t)r * d];
        int best = 1 << 30;
        std::fill(votes.begin(), votes.end(), 0);
        for (const auto& kv : seen) {
            const uint8_t* y = (const uint8_t*)kv.first.data();
            int dist = 0;
            for (int j = 0; j < d && dist <= best; j++) {
                dist += x[j] > y[j] ? x[j] - y[j] : y[j] - x[j];
            }
            if (dist < best) {
                best = dist;
                std::fill(votes.begin(), votes.end(), 0);
            }
            if (dist == best) {
                for (int i = 0; i < c; i++) {
                    votes[(size_t)i] += kv.second[(size_t)i];
                }
            }
        }
        int total = 0;
        for (int v : votes) {
            total += v;
        }
        right += total ? (double)votes[(size_t)data.label[(size_t)r]] / total : 0.0;
    }
    return rows.empty() ? 0.0 : right / (double)rows.size();
}

SplitScore split_nearest(const SplitData& data, const SplitPrep& p) {
    std::map<std::string, std::vector<int>> seen;
    int d = data.features;
    for (int r : p.fit) {
        const uint8_t* x = &data.x[(size_t)r * d];
        std::vector<int>& counts = seen[std::string((const char*)x, (size_t)d)];
        counts.resize((size_t)p.classes, 0);
        counts[(size_t)data.label[(size_t)r]]++;
    }
    SplitScore s;
    s.same = nearest_share(data, p, seen, p.same);
    s.held = nearest_share(data, p, seen, p.held);
    return s;
}
