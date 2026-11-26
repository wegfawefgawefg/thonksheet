# Prediction errors through the every-event rule (idea 183, 2026-11-26)

Queue item 3c. Idea 164 (`docs/results-2026-11-25-predhop.md`) sent the next-step
predictor's wrong answers to every hidden cell down the one-hop wires; the cells learned
"what just changed" and nothing more of the future, and the task did not move. Since then
the rule's ladder and idea 178 (`docs/results-2026-11-26-every-event.md`) found that rule
was crippled: its counters stepped with a small chance, and the error had no fine zone.
This job runs prediction errors through the repaired rule: 12-bit counters that step on
every marked input, a signed error per prediction line with a fine zone, and the errors of
the far horizons as well as the next step's.

(The answer, the short version and the tables are written below the predictions once the
runs are in.)

## Predictions (written and committed before any scoring run)

Seen before writing these: the three docs named above, the code, and three things run here
that are not scoring runs:

- `out/predevent/refcheck.sh` and `out/predevent/oldcheck.sh` (the old paths give the same
  bytes as before).
- One smoke pass (`scripts/predevent.sh smoke --quick`: seed 1, 1,500-step lives, 500 read
  steps), which says the code runs. Two things in it caught my eye and are in the guesses
  below: with a fine zone of 8 32nds and all four horizons the hidden cells fired at 67
  spikes per 1,000 cell-ticks after 1,500 steps (born 32), and the every-event rule with
  wrong answers only read the paddle's last move at 0.75 after 1,500 steps (one seed, 500
  read steps: not a number).

The arms are named in `scripts/predevent.sh`: `old` is idea 164's rule; `ev<set>f<fine>` is
the every-event rule taught by the horizons in `<set>` (1: the next step alone, 15: 1, 2,
4 and 8, 14: the far three) with a fine zone of `<fine>` 32nds (0: wrong answers only).

1. **What just changed, stronger.** The every-event rule with the next step's wrong answers
   (`ev1f0`) reads catch's "which way the paddle moved" 0.03 or more above the old rule and
   0.05 or more above its own coin control: 0.55. Forage's last move 0.02 or more above the
   old rule: 0.4.
2. **The future.** In the best every-event arm the hidden cells' reader of the lines 1 or 8
   steps on (share right, or the balanced score) beats born by 0.02 and both its coin and
   its shuffled life by 0.01 on at least 2 of the worlds run: 0.15. Why so low: the read's
   moves come from a coin, the lines' own reader already holds what can be known, and idea
   164 found nothing here with the same targets.
3. **Far horizons.** Teaching from 2, 4 and 8 steps on (sets 15 or 14) adds 0.02 or more
   over the next step alone, beyond its controls, on some inside read or future reader in
   at least 2 worlds: 0.2.
4. **The fine zone.** A zone above 0 (lines teach when right but not by a margin) reads the
   paddle's move better than wrong answers only: 0.3. More likely the cells are taught on
   nearly every step, the rate climbs and the reads fall: 0.5.
5. **The right button.** A fresh reader of the right button from the taught hidden cells
   beats born by 0.02 and its coin by 0.01 on catch or forage: 0.2.
6. **The task.** The life's late score is within 0.03 of the rule-off life on catch and on
   forage: 0.6. It is 0.03 or more better on any of the task worlds run, most seeds the
   same way: 0.12. It is 0.03 or more worse on at least one: 0.35.
7. **The predictor cells.** Their own skill (balanced, next step) rises 0.01 or more over
   the rule-off life on catch or forage, and the coin's does not: 0.2.
8. **Rate.** The best every-event arm is read at more than 36 spikes per 1,000 cell-ticks
   (born 32): 0.6. With the rate held at target the paddle read keeps at least half of its
   gain over born: 0.6.
9. **Cost.** The every-event rule with four horizons runs at 0.5 to 0.8 of the rule-off
   life's ticks a second: 0.6; under 0.5: 0.3.
10. **Delayed echo.** The cue is read worse than born (as under the old rule, -0.088): 0.7.
    Time since the cue rises 0.05 over born: 0.6, and its coin control is within 0.02 of
    it: 0.6.
11. **Beside the walk.** Under the one setting's button rule the every-event prediction
    rule scores within 0.03 with and without the backward walk on the tasks run: 0.6.
12. **The one sentence** will be "still only what just changed, read more strongly; not the
    future; the task does not move": 0.65.

Seen after the predictions and before the report runs: one timing run (catch, seed 1, the
rule off beside `ev15f8`) and the screens below. One arm was added after the first screen
showed the predictor cells are a poor output: the predictor outside the sheet
(`--predict-outside`). No prediction above covers it.

## What was built

All off by default. One new genome number, `hop_pred_fine`, at the end of the genome and of
`genome_args.cpp`. With it off and the old flags, the four reference lives are
byte-identical (`out/predevent/refcheck.sh`) and idea 164's five arms and the plane path
give the same csv lines as the base commit's build (`out/predevent/oldcheck.sh`).

**The prediction lines have their own file** (`src/blob/hop_pred.cpp`). `learn_hop.cpp`
lost the eleven lines that gathered them and calls `hop_pred_errors` instead; nothing else
in the rule changed.

**A fine zone for a prediction line** (`--g.hop_pred_fine F`, in 32nds). A button line's
fine zone (`hop_fine`) compares a wrong line's spikes with the right line's. A prediction
line is one bit, answered by a pair of motor-cell groups, "off" and "on"; the group with
more spikes in the step answers. The like of the margin is the pair's split: the answer's
miss is the wrong group's share of the two groups' spikes (a silent pair misses by a
half). With F > 0 a line teaches, toward what it turned out to be, when its miss is F
32nds or more. So 3 is the ladder's 0.1 (the line teaches unless the right group has nine
tenths of the spikes), 16 is "wrong or tied", 24 is "wrong by a margin" (stricter than
the old error), and 0 is idea 164's error (wrong answers only, a tie says off). The
predictor hands the rule the two counts and the line's value (`blob_hop_predicted`); the
rule makes the signs, as it does for the buttons' votes.

