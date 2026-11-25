#include "world/task.hpp"

// more worlds (idea 63): moving to a place, with feedback each step or only on arrival.
//
// track   a track_size x 2 bitmap: the target's cell lit on the top row, the marker's on
//         the bottom row. buttons 0 left, 1 right; no move, or a move into the end, stays.
//         track_reward 0: good when the marker ends a step nearer the target, bad when
//         further. 1: good only on arrival, bad if track_size steps pass without one. on
//         arrival the target jumps to another cell (never the marker's).
// forage  a forage_size square grid that wraps at the edges, seen from the animal, which
//         is always the middle cell: the bitmap shows only where the food is. buttons 0
//         up, 1 down, 2 left, 3 right; no move stays. forage_warmer 1: good when a step
//         ends nearer the food (steps along the grid, the short way round), bad when
//         further. 0: good only on reaching it, bad if 2 x (size - 1) steps pass without
//         (the food then moves). the food reappears at another random cell.
//
// both score: share of steps that ended nearer the target (an arrival counts), so the
// scripted player that always steps toward it scores 1.0 and standing still scores 0.

constexpr int TRACK_MAX = 16;

static int clamp(int v, int lo, int hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int absolute(int v) {
    return v < 0 ? -v : v;
}

// forage: offsets run -half..half; the grid has an odd side, so the short way is unique
static int wrap(int d, int size) {
    int half = size / 2;
    return d > half ? d - size : (d < -half ? d + size : d);
}

static int distance(const Task& task) {
    if (task.kind == TaskKind::track) {
        return absolute(task.target - task.marker);
    }
    return absolute(task.food_dx) + absolute(task.food_dy);
}

static void new_target(Task& task) {
    task.timer = 0;
    if (task.kind == TaskKind::track) {
        int n = task.settings.track_size;
        task.target = (task.marker + 1 + world_random_below(task.random, n - 1)) % n;
        return;
    }
    int size = task.settings.forage_size;
    int cell = 1 + world_random_below(task.random, size * size - 1); // never the animal's
    int half = size / 2;
    task.food_dx = (cell % size + half) % size - half;
    task.food_dy = (cell / size + half) % size - half;
}

void move_reset(Task& task) {
    TaskSettings& s = task.settings;
    s.track_size = clamp(s.track_size, 2, TRACK_MAX);
    s.forage_size = clamp(s.forage_size | 1, 3, 15);
    task.marker = s.track_size / 2;
    new_target(task);
}

TaskShape move_shape(const Task& task) {
    TaskShape shape;
    if (task.kind == TaskKind::track) {
        shape.sheet_w = task.settings.track_size;
        shape.sheet_h = 2;
        shape.motors = 2;
        return shape;
    }
    shape.sheet_w = task.settings.forage_size;
    shape.sheet_h = task.settings.forage_size;
    shape.motors = 4;
    return shape;
}

void move_sense(const Task& task, TaskSense& sense) {
    if (task.kind == TaskKind::track) {
        int n = task.settings.track_size;
        sense.sheet[task.target] = 1;
        sense.sheet[n + task.marker] = 1;
        return;
    }
    int size = task.settings.forage_size;
    int half = size / 2;
    sense.sheet[(half + task.food_dy) * size + half + task.food_dx] = 1;
}

int move_perfect_move(const Task& task) {
    if (task.kind == TaskKind::track) {
        return task.target < task.marker ? 0 : 1;
    }
    // along whichever axis is further off
    if (absolute(task.food_dx) >= absolute(task.food_dy)) {
        return task.food_dx < 0 ? 2 : 3;
    }
    return task.food_dy < 0 ? 0 : 1;
}

static void apply_move(Task& task, int move) {
    if (task.kind == TaskKind::track) {
        int n = task.settings.track_size;
        if (move == 0 && task.marker > 0) {
            task.marker--;
        }
        if (move == 1 && task.marker < n - 1) {
            task.marker++;
        }
        return;
    }
    // the animal moves, so the food moves the other way in its view
    int size = task.settings.forage_size;
    int dx = move == 2 ? 1 : (move == 3 ? -1 : 0);
    int dy = move == 0 ? 1 : (move == 1 ? -1 : 0);
    task.food_dx = wrap(task.food_dx + dx, size);
    task.food_dy = wrap(task.food_dy + dy, size);
}

int move_step(Task& task, const TaskStepIn& in) {
    int before = distance(task);
    apply_move(task, in.move);
    int after = distance(task);
    task.timer++;
    task.tally[1]++;
    task.tally[0] += after < before;
    bool shaped = task.kind == TaskKind::track ? task.settings.track_reward == 0
                                               : task.settings.forage_warmer != 0;
    if (after == 0) {
        task.episodes++;
        task.wins++;
        new_target(task);
        return 1;
    }
    if (shaped) {
        return after < before ? 1 : (after > before ? -1 : 0);
    }
    int limit = task.kind == TaskKind::track ? task.settings.track_size
                                             : 2 * (task.settings.forage_size - 1);
    if (task.timer >= limit) {
        task.episodes++;
        new_target(task);
        return -1;
    }
    return 0;
}
