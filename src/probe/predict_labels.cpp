#include "probe/predict_labels.hpp"

// blobprobe predict's questions (predict_labels.hpp)

static void add(ReadLabels& out, const char* name, int classes, int value) {
    out.name[out.count] = name;
    out.classes[out.count] = classes;
    out.value[out.count] = value;
    out.count++;
}

// the paddle or marker of catch and track
static int place_of(const Task& task) {
    return task.kind == TaskKind::catch_dot ? task.paddle_x : task.marker;
}

bool read_labels(const Task& task, const ReadMemory& m, ReadLabels& out) {
    out = ReadLabels{};
    if (m.steps < 2) {
        return false;
    }
    int k = task.settings.k;
    int none = 0;
    switch (task.kind) {
    case TaskKind::catch_dot:
    case TaskKind::track: {
        int d = place_of(task) - m.place;
        add(out, "moved", 3, d < 0 ? 0 : (d > 0 ? 2 : 1));
        break;
    }
    case TaskKind::forage:
        none = 4;
        add(out, "last_move", 5, m.move < 0 ? none : m.move);
        break;
    case TaskKind::cartpole:
    case TaskKind::pendulum:
    case TaskKind::mountain_car:
        add(out, "last_push", 3, m.move < 0 ? 2 : m.move);
        break;
    case TaskKind::chain:
        add(out, "last_press", k + 1, m.move < 0 ? k : m.move);
        break;
    case TaskKind::delayed_echo:
        add(out, "since_cue", task.settings.delay + 2, task.phase);
        add(out, "cue", k, task.cue);
        break;
    case TaskKind::echo:
    case TaskKind::shuffled_echo:
        add(out, "last_cue", k, m.cue);
        break;
    case TaskKind::predict_next: {
        int next = task_perfect_move(task);
        if (next < 0) {
            return false;
        }
        add(out, "next", k, next);
        add(out, "before", k, m.next[1]);
        break;
    }
    case TaskKind::beat:
        add(out, "beat_phase", task.settings.period, (int)(task.steps % task.settings.period));
        break;
    default:
        return false;
    }
    return true;
}

void read_remember(const Task& task, ReadMemory& m, int move) {
    if (task.kind == TaskKind::catch_dot || task.kind == TaskKind::track) {
        m.place = place_of(task);
    }
    if (task.kind == TaskKind::predict_next) {
        m.next[1] = m.next[0];
        m.next[0] = task_perfect_move(task);
    }
    m.move = move;
    m.cue = task.cue;
    m.steps++;
}