**Far horizons** (`--predict-hop` is now a set of horizons: bit i for the i-th of
`--predict`'s list; 1 is the next step alone as before, 15 is 1, 2, 4 and 8, 14 the far
three). An answer about h steps ahead is graded h steps after the cells made it, when the
blob's rings no longer hold that step. The rule keeps a copy of its two rings (16 spike
maps, 16 near maps) at each step's end for the last 16 steps (`blob_hop_keep_step`), and a
far horizon's verdict reads its marks from the kept step: "this source was on while the
cell was near, in the step the answer was made". The teaching is the same code: the kept
rings stand in for the blob's for the length of one verdict. Each horizon has its own wire
lines, so a cell hears "line 7 one step on" and "line 7 eight steps on" through different
hashed wires. The 1-step horizon still waits for the next tick and uses the live rings.

**The coin for a prediction line** (`--g.hop_coin 3`): the cells get the errors of another
step of the same horizon, drawn from the last 256, so the errors keep their counts and
pairings and lose only their tie to what was shown (the ladder's coin; idea 164's coin put
each error on a random line, which changes how many cells a step teaches).

**A predictor outside the sheet, as a measuring stick** (`--predict-outside M`,
`src/run/predict_outside.cpp`). The every-event doc found the rule needs the error of an
output that is right most of the time and falls silent when it is right by a margin. The
sheet's predictor cells are not that (below). So, as that doc did for the labels, the
errors can come from a whole-number predictor that is no part of the blob: for every line
and horizon a row of 16-bit weights over the non-sensor cells' spike counts of the step; a
line that came on with a vote under M steps' worth takes a perceptron step up and is told
"higher", a line that stayed off with a vote over minus M steps' worth a step down and
"lower", the rest are fine and send nothing. The predictor cells still learn beside it.

**The measure** (`blobprobe predict`): the future reader also gives a balanced score (the
mean of the shares right on on- and off-line-steps; lines are mostly off, so the plain
share hides nearly everything) and a changed score (right on line-steps that differ from h
steps before); a new question, the right button of the step, for every world with
buttons; `--read-own 1` reads on the frozen sheet's own moves instead of a coin's; the
life's count of prediction lines graded and taught.

