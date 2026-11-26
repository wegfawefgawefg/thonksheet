#include "mnist/mnist_run.hpp"

#include <cmath>
#include <cstdio>

// blobmnist ceiling --life-outside 1 (idea 178, docs/results-2026-11-26-every-event.md): in
// the life, the one-hop rule's error lines come from an output that is not the sheet's own.
// a softmax layer outside the sheet reads every non-sensor cell's spike count of the digit,
// learns by its own delta rule a digit at a time, and sends each line's error to the rule as
// a sign with a "fine" zone (blob_hop_errors). this is the output the rule's ladder used in
// the model (docs/results-2026-11-25-rule-ladder.md, rungs T5 to T7), copied so the engine's
// rule can be run on the errors the model's rule had. it is a measuring stick: it uses
// floats and is no part of the blob. the sheet's own readout plays no part in what the
// cells learn here.
//   --life-outside-coin 1   the control: the cells get the errors of another digit, drawn
//                           from the last 256; the layer still learns from its own
//   --outside-lr-1000 N     the layer's step, in thousandths (300; the model's 0.3,
//                           divided by 1 + the squared size of its input)
//   --outside-fine-1000 N   an error under this is fine and sends nothing (100)
//   --outside-form 1        what the layer decides is only when: on a digit whose right
//                           line it has under 1 - fine, the cells get the answer (the right
//                           line higher, every other line lower); on the rest nothing
//   --outside-form 2        no fine zone: the answer as signs on every digit
//   --outside-form 3        the same job in whole numbers, no floats: a 16-bit weight per
//                           line per cell, a line's vote is its weights times the counts,
//                           and a perceptron step with a margin: a wrong line whose vote is
//                           within --outside-margin N (8) steps' worth of the right line's
//                           loses the counts and is told "lower", and the right line then
//                           gains them and is told "higher". a hint that the output the
//                           rule needs can be built from integers

struct Outside {
    std::vector<int> cols;  // the non-sensor cells
    std::vector<float> v;   // lines x cols
    std::vector<float> bias, x;
    int lines = 0;
    float lr = 0.3f, fine = 0.1f;
    int coin = 0, form = 0, margin = 8;
    std::vector<int16_t> w; // form 3: lines x cols
    std::vector<int8_t> ring; // the last 256 digits' error signs
    int64_t ring_count = 0;
    uint64_t rng = 0x6f757473696465ull;
    int64_t right = 0, seen = 0;
};

