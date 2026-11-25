# Competitive learning, followed up: rate, churn, products, cost, time (2026-11-25)

Queue item 2a of `docs/plan.md`, following `docs/results-2026-11-25-compete.md`. The claims
tested are findings 43, 44 and 47.

## Predictions (written before any scoring run)

Seen before writing this: speed runs only (no reader was fitted on a full life). On a
6,000-digit MNIST life, seed 5: the rule off with tile winners runs 53,700 ticks a second at
33 spikes per 1,000 cell-ticks; the born sheet pushed to 63 by a higher target (no rule)
43,700; the rule (B5) 26,700 at 63; the rule at cap 1000 41,100 at 33. So the higher rate
alone costs about 1.23 times. And 81 % of the rule's updates cross a bound (78 % change the
wiring), so skipping re-picks that change nothing cannot save much on its own.

1. MNIST, 8 seeds, rate held at target (cap 1000) in life and read, tile winners on: lived
   minus shuffled is 0.005 or more: probability 0.3. Without tile winners: 0.65.
2. MNIST, the churn control (same moves toward random sources): lived minus churn is 0.005 or
   more, as the sheet fires: probability 0.85; at matched rate: 0.75.
3. Calc, 8 seeds: held-out xor4 survives its shuffled control (lived minus shuffled at least
   0.02 on the mean and above on at least 6 of 8 seeds): probability 0.55. Held-out add4 the
   same: 0.4.
4. The cheap read-off gets the cost under 1.2 times (MNIST life ticks per second, rule on
   against off, both with tile winners): at the rule's own rate: 0.1 (the rate alone is
   1.23); with both held at target rate (cap 1000): 0.6.
5. Competitive learning plus the one-hop rule is worse than the one-hop rule alone on
   held-out xor4 again on 4 seeds (by 0.03 or more): 0.75. The tile winners, not the
   competition for counters, are most of that: 0.4.
6. Time rows (delayed echo, order, gap; whole blob learns): rule on against off moves none
   of them by 0.03 or more: 0.6.

## Short version

- **The claim, rate-matched, is small.** MNIST, seeds 5-12, every sheet held at its target
  rate in the life and the read: lived minus a life on shuffled digits is **+0.0029** with
  the tile's winners (5 of 8 seeds above) and **+0.0021** without (6 of 8). Under 0.005
  both. As the sheet fires it is +0.0134 (8 of 8), but there the learned cells fire at 64
  and the shuffled ones at 43. The stroke structure is there either way (pixel likeness 0.25
  against 0.05); a linear reader gets little from it beyond "cells that listen to pixels".
- **Churn is not the gain.** The churn control (as many counter moves and re-picks, toward
  sources drawn at random) reads at or under born in every setting (0.920 to 0.922 against
  0.921 to 0.926). Lived minus churn: +0.030 as it fires, +0.015 at matched rate, 8 of 8
  seeds. So the lift over born is all about the input (moving toward what is on), but only
  a sliver of it is about which pixels go together. Finding 47 holds for rewiring toward
  active sources, not for rewiring as such.
- **Calc:** held-out xor4 +0.11 to +0.15 and add4 +0.06 to +0.09 over born and over churn,
  8 of 8 seeds, at either rate. The shuffled control reads the same as lived, because in
  xor4 and add4 every input line is an independent coin flip: shuffling them changes
  nothing. The gain is generic (cells become detectors of lines on together), not fake.
- **Beside the one-hop rule, competitive learning takes the counters:** held-out xor4 0.794
  both against 0.878 the one-hop rule alone (4 seeds, every seed lower). Not the rate (both
  read at 36), not mainly the tile's winners (0.836 with them, no competition); a quarter of
  the competitive moves brings it back to 0.836.
- **Cheap read-off, same wiring:** full 60,000-digit B5 lives of seeds 5-8 byte-identical to
  the compete job's old runs; MNIST life at target rate 1.32 -> **1.17** times the rule-off
  tick; at the rule's own rate 1.93 -> 1.64, of which the doubled spike rate alone is 1.2.
