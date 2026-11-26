# Do the inner rules learn more from a steadier sheet? (2026-11-26)

Queue item 2j, idea 174, following `docs/results-2026-11-26-repeatable.md` (idea 168). There a
cell's count on a digit turned out to be about two fifths regulation and carry-over, with 0.66
spikes a digit; read with regulation frozen and the digit shown 80 ticks (the quiet read) the
born sheet goes 0.924 / 0.673 to 0.954 / 0.752. Competitive learning and the one-hop rule
decide what to wire from those same noisy spikes. This job gives the rules a steadier sheet
during the life and asks whether what they learn about digits grows: lived minus a life on
shuffled digits (and, for the one-hop rule, lived minus its coin), at matched firing rate, on
MNIST and the pair test, read the standard way and the quiet way.

## Predictions (written and committed before any scoring run was read)

Seen before writing these: the four docs this job follows, the code of `learn_compete.cpp`,
`regulate.cpp` and the MNIST harness, and two timing runs on 6,000 digits (no score of them
read as a result). Nothing below has been run at full size.

Every number is at matched rate (`--g.threshold_cap 1000` in the life and the read, as
compete2's "B5, cap"), means of seeds 5-8 unless said. MNIST / pairs. Known: competitive
learning, standard read, lived minus shuffled +0.0029 on MNIST (8 seeds); the pair test at cap
was never run.

| arm (life only; the read is the same for every arm) | lived - shuffled, standard read | quiet read |
|---|---|---|
| base: compete B5, 60,000 digits of 20 ticks | +0.003 / +0.010 | +0.005 / +0.015 |
| 1 shown 40 ticks, same digits (twice the ticks) | +0.003 / +0.010 | |
| 1 shown 40 ticks, same ticks (30,000 digits) | +0.002 / +0.008 | |
| 1 shown 80 ticks, same digits | +0.004 / +0.012 | +0.006 / +0.016 |
| 1 shown 80 ticks, same ticks (15,000 digits) | +0.002 / +0.007 | |
| 2 threshold regulation 8 times slower | +0.003 / +0.010 | |
| 2 thresholds frozen after 3,000 digits, sector bias to 32 | +0.001 / +0.005 | |
| 2 sector activity smoothed over 256 ticks | +0.003 / +0.010 | |
| 3 counted: the tile's 4 most active over a digit move, toward what was on in it | +0.004 / +0.012 | +0.006 / +0.018 |
| 4 compete_shift 6 / 7, 60,000 digits | +0.003 / +0.010 | |
| 4 compete_shift 6 / 7, 120,000 digits | +0.004 / +0.011 | |

Reasons. A competitive move is made per spike, toward the sources on at that spike. The pixel
lines are on for the whole digit, so the pixel part of what a winner moves toward is the same
whether the digit is shown 20 ticks or 80, and the counters already average over many spikes;
which cells win is the noisy part, and the counters average that too. So I expect the life's
noise to matter little to what is learned, unlike the read, where one digit's count is all
there is. Freezing thresholds takes away the handicap that keeps winners from taking
everything; I expect more silent cells and a worse lived sheet, not a better one.

- Lived minus shuffled grows by 0.003 or more on MNIST (standard read, 4 seeds) under any arm:
  0.25. By 0.01 or more on pairs under any arm: 0.3. The likeliest to do it: the counted rule
  (0.35 of the 0.25), then 80 ticks with the same digits.
- If an arm grows it, "same ticks" grows it less than "same digits", i.e. part of it is more
  experience, not steadier showing: 0.6.
- The quiet read shows a larger lived-minus-shuffled than the standard read on the base life:
  0.6 (MNIST), 0.65 (pairs).
- Thresholds frozen in the life with the bias to 32: the life's rate rises to 40-60 per 1,000
  cell-ticks and 5% or more of the hidden cells end silent on the read: 0.5. Slower threshold
  regulation and the slower bias keep the rate within 10% of target: 0.8.
- The counted rule, at matched moves (`compete_shift 2`, about one move a tile a digit, as B5
  at cap), reads within 0.005 of B5's lived sheet on MNIST: 0.6.
- The one-hop rule (errors from the answer, `hop_shift 9`, cap): lived minus coin is +0.003 /
  +0.008 on the base life; it grows by 0.003 or more on MNIST under any arm: 0.3 (likeliest:
  the slower rule over 120,000 digits). Longer showing does little for it, because its marks
  cover only the last 12 ticks of a digit: 0.7.
- Nothing here reaches the teacher's wiring (0.966 / 0.785 standard, 0.975 / 0.826 quiet):
  0.97.

Added after the predictions, from smoke runs of 6,000 digits read for their life lines only (no
score used): the counted rule as first written ("toward every source that fired at least once
in the digit") moves the cells off the pixels (2.3 pixel inputs of 34 against 21 for the spike
rule), because a hidden cell fires at least once in about half the digits and a pixel in a
cell's field is on in about a fifth; so "on" became a genome number, and at "on at least 5 of
the 20 ticks" the counted rule wires as many pixels as the spike rule (19.4 against 21.0). Both
are run (count5, count1). Thresholds frozen with the sector bias to 32 run away (85 spikes per
1,000 cell-ticks against 31); even an unbounded bias holds the hidden cells only at 45, because
a sector counts its motor cells too and without per-cell thresholds the spikes move to the
hidden cells. So the rate-held form of that arm is thresholds frozen, bias to 128 and the
sectors' targets at 60% for the life (thfreeze); the literal one (th32) is run as the failure
it is.

## Short version (written at tabling; the job was stopped before its last batch finished)

**On the standard sheet competitive learning's lived sheet reads 0.9351 / 0.6990 against
0.9322 / 0.6924 after a shuffled life (difference +0.0029 / +0.0066); with the counted rule
(count5: the tile's 4 most active cells over a digit move, once, toward the sources on at least
5 of its 20 ticks) it reads 0.9382 / 0.7046 against 0.9262 / 0.6775 (difference +0.0120 /
+0.0271), at firing rates 31.2 and 31.2 (pairs 31.1 and 31.1); the one-hop rule over its coin
goes +0.0042 / +0.0032 to +0.0065 / +0.0173 with a 120,000-digit life.** (MNIST / pair test,
standard read, seeds 5-12; the counted and long-life margins are above their base on 8 of 8
seeds, h9l on 7 of 8 on MNIST.)

- **Most of that growth is the control getting worse, not the lived sheet getting better.**
  The reader pays for "cells hear pixels at all" (compete2), and the counted rule's shuffled
  life wires fewer pixels (21.8 pixel inputs a cell against 27.5 for the spike rule). The
  lived sheet itself gains +0.003 / +0.006. Against a no-structure sheet with as many pixel
  inputs (a straight line of score on pixel inputs over every shuffled and churn sheet, per
  seed) the lived sheet is +0.0071 / +0.0171 (spike rule) and +0.0113 / +0.0239 (counted), 8 of
  8 seeds: a real but small gain of about +0.004 / +0.007.
- **Read the quiet way the lived sheets are nearly all alike.** MNIST: born 0.9535, base lived
  0.9546 against shuffled 0.9558 (-0.0012, 1 of 8), counted 0.9560 against 0.9522 (+0.0038, 8
  of 8), teacher 0.9702. What the rules learn about digit shapes is worth very little to a
  linear reader even with the reading noise taken out.
- **A steadier sheet in the life does little.** Longer showings, slower threshold
  regulation, a slower sector bias, frozen thresholds, slower rules and longer lives each move
  the MNIST margin by -0.001 to +0.004 (4 seeds). On pairs several arms show +0.01 to +0.02 over
  base, but pair margins are noisy at that size (compete_shift 6 at 60,000 digits +0.024, at
  120,000 +0.008). The one-hop rule gains from more digits at the same step (h9l), not from a
  slower step (h10, h11, h10l, h11l at or under base).
- So the noise is not the main reason the rules learn little; deciding on a whole digit's
  count is the one change that moves anything, and mostly by wiring fewer pixels when the
  input has no structure.

## The arms the conclusion rests on (seeds 5-12)

Matched rate (`--g.threshold_cap 1000` in life and read). ctrl1: shuffled (competitive
learning) or coin (one-hop rule); ctrl2: churn or shuffled. Paired differences (seeds above).
`out/steady/table-final-5-12.txt`.

| read, test | arm | lived | ctrl1 | lived - ctrl1 | lived - ctrl2 | px lived / ctrl1 |
|---|---|---|---|---|---|---|
| std, MNIST | born (tile winners) / teacher | 0.9217 / 0.9569 | | | | |
| | base | 0.9351 | 0.9322 | +0.0029 (5/8) | +0.0150 (8/8) | 21.5 / 27.5 |
| | count5 | 0.9382 | 0.9262 | +0.0120 (8/8) | +0.0204 (8/8) | 19.9 / 21.8 |
| | c80d (counted over 80-tick digits) | 0.9392 | 0.9313 | +0.0080 (8/8) | +0.0197 (8/8) | 20.1 / 27.0 |
| | hbase (one-hop) | 0.9348 | 0.9305 | +0.0042 (8/8) | +0.0118 (8/8) | |
| | h9l (one-hop, 120,000 digits) | 0.9363 | 0.9298 | +0.0065 (8/8) | +0.0134 (8/8) | |
| std, pairs | born / teacher | 0.6735 / 0.7643 | | | | |
| | base | 0.6990 | 0.6924 | +0.0066 (7/8) | +0.0332 (8/8) | 20.4 / 27.1 |
| | count5 | 0.7046 | 0.6775 | +0.0271 (8/8) | +0.0410 (8/8) | 19.6 / 21.2 |
| | c80d | 0.7084 | 0.6929 | +0.0155 (8/8) | +0.0423 (8/8) | 19.9 / 26.7 |
| | hbase | 0.6912 | 0.6880 | +0.0032 (5/8) | +0.0199 (8/8) | |
| | h9l | 0.7009 | 0.6836 | +0.0173 (8/8) | +0.0288 (8/8) | |
| quiet, MNIST | born / teacher (4 seeds) | 0.9535 / 0.9702 | | | | |
| | base | 0.9546 | 0.9558 | -0.0012 (1/8) | | |
| | count5 | 0.9560 | 0.9522 | +0.0038 (8/8) | | |
| | c80d | 0.9573 | 0.9558 | +0.0015 (5/8) | | |
| | hbase (4 seeds) | 0.9576 | 0.9549 | +0.0028 (4/4) | | |
| quiet, pairs | born / teacher (4 seeds) | 0.7512 / 0.8155 | | | | |
| | base (6 seeds) | 0.7648 | 0.7634 | +0.0014 (4/6) | | |
| | count5 (5 seeds) | 0.7655 | 0.7544 | +0.0106 (5/5) | | |
| | c80d | 0.7687 | 0.7626 | +0.0061 (8/8) | | |
| | hbase (4 seeds) | 0.7597 | 0.7654 | -0.0057 (0/4) | | |

Read rate 31.1-31.2 standard, 31-40 quiet; life rate 41 in the first tenth to 31 in the last;
no silent or stuck cells in any of these. The room asked for, at the engine's own rate: born
0.924 / 0.673, competitive learning as it fires 0.952 / 0.726, teacher 0.966 / 0.785
(standard); born 0.954 / 0.752, teacher 0.975 / 0.826 (quiet). At cap the teacher reads
0.9569 / 0.7643 standard and 0.9702 / 0.8155 quiet (4 seeds).

## Every arm, standard read, seeds 5-8

Lived minus ctrl1 (shuffled / coin), and that less base's (hbase's), paired.
`out/steady/table-std-5-8.txt`.

