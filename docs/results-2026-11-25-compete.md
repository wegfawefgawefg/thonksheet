# Competitive learning, and the inside-learning test as a script (2026-11-25)

Queue items 1 and 2 of `docs/plan.md`. The one question: can a local rule, with no reward,
make the inside of the sheet learn features? Part 1 is the test as one script
(`scripts/inside.sh`); part 2 is the first reward-free candidate scored by it.

A note on words, after the owner's "do we need pretraining step lol": the machine has no
pretraining stage. The test below lets the sheet live a while under a rule, then freezes it
and fits a fresh reader; that freeze is only how the inside is measured. The rule itself is
meant to be on all the time, in the one life, beside the readout. That last case is scored
separately (step 4).

## Prediction (written before any scoring run)

Seen before writing this: two smoke runs only (validation, 2,000-digit life, 3,000 digits
fitted): competitive learning lifted the reader 0.885 to 0.899 and the pixel likeness of a
cell's sources 0.05 to 0.25; and the first full run of the script on the old co-activity
rule, seed 1: born 0.924, lived 0.936, shuffled control 0.925.

- MNIST, test, 60,000-digit life, fresh reader fit on 60,000: the lived sheet beats the born
  sheet's 0.924 by at least 0.005: probability 0.7. Best guess +0.010 (0.934), range +0.000 to
  +0.020. It does not reach the random projection's 0.943: probability 0.8.
- It beats its shuffled control by at least 0.005: probability 0.6.
- It beats the co-activity rule's lived sheet (0.936 on seed 1): probability 0.35.
- Calc (xor4, parity4, add4): no member's held-out column moves by more than 0.03:
  probability 0.75. The rule pulls cells toward input lines that are on together; these
  members need products of lines far apart, which nothing here rewards.

How it scored (the best setting, 8 seeds): beats born by 0.005 or more, right, and by much
more than guessed (+0.026, above the top of the range); stays under 0.943, wrong (0.952);
beats its shuffled control by 0.005 or more, right (+0.013, every seed); beats co-activity
wiring, right (0.952 against 0.937); calc held-out moves under 0.03, wrong (xor4 +0.06 to
+0.19, add4 up to +0.11).

## Short version

- **Yes: a reward-free local rule makes the inside learn features that a fresh reader can
  use.** Competitive learning with winners in the tile (setting B below), 60,000 digits
  with no reward, then frozen and read: **0.952** on the MNIST test set, seeds 5-12 (0.949
  to 0.954). The same sheet as born: 0.926. The same life on digits whose pixels are
  shuffled afresh for each digit: 0.940. Baselines: linear on pixels 0.922, random
  projection of the same bits 0.943, two-layer net 0.955. It beats the born sheet by 0.019
  to 0.031 and its shuffled control by 0.006 to 0.018 on every one of the 8 seeds, and it
  passes the random projection, which no sheet had.
- **The caveat is the firing rate.** The learned cells fire about twice their target
  (63 spikes per 1,000 cell-ticks against 33; threshold regulation hits its cap), the
  shuffled control's 44. With the cap raised so all fire at target, lived still beats born
  by 0.013, but beats the shuffled control by 0.012 without tile winners and by only 0.002
  with them (2 seeds each). The stroke structure is the same either way; how much a reader
  gains from it depends on letting those cells fire more.
- **The old co-activity wiring already passes the test too**, which nobody had measured:
  0.937 against born 0.924 and shuffled 0.926 (2 seeds). The earlier "a hair better" was a
  10,000-digit pass on validation. So reward-free inner learning was there all along; the
  new rule roughly doubles what it buys (+0.027 against +0.013).
- **What the cells learn is visible as a number.** The pixels a hidden cell listens to go
  from unrelated (mean correlation over pairs, on real digits, 0.05) to strokes (0.25), and
  from 4.1 to 2.5 pixels from their middle. After the shuffled life they stay at 0.05.
  Co-activity wiring gets halfway (0.16, 3.2 pixels).
