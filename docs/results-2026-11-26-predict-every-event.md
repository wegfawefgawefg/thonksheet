# Prediction errors through the every-event rule (idea 183, 2026-11-26)

Queue item 3c. Idea 164 (`docs/results-2026-11-25-predhop.md`) sent the next-step
predictor's wrong answers to every hidden cell down the one-hop wires; the cells learned
"what just changed" and nothing more of the future, and the task did not move. Since then
the rule's ladder and idea 178 (`docs/results-2026-11-26-every-event.md`) found that rule
was crippled: its counters stepped with a small chance, and the error had no fine zone.
This job runs prediction errors through the repaired rule: 12-bit counters that step on
every marked input, a signed error per prediction line with a fine zone, and the errors of
the far horizons as well as the next step's.

**The answer: yes for the inside, but only in a world with time in it and with a better
predictor than the sheet's own, no for catch and forage, and barely for the task: with
every-event counters and the errors of a predictor that is right most of the time and
silent inside a margin (a whole-number perceptron outside the sheet), sent from several
horizons, the hidden cells on delayed echo come to hold the time since the cue well enough
that the
sheet's own predictor cells go from 0.59 to 0.73 eight steps ahead (their coin 0.61, every
one of 8 seeds) and one sheet in eight learns the task; with the predictor cells' own
errors the far horizons give over half of that (0.68) and the next step alone is still
"what just changed"; a fine zone on those cells' answers makes things worse; and under the
one setting's button rule the life's score falls on three worlds of four.**

## Short version

Hidden cells read by a fresh linear reader after a 20,000-step life, or the predictors'
own balanced skill late in the life. "coin": the same life, the cells given another step's
errors. 8 seeds (5 to 12) unless said.

| | rule off | prediction errors | its coin | |
|---|---|---|---|---|
| catch: which way the paddle moved, the cells' own next-step errors | 0.549 (born 0.521) | 0.799 | 0.618 | idea 164's rule 0.742 |
| ... the same with the rate held (4 seeds) | born 0.528 | 0.781 | 0.702 | |
| delayed echo: the sheet's own predictor cells, 8 steps on; outside errors, four horizons | 0.592 | **0.733** | 0.609 | every seed above |
| ... the cells' own errors, four horizons | 0.592 | 0.679 | 0.608 | 7 seeds of 8 |
| ... the cells' own errors, the next step alone | 0.592 | 0.629 | 0.609 | |
| delayed echo: the outside predictor, 8 steps on | 0.584 | **0.740** | 0.603 | every seed above |
| delayed echo: time since the cue, hidden cells | 0.897 (born 0.859) | 0.983 | 0.955 | shuffled life 0.883 |
| delayed echo: the task's late score, the far three horizons | 0.514 | 0.575 | 0.512 | one seed at 0.80 |
| predict-next: the sheet's own predictor cells, 1 step on | 0.687 | 0.734 | 0.699 | 8 of 8 |
| MNIST, digits moved a pixel a step, no labels: the usual labelled read | born 0.925 | **0.944** | 0.938 | 8 of 8 over both |

- **The fine zone.** For a button line it compares a wrong line's spikes with the right
  line's. A prediction line is one bit answered by two groups of motor cells, so its like
  is the split between the two: the wrong group's share of the pair's spikes
  (`--g.hop_pred_fine`, in 32nds). On the predictor cells' answers every zone but "wrong
  answers only" is worse (screen, 2 seeds: the paddle read 0.91 wrong-only, 0.49 to 0.75
  with a zone), because those cells are seldom right by a margin.
- **The predictor cells are a poor output.** One step on they are 0.63 balanced on catch
  where a perceptron on the same sheet is 0.84, and under "every line keeps its common
  value" on the plain share. Idea 178's lesson again: a free target is not a good error.
- **What just changed: learned, a little stronger on catch (0.799 against 0.742, 5 seeds
  of 8), the same on forage (0.323 against 0.324).** One step ahead the catch cells now
  read the lines' future as well as the lines do (0.774; lines 0.770; born 0.751). With
  the rate held the coin gets most of the paddle read (0.702) and all of forage's.
