#include "mnist/mnist_ladder.hpp"
#include "blob/random_words.hpp"

#include <cmath>

// rungs 5 and 6 of the ladder: a button is its motor cells, each hearing only the 11 slots
// the engine gives it (its own tile and the 8 around, two far tiles), as recorded spike
// counts. docs/results-2026-11-25-output-gap.md
//   rung 5: free integer weights on every source a cell can hear, the engine's good and bad
//     rule (pressed cells with the verdict at step 16, the others against it at step 1)
//   rung 6: the engine's evidence counters (8 bits, chance 2^-6 per tick a source fired in
//     the last 15 ticks, the others at 2^-10), excitatory and inhibitory sources moving
//     opposite ways; read as graded weights (6c) or as one-bit wiring of the top counters
//     at the cell's born count (6b, the engine's readout_evidence 2), 6g with the gate

constexpr int ROW = SLOTS * TILE_NEURONS;
constexpr int MAX_COUNT = 20;
constexpr int READOUT_SHIFT = 6;
constexpr int COUNTER_TOP = 127;

struct CellReader {
    int rung = 5;
    int vote = 1; // 0 a button is the sum of its cells' drives, 1 a count of cells over threshold
    Rung6 opt;    // rung 6: the form and the fixes on it
    std::vector<int32_t> w;      // rung 5: cells x ROW
    std::vector<int8_t> counter; // rung 6: cells x ROW
    std::vector<uint64_t> wired; // rung 6: cells x SLOTS
    std::vector<uint64_t> strong; // rung 6, opt.strong: the wired sources that count 4
    std::vector<double> drive, theta, mean, var;
    double z = 0; // the threshold stands z spreads over a cell's mean drive
    std::vector<uint64_t> ge20, ge15; // per digit: MAX_COUNT x tiles, bits whose count >= r
    int tiles = 0;
    uint64_t rng[4] = {};
};

static void count_masks(CellReader& R, const uint8_t* counts,
                        std::vector<uint64_t>& ge) {

    for (int t = 0; t < R.tiles; t++) {
        for (int r = 1; r <= MAX_COUNT; r++) {
            uint64_t m = 0;
            for (int b = 0; b < TILE_NEURONS; b++) {
                m |= (uint64_t)(counts[t * TILE_NEURONS + b] >= r) << b;
            }
            ge[(size_t)(r - 1) * R.tiles + t] = m;
            if (!m) {
                for (int q = r + 1; q <= MAX_COUNT; q++) {
                    ge[(size_t)(q - 1) * R.tiles + t] = 0;
                }
                break;
            }
        }
    }
}

// each cell's drive for one digit's counts x (all 20 ticks)
static void cell_drives(CellReader& R, const Ladder& L, const uint8_t* x) {
    const Cells& C = L.cells;
    if (R.rung == 6 && R.opt.form >= 1) {
        count_masks(R, x, R.ge20);
    }
    for (int m = 0; m < C.count; m++) {
        int64_t sum = 0;
        for (int k = 0; k < SLOTS; k++) {
            int t = C.tile[(size_t)m * SLOTS + k];
            if (t < 0) {
                continue;
            }
            const uint8_t* xs = x + (size_t)t * TILE_NEURONS;
            if (R.rung == 5) {
                const int32_t* w = &R.w[(size_t)m * ROW + (size_t)k * TILE_NEURONS];
                for (int b = 0; b < TILE_NEURONS; b++) {
                    sum += (int64_t)w[b] * xs[b];
                }
            } else if (R.opt.form == 0) {
                const int8_t* c = &R.counter[(size_t)m * ROW + (size_t)k * TILE_NEURONS];
                uint64_t u = C.usable[(size_t)m * SLOTS + k], inh = C.inhib[(size_t)m * SLOTS + k];
                for (int b = 0; b < TILE_NEURONS; b++) {
                    int v = ((u >> b) & 1) && c[b] > 0 ? c[b] : 0;
                    sum += (int64_t)v * xs[b] * (((inh >> b) & 1) ? -C.inhibit_gain : 1);
                }
            } else {
                // the engine's hot loop: AND and popcount, one level of the count at a time
                uint64_t wd = R.wired[(size_t)m * SLOTS + k], inh = C.inhib[(size_t)m * SLOTS + k];
                uint64_t st = R.opt.strong ? R.strong[(size_t)m * SLOTS + k] : 0;
                for (int r = 0; r < MAX_COUNT; r++) {
                    uint64_t on = R.ge20[(size_t)r * R.tiles + t];
                    if (!on) {
                        break;
                    }
                    // a strong source is a bit in both planes: 1 + 3 = 4
                    sum += __builtin_popcountll(wd & on & ~inh) -
                           C.inhibit_gain * __builtin_popcountll(wd & on & inh) +
                           3 * (__builtin_popcountll(st & on & ~inh) -
                                C.inhibit_gain * __builtin_popcountll(st & on & inh));
                }
            }
        }
        R.drive[(size_t)m] = (double)sum;
    }
}