| arm | MNIST lived-ctrl1 | vs base | pairs lived-ctrl1 | vs base | life rate first > last |
|---|---|---|---|---|---|
| base | +0.0021 | | +0.0053 | | 41>31 |
| s40d shown 40, same digits | +0.0025 | +0.0005 | +0.0182 | +0.0129 | 37>31 |
| s40t shown 40, same ticks | +0.0033 | +0.0012 | +0.0092 | +0.0039 | 42>31 |
| s80d shown 80, same digits | +0.0035 | +0.0015 | +0.0112 | +0.0059 | 34>31 |
| s80t shown 80, same ticks | +0.0046 | +0.0025 | +0.0127 | +0.0074 | 42>31 |
| slowth thresholds 8x slower | +0.0061 | +0.0040 | +0.0227 | +0.0174 | 69>33 |
| th32 frozen, bias 32 (runs away) | +0.0051 | +0.0030 | +0.0165 | +0.0112 | 77>86 |
| thfreeze frozen, bias 128, targets 60% | +0.0034 | +0.0013 | +0.0127 | +0.0074 | 30>33 |
| slowbias | +0.0029 | +0.0008 | +0.0092 | +0.0039 | 41>31 |
| count5 | +0.0126 | +0.0105 | +0.0257 | +0.0204 | 41>31 |
| count1 (on at least once) | +0.0254 | +0.0234 | +0.0518 | +0.0465 | 33>31 |
| r6 / r7 | +0.0034 / +0.0041 | | +0.0241 / +0.0121 | | 41>31 |
| r5l / r6l / r7l (120,000 digits) | +0.0034 / +0.0048 / +0.0027 | | +0.0096 / +0.0077 / +0.0098 | | 37>31 |
| cslow (count5 + slowth) | +0.0193 | +0.0172 | +0.0480 | +0.0427 | 63>33 |
| c3 (count5, shift 3) | +0.0056 | +0.0035 | +0.0094 | +0.0041 | 41>31 |
| c80d | +0.0098 | +0.0077 | +0.0144 | +0.0091 | 34>31 |
| hbase | +0.0033 | | +0.0054 | | 34>31 |
| hs40d / hs80d / hs80t | +0.0031 / +0.0028 / +0.0004 | | +0.0061 / +0.0103 / -0.0015 | | 32-33>31 |
| hslowth / hslowbias | +0.0056 / +0.0032 | | +0.0110 / +0.0105 | | |
| hthfreeze (60%: nearly dies, 8>15) / hthf140 | +0.0020 / +0.0052 | | -0.0012 / +0.0063 | | |
| h10 / h11 / h10l / h11l | +0.0020 / +0.0021 / +0.0033 / +0.0022 | | +0.0075 / -0.0016 / +0.0065 / -0.0012 | | 33>31 |
| h9l (120,000 digits) | +0.0059 | +0.0027 | +0.0185 | +0.0131 | 33>31 |

