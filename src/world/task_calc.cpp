#include "world/calc_ops.hpp"
#include "world/task.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

// more motor lines than the plug takes: it refuses the task
constexpr int PLUG_REFUSED_MOTORS = 1000;

// the calculation battery as a task (task_calc.hpp). the answer is read off the ask step
// as one bit per output pair. grading: whole (one good or bad for the whole answer, every
// output's cells marked) or per bit (bad when any bit is wrong, and only the wrong bits'
// cells marked, so the readout's error step lands on them alone). held-out cases give no
// good or bad at all.

static uint64_t mix(uint64_t z) {
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ull;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebull;
    return z ^ (z >> 31);
}

static bool is_held(const Task& task, uint64_t id) {
    const CalcState& c = task.calc;
    if (task.settings.calc.hold <= 0) {
        return false;
    }
    if (c.cases <= CALC_EXACT_CASES) {
        return (c.held_exact[id / 64] >> (id % 64)) & 1;
    }
    return (int)(mix(id ^ c.salt) & 15) < task.settings.calc.hold;
}

// a small case set holds out exactly cases * hold / 16 of its valid cases (at least one,
// never all), the ones whose hash with the life's salt is lowest. with `ball`, each case
// taken in that order brings every valid case within `ball` line flips of it along, so a
// held case's nearest seen case is at least ball + 1 flips away
static int flips(const CalcTrial& a, const CalcTrial& b) {
    int d = 0;
    for (int i = 0; i <= CALC_MAX_STEPS; i++) {
        d += __builtin_popcount(a.lines[i] ^ b.lines[i]);
    }
    return d;
}

static void choose_held(Task& task) {
    CalcState& c = task.calc;
    const CalcSettings& s = task.settings.calc;
    if (s.hold <= 0 || c.cases > CALC_EXACT_CASES) {
        return;
    }
    std::vector<std::pair<uint64_t, uint64_t>> valid;
    std::vector<CalcTrial> trial((size_t)c.cases);
    for (uint64_t id = 0; id < c.cases; id++) {
        if (calc_case(s, id, trial[id])) {
            valid.push_back({mix(id ^ c.salt), id});
        }
    }
    size_t want = valid.size() * (size_t)s.hold / 16;
    want = want < 1 ? 1 : want;
    want = want >= valid.size() ? valid.size() - 1 : want;
    std::sort(valid.begin(), valid.end());
    size_t held = 0;
    for (size_t i = 0; i < valid.size() && held < want; i++) {
        uint64_t centre = valid[i].second;
        for (size_t j = 0; j < valid.size() && held < valid.size() - 1; j++) {
            uint64_t id = valid[j].second;
            bool near = id == centre || flips(trial[centre], trial[id]) <= s.ball;
            if (near && !((c.held_exact[id / 64] >> (id % 64)) & 1)) {
                c.held_exact[id / 64] |= 1ull << (id % 64);
                held++;
            }
        }
    }
}

// the button form: one button per possible answer. every pattern of up to 4 bits, or the
// answers a small case set can give (place-coded geometry); false if more than 16
static bool choose_buttons(Task& task) {
    CalcState& c = task.calc;
    if (c.out_bits <= 4) {
        c.buttons = 1 << c.out_bits;
        for (int i = 0; i < c.buttons; i++) {
            c.answer_of[i] = (uint64_t)i;
        }
        return true;
    }
    if (c.cases > CALC_EXACT_CASES) {
        return false;
    }
    std::vector<uint64_t> seen;
    CalcTrial t;
    for (uint64_t id = 0; id < c.cases; id++) {
        if (calc_case(task.settings.calc, id, t)) {
            seen.push_back(t.answer);
        }
    }
    std::sort(seen.begin(), seen.end());
    seen.erase(std::unique(seen.begin(), seen.end()), seen.end());
    if (seen.size() > 16) {
        return false;
    }
    c.buttons = (int)seen.size();
    for (int i = 0; i < c.buttons; i++) {
        c.answer_of[i] = seen[(size_t)i];
    }
    return true;
}

