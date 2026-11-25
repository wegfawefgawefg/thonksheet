# Prediction errors down the one-hop wires (idea 164, 2026-11-25)

Queue item 3b of `docs/plan.md`. The owner: "in all of our envs we should extract rich
signal. thonksheet shouldnt just output policy right. shouldnt it predict current state, next
state, future states ... that info aids the goal and feature learning." Idea 162 built
predictor cells for every input line but nothing in it could change a hidden cell; idea 159's
one-hop rule teaches hidden cells from a handful of button errors. This joins them: each
input line's prediction error is an error line of the one-hop rule, so every hidden cell is
taught by what the sheet failed to predict, with no reward.

## Predictions (written before any report-seed run)

Seen before writing this: one smoke pass of the battery (`scripts/predhop.sh smoke --quick`:
seed 1, 2,000-step lives, 600 read steps), which says only that the code runs.

1. The hidden cells' reader of the future (h = 1 or 8) rises 0.02 or more over born on at
   least 2 worlds (mean of 8 seeds) and beats both the coin control and the time-shuffled
   life there by 0.01 or more. Probability 0.2. Reason: in idea 162 a reader of the lines
   did as well as a reader of the sheet nearly everywhere; with the read's moves from a coin
   most of these worlds' futures are in the lines, and the rule must move slowly.
2. In the hop arm the hidden cells' reader beats the reader of the lines by 0.01 or more on
   at least 2 worlds where the born sheet does not. Probability 0.15.
3. The predictor cells' own skill (h = 1, share right) rises 0.01 or more over the rule-off
   life on at least 3 of the 10 worlds, and the coin control's does not. Probability 0.25.
4. Time since the cue (delayed echo), hidden cells, rises 0.02 or more over born, at a firing
   rate within 2 spikes per 1,000 of born's. Probability 0.3.
5. Local wires beat random wires on the hidden cells' reader of the future, mean over worlds
   and both horizons. Probability 0.35.
6. The task score with the rule on is within 0.03 of the rule-off life (cartpole: within one
   step a fall) on at least 8 of 10 worlds. Probability 0.7.
7. Cost: the rule on runs at 0.80 to 0.95 of the rule-off life's ticks per second.
   Probability 0.75.
8. Both teachers together beat the buttons alone on the hidden future reader by 0.01 on at
   least 2 worlds. Probability 0.2.

## Short version (8 seeds, 5 to 12; `scripts/predhop.sh main`)

- **Does the inside come to hold more of the future than the lines? No, beyond what its
  controls get.** A fresh ridge reader of the hidden cells, asked every predicted line 1 and
  8 steps on, moves by less than 0.01 from born on 8 of the 10 worlds. Where it moves,
  predict-next (Markov order 2) one step ahead, 0.822 born to 0.870 lived (8 seeds of 8),
  the coin control gets 0.861 and the time-shuffled life 0.864: the gain is churn, not the
  errors (finding 47 again). Delayed echo: 0.920 to 0.933, coin 0.935.
- **The inside read: yes, on two questions, beating both controls at the same firing rate.**
  Which way the paddle moved (catch), hidden cells: born 0.521, rule off 0.549, **prediction
  errors 0.742**, coin 0.672, time-shuffled life 0.649 (+0.070 and +0.093, 7 seeds of 8 each;
  rates 33.0 / 31.5 / 31.7). Forage's last move: born 0.275, **0.324**, coin 0.253, shuffled
  0.256 (+0.070 and +0.067, 8 of 8; rates 31.5 / 31.5 / 31.4). Both are the step-to-step
  change of the lines: cells taught by "I said the paddle stays and it moved" come to carry
  the change. Time since the cue rises 0.859 to 0.956 at born's rate (31.5 against 32.1),
  but the coin gets 0.963: churn again.
- **The predictor cells' own skill: not lifted beyond churn.** Horizon 1, prediction errors
  against the rule off: predict-next +0.015 (7 of 8; coin +0.008), chain +0.011 (coin
  +0.023), delayed echo +0.005 (coin +0.005), elsewhere within 0.002. The loop does not
  close in a way the controls cannot match.
- **The task: unchanged** within 0.03 on 9 of 10 worlds (chain -0.075, 5 seeds of 7 lower).
  The buttons' errors alone (idea 159 as it is) lift cartpole by 1.3 steps a fall (8 of 8).
- **Local wires are worse than random ones** on nearly every read (catch moved 0.620 against
  0.742; predict-next one step ahead 0.843 against 0.870). **Both teachers** read like the
  prediction errors alone; the buttons alone like the rule off.
- **Cost:** 0.76 to 0.86 of the rule-off life's ticks per second.

The three numbers that matter, with their controls (hidden cells, 8 seeds):

