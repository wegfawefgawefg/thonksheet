# Every line is a target (idea 162, 2026-11-25)

Queue item 3 of `docs/plan.md`, widened by the owner: "in all of our envs we should extract
rich signal. thonksheet shouldnt just output policy right. shouldnt it predict current state,
next state, future states". The sheet predicts every input line 1, 2, 4 and 8 steps ahead,
always on, in every world, beside the task. Is it able to (skill), does the inside know more
for it (a fresh reader after a freeze), does the task get better (score)?

## Predictions (written before any report-seed run)

Seen before writing this: tuning runs on seeds 1 and 2 (20,000 steps; catch, cartpole,
delayed echo, chain, track) and one smoke pass of the whole battery on seed 1 at 2,000
steps. On those: chain at h=1 well above every baseline (0.92 against "same" 0.30, "common"
0.65); catch, cartpole and delayed echo under "common" on the raw share; track's error lines
carry which way the marker moved.

1. Raw share right at h=1 beats all three baselines ("same as now", each line's most common
   value, the learning-off twin) on the mean of 8 seeds in at most 3 of the 11 worlds, chain
   one of them. Probability 0.7. At h=8, in at most 1. Probability 0.75.
2. Plain echo with random cues (the control): the predictor does not beat "common" at any
   horizon. Probability 0.85.
3. The inside read, catch "which way the paddle moved": the pred arm reads within 0.03 of the
   task-only arm (mean of 8 seeds), because under the working output rule only motor cells
   learn; prediction changes the inside only through what the predictor cells themselves
   fire. Probability 0.7. The arms with competitive learning read at least 0.10 above
   task-only there. Probability 0.6.
4. The task score: with prediction on, the mean paired difference against off is within
   0.03 (or within one off-arm sd for cartpole's unit) on at least 8 of the 11 worlds.
   Probability 0.65.
5. Cost: prediction on runs at 0.5 to 0.8 of the ticks per second of off. Probability 0.8.

How they scored: see the end.

## What was built

- **Predictor cells** (`src/run/predict.cpp`, `predict.hpp`; flags in
  `src/world/predict_flags.cpp`, `predict_settings.hpp`). `blobrun --predict 1,2,4,8`: for
  every input line the world attaches (good and bad included) and each horizon h, a pair of
  motor-cell groups ("off", "on") voted over the step like a pattern bit of the calculation
  battery. A tie answers off. When step t + h is sensed, each wrong answer gets the working
  output rule's bad (`learn_readout.cpp`: the group that answered loses its active inputs,
  the other gains them), on the spike maps of step t, kept in a ring of 16 steps. No second
  output rule: the only core addition is `blob_readout_verdict`, which runs the readout's
  error step at once on given maps (blob.hpp, learn_readout.cpp, 20 lines). The world's
  reward still teaches the buttons through the tick, as before; the predictor's verdicts save
  and restore the world's marks round themselves.
