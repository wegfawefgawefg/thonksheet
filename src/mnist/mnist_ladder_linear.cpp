#include "mnist/mnist_ladder.hpp"
#include "mnist/mnist_learn.hpp"
#include "blob/random_words.hpp"

#include <cmath>
#include <cstdlib>

// rungs 1 to 4 of the ladder: one output unit per button over every non-sensor cell's
// spike count (all 20 ticks). docs/results-2026-11-25-output-gap.md

constexpr int CLASSES = 10;
// the engine's balance: with ten buttons the nine others move at 2^-4 of the pressed one's
// chance (readout_balance 1), so here the pressed button's step is 16 and the others' 1
constexpr int PRESSED_STEP = 16;

static void inner_rows(const Ladder& L, const Record& r, Rows& out) {
    out = Rows{};
    out.d = (int)L.inner.size();
    std::vector<uint8_t> x(L.inner.size());
    for (int i = 0; i < r.n; i++) {
        const uint8_t* row = &r.c20[(size_t)i * r.d];
        for (size_t j = 0; j < L.inner.size(); j++) {
            x[j] = row[L.inner[j]];
        }
        rows_add(out, x.data(), r.label[(size_t)i]);
    }
}

double rung1_offline(const Ladder& L) {
    Rows tr, te;
    inner_rows(L, L.train, tr);
    inner_rows(L, L.test, te);
    Scale sc = scale_fit(tr, 2);
    Linear lin;
    linear_train(lin, tr, sc, CLASSES, 10, 0.001f, 1);
    return linear_accuracy(lin, te, sc);
}

struct LinReader {
    int kind = 2; // 2 float softmax, 3 integer told the answer, 4 integer told good or bad
    int good = 0; // kind 4: what moves on good (GoodHalf)
    int nonneg = 0; // 1: no weight goes below zero (a source can only speak for a button)
    int center = 0; // 1: each count less its running mean (x16, over about 128 digits)
    std::vector<int32_t> mean16;
    int d = 0;
    Scale sc;                // kind 2
    std::vector<float> wf;   // kind 2: (d + 1) x classes, the last row the bias
    std::vector<int32_t> wi; // kinds 3, 4: classes x d
    std::vector<float> xf;
    std::vector<int32_t> xi;
    float lr = 0.001f;
};

static void lin_scores(LinReader& R, const Ladder& L, const uint8_t* row, double* out) {
    int d = R.d;
    for (int j = 0; j < d; j++) {
        R.xi[(size_t)j] = row[L.inner[(size_t)j]];
        if (R.center) {
            R.xi[(size_t)j] = 16 * R.xi[(size_t)j] - R.mean16[(size_t)j];
        }
    }
    if (R.kind == 2) {
        for (int j = 0; j < d; j++) {
            R.xf[(size_t)j] = ((float)R.xi[(size_t)j] - R.sc.mean[(size_t)j]) * R.sc.inv[(size_t)j];
        }
        for (int k = 0; k < CLASSES; k++) {
            out[k] = R.wf[(size_t)d * CLASSES + k];
        }
        for (int j = 0; j < d; j++) {
            const float* w = &R.wf[(size_t)j * CLASSES];
            float x = R.xf[(size_t)j];
            for (int k = 0; k < CLASSES; k++) {
                out[k] += x * w[k];
            }
        }
        return;
    }
    for (int k = 0; k < CLASSES; k++) {
        const int32_t* w = &R.wi[(size_t)k * d];
        int64_t sum = 0;
        for (int j = 0; j < d; j++) {
            sum += (int64_t)w[j] * R.xi[(size_t)j];
        }
        out[k] = (double)sum;
    }
}

// move button k's weights by step times the input
static void lin_move(LinReader& R, int k, int step) {
    int32_t* w = &R.wi[(size_t)k * R.d];
    for (int j = 0; j < R.d; j++) {
        w[j] += step * R.xi[(size_t)j];
        w[j] = R.nonneg && w[j] < 0 ? 0 : w[j];
    }
}

