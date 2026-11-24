# Table neurons (idea 26, 2026-11-24)

A different neuron on the same sheet, the same plug and the same tasks. Code in
`src/table/`, driven by `blobrun --table`, run by `scripts/table.sh`.

## What was built

- **The neuron.** Each neuron reads K input bits. Input j is one other neuron's spike at its
  own delay (1 to 15 ticks, read from the same ring of 16 spike maps the blob keeps). The K
  bits are an address into the neuron's own table of 2^K counters of B bits. The neuron
  fires if the addressed counter is at or above its sector's bar, and noise flips its output
  (either way) with chance 2^-7 per tick. Evaluating a neuron is K bit reads and one table
  read: no popcounts, no floats. With K = 6 it is the FPGA's lookup cell.
- **Layout.** Flat and the same for every neuron: inputs as one `uint32` each (source neuron
  << 4 | delay), counters packed in 64-bit words (K = 8, B = 4 is 1,024 bits, 16 words), the
  address used on each of the last 16 ticks (one byte each, for learning), and one noise-flip
  bit per neuron per tick. Deterministic for a seed.
- **Wiring at birth.** Inputs are drawn from cells within 8 cells on each axis (about a
  tile), local delay 1 to 4; 5% of inputs are far (a ring 2 or more tiles out with a long
  tail, delay growing with distance). Nothing is aimed at a task. Sensor and motor cells are
  the same lattice the blob uses.
- **Learning.** On good or bad, for each of the last 16 ticks, each graded neuron has the
  counter it used that tick moved one step, by chance (2^-4): toward what it did on good,
  away from it on bad. Only that counter moves, so one context cannot overwrite another.
  Variants tried: who is graded (only neurons that fired; also quiet ones with an input on;
  everyone), noise is the experiment (only neurons whose output noise flipped, against the
  running average of reward), and growth (a neuron swaps an input that changed none of its
  recent answers for a recently active neighbour).
- **Keeping activity alive.** Two forms were tried. Per neuron: a counter that says fire
  steps down by chance, one that says quiet steps up by a smaller chance. Per sector (kept):
  each 2x2-tile sector has a bar; a neuron fires when its counter is at or above the bar;
  every 16 ticks the bar moves one step toward 32 spikes per 1,024 neurons per tick. The
  tables are never touched by it. This is the blob's sector bias, moved from the threshold to
  the counter.
- **Harness.** `TableBlob` starts with a `Blob` shell (spike ring, sensor and motor masks,
  `sense_in`, `sense_attached`, stats), as `world/standin.cpp` does, so `plug_write`,
  `blob_fired` and the tasks run unchanged. `blobrun --table` runs the table sheet, its
  learning-off twin (`table_off`: no reward learning, no growth, the bar still regulates),
  coin flips and the perfect player. Existing drivers' csv is byte-identical (checked echo
  and catch against d1cfe92).

## How the settings were chosen (seeds 1-4 only)

Every line: echo, 40,000 steps of 20 ticks, cue and buttons as in `task.sh` (3x3 groups, a
tile apart), late score = mean of the last 4 of 40 windows, table sheet only. Chance is 0.5
for k=2 and 0.25 for k=4 (a tie or no spikes counts as a miss, so coin flips score a little
under).