- **The time rows: a big lift, not from the rate.** Rule on for the whole life beside the
  readout, seeds 5-12: delayed echo 0.59 -> **0.91**, gap 0.54 -> **0.94**, order 0.66 ->
  0.73; with the rate held at target 0.58 -> 0.87, 0.53 -> 0.93, 0.71 -> 0.81. The churn
  control is under rule-off on all three. Half of each cell's inputs becoming input lines
  does not cost time; the rule makes time features.

## What was built

- **The churn control** (`--g.compete_churn 1`, `learn_compete.cpp`): a winner moves as many
  counters up as it had sources on, but the sources are drawn at random (none twice) among
  the excitatory sources it may wire, over all its slots; the silent-source steps down and
  the re-pick are as before. Same number of moves and re-picks, nothing about the input in
  which. Winners are still the cells that fired, so which cells move still follows the input.
- **A shuffled control for calc** (`blobprobe calc --life N --life-shuffle 1`,
  `src/probe/calc_shuffle.cpp`): each trial of the life shows its input lines moved by a
  permutation drawn fresh for that trial (the ask line stays). As many lines on, at the same
  steps; which lines are on together carries nothing that comes back. The read is on real
  cases. `blobprobe calc` now also prints and keeps (`rate,hidden_per_1000`) the hidden
  cells' rate while read.
- **The cheap read-off** (`learn_compete.cpp`, `tag_store.cpp`), in three parts:
  1. Bounds, as the one-hop rule keeps them: per cell the lowest wired, highest unwired,
     lowest strong and highest weak counter (excitatory sources; the rule never moves the
     inhibitory ones). A step that crosses none cannot change the pick, so no re-pick. A
     re-pick that changes nothing still draws chance for its ties (one draw per pick that
     wants any sources), so the cheap path draws the same number: lives stay
     byte-identical. A hash of the cell's wiring makes the bounds stale if any other rule
     rewired it; `tag_rewire` marks them stale too; with the one-hop rule on or
     `tag_counts` set, every update re-picks as before.
  2. The re-pick itself (`select_top`, used by every rule on the tag store): the line (the
     value the top counters reach down to) is found by stepping from the last re-pick's
     line, counting with one 64-byte compare per slot, instead of a 256-bucket histogram
     filled one counter at a time. Same picks, same draws (`tag_rewire_hinted`; plain
     `tag_rewire` halves the range instead).
  3. The bounds after a re-pick come from where its lines fell (no pass over the counters).
  `--g.compete_full 1` re-picks on every update (for comparing speed); `--g.compete_full 2`
  does that and stops the program if the bounds ever said "no change" when the re-pick
  changed the wiring.
- `scripts/compete2.sh mnist|calc|hopcomp|time|speed|all [--quick]` and `inside.py` kinds
  `churn` (MNIST and calc) and `shuffled` (calc).

## 1. The rate-matched control (MNIST)