- **On the calculation battery, held-out cases improve too** (bits right on held-out
  cases, born -> lived, seeds 5 and 6, the competitive settings: xor4 0.60 to 0.67 -> 0.73
  to 0.80, add4 0.52 to 0.61 -> 0.61 to 0.69): cells that listen to lines that are on
  together are AND detectors, which is the missing half of an exclusive-or for a linear
  reader. Parity of 4 has too few held-out cases to say anything.
- **In the one life, beside the readout, it does nothing for the readout.** Turned on from
  the first digit of `scripts/mnist.sh online`, the online curve with the rule on is the
  curve with it off, within seed noise, at every checkpoint to 120,000 digits (on: 0.723,
  0.697, 0.655 at 20,000, 60,000, 120,000; off on the same sheet: 0.720, 0.700, 0.676). It
  does not fight the readout the way the old inner rule did, and it does not help it: the
  readout's motor cells cannot use features that a softmax reader can.
- **It costs.** 3.6 times slower per tick on MNIST as set (14,300 against 51,400 ticks a
  second), because every update re-picks a cell's whole wiring from 704 counters. A
  quarter of the updates costs 1.9 times; held at the target firing rate, 2 times.
- **Unfinished:** the cost (a cheaper read-off), why the readout cannot use it, and the
  firing rate it leaves (see below).

## What was built

**Part 1, the test** (commit a67a88a, then 2 small follow-ups):

- `scripts/inside.sh NAME [rule flags]` (and `scripts/inside.py`): for MNIST, per seed,
  three runs of `blobmnist ceiling` on `scripts/mnist.sh`'s sheet and reader: (a) born,
  (b) after a life of `--life` digits (default 60,000) under the rule, no reward, (c) the
  same life with every digit's pixels shuffled by a permutation drawn fresh for that digit
  (same ink and timing, no shape that comes back), read on real digits. Fit on 60,000, test
  on 10,000. Beside them the baselines (run once, `out/inside/baselines.txt`). For calc,
  `blobprobe calc` born against after `--calc-life` ticks (default 200,000) on xor4, parity4,
  add4, seen and held-out. Table in `out/inside/NAME/table.txt`; finished runs are kept.
- `blobmnist ceiling --life N` (the older `--pretrain N` is the same): the life now runs the
  genome's own learning rules, whatever the learn mode is (it used to force learn mode 0;
  the default genome is learn mode 0, so default runs are unchanged). `--life-shuffle 1` is
  the control. Every tenth of the life it prints a line on the hidden cells' activity
  (spikes per 1,000 cell-ticks, share silent, share over 10 times the mean, tiles with no
  hidden spike) and on their wiring (below). `src/mnist/mnist_shuffle.cpp`,
  `src/mnist/mnist_wiring.cpp`; small edits in `mnist_ceiling.cpp`, `mnist_sheet.cpp`,
  `mnist_run.hpp`.
- `blobprobe calc --life N`: N ticks of the world with its verdicts never delivered, under
  the genome's own rules, then the frozen read as before. Held-out cases are shown in the
  life too (never with an answer).
- The wiring numbers, over hidden excitatory cells (not sensor, motor or pacemaker): inputs
  and strong inputs per cell; how many are pixel cells; the share of born inputs still
  wired; and for cells with two or more pixel inputs, **pixel likeness** (the mean
  correlation, over pairs, of the pixels a cell listens to, measured on 10,000 real training
  digits in the sheet's code) and **spread** (mean distance of those pixels from their
  middle, in pixels).

**Part 2, the rule** (`src/blob/learn_compete.cpp`, 98 lines; commit 079685a). Genome
numbers, all off by default:

- `compete_on 1`: a hidden cell (not sensor, motor or pacemaker; excitatory unless
  `compete_inh 1`) that fires is a winner. With chance 2^-`compete_shift` (3) it moves: for
  every excitatory source, the hidden counter gains a step if the slot read it active on
  this spike (at that slot's own delay, so exactly what drove the spike), and a silent
  source whose counter is above zero loses a step with chance 2^-`compete_down_shift` (2).
  Then the cell wires its top counters at the counts it was born with, excitatory and
  strong separately (the read-off the tags use, `tag_store.cpp`; finding 38). Moving toward
  one input means leaving another: the budget. Inhibitory sources are never moved.
- `compete_on 2`: also, with `tile_winners`, a cell still over threshold that lost its
  tile's competition (and did not fire in the last 6 ticks) steps its active sources down.
- The competition is the sheet's own: local inhibition always, and `tile_winners` (at most
  k cells of a tile keep their spike, the most driven) when set. The handicap that keeps
  cells from dying or taking everything is the threshold regulation that was already
  there: a cell's threshold rises on its spikes and falls while it is silent, so a cell
  that wins too often loses margin and a silent one gets easier. No new number was needed:
  no hidden cell went silent in any run, none fired over 10 times the mean.
- It runs at the top of the learning step, beside any learn mode. Learn mode 5 with no
  reward is "nothing else learns", which is how it is scored alone.

## How it differs from co-activity wiring

Co-activity wiring (`learn_wiring.cpp`, learn mode 0, what `--pretrain` used) is: on a
spike, by chance, grow one weak connection from a source that fired in the last 6 ticks
(up to 96), and drop idle weak connections; strong connections never move; every
non-sensor cell, motor and inhibitory included. That is Hebbian moving toward the input
on firing, so it is half of competitive learning already. What it lacks: a budget (a
cell's inputs grew from 34 to 59 over the life), evidence before commitment (one
coincidence wires a source; here a counter must climb past the wired ones), strong inputs
that move, and the exact causal read (the slot's delay, not "the last 6 ticks"). The
competition is whatever the sheet gives either rule; with `tile_winners 4` both get the
same. The ablation below (co-activity with the tile's winners) says the budget and
counters are worth +0.011 on MNIST and most of the calc gain.

## Scoring

All MNIST numbers: test set, fresh reader fit on 60,000 training digits, after a
60,000-digit life with no reward. `scripts/inside.sh NAME FLAGS`; per-run outputs in
`out/inside/NAME/` (copied to `out/compete/inside/`).

### Step 1: does the sheet stay alive?

Yes, in every setting, through 60,000 digits (1.2 million ticks): no hidden cell silent,
none over 10 times the mean, no tile without a hidden spike, from the first tenth of the
life to the end. The rate settles within the first 6,000 digits and stays flat.

But it does not stay at its target. The sheet is built for about 33 spikes per 1,000
cell-ticks; after the rule it fires 63 to 75. A cell that listens to pixels that are on
together gets far more drive while a digit is shown, and threshold regulation stops at
its cap (64). With the cap raised (`--g.threshold_cap 1000`, setting D) the rate is back
at target (31) and the gain is smaller (below). The higher rate is not itself the gain:
the born sheet pushed to 63 by a higher target reads worse, 0.910 and 0.909
(`scripts/inside.sh born-hot --g.sector_target_per_1024 64 --g.threshold_down_shift 8
--kinds born --only mnist`).

### Step 2: do the inputs change in a structured way?

| seed 1 | inputs | pixel inputs | born inputs kept | pixel likeness | spread (px) |
|---|---|---|---|---|---|
| born | 34.1 | 7.3 | 1.00 | 0.049 | 4.09 |
| co-activity, lived | 59.3 | 8.8 | 0.18 | 0.159 | 3.20 |
| co-activity, shuffled | 48.2 | 8.8 | 0.21 | 0.049 | 3.97 |
| compete (B), lived | 34.1 | 17.8 | 0.23 | 0.253 | 2.52 |
| compete (B), shuffled | 34.1 | 27.1 | 0.23 | 0.048 | 4.13 |
| compete at target rate (D), lived | 34.1 | 22.3 | 0.23 | 0.256 | 2.58 |

Yes. Both rules move cells toward pixels that are on together and close together; the
shuffled life moves just as much wiring (77 % of born inputs gone either way) but leaves
the pixels unrelated. Competitive learning turns most of a cell's inputs into pixels (17
of 34), the shuffled life even more (27 of 34: under shuffling every pixel is on equally
often, so every pixel beats a hidden source). Seeds 2 and 5-12 give the same numbers to
two decimals.

### Step 3: the inside-learning test

Seeds 1 and 2, MNIST; mean of 2:

| setting | flags | born | lived | shuffled | lived - born | lived - shuffled | rate |
|---|---|---|---|---|---|---|---|
| co-activity (old rule) | (none) | 0.9240 | 0.9372 | 0.9259 | +0.013 | +0.011 | 38 |
| co-activity, tile winners | `--g.tile_winners 4` | 0.9250 | 0.9411 | 0.9259 | +0.016 | +0.015 | 37 |
| A: compete | `--g.learn_mode 5 --g.compete_on 1` | 0.9240 | 0.9452 | 0.9298 | +0.021 | +0.015 | 75 |
| **B: compete, tile winners** | A + `--g.tile_winners 4` | 0.9250 | **0.9523** | 0.9387 | +0.027 | +0.014 | 63 |
| C: B with losers | `--g.compete_on 2 --g.tile_winners 4` | 0.9250 | 0.9476 | 0.9394 | +0.023 | +0.008 | 62 |
| D: A at target rate | A + `--g.threshold_cap 1000` | 0.9201 | 0.9340 | 0.9224 | +0.014 | +0.012 | 31 |
| E: B at target rate | B + `--g.threshold_cap 1000` | 0.9221 | 0.9349 | 0.9329 | +0.013 | +0.002 | 31 |
| **B5: B, a quarter of the updates** | B + `--g.compete_shift 5` | 0.9250 | **0.9536** | 0.9397 | +0.029 | +0.014 | 63 |

("rate": the lived sheet's hidden spikes per 1,000 cell-ticks while it is read; the born
sheets fire 31 to 33.)

- Every setting passes the test: lived beats born and beats its own shuffled control.
- B is the best: 0.952. The tile's winners help both rules (+0.004 for co-activity, +0.007
  for competition); with them, the budget and counters add +0.011 over co-activity.
