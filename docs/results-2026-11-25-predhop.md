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