// the buttons' scores from the cells' drives
static void button_scores(const CellReader& R, const Ladder& L, double* score) {
    const Cells& C = L.cells;
    for (int k = 0; k < C.buttons; k++) {
        score[k] = 0;
    }
    for (int m = 0; m < C.count; m++) {
        double d = R.drive[(size_t)m];
        double th = R.theta[(size_t)m], sd = std::sqrt(R.var[(size_t)m]) + 1e-9;
        double v = R.vote == 0 ? d
                               : (R.vote == 1 ? (d > th ? (d - th) / sd : 0.0) : (d >= th ? 1.0 : 0.0));
        score[C.button[(size_t)m]] += R.opt.opponent && C.against[(size_t)m] ? -v : v;
    }
}

// the stand-in for threshold regulation: each cell's threshold follows its drive's running
// mean and spread (over about 128 digits), z spreads up, so it is over it on about the share
// of digits the engine's motor cells fire on
static void regulate(CellReader& R, const Ladder& L, bool first) {
    for (int m = 0; m < L.cells.count; m++) {
        double d = R.drive[(size_t)m], &mu = R.mean[(size_t)m], &v = R.var[(size_t)m];
        if (first) {
            mu = d;
            v = d * d + 1.0;
        } else {
            mu += (d - mu) / 128.0;
            v += ((d - mu) * (d - mu) - v) / 128.0;
        }
        R.theta[(size_t)m] = mu + R.z * std::sqrt(v);
    }
}

// z with a normal tail of p over it
static double tail_z(double p) {
    double lo = -5, hi = 5;
    for (int i = 0; i < 60; i++) {
        double mid = (lo + hi) / 2;
        (0.5 * std::erfc(mid / std::sqrt(2.0)) > p ? lo : hi) = mid;
    }
    return (lo + hi) / 2;
}

// who moves, for one cell of button b: +1 or -1, at the pressed rate (1) or the others' (2);
// 0 when it does not move
static int move_plan(int good, int pick, int runner, bool close, int valence, int b, int& rate) {
    rate = 1;
    if (pick < 0) {
        return 1; // no press: every cell gains, at the pressed rate (the engine's balance)
    }
    if (valence < 0) {
        rate = b == pick ? 1 : 2;
        return b == pick ? -1 : 1;
    }
    bool closed = good == GOOD_ALL_CLOSE || good == GOOD_PRESSED_CLOSE || good == GOOD_RIVAL_CLOSE;
    if (good == GOOD_NONE || (closed && !close)) {
        return 0;
    }
    if (good == GOOD_PRESSED || good == GOOD_PRESSED_CLOSE) {
        return b == pick ? 1 : 0;
    }
    if (good == GOOD_RIVAL_CLOSE) {
        return b == pick ? 1 : (b == runner ? -1 : 0);
    }
    rate = b == pick ? 1 : 2;
    return b == pick ? 1 : -1;
}

// the engine's rewire (evidence_rewire_as, readout_evidence 2, strength 1): the top
// counters within the born count; the level that does not fit fills what is left from a
// random place
static void rewire(CellReader& R, const Ladder& L, int m) {
    const Cells& C = L.cells;
    const int8_t* c = &R.counter[(size_t)m * ROW];
    uint64_t usable[SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        usable[k] = C.usable[(size_t)m * SLOTS + k] & (R.opt.dale ? ~C.inhib[(size_t)m * SLOTS + k] : ~0ull);
    }
    int keep = C.keep[(size_t)m] * R.opt.keep_mul;
    int histogram[128] = {};
    for (int at = 0; at < ROW; at++) {
        if (((usable[at / TILE_NEURONS] >> (at % TILE_NEURONS)) & 1) && c[at] > 0) {
            histogram[(int)c[at]]++;
        }
    }
    int cut = 127, total = 0;
    while (cut > 1 && total + histogram[cut] <= keep) {
        total += histogram[cut];
        cut--;
    }
    if (total + histogram[cut] <= keep) {
        total += histogram[cut];
    } else {
        cut++;
    }
    int fill = cut > 1 ? keep - total : 0;
    uint64_t* wd = &R.wired[(size_t)m * SLOTS];
    for (int k = 0; k < SLOTS; k++) {
        wd[k] = 0;
    }
    int start = (int)(rng_next(R.rng) % (uint64_t)ROW);
    for (int j = 0; j < ROW && fill > 0; j++) {
        int at = (start + j) % ROW;
        if (((usable[at / TILE_NEURONS] >> (at % TILE_NEURONS)) & 1) && c[at] == cut - 1) {
            wd[at / TILE_NEURONS] |= 1ull << (at % TILE_NEURONS);
            fill--;
        }
    }
    for (int at = 0; at < ROW; at++) {
        wd[at / TILE_NEURONS] |= (uint64_t)(c[at] >= cut) << (at % TILE_NEURONS);
    }
    for (int k = 0; k < SLOTS; k++) {
        wd[k] &= usable[k];
    }
    if (!R.opt.strong) {
        return;
    }
    // the top quarter of what is wired, by counter, also in the strong plane
    int wired_n = 0, top[128] = {};
    for (int at = 0; at < ROW; at++) {
        if ((wd[at / TILE_NEURONS] >> (at % TILE_NEURONS)) & 1) {
            wired_n++;
            top[(int)(c[at] > 0 ? c[at] : 0)]++;
        }
    }
    int quota = wired_n / 4, scut = 127, have = 0;
    while (scut > 1 && have + top[scut] <= quota) {
        have += top[scut--];
    }
    uint64_t* st = &R.strong[(size_t)m * SLOTS];
    for (int at = 0; at < ROW; at++) {
        bool on = ((wd[at / TILE_NEURONS] >> (at % TILE_NEURONS)) & 1) && c[at] > scut;
        st[at / TILE_NEURONS] = (st[at / TILE_NEURONS] & ~(1ull << (at % TILE_NEURONS))) |
                                ((uint64_t)on << (at % TILE_NEURONS));
    }
}

