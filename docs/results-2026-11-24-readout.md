# A readout that can read what is there (idea 58, 2026-11-24)

The feature probe found order, exclusive-or and gap readable from the tiles next to the
input at birth, while a frozen sheet whose readout learns scored chance on the same tasks.
This job looks for a local, online rule at the motor cells, on a frozen sheet, that gets
as close as possible to what a plain linear reader can get.

## Short version

- **What a plain linear reader gets** (ridge fit to every neuron's spike count over the
  scored step, frozen sheet, task timing, held-out trials): echo at 1-3 tiles and echo k=4
  1.00; both-not-either 0.97; order 0.90 (0.82 from the tiles a packed button can hear);
  gap only 0.63. With delays of 4 to 8 ticks (seed 5): order 0.97, gap 0.80.
- **The rule that gets closest**, on a frozen sheet: an error rule at the motor cells (on
  bad, the pressed button's cells lose their recently active inputs and the other buttons'
  cells gain them), told by the world which cells belong to the pressed button, moving
  **hidden evidence counters** instead of connections (each cell wires the top of its born
  budget), **balanced** so the other buttons move less when there are more of them, on
  **128 motor cells per button** spread over the motor region. Seeds 5-12, against today's
  readout and the learning-off twin:

  | task | today's readout | best rule | its twin | reference |
  |---|---|---|---|---|
  | echo k=2, 1 tile apart | 0.79 | 1.00 | 0.52 | 1.00 |
  | echo k=2, 2 tiles apart | 0.44 | 0.91 | 0.48 | 1.00 |
  | echo k=2, 3 tiles apart | 0.48 | 0.85 | 0.48 | 1.00 |
  | order | 0.44 | 0.70 (0.84 with delays 4-8) | 0.48 | 0.90 (0.97) |
  | both-not-either | 0.46 | 0.88 | 0.49 | 0.97 |
  | gap | 0.47 | 0.52 (0.62 with delays 4-8) | 0.49 | 0.63 (0.80) |
  | echo k=4 | 0.20 | 1.00 | 0.25 | 1.00 |

  Reference: the whole-sheet linear reader (with delays in brackets, seed 5). It closes
  most of the gap on echo at a distance, exclusive-or and echo k=4, about half
  of it on order, and nothing on gap without delays (where there is little to read).
- **Each piece, measured**: the error rule alone is the first step (order 0.53, xor 0.57,
  echo 2 tiles 0.81 where today's readout gets 0.44 to 0.46); evidence counters are the
  second (xor 0.57 to 0.79, echo 3 tiles 0.57 to 0.74, packed); spreading the motor cells is
  the third (xor 0.79 to 0.88, order 0.58 to 0.70) but only for a rule that can use it:
  today's readout gains nothing from it. Balance fixes echo k=4 (0.49 to 1.00). Delays add
  0.14 on order and 0.10 on gap.
- **What did not help**: a cell that knows only whether it fired itself, instead of being
  told the pressed button (barely above today's readout); a budget on the plain error rule
  (worse on 5 of 7 tasks); far slots for the motor cells (nothing); centring the update
  against the last window (worse); a move on good as well as on bad (worse everywhere but
  unbalanced k=4); 8-bit counters instead of 6 (within noise; 4 bits is worse).
- **With the whole sheet learning** (the design rule everywhere plus the best rule at the
  motor cells) echo at a distance gets better still (2 tiles 0.99, 3 tiles 0.97, the full
  engine alone gets 0.51 and 0.51), but order and exclusive-or get worse (0.54 and 0.63
  against 0.70 and 0.88 frozen). The two fight on the feature tasks.
- **Side checks**: the best rule learns echo with 8 cues (0.84, chance 0.125; every engine
  so far was at chance) and shuffled echo with no locked lives (1.00 on 8 of 8).
- Predictions 16 and 17: both wrong as written (below).

## What was built

- **`blobprobe linear`** (`src/probe/linear_task.cpp`, `linear_fit.cpp`): the reference. It
  runs a real task through the plug with the task's own timing (20 ticks a step, 3x3
  groups), learning off, and keeps every non-sensor neuron's spike count over each scored
  step. A ridge least-squares reader (one output per button, largest wins, the ridge picked
  on held-back training trials) is fitted on the first three quarters of 4,000 trials and
  scored on the last quarter: share right, the task's own units. It reports the whole sheet,
  every tile alone, every 3x3 block of tiles (exactly what a motor cell in the middle tile
  can hear through its 9 local slots), and the union of the motor tiles' 3x3 blocks ("the
  motors' view"). The reader always answers, so chance is 0.50 (k=2) or 0.25 (k=4), not the
  0.44 coin flips get with ties. `blobprobe feature --linear 1` fits the same reader to the
  feature probe's own tries.
- **Learn mode 5** (`src/blob/learn_readout.cpp`): nothing inside the sheet learns; only
  connections into the buttons' motor cells move, by an error rule, the perceptron rule in
  bits. On bad, the cells of the button that was pressed lose their recently active inputs
  and the cells of the other button(s) gain those same inputs, one level by chance
  (2^-`readout_shift` per input per tick it fired, over the 15 ticks before the reward).
  Sign-aware: gaining strengthens excitatory sources and weakens inhibitory ones. Gains may
  wire in new sources. On good nothing moves (`readout_good 1`: on good the pressed button
  gains and the others lose). Learn mode 6 is the same at the motor cells plus the design
  rule everywhere, as in mode 0.
- **Which cells were pressed.** The blob cannot know this by itself, so the world tells it,
  one bit per cell, the way it writes `sense_in`: `motor_attached` marks the buttons' cells
  when the plug attaches, `motor_acted` marks the pressed button's cells just before the
  reward (`plug_mark_acted`, one line in `src/run/life.cpp`). It is an efference copy of
  the action taken. `readout_know_button 0` is the nearest purely local thing: a motor cell
  counts as "pressed" if it fired in the window, "other" if it stayed quiet.
- **Options**, each a genome number:
  - `readout_budget 1`: a cell keeps its count of connections; a new one costs one whose
    source was idle, a lost one is replaced by an idle source (item 2).
  - `readout_far 1`: every button cell gets both far slots drawn as at birth, two more
    tiles to listen to (item 4, pooling).
  - `readout_center 1`: a source moves by its spikes in this window less its spikes in the
    same ticks of the last rewarded window (a bit xor per tick), which is zero on average,
    so the activity every trial shares does not drive the update. The ridge reader centres
    its inputs; this was the guess for why the plain rule falls short.
  - `readout_evidence 1` (the coordinator's candidate 5, `src/blob/learn_evidence.cpp`):
    the rule moves a hidden signed counter per possible source of each motor cell (11 slots
    x 64 = 704 bytes per cell, clamped to +-31 at 6 bits) instead of the connection. After
    each update the cell wires the sources whose counters are highest, as many as it was
    born with, all at strength 4; nothing with a counter of 0 or less. Counters start at
    the born strengths and are touched only on reward. 256 motor cells x 704 bytes is
    180 KB; the tick is unchanged.
  - `readout_balance 1`: the other buttons' cells move with chance divided by their number
    over the pressed button's (rounded up to a power of two). With two buttons it changes
    nothing; with four, gains on the three other buttons otherwise outweigh the loss on the
    pressed one three to one and the counters run to the clamp.
  - `readout_counts 0`: one chance per source that fired at all in the window, not one per
    tick it fired; `readout_good 1`: on good the pressed button gains and the others lose.
- **`--motor-spread N`** (`src/world/plug_spread.cpp`): instead of one 3x3 group per
  button, each button gets N motor cells spread over the motor region. The region's motor
  lattice is dealt out like a checkerboard, cell (x, y) to button (x + y) mod buttons, so
  every tile holds cells of every button and the layout is the same whatever the cue is. The
  nearest N to the cues are used, or with `--motor-spread-even 1` N spaced from nearest to
  farthest; never nearer than `--distance` puts the packed buttons. 128 is the whole motor
  region (4 tile rows) for two buttons. It tells the sheet nothing about the answer.
- Off is byte-identical: order and echo `--frozen-readout` csvs, 3,000 steps, compared with
  `cmp` against a build of 66416e1.

## What is achievable: the plain linear reader

Share right on 1,000 held-out trials, frozen sheet (learning off), task timing, seeds 5 to
8 (seed-to-seed spread is small; 9 to 12 were not run for lack of machine time). Chance
is 0.50 (0.25 for echo k=4).

| task | whole sheet | motors' view (packed buttons) | best 3x3 block | best single tile |
|---|---|---|---|---|
| echo k=2, 1 tile apart | 1.000 | 1.000 | 1.000 | 1.000 |
| echo k=2, 2 tiles apart | 1.000 | 1.000 | 1.000 | 1.000 |
| echo k=2, 3 tiles apart | 1.000 | 0.911 ±0.013 | 1.000 | 1.000 |
| order | 0.905 ±0.022 | 0.825 ±0.011 | 0.845 ±0.022 | 0.726 ±0.017 |
| both-not-either | 0.972 ±0.009 | 0.982 ±0.006 | 0.991 ±0.003 | 0.875 ±0.012 |
| gap | 0.631 ±0.030 | 0.606 ±0.021 | 0.635 ±0.022 | 0.595 ±0.007 |
| echo k=4 | 1.000 | 1.000 | 1.000 | 0.998 ±0.002 |

(The whole-sheet reader fits 4,096 counts from 2,400 trials and overfits a little, which is
why a 3x3 block can beat it.)

The same reader on the feature probe's own tries (`blobprobe feature --linear 1`, seed 1,
as born): which 1.00, order 1.00, exclusive-or 0.99, gap 0.95 for the whole sheet; the best
3x3 block gets 1.00 on all four. The probe's timing is easier than the tasks': order is
read right after the second cue goes off, and gap is 3 against 9 ticks, not 1 against 3
steps of 20 ticks.

What it says:

- **Order and exclusive-or are well within reach of a linear reader at the task's own
  timing**, from exactly the tiles a packed button's motor cells can hear: 0.82 for order,
  0.98 for exclusive-or. So these are fair targets for a rule at the motor cells.
- **Gap at the task's timing is barely there.** 0.63 for the whole sheet, 0.61 from the
  motors' view. Two blinks 20 or 60 ticks apart, read 20 ticks after the second, leave
  almost nothing a linear reader can use. No readout rule should be expected to do much
  better than 0.6 on the suite's gap row; the feature probe's gap (3 against 9 ticks) is a
  different, much shorter question.
- **Order is spread thin and wide.** A single tile reads 0.60 to 0.73 almost anywhere
  within 4 tiles of the cue; it takes many tiles together to reach 0.9. Exclusive-or is
  local: 0.8 to 0.87 from single tiles next to the cue, 0.99 from a 3x3 block there.
- **Echo at three tiles**: the cue is still perfectly readable from the tiles in between,
  but the packed buttons' own 3x3 blocks see only 0.91 of it.

Per-tile grids for every run are in the probe output (`scripts/readout.sh reference
order`).

## 1. The error rule (seeds 5-12)

The full tables, with per-seed scores, are at the end. Late score (mean of the last 4 of
40 windows), the learning-off twin in the last row; packed buttons (one 3x3 group each).

| task | today's readout (mode 1) | error rule | error rule, cell knows only itself |
|---|---|---|---|
| echo k=2, 1 tile | 0.792 | **1.000** | 0.880 |
| echo k=2, 2 tiles | 0.442 | **0.812** | 0.627 |
| echo k=2, 3 tiles | 0.482 | **0.570** | 0.467 |
| order | 0.443 | **0.528** | 0.469 |
| both-not-either | 0.464 | **0.566** | 0.495 |
| gap | 0.465 | 0.454 | 0.462 |
| echo k=4 | 0.195 | **0.958** | 0.288 |
| (twin) | 0.48, 0.44, 0.45, 0.44, 0.46, 0.45, 0.15 | | |

- **The error rule beats today's readout on every task but gap** (t from 3.4 to 16.6; on
  echo 1 tile t is only 1.8 because one of today's lives locks at 0.02). It learns fast:
  late minus early is about 0 on echo, so it is done within the first 4,000 steps.
- **It needs to know which button was pressed.** The world marks it, one bit per motor
  cell (an efference copy). The nearest thing a cell knows by itself, "I fired in the
  window" against "I stayed quiet", gets about a third of the gain on echo and almost none
  on order and exclusive-or: individual motor cells fire about once in 32 ticks, so a
  cell's own firing says little about which button won.
- **On k=4 it needs balance**: gains on three buttons against a loss on one ran it to 0.43
  (seeds 1-4); scaling the other buttons' chance by their number gives 0.96.
- Tuning on seeds 1-4 (tables at the end): chance 2^-6 per input per tick it fired; 2^-4
  and 2^-8 within noise; one chance per source instead of one per tick it fired was worse
  on order and xor; also moving on good was worse on echo at 2 tiles (0.68 against 0.84)
  and on xor, better only on unbalanced k=4.

## 2. A budget on each motor cell (seeds 5-12)

`readout_budget 1`: a cell keeps the number of connections it was born with; a new one
costs one whose source was idle in the window, a lost one is replaced by an idle source.

| task | error rule | error rule, budget |
|---|---|---|
| echo k=2, 1 tile | 1.000 | 1.000 |
| echo k=2, 2 tiles | 0.812 | 0.751 |
| echo k=2, 3 tiles | 0.570 | 0.531 |
| order | 0.528 | 0.491 |
| both-not-either | 0.566 | 0.438 |
| gap | 0.454 | 0.467 |
| echo k=4 | 0.958 | 0.856 |

A null to slightly worse: the budget on the plain rule loses on 5 of 7 tasks (exclusive-or
-0.13). What the budget forces is that each gain throws out an idle source, and in a sheet
where the useful signal is thin, an idle source this time is often a useful one next time.
The budget that does work here is the one inside the evidence counters (below): the cell
wires a fixed number of sources, but which ones is decided by evidence summed over many
trials, not by one trial's activity.

## 3. Where the motor cells sit, and how many

`--motor-spread N`. Tuning on seeds 1-4 with the plain error rule (twin in brackets):

| plug | order | both-not-either |
|---|---|---|
| packed, 9 cells per button | 0.538 (0.432) | 0.610 (0.444) |
| 9 cells, nearest row, spread across | 0.547 (0.451) | 0.574 (0.448) |
| 9 cells, spread from near to far | 0.551 (0.454) | 0.533 (0.457) |
| 32 cells, nearest tile row | 0.590 (0.476) | 0.635 (0.499) |
| 32 cells, near to far | 0.586 (0.464) | 0.585 (0.483) |
| 128 cells, the whole motor region | 0.596 (0.493) | 0.617 (0.496) |
| today's readout, 128 cells | 0.481 (0.493) | 0.489 (0.496) |

Seeds 5-12, the report rules with 128 cells per button against packed:

| task | today's readout | error rule | evidence counters |
|---|---|---|---|
| echo k=2, 1 tile | 0.792 -> 0.790 | 1.000 -> 1.000 | 1.000 -> 1.000 |
| echo k=2, 2 tiles | 0.442 -> 0.528 | 0.812 -> 0.905 | 0.773 -> 0.905 |
| echo k=2, 3 tiles | 0.482 -> 0.472 | 0.570 -> 0.689 | 0.741 -> 0.853 |
| order | 0.443 -> 0.488 | 0.528 -> 0.602 | 0.575 -> 0.695 |
| both-not-either | 0.464 -> 0.487 | 0.566 -> 0.610 | 0.786 -> 0.883 |
| gap | 0.465 -> 0.485 | 0.454 -> 0.509 | 0.436 -> 0.519 |
| echo k=4 | 0.195 -> 0.238 | 0.958 -> 0.992 | 0.982 -> 0.997 |
| twin | 0.15-0.48 -> 0.25-0.52 | | |

- **Spreading does nothing for today's readout** (its gains are the twin's: more cells per
  button mean fewer ties). It helps the error rule and the counters by 0.02 to 0.13 on
  every task but echo at 1 tile, where they are already at 1.00.
