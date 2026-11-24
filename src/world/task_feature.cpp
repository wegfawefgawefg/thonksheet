#include "world/task.hpp"

// features nobody designed in (docs/bench.md, idea 49): tasks whose answer no single input
// line carries. every trial is a short script: one blank step that parts it from the last
// trial, the steps that show it, and a last step with the ask line on, which is the only
// step whose press counts. two buttons; the right one is good, the other bad, no move a
// miss. score: right answers over trials.
//
// order            lines A, B, ask. the first cue alone for order_gap steps, then both on
//                  for one step, then the ask with both off. button 0 if A came first, 1 if
//                  B did. both cues were last seen at the same moment, so only the order in
//                  which they came on tells the answer.
// both-not-either  lines A, B, ask. A and B are each on or off by coin flip, shown with the
//                  ask line for one step. button 1 if exactly one is on, 0 if both or
//                  neither (exclusive or).
// gap              lines cue, ask. the cue blinks for one step, then gap_short or gap_long
//                  blank steps, then blinks again, then the ask. button 0 short, 1 long.
// place            a place_size square sheet, plus the ask line. one of two diagonal
//                  pairs ("\" button 0, "/" button 1) appears with the ask at one of the
//                  (size - 1)^2 places. place_hold places are drawn per life and never
//                  rewarded; only trials there are scored, so the score is carrying the
//                  shape to a place it was never taught at. every single pixel is lit by
//                  both shapes at some place, so no one line tells the shape.

constexpr int FEATURE_A = 0, FEATURE_B = 1;

static int clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int place_count(const Task& task) {
    int span = task.settings.place_size - 1;
    return span * span;
}

static uint32_t bit(int line) {
    return 1u << line;
}

static void new_trial(Task& task) {
    const TaskSettings& s = task.settings;
    WorldRandom& r = task.random;
    uint32_t* steps = task.trial_lines;
    for (int i = 0; i < TASK_MAX_TRIAL; i++) {
        steps[i] = 0;
    }
    task.phase = 0;
    task.scored = true;
    int n = 1; // step 0 is the blank that parts trials
    switch (task.kind) {
    case TaskKind::order: {
        int ask = 2;
        int first = world_random_below(r, 2);
        for (int i = 0; i < s.order_gap; i++) {
            steps[n++] = bit(first);
        }
        steps[n++] = bit(FEATURE_A) | bit(FEATURE_B);
        steps[n++] = bit(ask);
        task.cue = first;
        break;
    }
    case TaskKind::both_not_either: {
        int ask = 2;
        int a = world_random_below(r, 2);
        int b = world_random_below(r, 2);
        steps[n++] = (a ? bit(FEATURE_A) : 0) | (b ? bit(FEATURE_B) : 0) | bit(ask);
        task.cue = a ^ b;
        break;
    }
    case TaskKind::gap: {
        int cue = 0, ask = 1;
        int is_long = world_random_below(r, 2);
        steps[n++] = bit(cue);
        n += is_long ? s.gap_long : s.gap_short;
        steps[n++] = bit(cue);
        steps[n++] = bit(ask);
        task.cue = is_long;
        break;
    }
    case TaskKind::place: {
        int ask = 0;
        task.shape = world_random_below(r, 2);
        task.place = world_random_below(r, place_count(task));
        task.scored = (task.held_out >> task.place) & 1;
        steps[n++] = bit(ask);
        task.cue = task.shape;
        break;
    }
    default:
        break;
    }
    task.trial_len = n;
}

void feature_reset(Task& task) {
    TaskSettings& s = task.settings;
    s.order_gap = clamp(s.order_gap, 1, TASK_MAX_TRIAL - 3);
    s.gap_short = clamp(s.gap_short, 0, TASK_MAX_TRIAL - 4);
    s.gap_long = clamp(s.gap_long, s.gap_short + 1, TASK_MAX_TRIAL - 4);
    s.gap_short = s.gap_short >= s.gap_long ? s.gap_long - 1 : s.gap_short;
    // at most 8 x 8 places so a 64-bit word holds which are held out
    s.place_size = clamp(s.place_size, 3, 9);
    s.place_hold = clamp(s.place_hold, 1, place_count(task) - 1);
    if (task.kind == TaskKind::place) {
        // the held-out places, drawn once per life
        int left = s.place_hold;
        while (left > 0) {
            int p = world_random_below(task.random, place_count(task));
            if (!((task.held_out >> p) & 1)) {
                task.held_out |= 1ull << p;
                left--;
            }
        }
    }
    new_trial(task);
}

TaskShape feature_shape(const Task& task) {
    TaskShape shape;
    shape.motors = 2;
    switch (task.kind) {
    case TaskKind::order:
    case TaskKind::both_not_either:
        shape.singles = 3; // A, B, ask
        break;
    case TaskKind::gap:
        shape.singles = 2; // cue, ask
        break;
    case TaskKind::place:
        shape.singles = 1; // ask
        shape.sheet_w = task.settings.place_size;
        shape.sheet_h = task.settings.place_size;
        break;
    default:
        break;
    }
    return shape;
}

void feature_sense(const Task& task, TaskSense& sense) {
    uint32_t lines = task.trial_lines[task.phase];
    for (int i = 0; i < 3; i++) {
        sense.singles[i] = (lines >> i) & 1;
    }
    bool answering = task.phase == task.trial_len - 1;
    if (task.kind == TaskKind::place && answering) {
        int size = task.settings.place_size;
        int x = task.place % (size - 1), y = task.place / (size - 1);
        // "\" lights (0,0) and (1,1) of its 2x2 corner, "/" lights (1,0) and (0,1)
        int flip = task.shape;
        sense.sheet[y * size + x + flip] = 1;
        sense.sheet[(y + 1) * size + x + 1 - flip] = 1;
    }
}

int feature_perfect_move(const Task& task) {
    return task.phase == task.trial_len - 1 ? task.cue : -1;
}

int feature_step(Task& task, const TaskStepIn& in) {
    if (task.phase < task.trial_len - 1) {
        task.phase++; // presses before the ask are ignored
        return 0;
    }
    bool right = in.move == task.cue;
    int valence = in.move < 0 ? 0 : (right ? 1 : -1);
    if (task.scored) {
        task.tally[1]++;
        task.tally[0] += right;
    } else {
        // place, a taught place: rewarded, counted apart from the score
        task.tally[3]++;
        task.tally[2] += right;
    }
    // place: held-out places are never rewarded
    if (task.kind == TaskKind::place && task.scored) {
        valence = 0;
    }
    new_trial(task);
    return valence;
}
