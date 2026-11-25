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
