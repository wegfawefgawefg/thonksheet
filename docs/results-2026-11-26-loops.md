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

## Short version

The learned loop fades after two steps because it is a fading echo, not a latch: each cell
forgets its input within 2 to 16 ticks (leak shifts 1-4), so the loop has to re-ignite itself
every few ticks, and with a quarter of its inputs strong (the read-off keeps the born count)
its gain stays under one; the sector bias, which climbs about a step a tick through the cue to
+14, ends it about a step sooner. With a slower leak (leak shifts 4-6, the threshold cap lifted
so regulation can follow) it holds three to four steps: delayed echo at 4 / 8 / 16 / 50 steps
goes 0.49 / 0.48 / 0.49 / 0.49 to 0.74 / 0.54 / 0.50 / 0.49 (rule off with the same leak
0.64 / 0.52 / 0.48 / 0.49, hold cells 1.00), and it costs the rows that need the old item gone:
at matched rate echo of 16 cues -0.17, recall with 1 between -0.17, 1-back -0.11, gap -0.10,
copy of 2 -0.08, exclusive-or -0.05, and the MNIST inside read -0.02.

- **No change found holds a cue until the answer.** The slow leak is a longer fade: +0.26 at 4
  steps (8 of 8 seeds), +0.06 at 8, nothing at 16 and 50. Leak shifts 5-6 slide the same trade
  further (0.79 at 4 steps, 0.61 at 8, echo of 16 cues 0.24).
- **A latch exists, but only as an edit, and only while regulation is held back.** Making
  every input a loop cell takes from its own loop strong, on a lived sheet, turns the loop into
  a latch that the next cue replaces: the loop's own vote is 0.99 to 1.00 after 16 blanks on
  3 of 4 seeds. With the threshold cap lifted the latch is gone (0.60 at 8 blanks): per-cell
  regulation raises the thresholds of a loop that stays lit until it cannot. Every learned
  route to more strong inputs (a larger born strong share, strength from the counter) runs the
  rate away at cap 64, and fades like the slow leak with the cap lifted. A long-held loop and a
  per-cell rate target are at odds; that is the wall.
- **The loops learned at a short delay carry further than the ones learned at a long delay.**
  A 2-step life with the rule, frozen and read by a fresh readout, gets delayed echo at 4 steps
  0.66 against 0.49 for a life at 4 steps; the same under the slow leak gets
  0.88 and 0.73 at 4 and 8 steps against 0.74 and 0.54. So part of what ends the
  trace at long delays is that a life at that delay builds weaker loops (the timerule job's
  wiring report saw 258 carriers at the first blank of a 4-step life against 599 at 2 steps).
- **The slow leak needs the cap lifted.** At the cap of 64 the rate holds for a 2-step life
  (37) but runs to 47-57 at longer delays and to 55 over a 160,000-step life, with a third of
  thresholds at the cap and 6% of cells silent. With the cap at 1000 a 160,000-step life stays
  at 30, no cell silent or stuck. The plain rule pinned at 64 drifts too (1-back over 160,000
  steps: rate 40, 10% of thresholds at cap).
- **Suspects that do not set the length**: the cell's own threshold (it moves by 0.3 inside a
  trial), the tile's winners (0, 4 or 8), inhibition (gain 3 against 5), cooldown, noise and
  pacemakers. The bias costs about a step. The leak and the loop's strong inputs are the
  levers.
- **The prediction sheet's rate** (side 2 with the wide sensor lattice): with the rule, 29% of
  hidden cells end at the threshold cap of 64, firing 150 per 1,024 against 31 for the rest, in
  clusters that take fewer sensor inputs than the other cells (0.8 against 3.4); the sector
  bias sits at its +16 limit 82% of the time. It is hidden-to-hidden runaway that per-cell
  regulation cannot reach past 64. Lifting the cap holds it at 32.

## What was built

- `blobprobe looptrace` (`src/probe/loop_trace.cpp`): lives a delayed-echo life, makes the
  after-life changes below, stops every learning rule (regulation keeps running), finds the
  loop cells (hidden excitatory cells with |d'| >= 0.5 for the cue at the first blank, 600
  calibration trials) and on 1,200 more trials with 16 blanks reports, per step, a fresh ridge
  reader of all hidden cells and the loop's own vote; per tick, the lit and rival loops'
  firing, thresholds and sector bias, lit cells cut by the tile's winners, the inhibitory and
  other hidden cells' firing (csv with `--out`); and the state of the sheet (rate, thresholds,
  share at cap, sector bias spread, what the capped cells listen to). `--loop-strong 1` makes
  every wired input a loop cell takes from its own loop strong.
- `src/probe/after_life.cpp`, used by every probe that takes `--life` (transfer, timewire,
  looptrace): `--after "<name value ...>"` sets genome numbers on the lived sheet (names without
  `--g.`), `--set-cool`, `--set-leak`, `--set-bias`, `--quiet-pace 1`. The wiring stays the
  life's.
- Genome `sector_step_shift` (end of the genome, 0 off, nothing drawn): the sector bias takes
  its one step only with chance 2^-this a tick. Tried; not recommended.
- `scripts/loops.sh` (`loops.py`, `loops_probe.py`, on `timerule_lib.py`): every table here.
  `best` and `suspects` copy the plain arms' lives of the timerule job (one rerun on this code:
  byte-identical); the MNIST rule-alone-at-cap runs are compete2's (one rerun: identical).

## A. What ends the loop

`scripts/loops.sh trace` (seeds 5-8, `out/loops/trace.md`). Every sheet lived the suite's
delayed-echo life of 2 steps (40,000 steps; readout `--g.learn_mode 5 --g.readout_evidence 1
--g.readout_shift 4 --g.readout_balance 1 --motor-spread 128`; tile's winners 4) with the rule
on, unless it says "rule off" or "life:" (the rule living under the change from birth). Then
the suspect switched on the lived sheet, every learning rule stopped, delayed echo shown with
16 blanks. Regulation keeps running, so thresholds re-settle to the change in the calibration
trials.

- **vote**: the loop's own read of the cue (cue-1 loop spikes less cue-0 loop spikes, cut
  fitted on half the trials, scored on the other half). Under 0.5: the other loop fires more.
- **trace**: blanks in a row that a fresh ridge reader of all 2,700 hidden cells reads at 0.75
  or more, mean (per seed). The generous number: it finds faint traces no readout here uses.
- lit / rival: spikes per 1,000 cell-ticks of the loop of the cue shown and of the other loop.

