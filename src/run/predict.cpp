#include "run/predict.hpp"

#include <cstdio>
#include <cstring>

// idea 162: the predictor cells beside the buttons (predict.hpp)

// the extra lines a plug laid without them must add
static int extra_lines(const PredictSettings& s, const Plug& plug) {
    int targets = (plug.attached[PLUG_GOOD_LINE] ? 2 : 0) + plug.first_extra - plug.first_single;
    return (s.press ? plug.motor_lines : 0) + (s.errors ? 2 * targets : 0);
}

static bool fired(const Blob& blob, int n) {
    return (blob.spikes[blob.now][n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1;
}

// the motor cells no button uses, nearest the top of the sheet first, dealt round the groups
// so every group gets cells at the same spread of distances
static bool pick_cells(Predictor& p, const Plug& plug, const Blob& blob) {
    std::vector<uint8_t> taken((size_t)blob.neuron_count, 0);
    for (int line = 0; line < plug.motor_lines; line++) {
        const int* cells = nullptr;
        int count = plug_motor_cells(plug, line, &cells);
        for (int k = 0; k < count; k++) {
            taken[(size_t)cells[k]] = 1;
        }
    }
    std::vector<int> free;
    for (int y = 0; y < blob.tiles_h * 8; y++) {
        for (int x = 0; x < blob.tiles_w * 8; x++) {
            int n = blob_neuron_at(blob, x, y);
            if (((blob.motor[n / TILE_NEURONS] >> (n % TILE_NEURONS)) & 1) && !taken[(size_t)n]) {
                free.push_back(n);
            }
        }
    }
    int groups = 2 * p.bits;
    int per = (int)free.size() / groups;
    per = per < p.s.cells ? per : p.s.cells;
    if (per < 1) {
        std::fprintf(stderr,
                     "predict: %d groups need at least %d motor cells, %zu are free of the "
                     "buttons (widen the motor region, --g.motor_w 64, or give the buttons "
                     "fewer, --motor-spread 64)\n",
                     groups, groups, free.size());
        return false;
    }
    p.cells_per_group = per;
    p.cell.assign((size_t)groups * per, 0);
    for (int j = 0; j < groups * per; j++) {
        p.cell[(size_t)(j % groups) * per + j / groups] = free[(size_t)j];
    }
    return true;
}

// idea 164: the errors go one hop to the hidden cells; their timing is horizon 1's (the
// marks are the step the prediction was made in, still in the blob's rings at the next
// step's start). with a reach, each line's place is the centre of its sensor group
static bool hop_start(const Predictor& p, const Plug& plug, Blob& blob) {
    if (!p.s.hop) {
        return true;
    }
    if (p.s.horizon[0] != 1 || p.targets > HOP_MAX_PRED) {
        std::fprintf(stderr, "predict: --predict-hop needs horizon 1 and at most %d lines\n",
                     HOP_MAX_PRED);
        return false;
    }
    if (!blob.genome.hop_on) {
        std::fprintf(stderr, "predict: --predict-hop does nothing without --g.hop_on 1\n");
    }
    if (p.s.hop_reach > 0) {
        int16_t x[HOP_MAX_PRED], y[HOP_MAX_PRED];
        int half = plug.settings.sensor_side / 2;
        for (int l = 0; l < p.targets; l++) {
            x[l] = (int16_t)(plug.sensor_cell_x[p.target_line[l]] + half);
            y[l] = (int16_t)(plug.sensor_cell_y[p.target_line[l]] + half);
        }
        blob_hop_error_places(blob, x, y, p.targets, p.s.hop_reach);
    }
    return true;
}

static bool start(Predictor& p, const PredictSettings& s, const Plug& plug, Blob& blob,
                  int64_t steps) {
    p.s = s;
    // targets: good and bad when attached, then the task's own lines
    if (plug.attached[PLUG_GOOD_LINE]) {
        p.target_line[p.targets++] = PLUG_GOOD_LINE;
        p.target_line[p.targets++] = PLUG_BAD_LINE;
    }
    for (int line = plug.first_single; line < plug.first_extra; line++) {
        if (p.targets == PREDICT_MAX_TARGETS) {
            std::fprintf(stderr, "predict: more than %d lines to predict\n", PREDICT_MAX_TARGETS);
            return false;
        }
        p.target_line[p.targets++] = line;
    }
    int extra = 0;
    if (s.press) {
        p.first_press = plug.first_extra;
        p.press_lines = plug.motor_lines;
        extra += p.press_lines;
    }
    if (s.errors) {
        if (s.horizons < 1) {
            std::fprintf(stderr, "predict: --predict-errors needs --predict\n");
            return false;
        }
        p.first_error = plug.first_extra + extra;
        extra += 2 * p.targets;
    }
    if (extra != plug.settings.extra_lines) {
        std::fprintf(stderr, "predict: the plug has %d extra lines, %d wanted\n",
                     plug.settings.extra_lines, extra);
        return false;
    }
    p.words = blob.tile_count + 1;
    p.score_from = s.score_from >= 0 ? s.score_from : steps - steps / 4;
    p.saved_attached.assign((size_t)p.words, 0);
    p.saved_acted.assign((size_t)p.words, 0);
    p.score.horizons = s.horizons;
    p.score.targets = p.targets;
    for (int i = 0; i < s.horizons; i++) {
        p.score.horizon[i] = s.horizon[i];
    }
    p.on = true;
    if (s.horizons < 1) {
        return true;
    }
    if (blob.genome.learn_mode != 5 && blob.genome.learn_mode != 6) {
        std::fprintf(stderr, "predict: the predictor cells learn by the readout's rule; it "
                             "needs --g.learn_mode 5 (or 6)\n");
        return false;
    }
    p.bits = p.targets * s.horizons;
    if (p.bits < 1 || !pick_cells(p, plug, blob) || !hop_start(p, plug, blob)) {
        return false;
    }
    p.score.cells_per_group = p.cells_per_group;
    p.count.assign((size_t)2 * p.bits, 0);
    p.maps.assign((size_t)PREDICT_RING * HISTORY * p.words, 0);
    return true;
}

bool predict_attach(Predictor& p, TaskTicker& tt, Blob& blob, const RunFlags& flags) {
    p = Predictor{};
    if (!predict_wanted(flags.predict)) {
        return true;
    }
    if (flags.plug.rest_ticks > 0) {
        std::fprintf(stderr, "predict: not with --rest (a step's lines would be quiet)\n");
        return false;
    }
    PlugSettings with = flags.plug;
    with.extra_lines = extra_lines(flags.predict, tt.plug);
    if (with.extra_lines > 0) {
        plug_detach(tt.plug, blob);
        if (!plug_attach(tt.plug, blob, task_shape(tt.task), with)) {
            return false;
        }
    }
    return start(p, flags.predict, tt.plug, blob, flags.steps);
}

static uint8_t line_value(const Plug& plug, int line) {
    if (line == PLUG_GOOD_LINE) {
        return plug.good_left > 0;
    }
    if (line == PLUG_BAD_LINE) {
        return plug.bad_left > 0;
    }
    return plug.line_on[line] != 0;
}

static void mark_group(const Predictor& p, Blob& blob, int group, bool acted) {
    for (int k = 0; k < p.cells_per_group; k++) {
        int n = p.cell[(size_t)group * p.cells_per_group + k];
        blob.motor_attached[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
        if (acted) {
            blob.motor_acted[n / TILE_NEURONS] |= 1ull << (n % TILE_NEURONS);
        }
    }
}

// one verdict on horizon i's answers from ring slot `from`, for the targets in `which`
// (valence -1: the wrong ones, +1: the right ones). only these bits' cells are marked, each
// pair's answering group as pressed (a tie answers off); the world's own marks are put back
static void verdict(Predictor& p, Blob& blob, int i, int from, const uint8_t* which, int valence) {
    size_t bytes = (size_t)p.words * sizeof(uint64_t);
    std::memcpy(p.saved_attached.data(), blob.motor_attached, bytes);
    std::memcpy(p.saved_acted.data(), blob.motor_acted, bytes);
    std::memset(blob.motor_attached, 0, bytes);
    std::memset(blob.motor_acted, 0, bytes);
    for (int l = 0; l < p.targets; l++) {
        if (!which[l]) {
            continue;
        }
        int b = l * p.s.horizons + i;
        uint8_t said = p.said[from][i][l];
        mark_group(p, blob, 2 * b, said != 1);
        mark_group(p, blob, 2 * b + 1, said == 1);
    }
    uint64_t* maps[HISTORY];
    for (int j = 0; j < HISTORY; j++) {
        maps[j] = &p.maps[((size_t)from * HISTORY + j) * p.words];
    }
    // as in the tick after a reward: age 1 is the step's last tick
    int now = (p.map_now[from] + 1) & (HISTORY - 1);
    int shift = p.s.shift >= 0 ? p.s.shift : blob.genome.readout_shift;
    blob_readout_verdict(blob, valence, maps, now, shift);
    std::memcpy(blob.motor_attached, p.saved_attached.data(), bytes);
    std::memcpy(blob.motor_acted, p.saved_acted.data(), bytes);
    p.score.verdicts++;
}

static void tally(PredictTally& t, const uint8_t* was, const uint8_t* now, const uint8_t* said,
                  int targets) {
    t.steps++;
    for (int l = 0; l < targets; l++) {
        bool right = (said[l] == 1) == (now[l] == 1);
        t.total++;
        t.right += right;
        t.tied += said[l] == 2;
        t.same += was[l] == now[l];
        t.changed += was[l] != now[l];
        t.changed_right += was[l] != now[l] && right;
        t.on += now[l];
        t.on_right += now[l] && right;
        t.same_on += now[l] && was[l];
        t.off_right += !now[l] && right;
        t.line_on[l] += now[l];
    }
}

void predict_step_begin(Predictor& p, Plug& plug, Blob& blob) {
    int slot = (int)(p.step % PREDICT_RING);
    uint8_t* now = p.truth[slot];
    for (int l = 0; l < p.targets; l++) {
        now[l] = line_value(plug, p.target_line[l]);
        p.error[l] = 0;
    }
    uint8_t wrong[PREDICT_MAX_TARGETS], right[PREDICT_MAX_TARGETS];
    for (int i = 0; i < p.s.horizons; i++) {
        int h = p.s.horizon[i];
        if (p.step < h) {
            continue;
        }
        int from = (int)((p.step - h) % PREDICT_RING);
        const uint8_t* said = p.said[from][i];
        bool any_wrong = false, any_right = false;
        for (int l = 0; l < p.targets; l++) {
            wrong[l] = (said[l] == 1) != (now[l] == 1);
            right[l] = !wrong[l];
            any_wrong = any_wrong || wrong[l];
            any_right = any_right || right[l];
            if (i == 0) {
                p.error[l] = (int8_t)((int)now[l] - (said[l] == 1));
            }
        }
        if (p.step >= p.score_from) {
            tally(p.score.tally[i], p.truth[from], now, said, p.targets);
        }
        if (any_wrong) {
            verdict(p, blob, i, from, wrong, -1);
        }
        if (i == 0 && p.s.hop) {
            blob_hop_errors(blob, p.error, p.targets); // idea 164, taught on the next tick
        }
        if (any_right && blob.genome.readout_good) {
            verdict(p, blob, i, from, right, +1);
        }
    }
    for (int b = 0; b < p.press_lines; b++) {
        plug.line_on[p.first_press + b] = p.last_move == b;
    }
    if (p.first_error >= 0) {
        for (int l = 0; l < p.targets; l++) {
            plug.line_on[p.first_error + 2 * l] = p.error[l] < 0;
            plug.line_on[p.first_error + 2 * l + 1] = p.error[l] > 0;
        }
    }
}

void predict_before_tick(Predictor& p, TaskTicker& tt, Blob& blob) {
    if (!p.on || tt.tick_in_step != 0 || tt.rest_left > 0) {
        return;
    }
    // the ticker senses again on this tick and gets the same lines; the extra lines set
    // here are past the task's, which it never touches
    TaskSense sense;
    task_sense(tt.task, sense);
    plug_set_lines(tt.plug, sense);
    predict_step_begin(p, tt.plug, blob);
}

void predict_after_tick(Predictor& p, const TaskTicker& tt, const Blob& blob) {
    if (p.on && !tt.resting) {
        predict_count(p, blob);
    }
}

void predict_count(Predictor& p, const Blob& blob) {
    int groups = 2 * p.bits;
    for (int g = 0; g < groups; g++) {
        const int* cells = &p.cell[(size_t)g * p.cells_per_group];
        int c = 0;
        for (int k = 0; k < p.cells_per_group; k++) {
            c += fired(blob, cells[k]);
        }
        p.count[(size_t)g] = (uint16_t)(p.count[(size_t)g] + c);
    }
}

void predict_step_end(Predictor& p, const Blob& blob, int move) {
    if (!p.on) {
        return;
    }
    int slot = (int)(p.step % PREDICT_RING);
    for (int i = 0; i < p.s.horizons; i++) {
        for (int l = 0; l < p.targets; l++) {
            int b = l * p.s.horizons + i;
            int off = p.count[(size_t)2 * b], on = p.count[(size_t)2 * b + 1];
            p.said[slot][i][l] = on > off ? 1 : (on == off ? 2 : 0);
        }
    }
    for (uint16_t& c : p.count) {
        c = 0;
    }
    if (p.bits > 0) {
        for (int j = 0; j < HISTORY; j++) {
            std::memcpy(&p.maps[((size_t)slot * HISTORY + j) * p.words], blob.spikes[j],
                        (size_t)p.words * sizeof(uint64_t));
        }
        p.map_now[slot] = blob.now;
    }
    p.last_move = move;
    p.step++;
}