| what changed | echo k=2, seeds 1-4 | mean | echo k=4, mean |
| --- | --- | --- | --- |
| first try: K=6, 2-bit counters, per-neuron regulation, every reward grades all | 0.62 0.51 0.46 0.64 | 0.56 | |
| grade only neurons that fired | 0.47 0.49 0.41 0.46 | 0.46 | |
| noise is the experiment (only noise-flipped neurons graded) | 0.46 0.44 0.48 0.48 | 0.47 | |
| K=6, 4-bit counters, grade fired + quiet-with-input | 0.74 0.86 0.49 0.83 | 0.73 | |
| K=6, 4-bit, grade everyone | 0.86 0.87 0.48 0.87 | 0.77 | |
| K=4, 4-bit, grade everyone | 0.95 0.96 0.97 0.87 | 0.94 | |
| the same, chance 2^-4 instead of 2^-3 | 0.95 0.99 0.97 0.98 | 0.97 | 0.26 |
| the same, sector bar instead of per-neuron regulation | 1.00 1.00 0.99 0.99 | 0.997 | 0.25 |
| + on bad, quiet neurons pushed to fire only 1/4 as often | 1.00 0.89 1.00 0.75 | 0.91 | 0.43 |
| K=6 with that | 0.84 0.80 0.77 1.00 | 0.85 | 0.66 |
| K=7 with that | 0.89 0.99 0.99 1.00 | 0.97 | 0.73 |
| **K=8 with that (chosen)** | 1.00 0.87 0.92 0.80 | 0.90 | **0.85** |
| K=8 without the bad damping | | | 0.31 |
| K=8, reach 12 cells (run again after a scratch-file mixup) | | | 0.70 |
| K=8, target 64 per 1,024 (the same) | | | 0.44 |
| noise is the experiment, K=4, sector bar, chance rounded to powers of two | 0.00 0.00 0.00 0.00 | 0.00 | |
| the same, chance exactly proportional to reward minus average | 0.30 0.00 0.00 0.00 | 0.08 | 0.02 |
| the same with the sign turned over (mode 2) | 1.00 1.00 1.00 0.98 | 0.99 | 0.26 |

The chosen setting is the defaults in `src/table/table.hpp`: K=8, 4-bit counters, sector bar
at 32 per 1,024, every neuron graded with chance 2^-4 over the last 16 ticks, a quiet neuron
told to fire on bad only a quarter as often. It was picked because it is the only one that
does both k=2 and k=4. K=4 is the better k=2 learner; the ladder below also reports K=4 and
K=6 on the new seeds.

Things learned on the way:
- **Counters need room.** With 2-bit counters every reward's blind grading is a random walk
  that ends up spread evenly over 0..3, so half the entries fire and activity sits at 35%.
  4-bit counters need a consistent push to cross the middle.
- **Per-neuron regulation fights learning.** A motor neuron that should fire whenever its
  cue is on (half the time) is pushed down by a regulator aiming at 3%. Stronger regulation
  made echo worse (0.57 mean). The sector bar moves the firing line instead of the tables,
  holds 3.2% activity, and learned entries (counters at 0 or 15) are not disturbed.
- **Fewer inputs, cleaner contexts.** With K=4 a neuron uses 13 of 16 entries and each gets
  many visits; with K=6 at 35% activity 57 of 64 entries are visited and each learns slowly.
  More inputs reach more cues (k=4 needs K=7 or 8).
- **Bad is the hard signal.** "Do the opposite" is right for the pressed motor group and
  wrong for most other neurons. With k=4 three of four answers are bad, everything is told
  to flip, counters climb to the top and activity runs away; damping the push toward firing
  on bad fixed that.
- **Noise is the experiment fails here, and the reason is not understood.** Graded only on
  noise flips against the average reward, the sheet settles on exactly the wrong answer
  (score 0.000 on four seeds, both buttons swapped, cue-specific). A probe on a frozen sheet
  shows a noise spike in motor group m does raise the chance of pressing m (0.42 to 0.54), so
  the causal sign is right at the motor cells. Once the wrong answer is learned every reward
  is bad, bad equals the average, and nothing moves again; that explains why it stays but not
  why it goes there. Rounding the chance to powers of two (first version) and exact
  proportional chance (second) both do it. Turning the sign over (mode 2: worse than average
  moves flipped neurons toward what noise did, better moves them back to their table) learns
  echo k=2 on all four seeds, and one shuffled-echo seed of four locks at 0.000 instead.

## 1. The task ladder, seeds 5-12

