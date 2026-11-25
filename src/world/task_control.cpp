#include "world/task.hpp"

// stage 3: catch and cartpole. all physics in integers so a run repeats exactly.

// --- catch ---
// a dot falls one row per step down a catch_w x catch_h sheet; the paddle sits on the bottom
// row. motors: 0 left, 1 right, no move stays. when the dot reaches the bottom row it is
// judged (good if the paddle is under it, bad if not) and a new dot starts at the top. a dot
// needs catch_h - 1 steps to fall, so with catch_w <= catch_h every dot can be caught.

static void catch_new_dot(Task& task) {
    task.dot_x = world_random_below(task.random, task.settings.catch_w);
    task.dot_y = 0;
}

void catch_reset(Task& task) {
    int& w = task.settings.catch_w;
    int& h = task.settings.catch_h;
    w = w < 2 ? 2 : w;
    h = h < 2 ? 2 : h;
    if (w * h > TASK_MAX_SHEET) {
        w = 6;
        h = 6;
    }
    task.paddle_x = w / 2;
    catch_new_dot(task);
}

TaskShape catch_shape(const Task& task) {
    TaskShape shape;
    shape.sheet_w = task.settings.catch_w;
    shape.sheet_h = task.settings.catch_h;
    shape.motors = 2; // left, right
    return shape;
}

void catch_sense(const Task& task, TaskSense& sense) {
    int w = task.settings.catch_w;
    int h = task.settings.catch_h;
    sense.sheet[task.dot_y * w + task.dot_x] = 1;
    sense.sheet[(h - 1) * w + task.paddle_x] = 1;
}

int catch_perfect_move(const Task& task) {
    if (task.dot_x < task.paddle_x) {
        return 0;
    }
    if (task.dot_x > task.paddle_x) {
        return 1;
    }
    return -1;
}

int catch_step(Task& task, const TaskStepIn& in) {
    if (in.move == 0 && task.paddle_x > 0) {
        task.paddle_x--;
    }
    if (in.move == 1 && task.paddle_x < task.settings.catch_w - 1) {
        task.paddle_x++;
    }
    task.dot_y++;
    if (task.dot_y < task.settings.catch_h - 1) {
        return 0;
    }
    task.tally[1]++;
    task.episodes++;
    bool caught = task.paddle_x == task.dot_x;
    catch_new_dot(task);
    if (caught) {
        task.tally[0]++;
        task.wins++;
        return 1;
    }
    return -1;
}

// --- cartpole ---
// the classic cart and pole (gym constants) in 16.16 fixed point, Euler steps of 0.02 s.
// motors: 0 push left, 1 push right, no move pushes nothing. a fall is the pole past 12
// degrees or the cart past 2.4 m: bad, and a fresh start near upright. every 25 steps
// without a fall is good. sensed as four numbers through threshold lines.

constexpr int64_t fix(double v) {
    return (int64_t)(v * 65536.0 + (v < 0 ? -0.5 : 0.5));
}

constexpr int64_t CP_GRAVITY = fix(9.8);
constexpr int64_t CP_MASS_POLE = fix(0.1);
constexpr int64_t CP_MASS_TOTAL = fix(1.1);
constexpr int64_t CP_HALF_LENGTH = fix(0.5);
constexpr int64_t CP_POLE_MASS_LENGTH = fix(0.05);
constexpr int64_t CP_FORCE = fix(10.0);
constexpr int64_t CP_TAU = fix(0.02);
constexpr int64_t CP_FOUR_THIRDS = fix(4.0 / 3.0);
constexpr int64_t CP_X_LIMIT = fix(2.4);
constexpr int64_t CP_ANGLE_LIMIT = fix(0.2094395); // 12 degrees
constexpr int64_t CP_START = fix(0.05);
constexpr int CP_GOOD_EVERY = 25;
constexpr int CP_LINES = 6;

static int64_t fmul(int64_t a, int64_t b) {
    return (a * b) >> 16;
}

static int64_t fdiv(int64_t a, int64_t b) {
    return (a * FIX_ONE) / b;
}

static int64_t start_value(Task& task, int64_t range = CP_START) {
    return world_random_below(task.random, (int)(2 * range + 1)) - range;
}

// cart_start 1 (wide): the cart anywhere within 1 m of the middle, speed and spin within
// 0.5, lean within 0.1 rad (half the way to a fall). the scripted player still holds these.
static void cartpole_restart(Task& task) {
    bool wide = task.settings.cart_start == 1;
    task.cart_x = start_value(task, wide ? fix(1.0) : CP_START);
    task.cart_v = start_value(task, wide ? fix(0.5) : CP_START);
    task.pole_a = start_value(task, wide ? fix(0.1) : CP_START);
    task.pole_w = start_value(task, wide ? fix(0.5) : CP_START);
    task.alive = 0;
}

void cartpole_reset(Task& task) {
    cartpole_restart(task);
}