// one digit learned: scores are those lin_scores just gave for it
static void lin_learn(LinReader& R, double* s, int pick, int label) {
    if (R.kind == 2) {
        double top = s[0], sum = 0;
        for (int k = 1; k < CLASSES; k++) {
            top = s[k] > top ? s[k] : top;
        }
        for (int k = 0; k < CLASSES; k++) {
            s[k] = std::exp(s[k] - top);
            sum += s[k];
        }
        float g[CLASSES];
        for (int k = 0; k < CLASSES; k++) {
            g[k] = (float)(s[k] / sum) - (k == label ? 1.0f : 0.0f);
        }
        for (int j = 0; j < R.d; j++) {
            float x = R.xf[(size_t)j];
            float* w = &R.wf[(size_t)j * CLASSES];
            for (int k = 0; k < CLASSES; k++) {
                w[k] -= R.lr * g[k] * x;
            }
        }
        float* bias = &R.wf[(size_t)R.d * CLASSES];
        for (int k = 0; k < CLASSES; k++) {
            bias[k] -= R.lr * g[k];
        }
        return;
    }
    if (R.kind == 3) {
        if (pick != label) {
            lin_move(R, label, 1);
            if (pick >= 0) {
                lin_move(R, pick, -1);
            }
        }
        return;
    }
    // told good or bad: as the engine's rule, the pressed button with the verdict, the
    // others against it at a sixteenth; no press (a tie) on bad lifts every button
    int valence = pick == label ? 1 : -1;
    if (valence > 0) {
        int runner = -1;
        bool close = ladder_close(s, CLASSES, pick, runner);
        bool closed = R.good == GOOD_ALL_CLOSE || R.good == GOOD_PRESSED_CLOSE ||
                      R.good == GOOD_RIVAL_CLOSE;
        if (R.good == GOOD_NONE || (closed && !close)) {
            return;
        }
        if (R.good == GOOD_PRESSED || R.good == GOOD_PRESSED_CLOSE) {
            lin_move(R, pick, PRESSED_STEP);
            return;
        }
        if (R.good == GOOD_RIVAL_CLOSE) {
            lin_move(R, pick, PRESSED_STEP);
            lin_move(R, runner, -PRESSED_STEP);
            return;
        }
    }
    for (int k = 0; k < CLASSES; k++) {
        if (pick < 0) {
            lin_move(R, k, PRESSED_STEP);
        } else {
            lin_move(R, k, k == pick ? valence * PRESSED_STEP : -valence);
        }
    }
}

static void lin_check(LinReader& R, const Ladder& L, Curve& c, int at) {
    int right = 0, ties = 0;
    int presses[CLASSES] = {};
    double s[CLASSES];
    for (int i = 0; i < L.test.n; i++) {
        lin_scores(R, L, &L.test.c20[(size_t)i * L.test.d], s);
        int pick = ladder_pick(s, CLASSES);
        right += pick == L.test.label[(size_t)i];
        ties += pick < 0;
        presses[pick < 0 ? 0 : pick] += pick >= 0;
    }
    c.acc[at] = (double)right / L.test.n;
    c.ties[at] = (double)ties / L.test.n;
    for (int k = 0; k < CLASSES; k++) {
        c.quiet_buttons[at] += presses[k] * 100 < L.test.n;
    }
}

static void lin_run(LinReader& R, const Ladder& L, Curve& c) {
    R.d = (int)L.inner.size();
    R.xf.assign((size_t)R.d, 0.0f);
    R.xi.assign((size_t)R.d, 0);
    R.mean16.assign((size_t)R.d, 0);
    if (R.kind == 2) {
        Rows tr;
        inner_rows(L, L.train, tr);
        R.sc = scale_fit(tr, 2); // label-free statistics of the recorded digits
        R.wf.assign((size_t)(R.d + 1) * CLASSES, 0.0f);
    } else {
        // small random starting weights (0..15), so the buttons are not tied for ever
        R.wi.assign((size_t)R.d * CLASSES, 0);
        uint64_t rng[4];
        rng_seed(rng, L.seed * 31 + (uint64_t)R.kind);
        for (int32_t& w : R.wi) {
            w = (int32_t)(rng_next(rng) & 15);
        }
    }
    double s[CLASSES];
    int at = 0;
    for (int64_t i = 0; at < LADDER_CHECKS; i++) {
        if (i == LADDER_CHECK_AT[at]) {
            lin_check(R, L, c, at++);
            if (at == LADDER_CHECKS) {
                break;
            }
        }
        int r = ladder_stream(L, i);
        lin_scores(R, L, &L.train.c20[(size_t)r * L.train.d], s);
        int pick = ladder_pick(s, CLASSES);
        lin_learn(R, s, pick, L.train.label[(size_t)r]);
        // the running means move only while learning (xi holds 16 x - mean)
        for (int j = 0; R.center && j < R.d; j++) {
            R.mean16[(size_t)j] += R.xi[(size_t)j] / 128;
        }
    }
}

void rung2_online_float(const Ladder& L, Curve& c) {
    LinReader R;
    R.kind = 2;
    lin_run(R, L, c);
}

void rung3_integer_told(const Ladder& L, int center, Curve& c) {
    LinReader R;
    R.kind = 3;
    R.center = center;
    lin_run(R, L, c);
}

void rung4_integer_good_bad(const Ladder& L, int good, int nonneg, int center, Curve& c) {
    LinReader R;
    R.kind = 4;
    R.center = center;
    R.good = good;
    R.nonneg = nonneg;
    lin_run(R, L, c);
}
