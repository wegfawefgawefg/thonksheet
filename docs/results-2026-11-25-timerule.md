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

## Short version

- **What decides: the delay, then the sheet.** On the suite's sheet the rule's gain on
  delayed echo is +0.33 at 2 steps, +0.07 at 3, +0.02 at 4 and nothing at 8 or 16 (8 seeds).
  The prediction battery asked at 4 steps, past that edge, where its rule-off arm was at the
  coin too. Its sheet also fails at 2 steps: sensor groups of side 2 together with the
  sensor lattice across the top half, where the rule pushes the rate from 29 to 50 (either
  change alone keeps two thirds of the gain or more).
- **The inside carries it, and it is not time.** After a life with the rule, a frozen sheet
  and a fresh readout get all of the gain (delayed echo 0.56 born, 0.92 lived; gap 0.54,
  0.95). A life on the same frames in shuffled order gives nearly as much (0.89, 0.96), so
  does a life with no reward (0.86, 0.92); churn gives nothing (0.56, 0.52).
- **What the cells wired: loops per cue.** The cells that hold a cue through the blanks take
  28 of their 49 excitatory inputs from other cells holding the same cue (born: 3), plus
  that cue's own sensor cells. Slot delays are unchanged (about 2 ticks; a step is 20).
  These are Hebbian cell assemblies: what a cue lights stays lit for about two steps. The
  compete2 guess ("this cue now and that one some ticks ago" through slot delays) is wrong.
- **How far:** 10 of 16 memory and time rows move by 0.05 or more, all of them ones a trace
  of one or two steps answers: 1-back 0.73 to 0.98 (the GRU 0.97), same-or-different in turn
  0.53 to 0.83, copy of 2 0.69 to 0.89, gap 0.54 to 0.94. Beat falls, sequence stays, delayed
  echo past 3 steps stays. With the output-gap readout most rows gain a little more and
  1-back loses 0.14 (its exclusive-or cost).

## What was built