- **Most of the gain is the number of cells, not where they sit.** With the plain rule 9
  spread cells are no better than 9 packed (with counters, 0.02-0.05 better); 32 cells in the nearest tile row are as good as 128 over the whole
  region. A button's answer is the sum of its cells' spikes, each firing about once in 32
  ticks, so 9 cells give a noisy count; 32 or more average the noise out. Placing cells
  near the cue mattered less than expected because a packed button already sits within a
  tile of the cues (distance 1) and hears almost everything a reader can use.
- At 2 and 3 tiles apart "128" is every motor cell at least that far from the cues (96
  and 64 per button).

## 4. Pooling, centring, and evidence counters (seeds 1-4, then 5-12)

Tuning on seeds 1-4, packed unless said:

| variant | order | both-not-either |
|---|---|---|
| error rule (2^-6) | 0.538 | 0.610 |
| far slots for the motor cells (pooling) | 0.539 | 0.609 |
| centred against the last window | 0.496 | 0.557 |
| centred, also on good | 0.527 | 0.563 |
| evidence counters, 2^-4 | 0.593 | 0.775 |
| evidence counters, 2^-2 | 0.613 | 0.646 |
| evidence counters, 2^-4, centred | 0.596 | 0.693 |
| evidence counters, 4 bits | 0.574 | 0.610 |
| centred, 128 cells | 0.553 | 0.534 |
| evidence counters, 128 cells | **0.705** | **0.892** |
| evidence, 128 cells, 8 bits | 0.685 | 0.919 |
| evidence, 128 cells, wired at strength 1 | 0.676 | 0.755 |
| evidence, 128 cells, also on good | 0.537 | 0.526 |
| evidence, 32 cells | 0.682 | 0.881 |
| evidence, 9 spread cells | 0.643 | 0.796 |

- **Pooling through far slots does nothing.** Two more random tiles per motor cell add
  sources, not signal.
- **Centring was my guess for the gap and it was wrong.** The ridge reader centres its
  inputs, so I expected the activity every trial shares to swamp the plain rule. Moving a
  source by "spikes now minus spikes in the last window" made it worse on both tasks.
- **Evidence counters (the coordinator's candidate 5) are the piece that works.** Against
  the same error rule without counters, seeds 5-12, packed: order 0.528 -> 0.575,
  exclusive-or 0.566 -> 0.786, echo at 3 tiles 0.570 -> 0.741, echo k=4 0.958 -> 0.982;
  with 128 cells: order 0.602 -> 0.695, exclusive-or 0.610 -> 0.883, echo at 3 tiles
  0.689 -> 0.853. Packed, they lose a little at 2 tiles (0.812 -> 0.773,
  two seeds at 0.64-0.65); with 128 cells they tie (0.905). The plain rule flips a connection on one trial's activity;
  with counters a source has to be on the right side many times before it is wired, which
  is what a reader of a thin signal needs. Unlike the plain rule they are still learning at
  the end of a life (late minus early 0.15 to 0.20 on order and exclusive-or).
- Settings: 2^-4 per input per tick it fired, 6-bit counters (+-31), wired at strength 4.
  8 bits is within noise of 6; 4 bits is worse; strength 1 is worse; moving on good as well
  as bad ruins it.
- Cost: 704 bytes per motor cell (180 KB for the 256 motor cells), touched only on a
  reward. Lives with counters and 128 cells per button ran at about 45,000 ticks per
  second against 60,000 for today's readout (both under nice with other agents' runs on the
  machine, so rough): the update and the rewire of 256 cells once per step is not free. The
  tick itself is unchanged.

## Side checks: eight cues, and the lock (seeds 5-12)

Not in the brief, run because they speak to two walls in the findings. Best rule; 8 cues
with 256 motor cells (32 per button, all that fit), 2 buttons also 256 (128 each).

| task | today's readout | error rule | evidence counters, packed | best rule | twin |
|---|---|---|---|---|---|
| echo k=8 | 0.079 | 0.517 | 0.701 | **0.842** | 0.107 / 0.116 |
| shuffled echo k=2 | 0.872 | 0.997 | 0.990 | **1.000** | 0.491 / 0.523 |

- **Eight cues are learned**: 0.84 where chance is 0.125 and every engine so far scored at
  chance, still rising at the end of the life (late minus early 0.26).
- **No lock on shuffled echo**: 8 of 8 lives at 1.00, including the crossed lives where
  today's readout stays at 0.56 and 0.61. The rule never strengthens its own answer on
  good, so there is nothing to lock in.

## The whole sheet learning (seeds 5-12)

Learn mode 6: the design rule everywhere (prediction, good/bad, growth, pruning, as in
mode 0) plus the best rule at the buttons' cells. Note the design rule also still acts on
the motor cells, between the counters' rewires.

| task | design rule (mode 0) | mode 0, 128 cells | design rule + best rule, 128 cells | frozen sheet + best rule, 128 cells |
|---|---|---|---|---|
| echo k=2, 1 tile | 0.874 | 0.612 | 1.000 | 1.000 |
| echo k=2, 2 tiles | 0.507 | 0.550 | **0.993** | 0.905 |
| echo k=2, 3 tiles | 0.505 | 0.498 | **0.967** | 0.853 |
| order | 0.417 | 0.496 | 0.539 | **0.695** |
| both-not-either | 0.454 | 0.487 | 0.627 | **0.883** |
| gap | 0.484 | 0.485 | 0.495 | 0.519 |
| echo k=4 | 0.312 | 0.300 | 0.995 | 0.997 |

(Packed, design rule + best rule: 1.000, 0.755, 0.857, 0.439, 0.469, 0.440, 0.961.)

- **On echo at a distance they help each other**: the whole sheet learning plus the
  readout reaches 0.99 at 2 tiles and 0.97 at 3, the best reach any engine here has shown.
  The design rule's wiring by co-activity carries the cue further; the readout reads it.
- **On order and exclusive-or they fight**: 0.54 and 0.63 against 0.70 and 0.88 frozen.
  The inner learning reshapes the sheet toward what predicts the input and what was
  co-active, and the time and conjunction features the frozen sheet had at birth get worse
  or move. Whether some of the loss is the design rule also rewiring the motor cells was
  not separated (see "next").