static void new_trial(Task& task) {
    CalcState& c = task.calc;
    CalcTrial t;
    uint64_t id = 0;
    do {
        id = c.cases >= (1ull << 62) ? world_random_word(task.random) % c.cases
                                     : (world_random_word(task.random) >> 2) % c.cases;
    } while (!calc_case(task.settings.calc, id, t));
    c.steps = t.steps;
    for (int i = 0; i <= CALC_MAX_STEPS; i++) {
        c.step_lines[i] = t.lines[i];
    }
    c.answer = t.answer;
    c.case_id = id;
    c.held = is_held(task, id);
    c.button = -1;
    for (int i = 0; i < c.buttons; i++) {
        c.button = c.answer_of[i] == c.answer ? i : c.button;
    }
    task.phase = 0;
}

void calc_reset(Task& task) {
    CalcSettings& s = task.settings.calc;
    s.bits = s.bits < 1 ? 1 : (s.bits > 16 ? 16 : s.bits);
    s.length = s.length < 1 ? 1 : (s.length > CALC_MAX_STEPS - 1 ? CALC_MAX_STEPS - 1 : s.length);
    s.size = s.size < 2 ? 2 : (s.size > 8 ? 8 : s.size);
    s.list = s.list < 2 ? 2 : (s.list > 16 ? 16 : s.list);
    s.hold = s.hold < 0 ? 0 : (s.hold > 15 ? 15 : s.hold);
    CalcLayout l = calc_layout(s);
    CalcState& c = task.calc;
    c.lines = l.lines;
    c.out_bits = l.out_bits;
    c.cases = l.cases > 0 ? l.cases : 1;
    if (c.lines > CALC_MAX_LINES || c.out_bits > CALC_MAX_OUT || c.out_bits < 1) {
        std::fprintf(stderr, "calc: %s needs %d lines and %d output bits (at most %d, %d)\n",
                     calc_op_name(s.op), c.lines, c.out_bits, CALC_MAX_LINES, CALC_MAX_OUT);
        c.lines = c.lines > CALC_MAX_LINES ? CALC_MAX_LINES : c.lines;
        c.out_bits = c.out_bits > CALC_MAX_OUT ? CALC_MAX_OUT : (c.out_bits < 1 ? 1 : c.out_bits);
    }
    c.salt = world_random_word(task.random);
    s.ball = s.ball < 0 ? 0 : s.ball;
    choose_held(task);
    if (s.form == 1 && !choose_buttons(task)) {
        std::fprintf(stderr, "calc: %s has more than 16 possible answers; no button form\n",
                     calc_op_name(s.op));
        c.buttons = 0;
    }
    c.taught = c.cases > CALC_EXACT_CASES ? ~0u : 0u;
    CalcTrial t;
    for (uint64_t id = 0; s.form == 1 && c.cases <= CALC_EXACT_CASES && id < c.cases; id++) {
        if (calc_case(s, id, t) && !is_held(task, id)) {
            for (int i = 0; i < c.buttons; i++) {
                c.taught |= (uint32_t)(c.answer_of[i] == t.answer) << i;
            }
        }
    }
    new_trial(task);
}

TaskShape calc_shape(const Task& task) {
    TaskShape shape;
    shape.singles = task.calc.lines;
    shape.pattern_bits = task.calc.out_bits;
    shape.motors = 2 * task.calc.out_bits;
    if (task.settings.calc.form == 1) {
        // one button per answer; too many for the plug (it refuses) when there is no form
        shape.pattern_bits = 0;
        shape.motors = task.calc.buttons > 0 ? task.calc.buttons : PLUG_REFUSED_MOTORS;
    }
    return shape;
}

void calc_sense(const Task& task, TaskSense& sense) {
    uint32_t lines = task.calc.step_lines[task.phase];
    for (int i = 0; i < task.calc.lines; i++) {
        sense.singles[i] = (lines >> i) & 1;
    }
}