- `scripts/timerule.sh NAME [--quick]` (`timerule.py`, `timerule_lib.py`, `timerule_probe.py`):
  every table below from one line. Lives are plain `blobrun` lives on one of two sheets:
  - *suite*: `scripts/suite.py`'s standard life (`--ticks 20 --sensor-side 3 --motor-side 3`,
    40,000 steps) with the scoreboard's blob-readout readout (`--g.learn_mode 5
    --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128`)
    and the tile's winners in every arm, as in compete2's time rows;
  - *pred*: `scripts/predict.py`'s sheet (`--sensor-side 2 --motor-spread 64 --g.sensor_w 64
    --g.motor_stride 1`, 20,000 steps), the same readout, the tile's winners only with the
    rule, delayed echo at 4 steps.
  Arms: off (born sheet, readout learns), on (+ `--g.compete_on 1 --g.compete_shift 5`), churn
  (+ `--g.compete_churn 1`), oncap (on, `--g.threshold_cap 1000`).
- `blobprobe transfer` (`src/probe/lived_sheet.cpp`) gains `--life-quiet 1` (a reward-free
  life: good and bad worked out but withheld, as the shown protocol does) and `--then-keep 1`
  (the inside frozen after the life but the life's readout kept and learning on: the rule on
  for the first half of one life, then off). Its `--life-scramble 1` (the hop job's shuffled
  input: each step shows the lines of a step drawn from the last 256) is the time-shuffled
  life. Both default off.
- `blobprobe timewire` (`src/probe/time_wiring.cpp`): lives the life, stops every rule, shows
  delayed echo again (fresh task seed, 2,000 trials) and counts each cell's spikes per step.
  Reports a fresh ridge reader of the cue from all hidden cells and from those the readout
  can hear (the 3x3 tiles round a button's cell), at each step of the trial; each cell's d'
  for the cue per step and over the blanks ("carriers": |d'| >= 0.5); and where cells' inputs
  come from (the cue line they prefer or the other, the ask line, other lines, motor cells,
  hidden cells carrying the same cue, the other cue, neither, inhibitory) with each class's
  mean slot delay. `--out` writes a row per cell so the same cells can be followed across
  sheets.
- No genome numbers, no core changes, no blobrun flags.

## 1. Both numbers reproduce

`scripts/timerule.sh repro` (`out/timerule/repro.md`), seeds 5 and 6, late score per seed:

| row | arm | 5 | 6 | rate | learning off |
|---|---|---|---|---|---|
| suite delayed echo, 2 steps | off | 0.63 | 0.60 | 31 | 0.484 |
| | on | 0.98 | 0.95 | 34 | |
| suite order | off | 0.61 | 0.70 | 32 | 0.471 |
| | on | 0.72 | 0.63 | 41 | |
| suite gap | off | 0.56 | 0.57 | 31 | 0.465 |
| | on | 0.92 | 0.85 | 44 | |
| pred delayed echo, 4 steps | off | 0.46 | 0.51 | 29 | 0.496 |
| | on | 0.47 | 0.48 | 43 | |

Every number is the one in `out/from-agents/compete2/time.txt` and the prediction battery's
raw runs. On 8 seeds (`walk8`, below) the suite's delayed echo is again 0.587 off, 0.913 on,
0.874 on at target rate, 0.537 churn: compete2's numbers exactly.

## 2. What decides

`scripts/timerule.sh walk` (seeds 5-8, `out/timerule/walk.md`) and `scripts/timerule.sh
walk8` (seeds 5-12, `out/timerule/walk8.md`). Delayed echo, off / on / churn, late score with
the rate in brackets. "suite" is compete2's setting; "pred" the prediction battery's. Each
row changes one thing (two in the last three).

| row (seeds 5-8) | off | on | churn | on - off | seeds up |
|---|---|---|---|---|---|
| suite (side 3, std lattice, spread 128, 2 steps, tile winners in off, 40k) | 0.582 (31) | 0.962 (34) | 0.545 (31) | +0.380 | 4/4 |
| suite, sensor side 2 | 0.533 (31) | 0.740 (41) | 0.498 (31) | +0.207 | 3/4 |
| suite, pred lattice (`--g.sensor_w 64 --g.motor_stride 1`) | 0.551 (29) | 0.834 (35) | 0.526 (29) | +0.282 | 4/4 |
| suite, spread 64 | 0.567 (31) | 0.825 (34) | 0.535 (31) | +0.258 | 4/4 |
| **suite, 4 steps** | 0.477 (31) | 0.499 (31) | 0.481 (31) | **+0.021** | 4/4 |
| suite, no tile winners in off | 0.543 (31) | 0.962 (34) | 0.545 (31) | +0.418 | 4/4 |
| suite, 20,000 steps | 0.561 (31) | 0.957 (33) | 0.527 (31) | +0.395 | 4/4 |
| suite, 8 steps | 0.476 (30) | 0.481 (31) | 0.491 (30) | +0.005 | 2/4 |
| suite, 16 steps | 0.500 (30) | 0.525 (38) | 0.491 (30) | +0.025 | 3/4 |
| pred (side 2, pred lattice, spread 64, 4 steps, no tile winners in off, 20k) | 0.489 (29) | 0.495 (44) | 0.482 (28) | +0.006 | 2/4 |
| pred, side 3 | 0.503 (29) | 0.491 (31) | 0.493 (29) | -0.012 | 1/4 |
| pred, std lattice | 0.505 (31) | 0.497 (31) | 0.465 (30) | -0.008 | 1/4 |
| pred, spread 128 | 0.498 (29) | 0.487 (46) | 0.468 (28) | -0.012 | 1/4 |
| **pred, 2 steps** | 0.517 (29) | 0.489 (51) | 0.484 (29) | **-0.028** | 1/4 |
| pred, tile winners in off | 0.485 (28) | 0.495 (44) | 0.482 (28) | +0.010 | 3/4 |
| pred, 40,000 steps | 0.464 (29) | 0.467 (51) | 0.468 (28) | +0.003 | 2/4 |
| pred, 8 steps | 0.469 (28) | 0.461 (43) | 0.478 (28) | -0.007 | 1/4 |
| pred, 16 steps | 0.461 (28) | 0.468 (52) | 0.460 (28) | +0.007 | 3/4 |
| pred at 2 steps, side 3 | 0.553 (29) | 0.802 (35) | 0.542 (29) | +0.249 | 4/4 |
| pred at 2 steps, std lattice | 0.537 (31) | 0.748 (32) | 0.488 (31) | +0.210 | 4/4 |
| pred at 2 steps, spread 128 | 0.496 (29) | 0.502 (50) | 0.508 (29) | +0.006 | 2/4 |
| pred at 2 steps, tile winners in off | 0.528 (29) | 0.489 (51) | 0.484 (29) | -0.040 | 1/4 |
| pred at 2 steps, 40,000 steps | 0.517 (29) | 0.477 (50) | 0.489 (29) | -0.041 | 0/4 |
| **suite, side 2 + pred lattice** | 0.485 (29) | 0.480 (50) | 0.489 (29) | **-0.005** | 1/4 |
| suite, side 2 + spread 64 | 0.515 (31) | 0.762 (37) | 0.504 (31) | +0.247 | 4/4 |
| suite, pred lattice + spread 64 | 0.578 (29) | 0.829 (34) | 0.552 (29) | +0.251 | 4/4 |

The cells that carry the conclusion, seeds 5-12, with the rate held at target beside:

| row (seeds 5-12) | off | on | churn | on, cap | on - off | seeds up |
|---|---|---|---|---|---|---|
| suite, 2 steps | 0.587 (31) | 0.913 (34) | 0.537 (31) | 0.874 (31) | +0.326 | 8/8 |
| suite, 3 steps | 0.498 (31) | 0.572 (32) | 0.481 (31) | 0.657 (31) | +0.074 | 8/8 |
| suite, 4 steps | 0.480 (31) | 0.499 (32) | 0.480 (31) | 0.486 (31) | +0.019 | 7/8 |
| pred, 4 steps | 0.479 (28) | 0.480 (43) | 0.471 (28) | 0.486 (28) | +0.001 | 4/8 |
| pred, 2 steps | 0.511 (29) | 0.480 (50) | 0.482 (29) | 0.594 (28) | -0.030 | 1/8 |
| pred at 2 steps, side 3 | 0.533 (29) | 0.823 (35) | 0.523 (29) | 0.773 (29) | +0.290 | 8/8 |
| suite, side 2 + pred lattice | 0.491 (29) | 0.473 (50) | 0.486 (29) | 0.618 (29) | -0.018 | 1/8 |

**The answer: the delay decides first (the gain is +0.33 at 2 steps, +0.07 at 3, +0.02 at 4
and nothing beyond, on the suite's own sheet), and the prediction battery's sheet fails a
second, separate way even at 2 steps: sensor groups of side 2 together with the sensor
lattice across the top half, where the rule pushes the firing rate from 29 to 50 and the
gain is gone (held at target rate, a part of it comes back: 0.59 and 0.62 against 0.51
and 0.49).** The tile's winners in the off arm, the motor spread and the life length do
not matter; either shape change alone costs a third of the gain at most.

## 3. Is it time? The freeze, shuffled, reward-free, churn and rate arms

`scripts/timerule.sh controls` (`out/timerule/controls.md`), seeds 5-12, the suite's sheet.
These arms freeze the inside part way: they are measurements of what the rule left in the
cells, not a method (the method stays the one life with every rule on). "One life" is the
plain blobrun life of 40,000 steps. Transfer is `blobprobe transfer`: a life of 20,000 steps,
then every inner rule stopped, the motor cells put back as born and a fresh readout learning
the task (fresh task seed) for 20,000 steps; its late score.

- born: no life. lived: the rule on, the task in order, rewarded.
- shuffled: the same life, but each step shows the lines of a step drawn at random from the
  last 256 (`--life-scramble 1`): the same frames, the time order gone.
- quiet: the rule on, no good or bad reaching the sheet (`--life-quiet 1`).
- churn: the rule's moves toward random sources.
- running: the rule left on after the life, beside the fresh readout (`--then-inside 1`).
- kept: the rule on for the first half, then off, the same readout learning throughout
  (`--then-keep 1`).
- -cap: the rate held at target (`--g.threshold_cap 1000`) in life and read.

| row | one life: off | on | churn | on, cap | born | lived | shuffled | quiet | churn | running | kept | born, cap | lived, cap | shuffled, cap |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| delayed echo, 2 steps | 0.587 | 0.913 | 0.537 | 0.874 | 0.560 | 0.920 | 0.889 | 0.860 | 0.557 | 0.925 | 0.924 | 0.575 | 0.880 | 0.785 |
| gap | 0.544 | 0.938 | 0.503 | 0.929 | 0.537 | 0.952 | 0.959 | 0.917 | 0.523 | 0.929 | 0.960 | 0.530 | 0.934 | 0.845 |
| order | 0.664 | 0.728 | 0.606 | 0.809 | 0.657 | 0.705 | 0.808 | 0.744 | 0.649 | 0.703 | 0.734 | 0.667 | 0.817 | 0.689 |

Paired against born (transfer; the cap arms against born at cap), mean and seeds above:

| row | lived | shuffled | quiet | churn | running | kept | lived, cap | shuffled, cap |
|---|---|---|---|---|---|---|---|---|
| delayed echo, 2 steps | +0.360 (8/8) | +0.330 (8/8) | +0.300 (8/8) | -0.002 (3/8) | +0.366 (8/8) | +0.364 (8/8) | +0.305 (8/8) | +0.211 (8/8) |
| gap | +0.415 (8/8) | +0.422 (8/8) | +0.379 (8/8) | -0.014 (3/8) | +0.392 (8/8) | +0.422 (8/8) | +0.404 (8/8) | +0.316 (8/8) |
| order | +0.048 (6/8) | +0.151 (8/8) | +0.087 (8/8) | -0.007 (3/8) | +0.046 (5/8) | +0.077 (7/8) | +0.150 (7/8) | +0.021 (5/8) |

- **The inside carries it.** After the life the rule can stop: a fresh readout on the frozen
  sheet gets all of the gain (delayed echo 0.920 frozen against 0.925 with the rule still on;
  gap 0.952 against 0.929). With the rule on for half a life and then off, the readout ends
  where the whole-life rule does (0.924, 0.960).
- **It is not the task's time order.** A life on the same frames in shuffled order builds
  nearly all of it (+0.33 and +0.42 against +0.36 and +0.42; at target rate two thirds:
  +0.21 and +0.32). Nothing about "cue, then 2 blanks, then ask" has to be seen: what the
  rule uses is which cells are on together inside a step.
- **No reward is needed.** The quiet life gives +0.30 and +0.38.
- **Churn gives nothing**, and the rate is not it (the cap arms keep most of the gain).
- Order moves little on the whole life (+0.06; at target rate +0.15) and its shuffled life
  does better than its real one: the gain there is small and not about order.

## 4. What the cells wired

`scripts/timerule.sh wiring` (seeds 5-8, `out/timerule/wiring.md`; ran on stevejobs). After
the life every rule is stopped and delayed echo shown again for 2,000 trials (fresh task
seed). A fresh ridge reader of the cue from the hidden cells, test trials, at each step of
the trial (cue / blank / ... / ask; the ask step also hears the press, which carries the
answer once the readout has it), and the number of hidden cells with |d'| >= 0.5 for the cue
at that step. "near": only the hidden cells in the 3x3 tiles round a button's cell, which is
all the readout can hear.

| sheet | read by step, all hidden | near the buttons | cells with \|d'\| >= 0.5 |
|---|---|---|---|
| suite 2 steps, born | 1.000 / 1.000 / 0.967 / 0.722 | 1.000 / 1.000 / 0.957 / 0.692 | 451 / 388 / 30 / 0 |
| suite 2 steps, rule on | 1.000 / 1.000 / 1.000 / 0.998 | 1.000 / 1.000 / 0.999 / 0.993 | 599 / 768 / 418 / 360 |
| suite 2 steps, churn | 1.000 / 1.000 / 0.962 / 0.697 | 1.000 / 1.000 / 0.940 / 0.677 | 417 / 406 / 33 / 0 |
| suite 2 steps, shuffled life | 1.000 / 1.000 / 1.000 / 0.994 | 1.000 / 1.000 / 0.996 / 0.992 | 598 / 642 / 379 / 590 |
| suite 2 steps, quiet life | 1.000 / 1.000 / 0.994 / 0.987 | 1.000 / 1.000 / 0.988 / 0.976 | 564 / 630 / 194 / 172 |
| suite 4 steps, born | 1.000 / 1.000 / 0.972 / 0.728 / 0.550 / 0.485 | 1.000 / 0.998 / 0.956 / 0.708 / 0.527 / 0.510 | 444 / 388 / 34 / 0 / 0 / 0 |
| suite 4 steps, rule on | 1.000 / 1.000 / 0.950 / 0.733 / 0.557 / 0.538 | 1.000 / 1.000 / 0.941 / 0.740 / 0.584 / 0.559 | 258 / 261 / 80 / 2 / 0 / 0 |
| pred 4 steps, born | 1.000 / 0.999 / 0.848 / 0.609 / 0.525 / 0.516 | 1.000 / 0.995 / 0.812 / 0.607 / 0.522 / 0.524 | 178 / 135 / 5 / 0 / 0 / 0 |
| pred 4 steps, rule on | 1.000 / 1.000 / 0.613 / 0.525 / 0.499 / 0.486 | 1.000 / 0.999 / 0.559 / 0.516 / 0.495 / 0.512 | 117 / 54 / 2 / 0 / 0 / 0 |
| pred 2 steps, born | 1.000 / 0.999 / 0.825 / 0.615 | 1.000 / 0.998 / 0.774 / 0.597 | 172 / 124 / 3 / 0 |
| pred 2 steps, rule on | 1.000 / 1.000 / 0.589 / 0.519 | 1.000 / 1.000 / 0.591 / 0.504 | 210 / 152 / 0 / 0 |

Where the connections of the cells that carry the cue through the blanks come from
(connections per cell; carriers: |d'| >= 0.5 over the blanks together, each by its own
preference; cue+ / cue-: sensor cells of the cue line it prefers / the other; carry+ /
carry-: hidden carriers of the same / the other cue; hidden: other hidden cells):

| sheet | cue+ | cue- | ask | other lines | motor | carry+ | carry- | hidden | inhibitory |
|---|---|---|---|---|---|---|---|---|---|
| suite 2 steps, born | 0.91 | 0.39 | 0.34 | 7.02 | 4.38 | 3.06 | 2.29 | 35.58 | 14.99 |
| suite 2 steps, rule on | 2.27 | 0.08 | 0.15 | 1.43 | 2.40 | **27.85** | 2.49 | 12.29 | 14.18 |
| suite 2 steps, churn | 0.90 | 0.44 | 0.35 | 8.29 | 3.74 | 2.96 | 2.23 | 35.44 | 14.47 |
| suite 2 steps, shuffled life | 2.41 | 0.11 | 0.17 | 1.67 | 2.28 | **30.01** | 2.46 | 11.56 | 13.98 |
| suite 2 steps, quiet life | 3.06 | 0.13 | 0.20 | 2.18 | 2.21 | **25.70** | 2.41 | 15.19 | 13.97 |
| suite 4 steps, rule on | 6.53 | 0.40 | 0.13 | 1.04 | 3.11 | **22.09** | 1.54 | 18.84 | 14.76 |
| pred 4 steps, rule on | 3.38 | 0.08 | 0.53 | 1.53 | 5.05 | 5.56 | 0.14 | 29.36 | 13.03 |
| pred 2 steps, rule on | 2.79 | 0.12 | 0.45 | 2.93 | 3.80 | 15.41 | 1.26 | 22.50 | 12.59 |

The cells a fresh reader of the last blank leans on most (top 64 by weight) in the lived
sheet, and the same cells in the born, churned and time-shuffled sheets of the same seed:

| same cells, in | cue+ | carry+ | carry- | hidden | motor | inhibitory | \|carry d'\| |
|---|---|---|---|---|---|---|---|
| lived (rule on) | 0.22 | 10.49 | 7.80 | 12.79 | 5.05 | 12.47 | 1.74 |
| born | 0.21 | 1.38 | 1.39 | 27.23 | 5.63 | 12.47 | 0.18 |
| churn | 0.21 | 1.28 | 1.26 | 27.30 | 5.71 | 12.47 | 0.19 |
| shuffled life | 0.23 | 8.89 | 7.35 | 14.93 | 4.93 | 12.47 | 0.98 |

Mean slot delay of the carriers' inputs is 1.9 to 2.2 ticks for every class and every sheet
(table in `out/timerule/wiring.md`); a step is 20 ticks and the longest delay a slot can
have is 15. No cell sees "the cue now and the cue some steps ago" through its own slots.

What the cells wired, plainly:

- **Not time detectors. Cell assemblies that hold the cue.** The compete2 guess was wrong.
  After a life with the rule, the hidden cells that carry a cue through the blanks take 28 of
  their 49 excitatory connections from other carriers of the *same* cue and 2.5 from
  carriers of the other (born: 3.1 and 2.3), plus the sensor cells of their own cue (2.3
  against 0.08). That is a self-exciting loop per cue: what a cue lights stays lit after
  it goes off. At 2 steps it lasts to the ask (360 cells still carry the cue there, born
  none) and the cue is read at 0.99 to 1.00 by any reader, the readout's neighbours
  included. Classic Hebbian assemblies (Hebb 1949; the reverberating loops of attractor
  working memory), made here by "move toward what was on when you fired".
- **They hold for about two steps.** At 4 steps the rule builds the same loops (22 against
  1.5) but the cue has faded by the third blank on the lived sheet as on the born one
  (0.557 against 0.550). That is the delay cutoff of point 2: the loops prolong the trace,
  they do not latch.
- **On the prediction sheet the loops do not form strongly enough.** 0 to 2 cells carry the
  cue past the first blank with the rule; the readable trace is *shorter* than born (0.589
  against 0.825 at the second blank). The rate climbs to 50 and the extra firing is not
  cue-specific. That is finding 50's "time since the cue read worse" on that sheet.
- **They come from co-activity, not order.** The shuffled life wires the same loops (30
  against 2.5) in the same cells (the reader's top cells carry at |d'| 0.98 after a shuffled
  life, 1.74 after the real one, 0.18 as born).

## 5. How far it goes

`scripts/timerule.sh reach` (`out/timerule/reach.md`), seeds 5-12, the suite's sheet and life
(the setting that works), rule off / on / churn / on at target rate. Late score, rate in
brackets; the GRU reference is `docs/scoreboard.md`'s gru-fixed column where it has the row.

| row | off | on | churn | on, cap | on - off | seeds up | GRU |
|---|---|---|---|---|---|---|---|
| delayed echo, 2 steps | 0.587 (31) | **0.913** (34) | 0.537 (31) | 0.874 (31) | +0.326 | 8/8 | 1.00 |
| delayed echo, 3 steps | 0.498 (31) | 0.572 (32) | 0.481 (31) | 0.657 (31) | +0.074 | 8/8 | - |
| delayed echo, 4 steps | 0.480 (31) | 0.499 (32) | 0.480 (31) | 0.486 (31) | +0.019 | 7/8 | - |
| delayed echo, 8 steps | 0.487 (30) | 0.482 (31) | 0.492 (30) | 0.479 (31) | -0.004 | 3/8 | - |
| delayed echo, 16 steps | 0.494 (30) | 0.517 (44) | 0.484 (30) | 0.494 (31) | +0.024 | 6/8 | - |
| beat, every 4 steps | 0.479 (31) | 0.451 (58) | 0.482 (30) | 0.480 (31) | -0.028 | 2/8 | 1.00 |
| sequence, 8 lines x 4 | 0.569 (40) | 0.589 (58) | 0.572 (40) | 0.572 (39) | +0.021 | 8/8 | 0.50 |
| copy 2 of 2 | 0.688 (32) | **0.894** (39) | 0.596 (32) | 0.854 (32) | +0.206 | 8/8 | 1.00 |
| copy 2 to 3 of 2 | 0.526 (32) | 0.626 (40) | 0.499 (32) | 0.589 (32) | +0.100 | 8/8 | - |
| recall, 1 between | 0.698 (32) | 0.799 (37) | 0.611 (32) | **0.865** (32) | +0.101 | 7/8 | 1.00 |
| recall, 2 between | 0.510 (32) | 0.532 (38) | 0.496 (32) | **0.682** (33) | +0.022 | 7/8 | - |
| 1-back, 4 symbols | 0.726 (33) | **0.978** (40) | 0.591 (33) | 0.864 (33) | +0.252 | 8/8 | 0.97 |
| 2-back, 4 symbols | 0.501 (33) | 0.611 (39) | 0.483 (33) | 0.553 (33) | +0.111 | 8/8 | 0.94 |
| same or different in turn | 0.531 (32) | **0.825** (34) | 0.509 (32) | 0.782 (32) | +0.294 | 8/8 | 0.96 |
| order | 0.664 (32) | 0.728 (40) | 0.606 (32) | 0.809 (32) | +0.064 | 5/8 | 1.00 |
| gap | 0.544 (31) | **0.938** (39) | 0.503 (31) | 0.929 (31) | +0.394 | 8/8 | 1.00 |

- **10 of 16 rows move by 0.05 or more** (9 of the 15 asked for; 3 steps added), every one
  of them on 7 or 8 seeds of 8 except order (5/8). Churn is under off on 13 of 16.
- **The rows that move are the ones a one-to-two-step trace answers**: what was there one
  or two steps ago (1-back 0.73 to 0.98, the GRU's 0.97; same-or-different in turn 0.53 to
  0.83; copy of 2; gap; delayed echo at 2). Where the answer is three or more steps back
  (delayed echo at 4, 8, 16; recall with 2 between, except at target rate; 2-back only to
  0.61) the gain shrinks or goes.
- **Rhythm does not move**: beat falls (0.48 to 0.45, rate 31 to 58) and sequence stays at
  its baseline. A loop that holds what was on carries no phase.
- At target rate recall gains more (1 between 0.87, 2 between 0.68 against 0.51), 1-back
  and copy a little less. The cap is not tuned here.

## 6. With the output-gap readout

`scripts/timerule.sh readout` (`out/timerule/readout.md`), seeds 5-12, the rule on, the readout
`--g.readout_evidence 2 --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2` (one arm,
as asked; seeds 9-12 ran on stevejobs). Beside it the working readout off and on from point 5.

| row | off | on | on, new readout | new - on | seeds up | GRU |
|---|---|---|---|---|---|---|
| delayed echo, 2 steps | 0.587 | 0.913 | **0.937** | +0.024 | 5/8 | 1.00 |
| delayed echo, 3 steps | 0.498 | 0.572 | 0.669 | +0.097 | 6/8 | - |
| delayed echo, 4 steps | 0.480 | 0.499 | 0.518 | +0.019 | 5/8 | - |
| delayed echo, 8 steps | 0.487 | 0.482 | 0.515 | +0.032 | 6/8 | - |
| delayed echo, 16 steps | 0.494 | 0.517 | 0.496 | -0.021 | 4/8 | - |
| beat, every 4 steps | 0.479 | 0.451 | 0.434 | -0.017 | 0/8 | 1.00 |
| sequence, 8 lines x 4 | 0.569 | 0.589 | 0.589 | +0.000 | 0/8 | 0.50 |
| copy 2 of 2 | 0.688 | 0.894 | **0.920** | +0.026 | 5/8 | 1.00 |
| copy 2 to 3 of 2 | 0.526 | 0.626 | 0.733 | +0.107 | 8/8 | - |
| recall, 1 between | 0.698 | 0.799 | 0.801 | +0.002 | 4/8 | 1.00 |
| recall, 2 between | 0.510 | 0.532 | 0.550 | +0.017 | 6/8 | - |
| 1-back, 4 symbols | 0.726 | **0.978** | 0.838 | -0.140 | 0/8 | 0.97 |
| 2-back, 4 symbols | 0.501 | 0.611 | 0.646 | +0.035 | 7/8 | 0.94 |
| same or different in turn | 0.531 | 0.825 | **0.862** | +0.036 | 6/8 | 0.96 |
| order | 0.664 | 0.728 | 0.784 | +0.056 | 5/8 | 1.00 |
| gap | 0.544 | 0.938 | **0.946** | +0.008 | 4/8 | 1.00 |

- Small gains on most rows (copy of up to 3 +0.11, delayed echo at 3 steps +0.10, order
  +0.06), one large loss: 1-back 0.978 to 0.838 on every seed. 1-back is "same as the last
  one or not", an exclusive-or of two items, which is what the output-gap job said this
  readout costs with two buttons.
- With either readout the rule leaves the GRU's numbers out of reach except on 1-back
  (0.978 against 0.97) and gap (0.946 against 1.00).

## What is real, what is suspect

Real (8 seeds, every number from a script line below):

- Delayed echo's gain is bounded by the delay: +0.33 at 2 steps, +0.07 at 3, +0.02 at 4,
  nothing at 8 and 16, on the sheet where the rule works best. The prediction battery tested
  at 4 steps, past the edge, and its off arm was at the coin there too.
- The prediction battery's sheet fails a second way: sensor side 2 with the top-half sensor
  lattice. Either alone keeps two thirds of the gain or more; together the rule doubles the
  firing rate and the gain is gone (part of it back at target rate).
- The inside carries the gain: a frozen sheet after a life with the rule gives a fresh
  readout all of it; the rule need not keep running.
- It is not the task's time order and not the reward: a time-shuffled life and a quiet life
  give nearly the same gain. Churn gives none. The rate is not it.
- What the cells wired: cue-specific self-exciting loops (carriers take 28 of 49 excitatory
  inputs from same-cue carriers against 3 as born), in the same cells after a shuffled life.
  Slot delays do not change. Finding 50 holds on the prediction sheet (the rule shortens the
  trace there) and is false on the suite's sheet.
- Ten of sixteen memory and time rows move by 0.05 or more: the ones a trace of one or two
  steps answers (1-back to 0.978, the GRU's 0.97). Rhythm rows do not move.

Suspect:

- "Time feature" is the wrong name for it. What the rule makes is a longer trace of what was
  on, a cell-assembly memory, learned from which lines are on together. It knows nothing of
  order or intervals beyond what a fading trace carries; gap and order are answered by how
  the trace of one blink meets the next.
- The trace lasts about two steps of 20 ticks. Whether that is a fixed property of these
  loops (fatigue, the threshold's handicap, inhibition) or a number one setting moves was not
  looked at: nothing was tuned.
- The wiring report is 4 seeds and one world (delayed echo, k 2). The "carrier" cut (|d'| >=
  0.5 over the blanks) is a choice; the top-cell table follows cells chosen by one reader.
- The whole-sheet ridge reader reads the cue perfectly at the first blank even on the born
  sheet (2,000 trials, ~3,600 cells): the difference is in how long it lasts, not in whether
  it is there. The near reader (what the readout can hear) behaves the same, so the readout's
  reach is not what limits these rows.
- Point 6's readout is the plain gate on these two-button rows, with its known exclusive-or
  cost; the 1-back loss is that cost, not something about the rule.
- The 3-step row was added beside the asked delays (2, 4, 8, 16), and the 4 settings from
  "pred at 2 steps" on are extra walk rows; nothing else was added or tuned.

## Not done

- Why the loops fade after about two steps, and whether anything (a slower threshold
  handicap, fatigue off on carriers, longer life) lengthens them. No tuning was asked.
- Why side 2 with the wide sensor lattice makes the rule run the rate up: not looked at
  beyond the cap arm.
- The wiring report on gap, order, 1-back (only delayed echo is supported by `timewire`).
- A shuffled-in-time control for a life the readout learns on at the same time (the
  shuffled arm here freezes the inside after a shuffled life).
- The rest of the suite (and the scoreboard) with the rule on, as compete2 left it.

## Predictions, scored

1. What decides. Delay alone (0.4): half right; the delay decides on the suite's sheet, but
   the prediction sheet at 2 steps lacks the gain too. Shape (0.35): half right the other
   way. Life length (0.1), tile winners in the off arm (0.05): no. "Something else or a mix"
   (0.1) is the outcome: delay and the side-2-plus-wide-lattice shape, each enough alone.
2. The frozen sheet carries at least half the gain (0.55): yes, all of it.
3. Gain under 0.05 by 4 steps (0.3): yes, +0.019 (+0.074 at 3). Still 0.05 or more at 16
   (0.1): no.
4. The time-shuffled life within 0.05 of the real-order life (0.5): yes on delayed echo
   (-0.030) and gap (+0.007); order is outside it the other way (+0.10).
5. Six of the fifteen rows move by 0.05 or more, 4 to 8 at 0.6: wrong, 9 of the 15 asked for
   (10 of 16 with 3 steps).

## Script lines

```
scripts/timerule.sh repro            section 1 (16 runs)
scripts/timerule.sh walk             section 2, seeds 5-8 (312 runs)
scripts/timerule.sh walk8            section 2, the deciding cells on seeds 5-12
scripts/timerule.sh controls         section 3 (96 lives, 240 transfers)
scripts/timerule.sh wiring           section 4, seeds 5-8 (44 probes; ran on stevejobs)
scripts/timerule.sh reach            section 5 (512 lives, those of section 3 reused)
scripts/timerule.sh readout          section 6 (128 lives; seeds 9-12 on stevejobs with
                                     --seeds 9-12 --arms goodon)
