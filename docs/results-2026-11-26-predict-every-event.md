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