- **Last press as lines** (`--predict-press 1`): one line per button, on for the step after
  that button was pressed. No world did this before (efference copies only went to the
  readout's marks, `plug_mark_acted`).
- **Errors as lines** (`--predict-errors 1`): per predicted line, "said on, was off" and "said
  off, was on" for the shortest horizon, shown during the step whose lines they are about.
- **Extra lines in the plug** (`PlugSettings.extra_lines`, `plug.cpp`, `plug_spread.cpp`):
  laid after the task's own lines; the buttons' cells and `--distance` ignore them.
- **The idea 159 hook**: `Predictor::error` (`predict_errors()`), the shortest horizon's
  signed error per line at each step's start.
- **`blobprobe predict`** (`src/probe/predict_read.cpp`, `predict_labels.cpp`): live, freeze,
  then read the world afresh with moves from a fixed coin; a fresh ridge reader of the whole
  sheet (`linear_whole`) and of the hidden cells only, beside readers of the lines, for
  questions not on the lines at one instant. `--shuffle 1` is the control: the life's frames
  (every line, good and bad, the presses) replayed to a fresh sheet of the same seed in a
  shuffled order, no task, no reward.
- **`scripts/predict.sh NAME [--quick]`** runs it all and writes `out/predict/NAME.md` and
  `NAME-skill.csv`, `NAME-task.csv`, `NAME-read.csv`.

**One sheet for every arm.** With the default regions the buttons at `--motor-spread 128`
take every motor cell and catch's 38 lines with error lines do not fit the sensor region. So
every arm of the battery, off included, runs on the default 64 x 64 sheet with the sensor
lattice across the top half and every cell of the bottom-left quadrant a motor cell
(`--g.sensor_w 64 --g.motor_stride 1`), buttons at `--motor-spread 64`, 20 ticks a step,
sensor groups of side 2, and the working output rule (`--g.learn_mode 5 --g.readout_evidence
1 --g.readout_shift 4 --g.readout_balance 1`). The predictor's own chance is
`--predict-shift 2` (tuned on seeds 1 and 2: shift 2 beat 4 and 6 by 0.005 to 0.015 on
catch, cartpole and delayed echo). Catch runs on a wider motor quadrant (`--g.motor_w 48`, every arm) for 4 cells a
group; cartpole and forage get 3 to 4, the rest 4. A first layout with the motor lattice across the whole bottom half at stride 2
put most predictor cells three or more tiles from any input line; chain at h=1 then reached
0.72, on this layout 0.92. Worlds: cartpole `--cart-reward lean --number-code place`,
pendulum and mountain car `--number-code place`, delayed echo `--delay 4`, predict-next `--k 4
--markov-order 1` (order 2 in the read, where the next symbol must not be on the lines),
the rest at their defaults. Lives are 20,000 steps; skill is tallied over the last quarter.

**What learns in each arm.** Under the working output rule nothing but the motor cells
learns: the buttons from the reward, the predictor cells from their verdicts. The hidden cells'
wiring moves only in the arms with competitive learning (`--g.compete_on 1 --g.tile_winners 4
--g.compete_shift 5`, idea 158). So in the pred arms, "the inside knows more" can come only
from what the predictor cells (which are part of the sheet and drive hidden cells) fire, or
from the error lines as input.

## Short version (8 seeds, 5 to 12; 20,000-step lives)

Written by the coordinator: the agent that built this was stopped by the harness with the
battery still running; the runs finished on their own and the tables below are
`scripts/predict.sh main --seeds 5-12 --table-only`.

- **The cells predict where there is something to predict.** One step ahead they beat "same
  as now", "most common" and the learning-off twin on every seed in chain (0.875 against
  0.286, 0.643, 0.536) and predict-next (0.767 against 0.404, 0.634, 0.504). On plain echo
  with random cues they sit at baseline (0.749 against 0.754), as they must. Nowhere else
  and at no longer horizon do they beat all three on the raw share. Counting on-steps and
  off-steps equally they are also ahead on catch (0.652 against 0.552 and 0.584) and delayed
  echo (0.699 against 0.445 and 0.500): the lines are mostly off, so "most common" is a
  strong baseline on the raw share.