**The battery**: `scripts/predevent.sh NAME` (`scripts/predevent.py`, on
`scripts/predhop.py`'s sheet, worlds and helpers). One `blobprobe predict` run per world,
arm and seed: a 20,000-step life with the predictor cells at 1, 2, 4 and 8 steps, freeze,
3,000 read steps with moves from a coin. Arm names: `born`; `pred` the rule off (`predo2`:
the same life with the outside predictor learning beside it, the same sheet to the bit);
`old` idea 164's rule; `ev<set>f<fine>` the every-event rule (`--g.hop_bits 12
--g.hop_shift 0 --g.hop_teach 0`) taught by the horizons in `<set>` with fine zone
`<fine>`; `o2` after it: the errors come from the outside predictor at a margin of 2 steps;
then `c` its coin, `s` its life on shuffled frames, `k` the rate held at target.

## The screens (seeds 1 and 2; nothing here is a report number)

Every screen: catch, forage, delayed echo (4 blank steps) and predict-next (order 2), full
lives. `scripts/predevent.sh screen ...` (lines at the end).

### The fine zone on the cells' own answers: every zone but "wrong answers only" is worse

The next step's errors alone, every-event counters. "teach": the share of line-steps that
sent an error on catch.

| arm | a line teaches when | catch: paddle moved | forage: last move | predict-next: next symbol | teach | rate on catch |
|---|---|---|---|---|---|---|
| born | | 0.535 | 0.269 | 0.761 | | 32.1 |
| `old` | wrong (chance 1/32, 6 bits) | 0.806 | 0.322 | 0.813 | 0.089 | 33.9 |
| `ev1f0` | wrong | **0.907** | 0.342 | **0.849** | 0.089 | 38.8 |
| `ev1f0n` | wrong, near-not-fired marks | 0.883 | **0.370** | 0.841 | 0.085 | 38.1 |
| `ev1f24` | wrong by a margin (miss 0.75) | 0.753 | 0.309 | 0.733 | 0.029 | 44.3 |
| `ev1f16` | wrong or tied (0.5) | 0.630 | 0.297 | 0.802 | 0.137 | 58.2 |
| `ev1f12` | miss 0.375 or more | 0.595 | 0.309 | 0.783 | 0.236 | 43.1 |
| `ev1f8` | miss 0.25 or more | 0.489 | 0.331 | 0.784 | 0.451 | 38.2 |

A zone that also teaches on answers that were right but not by a margin (8, 12, 16)
teaches on up to half of all line-steps, runs the rate up (58 at "wrong or tied") and reads
the paddle worse than the old rule. A stricter zone than "wrong" (24) is worse too. The
fine zone works on MNIST because the output it sits on is right nine times in ten; these
cells are not (next section), so "right but unsure" is most of what they ever are.

### The predictor cells are a poor output

With the rule off (seeds 1, 2), the predictor cells' balanced skill one step on and eight
steps on, beside a whole-number perceptron reading the same sheet (`predo2`):

| world | cells, 1 step | perceptron, 1 step | cells, 8 steps | perceptron, 8 steps |
|---|---|---|---|---|
| catch | 0.626 | 0.841 | 0.583 | 0.586 |
| forage | 0.652 | 0.725 | 0.627 | 0.683 |
| delayed echo | 0.690 | 0.741 | 0.595 | 0.618 |
| predict-next | 0.714 | 0.859 | 0.621 | 0.720 |

On catch (seed 1) the cells' plain share right is 0.916 where "every line stays at its
common value" gets 0.942; they get 28% of the lines that come on. This is idea 178's lesson
again, with predictions in place of labels: the world hands over the right answer for
free, but the error is only as good as the output it is the error of.

### Horizons, with the cells' own errors

| arm | horizons that teach | catch: paddle moved | its coin / shuffled life | forage: last move | its coin / shuffled | delayed echo: cells' skill at 8 steps, less rule off / less coin |
|---|---|---|---|---|---|---|
| `ev1f0` | 1 | 0.907 | 0.577 / 0.749 | 0.342 | 0.249 / 0.281 | +0.020 / +0.005 |
| `ev15f0` | 1, 2, 4, 8 | 0.733 | 0.571 / 0.669 | 0.391 | 0.283 / 0.345 | +0.056 / +0.037 |
| `ev14f0` | 2, 4, 8 | 0.633 | 0.567 / 0.697 | 0.327 | 0.262 / 0.335 | +0.073 / +0.022 |
| `ev8f0` | 8 | 0.598 | | 0.311 | | +0.006 / |
| `old` | 1 | 0.806 | 0.703 / 0.741 | 0.322 | 0.249 / 0.243 | +0.041 / +0.014 |

The far horizons dilute the paddle read and add little that their coin does not. The coin
here (another step's errors) gets almost nothing on the paddle (0.577, born 0.535), where
idea 164's coin (the same errors on random lines) got 0.703; the shuffled-frame life gets
0.749, because its errors are still about what changed between two frames.

### The outside predictor's errors

| arm | delayed echo: cells' skill at 8 steps (rule off 0.595) | its coin | outside predictor at 8 steps (0.618) | its coin | catch: paddle moved | its coin |
|---|---|---|---|---|---|---|
| `ev1f0o2` | 0.770 | 0.604 | 0.758 | 0.608 | 0.747 | 0.645 |
| `ev15f0o2` | 0.736 | 0.601 | 0.744 | 0.621 | 0.664 | 0.536 |
| `ev14f0o2` | 0.750 | 0.622 | 0.754 | 0.625 | 0.694 | 0.573 |

On delayed echo both predictors get much better when the outside predictor's errors teach
the cells, and not under the coin. That went to the report. On catch and forage the
outside errors read the paddle less well than the cells' own (0.75 against 0.91) and move
no predictor.

## The report: catch and forage, the cells' own errors (seeds 5 to 12)

Idea 164's two worlds and its seeds. `ev1f0`: every-event counters, the next step's wrong
answers, the every-event doc's marks. `scripts/predevent.sh main ...`; every number in
`out/predevent/main.md`. The `old` column is idea 164's rule run again here; it gives that
doc's numbers to the digit (0.742, 0.324).

| hidden cells, a fresh reader | born | rule off | `old` | `ev1f0` | its coin | its shuffled life | the lines |
|---|---|---|---|---|---|---|---|
| catch: which way the paddle moved (rate) | 0.521 (32.2) | 0.549 (31.3) | 0.742 (33.0) | **0.799** (38.4) | 0.618 (32.9) | 0.737 (32.2) | 0.450 |
| forage: last move (rate) | 0.275 (32.0) | 0.267 (31.4) | 0.324 (31.5) | 0.323 (32.4) | 0.272 (34.5) | 0.275 (33.0) | 0.221 |
| catch: the right button | 0.818 | 0.815 | 0.825 | 0.818 | 0.814 | 0.810 | 0.835 |
| catch: every line 1 step on, balanced | 0.751 | 0.753 | 0.747 | **0.774** | 0.745 | 0.756 | 0.770 |
| catch: every line 8 steps on, balanced | 0.534 | 0.535 | 0.535 | 0.522 | 0.528 | 0.526 | 0.537 |
| forage: every line 1 step on, balanced | 0.509 | 0.508 | 0.505 | 0.506 | 0.503 | 0.505 | 0.516 |

Paired (mean; seeds above / below): the paddle, `ev1f0` less born +0.278 (8/0), less its
coin **+0.181 (8/0)**, less its shuffled life +0.062 (6/2), less `old` +0.057 (5/3: +0.071,
-0.032, +0.126, +0.126, +0.062, +0.142, -0.027, -0.014). Forage's last move: less born
+0.047 (8/0), less its coin +0.051 (8/0), less its shuffled life +0.047 (7/1), less `old`
-0.001. Catch one step on: less born +0.022 (8/0), less its coin +0.028 (7/1), less its
shuffled life +0.018 (6/2); eight steps on -0.012 (0/8).

- **What just changed: learned, as before; a little stronger on catch, the same on
  forage.** The screen's two seeds had it at 0.907 against 0.806; on these eight it is
  0.799 against 0.742 with 5 seeds of 8 above. The every-event step is not the lever here
  that it was on MNIST.
- **The better coin shows more of it is real.** Another step's errors, with their counts and
  pairings kept, get 0.618 on the paddle; idea 164's coin (the same errors on random
  lines) got 0.672. The shuffled-frame life still gets 0.737: its errors are about what
  changed between two frames too.
- **One step ahead on catch the hidden cells now read as well as the lines do** (0.774;
  the lines 0.770; born 0.751; every seed 0.768 to 0.780 against born's 0.737 to 0.760,
  over the coin on 7 of 8). That is the change of the paddle and dot carried one step, not
  more of the future than the lines hold; eight steps on it is a little worse than born.
- **Nothing else moves.** The right button, forage's future, the predictor cells' skill
  (catch -0.010, forage +0.004 against the rule off) and the task (catch 0.201 against
  0.213, forage 0.681 against 0.698; 2 seeds of 8 above in each).

**The rate held at target** (`--g.threshold_cap 1000` in the life and the read, seeds 5 to
8; `scripts/predevent.sh cap ...`). `ev1f0` is read at 38.4 spikes per 1,000 cell-ticks on
catch against born's 32.2, so:

| hidden cells | born | `ev1f0` | its coin | the same seeds, rate free: born / `ev1f0` / coin |
|---|---|---|---|---|
| catch: which way the paddle moved (rate) | 0.528 (32.2) | 0.781 (31.0) | 0.702 (30.5) | 0.522 / 0.805 / 0.593 |
| forage: last move (rate) | 0.267 (32.1) | 0.322 (30.7) | 0.317 (30.5) | 0.278 / 0.315 / 0.276 |

The gain over born does not ride on the rate (+0.253 held, +0.283 free, 4 seeds of 4). But
with the rate held the coin gets most of it: the paddle +0.079 over the coin (4 of 4)
where it was +0.21 free, and forage's last move is level with its coin (+0.005). I do not
know why the coin learns the change when the thresholds are free to rise; it was not
chased. It makes "what just changed" less clean than idea 164's doc and the table above
have it: under one of the two regulation settings most of it is churn.

## The report: the worlds with time in them (seeds 5 to 12)

Delayed echo (a cue, 4 blank steps, an ask; the phase repeats every 6 steps) and
predict-next (order 2: the next symbol depends on the last two). `scripts/predevent.sh
time ...`; every number in `out/predevent/time8.md`. Hidden rates while read: 31.5 to 32.1
in every delayed echo arm; predict-next 31.4 to 37.2 (the coins 33.7 to 39.4).

### Delayed echo

"cells" and "outside": the sheet's own predictor cells' and the outside predictor's
balanced skill in the last quarter of the life. "future": a fresh ridge reader of the
hidden cells for every line 8 steps on, balanced. The rest: a fresh reader of the hidden
cells.

| | rule off | `ev1f0` (cells' own errors) | its coin | `ev1f0o2` | its coin | `ev15f0o2` | its coin | its shuffled life | `ev14f0o2` | its coin |
|---|---|---|---|---|---|---|---|---|---|---|
| cells, 1 step | 0.699 | 0.717 | 0.697 | 0.733 | 0.700 | **0.777** | 0.702 | | 0.773 | 0.699 |
| cells, 8 steps | 0.592 | 0.629 | 0.609 | 0.666 | 0.598 | **0.733** | 0.609 | | 0.731 | 0.597 |
| outside, 1 step | 0.719 | | | 0.763 | 0.715 | 0.790 | 0.730 | | **0.798** | 0.705 |
| outside, 8 steps | 0.584 | | | 0.657 | 0.586 | 0.740 | 0.603 | | **0.747** | 0.586 |
| outside: line-steps that still teach, 8 steps | 0.191 | | | 0.166 | 0.186 | 0.141 | 0.182 | | 0.139 | 0.188 |
| future, 8 steps (born 0.622) | 0.644 | 0.682 | 0.662 | 0.699 | 0.659 | **0.735** | 0.684 | 0.642 | 0.712 | 0.660 |
| time since the cue (born 0.859) | 0.897 | 0.954 | 0.937 | 0.967 | 0.924 | **0.983** | 0.955 | 0.883 | 0.983 | 0.942 |
| the cue (born 0.706) | 0.707 | 0.673 | 0.703 | 0.693 | 0.686 | 0.723 | 0.682 | 0.698 | 0.727 | 0.679 |
| the right button (born 0.914) | 0.915 | 0.919 | 0.918 | 0.917 | 0.913 | 0.931 | 0.919 | 0.917 | **0.936** | 0.917 |
| the task, late score | 0.514 | 0.531 | 0.527 | 0.523 | 0.532 | 0.554 | 0.527 | | **0.575** | 0.512 |

Paired over the 8 seeds (mean; seeds above / below):

| | `ev15f0o2` - rule off | - its coin | `ev14f0o2` - rule off | - its coin | `ev1f0o2` - rule off | - its coin | `ev1f0` - rule off | - its coin |
|---|---|---|---|---|---|---|---|---|
| cells, 1 step | +0.078 (8/0) | +0.074 (8/0) | +0.074 (8/0) | +0.074 (8/0) | +0.034 (7/1) | +0.033 (6/2) | +0.018 (8/0) | +0.020 (6/2) |
| cells, 8 steps | **+0.141 (8/0)** | **+0.124 (8/0)** | +0.140 (8/0) | +0.135 (8/0) | +0.075 (8/0) | +0.068 (7/1) | +0.038 (8/0) | +0.020 (6/2) |
| outside, 8 steps | +0.156 (8/0) | +0.138 (8/0) | +0.163 (8/0) | +0.161 (8/0) | +0.073 (8/0) | +0.071 (8/0) | | |
| future, 8 steps | +0.090 (8/0) | +0.051 (7/1) | +0.068 (8/0) | +0.052 (7/1) | +0.055 (8/0) | +0.040 (6/2) | +0.038 (7/1) | +0.021 (6/2) |
| time since the cue | +0.085 (8/0) | +0.028 (7/1) | +0.086 (8/0) | +0.041 (8/0) | +0.069 (8/0) | +0.043 (7/1) | +0.057 (8/0) | +0.017 (4/4) |
| the cue | +0.016 (4/4) | +0.041 (6/2) | +0.021 (4/4) | +0.048 (6/2) | -0.014 (3/5) | +0.006 (4/4) | -0.034 (1/6) | -0.030 (3/5) |
| the right button | +0.016 (7/1) | +0.012 (6/2) | +0.021 (6/2) | +0.019 (6/1) | +0.002 (5/3) | +0.004 (4/3) | +0.004 (6/2) | +0.001 (4/4) |
| the task | +0.040 (7/1) | +0.027 (6/2) | +0.061 (7/1) | +0.063 (7/1) | +0.009 (5/3) | -0.010 (3/5) | +0.017 (6/2) | +0.005 (3/5) |

Per seed (5 to 12). Cells at 8 steps: rule off 0.600, 0.579, 0.591, 0.580, 0.602, 0.588,
0.600, 0.593; `ev15f0o2` 0.717, 0.743, 0.707, 0.758, 0.746, 0.725, 0.713, 0.752; its coin
0.592, 0.580, 0.662, 0.614, 0.612, 0.603, 0.622, 0.584. The task: rule off 0.502, 0.537,
0.571, 0.559, 0.499, 0.526, 0.472, 0.447; `ev14f0o2` 0.799, 0.550, 0.555, 0.580, 0.541,
0.535, 0.564, 0.474; its coin 0.496, 0.535, 0.522, 0.540, 0.463, 0.490, 0.513, 0.534;
`ev15f0o2` 0.571, 0.529, 0.621, 0.562, 0.517, 0.621, 0.505, 0.508.

- **The future is taught here.** With the outside predictor's errors from all four
  horizons, the sheet's own predictor cells, which were never told anything new, go from
  0.592 to 0.733 at 8 steps and from 0.699 to 0.777 at 1 step; every seed of the 8 is above
  every seed of the rule-off life and of the coin. The outside predictor itself gains as
  much (0.584 to 0.740) and has less left to teach (19% of line-steps down to 14%). Under
  the coin both stay where they were (+0.017, +0.019). So the loop closes: errors about
  what will happen teach the cells something that makes what will happen easier to say.
- **What the cells learned is the time.** Time since the cue reads 0.983 (born 0.859, coin
  0.955, shuffled life 0.883), and in this world the phase is most of what can be known 8
  steps ahead. A fresh ridge reader of the future gains too (+0.090 over the rule-off
  life), though the coin gets about half of that (churn helps a fitted reader, finding 47;
  it does not help the online predictors).
- **The cue is kept, not gained.** Idea 164's rule cost the cue (0.706 to 0.618). With the
  cells' own errors it still costs some (0.673, 7 seeds of 7 under born). With the outside
  errors it is level with born (0.723, 0.727) and above the coin by 0.04 to 0.05 (6 seeds
  of 8).
- **The right button.** On five steps of six there is no right button and every reader says
  so; the sixth is the ask, and its answer is the cue of five steps before. 0.914 born is
  chance on the ask steps; 0.936 (`ev14f0o2`) is about 0.62 right on them (my arithmetic
  from the share, not a measured split). 6 to 7 seeds of 8 over born and over the coin.
- **The task moves a little.** Late score 0.514 rule off, 0.554 with all four horizons,
  0.575 with the far three; 7 seeds of 8 above the rule-off life in both, and above the
  coin on 6 and 7. Most of the far-three mean is one seed (0.799; that sheet reads the cue
  at 0.884 and the right button at 0.983); the other seven average 0.543. A hint, not a
  result: this readout is at chance on this world to begin with.
- **The cells' own errors do a quarter of it** (cells at 8 steps +0.038 over the rule-off
  life, +0.020 over their coin on 6 seeds of 8), and the next step alone, from outside,
  about half (+0.075, +0.068 over its coin).

### Predict-next

| | rule off | `ev1f0` | its coin | `ev1f0o2` | its coin | `ev15f0o2` | its coin | its shuffled life | `ev14f0o2` | its coin |
|---|---|---|---|---|---|---|---|---|---|---|
| cells, 1 step | 0.687 | 0.715 | 0.707 | 0.721 | 0.699 | **0.734** | 0.699 | | 0.733 | 0.705 |
| cells, 8 steps | 0.589 | 0.593 | 0.595 | 0.594 | 0.593 | 0.594 | 0.591 | | 0.594 | 0.591 |
| outside, 1 step | 0.846 | | | 0.889 | 0.848 | **0.897** | 0.860 | | 0.892 | 0.856 |
| outside, 8 steps | 0.692 | | | 0.698 | 0.693 | 0.704 | 0.695 | | 0.707 | 0.692 |
| the next symbol (born 0.742) | 0.743 | 0.825 | 0.780 | 0.815 | 0.789 | **0.830** | 0.806 | 0.786 | 0.829 | 0.810 |
| the symbol before (born 0.902) | 0.897 | 0.979 | 0.953 | 0.979 | 0.962 | 0.992 | 0.985 | 0.956 | 0.992 | 0.984 |
| future, 1 step (born 0.822) | 0.821 | 0.878 | 0.858 | 0.875 | 0.862 | 0.885 | 0.877 | 0.854 | 0.880 | 0.874 |
| the task, late score | 0.470 | 0.482 | 0.454 | 0.479 | 0.467 | 0.469 | 0.464 | | 0.471 | 0.466 |

- One step on, the same as delayed echo at a third of the size: the cells' skill +0.047
  (8 of 8 seeds; +0.034 over the coin, 8 of 8), the outside predictor +0.051 (+0.037 over
  the coin, 8 of 8). The next symbol read from the hidden cells 0.830 against 0.806 under
  the coin (+0.024, 6 seeds of 7 that differ) and 0.742 born.
- Eight steps on, nothing (a Markov chain of order 2 has little to say that far ahead:
  the lines' own reader gets 0.703).
- Here most of the inside read is still churn: the coin and the shuffled life get two
  thirds of the gain on the next symbol and nearly all of it on the symbol before.
- The task does not move.

## MNIST with something to predict in time: moved digits (part 4)

Built because part 2 showed a gain. `blobmnist ceiling --life-moved 3`
(`src/mnist/mnist_moved.cpp`): in the life every digit is shown for 3 steps in a row, one
pixel further each step along one of four directions (the digit's own, by its number), so
the next frame follows from this one and the way it is going. The sheet has no motor cells
to spare for 784 lines, so the errors come from the outside predictor alone: 121 pixels on
a lattice (every second pixel of the middle 22 x 22), one row of 16-bit weights each over
the non-sensor cells' counts, margin 2 steps, its signs sent to the every-event rule as
prediction lines one step back. No label reaches the sheet. A 20,000-digit life (60,000
steps, the ticks of the usual 60,000-digit life), then the usual read: frozen, a fresh
linear reader of the non-sensor cells fit on 60,000 unmoved digits with their labels.
`scripts/predevent.sh moved movedtest --test --seeds 5-12 --arms born,moved,movedc,moveds`.

Test digits, seeds 5 to 12:

| arm | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate | pixels | next-frame predictor |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| born | 0.9248 | 0.9272 | 0.9273 | 0.9239 | 0.9255 | 0.9192 | 0.9249 | 0.9278 | **0.9251** | 33.1 | 7.2 | |
| next-frame errors (`moved`) | 0.9452 | 0.9437 | 0.9427 | 0.9445 | 0.9420 | 0.9441 | 0.9423 | 0.9452 | **0.9437** | 44.3 | 6.4 | 0.869 |
| its coin: another step's errors | 0.9401 | 0.9348 | 0.9383 | 0.9357 | 0.9401 | 0.9416 | 0.9375 | 0.9387 | 0.9384 | 38.5 | 5.6 | 0.860 |
| pixels shuffled afresh every step | 0.9258 | 0.9227 | 0.9237 | 0.9230 | 0.9261 | 0.9233 | 0.9254 | 0.9269 | 0.9246 | 33.2 | 7.5 | 0.617 |

"rate": hidden spikes per 1,000 cell-ticks while read; "pixels": pixel inputs a hidden
excitatory cell; "next-frame predictor": the outside predictor's balanced skill in the last
tenth of the life. Seed by seed: next-frame errors less born +0.0186 (8 of 8; +0.0154 to
+0.0249); less the coin **+0.0054 (8 of 8; +0.0019 to +0.0089)**; less the shuffled life
+0.0191 (8 of 8); the coin less born +0.0133 (8 of 8). On the validation slice (seeds 1,
2): born 0.9259, a life on moved digits with the rule off 0.9268, next-frame errors 0.9466,
coin 0.9412, shuffled 0.9245, margin 1 0.9456.

- **Predicting the next frame, with no labels, lifts the labelled read from 0.925 to
  0.944** on every seed. Beside it from their own docs: the labelled every-event rule on an
  outside output's error 0.951, competitive learning 0.952, the engine's old rule from the
  answer 0.938.
- **Most of that is not the errors being about what was shown.** The coin gets 0.938, so
  0.013 of the 0.019 comes from the rule re-picking cells under errors of the right kind
  and number, and 0.005 from the errors being this step's (8 seeds of 8, small). In the
  labelled runs of idea 178 the coin got 0.007 of 0.026.
- **The loop does not close here.** The next-frame predictor is as good on the untaught
  sheet (0.869 with the rule off, validation) as on the taught one (0.869): a moving digit's
  next frame can already be read from the cells as born, so there was little for the cells
  to learn about time.
- **The rate held at target** (`--g.threshold_cap 1000`, seeds 5 to 8,
  `scripts/predevent.sh moved movedcap ...`): born 0.9221, next-frame errors 0.9402, coin
  0.9319, all read at 31. The gain over born is the same (+0.0180, 4 of 4) and the margin
  over the coin a little wider (+0.0083, 4 of 4: +0.0055, +0.0123, +0.0049, +0.0104). So it
  does not ride on the rate (44 against 33 when the rate is free).
- The pair test shown as two halves one after the other was not built (below).

TODO-PIECES

## The other six worlds (seeds 5 and 6: a screen)

The outside predictor's errors from the far three horizons (`ev14f0o2`) beside the rule-off
life and the coin, on idea 164's other worlds. `scripts/predevent.sh others ...`,
`out/predevent/others.md`. Two seeds: only where both seeds agree is anything said.

| world | what | rule off | `ev14f0o2` | its coin |
|---|---|---|---|---|
| mountain car | predictor cells, 8 steps | 0.608 | 0.661 | 0.622 |
| mountain car | outside predictor, 8 steps | 0.863 | 0.886 | 0.866 |
| mountain car | the right button (born 0.553) | 0.571 | 0.628 | 0.561 |
| pendulum | every line 8 steps on, balanced (born 0.614) | 0.614 | 0.641 | 0.604 |
| pendulum | outside predictor, 8 steps | 0.763 | 0.770 | 0.749 |
| cartpole | the last push (born 0.485; rates 31.5 / 45.2 / 55.5) | 0.473 | 0.535 | 0.468 |
| cartpole | outside predictor, 8 steps | 0.761 | 0.765 | 0.769 |
| beat | beat phase (born 0.465) | 0.479 | 0.537 | 0.478 |
| chain | outside predictor, 8 steps | 0.779 | 0.770 | 0.832 |
| echo | predictor cells, 8 steps | 0.749 | 0.751 | 0.754 |

- **Mountain car looks like delayed echo at a third of the size**: both predictors 8 steps
  on and the right button rise over the rule-off life and the coin on both seeds. It is the
  world here whose future needs the way things are going (the car's speed).
- Pendulum (the future 8 steps on) and cartpole (the last push) show the same direction on
  both seeds; cartpole's rates are far apart. Beat's phase rises on one seed of two.
- Chain and echo: nothing (echo's future is a coin).
- No task score moves (cartpole 17.0 steps a fall, 16.8 with the rule off, 17.1 under the
  coin; pendulum 0.221, 0.244 with the rule off, 0.221 under the coin).

## The task, and the plane store beside other rules (part 3)

**On idea 164's sheet and button rule** (the tables above; 8 seeds). The life's late score
with prediction errors teaching the hidden cells, less the rule-off life: catch -0.012
(cells' own errors; its coin -0.028), forage -0.017 (coin -0.019), predict-next +0.012 to
-0.001 over four arms (their coins -0.016 to -0.003), delayed echo +0.040 and +0.061 with
the outside predictor's far errors (7 seeds of 8 each; coins +0.013 and -0.002). So the task does not move, except on
delayed echo, a little, where the inside gain is.

**With the one setting's button rule** (`--base one`: the output rule with the margin gate,
time and hold cells, tiring, two winners, and the backward walk on bad steps, on the same
sheet so the predictor cells have room; `onenw`: the same without the walk). Seeds 5 to 7,
catch, forage and two rows of the quick map with time in them (order; delayed echo at 2
blank steps). `scripts/predevent.sh task1 ...`, `tasknw ...`. Late score of the life:

| world | rule off | `ev1f0` | `ev14f0o2` | without the walk: rule off | `ev14f0o2` |
|---|---|---|---|---|---|
| catch | 0.216 | 0.261 | 0.216 | 0.197 | 0.271 |
| forage | 0.898 | 0.818 | 0.801 | 0.938 | 0.902 |
| order | 0.849 | 0.720 | 0.709 | 0.855 | 0.707 |
| delayed echo, 2 steps | 0.989 | 0.927 | 0.917 | 0.971 | 0.899 |

A fresh reader of the hidden cells after the same lives (with the walk; rule off /
`ev1f0` / `ev14f0o2`): catch's right button 0.778 / 0.786 / 0.788; delayed echo's cue 0.916
/ 0.960 / 0.957, its right button 0.966 / 0.961 / 0.972; order's right button 1.000 in
all.

- **Where the readout already does the task, the rule costs it**: order -0.13 to -0.15 (0
  seeds of 3 above, with or without the walk), delayed echo -0.06 to -0.07 (one seed of the
  three falls from 0.97 to 0.79 and 0.75, two stay at 1.0), forage -0.08 to -0.10 with the walk. On
  catch, which this readout does not learn, +0.045 (3 of 3, cells' own errors) and +0.074
  (2 of 3, outside errors, no walk).
- **A fresh reader does not lose what the life loses.** Read afresh, the taught cells are
  level or a little better (the cue +0.04). The sheet's own readout does not follow cells
  that keep being re-picked, as idea 178 found on MNIST (its own readout stayed at 0.69
  while a fresh reader gained).
- Under this button rule the predictor cells are at chance on catch (balanced 0.50 against
  0.63 under idea 164's rule), so `ev1f0`'s errors there are little more than "the lines
  that came on".

**What "not beside competitive learning or the tags" means for prediction lives.**

- The predictor cells and the buttons are motor cells and learn by the readout's rule in
  its own counters. The one-hop rule teaches hidden excitatory cells only. No cell has two
  stores, so prediction lives on idea 164's sheet are safe on planes as they are.
- A rule that re-picks hidden cells from another store does fight it, and there are three:
  competitive learning and the tags (the tag store) and the backward walk
  (`replay_wire.cpp`, its own byte counters), which is part of the one setting. Each store
  is filled from a cell's wiring the first time it touches the cell and wires the cell
  from its own counters after that, so each undoes what the other did since. Idea 164's
  `compete` arm cannot be run on planes.
- Measured for the walk (3 seeds, the table above): the order and delayed echo losses are
  the same with and without the walk, so they are the rule's, not the fight's. On forage
  the walk beside the planes loses 0.06 more than the planes alone, on catch 0.07 more. A
  hint that the fight costs something where the walk has work to do.
- Time and hold cells are hidden excitatory cells, so the rule teaches them too. Their
  firing comes from their own counts, not their wiring, so that teaching is wasted work
  (about a fifth of the learners under the one setting).
- If the every-event rule is to run beside any of these, they need one store: the plane
  counters stepped by both (idea 178's doc proposed the same for competitive learning).

TODO-COST

TODO-REAL

## Not done

- **A predictor inside the sheet that is good enough.** Everything that works here takes
  its errors from a table outside the blob (a 16-bit weight per line, horizon and cell,
  stepped only inside a margin). The predictor cells are motor cells with bit weights and
  no margin; nothing here made them better at being an error source. That is item 9's job
  over again, for prediction lines: the same output the labelled rule needs.
- **The pair test as two halves one after the other** (part 4's first half). The moved-digit
  life was the cheaper of the two and its loop did not close; the halves need a new way of
  showing a pair and were left.
- The outside predictor's margin was not picked: 2 steps is idea 178's; on 3 seeds of
  delayed echo 4 steps reads higher and 1 lower (the pieces table).
- Horizons other than 1, 2, 4, 8, and horizons past 15 steps (the rings are kept for 16).
- Who learns (`hop_who`: inhibitory, pacemaker, motor cells) under prediction errors.
- Local wires (`--predict-hop-reach`) with far horizons: refused by the flag; idea 164
  found local wires worse.
- Prediction errors and button errors together on planes (`b` arms exist, none was run).
- The every-event rule beside competitive learning: the two stores fight (above).
- Lives longer than 20,000 steps; whether the delayed echo gain keeps growing or the task
  follows later.
- Why the coin learns the paddle's move when the rate is held.
- The calculation battery (no answer is shown as a line there, as idea 164's doc said).
- No `blobbench` numbers: with `hop_on` 0 the tick is untouched, and the four reference
  lives are byte-identical.

## Script lines

Outputs in `out/predevent/NAME/` (a log and a csv per run) and `out/predevent/NAME.md`; a
finished run is kept, so a line run again only runs what is missing.

```
scripts/predevent.sh smoke --quick        every part end to end, 1 seed, short lives
scripts/predevent.sh screen --seeds 1,2 --worlds catch,forage,delayed-echo,predict-next --arms born,pred,old,ev1f0,ev1f0n,ev1f8,ev1f12,ev1f16,ev1f24
scripts/predevent.sh screen --seeds 1,2 --worlds catch,forage,delayed-echo,predict-next --arms ev15f0,ev14f0,ev15f16,ev14f16,ev15f0n,ev8f0,ev1f0c,ev1f0s,ev15f0c,ev15f0s,ev14f0c,ev14f0s,oldc,olds
scripts/predevent.sh screen --seeds 1,2 --worlds catch,forage,delayed-echo,predict-next --arms predo2,ev1f0o2,ev15f0o2,ev14f0o2,ev1f0o2n,ev15f0o2n,ev8f0o2,ev1f0o2c,ev15f0o2c,ev1f0o2s,ev15f0o2s,ev14f0o2c
scripts/predevent.sh main --seeds 5-12 --worlds catch,forage --arms born,pred,old,ev1f0,ev1f0c,ev1f0s
scripts/predevent.sh cap --seeds 5-8 --worlds catch,forage --arms bornk,ev1f0k,ev1f0ck
scripts/predevent.sh time --seeds 5-12 --worlds delayed-echo,predict-next --arms born,predo2,ev1f0,ev1f0c,ev1f0o2,ev1f0o2c,ev15f0o2,ev15f0o2c,ev15f0o2s,ev14f0o2,ev14f0o2c
scripts/predevent.sh time --seeds 5-12 --worlds delayed-echo --arms ev15f0,ev15f0c,ev14f0,ev14f0c            the cells' own errors, all horizons
scripts/predevent.sh time --as pieces --seeds 5-7 --worlds delayed-echo --arms born,predo2,old,oldo2,oldo2c,ev15f0,ev15f0c,ev14f0,ev14f0c,ev15f0o1,ev15f0o4,ev8f0o2,ev2f0o2,ev15f0o2n,ev15f0o2,ev15f0o2c
scripts/predevent.sh cap --as capde --seeds 5-8 --worlds delayed-echo --arms bornk,predo2k,ev15f0o2k,ev15f0o2ck
scripts/predevent.sh main --seeds 5-8 --worlds catch,forage --arms predo2,ev14f0o2,ev14f0o2c
scripts/predevent.sh others --seeds 5,6 --worlds cartpole,pendulum,mountain-car,chain,beat,echo --arms born,predo2,ev14f0o2,ev14f0o2c
scripts/predevent.sh task1 --base one --seeds 5-7 --worlds catch,forage,order,delayed2 --arms predo2,ev1f0,ev14f0o2
scripts/predevent.sh tasknw --base onenw --seeds 5-7 --worlds catch,forage,order,delayed2 --arms predo2,ev14f0o2
scripts/predevent.sh own --seeds 5-7 --worlds catch,delayed-echo --arms predo2,ev1f0,ev14f0o2,ev14f0o2c --read-own 1
scripts/predevent.sh speed --seeds 5 --worlds catch,delayed-echo --arms pred,predo2,old,ev1f0,ev15f0,ev15f0o2,ev14f0o2 --jobs 1 --trials 300
scripts/predevent.sh moved movedval --seeds 1,2 --arms born,off,moved,movedc,moveds,movedm1
scripts/predevent.sh moved movedtest --test --seeds 5-12 --arms born,moved,movedc,moveds
scripts/predevent.sh moved movedcap --test --seeds 5-8 --arms bornk,movedk,movedck
scripts/predevent.sh NAME --table-only --as OTHER --seeds ... --arms ...      a table from finished runs, saved as OTHER.md
out/predevent/refcheck.sh                 the four reference lives against main's
out/predevent/oldcheck.sh                 idea 164's arms against the base commit's build (needs out/predevent/base-src built)
out/predevent/perseed.py NAME table world key arm,arm    one number seed by seed from NAME.csv
```

The `main` and `cap` lines for catch and forage, the first far-horizon screen and `others`
ran on stevejobs in `thonk-predevent` (`python3 scripts/predevent.py ...`, three at a
time); the rest here. Here a run on idea 164's sheet takes about a minute, a far-horizon
one two, one under the one setting's button rule three, a moved-digit MNIST run four.

## Code

- `src/blob/hop_pred.cpp` (new): the prediction lines: fine zone, kept steps and far
  horizons, coins, `blob_hop_predicted`, `blob_hop_keep_step`, `blob_hop_pred_counts`.
- `src/blob/learn_hop.cpp`: the eleven lines that gathered the prediction lines replaced by
  a call to `hop_pred_errors`; `hop_pred_free` in `hop_free`; a comment line. Nothing else.
- `src/blob/hop_state.hpp`: the kept steps, the coin ring and two counts at the end of
  `HopState`; two constants; two declarations.
- `src/blob/blob.hpp`: three declarations after `blob_hop_error_places`.
  `src/blob/genome.hpp`, `genome_args.cpp`: `hop_pred_fine` at the end, 0 by default.
- `src/run/predict.cpp`, `predict.hpp`: the group counts kept per ring step and sent to the
  rule (`hop_send`), `--predict-hop` as a set of horizons, the outside predictor's hooks;
  `predict_step_end` takes the blob without `const`. `src/run/predict_outside.cpp` (new).
- `src/world/predict_settings.hpp`, `predict_flags.cpp`: `--predict-outside`, the outside
  tally, the help text.
- `src/probe/predict_read.cpp`, `predict_future.cpp`, `predict_future.hpp`,
  `predict_labels.cpp`, `predict_labels.hpp`: the balanced and changed scores, the "right"
  question, `--read-own`, the new csv keys.
- `src/mnist/mnist_moved.cpp` (new); hooks in `mnist_sheet.cpp` (`moved_digit`),
  `mnist_ceiling.cpp` (two flags, make and free), `mnist_wiring.cpp` (one report line),
  `mnist_run.hpp`.
- `CMakeLists.txt`: `blobprobe` lists `src/run/predict_outside.cpp`.
- `scripts/predevent.sh`, `predevent.py`, `predmoved.py` (reuse `predhop.py`, `inside.py`,
  `hop2.py`).
- Checks: a build with no warnings; the four reference lives byte-identical
  (`out/predevent/refcheck.sh`, run again on the last commit); idea 164's five arms and the
  plane path the same csv lines as the base commit's build (`out/predevent/oldcheck.sh`);
  `scripts/predevent.sh smoke --quick` end to end. Every file is under 500 lines
  (`predict_read.cpp` 489, `learn_hop.cpp` 440).