`scripts/table.sh ladder`. Each cell is the mean over seeds 5-12 of the late score (last 4
of 40 windows of 1,000 steps). The blob is the popcount blob at today's defaults with the
same flags (`ENGINE=blob`). Each engine has its own learning-off twin; coin flips are the
same for both. Catch and cartpole use 2x2 groups (`--sensor-side 2 --motor-side 2`), since
their lines do not fit at 3x3. Cartpole's score is steps per fall (higher is better).
Per-seed numbers: `out/table/<engine>/<label>/summary.txt`.

| task | table | table off | blob | blob off | coin |
| --- | --- | --- | --- | --- | --- |
| echo k=2 | **0.90** | 0.44 | 0.87 | 0.48 | 0.47 |
| echo k=4 | **0.68** | 0.21 | 0.31 | 0.15 | 0.23 |
| echo k=8 | 0.12 | 0.12 | 0.09 | 0.11 | 0.11 |
| shuffled echo k=2 | **0.78** | 0.47 | 0.76 | 0.49 | 0.48 |
| delayed echo k=2, delay 2 | 0.41 | 0.46 | 0.44 | 0.45 | 0.48 |
| catch 6x6 | 0.17 | 0.16 | 0.16 | 0.13 | 0.17 |
| cartpole (steps per fall) | 23.9 | 15.0 | 10.1 | 12.8 | 23.0 |
| cartpole, `--cart-reward lean --number-code place` | 19.2 | 17.2 | 15.4 | 21.3 | 23.0 |

Per seed, table sheet against its own twin:

| task | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| echo k=2 | 0.82 / 0.43 | 0.88 / 0.38 | 0.95 / 0.49 | 0.98 / 0.47 | 0.95 / 0.17 | 0.76 / 0.47 | 0.94 / 0.50 | 0.95 / 0.64 |
| echo k=4 | 0.73 / 0.16 | 0.78 / 0.15 | 0.24 / 0.18 | 0.71 / 0.22 | 0.95 / 0.25 | 0.24 / 0.19 | 0.99 / 0.23 | 0.78 / 0.30 |
| shuffled echo | 0.82 / 0.43 | 0.84 / 0.56 | 0.70 / 0.49 | 0.48 / 0.48 | 0.83 / 0.16 | 0.79 / 0.48 | 0.98 / 0.50 | 0.85 / 0.63 |
| cartpole | 20.4 / 13.4 | 54.1 / 9.6 | 19.6 / 18.7 | 18.0 / 13.1 | 15.0 / 15.3 | 16.9 / 13.0 | 13.2 / 9.4 | 34.4 / 27.3 |

Blob, per seed 5-12: echo k=2 0.90 0.99 0.63 0.67 0.96 0.97 0.94 0.94; echo k=4
0.46 0.29 0.36 0.29 0.19 0.36 0.28 0.25; shuffled 0.96 0.01 0.50 0.97 0.99 0.68 0.94 1.00.