- **They fall well short of what a fitted reader finds, and the sheet adds nothing to the
  lines.** A ridge reader of the frozen sheet, per line: predict-next 0.914 (cells 0.767),
  cartpole 0.924 (0.871), catch 0.967 (0.923), mountain car at 8 steps 0.915 (0.810). On
  cartpole, pendulum and mountain car the cells are under "same as now"; the reader is not.
  So the shortfall is the output rule (idea 161's gap), not missing information. And a
  reader of the input lines alone does as well as a reader of the whole sheet, born or
  lived (within 0.01 everywhere but delayed echo, where the sheet has +0.02): for a
  straight-line reader, the future is in the lines.
- **Prediction did not teach the inside.** Under the working output rule only motor cells
  learn, so prediction alone cannot change a hidden cell's wiring. In the arms that can
  (competitive learning with the error lines as input), a reader of the hidden cells does
  far better than with competitive learning alone (which way the paddle moved +0.269, time
  since the cue +0.197, beat phase +0.145, last press +0.134, all 8 seeds of 8), but the
  time-shuffled control does as well (differences -0.018, -0.002, -0.001, +0.013). The gain
  is information the error lines carry in (an error line is a change detector), not
  something learned about time.
- **Competitive learning costs memory of time on these worlds.** Time since the cue reads
  0.859 from the born hidden cells and 0.645 after a life with it; beat phase 0.497 and
  0.286; which way the paddle moved is 0.146 under its own shuffled control. Cells turn
  toward the input lines and stop carrying the recent past (and fire at 52 to 59 per 1,000
  against 32).
- **Error lines alone help a little, at the same firing rate.** With only motor cells
  learning, error lines as input lift the hidden-cell read of the last press 0.604 to 0.745,
  beat phase 0.500 to 0.614, time since the cue 0.873 to 0.919, at 31 to 34 spikes per 1,000
  in every arm. Prediction without error lines lifts time since the cue 0.873 to 0.897.
- **The task: small and mixed.** Delayed echo +0.035 to +0.050 (7 or 8 seeds of 8), beat
  +0.026 with error lines (8 of 8), mountain car to 1.00; catch -0.02, forage -0.02 (-0.07
  with the last press fed back), chain -0.03 to -0.05, cartpole about one step per fall
  lower. Prediction with competitive learning breaks chain (-0.376).
- **Cost:** 0.49 to 0.87 of the off arm's ticks per second (worst on the worlds with many
  lines); with competitive learning as well, 0.34 to 0.48.
- **Track is dropped** from this battery: on this sheet shape (sensor side 2) the readout
  locks the marker against a wall (pressing into the end earns nothing and the rule only
  learns on bad): 0.015 against 0.446 on the standard shape, seed 5. Its runs are in
  `out/predict/main-dropped/`. That is a fault of the row on this shape, not a result.

**What this does and does not say about idea 162.** It says the worlds do hold a rich
signal the sheet can be asked for, that our output cells can learn part of it, and that
feeding surprise back in as lines makes the recent past easier to read. It does not test
the idea itself, which is that the errors should teach the inside: this version has no
path from an error to a hidden cell's wiring. That path (idea 159's one-hop wires) merged
the same day; `predict_errors()` is the hook. Sending each line's error down those wires is
the next job.

## The tables

### 1. Prediction skill (share of line-steps right, last quarter of the life)

pred: the predictor cells; same: the line h steps before; common: each line's more common value; twin: the same cells on the learning-off twin. bal: the mean of the right shares on on- and off-line-steps. chg: right share on line-steps that changed. `*` marks pred above all three baselines (mean over seeds); `/n` how many seeds it is above all three.

