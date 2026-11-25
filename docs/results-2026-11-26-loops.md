# Making the learned loops hold as long as the task needs (idea 172, 2026-11-26)

Queue item 2i of `docs/plan.md`, following `docs/results-2026-11-25-timerule.md`.

## Predictions (written before any suspect switch or scoring run)

Seen before writing this: one smoke trace (`blobprobe looptrace`, a 4,000-step life with the
rule, seed 5, read at 6 blanks). In it the lit loop fires at about 0.24 a tick through the cue
and the first part of blank 1, then falls to 0.02 by blank 2; the sector bias climbs about one
step a tick through the cue (0.7 to 14) and is still 13 when the loop dies, then swings to -11;
the loop cells' own thresholds sit near 27 and move by 0.3 inside a trial; a fifth of the lit
cells are over their bar but cut by the tile's winners during the cue.

1. Which suspect ends the loop (switched off after a normal life, lengthens the trace most):
   - the sector bias moving toward the rate target: 0.6
   - the tile's winners and the inhibition the loop recruits: 0.2
   - too few strong inputs inside the loop: 0.07
   - the cell's own threshold: 0.05
   - the blank frames (noise, pacemakers, presses) driving other cells: 0.05
   - cooldown and leak: 0.03
2. Sector regulation stopped after a normal life (nothing else changed) takes the trace (a
   fresh reader at 0.75 or more) from about 3 blanks to 8 or more: 0.45.
3. The best change of part B, with the rule living under it, seeds 5-12, delayed echo:
   at 4 steps 0.75 or more: 0.5; at 8 steps 0.70 or more: 0.35; at 16 steps 0.65 or more:
   0.25; at 50 steps 0.60 or more: 0.15. (Rule on today: 0.50, 0.48, 0.52; hold cells 1.00.)
4. The best change costs one of the rows that must not break (exclusive-or, echo of 16 cues,
   echo at 5 tiles, the MNIST inside read) 0.03 or more: 0.4.
5. The best change raises the firing rate by more than 5 per 1,024 over the rule-on arm on
   delayed echo: 0.35.
6. Why the rate runs to 50 on sensor side 2 with the wide lattice: the sector bias pinned
   at its cap where sensors crowd: 0.4; the cells' thresholds at their cap: 0.3; something else:
   0.3.
