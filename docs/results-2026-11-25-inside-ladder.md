# The inside's ladder: what can these cells hold under the best teacher? (2026-11-25)

Queue item 2e, idea 165. **Backprop here is a measuring stick, not the method.** The project's
method stays local rules on bits. This job asks only what the sheet's cells can represent when
the teacher is as good as a teacher can be, so that a shortfall can be put on the rule or on
the cell.

## Predictions (written before any training run was read)

Seen before writing these: the engine's numbers below; the tree's reader against my reader on
the born sheet's own spike counts (seed 5: MNIST 0.9248 tree, 0.923 mine; pairs 0.6924,
0.6965); and the model's born arms on seed 5, nothing trained: the real born wiring in the
model reads **0.961 on MNIST and 0.766 on the pair test** (rung 7b, the engine's rate curve),
against 0.925 and 0.692 for the same wiring in the real sheet. One trained development run
(validation, seed 1, 2 epochs) was started before this was written; its output was not read.

Guesses, fresh linear reader, test set (MNIST / pair test):

| rung | guess |
|---|---|
| 0 unconstrained net (3,072 relu, full fan-in) | 0.982 / 0.89 |
| 1 count units, full fan-in | 0.980 / 0.88 |
| 2 born fan-in (33), learned top-K of the pixels | 0.975 / 0.80 |
| 3a one-bit strengths, sign free | 0.970 / 0.78 |
| 3b strengths 1 and 4 at born counts | 0.972 / 0.79 |
| 4 cells hear cells, 2 settle steps | 0.975 / 0.82 |
| 5 fixed sign per source | 0.968 / 0.78 |
| 6 within reach | 0.965 / 0.77 |
| 7a activity at the engine's rate | 0.965 / 0.77 |
| 7b the engine's rate curve (bottom) | 0.970 / 0.79 (range 0.96-0.98 / 0.77-0.84) |
| bottom rung, no labels (autoencoder) | 0.962 / 0.77 |

- The rung that costs the most: 5, the fixed sign (pixels can only excite) 0.35; 6, reach
  0.25; 2, the fan-in 0.15; 7a/7b, the rate 0.15; 3, strengths 0.05; 4, 0.05.
- The transplant lands within 0.02 of the model's bottom rung: 0.25 (the born wiring already
  loses 0.036 and 0.074 between model and sheet). It beats the real born sheet (0.925,
  0.692): 0.6.

Seen after the predictions and before the report runs: the validation picks (seed 1, the
first form of rung 7b, without the sector bias): 12 and 20 epochs read the same (MNIST
0.978 and 0.978, pairs 0.867 and 0.868), so every rung trains 12 epochs.

## Short version

**Under the best teacher, cells with the engine's constraints read 0.980 on MNIST and 0.860
on the pair test in the model, and 0.966 and 0.785 when that wiring is written into the real
sheet; against 0.961 and 0.768 as born in the model (0.924 and 0.673 for the real born
sheet), and 0.981 and 0.933 for the unconstrained net. The step that costs the most is the
one from the model to the real sheet (what the sheet does in time: 0.013 on MNIST, 0.075 on
pairs); inside the model it is holding every cell at the engine's rate (rung 7a: 0.004 and
0.029), then, on pairs, the fan-in (rung 2: 0.024).** (Model rows: 8 seeds for the top, the
bottom and its born arm, 3 seeds for the rest; transplant 8 seeds, 5-12; test digits.)

- **The rule is the limit, not the cell.** A wiring that obeys every constraint the engine
  has (33 inputs a cell from its own neighbourhood, strengths 1 and 4 at the born counts, the
  senders' fixed signs, the engine's regulation) exists, and in the real sheet, read by the
  tree's own reader at the engine's own rate (34 and 32 spikes per 1,000 cell-ticks), it
  reads **0.966 on MNIST and 0.785 on the pair test**, every one of 8 seeds above its born
  sheet (+0.039 to +0.045; +0.095 to +0.128). The best local rule so far, competitive
  learning, reaches 0.952 and 0.726 at twice the rate. So a perfect teacher would add
  +0.042 and +0.113 to the real sheet where competitive learning adds +0.026 and +0.049;
  the cells pass the tree's own two-layer net's one pass on MNIST (0.955; five passes
  0.969) and come near its one pass on pairs (0.796; five passes 0.852).
