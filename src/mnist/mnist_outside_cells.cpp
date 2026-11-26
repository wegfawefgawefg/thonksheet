#include "mnist/mnist_outside.hpp"

#include <algorithm>
#include <cstdio>

// blobmnist ceiling --life-outside 1 --outside-form 4 (idea 182,
// docs/results-2026-11-26-output-inside.md): the ladder from the outside whole-number output
// (form 3) down toward the engine's readout, one difference an option. still outside the
// blob, still a measuring stick: it says which difference costs the output its accuracy.
// with every option at its default it is form 3 to the digit.
//   --outside-read 1      a line is its motor cells, and each cell reads the places it can
//                         hear (9 local tiles and its far tiles; itself left out; no
//                         inhibitory source on a far slot), sensor cells included. 0: a line
//                         reads every non-sensor cell once
//   --outside-read 2      as 1, but no cell reads a sensor cell
//   --outside-top 1       only the wrong line with the highest vote can be too close (one
//                         line down for one line up, so what all lines share cannot drift)
//   --outside-act 1       a source is one bit: it fired in the last --outside-window (15)
//                         ticks of the digit. 0: its spike count over the digit
//   --outside-bits N      a weight stops at +-(2^(N-1) - 1) (16)
//   --outside-step-shift N   each spike steps a weight with chance 2^-N (0: every spike)
//   --outside-vote 1      a line's vote is the number of its cells whose own sum is over
//                         zero (cells that would spike once), not the sum
//   --outside-vote 3      as 1, but a cell's bar is the running mean of its own sum (over
//                         about 64 digits), as threshold regulation would hold it
//   --outside-vote 2      each cell wires its highest weights at its born count, one bit
//                         each (an inhibitory source by its most negative weight, and it
//                         then counts -5); the vote is the sum of the cells' drives
//   --outside-rule 0      told the right line: the perceptron with a margin (form 3's)
//   --outside-rule 1      told good or bad only, the engine's plain rule: on bad the pressed
//                         line loses 16 and the others gain 1, on good the reverse
//   --outside-rule 2      the same with the gate: on good only when the lead was thin
//   --outside-rule 3      on a thin good the pressed line gains 16 and the runner-up loses 16
//   --outside-margin N    "too close" and "thin": within N steps' worth (2 here)
//   --outside-zone-32 N   votes 1 and 2: too close is within N 32nds of the right line's
//                         vote (hop_teach 5's zone), since a step has no size there
// the error signs sent to the cells are the same in every form: a wrong line too close to
// the right line's vote (or over it) is told "lower", and then the right line "higher".

struct MotorCell {
    int n = 0, line = 0, keep = 1;
    int src[SLOTS] = {};
};

struct OutsideCells {
    int lines = 0, neurons = 0, tiles = 0;
    int read = 0, act = 0, bits = 16, step_shift = 0, vote = 0, rule = 0, margin = 2, zone = 0;
    int window = 15, inhibit_gain = 5, top = 0;
    std::vector<int32_t> w;    // lines x neurons: the weight a line's cells keep for a source
    std::vector<int32_t> mult; // lines x neurons: how many of the line's places hear it
    std::vector<int32_t> a;    // this digit's activity per neuron
    std::vector<uint8_t> inhibitory, skip; // skip: sources no cell may read
    std::vector<MotorCell> cells;
    std::vector<int> first;              // line l's cells: cells[first[l] .. first[l + 1])
    std::vector<std::vector<int>> wired; // vote 2: each cell's wired sources
    std::vector<uint8_t> stale;          // vote 2: the line's cells must re-pick
    std::vector<int64_t> bar;            // vote 3: 64 times each cell's running mean sum
    uint64_t rng = 0x6c6164646572ull;
};

