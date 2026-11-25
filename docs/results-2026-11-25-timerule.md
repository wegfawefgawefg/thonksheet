# Why competitive learning lifts the time rows, and how far (idea 166, 2026-11-25)

Queue item 2f of `docs/plan.md`. Finding 50 is the contradiction: on the suite's sheet the rule
(idea 158's B5, `--g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5`, on for the whole
life beside the readout) lifts delayed echo at 2 steps 0.59 to 0.91 and gap 0.54 to 0.94; on
the prediction battery's sheet at 4 steps it left delayed echo at 0.485 to 0.486.

## Predictions (written after point 1 only)

Seen before writing this: the reproduction (point 1, seeds 5 and 6, every number matches
the two docs), and from the prediction battery's raw runs that its rule-off arm of delayed
echo at 4 steps is already at the coin (0.485 against learning-off 0.496): nothing learned
there with or without the rule.

1. What decides (the suite's sheet at 4 steps loses the gain, or the prediction battery's
   sheet at 2 steps lacks it):
   - the delay alone (suite's sheet at 4 steps gains under 0.05; prediction sheet at 2 steps
     gains as the suite's does): 0.4
   - the sheet's shape (sensor side, lattice, motor spread; the suite's sheet still gains at
     4 steps, the prediction sheet does not at 2): 0.35
   - the life length (20,000 steps too short): 0.1
   - the tile's winners in the off arm: 0.05
   - something else or a mix: 0.1
2. The frozen sheet carries the gain (a life with the rule, then the inside frozen and a
   fresh readout: at least half the running rule's gain over the born sheet): 0.55.
3. On the suite's sheet the gain (on minus off) is under 0.05 by 4 steps: 0.3; by 8 steps:
   0.7 (cumulative); still 0.05 or more at 16 steps: 0.1.
4. The time-shuffled life gives most of the gain (within 0.05 of the real-order life, both
   frozen, fresh readout): 0.5.
5. Of point 5's 15 rows, the number that move by 0.05 or more with the rule: 6 (4 to 8 with
   probability 0.6).