Outside* outside_make(const Sheet& s, int argc, char** argv) {
    const Blob& b = *s.blob;
    Outside* o = new Outside;
    for (int n = 0; n < b.neuron_count; n++) {
        if (!((b.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1)) {
            o->cols.push_back(n);
        }
    }
    o->lines = s.tt->plug.motor_lines;
    o->v.assign((size_t)o->lines * o->cols.size(), 0.0f);
    o->bias.assign((size_t)o->lines, 0.0f);
    o->x.assign(o->cols.size(), 0.0f);
    o->lr = (float)int_flag(argc, argv, "--outside-lr-1000", 300) / 1000.0f;
    o->fine = (float)int_flag(argc, argv, "--outside-fine-1000", 100) / 1000.0f;
    o->coin = int_flag(argc, argv, "--life-outside-coin", 0);
    o->form = int_flag(argc, argv, "--outside-form", 0);
    o->margin = int_flag(argc, argv, "--outside-margin", 8);
    o->w.assign(o->form == 3 ? (size_t)o->lines * o->cols.size() : 0, 0);
    o->ring.assign((size_t)256 * (size_t)o->lines, 0);
    return o;
}

void outside_free(Outside* o) {
    delete o;
}

double outside_right(Outside& o) {
    double share = o.seen > 0 ? (double)o.right / (double)o.seen : 0.0;
    o.right = o.seen = 0;
    return share;
}

// form 3: the whole-number output's answer, error signs and step for one digit
static void whole_digit(Outside& o, const uint8_t* counts, int label, int8_t* sign) {
    size_t d = o.cols.size();
    int64_t vote[PLUG_MAX_MOTOR_LINES], size = 0;
    int best = 0;
    for (size_t j = 0; j < d; j++) {
        size += (int64_t)counts[o.cols[j]] * counts[o.cols[j]];
    }
    for (int l = 0; l < o.lines; l++) {
        const int16_t* row = &o.w[(size_t)l * d];
        int64_t a = 0;
        for (size_t j = 0; j < d; j++) {
            a += (int64_t)row[j] * counts[o.cols[j]];
        }
        vote[l] = a;
        best = a > vote[best] ? l : best;
    }
    o.right += best == label;
    o.seen++;
    // one step moves a line's vote by `size`; too close is within `margin` steps
    auto step = [&](int l, int dir) {
        int16_t* row = &o.w[(size_t)l * d];
        for (size_t j = 0; j < d; j++) {
            int v = row[j] + dir * counts[o.cols[j]];
            row[j] = (int16_t)(v > 32767 ? 32767 : (v < -32767 ? -32767 : v));
        }
    };
    bool any = false;
    for (int l = 0; l < o.lines; l++) {
        bool close = l != label && vote[l] + o.margin * size > vote[label];
        sign[l] = (int8_t)(close ? -1 : 0);
        any |= close;
        if (close) {
            step(l, -1);
        }
    }
    if (any && label >= 0 && label < o.lines) {
        sign[label] = 1;
        step(label, 1);
    }
}

// the signs to the blob; the coin control swaps them for another digit's first
static void send_signs(Sheet& s, Outside& o, int8_t* sign);

// one digit of the life: the layer answers, its error signs go to the blob, it learns
void outside_digit(Sheet& s, const uint8_t* counts, int label) {
    Outside& o = *s.outside;
    size_t d = o.cols.size();
    if (o.form == 3) {
        int8_t sign[PLUG_MAX_MOTOR_LINES];
        whole_digit(o, counts, label, sign);
        send_signs(s, o, sign);
        return;
    }
    float size = 0;
    for (size_t j = 0; j < d; j++) {
        o.x[j] = (float)counts[o.cols[j]];
        size += o.x[j] * o.x[j];
    }
    float out[PLUG_MAX_MOTOR_LINES], top = -1e30f, sum = 0;
    int best = 0;
    for (int l = 0; l < o.lines; l++) {
        const float* row = &o.v[(size_t)l * d];
        float a = o.bias[(size_t)l];
        for (size_t j = 0; j < d; j++) {
            a += row[j] * o.x[j];
        }
        out[l] = a;
        best = a > top ? l : best;
        top = a > top ? a : top;
    }
    for (int l = 0; l < o.lines; l++) {
        out[l] = std::exp(out[l] - top);
        sum += out[l];
    }
    o.right += best == label;
    o.seen++;
    // the error of each line (its share less the answer), and its sign for the cells:
    // +1 the line should have been higher, -1 lower, 0 fine
    int8_t sign[PLUG_MAX_MOTOR_LINES];
    float step = o.lr / (1.0f + size);
    bool sure = label >= 0 && label < o.lines && out[label] / sum > 1.0f - o.fine;
    for (int l = 0; l < o.lines; l++) {
        float e = out[l] / sum - (l == label ? 1.0f : 0.0f);
        sign[l] = (int8_t)(e >= o.fine ? -1 : (e <= -o.fine ? 1 : 0));
        if (o.form != 0) {
            sign[l] = (int8_t)(o.form == 1 && sure ? 0 : (l == label ? 1 : -1));
        }
        float* row = &o.v[(size_t)l * d];
        for (size_t j = 0; j < d; j++) {
            row[j] -= step * e * o.x[j];
        }
        o.bias[(size_t)l] -= step * e;
    }
    send_signs(s, o, sign);
}

static void send_signs(Sheet& s, Outside& o, int8_t* sign) {
    if (o.coin) {
        // this digit's signs go into the ring, another digit's come out (splitmix64)
        int64_t kept = o.ring_count < 256 ? o.ring_count : 256;
        int8_t* slot = &o.ring[(size_t)(o.ring_count % 256) * (size_t)o.lines];
        int8_t mine[PLUG_MAX_MOTOR_LINES];
        for (int l = 0; l < o.lines; l++) {
            mine[l] = sign[l];
        }
        if (kept > 0) {
            uint64_t z = (o.rng += 0x9e3779b97f4a7c15ull);
            z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
            z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
            const int8_t* got = &o.ring[(size_t)((z ^ (z >> 31)) % (uint64_t)kept) * (size_t)o.lines];
            for (int l = 0; l < o.lines; l++) {
                sign[l] = got[l];
            }
        }
        for (int l = 0; l < o.lines; l++) {
            slot[l] = mine[l];
        }
        o.ring_count++;
    }
    blob_hop_errors(*s.blob, sign, o.lines);
}