- **The future is taught on delayed echo, by a good predictor's errors.** Both predictors
  gain 0.14 to 0.16 at 8 steps and 0.07 at 1 step, 8 seeds of 8 over the rule-off life and
  the coin; the cells hold the time since the cue (0.983) and keep the cue, which the old
  rule lost. It holds under the other regulation setting and on the sheet's own moves.
  Predict-next gains a third as much at 1 step; mountain car leans the same way on 2
  seeds; catch, forage, chain and echo gain nothing.
- **Which horizons.** Several together teach about twice what one does; the far three
  alone do as well as all four; the next step alone gives half; 2 alone or 8 alone a
  third (3 seeds).
- **Which pieces.** As on MNIST, the good error and the every-event counters only work
  together: the outside error on the old chance counters is level with its coin (3 seeds).
- **The task.** On idea 164's sheet it moves only on delayed echo (+0.04, +0.06; 7 seeds of
  8; one sheet does the task, frozen, at 0.80). With the one setting's button rule the
  life's score falls where the readout was doing well (order -0.14, delayed echo at 2
  steps -0.07, forage -0.09; 3 seeds), while a fresh reader of the same cells is level or
  better: the sheet's own readout does not follow cells that keep being re-picked.
- **The plane store** is safe in prediction lives on idea 164's sheet (the predictor cells
  and buttons are motor cells; the rule teaches hidden cells), and fights any rule that
  re-picks hidden cells from another store: competitive learning, the tags, and the one
  setting's backward walk.
- **MNIST with moved digits.** Next-frame errors with no labels lift the labelled read
  0.925 to 0.944; 0.013 of that the coin gets too, 0.005 is the errors being this step's
  (8 seeds of 8). The loop does not close there: the next-frame predictor is as good on the
  untaught sheet.
- **Cost.** The next step alone: 0.76 to 0.81 of the rule-off life's speed. Four horizons:
  0.46 to 0.56 (four verdicts a step). 8 words a neuron for the kept steps.
- **What it means for the queue.** Same as idea 178's: the missing part is the output. A
  predictor inside the sheet that is right most of the time and steps only inside a
  margin would make this a result of the method; the far horizons already give half of it
  from the cells as they are.

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

**The other regulation setting** (`--g.threshold_cap 1000`, seeds 5 to 8,
`scripts/predevent.sh cap --as capde ...`). The rates are level without it, so this is not
a rate check; it asks whether the result leans on the default regulation, as the paddle
read's margin over its coin did. Rule off / `ev15f0o2` / its coin: the outside predictor
8 steps on 0.538 / 0.664 / 0.541 (4 seeds of 4 over both), time since the cue 0.829 /
0.961 / 0.882 (4 of 4), the cue 0.683 / 0.724 / 0.691 (4 of 4), the future 8 steps on
0.614 / 0.716 / 0.616 (4 of 4 over the coin). The inside gain holds, with wider margins
over the coin. The sheet's own predictor cells do not show it there: under that setting
they are poor to begin with (0.542 at 8 steps with the rule off, against 0.592) and gain
+0.028 (+0.015 over the coin, 3 of 4). The task: 0.502 / 0.514 / 0.494.

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

## Which horizons teach, and which pieces matter (delayed echo)

The sheet's own predictor cells' balanced skill 8 steps on, in the last quarter of the
life (the rule off: 0.592 on seeds 5 to 12, 0.590 on seeds 5 to 7). `scripts/predevent.sh
time ...` (8 seeds, `out/predevent/cells8.md`, `time8.md`) and `... --as pieces` (3 seeds,
`out/predevent/pieces.md`).