Reading:
- **Echo k=2 and shuffled echo:** the table sheet learns on 8 of 8 and 7 of 8 seeds, about as
  well as the blob on average (0.90 against 0.87, 0.78 against 0.76). It fails less badly
  (worst seed 0.76 and 0.48, the blob's worst 0.63 and 0.01) but tops out lower on shuffled
  echo (the blob has five seeds above 0.93, the table one).
- **Echo k=4 is the clear difference:** 6 of 8 seeds at 0.71 to 0.99, two stuck at chance
  (0.24); the blob reaches 0.31 on average and no seed passes 0.46.
- **Echo k=8, delayed echo, catch: nothing,** for both engines. Delayed echo is slightly
  below its twin (0.41 against 0.46). Two blank steps are 40 ticks, more than the 16-tick
  ring and the 16 graded ticks reach, so the cue is probably gone before the ask and
  learning on the ask step only adds noise (not checked with a probe).
- **Cartpole:** better than its own twin on 7 of 8 seeds (mean 23.9 against 15.0) but no
  better than coin flips (23.0), which push left and right at random and so stay balanced a
  while. One seed (6) reaches 54. The blob is below both its twin and coin. With main's
  shaped reward and place code (run after the merge) the table sheet is 19.2 against its
  twin's 17.2, still under coin. Not a learner of cartpole in any useful sense.

## 2. Cue and buttons further apart

`scripts/table.sh distance`. Echo k=2, seeds 5-12. The sensor region is cut to one tile row
(`--g.sensor_h 8`), so the cue cells sit on tile row 0, and the motor region is one tile row
at row d (`--g.motor_y 8d --g.motor_h 8`). On top of that the plug packs cues and buttons
from the left, so each button is also about 1.5 tiles sideways from its cue. Both engines
get the same layout.

| tile rows apart | table | table off | blob | blob off | coin | table seeds above 0.6 | blob seeds above 0.6 |
| --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | **0.92** | 0.48 | 0.88 | 0.61 | 0.48 | 8 | 8 |
| 2 | **0.80** | 0.50 | 0.51 | 0.46 | 0.48 | 7 | 0 |
| 3 | 0.49 | 0.47 | 0.50 | 0.44 | 0.48 | 1 (0.80) | 0 |
| 5 | 0.44 | 0.48 | 0.49 | 0.45 | 0.48 | 0 | 0 |

Per seed, table sheet: 1 apart 0.77 1.00 0.98 0.99 0.80 0.85 0.99 1.00; 2 apart 0.65 0.96
0.83 0.80 0.45 0.77 0.98 1.00; 3 apart 0.44 0.42 0.43 0.43 0.45 0.53 0.80 0.43.
Blob: 1 apart 0.95 0.99 0.78 0.92 0.95 0.99 0.64 0.79; 2 apart all between 0.36 and 0.60.

Reading: learning reaches one tile row further than the blob's. Two rows apart (one empty
tile row between cue and buttons, plus the sideways offset) the table sheet learns on 7 of
8 seeds and the blob on none. Three rows apart it is one lucky seed. Five, nothing for
either. With K=8 inputs drawn within 8 cells, a cue two tile rows away is two or three hops
from a button, so learning reaches through about two layers of relays and not three.

Growth (`--t.grow_shift 11`, a neuron checked about every 2,000 ticks) does not extend it:
2 apart 0.65 (0.44-0.96), 3 apart 0.44, both worse than without growth.

**Does a cue carry?** A probe (`out/table/raw/carry.cpp`; 3 tile rows apart, seeds 5 and
6, the last quarter of a 40,000-step life) sums, over each tile row's neurons, how
differently they fire under cue 0 and cue 1, against the same measure between two random
halves of the steps (the noise floor). Spikes per step:

| tile row | 0 (cues) | 1 | 2 | 3 (buttons) | 4 | 5-7 |
| --- | --- | --- | --- | --- | --- | --- |
| seed 5, learning off | 143 / 6 | 67 / 7 | 17 / 7 | 10 / 7 | 9 / 7 | 7-8 / 7-8 |
| seed 6, learning off | 124 / 6 | 150 / 7 | 41 / 6 | 17 / 6 | 11 / 7 | 11-12 / 7 |
| seed 5, learning on | 55 / 18 | 27 / 18 | 19 / 22 | 18 / 20 | 18 / 21 | 17-19 / 20-22 |
| seed 6, learning on | 54 / 18 | 34 / 18 | 20 / 19 | 19 / 20 | 18 / 20 | 18-20 / 19 |

In the frozen sheet the cue is plain two rows out and still above the floor at the button
row (1.4x on seed 5, 2.7x on seed 6), so the signal arrives; it fades about 2-3x per tile
row. After a learning life the cue is only above the floor on rows 0 and 1: learning makes
the activity much noisier (the floor triples) and the cue is lost in it before the
buttons. So at three rows the failure is partly carrying (learning drowns the cue) and
partly credit.

### Variants on the same seeds (5-12)

`LABEL=... scripts/table.sh seeds TASK 5 12 [flags]`, labels as in the first column. Mean late
score, with the per-seed spread.

