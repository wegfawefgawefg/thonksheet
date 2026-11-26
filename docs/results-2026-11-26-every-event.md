# Every-event counters in the engine's one-hop rule (2026-11-26)

Queue item 2l, idea 178, following `docs/results-2026-11-25-rule-ladder.md` (idea 167).
**Backprop is a measuring stick here, never the project's method.** The ladder found, in an
offline model of the cells, that the one-hop rule keeps most of what backprop finds if its
hidden counters step on every marked input instead of with a small chance, and that wiring,
written into the real sheet, reads 0.961 on MNIST and 0.761 on the pair test. Nobody had run
that rule inside the engine. This job does.

**The answer: run as a live rule inside the real sheet, the one-hop rule with every-event
counters reads 0.951 on MNIST (0.955 with every cell kind learning; the written-in sheet
0.961, born 0.925) and 0.770 on the pair test (written-in 0.761, born 0.677), but only when
its error comes from an output that is right about nine times in ten and names just the
right line and the lines confused with it; the piece that gets it there is that error
together with the every-event counters, neither works alone, and the sheet's own output
cannot supply it, so with the engine's own errors every-event counters read under the old
rule on MNIST.**

## Short version

Fresh linear reader after a 60,000-digit life, test digits, MNIST / pair test, hidden spikes
per 1,000 cell-ticks on MNIST in brackets.