// rung 6: one cell's counters moved by the 15-tick window, with the engine's chances.
// sign +1 gains (excitatory sources up, inhibitory down), -1 loses
static void move_counters(CellReader& R, const Ladder& L, int m, int sign, int shift) {
    const Cells& C = L.cells;
    int8_t* row = &R.counter[(size_t)m * ROW];
    bool full = false;
    for (int k = 0; k < SLOTS; k++) {
        int t = C.tile[(size_t)m * SLOTS + k];
        if (t < 0) {
            continue;
        }
        uint64_t u = C.usable[(size_t)m * SLOTS + k], inh = C.inhib[(size_t)m * SLOTS + k];
        int8_t* c = row + k * TILE_NEURONS;
        for (int r = 0; r < MAX_COUNT; r++) {
            uint64_t active = R.ge15[(size_t)r * R.tiles + t];
            if (!active) {
                break;
            }
            uint64_t chance = active & u & sparse_word(R.rng, shift);
            uint64_t up = sign > 0 ? chance & ~inh : chance & inh;
            uint64_t down = chance & ~up;
            while (up) {
                int b = __builtin_ctzll(up);
                up &= up - 1;
                full = full || c[b] >= COUNTER_TOP - 1;
                c[b] = (int8_t)(c[b] < COUNTER_TOP ? c[b] + 1 : COUNTER_TOP);
            }
            while (down) {
                int b = __builtin_ctzll(down);
                down &= down - 1;
                full = full || c[b] <= -COUNTER_TOP + 1;
                c[b] = (int8_t)(c[b] > -COUNTER_TOP ? c[b] - 1 : -COUNTER_TOP);
            }
        }
    }
    // opt.halve: a counter at the clamp halves the whole row, so the order is kept and the
    // top does not fill up with ties
    if (full && R.opt.halve) {
        for (int at = 0; at < ROW; at++) {
            row[at] = (int8_t)(row[at] / 2);
        }
    }
    if (R.opt.form >= 1) {
        rewire(R, L, m);
    }
}

// one digit learned, after cell_drives and button_scores gave `score` and the pick
static void cells_learn(CellReader& R, const Ladder& L, const uint8_t* x20, const uint8_t* x15,
                        const double* score, int pick, int label) {
    const Cells& C = L.cells;
    int valence = pick == label ? 1 : -1;
    int runner = -1;
    bool close = pick >= 0 && ladder_close(score, C.buttons, pick, runner);
    if (R.rung == 5) {
        for (int m = 0; m < C.count; m++) {
            int rate = 1;
            int sign = move_plan(R.opt.good, pick, runner, close, valence, C.button[(size_t)m], rate);
            if (!sign) {
                continue;
            }
            int step = sign * (rate == 1 ? 16 : 1);
            for (int k = 0; k < SLOTS; k++) {
                int t = C.tile[(size_t)m * SLOTS + k];
                if (t < 0) {
                    continue;
                }
                uint64_t u = C.usable[(size_t)m * SLOTS + k];
                int32_t* w = &R.w[(size_t)m * ROW + (size_t)k * TILE_NEURONS];
                const uint8_t* xs = x20 + (size_t)t * TILE_NEURONS;
                for (int bit = 0; bit < TILE_NEURONS; bit++) {
                    w[bit] += ((u >> bit) & 1) ? step * xs[bit] : 0;
                }
            }
        }
        return;
    }
    count_masks(R, x15, R.ge15);
    // the balance (readout_balance 1): the others' chance is lower by their number over
    // the pressed button's, rounded up to a power of two; no press lifts every cell
    int pressed = 0;
    for (int m = 0; m < C.count; m++) {
        pressed += C.button[(size_t)m] == pick;
    }
    int other_shift = READOUT_SHIFT;
    for (int e = 0; pressed > 0 && (pressed << e) < C.count - pressed; e++) {
        other_shift++;
    }
    for (int m = 0; m < C.count; m++) {
        int rate = 1;
        int sign = move_plan(R.opt.good, pick, runner, close, valence, C.button[(size_t)m], rate);
        sign = R.opt.opponent && C.against[(size_t)m] ? -sign : sign;
        if (sign) {
            move_counters(R, L, m, sign, rate == 1 ? READOUT_SHIFT : other_shift);
        }
    }
}