- The shuffled control matters. With winners in the tile, the shuffled life also reads
  better than born (0.939 against 0.925): it turns cells into listeners of random pixel
  sets, a random projection of the input, which is worth about what the real random
  projection is worth (0.943). What only real digits give is the rest: +0.013 to +0.014.
- Losers moving away (C) hurt: they lower lived and raise the shuffled control.
- **The caveat: the firing rate.** B's lived sheet fires 63, its shuffled control 44, the
  born sheet 33. Held at target rate, so all three fire 31, the margin over the control
  is +0.012 without tile winners (D) and only +0.002 with them (E; seed 2 is -0.001), and
  lived scores 0.934 to 0.935 in both. So with winners, much of B's margin over its control
  comes with the higher rate that the learned cells reach and the shuffled ones do not. The
  rate alone is not the gain (the born sheet at 63 reads 0.910), and the stroke structure
  is the same at target rate (pixel likeness 0.25), but a reader of those cells only beats
  a random-pixel version of them by much when they are allowed to fire more. Two seeds
  each; this is the least settled part of the result.
- A quarter of the updates (B5) is as good as B or better (0.954 against 0.952, 2 seeds)
  at half the cost. It is the setting to use; its 8-seed report is below B's.

**Seeds 5-12, setting B** (`scripts/inside.sh b-report --g.learn_mode 5 --g.compete_on 1
--g.tile_winners 4 --only mnist --seeds 5-12`; 5-8 here, 9-12 on stevejobs, the same
binary results):