scripts/timerule.sh all --quick      smoke check, 2 seeds, short lives (ran end to end)
scripts/timerule.sh reach --table-only   print a table from what is there
bash out/timerule/refcheck.sh        the four reference lives against out/merge-ref: same
```

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md` 50: on the prediction battery's sheet (side 2, wide sensor lattice) the
  rule shortens the trace (true); on the suite's sheet it lengthens it to about two steps.
  The difference is the shape and the delay. Replace "costs them the recent past" with
  "builds cue loops that hold what was on for about two steps, where the sheet's shape lets
  them form".
- New finding (likely, 8 seeds, controls): **reward-free competitive learning builds cell
  assemblies that hold a cue for about two steps.** It is learned from co-activity (a
  time-shuffled life gives it), stays when the rule stops, needs no reward, and moves 10 of
  16 memory rows (1-back 0.73 to 0.98). It is not a time feature: past three steps and on
  rhythm rows it does nothing.
- `docs/ideas.md` 166: done; next questions are the fade (what sets two steps) and a
  latch (a loop that does not fade until the ask).
- `docs/predictions.md`: the five above.

## Files that may collide with other branches

- `src/probe/lived_sheet.cpp` (`parse_life` no longer static; `live()` takes a quiet flag;
  `--life-quiet`, `--then-keep`; header comment), `src/probe/main.cpp` (one line and one
  declaration), new `src/probe/time_wiring.cpp`.
- New `scripts/timerule.sh`, `timerule.py`, `timerule_lib.py`, `timerule_probe.py`. None of
  `learn_hop.cpp`, `src/run/predict.*`, `scripts/predict.*`, `src/mnist/*` was touched.
