# The blob's own output for the one-hop rule (2026-11-26)

Queue item 9a, idea 182, following `docs/results-2026-11-26-every-event.md` (idea 178).
**Backprop is a measuring stick here, never the project's method.** The every-event job
found that the one-hop rule with every-event counters teaches the inside when its error
comes from an output outside the sheet, and not when it comes from the sheet's own readout.
This job builds the output inside.

**The answer: the smallest change is a rule, not more accuracy: at each verdict, told the
right line, exactly two lines move, the right one up and the highest wrong line that came
too close down, and those two lines are the error the one-hop rule hears; on the engine's
motor cells as they are (a new value of `readout_good`, no new counters) that output is right
0.86 of the time during an MNIST life and the closed loop reads 0.945 / 0.732; with each
motor cell's counters used as the weights themselves, summed over the sources that fired
and read once a step (`out_on`: a signed 12-bit counter per source a motor cell can hear,
132 words a motor cell, about 160,000 word operations a verdict, no chance drawn, nothing
re-picked) the blob's own output is right 0.925 / 0.708 of the time online and the closed
loop, no outside part, reads 0.956 on MNIST and 0.759 on the pair test after a 60,000-digit
life at the setting fixed before the test digits were touched, and 0.961 / 0.776 with the
output right 0.936 / 0.727 at the setting the validation slice picked afterwards (a
10-tick window, inhibitory and pacemaker cells learning too): one setting for both tests,
at or over the outside outputs' two best numbers (0.951 / 0.770), level with the wiring
written in from the model on MNIST (0.961) and over it on pairs (0.761).**

(MNIST / pair test throughout; "reads": a fresh linear reader of the non-sensor cells
after the life, test digits, seeds 5-12, unless it says validation.)

## Short version

| | MNIST | pairs | its output right, online | |
|---|---|---|---|---|
| born | 0.9251 | 0.6774 | | 8 seeds |
| the engine's readout as the error source (`ev5`, every-event doc) | 0.926 | 0.703 | 0.68 / 0.55 | 3 seeds |
| outside softmax output, fine 0.1 and fine 0.3 (every-event doc) | 0.9508, 0.9446 | 0.7119, 0.7702 | 0.91 / 0.74 | 8 seeds |
| outside whole-number output (every-event doc) | 0.9415 | 0.7677 | 0.90 / 0.72 | 8 seeds |
| the same with only the top wrong line stepping (`outtop`) | 0.9576 | 0.7677 | 0.93 / 0.72 | 8 seeds |
| **the closed loop: the blob's own output, the setting fixed before the test digits (`intope`)** | **0.9559** | **0.7594** | **0.925 / 0.708** | 8 seeds |
| its coin (the cells get another verdict's errors) | 0.9375 | 0.6798 | 0.90 / 0.61 | 8 seeds |
| its shuffled-digit life | 0.9256 | 0.6698 | | 8 seeds |
| the closed loop, the errors paid back along the output's own sources (`paytope`) | 0.9557 | 0.7642 | 0.924 / 0.714 | 8 seeds |
| **the closed loop, the setting validation picked afterwards (`intopeallw10`)** | **0.9613** | **0.7757** | **0.936 / 0.727** | 8 seeds |
| its coin | 0.9433 | 0.6801 | 0.90 / 0.61 | 8 seeds |
| that setting with payment (`paytopeallw10`) | 0.9616 | 0.7773 | 0.934 / 0.727 | 8 seeds |
| the smaller change: the motor cells as they are under the rule (`enganstope`) | 0.9454 | 0.7319 | 0.857 / 0.624 | 8 seeds |
| the every-event wiring learned in the model and written in | 0.961 | 0.761 | | ladder doc |
| the perfect teacher's wiring written in | 0.966 | 0.785 | | idea 165 |

- **What the readout lacked.** Not accuracy. It moved every button at every verdict (one
  gains, nine lose), and its votes sent the cells an error on two digits in three for the
  whole life. The ladder below takes the outside whole-number output down to the engine's
  readout a difference at a time. Reading through the motor cells' own fields, one bit a
  source, 12 bits: nothing lost for the cells. What breaks is any rule in which more than
  one line comes down for the one that goes up: what the lines share then drifts, narrow
  counters reach their end and any cell with a bar goes silent. One line down for one up
  and every rung holds, the engine's spiking motor cells among them.
- **What the cells need to be told.** Two lines: the right one and the one most confused
  with it, on the digits the output has too close (one in five on MNIST). That takes the
  fresh reader from 0.944 to 0.957 on validation whether the output sits outside or
  inside. An output right 0.71 of the time teaches as well as one right 0.90 when it
  learns by the errors it sends; the engine's readout, right 0.83 under its runner-up
  rule, teaches no better than at 0.67.
- **The closed loop, as fixed before the test digits,** reads 0.9559 / 0.7594: every seed
  of both tests over born (+0.031 / +0.082), over its coin (+0.018 / +0.080) and over a
  shuffled-digit life (+0.030 / +0.090). On MNIST every seed is over the outside softmax
  output (+0.005) and 0.005 under the written-in sheet; on pairs every seed is under the
  outside outputs (-0.011 and -0.008) and the mean is level with the written-in sheet
  (0.761). Its own output is right 0.925 / 0.708 online and 0.931 / 0.715 on the test
  digits afterwards, frozen. Hidden cells fire at 38.7 / 33.5 per 1,000 cell-ticks while
  read (born 33.1 / 32.7); with the rate held at target the MNIST gain is the same
  (+0.0315 at 31, 4 of 4 seeds).
- **With the two knobs validation liked** (a source is "on" for 10 ticks, not 15;
  inhibitory and pacemaker cells learn too) the loop reads 0.9613 / 0.7757: over the first
  setting on every seed of both tests (+0.005 / +0.016), over the outside softmax output
  at its own best settings (+0.011, 8 of 8; +0.006, 6 of 8), over the same rule as an
  outside output on 15 of 16 seeds, +0.018 / +0.096 over its coin. This arm was chosen
  after the first report was known, by validation numbers only.
- **The smaller change** (the engine's motor cells as they are, told the right line, two
  buttons moving): 0.9454 / 0.7319 on test, the output right 0.857 / 0.624, every seed
  over born and over its coin; the counters-as-weights output is over it on every seed
  (+0.011 / +0.028). With its zone a sixteenth instead of a quarter it reads 0.952 / 0.736
  on validation (two seeds; not run on test), where the outside softmax output read 0.951.
- **The output alone**, on a sheet that does not learn, is right 0.906 / 0.620 online
  (0.914 / 0.628 on the test digits). The engine's readout on that sheet: 0.68 / 0.51.
- **One zone for both tests.** The margin is counted in steps' worth, so it scales with
  how much was on; 1 and 2 are level on both tests on validation, 4 and 8 worse on both.
  The report ran at the default, 2.
- **Hardware.** 132 words a motor cell (the old readout: 88), no random word, no re-pick;
  an AND and a popcount a plane a slot a motor cell once a step. 8-bit counters read the
  same on validation (88 words). The world reads ten numbers, not spikes, and the output
  must be told the right line: those are the design's costs.
- **Payment** along the output's own sources in place of the hashed wires: level on MNIST
  (-0.0002, 3 of 8 seeds), +0.005 on pairs (7 of 8) at the first setting; level on both
  at the second (+0.0004 / +0.0016). It needs no hashed wires. The model's lead was
  +0.004 / +0.012.
- **Toys.** Xor: nothing lost (the output answers 1.000 in the life; the cells read
  0.995 by the old fresh readout, 0.999 before). **Parity of 3: a loss.** The cells the
  new loop teaches read 0.758 by the old fresh readout where the every-event arm's read
  0.919 (8 of 8 seeds lower; born 0.552), and the new output's own score in the life is
  0.867 against the old readout's 0.909. A wider margin and a read without sensor cells
  do not mend it; the cause was not found.
- **The map.** Sixteen cues go from 0.27 to 0.97 with the new output alone and 0.997 with
  the loop; the map's MNIST row (20,000 digits, online) from 0.21 to 0.89 and 0.90. Not an
  even match: the old readout hears good and bad, this output is told the right button.
- **Speed.** A closed-loop MNIST life runs at 38,400 ticks a second here, 6.7 times a
  judged life under the old readout (5,700); the output alone costs about 3 microseconds
  a tick. `blobbench` with the numbers off is unchanged.
- **What it means for the queue.** The output is no longer the missing piece: the loop is
  closed inside the blob at or over the outside outputs' level, and the live rule now
  reads what the model's wiring read when written in. What is left: 0.005 / 0.010 to the
  perfect teacher's wiring; an answer in spikes; and an output that learns without being
  told the right line.

## Predictions (written and committed before any scoring run)

Seen before writing these: the three docs named in the brief with their numbers (the outside
whole-number output `outint2`: fresh reader 0.9444 / 0.7702 on validation and 0.9415 / 0.7677
on test, right 0.90 / 0.72 of the time online, teaching on 0.34 / 0.56 of the digits; the
softmax output 0.951 / 0.770 at its two fine zones; born 0.925 / 0.677; the engine's readout
right 0.68 online under its plain rule, 0.845 at 60,000 digits on a born sheet under the
runner-up rule with the gate; `ev5`, the plain readout's votes as the error, 0.932 / 0.720 on
validation), the code, and three 3,000-digit lives on validation seed 1 that say nothing
about scores (they showed that the ladder's form 4 with every option off gives form 3's
life to the digit, and that a life takes about five seconds at that length).

The ladder, as outside outputs of a live life (names as in `scripts/outputinside.py`); my
guess for the output's share right in the last tenth of an MNIST life, and for the fresh
reader after it, MNIST / pairs:

| rung | what changes | online | fresh reader |
|---|---|---|---|
| outint2 | the start: every cell, 16 bits, counts, margin perceptron | 0.90 (known) | 0.944 / 0.770 (known) |
| fld | a line is its motor cells, each reading the places it can hear | 0.91 | 0.944 / 0.765 |
| fldbit | a source is one bit: fired in the last 15 ticks | 0.895 | 0.942 / 0.760 |
| fld12 | 12-bit counters | as fldbit, within 0.003: 0.7 | |
| fld8 | 8-bit counters, every event | 0.86 | 0.935 / 0.740 |
| fld8s6 | 8 bits at the engine's chance, 2^-6 a spike | 0.84 | 0.935 / 0.735 |
| fldr1 | told good or bad only, the engine's plain rule | 0.60 | 0.927 / 0.700 |
| fldr2 | with the gate | 0.88 | 0.940 / 0.750 |
| fldr3 | runner-up and gate | 0.87 | 0.940 / 0.750 |
| com | a cell votes one bit (its sum over zero) | 0.85 | 0.935 / 0.740 |
| bit1 | one-bit wiring at the born count, drives summed | 0.75 | 0.932 / 0.720 |
| eng | the engine's readout, plain rule | 0.68 (known) | 0.932 / 0.720 (known) |
| enggate | the gate | 0.80 | 0.935 / 0.720 |
| engrun | runner-up and gate | 0.84 | 0.937 / 0.725 |
| engrunstr | and the top quarter strong | 0.85 | 0.937 / 0.725 |

- **The rung where the output falls**: one-bit wiring at a fixed count 0.45; the rule's good
  half 0.25 (known from the output-gap doc, and the gate mends it); cells that spike
  against a threshold 0.15; the counters' width and chance 0.10; the motor cells' fields or
  one bit a source 0.05.
- **The piece I expect to build**: each motor cell keeps a signed counter per source it can
  hear and its vote is the sum of its counters over the sources that fired, read at the
  verdict; a line's vote is its cells' sum; a margin perceptron told the right line.
- **That output inside the blob, alone**: right in the last tenth of an MNIST life 0.90
  (between 0.89 and 0.91: 0.6); above 0.90 as the mean of 8 seeds: 0.45. Pairs 0.71.
- **The closed loop, fresh reader** (test, seeds 5-12): MNIST 0.942, pairs 0.760. Within
  0.005 of `outint2` on MNIST (0.9415): 0.6; within 0.010 of it on pairs (0.768): 0.5.
  Reaches 0.951 on MNIST: 0.1; reaches 0.770 on pairs: 0.25. Above its coin on every seed
  of both tests: 0.8. Above a shuffled-digit life on every seed: 0.9.
- **One fine-zone setting for both tests** (within 0.005 of each test's own best on
  validation): 0.6.
- **Firing rate** of the closed loop while read: 37 to 41 per 1,000 cell-ticks on MNIST
  (0.7), 33 to 36 on pairs (0.7).
- **Payment along the output's own sources** in place of the hash wires: 0.944 / 0.765.
  Beats the hash wires on MNIST by 0.002 or more: 0.35; on pairs by 0.005 or more: 0.35;
  on both: 0.2. (The model's lead was +0.004 / +0.012 with an output that read every cell
  with float weights.)
- **Toys with the new output as the blob's output**: xor fresh output late 0.99 (0.95 or
  more: 0.7); parity of 3 0.90 (0.85 or more: 0.5). Two quick-map rows: within 0.03 of the
  old output's score: 0.5.