| seed | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean |
|---|---|---|---|---|---|---|---|---|---|
| born | 0.9252 | 0.9287 | 0.9259 | 0.9217 | 0.9277 | 0.9231 | 0.9296 | 0.9291 | 0.9264 |
| lived | 0.9524 | 0.9532 | 0.9525 | 0.9523 | 0.9525 | 0.9536 | 0.9487 | 0.9512 | **0.9521** |
| shuffled | 0.9386 | 0.9355 | 0.9403 | 0.9391 | 0.9412 | 0.9402 | 0.9427 | 0.9385 | 0.9395 |

Lived minus born +0.019 to +0.031 (mean +0.026); lived minus shuffled +0.006 to +0.018
(mean +0.013), every seed. Against the baselines: linear on pixels 0.922, the same reader
on the bits 0.912 to 0.916, random projection 0.943, two-layer net 0.955. It closes 0.026
of the 0.028 between the born sheet and the two-layer net.

**Calc** (seeds 5 and 6; bits right, seen / held-out; ridge reader on the frozen sheet;
200,000-tick life with no verdicts; no shuffled control here):

| member | inputs | born (B's sheet) | co-activity + winners | B | A | D |
|---|---|---|---|---|---|---|
| xor4, 5 | 0.53 / 0.41 | 0.70 / 0.67 | 0.74 / 0.70 | 0.80 / 0.73 | 0.81 / 0.74 | 0.85 / 0.80 |
| xor4, 6 | 0.53 / 0.34 | 0.69 / 0.66 | 0.76 / 0.71 | 0.82 / 0.77 | 0.82 / 0.77 | 0.82 / 0.78 |
| add4, 5 | 0.60 / 0.49 | 0.66 / 0.61 | 0.66 / 0.62 | 0.74 / 0.63 | 0.75 / 0.69 | 0.72 / 0.66 |
| add4, 6 | 0.62 / 0.45 | 0.65 / 0.52 | 0.65 / 0.50 | 0.75 / 0.63 | 0.73 / 0.61 | 0.72 / 0.61 |
| parity4, 5 | 0.61 / 0.25 | 0.71 / 0.13 | 0.69 / 0.17 | 0.76 / 0.15 | 0.86 / 0.21 | 0.87 / 0.17 |
| parity4, 6 | 0.58 / 0.23 | 0.67 / 0.11 | 0.63 / 0.12 | 0.79 / 0.05 | 0.93 / 0.17 | 0.82 / 0.01 |

E (B at target rate): xor4 0.82 / 0.76 and 0.79 / 0.75, add4 0.75 / 0.67 and 0.74 / 0.59,
parity4 0.84 / 0.20 and 0.84 / 0.10, from its born 0.69 / 0.65, 0.68 / 0.63, 0.63 / 0.57,
0.63 / 0.56: on calc the gain holds at target rate.
(A and D's born sheet has no tile winners: xor4 0.65 / 0.64 and 0.66 / 0.63, add4 0.63 /
0.57 and 0.62 / 0.56.) Held-out cases gain with seen cases on xor4 and add4, so it is not
a table: xor4 held-out 0.63 -> 0.80 at best. Parity of 4 has 4 held-out cases, each the
opposite of its nearest seen one; the seen column rises (0.67 -> 0.93) and held-out stays
at or below chance, as it does for every learner in the battery. Without a shuffled
control on calc, part of this may be what any life on these inputs does (on MNIST that
part was about half).

### Step 4: on from the first digit of the rewarded online life

The owner's correction: no pretraining stage. So the rule on from digit one of
`scripts/mnist.sh online` (the readout learning from good and bad, learn mode 5), the same
life, against off, seeds 5 and 6. Test accuracy at each checkpoint, mean of the 2 seeds
(each seed in `out/compete/online/`):

| digits seen | 1,000 | 5,000 | 20,000 | 60,000 | 120,000 |
|---|---|---|---|---|---|
| off, `scripts/mnist.sh online` | 0.556 | 0.713 | 0.718 | 0.693 | 0.667 |
| off, `--g.tile_winners 4` | 0.552 | 0.716 | 0.720 | 0.700 | 0.676 |
| **on**, `--g.tile_winners 4 --g.compete_on 1` | 0.539 | 0.686 | 0.723 | 0.697 | 0.655 |
| off, winners, `--g.threshold_cap 1000` | 0.558 | 0.715 | 0.724 | 0.707 | 0.699 |
| **on**, winners, cap 1000 | 0.514 | 0.712 | 0.714 | 0.701 | 0.691 |

Seeds differ by up to 0.04 at a checkpoint, so these rows are all the same curve. The
rule neither helps the readout nor fights it: the old inner rule cost the feature tasks,
this one costs nothing, but a sheet that a fresh softmax reads at 0.952 is read by the
sheet's own readout at 0.72, the same as the sheet as born. The readout (about 100 spiking
cells a button, each wiring the inputs its counters favour, at one strength) is the gap,
as the MNIST job found; better features do not reach it. One guess, not tested: the motor
cells sit on the odd lattice and hear their 9 tiles' cells through born, unlearned wiring
chosen by error counters; the features are there, but the readout's per-cell vote of ~35
binary inputs cannot weigh them.

