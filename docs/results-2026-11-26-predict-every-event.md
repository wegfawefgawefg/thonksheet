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
`scripts/predhop.py`'s sheet, worlds and helpers).