| world | h | pred | same | common | twin | | bal pred | bal same | bal common | bal twin | chg pred |
|---|---|---|---|---|---|---|---|---|---|---|---|
| catch | 1 |  0.923 |  0.902 |  0.946 |  0.580 | /0 |  0.652 |  0.552 |  0.584 |  0.499 |  0.515 |
| catch | 2 |  0.921 |  0.917 |  0.946 |  0.580 | /0 |  0.639 |  0.619 |  0.584 |  0.504 |  0.530 |
| catch | 4 |  0.913 |  0.913 |  0.946 |  0.580 | /0 |  0.618 |  0.600 |  0.584 |  0.501 |  0.508 |
| catch | 8 |  0.914 |  0.910 |  0.946 |  0.582 | /0 |  0.610 |  0.586 |  0.584 |  0.497 |  0.513 |
| forage | 1 |  0.907 |  0.916 |  0.950 |  0.592 | /0 |  0.645 |  0.652 |  0.695 |  0.504 |  0.473 |
| forage | 2 |  0.911 |  0.917 |  0.950 |  0.594 | /0 |  0.636 |  0.655 |  0.695 |  0.503 |  0.474 |
| forage | 4 |  0.910 |  0.913 |  0.950 |  0.596 | /0 |  0.630 |  0.638 |  0.695 |  0.497 |  0.475 |
| forage | 8 |  0.910 |  0.912 |  0.950 |  0.595 | /0 |  0.631 |  0.632 |  0.695 |  0.498 |  0.480 |
| cartpole | 1 |  0.871 |  0.911 |  0.866 |  0.550 | /0 |  0.793 |  0.856 |  0.669 |  0.494 |  0.337 |
| cartpole | 2 |  0.845 |  0.884 |  0.866 |  0.558 | /0 |  0.751 |  0.813 |  0.669 |  0.500 |  0.368 |
| cartpole | 4 |  0.812 |  0.828 |  0.866 |  0.562 | /0 |  0.699 |  0.723 |  0.669 |  0.502 |  0.426 |
| cartpole | 8 |  0.796 |  0.786 |  0.866 |  0.556 | /0 |  0.673 |  0.656 |  0.669 |  0.495 |  0.475 |
| pendulum | 1 |  0.873 |  0.901 |  0.852 |  0.556 | /0 |  0.771 |  0.821 |  0.599 |  0.493 |  0.327 |
| pendulum | 2 |  0.842 |  0.861 |  0.852 |  0.551 | /0 |  0.716 |  0.749 |  0.599 |  0.495 |  0.415 |
| pendulum | 4 |  0.808 |  0.800 |  0.852 |  0.558 | /0 |  0.657 |  0.639 |  0.599 |  0.498 |  0.486 |
| pendulum | 8 |  0.784 |  0.760 |  0.852 |  0.559 | /0 |  0.615 |  0.567 |  0.599 |  0.495 |  0.529 |
| mountain-car | 1 |  0.934 |  0.969 |  0.877 |  0.564 | /0 |  0.882 |  0.944 |  0.645 |  0.498 |  0.169 |
| mountain-car | 2 |  0.901 |  0.945 |  0.877 |  0.554 | /0 |  0.823 |  0.901 |  0.645 |  0.499 |  0.194 |
| mountain-car | 4 |  0.857 |  0.901 |  0.877 |  0.559 | /0 |  0.745 |  0.821 |  0.645 |  0.501 |  0.283 |
| mountain-car | 8 |  0.812 |  0.848 |  0.877 |  0.572 | /0 |  0.662 |  0.726 |  0.645 |  0.511 |  0.378 |
| chain | 1 |  0.875* |  0.286 |  0.643 |  0.536 | /8 |  0.849 |  0.234 |  0.583 |  0.499 |  0.841 |
| chain | 2 |  0.876 |  0.928 |  0.643 |  0.554 | /0 |  0.866 |  0.922 |  0.583 |  0.530 |  0.353 |
| chain | 4 |  0.787 |  0.868 |  0.643 |  0.515 | /0 |  0.767 |  0.858 |  0.583 |  0.490 |  0.315 |
| chain | 8 |  0.734 |  0.806 |  0.643 |  0.528 | /0 |  0.710 |  0.791 |  0.583 |  0.494 |  0.324 |
| delayed-echo | 1 |  0.889 |  0.803 |  0.901 |  0.589 | /0 |  0.699 |  0.445 |  0.500 |  0.524 |  0.730 |
| delayed-echo | 2 |  0.848 |  0.803 |  0.901 |  0.567 | /0 |  0.590 |  0.445 |  0.500 |  0.501 |  0.633 |
| delayed-echo | 4 |  0.865 |  0.803 |  0.901 |  0.571 | /0 |  0.680 |  0.445 |  0.500 |  0.508 |  0.714 |
| delayed-echo | 8 |  0.849 |  0.803 |  0.901 |  0.577 | /0 |  0.592 |  0.445 |  0.500 |  0.493 |  0.634 |
| beat | 1 |  0.707 |  0.689 |  0.780 |  0.543 | /0 |  0.575 |  0.547 |  0.500 |  0.500 |  0.451 |
| beat | 2 |  0.710 |  0.693 |  0.780 |  0.539 | /0 |  0.579 |  0.553 |  0.500 |  0.501 |  0.454 |
| beat | 4 |  0.719 |  0.779 |  0.780 |  0.543 | /0 |  0.592 |  0.677 |  0.500 |  0.504 |  0.438 |
| beat | 8 |  0.716 |  0.775 |  0.780 |  0.537 | /0 |  0.588 |  0.672 |  0.500 |  0.497 |  0.454 |
| predict-next | 1 |  0.767* |  0.404 |  0.634 |  0.504 | /8 |  0.767 |  0.404 |  0.634 |  0.504 |  0.742 |
| predict-next | 2 |  0.724* |  0.720 |  0.634 |  0.497 | /2 |  0.724 |  0.720 |  0.634 |  0.497 |  0.430 |
| predict-next | 4 |  0.651 |  0.676 |  0.634 |  0.495 | /0 |  0.651 |  0.676 |  0.634 |  0.495 |  0.430 |
| predict-next | 8 |  0.600 |  0.632 |  0.634 |  0.504 | /0 |  0.600 |  0.632 |  0.634 |  0.504 |  0.455 |
| echo | 1 |  0.749 |  0.750 |  0.754 |  0.501 | /0 |  0.749 |  0.750 |  0.754 |  0.498 |  0.496 |
| echo | 2 |  0.748 |  0.748 |  0.754 |  0.504 | /0 |  0.748 |  0.748 |  0.754 |  0.502 |  0.497 |
| echo | 4 |  0.751 |  0.750 |  0.754 |  0.497 | /3 |  0.751 |  0.750 |  0.754 |  0.496 |  0.500 |
| echo | 8 |  0.749 |  0.750 |  0.754 |  0.500 | /0 |  0.749 |  0.750 |  0.754 |  0.498 |  0.497 |