## Cost

On this PC (shared, load 6 from other agents, nice 19, one job; rough), MNIST as set,
learn mode 5, life ticks per second:

| | ticks/s |
|---|---|
| nothing learning | 56,400 |
| tile winners 4, nothing learning | 51,400 |
| B: winners + compete | 14,300 (3.6 times slower) |
| B with `compete_shift 5` (a quarter of the updates) | 26,500 (1.9 times) |
| B with `threshold_cap 1000` (rate at target) | 25,600 (2.0 times) |
| co-activity wiring (learn mode 0, no winners) | 35,600 (from the life lines) |

`scripts/bench.sh`'s sheet (default genome, random input, 8x8 tiles): learn mode 5 68,400;
with compete 12,800; `compete_shift 5` 39,000; `compete_shift 6` 50,700. In the online life
at target rate, rule on against off: 50,000 against 53,700.

Where it goes: each update re-picks the cell's wiring from its 704 counters (two
histogram passes and a select per kind), about 4 microseconds; B makes about 12 updates a
tick (1,548 cells at 6.4 % firing, 1 in 8). Cheaper read-offs, untried: re-pick only when a
counter crosses the weakest wired one; or a swap form with no counters (gain one active
source, drop one silent wired one).

## Reading

- The answer to the project's one question, for this test, is yes. A local rule with no
  reward and no backprop makes the hidden cells of a fixed-shape sheet into stroke
  detectors that a linear reader uses better than a random projection of the same bits.
  It is classic competitive learning (Rumelhart and Zipser 1985) put in bits: counters
  for evidence, a fixed budget of inputs, the tile's winners for the competition, the
  existing threshold regulation as the handicap.
- The old co-activity wiring was already half of it. The budget and counters (finding 38's
  "keep its counts", now for a reward-free rule) are what doubled the gain.
- What it does not do: transfer to the readout. Learning inside and the output learning
  from good and bad do not meet yet.
- The firing rate rises past the target and the cap holds it there; at target the gain is
  smaller. Which rate a reader wants is a question for the reader, not the rule.