- **Speed**: a closed-loop MNIST life runs at 25,000 ticks a second or more here (the
  outside whole-number life: 39,400; a judged life under the old readout: 5,950): 0.6.

Seen after the predictions and before the report runs: the ladder and the validation
screens below. Everything the ladder found was added after them: the top-line-only step
(`--outside-top`, `out_top`), `readout_good 3` and `4`, `out_hidden`, the even layout on
the pair test, and every knob arm. The report's arm (`intope`) was fixed before any run on
the test digits or seeds 5-12: the closed loop at its defaults (margin 2, window 15, 12
bits, hidden excitatory learners) with `out_top 1` and `--motor-spread-even 1`. Payment
(`paytope`) went into the report beside it.

## What was built

All off by default; the new genome numbers are at the end of the genome and of
`genome_args.cpp`.

**The ladder's outside forms** (`blobmnist ceiling --life-outside 1 --outside-form 4`,
`src/mnist/mnist_outside_cells.cpp`; floats are allowed there and it uses none). The
every-event job's whole-number output (form 3) with one option per difference from the
engine's readout; with every option off it gives form 3's life to the digit. It is still a
table outside the blob: a measuring stick for which difference costs what.

| option | what it changes |
|---|---|
| `--outside-read 1` | a line is its 102 motor cells, and each cell reads the 704 places it can hear (9 local tiles, 2 far; itself left out; no inhibitory source on a far slot), sensor cells included. `2`: the same without sensor cells |
| `--outside-act 1` | a source is one bit, "fired in the last 15 ticks", not a spike count over the digit |
| `--outside-bits N`, `--outside-step-shift N` | the weights' width; each spike steps with chance 2^-N |
| `--outside-top 1` | of the wrong lines that were too close only the highest steps down (and only it is named to the cells) |
| `--outside-rule 1, 2, 3` | told good or bad only: the engine's plain rule, with the gate, with the runner-up rule and the gate (16 to 1, in whole numbers) |
| `--outside-vote 1, 3` | a line's vote is the number of its cells whose own sum is over zero (1), or over the cell's own running mean (3) |
| `--outside-vote 2` | each cell wires its highest weights at its born count, one bit each; the vote is the cells' summed drive |

**The blob's own output** (`--g.out_on 1`, `src/blob/learn_output.cpp`, `output_state.hpp`).
The engine's readout is motor cells that spike: each wires a fixed number of sources, one
bit each, picked by hidden 8-bit counters that move by chance. This keeps the counters and
drops the wiring: the counters are the weights.

- State. Each motor cell of a button keeps a signed counter per source it can hear, as
  `out_bits` bit planes a slot (the one-hop rule's layout, `hop_planes.cpp`). All start at
  zero.
- Read. On the last tick of a step a source is "on" if it fired in the last `out_window`
  ticks (15): one word a tile, an OR of spike maps. A cell's vote is the sum of its counters
  over the sources that are on: an AND and a popcount a plane a slot, shifted by the plane.
  A line's vote is the sum of its cells' votes. The world takes the line with the highest
  vote. No motor cell's spike is read.
- Learn. The world names the right line at the verdict, as it does for the one-hop rule. A
  wrong line whose vote is within `out_margin` steps' worth of the right line's, or over it,
  steps down: each of its cells takes one count off every source that was on. With
  `out_top 1` only the highest such line steps. Then the right line steps up the same way.
  A step's worth is what one step would move the vote by (the number of cell-source pairs
  that were on). A right line that wins by more than the margin moves nothing. Every event
  counts; no chance is drawn; nothing is re-picked.
- Teach. The lines that stepped are the one-hop rule's errors (`--g.hop_teach 6`, a new
  value): "lower" for the wrong line, "higher" for the right one, nothing for the rest and
  nothing at all on a step won clear. So the fine zone is the output's own margin.
  `out_fine` gives the cells a zone of their own; `out_top 2` lets the output step one
  line while the cells hear of every close line.
- Pay (`--g.out_pay 1`). The rule's errors go back along the output's own sources instead
  of the hashed wires: a line told "higher" pays each source that was on and has a counter
  above zero in one of its cells and charges each with a counter below zero; a line told
  "lower" the reverse. The tallies are bit-sliced counts a tile; a cell paid more often
  than charged is told "more", the reverse "less". The rule's own marks and gate (near its
  bar or fired) stay.
- `out_hidden 1`: no motor cell counts a sensor cell.

**The same rule on the motor cells as they are** (`--g.readout_good 4`, a new value;
`learn_readout.cpp`). The spiking readout keeps its cells, its one-bit wiring and its 8-bit
chance counters and changes only who moves: told the right line, the cells of the highest
wrong line whose spikes were within `hop_fine` 32nds of the right line's lose their active
inputs, and the right line's cells then gain theirs; a step won clear moves nothing; good
and bad play no part. `hop_teach 5` then names the same two lines to the cells.
`readout_good 3` is the form that moves every close wrong line.

**The world's side** (`src/world/plug_output.cpp`, `plug.cpp`, `plug_hop.cpp`,
`src/run/task_ticker.cpp`). Plain reads and writes of blob fields, as for `sense_in`: at
attach the plug names each button's motor cells (`blob.motor_line`, a byte a neuron); the
ticker sets `blob.out_read` before a step's last tick; the blob answers in that tick with
`blob.out_vote`, a number a line, which the plug takes in place of its spike counts; and at
the verdict the plug passes the right line (`blob_output_verdict`), beside what it already
passes to the one-hop rule. A tie between lines goes to the lowest line (`plug.tie_low`),
so a newborn output, every vote zero, still answers and can be told it was wrong. A
pattern answer (the calc battery) is handled a pair of lines at a time. An output
re-attached starts from zero.

## Hardware

For the MNIST sheet: 4,096 neurons, 1,020 motor cells, 10 lines, `out_bits 12`.

- **Bits per source**: 12 (a signed counter, in planes). The old readout: 8 bits of
  counter and the 2 wiring bits every neuron has.
- **Words per motor cell**: 11 slots x 12 planes = 132 (1,056 bytes), the same as a hidden
  cell under the every-event rule. The old readout: 88 words of byte counters. Whole sheet:
  134,640 words, 1.1 MB. Nothing per hidden cell.
- **Work per verdict (the read)**: 64 tiles x 15 ORs for "on"; then per motor cell per
  slot one AND, one popcount for the step's worth, and 12 ANDs and popcounts: 1,020 x 11 x
  14, about 157,000 word operations a step, 7,900 a tick. The tick itself does about
  90,000 ANDs and popcounts (4,096 neurons x 11 slots x 2 planes) before inhibition and
  learning, so the read adds under a tenth.