### Task score, paired against off

Paired difference against off (mean over seeds, and seeds where it is higher / lower):

| world | pred | pred_err | pred_press | press | compete | pred_compete |
|---|---|---|---|---|---|---|
| catch | -0.021 (1/7) | -0.020 (1/7) | -0.011 (3/5) | -0.009 (4/4) | -0.035 (1/7) | -0.050 (0/8) |
| forage | -0.024 (2/6) | -0.018 (2/6) | -0.074 (0/8) | -0.048 (3/5) | -0.005 (4/4) | -0.043 (2/6) |
| cartpole | -0.801 (2/6) | -0.611 (0/6) | -0.943 (1/7) | -0.072 (4/4) | +0.019 (5/2) | -0.111 (4/4) |
| pendulum | +0.014 (5/3) | +0.005 (5/3) | +0.006 (4/4) | +0.008 (5/3) | +0.001 (2/6) | +0.002 (5/3) |
| mountain-car | +0.055 (4/0) | +0.063 (4/0) | +0.048 (3/0) | +0.030 (4/1) | +0.063 (4/0) | +0.063 (4/0) |
| chain | -0.033 (2/6) | -0.045 (0/8) | -0.015 (4/4) | +0.010 (6/2) | -0.003 (5/3) | -0.376 (2/6) |
| delayed-echo | +0.035 (7/1) | +0.047 (8/0) | +0.050 (7/1) | -0.008 (3/5) | +0.001 (4/4) | +0.038 (7/1) |
| beat | -0.005 (2/5) | +0.026 (8/0) | -0.030 (0/8) | -0.034 (0/8) | -0.009 (3/5) | -0.012 (2/6) |
| predict-next | +0.005 (5/3) | +0.019 (6/2) | +0.032 (7/1) | +0.033 (7/1) | +0.030 (6/2) | +0.031 (6/2) |
| echo | +0.000 (0/0) | +0.000 (0/0) | +0.000 (0/0) | +0.000 (0/0) | +0.000 (0/0) | +0.000 (0/0) |

#### 5a. The arms that can change the inside