`scripts/compete2.sh mnist` (tables in `out/compete2/mnist.txt`). Test digits, a fresh linear
reader of the non-sensor cells fit on 60,000 training digits after a 60,000-digit life with
no reward; seeds 5-12. "cap": `--g.threshold_cap 1000` in the life and the read, so every
sheet fires at its target (31 spikes per 1,000 cell-ticks). "read at": the hidden cells'
rate while the reader's digits are shown. B5 is `--g.learn_mode 5 --g.compete_on 1
--g.tile_winners 4 --g.compete_shift 5`; A5 the same without `--g.tile_winners 4`.

| setting | born | lived | shuffled | churn | read at (born / lived / shuffled / churn) |
|---|---|---|---|---|---|
| B5, as it fires | 0.9264 | **0.9523** | 0.9389 | 0.9222 | 33 / 64 / 43 / 32 |
| B5, cap | 0.9217 | **0.9351** | 0.9322 | 0.9201 | 31 / 31 / 31 / 31 |
| A5, as it fires | 0.9251 | **0.9447** | 0.9398 | 0.9224 | 33 / 75 / 47 / 32 |
| A5, cap | 0.9214 | **0.9335** | 0.9314 | 0.9195 | 31 / 31 / 31 / 31 |

Random projection of the same bits (`blobmnist baselines --seed S`, seeds 5-12): 0.9418
(0.9396 to 0.9432). It has no firing rate to hold: each unit is on for half the digits.

**The claim, lived minus shuffled, per seed:**

| | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | seeds above |
|---|---|---|---|---|---|---|---|---|---|---|
| B5, as it fires | +0.0107 | +0.0135 | +0.0157 | +0.0173 | +0.0146 | +0.0134 | +0.0103 | +0.0114 | +0.0134 | 8 of 8 |
| **B5, cap** | +0.0047 | -0.0010 | +0.0060 | -0.0014 | +0.0051 | +0.0076 | +0.0027 | -0.0009 | **+0.0029** | 5 of 8 |
| A5, as it fires | +0.0081 | +0.0048 | +0.0068 | +0.0026 | +0.0045 | +0.0045 | +0.0049 | +0.0033 | +0.0049 | 8 of 8 |
| **A5, cap** | +0.0013 | +0.0005 | +0.0072 | +0.0029 | -0.0010 | +0.0040 | +0.0025 | -0.0007 | **+0.0021** | 6 of 8 |

**That number is the claim, and it is small: at matched rate the lived sheet beats a life on
shuffled digits by 0.003 with tile winners (5 of 8 seeds) and 0.002 without (6 of 8).** Both
are under the 0.005 that would count. The 2-seed numbers of the compete doc were +0.002 and
+0.012; the second does not hold on 8 seeds (and at shift 5, not 3).

What this says, plainly:

- The stroke structure is real and the same either way: pixel likeness 0.25 lived against
  0.05 shuffled, at both rates (wiring lines of each run; below). But a fresh linear reader
  gets almost all of its lift from cells that listen to pixels at all, related or not: the
  shuffled life turns 27 of a cell's 34 inputs into pixels and lifts the reader from 0.922 to
  0.932 at target rate; the related pixels add 0.003 more.
- The 0.952 headline is mostly two things a random-pixel life also gives, plus rate. As the
  sheet fires, B5 reads 0.013 over shuffled, but its cells fire 64 against 43. Held at the
  same rate, 0.003. The rate is not the gain by itself (born pushed to 63 reads 0.910,
  compete doc), but the learned cells use the higher rate better than random-pixel cells do,
  and the reader sees that.
- Neither lived sheet beats the random projection at target rate (0.935 and 0.934 against
  0.942).

Wiring at the end of the life (means of seeds 5-12; pixel inputs of 34, pixel likeness,
share of born inputs still wired):

| | lived | shuffled | churn |
|---|---|---|---|
| B5 | 17.6, 0.252, 0.23 | 26.5, 0.047, 0.24 | 6.9, 0.047, 0.23 |
| B5, cap | 21.5, 0.245, 0.23 | 27.5, 0.047, 0.25 | 6.9, 0.047, 0.23 |
| A5 | 15.3, 0.251, 0.23 | 26.4, 0.048, 0.24 | 7.0, 0.047, 0.23 |
| A5, cap | 21.6, 0.250, 0.23 | 27.5, 0.048, 0.24 | 7.0, 0.046, 0.23 |

(Born: 7.2 pixel inputs, likeness 0.052.)

## 2. The churn control

The same table's churn column, and per seed (`out/compete2/mnist.txt`):

| lived minus churn | mean | seeds above | churn minus born |
|---|---|---|---|
| B5, as it fires | +0.0301 | 8 of 8 | -0.0042 |
| B5, cap | +0.0150 | 8 of 8 | -0.0016 |
| A5, as it fires | +0.0223 | 8 of 8 | -0.0027 |
| A5, cap | +0.0140 | 8 of 8 | -0.0019 |

- **Churn alone buys nothing; it costs a little.** The same number of moves and re-picks,
  three quarters of the born inputs replaced, toward sources drawn at random: the reader
  falls 0.002 to 0.004 under born, every setting. Finding 47 ("rewiring busy cells at their
  born counts lifts a linear reader whatever drives it") does not hold for rewiring that is
  truly random. In the three rules behind finding 47 the moves still went toward sources
  that had just been on (marks, coin errors on the same marks); what they share is "move
  toward what is active", not "move".
- So **all of the lift over born is about the input, but only a sliver of it is about the
  digits' shapes.** Lived minus churn at matched rate is 0.014 to 0.015; lived minus shuffled
  is 0.002 to 0.003. Moving toward pixels that are on is the active part; which pixels go
  together matters little to a linear reader.

## 3. Calculation, held-out cases

`scripts/compete2.sh calc` (`out/compete2/calc.txt`): `blobprobe calc`'s ridge reader of the
frozen sheet after a 200,000-tick life with no verdicts; bits right on held-out cases (never
answered; shown in the life). Seeds 5-12. Means:

| member, setting | born | lived | shuffled | churn | lived - shuffled (seeds above) | lived - churn (seeds above) | read at (lived / shuffled) |
|---|---|---|---|---|---|---|---|
| xor4, B5 | 0.667 | 0.779 | 0.786 | 0.649 | -0.008 (3 of 8) | +0.130 (8 of 8) | 38 / 38 |
| xor4, B5 cap | 0.644 | 0.765 | 0.768 | 0.645 | -0.003 (3 of 8) | +0.120 (8 of 8) | 32 / 32 |
| xor4, A5 | 0.630 | 0.743 | 0.745 | 0.615 | -0.002 (3 of 8) | +0.128 (8 of 8) | 62 / 62 |
| xor4, A5 cap | 0.613 | 0.767 | 0.776 | 0.636 | -0.009 (3 of 8) | +0.130 (8 of 8) | 31 / 31 |
| add4, B5 | 0.577 | 0.646 | 0.640 | 0.572 | +0.006 (5 of 8) | +0.074 (8 of 8) | 38 / 38 |
| add4, B5 cap | 0.565 | 0.630 | 0.645 | 0.584 | -0.015 (2 of 8) | +0.043 (8 of 8) | 32 / 32 |
| add4, A5 | 0.562 | 0.647 | 0.641 | 0.567 | +0.006 (4 of 8) | +0.080 (8 of 8) | 62 / 62 |
| add4, A5 cap | 0.563 | 0.621 | 0.618 | 0.584 | +0.004 (5 of 8) | +0.037 (8 of 8) | 31 / 31 |

- **The held-out gain is real and large against born and churn** (xor4 +0.11 to +0.15, add4
  +0.06 to +0.09; churn is at born), on every seed, at either rate. The rule makes the
  inside carry what a linear reader needs for cases it never saw answered.
- **It does not survive its shuffled control, and on these members it could not:** a life
  on lines moved by a fresh permutation each trial reads the same (xor4 -0.002 to -0.009,
  add4 -0.015 to +0.006). xor4 and add4 show two random numbers: every input line is an
  independent coin flip, so permuting the lines does not change what the life sees at all.
  There is no structure in these inputs beyond "which lines are on", and the rule turns
  cells into detectors of lines that are on together (AND of a few lines), which is what a
  linear reader lacks for exclusive-or. The control says the gain is generic, not that it
  is fake: the lines being on is the information, and churn (moves toward lines drawn at
  random) gets none of it.
- The rate does not explain calc: at target rate the gain is the same size.

## Competitive learning beside the one-hop rule (calc, taught life)

The hop doc saw competitive learning plus the one-hop rule read worse on held-out xor4 than
the one-hop rule alone (0.778 against 0.871, 2 seeds). `scripts/compete2.sh hopcomp` splits
it: the hop doc's taught life (400,000 ticks, a verdict per bit, its readout learning,
motor cells put back as born), seeds 5-8, run on stevejobs (`out/compete2/hopcomp.txt`).
Held-out bits right, mean of 4 seeds (per seed in the file), and the read rate:

| life | xor4 | add4 | read at |
|---|---|---|---|
| born / born with tile winners | 0.627 / 0.661 | 0.569 / 0.573 | 33 / 32 |
| readout alone / with tile winners | 0.633 / 0.658 | 0.544 / 0.567 | 32 / 32 |
| competitive learning, tile winners (hop doc's "competitive alone") | 0.782 | 0.634 | 37 |
| competitive learning, no tile winners | 0.754 | 0.646 | 59 |
| **one-hop rule** (hop doc 0.871 on seeds 5, 6) | **0.878** | 0.654 | 33 |
| one-hop rule, tile winners | 0.836 | **0.679** | 32 |
| one-hop + competitive, no tile winners | 0.775 | 0.647 | 52 to 60 |
| **one-hop + competitive, tile winners** (hop doc's "both", 0.778) | **0.794** | 0.654 | 36 |
| one-hop + competitive at a quarter of its moves (`compete_shift 7`), tile winners | 0.836 | 0.663 | 35 |

- **Reproduced:** both together 0.794 against the one-hop rule alone 0.878 on xor4, every
  seed lower (0.783, 0.773, 0.818, 0.803 against 0.889, 0.854, 0.893, 0.876).
- **Why: competitive learning takes the counters.** Without the tile's winners the pair is
  just as bad (0.775), so the winners are not the cause; they cost the one-hop rule 0.04 by
  themselves (0.878 to 0.836). The rate is not the cause either: with winners the pair reads
  at 36, near target, and is still 0.794. Slowing the competitive rule to a quarter of its
  moves brings the pair back to the one-hop rule with winners (0.836 = 0.836). Both rules
  move the same counters in the tag store and re-pick from them; competitive learning moves
  them on every winning spike (about 6 updates a tick), the one-hop rule only at a verdict
  (once a trial), so the reward-free rule sets the wiring and the teaching steps are
  outvoted. On add4 the one-hop rule gains less and the pair costs nothing.
- If the two are to share a cell, they should not share its counters, or the competitive
  rule must move much more slowly than every winning spike.

## 4. The cheap read-off

**Same wiring, proven by lives.** With the new read-off (the default, `compete_full 0`):

- The full B5 MNIST lives of seeds 5, 6, 7 and 8 (born, lived, shuffled: 60,000-digit life,
  60,000-digit fit) give output byte-identical, every wiring and activity line and the test
  score, to the compete job's runs of the same lines with the old read-off
  (`out/from-agents/compete/inside/b5-report/`, run before this job's code existed). Seeds
  9-12 of B5 in `out/compete2/mnist/b5/` are those old runs, copied.
- `out/compete2/proof/quick.sh` (a 3,000- and a 6,000-digit B5 life, and A5 at cap 1000) and
  `out/compete2/proof/calc.sh` (calc xor4, seed 5, B5 and B5 at cap 1000): byte-identical to a
  build of 6bc8130 (the commit before this job), and to `--g.compete_full 2`, which re-picks
  on every update and aborts if the bounds ever said "no change" when the re-pick changed
  anything (it never aborted).
- The four reference lives: byte-identical to `out/merge-ref/` (`out/compete2/refcheck.sh`).

**Speed** (`COMPETE2_BEFORE=build-old scripts/compete2.sh speed`, `out/compete2/speed.txt`;
this PC, one job at a time, load about 7 from other agents; rough). "before" is a build of
6bc8130:

| | before | now | rule off | cost before | cost now |
|---|---|---|---|---|---|
| `blobbench speed --tiles 8 --learn 1`, rule shift 5 | 42,279 | 52,698 | 69,326 | 1.64 | 1.32 |
| the same, shift 3 | 19,579 | 36,156 | 69,326 | 3.54 | 1.92 |
| MNIST life, B5 (fires at 63) | 27,330 | 32,295 | 52,843 (33) | 1.93 | 1.64 |
| MNIST life, B5 at cap 1000 (fires at 33) | 40,438 | 45,449 | 53,214 | 1.32 | **1.17** |

- At target rate the rule now costs 1.17 times per tick (it was 1.32). At its own rate it
  costs 1.64, and most of that is the rate, not the rule: the born sheet pushed to the same
  63 with the rule off runs 44,091, so the rule's own work there is 1.37 times. The read-off
  cannot reach under 1.2 while the rule doubles the spikes the tick must carry.
- Where it went: re-picking on every update with the new select (`compete_full 1`) runs
  29,784, so on MNIST the faster re-pick gives +9 % and the bounds +8 %. 81 % of updates cross a
  bound (an unwired counter climbs past the lowest wired one at the top of the 6-bit range,
  or a wired one is stepped down), so most updates really do change the wiring; the rule
  churns among sources that are on about equally often. What was slow was the re-pick
  itself: a 256-bucket histogram filled one counter at a time, where most counters sit at
  the same value (a chain of increments on one bucket). Now the line is found by stepping
  from the last one, each step one 64-byte compare per slot.
- `select_top` is shared: the tags and the one-hop rule re-pick faster too, with the same
  picks (they call plain `tag_rewire`, which halves the range instead of stepping).

## 5. The time rows

`scripts/compete2.sh time` (`out/compete2/time.txt`): the suite's delayed echo (2 steps),
order and gap rows, whole blob learns, 40,000 steps, seeds 5-12, through `scripts/suite.py`
(cards `out/scorecards/compete2-time-*.csv`, runs in `out/suite/compete2-time-*/`). The
readout is the scoreboard's blob-readout column (`--g.learn_mode 5 --g.readout_evidence 1
--g.readout_shift 4 --g.readout_balance 1 --motor-spread 128`) with `--g.tile_winners 4` in
every card; "on" adds `--g.compete_on 1 --g.compete_shift 5` for the whole life, "cap" adds
`--g.threshold_cap 1000`. Late score (mean of the last tenth), per seed:

| row | card | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate |
|---|---|---|---|---|---|---|---|---|---|---|---|
| delayed echo | off | 0.63 | 0.60 | 0.55 | 0.55 | 0.61 | 0.56 | 0.60 | 0.59 | 0.587 | 31 |
| | **on** | 0.98 | 0.95 | 0.98 | 0.93 | 0.88 | 0.84 | 0.82 | 0.92 | **0.913** | 34 |
| | off, cap | 0.60 | 0.56 | 0.57 | 0.56 | 0.57 | 0.56 | 0.61 | 0.58 | 0.576 | 31 |
| | **on, cap** | 0.99 | 0.76 | 0.87 | 0.97 | 0.83 | 0.84 | 0.81 | 0.93 | **0.874** | 31 |
| order | off | 0.61 | 0.70 | 0.70 | 0.63 | 0.69 | 0.61 | 0.71 | 0.66 | 0.664 | 32 |
| | **on** | 0.72 | 0.63 | 0.68 | 0.71 | 0.76 | 0.78 | 0.67 | 0.87 | **0.728** | 40 |
| | off, cap | 0.66 | 0.73 | 0.71 | 0.69 | 0.73 | 0.64 | 0.77 | 0.72 | 0.707 | 32 |
| | **on, cap** | 0.94 | 0.86 | 0.87 | 0.81 | 0.76 | 0.76 | 0.78 | 0.68 | **0.809** | 32 |
| gap | off | 0.56 | 0.57 | 0.53 | 0.48 | 0.54 | 0.56 | 0.59 | 0.52 | 0.544 | 31 |
| | **on** | 0.92 | 0.85 | 0.96 | 0.99 | 0.91 | 0.97 | 0.96 | 0.95 | **0.938** | 39 |
| | off, cap | 0.54 | 0.48 | 0.51 | 0.51 | 0.51 | 0.56 | 0.59 | 0.50 | 0.525 | 31 |
| | **on, cap** | 0.89 | 0.95 | 0.94 | 0.93 | 0.97 | 0.87 | 0.95 | 0.94 | **0.929** | 31 |

(Learning-off twins 0.48 to 0.49, coin flips 0.47 to 0.48. "rate": the life's spikes per
1,024 neuron-ticks, the suite's own measure.)

The churn control (`--g.compete_churn 1` with the rule on, as it fires): delayed echo 0.52,
0.57, 0.55, 0.54, 0.52, 0.53, 0.56, 0.51 (mean 0.537), order 0.606, gap 0.503, at rate 31 to
32: under the rule-off card on all three. The moves have to go toward what was on.

- **It helps, a lot, and not through the rate.** Delayed echo +0.33 (every seed), gap +0.39
  (every seed), order +0.06 (5 of 8); held at target rate +0.30, +0.40 and +0.10 (7 of 8).
  The scoreboard for comparison: blob-readout 0.55 / 0.69 / 0.52, the richest sheet
  (blob-rich2) 0.89 / 0.86 / 0.89, the GRU 1.00 on all three. With the rule, the plain
  sheet reaches blob-rich2 on delayed echo and gap. On these worlds the rule does not leave
  the cells firing much faster (34 to 40 against 31) and the cap changes little.
- Why, a reading, not tested: a winner's counters gain for sources that were on *at the
  delay of the slot that reads them*, so a cell comes to wire the sources that, at their
  own delays, line up on its spike: a cue now and the same cue some ticks ago, or a cue
  and a hidden cell still carrying an older one. That is a detector of a time pattern,
  learned with no reward, which a readout of 2 buttons can use. Delayed echo and gap are
  both "what was on so many ticks ago".
- This is the first reward-free inner rule that moves a time row of the suite. It was
  measured here only on these three rows (as asked); the rest of the suite with the rule
  on is not run.

## What is real, what is suspect

Real (8 seeds, every number from a script line below):

- At matched rate, what competitive learning learns about the *shapes* of digits is worth
  +0.002 to +0.003 to a fresh linear reader (5 and 6 of 8 seeds). What it learns about
  *which inputs are on* is worth +0.014 to +0.015 over churn. The headline 0.952 is mostly
  "cells listen to pixels" plus a higher rate that the learned cells use better.
- Random rewiring at born counts does not lift the reader (churn at or under born, every
  setting). Finding 47 should say "rewiring toward active sources".
- On calc the rule lifts held-out xor4 by 0.11 to 0.15 and add4 by 0.06 to 0.09 over born and
  churn; the shuffled control matches it because these inputs have no structure to shuffle.
- Competitive learning sharing counters with the one-hop rule outvotes it (0.794 against
  0.878 on xor4; a quarter of the moves gives 0.836).
- The rule lifts delayed echo and gap by 0.3 to 0.4 at either rate.
- The read-off gives the same wiring and the same lives, faster.

Suspect:

- The time rows are 3 rows of one readout, 40,000 steps, and the reading of why is a guess.
  Its controls are the cap and churn; there is no shuffled-input control for them yet.
- Calc's shuffled control is uninformative on these two members (above). A calc member
  with structured inputs (lines that tend to be on together) would test what this cannot.
- The hop decomposition is 4 seeds and one life length. Whether the outvoting is the
  number of moves or the order of moves (competition every tick, errors at verdicts) is
  not separated beyond `compete_shift 7`.
- Speeds are from a shared machine (load about 7). The ratios held within a few percent
  across repeats; the absolute numbers did not.
- The time suite's "off" card ran on stevejobs, the others here; results are
  byte-identical between machines (AGENTS.md). Two "on" runs started there were killed
  mid-write, left empty summaries, and were rerun here.

## Not done

- The rest of the suite (and the scoreboard) with the rule on. Given the time rows, this is
  the next thing to run: `scripts/suite.sh compete-on --g.learn_mode 5
  --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128
  --g.tile_winners 4 --g.compete_on 1 --g.compete_shift 5`.
- A shuffled control for the time rows (cue lines moved per trial), and the time rows on a
  frozen sheet after a reward-free life (does the inside carry the time feature, or does it
  need the rule on beside the readout?).
- A real-data test that needs products (idea 163's branch builds it).
- The read-off could drop further only by firing less: at the rule's own rate the tick
  itself (more spikes) is most of the remaining cost.
- Nothing was tuned; the 4 settings are the compete doc's B5 and A5 with and without cap.

## Predictions, scored

1. B5 at cap, lived minus shuffled 0.005 or more (0.3): no, +0.0029. A5 at cap (0.65): no,
   +0.0021. Wrong on A5: the 2-seed +0.012 did not hold.
2. Lived minus churn 0.005 or more, as it fires (0.85): yes, +0.030. At cap (0.75): yes,
   +0.015.
3. Held-out xor4 survives its shuffled control (0.55): no (-0.008); add4 (0.4): no
   (+0.006, 5 of 8). The control turned out to be no test on these members.
4. Under 1.2 times at the rule's own rate (0.1): no, 1.64. At target rate (0.6): yes, 1.17.
5. Both rules worse than the one-hop rule alone by 0.03 or more on xor4 (0.75): yes, -0.084.
   The tile's winners most of it (0.4): no; the competition for the counters is.
6. No time row moves by 0.03 or more (0.6): wrong. Delayed echo +0.33, gap +0.39, order
   +0.06.

## Script lines

```
scripts/compete2.sh mnist                  section 1 and 2 (128 runs; B5 seeds 9-12 born/lived/shuffled copied from the compete job)
scripts/compete2.sh calc                   section 3 (256 runs)
scripts/compete2.sh hopcomp                the one-hop rule beside competitive learning (88 runs, on stevejobs)
scripts/compete2.sh time                   section 5 (5 suite cards, 3 rows each)
COMPETE2_BEFORE=build-old scripts/compete2.sh speed     section 4 (build-old: cmake build of 6bc8130)
scripts/compete2.sh all --quick            smoke check, 2 seeds, short lives (ran end to end)
scripts/compete2.sh mnist --table-only     print a table from what is there
bash out/compete2/proof/quick.sh 6000      byte-identity, MNIST (needs build-old)
bash out/compete2/proof/calc.sh            byte-identity, calc xor4 (needs build-old)
bash out/compete2/refcheck.sh              the four reference lives against out/merge-ref
```

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md` 43: the 0.952 holds, but at matched rate lived beats a shuffled-digit
  life by only +0.003 (tile winners, 5 of 8 seeds) and +0.002 (without, 6 of 8); lived beats
  random churn by +0.015 at matched rate (8 of 8). "Teaches the inside" is true of moving
  toward active inputs; what it learns about digit shapes is worth little to a linear
  reader.
