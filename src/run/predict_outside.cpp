#include "run/predict.hpp"

// --predict-outside M (idea 183, docs/results-2026-11-26-predict-every-event.md): a
// predictor that is not the sheet's own, as a measuring stick. the every-event doc found
// that the one-hop rule needs the error of an output that is right most of the time and
// stops teaching when it is right by a margin; the sheet's predictor cells are not that
// (they are under "every line stays off" on most worlds). this one is the every-event
// doc's whole-number output (mnist_outside.cpp, form 3) turned to prediction: for every
// predicted line and horizon a row of 16-bit weights over the non-sensor cells' spike
// counts of the step. the vote is weights times counts; the line is said on when the vote
// is above 0. when the line's value arrives: a line that came on with a vote under M steps'
// worth takes one step up (its weights gain the counts) and is told "higher"; a line that
// stayed off with a vote over minus M steps' worth takes one step down and is told
// "lower"; the rest are fine and send nothing. one step moves a vote by the squared size
// of the counts. whole numbers only, but outside the blob: it is no part of the method.

void outside_start(Predictor& p, const Blob& blob) {
    OutsidePredictor& o = p.outside;
    o.margin = p.s.outside;
    if (o.margin <= 0 || p.bits < 1) {
        return;
    }
    for (int n = 0; n < blob.neuron_count; n++) {
        if (!((blob.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1)) {
            o.cell.push_back(n);
        }
    }
    size_t d = o.cell.size();
    o.count.assign(d, 0);
    o.kept.assign((size_t)PREDICT_RING * d, 0);
    o.weight.assign((size_t)p.bits * d, 0);
    o.on = true;
}

void outside_count(Predictor& p, const Blob& blob) {
    OutsidePredictor& o = p.outside;
    const uint64_t* spikes = blob.spikes[blob.now];
    for (size_t j = 0; j < o.cell.size(); j++) {
        int n = o.cell[j];
        o.count[j] = (uint8_t)(o.count[j] + ((spikes[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1));
    }
}

void outside_step_end(Predictor& p, int slot) {
    OutsidePredictor& o = p.outside;
    size_t d = o.cell.size();
    for (size_t j = 0; j < d; j++) {
        o.kept[(size_t)slot * d + j] = o.count[j];
        o.count[j] = 0;
    }
}

void outside_grade(Predictor& p, int i, int from, const uint8_t* now, bool scored) {
    OutsidePredictor& o = p.outside;
    size_t d = o.cell.size();
    const uint8_t* x = &o.kept[(size_t)from * d];
    int64_t size = 0;
    for (size_t j = 0; j < d; j++) {
        size += (int64_t)x[j] * x[j];
    }
    PredictOutsideTally& t = p.score.outside[i];
    for (int l = 0; l < p.targets; l++) {
        int16_t* row = &o.weight[(size_t)(l * p.s.horizons + i) * d];
        int64_t vote = 0;
        for (size_t j = 0; j < d; j++) {
            vote += (int64_t)row[j] * x[j];
        }
        bool was = now[l] != 0;
        int dir = was ? (vote < o.margin * size ? 1 : 0) : (vote > -o.margin * size ? -1 : 0);
        o.error[l] = (int8_t)dir;
        for (size_t j = 0; dir != 0 && j < d; j++) {
            int v = row[j] + dir * x[j];
            row[j] = (int16_t)(v > 32767 ? 32767 : (v < -32767 ? -32767 : v));
        }
        if (scored) {
            t.total++;
            t.on += was;
            t.right += (vote > 0) == was;
            t.on_right += was && vote > 0;
            t.sent += dir != 0;
        }
    }
}