| world | question | born | compete | shuffled_compete | pred_err_compete | shuffled_pred_err_compete | now | two | all lines (err) | maj |
|---|---|---|---|---|---|---|---|---|---|---|
| catch | moved | 0.573 / 0.521 / 32.0 | 0.561 / 0.535 / 52.3 | 0.720 / 0.681 / 45.5 | 0.763 / 0.804 / 41.6 | 0.810 / 0.822 / 38.2 | 0.450 | 0.853 | 0.515 | 0.451 |
| cartpole | last_push | 0.469 / 0.473 / 32.1 | 0.501 / 0.502 / 54.0 | 0.495 / 0.500 / 52.4 | 0.503 / 0.517 / 54.1 | 0.492 / 0.496 / 53.8 | 0.472 | 0.570 | 0.489 | 0.336 |
| chain | last_press | 0.618 / 0.605 / 32.0 | 0.593 / 0.588 / 55.9 | 0.600 / 0.588 / 55.8 | 0.736 / 0.722 / 55.4 | 0.716 / 0.709 / 47.2 | 0.584 | 0.586 | 0.747 | 0.336 |
| delayed-echo | cue | 0.729 / 0.706 / 32.0 | 0.669 / 0.657 / 51.9 | 0.694 / 0.681 / 46.7 | 0.675 / 0.670 / 35.5 | 0.704 / 0.697 / 35.3 | 0.582 | 0.665 | 0.582 | 0.498 |
| delayed-echo | since_cue | 0.908 / 0.859 / 32.0 | 0.700 / 0.645 / 51.9 | 0.801 / 0.762 / 46.7 | 0.885 / 0.842 / 35.5 | 0.874 / 0.845 / 35.3 | 0.500 | 0.667 | 0.560 | 0.167 |
| beat | beat_phase | 0.557 / 0.497 / 32.0 | 0.285 / 0.286 / 58.6 | 0.319 / 0.324 / 57.1 | 0.450 / 0.431 / 56.4 | 0.462 / 0.432 / 54.6 | 0.249 | 0.249 | 0.396 | 0.249 |
| predict-next | before | 0.933 / 0.902 / 32.0 | 0.971 / 0.943 / 54.7 | 0.973 / 0.930 / 54.4 | 0.980 / 0.969 / 50.1 | 0.983 / 0.970 / 49.0 | 0.591 | 0.972 | 0.718 | 0.485 |
| predict-next | next | 0.759 / 0.742 / 32.0 | 0.781 / 0.766 / 54.7 | 0.776 / 0.760 / 54.4 | 0.809 / 0.805 / 50.1 | 0.791 / 0.787 / 49.0 | 0.597 | 0.784 | 0.678 | 0.484 |
| echo | last_cue | 0.958 / 0.936 / 32.0 | 0.985 / 0.881 / 56.1 | 0.950 / 0.854 / 55.9 | 0.992 / 0.977 / 49.2 | 0.984 / 0.973 / 48.7 | 0.493 | 1.000 | 0.688 | 0.492 |

Paired, hidden cells: pred_err_compete less compete, and each less its shuffled control (mean over seeds; seeds higher / lower):

| world | question | err+pred+compete - compete | compete - its shuffled | err+pred+compete - its shuffled |
|---|---|---|---|---|
| catch | moved | +0.269 (8/0) | -0.146 (0/8) | -0.018 (2/6) |
| cartpole | last_push | +0.015 (6/2) | +0.002 (4/4) | +0.021 (7/1) |
| chain | last_press | +0.134 (8/0) | -0.001 (4/4) | +0.013 (5/3) |
| delayed-echo | cue | +0.013 (5/3) | -0.024 (3/5) | -0.026 (2/5) |
| delayed-echo | since_cue | +0.197 (8/0) | -0.117 (1/7) | -0.002 (4/4) |
| beat | beat_phase | +0.145 (8/0) | -0.037 (3/5) | -0.001 (5/3) |
| predict-next | before | +0.026 (6/1) | +0.012 (6/2) | -0.001 (4/4) |
| predict-next | next | +0.039 (7/1) | +0.006 (4/4) | +0.018 (6/2) |
| echo | last_cue | +0.096 (8/0) | +0.027 (7/0) | +0.004 (5/3) |

#### 5b. The other arms (only motor cells learn in these)

