#include "run/task_ticker.hpp"

bool ticker_start(TaskTicker& tt, Blob& blob, const RunFlags& flags) {
    tt = TaskTicker{};
    task_reset(tt.task, flags.task, flags.task_settings, flags.task_seed);
    tt.ticks_per_step = flags.plug.ticks_per_step;
    tt.rest_ticks = flags.plug.rest_ticks;
    return plug_attach(tt.plug, blob, task_shape(tt.task), flags.plug);
}

int ticker_before_tick(TaskTicker& tt, Blob& blob) {
    // quiet ticks after a rewarded step, so good/bad lands on that step's activity
    tt.resting = tt.rest_left > 0;
    if (tt.resting) {
        tt.rest_left--;
        plug_write(tt.plug, blob);
        return -1;
    }
    // a step's first tick: what the task shows, and what would be right. a task whose
    // lines change inside a step (the battery's tunes) senses on every tick
    if (tt.tick_in_step == 0 || inv_by_tick(tt.task)) {
        inv_set_tick(tt.task, tt.tick_in_step, tt.ticks_per_step);
        task_sense(tt.task, tt.sense);
        plug_set_lines(tt.plug, tt.sense);
    }
    if (tt.tick_in_step == 0) {
        tt.perfect_move = task_perfect_move(tt.task);
        tt.in = TaskStepIn{};
        tt.in.sensed_neurons = tt.plug.sensed_neurons;
    }
    plug_write(tt.plug, blob);
    return tt.perfect_move;
}

bool ticker_after_tick(TaskTicker& tt, const Blob& blob) {
    if (tt.resting) {
        return false;
    }
    // surprise is read after the step's first tick
    if (tt.tick_in_step == 0) {
        tt.in.surprise = (int)blob.stats.surprise;
    }
    plug_count(tt.plug, blob);
    tt.tick_in_step++;
    if (tt.tick_in_step < tt.ticks_per_step) {
        return false;
    }

    // the answer: the motor line with the most spikes over the step
    tt.in.move = plug_choice(tt.plug);
    tt.cue = tt.task.cue;
    tt.right_move = tt.perfect_move;
    tt.move = tt.in.move;
    tt.valence = task_step(tt.task, tt.in);
    tt.tick_in_step = 0;
    if (tt.valence != 0) {
        plug_reward(tt.plug, tt.valence);
        plug_quiet_lines(tt.plug);
        tt.rest_left = tt.rest_ticks;
    }
    return true;
}