| variant | echo k=2 | echo k=4 | shuffled echo |
| --- | --- | --- | --- |
| chosen: K=8, bad damping 2 | 0.90 (0.76-0.98) | **0.68** (0.24-0.99) | 0.78 (0.48-0.98) |
| K=6 (the FPGA cell), bad damping 2 | 0.93 (0.66-1.00) | 0.53 (0.22-0.96) | 0.88 (0.64-1.00) |
| K=4, bad damping 2 | 0.92 (0.78-1.00) | 0.26 (0.22-0.40) | 0.93 (0.82-1.00) |
| **K=4, no bad damping** (`--t.inputs 4 --t.bad_rise_shift 0`) | **0.99** (0.91-1.00) | 0.25 (seeds 1-4) | **0.99** (0.95-1.00) |
| noise is the experiment, K=4 (`--t.learn_mode 1`) | 0.08 (0.00-0.47) | | |
| the same, sign turned over (`--t.learn_mode 2`) | 0.87 (seven at 0.99-1.00, one at 0.00) | 0.26 (seeds 1-4) | **1.00** (0.99-1.00) |
| chosen + growth (`--t.grow_shift 11`) | 0.93 (0.81-1.00) | 0.69 (0.45-0.90) | 0.87 (0.78-0.98) |
| learning off twin (all rows) | 0.44-0.51 | 0.21-0.24 | 0.46-0.47 |
| popcount blob | 0.87 | 0.31 | 0.76 |
| coin | 0.47 | 0.23 | 0.48 |

The rows were run after the choice, so they are reported on the held-out seeds but were not
used to pick the default. They say the chosen default is a compromise: for two cues the
small K=4 sheet with plain grading learns echo and shuffled echo almost perfectly on every
seed (0.99 and 0.99, against the blob's 0.87 and 0.76), but it cannot do four cues. Only
K=8 does four.

Growth by co-activity was chosen on seeds 1-4 between a check every 2^8 and every 2^11
ticks per neuron (2^8: 0.84 and 0.52 on k=2 and k=4, 2^11: 0.97 and 0.75, against 0.90 and
0.85 without). On the held-out seeds it helps a little and steadies the worst seed (k=4
worst 0.45 against 0.24, shuffled worst 0.78 against 0.48), changes the means by +0.01 to
+0.09, and does not help with distance (section 2).

Noise is the experiment fails on the held-out seeds too: six of eight lock onto exactly the
wrong answer (0.000). The sign-turned-over version gets shuffled echo to 0.999 on all eight
seeds, the best two-cue result here, and locks one echo seed at 0.000 the other way. Both
lock-ins come from the same place: with reward graded against its own average, a sheet that
is always right or always wrong gets no signal at all.

## 3. Capacity: how many cue-button pairs

`scripts/table.sh capacity` (echo k = 2, 4, 8, 16, seeds 5-12, the default 3x3 groups;
k=2, 4, 8 are the ladder runs). Chance is 1/k.

| pairs | table | table off | blob | coin | chance |
| --- | --- | --- | --- | --- | --- |
| 2 | 0.90 | 0.44 | 0.87 | 0.47 | 0.50 |
| 4 | 0.68 | 0.21 | 0.31 | 0.23 | 0.25 |
| 8 | 0.12 | 0.12 | 0.09 | 0.11 | 0.125 |
| 16 | 0.06 | 0.05 | (not run) | 0.05 | 0.0625 |

The sheet holds four pairs on six seeds of eight and never eight. At 4 pairs the good
seeds are 0.71-0.99, so it is not partial knowledge spread over all cues; the two failed
seeds are at chance. This is not a count of what tables can store: the sheet has about a
million counters (4,096 neurons x 256) and echo needs the right ones in a few dozen
neurons. It is geometry and credit. The plug packs cues and buttons
side by side: at k=8 the sensor groups fill two shelf rows and the buttons a row and a half
of a 5x5 grid of 6-cell groups, so buttons sit one to four tiles from their cue, most of them two or more, which
is past where learning reaches (section 2). And with 7 of 8 answers wrong, "bad" carries
little about which button was right.

## 4. Does it saturate?

`scripts/table.sh saturate`: echo k=2, seeds 5 and 6, 200,000 steps (4 million ticks).
The sheet prints the spread of its counters and of per-neuron firing rates over each span
of 65,536 ticks. "Used entries" are the counters some neuron actually addressed in that
span. Sensor neurons left out. Seed 5 (seed 6 is alike):

| | start (ticks 0-65k) | end (ticks 3.93M-4.00M) | learning-off twin |
| --- | --- | --- | --- |
| used counters at 0 (strongly quiet) | 12% | 46% | 0% |
| used counters at 15 (strongly fire) | 1% | 11% | 0% |
| used counters 6-9 (near the bar) | 79% | 19% | 100% (all 7 or 8) |
| all counters at 0 / at 15 | 3.5% / 0.2% | 15% / 3.8% | 0 / 0 |
| entries used per neuron per span | 76 of 256 | 71 of 256 | 99 of 256 |
| firing rate, mean | 0.033 | 0.032 | 0.068 |
| neurons under 1/64 | 49% | 63% | 29% |
| neurons at 1/2 or more | 1.1% | 0.4% | 0% |
| sector bars | 7 to 11 | 2 to 14 | 8 |
| late score | | 0.83 (seed 6: 0.90) | 0.44 (0.37) |

Reading: it does saturate, but only where it is used and in both directions. After 4
million ticks 57% of the counters neurons actually use are pinned at an end, and 4 in 5 of
those are pinned at "quiet". Across all counters 81% are at neither end and 40% still hold
their birth value (7 or 8): the tables have plenty of room, and a context that is never
used never moves. That is
the opposite of the blob, where 87% of all connections end at top strength. Activity stays
at the bar's target (3.2%) with no neuron stuck on; the spread of rates widens (more nearly
silent neurons). Score kept rising to the end (late 0.83 and 0.90, against 0.82 and 0.88 at
40,000 steps).