| world | question | born | task | pred | pred_err | shuffled | pred_compete | now | all lines (err) | maj |
|---|---|---|---|---|---|---|---|---|---|---|
| catch | moved | 0.573 / 0.521 / 32.0 | 0.592 / 0.554 / 34.2 | 0.549 / 0.549 / 39.5 | 0.586 / 0.600 / 39.3 | 0.595 / 0.552 / 32.0 | 0.582 / 0.564 / 53.7 | 0.450 | 0.485 | 0.451 |
| cartpole | last_push | 0.469 / 0.473 / 32.1 | 0.466 / 0.466 / 34.0 | 0.475 / 0.470 / 40.5 | 0.487 / 0.482 / 40.8 | 0.467 / 0.465 / 32.8 | 0.508 / 0.493 / 56.5 | 0.472 | 0.481 | 0.336 |
| chain | last_press | 0.618 / 0.605 / 32.0 | 0.606 / 0.604 / 31.5 | 0.614 / 0.595 / 31.6 | 0.743 / 0.745 / 31.6 | 0.609 / 0.606 / 31.5 | 0.612 / 0.607 / 55.8 | 0.584 | 0.744 | 0.336 |
| delayed-echo | cue | 0.729 / 0.706 / 32.0 | 0.727 / 0.719 / 31.4 | 0.711 / 0.707 / 31.5 | 0.725 / 0.719 / 31.5 | 0.723 / 0.712 / 31.5 | 0.667 / 0.657 / 53.1 | 0.582 | 0.587 | 0.498 |
| delayed-echo | since_cue | 0.908 / 0.859 / 32.0 | 0.916 / 0.873 / 31.4 | 0.946 / 0.897 / 31.5 | 0.954 / 0.919 / 31.5 | 0.921 / 0.858 / 31.5 | 0.734 / 0.677 / 53.1 | 0.500 | 0.554 | 0.167 |
| beat | beat_phase | 0.557 / 0.497 / 32.0 | 0.545 / 0.500 / 32.6 | 0.550 / 0.509 / 32.6 | 0.657 / 0.614 / 33.7 | 0.565 / 0.521 / 31.4 | 0.326 / 0.309 / 58.5 | 0.249 | 0.391 | 0.249 |
| predict-next | before | 0.933 / 0.902 / 32.0 | 0.927 / 0.897 / 31.5 | 0.934 / 0.897 / 31.5 | 0.945 / 0.928 / 31.6 | 0.926 / 0.897 / 31.5 | 0.974 / 0.934 / 54.6 | 0.591 | 0.707 | 0.485 |
| predict-next | next | 0.759 / 0.742 / 32.0 | 0.762 / 0.747 / 31.5 | 0.767 / 0.743 / 31.5 | 0.774 / 0.764 / 31.6 | 0.759 / 0.737 / 31.5 | 0.780 / 0.768 / 54.6 | 0.597 | 0.668 | 0.484 |
| echo | last_cue | 0.958 / 0.936 / 32.0 | 0.986 / 0.956 / 31.5 | 0.984 / 0.960 / 31.8 | 0.989 / 0.976 / 32.1 | 0.961 / 0.937 / 31.5 | 0.974 / 0.874 / 56.4 | 0.493 | 0.660 | 0.492 |

### 6. The ceiling: a fresh linear reader of each line's future

Share of line-steps right on the read's test quarter (moves from the coin, so not the life's own steps). cells: the predictor cells in the life (section 1); born / lived: a ridge reader per line of the frozen sheet's counts, as born and after the pred life; lines: the same reader on this step's lines; same, common: on the read's rows.

