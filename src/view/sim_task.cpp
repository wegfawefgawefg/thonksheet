// The task side of the sim thread: the world plug drives the blob (any engine) through the
// same ticker blobrun uses (src/run/task_ticker.cpp), one tick at a time, and keeps what the
// task panel, the readout panel and the sheet's marks show. Also --input's random bits,
// through blob_sense.
//
// Nothing here knows one task from another: names come from world/task_names.hpp, the
// lines and the bitmap from the plug and the task's TaskSense, the score from
// task_window_score.

#include "sim.hpp"
#include "world/task_names.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

static void name_or(char* out, const char* name, const char* plain, int i) {
    if (name != nullptr) {
        std::snprintf(out, TASK_NAME, "%s", name);
    } else {
        std::snprintf(out, TASK_NAME, "%s %d", plain, i);
    }
}

// the task's lines and buttons by name, once per life
static void name_lines(TaskView& v, const Task& task, const TaskShape& shape) {
    v.singles = std::min(shape.singles, TASK_NAMES);
    v.motors = std::min(shape.motors, TASK_NAMES);
    for (int i = 0; i < v.singles; ++i) {
        name_or(v.single_names[i], task_single_name(task, i), "line", i);
    }
    for (int i = 0; i < v.motors; ++i) {
        name_or(v.motor_names[i], task_motor_name(task, i), "button", i);
    }
    v.sheet_w = shape.sheet_w;
    v.sheet_h = shape.sheet_h;
    v.numbers = std::min(shape.numbers, TASK_WORLD_NUMBERS);
    v.number_lines = shape.number_lines;
    for (int i = 0; i < v.numbers; ++i) {
        name_or(v.number_names[i], task_number_name(task, i), "number", i);
        v.number_lo[i] = shape.number_lo[i];
        v.number_hi[i] = shape.number_hi[i];
    }
}

bool sim_task_start(Sim& sim) {
    SimTask& st = sim.task;
    const RunFlags& run = sim.settings.run;
    if (!ticker_start(st.ticker, *sim.blob, run)) {
        return false;
    }
    st.on = true;
    st.plugged = true;
    TaskView& v = st.view;
    const int life = v.life + 1;
    v = TaskView{};
    st.scores.clear();
    v.life = life;
    v.on = true;
    const Task& task = st.ticker.task;
    const TaskShape shape = task_shape(task);
    const int* cells = nullptr;
    const int per_button = shape.motors > 0 ? plug_motor_cells(st.ticker.plug, 0, &cells) : 0;
    std::snprintf(v.name, sizeof v.name, "%s", task_name(run.task));
    std::snprintf(v.settings, sizeof v.settings,
                  "%s, %d ticks/step, rest %d, reward line %d, sides %d/%d, %d cells a button, "
                  "seeds %llu/%llu",
                  engine_names[sim.engine], run.plug.ticks_per_step, run.plug.rest_ticks,
                  run.plug.reward_line_ticks, run.plug.sensor_side, run.plug.motor_side, per_button,
                  static_cast<unsigned long long>(sim.settings.seed),
                  static_cast<unsigned long long>(run.task_seed));
    v.learning_on = sim.settings.engine.genome.learning_on != 0;
    v.window = run.window;
    v.ticks_per_step = run.plug.ticks_per_step;
    v.twin = sim.settings.twin;
    name_lines(v, task, shape);
    readout_watch_start(st.readout, sim.engine, *sim.blob, st.ticker.plug);
    return true;
}

void sim_task_before_tick(Sim& sim) {
    ticker_before_tick(sim.task.ticker, *sim.blob);
}