TaskShape cartpole_shape(const Task& task) {
    (void)task;
    TaskShape shape;
    shape.numbers = 4;
    shape.number_lines = CP_LINES;
    // cart position, cart speed, pole angle, pole spin
    shape.number_lo[0] = (int32_t)-CP_X_LIMIT;
    shape.number_hi[0] = (int32_t)CP_X_LIMIT;
    shape.number_lo[1] = (int32_t)-fix(1.5);
    shape.number_hi[1] = (int32_t)fix(1.5);
    shape.number_lo[2] = (int32_t)-CP_ANGLE_LIMIT;
    shape.number_hi[2] = (int32_t)CP_ANGLE_LIMIT;
    shape.number_lo[3] = (int32_t)-fix(2.0);
    shape.number_hi[3] = (int32_t)fix(2.0);
    shape.motors = 2; // left, right
    return shape;
}

void cartpole_sense(const Task& task, TaskSense& sense) {
    sense.numbers[0] = (int32_t)task.cart_x;
    sense.numbers[1] = (int32_t)task.cart_v;
    sense.numbers[2] = (int32_t)task.pole_a;
    sense.numbers[3] = (int32_t)task.pole_w;
}

int cartpole_perfect_move(const Task& task) {
    // a linear push rule on the true state: lean and spin first, then bring the cart home
    int64_t u = task.pole_a + fmul(fix(0.5), task.pole_w) + fmul(fix(0.05), task.cart_x) +
                fmul(fix(0.15), task.cart_v);
    return u > 0 ? 1 : 0;
}

// how far from upright and centred: the lean, plus the cart's offset counted about half as
// much across its range (0.209 rad of lean against 2.4 m of track)
constexpr int64_t CP_CENTRE_WEIGHT = fix(0.04);
constexpr int64_t CP_HOME_BAND = fix(0.02); // about one degree with the cart at the middle

static int64_t cartpole_off(const Task& task) {
    int64_t lean = task.pole_a < 0 ? -task.pole_a : task.pole_a;
    int64_t offset = task.cart_x < 0 ? -task.cart_x : task.cart_x;
    return lean + fmul(CP_CENTRE_WEIGHT, offset);
}

int cartpole_step(Task& task, const TaskStepIn& in) {
    int64_t before = cartpole_off(task);
    int64_t force = 0;
    if (in.move == 0) {
        force = -CP_FORCE;
    } else if (in.move == 1) {
        force = CP_FORCE;
    }
    // small-angle sine and cosine, good to well under 0.1% inside 12 degrees
    int64_t a = task.pole_a;
    int64_t a2 = fmul(a, a);
    int64_t sin_a = a - fmul(a2, a) / 6;
    int64_t cos_a = FIX_ONE - a2 / 2 + fmul(a2, a2) / 24;

    int64_t w = task.pole_w;
    int64_t temp = fdiv(force + fmul(CP_POLE_MASS_LENGTH, fmul(fmul(w, w), sin_a)), CP_MASS_TOTAL);
    int64_t lever =
        fmul(CP_HALF_LENGTH,
             CP_FOUR_THIRDS - fdiv(fmul(CP_MASS_POLE, fmul(cos_a, cos_a)), CP_MASS_TOTAL));
    int64_t pole_acc = fdiv(fmul(CP_GRAVITY, sin_a) - fmul(cos_a, temp), lever);
    int64_t cart_acc = temp - fdiv(fmul(CP_POLE_MASS_LENGTH, fmul(pole_acc, cos_a)), CP_MASS_TOTAL);

    if (task.settings.cart_semi) {
        // semi-implicit Euler, the default (gym's other option): speeds first, so this
        // step's push already moves the cart and pole, and a reward on where they ended
        // judges this press, not the one before
        task.cart_v += fmul(CP_TAU, cart_acc);
        task.cart_x += fmul(CP_TAU, task.cart_v);
        task.pole_w += fmul(CP_TAU, pole_acc);
        task.pole_a += fmul(CP_TAU, task.pole_w);
    } else {
        // --cart-integrator euler, the old step (gym's default), kept to reproduce old
        // numbers: positions move by the speeds from before the push, so a push shows only
        // next step and a step's reward grades the press before it
        task.cart_x += fmul(CP_TAU, task.cart_v);
        task.cart_v += fmul(CP_TAU, cart_acc);
        task.pole_a += fmul(CP_TAU, task.pole_w);
        task.pole_w += fmul(CP_TAU, pole_acc);
    }
    task.alive++;
    task.tally[0]++;

    bool fell = task.cart_x > CP_X_LIMIT || task.cart_x < -CP_X_LIMIT ||
                task.pole_a > CP_ANGLE_LIMIT || task.pole_a < -CP_ANGLE_LIMIT;
    if (fell) {
        task.tally[1]++;
        task.episodes++;
        cartpole_restart(task);
        return -1;
    }
    if (task.settings.cart_reward == 1) {
        return 1;
    }
    if (task.settings.cart_reward == 2) {
        int64_t after = cartpole_off(task);
        if (after < CP_HOME_BAND || after < before) {
            return 1;
        }
        return after > before ? -1 : 0;
    }
    if (task.settings.cart_reward == 3) {
        // relative: good when the step ended nearer home than the recent average (about the
        // last 16 steps), bad when further, so good and bad come about equally often
        int64_t after = cartpole_off(task);
        int64_t mean = task.steps == 0 ? before : task.off_average;
        task.off_average = mean + (after - mean) / 16;
        return after < mean ? 1 : (after > mean ? -1 : 0);
    }
    return task.alive % CP_GOOD_EVERY == 0 ? 1 : 0;
}
