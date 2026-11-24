# How far a cue carries (2026-11-24)

Idea 42 in `docs/ideas.md`. The question behind it: when Echo fails with the cue and the
button more than a tile or two apart, is that because the cue never arrives (carrying), or
because good and bad cannot wire it up when it does (credit)?

## Method

`scripts/carry.sh` (the probe is `src/probe/carry.cpp`). 4,096 neurons, seed 1. The 16
sensor cells of the corner tile are split into cue A and cue B. In each block of 20 ticks
exactly one is on, so the total input is constant and only the identity changes. No reward
is ever given. After 30,000 ticks to settle, 4,000 blocks are measured. For every neuron:
the gap between its mean spikes per block under A and under B, in units of its
block-to-block spread. A tile's number combines its 64 neurons, less what chance alone
would give. It is what an ideal reader of that tile could get, so it is an upper bound.
1.0 is easy to read; under 0.3 is lost.

## Result

Distance is in tiles from the cue's tile.

| changed | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | reach |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| learning off (as born) | 12.2 | 9.1 | 1.9 | 0.58 | 0.27 | 0.07 | 0.05 | 0.05 | 3 |
| learning on, no reward | 15.1 | 14.7 | 2.8 | 1.31 | 0.90 | 0.23 | 0.17 | 0.08 | 4 |
| same, after 600,000 ticks | 15.7 | 15.3 | 3.4 | 1.38 | 1.39 | 0.57 | 0.35 | 0.11 | 6 |
| no growth or pruning | 11.9 | 9.3 | 1.9 | 0.59 | 0.28 | 0.07 | 0.06 | 0.07 | 3 |
| no prediction learning | 16.6 | 14.7 | 2.8 | 1.22 | 1.10 | 0.21 | 0.15 | 0.09 | 4 |
| half the neurons send far | 13.7 | 15.7 | 3.0 | 1.46 | 1.20 | 0.83 | 0.41 | 0.13 | 6 |
| all delays 1 | 14.8 | 14.9 | 2.7 | 0.80 | 0.76 | 0.33 | 0.31 | 0.20 | 6 |
| inhibition gain 2 | 18.8 | 20.2 | 3.9 | 2.34 | 1.16 | 0.30 | 0.04 | 0.00 | 4 |
| no pulse | 15.8 | 14.4 | 2.7 | 1.01 | 0.85 | 0.22 | 0.23 | 0.14 | 4 |
| branch bonus 6 | 14.7 | 15.0 | 3.4 | 1.70 | 1.17 | 0.22 | 0.19 | 0.11 | 4 |

Denser wiring, more strong connections at birth, less noise, and a lower or higher
activity target all came out close to "learning on" (reach 4).

## Reading

- **The cue does arrive, three or four tiles out.** At 3 and 4 tiles an ideal reader of
  the tile gets about 1.0. Echo fails well inside that range. So between 2 and 4 tiles the
  problem is credit, not carrying: the information is there, spread thin over many
  neurons, and the good/bad rule cannot pick it out.
- **Past about 5 tiles it is carrying.** Nothing readable arrives.
- **Wiring by co-activity is what extends reach.** With growth and pruning off the blob
  carries exactly as far as it did at birth; with them on, and no reward at all, reach
  grows from 3 to 4 tiles and to 6 after a long life. Prediction learning adds nothing to
  it.
- **More far links carry further** (reach 6 with half the neurons sending far).
- **The steepest drop is from 1 tile to 2**, about five times. After that about two times
  per tile.

One seed, and an ideal reader. The differences in the reading are large, but this wants
more seeds, and a reader as weak as the real rule, before it is built on.