static bool flag_bit(const uint64_t* words, int n) {
    return (words[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

// may cell c wire source s through slot k
static bool usable(const OutsideCells& o, const MotorCell& c, int k, int s) {
    return s != c.n && !o.skip[(size_t)s] && !(k >= LOCAL_SLOTS && o.inhibitory[(size_t)s]);
}

OutsideCells* outside_cells_make(const Sheet& s, int argc, char** argv) {
    const Blob& b = *s.blob;
    const Plug& plug = s.tt->plug;
    OutsideCells* o = new OutsideCells;
    o->lines = plug.motor_lines;
    o->neurons = b.neuron_count;
    o->tiles = b.tile_count;
    o->read = int_flag(argc, argv, "--outside-read", 0);
    o->act = int_flag(argc, argv, "--outside-act", 0);
    o->bits = int_flag(argc, argv, "--outside-bits", 16);
    o->step_shift = int_flag(argc, argv, "--outside-step-shift", 0);
    o->vote = int_flag(argc, argv, "--outside-vote", 0);
    o->rule = int_flag(argc, argv, "--outside-rule", 0);
    o->margin = int_flag(argc, argv, "--outside-margin", 2);
    o->zone = int_flag(argc, argv, "--outside-zone-32", 0);
    o->window = int_flag(argc, argv, "--outside-window", 15);
    o->top = int_flag(argc, argv, "--outside-top", 0);
    o->inhibit_gain = b.genome.inhibit_gain;
    size_t all = (size_t)o->lines * (size_t)o->neurons;
    o->w.assign(all, 0);
    o->mult.assign(all, 0);
    o->a.assign((size_t)o->neurons, 0);
    o->inhibitory.assign((size_t)o->neurons, 0);
    o->skip.assign((size_t)o->neurons, 0);
    for (int n = 0; n < o->neurons; n++) {
        o->inhibitory[(size_t)n] = flag_bit(b.inhibitory, n);
        o->skip[(size_t)n] = o->read == 2 && flag_bit(b.sensor, n);
    }
    o->first.assign((size_t)o->lines + 1, 0);
    for (int l = 0; l < o->lines; l++) {
        const int* cells = nullptr;
        int count = plug_motor_cells(plug, l, &cells);
        o->first[(size_t)l] = (int)o->cells.size();
        for (int i = 0; i < count; i++) {
            MotorCell c;
            c.n = cells[i];
            c.line = l;
            c.keep = b.fan_in[c.n] > 0 ? b.fan_in[c.n] : 1;
            for (int k = 0; k < SLOTS; k++) {
                uint32_t src = k < LOCAL_SLOTS
                                   ? b.local_tile[(c.n / TILE_NEURONS) * LOCAL_SLOTS + k]
                                   : b.far_tile[c.n * FAR_SLOTS + k - LOCAL_SLOTS];
                c.src[k] = src == (uint32_t)b.tile_count ? -1 : (int)src;
            }
            o->cells.push_back(c);
        }
    }
    o->first[(size_t)o->lines] = (int)o->cells.size();
    for (int l = 0; l < o->lines; l++) {
        int32_t* m = &o->mult[(size_t)l * (size_t)o->neurons];
        for (int n = 0; o->read == 0 && n < o->neurons; n++) {
            m[n] = !flag_bit(b.sensor, n);
        }
        for (int i = o->first[(size_t)l]; o->read != 0 && i < o->first[(size_t)l + 1]; i++) {
            const MotorCell& c = o->cells[(size_t)i];
            for (int k = 0; k < SLOTS; k++) {
                for (int j = 0; c.src[k] >= 0 && j < TILE_NEURONS; j++) {
                    int src = c.src[k] * TILE_NEURONS + j;
                    m[src] += usable(*o, c, k, src);
                }
            }
        }
    }
    o->wired.resize(o->cells.size());
    o->stale.assign((size_t)o->lines, 1);
    o->bar.assign(o->cells.size(), 0);
    return o;
}

void outside_cells_free(OutsideCells* c) {
    delete c;
}

static uint64_t draw(OutsideCells& o) {
    uint64_t z = (o.rng += 0x9e3779b97f4a7c15ull);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

// line l moves by `gain` times the activity, up or down
static void step_line(OutsideCells& o, int l, int dir, int gain) {
    int32_t* w = &o.w[(size_t)l * (size_t)o.neurons];
    const int32_t* m = &o.mult[(size_t)l * (size_t)o.neurons];
    int32_t top = (1 << (o.bits - 1)) - 1;
    uint64_t mask = (1ull << o.step_shift) - 1;
    for (int n = 0; n < o.neurons; n++) {
        if (!m[n] || !o.a[(size_t)n]) {
            continue;
        }
        int32_t by = o.a[(size_t)n];
        if (o.step_shift > 0) {
            by = 0;
            for (int i = 0; i < o.a[(size_t)n]; i++) {
                by += (draw(o) & mask) == 0;
            }
        }
        int32_t v = w[n] + dir * gain * by;
        w[n] = v > top ? top : (v < -top ? -top : v);
    }
    o.stale[(size_t)l] = 1;
}

// vote 2: every cell of a line whose weights moved wires its highest weights again
static void repick(OutsideCells& o, int l) {
    const int32_t* w = &o.w[(size_t)l * (size_t)o.neurons];
    std::vector<std::pair<int32_t, int>> key;
    for (int i = o.first[(size_t)l]; i < o.first[(size_t)l + 1]; i++) {
        const MotorCell& c = o.cells[(size_t)i];
        key.clear();
        for (int k = 0; k < SLOTS; k++) {
            for (int j = 0; c.src[k] >= 0 && j < TILE_NEURONS; j++) {
                int src = c.src[k] * TILE_NEURONS + j;
                int32_t v = o.inhibitory[(size_t)src] ? -w[src] : w[src];
                if (v > 0 && usable(o, c, k, src)) {
                    key.push_back({-v, src});
                }
            }
        }
        size_t keep = (size_t)c.keep < key.size() ? (size_t)c.keep : key.size();
        std::partial_sort(key.begin(), key.begin() + (long)keep, key.end());
        o.wired[(size_t)i].clear();
        for (size_t j = 0; j < keep; j++) {
            o.wired[(size_t)i].push_back(key[j].second);
        }
    }
    o.stale[(size_t)l] = 0;
}

// each line's vote for this digit's activity, and what one step is worth to it
static void votes(OutsideCells& o, int64_t* vote, int64_t* size) {
    for (int l = 0; l < o.lines; l++) {
        const int32_t* w = &o.w[(size_t)l * (size_t)o.neurons];
        const int32_t* m = &o.mult[(size_t)l * (size_t)o.neurons];
        int64_t v = 0, sz = 0;
        for (int n = 0; n < o.neurons; n++) {
            int64_t a = o.a[(size_t)n];
            v += (int64_t)m[n] * w[n] * a;
            sz += (int64_t)m[n] * a * a;
        }
        vote[l] = v;
        size[l] = sz;
        if (o.vote == 0 || o.read == 0) {
            continue;
        }
        if (o.vote == 2 && o.stale[(size_t)l]) {
            repick(o, l);
        }
        // the cells' own sums: the tiles' sums (all sources, and excitatory ones for far slots)
        std::vector<int64_t> all((size_t)o.tiles, 0), exc((size_t)o.tiles, 0);
        for (int n = 0; o.vote != 2 && n < o.neurons; n++) {
            int64_t x = (int64_t)w[n] * o.a[(size_t)n];
            all[(size_t)(n / TILE_NEURONS)] += x;
            exc[(size_t)(n / TILE_NEURONS)] += o.inhibitory[(size_t)n] ? 0 : x;
        }
        v = 0;
        for (int i = o.first[(size_t)l]; i < o.first[(size_t)l + 1]; i++) {
            const MotorCell& c = o.cells[(size_t)i];
            int64_t sum = 0;
            if (o.vote == 2) {
                for (int src : o.wired[(size_t)i]) {
                    sum += (o.inhibitory[(size_t)src] ? -o.inhibit_gain : 1) * o.a[(size_t)src];
                }
                v += sum;
                continue;
            }
            for (int k = 0; k < SLOTS; k++) {
                if (c.src[k] < 0) {
                    continue;
                }
                sum += k < LOCAL_SLOTS ? all[(size_t)c.src[k]] : exc[(size_t)c.src[k]];
                bool own = k < LOCAL_SLOTS && c.src[k] == c.n / TILE_NEURONS;
                sum -= own ? (int64_t)w[c.n] * o.a[(size_t)c.n] : 0;
            }
            int64_t& bar = o.bar[(size_t)i];
            v += o.vote == 3 ? 64 * sum > bar : sum > 0;
            bar += sum - bar / 64;
        }
        vote[l] = v;
    }
}

int outside_cells_digit(const Sheet& s, OutsideCells& o, const uint8_t* counts, int label,
                        int8_t* sign) {
    const Blob& b = *s.blob;
    // the activity: counts over the digit, or one bit for the last ticks
    for (int n = 0; n < o.neurons; n++) {
        o.a[(size_t)n] = counts[n];
    }
    for (int t = 0; o.act != 0 && t < o.tiles; t++) {
        uint64_t fired = 0;
        for (int age = 0; age < o.window && age < HISTORY; age++) {
            fired |= b.spikes[(b.now - age) & (HISTORY - 1)][t];
        }
        for (int j = 0; j < TILE_NEURONS; j++) {
            o.a[(size_t)(t * TILE_NEURONS + j)] = (int32_t)((fired >> j) & 1);
        }
    }
    int64_t vote[PLUG_MAX_MOTOR_LINES], size[PLUG_MAX_MOTOR_LINES];
    votes(o, vote, size);
    int best = 0, runner = -1;
    for (int l = 1; l < o.lines; l++) {
        best = vote[l] > vote[best] ? l : best;
    }
    for (int l = 0; l < o.lines; l++) {
        runner = l != best && (runner < 0 || vote[l] > vote[runner]) ? l : runner;
    }
    bool known = label >= 0 && label < o.lines;
    int gain = o.rule == 0 ? 1 : 16;
    // too close: within the zone of the other line's vote
    auto close = [&](int low, int high) {
        if (o.zone > 0 && o.vote != 0) {
            return vote[high] <= 0 ||
                   (vote[low] > 0 && 32 * vote[low] > (32 - o.zone) * vote[high]);
        }
        int64_t unit = o.vote != 0 ? 1 : ((size[low] + size[high]) / 2 * gain) >> o.step_shift;
        return vote[low] + o.margin * unit > vote[high];
    };
    bool any = false;
    int high = -1;
    for (int l = 0; l < o.lines; l++) {
        bool lower = known && l != label && close(l, label);
        sign[l] = (int8_t)(lower ? -1 : 0);
        any |= lower;
        high = lower && (high < 0 || vote[l] > vote[high]) ? l : high;
    }
    for (int l = 0; o.top && l < o.lines; l++) {
        sign[l] = (int8_t)(l == high ? -1 : 0);
    }
    if (any) {
        sign[label] = 1;
    }
    if (o.rule == 0) {
        for (int l = 0; l < o.lines; l++) {
            if (sign[l] != 0) {
                step_line(o, l, sign[l], 1);
            }
        }
        return best;
    }
    // told good or bad only: the engine's rules in whole numbers
    bool good = best == label;
    bool thin = runner >= 0 && close(runner, best);
    if (good && o.rule >= 2 && !thin) {
        return best;
    }
    for (int l = 0; l < o.lines; l++) {
        if (l == best) {
            step_line(o, l, good ? 1 : -1, 16);
        } else if (good && o.rule == 3) {
            if (l == runner) {
                step_line(o, l, -1, 16);
            }
        } else {
            step_line(o, l, good ? -1 : 1, 1);
        }
    }
    return best;
}