The learning-off twin is not a steady sheet: its counters sit at 7 or 8, so the bar can
only choose between 12% of neurons firing (bar 8) and almost none (bar 9), and it flips
between the two every 16 ticks (6.8% on average). The twin is still the frozen-wiring
baseline the score needs, but its activity is a blinking one.

## 5. Speed and size

`scripts/table.sh speed` (`tablebench`): one core, random input on every sensor cell, good
or bad every 20 ticks, timed for one second per size after 200 ticks. Measured with other
agents' runs on the machine (load about 7 of 32 cores); `blobbench` alongside for the blob.

| neurons | K=8 (chosen) | K=6 | K=4 | K=8 learning off | blob, learning on | blob, off |
| --- | --- | --- | --- | --- | --- | --- |
| 4,096 | 38,600 | 45,900 | 57,400 | 40,300 | 44,000 | 62,600 |
| 65,536 | 2,270 | 2,820 | 3,520 | 2,420 | 2,930 | 3,710 |
| 1,048,576 | 94 | 149 | 208 | 105 | | |

Ticks per second. As neuron updates per second: K=8 1.6e8 at 4,096 and 1.0e8 at a million,
K=4 2.3e8 and 2.2e8; the blob 1.8e8 (learning on) to 2.6e8.

Bytes per neuron (state that grows with neurons):

| | inputs | counters | addresses kept for learning | spike + noise rings | total |
| --- | --- | --- | --- | --- | --- |
| K=8, 4-bit | 32 | 128 | 16 | 4 | **180** |
| K=6, 4-bit | 24 | 32 | 16 | 4 | 76 |
| K=4, 4-bit | 16 | 8 | 16 | 4 | 44 |
| blob | 176 of weight masks, 11 of delays, 9 of per-neuron state | | | 2 | about 200 |