- `docs/findings.md` 44: confirmed, and the rate-matched control is decisive on MNIST. Add
  churn (moves toward random sources) as the third control for a rewiring rule.
- `docs/findings.md` 47: rewiring at born counts toward sources drawn at random lowers the
  reader (churn 0.922 against born 0.926). The common part of the three rules is moving
  toward sources that were just active.
- New finding (likely, 8 seeds, 3 rows, one readout): **reward-free competitive learning
  makes time features.** Delayed echo 0.59 -> 0.91, gap 0.54 -> 0.94 with the rule on beside
  the readout, at either rate; churn does not. First inner rule that moves the time rows.
- New finding (hint, 4 seeds): two rules on one counter store: the one that moves every
  tick outvotes the one that moves at verdicts.
- `docs/plan.md`: item 2a done. Next: the whole suite with the rule on (script line under
  "Not done"); a time-row shuffled control and a frozen read after a reward-free life on a
  time world; one-hop and competitive learning on separate counters.
- `docs/predictions.md`: the six above.
- `docs/design.md`: compete_churn, compete_full, and the read-off (bounds + stepped line in
  `select_top`, `tag_rewire_hinted`); compete's numbers were not in design.md before, so
  nothing was added there.

## Files that may collide with other branches

- `src/blob/tag_store.cpp` (`select_top`, `tag_rewire_hinted`; one line marking compete
  stale) and `tag_store.hpp`: the one-hop rule calls `tag_rewire` and is otherwise
  untouched; `learn_hop.cpp` not edited.
- `src/blob/blob.hpp` (one pointer), `genome.hpp`, `genome_args.cpp` (two numbers each),
  `tick_steps.hpp`, `birth.cpp` (one free line).
- `src/probe/linear_calc.cpp` (life shuffle flag, read rate, a `rate` line in the csv), new
  `src/probe/calc_shuffle.cpp`. `probe/main.cpp`, `linear_fit.*`, `mnist_*`, `run_flags.cpp`,
  `life*.cpp`, `plug*.cpp`, `learn_readout.cpp` not touched.
- `scripts/inside.py` (kinds churn and calc shuffled), `scripts/inside.sh` (one comment).
