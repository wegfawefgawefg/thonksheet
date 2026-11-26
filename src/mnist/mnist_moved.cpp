#include "mnist/mnist_run.hpp"

#include <cstdio>

// blobmnist ceiling --life-moved S (idea 183, docs/results-2026-11-26-predict-every-event.md):
// a life with something to predict in time. every digit of the life is shown for S steps in
// a row, one pixel further each step along one of four directions (the digit's own, by its
// number), so the next frame follows from this one and the way it is going. no label
// reaches the sheet.
//
// the errors come from a predictor outside the sheet, the measuring stick of
// run/predict_outside.cpp: for each of up to 121 pixels on a lattice (every second pixel
// of the middle 22 x 22) a row of 16-bit weights over the non-sensor cells' spike counts of
// a step. when the next step's frame arrives: a pixel that came on with a vote under
// --moved-margin steps' worth (default 2) takes a perceptron step up and is told "higher", a
// pixel that stayed off with a vote over minus that a step down and "lower", the rest are
// fine. the signs go to the one-hop rule as prediction lines (blob_hop_predicted): the
// cells are taught on the new step's first tick, with the marks of the step the guess was
// made in. nothing is guessed across two digits. --g.hop_coin 3 is the coin control (the
// cells get another step's errors).

constexpr int MOVED_LINES = 121;

struct Moved {
    int steps = 3, margin = 2;
    std::vector<int> cols;      // the non-sensor cells
    std::vector<int> pixel;     // the predicted pixels, as places on the 28 x 28 sheet
    std::vector<int16_t> w;     // pixels x cols
    std::vector<uint8_t> x;     // the step before's counts, by column
    bool have = false;          // x is a step of the digit now shown
    int64_t seen = 0, right = 0, on = 0, on_right = 0, sent = 0;
};

Moved* moved_make(const Sheet& s, int argc, char** argv) {
    int steps = int_flag(argc, argv, "--life-moved", 0);
    if (steps < 2) {
        return nullptr;
    }
    const Blob& b = *s.blob;
    Moved* m = new Moved;
    m->steps = steps;
    m->margin = int_flag(argc, argv, "--moved-margin", 2);
    for (int n = 0; n < b.neuron_count; n++) {
        if (!((b.sensor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1)) {
            m->cols.push_back(n);
        }
    }
    for (int y = 3; y < 25; y += 2) {
        for (int x = 3; x < 25; x += 2) {
            m->pixel.push_back(y * 28 + x);
        }
    }
    m->w.assign(m->pixel.size() * m->cols.size(), 0);
    m->x.assign(m->cols.size(), 0);
    return m;
}

void moved_free(Moved* m) {
    delete m;
}

int moved_steps(const Sheet& s) {
    return s.moved ? s.moved->steps : 1;
}

void moved_grade(Sheet& s) {
    Moved& m = *s.moved;
    if (!m.have) {
        return;
    }
    TaskSense sense;
    task_sense(s.tt->task, sense);
    size_t d = m.cols.size();
    int64_t size = 0;
    for (size_t j = 0; j < d; j++) {
        size += (int64_t)m.x[j] * m.x[j];
    }
    uint16_t off[MOVED_LINES], on[MOVED_LINES];
    uint8_t truth[MOVED_LINES];
    int lines = (int)m.pixel.size();
    for (int l = 0; l < lines; l++) {
        int16_t* row = &m.w[(size_t)l * d];
        int64_t vote = 0;
        for (size_t j = 0; j < d; j++) {
            vote += (int64_t)row[j] * m.x[j];
        }
        bool was = sense.sheet[m.pixel[(size_t)l]] != 0;
        int dir = was ? (vote < m.margin * size ? 1 : 0) : (vote > -m.margin * size ? -1 : 0);
        for (size_t j = 0; dir != 0 && j < d; j++) {
            int v = row[j] + dir * m.x[j];
            row[j] = (int16_t)(v > 32767 ? 32767 : (v < -32767 ? -32767 : v));
        }
        // the verdict as a pair of counts: a line to be taught has its spike in the wrong
        // group, a fine one in the right group
        truth[l] = was;
        on[l] = was != (dir != 0);
        off[l] = !on[l];
        m.seen++;
        m.on += was;
        m.right += (vote > 0) == was;
        m.on_right += was && vote > 0;
        m.sent += dir != 0;
    }
    blob_hop_predicted(*s.blob, 0, 1, off, on, truth, lines);
}

void moved_keep(Sheet& s, const uint8_t* counts, bool more) {
    Moved& m = *s.moved;
    for (size_t j = 0; j < m.cols.size(); j++) {
        m.x[j] = counts[m.cols[j]];
    }
    m.have = more;
}

void moved_report(Sheet& s, const char* label) {
    Moved& m = *s.moved;
    double hit = m.on ? (double)m.on_right / (double)m.on : 0;
    double rest = m.seen > m.on ? (double)(m.right - m.on_right) / (double)(m.seen - m.on) : 0;
    std::printf("%-24s next-frame predictor: right %.3f balanced %.3f, pixels that taught %.3f\n",
                label, m.seen ? (double)m.right / (double)m.seen : 0.0, (hit + rest) / 2,
                m.seen ? (double)m.sent / (double)m.seen : 0.0);
    m.seen = m.right = m.on = m.on_right = m.sent = 0;
}
