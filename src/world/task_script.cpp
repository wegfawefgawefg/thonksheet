#include "world/task_script.hpp"

// the trial script runner for tiers 9 and 10 (task_script.hpp lists the families). a step's
// press counts only where the script wants a button; scored steps feed the score, rewarded
// ones get good or bad, probe steps are kept by blobprobe linear with the wanted button as
// the label. the no-teacher kinds keep their own counts in task.extra.

enum class Family { relation, sheet_number, grammar, chance, memory, unsupervised, none };

static Family family_of(TaskKind kind) {
    switch (kind) {
    case TaskKind::same:
    case TaskKind::same_then:
    case TaskKind::odd_one:
    case TaskKind::bigger:
    case TaskKind::analogy:
        return Family::relation;
    case TaskKind::how_many:
    case TaskKind::more_fewer:
    case TaskKind::parts:
        return Family::sheet_number;
    case TaskKind::grammar:
    case TaskKind::brackets:
        return Family::grammar;
    case TaskKind::noisy_echo:
    case TaskKind::rare_cue:
    case TaskKind::noisy_lines:
    case TaskKind::context:
    case TaskKind::switch_rule:
        return Family::chance;
    case TaskKind::copy:
    case TaskKind::recall:
    case TaskKind::n_back:
        return Family::memory;
    case TaskKind::predict_next:
    case TaskKind::fill_in:
    case TaskKind::odd_event:
        return Family::unsupervised;
    default:
        return Family::none;
    }
}

bool script_kind(TaskKind kind) {
    return family_of(kind) != Family::none;
}

uint32_t random_pattern(WorldRandom& r, int n, int w) {
    uint32_t bits = 0;
    int left = w < n ? w : n;
    while (left > 0) {
        uint32_t b = line_bit(world_random_below(r, n));
        if (!(bits & b)) {
            bits |= b;
            left--;
        }
    }
    return bits;
}

int all_patterns(int n, int w, int* out, int most) {
    int count = 0;
    for (uint32_t bits = 0; bits < (1u << n) && count < most; bits++) {
        if (__builtin_popcount(bits) == w) {
            out[count++] = (int)bits;
        }
    }
    return count;
}

static void new_trial(Task& task) {
    script_clear(task);
    switch (family_of(task.kind)) {
    case Family::relation:
        relation_trial(task);
        break;
    case Family::sheet_number:
        sheet_number_trial(task);
        break;
    case Family::grammar:
        grammar_trial(task);
        break;
    case Family::chance:
        chance_trial(task);
        break;
    case Family::memory:
        memory_trial(task);
        break;
    case Family::unsupervised:
        unsupervised_trial(task);
        break;
    case Family::none:
        break;
    }
    if (task.script.len == 0) {
        script_add(task, 0, -1, 0); // never an empty trial
    }
}

void script_reset(Task& task) {
    TaskSettings& s = task.settings;
    s.k = s.k < 2 ? 2 : (s.k > TASK_MAX_MOTORS ? TASK_MAX_MOTORS : s.k);
    s.item_lines = s.item_lines < 2 ? 2 : (s.item_lines > 16 ? 16 : s.item_lines);
    s.item_on = s.item_on < 1 ? 1 : (s.item_on >= s.item_lines ? s.item_lines - 1 : s.item_on);
    s.hold = s.hold < 0 ? 0 : s.hold;
    s.rare = s.rare < 2 ? 2 : s.rare;
    s.switch_every = s.switch_every < 1 ? 1 : s.switch_every;
    s.back = s.back < 1 ? 1 : (s.back > 8 ? 8 : s.back);
    s.fill = s.fill < 0 ? 0 : (s.fill > SCRIPT_MAX - 4 ? SCRIPT_MAX - 4 : s.fill);
    switch (family_of(task.kind)) {
    case Family::relation:
        relation_reset(task);
        break;
    case Family::sheet_number:
        sheet_number_reset(task);
        break;
    case Family::grammar:
        grammar_reset(task);
        break;
    case Family::chance:
        chance_reset(task);
        break;
    case Family::memory:
        memory_reset(task);
        break;
    case Family::unsupervised:
        unsupervised_reset(task);
        break;
    case Family::none:
        break;
    }
    new_trial(task);
}

