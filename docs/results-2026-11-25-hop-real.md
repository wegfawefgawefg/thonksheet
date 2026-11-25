# What stops the one-hop error on real data (2026-11-25)

Queue item 2d of `docs/plan.md`, idea 163, following `docs/results-2026-11-25-hop.md` (idea
159). There the one-hop error rule taught the inside products on the toys and on held-out
calculation cases, far above its coin control, but on MNIST the coin control got nearly all of
its gain (0.9355 against 0.9346). Three suspects: MNIST is the wrong yardstick (a straight
line already reads it); one cell cannot become what the error asks (it hears about 6 pixels);
the errors are poor (a readout right two times in three, with dead buttons).

## Predictions (written before any run, after reading the code)

Nothing was run before these were written; not even the baselines of the new test.

1. A real-data test from the MNIST files exists where a linear reader of the born sheet is
   at least 0.10 under a two-layer net: 0.85.
2. On it, the hop rule told the answer (8 report seeds) beats its coin control by 0.02 or
   more: 0.30. Beats the born sheet by 0.02 or more: 0.45. Beats competitive learning
   alone: 0.35.
3. Shuffled-input control on the toys: the hop rule's lived sheet beats the same life on
   shuffled inputs by 0.05 or more on xor and on parity of 3: 0.8. On the calc held-out
   xor4 column by 0.05 or more: 0.75.
4. Wider cells (more pixel inputs a cell) on MNIST: the hop rule beats its coin by 0.005 or
   more: 0.2. The wide lived sheet beats the normal lived sheet (0.9355) by 0.005 or more:
   0.35.
5. Depth (a share of cells that hear no pixels, only cells): parity of 4 reaches 0.60 on the
   toy fresh-output test: 0.10. Depth lifts xor4 held-out by 0.03 or more over no depth:
   0.25. Depth lifts the new real-data test's lived minus coin by 0.01 or more: 0.2.
6. Parity of 4 with near-not-fired marks (`--g.hop_fired 0`) reaches 0.55: 0.15.
7. Errors against the answer alone (every step teaches, target minus a running rate per line)
   on MNIST: beats its own coin by 0.005 or more: 0.30; its lived sheet beats today's 0.9355
   by 0.003 or more: 0.3.

## Short version

- **MNIST being easy is not the reason.** A test made from the same files that a straight
  line fails: two half-size digits interleaved on the sheet, "are they the same class"
  (linear on the bits 0.510, born sheet 0.677, random projection 0.705, two-layer net 0.796
  to 0.852). The rule told the answer (best setting on validation) lifts the sheet to
  0.691, over born and over the shuffled-digit life on all 8 seeds, and **its coin control
  gets 0.691 too** (+0.0007, 5 of 8; a coin whose errors never cancel, -0.0003). On "is the sum of two side-by-side digits
  odd" nothing moves anything (all 0.52; net 0.77).
- **Better errors are the biggest lever found.** Errors from the answer alone on every step
  (`--g.hop_teach 3`: each line's target less its running rate, the readout's state left
  out) beat their coin on MNIST **0.9377 against 0.9317, 8 of 8 seeds** (+0.0060; born
  0.9251), and on the pair test 0.698 against 0.690 (4 of 4). The readout's errors at the
  same slow step get +0.0015 and +0.0027, against coins that match (same churn). Held at
  target rate the MNIST margin is +0.0033 (4 of 4), about the old rule's at target.
- **The cell, half.** Four times the pixel inputs a cell makes a better born sheet (MNIST
  0.936 with no life) and the rule then pulls the cells back off the pixels; the errors'
  share barely moves (+0.002). Half the cells hearing only cells lets the old errors count on
  the pair test (+0.0071 over the coin, 8 of 8, against +0.0007), but that sheet starts and
  ends lower. Neither helps the calc columns or parity of 4.