| horizons that teach | the cells' own errors | less rule off | less its coin | the outside predictor's errors | less rule off | less its coin |
|---|---|---|---|---|---|---|
| 1 | 0.629 | +0.038 (8/0) | +0.020 (6/2) | 0.666 | +0.075 (8/0) | +0.068 (7/1) |
| 1, 2, 4, 8 | **0.679** | +0.088 (7/1) | +0.071 (7/1) | **0.733** | +0.141 (8/0) | +0.124 (8/0) |
| 2, 4, 8 | 0.672 | +0.080 (7/1) | +0.065 (8/0) | 0.731 | +0.140 (8/0) | +0.135 (8/0) |
| 2 alone (3 seeds) | | | | 0.640 | +0.050 (3/0) | |
| 8 alone (3 seeds) | | | | 0.634 | +0.045 (3/0) | |

- **Several horizons together teach about twice what any one does**, from either error
  source; the next step is not needed (the far three alone do as well as all four). One
  horizon alone, near or far, gives a third to a half.
- **The sheet's own errors do more than half of it when the far horizons are in.** No
  outside predictor: the cells' skill 8 steps on goes 0.592 to 0.679 (7 seeds of 8; its
  coin 0.608) and 1 step on 0.699 to 0.740 (coin 0.703). That part of the result is inside
  the method. But a fresh reader does not see it beyond the coin: time since the cue 0.947
  against the coin's 0.970 (1 seed of 8 above), the future 8 steps on +0.010 over the coin
  (6 of 8), and the cue is read 0.034 worse than born. With the outside predictor's errors
  all of those go the right way (above).

The pieces, on seeds 5 to 7:

| counters | error | horizons | cells, 8 steps | its coin | outside, 8 steps | its coin |
|---|---|---|---|---|---|---|
| the rule off | | | 0.590 | | 0.559 | |
| 6 bits, chance 1/32 (`old`) | the cells' own | 1 | 0.630 | | | |
| 6 bits, chance 1/32 (`oldo2`) | outside | 1 | 0.626 | 0.637 | 0.607 | 0.623 |
| every event (`ev15f0`) | the cells' own | 1, 2, 4, 8 | 0.664 | 0.625 | | |
| every event (`ev15f0o2`) | outside, margin 2 | 1, 2, 4, 8 | **0.723** | 0.611 | **0.731** | 0.587 |
| every event (`ev15f0o1`) | outside, margin 1 | 1, 2, 4, 8 | 0.691 | | 0.721 | |
| every event (`ev15f0o4`) | outside, margin 4 | 1, 2, 4, 8 | 0.753 | | 0.746 | |
| every event, near-not-fired marks (`ev15f0o2n`) | outside, margin 2 | 1, 2, 4, 8 | 0.668 | | 0.646 | |

- **Idea 178's finding again: the good error and the every-event counters only work
  together.** The outside predictor's error on the old chance counters is level with its
  own coin (0.626 against 0.637, 0 seeds of 3 above). On every-event counters it is 0.11
  over its coin on every seed.
- A wider margin read higher on these three seeds (4 steps: 0.753) and a narrower one
  lower; the margin was not picked (2 is idea 178's). Idea 164's near-not-fired marks are
  worse here.

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

## The read on the sheet's own moves (seeds 5 to 7)

Idea 164's doc left this open: its read took moves from a coin, so the next lines hung on a
press nobody could know. `--read-own 1` lets the frozen sheet's own buttons move it (each
arm then sees its own world, so only differences against that arm's own lines mean
anything), and gives the frozen sheet's score. `scripts/predevent.sh own ...`.

| | rule off | `ev1f0` | `ev14f0o2` | its coin |
|---|---|---|---|---|
| catch: every line 1 step on, hidden cells less the lines' own reader | -0.012 | -0.002 | -0.004 | -0.003 |
| catch: the frozen sheet's score | 0.215 | 0.232 | 0.244 | 0.235 |
| delayed echo: the cue | 0.696 | 0.680 | 0.776 | 0.645 |
| delayed echo: time since the cue | 0.868 | 0.926 | 0.979 | 0.938 |
| delayed echo: every line 8 steps on, balanced | 0.647 | 0.679 | 0.714 | 0.648 |
| delayed echo: the frozen sheet's score | 0.469 | 0.480 | 0.602 | 0.472 |