| | born | rule off | prediction errors | coin | time-shuffled |
|---|---|---|---|---|---|
| future of every line, predict-next h=1 (lines alone 0.727) | 0.822 | 0.821 | 0.870 | 0.861 | 0.864 |
| catch: which way the paddle moved (rate) | 0.521 (32.2) | 0.549 (31.3) | **0.742** (33.0) | 0.672 (31.5) | 0.649 (31.7) |
| forage: last move (rate) | 0.275 (32.0) | 0.267 (31.4) | **0.324** (31.5) | 0.253 (31.5) | 0.256 (31.4) |

## What was built

- **Prediction errors as error lines** (`src/blob/learn_hop.cpp`, about 50 lines at the edges;
  `blob_hop_errors`, `blob_hop_error_places` and `HOP_MAX_PRED` in `blob.hpp`). At each
  step's start the predictor (`src/run/predict.cpp`, `predict_step_begin`) has the horizon-1
  answer of every predicted line graded against the line's value: "said off, was on" is +1,
  "said on, was off" is -1, right is 0 (`predict_errors()`, idea 162's hook). With
  `--predict-hop 1` it hands them to the blob; the one-hop rule takes them on the next tick as
  more error lines (wire line 64 + the target's number, so their hashed wires differ from the
  buttons'), beside the buttons' lines when there are any, and everything after that is idea
  159's rule unchanged: each hidden excitatory cell's teaching bit is the sign of the sum of
  its wires times the errors; its marked sources move their hidden counters with chance
  2^-`hop_shift`; it re-picks its top counters at its born counts only when a counter crossed
  a wired one.
- **No reward needed**: `--g.hop_teach 0` (new value; 1 and 2 as before) sends no button
  errors, so in the "prediction errors only" arm nothing about reward reaches a hidden cell.
- **The coin control** (`--g.hop_coin 1`) sends each prediction error to a prediction line
  drawn at random (each button error to a random button line, as before).
- **Local wires** (`--predict-hop-reach R`): line l's error reaches only the hidden cells
  within R cells (each way) of the centre of its sensor group, with a hashed sign; every cell
  in reach is wired (no thinning). This is the predictive-coding form: a cell hears the
  errors of the input lines next to it. Hidden cells far from every line hear nothing.
- **Off by default.** `--predict-hop` is a run flag (`src/world/predict_flags.cpp`,
  `predict_settings.hpp`); it needs `--g.hop_on 1`, and `--predict 1,...` with horizon 1.
  With it off, a prediction life (blobrun, catch, predictor cells and error lines, 3,000
  steps) is byte-identical to main's, `blobprobe predict`'s numbers are identical, and the
  four reference lives are the same (`out/predhop/cmpref.sh`).

**The timing.** A prediction for step t + 1 is made from the predictor cells' spikes over
step t; its error is known when step t + 1's lines arrive, at that step's first tick. So the
marks must be "this source was on while the cell was near its bar during step t": the cells
that were near when the prediction was made. For horizon 1 nothing has to be kept for that:
the predictor grades at step t + 1's start before any of its ticks, and the one-hop rule
applies the errors on that first tick, when the blob's own rings (16 ticks of spikes and of
"near" words) still hold the last 16 ticks of step t. The window (`hop_window`, 12) is the
last 12 of step t's 20 ticks. That is the same timing the buttons' errors have: the reward for
step t arrives after its last tick and is learned on the next. A longer horizon would need
the step-t near words kept for h steps, a ring of per-step near maps beside the predictor's
ring of spike maps (16 steps x 65 words a step for the sheet); not built, since only horizon 1
is sent (`--predict-hop` refuses a shortest horizon other than 1).

**The measure** (`blobprobe predict`, small additions in `src/probe/predict_read.cpp`):

- the future reader (`--future 1,8`) now also reads from the hidden cells only (no sensor,
  no motor cell: the predictor cells and the buttons are motor cells, so their own learning
  cannot show here) and from the hidden cells and the lines together (keys `h<h>_hidden`,
  `h<h>_both`);
- the life hands the one-hop rule the buttons' lines (`plug_hop_lines`), as blobrun does;
- with `--out`, the life's own numbers: the task's late score (last tenth of its windows),
  ticks per second, the predictor cells' skill per horizon, and the rule's counts.

**The battery** (`scripts/predhop.sh NAME [--quick]`, `scripts/predhop.py`): one
`blobprobe predict` run per world, arm and seed on the prediction battery's sheet and flags
(`scripts/predict.py`; track dropped as there; predict-next at Markov order 2 in the life and
the read). A 20,000-step life, freeze, 3,000 read steps with moves from a coin. Arms, each with
the predictor cells at 1, 2, 4, 8 steps:

| arm | what teaches the hidden cells |
|---|---|
| born | nothing (no life) |
| pred | nothing (the rule off; the predictor cells and buttons learn) |
| hop | the prediction errors only (`--g.hop_on 1 --g.hop_teach 0 --predict-hop 1`) |
| coin | the same errors, each on a random prediction line (`--g.hop_coin 1`) |
| shuf | the hop life's frames replayed in a shuffled order to a fresh sheet of the same seed, no task, no reward: its prediction errors teach (the time-shuffled life) |
| buttons | the buttons' errors only (idea 159 as it is, `--g.hop_teach 2`) |
| both | the buttons' and the predictions' |
| local | the prediction errors on local wires (`--predict-hop-reach`) |
| compete | hop with competitive learning (`--g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5`) |

The rule's setting in every arm: `--g.hop_shift 5 --g.hop_fired 0` (near-not-fired marks,
finding 46; a step with chance 1 in 32, picked on seeds 1 and 2 below), local reach 8, the
rest idea 159's defaults (half the lines wired to a cell, window 12, near = within a quarter
of the bar). Runs read lean (`--lean 1`, new): only the hidden cells' and the lines' readers,
since the ridge fits are most of a run's time (perf: `future_fit` and `ridge_solve` 99%).

