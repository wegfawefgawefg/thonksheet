// The task side of the sim thread: the world plug drives a real blob through the same
// ticker blobrun uses (src/run/task_ticker.cpp), one tick at a time, and keeps what the
// task panel and the sheet's marks show. Also --input's random bits, through blob_sense.

#include "sim.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

#if BLOB_HAS_CORE

bool sim_task_start(Sim& sim) {
    SimTask& st = sim.task;
    const RunFlags& run = sim.settings.run;
    if (!ticker_start(st.ticker, *sim.blob, run)) {
        return false;
    }
    st.on = true;
    TaskView& v = st.view;
    const int life = v.life + 1;
    v = TaskView{};
    st.scores.clear();
    v.life = life;
    v.on = true;
    const TaskSettings& ts = st.ticker.task.settings;
    const TaskKind kind = run.task;
    if (kind == TaskKind::echo || kind == TaskKind::shuffled_echo) {
        std::snprintf(v.name, sizeof v.name, "%s k=%d", task_name(kind), ts.k);
    } else if (kind == TaskKind::delayed_echo) {
        std::snprintf(v.name, sizeof v.name, "%s k=%d delay %d", task_name(kind), ts.k, ts.delay);
    } else if (kind == TaskKind::beat) {
        std::snprintf(v.name, sizeof v.name, "%s period %d", task_name(kind), ts.period);
    } else {
        std::snprintf(v.name, sizeof v.name, "%s", task_name(kind));
    }
    std::snprintf(v.settings, sizeof v.settings,
                  "%d ticks/step, rest %d, reward line %d, sensor side %d, motor side %d, "
                  "seeds %llu/%llu",
                  run.plug.ticks_per_step, run.plug.rest_ticks, run.plug.reward_line_ticks,
                  run.plug.sensor_side, run.plug.motor_side,
                  static_cast<unsigned long long>(sim.settings.seed),
                  static_cast<unsigned long long>(run.task_seed));
    v.learning_on = sim.settings.genome.learning_on != 0;
    v.ratio_score = kind != TaskKind::cartpole;
    v.has_cue = kind == TaskKind::echo || kind == TaskKind::shuffled_echo ||
                kind == TaskKind::delayed_echo || kind == TaskKind::go_no_go;
    v.window = run.window;
    v.ticks_per_step = run.plug.ticks_per_step;
    v.twin = sim.settings.twin;
    return true;
}

void sim_task_before_tick(Sim& sim) {
    ticker_before_tick(sim.task.ticker, *sim.blob);
}

bool sim_task_after_tick(Sim& sim) {
    SimTask& st = sim.task;
    TaskTicker& tt = st.ticker;
    if (!ticker_after_tick(tt, *sim.blob)) {
        return false;
    }
    if (tt.valence != 0) {
        blob_reward(*sim.blob, tt.valence);
    }

    // the try, the counts, and a window's score when one closes
    TaskView& v = st.view;
    TaskTry& t = v.tries[v.tries_total % TASK_TRIES];
    t.cue = static_cast<int8_t>(tt.cue);
    t.right = static_cast<int8_t>(tt.right_move);
    t.move = static_cast<int8_t>(tt.move);
    t.valence = static_cast<int8_t>(tt.valence);
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

#else

bool sim_task_start(Sim&) {
    std::fprintf(stderr, "blobview: a task needs the core\n");
    return false;
}
void sim_task_before_tick(Sim&) {
}
bool sim_task_after_tick(Sim&) {
    return false;
}

#endif

// what a line is called on the sheet; only the first line of a bitmap or a number is named
static void line_label(const Plug& plug, TaskKind kind, int line, char* out, int size) {
    out[0] = '\0';
    if (line == PLUG_GOOD_LINE) {
        std::snprintf(out, size, "good");
    } else if (line == PLUG_BAD_LINE) {
        std::snprintf(out, size, "bad");
    } else if (line < plug.first_sheet) {
        const int i = line - plug.first_single;
        if (kind == TaskKind::go_no_go) {
            std::snprintf(out, size, "%s", i == 0 ? "go" : "no-go");
        } else if (kind == TaskKind::delayed_echo && i == plug.shape.singles - 1) {
            std::snprintf(out, size, "ask");
        } else if (kind == TaskKind::sequence) {
            std::snprintf(out, size, "line %d", i);
        } else {
            std::snprintf(out, size, "cue %d", i);
        }
    } else if (line == plug.first_sheet) {
        std::snprintf(out, size, "bitmap %dx%d", plug.shape.sheet_w, plug.shape.sheet_h);
    } else if (line >= plug.first_number && plug.shape.number_lines > 0 &&
               (line - plug.first_number) % plug.shape.number_lines == 0) {
        static const char* const cartpole[4] = {"cart x", "cart speed", "pole angle", "pole spin"};
        const int i = (line - plug.first_number) / plug.shape.number_lines;
        if (kind == TaskKind::cartpole && i < 4) {
            std::snprintf(out, size, "%s", cartpole[i]);
        } else {
            std::snprintf(out, size, "number %d", i);
        }
    }
}

static void motor_label(TaskKind kind, int line, char* out, int size) {
    if (kind == TaskKind::go_no_go || kind == TaskKind::beat) {
        std::snprintf(out, size, "%s", line == 0 ? "press" : "rest");
    } else if (kind == TaskKind::catch_dot || kind == TaskKind::cartpole) {
        std::snprintf(out, size, "%s", line == 0 ? "left" : "right");
    } else {
        std::snprintf(out, size, "motor %d", line);
    }
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

    // the lines on the sheet, sensor then motor
    const TaskTicker& tt = st.ticker;
    const Plug& plug = tt.plug;
    const Genome& g = sim.settings.genome;
    out.tick_in_step = tt.tick_in_step;
    lines.clear();
    const int s_span = (plug.settings.sensor_side - 1) * g.sensor_stride + 1;
    for (int line = 0; line < plug.sensor_lines; ++line) {
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
        line_label(plug, sim.settings.run.task, line, l.label, sizeof l.label);
        lines.push_back(l);
    }
    const int m_span = (plug.settings.motor_side - 1) * g.motor_stride + 1;
    for (int line = 0; line < plug.motor_lines; ++line) {
        TaskLine l;
        l.x0 = static_cast<int16_t>(plug.motor_cell_x[line]);
        l.y0 = static_cast<int16_t>(plug.motor_cell_y[line]);
        l.x1 = static_cast<int16_t>(l.x0 + m_span);
        l.y1 = static_cast<int16_t>(l.y0 + m_span);
        l.kind = TASK_LINE_MOTOR;
        l.on = out.tries_total > 0 && tt.move == line;
        l.count = static_cast<int16_t>(std::min(plug.motor_count[line], 32767));
        motor_label(sim.settings.run.task, line, l.label, sizeof l.label);
        lines.push_back(l);
    }
    out.lines = std::move(lines);
}

void sim_random_input(Sim& sim, bool on) {
#if BLOB_HAS_CORE
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
#else
    (void)sim;
    (void)on;
#endif
}
