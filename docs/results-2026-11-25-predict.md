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
catch, cartpole and delayed echo). Catch gets 2 cells a group, cartpole and forage 3 to 4,
the rest 4. A first layout with the motor lattice across the whole bottom half at stride 2
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

## Status at hand-back (results not final)

The report battery was still running at hand-back: `scripts/predict.sh main --seeds 5-11
--jobs 3` on this PC (log `out/predict/main-local.log`, runs in `out/predict/main/`) and
`MINI_DIR=thonk-predict scripts/mini.sh bg predict-main 'scripts/predict.sh main --seeds 12
--jobs 2'` on stevejobs (pull with `MINI_DIR=thonk-predict scripts/mini.sh pull`, then copy
`out/mini/predict/main/*` into `out/predict/main/`). Tables: `scripts/predict.sh main --seeds
5-12 --table-only`. The `future` part (the ceiling) was added after the battery started and
has not run: `scripts/predict.sh main --seeds 5-12 --only future`.

Seed 5 only (one seed, not a claim), 20,000-step lives, from `scripts/predict.sh main --seeds
5 --table-only`:
- Skill at h=1 beats all three baselines only on chain (0.876 against same 0.262, common
  0.631, twin 0.540) and predict-next (0.780 against same 0.465, common 0.685, twin 0.505).
  Everywhere else and at every longer horizon it is under "same as now" or "common". On
  cartpole, pendulum and mountain car the cells do not even copy the present (h=1: 0.871,
  0.874, 0.927 against same 0.912, 0.902, 0.965): the output rule, not the information, looks
  like the limit (the ceiling part is built to test exactly this).
- The echo control sits at baseline (0.751 against common 0.755), as it should.
- Track is degenerate on this sheet: the readout's policy stops at an end (score 0.000 in
  every arm), so no line changes and every predictor is trivially right.
- Cost: prediction on runs at about 0.5 to 0.9 of the off arm's ticks per second (catch
  33,000 against 53,000).