## Tuning (seeds 1 and 2 only)

Three speeds of the rule, the prediction errors only (arm hop) against its coin control and
its time-shuffled life, every world, seeds 1 and 2. Lines:

```
scripts/predhop.sh tune7 --seeds 1,2 --g.hop_shift 7 --reach 4 --full        every arm
scripts/predhop.sh tune5 --seeds 1,2 --arms hop,coin,shuf --g.hop_shift 5     (stevejobs)
scripts/predhop.sh tune3 --seeds 1,2 --arms hop,coin,shuf --g.hop_shift 3     (stevejobs)
scripts/predhop.sh tune5b --seeds 1,2 --arms buttons,both,local,compete --g.hop_shift 5 --reach 4
scripts/predhop.sh tune5r8 --seeds 1,2 --arms local --reach 8 --g.hop_shift 5
```

(The born and rule-off runs are tune7's; they do not depend on the rule's numbers. tune7
was run before the defaults changed, so its line names the old ones.) Hidden cells, mean of
seeds 1 and 2, born / hop / coin / shuffled:

| question | step 1 in 128 (shift 7) | 1 in 32 (shift 5) | 1 in 8 (shift 3) |
|---|---|---|---|
| catch: which way the paddle moved | 0.535 / 0.670 / 0.611 / 0.648 | 0.535 / **0.806** / 0.703 / 0.741 | 0.535 / 0.773 / 0.728 / 0.699 |
| forage: last move | 0.269 / 0.305 / 0.275 / 0.277 | 0.269 / **0.322** / 0.249 / 0.243 | 0.269 / 0.325 / 0.253 / 0.255 |
| cartpole: last push | 0.456 / 0.533 / 0.499 / 0.513 | 0.456 / 0.535 / 0.536 / 0.523 | 0.456 / 0.541 / 0.516 / 0.531 |
| chain: last press | 0.593 / 0.605 / 0.614 / 0.584 | 0.593 / 0.611 / 0.580 / 0.587 | 0.593 / 0.595 / 0.599 / 0.595 |
| delayed echo: time since the cue | 0.953 / 0.969 / 0.960 / 0.955 | 0.953 / 0.975 / 0.977 / 0.955 | 0.953 / 0.981 / 0.975 / 0.978 |
| predict-next: the symbol before | 0.909 / 0.974 / 0.960 / 0.971 | 0.909 / 0.976 / 0.976 / 0.971 | 0.909 / 0.963 / 0.959 / 0.975 |
| echo: last cue | 0.929 / 0.969 / 0.963 / 0.963 | 0.929 / 0.983 / 0.980 / 0.953 | 0.929 / 0.973 / 0.989 / 0.969 |
| the future, hidden cells: hop minus born, mean of 10 worlds x 2 h | +0.000 | +0.004 | +0.003 |
| ... hop minus coin / minus shuffled | -0.000 / -0.002 | +0.004 / +0.003 | +0.002 / +0.001 |

