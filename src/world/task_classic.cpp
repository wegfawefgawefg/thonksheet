#include "world/task.hpp"

// more worlds (idea 63): mountain car and pendulum swing-up from the classic set, in 16.16
// fixed point like cartpole, so a run repeats exactly. each is sensed as two numbers, 8
// lines each (run them with --number-code place). buttons 0 push left, 1 push right; no
// move pushes nothing.
//
// mountain car  the gym constants: position -1.2..0.6, speed at most 0.07, push 0.001,
//               gravity 0.0025 cos(3x), the goal at 0.5. an episode ends at the goal or
//               after 200 steps; a new one starts at rest between -0.6 and -0.4. the push
//               alone cannot climb the hill: the car has to be rocked. classic_reward 0
//               (energy): every step is good if the car's energy (height plus speed) rose,
//               bad if it fell, and reaching the goal is good. 1 (goal): good at the goal,
//               bad when the 200 steps run out. score: share of episodes that reach the goal.
// pendulum      the gym pendulum: angle 0 is upright, gravity 10, length 1, mass 1, torque
//               2 (too weak to lift it straight up), spin at most 8, steps of 0.05 s. an
//               episode is 200 steps from a random angle and spin within +-1. upright is
//               within 0.4 rad (23 degrees). classic_reward 0 (energy): ending a step upright
//               is good and falling out of upright is bad; elsewhere a step is good when the
//               energy gap to upright shrank, bad when it grew. 1 (goal): only the upright
//               part. score: share of steps that end upright.

constexpr int64_t fix(double v) {
    return (int64_t)(v * 65536.0 + (v < 0 ? -0.5 : 0.5));
}

static int64_t fmul(int64_t a, int64_t b) {
    return (a * b) >> 16;
}

constexpr int64_t FIX_PI = fix(3.14159265358979);
constexpr int64_t FIX_HALF_PI = fix(1.57079632679490);
constexpr int CLASSIC_LINES = 8;
constexpr int CLASSIC_EPISODE = 200;

// sine in 16.16: fold into -pi/2..pi/2, then the series to the 9th power (error under 4e-6)
static int64_t fix_sin(int64_t a) {
    while (a > FIX_PI) {
        a -= 2 * FIX_PI;
    }
    while (a < -FIX_PI) {
        a += 2 * FIX_PI;
    }
    if (a > FIX_HALF_PI) {
        a = FIX_PI - a;
    } else if (a < -FIX_HALF_PI) {
        a = -FIX_PI - a;
    }
    int64_t a2 = fmul(a, a);
    // a (1 - a2/6 (1 - a2/20 (1 - a2/42 (1 - a2/72))))
    int64_t t = FIX_ONE - a2 / 72;
    t = FIX_ONE - fmul(a2, t) / 42;
    t = FIX_ONE - fmul(a2, t) / 20;
    t = FIX_ONE - fmul(a2, t) / 6;
    return fmul(a, t);
}

static int64_t fix_cos(int64_t a) {
    return fix_sin(a + FIX_HALF_PI);
}

