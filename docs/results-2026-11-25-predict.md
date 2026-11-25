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