- It has been measured on still images only. Whether it helps time tasks, or harms them by
  pulling cells away from hidden sources (half of a cell's inputs become pixels), is not
  measured.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, "Where it stands": a reward-free local rule passes the
  inside-learning test on MNIST: competitive learning (tile winners + hidden counters +
  born-count budget), 60,000 digits with no reward, frozen: 0.952 (seeds 5-12; born 0.926,
  shuffled-digit control 0.940; random projection 0.943, two-layer net 0.955). It also
  lifts held-out xor4 and add4 on the calc battery. It does not help the online readout
  (0.72 either way). Caveat to carry with it: the learned cells fire twice their target;
  held at target, lived still beats born (+0.013) but beats the shuffled control by only
  +0.002 with tile winners (+0.012 without); 2 seeds.
- `docs/findings.md`: the old co-activity wiring also passes (0.937, born 0.924, shuffled
  0.926; 2 seeds). The MNIST doc's "a hair better" (10,000 digits, validation) undersold
  it. The line "neither helps MNIST" under "inside learns, a second way" is about the
  reward-driven rules only.
- New principle (likely): **a reward-free rule teaches the inside when it keeps the counts.**
  The budget plus counters doubles what co-activity wiring buys (+0.026 against +0.013)
  with the same competition. Second result for principle 38.
- New principle (likely): **every inside-learning claim needs a shuffled-input control.**
  A life on shuffled digits lifts the reader 0.014 by itself under tile winners (cells
  become a random projection of the input); only the rest is learned structure.
- Principle 8 / the MNIST doc: "the sheet adds less than a random projection" holds for the
  sheet as born, not after a life.
- `docs/predictions.md`: the four predictions above and how they scored.
- `docs/plan.md`: items 1 and 2 done. Next for this line: why the readout cannot use the
  features (a readout that hears the hidden cells more widely, or graded); the cheap
  read-off; the rule in the time suite and on the scoreboard (does pulling cells toward
  the input cost delayed echo and order?).
- `docs/design.md` and `genome.hpp` (core owner): `compete_on`, `compete_shift`,
  `compete_down_shift`, `compete_inh`, learn_compete.cpp; it shares the tag store with
  `tag_mode` (both on: one counter store, both move it).
- `docs/notes-world.md` / MNIST doc: `blobmnist ceiling --life N --life-shuffle 1`,
  `blobprobe calc --life N`, `scripts/inside.sh`.

## Script lines

```
scripts/inside.sh cowire                                          the old rule (co-activity)
scripts/inside.sh cowire-tw4 --g.tile_winners 4
scripts/inside.sh compete-a --g.learn_mode 5 --g.compete_on 1
scripts/inside.sh compete-b --g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4      best
scripts/inside.sh compete-c --g.learn_mode 5 --g.compete_on 2 --g.tile_winners 4
scripts/inside.sh compete-d --g.learn_mode 5 --g.compete_on 1 --g.threshold_cap 1000
scripts/inside.sh compete-e --g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4 --g.threshold_cap 1000
scripts/inside.sh compete-b5 --g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5 --only mnist
scripts/inside.sh b-report --g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4 --only mnist --seeds 5-12
scripts/inside.sh born-hot --g.sector_target_per_1024 64 --g.threshold_down_shift 8 --kinds born --only mnist
scripts/mnist.sh online --seed 5 --checks 1000,5000,20000,60000,120000 --g.tile_winners 4 --g.compete_on 1
scripts/mnist.sh online --seed 5 --checks 1000,5000,20000,60000,120000 --g.tile_winners 4
scripts/bench.sh --g.learn_mode 5 --g.compete_on 1
```

Each `inside.sh` line is about 15 minutes with 3 jobs here (MNIST 6 runs, calc 12).

## Rough

- Two seeds for every setting but B; the setting was picked on test seeds 1 and 2 (there
  is no separate validation pass), then reported on 5-12.
- Speeds are from a shared machine; the ratios are more trustworthy than the numbers.
- The calc rows have no shuffled control and two seeds.
- The losers' test (C) reads "over threshold" after the tick, ignoring the extras of ideas
  66-75; close enough for a rule that did not help.
- The four merge-reference lives: REFCHECK.