TaskShape script_shape(const Task& task) {
    switch (family_of(task.kind)) {
    case Family::relation:
        return relation_shape(task);
    case Family::sheet_number:
        return sheet_number_shape(task);
    case Family::grammar:
        return grammar_shape(task);
    case Family::chance:
        return chance_shape(task);
    case Family::memory:
        return memory_shape(task);
    case Family::unsupervised:
        return unsupervised_shape(task);
    case Family::none:
        break;
    }
    return TaskShape{};
}

void script_sense(const Task& task, TaskSense& sense) {
    const Task::Script& s = task.script;
    int at = task.phase;
    for (int i = 0; i < TASK_MAX_SINGLES; i++) {
        sense.singles[i] = (s.lines[at] >> i) & 1;
    }
    TaskShape shape = script_shape(task);
    int cells = shape.sheet_w * shape.sheet_h;
    for (int i = 0; i < cells && i < 64 * SCRIPT_SHEET_WORDS; i++) {
        sense.sheet[i] = (s.sheet[at][i / 64] >> (i % 64)) & 1;
    }
    sense.hidden = s.hidden[at];
    sense.hidden_on = s.hidden_on[at];
}

int script_perfect_move(const Task& task) {
    return task.script.want[task.phase];
}

// the no-teacher kinds: what the sheet's own predictions did this step
static void count_unsupervised(Task& task, const TaskStepIn& in, int at) {
    int64_t* e = task.extra;
    int wrong = in.surprise < in.sensed_neurons ? in.surprise : in.sensed_neurons;
    switch (task.kind) {
    case TaskKind::predict_next:
        // share of attached sensor neurons predicted right on the step's first tick
        e[0] += in.sensed_neurons - wrong;
        e[1] += in.sensed_neurons;
        break;
    case TaskKind::fill_in:
        // hidden cells: predicted on when truly on, against predicted on when truly off
        e[0] += in.fill_hit;
        e[1] += in.fill_on;
        e[2] += in.fill_false;
        e[3] += in.fill_off;
        break;
    case TaskKind::odd_event: {
        // wrong predictions per sensed neuron, on breaks against on regular steps
        bool odd = task.script.flags[at] & SCRIPT_SCORED;
        e[odd ? 0 : 2] += wrong;
        e[odd ? 1 : 3] += in.sensed_neurons;
        break;
    }
    default:
        break;
    }
}

int script_step(Task& task, const TaskStepIn& in) {
    const Task::Script& s = task.script;
    int at = task.phase;
    int want = s.want[at];
    uint8_t flags = s.flags[at];
    int valence = 0;
    if (family_of(task.kind) == Family::unsupervised) {
        count_unsupervised(task, in, at);
    } else if (want >= 0 && (flags & (SCRIPT_SCORED | SCRIPT_REWARDED))) {
        bool right = in.move == want;
        // tally[0..1] the score's answers, tally[2..3] rewarded ones it leaves out; rare cue
        // keeps its two cues apart for a balanced score
        int slot = (flags & SCRIPT_SCORED) ? 0 : 2;
        if (task.kind == TaskKind::rare_cue) {
            slot = want == 1 ? 0 : 2;
        }
        task.tally[slot + 1]++;
        task.tally[slot] += right;
        if (flags & SCRIPT_REWARDED) {
            valence = in.move < 0 ? 0 : (right ? 1 : -1);
            if (task.kind == TaskKind::noisy_echo) {
                valence = chance_valence(task, right, in.move, valence);
            }
        }
    }
    if (flags & SCRIPT_PROBE) {
        task.probe_marks++;
    }
    task.phase++;
    if (task.phase >= s.len) {
        new_trial(task);
    }
    return valence;
}

static double ratio(int64_t num, int64_t den) {
    return den > 0 ? (double)num / (double)den : 0.0;
}

double script_window_score(Task& task) {
    const int64_t* t = task.tally;
    const int64_t* e = task.extra;
    double score = 0.0;
    switch (task.kind) {
    case TaskKind::rare_cue:
        // balanced: the rare cue's hit rate and the common one's, averaged
        score = 0.5 * (ratio(t[0], t[1]) + ratio(t[2], t[3]));
        break;
    case TaskKind::predict_next:
        score = ratio(e[0], e[1]);
        break;
    case TaskKind::fill_in:
    case TaskKind::odd_event:
        // hit rate less false alarms (fill-in); surprise on breaks less on the rest (odd)
        score = ratio(e[0], e[1]) - ratio(e[2], e[3]);
        break;
    default:
        score = ratio(t[0], t[1]);
        break;
    }
    for (int i = 0; i < 4; i++) {
        task.tally[i] = 0;
        task.extra[i] = 0;
    }
    return score;
}