Reading: on one core the table sheet is no faster than the blob. A neuron costs K scattered
bit reads (one per input, each from its own delayed map) and one table read; at about 30
cycles per neuron the loop is bound by those reads and their address arithmetic, not by
arithmetic. The cost is flat per neuron from 4,096 to a million (K=4 loses 7%, K=8 38% as
the 128-byte tables leave the cache). It is the shape that pays off on hardware: K=6 is
exactly one FPGA lookup cell plus one 6-bit address register, with the counters in a small
RAM next to it; the inputs are fixed wires; there is nothing to add up. The 16 bytes of
kept addresses could be recomputed from the spike ring when a reward comes (the ring holds
16 ticks), which would bring K=6 to 60 bytes per neuron.

## What the numbers say

- **The table neuron learns the reflex tasks as well as the popcount blob or better, with
  8 inputs instead of up to 96 connections.** Echo k=2 0.90 against 0.87; shuffled echo
  0.78 against 0.76; and the small K=4 sheet does both at 0.99, on every held-out seed.
- **It does four cues** (0.68 against the blob's 0.31; six of eight seeds at 0.71-0.99),
  **and reaches one tile row further** (two rows apart: 0.80 against 0.51).
- **Neither engine does the rest**: echo with 8 or 16 cues, delayed echo, catch and
  cartpole are at chance (or coin level) for both.
- **It saturates only where used, and mostly toward "quiet".** Untouched contexts keep their
  middle values; only the addressed counter moves, as designed.
- **Getting there needed three things the brief did not have**: 4-bit counters (2-bit ones
  random-walk to 50% firing), a sector bar on the counter instead of per-neuron regulation
  of the tables (which fights learning), and a weaker push toward firing on bad.
- **More neurons were not needed** at this size. The same 4,096 cells were enough; what
  mattered was more inputs per neuron (K=8 for four cues), because a cell that cannot
  generalise needs its few inputs to include the cue, or a relay of it.
- **Noise is the experiment fails** in its natural form here and locks onto the exact wrong
  answer; its sign-reversed form works for two cues (shuffled echo 1.00 on 8 of 8 seeds).
  This is unexplained and worth a closer look before the same rule is trusted on the blob.
- **Speed is a wash on a CPU core** (the blob is a little faster); the case for it is the
  hardware shape.

## What to try next on this idea

1. Find out why noise-graded learning goes to the wrong answer: log, per reward, which
   flipped neurons were graded and whether they were motor, relay or far, on one seed.
2. A per-neuron bar or a small random spread of bars, so the learning-off twin does not
   blink and the frozen sheet is a fair baseline.
3. K=6 with the addresses recomputed from the ring: the FPGA-exact cell at 60 bytes; try
   two K=6 cells per site (one wide, one local) to get K=8's reach at K=6's cost.
4. Grading only neurons within reach of a motor cell's recent inputs (a credit trace that
   walks back through the addresses that were used), to push the distance past two rows.
5. A GPU or AVX-512 gather version of the tick; the scalar loop is read-bound.

## Rough

- The learning-off twin blinks between two activity levels (section 4).
- The default (K=8 with bad damping) was the only setting that did both k=2 and k=4 on
  seeds 1-4; it is not the best for two cues (K=4 without damping is). Rows for both are
  given; the coordinator should not read the K=4 rows as chosen in advance.
- The three k=4 tuning runs the shared-scratchpad mixup broke (reach 12, target 64,
  growth at k=4) were run again in the worktree; none of the three was used to choose.
- Tables are allocated for sensor neurons too (unused), to keep every neuron the same
  shape. Sensor neurons' predictions are written to `predicted` but not learned.
- Run lines: `scripts/table.sh ladder`, `distance`, `capacity`, `saturate`, `speed`; the
  variant rows with `LABEL=name scripts/table.sh seeds TASK 5 12 [flags]` as listed in
  `out/table/raw/queue.sh` and `queue2.sh`. Raw outputs, probes and the seeds 1-4 tuning
  runs are under `out/table/` (not in git).