bool sim_task_after_tick(Sim& sim) {
    SimTask& st = sim.task;
    TaskTicker& tt = st.ticker;
    // the readout is watched while drawn; flat out undrawn it costs nothing
    const bool watch = sim.shared.mode.load(std::memory_order_relaxed) != RUN_TURBO;
    readout_watch_settle(st.readout, *sim.blob);
    if (watch) {
        readout_watch_tick(st.readout, *sim.blob);
    }
    if (!ticker_after_tick(tt, *sim.blob)) {
        return false;
    }
    TaskView& v = st.view;
    readout_watch_answer(st.readout, tt.move);
    if (tt.valence != 0) {
        // the world tells the buttons' cells which button was pressed, then rewards
        plug_mark_acted(tt.plug, *sim.blob, tt.move);
        engine_reward(sim.engine, *sim.blob, tt.valence);
        readout_watch_reward(st.readout, v.step + 1, tt.valence, tt.move, watch);
    }

    // the try, the counts, and a window's score when one closes
    TaskTry& t = v.tries[v.tries_total % TASK_TRIES];
    t.cue = static_cast<int8_t>(tt.cue);
    t.right = static_cast<int8_t>(tt.right_move);
    t.move = static_cast<int8_t>(tt.move);
    t.valence = static_cast<int8_t>(tt.valence);
    t.lines = 0;
    for (int i = 0; i < std::min(v.singles, 32); ++i) {
        t.lines |= tt.sense.singles[i] ? 1u << i : 0u;
    }
    ++v.tries_total;
    ++v.step;
    v.good += tt.valence > 0 ? 1 : 0;
    v.bad += tt.valence < 0 ? 1 : 0;
    v.answer_tick = sim.blob->stats.tick;
    if (v.step % v.window == 0) {
        st.scores.push_back(static_cast<float>(task_window_score(tt.task)));
    }
    sim.twin.target.store(v.step, std::memory_order_relaxed);
    return true;
}

// a list that only grows within a life: copy the new tail, or all of it after a restart
static void copy_grown(std::vector<float>& to, const std::vector<float>& from, bool fresh) {
    if (fresh || to.size() > from.size()) {
        to = from;
    } else {
        to.insert(to.end(), from.begin() + static_cast<long>(to.size()), from.end());
    }
}

void task_scores_copy(TaskScores& to, const TaskScores& from) {
    const bool fresh = to.life != from.life;
    to.life = from.life;
    copy_grown(to.blob, from.blob, fresh);
    copy_grown(to.twin, from.twin, fresh);
    copy_grown(to.coin, from.coin, fresh);
}

// the world as the task shows it this step: the bitmap and the numbers with their lines
static void fill_world(TaskView& out, const TaskTicker& tt) {
    const Plug& plug = tt.plug;
    const int pixels = std::min<int>(out.sheet_w * out.sheet_h, sizeof tt.sense.sheet);
    out.sheet.assign(tt.sense.sheet, tt.sense.sheet + std::max(0, pixels));
    for (int i = 0; i < out.numbers; ++i) {
        out.number[i] = tt.sense.numbers[i];
        out.number_on[i] = 0;
        for (int j = 0; j < std::min(out.number_lines, 32); ++j) {
            const int line = plug.first_number + i * out.number_lines + j;
            out.number_on[i] |= line < plug.sensor_lines && plug.line_on[line] ? 1u << j : 0u;
        }
    }
}

// what a line is called on the sheet; only the first line of a bitmap or a number is named
static void line_label(const Plug& plug, const TaskView& v, int line, char* out, int size) {
    out[0] = '\0';
    if (line == PLUG_GOOD_LINE) {
        std::snprintf(out, size, "good");
    } else if (line == PLUG_BAD_LINE) {
        std::snprintf(out, size, "bad");
    } else if (line < plug.first_sheet) {
        const int i = line - plug.first_single;
        std::snprintf(out, size, "%s", i < v.singles ? v.single_names[i] : "");
    } else if (line == plug.first_sheet && plug.first_number > plug.first_sheet) {
        std::snprintf(out, size, "bitmap %dx%d", plug.shape.sheet_w, plug.shape.sheet_h);
    } else if (line >= plug.first_number && plug.shape.number_lines > 0 &&
               (line - plug.first_number) % plug.shape.number_lines == 0) {
        const int i = (line - plug.first_number) / plug.shape.number_lines;
        std::snprintf(out, size, "%s", i < v.numbers ? v.number_names[i] : "number");
    }
}