- **In the model, MNIST costs nothing on the whole ladder** (0.981 at the top, 0.980 at the
  bottom; no rung moves it by more than 0.004). The pair test loses 0.073 between the top and
  the bottom: 0.009 for count units, 0.024 for the fan-in, nothing for one-bit or 1-and-4
  strengths, nothing for cells hearing cells, 0.012 for fixed signs, nothing for reach,
  0.029 for the rate, 0.005 for the engine's own cell and bounded regulation.
- **What the model leaves out costs as much as the whole ladder on pairs.** The same wiring
  reads 0.075 lower in the real sheet than in the model on pairs (0.013 on MNIST), and the
  born wiring already loses 0.037 and 0.095 between model and sheet. Digit by digit the
  real sheet does not agree with itself: run twice on the same test digits from two starting
  states, a cell's counts correlate 0.61 (MNIST) and 0.59 (pairs). About two fifths of what
  a cell does on a digit is the sheet's state, not the digit. That is the part a reader
  cannot use and the model does not have.
- **Labels are needed for these features, at least by this route.** The bottom rung trained
  with no labels (an autoencoder: give the bits back through a linear decoder) reads 0.960
  on MNIST, the born arm's level, and 0.745 on pairs, under it (0.768).
- **What the trained cells look like** (bottom rung, MNIST / pairs, 8 seeds): 15.1 / 16.6
  pixel inputs of 33 (born 7.0; competitive learning 17.8); the pixels spread 3.4 px from
  their middle (born 4.0; competitive learning 2.5: the teacher's cells are less tight);
  5.6 inhibitory inputs (the born count, kept by the read-off); 7.5% of the born inputs
  kept; on 14.5% / 12% of digits a cell spikes (born 22%), 0.57 / 0.55 spikes a digit, and
  8% / 18% of cells never spike on the test digits. The answer explains 13% of a cell's
  count variance on MNIST (born 11.5%; the unconstrained net's units 32%); on pairs 0.2%
  (born 0.1%): no single cell says "same", the reader builds it from many.
- **The best code is sparse; the engine's regulation forbids it.** Unconstrained (rungs 1
  to 4), the teacher silences 38% to 48% of the cells on the pair test, and 1% to 4% of all
  cell-digits have a spike. Holding every cell at 0.66 spikes a digit (7a) costs 0.029 there,
  the largest step inside the model, and the cells take more pixels to fire that
  often (10 pixels a cell at rung 6, 17 to 21 at 7a). A lower target rate per cell is a
  genome number, free per tick; untested here.

## The ladder

Fresh linear reader of the 3,072 cells, test digits. Mean (lowest to highest, seeds). "own":
the net's own output layer. "active": share of cell-digits with a spike; "mean": spikes a
digit a cell (born sheet 0.66). Born arm: the same rung, random wiring (6, 7b: the real born
wiring), nothing trained. Seeds 5, 6, 7 unless marked 8 (seeds 5-12). Every row is
`scripts/ladder2.sh full` (the lines under "Script lines"); `out/ladder2/full/table.txt`.

MNIST:

| rung | reader | own | active | mean | born arm |
|---|---|---|---|---|---|
| 0 linear reader on the bits | 0.915 | | | | |
| 0 tree's two-layer net, 100 hidden, 1 / 5 passes (`blobmnist baselines`) | 0.955 / 0.969 | | | | |
| 0 unconstrained net, 3,072 relu (8 seeds) | 0.9811 (0.980-0.982) | 0.983 | 0.35 | | |
| 1 count units, full fan-in | 0.9814 (0.981-0.982) | 0.982 | 0.10 | 0.32 | |
| 2 born fan-in, learned top-K | 0.9814 (0.980-0.982) | 0.982 | 0.10 | 0.28 | 0.960 |
| 3a one-bit strengths | 0.9812 (0.981-0.981) | 0.983 | 0.09 | 0.21 | 0.960 |
| 3b strengths 1 and 4 at born counts | 0.9813 (0.981-0.981) | 0.982 | 0.11 | 0.24 | 0.960 |
| 4 cells hear cells, 2 steps | 0.9808 (0.980-0.982) | 0.982 | 0.10 | 0.28 | 0.957 |
| 4s3 3 steps | 0.9794 (0.978-0.981) | 0.981 | 0.12 | 0.35 | |
| 5 fixed sign per source | 0.9785 (0.977-0.980) | 0.979 | 0.09 | 0.38 | 0.947 |
| 6 within reach | 0.9809 (0.980-0.982) | 0.982 | 0.12 | 0.50 | 0.953 |
| 7a threshold by regulation | 0.9769 (0.976-0.978) | 0.976 | 0.24 | 0.60 | |
| **7b the engine's cell (8 seeds)** | **0.9795 (0.979-0.980)** | 0.978 | 0.15 | 0.57 | **0.9608 (0.959-0.962)** |
| 7b with no labels (autoencoder) | 0.9602 (0.958-0.962) | | 0.27 | 0.79 | |
| **real sheet, 7b's wiring (8 seeds)** | **0.9663 (0.965-0.968)** | | | | **0.9241 (0.922-0.927)** |

Pair test (same class, interleaved):

| rung | reader | own | active | mean | born arm |
|---|---|---|---|---|---|
| 0 linear reader on the bits | 0.499 | | | | |
| 0 tree's two-layer net, 1 / 5 passes | 0.796 / 0.852 | | | | |
| 0 unconstrained net (8 seeds) | 0.9328 (0.928-0.939) | 0.932 | 0.02 | | |
| 1 count units, full fan-in | 0.9234 (0.922-0.925) | 0.924 | 0.01 | 0.03 | |
| 2 born fan-in, learned top-K | 0.8996 (0.894-0.903) | 0.899 | 0.01 | 0.03 | 0.777 |
| 3a one-bit strengths | 0.8955 (0.894-0.897) | 0.893 | 0.02 | 0.03 | 0.773 |
| 3b strengths 1 and 4 at born counts | 0.9015 (0.899-0.903) | 0.902 | 0.03 | 0.04 | 0.777 |
| 4 cells hear cells, 2 steps | 0.9007 (0.898-0.904) | 0.901 | 0.02 | 0.05 | 0.766 |
| 4s3 3 steps | 0.8966 (0.893-0.901) | 0.895 | 0.04 | 0.10 | |
| 5 fixed sign per source | 0.8893 (0.883-0.893) | 0.889 | 0.03 | 0.14 | 0.743 |
| 6 within reach | 0.8938 (0.892-0.895) | 0.896 | 0.05 | 0.23 | 0.757 |
| 7a threshold by regulation | 0.8645 (0.861-0.869) | 0.860 | 0.31 | 0.57 | |
| **7b the engine's cell (8 seeds)** | **0.8599 (0.853-0.865)** | 0.840 | 0.12 | 0.54 | **0.7682 (0.757-0.777)** |
| 7b with no labels (autoencoder) | 0.7448 (0.741-0.749) | | | | |
| **real sheet, 7b's wiring (8 seeds)** | **0.7853 (0.775-0.798)** | | | | **0.6728 (0.664-0.680)** |

Read down the pair column, each line one constraint more:

- **Count units (1): -0.009.** Spikes 0 to 7 instead of a float.
- **The born fan-in (2): -0.024.** 33 inputs a cell, the best 33 of 784 chosen freely. The
  largest constraint of the cell itself. Wider cells exist (`--g.wire_pixel_shift`).
- **Strengths (3a, 3b): nothing.** One bit is as good as a float (-0.004), and 1 and 4 at the
  born counts gets it back (+0.006). The lopsided strengths are no loss.
- **Cells hearing cells (4): nothing.** 2 steps 0.901, 3 steps 0.897. Given the choice a
  cell wires 10 pixels and 23 cells, and gains nothing over pixels alone.
- **Fixed signs (5): -0.012.** Pixels can only excite; "this pixel must be off" has to come
  through an inhibitory cell. The second largest cost of the cell. On MNIST -0.002.
- **Reach (6): nothing** (+0.005, within the spread). The 103 pixels within reach of a cell
  hold what it needs.
- **The rate (7a): -0.029, the largest model rung.** Above, the teacher keeps most cells
  silent; here every cell must spike 0.66 times a digit.
- **The engine's cell and its bounded regulation (7b): -0.005.** A leaky integrator with its
  own leak and cooldown; threshold 1 to 64 plus a sector bias of -16 to 16. The bounds let
  some cells stay quiet (18% never spike), the rate curve costs a little.
- **The real sheet (spikes in time, delays, noise, recurrent dynamics, regulation moving
  during the read): -0.075.** The largest single step of all on pairs.

The born arms read alike at every rung (MNIST 0.947 to 0.961, pairs 0.74 to 0.78): a random
wiring under any of these constraints is a good random projection in the model. Trained minus
born is +0.019 to +0.032 on MNIST and +0.09 to +0.15 on pairs at every rung.

## The transplant

The bottom rung's wiring, written into the real born sheet (`blobmnist ceiling
--wiring-load`: 3,072 cells rewired, 101,000 connections, none outside a cell's slots), 3,000
digits of regulation with learning off, then the inside-learning test's reader. Born: the
same sheet without the load, read the same way. Hidden spikes per 1,000 cell-ticks in
brackets.

| seed | MNIST transplant | born | pairs transplant | born |
|---|---|---|---|---|
| 5 | 0.9666 [34.3] | 0.9228 [33.0] | 0.7857 [31.7] | 0.6787 [32.7] |
| 6 | 0.9670 [35.0] | 0.9232 [32.9] | 0.7747 [31.2] | 0.6801 [32.5] |
| 7 | 0.9665 [34.7] | 0.9267 [32.8] | 0.7799 [32.4] | 0.6635 [32.4] |
| 8 | 0.9648 [35.2] | 0.9221 [33.1] | 0.7949 [31.5] | 0.6758 [32.6] |
| 9 | 0.9660 [35.3] | 0.9235 [33.3] | 0.7886 [31.8] | 0.6720 [32.9] |
| 10 | 0.9675 [34.1] | 0.9225 [32.9] | 0.7808 [31.6] | 0.6731 [32.5] |
| 11 | 0.9662 [35.4] | 0.9258 [33.2] | 0.7979 [32.1] | 0.6695 [32.8] |
| 12 | 0.9659 [34.2] | 0.9265 [33.1] | 0.7798 [31.6] | 0.6696 [32.8] |
| mean | **0.9663** | 0.9241 | **0.7853** | 0.6728 |

- **MNIST lands within 0.02 of the model (0.013 under it); the pair test does not (0.075
  under).** So the model is trustworthy for MNIST and optimistic for pairs by about as much
  as the whole ladder costs.
- The transplanted sheet runs at the engine's rate (31 to 35 per 1,000 cell-ticks against
  the born 33): unlike competitive learning's lived sheet (63 to 66), it needs no extra
  firing for its lead.
- **What differs, digit by digit** (seed 5, `scripts/ladder2.sh full --seeds 5-7 --parts
  transplant,match`; the model given the same wiring and the same digits, thresholds by regulation):

| | MNIST born | MNIST transplant | pairs born | pairs transplant |
|---|---|---|---|---|
| reader, real sheet | 0.923 | 0.963 | 0.686 | 0.790 |
| reader, model, same wiring and digits | 0.961 | 0.980 | 0.765 | 0.854 |
| a cell's counts, model against sheet (correlation over digits, mean over cells) | 0.39 | 0.48 | 0.40 | 0.35 |
| a cell's counts, sheet against itself from another starting state | 0.61 | | 0.59 | |
| share of cell-digits with a spike, sheet / model | 0.42 / 0.21 | 0.36 / 0.15 | 0.42 / 0.22 | 0.36 / 0.12 |

  The real sheet's cells answer a digit only 0.6 like themselves when the sheet starts from
  another state (the reader still reads 0.925 and 0.688 on the second run: the code is
  noisy, not wrong). The model's cells are noise-free and fire on half as many digits; more
  of the sheet's spikes are single spikes that do not depend on the digit. That is what "the
  rest of the engine" is worth: roughly 0.04 on the born MNIST sheet and 0.08 to 0.10 on
  pairs. Two cheap guesses did not explain it on the born MNIST sheet (seed 5): a 20-tick
  quiet gap after every digit (0.9288 against 0.9248) and noise off (0.9288)
  (`out/ladder2/probe/`). Where the rest comes from (inhibition at 5 times strength with
  delays of 1 to 4 ticks making the activity chaotic is my guess) is not found.

## Consequence

The bottom rung is near the top on MNIST and well above the born sheet on both tests, in
the model and in the real sheet. So **the shortfall on real data is the rule's**: no change
to the cell is needed to reach 0.966 and 0.785 in the engine as it is, and the trained wiring
is a concrete target for a local rule. What the target looks like, against what competitive
learning makes: about 15 to 17 pixel inputs of 33 (compete 17.8), less tightly grouped (3.4
px against 2.5), the born inhibitory inputs, at the engine's rate (compete runs at twice
it). On pairs the target cells are individually no "same" detectors (the answer explains
0.2% of a cell's variance); they are features whose combination a reader can use.

What the cell does cost, in order: on pairs the real sheet's dynamics (0.075 of a wiring's
worth, and about two fifths of each cell's count is state, not digit), the rate every cell is
held to (0.029), the fan-in (0.024), fixed signs (0.012). Lifting the first is not a wiring
question; it would mean less state-driven firing during a digit (a quieter sheet, or a
reader of more than one look). The second is a genome number (a lower per-cell target), free
per tick. The third, wider cells, is free per tick too: a slot's masks are whole words
whatever is wired in them (the hop-real doc's `wire_pixel_shift`).

## The engine's numbers (seed 5 unless said; seeds 6 and 7 within a few hundredths)

From the born sheet itself (`blobmnist ceiling --describe`, written by
`src/mnist/mnist_transplant.cpp`) and the code:

| what | number | where |
|---|---|---|
| neurons | 4,096 (8 x 8 tiles of 64) | `genome.hpp:8-9` |
| sensor cells | 1,024 (every second cell both ways of the 64 x 64 sheet); 784 carry a pixel | `genome.hpp:14-15`, `scripts/mnist.sh` SHEET |
| non-sensor cells (the reader's 3,072) | 1,024 motor-lattice cells, 783 inhibitory, 7 pacemakers, 1,530 hidden excitatory (some overlap: a motor cell may be inhibitory) | `birth.cpp:115-155` |
| inhibitory | 25% of non-sensor cells; a sender's sign is fixed; inhibition counts 5 times | `genome.hpp:20, 39` |
| a cell's candidates | the 576 cells of its own tile and the 8 around (not itself), plus each far tile's excitatory cells; on this sheet that is 103 pixels (24 to 176) and 370 cells (92 inhibitory) a cell | `blob.hpp:12-14`, `tag_store.cpp:158-162` |
| inputs at birth | each local candidate with chance 1/16 (`--g.local_connect_shift 4`): 33.1 a cell (8 to 84), of them 7.0 pixels, 5.8 inhibitory cells | `birth.cpp:218-224` |
| strengths at birth | 1 or 4: a wired input is strong (4) with chance 1/4: 7.55 strong a cell; no cell is born with a 5 | `genome.hpp:29`, `birth.cpp:222-224` |
| far slots | 15% of excitatory cells per slot, a quarter of the far tile's excitatory cells at 1: 0.22 far slots a cell | `genome.hpp:30, 35`, `birth.cpp:229-245` |
| the counters' read-off | a rewired cell keeps its born count of excitatory and of inhibitory inputs, and its born count of strong ones of each kind (27.3, 5.8; 6.1, 1.4) | `tag_store.cpp:51-79, 155-195` |
| threshold | born 4 to 12; regulated: up one with chance 1/16 on a spike, down one with chance 1/512 a tick; floor 1, cap 64 | `genome.hpp:21, 49-51`, `regulate.cpp:7, 70-97` |
| sector bias | a sector is 2 x 2 tiles; its bias moves one step a tick toward 32 spikes per 1,024 free neurons a tick, within -16 to 16 | `genome.hpp:45-48`, `regulate.cpp:10-31` |
| leak, cooldown | leak shift 1 to 4, cooldown 1 to 4 ticks, drawn per cell (about a quarter each) | `genome.hpp:22-23` |
| a digit | 20 ticks, no gap; a pixel line fires on every tick it is on | `mnist_sheet.cpp:32, 43`, `plug.cpp:391-414` |
| winners per tile | none on this sheet (`tile_winners` 0; the compete job's setting B used 4) | `genome.hpp:245` |
| the born sheet's activity | 33.1 spikes per 1,000 cell-ticks: 0.66 spikes a digit a cell; 42% of cell-digits have a spike (58% 0, 27% 1, 10% 2, 4% 3, 1.3% 4, 0.5% more) | `out/ladder2/probe/` |

Not modelled: the branch bonus (+2 for 3 or more excitation in one slot), delays (1 to 4
ticks per slot), noise (2^-10 a tick), pacemakers (7 cells treated as ordinary), the leak's
integer steps.

## The model and its rungs

`scripts/ladder2_model.py`. Every rung has 3,072 cells, one per non-sensor cell of the real
sheet, read in the order the tree's reader reads them. A cell gives a spike count for the
digit (0 to 7: 99.9% of the born sheet's counts are under 8). Pixels are the sheet's bits (on
at half ink). The pair test is `--pair 2 --pair-mix 1`, same class or not, interleaved (the
brief's `--pair 1` is the parity-of-the-sum test; I used 2, the hop-real doc's main test). A
cell that hears other cells hears their count / 20 a tick, because a pixel line is on for
all 20 ticks of a digit and a cell only on its spikes. Each rung is trained jointly with a
softmax output layer by Adam (rate 0.002, cosine to 0, batches of 256, 12 passes; the pair
test draws fresh partners every pass, as the sheet's stream does). Non-differentiable steps
pass gradients straight through: a rounded count passes its unrounded value's slope (with a
small slope below 0 so a silent cell can come back), and a wiring passes each weight's
gradient to its candidate's score.

| rung | adds |
|---|---|
| 0 | the bars: a linear reader on the bits; the tree's two-layer net (`blobmnist baselines`); an unconstrained net of 3,072 relu units, full fan-in, float weights, trained as the rungs are |
| 1 | count units (0 to 7 spikes), full fan-in from the 784 pixels, float weights; threshold and gain learned per cell |
| 2 | fan-in cut to the cell's born count (33 on average), the inputs chosen as the top of a learned score per pixel: the engine's counter per candidate, wired at the top counters; float weights on what is wired |
| 3a | every wired input at strength 1 (the sign still free per input) |
| 3b | strengths 1 and 4: the cell's born count of strong inputs at 4 (the top scores), the rest of its born count at 1 |
| 4 | cells hear cells: every pixel and every other cell is a candidate under the one budget; 2 settle steps (4s3: 3) |
| 5 | a fixed sign per source: pixels and excitatory cells excite; the real sheet's inhibitory cells inhibit at 5 times the strength; excitatory, inhibitory and strong inputs at the cell's born counts of each kind (the read-off of `tag_rewire`) |
| 6 | candidates only within reach: the cell's 9 tiles and its far tiles (far: excitatory only), from the real sheet's geometry |
| 7a | the threshold is no longer the teacher's: regulation moves each cell's threshold toward 0.66 spikes a digit (the born sheet's 33 per 1,000 cell-ticks), unbounded |
| 7b | the bottom: the engine's cell at constant drive (potential p <- p - p/2^L + D, a spike when p reaches the bar, reset to 0, no spike for `cooldown` ticks; count = 20 / ticks between spikes, with the cell's own leak L and cooldown) and the engine's regulation with its bounds (the cell's threshold 1 to 64 plus its sector's bias -16 to 16, each moved toward the target). The gain is no longer the teacher's either |

Why this order and not the brief's: the engine gives a cell one budget for pixels and cells
together, and a fixed sign only means something once cells hear cells (pixels can only
excite; inhibition comes only from inhibitory cells). So cells hearing cells comes before the
sign, not after the rate. Settle steps rather than layers, because the sheet is one recurrent
sheet whose cells hear each other; 2 steps by default, 3 as an arm. Reach comes after the
sign because it is drawn from the real sheet's geometry.

Every wired rung has a **born arm**: the same constraints with a random wiring (rungs 6 and
7b: the real born sheet's wiring), nothing trained, thresholds (and at rungs 2 to 6 one gain
for all cells) set by regulation to the born sheet's 0.66 spikes a digit and 42% of
cell-digits active. That is the rung's born sheet; trained minus born is what a perfect
teacher adds.

Every arm is read the same way: the cells frozen, the training digits shown once, a fresh
softmax reader of the 3,072 counts (centred and scaled as the tree's reader's `--scale 2`),
scored on the test digits. My reader against the tree's on the same counts (the born sheet's
rows from `blobmnist ceiling --rows-out`, seed 5): MNIST 0.923 against 0.9248, pairs 0.6965
against 0.6924. The same measure within 0.004.

**The transplant** (`blobmnist ceiling --wiring-load F`, `src/mnist/mnist_transplant.cpp`):
the bottom rung's trained wiring (sources and strengths; the signs are the sources' own)
written into the real born sheet, then 3,000 digits with learning off and regulation on so
the thresholds settle, then the inside-learning test's reader. Beside it the born sheet read
the same way. `--describe F` writes the born sheet for the model; `--rows-out F` writes every
cell's counts for every digit (`scripts/ladder2_match.py` compares them with the model's).

## What is real, what is suspect

- Real: the transplant (8 seeds, every seed far above born on both tests, the tree's own
  binary and reader, at the engine's rate); the bottom rung, its born arm and the top (8
  seeds, spreads of 0.001 to 0.012); MNIST flat down the model.
- Real, 3 seeds: the pair ladder's steps of 0.02 to 0.03 (spreads within a rung 0.003 to
  0.010). The steps of 0.005 or less are within the spread and called "nothing".
- Suspect: the model's absolute pair numbers. The model reads the born wiring 0.095 higher
  than the real sheet on pairs, so every pair number in the model is optimistic by about that
  much; the order of the rungs is what the model is for, and the transplant is the number to
  quote for the engine.
- Suspect: rung 7b's own output layer on pairs (0.840, one seed 0.781): regulation moves the
  thresholds after the output layer has fit; the fresh reader (0.860) is the measure.
- The autoencoder is one label-free objective, kept small (a linear decoder of the bits);
  "labels are needed" holds for it, not for every label-free rule.
- Rungs are trained 12 passes (picked on validation, seed 1: 12 and 20 the same). Seeds 5-7
  of the trained rungs ran on stevejobs, seeds 8-12 of 7b and the net and every born arm and
  transplant here; torch on the two machines is not bit-identical, each run is one machine.
- The pair test's partners: the model draws its own (half same class, fresh each pass; the
  test pairs fixed by the seed); the sheet's runs use the task's. The match table uses the
  sheet's own digits and partners for both.
- The brief named `--pair 1 --pair-mix 1`; in the code `--pair 1` is the parity of the sum,
  so I used `--pair 2 --pair-mix 1` (same class, interleaved), the hop-real doc's main test.

## Not done

- Winners per tile (`tile_winners`) as a rung: not on this sheet by default.
- A spiking version of the model (20 ticks, delays, noise) to split the model-to-sheet step;
  only the cell-by-cell match and two cheap guesses were done.
- A lower per-cell target rate in the real sheet, and wider cells with the transplant: the two
  cheap levers named above, untested.
- Other label-free objectives (contrastive) at the bottom rung.
- The match table on more than seed 5.
- The parity-of-the-sum test.

## Script lines

Outputs in `out/ladder2/NAME/`; a finished run is kept, so a line run again only runs what
is missing. On stevejobs the same lines ran with `THONK_MNIST` and `LADDER_VENV` set.

```
scripts/ladder2.sh full --parts bars,model --kinds lin,born          bars, linear, born arms; seeds 5-7
scripts/ladder2.sh full --parts model --kinds trained,auto --jobs 2   every rung trained, autoencoder (stevejobs)
scripts/ladder2.sh full --seeds 5-7 --parts model --kinds trained --rungs net,1,2,3a,3b   part of it, here
scripts/ladder2.sh full --seeds 8-12 --parts model --kinds trained,born --rungs net,7b   the 8-seed rows
scripts/ladder2.sh full --seeds 8-12 --parts transplant               the transplant and born reads
scripts/ladder2.sh full --seeds 5-7 --parts transplant,match          the same, and cell by cell (seed 5)
scripts/ladder2.sh full --seeds 5-12 --table-only                     the tables above
scripts/ladder2.sh smoke --quick                                      one seed, one pass on 10,000 digits
```

The pass count was picked with `ladder2.py run mnist|pair 7b trained 1 SHEET OUT --val
--epochs 12|20` (seed 1, on stevejobs, before rung 7b had its sector bias); the same as a
script line: `scripts/ladder2.sh pick --val --seeds 1 --rungs 7b --kinds trained --epochs 20
--parts model`.

A trained rung with cells hearing cells takes about 5 minutes on stevejobs (one thread),
about 20 here; the others 1 to 2 minutes; a transplant about a minute.

## Code

- `scripts/ladder2.sh`, `ladder2.py` (runner), `ladder2_data.py` (bits, pairs, the reader),
  `ladder2_model.py` (the rungs), `ladder2_train.py` (one run), `ladder2_table.py`,
  `ladder2_match.py` (the model against the sheet, digit by digit).
- `src/mnist/mnist_transplant.cpp` (new) and three flags of `blobmnist ceiling`
  (`--describe`, `--wiring-load`, `--rows-out`), all off by default; nothing in `src/blob/`
  or on the tick's path. Clean build, no new warnings; the four reference lives: csvs the
  same, the -sum files differ only in their ticks-per-second lines
  (`out/ladder2/refcheck.txt`).

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, findings 41, 47, 51 and "Where it stands": **the cells are not the
  limit.** A wiring within every constraint the engine has (fan-in, reach, 1 and 4 at born
  counts, fixed signs, its regulation) reads 0.966 on MNIST and 0.785 on the pair test in the
  real sheet at the engine's rate (born 0.924, 0.673; competitive learning 0.952, 0.726 at
  twice the rate), 8 seeds. Finding 41 ("a linear-plus-pairs feature bank") describes the born
  sheet, not what the cells can hold; finding 47's "rewiring at born counts" has room above
  it of +0.014 (MNIST) and +0.06 (pairs) over the best rule; finding 51's "what is missing is
  not the error reaching the cell" is right: the cell can hold it. [likely: one engine, two
  tests, 8 seeds]
- New finding: **the real sheet's cells answer a digit only 0.6 like themselves from another
  starting state**; the static model of the same wiring reads 0.04 (MNIST) to 0.08-0.10
  (pairs) higher. [hint: seed 5]
- New finding: **the best code is sparse and per-cell regulation forbids it**: holding every
  cell at 0.66 spikes a digit is the largest model rung on pairs (-0.029). [hint: 3 seeds]
- `docs/plan.md`: item 2e done. Next candidates: a local rule scored against the
  transplant's target (0.966, 0.785) rather than against born; the per-cell target rate
  lowered; what makes the sheet's cells state-driven.
- `docs/notes-world.md`: `blobmnist ceiling --describe | --wiring-load | --rows-out`,
  `scripts/ladder2.sh`.
- `docs/predictions.md`: the predictions above and their scores.

## Predictions, scored

| prediction (MNIST / pairs) | outcome |
|---|---|
| 0 net 0.982 / 0.89 | 0.981 / 0.933 |
| 1 0.980 / 0.88 | 0.981 / 0.923 |
| 2 0.975 / 0.80 | 0.981 / 0.900 |
| 3a 0.970 / 0.78; 3b 0.972 / 0.79 | 0.981 / 0.896; 0.981 / 0.902 |
| 4 0.975 / 0.82 | 0.981 / 0.901 |
| 5 0.968 / 0.78 | 0.979 / 0.889 |
| 6 0.965 / 0.77 | 0.981 / 0.894 |
| 7a 0.965 / 0.77 | 0.977 / 0.865 |
| 7b 0.970 / 0.79 (0.96-0.98 / 0.77-0.84) | 0.980 / 0.860: MNIST in range, pairs above it |
| autoencoder 0.962 / 0.77 | 0.960 / 0.745 |
| the costliest rung: 5 (0.35), 6 (0.25), 2 (0.15), 7 (0.15) | inside the model 7a (0.15 given), then 2 on pairs; overall the model-to-sheet step, which I did not list. Wrong |
| the transplant within 0.02 of the model: 0.25 | MNIST yes (0.013), pairs no (0.075) |
| it beats the real born sheet: 0.6 | yes, every seed, both tests |

I underrated the cells on pairs everywhere (0.07 to 0.10 too low at every rung) and expected
the sign and reach to cost; they cost 0.012 and nothing.