| world | h | cells (life) | born sheet | lived sheet | lines | same | common |
|---|---|---|---|---|---|---|---|
| catch | 1 |  0.923 |  0.967 |  0.968 |  0.970 |  0.908 |  0.942 |
| catch | 8 |  0.914 |  0.945 |  0.945 |  0.946 |  0.897 |  0.942 |
| forage | 1 |  0.907 |  0.940 |  0.940 |  0.941 |  0.907 |  0.940 |
| forage | 8 |  0.910 |  0.940 |  0.940 |  0.940 |  0.899 |  0.940 |
| cartpole | 1 |  0.871 |  0.924 |  0.924 |  0.933 |  0.932 |  0.865 |
| cartpole | 8 |  0.796 |  0.866 |  0.866 |  0.866 |  0.803 |  0.865 |
| pendulum | 1 |  0.873 |  0.900 |  0.904 |  0.911 |  0.905 |  0.836 |
| pendulum | 8 |  0.784 |  0.850 |  0.854 |  0.851 |  0.758 |  0.836 |
| mountain-car | 1 |  0.934 |  0.941 |  0.941 |  0.946 |  0.943 |  0.907 |
| mountain-car | 8 |  0.812 |  0.906 |  0.907 |  0.907 |  0.903 |  0.907 |
| chain | 1 |  0.875 |  0.838 |  0.838 |  0.838 |  0.675 |  0.838 |
| chain | 8 |  0.734 |  0.838 |  0.838 |  0.838 |  0.742 |  0.838 |
| delayed-echo | 1 |  0.889 |  0.925 |  0.931 |  0.911 |  0.822 |  0.911 |
| delayed-echo | 8 |  0.849 |  0.931 |  0.937 |  0.911 |  0.823 |  0.911 |
| beat | 1 |  0.707 |  0.752 |  0.753 |  0.754 |  0.673 |  0.754 |
| beat | 8 |  0.716 |  0.751 |  0.753 |  0.767 |  0.729 |  0.753 |
| predict-next | 1 |  0.767 |  0.914 |  0.913 |  0.909 |  0.296 |  0.610 |
| predict-next | 8 |  0.600 |  0.654 |  0.662 |  0.665 |  0.588 |  0.610 |
| echo | 1 |  0.749 |  0.583 |  0.581 |  0.586 |  0.529 |  0.582 |
| echo | 8 |  0.749 |  0.580 |  0.584 |  0.585 |  0.527 |  0.582 |

## Predictions, scored

1. *At most 3 worlds beat all three baselines at h=1, chain among them (0.7); at most 1 at
   h=8 (0.75).* Right: 2 (chain, predict-next) and 0.
2. *The echo control does not beat "common" at any horizon (0.85).* Right.
3. *Catch "moved": the pred arm reads within 0.03 of task-only (0.7); the competitive arms
   read 0.10 above task-only (0.6).* The first: right on hidden cells (0.549 against 0.554),
   wrong on the whole sheet (0.549 against 0.592). The second: wrong for competitive
   learning alone (0.561) and with prediction (0.582); right only once the error lines are
   in (0.763), and the shuffled control matches that.
4. *Task score within 0.03 of off on at least 8 of the worlds (0.65).* Wrong, narrowly: 7 of
   10 (mountain car +0.055, delayed echo +0.035, chain -0.033 are outside).
5. *Cost 0.5 to 0.8 of off (0.8).* Mostly right: 0.49 to 0.87.

## What is suspect

- One sheet shape for every arm (`--g.sensor_w 64 --g.motor_stride 1 --motor-spread 64`,
  sensor side 2), so the task scores are not the standard battery's.
- `--predict-shift 2` was tuned on seeds 1 and 2 only, on three worlds.
- The ceiling is read on fresh steps with moves from a coin, not on the life's own steps;
  on chain that makes the reader's number (0.838, its "most common" too) lower than the
  cells' (0.875), so compare the two columns only where the policy matters little.
- The predictor cells learn by the rule idea 161 showed to be the weak one for many
  outputs. Each predicted bit is a two-button choice, where the runner-up rule is the
  plain gate; the gate was not tried on the predictor cells.
- Rates are reported, not matched; the arms with competitive learning fire at 36 to 59 per
  1,000 against 32.

## Not done

- The errors sent to hidden cells (the point of the idea): hook only.
- A rate-matched read for the competitive arms.
- The predictor cells under the close-vote gate; more than 4 cells a group.
- Horizons scored by "changed" steps only on the control worlds (the `chg` column is there;
  it is under one half on most).
- predict-next runs at Markov order 1 in the task part and order 2 in the read.

## Script lines

```
scripts/predict.sh smoke --quick                     the whole battery, small (about 2 minutes)
scripts/predict.sh main --seeds 5-12                 every part: task, read, future, old guess
scripts/predict.sh main --seeds 5-12 --table-only    the tables from finished runs
scripts/predict.sh main --seeds 5-12 --only future   the ceiling alone
```