| sheet | trace | vote b1 | vote b2 | vote b3 | vote b4 | vote b8 | vote b16 | lit cue / b1 / b2 / b4 | rival b4 | bias cue / b1 / b2 | rate |
|---|---|---|---|---|---|---|---|---|---|---|---|
| born | 2.5 (2 3 3 2) | 1.000 | 0.698 | 0.609 | 0.486 | 0.477 | 0.464 | 56 / 39 / 36 / 29 | 30 | -1.7 / -4.5 / -8.3 | 31.8 |
| rule off | 2.0 (2 2 2 2) | 1.000 | 0.685 | 0.565 | 0.485 | 0.465 | 0.480 | 61 / 40 / 36 / 29 | 29 | -0.1 / -3.5 / -7.9 | 31.6 |
| rule off, life: leak shift 4-6 | 5.5 (5 6 6 5) | 1.000 | 0.961 | 0.783 | 0.678 | 0.522 | 0.495 | 64 / 37 / 40 / 32 | 28 | 1.3 / 0.9 / -8.1 | 30.8 |
| as lived | 6.0 (6 7 6 5) | 1.000 | 0.969 | 0.850 | 0.715 | 0.413 | 0.412 | 163 / 201 / 114 / 27 | 7 | 10.1 / 13.3 / 8.2 | 32.0 |
| sector bias held at 0 | 5.8 (6 5 6 6) | 1.000 | 0.994 | 0.920 | 0.800 | 0.535 | 0.492 | 154 / 184 / 149 / 71 | 4 | 0.0 / 0.0 / 0.0 | 30.9 |
| sector bias at most 4 | 5.5 (6 5 5 6) | 1.000 | 0.996 | 0.940 | 0.823 | 0.533 | 0.438 | 154 / 195 / 163 / 71 | 4 | 3.9 / 3.1 / 1.7 | 31.6 |
| thresholds frozen | 8.0 (4 16 8 4) | 1.000 | 0.988 | 0.388 | 0.542 | 0.627 | 0.484 | 131 / 94 / 31 / 8 | 9 | 4.2 / 7.2 / -2.2 | 27.6 |
| thresholds 4x slower | 6.2 (6 6 7 6) | 1.000 | 0.973 | 0.875 | 0.754 | 0.497 | 0.480 | 164 / 207 / 127 / 35 | 5 | 10.0 / 13.4 / 8.8 | 32.1 |
| no tile winners | 6.0 (6 7 6 5) | 1.000 | 0.958 | 0.765 | 0.662 | 0.431 | 0.415 | 174 / 197 / 93 / 21 | 8 | 10.7 / 14.7 / 9.4 | 32.5 |
| tile winners 8 | 6.5 (7 7 6 6) | 1.000 | 0.956 | 0.773 | 0.664 | 0.436 | 0.422 | 176 / 199 / 96 / 21 | 8 | 10.6 / 14.5 / 9.3 | 32.5 |
| inhibition 3 (was 5) | 5.8 (6 6 6 5) | 1.000 | 0.967 | 0.833 | 0.702 | 0.421 | 0.415 | 164 / 191 / 103 / 26 | 7 | 10.8 / 13.3 / 8.0 | 32.2 |
| cooldown 1 | 6.0 (7 7 5 5) | 1.000 | 0.930 | 0.785 | 0.706 | 0.448 | 0.439 | 196 / 221 / 130 / 28 | 5 | 9.9 / 13.7 / 9.0 | 32.1 |
| leak shift 6 (was 1-4) | 14.5 (13 16 14 15) | 1.000 | 0.977 | 0.954 | 0.889 | 0.756 | 0.657 | 107 / 138 / 111 / 82 | 3 | 10.3 / 14.5 / 10.0 | 36.4 |
| loop inputs all strong | 14.0 (16 16 8 16) | 0.999 | 0.991 | 0.956 | 0.935 | 0.914 | 0.901 | 185 / 253 / 248 / 236 | 48 | 14.8 / 14.9 / 12.9 | 43.8 |
| loop inputs all strong, cap 1000 | 6.2 (7 6 6 6) | 1.000 | 1.000 | 0.978 | 0.859 | 0.596 | 0.533 | 120 / 210 / 194 / 92 | 3 | 9.1 / 13.2 / 9.5 | 32.1 |
| no noise, no pacemakers | 5.8 (6 6 6 5) | 1.000 | 0.968 | 0.839 | 0.729 | 0.426 | 0.404 | 161 / 203 / 119 / 29 | 5 | 9.6 / 12.8 / 7.8 | 31.9 |
| life: sector bias off | 4.8 (5 5 4 5) | 1.000 | 0.978 | 0.840 | 0.724 | 0.561 | 0.516 | 142 / 176 / 133 / 65 | 3 | 0.0 / 0.0 / 0.0 | 30.7 |
| life: sector bias at most 4 | 5.0 (5 5 5 5) | 1.000 | 0.988 | 0.917 | 0.797 | 0.511 | 0.419 | 146 / 203 / 162 / 77 | 4 | 4.0 / 3.6 / 2.0 | 31.6 |
| life: thresholds 4x slower | 3.2 (3 4 3 3) | 1.000 | 0.762 | 0.590 | 0.525 | 0.487 | 0.484 | 209 / 139 / 22 / 18 | 14 | 9.0 / 12.2 / 3.8 | 32.0 |
| life: no tile winners | 3.0 (3 1 4 4) | 1.000 | 0.740 | 0.641 | 0.476 | 0.466 | 0.459 | 249 / 86 / 7 / 17 | 16 | 9.5 / 15.9 / 8.9 | 31.9 |
| life: inhibition 3 | 5.5 (5 7 5 5) | 1.000 | 0.958 | 0.797 | 0.675 | 0.431 | 0.428 | 179 / 195 / 104 / 31 | 5 | 10.5 / 14.5 / 9.1 | 32.3 |
| life: cooldown 1 | 5.8 (6 6 5 6) | 1.000 | 0.951 | 0.783 | 0.630 | 0.490 | 0.458 | 177 / 276 / 187 / 63 | 4 | 9.4 / 13.8 / 10.7 | 32.3 |
| life: no noise | 5.8 (6 5 6 6) | 1.000 | 0.959 | 0.855 | 0.705 | 0.444 | 0.415 | 177 / 191 / 112 / 24 | 6 | 9.7 / 12.7 / 6.7 | 32.0 |
| life: leak shift 4-6 | 10.8 (12 10 11 10) | 1.000 | 0.968 | 0.908 | 0.876 | 0.660 | 0.481 | 117 / 134 / 100 / 59 | 6 | 10.7 / 15.3 / 10.5 | 32.1 |
| life: leak shift 4-6, slow bias step | 12.5 (13 10 14 13) | 1.000 | 0.990 | 0.957 | 0.897 | 0.606 | 0.466 | 110 / 151 / 145 / 109 | 22 | 5.9 / 7.0 / 8.0 | 46.0 |
| life: slow bias step | 12.8 (9 13 13 16) | 1.000 | 0.935 | 0.909 | 0.864 | 0.445 | 0.375 | 126 / 190 / 168 / 109 | 2 | -5.8 / -3.7 / -2.4 | 32.0 |
| life: strength from the counter, cap 1000 | 4.8 (5 5 5 4) | 1.000 | 0.914 | 0.807 | 0.720 | 0.592 | 0.512 | 174 / 167 / 107 / 61 | 4 | 2.3 / 9.9 / 8.5 | 31.8 |
| life: leak shift 4-6, cap 1000 | 10.5 (12 9 11 10) | 1.000 | 0.879 | 0.906 | 0.880 | 0.755 | 0.573 | 115 / 127 / 51 / 57 | 5 | 1.2 / 8.9 / 9.1 | 31.4 |