void sim_task_fill(Sim& sim, TaskView& out, TaskScores& scores) {
    const SimTask& st = sim.task;
    if (!st.on) {
        out.on = false;
        return;
    }
    std::vector<TaskLine> lines = std::move(out.lines);
    out = st.view;
    const bool fresh = scores.life != out.life;
    scores.life = out.life;
    copy_grown(scores.blob, st.scores, fresh);
    {
        std::lock_guard<std::mutex> hold(sim.twin.lock);
        copy_grown(scores.twin, sim.twin.scores, fresh);
        copy_grown(scores.coin, sim.twin.coin_scores, fresh);
    }

    // the lines on the sheet, sensor then motor (none while the probe has the sheet)
    const TaskTicker& tt = st.ticker;
    const Plug& plug = tt.plug;
    const Genome& g = sim.settings.engine.genome;
    out.tick_in_step = tt.tick_in_step;
    fill_world(out, tt);
    lines.clear();
    const int s_span = (plug.settings.sensor_side - 1) * g.sensor_stride + 1;
    for (int line = 0; st.plugged && line < plug.sensor_lines; ++line) {
        if (!plug.attached[line]) {
            continue;
        }
        TaskLine l;
        l.x0 = static_cast<int16_t>(plug.sensor_cell_x[line]);
        l.y0 = static_cast<int16_t>(plug.sensor_cell_y[line]);
        l.x1 = static_cast<int16_t>(l.x0 + s_span);
        l.y1 = static_cast<int16_t>(l.y0 + s_span);
        l.kind = line == PLUG_GOOD_LINE  ? TASK_LINE_GOOD
                 : line == PLUG_BAD_LINE ? TASK_LINE_BAD
                                         : TASK_LINE_SENSE;
        l.on = plug.line_on[line];
        if (line == PLUG_GOOD_LINE || line == PLUG_BAD_LINE) {
            l.on = line == PLUG_GOOD_LINE ? plug.good_left > 0 : plug.bad_left > 0;
        }
        line_label(plug, out, line, l.label, sizeof l.label);
        lines.push_back(l);
    }
    // motor groups get a ring only when packed; spread cells are coloured one by one
    const int m_span = (plug.settings.motor_side - 1) * g.motor_stride + 1;
    for (int line = 0; st.plugged && line < plug.motor_lines; ++line) {
        TaskLine l;
        l.x0 = static_cast<int16_t>(plug.motor_cell_x[line]);
        l.y0 = static_cast<int16_t>(plug.motor_cell_y[line]);
        l.x1 = static_cast<int16_t>(l.x0 + m_span);
        l.y1 = static_cast<int16_t>(l.y0 + m_span);
        l.kind = TASK_LINE_MOTOR;
        l.on = out.tries_total > 0 && tt.move == line;
        l.count = static_cast<int16_t>(std::min(plug.motor_count[line], 32767));
        l.spread = plug.settings.motor_spread > 0;
        std::snprintf(l.label, sizeof l.label, "%s",
                      line < out.motors ? out.motor_names[line] : "button");
        lines.push_back(l);
    }
    out.lines = std::move(lines);
}

void sim_random_input(Sim& sim, bool on) {
    Blob& b = *sim.blob;
    if (!on) {
        if (sim.input_was_on) {
            blob_detach_all(b);
            sim.input_was_on = false;
        }
        return;
    }
    // 1 in 8 sensor neurons on, each tick
    sim.input_was_on = true;
    for (int t = 0; t < b.tile_count; ++t) {
        uint64_t r = ~0ull;
        for (int i = 0; i < 3; ++i) {
            r &= fake_random(sim.rng);
        }
        for (uint64_t bits = b.sensor[t]; bits != 0; bits &= bits - 1) {
            const int bit = __builtin_ctzll(bits);
            blob_sense(b, t * TILE_NEURONS + bit, (r >> bit) & 1);
        }
    }
}
