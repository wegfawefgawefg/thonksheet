#include "world/task.hpp"

#include <cstring>

// names as typed on the blobrun command line
static const char* const task_names[(int)TaskKind::count] = {
    "echo",  "shuffled-echo", "delayed-echo", "go-no-go",        "beat", "sequence",
    "catch", "cartpole",      "order",        "both-not-either", "gap",  "place",
    // tiers 9 and 10
    "same", "same-then", "odd-one", "bigger", "analogy", "how-many", "more-fewer", "grammar",
    "brackets", "parts", "noisy-echo", "rare-cue", "noisy-lines", "context", "switch-rule",
    "copy", "recall", "n-back", "predict-next", "fill-in", "odd-event",
};

const char* task_name(TaskKind kind) {
    return task_names[(int)kind];
}

bool task_from_name(const char* name, TaskKind& kind) {
    for (int i = 0; i < (int)TaskKind::count; i++) {
        if (std::strcmp(name, task_names[i]) == 0) {
            kind = (TaskKind)i;
            return true;
        }
    }
    return false;
}

void task_reset(Task& task, TaskKind kind, const TaskSettings& settings, uint64_t seed) {
    task = Task{};
    task.kind = kind;
    task.settings = settings;
    world_random_seed(task.random, seed);
    if (script_kind(kind)) {
        script_reset(task);
        return;
    }
    switch (kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
    case TaskKind::delayed_echo:
    case TaskKind::go_no_go:
    case TaskKind::beat:
        reflex_reset(task);
        break;
    case TaskKind::sequence:
        sequence_reset(task);
        break;
    case TaskKind::catch_dot:
        catch_reset(task);
        break;
    case TaskKind::cartpole:
        cartpole_reset(task);
        break;
    case TaskKind::order:
    case TaskKind::both_not_either:
    case TaskKind::gap:
    case TaskKind::place:
        feature_reset(task);
        break;
    case TaskKind::count:
    default:
        break;
    }
}

TaskShape task_shape(const Task& task) {
    if (script_kind(task.kind)) {
        return script_shape(task);
    }
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
    case TaskKind::delayed_echo:
    case TaskKind::go_no_go:
    case TaskKind::beat:
        return reflex_shape(task);
    case TaskKind::sequence:
        return sequence_shape(task);
    case TaskKind::catch_dot:
        return catch_shape(task);
    case TaskKind::cartpole:
        return cartpole_shape(task);
    case TaskKind::order:
    case TaskKind::both_not_either:
    case TaskKind::gap:
    case TaskKind::place:
        return feature_shape(task);
    case TaskKind::count:
    default:
        break;
    }
    return TaskShape{};
}

void task_sense(const Task& task, TaskSense& sense) {
    sense = TaskSense{};
    if (script_kind(task.kind)) {
        script_sense(task, sense);
        return;
    }
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
    case TaskKind::delayed_echo:
    case TaskKind::go_no_go:
    case TaskKind::beat:
        reflex_sense(task, sense);
        break;
    case TaskKind::sequence:
        sequence_sense(task, sense);
        break;
    case TaskKind::catch_dot:
        catch_sense(task, sense);
        break;
    case TaskKind::cartpole:
        cartpole_sense(task, sense);
        break;
    case TaskKind::order:
    case TaskKind::both_not_either:
    case TaskKind::gap:
    case TaskKind::place:
        feature_sense(task, sense);
        break;
    case TaskKind::count:
    default:
        break;
    }
}

int task_perfect_move(const Task& task) {
    if (script_kind(task.kind)) {
        return script_perfect_move(task);
    }
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
    case TaskKind::delayed_echo:
    case TaskKind::go_no_go:
    case TaskKind::beat:
        return reflex_perfect_move(task);
    case TaskKind::sequence:
        return -1; // no motors; the perfect player predicts its input instead
    case TaskKind::catch_dot:
        return catch_perfect_move(task);
    case TaskKind::cartpole:
        return cartpole_perfect_move(task);
    case TaskKind::order:
    case TaskKind::both_not_either:
    case TaskKind::gap:
    case TaskKind::place:
        return feature_perfect_move(task);
    case TaskKind::count:
    default:
        break;
    }
    return -1;
}

int task_step(Task& task, const TaskStepIn& in) {
    int valence = 0;
    if (script_kind(task.kind)) {
        valence = script_step(task, in);
        task.steps++;
        return valence;
    }
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
    case TaskKind::delayed_echo:
    case TaskKind::go_no_go:
    case TaskKind::beat:
        valence = reflex_step(task, in);
        break;
    case TaskKind::sequence:
        valence = sequence_step(task, in);
        break;
    case TaskKind::catch_dot:
        valence = catch_step(task, in);
        break;
    case TaskKind::cartpole:
        valence = cartpole_step(task, in);
        break;
    case TaskKind::order:
    case TaskKind::both_not_either:
    case TaskKind::gap:
    case TaskKind::place:
        valence = feature_step(task, in);
        break;
    case TaskKind::count:
    default:
        break;
    }
    task.steps++;
    return valence;
}

static double ratio(int64_t num, int64_t den) {
    return den > 0 ? (double)num / (double)den : 0.0;
}

double task_window_score(Task& task) {
    if (script_kind(task.kind)) {
        return script_window_score(task);
    }
    const int64_t* t = task.tally;
    double score = 0.0;
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
    case TaskKind::delayed_echo:
    case TaskKind::go_no_go:
    case TaskKind::catch_dot:
    case TaskKind::sequence:
    case TaskKind::order:
    case TaskKind::both_not_either:
    case TaskKind::gap:
    case TaskKind::place:
        // right answers over chances to answer
        score = ratio(t[0], t[1]);
        break;
    case TaskKind::beat:
        // balanced: hit rate on due steps and hold rate on the rest, averaged
        score = 0.5 * (ratio(t[0], t[1]) + ratio(t[2], t[3]));
        break;
    case TaskKind::cartpole:
        // steps per fall; a window with no fall scores its length
        score = (double)t[0] / (double)(t[1] > 0 ? t[1] : 1);
        break;
    case TaskKind::count:
    default:
        break;
    }
    for (int i = 0; i < 4; i++) {
        task.tally[i] = 0;
    }
    return score;
}