int calc_perfect_move(const Task& task) {
    if (task.phase != task.calc.steps - 1) {
        return -1;
    }
    return task.settings.calc.form == 1 ? task.calc.button : (int)task.calc.answer;
}

int calc_step(Task& task, const TaskStepIn& in) {
    CalcState& c = task.calc;
    task.pattern_graded = 0;
    if (task.phase < c.steps - 1) {
        task.phase++; // answers before the ask are ignored
        return 0;
    }
    uint64_t all = (1ull << c.out_bits) - 1;
    uint64_t wrong = ((in.pattern ^ c.answer) | in.pattern_tied) & all;
    bool buttons = task.settings.calc.form == 1;
    if (buttons) {
        // the pressed button's answer, bit by bit; no press is every bit wrong
        bool pressed = in.move >= 0 && in.move < c.buttons;
        wrong = pressed ? (c.answer_of[in.move] ^ c.answer) & all : all;
    }
    int h = c.held ? 1 : 0;
    if (buttons && c.held && c.button >= 0 && !((c.taught >> c.button) & 1)) {
        new_trial(task); // an answer no seen case has: not scored
        return 0;
    }
    c.trials[h]++;
    c.whole[h] += wrong == 0;
    c.bits[h] += c.out_bits - __builtin_popcountll(wrong);
    for (int b = 0; b < c.out_bits; b++) {
        c.bit_right[h][b] += !((wrong >> b) & 1);
    }
    int valence = wrong ? -1 : 1;
    valence = buttons && in.move < 0 ? 0 : valence; // no press is a miss, as with buttons
    // per bit: the verdict goes to the wrong bits only (all of them when all are right)
    task.pattern_graded = task.settings.calc.grade == 1 && wrong ? wrong : all;
    if (c.held) {
        valence = 0;
        task.pattern_graded = 0;
    }
    new_trial(task);
    return valence;
}

static double ratio(int64_t num, int64_t den) {
    return den > 0 ? (double)num / (double)den : 0.0;
}

// held-out scores: not a number when there was nothing held out to score
static double or_none(int64_t num, int64_t den) {
    return den > 0 ? (double)num / (double)den : std::nan("");
}

// score: share of output bits right on seen cases. extras: whole answers right on seen
// cases, bits right on held-out ones, whole answers right on held-out ones, then each bit
// on seen cases, then each bit on held-out ones
double calc_window_score(Task& task) {
    CalcState& c = task.calc;
    int out = c.out_bits;
    double score = ratio(c.bits[0], c.trials[0] * out);
    task.extra[0] = ratio(c.whole[0], c.trials[0]);
    task.extra[1] = or_none(c.bits[1], c.trials[1] * out);
    task.extra[2] = or_none(c.whole[1], c.trials[1]);
    for (int b = 0; b < out; b++) {
        task.extra[3 + b] = ratio(c.bit_right[0][b], c.trials[0]);
        task.extra[3 + out + b] = or_none(c.bit_right[1][b], c.trials[1]);
    }
    task.extras = 3 + 2 * out;
    for (int h = 0; h < 2; h++) {
        c.trials[h] = c.whole[h] = c.bits[h] = 0;
        for (int b = 0; b < CALC_MAX_OUT; b++) {
            c.bit_right[h][b] = 0;
        }
    }
    return score;
}

void task_extra_name(const Task& task, int i, char* out, int size) {
    int bits = task.calc.out_bits;
    static const char* const first[3] = {"whole_seen", "bits_held", "whole_held"};
    if (i < 3) {
        std::snprintf(out, (size_t)size, "%s", first[i]);
    } else if (i < 3 + bits) {
        std::snprintf(out, (size_t)size, "bit%d_seen", i - 3);
    } else {
        std::snprintf(out, (size_t)size, "bit%d_held", i - 3 - bits);
    }
}