count1 and cslow: the lived sheet is no better than count5's (count1 worse, 0.9235); the margin
is a shuffled life that wires almost no pixels (count1) or fewer (cslow, 14.6).

Quiet read, seeds 5-8, lived minus shuffled, MNIST / pairs: base -0.0007 / +0.0005, count5
+0.0042 / +0.0124, slowth +0.0001 / +0.0071, s80d -0.0014 / +0.0018, s80t -0.0007 / +0.0019,
r6 0.0000 / +0.0080, thfreeze +0.0016 / +0.0017, cslow +0.0124 / +0.0421, c3 +0.0010 /
+0.0010, c80d +0.0011 / +0.0064; hslowth over its coin +0.0014 / -0.0005.

## Hardware cost of the counted rule

Per cell a spike count for the window (5 bits for 20 ticks, 7 for 80; a byte in the code) and
one bit per neuron for "on in the window" (a word per tile). Per tick: one OR per tile and an
increment per spike. Per window: a top-4 over the tile's fired cells and about one move a
tile (the spike rule at cap makes about 0.9 a tile a digit). Not benchmarked with the rule on;
with compete_window 0 the change costs nothing measurable (blobbench in commit a71e754).

## What is real, what is suspect

- Real (8 seeds): the counted rule's lived sheet beats the spike rule's on both tests in the
  standard read (+0.003 / +0.006) and its margin over shuffled is larger (8 of 8). The one-hop
  rule's margin over its coin grows with a 120,000-digit life.