- **On its own moves the catch sheet still holds what its lines hold and no more.** The
  hidden cells' reader sits just under the lines' reader in every arm. The coin read was
  not hiding a future.
- **Delayed echo reads the same way as under the coin's moves**: the cue +0.13 over the
  coin (3 seeds of 3), time since the cue +0.04 (3 of 3).
- **One sheet of the three does the task frozen.** The frozen scores of `ev14f0o2` are
  0.805, 0.502, 0.498 (the rule-off sheets 0.451, 0.495, 0.462; the coin's 0.485, 0.485,
  0.444). Seed 5 is the sheet whose life ended at 0.799: it learned delayed echo and keeps
  it with learning off. The other two are at chance. So the task gain above is one sheet
  in eight that learned the task, and seven that moved by a few hundredths.

## The task, and the plane store beside other rules (part 3)

**On idea 164's sheet and button rule** (the tables above; 8 seeds). The life's late score
with prediction errors teaching the hidden cells, less the rule-off life: catch -0.012
(cells' own errors; its coin -0.028), forage -0.017 (coin -0.019), predict-next +0.012 to
-0.001 over four arms (their coins -0.016 to -0.003), delayed echo +0.040 and +0.061 with
the outside predictor's far errors (7 seeds of 8 each; coins +0.013 and -0.002). So the
task does not move, except on delayed echo, a little, where the inside gain is.

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

## Cost (part 5)

**Speed.** One life at a time on this PC (`scripts/predevent.sh speed ... --jobs 1`; the
governor makes any number here rough), seed 5, ticks a second over a 20,000-step life with
the predictor cells at four horizons:

| life | catch | of the rule off | delayed echo | of the rule off |
|---|---|---|---|---|
| the rule off (`pred`) | 24,500 | | 48,000 | |
| the rule off, the outside predictor learning beside (`predo2`) | 22,100 | 0.90 | 42,200 | 0.88 |
| idea 164's rule (`old`) | 21,000 | 0.86 | 40,400 | 0.84 |
| every-event, the next step (`ev1f0`) | 18,800 | 0.76 | 38,700 | 0.81 |
| every-event, four horizons (`ev15f0`) | 11,300 | 0.46 | 27,100 | 0.56 |
| the same on the outside predictor's errors (`ev15f0o2`) | 11,000 | 0.45 | 26,600 | 0.55 |
| the far three, outside errors (`ev14f0o2`) | 12,400 | 0.50 | 28,100 | 0.59 |

A moved-digit MNIST life: 55,400 with the rule off, 28,900 with it (0.52). Under the one
setting's button rule the rule-off lives ran at 3,900 (catch) to 15,700 (delayed echo),
three at a time, and `ev14f0o2` at 0.6 to 0.8 of that; why that button rule is so slow on
this sheet was not looked at.

- With the next step alone the every-event rule costs a quarter to a fifth, a little more
  than the old rule. Each further horizon is another verdict a step: on catch, where nearly
  every step has a wrong line at every horizon, four horizons are four verdicts a step
  (79,000 in 20,000 steps), about 420 cells taught each, 2.2 billion counter steps and 4.8
  million re-picks in the life. That is what halves the speed.
- The outside predictor's arithmetic (152 rows of weights times about 3,500 counts a step)
  costs a tenth of the rule-off life. It is not part of the blob.

**Memory**, beyond idea 178's planes (132 words a neuron at 12 bits):

- The kept steps for the far horizons: 16 steps of 16 spike maps and 16 near maps, 512
  words a tile, 8 words a neuron (266 KB on this 4,096-neuron sheet). Allocated only when a
  horizon past one step teaches. A horizon of 8 needs 8 of the 16 steps, and a verdict
  reads at most the last 15 ticks of a step, so this could be cut to 4 words a neuron; it
  was not. Copying the rings is 2,080 words a step.
- The predictor's kept answers: 16 steps of two 16-bit counts per line and horizon (10 KB
  for 38 lines).
