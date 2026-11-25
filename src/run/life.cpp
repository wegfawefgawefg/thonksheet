#include "run/life.hpp"

#include "run/predict.hpp"
#include "run/task_ticker.hpp"

#include <chrono>

LifePhase life_phase_main(const RunFlags& flags) {
    LifePhase p;
    p.task = flags.task;
    p.settings = flags.task_settings;
    p.task_seed = flags.task_seed;
    p.steps = flags.steps;
    return p;
}

// the task ticker is large; one per process is enough, lives run one after another
static TaskTicker ticker;
static Predictor predictor; // idea 162, when flags.predict asks (predict.hpp)

bool life_run(Driver driver, const RunFlags& flags, const LifePhase* phases, int count,
              LifeResult& out) {
    out = LifeResult{};
    Genome genome = flags.genome;
    if (driver == Driver::learn_off) {
        genome_learning_off(genome);
    }
    Blob* blob = birth(driver, genome, flags.seed);
    TaskTicker& tt = ticker;
    tt = TaskTicker{};
    bool plugged = false;
    bool measure = engine_driver(driver);
    AliveCount alive;
    auto start = std::chrono::steady_clock::now();

    for (int p = 0; p < count && p < LIFE_MAX_PHASES; p++) {
        const LifePhase& phase = phases[p];
        bool last = p == count - 1;
        out.first_window[p] = (int)out.scores.size();
        if (plugged) {
            plug_detach(tt.plug, *blob);
            plugged = false;
        }

        // unplugged: no line is written or attached, good and bad are quiet
        if (phase.steps <= 0) {
            blob_detach_all(*blob);
            for (int64_t t = 0; t < phase.idle_ticks; t++) {
                tick(driver, *blob, tt.plug, -1);
                if (measure && last && t >= phase.idle_ticks - flags.alive_ticks) {
                    alive_count_tick(alive, *blob);
                }
            }
            out.ticks += phase.idle_ticks;
            continue;
        }

        RunFlags pf = flags;
        pf.task = phase.task;
        pf.task_settings = phase.settings;
        pf.task_seed = phase.task_seed;
        pf.steps = phase.steps;
        if (!core_blob(driver)) {
            pf.predict = PredictSettings{}; // idea 162's cells learn by the full engine's rule
        }
        if (!ticker_start(tt, *blob, pf) || !predict_attach(predictor, tt, *blob, pf)) {
            life_hooks_free(*blob);
            death(driver, blob);
            return false;
        }
        plugged = true;
        tt.withhold = phase.withhold;
        if (phase.damage_pct > 0 || phase.damage_tile) {
            life_damage(*blob, tt.plug, phase, flags.seed);
        }
        // aliveness is counted over the last steps of the last phase
        int64_t per_step = flags.plug.ticks_per_step;
        int64_t count_from = phase.steps - (flags.alive_ticks + per_step - 1) / per_step;

        // the blob ticks several times per step; the ticker senses, counts motor spikes, and
        // has the answer on the step's last tick (then rest ticks, if any, before the next)
        int64_t step = 0;
        while (step < phase.steps) {
            predict_before_tick(predictor, tt, *blob);
            int perfect_move = ticker_before_tick(tt, *blob);
            if (phase.shown_ticks > 0) {
                // the answer shown: the right button's cells fire at the end of the step
                bool late = tt.tick_in_step >= tt.ticks_per_step - phase.shown_ticks;
                life_force(*blob, tt.plug, !tt.resting && late ? perfect_move : -1);
            }
            tick(driver, *blob, tt.plug, perfect_move);
            out.ticks++;
            if (measure && last && step >= count_from) {
                alive_count_tick(alive, *blob);
            }
            predict_after_tick(predictor, tt, *blob);
            if (!ticker_after_tick(tt, *blob)) {
                continue;
            }
            predict_step_end(predictor, *blob, tt.in.move);
            // with a critic the blob hears every step, valence 0 included (blob/critic.hpp)
            if (tt.valence != 0 || (flags.genome.critic_on && core_blob(driver))) {
                if (tt.plug.shape.pattern_bits > 0) {
                    plug_mark_pattern(tt.plug, *blob, tt.in.pattern, tt.in.pattern_tied,
                                      tt.task.pattern_graded);
                } else {
                    plug_mark_acted(tt.plug, *blob, tt.move);
                    plug_mark_vote(tt.plug, *blob, tt.move);
                }
                reward(driver, *blob, tt.valence);
            }
            step++;
            if (step % flags.window == 0) {
                if (babble_on(flags.babble)) {
                    out.clean.push_back(
                        babble_window_clean(tt.babble, tt.task.tally[0], tt.task.tally[1]));
                }
                out.taught.push_back(task_taught_score(tt.task));
                out.scores.push_back(task_window_score(tt.task));
                // extra scores (the calculation battery), one list each
                for (int i = 0; i < tt.task.score_extras && i < TASK_MAX_EXTRA; i++) {
                    out.extra[i].push_back(tt.task.score_extra[i]);
                }
                out.extras = tt.task.score_extras > out.extras ? tt.task.score_extras : out.extras;
                out.extra_task = &tt.task;
            }
        }
        out.predict = predictor.score;
        out.episodes += tt.task.episodes;
        out.wins += tt.task.wins;
        // no cell stays urged past the phase
        babble_step_end(tt.babble, tt.plug, *blob);
        const Babble& bb = tt.babble;
        out.steps += bb.steps, out.babbled += bb.babbled, out.obeyed += bb.obeyed;
        out.still += bb.still;
        out.good_raw += bb.good_raw, out.bad_raw += bb.bad_raw;
        out.good_sent += bb.good_sent, out.bad_sent += bb.bad_sent;
    }
    out.first_window[count] = (int)out.scores.size();
    out.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    if (measure) {
        life_end_room(out.end, *blob);
        life_end_alive(out.end, alive, *blob);
        // the rhythm, with nothing plugged in so no task step sets the beat
        if (plugged) {
            plug_detach(tt.plug, *blob);
        }
        blob_detach_all(*blob);
        std::vector<double> fired((size_t)(flags.rhythm_ticks > 0 ? flags.rhythm_ticks : 0));
        for (double& f : fired) {
            tick(driver, *blob, tt.plug, -1);
            f = fired_now(*blob);
        }
        life_end_rhythm(out.end, fired);
    }
    life_hooks_free(*blob);
    death(driver, blob);
    return true;
}