- Suspect: lived minus shuffled is not a clean measure when an arm changes how many pixels a
  shuffled life wires; the pixel-matched line is a crude correction (a straight line through
  two clusters, churn near 7 pixel inputs, shuffled near 22-27). Quiet-read lines have no
  churn sheets in them.
- Suspect: pair margins move by 0.01-0.02 between arms that should be alike (r6 against r6l);
  every pair number of the 4-seed search is a hint only.
- Life rates are not matched for slowth (69 in the first tenth) and th32 (runs away to 86);
  hthfreeze at 60% targets nearly dies; hthf140 goes 27 to 38.

## Not finished at tabling

- Quiet reads, seeds 9-12, of the born sheets and the teacher (4 seeds), of pairs for base
  and count5 (6 and 5 seeds), of hbase; of h9l on any seed; of every churn sheet; of born-hop.
- h9l standard read seeds 9-12 only partly; h9lslow (long life and slow thresholds) one seed.
- `blobbench` with the counted rule on.
- Quiet reads of the search arms not in the quiet line above (s40d, s40t, th32, slowbias, r7,
  the long competitive lives, the one-hop arms but hbase and hslowth).

## Predictions, scored briefly

- Margin grows by 0.003+ on MNIST under some arm (0.25): yes, the counted rule +0.0105 over
  base, mostly through a weaker control. By 0.01+ on pairs (0.3): yes, the same arm.