Tick by tick, mean of seeds 5-8 (the cue is ticks 0-19, blank 1 ticks 20-39; lit / rival /
inhib in spikes per 1,000 cell-ticks; cut: lit cells over their bar, rested, not fired, so cut
by the tile's winners; full: lit cells whose tile spent all 4 winners):

as lived:

| tick | lit | rival | inhib | lit_thr | lit_bias | lit_cut | lit_full |
|---|---|---|---|---|---|---|---|
| 0 | 9 | 9 | 30 | 28.60 | 4.0 | 0.018 | 0.804 |
| 10 | 202 | 11 | 26 | 28.65 | 10.8 | 0.190 | 0.617 |
| 19 | 240 | 8 | 12 | 28.76 | 14.6 | 0.223 | 0.523 |
| 20 | 249 | 8 | 11 | 28.77 | 14.7 | 0.223 | 0.545 |
| 25 | 218 | 5 | 13 | 28.84 | 14.2 | 0.146 | 0.392 |
| 30 | 200 | 3 | 13 | 28.89 | 13.3 | 0.110 | 0.315 |
| 35 | 177 | 4 | 17 | 28.94 | 12.3 | 0.098 | 0.280 |
| 40 | 154 | 5 | 21 | 28.98 | 11.2 | 0.086 | 0.244 |
| 50 | 111 | 5 | 30 | 29.04 | 8.2 | 0.063 | 0.205 |
| 60 | 77 | 4 | 32 | 29.08 | 4.3 | 0.045 | 0.165 |
| 80 | 34 | 6 | 35 | 29.10 | -3.6 | 0.021 | 0.174 |
| 100 | 22 | 8 | 31 | 29.09 | -7.4 | 0.014 | 0.163 |
| 160 | 18 | 28 | 33 | 29.05 | -7.8 | 0.012 | 0.191 |
| 260 | 14 | 32 | 33 | 28.95 | -7.1 | 0.009 | 0.192 |

sector bias held at 0:

| tick | lit | rival | inhib | lit_thr | lit_bias | lit_cut | lit_full |
|---|---|---|---|---|---|---|---|
| 0 | 35 | 35 | 37 | 25.30 | 0.0 | 0.079 | 0.849 |
| 10 | 190 | 34 | 55 | 25.35 | 0.0 | 0.212 | 0.778 |
| 19 | 215 | 14 | 41 | 25.45 | 0.0 | 0.210 | 0.583 |
| 20 | 215 | 12 | 38 | 25.46 | 0.0 | 0.213 | 0.575 |
| 25 | 191 | 6 | 34 | 25.52 | 0.0 | 0.157 | 0.461 |
| 30 | 182 | 4 | 30 | 25.56 | 0.0 | 0.134 | 0.404 |
| 35 | 176 | 3 | 30 | 25.61 | 0.0 | 0.129 | 0.388 |
| 40 | 169 | 3 | 29 | 25.65 | 0.0 | 0.125 | 0.370 |
| 50 | 151 | 2 | 28 | 25.73 | 0.0 | 0.108 | 0.335 |
| 60 | 126 | 2 | 27 | 25.79 | 0.0 | 0.092 | 0.292 |
| 80 | 85 | 3 | 26 | 25.88 | 0.0 | 0.062 | 0.229 |
| 100 | 58 | 4 | 25 | 25.93 | 0.0 | 0.043 | 0.200 |
| 160 | 28 | 6 | 27 | 25.95 | 0.0 | 0.021 | 0.190 |
| 260 | 15 | 9 | 28 | 25.88 | 0.0 | 0.014 | 0.217 |

leak shift 6 (was 1-4):

| tick | lit | rival | inhib | lit_thr | lit_bias | lit_cut | lit_full |
|---|---|---|---|---|---|---|---|
| 0 | 22 | 22 | 30 | 42.56 | 6.1 | 0.046 | 0.789 |
| 10 | 120 | 17 | 32 | 42.58 | 10.7 | 0.132 | 0.681 |
| 19 | 183 | 7 | 23 | 42.65 | 14.0 | 0.174 | 0.550 |
| 20 | 186 | 6 | 23 | 42.66 | 14.3 | 0.178 | 0.548 |
| 25 | 145 | 3 | 22 | 42.70 | 15.1 | 0.086 | 0.364 |
| 30 | 123 | 2 | 18 | 42.73 | 14.9 | 0.031 | 0.177 |
| 35 | 123 | 2 | 20 | 42.75 | 14.1 | 0.030 | 0.193 |
| 40 | 120 | 2 | 22 | 42.78 | 13.0 | 0.032 | 0.197 |
| 50 | 110 | 2 | 26 | 42.83 | 9.9 | 0.032 | 0.188 |
| 60 | 99 | 2 | 28 | 42.87 | 6.3 | 0.030 | 0.183 |
| 80 | 82 | 3 | 29 | 42.93 | 0.2 | 0.027 | 0.179 |
| 100 | 82 | 3 | 30 | 42.99 | -2.7 | 0.030 | 0.208 |
| 160 | 59 | 4 | 30 | 43.12 | -1.8 | 0.021 | 0.203 |
| 260 | 35 | 6 | 31 | 43.18 | -0.6 | 0.015 | 0.249 |

life: leak shift 4-6:

| tick | lit | rival | inhib | lit_thr | lit_bias | lit_cut | lit_full |
|---|---|---|---|---|---|---|---|
| 0 | 25 | 25 | 27 | 34.59 | 5.8 | 0.107 | 0.866 |
| 10 | 136 | 20 | 47 | 34.62 | 11.1 | 0.223 | 0.837 |
| 19 | 183 | 9 | 23 | 34.69 | 14.9 | 0.194 | 0.585 |
| 20 | 185 | 8 | 22 | 34.70 | 15.1 | 0.202 | 0.576 |
| 25 | 149 | 4 | 24 | 34.74 | 15.8 | 0.126 | 0.459 |
| 30 | 116 | 3 | 14 | 34.77 | 15.6 | 0.034 | 0.158 |
| 35 | 112 | 3 | 16 | 34.80 | 15.0 | 0.032 | 0.163 |
| 40 | 111 | 3 | 19 | 34.82 | 14.0 | 0.036 | 0.183 |
| 50 | 99 | 4 | 24 | 34.87 | 10.6 | 0.034 | 0.168 |
| 60 | 88 | 4 | 30 | 34.90 | 5.6 | 0.032 | 0.159 |
| 80 | 66 | 5 | 30 | 34.96 | -2.2 | 0.026 | 0.153 |
| 100 | 55 | 6 | 31 | 34.99 | -5.1 | 0.026 | 0.161 |
| 160 | 41 | 11 | 31 | 35.04 | -5.7 | 0.021 | 0.186 |
| 260 | 27 | 20 | 30 | 35.04 | -3.2 | 0.018 | 0.235 |

What it says:

- **The loop decays on its own.** With the bias held at 0 the lit loop still goes 215 (end of
  cue) -> 169 (tick 40) -> 126 (60) -> 58 (100) -> 28 (160): a fade over about 2.5 steps.
  Nothing in the loop cells' own state moves inside a trial (lit thresholds 25.3 to 25.9), so
  it is a loop gain under one, not fatigue.
- **The bias costs about a step.** As lived the bias climbs one step a tick through the cue (4
  to 14.7 by tick 20) and is still 11 at tick 40; the loop is 154 there against 169 with the
  bias flat, 22 against 58 at tick 100. Holding it at 0 or at most 4 lifts the vote at 4 blanks
  from 0.72 to 0.80 and 0.82, and nothing at 8.
- **The leak is the time constant.** Leak shift 6 on the lived sheet (nothing else changed):
  the lit loop holds 120 through blank 1 and 82 at tick 100; trace 14.5 blanks, vote 0.89 / 0.76
  / 0.66 at 4 / 8 / 16 blanks. The rule living under leak 4-6 with the cap lifted: 0.88 / 0.76 /
  0.57.
- **Strong loop inputs make a latch while the cap holds regulation back.** All of a loop
  cell's own-loop inputs strong: the lit loop stays at 236 to 248 to the end of 16 blanks, the
  rival at 48; the vote is 0.99 to 1.00 at 16 blanks on seeds 5, 6 and 8 (0.62 on 7), and the
  next cue replaces it every trial (vote 0.999 at blank 1). The rate rises to 44. With the cap
  at 1000 the thresholds rise past 64 and the latch fades: vote 0.86 / 0.60 / 0.53.
- **The rest do not move it**: no tile's winners or 8 of them (vote at 4 blanks 0.66 against
  0.72; the tile's winners cut a fifth of lit cells in the cue and a tenth after), inhibition 3,
  cooldown 1, no noise or pacemakers, thresholds 4x slower: all within 0.06 of as lived at 4
  blanks. Frozen thresholds spread from 4 to 16 blanks by seed and the vote turns over at blank
  3: a frozen sheet out of balance, not a cause.
- **Living under the change**: the bias off or capped at 4 gives what the switch gave; a slow
  bias step (`sector_step_shift` 3) gives trace 12.8 but the vote turns over by 8 blanks (0.45),
  and with the slow leak it runs the rate to 46. No tile's winners in the life and thresholds 4x
  slower in the life shorten it (trace 3.0 and 3.2): the winners shape which loops form.

Delayed echo with each suspect (`scripts/loops.sh suspects --seeds 5-8`). Upper table: a
2-step life with the rule (20,000 steps), the suspect switched, the inside frozen, the motor
cells as born and a fresh readout learning delayed echo at the column's delay for 20,000 steps;
late score. Lower table: a whole life (blobrun, 40,000 steps) at that delay with the rule living
under the change; late score and rate.

| after a normal life | 2 steps | 3 steps | 4 steps | 8 steps | 16 steps |
|---|---|---|---|---|---|
| as lived | 0.962 | 0.804 | 0.655 | 0.519 | 0.469 |
| sector bias held at 0 | 0.731 | 0.595 | 0.507 | 0.495 | 0.495 |
| thresholds frozen | 0.980 | 0.811 | 0.700 | 0.609 | 0.507 |
| no tile winners | 0.986 | 0.767 | 0.628 | 0.535 | 0.500 |
| inhibition 3 (was 5) | 0.938 | 0.780 | 0.617 | 0.541 | 0.500 |
| leak shift 6 (was 1-4) | 0.968 | 0.922 | 0.878 | 0.766 | 0.647 |
| lived under leak 4-6, cap 1000 | 0.929 | 0.920 | 0.882 | 0.733 | 0.579 |

| rule living under it | 2 steps | 3 steps | 4 steps | 8 steps | 16 steps |
|---|---|---|---|---|---|
| rule on, as it is | 0.962 (34) | 0.564 (32) | 0.499 (31) | 0.481 (31) | 0.525 (38) |
| life: sector bias off | 0.719 (33) | 0.511 (31) | 0.474 (30) | 0.472 (29) | 0.470 (28) |
| life: slow bias step | 0.744 (33) | 0.581 (32) | 0.518 (32) | 0.491 (31) | 0.481 (41) |
| life: leak shift 4-6 | 0.954 (34) | 0.756 (42) | 0.568 (45) | 0.484 (55) | 0.490 (57) |
| life: leak shift 5-6 | 0.909 (38) | 0.773 (47) | 0.612 (48) | 0.480 (50) | 0.493 (55) |

- **The 2-step loops reach further than a life at the delay does.** As lived, frozen and read
  at 4 steps: 0.66; a life at 4 steps: 0.50. Under the slow leak with the cap lifted: 0.88 at 4
  and 0.73 at 8 against 0.74 and 0.54 for lives at those delays.
- **The leak is the only switch that carries the task**: leak shift 6 on the 2-step sheet gives
  0.88 / 0.77 / 0.65 at 4 / 8 / 16 steps.
- **Holding the bias at 0 hurts the task** although it lengthens the loop (0.73 at 2 steps
  against 0.96): the readout needs the sector bias where its own cells are. Not a lever.
- Thresholds frozen (0.70 / 0.61 at 4 / 8), no tile's winners and inhibition 3 stay within
  about 0.05 of as lived at 4 steps.
- Lives under the change at cap 64 run the rate up with the delay (leak 4-6: 42 to 57) and
  lose most of it; the same lives with the cap lifted are part B's leak-cap arm.

## B. The smallest change that holds it

Candidates, in the order part A pointed:

| candidate | cost to the machine | vote at 4 / 8 blanks (trace) | delayed echo at 4 / 8 steps, rate | verdict |
|---|---|---|---|---|
| leak shifts 4-6, cap 1000 (`--g.leak_shift_min 4 --g.leak_shift_max 6 --g.threshold_cap 1000`) | none | 0.88 / 0.76 (10.5) | 0.74 / 0.54, 31 | scored in full |
| the same at cap 64 | none | 0.88 / 0.66 (10.8) | 0.59 / 0.50, 47 / 54 | rate runs up |
| leak shifts 5-6, cap 1000 | none | - | 0.79 / 0.61, 30 | runner-up: further, costs more |
| sector bias at most 4, or off | none | 0.80 / 0.51 (5.0) | 0.47 at 4 (bias off) | a step at most |
| slow bias step (`sector_step_shift` 3) | one random word per 64 sectors a tick | 0.86 / 0.45 (12.8) | 0.53 / 0.50, 32 | vote turns over |
| more strong inputs from birth (`strong_shift` 1, 0; 2 seeds) | none | 0.59 / 0.55, 0.48 / 0.48 | life at 2 steps 0.44, 0.46; rate 60-67 | runs away |
| strength from the counter (`tag_counts` 1; 2 seeds) | no cheap read-off | 0.49 / 0.50 | life at 2 steps 0.45; rate 68 | runs away |
| the same, cap 1000 | no cheap read-off | 0.72 / 0.59 (4.8) | - | fades like the leak |

Not tried, and why: a loop that inhibits rival loops (the rival loop is already silent, 3 to 8
per 1,000, while the lit loop fades: the rival is not what ends it); a slower
`compete_down_shift` (it changes which inputs are wired, not how strong; the loop already has
28 of 49); a threshold that rises only when the tile is over target (the cell's threshold does
not move inside a trial; it may be what a learned latch needs, below).

### The slow leak on every row

`scripts/loops.sh best` (seeds 5-12; seed 12 on stevejobs), the suite's sheet and life, 40,000
steps. Arms: off / on / oncap (rule on, cap 1000) / churn are the timerule job's lives; leak-off
(rule off, the leak), leak-on (rule on, the leak, cap 64), **leak-cap (rule on, the leak, cap
1000: the change)**, leak-churn (the leak, the rule's moves toward random sources; on the rows
a trace should move). Late score, rate per 1,024 in brackets.

| row | off | on | oncap | churn | leak-off | leak-on | leak-cap | leak-churn | leak-on - on (seeds up) | leak-on - leak-off (seeds up) | leak-cap - oncap (seeds up) |
|---|---|---|---|---|---|---|---|---|---|---|---|
| delayed echo, 2 steps | 0.587 (31) | 0.913 (34) | 0.874 (31) | 0.537 (31) | 0.861 (31) | 0.918 (37) | 0.923 (31) | 0.773 (31) | +0.005 (3/8) | +0.057 (7/8) | +0.049 (6/8) |
| delayed echo, 3 steps | 0.498 (31) | 0.572 (32) | 0.657 (31) | 0.481 (31) | 0.733 (31) | 0.779 (43) | 0.850 (31) |   -   (  -  ) | +0.207 (7/8) | +0.046 (5/8) | +0.193 (6/8) |
| delayed echo, 4 steps | 0.480 (31) | 0.499 (32) | 0.486 (31) | 0.480 (31) | 0.639 (30) | 0.585 (47) | 0.743 (31) | 0.591 (30) | +0.087 (4/8) | -0.054 (2/8) | +0.257 (8/8) |
| delayed echo, 8 steps | 0.487 (30) | 0.482 (31) | 0.479 (31) | 0.492 (30) | 0.524 (30) | 0.497 (54) | 0.537 (30) | 0.497 (30) | +0.015 (7/8) | -0.026 (0/8) | +0.058 (7/8) |
| delayed echo, 16 steps | 0.494 (30) | 0.517 (44) | 0.494 (31) | 0.484 (30) | 0.479 (30) | 0.485 (57) | 0.499 (30) | 0.497 (30) | -0.033 (0/8) | +0.006 (4/8) | +0.005 (4/8) |
| delayed echo, 50 steps | 0.521 (30) | 0.504 (57) | 0.493 (30) |   -   (  -  ) | 0.485 (30) | 0.475 (56) | 0.490 (29) |   -   (  -  ) | -0.029 (2/8) | -0.010 (5/8) | -0.003 (5/8) |
| beat, every 4 steps | 0.479 (31) | 0.451 (58) | 0.480 (31) | 0.482 (30) | 0.435 (35) | 0.453 (55) | 0.470 (31) |   -   (  -  ) | +0.003 (5/8) | +0.019 (8/8) | -0.010 (2/8) |
| copy 2 of 2 | 0.688 (32) | 0.894 (39) | 0.854 (32) | 0.596 (32) | 0.751 (32) | 0.922 (42) | 0.775 (32) | 0.684 (32) | +0.029 (5/8) | +0.172 (8/8) | -0.079 (0/8) |
| copy 2 to 3 of 2 | 0.526 (32) | 0.626 (40) | 0.589 (32) | 0.499 (32) | 0.621 (32) | 0.714 (42) | 0.617 (31) |   -   (  -  ) | +0.089 (8/8) | +0.093 (7/8) | +0.028 (6/8) |
| recall, 1 between | 0.698 (32) | 0.799 (37) | 0.865 (32) | 0.611 (32) | 0.719 (32) | 0.933 (38) | 0.691 (32) |   -   (  -  ) | +0.134 (8/8) | +0.215 (8/8) | -0.174 (0/8) |
| recall, 2 between | 0.510 (32) | 0.532 (38) | 0.682 (33) | 0.496 (32) | 0.586 (32) | 0.722 (40) | 0.566 (32) |   -   (  -  ) | +0.190 (8/8) | +0.136 (8/8) | -0.116 (1/8) |
| 1-back, 4 symbols | 0.726 (33) | 0.978 (40) | 0.864 (33) | 0.591 (33) | 0.720 (32) | 0.965 (49) | 0.755 (32) | 0.590 (32) | -0.013 (2/8) | +0.245 (8/8) | -0.109 (0/8) |
| 2-back, 4 symbols | 0.501 (33) | 0.611 (39) | 0.553 (33) | 0.483 (33) | 0.541 (32) | 0.684 (47) | 0.578 (32) |   -   (  -  ) | +0.073 (8/8) | +0.143 (8/8) | +0.025 (7/8) |
| order | 0.664 (32) | 0.728 (40) | 0.809 (32) | 0.606 (32) | 0.924 (32) | 0.840 (41) | 0.787 (32) |   -   (  -  ) | +0.112 (7/8) | -0.084 (0/8) | -0.021 (4/8) |
| gap | 0.544 (31) | 0.938 (39) | 0.929 (31) | 0.503 (31) | 0.696 (31) | 0.979 (36) | 0.829 (31) | 0.605 (31) | +0.041 (7/8) | +0.283 (8/8) | -0.100 (0/8) |
| exclusive-or (both-not-either) | 0.960 (32) | 0.998 (37) | 0.995 (32) |   -   (  -  ) | 0.863 (32) | 0.990 (42) | 0.947 (32) |   -   (  -  ) | -0.008 (1/8) | +0.126 (8/8) | -0.047 (0/8) |
| echo, 16 cues | 0.318 (32) | 0.458 (33) | 0.460 (32) |   -   (  -  ) | 0.174 (31) | 0.370 (48) | 0.290 (31) |   -   (  -  ) | -0.088 (0/8) | +0.196 (8/8) | -0.171 (0/8) |
| echo, cue and button 5 tiles apart | 0.595 (32) | 0.902 (47) | 0.600 (33) |   -   (  -  ) | 0.601 (32) | 0.951 (56) | 0.590 (33) |   -   (  -  ) | +0.049 (7/8) | +0.349 (8/8) | -0.011 (3/8) |

- **Delayed echo**: at matched rate the leak lifts 3 steps 0.66 -> 0.85 and 4 steps 0.49 ->
  0.74 (8 of 8 seeds), 8 steps +0.06 (7 of 8), 16 and 50 nothing. Rule off with the leak gets
  0.64 at 4 steps and churn with it 0.59: of the +0.26, the learned loop is +0.10, the slower
  cell the rest.
- **What it costs at matched rate**: echo of 16 cues 0.46 -> 0.29, recall with 1 between 0.87
  -> 0.69 and with 2 between 0.68 -> 0.57, 1-back 0.86 -> 0.76, gap 0.93 -> 0.83, copy of 2 0.85
  -> 0.78, exclusive-or 0.995 -> 0.947: every row whose answer needs the last item gone or two
  items told apart in time. Echo at 5 tiles is unchanged (0.59).
- **At cap 64 (as the rate runs)** the leak looks better on the short rows (recall 0.93, 1-back
  0.97, gap 0.98, echo at 5 tiles 0.95) but those come with rates of 36 to 56 against 32 to 47
  for the rule alone; at 4 steps it falls to 0.59 at rate 47. The rule's gains at cap 64 on
  these rows were already partly rate (timerule).
- **How a held cue is replaced**: the leak's trace of the old item lingers into the next
  steps, which is what 1-back (-0.11) and echo of 16 cues (-0.17) pay at matched rate. The
  latch of part A was replaced cleanly by the next cue; a fade is not replaced, it overlaps.

Controls: `scripts/loops.sh controls --seeds 5-8`, a life of 20,000 steps at the row's delay
(born: none), then the inside frozen and a fresh readout for 20,000 steps; late score.

| row | born | lived | born, change | lived, change | shuffled, change | churn, change | lived, change, cap | born, change, cap | shuffled, change, cap |
|---|---|---|---|---|---|---|---|---|---|
| delayed echo, 4 steps | 0.481 | 0.498 | 0.646 | 0.583 | 0.620 | 0.642 | 0.753 | 0.627 | 0.758 |
| delayed echo, 8 steps | 0.481 | 0.491 | 0.527 | 0.519 | 0.511 | 0.508 | 0.519 | 0.518 | 0.549 |

- At matched rate the life under the leak adds +0.13 over the born sheet with the leak at 4
  steps (0.75 against 0.63) and nothing at 8. A time-shuffled life gives all of it (0.76): as in
  the timerule job, the loops come from what is on together, not from order.

The runner-up, leak shifts 5-6 (`scripts/loops.sh runner --change leak56`, seeds 5-12; set
leak56-cap beside the oncap column of the table above):

| row | on | leak56-on | leak56-cap | leak56-on - on (seeds up) |
|---|---|---|---|---|
| delayed echo, 2 steps | 0.913 (34) | 0.923 (39) | 0.923 (31) | +0.010 (5/8) |
| delayed echo, 3 steps | 0.572 (32) | 0.790 (46) | 0.883 (31) | +0.218 (7/8) |
| delayed echo, 4 steps | 0.499 (32) | 0.565 (49) | 0.790 (30) | +0.066 (4/8) |
| delayed echo, 8 steps | 0.482 (31) | 0.499 (52) | 0.613 (30) | +0.017 (5/8) |
| delayed echo, 16 steps | 0.517 (44) | 0.496 (55) | 0.510 (30) | -0.021 (3/8) |
| delayed echo, 50 steps | 0.504 (57) | 0.511 (56) | 0.482 (29) | +0.007 (5/8) |
| exclusive-or (both-not-either) | 0.998 (37) | 0.976 (45) | 0.937 (32) | -0.022 (0/8) |
| echo, 16 cues | 0.458 (33) | 0.350 (53) | 0.240 (31) | -0.108 (0/8) |
| echo, cue and button 5 tiles apart | 0.902 (47) | 0.937 (57) | 0.583 (32) | +0.035 (7/8) |

### Long lives

`scripts/loops.sh long` (seeds 5-8, 160,000 steps, ran on stevejobs): late score, rate per
1,024, share of cells silent and stuck at the end, share of thresholds at the cap.

| row | arm | late | rate_per_1024 | share_silent | share_stuck | share_threshold_cap |
|---|---|---|---|---|---|---|
| delayed echo, 8 steps, 160,000 steps | on | 0.487 | 31.566 | 0.000 | 0.000 | 0.006 |
| delayed echo, 8 steps, 160,000 steps | leak-on | 0.497 | 55.350 | 0.061 | 0.000 | 0.320 |
| delayed echo, 8 steps, 160,000 steps | oncap | 0.478 | 30.697 | 0.000 | 0.000 | 0.000 |
| delayed echo, 8 steps, 160,000 steps | leak-cap | 0.536 | 30.127 | 0.000 | 0.000 | 0.000 |
| 1-back, 160,000 steps | on | 0.957 | 39.571 | 0.000 | 0.000 | 0.102 |
| 1-back, 160,000 steps | leak-on | 0.952 | 51.539 | 0.035 | 0.000 | 0.255 |
| 1-back, 160,000 steps | oncap | 0.855 | 32.629 | 0.000 | 0.000 | 0.000 |
| 1-back, 160,000 steps | leak-cap | 0.776 | 32.192 | 0.000 | 0.000 | 0.000 |

At cap 1000 the change is stable over a long life (rate 30 to 32, nothing silent or stuck);
at cap 64 it is not. The rule alone at cap 64 drifts on 1-back (rate 40, 10% at cap). The
delayed echo at 8 steps gain is the 40,000-step one (+0.06).

### MNIST inside read

`scripts/loops.sh mnist` (`scripts/inside.sh`, seeds 5-8): a fresh linear reader of the
non-sensor cells after a 60,000-digit life with competitive learning (B5: `--g.learn_mode 5
--g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5`), against the born sheet and a life on
shuffled digits.

| sheet | born | lived | shuffled | lived - born | read at (lived) |
|---|---|---|---|---|---|
| B5, cap 64 | 0.9254 | 0.9521 | 0.9378 | +0.027 | 63 |
| B5 and the leak, cap 64 | 0.9100 | 0.9332 | 0.9215 | +0.023 | 78 |
| B5, cap 1000 | 0.9218 | 0.9342 | 0.9321 | +0.012 | 31 |
| **B5 and the leak, cap 1000 (the change)** | 0.9029 | 0.9137 | 0.9130 | +0.011 | 31 |

The leak lowers the whole sheet's read by about 0.02 (born too); what the life adds is the
same. Under the change the lived sheet no longer beats the random projection (0.943) or the
pixels (0.922).

## The prediction sheet's rate

`scripts/loops.sh pred` (seeds 5-8): delayed echo of 2 steps (the prediction battery's sheet:
`--sensor-side 2 --g.sensor_w 64 --g.motor_stride 1 --motor-spread 64`, 20,000 steps), read as
in part A at cap 64. Rate per 1,024 over the read; thresholds of hidden cells, mean and share
at the cap; sector bias, mean and share of sector-ticks at +16 and -16; the capped cells'
sensor inputs and rate against the others'.

| sheet | rate | threshold mean | at cap | bias mean | at +max | at -max | loop cells | vote b1 / b2 | capped cells: sensor inputs (others) | capped: rate (others) |
|---|---|---|---|---|---|---|---|---|---|---|
| side 2, wide lattice, off | 31.7 | 22.5 | 0.000 | -8.0 | 0.000 | 0.080 | 121 | 0.998 / 0.707 | 3.3 (9.2) | 36.0 (31.7) |
| side 2, wide lattice, on | 66.0 | 33.4 | 0.294 | 12.9 | 0.818 | 0.005 | 167 | 1.000 / 0.908 | 0.8 (3.4) | 149.5 (31.2) |
| side 2, wide lattice, oncap | 31.8 | 32.0 | 0.000 | -5.0 | 0.141 | 0.339 | 96 | 1.000 / 0.655 | - (-) | - (-) |
| side 3, wide lattice, off | 31.7 | 22.3 | 0.000 | -7.6 | 0.002 | 0.084 | 178 | 1.000 / 0.707 | 16.7 (9.2) | 33.5 (31.7) |
| side 3, wide lattice, on | 33.3 | 29.4 | 0.002 | -4.3 | 0.176 | 0.346 | 172 | 1.000 / 0.843 | 1.0 (3.3) | 35.5 (33.7) |
| side 3, wide lattice, oncap | 31.8 | 31.1 | 0.000 | -5.6 | 0.126 | 0.325 | 178 | 1.000 / 0.696 | - (-) | - (-) |
| side 2, standard lattice, off | 31.6 | 21.6 | 0.000 | -7.7 | 0.000 | 0.080 | 177 | 1.000 / 0.767 | 5.2 (3.6) | 33.3 (31.6) |
| side 2, standard lattice, on | 32.3 | 28.6 | 0.003 | -4.2 | 0.201 | 0.372 | 377 | 0.996 / 0.796 | 1.7 (1.4) | 37.4 (32.2) |
| side 2, standard lattice, oncap | 31.9 | 30.2 | 0.000 | -4.7 | 0.171 | 0.366 | 206 | 1.000 / 0.812 | - (-) | - (-) |
| suite (side 3, standard), off | 31.6 | 21.4 | 0.000 | -7.1 | 0.001 | 0.073 | 246 | 1.000 / 0.685 | 0.0 (3.6) | 35.3 (31.6) |
| suite (side 3, standard), on | 32.0 | 26.2 | 0.002 | -4.1 | 0.194 | 0.371 | 472 | 1.000 / 0.969 | 1.7 (2.0) | 34.1 (32.0) |
| suite (side 3, standard), oncap | 31.8 | 28.6 | 0.000 | -4.4 | 0.157 | 0.334 | 524 | 1.000 / 0.926 | - (-) | - (-) |

Only side 2 together with the wide lattice runs away, as the timerule job found. There the
rule wires cells into hidden-to-hidden clusters that fire at five times the target with few
sensor inputs (0.8 against 3.4): with groups of 4 cells per line spread across the top half,
a hidden cell sees little of any one line, so what is on when it fires is mostly other hidden
cells, and "move toward what was on" builds loops of nothing. Per-cell regulation raises their
thresholds to the cap of 64 and stops; the sector bias goes to +16 and stops. With the cap at
1000 the rate holds at 32 and the cue loops form weakly (vote at the second blank 0.66 against
0.71 born). Why side 2 alone with the standard lattice does not run away (32) was not looked at.

## What is real, what is suspect

Real (4 to 8 seeds, every number from a script line below):

- The learned loop is a fade with a time constant of about 2.5 steps when the bias is held,
  2 when it runs; the leak sets it; the bias shortens it by about a step; threshold, tile's
  winners, inhibition, cooldown and noise do not.
- The slow leak with the cap lifted: delayed echo 0.74 at 4 steps (8 of 8 seeds over the rule
  at matched rate), 0.54 at 8, nothing at 16 and 50; costs of 0.05 to 0.17 on six rows and 0.02
  on MNIST; stable over 160,000 steps.
- At cap 64 the slow leak runs the rate away; so do the side-2 sheet and every route to more
  strong inputs; the plain rule drifts there over long lives.
- Strong own-loop inputs latch the loop on a lived sheet and the next cue replaces it, but only
  while thresholds are capped at 64.

Suspect:

- The headline "rule off with the same leak" (0.64 at 4 steps) is at cap 64 with the rate at
  30; a rule-off arm at cap 1000 was not run on the full rows (the frozen-read control at cap
  has it: born 0.63).
- The suspects table (part A, task scores) uses a 2-step life read at each delay by a fresh
  readout with the inside frozen: it measures what the 2-step loops carry, not a life at that
  delay. Its strongest number (leak shift 6 after the life: 0.91 / 0.76 / 0.64 at 4 / 8 / 16)
  is an edit of a lived sheet, not a rule.
- "Loops learned at short delays carry further" rests on transfers at seeds 5-8; the reason
  (blank-time learning diluting the loop) is a guess.
- The trace column (fresh ridge reader of all hidden cells) reads faint traces that the
  readout does not use; the vote and the task scores are the honest numbers.
- `strong_shift` and `tag_counts` explorations are 2 seeds each (`out/loops/test`), outside the
  script; they are why those candidates were dropped, not results.
- MNIST rows: 4 seeds; the B5 cap rows are compete2's runs (one rerun identical).

## Not done

- A learned latch. Part A says what it would take: a loop's own inputs strong, and a rate
  regulation that does not take back a loop held for many steps (counting over the tile or the
  sector, or over a longer span than a trial). Neither was built.
- Why a life at 4 or 8 steps builds weaker loops than a life at 2 steps, and whether gating
  the rule to the cue's steps fixes it.
- Rule off at cap 1000 on every row; leak-churn on every row (only on 7).
- The suspects table on more than seeds 5-8; the runner-up's controls, long life and MNIST.
- Why side 2 with the standard lattice holds where side 2 with the wide lattice runs away.

## Hardware

The change recommended is two genome numbers and no new state or work: `leak_shift_min 4`,
`leak_shift_max 6` (was 1 and 4). Each cell already carries its leak shift as one byte drawn
at birth, and the leak is the same shift and subtract in the same loop; only the range of the
draw moves. Its companion is `threshold_cap 1000` (was 64): thresholds are already 16-bit
words, the cap is one compare. A slower leak makes a cell's potential 16 to 64 times its drive
instead of 2 to 16 times, so a cell that is to fire at the target rate needs a threshold
above 64 (they settle near 35 to 85); with the cap at 64 regulation runs out of range and the
rate climbs (55 over a 160,000-step life). The potential is 16 bits with a ceiling of 16,000:
at 64 times a drive of 50 it reaches 3,200, inside the range. `sector_step_shift` (added, off,
not recommended) costs one random word per 64 sectors a tick when on and nothing when off
(no draw). `blobbench` with every new number off: same within noise, three rounds each on this PC under the temperature governor (rough): 8 tiles, learning off, 68,600 to 69,400 ticks a second at the base (the branch point of this job) against 68,800 to 69,900 here; learning on 45,800 to 47,200 against 45,600 to 46,400; 32 tiles 3,200 to 3,800 off and 2,400 to 3,100 on against 3,400 to 3,500 and 2,400 to 2,600.

## Script lines

```
scripts/loops.sh trace              part A: looptrace, each suspect switched or lived under
                                    (seeds 5-8; 31 sheets, 124 probes)
scripts/loops.sh suspects --seeds 5,6   part A: delayed echo at 2-16 steps per suspect
scripts/loops.sh pred               the prediction sheet's rate (seeds 5-8)
scripts/loops.sh best --seeds 5-11  part B, this PC (seed 12 on stevejobs:
                                    MINI_DIR=thonk-loops scripts/mini.sh bg b12
                                    'scripts/loops.sh best --seeds 12 --jobs 2')
scripts/loops.sh runner --change leak56   the runner-up
scripts/loops.sh controls --seeds 5-8     frozen read, shuffled, churn, born (transfer)
scripts/loops.sh long               160,000-step lives (ran on stevejobs)
scripts/loops.sh mnist              the MNIST inside read, four arms (scripts/inside.sh)
scripts/loops.sh all --quick        smoke check, 2 seeds, short lives (ran end to end)
scripts/loops.sh best --seeds 5-12 --table-only    a table from what is there
bash out/loops/refcheck.sh          the four reference lives against out/merge-ref
```

## Predictions, scored

1. Which suspect ends the loop. The sector bias (0.6): wrong; it costs about a step. The leak,
   given 0.03 under "cooldown and leak", sets the length, and the loop's few strong inputs
   (0.07) are the other lever. The tile's winners and inhibition (0.2), the threshold (0.05)
   and the blank frames (0.05): no.
2. The bias stopped after the life takes the trace to 8 blanks or more (0.45): no, 5.8 (the
   generous reader), vote at 4 blanks 0.72 -> 0.80.
3. The best change, seeds 5-12: at 4 steps 0.75 or more (0.5): no, 0.743 (the runner-up
   0.79); at 8 steps 0.70 or more (0.35): no, 0.54; at 16 steps 0.65 or more (0.25): no, 0.50;
   at 50 steps 0.60 or more (0.15): no, 0.49.
4. It costs a row that must not break 0.03 or more (0.4): yes; echo of 16 cues -0.17,
   exclusive-or -0.05, MNIST -0.02.
5. It raises the rate by more than 5 per 1,024 over the rule on delayed echo (0.35): for the
   change as recommended, no (31 against 31 at matched rate); at the cap of 64, yes (47 against
   32 at 4 steps).
6. The prediction sheet's rate: thresholds at their cap (0.3) is the one that matters (29% at
   cap; lifting it holds the rate); the sector bias is pinned at +16 too (0.4 was on it), but
   as a consequence.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`: a new finding (likely, 4 to 8 seeds): **the learned cue loop is a
  fade, not a latch.** Its time constant is the cells' leak; the sector bias shortens it by
  about a step; the cell threshold, the tile's winners, inhibition, cooldown and noise do not
  set it. A slower leak with the threshold cap lifted holds a cue three to four steps (delayed
  echo at 4 steps 0.49 -> 0.74) at the price of the rows that need the old item gone (echo of
  16 cues -0.17, recall -0.17, 1-back -0.11). A loop whose own inputs are all strong latches
  (0.99 at 16 blanks) and is replaced by the next cue, but no learned route to it was found:
  more strong inputs from birth or from the counter run the rate away.
- `docs/findings.md`: finding 50's second half, the prediction sheet: the rate runs to 50-66
  because a third of the hidden cells end at the threshold cap of 64 in hidden-to-hidden
  clusters that fire five times the target (few sensor inputs), with the sector bias pinned at
  +16; lifting the cap holds the rate at 32.
- `docs/findings.md`: the threshold cap of 64 binds whenever the cells' gain rises (slower
  leak, more strong inputs, the side-2 sheet, and the plain rule on 1-back over 160,000 steps:
  10% at cap, rate 40). `--g.threshold_cap 1000` is not just a control; consider it the default.
- `docs/plan.md`: item 2i done with this doc. New items: (a) why a life at 4 steps builds
  weaker loops than a life at 2 steps (the 2-step sheet, frozen, reads 0.72 at 4 steps; a
  4-step life 0.49; the guess is the rule moving loop cells toward blank-time activity); (b) a
  learned latch: strength from co-activity for a loop's own inputs only, under a regulation
  that acts on the tile or sector rather than the cell, so a latched loop is not regulated away
  (part A's edit shows the latch works and is replaced by the next cue).
- `docs/ideas.md` 172: done; the two items above as new ideas.
- `docs/predictions.md`: the six above, scored.

## Files that may collide with other branches

- Shared: `src/blob/genome.hpp` (one number at the end), `src/blob/genome_args.cpp` (one line
  at the end of the table), `src/blob/regulate.cpp` (one argument to `regulate_one_sector` and
  a three-line block in `regulate_sectors`; nothing in `regulate_thresholds`).
- `src/probe/lived_sheet.cpp` (`probe_sheet` calls `after_life`: two lines and a
  declaration), `src/probe/time_wiring.cpp` (`Counts` and `ridge_read` moved to the new
  `src/probe/time_read.hpp`, `ridge_read` no longer static), `src/probe/main.cpp` (one line
  and one declaration).
- New: `src/probe/loop_trace.cpp`, `src/probe/after_life.cpp`, `src/probe/time_read.hpp`,
  `scripts/loops.sh`, `scripts/loops.py`, `scripts/loops_probe.py`. Not touched: `fire.cpp`,
  `src/mnist/*`, `learn_hop.cpp`, `src/run/predict.*`, `scripts/ladder3*`, `timerule_lib.py`
  (only imported; `loops.py` adds arms to its table at run time).
