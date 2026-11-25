#include "world/task_names.hpp"

// per kind; anything not named here falls back to the caller's plain names

static const char* cue_name(int i) {
    static const char* const cues[16] = {"cue 0",  "cue 1",  "cue 2",  "cue 3", "cue 4",  "cue 5",
                                         "cue 6",  "cue 7",  "cue 8",  "cue 9", "cue 10", "cue 11",
                                         "cue 12", "cue 13", "cue 14", "cue 15"};
    return i >= 0 && i < 16 ? cues[i] : nullptr;
}

const char* task_single_name(const Task& task, int i) {
    switch (task.kind) {
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
        return cue_name(i);
    case TaskKind::delayed_echo:
        return i == task.settings.cue_offset + task.settings.k ? "ask" : cue_name(i);
    case TaskKind::go_no_go:
        return i == 0 ? "go" : "no-go";
    case TaskKind::order:
    case TaskKind::both_not_either: {
        static const char* const names[3] = {"A", "B", "ask"};
        return i < 3 ? names[i] : nullptr;
    }
    case TaskKind::gap:
        return i == 0 ? "cue" : "ask";
    case TaskKind::place:
        return "ask";
    default:
        return nullptr;
    }
}

const char* task_number_name(const Task& task, int i) {
    static const char* const cartpole[4] = {"cart x", "cart speed", "pole angle", "pole spin"};
    if (task.kind == TaskKind::cartpole && i < 4) {
        return cartpole[i];
    }
    return nullptr;
}

const char* task_motor_name(const Task& task, int i) {
    switch (task.kind) {
    case TaskKind::go_no_go:
    case TaskKind::beat:
        return i == 0 ? "press" : "rest";
    case TaskKind::catch_dot:
    case TaskKind::cartpole:
        return i == 0 ? "left" : "right";
    case TaskKind::order:
        return i == 0 ? "A first" : "B first";
    case TaskKind::both_not_either:
        return i == 0 ? "both/neither" : "just one";
    case TaskKind::gap:
        return i == 0 ? "short" : "long";
    case TaskKind::place:
        return i == 0 ? "shape \\" : "shape /";
    default:
        return nullptr;
    }
}