- Same ticks grows it less than same digits (0.6): no (s80t +0.0046, s80d +0.0035).
- Quiet read shows a larger margin on the base life (0.6 / 0.65): wrong, -0.0012 / +0.0014.
- Frozen thresholds with bias 32: rate 40-60, 5% silent (0.5): the rate ran to 86, none
  silent. Slower regulation within 10% of target (0.8): no for slowth (69 early).
- Counted rule within 0.005 of B5's lived sheet (0.6): yes, +0.003.
- One-hop margin grows by 0.003+ (0.3): +0.0023 MNIST, +0.014 pairs, from the longer life at
  the same step, not a slower one. Longer showing does little for it (0.7): right.
- Nothing reaches the teacher (0.97): right.

## Script lines

```
scripts/steady.sh search --reads std                                   every arm, seeds 5-8
scripts/steady.sh search --seeds 9-12 --arms base,count5,c80d,hbase    the conclusion arms
scripts/steady.sh search --reads quiet --kinds born,teacher,lived,shuffled --arms base,count5,c80d
scripts/steady.sh search --seeds 5-12 --table-only --arms base,count5,c80d,hbase,h9l
scripts/steady.sh smoke --quick
```

Long lives ran on stevejobs (`MINI_DIR=thonk-steady`, `THONK_MNIST=$HOME/thonk-hop2/data/mnist`).

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`: the inner rules do not learn more about digits from a steadier sheet in
  the life; the quiet read flattens lived sheets (MNIST 0.955-0.957). A whole-digit count for
  competitive learning lifts the lived sheet a little (+0.003 / +0.006) and the shuffled
  margin a lot, mostly by wiring fewer pixels without structure. [likely, 8 seeds]
- `docs/findings.md`: lived minus shuffled is confounded by the shuffled sheet's pixel count;
  report pixel inputs beside every shuffled control. [likely]
- `docs/plan.md`: item 2j tabled with this doc; the unfinished runs above.
- `docs/ideas.md`: the counted rule (compete_window) as a cheap competitive rule; a shuffled
  control matched for pixel inputs.
- `docs/design.md`: compete_window, compete_window_on; `docs/notes-world.md`: `blobmnist
  ceiling --life-ticks | --life-settle | --life-set | --life-target`; `scripts/steady.sh`.
- `docs/predictions.md`: the predictions above.

Files that may collide on merge: `src/blob/genome.hpp`, `src/blob/genome_args.cpp` (two numbers
at the end), `src/blob/learn_compete.cpp` (two fields, one argument on move_cell, a check at
the top of learn_compete, a block at the end), `src/mnist/mnist_ceiling.cpp`,
`src/mnist/mnist_run.hpp`; new `src/mnist/mnist_steady.cpp`, `scripts/steady.sh`,
`scripts/steady.py`.