| | MNIST | pairs | |
|---|---|---|---|
| born | 0.925 [33] | 0.677 | |
| the engine's rule as it was (from the answer, chance 2^-9, 6 bits) | 0.938 [38] | 0.698 | hop-real doc |
| competitive learning | 0.952 [65] | 0.726 | compete doc |
| **live: every event, an outside output's error, fine under 0.1** (`out`) | **0.951** [43] | 0.712 | 8 seeds |
| **live: the same, fine under 0.3** (`outf3`) | 0.945 [41] | **0.770** | 8 seeds |
| live: the same, the outside output in whole numbers (`outint2`) | 0.942 [38] | 0.768 | 8 seeds |
| live: `out` with every cell kind learning (`outall`, the model's T7x rule) | **0.955** [42] | 0.686 | 8 seeds |
| the every-event wiring learned in the model and written in | 0.961 [43] | 0.761 | ladder doc |
| the perfect teacher's wiring written in | 0.966 | 0.785 | idea 165 |

- **How close.** MNIST: 0.951, every one of 8 seeds within 0.001 of that, 0.010 under the
  written-in sheet (0.955 and 0.006 under with every cell kind learning, which costs the
  pair test 0.026); seven to eight tenths of its gain over born. Pairs: 0.770, 0.009 over the
  written-in sheet, at a fine zone picked on validation; at the ladder's own zone 0.712.
  Every seed of `out`, `outf3` and `outint2` is over born, over its coin control (which
  gets +0.000 to +0.007) and over a shuffled-digit life (which ends under born), but one
  pair seed of `out` level with its coin; `outall` is on MNIST and is level with its coin
  on pairs. With the rate held at target the MNIST gain is the same (+0.025 over born at
  31, 4 of 4).
- **Which piece.** Not the counters alone and not the error alone (test, seeds 5-7; the
  outside error at fine 0.1 or 0.3, the old counters at chance 2^-3 or 2^-5):

  | | MNIST | pairs |
  |---|---|---|
  | old counters, old error (the engine's rule) | 0.938 | 0.696 |
  | every-event counters, old error | 0.919 | 0.708 |
  | old counters, the outside error | 0.935 to 0.942 | 0.709 to 0.722 |
  | every-event counters, the outside error | **0.951** (fine 0.1) | **0.775** (fine 0.3) |

  Every-event counters on the answer-alone error take MNIST under born: the cells drop their
  pixels (2.4 a cell from 7.2) and listen to each other.
- **The error has to come from a good output, and the sheet has none.** The outside output
  is right 0.91 of the time during the MNIST life. The sheet's own readout is right 0.68;
  its errors (told the right button on bad steps, or each line's miss by share or by margin)
  leave every-event counters at 0.926 to 0.930, under the old rule. In a life where a fresh
  reader of the taught sheet ends at 0.946, the sheet's own readout stays at 0.69.
- **What in the error matters: which lines, and when.** The right line and the one or two
  lines confused with it, on the digits the output is unsure of (half of them on MNIST).
  The whole answer on those digits: 0.932. The answer on every digit: born. The size of the
  fine zone is a real knob and the two tests want different values (MNIST 0.1, pairs 0.3;
  no one value gets both).
- **It needs no floats.** A perceptron with 16-bit weights and a margin as the outside
  output gives 0.942 / 0.768, the closest to one setting for both tests.
- **Who learns.** Hidden excitatory cells alone are best on pairs; adding inhibitory,
  pacemaker and motor cells gives +0.004 on MNIST and -0.03 on pairs. The model had it the
  other way.
- **The model's other differences close nothing**: re-picking every 64 verdicts is level
  and a fifth faster; one mark a digit is worse; 9 bits are worse; 16 bits are the same
  runs as 12.
- **Nothing lost on the toys, some gained.** With every-event counters parity of 3 goes
  0.847 to 0.919 (8 of 8 seeds), xor 0.997 to 0.999, the held-out calc columns 0.871 to
  0.933 and 0.660 to 0.724.
- **Cost.** 132 words a neuron for 12-bit planes against 88 for the byte store. A judged
  life runs at 92% of the old rule's speed; planes stepping every source are faster than
  bytes stepping one in eight.
- **What it means for the queue.** The counters are no longer the missing piece. The output
  is: the sheet needs a readout that is right about nine times in ten online and says, per
  line, when it was too close. That is item 9 with a sharper aim.

## Predictions (written and committed before any scoring run)

Seen before writing these: the three docs named above with their numbers (written-in 0.961 /
0.761; born 0.924 / 0.673; the engine's rule from the answer 0.938 / 0.698, its coin 0.932 /
0.690; competitive learning 0.952 / 0.726; the model's rungs), the code, and two things run
here that are not scoring runs:

- A 12,000-digit taught life on MNIST and on the pair test with the old rule, to see what
  the motor lines' spike counts look like (needed to define the sheet's own error). The
  readout in that life is right 0.66 of the time on MNIST and 0.49 on pairs (chance); the
  right line holds about 17% of the lines' spikes on MNIST when it wins, so "share of the
  spikes less the answer" calls the right line too low on every digit.
- Eleven 2,000-digit lives (validation seed 1, reader fit on 1,000 digits), one per new
  code path, to see that each runs. Their scores say nothing (0.78 to 0.83, born at that
  fit size is about there too). What they showed: with every-event steps a taught cell
  moves about 100 counters a verdict; born inputs kept fall to 0.09 to 0.14 within 2,000
  digits (the old rule: 0.77); the rate rises to 43 in some arms; a judged life runs at
  about 5,100 ticks a second with every-event counters against 5,600 with the old rule.

The arms (names as in `scripts/everyevent.py`), all on 12-bit plane counters stepped on every
marked input unless said:

| arm | what |
|---|---|
| old | the engine's rule as it was: from the answer (`hop_teach 3`), chance 2^-9, 6 bits |
| ev3 | piece 1 alone: every event, from the answer, hidden excitatory cells learn |
| ev3i, ev3all | piece 3 on it: inhibitory and pacemaker cells learn too; motor cells too |
| ev4, ev5 | piece 2 with the sheet's own output: each line's signed error from the lines' spikes, by share (4) or by margin (5) |
| out, outi, outall | piece 2 with the model's output: an outside softmax layer's signed error per line (a measuring stick, floats); `outall` is the model's T7x rule run live |
| out6all | the same errors and learners on the old counters (6 bits, chance 2^-3): the model's T7 |

Guesses, MNIST / pair test, test digits, seeds 5-12, fresh linear reader:

| arm | guess | chances |
|---|---|---|
| ev3 | 0.943 / 0.712 | above old (0.938 / 0.698) by 0.003 / 0.005 or more: 0.6 / 0.6; above its coin by 0.005 / 0.010 or more: 0.6 / 0.5 |
| ev3all | 0.940 / 0.715 | above ev3 on MNIST: 0.35 (the model's T8x fell under born there); on pairs: 0.5 |
| ev4 or ev5, the better | 0.940 / 0.712 | above ev3 on MNIST by 0.002 or more: 0.3; on pairs within 0.005 of ev3: 0.7 (the readout is at chance there, so its error is the answer) |
| outall | 0.950 / 0.735 | above its coin by 0.010 / 0.020 or more: 0.7 / 0.6; above competitive learning (0.952 / 0.726): 0.35 / 0.6 |
| out6all | 0.940 / 0.705 | under outall on both tests: 0.75 |

- **The best live arm against the written-in 0.961 / 0.761**: within 0.005 on MNIST: 0.2;
  within 0.010 on pairs: 0.2; reaches both numbers: 0.07. My best guess for the gap that
  stays: 0.010 on MNIST, 0.025 on pairs.
- **Which piece gives the most when added to the engine's rule** (on pairs, where the room
  is): every-event counters 0.4; the outside output's error 0.35; who learns 0.1; none
  clearly 0.15.
- The sheet's own output's error (ev4, ev5) is worth less than the outside output's on
  MNIST: 0.75.
- The best arm's firing rate on MNIST while read is between 40 and 48 per 1,000 cell-ticks:
  0.6 (the written-in sheet: 43).
- Piece 4 (re-pick every 64 verdicts, or one mark a digit) lifts `outall` by 0.003 or more
  on MNIST on the validation slice: 0.3.
- Toys with every-event counters, told the answer on bad steps: parity of 3 fresh output
  0.80 (the old rule 0.847), 0.80 or more: 0.5; xor 0.99 or more: 0.7; calc xor4 held-out
  0.82 or more (old 0.871): 0.5.
- The sheet's own readout during a judged MNIST life (last tenth) is lower with every-event
  counters than with the rule off by 0.03 or more: 0.6 (the cells under it keep changing).
- A judged MNIST life with every-event plane counters runs at 85% or more of the old rule's
  ticks a second: 0.7.

Why not higher: the written-in wiring was learned on noise-free cells with one clean mark a
digit and then met the real sheet once; the live rule has to learn through the sheet's own
noise, with marks from a 12-tick window, re-picking after every digit, and regulation
chasing the changing cells. And the engine's rule read under its own model twin on pairs
(0.698 against 0.721), so some live cost is already known.

Seen after the predictions and before the report runs: a smoke run (one seed, 3,000-digit
lives) and the validation picks below. The arms grew from them: `hop_balance`, the forms of
the outside error, its fine zone, the whole-number output, `ev2`, and the old-counter rows
of the pieces table were all added after the first validation table showed that every-event
counters alone made MNIST worse and the outside error made it better.

## What was built

All off by default; new genome numbers are at the end of the genome and of
`genome_args.cpp`. With them off the four reference lives are byte-identical
(`out/everyevent/refcheck.sh`) and five short lives under the hop rule's old settings match
the main checkout's build line for line (`out/everyevent/hopcheck.sh`).

**Piece 1, every-event counters as bit planes** (`--g.hop_bits 12 --g.hop_shift 0`,
`src/blob/hop_planes.cpp`). With `hop_bits` above 0 the one-hop rule keeps its own counters
instead of the tag store's bytes: for each neuron, for each of its 11 slots, `hop_bits`
words; plane 0 holds the lowest bit of the slot's 64 counters, the last plane the sign.
`hop_shift 0` makes every marked source step (the chance word is all ones and nothing is
drawn); any other `hop_shift` still works on planes.

- Step: one count up for a whole word of marked sources is a ripple carry down the planes
  (an AND and an XOR a plane), one down the same with a borrow; sources at the counter's
  ends are masked out first. No branch per source, no random word.
- Read-off: the cell wires its highest counters at its born counts, weak and strong, for
  excitatory and inhibitory sources apart, as `tag_rewire` does (finding 38). The highest
  N of a set are found a plane at a time from the top (a radix select: one AND and one
  popcount per slot per plane), so no counter is ever read out as a number. At the line
  the sources already wired stay first; chance is drawn only when some at the line must be
  left out.
- The cell re-picks after every verdict that moved its counters (no bounds are kept: the
  read-off costs less than keeping them), or with `--g.hop_rewire_every N` all the cells
  whose counters moved re-pick together every N verdicts.
- Not for use beside another rule that re-picks from the tag store (competitive learning,
  the tags): the two stores would fight over one cell's wiring.

**Piece 2, the output's own signed error per line** (`src/blob/hop_lines.cpp`, which now
holds every button error and coin; `blob_hop_votes` in `blob.hpp`, sent by
`plug_hop_lines`). The ladder's rungs T5 and T6 take the sign of the model's softmax output
less the answer on each line, and 0 ("fine") when that is under 0.1. The engine's output is
ten groups of motor cells and a count of each group's spikes in the step; it has no softmax.
Two forms on those counts, every verdict teaching:

- `hop_teach 4`, by share: a line's share of all the lines' spikes less its target (1 for
  the right line, 0 for the others); under `hop_fine` 32nds either way (default 3, 0.094) is
  fine. This is the model's formula with shares in place of softmax outputs.
- `hop_teach 5`, by margin: a wrong line is told "lower" when its spikes are within
  `hop_fine` 32nds of the right line's or above them, the right line "higher" when any wrong
  line is; a right line that wins by more than the margin teaches nothing. This is what the
  model's formula does when the output is sharp. Run with `--g.hop_fine 8` (a quarter).

Where they differ from the model: the model's output is a trained softmax layer over all
3,072 cells, right 0.94 of the time during its life, and sure of itself, so most digits are
"fine" late in the life and the errors that remain are about the hard digits. The sheet's
readout is right about two times in three on MNIST and the winning line holds about a sixth
of the spikes, so by share the right line is told "higher" on every digit and four or five
wrong lines "lower" (it is the answer with the loud wrong lines added); by margin about six
digits in ten teach. On the pair test the sheet's readout is at chance (0.49), so both forms
are the answer on nearly every step.

Because of that a third form was built, **the model's output itself** (`blobmnist ceiling
--life-outside 1`, `src/mnist/mnist_outside.cpp`): a softmax layer outside the sheet reads
every non-sensor cell's spike count of the digit, learns by the model's delta rule (a step of
0.3 / (1 + the squared size of its input)) a digit at a time, and sends each line's error to
the rule as a sign, fine under 0.1, through `blob_hop_errors` (idea 164's entry for error
lines that are not buttons; `hop_teach` is set to 0 so the buttons send nothing). It uses
floats and is no part of the blob: it is a measuring stick for what the rule does when it is
given the errors the model's rule had. A life under it is not judged, so the sheet's own
readout does not learn and the motor cells are free to learn as plain cells. Its fine zone
is a flag (`--outside-fine-1000`). Three more forms of it were added after the first
validation runs, to find what in its error matters: `--outside-form 1` (it only decides
when: on a digit it is unsure of the cells get the whole answer, the right line higher and
every other line lower), `--outside-form 2` (no fine zone: the answer as signs on every
digit), and `--outside-form 3`, **the same job in whole numbers**: a 16-bit weight per line
per cell, a line's vote is its weights times the cells' spike counts, and a perceptron step
with a margin: a wrong line whose vote is within `--outside-margin` steps' worth of the right
line's loses the counts and is told "lower", the right line then gains them and is told
"higher". No floats, no softmax, no step size. It is still outside the blob.

**Piece 3, who learns** (`--g.hop_who`): added bits, 1 inhibitory cells, 2 pacemaker cells,
4 motor cells. With 4 the readout's own rule is skipped (`tick.cpp`), since a motor cell
cannot be taught by both; in a judged life those arms keep the motor cells after the life
(`--life-reborn 0`), as the written-in sheet kept the model's. With the sheet's own output
as the error (`hop_teach 4`, 5) the motor cells are that output, so only inhibitory and
pacemaker cells can be added (`hop_who 3`).

**Piece 4, the model's other differences**: `--g.hop_rewire_every 64` (cells re-pick
together every 64 verdicts, as the model's blocks), `--g.hop_loose 1` (one mark a verdict: a
source on at any tick of the window is marked for a cell near at any tick of it; the model
has one mark a digit), `--g.hop_bits 9`.

**A guess that was wrong, kept as a number** (`--g.hop_balance 1`): each cell is taught
"more" and "less" equally often; it keeps a count of one less the other, and a verdict that
would take the count past the bound is skipped. One byte a neuron. Built after the first
validation runs to test why the answer-alone error fails with every-event steps; it does
not help (below).

**Coin controls.** `hop_teach 3` keeps its coin (`hop_coin 1`: the right line drawn at
random). For the signed errors `hop_coin 3` is new: the cells get the errors of another
verdict, drawn from the last 256, so the errors keep their counts and pairings and lose only
their tie to what was shown. That is the ladder's coin. The outside output has the same
(`--life-outside-coin 1`); it still learns from its own errors.

## Hardware

- **Memory.** Counters as planes: `hop_bits` x 11 words a neuron. 12 bits: 132 words (1,056
  bytes) against the byte store's 88 words (704 bytes), 1.5 times; 9 bits: 99 words, 1.1
  times. The MNIST sheet (4,096 neurons): 4.3 MB at 12 bits. The eight bytes of bounds a
  neuron of the byte path are not needed. The error wires still cost nothing (a hash).
- **Work per verdict, per taught cell.** Marks as before (up to 12 ORs of spike words a
  slot). Step: about 6 word operations a plane a slot (2 to mask the ends, 2 carry, 2
  borrow): 72 a slot at 12 bits, about 800 a cell. Read-off: four selects (wired and strong,
  two kinds) of 12 planes x 11 slots x about 3 operations: about 1,600. So about 2,400 word
  operations a taught cell a verdict, whatever the number of marked sources. The byte path
  stepping every marked source does about 100 counter steps a cell (measured), each a load,
  compare, store and bound update with branches, and a full `tag_rewire` whenever a bound is
  crossed, which at every-event steps is most verdicts.
- **Nothing on the tick's path changes**: the near maps are the old rule's; with `hop_who`
  the mask of learners is three more ORs a tile.
- **No random words** in the step. Chance is drawn only at ties in the read-off.
- **Measured** (`out`, an MNIST life): half the digits teach; a teaching verdict reaches
  about 470 cells and moves about 88 counters in each, 41,000 counter steps a teaching
  verdict (the old rule: 137 a verdict), 1.2 billion in the life; 1.4 million re-picks
  changed a cell's wiring. Speeds are in their own section below.

## Which pieces matter (validation slice, seeds 1 and 2)

Every arm was first run on the validation slice (train on digits 0-49,999, score on
50,000-59,999), seeds 1 and 2, a 60,000-digit life, then a fresh linear reader of the
non-sensor cells. Nothing here uses the test digits or seeds 5-12; the arms for the report
were picked from this table. MNIST / pair test. "rate": hidden spikes per 1,000 cell-ticks
while read; "pixels": pixel inputs a hidden excitatory cell (born 7.2); "output": the share
right in the last tenth of the life of the output that makes the errors (the sheet's own
readout, or the outside output); "teach": the share of the life's digits on which any error
was sent. `scripts/everyevent.sh pick`, `pick2` to `pick6`, `pick4`.

| arm | what | MNIST | pairs | rate | pixels | output | teach |
|---|---|---|---|---|---|---|---|
| born | | 0.9259 | 0.6756 | 33 / 33 | 7.2 / 7.2 | | |
| old | the engine's rule: from the answer, chance 2^-9, 6 bits | 0.9390 | 0.6919 | 38 / 38 | 5.6 / 6.4 | 0.67 / 0.51 | all |
| old, coin | | 0.9329 | 0.6971 | 37 / 38 | 5.6 / 6.5 | | |
| **piece 1 alone** | | | | | | | |
| ev3 | every event, 12-bit planes, from the answer | 0.9210 | 0.7097 | 32 / 34 | 2.5 / 5.2 | 0.61 / 0.54 | all |
| ev3, coin | | 0.9228 | 0.6937 | 32 / 37 | 6.5 / 7.0 | | |
| ev3b8 | the same in the byte store at 8 bits | 0.9113 | 0.6911 | 31 / 32 | 4.1 / 5.0 | 0.65 / 0.52 | all |
| ev3bal | ev3, each cell taught "more" and "less" equally often | 0.9215 | 0.6971 | 33 / 33 | 1.7 / 3.9 | 0.59 / 0.54 | all |
| **piece 3 on it** | | | | | | | |
| ev3i | and inhibitory and pacemaker cells learn | 0.9159 | 0.6963 | 32 / 35 | 2.5 / 5.2 | | all |
| ev3all | and motor cells too | 0.9060 | 0.6794 | 33 / 35 | 2.8 / 4.7 | | all |
| **piece 2, the sheet's own output** | | | | | | | |
| ev4 | its error by share (`hop_teach 4`) | 0.9278 | 0.7106 | 32 / 33 | 4.4 / 4.8 | 0.67 / 0.55 | all |
| ev5 | by margin, a quarter (`hop_teach 5 --g.hop_fine 8`) | 0.9324 | 0.7204 | 34 / 34 | 5.2 / 4.6 | 0.67 / 0.58 | 0.61 / 0.78 |
| ev5, coin | | 0.9231 | 0.6723 | 32 / 37 | 7.1 / 6.3 | | |
| ev5f4, ev5f16 | margin an eighth, a half | 0.9304, 0.9256 | 0.7138, 0.7251 | | | 0.67, 0.65 / 0.55, 0.60 | |
| ev5bal | ev5, taught "more" and "less" equally often | 0.9264 | 0.7216 | 34 / 33 | 2.1 / 4.4 | 0.66 / 0.59 | |
| ev2 | told the right button on bad steps (`hop_teach 2`) | 0.9348 | 0.6793 | 37 / 37 | 3.9 / 4.8 | 0.68 / 0.52 | 0.33 / 0.49 |
| **piece 2, the outside output** | | | | | | | |
| out | softmax, fine under 0.1 (the ladder's), hidden excitatory cells learn | **0.9513** | 0.7189 | 44 / 33 | 5.5 / 5.0 | 0.91 / 0.69 | 0.50 / 0.94 |
| outf05, outf2 | fine under 0.05, 0.2 | 0.9474, 0.9493 | 0.7082, 0.7383 | 44, 42 / 33, 34 | 5.5, 6.0 / 4.9, 5.7 | 0.90, 0.91 / 0.68, 0.71 | 0.65, 0.37 / 0.99, 0.77 |
| outf3 | fine under 0.3 | 0.9459 | **0.7795** | 41 / 35 | 6.5 / 6.8 | 0.91 / 0.73 | 0.31 / 0.58 |
| outf5 | fine under 0.5 | 0.9423 | 0.7169 | 40 / 36 | 6.9 / 5.7 | 0.91 / 0.68 | 0.20 / 0.35 |
| outwhen | the outside output only decides when; the cells get the whole answer | 0.9323 | 0.7189 | 34 / 33 | 7.2 / 5.0 | 0.89 / 0.69 | 0.59 / 0.94 |
| outsigns | no fine zone: the answer as signs, every digit | 0.9260 | 0.7068 | 32 / 33 | 6.2 / 4.8 | 0.88 / 0.67 | all |
| outint1, outint2 | the outside output in whole numbers, margin 1, 2 steps | 0.9425, 0.9444 | 0.7688, **0.7702** | 38 / 35 | 8.1 / 6.5 | 0.90 / 0.71 | 0.34 / 0.56 |
| outint, outint32 | margin 8, 32 steps | 0.9402, 0.9370 | 0.7664, 0.7500 | | | | |
| **piece 3 on out** | | | | | | | |
| outi | and inhibitory and pacemaker cells learn | 0.9540 | 0.7071 | 45 / 33 | 5.7 / 5.1 | 0.91 / 0.68 | |
| outall | and motor cells too (the model's T7x rule, live) | 0.9543 | 0.6892 | 43 / 34 | 5.0 / 4.6 | 0.91 / 0.66 | |
| outall, coin | | 0.9198 | 0.6817 | 36 / 36 | 5.8 / 6.1 | 0.82 / 0.60 | |
| **piece 1's worth beside the outside error** | | | | | | | |
| out6all | outall on the old counters (6 bits, chance 2^-3): the model's T7 | 0.9458 | 0.6927 | 34 / 33 | 5.0 / 5.0 | 0.88 / 0.62 | |
| out9 | out with 9-bit counters | 0.9441 | 0.6959 | 36 / 33 | 5.3 / 4.8 | 0.89 / 0.63 | |
| out16 | out with 16-bit counters | 0.9513 | 0.7189 | | | | |
| **piece 4 on out** | | | | | | | |
| out64 | cells re-pick together every 64 verdicts | 0.9524 | 0.7177 | 44 / 33 | 5.7 / 5.0 | | |
| outloose | one mark a verdict | 0.9474 | 0.6959 | 37 / 32 | 3.8 / 4.1 | | |
| outboth | both | 0.9458 | 0.7019 | 37 / 33 | 3.8 / 4.2 | | |
| outallboth | both, on outall | 0.9498 | 0.6643 | 37 / 33 | 3.9 / 3.8 | | |

(Two seeds: differences under about 0.005 on MNIST and 0.015 on pairs are not to be read.
The two seeds are within 0.005 of each other on MNIST for every arm but ev5f4 (0.008), the
coin of ev3 (0.007) and outf5 (0.006), and within 0.013 on pairs for every arm but ev5f4
(0.046), ev5 (0.030), out9 and ev5bal (0.021).)

What it says:

- **Every-event counters alone make MNIST worse than the old rule and worse than born.**
  From the answer (ev3): 0.921 against born 0.926 and old 0.939, and level with its own coin
  (0.923). The cells drop their pixels (2.5 a cell from 7.2; the coin's cells keep 6.5) and
  listen to each other. It is not the store: the byte store at 8 bits does the same
  (0.911). It is not that a cell hears "less" more often than "more": taught the two equally
  often (ev3bal, built to test that guess) it reads the same and keeps fewer pixels still
  (1.7). On the pair test the same arm gains (0.710 against old 0.692, coin 0.694).
- **The sheet's own output cannot supply the error.** By share 0.928, by margin 0.932, told
  the right button on bad steps 0.935: all under the old rule's 0.939 on MNIST. The readout
  that makes these errors is right 0.67 of the time on MNIST and 0.51 to 0.60 on pairs. On
  pairs the margin form reads 0.72 (+0.048 over its coin), level with `out`.
- **The outside output's error is what makes the rule work**: 0.951 on MNIST (+0.025 over
  born, +0.012 over old) with the same counters, marks, wires and learners as ev3. That
  output is right 0.91 of the time during the life.
- **What matters in that error is which lines it names, and when.** Give the cells the whole
  answer on the digits the outside output is unsure of (outwhen) and MNIST falls to 0.932;
  give it on every digit as signs (outsigns) and it is born (0.926). The working error names
  the right line and the one or two lines the output confused with it, on half the digits.
  Through a few +-1 wires that is, for each cell, one class against another on the digits
  that are hard; the whole answer as signs is, for each cell, nearly the same bit on every
  digit (its wire to the right line against the sum of all its wires).
- **The fine zone is a real knob and the two tests want different values.** MNIST is best at
  the ladder's 0.1 (0.947 at 0.05, 0.951 at 0.1, 0.949 at 0.2, 0.946 at 0.3, 0.942 at 0.5).
  The pair test reads 0.708 at 0.05 and 0.719 at 0.1, where the output is never sure and 94%
  or more of the digits teach, 0.738 at 0.2, **0.780 at 0.3** (58% teach), and 0.717 at 0.5
  (35%).
- **The outside output can be whole numbers.** A perceptron with 16-bit weights and a margin
  (no floats, no softmax; `--outside-form 3`) gives 0.944 / 0.770 at a margin of 2 steps:
  under the softmax form on MNIST by 0.007, level with the best softmax form on pairs. A
  smaller margin is better down to 1; at 0 it never starts.
- **Every-event counters are worth about 0.009 on MNIST once the error is right** (outall
  0.954 against the same on 6-bit chance counters 0.946) and nothing on pairs at fine 0.1
  (0.689 against 0.693). 9 bits are not enough live (0.944 / 0.696, where the model lost
  nothing at 9); 16 bits give the same runs as 12 to the last digit, so no counter passes
  12 bits here either.
- **Who learns: little on MNIST, a loss on pairs.** With the outside error, adding
  inhibitory and pacemaker cells gives +0.003 on MNIST and -0.012 on pairs; adding the motor
  cells +0.000 and -0.018 more. With the answer every added kind makes both tests worse.
  The model had it the other way (only the engine's learners cost MNIST 0.010 there).
- **The model's other differences do not close anything.** Re-picking every 64 verdicts is
  level (0.952 / 0.718) and a fifth faster; one mark a verdict is worse on both (0.947 /
  0.696): the engine's own tight marks (the source on at the tick the cell was near) are the
  better ones.

## The report (test digits, seeds 5-12)

Four arms from the table above, never run on these seeds before: `out` (fine under 0.1, the
ladder's: best on MNIST on validation among the arms with hidden excitatory learners),
`outf3` (fine under 0.3: best on pairs), `outint2` (the whole-number output), and `outall`
(`out` with every cell kind learning: the model's T7x rule as the model had it; no shuffled
life was run for it). All: 12-bit plane counters, every marked input steps, the error from
the outside output. A
60,000-digit life, then frozen, a fresh linear reader of the non-sensor cells fit on 60,000
digits, test digits. "coin": the same life, the cells given another digit's errors.
"shuffled": the same life on digits whose pixels are shuffled. "rate": hidden spikes per
1,000 cell-ticks while read; "pixels": pixel inputs a hidden excitatory cell; "kept": born
inputs still wired; "output": the outside output's share right in the last tenth of the life.
`scripts/everyevent.sh report`.

MNIST:

| arm | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate | pixels | kept | output |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| born | 0.9248 | 0.9272 | 0.9273 | 0.9239 | 0.9255 | 0.9192 | 0.9249 | 0.9278 | **0.9251** | 33.1 | 7.2 | 1.00 | |
| out | 0.9504 | 0.9513 | 0.9506 | 0.9511 | 0.9508 | 0.9515 | 0.9507 | 0.9499 | **0.9508** | 43.1 | 5.6 | 0.08 | 0.907 |
| out, coin | 0.9318 | 0.9264 | 0.9330 | 0.9339 | 0.9320 | 0.9311 | 0.9322 | 0.9342 | 0.9318 | 38.8 | 6.1 | 0.09 | 0.881 |
| out, shuffled | 0.9114 | 0.9080 | 0.9089 | 0.9024 | 0.9083 | 0.9109 | 0.9123 | 0.9110 | 0.9092 | 31.1 | 3.0 | 0.09 | 0.208 |
| outf3 | 0.9454 | 0.9446 | 0.9458 | 0.9447 | 0.9415 | 0.9441 | 0.9469 | 0.9439 | **0.9446** | 40.7 | 6.2 | 0.09 | 0.904 |
| outf3, coin | 0.9248 | 0.9228 | 0.9228 | 0.9263 | 0.9231 | 0.9255 | 0.9279 | 0.9277 | 0.9251 | 36.8 | 7.8 | 0.11 | 0.875 |
| outf3, shuffled | 0.9157 | 0.9127 | 0.9144 | 0.9153 | 0.9132 | 0.9110 | 0.9138 | 0.9136 | 0.9137 | 30.9 | 3.7 | 0.10 | 0.209 |
| outint2 | 0.9390 | 0.9421 | 0.9413 | 0.9430 | 0.9432 | 0.9410 | 0.9403 | 0.9417 | **0.9415** | 37.8 | 8.3 | 0.11 | 0.902 |
| outint2, coin | 0.9257 | 0.9244 | 0.9259 | 0.9267 | 0.9293 | 0.9269 | 0.9246 | 0.9277 | 0.9264 | 36.1 | 8.4 | 0.11 | 0.876 |
| outall | 0.9558 | 0.9563 | 0.9522 | 0.9545 | 0.9549 | 0.9573 | 0.9532 | 0.9535 | **0.9547** | 42.2 | 5.2 | 0.08 | 0.906 |
| outall, coin | 0.9144 | 0.9182 | 0.9189 | 0.9178 | 0.9133 | 0.9095 | 0.9179 | 0.9232 | 0.9166 | 36.7 | 5.7 | 0.10 | 0.823 |

Pair test:

| arm | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate | pixels | kept | output |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| born | 0.6924 | 0.6724 | 0.6675 | 0.6784 | 0.6760 | 0.6838 | 0.6813 | 0.6675 | **0.6774** | 32.7 | 7.2 | 1.00 | |
| out | 0.7318 | 0.7119 | 0.7120 | 0.7125 | 0.7155 | 0.7163 | 0.7043 | 0.6910 | **0.7119** | 32.9 | 4.8 | 0.42 | 0.690 |
| out, coin | 0.6749 | 0.6802 | 0.6891 | 0.6780 | 0.6844 | 0.6893 | 0.6724 | 0.6910 | 0.6824 | 36.3 | 6.1 | 0.42 | 0.637 |
| out, shuffled | 0.6649 | 0.6763 | 0.6541 | 0.6784 | 0.6651 | 0.6632 | 0.6612 | 0.6624 | 0.6657 | 32.6 | 6.7 | 0.42 | 0.505 |
| outf3 | 0.7724 | 0.7796 | 0.7736 | 0.7678 | 0.7757 | 0.7688 | 0.7617 | 0.7621 | **0.7702** | 34.6 | 6.5 | 0.42 | 0.744 |
| outf3, coin | 0.6844 | 0.6778 | 0.6804 | 0.6786 | 0.6855 | 0.6746 | 0.6854 | 0.6861 | 0.6816 | 36.1 | 6.0 | 0.42 | 0.634 |
| outf3, shuffled | 0.6841 | 0.6679 | 0.6680 | 0.6802 | 0.6678 | 0.6642 | 0.6729 | 0.6706 | 0.6720 | 33.1 | 7.6 | 0.42 | 0.508 |
| outint2 | 0.7750 | 0.7663 | 0.7653 | 0.7819 | 0.7631 | 0.7651 | 0.7672 | 0.7580 | **0.7677** | 34.5 | 6.4 | 0.42 | 0.721 |
| outint2, coin | 0.6866 | 0.6799 | 0.6877 | 0.6849 | 0.6822 | 0.6782 | 0.6760 | 0.6789 | 0.6818 | 35.7 | 6.0 | 0.42 | 0.629 |
| outall | 0.6912 | 0.6807 | 0.6931 | 0.6882 | 0.6900 | 0.6840 | 0.6894 | 0.6730 | **0.6862** | 33.5 | 4.5 | 0.42 | 0.662 |
| outall, coin | 0.6830 | 0.7002 | 0.6832 | 0.6826 | 0.6770 | 0.6845 | 0.6834 | 0.6805 | 0.6843 | 35.8 | 6.0 | 0.42 | 0.616 |

Seed by seed:

| difference | MNIST | seeds above | range | pairs | seeds above | range |
|---|---|---|---|---|---|---|
| out - born | +0.0257 | 8 of 8 | +0.0221 to +0.0323 | +0.0345 | 8 of 8 | +0.0230 to +0.0445 |
| out - its coin | +0.0190 | 8 of 8 | +0.0157 to +0.0249 | +0.0295 | 7 of 8 | +0.0000 to +0.0569 |
| out - shuffled | +0.0416 | 8 of 8 | +0.0384 to +0.0487 | +0.0462 | 8 of 8 | +0.0286 to +0.0669 |
| outf3 - born | +0.0195 | 8 of 8 | +0.0160 to +0.0249 | +0.0928 | 8 of 8 | +0.0800 to +0.1072 |
| outf3 - its coin | +0.0195 | 8 of 8 | +0.0162 to +0.0230 | +0.0886 | 8 of 8 | +0.0760 to +0.1018 |
| outf3 - shuffled | +0.0309 | 8 of 8 | +0.0283 to +0.0331 | +0.0982 | 8 of 8 | +0.0876 to +0.1117 |
| outint2 - born | +0.0164 | 8 of 8 | +0.0139 to +0.0218 | +0.0903 | 8 of 8 | +0.0813 to +0.1035 |
| outint2 - its coin | +0.0151 | 8 of 8 | +0.0133 to +0.0177 | +0.0859 | 8 of 8 | +0.0776 to +0.0970 |
| outall - born | +0.0296 | 8 of 8 | +0.0249 to +0.0381 | +0.0088 | 7 of 8 | -0.0012 to +0.0256 |
| outall - its coin | +0.0381 | 8 of 8 | +0.0303 to +0.0478 | +0.0019 | 5 of 8 | -0.0195 to +0.0130 |
| outall - out | +0.0039 | 8 of 8 | +0.0016 to +0.0058 | -0.0257 | 0 of 8 | -0.0406 to -0.0149 |

Beside them (8 seeds each, from their own docs, the same test and reader): the written-in
every-event wiring 0.9607 / 0.7607 (its born sheet 0.9241 / 0.6728, read after 3,000 digits
of regulation); the perfect teacher's wiring 0.9663 / 0.7853; competitive learning 0.952 /
0.726 at about 65 spikes per 1,000 cell-ticks; the engine's rule from the answer 0.9377 /
0.698 (pairs 4 seeds).

- **MNIST: 0.951 live against 0.961 written in.** Every seed is within 0.001 of the mean
  (0.9499 to 0.9515) and above born, coin and shuffled life; every seed is 0.008 to 0.013
  under the same seed of the written-in sheet. The gain over born is +0.026 live against
  +0.037 written in: seven tenths of it. With every cell kind learning (`outall`) it is
  0.955, +0.004 over `out` on every seed, 0.006 under the written-in sheet, eight tenths of
  its gain; that arm reads 0.686 on pairs, level with its own coin.
- **Pair test: 0.770 live against 0.761 written in**, with the fine zone at 0.3 (picked on
  validation); the whole-number output gives 0.768. Against the same seed of the
  written-in sheet `outf3` is above on 7 of 8 seeds (-0.003 to +0.020) and `outint2` on 8
  of 8 (+0.001 to +0.023). The gain over born is +0.093 against +0.088. At the ladder's own
  fine zone (`out`) the pair test reads 0.712.
- **The coins get little or nothing**: on MNIST +0.000 to +0.007 over born (the coin of
  `outall` -0.009), on pairs +0.004
  to +0.005; a shuffled-digit life ends under born on both tests (-0.011 to -0.016 on MNIST,
  -0.005 to -0.012 on pairs). The old rule's coin got half of its gain (hop-real doc); here
  nearly all of the gain is the errors being about the digit shown.
- **Rates**: MNIST 43 (`out`), 41 (`outf3`), 38 (`outint2`) against born 33; the written-in
  sheet ran at 43. Pairs 33 to 35 against 33. Held at the target rate the MNIST gain is the
  same (below).

## The pieces on the test digits (seeds 5-7)

The same test, three seeds, `scripts/everyevent.sh pieces`. Each row is a full life.

| arm | counters | error | MNIST | pairs | rate | pixels |
|---|---|---|---|---|---|---|
| born | | | 0.9264 | 0.6774 | 33 / 33 | 7.1 / 7.1 |
| old | 6 bits, chance 2^-9 | the answer | 0.9382 | 0.6961 | 38 / 38 | 5.6 / 6.4 |
| ev3 | every event | the answer | 0.9193 | 0.7076 | 32 / 33 | 2.4 / 5.1 |
| ev3, coin | | | 0.9238 | 0.6862 | 31 / 37 | 6.5 / 6.5 |
| ev2 | every event | the sheet's own: told the right button on bad steps | 0.9302 | 0.6700 | 35 / 36 | 4.1 / 5.2 |
| ev5 | every event | the sheet's own: by margin | 0.9262 | 0.7033 | 33 / 36 | 5.1 / 5.3 |
| ev5, coin | | | 0.9181 | 0.6766 | 31 / 35 | 6.9 / 8.0 |
| out6s3 | 6 bits, chance 2^-3 | outside, fine 0.1 | 0.9347 | 0.7091 | 34 / 33 | 4.7 / 5.0 |
| out6s5 | 6 bits, chance 2^-5 | outside, fine 0.1 | 0.9416 | 0.7215 | 41 / 34 | 6.1 / 5.9 |
| **out** | every event | outside, fine 0.1 | **0.9508** | 0.7186 | 43 / 33 | 5.6 / 4.9 |
| outf36s3 | 6 bits, chance 2^-3 | outside, fine 0.3 | 0.9345 | 0.7173 | 33 / 33 | 5.6 / 5.5 |
| outf36s5 | 6 bits, chance 2^-5 | outside, fine 0.3 | 0.9369 | 0.7167 | 39 / 35 | 6.3 / 6.0 |
| **outf3** | every event | outside, fine 0.3 | 0.9453 | **0.7752** | 40 / 35 | 6.2 / 6.5 |
| outint2 | every event | outside, whole numbers | 0.9408 | 0.7689 | 38 / 34 | 8.1 / 6.4 |
| outall | every event, every cell kind learns | outside, fine 0.1 | **0.9548** | 0.6883 | 42 / 34 | 5.1 / 4.5 |
| outall, coin | | | 0.9172 | 0.6888 | 37 / 37 | 5.6 / 6.2 |

(The old rule's three seeds are the hop-real doc's to the digit: 0.9379, 0.9379, 0.9387.
The 6-bit chances 2^-3 and 2^-5 are the ladder's picks for its T7 rung on MNIST and on
pairs; they were not picked again here, so both are shown.)

- **Neither piece works alone; together they do.** On MNIST: the old rule 0.938; every-event
  counters with the old error 0.919 (under born, 3 of 3); the outside error on the old
  counters 0.935 to 0.942 (+0.003 over the old rule at best); both 0.951 (+0.013 over the
  old rule, 3 of 3). On pairs: 0.696; 0.708; 0.717 to 0.722; both, at fine 0.3, 0.775
  (+0.079, 3 of 3).
- **Every-event counters are worth +0.009 to +0.016 on MNIST and +0.058 on pairs once the
  error is right** (every seed, against either chance), and nothing on pairs at fine 0.1
  (-0.003 to +0.009), where nearly every digit teaches.
- **The sheet's own output as the error source stays under the old rule on MNIST** (ev2
  -0.008, ev5 -0.012, 0 of 3 each) though it beats its coin (+0.008). On pairs the margin
  form reads 0.703 (+0.027 over its coin, 3 of 3); told the right button on bad steps reads
  under born there (0.670), where the readout that names the bad steps is at chance.
- **Every cell kind learning** (outall): MNIST +0.004 over `out` (3 of 3), the best MNIST
  number here; pairs -0.030 (0 of 3), level with its own coin.

## Why the old error fails with every-event steps

What the runs show (a reading from them follows; the reading was not tested directly):

- From the answer with every-event steps the cells stop listening to pixels: 2.4 to 2.5
  pixel inputs a hidden excitatory cell after the life (born 7.2; the old rule 5.6), and
  fewer than half the cells keep two pixels. The coin of the same arm, whose errors are as
  many and as large but not about the digit, keeps 6.5, about the share of pixels among
  the sources a cell may wire. So the real errors push the cells off the pixels.
- It is not a lopsided teaching bit. `hop_balance 1` was built on the guess that a cell told
  "less" on most digits loses every often-on source; with "more" and "less" made equally
  frequent for each cell the score is the same and the pixels fall further (1.7).
- The outside output's error does not do this (5.6 pixels, level with the old rule), and
  the same error without its fine zone does not teach at all (outsigns: born).

The reading: from the answer, every cell is told about the label of every digit, for the
whole life. The sources that best foretell a cell's teaching bit are then other cells taught
by the same label, not pixels, and with every mark counted the counters find them fast; the
cells end up listening to each other. With the old chance (2^-9) the counters hardly move in
a life (8 million steps against 2.5 billion), so the drift never got far, and nor did the
learning. An error with a fine zone is about what the output still gets wrong: it names the
right line and the line confused with it, stops when the output is sure, and so gives each
cell one class against another on the hard digits instead of the label for ever.

## The rate held at target (MNIST, seeds 5-8)

`out` fires at 43 per 1,000 cell-ticks while read against born 33, so the same arms were run
with the threshold regulation uncapped (`--g.threshold_cap 1000`, as the hop-real doc's
`targetcap`), which holds every sheet at 31. `scripts/everyevent.sh cap`.

| arm | 5 | 6 | 7 | 8 | mean | rate |
|---|---|---|---|---|---|---|
| born | 0.9239 | 0.9238 | 0.9191 | 0.9217 | 0.9221 | 31.3 |
| out | 0.9495 | 0.9482 | 0.9458 | 0.9460 | **0.9474** | 31.1 |
| out, coin | 0.9286 | 0.9263 | 0.9303 | 0.9314 | 0.9292 | 31.0 |

At matched rate the gain over born is +0.025 (4 of 4; +0.026 free) and over the coin +0.018
(4 of 4; +0.019 free). The gain does not ride on the rate. The pair arms run at 33 to 35
against born's 33 and need no such check.

## The toys and the calculation columns

The best arm's error (an outside output over MNIST's cells) does not exist for the toys, so
the check is on the piece that could have cost them, the counters: the hop doc's toy rule
(told the right button on bad steps, `hop_teach 2`, which on the toys is an error from an
output that learns them) on the old counters (6 bits, chance 2^-3) and on every-event
planes. Scored as the hop-real doc scores them: a 40,000-step life, the inside frozen, a
fresh readout's late score (first quarter in brackets), seeds 5-12; the calc columns by a
ridge reader after a 400,000-tick life, seeds 5 and 6. `scripts/everyevent.sh toys`, `calc`.

| | born | old counters | **every event** | its coin | shuffled input |
|---|---|---|---|---|---|
| xor | 0.855 (0.685) | 0.997 (0.951) | **0.999 (0.981)** | 0.900 (0.748) | 0.888 (0.738) |
| parity of 3 | 0.552 (0.531) | 0.847 (0.716) | **0.919 (0.840)** | 0.615 (0.555) | 0.564 (0.545) |
| calc xor4, held-out bits | 0.633 | 0.871 | **0.933** | 0.672 | 0.606 |
| calc add4, held-out bits | 0.565 | 0.660 | **0.724** | 0.555 | 0.547 |

Nothing was lost; parity of 3 gains 0.072 (8 of 8 seeds above the old counters, 0.895 to
0.957 against 0.810 to 0.894) and both held-out columns gain 0.06 (both seeds). The born and
old-counter rows are the hop-real doc's to the digit. Parity of 4 was not run.

## The sheet's own readout during the life (seeds 5-7)

The share of digits the sheet's own readout (the motor cells, learning by their rule in a
judged life) got right in the last tenth of the life, beside the fresh reader afterwards.
`outj` and `outintj` are `out` and `outint2` in a judged life, so the sheet's readout learns
while the outside output teaches the cells. `scripts/everyevent.sh online`.

| life | MNIST: own readout | fresh reader | pairs: own readout | fresh reader |
|---|---|---|---|---|
| the rule off | 0.683 | 0.9249 | 0.513 | 0.6714 |
| old | 0.680 | 0.9382 | 0.518 | 0.6961 |
| ev3 | 0.617 | 0.9193 | 0.550 | 0.7076 |
| ev2 | 0.643 | 0.9302 | 0.506 | 0.6700 |
| ev5 | 0.676 | 0.9262 | 0.550 | 0.7033 |
| outj | 0.688 | 0.9458 | 0.545 | 0.7170 |
| outintj | 0.679 | 0.9374 | 0.541 | 0.7668 |

- **The sheet's own readout does not follow.** On MNIST a fresh linear reader of the taught
  sheet gains 0.021 (outj) while the sheet's own readout in that life stays at 0.69, where
  it is with the rule off. On pairs it gains 0.03 over chance in every arm that teaches the
  cells well, whatever the size of the fresh reader's gain (0.717 or 0.767). The gap
  between what a linear reader gets from the sheet and what the sheet's readout gets (item
  9) is untouched, and wider.
- In a judged life `out` reads 0.005 lower (0.9458 against 0.9508) and `outint2` 0.003
  lower on MNIST, level on pairs: the hidden cells also hear the motor cells, which the
  readout's rule keeps changing and which are put back as born before the read.
- Every-event steps from the answer cost the sheet's own readout 0.066 on MNIST.

## Speed

One life at a time on this PC (the governor makes any number here rough), 20,000-digit
MNIST lives, ticks a second over the life. `scripts/everyevent.sh speed --jobs 1`. The
frozen sheet runs at about 58,000.

| judged life (the readout's rule runs) | ticks/s | | unjudged life (the outside output) | ticks/s |
|---|---|---|---|---|
| the rule off | 6,220 | | `out` | 35,300 |
| old (6 bits, chance 2^-9) | 5,950 | | `out64` (re-pick every 64 verdicts) | 44,200 |
| ev3 (every event, planes) | 5,500 | | `outint2` | 39,400 |
| ev3b6 (every event, bytes) | 5,220 | | `outall` (twice the learners) | 29,000 |
| outj | 5,760 | | `out6all` (bytes, chance 2^-3) | 25,500 |

- In a judged life the readout's own rule is nine tenths of the cost; every-event planes run
  at 92% of the old rule's speed and 88% of the rule off.
- Without the readout's rule the every-event rule on planes costs about 11 microseconds a
  tick over the frozen sheet (35,300 against 58,000), about 220 a digit, the near maps and
  the outside output's arithmetic included. Half the digits teach, about 470 cells each, so
  a taught cell costs well under a microsecond.
- Planes stepping every marked source are faster than bytes stepping one in eight
  (`outall` 29,000 against `out6all` 25,500, the same learners and errors): a word of
  sources at a time against a branch per source and a full re-pick at each crossing.

## What is real, what is suspect

- Real: the report (8 seeds, both tests; every seed of `out`, `outf3` and `outint2` above
  born, its coin and its shuffled life, but one pair seed of `out` level with its coin;
  `outall` on MNIST only); the rate-held check;
  the toys (8 seeds). The old path: four reference lives byte-identical, five hop lives the
  same as main's build, and the old rule's and born's numbers here equal the hop-real
  doc's to the digit.
- Real, with three seeds: the pieces table. Every difference it is quoted for has all three
  seeds on one side.
- **The working error does not come from the sheet.** It comes from a layer outside the
  blob that reads every non-sensor cell's spike count of the digit: a softmax in floats, or
  a perceptron in 16-bit whole numbers. The result is what the engine's rule does when
  given such an output's error. The engine has no such output today: its own readout is
  right 0.68 of the time on MNIST and at chance on pairs, and its errors leave the rule
  under the old one on MNIST.
- **No one setting gets both tests.** The fine zone was picked per test on the validation
  slice (two seeds): 0.1 gives 0.951 / 0.712, 0.3 gives 0.945 / 0.770. The whole-number
  output at one margin gives 0.942 / 0.768, the closest to one setting for both.
- The validation table is two seeds an arm; it picks arms and is not quoted for small
  differences. Balance, the margins, the loose marks, 9 and 16 bits, and who learns under
  the answer were run only there.
- The written-in numbers were read after 3,000 digits of regulation with learning off,
  beside a born sheet of 0.9241 / 0.6728; the live ones after a life, beside 0.9251 /
  0.6774. Compared as gains over their own born sheets: MNIST +0.026 live against +0.037;
  pairs +0.093 against +0.088.
- The 6-bit chances in the pieces table (2^-3, 2^-5) are the ladder's picks, not picked
  again here; a slower chance might do better on the old counters with this error (the
  old rule's own pick was 2^-9 with another error).
- The "why" section is a reading of pixel counts, not a test.
- Runs went to both machines; four runs done on both match line for line.
- Speeds are from one PC under a temperature governor.

## Not done

- **The output the rule needs, inside the sheet.** Nothing here gives the engine an output
  that is right nine times in ten and says which lines it confused. The whole-number
  perceptron shows it needs no floats; it is still a table outside the blob, 16 bits per
  line per cell, not motor cells with bit weights. That is item 9's job.
- `hop_teach 4` and 5 for pattern answers (the calc battery): the votes are only sent for
  button answers, so those forms teach nothing there.
- Every-event counters beside competitive learning or the tags: the plane store is the hop
  rule's own; two stores would fight over a cell's wiring.
- Why the pair test wants a wider fine zone than MNIST, and why 0.5 is as bad as 0.1 there,
  beyond the count of digits that teach (94%, 58%, 35%).
- Why adding inhibitory, pacemaker and motor learners costs the pair test 0.03 live when
  the model needed them.
- The remaining MNIST gap (0.951 or 0.955 against 0.961). The model's other differences
  (re-pick every 64, one mark a digit) do not explain it; what is left is that the model's
  cells are noise-free and its wiring met the real sheet only once.
- Parity of 4; longer lives; a second pass over the digits; the toys with the margin forms.
- The answer-alone error with a slower every-event step (wide counters at a chance between
  1 and 2^-9): the ladder says the chance loses, and it was left out.
- No `blobbench` change to report: with every new number off the tick is the old tick
  (2,650 to 2,980 ticks a second before, 2,700 to 2,790 after, 65,536 neurons, learning on).

## Script lines

Outputs in `out/everyevent/NAME/` (`pieces` and `online` write into `report/`); a finished
run is kept, so a line run again only runs what is missing. On stevejobs the same lines ran
with `THONK_MNIST=$HOME/thonk-hop2/data/mnist` in `thonk-everyevent`.

```
scripts/everyevent.sh pick                the first arms, validation, seeds 1, 2
scripts/everyevent.sh pick2               balance, margins, told the right button, which part of the outside error
scripts/everyevent.sh pick3               the outside output in whole numbers; 16-bit counters
scripts/everyevent.sh pick5               its margins 0 and 1
scripts/everyevent.sh pick6               the softmax output's fine zone: 0.05, 0.2, 0.3, 0.5
scripts/everyevent.sh pick4               re-pick every 64, one mark a verdict, 9 bits
scripts/everyevent.sh report              out, outf3, outint2 with coins and shuffled lives, test, seeds 5-12
scripts/everyevent.sh report --arms outall --coins outall --shuffled none      outall, seeds 5-12
scripts/everyevent.sh pieces              the pieces table, seeds 5-7
scripts/everyevent.sh online              the sheet's own readout in judged lives, seeds 5-7
scripts/everyevent.sh cap                 the rate held at target, MNIST, seeds 5-8
scripts/everyevent.sh toys                xor and parity of 3, seeds 5-12
scripts/everyevent.sh calc                calc held-out columns, seeds 5, 6
scripts/everyevent.sh speed --jobs 1      ticks a second, one life at a time
scripts/everyevent.sh report --table-only     print a finished table
scripts/everyevent.sh smoke --quick       one seed, short lives, every part
out/everyevent/refcheck.sh                the four reference lives against main's
out/everyevent/hopcheck.sh                five lives under the hop rule's old settings against main's build
```

Here a judged MNIST life with its read is about 6 minutes, an outside-output life about 3, a
pair life 3, a toy run under a minute, a calc run 2; three at a time. On stevejobs about
twice that, two at a time.

## Code

- `src/blob/hop_planes.cpp` (new): the bit-plane counters, their step and read-off.
- `src/blob/hop_lines.cpp` (new): every button error of the rule (the old three moved here
  unchanged, `hop_teach 4` and 5 added) and the coins (`hop_coin 3` added).
- `src/blob/hop_state.hpp` (new): the rule's state, shared by the three files.
- `src/blob/learn_hop.cpp`: the plane path beside the byte path, `hop_who`, `hop_balance`,
  `hop_rewire_every`, `hop_loose`. `src/blob/tick.cpp`: with `hop_who 4` the readout's rule
  is skipped. `src/blob/blob.hpp`: `blob_hop_votes`. `src/world/plug_hop.cpp` sends the votes.
- `src/blob/genome.hpp`, `genome_args.cpp`: `hop_bits`, `hop_who`, `hop_fine`,
  `hop_rewire_every`, `hop_loose`, `hop_balance`, at the end, all 0 or unused by default.
  `hop_teach 4`, `5` and `hop_coin 3` are new values of old numbers.
- `src/mnist/mnist_outside.cpp` (new): the outside output (`--life-outside 1`,
  `--life-outside-coin`, `--outside-form 1|2|3`, `--outside-fine-1000`, `--outside-margin`,
  `--outside-lr-1000`); `mnist_ceiling.cpp` (`--life-reborn 0`), `mnist_sheet.cpp`,
  `mnist_wiring.cpp`, `mnist_run.hpp`: its hooks.
- `scripts/everyevent.sh`, `everyevent.py` (reuse `inside.py`, `hop.py`, `hop2.py`).
- Checks: a clean build with no warnings from this tree's code; the four reference lives
  byte-identical with every new number off (`out/everyevent/refcheck.sh`, run again on the
  last commit); five lives under the hop rule's old settings the same as main's build line
  for line (`out/everyevent/hopcheck.sh`); a one-off check of the plane step against plain
  integers and of the read-off against a brute-force reading (7,032 cell-kinds);
  `scripts/everyevent.sh smoke --quick` end to end.
- Every file is under 500 lines (`learn_hop.cpp` 448).
- Left in the tree and found not to help: `hop_balance`, `hop_loose`, `hop_teach 4`. They
  are a few lines each and off; the coordinator may want them out.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, the every-event finding (idea 167's, "[likely ... not yet run in the
  engine itself]"): **run live, the every-event one-hop rule reaches MNIST 0.951 and pairs
  0.770 (8 seeds each, every seed above born, coin and shuffled life; written-in 0.961 /
  0.761; born 0.925 / 0.677), but only on the error of an output that is right nine times
  in ten and names just the right line and the lines confused with it.** That output was a
  layer outside the sheet (softmax, or a 16-bit perceptron with a margin: 0.942 / 0.768).
  [solid for what it measures: 8 seeds, two tests, rate-held check; the output is not the
  sheet's]
- `docs/findings.md`, new finding candidate: **every-event counters and a good error only
  work together.** Every-event counters on the engine's own errors read under the old rule
  on MNIST (from the answer 0.919, under born; from the readout's votes 0.926 to 0.930; old
  0.938); the outside error on the old chance counters gains 0.003 (0.942); both together
  0.951. Pairs: 0.696 old, 0.708, 0.717, both 0.775. [likely: 3 test seeds, 2 validation
  seeds, all the same way]
- New finding candidate: **what the error must be.** Which lines it names and when: the
  whole answer on unsure digits 0.932, the answer on every digit born; the fine zone is a
  knob and the tests differ (MNIST 0.1, pairs 0.3). From the answer alone, every-event
  steps make the cells drop their pixels and listen to each other (2.4 pixel inputs a cell
  from 7.2; the coin keeps 6.5). [hint: validation, 2 seeds; the reason is a reading]
- New finding candidate: **who learns is the model's one wrong call.** Live, more learner
  kinds give +0.004 on MNIST and -0.03 on pairs; the model had the engine's learners
  costing MNIST 0.010. [hint: 3 seeds]
- Finding 52 and "Where it stands": the sheet's own readout does not follow the taught
  cells (0.69 on MNIST in a life where a fresh linear reader ends at 0.946). The largest
  number on the board is now the same gap seen from the other side: the rule is held back
  by the output, not by the counters.
- `docs/plan.md`: item 2l done (this doc). Next, in order: (1) item 9, with a sharper aim:
  an output inside the sheet that is right about nine times in ten online and reports, per
  line, whether it was too close to the right line; the whole-number perceptron here is a
  working sketch (16-bit weight per line per cell, vote by spike counts, a step only on a
  thin margin); (2) feed it to `hop_teach 5` with `hop_bits 12 --g.hop_shift 0`; (3) one
  fine zone for both tests, or a zone that sets itself from the share of digits that teach
  (about a half on MNIST and on pairs at their best settings); (4) the hop rule's plane
  store shared with competitive learning, if the two are to run together (idea 179).
- `docs/ideas.md`: idea 178 done, with the above. New ideas: a margin perceptron as the
  sheet's output and the hop rule's teacher; a fine zone that regulates itself to a target
  share of teaching digits; "an error must be a residual": any label-only teaching signal
  with every-event steps turns the cells toward each other.
- `docs/design.md`, `genome.hpp` (core owner; edited here because the job is theirs):
  `hop_bits`, `hop_who`, `hop_fine`, `hop_rewire_every`, `hop_loose`, `hop_balance`,
  `hop_teach 4|5`, `hop_coin 3`; `hop_planes.cpp`, `hop_lines.cpp`, `hop_state.hpp`;
  `blob_hop_votes`. `hop_rewire_every 64` is free (level scores, a fifth faster) and could
  become the default with planes.
- `docs/notes-world.md`: `blobmnist ceiling --life-outside 1` and its flags,
  `--life-reborn 0`, `scripts/everyevent.sh`.
- `docs/predictions.md`: the predictions above and their scores.

## Predictions, scored

| prediction (MNIST / pairs) | outcome |
|---|---|
| ev3 0.943 / 0.712 | 0.919 / 0.708 (3 seeds): MNIST far off, under born |
| ev3 above old by 0.003 / 0.005: 0.6 / 0.6 | MNIST no (-0.019); pairs yes (+0.012) |
| ev3 above its coin by 0.005 / 0.010: 0.6 / 0.5 | MNIST no (-0.005); pairs yes (+0.021) |
| ev3all 0.940 / 0.715; above ev3 on MNIST 0.35, on pairs 0.5 | 0.906 / 0.679 (validation): under ev3 on both |
| the better of ev4, ev5: 0.940 / 0.712 | ev5 0.926 / 0.703 (ev2 0.930 / 0.670) |
| ... above ev3 on MNIST by 0.002: 0.3 | happened (+0.007), both still under the old rule |
| ... on pairs within 0.005 of ev3: 0.7 | right (0.703 against 0.708) |
| outall 0.950 / 0.735 | 0.955 / 0.686 (8 seeds) |
| outall above its coin by 0.010 / 0.020: 0.7 / 0.6 | MNIST yes (+0.038); pairs no (level) |
| outall above competitive learning (0.952 / 0.726): 0.35 / 0.6 | MNIST yes (0.955); pairs no (0.686; `outf3` 0.770 is) |
| out6all 0.940 / 0.705, under outall on both: 0.75 | 0.946 / 0.693 (validation): under on MNIST only |
| best live arm within 0.005 of 0.961 on MNIST: 0.2 | no: 0.951 (`out`), 0.955 (`outall`) |
| ... within 0.010 of 0.761 on pairs: 0.2 | yes, and past it (0.770), with a knob I had not thought of |
| ... reaches both: 0.07 | no |
| the gap that stays: 0.010 / 0.025 | 0.010 (or 0.006) / -0.009 |
| the piece that gives most on pairs: every event 0.4, the outside error 0.35, who learns 0.1, none 0.15 | the question was wrong: alone +0.012 and +0.021 to +0.025, together +0.079 |
| the sheet's own output's error worth less than the outside one's on MNIST: 0.75 | right (0.926 to 0.930 against 0.951) |
| best arm's rate on MNIST 40 to 48: 0.6 | right (43) |
| piece 4 lifts outall by 0.003 on MNIST: 0.3 | no (run on `out`: +0.001 and -0.004; on outall -0.005) |
| toys: parity of 3 0.80, 0.80 or more: 0.5 | 0.919 |
| xor 0.99 or more: 0.7 | 0.999 |
| calc xor4 held-out 0.82 or more: 0.5 | 0.933 |
| own readout in a judged MNIST life 0.03 lower than with the rule off: 0.6 | from the answer yes (-0.066); under the outside error no (+0.005) |
| a judged life on planes at 85% of the old rule's speed: 0.7 | right (92%) |

I had every-event counters as a piece that adds on its own (0.4 that it gives the most) and
they do not: on the engine's errors they take MNIST under born. I had the fine zone as a
detail of the error and it is half of the pair result. I guessed the pair gap would stay the
larger; it closed and MNIST's stayed. The toys I had losing a little; they gained.