## Long local delays (seeds 5-12)

`--g.local_delay_min 4 --g.local_delay_max 8`, best rule, 128 cells; delays are not
learning numbers, so the delay rows have their own twin.

| task | best rule | best rule, delays 4-8 | error rule (no counters), 128 cells, delays 4-8 | twin | reference, delays 4-8 (seed 5) |
|---|---|---|---|---|---|
| order | 0.695 | **0.839** | 0.746 | 0.480 | 0.970 whole, 0.922 motors' view |
| gap | 0.519 | **0.620** | 0.558 | 0.479 | 0.801 whole, 0.755 motors' view |

Delays make both features more readable (the reference goes 0.90 -> 0.97 on order and
0.63 -> 0.80 on gap) and the rule follows: +0.14 on order, +0.10 on gap. Counters still
beat the plain rule with delays (+0.09, +0.06).

A 4x longer life (160,000 steps, seeds 5-8, no twin): order 0.718 without delays, 0.872
with them (at 40,000 steps the same seeds had 0.70 and 0.83). The rule levels off below
the reference; the rest of the gap is not just time.

## Predictions 16 and 17

- **16. A plain error rule at the motor cells learns order and exclusive-or on a frozen
  sheet to above 0.75, with the motor cells within a tile of the cue.** Wrong. The plain
  error rule, packed within a tile of the cues, gets 0.53 on order and 0.57 on
  exclusive-or. Exclusive-or passes 0.75 only with evidence counters (0.79 packed, 0.88
  with 128 cells); order passes it only with counters, 128 cells and long delays (0.84).
- **17. Spreading a button's motor cells through the sheet matters more than the rule.**
  Wrong. With today's rule, spreading changes nothing; the rule (error rule, then counters)
  is worth 0.1 to 0.8 on every task but gap. Spreading adds 0.02 to 0.13 on top of a rule
  that can use it, and that gain is mostly from having more cells per button, not from
  where they sit.

## Reading