- The coin's ring is a control (131 KB) and the outside predictor a measuring stick (16
  bits per line, horizon and cell: 1.1 MB on catch; 121 x 3,072 for the moved digits).
- Nothing on the tick's path changes: the near maps are the old rule's. The far verdicts
  run at a step's start, outside the tick, on the rule's own code.

## What is real, what is suspect

- Real: on delayed echo, with every-event counters and the outside predictor's errors
  from several horizons, the hidden cells come to hold the time since the cue (and keep
  the cue), and both predictors get better at 1 and 8 steps by amounts their coin does not
  get. 8 seeds, every seed on one side for the predictors; holds under the other
  regulation setting for the outside predictor and the fresh reads; holds on the sheet's
  own moves (3 seeds).
- Real, smaller: the same with the sheet's own predictor cells' errors when the far
  horizons teach (cells at 8 steps +0.088, +0.071 over the coin, 7 seeds of 8), seen in
  the predictor cells' skill only, not by a fresh reader beyond the coin.
- Real, small: predict-next one step on (+0.03 to +0.04 over the coin on both predictors,
  8 of 8); MNIST with moved digits (+0.005 over the coin, 8 of 8; +0.008 with the rate
  held, 4 of 4).
- Real: the old paths. Four reference lives byte-identical; idea 164's arms the same csv
  lines as the base commit's build; the `old` arm here gives idea 164's numbers to the
  digit on its seeds.
- **The working error does not come from the sheet.** It comes from a table outside the
  blob: 16 bits per line, horizon and non-sensor cell, a vote over spike counts, a step
  only inside a margin. The sheet's own predictor cells give about half of it at best.
- **One world.** The clear result is delayed echo: a world whose future is a clock. Mountain
  car (2 seeds) and predict-next (1 step) lean the same way at a third of the size; catch,
  forage, chain and echo show nothing. The worlds here mostly have no hidden state that
  matters; catch's dot falls straight down, so there is no ball direction to learn.
- **The task claim is one sheet.** Late score +0.04 and +0.06 on 7 seeds of 8, but one seed
  at 0.80 carries the far-three mean, and the frozen sheets of the other seeds checked are
  at chance.
- **"What just changed" is less clean than idea 164 had it.** With the rate held the coin
  gets most of the paddle read and all of forage's.
- The screens are two seeds an arm (the fine zones, near-not-fired marks, the far horizons
  on catch and forage, the six other worlds); the pieces three; the one-setting task runs
  three. None of those is quoted for a small difference.
- The read's moves come from a coin, except in the own-moves check (3 seeds, 2 worlds).
- In a world with a period the coin is a slightly stronger control than it looks: one
  step in six of the last 256 has this step's phase, so a sixth of the coin's errors are
  the right kind.
- The "right button" on ask steps (0.62) is arithmetic on a share, not a measured split.
- The shuffled-frame life's predictors learn on shuffled frames; its errors are more
  numerous than the real life's.
- Lives are 20,000 steps; nothing says whether the gains keep growing.
- The MNIST coin gets 0.013 by itself, twice what idea 178's coins got; why was not looked
  at.
- Speeds are from one PC under a temperature governor.

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
out/predevent/oldcheck.sh                 idea 164's arms against the base commit's build (out/predevent/base-src)
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
  One meaning changed: `hop_coin 3` on prediction lines used to do what `hop_coin 1` does
  (each error on a random line); it is now "another step's errors", as it is for buttons.
  No arm of idea 164 or 178 used that pair.
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
  `scripts/predevent.sh smoke --quick` end to end (its moved-digit part needs
  `data/mnist` or `THONK_MNIST` and is skipped without). Every file is under 500 lines
  (`predict_read.cpp` 489, `learn_hop.cpp` 440).