- **Work per verdict that moves** (about one in five on MNIST late in a life): two lines x
  102 cells x 11 slots x about 72 word operations (the planes' carry and borrow): about
  160,000. No random word, no branch per source, no sort, no re-pick. The old readout's
  rule draws a chance word a slot a tick of the window for every cell it moves and then
  re-picks each of them over 704 counters; it is nine tenths of a judged life's cost.
- **On the tick's path**: two tests of a genome number. The read runs once a step, the
  moves once a verdict.
- **What the world reads**: ten numbers at the end of a step, not spikes. This is a change
  to the contract ("the world reads bits off motor neurons"), and the plainest cost of the
  design: the answer is a sum of counters, and nothing in the sheet hears it.
- **Payment** (`out_pay`): per line in error, per cell, per slot, 12 ORs (which counters
  are not zero) and two bit-sliced adds of 8 planes; then 24 word operations a tile to
  compare: about 60,000 a verdict that teaches. The hashed wires cost a hash a cell a line.
- **Narrower**: `out_bits 8` (88 words a motor cell, the old readout's size) reads the
  same on validation (0.9565 / 0.7656 against 0.9554 / 0.7635); no counter passed 12 bits
  in any life with one line stepping (16 bits give 12's runs to the digit).
- **Measured**: the output alone costs about 3 microseconds a tick over the frozen sheet
  and the closed loop runs at 38,400 ticks a second, 6.7 times a judged life under the old
  readout (the speed table below).
- **The smaller change** (`readout_good 4`): no new state at all beside a byte a neuron for
  its line; two buttons' cells move at a verdict that teaches instead of ten.

## The ladder (validation slice, seeds 1 and 2)

Each rung is the output that makes the one-hop rule's errors during a 60,000-digit life
(12-bit plane counters, every marked input steps, hidden excitatory cells learn: the
every-event job's `EV`), then the sheet is frozen and a fresh linear reader is fit. "right":
the output's share right in the last tenth of the life. "reader": the fresh reader on the
validation digits. "teach": the share of the last tenth's digits that sent any error.
`scripts/outputinside.sh ladder`, `ladder2`, `ladder3`, `ladderengine`, `ladderengine2`,
`ladderengine3`.

Every close line steps (the every-event job's rule):

| rung | one difference from the rung above it | MNIST right | reader | pairs right | reader | teach |
|---|---|---|---|---|---|---|
| born | | | 0.9259 | | 0.6756 | |
| outint2 | the start: every non-sensor cell once, 16-bit weights, spike counts, margin perceptron told the right line | 0.903 | 0.9444 | 0.712 | 0.7702 | 0.28 / 0.50 |
| fld | a line is its 102 motor cells, each reading the 704 places it can hear, sensor cells among them | 0.833 | 0.9419 | 0.647 | 0.7351 | 0.34 / 0.59 |
| fldh | (the same with no sensor cell read) | 0.869 | 0.9416 | 0.666 | 0.7534 | 0.27 / 0.56 |
| fldbit | a source is one bit: fired in the last 15 ticks | 0.871 | 0.9420 | 0.657 | 0.7484 | 0.27 / 0.58 |
| fld12 | 12-bit counters | 0.850 | 0.9430 | 0.657 | 0.7484 | 0.30 / 0.58 |
| fld8 | 8-bit counters, every event | 0.234 | 0.9250 | 0.651 | 0.7412 | 0.98 / 0.59 |
| fld8s3 | 8 bits, a step with chance 2^-3 | 0.714 | 0.9425 | 0.608 | 0.7409 | 0.48 / 0.49 |
| fld8s6 | 8 bits, counts, chance 2^-6 a spike (the engine's width and chance) | 0.761 | 0.9431 | 0.595 | 0.7329 | 0.42 / 0.50 |
| fldr1 | (from fld) told good or bad only, the engine's plain rule | 0.604 | 0.9272 | 0.498 | 0.6557 | 0.40 / 0.50 |
| fldr2 | with the gate | 0.883 | 0.9395 | 0.516 | 0.6829 | 0.22 / 0.51 |
| fldr3 | runner-up and gate | 0.878 | 0.9382 | 0.518 | 0.6740 | 0.31 / 0.51 |
| com | (from fld12) a cell votes one bit: its own sum over zero | 0.097 | 0.9260 | 0.601 | 0.7410 | 1.00 / 0.42 |
| comreg | the bar at the cell's own running mean | 0.245 | 0.9302 | 0.575 | 0.7386 | 0.88 / 0.44 |
| bit1 | (from fld12) one-bit wiring: each cell wires its top weights at its born count | 0.145 | 0.9260 | 0.623 | 0.7370 | 1.00 / 0.62 |

The engine's own readout (motor cells that spike, one-bit wiring, 8-bit chance counters),
its votes making the errors by margin (`hop_teach 5`, a quarter):

| rung | its rule | MNIST right | reader | pairs right | reader | teach |
|---|---|---|---|---|---|---|
| eng | plain (good: pressed gains, nine lose; bad: the reverse) | 0.669 | 0.9324 | 0.576 | 0.7204 | 0.62 / 0.68 |
| enggate | with the gate | 0.774 | 0.9313 | 0.575 | 0.7145 | 0.71 / 0.87 |
| engrun | runner-up and gate | 0.825 | 0.9303 | 0.575 | 0.7145 | 0.64 / 0.87 |
| engrunstr | and the top quarter strong | 0.837 | 0.9262 | 0.576 | 0.7048 | 0.64 / 0.88 |
| engrunf2 | runner-up and gate, the zone a sixteenth | 0.831 | 0.9367 | 0.586 | 0.7339 | 0.24 / 0.54 |
| engrun2 | runner-up and gate, the rule told the right button on bad steps (`hop_teach 2`) | 0.832 | 0.9411 | 0.531 | 0.7117 | 0.17 / 0.47 |
| engans | told the right line, every close wrong line's cells lose (`readout_good 3`) | 0.176 | 0.9293 | 0.592 | 0.7271 | 0.97 / 0.73 |

Only the highest close wrong line steps (and only it is named to the cells). The pair test
has two lines, so there this is the same rule and the same run as above:

| rung | what | MNIST right | reader | teach |
|---|---|---|---|---|
| outtop | outint2 so | 0.933 | 0.9566 | 0.17 |
| outtopm1, outtopm8 | outtop at a margin of 1 and of 8 steps' worth (2 above) | 0.927, 0.928 | 0.9577, 0.9554 | 0.15, 0.25 |
| fldtop | fld12 so (fields, one bit a source, 12 bits) | 0.928 | 0.9566 | 0.19 |
| fldhtop | no sensor cell read | 0.927 | 0.9538 | 0.19 |
| fld8top | 8-bit counters, every event | 0.927 | 0.9552 | 0.18 |
| fld6top | 6-bit counters, every event | 0.915 | 0.9550 | 0.27 |
| fld8s3top | 8 bits, chance 2^-3 | 0.897 | 0.9564 | 0.19 |
| comtop | a cell votes one bit (sum over zero) | 0.879 | 0.9546 | 0.14 |
| comregtop | the bar at the cell's running mean | 0.710 | 0.9459 | 0.33 |
| bit1top | one-bit wiring at the born count | 0.903 | 0.9555 | 0.33 |
| enganstop | the engine's motor cells as they are (`readout_good 4`) | 0.857 | 0.9445 | 0.44 |
| enganstop4 | the same at chance 2^-4 | 0.864 | 0.9459 | 0.42 |
| enganstopf4 | the same, the zone an eighth in spikes (a quarter above) | 0.859 | 0.9496 | 0.30 |
| enganstopf2 | the zone a sixteenth | 0.847 | 0.9519 | 0.23 |
| enganstopf4s4 | the zone an eighth, chance 2^-4 | 0.871 | 0.9506 | 0.27 |

(Two seeds: the two are within 0.006 of each other on MNIST for every row but comreg
(0.007), and within 0.016 on pairs for every row but eng (0.030). `com`, `bit1` and `engans` on pairs work because with two lines
one goes up and one comes down already. On pairs, old layout, the `enganstop` rows read
0.7271, 0.7357, 0.7327, 0.7359, 0.7360 with the output right 0.57 to 0.62.)

What it says:

- **Online accuracy is not what the cells need.** The 8-bit chance perceptron is right
  0.71 to 0.76 of the time and its cells read 0.943, the same as the 16-bit one at 0.90.
  The engine's readout under the runner-up rule is right 0.83 and its cells read 0.930,
  under born's gain by half. The brief's "right nine times in ten" was a guess from one
  pair of numbers; what separates the rows is the rule the output learns by and which
  lines it names.
- **Where the output falls.** Not at the read: the motor cells' own fields cost 0.03 to
  0.07 online with spike counts (a pixel's count of 20 swamps a cell's count of 1; one
  bit a source gets most of it back) and nothing in what the cells learn. Not much at the
  width while steps are by chance. It falls at three places, and one cause is under all
  three:
  - told only good or bad (the engine's plain rule): 0.604, and the cells learn nothing
    (0.927, born); the gate mends MNIST (0.88) and not the pair test (0.52, chance);
  - narrow counters stepped on every event: 0.234 at 8 bits;
  - a cell that votes one bit, or wires one bit a source at a fixed count: 0.10 to 0.15.
- **The cause: what all lines share drifts.** When every wrong line that is too close
  steps down and one right line steps up, more counts come off than go on, on every
  source that is often on. In a sum that is harmless until the counters hit their end
  (12 bits: the output stops rising at half the life, 0.86, and ends at 0.85 where the
  16-bit one ends at 0.87; 16-bit counts of pixels: 0.87 at seven tenths of the life,
  0.83 at its end). In anything with a bar it is fatal: a cell's sum goes under zero for
  every line and no cell votes. The engine's plain rule has the same shape from the other
  side (one button gains, nine lose). This is a reading of which rungs fail and which fix
  mends them all; no counter was printed.
- **The fix is one line down for one line up.** Let only the highest of the close wrong
  lines step (the multiclass perceptron as Crammer and Singer wrote it). Then nothing the
  lines share can move, and every rung holds: 8-bit counters stepped on every event 0.927,
  6-bit 0.915, a one-bit vote a cell 0.879, one-bit wiring at the born count 0.903, and the
  full-width outside output goes from 0.903 to 0.933.
- **And naming only those two lines to the cells is worth more than the output's
  accuracy**: the fresh reader goes from 0.944 to 0.957 at every rung that survives, above
  the every-event job's softmax output (0.951 on this slice). With the output stepping
  one line but the cells still told of every close line (`out_top 2`, below) the reader
  is back at 0.942.
- **The engine's own motor cells under that rule** (`enganstop`): right 0.86 online, reader
  0.9445. That is the smallest change that works: no new state, a new value of
  `readout_good`. Part of its distance from the counters-as-weights rows (0.93, 0.957) is
  the zone: a quarter in spikes sends an error on 44% of the digits, a sixteenth on 23%,
  and at a sixteenth the reader is 0.952 with the output no better (0.85). So the spiking
  readout, told the right line and moving two buttons, teaches the cells as well as the
  every-event job's outside softmax output did (0.951 on this slice). What is left
  between it and the counters-as-weights output is the output's own accuracy (0.86
  against 0.93) and 0.005 on the reader. The narrower zones were run on validation only;
  the report's `enganstope` is the quarter.
- **With no new code at all**: the runner-up readout with the rule told the right button
  on bad steps (`engrun2`) reads 0.941, where the every-event job had 0.935 with the plain
  readout. Two lines named, no zone.

## The closed loop (validation slice, seeds 1 and 2)

No outside part: a judged life in which the blob's own output answers, learns from the
right line, and its steps are the one-hop rule's errors (`--g.learn_mode 5 --life-teach 6
--g.hop_on 1 --g.hop_bits 12 --g.hop_shift 0 --g.out_on 1`). "own": the blob's own answer's
share right in the last tenth of the life; "frozen": its own answer on the validation
digits after the life, nothing learning; "reader": the fresh linear reader. MNIST / pairs.
`scripts/outputinside.sh loop`, `looppair`, `looppay`, `loopknobs`.

| arm | what | own | frozen | reader | teach |
|---|---|---|---|---|---|
| born | | | | 0.9259 / 0.6756 | |
| alone | the output on a sheet that does not learn; every close line steps | 0.845 / 0.548 | 0.846 / 0.561 | (born) | |
| alonee | the same, only the top wrong line steps, buttons' cells spread evenly | **0.909** / 0.617 | 0.916 / 0.636 | (born) | |
| in | the closed loop, every close line steps | 0.849 / 0.657 | 0.864 / 0.676 | 0.9412 / 0.7451 | 0.30 / 0.57 |
| inb16 | 16-bit counters | 0.868 / 0.657 | 0.877 / 0.676 | 0.9423 / 0.7451 | |
| inh | no sensor cell counted | 0.835 / 0.654 | 0.851 / 0.663 | 0.9429 / 0.7430 | |
| **intop** | only the top wrong line steps (`out_top 1`) | **0.927** / 0.657 | 0.931 / 0.676 | **0.9575** / 0.7451 | 0.18 / 0.57 |
| intop2 | the output steps one line, the cells hear of every close line (`out_top 2`) | 0.913 | 0.914 | 0.9416 | 0.22 |
| inhtop | intop, no sensor cell counted | 0.929 / 0.654 | 0.930 / 0.663 | 0.9560 / 0.7430 | |
| **intope** | intop, the buttons' cells spread evenly (`--motor-spread-even 1`) | 0.927 / **0.698** | 0.936 / 0.718 | 0.9554 / **0.7635** | 0.18 / 0.51 |
| intope, coin | the cells get another verdict's errors | 0.904 / 0.606 | 0.908 / 0.618 | 0.9401 / 0.6797 | 0.23 / 0.65 |
| intop512 | all 512 motor cells a button (pairs) | 0.701 | 0.720 | 0.7626 | |

Knobs on `intope`:

| arm | what | own | reader |
|---|---|---|---|
| intopem1, m4, m8 | margin 1, 4, 8 steps' worth (2 is the default) | 0.921, 0.929, 0.925 / 0.683, 0.713, 0.726 | 0.9568, 0.9548, 0.9524 / 0.7641, 0.7608, 0.7541 |
| intopef1, f4 | the cells' own zone 1 or 4 steps' worth, the output's 2 (`out_fine`) | 0.923, 0.921 / 0.695, 0.669 | 0.9537, 0.9460 / 0.7674, 0.7187 |
| intopew5, w10 | a source is on if it fired in the last 5 or 10 ticks (15) | 0.931, 0.933 / 0.717, 0.709 | 0.9560, 0.9566 / 0.7729, 0.7729 |
| intopeb8, b16 | 8-bit or 16-bit counters (12) | 0.928, 0.927 / 0.695, 0.698 | 0.9565, 0.9554 / 0.7656, 0.7635 |
| intope64 | the cells re-pick together every 64 verdicts | 0.929 / 0.704 | 0.9557 / 0.7624 |
| intopeall | inhibitory and pacemaker cells learn too (`hop_who 3`) | 0.933 / 0.704 | 0.9594 / 0.7701 |
| intopeallw10 | both: more learner kinds and a 10-tick window (`loopcombo`) | 0.940 / 0.714 | 0.9604 / 0.7796 |
| paytopeall, paytopew10 | payment with more learner kinds; with a 10-tick window | 0.931, 0.929 / 0.714, 0.720 | 0.9606, 0.9569 / 0.7776, 0.7750 |
| paytopeallw10 | payment with both | 0.936 / 0.716 | 0.9615 / 0.7804 |

(Two seeds. The two are within 0.006 of each other on MNIST for every arm, and within
0.013 on pairs for every arm but the coin (0.017). 16-bit counters give 12-bit's runs to
the digit when one line steps: no counter passes 12 bits.)

- **The closed loop reads 0.9554 / 0.7635 on this slice** (`intope`; 0.9575 on MNIST
  without the even layout; the every-event job's outside outputs here: softmax 0.9513 /
  0.7189, softmax at fine 0.3 0.9459 / 0.7795, whole-number 0.9444 / 0.7702), and its own
  output is right 0.927 / 0.698 of the time online.
- **Which lines step is the whole of the MNIST gain.** Every close line: 0.849 online,
  reader 0.941. The top one only: 0.927, 0.9575. The output stepping one line with the
  cells told of every close line: 0.913, 0.942. So it is what the cells are told (two
  lines: the right one and the one most confused with it) more than what the output does.
- **On the pair test the layout was the handicap.** `--motor-spread 102` gives two buttons
  the 204 motor cells nearest the top edge, and a motor cell hears its own tile and the
  eight around it, so the output read the top three tile rows of eight. Spread evenly over
  the sheet the same 204 cells read 0.698 online and the fresh reader 0.7635 (from 0.657
  and 0.7451). With ten buttons every motor cell is a button's already.
- **One zone for both tests.** Margin 1 and 2 are level on both tests (0.9568 / 0.7641,
  0.9554 / 0.7635); 4 and 8 are worse on both. The zone is counted in steps' worth, which
  scales itself with how much was on. Giving the cells a zone other than the output's own
  does not help (1: -0.002 / +0.004; 4: -0.009 / -0.045).
- **The output alone is right 0.909 of the time on a born sheet** with one line stepping
  (0.845 with every close line stepping). The engine's readout on that sheet: 0.68 plain,
  0.83 under the runner-up rule.
- **The coin reads well above born on MNIST** (0.9401 against 0.9259) and at born on pairs
  (0.6797 against 0.6756). A rule that teaches one digit in five moves the born wiring
  toward what is often on, whatever the error says (the credit-rules doc saw the same of
  its gentle rules). The margin to read is over the coin: +0.015 / +0.084.
- Width: 8-bit counters are level with 12 (0.9565 / 0.7656). A shorter window is level on
  MNIST and +0.009 on pairs (both seeds). More learner kinds: +0.004 / +0.007 (both seeds
  on both tests; the every-event job had them costing the pair test 0.012 under the softmax
  error). The two together read 0.9604 / 0.7796, and with payment 0.9615 / 0.7804: the
  every-event job's two best outside numbers on this slice (0.9513 at one fine zone, 0.7795
  at the other) from one setting inside the blob. Two seeds; these two went to the test
  digits as a second report (below).

## The report (test digits, seeds 5-12)

Arms never run on these seeds before. A 60,000-digit life, then frozen, a fresh linear
reader of the non-sensor cells fit on 60,000 digits, test digits. "coin": the same life,
the cells given another verdict's errors (drawn from the last 256; the output still learns
from its own). "shuffled": the same life on digits whose pixels are shuffled. "rate":
hidden spikes per 1,000 cell-ticks while read; "pixels": pixel inputs a hidden excitatory
cell; "own": the share right, in the last tenth of the life, of the output that makes the
errors; "frozen": the blob's own answer on the test digits after the life, nothing
learning; "teach": the share of the last tenth's digits that sent an error.
`scripts/outputinside.sh report`, `reportcells`.

- `intope`: the closed loop (`--g.out_on 1 --g.out_top 1 --g.hop_teach 6`, every-event
  counters, hidden excitatory learners, margin 2, window 15, 12 bits, the buttons' cells
  spread evenly).
- `paytope`: the same with the errors paid back along the output's own sources.
- `enganstope`: the engine's motor cells as they are under the same rule
  (`--g.readout_good 4`, zone a quarter in spikes, `hop_teach 5`).
- `alonee`: the blob's own output on a sheet in which nothing else learns.
- `outtop`: the same rule as an outside output reading every non-sensor cell's spike count
  with 16-bit weights. On the pair test it is the every-event job's `outint2`, and its
  eight runs match that job's to the digit.

MNIST:

| arm | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate | pixels | own | frozen | teach |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| born | 0.9248 | 0.9272 | 0.9273 | 0.9239 | 0.9255 | 0.9192 | 0.9249 | 0.9278 | **0.9251** | 33.1 | 7.2 | | | |
| intope | 0.9553 | 0.9563 | 0.9577 | 0.9568 | 0.9551 | 0.9557 | 0.9555 | 0.9548 | **0.9559** | 38.7 | 5.9 | 0.925 | 0.931 | 0.19 |
| intope, coin | 0.9359 | 0.9364 | 0.9401 | 0.9383 | 0.9344 | 0.9364 | 0.9396 | 0.9393 | 0.9375 | 38.4 | 5.7 | 0.903 | 0.911 | 0.24 |
| intope, shuffled | 0.9259 | 0.9271 | 0.9247 | 0.9250 | 0.9265 | 0.9229 | 0.9276 | 0.9250 | 0.9256 | 32.9 | 7.4 | 0.179 | 0.163 | 0.98 |
| paytope | 0.9539 | 0.9553 | 0.9547 | 0.9580 | 0.9542 | 0.9542 | 0.9593 | 0.9560 | **0.9557** | 37.0 | 6.3 | 0.924 | 0.929 | 0.19 |
| paytope, coin | 0.9388 | 0.9365 | 0.9395 | 0.9382 | 0.9385 | 0.9367 | 0.9367 | 0.9424 | 0.9384 | 38.4 | 5.2 | 0.902 | 0.909 | 0.25 |
| enganstope | 0.9438 | 0.9456 | 0.9478 | 0.9459 | 0.9426 | 0.9447 | 0.9465 | 0.9462 | **0.9454** | 38.8 | 4.4 | 0.857 | | 0.43 |
| enganstope, coin | 0.9325 | 0.9329 | 0.9372 | 0.9348 | 0.9368 | 0.9353 | 0.9363 | 0.9373 | 0.9354 | 36.5 | 5.7 | 0.852 | | 0.49 |
| alonee | 0.9238 | 0.9284 | 0.9240 | 0.9237 | 0.9248 | 0.9236 | 0.9288 | 0.9268 | 0.9255 | 33.0 | 7.2 | 0.906 | 0.914 | |
| outtop | 0.9585 | 0.9567 | 0.9596 | 0.9564 | 0.9566 | 0.9578 | 0.9595 | 0.9554 | **0.9576** | 40.0 | 6.1 | 0.928 | | 0.18 |

Pair test:

| arm | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate | pixels | own | frozen | teach |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| born | 0.6924 | 0.6724 | 0.6675 | 0.6784 | 0.6760 | 0.6838 | 0.6813 | 0.6675 | **0.6774** | 32.7 | 7.2 | | | |
| intope | 0.7654 | 0.7649 | 0.7583 | 0.7656 | 0.7566 | 0.7564 | 0.7606 | 0.7471 | **0.7594** | 33.5 | 5.9 | 0.708 | 0.715 | 0.50 |
| intope, coin | 0.6821 | 0.6752 | 0.6720 | 0.6883 | 0.6796 | 0.6857 | 0.6817 | 0.6739 | 0.6798 | 35.1 | 5.8 | 0.609 | 0.607 | 0.66 |
| intope, shuffled | 0.6763 | 0.6742 | 0.6688 | 0.6703 | 0.6667 | 0.6705 | 0.6778 | 0.6539 | 0.6698 | 32.9 | 7.5 | 0.502 | 0.500 | 0.78 |
| paytope | 0.7644 | 0.7668 | 0.7683 | 0.7726 | 0.7582 | 0.7669 | 0.7622 | 0.7542 | **0.7642** | 31.3 | 5.3 | 0.714 | 0.718 | 0.49 |
| paytope, coin | 0.6938 | 0.6925 | 0.6808 | 0.6883 | 0.6875 | 0.6994 | 0.7002 | 0.6882 | 0.6913 | 36.4 | 4.9 | 0.606 | 0.612 | 0.65 |
| enganstope | 0.7444 | 0.7268 | 0.7252 | 0.7448 | 0.7214 | 0.7309 | 0.7391 | 0.7224 | **0.7319** | 33.0 | 5.4 | 0.624 | | 0.72 |
| enganstope, coin | 0.6857 | 0.6779 | 0.6718 | 0.6889 | 0.6803 | 0.6759 | 0.6878 | 0.6737 | 0.6803 | 35.4 | 5.9 | 0.518 | | 0.94 |
| alonee | 0.6828 | 0.6711 | 0.6688 | 0.6799 | 0.6773 | 0.6803 | 0.6775 | 0.6677 | 0.6757 | 32.6 | 7.2 | 0.620 | 0.628 | |
| outtop (= outint2) | 0.7750 | 0.7663 | 0.7653 | 0.7819 | 0.7631 | 0.7651 | 0.7672 | 0.7580 | **0.7677** | 34.5 | 6.4 | 0.721 | | 0.48 |

Seed by seed:

| difference | MNIST | seeds above | range | pairs | seeds above | range |
|---|---|---|---|---|---|---|
| intope - born | +0.0308 | 8 of 8 | +0.0270 to +0.0365 | +0.0819 | 8 of 8 | +0.0726 to +0.0925 |
| intope - its coin | +0.0183 | 8 of 8 | +0.0155 to +0.0207 | +0.0795 | 8 of 8 | +0.0707 to +0.0897 |
| intope - shuffled | +0.0303 | 8 of 8 | +0.0279 to +0.0330 | +0.0896 | 8 of 8 | +0.0828 to +0.0953 |
| paytope - born | +0.0306 | 8 of 8 | +0.0274 to +0.0350 | +0.0868 | 8 of 8 | +0.0720 to +0.1008 |
| paytope - its coin | +0.0173 | 8 of 8 | +0.0136 to +0.0226 | +0.0729 | 8 of 8 | +0.0620 to +0.0875 |
| paytope - intope | -0.0002 | 3 of 8 | -0.0030 to +0.0038 | +0.0048 | 7 of 8 | -0.0010 to +0.0105 |
| enganstope - born | +0.0203 | 8 of 8 | +0.0171 to +0.0255 | +0.0545 | 8 of 8 | +0.0454 to +0.0664 |
| enganstope - its coin | +0.0100 | 8 of 8 | +0.0058 to +0.0127 | +0.0516 | 8 of 8 | +0.0411 to +0.0587 |
| intope - enganstope | +0.0105 | 8 of 8 | +0.0086 to +0.0125 | +0.0275 | 8 of 8 | +0.0208 to +0.0381 |
| intope - outtop | -0.0017 | 1 of 8 | -0.0040 to +0.0004 | -0.0084 | 0 of 8 | -0.0163 to -0.0014 |
| intope - the outside softmax output (`out` on MNIST, `outf3` on pairs; every-event doc) | +0.0051 | 8 of 8 | +0.0042 to +0.0071 | -0.0108 | 0 of 8 | -0.0191 to -0.0011 |
| intope - the outside softmax output, every cell kind learning (`outall`) | +0.0012 | 5 of 8 | -0.0016 to +0.0055 | | | |
| intope - the outside whole-number output (`outint2`) | +0.0145 | 8 of 8 | +0.0119 to +0.0164 | -0.0084 | 0 of 8 | -0.0163 to -0.0014 |
| alonee - born | +0.0004 | 3 of 8 | -0.0033 to +0.0044 | -0.0017 | 4 of 8 | -0.0096 to +0.0015 |

Beside them (8 seeds each, their own docs, the same test and reader): the written-in
every-event wiring 0.9607 / 0.7607; the perfect teacher's wiring 0.9663 / 0.7853;
competitive learning 0.952 / 0.726.

- **MNIST: 0.9559, no outside part.** Every seed is between 0.9548 and 0.9577. It is over
  the every-event job's best outside output with these learners on every seed (+0.005),
  level with that job's best MNIST number of all (`outall`, 0.9547, which reads 0.686 on
  pairs), and 0.005 under the written-in sheet. The gain over born is +0.031 against the
  written-in sheet's +0.037.
- **Pair test: 0.7594.** Over born by +0.082 and over its coin by +0.080 on every seed.
  It is 0.011 under the outside softmax output at that output's own pair setting and 0.008
  under the outside whole-number output, on every seed, and level with the written-in
  sheet. The same rule outside (`outtop`) reads every cell's spike count; the output
  inside reads one bit a source through the motor cells' fields. On validation the pair
  test liked a 10-tick window better (+0.009) and counts a little better than one bit;
  neither is in this report.
- **The output is right 0.925 / 0.708 online** in the loop (0.931 / 0.715 frozen on the
  test digits), and 0.906 / 0.620 alone on a sheet that does not learn: the cells it
  teaches make it better by 0.02 / 0.09. The fresh linear reader of the same sheet gets
  0.956 / 0.759, so on MNIST the output now reads 0.025 under what a reader fit offline
  with floats gets (it was 0.26 under), and on pairs 0.044 under.
- **The coin reads over born on MNIST** (+0.012) and at born on pairs (+0.002); a
  shuffled-digit life ends at born (+0.0005 / -0.008). The every-event job's softmax coin
  was +0.007. The reading: a rule that teaches one digit in five and steps only sources
  that were on moves each cell's fixed number of inputs toward sources that are on while
  it is near its bar, whatever the bit says (finding 47; the credit-rules doc's gentle
  coins). The margin to read is the one over the coin: +0.018 / +0.080.
- **The smaller change** (the motor cells as they are, `readout_good 4`): 0.9454 / 0.7319,
  every seed over born and over its coin, the output right 0.857 / 0.624 online. Over the
  outside whole-number output of the every-event job on MNIST (+0.004, 7 of 8) with an
  output that is right less often. The counters-as-weights output is over it by 0.011 /
  0.028 on every seed.

### A second report: the knobs validation liked (test digits, seeds 5-12)

After the report above had been read, the validation screen of the knobs came in (previous
section): a 10-tick window and inhibitory and pacemaker learners each read a little better
on both tests, and together 0.9604 / 0.7796 on the two validation seeds. That one setting
(`intopeallw10`: `intope` with `--g.out_window 10 --g.hop_who 3`) and the same with payment
(`paytopeallw10`) were then run on the test digits, seeds 5-12. They were picked on the
validation slice alone, but after the first report was known. `scripts/outputinside.sh
report2`.

| arm | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate | pixels | own | frozen | teach |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| MNIST: intopeallw10 | 0.9607 | 0.9615 | 0.9633 | 0.9599 | 0.9596 | 0.9614 | 0.9627 | 0.9610 | **0.9613** | 39.8 | 6.1 | 0.936 | 0.942 | 0.16 |
| its coin | 0.9427 | 0.9405 | 0.9441 | 0.9440 | 0.9422 | 0.9454 | 0.9409 | 0.9469 | 0.9433 | 38.2 | 5.7 | 0.903 | 0.912 | 0.24 |
| paytopeallw10 | 0.9621 | 0.9622 | 0.9612 | 0.9601 | 0.9616 | 0.9632 | 0.9602 | 0.9624 | **0.9616** | 38.1 | 6.8 | 0.934 | 0.941 | 0.17 |
| pairs: intopeallw10 | 0.7798 | 0.7797 | 0.7675 | 0.7819 | 0.7737 | 0.7763 | 0.7763 | 0.7705 | **0.7757** | 34.3 | 6.4 | 0.727 | 0.738 | 0.47 |
| its coin | 0.6822 | 0.6874 | 0.6825 | 0.6774 | 0.6767 | 0.6710 | 0.6859 | 0.6778 | 0.6801 | 36.0 | 6.0 | 0.606 | 0.608 | 0.65 |
| paytopeallw10 | 0.7752 | 0.7797 | 0.7786 | 0.7892 | 0.7734 | 0.7716 | 0.7746 | 0.7765 | **0.7773** | 31.2 | 5.8 | 0.727 | 0.734 | 0.47 |

| difference | MNIST | seeds above | range | pairs | seeds above | range |
|---|---|---|---|---|---|---|
| intopeallw10 - born | +0.0362 | 8 of 8 | +0.0332 to +0.0422 | +0.0983 | 8 of 8 | +0.0874 to +0.1073 |
| intopeallw10 - its coin | +0.0179 | 8 of 8 | +0.0141 to +0.0218 | +0.0956 | 8 of 8 | +0.0850 to +0.1053 |
| intopeallw10 - intope | +0.0054 | 8 of 8 | +0.0031 to +0.0072 | +0.0164 | 8 of 8 | +0.0092 to +0.0234 |
| intopeallw10 - the same rule outside (`outtop`) | +0.0037 | 8 of 8 | +0.0022 to +0.0056 | +0.0080 | 7 of 8 | +0.0000 to +0.0134 |
| intopeallw10 - the outside softmax output (`out` on MNIST, `outf3` on pairs) | +0.0105 | 8 of 8 | +0.0088 to +0.0127 | +0.0055 | 6 of 8 | -0.0061 to +0.0146 |
| intopeallw10 - the outside softmax output, every cell kind learning (`outall`) | +0.0065 | 8 of 8 | +0.0041 to +0.0111 | | | |
| paytopeallw10 - intopeallw10 | +0.0004 | 6 of 8 | -0.0025 to +0.0020 | +0.0016 | 3 of 8 | -0.0047 to +0.0111 |

- **0.9613 / 0.7757 from one setting**, no outside part: over each of the every-event
  job's outside outputs at that output's own best setting (MNIST +0.011 and +0.007 on
  every seed; pairs +0.006, 6 of 8), at the written-in sheet on MNIST (0.9607) and over it
  on pairs (0.7607), and 0.005 / 0.010 under the perfect teacher's wiring (0.9663 /
  0.7853). The gain over born is +0.036 / +0.098; over the coin +0.018 / +0.096.
- The blob's own output is right 0.936 / 0.727 online and 0.942 / 0.738 on the test
  digits afterwards.
- More learner kinds help both tests here. The every-event job had inhibitory and
  pacemaker learners at +0.003 / -0.012 on validation under the softmax error, which
  names more lines.
- Payment is level with the hashed wires at this setting (+0.0004 / +0.0016).
- It was not run with its own shuffled life or at matched rate; the first report's arm
  was.

## Payment along the output's own sources

The credit-rules doc found, in the model, that sending each line's error back along the
output's own read wires by the sign of the read weight (pay1) beats the hashed wires
(+0.004 / +0.012, written in), and left open whether that holds along a read like the
engine's. With the output inside the blob the read wires are there: each motor cell's
counters over the sources it can hear (`--g.out_pay 1`). A line told "higher" pays each
source that was on with a counter above zero and charges each with one below; a line told
"lower" the reverse; a cell paid by more motor cells than charged it is told "more". The
one-hop rule's marks and its gate (near its bar, or fired) stay; the hashed wires are not
used at all.

| | MNIST | pairs | seeds above |
|---|---|---|---|
| hashed wires (`intope`) | 0.9559 | 0.7594 | |
| payment (`paytope`) | 0.9557 | 0.7642 | |
| payment - hashed wires | -0.0002 | +0.0048 | 3 of 8 / 7 of 8 |
| payment over its coin | +0.0173 | +0.0729 | 8 of 8 / 8 of 8 |
| at the second report's setting: `paytopeallw10` - `intopeallw10` (0.9616 / 0.7773 against 0.9613 / 0.7757) | +0.0004 | +0.0016 | 6 of 8 / 3 of 8 |
| on validation (2 seeds): the old layout, `paytop` - `intop` | -0.0023 | +0.0148 | 0 of 2 / 2 of 2 |
| on validation: the even layout, `paytope` - `intope` | +0.0016 | +0.0066 | 2 of 2 / 1 of 2 |

- **Level on MNIST, a little better on pairs** (+0.005, 7 of 8 seeds) at the first
  setting, and level on both at the second, where the hashed wires already read 0.776 on
  pairs. At best half the model's lead on pairs and none of it on MNIST. The cells fire
  a little less under it (37.0 / 31.3 against 38.7 / 33.5) and its output is right as
  often (0.924 / 0.714).
- **It works with no wires of its own.** The hashed wires were the one part of the
  one-hop rule that came from nowhere; here the error returns along counters the output
  already keeps, for about 60,000 word operations a verdict that teaches.
- **Its coin is higher** (0.9384 / 0.6913 against 0.9375 / 0.6798): another verdict's two
  lines paid along this digit's sources still tells each cell which lines read it, which
  is not nothing. So its margin over its own coin is the smaller one on pairs (+0.073
  against +0.080).
- **With every close line stepping, payment wrecks MNIST** (`pay`, validation: 0.9095,
  the output right 0.73, 1.9 pixel inputs a cell): many lines charging at once is the
  every-event job's "the answer on every digit" again.
- Not tried: payment by the size of the counter, passing it back a second step, the
  gate "fired" alone.

## The rate held at target (MNIST, seeds 5-8)

The closed loop's cells fire at 38.7 per 1,000 cell-ticks while read against born's 33.1,
so the same arms were run with the threshold regulation uncapped (`--g.threshold_cap
1000`, as the every-event doc's `cap`), which holds every sheet at 31.
`scripts/outputinside.sh cap`.

| arm | 5 | 6 | 7 | 8 | mean | rate | own |
|---|---|---|---|---|---|---|---|
| born | 0.9239 | 0.9238 | 0.9191 | 0.9217 | 0.9221 | 31.3 | |
| intope | 0.9543 | 0.9550 | 0.9520 | 0.9531 | **0.9536** | 31.1 | 0.928 |
| intope, coin | 0.9296 | 0.9294 | 0.9293 | 0.9270 | 0.9288 | 31.1 | 0.899 |

At matched rate the gain over born is +0.0315 (4 of 4; +0.031 free) and over the coin
+0.025 (4 of 4; +0.018 free): the coin loses half of its lift over born when it cannot
fire more (+0.007 from +0.012), the real errors lose nothing. The pair arms fire at 33.5
against 32.7 and need no such check.

## The toys

Scored as the every-event doc scores them: a 40,000-step life, the inside frozen, a fresh
output's late score (first quarter in brackets). "old": that doc's every-event arm (the
old readout answers in the life and the rule is told the right button on bad steps). "in":
the blob's own output answers in the life and its steps teach the cells (`--g.out_on 1
--g.out_top 1 --g.hop_teach 6`). The fresh output afterwards is the old readout in both,
so the column compares what the cells learned. "life": the answering output's own late
score during the life. Seeds 5-12. `scripts/outputinside.sh toys`.

| the life | the fresh output afterwards | xor | parity of 3 | the life's own late score |
|---|---|---|---|---|
| none (born) | the old readout | 0.855 (0.685) | 0.552 (0.531) | |
| old: the old readout answers, the rule told the right button on bad steps | the old readout | 0.999 (0.981) | **0.919** (0.840) | 0.999 / 0.909 |
| **in: the blob's own output answers and teaches** | the old readout | 0.995 (0.966) | **0.758** (0.656) | 1.000 / 0.867 |
| the same, the cells given another verdict's errors | the old readout | 0.916 (0.776) | 0.570 (0.546) | 0.945 / 0.674 |
| none (born) | an output of the new kind | 0.941 (0.728) | 0.719 (0.547) | |
| in | an output of the new kind | **1.000** (0.995) | **0.879** (0.750) | 1.000 / 0.867 |
| the same, another verdict's errors | an output of the new kind | 0.958 (0.805) | 0.727 (0.555) | 0.945 / 0.674 |

(The first two rows are the every-event doc's to the digit. Seed ranges for parity of 3:
old 0.895 to 0.957; in, read by the old readout, 0.679 to 0.815; in, read by the new
output, 0.793 to 0.953.)

- **Xor: nothing lost.** The blob's own output answers 1.000 late in the life (the old
  readout 0.999) and the cells it teaches read 0.995 by the old readout (0.999).
- **Parity of 3: the new loop teaches the cells less than the old readout did.** Read by
  the same fresh old readout the taught sheet scores 0.758 against 0.919 (8 of 8 seeds
  lower), though it is far over born (0.552) and over its coin (0.570), and the new
  output's own score in the life is 0.867 against the old readout's 0.909. Read by a
  fresh output of the new kind the same sheet scores 0.879 (a born sheet: 0.719).
- A first look at why (`scripts/outputinside.sh toys2`, the same eight seeds, read by the
  old readout): a wider margin does not mend it (8: 0.751; 32: 0.707), nor does leaving
  sensor cells out of the read (0.747). So it is not that the cells hear too few errors.
  Not found. One thing the two arms differ in was not tested: the old readout's answer
  was spikes of motor cells, which the hidden cells hear and can build on; the new
  output's answer is a number nothing in the sheet hears.
- The blobprobe keeps the life's genome numbers for the fresh output, so the last three
  rows read with the new output and the middle ones needed `--after "out_on 0"`.

## Two rows of the quick map that lean on the output

`scripts/oneset.sh`'s rows echo16 (sixteen cues, sixteen buttons) and mnist (20,000 digits
seen once, the online score), 20,000 steps, seeds 5-12, each life beside its learning-off
twin. "base" is the map's own column, the engine's readout. `scripts/outputinside.sh map`.

| column | echo16 late | twin | of the way to perfect | mnist late | twin | of the way |
|---|---|---|---|---|---|---|
| base (`--g.learn_mode 5 --g.readout_evidence 2 --g.readout_shift 4 --g.readout_balance 1`) | 0.266 ±0.034 | 0.048 | 0.22 | 0.214 ±0.010 | 0.092 | 0.14 |
| the blob's own output (`--g.learn_mode 5 --g.out_on 1 --g.out_top 1`) | **0.966** ±0.021 | 0.061 | 0.96 | **0.893** ±0.008 | 0.099 | 0.88 |
| the closed loop (and `--g.hop_on 1 --g.hop_bits 12 --g.hop_shift 0 --g.hop_teach 6`) | **0.997** ±0.001 | 0.061 | 1.00 | **0.903** ±0.006 | 0.099 | 0.89 |

Under the base column neither row passes; both pass here (a row passes at halfway from
chance to perfect and over its twin). The output-gap doc's best readout rule had sixteen
cues at 0.72 on a 40,000-step life. The comparison is not even: the old readout hears
good and bad, this output is told the right button. The rest of the map was not run, and
every row whose task gives no right button (the bandits, the cart, catch) would leave this
output at its twin.

One thing did break on the way and is fixed: on tasks that give no verdict for "no move"
(the toys, the map; MNIST calls it wrong) a newborn output, every vote zero, tied, made no
move, was never told anything and stayed at zero for the whole life (xor: 0.000). With
`out_on` a tie now goes to the lowest line (`plug.tie_low`). The first report ran before
that change, so its main arm was run again with the final code, all 16 lives
(`scripts/outputinside.sh recheck`): every line of every life is the same, the fresh
reader's score included, but for one count in two pair lives (seeds 5 and 9), where the
newborn output's first answer, once a tie and no move, is now line 0 and was right. The
second report, the map rows, the toys and the speed table ran with the final code; the
rate-held check ran across the change (MNIST lives, which it does not alter).

## Speed

One life at a time on this PC (the governor and other agents' jobs make any number here
rough), 20,000-digit MNIST lives, ticks a second over the life. The frozen sheet runs at
about 58,000. `scripts/outputinside.sh speed --jobs 1`.

| life | ticks/s |
|---|---|
| the engine's readout, plain rule (`eng`) | 5,730 |
| runner-up rule and gate (`engrun`) | 12,790 |
| the motor cells told the right line, two buttons move (`enganstop`) | 22,990 |
| the every-event job's outside whole-number output (`outint2`) | 39,370 |
| the blob's own output alone, nothing else learning (`alonee`) | 49,590 |
| **the closed loop (`intope`)** | **38,360** |
| the closed loop with payment (`paytope`) | 38,290 |

- The output's read and steps cost about 3 microseconds a tick over the frozen sheet
  (49,590 against 58,000), 60 a digit.
- The closed loop runs 6.7 times as fast as a judged life under the old readout and at
  the speed of the outside whole-number life; what it spends is the one-hop rule's own
  every-event work, as the every-event doc measured.
- Payment costs nothing that shows.
- Measured in one MNIST life (seed 5): the output moved on 15,398 of 60,000 verdicts, two
  lines each; the rule taught 452 cells a verdict that moved, 86 counter steps a taught
  cell, 598 million steps in the life, and 1.7 million re-picks changed a cell's wiring.
  On the pair test it moved on 34,581 verdicts.
- `blobbench` (the tick alone, every new number off, 65,536 neurons, learning on, three
  runs each, alternating): 2,961 to 3,030 ticks a second on main's build, 2,986 to 3,098
  on this one. The tick gained two tests of a genome number.

## What is real, what is suspect

- Real: the report (8 seeds, both tests; the closed loop, payment and the smaller change
  each over born and over its coin on every seed; the closed loop over its shuffled life
  on every seed). Born's eight numbers on each test and `outtop`'s eight on the pair test
  equal the every-event job's to the digit.
- Real, and a loss: parity of 3 (8 seeds, the same fresh reader, every seed lower than
  under the every-event arm). Real: xor, and the two map rows (8 seeds each).
- **The output is told the right line at every verdict.** So was the one-hop rule
  (`hop_teach 2`) and so were the outside outputs. The engine's old readout learns from
  good and bad alone; this one cannot. On a task with no answer line it does nothing.
- **The answer is not spikes.** The world reads ten numbers the blob computes at the end
  of a step. The motor cells still fire as born cells and the fresh reader reads them as
  such, but the answer does not pass through their firing. `readout_good 4` keeps the
  answer in spikes and reads 0.011 / 0.028 lower.
- **The pair test's layout was changed after seeing validation.** With two buttons
  `--motor-spread 102` takes the 204 motor cells nearest the top edge. The report uses
  `--motor-spread-even 1` (the same count spread over the sheet) on both tests. It changes
  nothing for born. On MNIST, where ten buttons take nearly every motor cell either way,
  validation read 0.9575 without it and 0.9554 with it; on the pair test 0.745 without and
  0.7635 with.
  `enganstope` was run on test with that layout without a validation run of its own (the
  old layout: 0.9445 / 0.7271 on validation).
- **What made the difference was found on the validation slice with two seeds a rung**:
  the top-line-only step, and that it is the cells being told two lines that matters.
  Then one arm was fixed and run on the test digits. The knobs that looked better on
  validation (a 10-tick window, more learner kinds) went into a second report, chosen
  by validation numbers but after the first report was known. Its controls are born and
  a coin; it has no shuffled life and no matched-rate run of its own.
- **The coin is not at born on MNIST** (+0.012), so "over born" flatters every arm here by
  about that. The numbers to read are over the coin.
- **"What all lines share drifts" is a reading.** What was run: every form with a bar
  fails when every close line steps and holds when one does; 12-bit and 16-bit sums fall
  late in the life when every close line steps and not when one does. I did not print a
  counter.
- The engine's rungs of the ladder ran on stevejobs, the outside rungs here; results are
  byte-identical between the two for this engine (AGENTS.md), and I did not re-run one on
  both.
- The copies of one-bit votes and one-bit wiring in the ladder are mine, outside the
  blob: a cell's vote is its sum over zero, with no leak, cooldown, delays or regulation.
  The output-gap doc found such a copy 0.1 off the real cells under the gate. They say
  the rule is the cause; they do not say what the real cells would score, which is
  `enganstop`'s 0.86.
- The frozen "own answer" column means something only for `out_on` arms (after other
  judged lives the motor cells are put back as born).
- Speeds are from one PC under a temperature governor with other agents' jobs on it.

## Not done

- **An output that learns from good and bad alone.** This one is told the right line, as
  the one-hop rule already was (`hop_teach 2`). On a task that gives only good or bad it
  never moves. The ladder's good-and-bad rules on the same read do learn MNIST (0.88) and
  do not learn the pair test (chance).
- **Spikes.** The answer is a sum of counters read at the verdict; no motor cell fires it
  and nothing in the sheet hears it. The ladder says a cell voting one bit (its sum over
  zero) keeps 0.88 online and 0.955 on the reader once one line steps, which would be a
  verdict spike a cell; it was run outside the blob only.
- **The smaller change at its better zone on the test digits.** `readout_good 4` with the
  zone a sixteenth reads 0.952 / 0.736 on validation (two seeds); the report ran the
  quarter (0.945 / 0.732).
- **Pattern answers** (the calc battery): the output handles a pair of lines an answer bit
  and it was not run there.
- **Why parity of 3 loses** under the new loop (0.758 against 0.919 by the same fresh
  reader). A wider margin and a read without sensor cells were tried and do not mend it.
  Parity of 4; the calc columns; the rest of the quick map.
- A blobbench of the new read by itself: it is not on the tick's path, and its cost is in
  the speed table as whole-life ticks a second.
- Counts in place of one bit a source in the engine (the ladder: counts are worth 0.01 on
  the pair test's reader, nothing on MNIST once sensor cells are one bit).
- A shuffled-digit life and a matched-rate run for the second report's setting; that
  setting on the toys and the map; longer lives; a second pass.
- Why the coin of a rule that teaches one digit in five reads 0.012 over born on MNIST.
- Payment's second step, wealth, or any of the credit-rules doc's other rules.
- `docs/design.md`: not edited (the coordinator's at merge); the text for it is below.

## Code

- `src/blob/learn_output.cpp`, `output_state.hpp` (new): the blob's own output: the cells'
  lists, the read, the verdict, the step, the errors for the rule, payment.
- `src/blob/genome.hpp`, `genome_args.cpp`: `out_on`, `out_bits`, `out_margin`,
  `out_window`, `out_fine`, `out_pay`, `out_top`, `out_hidden`, at the end, all off or
  unused by default. `hop_teach 6` and `readout_good 3`, `4` are new values of old numbers.
- `src/blob/blob.hpp`: `motor_line`, `motor_line_set`, `out_read`, `out_lines`, `out_vote`,
  `output`; `blob_motor_line` (inline), `blob_output_verdict`, `blob_output_counts`.
  `birth.cpp` makes and frees `motor_line` while `out_on` or `readout_good` 3 or 4.
- `src/blob/tick.cpp`: the read after the fire step when asked; the step in the learning
  step; with `out_on` the readout's own rule is skipped. `learn_hop.cpp`, `hop_state.hpp`:
  the payment path (`out_pay`). `hop_lines.cpp`: `hop_teach 6`, `hop_margin_lines`, the top
  line for `readout_good 4`. `learn_readout.cpp`: `readout_good 3`, `4`.
- `src/world/plug_output.cpp` (new), `plug.cpp`, `plug.hpp`, `plug_hop.cpp`;
  `src/run/task_ticker.cpp`: the world's side, plain field reads and writes, so the
  stand-in engines link as before.
- `src/mnist/mnist_outside_cells.cpp`, `mnist_outside.hpp` (new), `mnist_outside.cpp`,
  `mnist_ceiling.cpp` (the flags; one new line, the blob's own answer on the scored digits),
  `mnist_wiring.cpp` (the output's counts in a life's lines).
- `scripts/outputinside.sh`, `outputinside.py` (reuse `everyevent.py`, `hop.py`, `hop2.py`,
  `inside.py`, `oneset.py`). `out/outputinside/`: `refcheck.sh`, `hopcheck.sh`,
  `listing.py`, `mdreport.py`, the raw outputs.
- Checks, all on the last code: a clean build with no warnings from this tree's code; the
  four reference lives byte-identical with every new number off
  (`out/outputinside/refcheck.sh`: "all four reference lives byte-identical"); six short
  lives under the every-event job's settings (its outside softmax and whole-number
  outputs, the readout's votes by margin, every-event from the answer with its coin, the
  old counters with `hop_coin 3`, a toy told the right button) the same as main's build
  line for line (`out/outputinside/hopcheck.sh`); form 4 with every option off gives form
  3's life to the digit; `blobbench` unchanged (above); the report's main arm run again
  with the final code (above); `scripts/outputinside.sh smoke --quick` end to end.
- Every file is under 500 lines (`plug.cpp` 477, `learn_hop.cpp` 459).
- Left in the tree and found not to help: `out_fine` (the cells' own zone), `out_hidden`,
  `out_top 2`, `readout_good 3`, `--outside-vote 3`. They are a few lines each and off;
  the coordinator may want them out.

## Script lines

Outputs in `out/outputinside/NAME/` (the ladder's and the loop's recipes share a folder
each); a finished run is kept, so a line run again only runs what is missing. On stevejobs
the same lines ran with `THONK_MNIST=$HOME/thonk-hop2/data/mnist` in `thonk-outputinside`
(the engine's rungs, `reportcells`, the toys).

```
scripts/outputinside.sh ladder            the outside rungs, validation, seeds 1, 2
scripts/outputinside.sh ladder2           no sensor cells; only the top wrong line steps; the bar at a running mean
scripts/outputinside.sh ladder3           with the top line only: narrow counters, one-bit votes, one-bit wiring
scripts/outputinside.sh ladderengine      the engine's readout under its own rules
scripts/outputinside.sh ladderengine2     the engine's motor cells told the right line (readout_good 3, 4); told on bad steps
scripts/outputinside.sh ladderengine3     readout_good 4 with a narrower zone
scripts/outputinside.sh loop              the blob's own output alone and in the closed loop
scripts/outputinside.sh looppair          the pair test with the buttons' cells spread evenly
scripts/outputinside.sh looppay           payment along the output's own sources
scripts/outputinside.sh loopknobs         margin, the cells' zone, window, width, re-pick every 64, who learns
scripts/outputinside.sh loopcombo         the knobs that looked better, together
scripts/outputinside.sh report            the closed loop, payment, the output alone, the rule outside: test, seeds 5-12
scripts/outputinside.sh reportcells       the motor cells as they are under the rule: test, seeds 5-12
scripts/outputinside.sh cap               the rate held at target, MNIST, seeds 5-8
scripts/outputinside.sh toys              xor and parity of 3, seeds 5-12
scripts/outputinside.sh toys2             parity of 3 with a wider margin, with no sensor cell read
scripts/outputinside.sh report2           the setting validation picked afterwards, and it with payment: test, seeds 5-12
scripts/outputinside.sh recheck           the first report's main arm again with the final code
scripts/outputinside.sh map               echo16 and the MNIST row of the quick map, seeds 5-12
scripts/outputinside.sh speed --jobs 1    ticks a second, one life at a time
scripts/outputinside.sh report --table-only     print a finished table
scripts/outputinside.sh smoke --quick     one seed, short lives, every part
out/outputinside/refcheck.sh              the four reference lives against main's
out/outputinside/hopcheck.sh              six lives under the every-event job's settings against main's build
python3 out/outputinside/mdreport.py report 5-12 born intope intope-coin     a table as markdown
```

Here a closed-loop life with its read is a little over a minute with three at a time, a
life under the old readout five. One closed-loop life, both tests:

```
scripts/outputinside.sh report --arms intope --seeds 5 --coins none --shuffled none
```

The closed loop's own flags, for any blobrun or blobmnist line: `--g.learn_mode 5
--g.hop_on 1 --g.hop_bits 12 --g.hop_shift 0 --g.hop_teach 6 --g.out_on 1 --g.out_top 1`
(with few buttons also `--motor-spread-even 1`).

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, "Where it stands" and findings 48 and 52: **the output was
  never short of accuracy, it had the wrong rule: it moved every button at every verdict.**
  Told the right line and moving two lines only (the right one up, the highest wrong line
  that was too close down), the blob's own output is right 0.925 of the time during an
  MNIST life and the loop it closes with the one-hop rule reads 0.956 / 0.759 at the
  setting fixed before the test digits, 0.961 / 0.776 at the one validation picked after
  (born 0.925 / 0.677; the outside outputs 0.951 / 0.770; written in 0.961 / 0.761; the
  backprop teacher's wiring 0.966 / 0.785). No outside part. [solid for what it measures:
  8 seeds, two tests, coin and shuffled controls on the first setting, born and coin on
  the second; the output is a sum of counters read at the verdict, not spikes, and it is
  told the right line]
- New finding: **one line down for one line up.** When every wrong line that is too close
  steps down, more counts come off than go on, and what all lines share drifts: 12-bit
  counters reach their end, 8-bit ones collapse (0.23 online), and any cell with a bar (a
  one-bit vote, one-bit wiring at a fixed count, the engine's spiking motor cells) goes
  silent (0.10 to 0.18). With only the top wrong line stepping every one of them holds:
  8-bit counters 0.927, 6-bit 0.915, one-bit votes 0.879, one-bit wiring 0.903, the
  engine's own motor cells 0.86 (and their cells read 0.945 / 0.732 on test; 0.952 on
  validation with the zone a sixteenth). This is the output-gap doc's "with many buttons a good
  should move only the competitors", for every verdict and with the answer given.
  [likely: validation, 2 seeds a rung, every rung the same way; the engine's cells 8 seeds]
- New finding: **what the cells must be told is two lines.** The right line and the one
  line most confused with it, on the digits the output has too close (one in five on
  MNIST): fresh reader 0.957 on validation. Every close line: 0.942. The same two lines
  from an outside output: 0.957. The every-event job's "right nine times in ten" is not
  the condition: an 8-bit chance perceptron right 0.71 of the time teaches the cells as
  well as a 16-bit one right 0.90 (0.943, 0.944), and the engine's readout right 0.83
  under the runner-up rule teaches them nothing over the plain one (0.930, 0.932).
  [likely: validation, 2 seeds; the closed loop 8 seeds on test]
- The every-event finding ("only when its error comes from an output that is right about
  nine times in ten"): reword. It needs an output that learns by the errors it sends and
  names two lines; that output can be the blob's own.
- "No one setting gets both tests" (every-event doc): withdrawn for this output. A margin
  counted in steps' worth is level at 1 and 2 on both tests.
- Payment (credit-rules finding, "[not in the engine]"): **in the engine, along the blob's
  own output's sources, payment is level with the hashed wires on MNIST
  (0.9557 against 0.9559) and +0.005 over them on pairs (0.7642 against 0.7594, 7 of 8
  seeds), with no wires of its own.** [likely: 8 seeds, two tests; about half the model's
  lead on pairs, none on MNIST]
- New finding candidate: **a coin of a gentle rule is a learning rule of its own.** The
  closed loop's coin reads 0.012 over born on MNIST. Give "over its coin" beside "over
  born" everywhere. [hint: reading of two coins here and five in the credit-rules doc]
- Toy and map lines for "Where it stands": with the blob's own output sixteen cues read
  0.97 (0.27 under the map's base column) and the map's MNIST row 0.89 online (0.21); xor
  is unchanged; **parity of 3 loses**: the cells the new loop teaches read 0.758 by a fresh
  old readout against 0.919 when the old readout was the teacher (8 of 8 seeds). [likely:
  8 seeds; cause not found]
- Layout note for `docs/notes-world.md`: `--motor-spread N` with few buttons takes the N
  cells nearest the top edge; any output that reads through the motor cells' own fields
  wants `--motor-spread-even 1` (pair test: fresh reader 0.745 to 0.7635, own output 0.657
  to 0.698).
- `docs/ideas.md`: idea 182 done, with the above. New ideas: (a) the same output giving a
  verdict spike a cell (a cell fires if its sum is over zero; the ladder's `comtop`: 0.88 /
  0.955), so the answer is bits again and the sheet can hear it; (b) an output of this kind
  that learns from good and bad alone (on bad the pressed line down and no line up breaks
  "one down for one up"; what replaces it); (c) 8-bit planes for the output (level with 12
  on validation) and for the hidden cells' counters under this error; (d) the readout's own
  spikes under `readout_good 4` with a narrower zone; (e) the closed loop on the calc
  battery (the output takes pattern answers a pair of lines at a time; not run); (f) why
  the new loop teaches parity of 3 less than the old readout did (not the margin, not the
  sensor cells; untested: the old answer was spikes the hidden cells could hear, so idea
  (a) may be the cure); (g) the blobprobe's fresh output inherits the life's
  genome numbers: a flag for "the old readout afterwards" would save the `--after`.
- `docs/plan.md`: item 9a done (this doc). Next, in order: (0) parity of 3 under the new
  loop (idea f), since it is the one place the new output is worse; (1) the verdict spike,
  so the output is bits (idea a); (2) the closed loop beside competitive learning (the every-event
  doc's item 4 still stands: two stores, one wiring); (3) the closed loop on the delayed
  rows with marks of two showings (credit-rules doc); (4) good-and-bad only (idea b).
- `docs/predictions.md`: the predictions above and their scores.
- `docs/design.md`, `genome.hpp` (core owner; edited here because the job is theirs):
  `out_on`, `out_bits`, `out_margin`, `out_window`, `out_fine`, `out_pay`, `out_top`,
  `out_hidden`; `hop_teach 6`; `readout_good 3`, `4`; `learn_output.cpp`,
  `output_state.hpp`; `blob.motor_line`, `out_read`, `out_vote`; `blob_output_verdict`. For
  design.md's list of mechanisms: "the blob's own output: a signed counter per source a
  motor cell can hear, in bit planes; a line's vote is its cells' counters summed over the
  sources that fired in the window, read once a step; told the right line, the right line
  steps up and the highest wrong line within the margin steps down; those two lines are
  the one-hop rule's errors. 11 x out_bits words a motor cell; an AND and a popcount a
  plane a slot a motor cell a step; no chance, no re-pick. The world reads a number a line,
  not spikes."
- `docs/notes-world.md`: `blobmnist ceiling --outside-form 4` and its options,
  `--life-teach 6`, the line "own answer on the ... digits, frozen", `scripts/outputinside.sh`.

## Predictions, scored

| prediction (output right online; fresh reader MNIST / pairs) | outcome |
|---|---|
| fld 0.91; 0.944 / 0.765 | 0.833; 0.942 / 0.735: the output lower (pixel counts swamp it), pairs lower (the layout) |
| fldbit 0.895; 0.942 / 0.760 | 0.871; 0.942 / 0.748 |
| fld12 as fldbit within 0.003: 0.7 | the reader yes; the output no (0.850: counters reach their end) |
| fld8 0.86; 0.935 / 0.740 | 0.234; 0.925 / 0.741: collapsed on MNIST |
| fld8s6 0.84; 0.935 / 0.735 | 0.761; 0.943 / 0.733 |
| fldr1 0.60; 0.927 / 0.700 | 0.604; 0.927 / 0.656: MNIST to the digit, pairs worse than guessed |
| fldr2 0.88; 0.940 / 0.750 | 0.883; 0.9395 / 0.683: MNIST right; on pairs the output is at chance |
| fldr3 0.87; 0.940 / 0.750 | 0.878; 0.938 / 0.674 |
| com 0.85; 0.935 / 0.740 | 0.097; 0.926 / 0.741: silent on MNIST |
| bit1 0.75; 0.932 / 0.720 | 0.145; 0.926 / 0.737: silent on MNIST |
| enggate 0.80; 0.935 / 0.720 | 0.774; 0.931 / 0.7145 |
| engrun 0.84; 0.937 / 0.725 | 0.825; 0.930 / 0.7145 |
| engrunstr 0.85; 0.937 / 0.725 | 0.837; 0.926 / 0.705 |
| the rung where the output falls: one-bit wiring 0.45, the good half 0.25, spiking 0.15, width 0.10, fields or one bit a source 0.05 | the question was half wrong. Three rungs fall (good and bad only, narrow every-event counters, anything with a bar) and one cause is under them, which was on no list of mine: more than one line coming down |
| the piece I expected to build: counters as weights, read at the verdict, a margin perceptron | built; as I had it (every close line stepping) it is right 0.85 and reads 0.941. The part that makes it work I did not foresee |
| that output alone right 0.90 (0.89 to 0.91: 0.6); over 0.90 as the mean of 8 seeds: 0.45 | 0.906 alone on a born sheet, 0.925 in the loop: yes. Pairs guessed 0.71: 0.620 alone, 0.708 in the loop |
| the closed loop 0.942 / 0.760 | 0.9559 / 0.7594 |
| within 0.005 of `outint2` on MNIST: 0.6 | no: over it by 0.014 |
| within 0.010 of it on pairs: 0.5 | yes (-0.008) |
| reaches 0.951 on MNIST: 0.1 | yes |
| reaches 0.770 on pairs: 0.25 | no at the report's setting (0.759); yes at the one validation picked afterwards (0.776) |
| over its coin on every seed of both tests: 0.8 | yes (16 of 16) |
| over a shuffled-digit life on every seed: 0.9 | yes (16 of 16) |
| one zone for both tests: 0.6 | yes: margin 1 and 2 level on both |
| firing rate 37 to 41 on MNIST (0.7), 33 to 36 on pairs (0.7) | 38.7 and 33.5: both |
| payment 0.944 / 0.765 | 0.9557 / 0.7642 |
| payment beats the hashed wires on MNIST by 0.002: 0.35 | no (-0.0002) |
| on pairs by 0.005: 0.35 | just under (+0.0048, 7 of 8 seeds) |
| on both: 0.2 | no |
| toys: xor 0.99 (0.95 or more: 0.7); parity of 3 0.90 (0.85 or more: 0.5) | xor 0.995 by the old fresh readout, 1.000 by the new: yes. Parity of 3: 0.758 by the old fresh readout, 0.879 by the new: under my guess by the reader the every-event doc used, and a loss against that doc's 0.919 that I did not expect |
| two map rows within 0.03 of the old output: 0.5 | the guess was about breaking and nothing broke: both rows are far over the old output (sixteen cues 0.27 to 0.97, the map's MNIST row 0.21 to 0.89) |
| a closed-loop life at 25,000 ticks a second or more: 0.6 | yes: 38,400 |

What I had wrong: I took the every-event job's "right nine times in ten" as the target
and expected to lose accuracy rung by rung until one-bit wiring. Accuracy was never the
variable. I expected the closed loop to land at the whole-number outside output's level
and it landed at the softmax one's, because the rule that fixes the output also names
fewer lines to the cells.