- **The toy and calc results stand on two new controls.** The transfer test had left the
  rule on while the fresh readout learned; frozen, parity of 3 is 0.847 (hop doc 0.816), coin
  0.618, and a life on shuffled inputs (each step shows another step's lines) 0.607; xor
  0.997, 0.945, 0.946; calc xor4 held-out 0.871 against 0.667 coin and 0.692 shuffled.
- **Parity of 4: nothing**, under any arm (0.46 to 0.50).
- Nothing here reaches competitive learning (pairs 0.726, MNIST 0.952) or a small net.

## What was built

All off by default; the four reference lives are byte-identical (checked at the end).

- **Pairs of digits, a real-data test a straight line fails** (`src/world/mnist_data.cpp`,
  `task_mnist.cpp`; `blobmnist ceiling|online|baselines --pair 1|2 --pair-mix 0|1`). Two
  MNIST digits at half size (2x2 averaged, on at half ink) share the usual 28x28 sheet; two
  buttons. `--pair 2`: are they the same class (half the pairs are); `--pair 1`: is the sum
  of their labels odd. `--pair-mix 1` interleaves them (the first digit on even columns, the
  second on odd ones, each half-size row shown twice), so the pixels of the two digits at one
  place sit side by side and a local cell hears both; `--pair-mix 0` puts them side by side
  (columns 0-13 and 14-27). The second digit is drawn from the first one's place in the
  order, so each pass pairs the digits afresh; the test set's pairs are fixed by the seed,
  the same for every arm of one seed. The shuffled-digit control (`--life-shuffle 1`) works
  unchanged (each pair's pixels moved by a fresh permutation). In pair mode the wiring
  report's "pixel likeness" still uses single digits' pixel correlations, so it is not
  comparable to MNIST's; it is left out of the pair tables.
- **The shuffled-input control for toys and calc** (`src/probe/input_scramble.cpp`;
  `blobprobe transfer ... --life-scramble 1`, `blobprobe calc ... --life-scramble 1`). In the
  life each step the sheet is shown the input lines of another step, drawn at random from
  the last 256 the task showed, while the world grades the step it really set. Same cases,
  same density, nothing about the answer. (Permuting a toy's lines, as MNIST permutes pixels,
  would not do: exclusive-or and parity do not change when their lines are swapped.)
- **The transfer test now freezes the inside.** `blobprobe transfer` put the motor cells back
  as born and switched the tags, replay and margin rules off for the fresh readout, but left
  `hop_on` (and `compete_on`) on, so in the hop doc's toy numbers the one-hop rule went on
  teaching the inside during the fresh readout's 20,000 steps. `freeze_inside` in
  `src/probe/lived_sheet.cpp` now turns both off; `--then-inside 1` gives the old behaviour.
  Checked on parity of 3, seed 5: the old way reproduces the hop doc's 0.826; frozen it is
  0.867 (coin 0.628 frozen, 0.582 the old way). So the hop doc's toy result does not come from
  the rule teaching during the read; frozen it is a little stronger. Every toy number in this
  doc is frozen.
- **Wider cells** (`--g.wire_pixel_shift S`, `src/blob/birth.cpp`): at birth a hidden
  excitatory cell (not sensor, motor, pacemaker or inhibitory) also wires each sensor cell of
  its 9 local tiles with chance 2^-S, strong as usual. Every rewire keeps the born counts, so
  the cells stay wide. S = 2 takes a MNIST hidden cell from 7.3 pixel inputs of 34 to 30.4
  of 62 (seed 5); the pixels still come from its own 12 x 12 pixel neighbourhood.
- **Depth** (`--g.wire_deep_percent P`, `src/blob/birth.cpp`, `tag_store.hpp`): P% of the
  hidden excitatory cells, picked by a hash of the cell number, hear no sensor cell: their
  pixel inputs are removed at birth and every rewire (the hop rule's, competitive learning's,
  the tags') leaves sensor cells out for them. They hear only cells, many of which hear
  pixels, and each is still taught one hop from the error like every other cell. Nothing
  goes backwards.
- **Errors from the answer alone** (`--g.hop_teach 3`, `--g.hop_rate_shift R`,
  `src/blob/learn_hop.cpp`): every verdict, good or bad, teaches. Each line's error is its
  target (1 if it is the right line, 0 if not) less its running rate of being right (in
  1/65536ths, over about 2^R verdicts), so what the readout pressed plays no part, and a
  hidden cell's teaching bit is the sign of its wires times those errors. This is direct
  random target projection (Frenkel et al. 2021), centred. The coin control draws the right
  line at random. Off by default (hop_teach 1 and 2 are untouched).
- `blobmnist baselines --net-passes N` (the two-layer net's passes, default 1).
- `scripts/hop2.sh NAME [--quick]`: every table below from one line (`scripts/hop2.py`
  holds the recipes).

## Seeds: where each setting was picked

- The pair tests: the rule's step chance and marks (`--g.hop_shift 7 --g.hop_fired 0`) were
  picked on the **validation slice** (train on digits 0-49,999, score on 50,000-59,999),
  seeds 1 and 2 (`scripts/hop2.sh pick`). Reported on the test set, seeds 5-12, never used
  for picking.
- Errors from the answer alone: `--g.hop_shift 9` picked on the validation slice, seeds 1
  and 2 (`scripts/hop2.sh targetpick`); reported on MNIST test seeds 5-12, and on the pair
  test and with the rate held at target on seeds 5-8 (the same shift, not picked again).
- MNIST with wider cells: the hop doc's MNIST setting (`--g.hop_shift 7`), which that doc
  picked on **test** seeds 1 and 2; reported here on test seeds 5-8 only, against that
  doc's numbers for the same seeds.
- Wider cells (`--g.wire_pixel_shift 2`) and depth (`--g.wire_deep_percent 50`): one value
  each, chosen before any run, not tuned.
- Toys and calc: the hop doc's untuned defaults (told the answer, `hop_shift 3`), seeds 5-12
  (toys), 5-8 (parity of 4), 5 and 6 (calc).
- Competitive learning: the compete doc's setting B5 (`--g.compete_on 1 --g.tile_winners 4
  --g.compete_shift 5`), not tuned here.

## Point 1: a real-data test a straight line fails

### The bar and the room (`scripts/hop2.sh bar`)

Four pair tests made from the MNIST files, 60,000 training pairs, test pairs from the test
set. "linear": the softmax reader on the sheet's bits (3 passes); "born": a fresh linear
reader of the non-sensor cells of the born sheet (seed 1, fit on 60,000 pairs; the
inside-learning test's reader); "random": 4,096 random 32-line threshold units read the same
way; "net": a two-layer net, 100 hidden units, backprop, 1 or 5 passes.

| test | linear | born sheet | random projection | net, 1 pass | net, 5 passes |
|---|---|---|---|---|---|
| **same class, interleaved** | 0.510 | **0.676** | 0.705 | 0.796 | 0.852 |
| parity of the sum, side by side | 0.490 | 0.521 | 0.631 | 0.767 | 0.772 |
| same class, side by side | 0.511 | 0.553 | 0.744 | 0.810 | 0.854 |
| parity of the sum, interleaved | 0.486 | 0.604 | 0.587 | 0.762 | 0.772 |

All four are tests a straight line fails (0.49 to 0.51 on the bits) and a small two-layer net
passes (0.76 to 0.85). The born sheet is 0.12 to 0.25 under the net on each.

Picked before any rule was run on them: **same class, interleaved** as the main test (8
seeds, every arm). The product it needs is local: the pixel of one digit times the pixel of
the other at the same place sits side by side on the sheet, so one cell can hear both, and
the born sheet's cells already carry some of it (0.676 against 0.510 on the bits). Parity of
the sum, side by side, as the second (4 seeds): the born sheet is at chance there, and the
product it needs (one digit's parity times the other's) is far apart.

### Picking the rule's setting (`scripts/hop2.sh pick`, validation, seeds 1 and 2)

Same class, interleaved, told the answer, 60,000-pair taught life. Mean of seeds 1 and 2:

| setting | lived | its coin | lived - coin | rate lived / coin |
|---|---|---|---|---|
| `--g.hop_shift 3` | 0.6701 | 0.6849 | -0.0149 | 33 / 34 |
| `--g.hop_shift 5` | 0.6870 | 0.6960 | -0.0090 | 34 / 38 |
| `--g.hop_shift 7` | 0.6924 | 0.6990 | -0.0066 | 39 / 39 |
| **`--g.hop_shift 7 --g.hop_fired 0`** | **0.6991** | 0.6973 | **+0.0019** | 41 / 40 |
| readout alone in the life (off) | 0.6803 | | | 33 |

Only near-not-fired marks at the slow step put the real errors over their coin, by a hair.
That setting is the report's. The readout in the life is at chance on this task (0.51 to
0.52 right), so every second step is a verdict, and on a two-button task "the pressed one
was wrong" names the right one: the errors are the labels, whatever the readout does.

### The report (`scripts/hop2.sh same`, test, seeds 5-12)

Same class, interleaved; test set; seeds 5-12; a 60,000-pair taught life (the readout of
`scripts/mnist.sh` learning, the rule hearing the errors), motor cells put back as born, a
fresh linear reader of the non-sensor cells fit on 60,000 pairs. "rate": hidden spikes per
1,000 cell-ticks while read; "pixels": pixel inputs per hidden excitatory cell; "kept": born
inputs still wired. The hop arms are `--g.hop_on 1 --g.hop_shift 7 --g.hop_fired 0`, told the
answer.

| arm | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate | pixels | kept |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| born | 0.6924 | 0.6724 | 0.6675 | 0.6784 | 0.6760 | 0.6838 | 0.6813 | 0.6675 | 0.6774 | 32.7 | 7.2 | 1.00 |
| **hop rule** | 0.7019 | 0.6887 | 0.6755 | 0.6997 | 0.6966 | 0.6921 | 0.6902 | 0.6868 | **0.6914** | 40.7 | 6.8 | 0.44 |
| coin | 0.6981 | 0.6935 | 0.6861 | 0.6948 | 0.6940 | 0.6800 | 0.6995 | 0.6802 | 0.6908 | 40.0 | 6.8 | 0.47 |
| coin on distinct lines (`hop_coin 2`) | 0.6941 | 0.6943 | 0.6886 | 0.6995 | 0.6906 | 0.6867 | 0.7048 | 0.6756 | 0.6918 | 40.0 | 6.7 | 0.44 |
| shuffled digits | 0.6761 | 0.6794 | 0.6657 | 0.6771 | 0.6772 | 0.6772 | 0.6793 | 0.6685 | 0.6751 | 33.1 | 8.2 | 0.46 |
| competitive learning | 0.7248 | 0.7255 | 0.7205 | 0.7309 | 0.7257 | 0.7232 | 0.7260 | 0.7290 | 0.7257 | 65.8 | 16.3 | 0.23 |
| both | 0.7312 | 0.7271 | 0.7329 | 0.7307 | 0.7311 | 0.7342 | 0.7278 | 0.7304 | 0.7307 | 65.3 | 16.3 | 0.14 |
| both, the hop rule's coin | 0.7376 | 0.7320 | 0.7317 | 0.7303 | 0.7287 | 0.7270 | 0.7349 | 0.7226 | 0.7306 | 65.0 | 16.4 | 0.14 |

| difference (seed by seed) | mean | seeds above | range |
|---|---|---|---|
| hop - born | +0.0140 | 8 of 8 | +0.0080 to +0.0213 |
| hop - shuffled digits | +0.0164 | 8 of 8 | +0.0093 to +0.0258 |
| **hop - coin** | **+0.0007** | 5 of 8 | -0.0106 to +0.0121 |
| hop - coin on distinct lines | -0.0003 | 5 of 8 | -0.0146 to +0.0112 |
| both - competitive learning | +0.0050 | 7 of 8 | -0.0002 to +0.0124 |
| **both - both with the coin** | **+0.0001** | 5 of 8 | -0.0071 to +0.0078 |

Bar and room on this test: linear on the bits 0.510, random projection 0.705, two-layer net
0.796 (1 pass) to 0.852 (5 passes).

- **The errors add nothing here either.** The lived sheet beats born by 0.014 and the
  shuffled-digit life by 0.016 on every seed, the same shape as MNIST, and the coin control
  gets all of it (+0.0007, 5 of 8). On a two-line task the plain coin teaches fewer cells
  than the real errors (its two errors land on one line half the time and cancel: seed 5,
  6.6 million cells taught against 12.2 million); the coin on distinct lines teaches as many
  and gets the same (0.6918). Rates while read are matched (40 to 41 for all three).
- **What lifts the sheet is the churn toward co-active pixels**, as on MNIST: born inputs
  kept fall to 0.44 to 0.47 in both the real and the coin life, and only the shuffled life,
  with no co-active pixels to move toward, stays at born.
- **Competitive learning is the best inside rule on this test too**: 0.726 against born
  0.677, past the random projection (0.705), every seed; at twice the rate (66 against 33),
  the compete doc's caveat. With the hop rule beside it, 0.731 (7 of 8 seeds above
  competitive learning alone), but its coin gives the same 0.731: the extra 0.005 is churn.
- No arm reaches the two-layer net (0.80 to 0.85).

### The second test: is the sum odd (`scripts/hop2.sh sum`, seeds 5-8)

Side by side; the same arms (no competitive learning).

| arm | 5 | 6 | 7 | 8 | mean | rate |
|---|---|---|---|---|---|---|
| born | 0.5188 | 0.5291 | 0.5281 | 0.5127 | 0.5222 | 31.2 |
| hop rule | 0.5268 | 0.5154 | 0.5175 | 0.5252 | 0.5212 | 36.9 |
| coin | 0.5282 | 0.5295 | 0.5219 | 0.5355 | 0.5288 | 35.2 |
| coin on distinct lines | 0.5222 | 0.5254 | 0.5196 | 0.5345 | 0.5254 | 35.4 |
| shuffled digits | 0.5208 | 0.5241 | 0.5228 | 0.5258 | 0.5234 | 31.1 |

Nothing moves: every arm reads 0.52 (random projection 0.63, net 0.77). The product this
test needs (one digit's parity against the other's, 14 pixels apart) is not in the born
sheet and no rule here builds it; the hop rule is under its coin on all 4 seeds (-0.008).

## Point 2: can one cell become what the error asks?

### Wider cells on MNIST (`scripts/hop2.sh wide`, test, seeds 5-8)

`--g.wire_pixel_shift 2` on every arm: 30.4 pixel inputs a hidden cell (of 62) instead of
7.3 (of 34). The rule at the hop doc's MNIST setting (`--g.hop_shift 7`, told the answer).
Beside it, the hop doc's runs of the plain sheet on the same seeds (`scripts/inside.sh
hop2s7-report`).

| arm | 5 | 6 | 7 | 8 | mean | rate | pixels a cell | kept |
|---|---|---|---|---|---|---|---|---|
| wide, born | 0.9372 | 0.9344 | 0.9362 | 0.9348 | **0.9356** | 65.4 | 30.6 | 1.00 |
| wide, hop rule | 0.9316 | 0.9338 | 0.9274 | 0.9303 | 0.9308 | 67.2 | 10.7 | 0.23 |
| wide, coin | 0.9279 | 0.9313 | 0.9257 | 0.9295 | 0.9286 | 76.4 | 10.1 | 0.22 |
| wide, shuffled digits | 0.9228 | 0.9187 | 0.9209 | 0.9195 | 0.9205 | 52.8 | 12.6 | 0.19 |
| plain, born (hop doc) | 0.9248 | 0.9272 | 0.9273 | 0.9239 | 0.9258 | 33 | 7.3 | 1.00 |
| plain, hop rule (hop doc) | 0.9358 | 0.9347 | 0.9353 | 0.9375 | 0.9358 | 38 | 5.7 | 0.21 |
| plain, coin (hop doc) | 0.9355 | 0.9349 | 0.9350 | 0.9344 | 0.9350 | 38 | 5.8 | 0.22 |

- **Wider cells help the born sheet, not the rule.** The wide born sheet reads 0.936, the
  plain lived sheet's level, with no life (at twice the rate, 65: its cells get more drive
  and regulation stops at its cap). A life under the rule then pulls the cells back off the
  pixels (30.6 to 10.7 a cell; the coin does the same) and the sheet ends **under** its born
  self (-0.005, every seed).
- The errors' share over the coin is +0.0022 (4 of 4, +0.0008 to +0.0037), against +0.0009
  for the plain sheet on the same seeds. A hint that wider cells let the errors say a little
  more, at a size the hop doc already called a few thousandths; not a change of picture.

### Wider cells on the same-class test (`scripts/hop2.sh widesame`, seeds 5-8)

| arm | 5 | 6 | 7 | 8 | mean | rate | pixels a cell |
|---|---|---|---|---|---|---|---|
| wide, born | 0.6904 | 0.6871 | 0.6955 | 0.6870 | 0.6900 | 64.0 | 30.6 |
| wide, hop rule | 0.6955 | 0.7015 | 0.6897 | 0.6784 | 0.6913 | 65.5 | 17.6 |
| wide, coin | 0.6926 | 0.6822 | 0.6915 | 0.6887 | 0.6887 | 80.2 | 17.2 |
| wide, shuffled digits | 0.6809 | 0.6705 | 0.6700 | 0.6765 | 0.6745 | 62.0 | 19.4 |

The same: the wide born sheet (0.690) is already where the plain lived sheet ends (0.691);
the rule adds +0.0013 over it and +0.0025 over its coin (2 of 4 seeds, -0.010 to +0.019).

### Depth on the same-class test (`scripts/hop2.sh deepsame`, seeds 5-12)

`--g.wire_deep_percent 50` on every arm: half the hidden excitatory cells hear no pixel (3.5
pixel inputs a cell on average, against 7.2), each still taught one hop from the error.

| arm | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate |
|---|---|---|---|---|---|---|---|---|---|---|
| deep, born | 0.6664 | 0.6647 | 0.6543 | 0.6596 | 0.6641 | 0.6703 | 0.6662 | 0.6522 | 0.6622 | 30.6 |
| deep, hop rule | 0.6908 | 0.6751 | 0.6697 | 0.6972 | 0.6792 | 0.6776 | 0.6945 | 0.6816 | **0.6832** | 34.4 |
| deep, coin | 0.6888 | 0.6659 | 0.6685 | 0.6796 | 0.6740 | 0.6763 | 0.6838 | 0.6721 | 0.6761 | 34.0 |
| deep, shuffled digits | 0.6700 | 0.6561 | 0.6552 | 0.6657 | 0.6607 | 0.6666 | 0.6663 | 0.6536 | 0.6618 | 30.8 |

- **With cells that hear only cells, the errors do add something over their coin: +0.0071,
  every one of 8 seeds (+0.0012 to +0.0176)**, at matched rates (34.4 and 34.0). On the plain
  sheet the same rule adds +0.0007 (5 of 8). Lived minus born is +0.021 (plain +0.014).
- But the deep sheet starts lower (born 0.662 against 0.677: half its cells lost their
  pixels) and ends lower (0.683 against the plain sheet's 0.691). Depth makes the errors
  count without making the sheet better than the plain one with churn.
- Depth on the calc columns and on parity of 4: nothing (tables above).

## Point 3: errors from the answer alone (`--g.hop_teach 3`)

Every verdict teaches, good or bad; each line's error is its target less its running rate,
so the readout's state plays no part. `--g.hop_shift 9` (picked on validation, seeds 1 and
2: lived minus coin +0.0061 at shift 9, +0.0040 at shift 7; `scripts/hop2.sh targetpick`).

**MNIST** (`scripts/hop2.sh target`, test, seeds 5-12):

| arm | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | mean | rate |
|---|---|---|---|---|---|---|---|---|---|---|
| born | 0.9248 | 0.9272 | 0.9273 | 0.9239 | 0.9255 | 0.9192 | 0.9249 | 0.9278 | 0.9251 | 33.1 |
| **from the answer** | 0.9379 | 0.9379 | 0.9387 | 0.9373 | 0.9366 | 0.9384 | 0.9377 | 0.9371 | **0.9377** | 38.4 |
| its coin | 0.9328 | 0.9290 | 0.9287 | 0.9302 | 0.9348 | 0.9313 | 0.9320 | 0.9351 | 0.9317 | 36.6 |
| shuffled digits | 0.9248 | 0.9247 | 0.9265 | 0.9209 | 0.9208 | 0.9219 | 0.9290 | 0.9284 | 0.9246 | 32.9 |

Lived minus coin **+0.0060, 8 of 8 seeds (+0.0018 to +0.0100)**; minus born +0.0126 (8 of
8); minus shuffled +0.0131 (8 of 8). The hop doc's rule on the same seeds (told the right
button on bad steps, shift 7): lived 0.9355, coin 0.9346, +0.0009.

Two controls for what this is:

| MNIST, seeds 5-8 | lived | coin | lived - coin | rate lived / coin |
|---|---|---|---|---|
| from the answer, shift 9 | 0.9379 | 0.9302 | +0.0078 (4 of 4) | 38 / 36 |
| told the right button on bad steps, **same shift 9** (`scripts/hop2.sh slow`) | 0.9328 | 0.9313 | +0.0015 (3 of 4) | 37 / 37 |
| from the answer, shift 9, **rate held at target** (`scripts/hop2.sh targetcap`) | 0.9332 | 0.9299 | +0.0033 (4 of 4) | 31 / 31 |
| told the right button, shift 7, rate held at target (hop doc, same seeds) | 0.9347 | 0.9322 | +0.0025 | 31 / 31 |

- **At the same step chance the errors from the answer are worth more than the readout's
  errors**: the two coins agree (0.9302, 0.9313: the same churn), the lived sheets do not
  (0.9379 against 0.9328). So the kind of error matters, not only how fast the counters
  move. Each verdict of the old rule (a third of the steps, about the readout's own
  confusions) teaches less than each step of the new one (every step, about the digit).
  Counter steps in the life: 7.6 million from the answer, 7.1 million its coin, 1.4 million
  for the old rule at shift 9.
- **Held at target rate, the margin shrinks to +0.0033**, about what the old rule gets held
  at target (+0.0025 on these seeds, +0.0033 on 8). Part of the free-rate margin comes with
  a rate the real errors raise a little more than the coin does (38.4 against 36.6). Two
  rate-held arms on 4 seeds; the least settled part of this doc.
- The best MNIST number the one-hop rule has reached (0.9377), still under competitive
  learning (0.952) and the random projection (0.943).

**Same-class pairs** (`scripts/hop2.sh targetsame`, seeds 5-8, rates while read in brackets):

| | born | from the answer, shift 9 | its coin | shuffled | told the answer on bad steps, shift 9 (`slowsame`) | its coin |
|---|---|---|---|---|---|---|
| mean | 0.6777 | **0.6979** (37.6) | 0.6898 (37.5) | 0.6746 | 0.6907 (37.5) | 0.6880 (36.6) |

From the answer: +0.0081 over its coin (4 of 4, +0.0038 to +0.0127), +0.020 over born, at
matched rates. Told the answer only on bad steps at the same step: +0.0027 (3 of 4). On a
two-button task the old rule's errors on a bad step are already the label, so the
difference here is that every step teaches, good ones too: twice the labels.

## The toys, frozen, with the shuffled-input control (`scripts/hop2.sh toys`)

A 40,000-step life with the tags job's readout and the rule told the answer (the hop doc's
toy life), then the inside frozen, the motor cells put back as born and a fresh readout
learning the same task for 20,000 steps; its late score (first quarter in brackets). Seeds
5-12. "shuffled": the same life, each step showing another step's input lines.

| | born | **told the answer** | coin | shuffled input |
|---|---|---|---|---|
| xor | 0.855 (0.685) | **0.997 (0.951)** | 0.945 (0.788) | 0.946 (0.790) |
| parity of 3 | 0.552 (0.531) | **0.847 (0.716)** | 0.618 (0.564) | 0.607 (0.558) |

Per seed, parity of 3: told the answer 0.867, 0.825, 0.838, 0.830, 0.894, 0.862, 0.846,
0.810; coin 0.628, 0.606, 0.646, 0.590, 0.649, 0.575, 0.625, 0.628; shuffled 0.660, 0.616,
0.627, 0.589, 0.600, 0.539, 0.626, 0.595. Every seed is above both controls on both tasks.

- **The toy result stands, and the shuffled-input life is worth what the coin is worth.**
  On parity of 3 the shuffled life moved the counters more than the real one (seed 5: 9,576
  verdicts and 50 million counter steps against 4,337 and 46 million; its own readout stays
  at chance, 0.45 to 0.49) and lands at 0.607 against 0.847. So the toy gain is the errors
  being about the input, not the amount of rewiring.
- Frozen (this doc) against the rule left on during the fresh readout (the hop doc): parity
  of 3 0.847 against 0.816, xor 0.997 against 0.998, coin 0.618 against 0.587.

## Parity of 4 (`scripts/hop2.sh parity4`)

Same test, seeds 5-8, late score of the fresh readout:

| born | told the answer | coin | near and not fired | deep (50%) | deep, born | wide | wide, born |
|---|---|---|---|---|---|---|---|
| 0.473 | 0.493 | 0.490 | 0.467 | 0.479 | 0.458 | 0.498 | 0.464 |

Nothing reads parity of 4, in any arm: every number is within 0.03 of chance and of the
coin. Neither near-not-fired marks, nor cells that hear only cells, nor cells with four times
the pixel inputs change that.

## The calculation battery's held-out columns (`scripts/hop2.sh calc`)

Ridge reader on the frozen sheet after a 400,000-tick taught life (told the answer, a verdict
per bit), bits right seen / held-out, seeds 5 and 6; the last column is the mean held-out.

| member | arm | seed 5 | seed 6 | held |
|---|---|---|---|---|
| xor4 | born | 0.648 / 0.635 | 0.669 / 0.631 | 0.633 |
| | **told the answer** | 0.919 / 0.889 | 0.863 / 0.854 | **0.871** |
| | coin | 0.718 / 0.685 | 0.714 / 0.649 | 0.667 |
| | shuffled input | 0.713 / 0.699 | 0.701 / 0.686 | 0.692 |
| | deep, born | 0.644 / 0.628 | 0.623 / 0.571 | 0.599 |
| | deep | 0.870 / 0.854 | 0.893 / 0.881 | 0.868 |
| | deep, coin | 0.691 / 0.660 | 0.689 / 0.623 | 0.642 |
| | wide, born | 0.686 / 0.649 | 0.721 / 0.649 | 0.649 |
| | wide | 0.812 / 0.824 | 0.850 / 0.854 | 0.839 |
| | wide, coin | 0.665 / 0.653 | 0.652 / 0.584 | 0.619 |
| add4 | born | 0.634 / 0.566 | 0.623 / 0.563 | 0.565 |
| | **told the answer** | 0.739 / 0.652 | 0.738 / 0.667 | **0.660** |
| | coin | 0.661 / 0.566 | 0.661 / 0.542 | 0.554 |
| | shuffled input | 0.662 / 0.559 | 0.681 / 0.520 | 0.540 |
| | deep, born | 0.611 / 0.565 | 0.632 / 0.530 | 0.547 |
| | deep | 0.713 / 0.616 | 0.730 / 0.676 | 0.646 |
| | deep, coin | 0.663 / 0.591 | 0.647 / 0.588 | 0.589 |
| | wide, born | 0.642 / 0.578 | 0.690 / 0.588 | 0.583 |
| | wide | 0.728 / 0.666 | 0.719 / 0.649 | 0.658 |
| | wide, coin | 0.660 / 0.594 | 0.671 / 0.591 | 0.593 |
| parity4 | born | 0.673 / 0.098 | 0.660 / 0.100 | 0.099 |
| | told the answer | 0.843 / 0.026 | 0.788 / 0.020 | 0.023 |
| | coin | 0.705 / 0.182 | 0.630 / 0.013 | 0.098 |
| | shuffled input | 0.686 / 0.201 | 0.625 / 0.005 | 0.103 |
| | deep | 0.843 / 0.031 | 0.687 / 0.085 | 0.058 |
| | wide | 0.754 / 0.185 | 0.821 / 0.062 | 0.123 |

(The born, told-the-answer and coin rows are byte-for-byte the hop doc's: the defaults did
not move.)

- **The held-out columns pass the shuffled-input control too**: xor4 0.871 against 0.692,
  add4 0.660 against 0.540.
- **Depth and width add nothing here.** With half the cells hearing no pixels the rule gets
  xor4 0.868 and add4 0.646 (without: 0.871, 0.660); with four times the pixel inputs 0.839
  and 0.658. Each still beats its own coin by about as much as the plain sheet does (xor4
  +0.23 deep, +0.22 wide, +0.20 plain). These members need products of a few input lines,
  which the plain cells already make.
- Parity of 4's held-out column is the hop doc's trap (every held case's nearest seen case
  has the other answer); nothing generalises it.

## Reading

- **Suspect 1, the yardstick: not the reason.** On a test built from the same digits that a
  straight line fails (0.51) and a small net passes (0.80 to 0.85), where the product needed
  is local and the born sheet already holds some of it, the rule (told the answer, at its
  best setting on validation) does
  what it did on MNIST: it lifts the sheet over born and over the shuffled-digit life on every
  seed, and its coin control gets all of it (+0.0007, 5 of 8; with the coin's errors on
  distinct lines -0.0003). On the second test, whose product is far apart, nothing moves at
  all. MNIST being easy is not why the errors add nothing.
- **Suspect 3, the errors: part of the reason, the largest part found.** Errors that come
  from the answer on every step, not from the readout's mistakes, beat their coin on MNIST on
  every one of 8 seeds (+0.0060) and on the pair test (+0.0081, 4 of 4), where the readout's
  errors at the same slow step do not (+0.0015, +0.0027). At matched churn the kind of error
  is what differs. Held at target rate the MNIST margin falls to +0.003, so some of it rides
  on rate.
- **Suspect 2, the cell: half.** Cells with four times the pixel inputs make a better born
  sheet, and the rule then pulls them back off the pixels; the errors' share barely moves
  (+0.002). Cells that hear only cells, each still one hop from the error, do let the old
  errors count on the pair test (+0.0071 over the coin, 8 of 8, against +0.0007 without),
  but the deep sheet starts and ends lower than the plain one. Neither width nor depth helps
  the calc columns or parity of 4.
- **What the toys have that real data lacks** (a reading, consistent with every number here,
  not tested directly). The rule moves a cell's counters by the sign it is told times the
  sources that were on while the cell was near its bar. Averaged over cases, a source gains
  when its being on goes with "fire more". On xor, parity and same-class, no single line
  goes with the answer; only the near gate (the cell's own nonlinearity) can make a source's
  mark go with it. On the toys a cell hears 2 to 4 cue lines, each 9 sensor cells wide,
  shown thousands of times, and its gate is a strong function of exactly those lines: it
  becomes the product. On a digit pair a cell's gate depends on a handful of the sheet's
  784 lines, almost independently of the label, so the expected drift is tiny and the walk is
  the churn the coin also gives. What helps fits that: more labels per step (every step
  teaches), and cells whose inputs are already nonlinear (cells hearing cells).
- **Nothing here reaches a small net or competitive learning.** On the pair test the best
  arm of the hop rule is 0.698 (from the answer); competitive learning 0.726 (at twice the
  rate); the net 0.80 to 0.85. On MNIST 0.938 against 0.952 and 0.955.
- **The toy and calc results stand on two new controls**: the inside frozen during the read
  (the old transfer test left the rule on; frozen is a little higher), and a life on
  shuffled inputs (same cases, other answers), which reads as low as the coin.

## Predictions, scored

| prediction | p | outcome |
|---|---|---|
| 1. a real-data test with linear-on-born 0.10 or more under a two-layer net | 0.85 | right (0.676 against 0.796 to 0.852; all four tests) |
| 2. on it, told the answer beats its coin by 0.02 or more | 0.30 | wrong (+0.0007) |
| ... beats born by 0.02 or more | 0.45 | wrong (+0.014) |
| ... beats competitive learning alone | 0.35 | wrong (0.691 against 0.726) |
| 3. toys: lived beats the shuffled-input life by 0.05 on xor and parity 3 | 0.8 | right (+0.051, +0.240) |
| ... calc xor4 held-out by 0.05 | 0.75 | right (+0.18) |
| 4. wider cells, MNIST: beats its coin by 0.005 or more | 0.2 | wrong (+0.0022) |
| ... wide lived beats plain lived (0.9355) by 0.005 or more | 0.35 | wrong (0.9308, under its own born) |
| 5. depth: parity of 4 reaches 0.60 | 0.10 | right that it did not (0.479) |
| ... depth lifts xor4 held-out by 0.03 over no depth | 0.25 | right that it did not (0.868 against 0.871) |
| ... depth lifts the pair test's lived minus coin by 0.01 or more | 0.2 | right that it did not, narrowly (+0.0064: +0.0071 against +0.0007) |
| 6. parity of 4 with near-not-fired marks reaches 0.55 | 0.15 | right that it did not (0.467) |
| 7. errors from the answer, MNIST: beats its coin by 0.005 or more | 0.30 | happened (+0.0060, 8 of 8; +0.0033 at target rate) |
| ... its lived beats 0.9355 by 0.003 or more | 0.3 | did not (0.9377, +0.0022) |

I expected the yardstick to matter more than it did (point 1 was the bet) and the errors to
matter less (point 3).

## Script lines

Each table above from one line; finished runs are kept, so a line run again only runs what
is missing. Outputs in `out/hop2/NAME/`.

```
scripts/hop2.sh bar                 the bar and the room, four pair tests, seed 1
scripts/hop2.sh pick                the pair test's setting: validation, seeds 1, 2
scripts/hop2.sh same                the pair test report, seeds 5-12, every arm
scripts/hop2.sh sum                 parity of the sum, seeds 5-8
scripts/hop2.sh toys                toys frozen, with the shuffled-input control, seeds 5-12
scripts/hop2.sh parity4             parity of 4 under every arm, seeds 5-8
scripts/hop2.sh calc                calc held-out: shuffled input, depth, width; seeds 5, 6
scripts/hop2.sh wide                MNIST with wider cells, seeds 5-8
scripts/hop2.sh widesame            the pair test with wider cells, seeds 5-8
scripts/hop2.sh deepsame            the pair test with depth, seeds 5-12
scripts/hop2.sh targetpick          errors from the answer: validation, seeds 1, 2
scripts/hop2.sh target              errors from the answer, MNIST, seeds 5-12
scripts/hop2.sh targetcap           the same, rate held at target, seeds 5-8
scripts/hop2.sh slow                the old errors at the same slow step, MNIST, seeds 5-8
scripts/hop2.sh targetsame          errors from the answer on the pair test, seeds 5-8
scripts/hop2.sh slowsame            the old errors at the same slow step, pair test, seeds 5-8
scripts/hop2.sh same --quick        one seed, short lives: a smoke check of the whole line
```

Here an MNIST-sized pair life is 2 to 3 minutes (the two-button readout re-picks little), a
taught MNIST life 4 to 5 minutes, a toy run under a minute, a calc run one to two minutes; on
stevejobs about 7 minutes a run, two at a time. Some arms ran on stevejobs (`thonk-hop2`:
`same` seeds 11 and 12, `target` seeds 5-8, `targetpick`, `targetcap`, `slow` hop arm); the
outputs were copied into the same folders here.

## What is real, what is suspect

- Real: the pair test's numbers (8 seeds, every arm, matched rates for hop and its two coins);
  the toys frozen against shuffled input (8 seeds, every seed); errors from the answer over
  their coin on MNIST at free rate (8 of 8); depth's +0.007 on the pair test (8 of 8).
- Suspect: the rate-held arms (4 seeds, two settings, one of them the hop doc's); the pair
  test arms on 4 seeds (wide, target, slow, sum); the margin from the answer is +0.003 to
  +0.008 depending on rate and test, a few thousandths, the size the hop doc called churn's
  noise floor for single settings.
- The pair test's "pixel likeness" is not reported: the wiring report computes it from single
  digits.
- The pair test's training stream pairs each digit afresh each pass; a 60,000-pair life is
  one pass, so every pair in the life is new, as on MNIST.
- The toys have no rate while read (the transfer probe does not print one); the hop doc's
  one-life check had the rule's toy lives at their born rate (33.8 against 32.9).

## Not done

- Errors from the answer with depth, or with near-not-fired marks; on the toys and calc; with
  competitive learning; in the online readout's life.
- Depth and width on MNIST beyond the one wide arm; other depth shares or widths (one value
  each, untuned).
- The old errors at shift 9 with the rate held at target (the matched pair of `targetcap`).
- No `blobbench` run: nothing on the tick's path changed (two new checks in `tag_rewire`
  and the hop rule's source mask, each a return of 0 when off; a taught MNIST life from the
  answer runs 4,760 ticks a second against 5,190 for the old rule at the same step, -8%,
  rough, on stevejobs).
- A parity-of-the-sum test where a cell can hear both digits' parity (it needs a product
  14 pixels apart, past a cell's reach).

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, under the one-hop rule's entry: **its failure on real data is not
  MNIST being easy.** On "are these two interleaved digits the same class" (a straight line
  0.51, the born sheet 0.677, a two-layer net 0.80 to 0.85) the rule lifts the sheet to 0.691
  and its coin control to 0.691 (8 seeds). **Errors computed from the answer on every step
  (`hop_teach 3`) do beat their coin**: MNIST 0.9377 against 0.9317 (8 of 8; +0.003 with the
  rate held at target), pairs 0.698 against 0.690 (4 of 4), where the readout's errors at the
  same step do not (+0.0015, +0.0027). Cells that hear only cells let even the readout's
  errors count on pairs (+0.007, 8 of 8). Width does not. Nothing reaches competitive
  learning (pairs 0.726, MNIST 0.952). [likely: two worlds, 8 seeds for the main arms]
- Finding 44 (controls): the coin on a two-line task cancels half its errors; use
  `--g.hop_coin 2` (distinct lines) there. And the toy transfer test left the hop rule and
  competitive learning running during the fresh readout's phase; now frozen by default
  (`--then-inside 1` for the old way). The hop doc's toy numbers move a little up frozen
  (parity of 3 0.816 to 0.847) and stand against a shuffled-input life (0.607).
- New principle candidate: **an error rule's error should be about the input on every step,
  not about the readout's mistakes.** [hint: one rule, two worlds]
- `docs/plan.md`: item 2d done (this doc). Worth a slot: errors from the answer with cells
  that hear only cells (the two things that helped, not yet run together); the same with
  competitive learning on; `hop_teach 3` as the error for idea 162's every-line targets (it
  is that rule's shape: target less a running rate).
- `docs/design.md` and `genome.hpp` (core owner; edited here because the mechanisms needed
  them): `hop_teach 3`, `hop_rate_shift`, `hop_coin 2`, `wire_pixel_shift`,
  `wire_deep_percent`; `deep_blocked` in `tag_store.hpp` (used by `tag_rewire` and the hop
  rule), `more_pixels` in `birth.cpp`. All off by default; four reference lives
  byte-identical.
- `docs/notes-world.md`: the MNIST pair tests (`--pair`, `--pair-mix`, `mnist_partner`,
  `mnist_pair_encode`), `blobmnist baselines --pair --net-passes`, `blobprobe transfer|calc
  --life-scramble 1`, `blobprobe transfer --then-inside 1`, `scripts/hop2.sh`.
- `docs/predictions.md`: the predictions above and their scores.