- Shared files touched, for the merge: `src/blob/learn_hop.cpp`, `hop_state.hpp`,
  `blob.hpp`, `genome.hpp`, `genome_args.cpp`; `src/run/predict.cpp`, `predict.hpp`;
  `src/world/predict_settings.hpp`, `predict_flags.cpp`; `src/probe/predict_read.cpp`,
  `predict_future.cpp`, `predict_future.hpp`, `predict_labels.cpp`, `predict_labels.hpp`;
  `src/mnist/mnist_sheet.cpp`, `mnist_ceiling.cpp`, `mnist_wiring.cpp`, `mnist_run.hpp`;
  `CMakeLists.txt`. `hop_lines.cpp`, `hop_planes.cpp` and `tick.cpp` are untouched.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, the idea 164 finding ("Prediction errors sent to every hidden cell
  teach it what just changed, and not more of the future"): it needs a second half.
  **With a good predictor's errors and every-event counters they do teach the future,
  where the world has time in it.** Delayed echo, 8 seeds: the sheet's own predictor cells
  8 steps on 0.592 to 0.733 (coin 0.609, every seed), the outside predictor 0.584 to 0.740
  (coin 0.603), time since the cue 0.859 born to 0.983 (coin 0.955), the cue kept (0.723;
  idea 164's rule 0.618). Several horizons together teach twice what one does. The errors
  came from a whole-number perceptron outside the sheet; the predictor cells' own errors
  give 0.679 when the far horizons teach and 0.629 from the next step alone. Predict-next
  a third as much at 1 step; catch, forage, chain, echo nothing. [solid for delayed echo
  with the outside predictor: 8 seeds, coin, shuffled life, a second regulation setting,
  the sheet's own moves; one world]
- The same finding, its first half: every-event counters make "what just changed" a
  little stronger on catch (0.799 against 0.742, 5 seeds of 8) and not on forage; against
  the better coin (another step's errors) the paddle read is +0.18 (8 of 8); **with the
  rate held the coin gets most of it** (0.781 against 0.702; forage level). [the caveat is
  4 seeds]
- A candidate: **a fine zone needs an output that is usually right.** On the predictor
  cells' answers (0.63 balanced on catch) every zone but "wrong answers only" reads worse
  (2 seeds, screen). The every-event doc's fine-zone finding should say what it leans on.
  [hint]
- The every-event finding ("every-event counters and a good error only work together")
  gains a second case: prediction lines on delayed echo, the outside error on chance
  counters level with its coin (3 seeds), on every-event counters +0.11 over it.
- Finding 47 (churn): the fresh ridge reader gets about half of the delayed echo gain
  under the coin; the online predictors get none of it under the coin. The predictors'
  own skill is the cleaner measure of "the future is taught".
- A candidate: **prediction errors on the inside cost a readout that already works.**
  Under the one setting's button rule the life's score falls on order (-0.14), delayed
  echo at 2 steps (-0.07) and forage (-0.09) while a fresh reader of the same cells is
  level or better (3 seeds). Idea 178 saw the same on MNIST's own readout. [hint: 3 seeds]
- A candidate: **next-frame prediction with no labels lifts MNIST's labelled read 0.925
  to 0.944** (digits moved a pixel a step, an outside next-frame predictor's errors, 8
  seeds; its coin 0.938; rate held 0.940 against 0.932). [likely: 8 seeds; most of the gain
  is not the errors' tie to the frame shown]
- `docs/plan.md`: item 3c done (this doc). What it leaves, in order: (1) a predictor
  inside the sheet with the two properties that mattered (right most of the time; a step
  and an error only inside a margin), which is item 9's output with prediction lines as
  its targets; the far horizons already give half the gain from the predictor cells as
  they are; (2) one counter store for every rule that re-picks hidden cells (planes
  stepped by the walk, the tags and competitive learning as well), without which the
  every-event rule cannot run under the one setting; (3) worlds with hidden state that the
  task needs (a ball with a direction, a cue that matters later): catch and forage cannot
  show a future being learned because theirs is in the lines; (4) the readout keeping up
  with cells that are being re-picked (the task falls under the one setting).
- `docs/ideas.md`: idea 183 done, with the answer above. New ideas: a margin predictor
  made of the sheet's own cells (per line and horizon, a vote and a margin, as the
  whole-number perceptron here); prediction errors gated off while the readout is doing
  well (the rule costs a working readout); the pair test shown as two halves; a catch with
  a ball that moves sideways.
- `docs/predictions.md`: the predictions above and their scores (below).
- `docs/design.md`, `docs/notes-world.md` (not edited here): `hop_pred.cpp`,
  `hop_pred_fine`, `blob_hop_predicted`, `blob_hop_keep_step`, `blob_hop_pred_counts`,
  `hop_coin 3` on prediction lines, `--predict-hop` as a set of horizons,
  `--predict-outside`, `blobprobe predict --read-own` and its new keys, `blobmnist ceiling
  --life-moved`, `scripts/predevent.sh`.

## Predictions, scored

| prediction | p | outcome |
|---|---|---|
| 1. `ev1f0` reads the paddle 0.03 over the old rule and 0.05 over its coin | 0.55 | happened: +0.057 (5 seeds of 8) and +0.181 (8 of 8) |
| ... forage's last move 0.02 over the old rule | 0.4 | no: -0.001 |
| 2. the hidden reader of the future +0.02 over born and +0.01 over both controls on 2 worlds | 0.15 | one world in one arm: delayed echo at 8 steps with the outside errors (+0.112, +0.051, +0.093); catch at 1 step with the cells' own (+0.022, +0.028, +0.018); never two worlds in one arm |
| 3. far horizons add 0.02 beyond the next step alone on 2 worlds | 0.2 | one world: delayed echo (the cells' skill at 8 steps 0.733 against 0.666; from their own errors 0.679 against 0.629); predict-next +0.013 |
| 4. a fine zone above 0 reads the paddle better | 0.3 | no (0.49 to 0.75 against 0.91, screen) |
| ... the rate climbs and the reads fall | 0.5 | happened (58 spikes per 1,000 at "wrong or tied") |
| 5. the right button +0.02 over born and +0.01 over the coin on catch or forage | 0.2 | no (catch +0.000; forage is at 1.000 born); on delayed echo +0.022 and +0.019 |
| 6. the task within 0.03 on catch and forage | 0.6 | happened (-0.012, -0.017) |
| ... 0.03 better on some task world, most seeds | 0.12 | happened: delayed echo +0.040 and +0.061 (7 of 8), one sheet carrying most of it |
| ... 0.03 worse on some | 0.35 | happened, under the one setting's button rule: order -0.14, forage -0.09, delayed echo at 2 steps -0.07 |
| 7. the predictor cells' skill +0.01 on catch or forage, not under the coin | 0.2 | no (catch -0.010, forage +0.004); yes on delayed echo and predict-next |
| 8. the best arm read at over 36 spikes per 1,000 | 0.6 | the paddle arm yes (38.4); the delayed echo arms no (31.9) |
| ... the paddle read keeps half its gain with the rate held | 0.6 | happened (+0.253 of +0.283), but the coin gets most of it there |
| 9. four horizons cost 0.5 to 0.8 of the speed | 0.6 | delayed echo yes (0.56); catch no (0.46) |
| 10. delayed echo's cue read worse than born | 0.7 | with the cells' own errors yes (-0.033, 0 seeds of 7 above); with the outside errors no (+0.017) |
| ... time since the cue +0.05 over born | 0.6 | happened (+0.095 to +0.124) |
| ... its coin within 0.02 | 0.6 | with the cells' own errors yes (+0.017); with the outside errors no (+0.028 to +0.043 over the coin) |
| 11. within 0.03 with and without the walk | 0.6 | order and delayed echo yes; forage and catch no (0.06, 0.07 worse beside the walk) |
| 12. the one sentence is "still only what just changed" | 0.65 | wrong for delayed echo; right for catch and forage and for the cells' own next-step errors |

I had the predictor as given and asked only what the counters and the zone would do with
its errors. What decided the job was the predictor: its own errors are poor, the fine zone
cannot fix that, and a better one outside the sheet made the future learnable where there
was a future to learn. I did not foresee that the far horizons would matter (0.2), that the
rule would cost the task under the one setting's button rule, or the MNIST number.