static void cells_check(CellReader& R, const Ladder& L, Curve& c, int at) {
    int right = 0, ties = 0;
    std::vector<int> presses((size_t)L.cells.buttons, 0);
    std::vector<double> score((size_t)L.cells.buttons);
    for (int i = 0; i < L.test.n; i++) {
        cell_drives(R, L, &L.test.c20[(size_t)i * L.test.d]);
        button_scores(R, L, score.data());
        int pick = ladder_pick(score.data(), L.cells.buttons);
        right += pick == L.test.label[(size_t)i];
        ties += pick < 0;
        if (pick >= 0) {
            presses[(size_t)pick]++;
        }
    }
    c.acc[at] = (double)right / L.test.n;
    c.ties[at] = (double)ties / L.test.n;
    for (int p : presses) {
        c.quiet_buttons[at] += p * 100 < L.test.n;
    }
}

static void cells_run(CellReader& R, const Ladder& L, Curve& c) {
    const Cells& C = L.cells;
    R.tiles = L.train.d / TILE_NEURONS;
    R.ge20.assign((size_t)MAX_COUNT * R.tiles, 0);
    R.ge15.assign((size_t)MAX_COUNT * R.tiles, 0);
    R.drive.assign((size_t)C.count, 0.0);
    R.theta.assign((size_t)C.count, 0.0);
    R.mean.assign((size_t)C.count, 0.0);
    R.var.assign((size_t)C.count, 1.0);
    R.z = tail_z(L.cells.fire_share);
    rng_seed(R.rng, L.seed * 977 + (uint64_t)(R.rung * 10 + R.opt.form));
    if (R.rung == 5) {
        // the born cell: excitatory sources at their strength, inhibitory at minus the gain
        R.w.assign((size_t)C.count * ROW, 0);
        for (int m = 0; m < C.count; m++) {
            for (int at = 0; at < ROW; at++) {
                uint64_t inh = C.inhib[(size_t)m * SLOTS + at / TILE_NEURONS];
                int s = C.born[(size_t)m * ROW + at];
                R.w[(size_t)m * ROW + at] = ((inh >> (at % TILE_NEURONS)) & 1) ? -C.inhibit_gain * s : s;
            }
        }
    } else {
        R.counter.assign(C.born.begin(), C.born.end());
        R.wired.assign((size_t)C.count * SLOTS, 0);
        R.strong.assign((size_t)C.count * SLOTS, 0);
        for (int m = 0; m < C.count; m++) {
            rewire(R, L, m);
        }
    }
    std::vector<double> score((size_t)C.buttons);
    int at = 0;
    for (int64_t i = 0; at < LADDER_CHECKS; i++) {
        if (i == LADDER_CHECK_AT[at]) {
            cells_check(R, L, c, at++);
            if (at == LADDER_CHECKS) {
                break;
            }
        }
        int r = ladder_stream(L, i);
        const uint8_t* x20 = &L.train.c20[(size_t)r * L.train.d];
        const uint8_t* x15 = &L.train.c15[(size_t)r * L.train.d];
        cell_drives(R, L, x20);
        button_scores(R, L, score.data());
        int pick = ladder_pick(score.data(), C.buttons);
        if (R.vote >= 1) {
            regulate(R, L, i == 0);
        }
        cells_learn(R, L, x20, x15, score.data(), pick, L.train.label[(size_t)r]);
    }
}

void rung5_cells(const Ladder& L, int vote, int good, Curve& c) {
    CellReader R;
    R.rung = 5;
    R.vote = vote;
    R.opt.good = good;
    cells_run(R, L, c);
}

void rung6_counters(const Ladder& L, const Rung6& opt, Curve& c) {
    CellReader R;
    R.rung = 6;
    R.vote = 1;
    R.opt = opt;
    cells_run(R, L, c);
}