static int64_t clamp64(int64_t v, int64_t lo, int64_t hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

static int64_t uniform(Task& task, int64_t lo, int64_t hi) {
    return lo + world_random_below(task.random, (int)(hi - lo + 1));
}

// --- mountain car ---
constexpr int64_t MC_X_MIN = fix(-1.2);
constexpr int64_t MC_X_MAX = fix(0.6);
constexpr int64_t MC_V_MAX = fix(0.07);
constexpr int64_t MC_GOAL = fix(0.5);
constexpr int64_t MC_POWER = fix(0.001);
constexpr int64_t MC_GRAVITY = fix(0.0025);

// height plus speed, at 32 fractional bits: v^2 / 2 + gravity sin(3x) / 3
static int64_t car_energy(const Task& task) {
    return task.vel * task.vel / 2 + fix_sin(3 * task.pos) * MC_GRAVITY / 3;
}

static int car_step(Task& task, int move) {
    int64_t before = car_energy(task);
    int64_t push = move == 0 ? -MC_POWER : (move == 1 ? MC_POWER : 0);
    task.vel =
        clamp64(task.vel + push - fmul(MC_GRAVITY, fix_cos(3 * task.pos)), -MC_V_MAX, MC_V_MAX);
    task.pos = clamp64(task.pos + task.vel, MC_X_MIN, MC_X_MAX);
    if (task.pos == MC_X_MIN && task.vel < 0) {
        task.vel = 0;
    }
    int64_t after = car_energy(task);
    bool goal = task.pos >= MC_GOAL;
    bool timeout = ++task.timer >= CLASSIC_EPISODE;
    if (goal || timeout) {
        task.tally[1]++;
        task.tally[0] += goal;
        task.episodes++;
        task.wins += goal;
        task.timer = 0;
        task.pos = uniform(task, fix(-0.6), fix(-0.4));
        task.vel = 0;
    }
    if (goal) {
        return 1;
    }
    if (task.settings.classic_reward == 1) {
        return timeout ? -1 : 0;
    }
    return after > before ? 1 : (after < before ? -1 : 0);
}

// --- pendulum ---
constexpr int64_t PD_DT = fix(0.05);
constexpr int64_t PD_GRAVITY_TERM = fix(15.0); // 3 g / (2 l)
constexpr int64_t PD_TORQUE_TERM = fix(6.0);   // 3 / (m l^2) times the torque 2
constexpr int64_t PD_SPIN_MAX = fix(8.0);
constexpr int64_t PD_UP = fix(0.4);
constexpr int64_t PD_GAP_BAND = fix(0.01); // energy changes smaller than this are neither

static void pendulum_restart(Task& task) {
    task.pos = uniform(task, -FIX_PI, FIX_PI - 1);
    task.vel = uniform(task, -FIX_ONE, FIX_ONE);
    task.timer = 0;
}

static bool upright(const Task& task) {
    return task.pos < PD_UP && task.pos > -PD_UP;
}

// how far the energy is from the upright pendulum's at rest: |w^2 / 2 + 15 cos a - 15|
static int64_t energy_gap(const Task& task) {
    int64_t e = fmul(task.vel, task.vel) / 2 + fmul(PD_GRAVITY_TERM, fix_cos(task.pos));
    int64_t gap = e - PD_GRAVITY_TERM;
    return gap < 0 ? -gap : gap;
}

static int pendulum_step(Task& task, int move) {
    bool was_up = upright(task);
    int64_t gap_before = energy_gap(task);
    int64_t torque = move == 0 ? -PD_TORQUE_TERM : (move == 1 ? PD_TORQUE_TERM : 0);
    int64_t acc = fmul(PD_GRAVITY_TERM, fix_sin(task.pos)) + torque;
    task.vel = clamp64(task.vel + fmul(acc, PD_DT), -PD_SPIN_MAX, PD_SPIN_MAX);
    task.pos += fmul(task.vel, PD_DT);
    task.pos += task.pos >= FIX_PI ? -2 * FIX_PI : (task.pos < -FIX_PI ? 2 * FIX_PI : 0);
    bool is_up = upright(task);
    int64_t gap_after = energy_gap(task);
    task.tally[1]++;
    task.tally[0] += is_up;
    if (++task.timer >= CLASSIC_EPISODE) {
        task.episodes++;
        task.wins += is_up;
        pendulum_restart(task);
    }
    if (is_up) {
        return 1;
    }
    if (was_up) {
        return -1;
    }
    if (task.settings.classic_reward == 1) {
        return 0;
    }
    if (gap_after < gap_before - PD_GAP_BAND) {
        return 1;
    }
    return gap_after > gap_before + PD_GAP_BAND ? -1 : 0;
}

void classic_reset(Task& task) {
    if (task.kind == TaskKind::mountain_car) {
        task.pos = uniform(task, fix(-0.6), fix(-0.4));
        task.vel = 0;
        task.timer = 0;
        return;
    }
    pendulum_restart(task);
}

TaskShape classic_shape(const Task& task) {
    TaskShape shape;
    shape.numbers = 2;
    shape.number_lines = CLASSIC_LINES;
    shape.motors = 2;
    if (task.kind == TaskKind::mountain_car) {
        shape.number_lo[0] = (int32_t)MC_X_MIN;
        shape.number_hi[0] = (int32_t)MC_X_MAX;
        shape.number_lo[1] = (int32_t)-MC_V_MAX;
        shape.number_hi[1] = (int32_t)MC_V_MAX;
        return shape;
    }
    shape.number_lo[0] = (int32_t)-FIX_PI;
    shape.number_hi[0] = (int32_t)FIX_PI;
    shape.number_lo[1] = (int32_t)-PD_SPIN_MAX;
    shape.number_hi[1] = (int32_t)PD_SPIN_MAX;
    return shape;
}

void classic_sense(const Task& task, TaskSense& sense) {
    sense.numbers[0] = (int32_t)task.pos;
    sense.numbers[1] = (int32_t)task.vel;
}

int classic_perfect_move(const Task& task) {
    if (task.kind == TaskKind::mountain_car) {
        // rock it: push the way it is already going
        return task.vel > 0 ? 1 : 0;
    }
    // near the top: push back against lean and spin; elsewhere pump energy in or out
    if (task.pos < fix(0.6) && task.pos > -fix(0.6)) {
        return task.pos + fmul(fix(0.3), task.vel) > 0 ? 0 : 1;
    }
    int64_t e = fmul(task.vel, task.vel) / 2 + fmul(PD_GRAVITY_TERM, fix_cos(task.pos));
    bool low = e < PD_GRAVITY_TERM;
    return (task.vel >= 0) == low ? 1 : 0;
}

int classic_step(Task& task, const TaskStepIn& in) {
    if (task.kind == TaskKind::mountain_car) {
        return car_step(task, in.move);
    }
    return pendulum_step(task, in.move);
}