- **The wall in findings ("reading a thin signal") is mostly a readout wall, and it can be
  moved.** A frozen sheet with nothing learned inside now does echo at 3 tiles (0.85, the
  full engine gets 0.51), exclusive-or (0.88), order (0.70, 0.84 with delays) and four cues
  (1.00, best before was table neurons' 0.68). The features were there; the rule was what
  could not add them up.
- **Three things did it, in order of size**: an error rule that moves the pressed
  button's cells one way and the others the other way; evidence summed in hidden counters
  before anything is wired; enough motor cells to average out spike noise. Balance between
  the buttons is needed as soon as there are more than two.
- **The error rule needs to know which action was taken.** Here the world tells the motor
  cells (an efference copy, one bit each). A cell that only knows whether it fired gets
  little. A brain has this signal (corollary discharge); the blob had no way to get it.
- **Gap at the suite's timing is not a readout problem.** A linear reader of the whole
  sheet gets 0.63. Delays raise it to 0.80 and the rule follows to 0.62.
- **Inner learning by the design rule helps reach and hurts the features.** That is the
  next thing to understand: the frozen sheet is a better feature bank than the learned one
  for order and exclusive-or.
- **Evidence before commitment is the general lesson** (the coordinator's candidate 5
  and idea 59). In this engine it is the difference between 0.57 and 0.79 on exclusive-or
  and between 0.57 and 0.74 at 3 tiles, packed. The minimal-engine test of the same idea is
  the second, independent result.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, the wall "Reading a thin signal": add that on a frozen sheet a
  readout with an error rule, an efference copy of the pressed button, hidden evidence
  counters, balance between buttons and 32-128 cells per button reads echo at 2 and 3 tiles
  (0.91, 0.85), exclusive-or (0.88), order (0.70; 0.84 with delays) and four cues (1.00),
  seeds 5-12. The wall stands for inner learning and for gap at the suite's timing.
- `docs/findings.md`, new principle candidates, each [likely] (one engine): evidence before
  commitment (counters beat one-trial flips by 0.05 to 0.27 on the thin-signal tasks); the
  output must know which action was taken (the self-only proxy gets a third of the gain at
  best); a readout needs enough cells to beat spike noise (9 per button is too few, 32 is
  enough). Principle 1 (a budget) gets a qualifier: a budget on one-trial flips hurt here;
  the budget that worked is a fixed count chosen by summed evidence.
- `docs/findings.md`, walls: "More than four things": eight cues are learned by the
  readout on a frozen sheet (0.84, chance 0.125). "Reach": echo at 2 and 3 tiles is now
  learned by the readout alone (0.91, 0.85) and better with the whole sheet learning too
  (0.99, 0.97). "Locking on": under the error rule no shuffled-echo life locks (8 of 8 at
  1.00, crossed lives included), which fits the findings' own diagnosis that the lock is
  the reward rule strengthening its own answer: this rule never does.
- `docs/predictions.md`: 16 wrong, 17 wrong (texts above).
- `docs/ideas.md`, idea 58: tried, kept: error rule + efference copy + evidence counters +
  balance + 128 spread cells; learn_mode 5 with `--g.readout_evidence 1 --g.readout_shift 4
  --g.readout_balance 1 --motor-spread 128`. Idea 59 gets a second test from here.
  Idea 48 (pooling): far slots did nothing; more motor cells did the pooling.
- `docs/bench.md` / the suite: the feature rows with `--frozen-readout` use learn_mode 1;
  a row with the best rule would show the features are readable. The gap row as timed is
  near the reader's ceiling (0.63) and tests little; a gap in ticks, or delays on, would.
- `src/world` owner: `--motor-spread` and `plug_mark_motors`/`plug_mark_acted` are in
  `src/world/plug*.cpp`, and one line in `src/run/life.cpp` marks the pressed button before
  the reward. The viewer and the minimal and table engines ignore the marks (null arrays).

## What I would try next

1. Keep the design rule off the motor cells in mode 6, to see whether the fight on order
   and exclusive-or is inside the sheet or at the readout.
2. The same counters for inner neurons, graded by the readout's error (a second layer):
   the first test of evidence-before-commitment for deep credit.
3. Close the rest of the order gap (0.70 against 0.82-0.90): divide each source's step by
   its usual rate (the ridge reader standardizes; centring alone did not help), or let
   counters move by count differences between the pressed and the other buttons.
4. Make `readout_balance` the default (it changes nothing at two buttons).
5. Delays 4-8 as the default for the time tasks, and the suite's gap timing revisited.
6. Cut the reward-time cost: rewire only cells whose counters crossed the cut.

## Script lines

```
cmake -S . -B build && nice -n 19 cmake --build build -j4
scripts/readout.sh reference order --seed 5                the reference, one task
scripts/readout.sh reference order --seed 5 --g.local_delay_min 4 --g.local_delay_max 8
scripts/readout.sh probe order                             the reader on the feature probe
scripts/readout.sh life order                              the best rule, one life
scripts/readout.sh tables                                  every report table (seeds 5-12)
```

The tables are `scripts/sweep.py` over `scripts/variants/readout.txt` (rules),
`readout-whole.txt` (whole sheet), `readout-delays.txt`, `readout-extra.txt`, each task with
`--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12, the
learning-off twin beside each. Tuning (seeds 1-4) used the same lines with the variant files
in the appendix; the reference runs were `blobprobe linear --task T --ticks 20
--sensor-side 3 --motor-side 3 --trials 4000 --seed S` (plus `--distance` and `--k`).

## Rough

- The pressed button comes from the world (`motor_acted`), not from the blob. That is the
  part of the rule that is not local in the strict sense; it is one bit per motor cell per
  step, written the way sensor input is.
- The reference was run on seeds 5-8 only (machine time), its chance is 0.50 (it always
  answers) where the tasks' chance is 0.44-0.48 (ties are no move), and it is fitted on
  3,000 trials of one frozen life; the rules run online for 40,000 steps. The comparison is
  in the same units, not the same conditions.
- Coin flips were not rerun; the learning-off twin is the baseline in every table (coin is
  0.44-0.48 for two buttons and 0.23 for four, `out/scorecards/blob-main.csv`).
- The feature probe's gap has a confound: the reading window sits at a different time
  after the try starts for the two classes, and one far tile (1, 6) reads gap at 1.00 with
  the linear reader, which looks like a clock locked to the try, not the cue.
- `--motor-spread` larger than fits gives all that fit (at 2 tiles apart, 96 per button;
  at 3, 64), quietly.
- Mode 6 lets the design rule move the motor cells too, so it is not a clean "fight" test.
- With the counters a motor cell's connections are all at one strength (4); the born
  strengths are only the counters' starting values.
- Lives with the counters ran about 25% slower than today's readout under load.

## Appendix: full tables

Columns as in `scripts/sweep.py`: late, the learning-off twin's late, their difference, late minus early, the paired difference to the first row with its standard error, and t. With 8 seeds, |t| under about 2 is noise.

### Rules on the frozen sheet, seeds 5-12

#### echo k=2, 1 tile

`--task echo --k 2 --distance 1 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (today) | 0.792 ±0.322 | 0.476 ±0.168 | 0.317 ±0.347 | 0.145 ±0.265 |  |  |
| error rule | 1.000 ±0.000 | 0.476 ±0.168 | 0.524 ±0.168 | 0.004 ±0.002 | +0.208 ±0.114 | +1.8 |
| error rule, cell knows only itself | 0.880 ±0.029 | 0.476 ±0.168 | 0.405 ±0.186 | -0.023 ±0.027 | +0.088 ±0.117 | +0.8 |
| error rule, budget | 1.000 ±0.000 | 0.476 ±0.168 | 0.524 ±0.168 | 0.004 ±0.001 | +0.208 ±0.114 | +1.8 |
| evidence counters | 1.000 ±0.000 | 0.476 ±0.168 | 0.524 ±0.168 | 0.002 ±0.001 | +0.208 ±0.114 | +1.8 |
| readout (today), spread 128 | 0.790 ±0.165 | 0.519 ±0.097 | 0.271 ±0.185 | 0.202 ±0.306 | -0.002 ±0.124 | -0.0 |
| error rule, spread 128 | 1.000 ±0.000 | 0.519 ±0.097 | 0.481 ±0.097 | 0.002 ±0.000 | +0.208 ±0.114 | +1.8 |
| evidence counters, spread 128 | 1.000 ±0.000 | 0.519 ±0.097 | 0.481 ±0.097 | 0.001 ±0.000 | +0.208 ±0.114 | +1.8 |

per seed, late:
  readout (today)    0.96 0.99 0.97 0.02 0.73 0.89 0.92 0.86
  error rule         1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00
  error rule, cell knows only itself 0.89 0.90 0.85 0.90 0.89 0.83 0.88 0.91
  error rule, budget 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00
  evidence counters  1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00
  readout (today), spread 128 0.97 0.78 0.45 0.73 0.91 0.77 0.96 0.75
  error rule, spread 128 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00
  evidence counters, spread 128 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00

#### echo k=2, 2 tiles

`--task echo --k 2 --distance 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (today) | 0.442 ±0.074 | 0.438 ±0.055 | 0.004 ±0.087 | 0.001 ±0.064 |  |  |
| error rule | 0.812 ±0.031 | 0.438 ±0.055 | 0.374 ±0.073 | -0.019 ±0.025 | +0.370 ±0.022 | +16.6 |
| error rule, cell knows only itself | 0.627 ±0.024 | 0.438 ±0.055 | 0.189 ±0.060 | 0.015 ±0.041 | +0.185 ±0.028 | +6.6 |
| error rule, budget | 0.751 ±0.085 | 0.438 ±0.055 | 0.313 ±0.098 | -0.041 ±0.035 | +0.309 ±0.030 | +10.3 |
| evidence counters | 0.773 ±0.097 | 0.438 ±0.055 | 0.335 ±0.122 | -0.080 ±0.082 | +0.331 ±0.038 | +8.8 |
| readout (today), spread 128 | 0.528 ±0.076 | 0.483 ±0.037 | 0.045 ±0.088 | -0.006 ±0.085 | +0.086 ±0.014 | +6.2 |
| error rule, spread 128 | 0.905 ±0.028 | 0.483 ±0.037 | 0.423 ±0.047 | -0.003 ±0.011 | +0.463 ±0.028 | +16.8 |
| evidence counters, spread 128 | 0.905 ±0.025 | 0.483 ±0.037 | 0.423 ±0.036 | -0.007 ±0.024 | +0.463 ±0.032 | +14.3 |

per seed, late:
  readout (today)    0.37 0.47 0.48 0.33 0.56 0.40 0.48 0.45
  error rule         0.80 0.85 0.81 0.81 0.86 0.79 0.81 0.76
  error rule, cell knows only itself 0.67 0.64 0.65 0.62 0.63 0.61 0.60 0.60
  error rule, budget 0.64 0.73 0.80 0.82 0.90 0.71 0.71 0.70
  evidence counters  0.65 0.83 0.64 0.87 0.91 0.74 0.77 0.77
  readout (today), spread 128 0.45 0.56 0.57 0.42 0.65 0.57 0.54 0.48
  error rule, spread 128 0.92 0.87 0.89 0.92 0.95 0.93 0.89 0.87
  evidence counters, spread 128 0.92 0.87 0.90 0.96 0.91 0.91 0.88 0.90

#### echo k=2, 3 tiles

`--task echo --k 2 --distance 3 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (today) | 0.482 ±0.056 | 0.445 ±0.036 | 0.037 ±0.033 | 0.002 ±0.049 |  |  |
| error rule | 0.570 ±0.104 | 0.445 ±0.036 | 0.125 ±0.079 | -0.018 ±0.012 | +0.088 ±0.022 | +4.1 |
| error rule, cell knows only itself | 0.467 ±0.028 | 0.445 ±0.036 | 0.022 ±0.033 | -0.004 ±0.021 | -0.015 ±0.018 | -0.8 |
| error rule, budget | 0.531 ±0.126 | 0.445 ±0.036 | 0.086 ±0.096 | -0.007 ±0.023 | +0.049 ±0.028 | +1.7 |
| evidence counters | 0.741 ±0.073 | 0.445 ±0.036 | 0.296 ±0.055 | 0.072 ±0.014 | +0.259 ±0.014 | +18.7 |
| readout (today), spread 128 | 0.472 ±0.088 | 0.476 ±0.016 | -0.004 ±0.079 | -0.007 ±0.023 | -0.010 ±0.032 | -0.3 |
| error rule, spread 128 | 0.689 ±0.097 | 0.476 ±0.016 | 0.214 ±0.094 | -0.017 ±0.018 | +0.207 ±0.027 | +7.7 |
| evidence counters, spread 128 | 0.853 ±0.044 | 0.476 ±0.016 | 0.377 ±0.044 | 0.078 ±0.029 | +0.371 ±0.015 | +24.2 |

per seed, late:
  readout (today)    0.60 0.52 0.46 0.47 0.45 0.46 0.47 0.42
  error rule         0.74 0.73 0.53 0.53 0.55 0.49 0.49 0.49
  error rule, cell knows only itself 0.49 0.51 0.43 0.47 0.47 0.45 0.44 0.49
  error rule, budget 0.75 0.71 0.45 0.48 0.46 0.46 0.45 0.48
  evidence counters  0.86 0.84 0.71 0.74 0.74 0.66 0.69 0.67
  readout (today), spread 128 0.47 0.63 0.50 0.52 0.39 0.49 0.34 0.44
  error rule, spread 128 0.78 0.89 0.62 0.64 0.68 0.61 0.63 0.68
  evidence counters, spread 128 0.90 0.94 0.83 0.81 0.84 0.82 0.84 0.84

#### order

`--task order --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (today) | 0.443 ±0.022 | 0.441 ±0.030 | 0.002 ±0.046 | 0.008 ±0.032 |  |  |
| error rule | 0.528 ±0.058 | 0.441 ±0.030 | 0.088 ±0.057 | 0.035 ±0.035 | +0.086 ±0.025 | +3.4 |
| error rule, cell knows only itself | 0.469 ±0.043 | 0.441 ±0.030 | 0.029 ±0.039 | 0.019 ±0.043 | +0.027 ±0.020 | +1.3 |
| error rule, budget | 0.491 ±0.033 | 0.441 ±0.030 | 0.051 ±0.044 | 0.016 ±0.040 | +0.049 ±0.015 | +3.3 |
| evidence counters | 0.575 ±0.039 | 0.441 ±0.030 | 0.135 ±0.046 | 0.077 ±0.048 | +0.133 ±0.018 | +7.4 |
| readout (today), spread 128 | 0.488 ±0.020 | 0.480 ±0.017 | 0.008 ±0.023 | 0.002 ±0.021 | +0.045 ±0.011 | +3.9 |
| error rule, spread 128 | 0.602 ±0.028 | 0.480 ±0.017 | 0.123 ±0.037 | 0.067 ±0.028 | +0.160 ±0.015 | +10.5 |
| evidence counters, spread 128 | 0.695 ±0.036 | 0.480 ±0.017 | 0.215 ±0.041 | 0.145 ±0.036 | +0.252 ±0.018 | +13.8 |

per seed, late:
  readout (today)    0.46 0.41 0.42 0.46 0.44 0.45 0.42 0.47
  error rule         0.56 0.57 0.52 0.51 0.56 0.43 0.60 0.47
  error rule, cell knows only itself 0.51 0.53 0.48 0.39 0.47 0.44 0.47 0.45
  error rule, budget 0.54 0.50 0.46 0.46 0.53 0.45 0.51 0.47
  evidence counters  0.58 0.56 0.59 0.54 0.57 0.53 0.66 0.58
  readout (today), spread 128 0.47 0.49 0.48 0.52 0.50 0.50 0.50 0.46
  error rule, spread 128 0.58 0.61 0.59 0.59 0.63 0.56 0.65 0.60
  evidence counters, spread 128 0.67 0.69 0.74 0.69 0.69 0.63 0.74 0.70

#### both-not-either

`--task both-not-either --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (today) | 0.464 ±0.025 | 0.457 ±0.034 | 0.008 ±0.040 | -0.006 ±0.033 |  |  |
| error rule | 0.566 ±0.038 | 0.457 ±0.034 | 0.109 ±0.060 | 0.014 ±0.030 | +0.102 ±0.014 | +7.2 |
| error rule, cell knows only itself | 0.495 ±0.020 | 0.457 ±0.034 | 0.038 ±0.043 | -0.009 ±0.015 | +0.030 ±0.014 | +2.1 |
| error rule, budget | 0.438 ±0.037 | 0.457 ±0.034 | -0.019 ±0.043 | -0.022 ±0.016 | -0.027 ±0.016 | -1.7 |
| evidence counters | 0.786 ±0.041 | 0.457 ±0.034 | 0.330 ±0.066 | 0.176 ±0.039 | +0.322 ±0.021 | +15.6 |
| readout (today), spread 128 | 0.487 ±0.023 | 0.489 ±0.019 | -0.001 ±0.035 | 0.005 ±0.028 | +0.023 ±0.011 | +2.1 |
| error rule, spread 128 | 0.610 ±0.026 | 0.489 ±0.019 | 0.121 ±0.033 | 0.025 ±0.016 | +0.146 ±0.016 | +9.1 |
| evidence counters, spread 128 | 0.883 ±0.040 | 0.489 ±0.019 | 0.395 ±0.041 | 0.199 ±0.036 | +0.419 ±0.020 | +21.2 |

per seed, late:
  readout (today)    0.47 0.49 0.45 0.44 0.46 0.51 0.46 0.45
  error rule         0.58 0.60 0.60 0.51 0.61 0.55 0.52 0.56
  error rule, cell knows only itself 0.49 0.47 0.50 0.51 0.48 0.47 0.53 0.50
  error rule, budget 0.46 0.43 0.48 0.45 0.47 0.44 0.36 0.42
  evidence counters  0.82 0.82 0.78 0.81 0.78 0.70 0.79 0.80
  readout (today), spread 128 0.49 0.50 0.46 0.45 0.51 0.47 0.50 0.51
  error rule, spread 128 0.61 0.61 0.66 0.60 0.63 0.57 0.59 0.62
  evidence counters, spread 128 0.90 0.93 0.89 0.89 0.86 0.80 0.90 0.89

#### gap

`--task gap --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (today) | 0.465 ±0.036 | 0.454 ±0.026 | 0.012 ±0.033 | 0.016 ±0.041 |  |  |
| error rule | 0.454 ±0.021 | 0.454 ±0.026 | 0.001 ±0.028 | -0.006 ±0.034 | -0.011 ±0.016 | -0.7 |
| error rule, cell knows only itself | 0.462 ±0.022 | 0.454 ±0.026 | 0.008 ±0.036 | -0.001 ±0.025 | -0.004 ±0.011 | -0.3 |
| error rule, budget | 0.467 ±0.025 | 0.454 ±0.026 | 0.014 ±0.032 | 0.014 ±0.019 | +0.002 ±0.014 | +0.1 |
| evidence counters | 0.436 ±0.020 | 0.454 ±0.026 | -0.018 ±0.026 | -0.015 ±0.032 | -0.029 ±0.014 | -2.1 |
| readout (today), spread 128 | 0.485 ±0.017 | 0.488 ±0.018 | -0.003 ±0.025 | 0.000 ±0.035 | +0.020 ±0.013 | +1.5 |
| error rule, spread 128 | 0.509 ±0.011 | 0.488 ±0.018 | 0.020 ±0.018 | 0.011 ±0.028 | +0.043 ±0.013 | +3.3 |
| evidence counters, spread 128 | 0.519 ±0.017 | 0.488 ±0.018 | 0.031 ±0.024 | 0.025 ±0.022 | +0.054 ±0.016 | +3.3 |

per seed, late:
  readout (today)    0.45 0.42 0.44 0.44 0.52 0.50 0.45 0.49
  error rule         0.48 0.44 0.44 0.47 0.45 0.44 0.48 0.43
  error rule, cell knows only itself 0.45 0.44 0.43 0.46 0.47 0.45 0.48 0.50
  error rule, budget 0.45 0.45 0.47 0.48 0.50 0.48 0.49 0.42
  evidence counters  0.43 0.44 0.41 0.44 0.41 0.46 0.44 0.46
  readout (today), spread 128 0.52 0.47 0.46 0.49 0.50 0.47 0.49 0.49
  error rule, spread 128 0.51 0.51 0.49 0.52 0.51 0.51 0.52 0.50
  evidence counters, spread 128 0.53 0.54 0.53 0.50 0.50 0.54 0.52 0.49

#### echo k=4

`--task echo --k 4 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (today) | 0.195 ±0.160 | 0.153 ±0.076 | 0.041 ±0.107 | -0.006 ±0.072 |  |  |
| error rule | 0.958 ±0.030 | 0.153 ±0.076 | 0.805 ±0.070 | 0.062 ±0.019 | +0.763 ±0.053 | +14.3 |
| error rule, cell knows only itself | 0.288 ±0.044 | 0.153 ±0.076 | 0.135 ±0.075 | -0.015 ±0.035 | +0.094 ±0.049 | +1.9 |
| error rule, budget | 0.856 ±0.048 | 0.153 ±0.076 | 0.703 ±0.070 | -0.023 ±0.045 | +0.662 ±0.051 | +13.0 |
| evidence counters | 0.982 ±0.010 | 0.153 ±0.076 | 0.829 ±0.067 | 0.029 ±0.013 | +0.788 ±0.054 | +14.7 |
| readout (today), spread 128 | 0.238 ±0.078 | 0.247 ±0.044 | -0.009 ±0.051 | -0.022 ±0.046 | +0.043 ±0.054 | +0.8 |
| error rule, spread 128 | 0.992 ±0.005 | 0.247 ±0.044 | 0.745 ±0.045 | 0.056 ±0.008 | +0.798 ±0.058 | +13.8 |
| evidence counters, spread 128 | 0.997 ±0.002 | 0.247 ±0.044 | 0.750 ±0.045 | 0.027 ±0.005 | +0.802 ±0.057 | +14.2 |

per seed, late:
  readout (today)    0.17 0.39 0.05 0.01 0.41 0.21 0.29 0.03
  error rule         0.98 0.97 0.95 0.98 0.96 0.96 0.98 0.89
  error rule, cell knows only itself 0.32 0.36 0.25 0.27 0.33 0.24 0.25 0.27
  error rule, budget 0.92 0.88 0.86 0.86 0.89 0.83 0.85 0.76
  evidence counters  0.98 0.99 0.98 0.98 0.99 0.99 0.98 0.96
  readout (today), spread 128 0.23 0.24 0.15 0.28 0.39 0.17 0.17 0.27
  error rule, spread 128 1.00 0.99 0.99 1.00 0.99 1.00 0.98 0.99
  evidence counters, spread 128 1.00 1.00 1.00 1.00 1.00 1.00 1.00 0.99

### The whole sheet learning, seeds 5-12

#### echo k=2, 1 tile

`--task echo --k 2 --distance 1 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| design rule (mode 0) | 0.874 ±0.140 | 0.476 ±0.168 | 0.398 ±0.165 | 0.219 ±0.113 |  |  |
| design rule + evidence counters | 1.000 ±0.000 | 0.476 ±0.168 | 0.524 ±0.168 | 0.002 ±0.001 | +0.126 ±0.050 | +2.5 |
| design rule, spread 128 | 0.612 ±0.086 | 0.519 ±0.097 | 0.093 ±0.029 | 0.030 ±0.134 | -0.262 ±0.054 | -4.9 |
| design rule + evidence counters, spread 128 | 1.000 ±0.000 | 0.519 ±0.097 | 0.481 ±0.097 | 0.002 ±0.001 | +0.126 ±0.050 | +2.5 |
| frozen sheet + evidence counters, spread 128 | 1.000 ±0.000 | 0.519 ±0.097 | 0.481 ±0.097 | 0.001 ±0.000 | +0.126 ±0.050 | +2.5 |

per seed, late:
  design rule (mode 0) 0.90 0.99 0.63 0.67 0.96 0.97 0.94 0.94
  design rule + evidence counters 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00
  design rule, spread 128 0.68 0.68 0.56 0.63 0.70 0.62 0.44 0.58
  design rule + evidence counters, spread 128 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00
  frozen sheet + evidence counters, spread 128 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00

#### echo k=2, 2 tiles

`--task echo --k 2 --distance 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| design rule (mode 0) | 0.507 ±0.139 | 0.438 ±0.055 | 0.069 ±0.137 | 0.024 ±0.181 |  |  |
| design rule + evidence counters | 0.755 ±0.257 | 0.438 ±0.055 | 0.316 ±0.285 | -0.111 ±0.221 | +0.248 ±0.091 | +2.7 |
| design rule, spread 128 | 0.550 ±0.042 | 0.483 ±0.037 | 0.067 ±0.045 | 0.022 ±0.053 | +0.043 ±0.046 | +0.9 |
| design rule + evidence counters, spread 128 | 0.993 ±0.004 | 0.483 ±0.037 | 0.511 ±0.035 | 0.023 ±0.009 | +0.487 ±0.050 | +9.7 |
| frozen sheet + evidence counters, spread 128 | 0.905 ±0.025 | 0.483 ±0.037 | 0.423 ±0.036 | -0.007 ±0.024 | +0.399 ±0.052 | +7.6 |

per seed, late:
  design rule (mode 0) 0.44 0.43 0.46 0.39 0.79 0.48 0.64 0.42
  design rule + evidence counters 0.41 0.95 0.93 0.98 0.98 0.71 0.71 0.35
  design rule, spread 128 0.51 0.58 0.51 0.61 0.59 0.54 0.57 0.50
  design rule + evidence counters, spread 128 0.99 0.99 1.00 1.00 0.99 0.99 0.99 1.00
  frozen sheet + evidence counters, spread 128 0.92 0.87 0.90 0.96 0.91 0.91 0.88 0.90

#### echo k=2, 3 tiles

`--task echo --k 2 --distance 3 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| design rule (mode 0) | 0.505 ±0.060 | 0.445 ±0.036 | 0.060 ±0.061 | 0.013 ±0.051 |  |  |
| design rule + evidence counters | 0.857 ±0.084 | 0.445 ±0.036 | 0.412 ±0.087 | 0.101 ±0.054 | +0.352 ±0.017 | +20.2 |
| design rule, spread 128 | 0.498 ±0.069 | 0.476 ±0.016 | 0.023 ±0.062 | 0.011 ±0.024 | -0.007 ±0.029 | -0.2 |
| design rule + evidence counters, spread 128 | 0.967 ±0.025 | 0.476 ±0.016 | 0.491 ±0.029 | 0.104 ±0.028 | +0.462 ±0.017 | +27.0 |
| frozen sheet + evidence counters, spread 128 | 0.853 ±0.044 | 0.476 ±0.016 | 0.377 ±0.044 | 0.078 ±0.029 | +0.348 ±0.022 | +16.1 |

per seed, late:
  design rule (mode 0) 0.61 0.52 0.48 0.55 0.55 0.44 0.45 0.44
  design rule + evidence counters 0.94 0.90 0.92 0.95 0.87 0.74 0.77 0.78
  design rule, spread 128 0.50 0.65 0.50 0.51 0.45 0.50 0.41 0.47
  design rule + evidence counters, spread 128 0.98 0.98 0.98 0.98 0.97 0.91 0.96 0.96
  frozen sheet + evidence counters, spread 128 0.90 0.94 0.83 0.81 0.84 0.82 0.84 0.84

#### order

`--task order --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| design rule (mode 0) | 0.417 ±0.039 | 0.441 ±0.030 | -0.023 ±0.044 | -0.023 ±0.043 |  |  |
| design rule + evidence counters | 0.439 ±0.027 | 0.441 ±0.030 | -0.002 ±0.046 | 0.007 ±0.036 | +0.021 ±0.022 | +1.0 |
| design rule, spread 128 | 0.496 ±0.031 | 0.480 ±0.017 | 0.016 ±0.042 | 0.013 ±0.037 | +0.079 ±0.019 | +4.1 |
| design rule + evidence counters, spread 128 | 0.539 ±0.030 | 0.480 ±0.017 | 0.059 ±0.030 | 0.036 ±0.035 | +0.122 ±0.012 | +10.6 |
| frozen sheet + evidence counters, spread 128 | 0.695 ±0.036 | 0.480 ±0.017 | 0.215 ±0.041 | 0.145 ±0.036 | +0.278 ±0.022 | +12.5 |

per seed, late:
  design rule (mode 0) 0.43 0.39 0.36 0.42 0.37 0.47 0.46 0.44
  design rule + evidence counters 0.44 0.44 0.47 0.46 0.44 0.41 0.39 0.45
  design rule, spread 128 0.51 0.52 0.51 0.47 0.47 0.44 0.51 0.54
  design rule + evidence counters, spread 128 0.52 0.51 0.53 0.55 0.52 0.53 0.60 0.56
  frozen sheet + evidence counters, spread 128 0.67 0.69 0.74 0.69 0.69 0.63 0.74 0.70

#### both-not-either

`--task both-not-either --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| design rule (mode 0) | 0.454 ±0.013 | 0.457 ±0.034 | -0.003 ±0.030 | -0.032 ±0.033 |  |  |
| design rule + evidence counters | 0.469 ±0.024 | 0.457 ±0.034 | 0.012 ±0.044 | -0.006 ±0.024 | +0.015 ±0.008 | +2.0 |
| design rule, spread 128 | 0.487 ±0.021 | 0.489 ±0.019 | -0.002 ±0.033 | 0.014 ±0.026 | +0.033 ±0.009 | +3.8 |
| design rule + evidence counters, spread 128 | 0.627 ±0.071 | 0.489 ±0.019 | 0.139 ±0.062 | 0.061 ±0.069 | +0.174 ±0.026 | +6.7 |
| frozen sheet + evidence counters, spread 128 | 0.883 ±0.040 | 0.489 ±0.019 | 0.395 ±0.041 | 0.199 ±0.036 | +0.430 ±0.016 | +26.6 |

per seed, late:
  design rule (mode 0) 0.43 0.46 0.46 0.46 0.47 0.46 0.43 0.46
  design rule + evidence counters 0.43 0.47 0.52 0.46 0.47 0.46 0.46 0.48
  design rule, spread 128 0.48 0.47 0.50 0.52 0.50 0.45 0.49 0.48
  design rule + evidence counters, spread 128 0.71 0.59 0.63 0.56 0.55 0.73 0.58 0.67
  frozen sheet + evidence counters, spread 128 0.90 0.93 0.89 0.89 0.86 0.80 0.90 0.89

#### gap

`--task gap --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| design rule (mode 0) | 0.484 ±0.033 | 0.454 ±0.026 | 0.030 ±0.033 | 0.032 ±0.044 |  |  |
| design rule + evidence counters | 0.440 ±0.041 | 0.454 ±0.026 | -0.014 ±0.045 | 0.006 ±0.047 | -0.044 ±0.019 | -2.3 |
| design rule, spread 128 | 0.485 ±0.027 | 0.488 ±0.018 | -0.004 ±0.034 | -0.003 ±0.032 | +0.001 ±0.016 | +0.0 |
| design rule + evidence counters, spread 128 | 0.495 ±0.016 | 0.488 ±0.018 | 0.007 ±0.022 | 0.007 ±0.031 | +0.011 ±0.012 | +1.0 |
| frozen sheet + evidence counters, spread 128 | 0.519 ±0.017 | 0.488 ±0.018 | 0.031 ±0.024 | 0.025 ±0.022 | +0.035 ±0.013 | +2.7 |

per seed, late:
  design rule (mode 0) 0.50 0.48 0.46 0.51 0.42 0.51 0.50 0.51
  design rule + evidence counters 0.42 0.40 0.42 0.48 0.47 0.43 0.51 0.40
  design rule, spread 128 0.50 0.45 0.48 0.48 0.51 0.45 0.48 0.53
  design rule + evidence counters, spread 128 0.51 0.47 0.51 0.48 0.49 0.50 0.50 0.52
  frozen sheet + evidence counters, spread 128 0.53 0.54 0.53 0.50 0.50 0.54 0.52 0.49

#### echo k=4

`--task echo --k 4 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| design rule (mode 0) | 0.312 ±0.083 | 0.153 ±0.076 | 0.159 ±0.109 | 0.049 ±0.152 |  |  |
| design rule + evidence counters | 0.961 ±0.010 | 0.153 ±0.076 | 0.808 ±0.079 | 0.023 ±0.012 | +0.650 ±0.030 | +21.6 |
| design rule, spread 128 | 0.300 ±0.080 | 0.247 ±0.044 | 0.053 ±0.084 | 0.010 ±0.094 | -0.012 ±0.038 | -0.3 |
| design rule + evidence counters, spread 128 | 0.995 ±0.003 | 0.247 ±0.044 | 0.747 ±0.045 | 0.035 ±0.006 | +0.683 ±0.029 | +23.4 |
| frozen sheet + evidence counters, spread 128 | 0.997 ±0.002 | 0.247 ±0.044 | 0.750 ±0.045 | 0.027 ±0.005 | +0.685 ±0.029 | +23.9 |

per seed, late:
  design rule (mode 0) 0.46 0.29 0.36 0.29 0.19 0.36 0.28 0.25
  design rule + evidence counters 0.96 0.95 0.95 0.97 0.96 0.97 0.96 0.97
  design rule, spread 128 0.36 0.29 0.18 0.38 0.37 0.37 0.25 0.20
  design rule + evidence counters, spread 128 1.00 0.99 0.99 1.00 0.99 1.00 1.00 0.99
  frozen sheet + evidence counters, spread 128 1.00 1.00 1.00 1.00 1.00 1.00 1.00 0.99

### Long delays, seeds 5-12

#### order

`--task order --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| evidence counters, spread 128 | 0.695 ±0.036 | 0.480 ±0.017 | 0.215 ±0.041 | 0.145 ±0.036 |  |  |
| same, delays 4-8 | 0.839 ±0.033 | 0.480 ±0.025 | 0.359 ±0.050 | 0.205 ±0.024 | +0.144 ±0.010 | +14.6 |
| error rule, spread 128, delays 4-8 | 0.746 ±0.036 | 0.480 ±0.025 | 0.266 ±0.047 | 0.145 ±0.028 | +0.052 ±0.007 | +7.0 |

per seed, late:
  evidence counters, spread 128 0.67 0.69 0.74 0.69 0.69 0.63 0.74 0.70
  same, delays 4-8   0.80 0.83 0.86 0.82 0.85 0.80 0.86 0.90
  error rule, spread 128, delays 4-8 0.71 0.73 0.80 0.77 0.76 0.70 0.78 0.72

#### gap

`--task gap --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| evidence counters, spread 128 | 0.519 ±0.017 | 0.488 ±0.018 | 0.031 ±0.024 | 0.025 ±0.022 |  |  |
| same, delays 4-8 | 0.620 ±0.050 | 0.479 ±0.022 | 0.140 ±0.055 | 0.098 ±0.037 | +0.101 ±0.019 | +5.2 |
| error rule, spread 128, delays 4-8 | 0.558 ±0.034 | 0.479 ±0.022 | 0.079 ±0.040 | 0.041 ±0.034 | +0.039 ±0.014 | +2.8 |

per seed, late:
  evidence counters, spread 128 0.53 0.54 0.53 0.50 0.50 0.54 0.52 0.49
  same, delays 4-8   0.61 0.60 0.63 0.57 0.59 0.58 0.72 0.66
  error rule, spread 128, delays 4-8 0.55 0.53 0.56 0.60 0.58 0.53 0.61 0.51

### Side checks, seeds 5-12

#### echo k=8

`--task echo --k 8 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (today) | 0.079 ±0.040 | 0.107 ±0.033 | -0.028 ±0.050 | -0.030 ±0.031 |  |  |
| error rule | 0.517 ±0.030 | 0.107 ±0.033 | 0.410 ±0.032 | 0.083 ±0.014 | +0.438 ±0.018 | +24.3 |
| evidence counters | 0.701 ±0.037 | 0.107 ±0.033 | 0.593 ±0.034 | 0.198 ±0.027 | +0.622 ±0.020 | +31.0 |
| evidence counters, spread 256 | 0.842 ±0.031 | 0.116 ±0.019 | 0.726 ±0.034 | 0.263 ±0.023 | +0.763 ±0.017 | +44.2 |

per seed, late:
  readout (today)    0.08 0.07 0.07 0.05 0.17 0.04 0.08 0.07
  error rule         0.58 0.51 0.50 0.53 0.51 0.50 0.50 0.50
  evidence counters  0.78 0.67 0.65 0.72 0.69 0.70 0.69 0.70
  evidence counters, spread 256 0.86 0.81 0.85 0.83 0.86 0.89 0.85 0.79

#### shuffled echo k=2

`--task shuffled-echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 5-12

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (today) | 0.872 ±0.178 | 0.491 ±0.167 | 0.381 ±0.215 | 0.278 ±0.141 |  |  |
| error rule | 0.997 ±0.008 | 0.491 ±0.167 | 0.506 ±0.168 | 0.010 ±0.012 | +0.125 ±0.061 | +2.0 |
| evidence counters | 0.990 ±0.025 | 0.491 ±0.167 | 0.499 ±0.174 | -0.001 ±0.012 | +0.118 ±0.058 | +2.1 |
| evidence counters, spread 256 | 1.000 ±0.000 | 0.523 ±0.100 | 0.477 ±0.100 | 0.001 ±0.000 | +0.128 ±0.063 | +2.0 |

per seed, late:
  readout (today)    0.99 0.56 0.61 0.95 0.99 0.98 0.96 0.92
  error rule         1.00 1.00 0.98 1.00 1.00 1.00 1.00 1.00
  evidence counters  1.00 0.99 0.93 1.00 1.00 1.00 1.00 1.00
  evidence counters, spread 256 1.00 1.00 1.00 1.00 1.00 1.00 1.00 1.00

### Tuning round A: the error rule, seeds 1-4

`error sN` is chance 2^-N; `OR trace` one chance per source that fired; `good too` also moves on good; `no button` the cell knows only whether it fired.

#### order

`--task order --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (mode 1) | 0.437 ±0.030 | 0.432 ±0.048 | 0.005 ±0.053 | -0.005 ±0.052 |  |  |
| error s6 | 0.538 ±0.035 | 0.432 ±0.048 | 0.106 ±0.078 | 0.045 ±0.029 | +0.101 ±0.017 | +5.8 |
| error s4 | 0.554 ±0.055 | 0.432 ±0.048 | 0.122 ±0.099 | 0.059 ±0.039 | +0.117 ±0.026 | +4.5 |
| error s8 | 0.532 ±0.067 | 0.432 ±0.048 | 0.100 ±0.110 | 0.076 ±0.069 | +0.095 ±0.044 | +2.2 |
| error s6 OR trace | 0.498 ±0.035 | 0.432 ±0.048 | 0.067 ±0.079 | 0.022 ±0.031 | +0.061 ±0.026 | +2.3 |
| error s6 good too | 0.552 ±0.010 | 0.432 ±0.048 | 0.120 ±0.053 | 0.022 ±0.037 | +0.115 ±0.011 | +10.4 |
| error s6 no button | 0.464 ±0.028 | 0.432 ±0.048 | 0.032 ±0.057 | -0.001 ±0.036 | +0.027 ±0.016 | +1.6 |

per seed, late:
  readout (mode 1)   0.44 0.43 0.41 0.48
  error s6           0.50 0.52 0.55 0.58
  error s4           0.50 0.52 0.59 0.61
  error s8           0.47 0.52 0.63 0.51
  error s6 OR trace  0.45 0.50 0.54 0.50
  error s6 good too  0.55 0.54 0.55 0.57
  error s6 no button 0.49 0.43 0.46 0.47

#### both-not-either

`--task both-not-either --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (mode 1) | 0.480 ±0.027 | 0.444 ±0.043 | 0.036 ±0.025 | -0.017 ±0.031 |  |  |
| error s6 | 0.610 ±0.022 | 0.444 ±0.043 | 0.166 ±0.046 | 0.018 ±0.032 | +0.130 ±0.011 | +11.3 |
| error s4 | 0.553 ±0.010 | 0.444 ±0.043 | 0.110 ±0.041 | -0.013 ±0.008 | +0.073 ±0.012 | +6.3 |
| error s8 | 0.589 ±0.038 | 0.444 ±0.043 | 0.145 ±0.048 | 0.041 ±0.041 | +0.109 ±0.014 | +7.9 |
| error s6 OR trace | 0.537 ±0.017 | 0.444 ±0.043 | 0.094 ±0.029 | 0.034 ±0.022 | +0.057 ±0.006 | +10.1 |
| error s6 good too | 0.578 ±0.030 | 0.444 ±0.043 | 0.134 ±0.053 | 0.007 ±0.039 | +0.098 ±0.016 | +6.3 |
| error s6 no button | 0.490 ±0.021 | 0.444 ±0.043 | 0.046 ±0.028 | -0.030 ±0.008 | +0.010 ±0.004 | +2.7 |

per seed, late:
  readout (mode 1)   0.45 0.48 0.49 0.51
  error s6           0.60 0.60 0.59 0.64
  error s4           0.55 0.54 0.55 0.57
  error s8           0.58 0.57 0.56 0.65
  error s6 OR trace  0.51 0.54 0.54 0.55
  error s6 good too  0.58 0.57 0.55 0.62
  error s6 no button 0.47 0.48 0.49 0.52

#### echo k=2, 2 tiles

`--task echo --k 2 --distance 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (mode 1) | 0.525 ±0.103 | 0.417 ±0.022 | 0.108 ±0.114 | 0.040 ±0.053 |  |  |
| error s6 | 0.842 ±0.057 | 0.417 ±0.022 | 0.425 ±0.078 | 0.006 ±0.036 | +0.317 ±0.039 | +8.1 |
| error s4 | 0.833 ±0.057 | 0.417 ±0.022 | 0.416 ±0.074 | -0.016 ±0.032 | +0.308 ±0.036 | +8.5 |
| error s8 | 0.849 ±0.045 | 0.417 ±0.022 | 0.432 ±0.064 | 0.063 ±0.035 | +0.324 ±0.041 | +8.0 |
| error s6 OR trace | 0.847 ±0.050 | 0.417 ±0.022 | 0.430 ±0.070 | 0.031 ±0.043 | +0.322 ±0.041 | +7.8 |
| error s6 good too | 0.680 ±0.019 | 0.417 ±0.022 | 0.263 ±0.038 | -0.012 ±0.002 | +0.155 ±0.051 | +3.1 |
| error s6 no button | 0.611 ±0.024 | 0.417 ±0.022 | 0.194 ±0.031 | -0.011 ±0.007 | +0.087 ±0.042 | +2.0 |

per seed, late:
  readout (mode 1)   0.41 0.60 0.62 0.47
  error s6           0.80 0.93 0.83 0.81
  error s4           0.77 0.91 0.83 0.83
  error s8           0.81 0.91 0.84 0.84
  error s6 OR trace  0.81 0.92 0.83 0.83
  error s6 good too  0.67 0.70 0.66 0.69
  error s6 no button 0.59 0.61 0.65 0.60

#### echo k=4

`--task echo --k 4 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| readout (mode 1) | 0.280 ±0.138 | 0.158 ±0.045 | 0.122 ±0.161 | 0.045 ±0.143 |  |  |
| error s6 | 0.429 ±0.106 | 0.158 ±0.045 | 0.272 ±0.135 | -0.064 ±0.052 | +0.150 ±0.056 | +2.7 |
| error s4 | 0.412 ±0.113 | 0.158 ±0.045 | 0.255 ±0.142 | -0.000 ±0.004 | +0.133 ±0.065 | +2.0 |
| error s8 | 0.590 ±0.290 | 0.158 ±0.045 | 0.432 ±0.285 | -0.203 ±0.250 | +0.310 ±0.098 | +3.2 |
| error s6 OR trace | 0.397 ±0.096 | 0.158 ±0.045 | 0.240 ±0.129 | -0.142 ±0.114 | +0.118 ±0.059 | +2.0 |
| error s6 good too | 0.718 ±0.049 | 0.158 ±0.045 | 0.561 ±0.071 | 0.012 ±0.010 | +0.439 ±0.061 | +7.2 |
| error s6 no button | 0.274 ±0.016 | 0.158 ±0.045 | 0.116 ±0.055 | -0.010 ±0.014 | -0.006 ±0.074 | -0.1 |

per seed, late:
  readout (mode 1)   0.32 0.40 0.08 0.32
  error s6           0.56 0.40 0.31 0.45
  error s4           0.56 0.35 0.30 0.44
  error s8           0.57 1.00 0.32 0.47
  error s6 OR trace  0.51 0.35 0.29 0.44
  error s6 good too  0.66 0.77 0.70 0.74
  error s6 no button 0.28 0.25 0.28 0.29

### Tuning round B: where the motor cells sit, seeds 1-4

#### order

`--task order --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| error s6 packed | 0.538 ±0.035 | 0.432 ±0.048 | 0.106 ±0.078 | 0.045 ±0.029 |  |  |
| spread 9 near | 0.547 ±0.048 | 0.451 ±0.036 | 0.097 ±0.060 | 0.048 ±0.041 | +0.009 ±0.024 | +0.4 |
| spread 9 even | 0.551 ±0.023 | 0.454 ±0.022 | 0.098 ±0.039 | 0.085 ±0.023 | +0.013 ±0.015 | +0.9 |
| spread 32 near | 0.590 ±0.029 | 0.476 ±0.011 | 0.114 ±0.038 | 0.079 ±0.034 | +0.052 ±0.027 | +1.9 |
| spread 32 even | 0.586 ±0.028 | 0.464 ±0.014 | 0.122 ±0.029 | 0.066 ±0.031 | +0.047 ±0.018 | +2.6 |
| spread 128 all | 0.596 ±0.022 | 0.493 ±0.017 | 0.103 ±0.023 | 0.072 ±0.027 | +0.058 ±0.022 | +2.7 |
| readout (mode 1) spread 128 | 0.481 ±0.005 | 0.493 ±0.017 | -0.013 ±0.014 | -0.016 ±0.034 | -0.057 ±0.019 | -3.1 |
| spread 32 near good too | 0.539 ±0.019 | 0.476 ±0.011 | 0.063 ±0.026 | 0.034 ±0.025 | +0.001 ±0.021 | +0.0 |
| spread 128 good too | 0.544 ±0.028 | 0.493 ±0.017 | 0.051 ±0.029 | 0.037 ±0.035 | +0.006 ±0.023 | +0.3 |

per seed, late:
  error s6 packed    0.50 0.52 0.55 0.58
  spread 9 near      0.57 0.50 0.52 0.60
  spread 9 even      0.52 0.57 0.56 0.56
  spread 32 near     0.63 0.56 0.60 0.58
  spread 32 even     0.60 0.55 0.58 0.61
  spread 128 all     0.61 0.57 0.62 0.58
  readout (mode 1) spread 128 0.48 0.48 0.48 0.48
  spread 32 near good too 0.56 0.52 0.55 0.53
  spread 128 good too 0.54 0.53 0.58 0.52

#### both-not-either

`--task both-not-either --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| error s6 packed | 0.610 ±0.022 | 0.444 ±0.043 | 0.166 ±0.046 | 0.018 ±0.032 |  |  |
| spread 9 near | 0.574 ±0.022 | 0.448 ±0.044 | 0.126 ±0.023 | 0.042 ±0.017 | -0.036 ±0.012 | -3.1 |
| spread 9 even | 0.533 ±0.036 | 0.457 ±0.023 | 0.076 ±0.058 | 0.030 ±0.012 | -0.077 ±0.027 | -2.9 |
| spread 32 near | 0.635 ±0.045 | 0.499 ±0.034 | 0.136 ±0.034 | 0.039 ±0.044 | +0.025 ±0.028 | +0.9 |
| spread 32 even | 0.585 ±0.036 | 0.483 ±0.019 | 0.102 ±0.042 | 0.035 ±0.041 | -0.025 ±0.023 | -1.1 |
| spread 128 all | 0.617 ±0.020 | 0.496 ±0.018 | 0.121 ±0.027 | 0.019 ±0.025 | +0.007 ±0.019 | +0.4 |
| readout (mode 1) spread 128 | 0.489 ±0.014 | 0.496 ±0.018 | -0.007 ±0.019 | 0.000 ±0.025 | -0.121 ±0.006 | -21.5 |
| spread 32 near good too | 0.535 ±0.019 | 0.499 ±0.034 | 0.036 ±0.020 | 0.009 ±0.009 | -0.075 ±0.008 | -8.9 |
| spread 128 good too | 0.523 ±0.010 | 0.496 ±0.018 | 0.027 ±0.010 | 0.001 ±0.005 | -0.087 ±0.012 | -7.3 |

per seed, late:
  error s6 packed    0.60 0.60 0.59 0.64
  spread 9 near      0.57 0.60 0.55 0.58
  spread 9 even      0.57 0.55 0.53 0.48
  spread 32 near     0.62 0.70 0.62 0.60
  spread 32 even     0.57 0.64 0.57 0.56
  spread 128 all     0.64 0.62 0.62 0.59
  readout (mode 1) spread 128 0.48 0.49 0.48 0.51
  spread 32 near good too 0.53 0.55 0.51 0.55
  spread 128 good too 0.53 0.53 0.51 0.52

### Tuning round C: centring, counters, budget, far slots, seeds 1-4

#### order

`--task order --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| error s6 | 0.538 ±0.035 | 0.432 ±0.048 | 0.106 ±0.078 | 0.045 ±0.029 |  |  |
| centred s6 | 0.496 ±0.024 | 0.432 ±0.048 | 0.065 ±0.047 | -0.000 ±0.040 | -0.042 ±0.027 | -1.6 |
| centred s6 good too | 0.527 ±0.056 | 0.432 ±0.048 | 0.095 ±0.099 | 0.017 ±0.027 | -0.012 ±0.013 | -0.9 |
| evidence s4 | 0.593 ±0.039 | 0.432 ±0.048 | 0.161 ±0.083 | 0.101 ±0.033 | +0.054 ±0.022 | +2.4 |
| evidence s2 | 0.613 ±0.027 | 0.432 ±0.048 | 0.181 ±0.053 | 0.087 ±0.047 | +0.075 ±0.024 | +3.1 |
| evidence s4 centred | 0.596 ±0.047 | 0.432 ±0.048 | 0.164 ±0.089 | 0.115 ±0.048 | +0.057 ±0.015 | +3.7 |
| evidence s4 4 bits | 0.574 ±0.038 | 0.432 ±0.048 | 0.142 ±0.079 | 0.091 ±0.043 | +0.035 ±0.014 | +2.5 |
| error s6 budget | 0.492 ±0.018 | 0.432 ±0.048 | 0.060 ±0.062 | 0.003 ±0.022 | -0.046 ±0.009 | -5.3 |
| error s6 far slots | 0.539 ±0.028 | 0.432 ±0.048 | 0.107 ±0.073 | 0.039 ±0.028 | +0.001 ±0.010 | +0.1 |
| centred s6 spread 128 | 0.553 ±0.022 | 0.493 ±0.017 | 0.060 ±0.025 | 0.028 ±0.029 | +0.015 ±0.020 | +0.7 |
| evidence s4 spread 128 | 0.705 ±0.026 | 0.493 ±0.017 | 0.211 ±0.023 | 0.142 ±0.022 | +0.167 ±0.025 | +6.6 |

per seed, late:
  error s6           0.50 0.52 0.55 0.58
  centred s6         0.50 0.52 0.51 0.46
  centred s6 good too 0.48 0.48 0.55 0.59
  evidence s4        0.59 0.55 0.64 0.59
  evidence s2        0.65 0.58 0.61 0.62
  evidence s4 centred 0.58 0.54 0.62 0.65
  evidence s4 4 bits 0.57 0.52 0.58 0.62
  error s6 budget    0.47 0.49 0.50 0.51
  error s6 far slots 0.53 0.51 0.56 0.57
  centred s6 spread 128 0.56 0.53 0.58 0.55
  evidence s4 spread 128 0.73 0.68 0.71 0.70

#### both-not-either

`--task both-not-either --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| error s6 | 0.610 ±0.022 | 0.444 ±0.043 | 0.166 ±0.046 | 0.018 ±0.032 |  |  |
| centred s6 | 0.557 ±0.027 | 0.444 ±0.043 | 0.113 ±0.043 | 0.021 ±0.003 | -0.053 ±0.013 | -4.1 |
| centred s6 good too | 0.563 ±0.034 | 0.444 ±0.043 | 0.120 ±0.054 | -0.003 ±0.011 | -0.046 ±0.010 | -4.6 |
| evidence s4 | 0.775 ±0.047 | 0.444 ±0.043 | 0.331 ±0.047 | 0.157 ±0.031 | +0.165 ±0.014 | +11.7 |
| evidence s2 | 0.646 ±0.035 | 0.444 ±0.043 | 0.203 ±0.055 | -0.001 ±0.057 | +0.037 ±0.027 | +1.3 |
| evidence s4 centred | 0.693 ±0.034 | 0.444 ±0.043 | 0.249 ±0.074 | 0.138 ±0.036 | +0.083 ±0.025 | +3.4 |
| evidence s4 4 bits | 0.610 ±0.008 | 0.444 ±0.043 | 0.166 ±0.048 | 0.014 ±0.028 | -0.000 ±0.013 | -0.0 |
| error s6 budget | 0.467 ±0.016 | 0.444 ±0.043 | 0.024 ±0.027 | -0.005 ±0.018 | -0.142 ±0.013 | -11.1 |
| error s6 far slots | 0.609 ±0.080 | 0.444 ±0.043 | 0.166 ±0.109 | 0.018 ±0.019 | -0.000 ±0.033 | -0.0 |
| centred s6 spread 128 | 0.534 ±0.006 | 0.496 ±0.018 | 0.038 ±0.019 | 0.003 ±0.031 | -0.076 ±0.009 | -8.8 |
| evidence s4 spread 128 | 0.892 ±0.048 | 0.496 ±0.018 | 0.396 ±0.033 | 0.210 ±0.031 | +0.282 ±0.029 | +9.8 |

per seed, late:
  error s6           0.60 0.60 0.59 0.64
  centred s6         0.57 0.52 0.56 0.58
  centred s6 good too 0.58 0.53 0.54 0.61
  evidence s4        0.74 0.77 0.75 0.84
  evidence s2        0.63 0.68 0.67 0.60
  evidence s4 centred 0.73 0.72 0.68 0.65
  evidence s4 4 bits 0.61 0.62 0.61 0.60
  error s6 budget    0.45 0.46 0.48 0.48
  error s6 far slots 0.63 0.65 0.49 0.66
  centred s6 spread 128 0.54 0.53 0.53 0.54
  evidence s4 spread 128 0.88 0.96 0.87 0.86

### Tuning round D: counter settings and cell counts, seeds 1-4

#### order

`--task order --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| evidence s4 spread 128 | 0.705 ±0.026 | 0.493 ±0.017 | 0.211 ±0.023 | 0.142 ±0.022 |  |  |
| evidence s2 spread 128 | 0.684 ±0.046 | 0.493 ±0.017 | 0.191 ±0.043 | 0.133 ±0.043 | -0.021 ±0.012 | -1.7 |
| evidence s4 spread 128 8 bits | 0.685 ±0.057 | 0.493 ±0.017 | 0.191 ±0.044 | 0.129 ±0.047 | -0.020 ±0.018 | -1.1 |
| evidence s4 spread 128 weak | 0.676 ±0.044 | 0.493 ±0.017 | 0.183 ±0.048 | 0.125 ±0.026 | -0.028 ±0.015 | -1.9 |
| evidence s4 spread 128 good too | 0.537 ±0.021 | 0.493 ±0.017 | 0.043 ±0.009 | 0.026 ±0.011 | -0.168 ±0.009 | -19.6 |
| evidence s4 spread 32 | 0.682 ±0.033 | 0.476 ±0.011 | 0.206 ±0.042 | 0.129 ±0.048 | -0.023 ±0.014 | -1.7 |
| evidence s4 spread 9 | 0.643 ±0.050 | 0.451 ±0.036 | 0.192 ±0.060 | 0.115 ±0.050 | -0.062 ±0.014 | -4.3 |

per seed, late:
  evidence s4 spread 128 0.73 0.68 0.71 0.70
  evidence s2 spread 128 0.74 0.63 0.68 0.69
  evidence s4 spread 128 8 bits 0.76 0.65 0.69 0.63
  evidence s4 spread 128 weak 0.71 0.62 0.67 0.71
  evidence s4 spread 128 good too 0.57 0.53 0.53 0.52
  evidence s4 spread 32 0.72 0.68 0.69 0.64
  evidence s4 spread 9 0.70 0.58 0.64 0.65

#### both-not-either

`--task both-not-either --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| evidence s4 spread 128 | 0.892 ±0.048 | 0.496 ±0.018 | 0.396 ±0.033 | 0.210 ±0.031 |  |  |
| evidence s2 spread 128 | 0.689 ±0.042 | 0.496 ±0.018 | 0.193 ±0.036 | 0.010 ±0.021 | -0.203 ±0.027 | -7.6 |
| evidence s4 spread 128 8 bits | 0.919 ±0.030 | 0.496 ±0.018 | 0.423 ±0.026 | 0.228 ±0.016 | +0.027 ±0.014 | +2.0 |
| evidence s4 spread 128 weak | 0.755 ±0.061 | 0.496 ±0.018 | 0.259 ±0.051 | 0.144 ±0.037 | -0.137 ±0.014 | -10.0 |
| evidence s4 spread 128 good too | 0.526 ±0.010 | 0.496 ±0.018 | 0.030 ±0.016 | -0.012 ±0.014 | -0.366 ±0.023 | -16.2 |
| evidence s4 spread 32 | 0.881 ±0.040 | 0.499 ±0.034 | 0.382 ±0.056 | 0.185 ±0.028 | -0.011 ±0.021 | -0.5 |
| evidence s4 spread 9 | 0.796 ±0.062 | 0.448 ±0.044 | 0.348 ±0.034 | 0.196 ±0.030 | -0.097 ±0.014 | -6.8 |

per seed, late:
  evidence s4 spread 128 0.88 0.96 0.87 0.86
  evidence s2 spread 128 0.73 0.70 0.63 0.69
  evidence s4 spread 128 8 bits 0.92 0.95 0.92 0.88
  evidence s4 spread 128 weak 0.72 0.84 0.76 0.70
  evidence s4 spread 128 good too 0.51 0.53 0.53 0.53
  evidence s4 spread 32 0.87 0.91 0.91 0.83
  evidence s4 spread 9 0.75 0.89 0.76 0.79

#### echo k=4

`--task echo --k 4 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| evidence s4 spread 128 | 0.493 ±0.016 | 0.212 ±0.029 | 0.281 ±0.031 | 0.019 ±0.006 |  |  |
| evidence s2 spread 128 | 0.475 ±0.049 | 0.212 ±0.029 | 0.263 ±0.054 | -0.011 ±0.035 | -0.018 ±0.018 | -1.0 |
| evidence s4 spread 128 8 bits | 0.493 ±0.010 | 0.212 ±0.029 | 0.281 ±0.028 | 0.038 ±0.012 | +0.000 ±0.003 | +0.0 |
| evidence s4 spread 128 weak | 0.306 ±0.036 | 0.212 ±0.029 | 0.094 ±0.046 | -0.538 ±0.057 | -0.186 ±0.026 | -7.2 |
| evidence s4 spread 128 good too | 0.502 ±0.038 | 0.212 ±0.029 | 0.290 ±0.062 | -0.166 ±0.079 | +0.010 ±0.016 | +0.6 |
| evidence s4 spread 32 | 0.486 ±0.012 | 0.209 ±0.062 | 0.277 ±0.052 | 0.021 ±0.010 | -0.006 ±0.007 | -0.9 |
| evidence s4 spread 9 | 0.492 ±0.021 | 0.207 ±0.070 | 0.285 ±0.050 | -0.117 ±0.118 | -0.000 ±0.012 | -0.0 |

per seed, late:
  evidence s4 spread 128 0.51 0.47 0.51 0.49
  evidence s2 spread 128 0.51 0.40 0.51 0.48
  evidence s4 spread 128 8 bits 0.50 0.48 0.50 0.49
  evidence s4 spread 128 weak 0.27 0.35 0.28 0.32
  evidence s4 spread 128 good too 0.56 0.48 0.49 0.48
  evidence s4 spread 32 0.48 0.48 0.50 0.48
  evidence s4 spread 9 0.47 0.48 0.52 0.50

#### echo k=2, 2 tiles

`--task echo --k 2 --distance 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| evidence s4 spread 128 | 0.911 ±0.008 | 0.510 ±0.021 | 0.401 ±0.025 | 0.010 ±0.019 |  |  |
| evidence s2 spread 128 | 0.886 ±0.010 | 0.510 ±0.021 | 0.376 ±0.018 | -0.037 ±0.018 | -0.024 ±0.004 | -5.9 |
| evidence s4 spread 128 8 bits | 0.906 ±0.008 | 0.510 ±0.021 | 0.397 ±0.013 | 0.006 ±0.025 | -0.004 ±0.007 | -0.6 |
| evidence s4 spread 128 weak | 0.858 ±0.014 | 0.510 ±0.021 | 0.349 ±0.023 | -0.011 ±0.009 | -0.052 ±0.005 | -9.8 |
| evidence s4 spread 128 good too | 0.605 ±0.013 | 0.510 ±0.021 | 0.095 ±0.024 | 0.000 ±0.015 | -0.305 ±0.005 | -63.3 |
| evidence s4 spread 32 | 0.878 ±0.044 | 0.512 ±0.047 | 0.366 ±0.025 | -0.025 ±0.020 | -0.032 ±0.020 | -1.7 |
| evidence s4 spread 9 | 0.890 ±0.040 | 0.426 ±0.039 | 0.464 ±0.036 | 0.019 ±0.017 | -0.020 ±0.022 | -0.9 |

per seed, late:
  evidence s4 spread 128 0.92 0.90 0.91 0.91
  evidence s2 spread 128 0.90 0.89 0.88 0.88
  evidence s4 spread 128 8 bits 0.90 0.92 0.91 0.90
  evidence s4 spread 128 weak 0.87 0.85 0.87 0.84
  evidence s4 spread 128 good too 0.62 0.60 0.62 0.59
  evidence s4 spread 32 0.93 0.88 0.88 0.83
  evidence s4 spread 9 0.87 0.93 0.92 0.84

### Tuning round E: balance on four buttons, seeds 1-4

#### echo k=4

`--task echo --k 4 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| evidence s4 spread 128 | 0.493 ±0.016 | 0.212 ±0.029 | 0.281 ±0.031 | 0.019 ±0.006 |  |  |
| evidence s4 spread 128 balance | 0.996 ±0.002 | 0.212 ±0.029 | 0.784 ±0.031 | 0.029 ±0.005 | +0.504 ±0.009 | +56.7 |
| evidence s4 packed balance | 0.976 ±0.022 | 0.158 ±0.045 | 0.818 ±0.055 | 0.019 ±0.013 | +0.483 ±0.018 | +26.8 |
| error s6 balance | 0.969 ±0.030 | 0.158 ±0.045 | 0.812 ±0.055 | 0.055 ±0.020 | +0.477 ±0.022 | +21.5 |

per seed, late:
  evidence s4 spread 128 0.51 0.47 0.51 0.49
  evidence s4 spread 128 balance 1.00 1.00 0.99 1.00
  evidence s4 packed balance 0.98 1.00 0.95 0.98
  error s6 balance   0.93 1.00 0.97 0.98