Picked: shift 5 (1 in 32), where the reads beat both controls most often and by most (catch
+0.10 over the coin, forage +0.07). Every life moves the inside a lot by idea 159's
standard: at shift 7, 5,000 to 20,000 teaching steps a life (every step with a wrong line),
300 to 500 cells taught each, 40,000 to 270,000 re-picks over about 2,500 learners. Local
wires: reach 4 (a 9 x 9 box round each line's group) teaches about 40 cells a step against
330 for the random wires, so the report uses reach 8 (17 x 17); on seeds 1 and 2 it is
still under the random wires on most reads (catch moved -0.16).

The future barely moves at any speed (means within 0.005 of born), so it was not a
criterion. One place stood out on these seeds at shift 5: predict-next one step ahead,
hidden cells 0.882 against born 0.839, coin 0.851, shuffled 0.866 (the lines 0.779).

## The tables (`scripts/predhop.sh main`; every number in `out/predhop/main.md`)

Seeds 5 to 12; lives of 20,000 steps; 3,000 read steps with moves from a coin. Seeds 5 to
10 ran here, 11 and 12 on stevejobs (`python3 scripts/predhop.py main --seeds 11,12 --arms
...`, byte-identical between the machines); `scripts/predhop.sh main --table-only` gathers
them.

### 1. The future: the hidden cells' reader of every predicted line h steps on

Share of line-steps right on the read's test quarter. "lines": the same reader on this
step's lines (the same rows in every arm).

| world | h | lines | born | rule off | errors (hop) | coin | shuffled | buttons | both | local | compete |
|---|---|---|---|---|---|---|---|---|---|---|---|
| catch | 1 | 0.970 | 0.968 | 0.968 | 0.968 | 0.964 | 0.963 | 0.966 | 0.968 | 0.967 | 0.962 |
| catch | 8 | 0.946 | 0.945 | 0.945 | 0.945 | 0.944 | 0.944 | 0.945 | 0.945 | 0.945 | 0.942 |
| forage | 1 | 0.941 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 |
| forage | 8 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 | 0.940 |
| cartpole | 1 | 0.933 | 0.923 | 0.923 | 0.925 | 0.923 | 0.924 | 0.919 | 0.923 | 0.924 | 0.924 |
| cartpole | 8 | 0.866 | 0.865 | 0.866 | 0.866 | 0.865 | 0.865 | 0.866 | 0.866 | 0.866 | 0.865 |
| pendulum | 1 | 0.911 | 0.897 | 0.900 | 0.904 | 0.901 | 0.900 | 0.902 | 0.904 | 0.903 | 0.903 |
| pendulum | 8 | 0.851 | 0.849 | 0.851 | 0.854 | 0.852 | 0.848 | 0.850 | 0.853 | 0.853 | 0.855 |
| mountain car | 1 | 0.946 | 0.940 | 0.939 | 0.941 | 0.942 | 0.938 | 0.940 | 0.941 | 0.942 | 0.942 |
| mountain car | 8 | 0.907 | 0.906 | 0.906 | 0.908 | 0.907 | 0.909 | 0.907 | 0.908 | 0.908 | 0.908 |
| chain | 1 | 0.838 | 0.838 | 0.838 | 0.837 | 0.837 | 0.837 | 0.837 | 0.838 | 0.837 | 0.838 |
| chain | 8 | 0.838 | 0.838 | 0.838 | 0.838 | 0.838 | 0.838 | 0.838 | 0.837 | 0.837 | 0.838 |
| delayed echo | 1 | 0.911 | 0.920 | 0.927 | 0.933 | 0.935 | 0.929 | 0.931 | 0.935 | 0.929 | 0.919 |
| delayed echo | 8 | 0.911 | 0.929 | 0.931 | 0.937 | 0.936 | 0.933 | 0.933 | 0.939 | 0.931 | 0.919 |
| beat | 1 | 0.754 | 0.754 | 0.749 | 0.752 | 0.753 | 0.754 | 0.753 | 0.754 | 0.751 | 0.754 |
| beat | 8 | 0.767 | 0.753 | 0.753 | 0.748 | 0.753 | 0.753 | 0.751 | 0.751 | 0.753 | 0.753 |
| predict-next | 1 | 0.727 | 0.822 | 0.821 | **0.870** | 0.861 | 0.864 | 0.824 | 0.870 | 0.843 | 0.851 |
| predict-next | 8 | 0.703 | 0.708 | 0.704 | 0.722 | 0.711 | 0.717 | 0.701 | 0.722 | 0.708 | 0.720 |
| echo | 1 | 0.586 | 0.582 | 0.580 | 0.579 | 0.583 | 0.582 | 0.586 | 0.579 | 0.575 | 0.577 |
| echo | 8 | 0.585 | 0.580 | 0.586 | 0.587 | 0.583 | 0.582 | 0.584 | 0.585 | 0.581 | 0.583 |

Paired over seeds (mean; seeds above / below), hidden cells, where anything moved:

| world | h | errors - born | errors - rule off | errors - coin | errors - shuffled | born: hidden - lines | errors: hidden - lines |
|---|---|---|---|---|---|---|---|
| pendulum | 1 | +0.006 (8/0) | +0.004 (8/0) | +0.003 (7/1) | +0.004 (8/0) | -0.013 | -0.007 |
| pendulum | 8 | +0.004 (8/0) | +0.003 (7/1) | +0.001 (4/4) | +0.005 (7/1) | -0.002 | +0.002 |
| delayed echo | 1 | +0.013 (7/1) | +0.005 (5/3) | -0.003 (3/5) | +0.004 (6/2) | +0.009 | +0.022 |
| delayed echo | 8 | +0.008 (6/2) | +0.006 (6/2) | +0.002 (8/0) | +0.004 (8/0) | +0.018 | +0.026 |
| predict-next | 1 | +0.047 (8/0) | +0.049 (8/0) | +0.008 (6/2) | +0.006 (6/2) | +0.096 | +0.143 |
| predict-next | 8 | +0.014 (6/2) | +0.018 (6/1) | +0.010 (5/2) | +0.004 (4/2) | +0.005 | +0.019 |

Every other world and horizon: errors minus born within 0.003, except beat h = 8 (-0.005,
1 seed above, 4 below) and echo h = 8 (+0.007, 6 above, 2 below; echo's future is a coin).

Per seed (5 to 12), hidden minus lines, born / errors / coin / shuffled. Predict-next h = 1:
+0.137 / +0.195 / +0.168 / +0.187; +0.142 / +0.181 / +0.168 / +0.175; +0.033 / +0.044 /
+0.048 / +0.055; +0.081 / +0.164 / +0.177 / +0.153; +0.141 / +0.217 / +0.201 / +0.197;
+0.110 / +0.195 / +0.175 / +0.187; +0.015 / +0.023 / +0.021 / +0.024; +0.105 / +0.125 /
+0.117 / +0.115. Delayed echo h = 1: +0.025 / +0.021 / +0.025 / +0.017; +0.003 / +0.028 /
+0.023 / +0.018; +0.003 / +0.017 / +0.014 / +0.002; +0.001 / +0.021 / +0.022 / +0.020;
+0.006 / +0.022 / +0.029 / +0.015; +0.008 / +0.025 / +0.033 / +0.025; +0.015 / +0.028 /
+0.024 / +0.025; +0.010 / +0.010 / +0.023 / +0.017. Every world's line is in
`out/predhop/main.md`.

What the table says. In these worlds, read with moves from a coin, the future of the lines is
in the lines: the hidden cells' reader is at or under the lines' reader everywhere except
where the next value depends on the past (delayed echo, predict-next at order 2), and there
the born sheet already holds that past (+0.096 on predict-next). A life with the prediction
errors adds to it, but the same life with the errors on random lines, or on shuffled frames,
adds nearly as much: the rewiring itself (cells re-picking their top counters at their born
counts, hundreds of thousands of times) makes the hidden cells hold the last symbol better.

### 2. The inside read (hidden cells only; share right / firing rate while read)

| world | question | born | rule off | errors | coin | shuffled | buttons | both | local | compete | lines now | most common |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| catch | which way the paddle moved | 0.521 / 32.2 | 0.549 / 31.3 | **0.742** / 33.0 | 0.672 / 31.5 | 0.649 / 31.7 | 0.587 / 35.4 | 0.725 / 33.6 | 0.620 / 33.2 | 0.842 / 38.7 | 0.450 | 0.451 |
| forage | last move | 0.275 / 32.0 | 0.267 / 31.4 | **0.324** / 31.5 | 0.253 / 31.5 | 0.256 / 31.4 | 0.307 / 31.4 | 0.331 / 31.5 | 0.333 / 31.5 | 0.281 / 35.6 | 0.221 | 0.189 |
| cartpole | last push | 0.473 / 32.2 | 0.470 / 31.5 | 0.515 / 38.1 | 0.505 / 35.4 | 0.494 / 38.0 | 0.518 / 45.3 | 0.530 / 44.6 | 0.498 / 37.0 | 0.502 / 58.2 | 0.472 | 0.336 |
| pendulum | last push | 0.332 / 32.1 | 0.337 / 31.4 | 0.337 / 31.8 | 0.344 / 31.6 | 0.340 / 31.6 | 0.336 / 31.6 | 0.338 / 32.3 | 0.335 / 31.6 | 0.340 / 38.4 | 0.339 | 0.336 |
| mountain car | last push | 0.331 / 32.1 | 0.347 / 31.3 | 0.334 / 32.5 | 0.338 / 31.4 | 0.338 / 32.2 | 0.328 / 31.8 | 0.337 / 33.6 | 0.345 / 32.7 | 0.346 / 55.4 | 0.344 | 0.336 |
| chain | last press | 0.605 / 32.1 | 0.595 / 31.4 | 0.600 / 34.5 | 0.594 / 37.2 | 0.590 / 31.8 | 0.609 / 31.4 | 0.591 / 34.5 | 0.593 / 32.8 | 0.591 / 62.3 | 0.584 | 0.336 |
| delayed echo | the cue | 0.706 / 32.1 | 0.707 / 31.4 | 0.618 / 31.5 | 0.619 / 31.6 | 0.681 / 31.5 | 0.674 / 31.4 | 0.631 / 31.6 | 0.707 / 31.4 | 0.668 / 41.8 | 0.582 | 0.498 |
| delayed echo | time since the cue | 0.859 / 32.1 | 0.897 / 31.4 | 0.956 / 31.5 | 0.963 / 31.6 | 0.924 / 31.5 | 0.941 / 31.4 | 0.977 / 31.6 | 0.901 / 31.4 | 0.811 / 41.8 | 0.500 | 0.167 |
| beat | beat phase | 0.497 / 32.1 | 0.509 / 31.3 | 0.509 / 31.5 | 0.508 / 31.5 | 0.504 / 31.5 | 0.512 / 32.1 | 0.519 / 32.1 | 0.500 / 31.4 | 0.317 / 63.0 | 0.249 | 0.249 |
| predict-next | the symbol before | 0.902 / 32.1 | 0.897 / 31.4 | 0.971 / 35.5 | 0.960 / 34.2 | 0.966 / 34.6 | 0.895 / 31.5 | 0.971 / 35.5 | 0.953 / 33.3 | 0.961 / 60.2 | 0.591 | 0.485 |
| predict-next | the next symbol | 0.742 / 32.1 | 0.743 / 31.4 | 0.797 / 35.5 | 0.777 / 34.2 | 0.792 / 34.6 | 0.738 / 31.5 | 0.797 / 35.5 | 0.764 / 33.3 | 0.778 / 60.2 | 0.597 | 0.484 |
| echo | last cue | 0.936 / 32.1 | 0.960 / 31.4 | 0.978 / 31.6 | 0.979 / 31.8 | 0.958 / 31.7 | 0.953 / 31.5 | 0.978 / 31.7 | 0.981 / 31.4 | 0.915 / 61.9 | 0.493 | 0.492 |

Paired, hidden cells (mean; seeds above / below):

| world | question | errors - born | errors - coin | errors - shuffled | both - buttons | local - errors | compete - errors |
|---|---|---|---|---|---|---|---|
| catch | moved | +0.221 (8/0) | **+0.070 (7/1)** | **+0.093 (7/1)** | +0.138 (7/1) | -0.122 (0/8) | +0.099 (7/1) |
| forage | last move | +0.048 (8/0) | **+0.070 (8/0)** | **+0.067 (8/0)** | +0.024 (6/2) | +0.010 (4/4) | -0.043 (0/8) |
| cartpole | last push | +0.042 (7/1) | +0.010 (6/2) | +0.021 (6/2) | +0.012 (6/2) | -0.017 (3/5) | -0.013 (3/5) |
| pendulum | last push | +0.006 (5/3) | -0.007 (3/5) | -0.003 (4/4) | +0.001 (5/3) | -0.002 (3/4) | +0.003 (3/5) |
| mountain car | last push | +0.003 (4/4) | -0.004 (3/5) | -0.004 (4/4) | +0.010 (5/3) | +0.011 (5/3) | +0.012 (6/2) |
| chain | last press | -0.005 (4/4) | +0.006 (5/3) | +0.010 (4/4) | -0.019 (1/7) | -0.007 (2/6) | -0.009 (4/4) |
| delayed echo | the cue | -0.088 (0/8) | -0.001 (5/3) | -0.063 (0/7) | -0.042 (0/8) | +0.089 (8/0) | +0.050 (7/1) |
| delayed echo | time since the cue | +0.097 (8/0) | -0.007 (3/5) | +0.032 (8/0) | +0.036 (7/0) | -0.055 (0/8) | -0.144 (0/8) |
| beat | beat phase | +0.012 (5/3) | +0.001 (3/5) | +0.005 (6/2) | +0.006 (4/4) | -0.009 (2/5) | -0.192 (1/7) |
| predict-next | the symbol before | +0.069 (8/0) | +0.012 (5/3) | +0.006 (5/3) | +0.076 (8/0) | -0.018 (0/8) | -0.011 (2/6) |
| predict-next | the next symbol | +0.055 (8/0) | +0.020 (6/2) | +0.005 (5/3) | +0.059 (7/1) | -0.033 (2/6) | -0.019 (2/5) |
| echo | last cue | +0.041 (8/0) | -0.001 (2/6) | +0.019 (7/1) | +0.025 (6/2) | +0.004 (5/3) | -0.063 (0/8) |

Per seed (5 to 12), born / errors / coin / shuffled. Catch moved: 0.507 / 0.812 / 0.637 /
0.625; 0.545 / 0.692 / 0.693 / 0.564; 0.511 / 0.691 / 0.636 / 0.705; 0.523 / 0.735 / 0.669 /
0.691; 0.469 / 0.851 / 0.704 / 0.648; 0.561 / 0.675 / 0.641 / 0.632; 0.525 / 0.752 / 0.681 /
0.677; 0.531 / 0.731 / 0.716 / 0.651. Forage last move: 0.280 / 0.292 / 0.232 / 0.269; 0.284
/ 0.329 / 0.277 / 0.265; 0.291 / 0.333 / 0.269 / 0.281; 0.255 / 0.331 / 0.248 / 0.231; 0.283
/ 0.352 / 0.231 / 0.253; 0.283 / 0.293 / 0.233 / 0.247; 0.279 / 0.287 / 0.256 / 0.252; 0.249
/ 0.371 / 0.281 / 0.253.

- **Which way the paddle moved, and the last move, are the step-to-step change of the
  lines**, which is what a horizon-1 error is about: "this line changed and I did not say
  so". A cell taught to fire more when the line it hears came on unannounced becomes, in
  effect, a change detector with a memory of the last step. The coin control and the
  shuffled life get part of it (0.672, 0.649 against 0.742), so part is churn; the rest is on
  7 or 8 seeds of 8. The rate does not explain it: 33.0 against 31.5 and 31.7 on catch, 31.5
  against 31.5 and 31.4 on forage.
- **The cue in delayed echo is lost** (0.706 to 0.618, 8 seeds of 8; the coin the same):
  rewiring costs the hidden cells some of what they held of a single past event.
- **Competitive learning beside it** reads the paddle best (0.842, rate 38.7) and costs time
  (time since the cue 0.811, beat phase 0.317): finding 50 again.
- **Local wires** reach 25 to 131 cells a step (random wires 300 to 530) and read worse
  nearly everywhere; on the cue in delayed echo they keep born's 0.707 because they leave
  most of the sheet alone.

### 3. The predictor cells' skill, the task, the cost

Horizon 1, share right in the last quarter of the life (seeds above / below):

| world | rule off | errors | coin | errors - rule off | coin - rule off | both - rule off |
|---|---|---|---|---|---|---|
| catch | 0.923 | 0.922 | 0.918 | -0.001 (2/6) | -0.005 (1/7) | -0.003 (4/4) |
| forage | 0.907 | 0.908 | 0.905 | +0.001 (7/1) | -0.002 (1/7) | +0.001 (7/1) |
| cartpole | 0.871 | 0.872 | 0.872 | +0.002 (5/3) | +0.001 (6/2) | +0.004 (8/0) |
| pendulum | 0.873 | 0.871 | 0.872 | -0.001 (2/6) | -0.001 (5/3) | -0.002 (1/7) |
| mountain car | 0.934 | 0.933 | 0.936 | -0.001 (3/5) | +0.002 (6/2) | +0.001 (5/3) |
| chain | 0.875 | 0.886 | 0.897 | +0.011 (4/4) | +0.023 (6/2) | +0.005 (3/5) |
| delayed echo | 0.889 | 0.893 | 0.894 | +0.005 (7/1) | +0.005 (7/1) | +0.009 (8/0) |
| beat | 0.707 | 0.707 | 0.707 | -0.000 (3/5) | +0.000 (6/2) | -0.001 (3/5) |
| predict-next | 0.687 | 0.702 | 0.696 | +0.015 (7/1) | +0.008 (7/1) | +0.015 (7/1) |
| echo | 0.749 | 0.750 | 0.750 | +0.001 (5/3) | +0.000 (4/4) | +0.001 (3/5) |

The balanced share and horizon 8 are in `out/predhop/main.md` (the same picture; with
competitive learning beside the rule chain goes to 0.965, and pendulum and mountain car at
h = 8 gain 0.02).

Task, late score (last tenth of the life), errors minus rule off: catch +0.000, forage
+0.008, cartpole +0.11 steps a fall (rule off 17.1), pendulum -0.016 (2/6), mountain car
-0.008, chain -0.075 (2/5), delayed echo +0.026 (5/3), beat -0.001, predict-next -0.002,
echo 0. The buttons' errors alone: cartpole +1.28 steps (8/0), delayed echo +0.030 (7/1),
chain -0.031. The coin control costs forage 0.061 (0/8).

Cost (ticks per second of the life, this PC, rough): the prediction errors run at 0.76 to
0.86 of the rule-off life (forage 21,800 against 28,600; cartpole 12,000 against 14,500); the
buttons' errors alone 0.88 to 0.97; local wires 0.90 to 0.97; with competitive learning 0.33
to 0.65. The rule teaches on 6,700 (chain) to 19,700 (catch) of the 20,000 steps, 300 to 530
cells a step, 170,000 to 790,000 re-picks a life over about 2,500 learners.

## What is real

- **Prediction errors sent one hop teach the hidden cells the step-to-step change of the
  input, with no reward.** Catch "which way the paddle moved" 0.521 born to 0.742, over the
  coin control by 0.070 and the time-shuffled life by 0.093 (7 seeds of 8 each); forage
  "last move" 0.275 to 0.324, over both by 0.07 (8 of 8). Rates matched within 1.5 spikes
  per 1,000. Two worlds, 8 seeds: likely.
- **They do not teach the inside more of the future than the lines hold.** Every world's
  future reader of the hidden cells stays within 0.01 of born, or (predict-next, delayed
  echo) moves and its coin control and shuffled life move nearly as much. The rewiring is
  heavy (up to 790,000 re-picks a life) and most of what it buys, the churn buys.
- **The predictor cells do not get better from it** beyond what the coin gives.

## What is suspect

- In the read the moves come from a coin, so in catch, forage, chain and the control worlds
  the next lines depend on a press nobody can know. "The future is in the lines" is partly
  the read's doing: on the life's own policy the press is part of the sheet. A read on the
  life's own moves would test that (not done).
- The coin control moves prediction errors to random prediction lines. In catch most real
  errors sit on the few lines that change (paddle, ball); the coin spreads them over 38.
  That makes the coin a weaker control of "how much moves" than of "which lines": its re-picks
  are 97% of the real arm's, but they land on other cells.
- One setting (shift 5, near-not-fired, half the lines wired, window 12), picked on seeds 1
  and 2 by the inside read, not by the future.
- The time-shuffled control replays the frames to a fresh sheet without the buttons or the
  task; its predictor learns on shuffled frames and is worse (its skill is not reported),
  so its errors are more numerous than the real life's.
- The hidden-plus-lines reader (`--full`, key `h<h>_both`) was dropped from the report: with
  one ridge strength for 2,500 cells and 40 lines it lost to the lines alone on the tuning
  runs, so it measures the reader, not the sheet.
- Rates are reported, not matched; cartpole and predict-next read at 35 to 38 spikes per
  1,000 against 32 born (their controls at 34 to 38 too).

## Predictions, scored

| prediction | p | outcome |
|---|---|---|
| 1. hidden future reader +0.02 over born on 2 worlds and over both controls by 0.01 | 0.2 | did not happen: predict-next h=1 +0.047 over born but +0.008 / +0.006 over coin / shuffled; nowhere else +0.02 |
| 2. hop's hidden reader beats the lines by 0.01 on 2 worlds where born does not | 0.15 | happened by the letter: delayed echo h=1 (born +0.009, hop +0.022) and predict-next h=8 (+0.005, +0.019); the coin gets the same (+0.024, +0.009) |
| 3. predictor skill +0.01 on 3 worlds, the coin's not | 0.25 | did not happen: predict-next +0.015 (coin +0.008), chain +0.011 (coin +0.023) |
| 4. time since the cue +0.02 over born at matched rate | 0.3 | happened: 0.859 to 0.956 at 31.5 against 32.1; but the coin gets 0.963 |
| 5. local wires beat random wires | 0.35 | did not happen: worse on most reads and the future |
| 6. task within 0.03 on 8 of 10 | 0.7 | happened: 9 of 10 (chain -0.075) |
| 7. cost 0.80 to 0.95 | 0.75 | wrong: 0.76 to 0.86 (four worlds under 0.80) |
| 8. both beat buttons on the future by 0.01 on 2 worlds | 0.2 | did not happen: one world (predict-next, +0.046 and +0.020) |

I did not predict the one clear positive, the change of the lines (catch, forage).

## Not done

- **The calculation battery check** (the brief's step 6, "if and only if 1 to 3 show
  something"): the main test (1) and the loop (3) showed nothing beyond the controls, and
  the calculation battery shows no answer as an input line for the predictor to be wrong
  about; making it do so is a world change, not a static check.
- Horizons other than 1 (the per-step ring of near maps; the timing section says what it
  would be).
- A read on the life's own moves instead of a coin.
- Rate-matched reads (`--g.threshold_cap 1000`).
- The hidden-plus-lines reader with a separate ridge strength per block.

## Script lines

```
scripts/predhop.sh quick --quick         the whole battery, 1 seed, 2,000-step lives (90 runs)
scripts/predhop.sh main                  the report: seeds 5-12, every arm (720 runs)
scripts/predhop.sh main --table-only     the tables from finished runs (out/predhop/main.md)
scripts/predhop.sh tune7 --seeds 1,2 --g.hop_shift 7 --reach 4 --full     tuning, see above
out/predhop/cmpref.sh                    the four reference lives against main's out/merge-ref
```

Stevejobs (for the seeds 11 and 12 part of the report): `MINI_DIR=thonk-predhop
scripts/mini.sh push`, then `MINI_DIR=thonk-predhop scripts/mini.sh bg main 'python3
scripts/predhop.py main --seeds 11,12 --jobs 2'`, then `scripts/mini.sh pull` and copy
`out/mini/predhop/main/*` into `out/predhop/main/`.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`: a candidate. **A prediction error sent one hop teaches the inside the
  change of the input, not its future.** Each line's horizon-1 error as an error line of the
  one-hop rule, no reward: hidden cells read which way the paddle moved 0.52 to 0.74 (coin
  0.67, time-shuffled 0.65) and forage's last move 0.28 to 0.32 (0.25, 0.26), 8 seeds; the
  future of every line, read from the hidden cells, moves no more than under its coin
  control. [likely: two worlds, 8 seeds]
- Finding 47 gains a fourth case: the time-shuffled life and the coin control both match
  the prediction-error rule on the future reader and on time since the cue.
- Finding 50 again: the prediction-error rule beside competitive learning reads the paddle
  best (0.84) and loses time since the cue (0.81 against 0.96) and beat phase (0.32 against
  0.51).
- A side result for idea 159: on cartpole the buttons' errors alone lift the task by 1.3
  steps a fall (17.1 to 18.4, 8 seeds of 8) in this sheet shape.
- `docs/plan.md`: item 3b done (this doc). A follow-up worth a slot only if the owner wants
  the future: read on the life's own moves, where the press is part of what predicts the
  next lines.
- `docs/ideas.md`, idea 164: done, with the line above.
- `docs/notes-world.md` and `docs/design.md`: `--predict-hop`, `--predict-hop-reach`,
  `--g.hop_teach 0`, `blob_hop_errors`, `blob_hop_error_places`, `HOP_MAX_PRED` (blob.hpp;
  edited here because the rule needed them; core owner's file), `blobprobe predict --lean`
  and its new keys, `scripts/predhop.sh`.
