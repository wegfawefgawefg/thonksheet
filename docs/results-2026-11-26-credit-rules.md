# The credit rules not yet tried, on the rule's ladder's bench (2026-11-26)

Queue item 6, idea 177 (ideas 91, 138, 139). **No rule here uses backprop.** Backprop appears
only as a diagnostic: how often a rule's teaching bit has the sign backprop's would have.

(Results are not in yet. This commit holds the rules as built and the predictions.)

## Predictions (written before any scoring run was read)

Seen before writing these: the three docs this builds on and their numbers (in the real
sheet: born 0.924 / 0.673, T7x 0.961 / 0.761, competitive learning 0.952 / 0.726, the
backprop teacher's wiring 0.966 / 0.785). From this job only mechanics runs
(`out/credit/dev/mech.py`: lives of 1,280 and 2,560 digits on seed 5, no reader, no real
sheet), which print for each arm the share of cell-digits taught, the share of "more" among
them, counter steps a digit, the largest counter, born inputs changed, and how often the
teaching bit has backprop's sign. What they showed:

- Agreement with backprop's sign in the first 1,280 digits: the one-hop rule 0.51, two
  phases 0.50 to 0.51, code with every cell perturbed 0.507, with 192 cells 0.53, with 12
  cells 0.63, payment 0.87 (MNIST; 1.00 on pairs, where the two read weights mirror each
  other), targets from the answer 0.52, tuned wires 0.51 to 0.52.
- Two phases with every spike an event: at a gain of 1/4 and above, nine "more" in ten and a
  largest counter of 8,000 after 2,560 digits; it will pass 16 bits. Its counters were given
  24 bits and the gain grid moved down to 1/16, 1/4, 1.
- A run takes 2 to 5 minutes here.

`scripts/credit.sh smoke --quick` had been started; its output was not read before this was
committed.

Guesses, the wiring written into the real sheet, seeds 5-7, MNIST / pairs:

| arm | guess | note |
|---|---|---|
| T7x again | 0.961 / 0.761 | the ladder's numbers; its coin 0.928 / 0.675 |
| hop (T7x through this bench's loop) | the same model reader as T7x to four places on seed 5: 0.8 | the check |
| P2s two phases, sign of the difference | 0.955 / 0.745 | the one-hop error, found by the cell's own counts |
| P2 two phases, every spike an event | 0.950 / 0.735 | runs to "more"; the pick will take the smallest gain |
| C2 code, every cell | 0.925 / 0.675 | within 0.003 of its coin on both |
| C2n192, C2n12 | 0.928 / 0.680, 0.927 / 0.678 | agreement over the life 0.51, 0.53, 0.62 for all, 192, 12 |
| C1 code, good or bad only | its coin within 0.003 | |
| pay1 the output pays the cells that fired | 0.960 / 0.755 | |
| pay passed back once more | 0.961 / 0.760 | within 0.003 of pay1 |
| payc passed back to one source | pay within 0.003 | |
| tgt a target from the answer | 0.945 / 0.690 | no cell can be a "same" detector |
| dtp wires the cell tuned itself | 0.958 / 0.750 | |
| hopb, dtpb (the bit passed back) | their rule within 0.003 | |
| d1n error one showing late, no trace | 0.928 / 0.675 | its coin's level |
| d1t2 marks last 2 showings | 0.955 / 0.745 | half the marks belong to another digit |
| d1t4, d3t4 marks last 4 | 0.948 / 0.725 both | the delay costs nothing inside the mark's life, the blur does |
| d0t4 error on time, marks last 4 | 0.950 / 0.730 | |

- **Some rule beats T7x in the real sheet on both tests** (seed by seed, mean above zero and
  at least two of three seeds on each test): 0.25. The likeliest: pay, hopb or dtp.
- pay or pay1 beats T7x on MNIST by 0.002 or more: 0.3; on pairs by 0.005 or more: 0.25.
- dtp beats T7x on both: 0.2. P2s beats T7x on both: 0.1.
- Credit by code beats its coin by 0.005 on either test at any number of cells perturbed:
  0.1.
- tgt beats born on MNIST: 0.85; on pairs: 0.5.
- The firing rate written in: every rule that learns sits between 33 and 46 spikes per
  1,000 cell-ticks (T7x: 43 on MNIST, 34 on pairs).

Which arms get 8 seeds (5-12), fixed now: born, T7x and its coin, and every rule that on
seeds 5-7 reads in the sheet within 0.005 of T7x or above it on either test, with its coin.

## The bench and the rules as run

The bench is the rule's ladder's (`scripts/ladder3_rule.py`, imported): idea 165's model of the
sheet's 3,072 non-sensor cells at its bottom rung (about 33 inputs a cell from its own
neighbourhood, strengths 1 and 4 at born counts, fixed signs, the engine's cell and
regulation, two settle steps), a counter per candidate input per cell starting at the born
strength, the cell wired to its top counters at its born counts, rewired and regulated every
64 digits, one pass of 60,000 digits one at a time, and a softmax output layer that learns by
its own delta rule. T7x on it is the one-hop rule: the output's signed error on each line
(+1, -1, or 0 when under 0.1), sent to each cell on fixed random wires (each line reaches a
cell with chance 1/2, as +1 or -1), the cell told "more" or "less" by the sign of the sum if
it was near its bar or fired, and then **every input that was on moves its counter one
count** (chance 1, 16-bit counters).

Every rule here keeps that last step and the same marks (an input that was on in the
digit), and changes only where the cell's "more / less / nothing" comes from. All integer,
every event counted. `scripts/credit_rule.py`.

| arm | where the cell's bit comes from |
|---|---|
| T7x | the ladder's own code, run again on the same seeds |
| hop | T7x through this job's loop: the check that this bench is that bench |
| **P2s** two phases, sign | The output lines reach the cells as plain inputs on the same fixed +-1 wires, at a gain g (drive a tick at full rate). Free phase: the lines carry the output's own answer (its ten rates). Clamped phase: they carry the right answer. Both phases settle twice (so the nudge also spreads one step through the cells' own wires). The cell compares its two counts: more in the clamped phase, "more"; less, "less". No error is computed anywhere and no gate is needed |
| **P2** two phases, every spike | The same phases; every spike a cell makes in the clamped phase counts its marked inputs up, every spike in the free phase counts them down (contrastive Hebbian as written). The cell remembers nothing between phases |
| **C2** code, two runs | Each cell has a code bit for the digit: force one more spike, or veto one. A clean run, then a run with every cell's code applied; the output says only whether the answer's line rose or fell. A cell whose code took effect and helped does more of it ("more" if it added a spike), one that hurt does the reverse. C2n192, C2n12: only 192 or 12 cells perturbed a digit |
| **C1** code, one run | One run with the codes applied; the outcome is only whether the pressed button was right |
| **pay1** payment, one step | The output pays backward along its own read wires: a line that should have been higher pays the fired cells it reads with a positive weight and charges those it reads with a negative one; a line that should have been lower the reverse. A cell that fired and was paid on balance is told "more", charged "less". Only the sign of each read weight is used |
| **pay** payment, passed back | pay1, and each paid or charged cell passes its bit to the wired cells whose firing let it fire (those that fired in the first settle step; through an inhibitory wire the bit flips, a strong wire counts 4). Those cells step their pixel inputs |
| **payc** conserved | pay, but a paying cell pays exactly one of those sources, drawn by chance |
| **tgt** a target from the answer | The answer alone on the fixed wires: +1 the cell should fire for this answer, -1 it should be silent, no wire no target. Near the bar and silent with target "fire": "more"; fired with target "silent": "less". The output plays no part |
| **dtp** tuned wires | Difference targets with a learned way back: each cell keeps an 8-bit up/down counter per answer line (+3 if it fired when that line was the answer, -1 if not). A line's wire into the cell is +1 if that counter is above the cell's mean counter, -1 if below. Then the output's signed errors arrive on those wires as in T7x |
| hopb, dtpb | T7x or dtp, then the bit passed back once as in pay |
| **d1n, d1t2, d1t4, d3t4, d0t4** time | T7x with the error arriving 1, 1, 1, 3, 0 showings late and a mark on each input (it was on while the cell was near or fired) that lasts 1, 2, 4, 4, 4 showings; every live mark counts when the error arrives |

Why these and not others. With fixed wires and no threshold on the way back, difference
target propagation is the one-hop rule (the target less the cell's activity is the wires
times the error), so the fixed-wire form of family 4 is tgt (targets from the answer alone,
no output) and the learned form is dtp. With a learned linear way back fitted to "what do I
do when line k is the answer", the back wire is the cell's own tuning, which is what dtp
keeps in its counters. Two phases with fixed wires differ from the one-hop rule in who does
the subtraction: the cell's own count, through its own bar, after the nudge has also passed
one step through the cells' wires.

**Coin controls**: the same rule with the teaching signal of another digit: T7x, dtp and
the time arms take another digit's line errors (the ladder's ring of 256); two phases take
another digit's output and answer on the lines; code takes another digit's outcome; payment
another digit's payments from the output; tgt another digit's answer. The no-trace arm d1n
is the time arms' control (a late error on this digit's marks).

**Scoring**: every wiring written into the real born sheet (`blobmnist ceiling
--wiring-load`, 3,000 digits of regulation with learning off, the inside-learning test's
reader, fit on 60,000 and scored on the 10,000 test digits), MNIST and the pair test
(`--pair 2 --pair-mix 1`). The model's own reader is given beside it and not used to rank.
