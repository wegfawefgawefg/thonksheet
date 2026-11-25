#include "run/task_ticker.hpp"

bool ticker_start(TaskTicker& tt, Blob& blob, const RunFlags& flags) {
    tt = TaskTicker{};
    task_reset(tt.task, flags.task, flags.task_settings, flags.task_seed);
    tt.ticks_per_step = flags.plug.ticks_per_step;
    tt.rest_ticks = flags.plug.rest_ticks;
    babble_start(tt.babble, flags.babble, flags.task_seed);
    return plug_attach(tt.plug, blob, task_shape(tt.task), flags.plug);
}

int ticker_before_tick(TaskTicker& tt, Blob& blob) {
    // quiet ticks after a rewarded step, so good/bad lands on that step's activity
    tt.resting = tt.rest_left > 0;
    if (tt.resting) {
        tt.rest_left--;
        babble_step_end(tt.babble, tt.plug, blob);
        plug_write(tt.plug, blob);
        return -1;
    }
    // a step's first tick: what the task shows, and what would be right
    if (tt.tick_in_step == 0) {
        task_sense(tt.task, tt.sense);
        plug_set_lines(tt.plug, tt.sense);
        tt.perfect_move = task_perfect_move(tt.task);
        tt.in = TaskStepIn{};
        tt.in.sensed_neurons = tt.plug.sensed_neurons;
        babble_step_end(tt.babble, tt.plug, blob);
        babble_step_begin(tt.babble, tt.plug.motor_lines);
    }
    plug_write(tt.plug, blob);
    babble_urge(tt.babble, tt.plug, blob);
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
    if (tt.sense.hidden) {
        plug_count_hidden(tt.plug, blob, tt.sense.hidden, tt.sense.hidden_on, tt.in);
    }
    tt.tick_in_step++;
    if (tt.tick_in_step < tt.ticks_per_step) {
        return false;
    }

    // the answer: the motor line with the most spikes over the step
    tt.in.move = plug_choice(tt.plug);
    tt.cue = tt.task.cue;
    tt.right_move = tt.perfect_move;
    tt.move = tt.in.move;
    int64_t right = tt.task.tally[0], tries = tt.task.tally[1];
    tt.valence = task_step(tt.task, tt.in);
    tt.scored_valence = tt.valence;
    if (tt.withhold) {
        tt.valence = 0; // scored as usual, but nothing reaches the blob
    }
    tt.valence = babble_signal(tt.babble, tt.valence, tt.in.move, tt.task.tally[0] - right,
                               tt.task.tally[1] - tries);
    tt.tick_in_step = 0;
    if (tt.valence != 0) {
        plug_reward(tt.plug, tt.valence);
        plug_quiet_lines(tt.plug);
        tt.rest_left = tt.rest_ticks;
    }
    return true;
}
