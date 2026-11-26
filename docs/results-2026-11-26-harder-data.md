# A harder real-data test with more room (2026-11-26)

Queue item 11c, idea 185. **Backprop is a measuring stick here, never the project's method.**
MNIST leaves 0.041 between the born sheet and the teacher's bound and the pair test 0.108, so
every comparison of rules has needed 8 seeds. This job puts other data sets in MNIST's own
file format through the same sheet and asks which gives the most room, and whether the live
every-event rule (docs/results-2026-11-26-every-event.md) gains there too.

**The answer, as far as the finished runs go (the job was stopped when the project was
tabled): Kuzushiji-MNIST's ten classes give the most room, 0.098 between the born sheet
(0.732) and the teacher's wiring in the real sheet (0.830), against MNIST's 0.042, Fashion-
MNIST's 0.023 and no pair test above MNIST's own 0.11, with a spread small enough that three
seeds show a difference of 3% of the room; and the live every-event rule's gain holds there:
`out` reads 0.776, +0.043 over born and +0.040 over its coin on 3 of 3 seeds, 44% of the
room, with competitive learning above it at 0.792.**

## Short version

Fresh linear reader of the non-sensor cells, test digits, seeds 5-7, the sheet and reader of
`scripts/mnist.sh` with only the data folder changed (`t28`, the same sizes; no set needed
another setting). Rate (hidden spikes per 1,000 cell-ticks while read) in brackets. Lives are
60,000 digits unless marked 20k.

| | MNIST classes | Kuzushiji classes | Fashion classes | Kuzushiji pairs | Fashion pairs |
|---|---|---|---|---|---|
| linear reader of the bits | 0.916 | 0.688 | 0.794 | 0.50 | 0.50 |
| two-layer net on the bits, 1 / 5 passes | 0.955 / 0.969 | 0.818 / 0.862 | 0.808 / 0.837 | 0.634 / 0.678 | 0.791 / 0.820 |
| born | 0.926 [33] | 0.733 [34] | 0.785 [43] | 0.576 [34] | 0.712 [41] |
| teacher's wiring in the sheet | 0.967 [35] | 0.830 [36] | 0.808 [40] | 0.606 [38] | 0.752 [40] |
| **room** (teacher less born read the same way) | 0.042 | **0.098** | 0.023 | 0.031 | 0.043 |
| competitive learning | 0.952 [63] | 0.792 [58] | 0.806 [71] 20k | 0.600 [60] | 0.746 [74] 20k |
| live rule, `out` | 0.951 [43] | **0.776** [40] | 0.794 [57] 20k | 0.574 [33] | 0.728 [39] 20k |
| its coin | 0.930 | 0.735 | not run | 0.581 (2 seeds) | not run |
| live rule, `outf3` | 0.945 [40] | 0.767 [38] | 0.798 [57] 20k | 0.608 (2 seeds) | 0.752 [42] 20k |
| live rule, `outint2` | 0.941 [38] | 0.755 [35] | 0.790 [51] 20k | 0.606 (2 seeds) | 0.745 [40] 20k |

MNIST's rows are the earlier jobs' own run files (the every-event report, the inside
ladder, compete2), copied under this job's names by `out/harderdata/mnist_from_main.sh`;
nothing of MNIST was run again here. MNIST's pair test: born 0.677, teacher 0.780, room
0.106 (seeds 5-7).

- **Which set.** Kuzushiji-MNIST's classes. A straight line reads 0.69 there and a small net
  0.86, the born sheet 0.733, the teacher's wiring 0.830. The room is 2.3 times MNIST's.
  Fashion-MNIST has less room than MNIST (0.023): at half ink it is silhouettes, the born
  sheet reads under a straight line on the bits (0.785 against 0.794) and the teacher adds
  little. Kannada-MNIST's born sheet also reads under the bits' straight line (0.877 against
  0.887); its teacher's wiring was not read before the stop.
- **The spread.** On Kuzushiji's classes a run's spread across seeds is 0.002 and the spread
  of the difference between two runs on the same seeds 0.003, so three seeds show a
  difference of 0.003, 3% of the room. By the same sum MNIST's classes show 4% on these
  three seeds and 7% with the spread taken from 8 seeds; MNIST's pairs 9 to 11%. So
  Kuzushiji's classes are about twice as sharp as MNIST's classes and three times as sharp
  as the pair test, on three seeds' worth of spread. The 8-seed run that would firm this up
  was stopped.
- **Rules that tie on MNIST separate here.** MNIST: competitive learning 0.952, `out` 0.951.
  Kuzushiji: 0.792 against 0.776, 3 of 3 seeds, 16% of the room apart.
- **The live rule is not an MNIST rule.** On Kuzushiji's classes every arm is over born and
  over its coin on 3 of 3 seeds: `out` +0.043 / +0.040, `outf3` +0.035 / +0.036, `outint2`
  +0.023 / +0.019. The coins get +0.002, -0.002 and +0.004 over born. The order of the arms
  is MNIST's. The outside output is right 0.84 of the time in the last tenth of the life
  (MNIST 0.91) and 67% of digits teach at fine 0.1 (MNIST 50%).
- **The pair tests want the wide fine zone, as on MNIST.** Kuzushiji's pairs: `out` (fine
  0.1) is under born and under its coin (-0.002, 1 of 3 seeds; -0.005 against the coin, 0 of
  2). There the outside output is right 0.61 of the time and 98% of digits teach, the
  failing case of the every-event doc. `outf3` and `outint2` gain +0.032 and +0.031 over
  born and +0.025 over their coins (2 seeds each), which is the whole of that test's room
  (0.031). The test itself is poor: half-size Kuzushiji characters are hard for every
  learner (the small net 0.68).
- **Fashion-MNIST, short lives only.** After 20,000 digits the three arms are +0.009,
  +0.012, +0.005 over born on the classes (3 of 3 each) and +0.016, +0.040, +0.033 on pairs,
  where `outf3` already reads at the teacher's wiring (0.752). No coins and no 60,000-digit
  lives were run, so nothing is claimed for Fashion. The rule's sheets fire at 51 to 57 there
  against born 43, and no rate-held check was run.
- **The second real-data test from now on:**
  `scripts/harderdata.sh full --sets kmnist --tests class`

## The tables

`scripts/harderdata.sh full --table-only --sets mnist,kmnist,fashion,kannada`; the whole
print is in `out/harderdata/run/table-full.txt`.

Kuzushiji-MNIST, classes (60,000-digit lives; "outside": the outside output's share right
in the last tenth of the life; "teach": the share of digits that sent an error):

| run | 5 | 6 | 7 | mean | sd | rate | pixels | outside | teach |
|---|---|---|---|---|---|---|---|---|---|
| born | 0.7346 | 0.7301 | 0.7334 | 0.7327 | 0.0023 | 34.3 | 7.1 | | |
| born, read after 3,000 digits of regulation | 0.7362 | 0.7314 | 0.7280 | 0.7319 | 0.0041 | 34.2 | 7.1 | | |
| teacher's wiring | 0.8309 | 0.8280 | 0.8307 | 0.8299 | 0.0016 | 36.4 | 7.7 | | |
| competitive learning | 0.7942 | 0.7899 | 0.7906 | 0.7916 | 0.0023 | 58.1 | 25.2 | | |
| out | 0.7750 | 0.7769 | 0.7746 | 0.7755 | 0.0012 | 39.6 | 7.2 | 0.837 | 0.67 |
| out, coin | 0.7391 | 0.7308 | 0.7354 | 0.7351 | 0.0042 | 36.3 | 5.9 | 0.808 | 0.77 |
| outf3 | 0.7681 | 0.7685 | 0.7656 | 0.7674 | 0.0016 | 38.3 | 6.7 | 0.835 | 0.46 |
| outf3, coin | 0.7319 | 0.7295 | 0.7316 | 0.7310 | 0.0013 | 35.1 | 5.3 | 0.806 | 0.54 |
| outint2 | 0.7557 | 0.7574 | 0.7528 | 0.7553 | 0.0023 | 35.1 | 5.6 | 0.820 | 0.50 |
| outint2, coin | 0.7405 | 0.7346 | 0.7337 | 0.7363 | 0.0037 | 34.2 | 5.4 | 0.793 | 0.55 |

After 20,000 digits the same arms read 0.784 (competitive learning), 0.765, 0.762, 0.748.
In the model of the cells: born 0.816, the teacher 0.894, the unconstrained net 0.912 (one
seed). The model-to-sheet loss is 0.083 born and 0.065 taught, against 0.036 and 0.013 on
MNIST.

Kuzushiji-MNIST, pairs (60,000-digit lives; a dash is a run not finished at the stop):

| run | 5 | 6 | 7 | mean | rate | outside | teach |
|---|---|---|---|---|---|---|---|
| born | 0.5648 | 0.5863 | 0.5762 | 0.5758 | 33.7 | | |
| teacher's wiring | 0.6002 | 0.6164 | 0.6024 | 0.6063 | 37.5 | | |
| competitive learning | 0.5953 | 0.6083 | 0.5953 | 0.5996 | 60.2 | | |
| out | 0.5743 | 0.5779 | 0.5686 | 0.5736 | 32.8 | 0.612 | 0.98 |
| out, coin | 0.5754 | 0.5864 | - | 0.5809 | 35.4 | 0.607 | 1.00 |
| outf3 | 0.6013 | 0.6137 | - | 0.6075 | 36.4 | 0.651 | 0.73 |
| outf3, coin | 0.5778 | 0.5867 | - | 0.5822 | 35.3 | 0.609 | 0.84 |
| outint2 | 0.6048 | 0.6074 | - | 0.6061 | 36.0 | 0.631 | 0.66 |
| outint2, coin | 0.5808 | 0.5804 | - | 0.5806 | 34.8 | 0.591 | 0.70 |

Fashion-MNIST (20,000-digit lives, no coins), classes / pairs:

| run | classes | rate | pairs | rate |
|---|---|---|---|---|
| born | 0.7853 | 42.7 | 0.7119 | 41.0 |
| teacher's wiring | 0.8078 | 39.6 | 0.7518 | 39.8 |
| competitive learning | 0.8064 | 70.5 | 0.7456 | 74.0 |
| out | 0.7941 | 57.2 | 0.7279 | 38.5 |
| outf3 | 0.7976 | 56.6 | 0.7517 | 41.6 |
| outint2 | 0.7904 | 51.2 | 0.7447 | 40.4 |

Kannada-MNIST: born 0.8765 / 0.5970 (rate 31.5 / 31.3); the bits' straight line 0.887; the
net 0.899 / 0.917 (1 / 5 passes) and 0.745 / 0.783 on pairs; in the model born 0.914 and the
teacher 0.941 on the classes. The teacher's wiring was not read in the sheet.

The room (teacher's wiring less the born sheet read the same way; "sd diff": the spread
across seeds of the difference between two runs on the same seed, pooled over born and the
lived arms; "3 seeds show": twice the standard error of such a difference on three seeds,
as a share of the room; "seeds": how many make that a tenth of the room):

| set, test | born | teacher | room (lowest to highest seed) | sd of a run | sd diff | 3 seeds show | seeds |
|---|---|---|---|---|---|---|---|
| MNIST classes, seeds 5-7 | 0.9242 | 0.9667 | 0.0425 (0.040 to 0.044) | 0.0013 | 0.0014 | 0.038 | 2 |
| MNIST classes, seeds 5-12 | 0.9241 | 0.9663 | 0.0422 (0.039 to 0.045) | 0.0017 | 0.0025 | 0.068 | 2 |
| MNIST pairs, seeds 5-7 | 0.6741 | 0.7801 | 0.1060 (0.095 to 0.116) | 0.0093 | 0.0101 | 0.110 | 4 |
| MNIST pairs, seeds 5-12 | 0.6728 | 0.7853 | 0.1125 (0.095 to 0.128) | 0.0086 | 0.0092 | 0.094 | 3 |
| **Kuzushiji classes** | 0.7319 | 0.8299 | **0.0980** (0.095 to 0.103) | 0.0020 | 0.0027 | **0.032** | 2 |
| Kuzushiji pairs | 0.5749 | 0.6063 | 0.0315 (0.025 to 0.040) | 0.0074 | 0.0072 | 0.265 | 22 |
| Fashion classes (20k lives) | 0.7844 | 0.8078 | 0.0234 (0.018 to 0.029) | 0.0019 | 0.0025 | 0.124 | 5 |
| Fashion pairs (20k lives) | 0.7088 | 0.7518 | 0.0430 (0.039 to 0.048) | 0.0034 | 0.0034 | 0.092 | 3 |

## The data

`scripts/get_data.sh NAME` fetches each set from its public home into `data/NAME/`, checks
the md5 torchvision lists and the unpacked sizes. All three are 60,000 training and 10,000
test images, 28 by 28 grey, ten balanced classes, so nothing but the folder changes.

| set | what | license | lines on at half ink (of 784) |
|---|---|---|---|
| MNIST | digits | | 104 |
| Fashion-MNIST (`fashion`) | clothes, Zalando Research | MIT | 247 |
| Kuzushiji-MNIST (`kmnist`) | cursive Japanese characters, CODH | CC BY-SA 4.0 | 151 |
| Kannada-MNIST (`kannada`) | Kannada digits, V. U. Prabhu | CC BY 4.0 by my memory of its paper and listings; the repository's readme has no license line, so check before any use beyond this | 64 |

Kannada-MNIST comes as npz files; `scripts/npz_to_idx.py` writes the four idx files (the npz
files stay in `data/kannada-npz/`). EMNIST letters was not tried (26 classes, more buttons).

## What is suspect

- Three seeds everywhere. The spread figures come from 3 seeds times 5 runs; on MNIST the
  same sum on 8 seeds gives a spread nearly twice the 3-seed one, so "3% of the room" on
  Kuzushiji could be 6%.
- By the seeds-for-a-tenth sum MNIST's classes also need only 2 seeds. What Kuzushiji adds
  is absolute room (0.098 against 0.042) with a spread that grew less (0.003 against 0.0014
  to 0.0025).
- The teacher's bound is found in the model and written in once. On Fashion's pairs a
  20,000-digit life of `outf3` already reads at it, so it is not a bound there. On
  Kuzushiji's classes the best lived arm is 0.038 under it.
- The teacher's model ran on this PC for Kuzushiji's classes and on stevejobs for the rest;
  the two machines' float results were not compared.
- MNIST's rows are copied files from earlier jobs, not re-run through this harness. The
  check that this harness gives the same MNIST numbers was planned and not run.
- Fashion-MNIST at half ink loses its greys (a straight line reads 0.841 on grey pixels and
  0.777 on the bits). The `g14` code might suit it better; not tried, the brief keeps `t28`.

## Not finished at tabling

- 60,000-digit lives and coins on Fashion-MNIST (both tests): not started.
- Kuzushiji pairs, seed 7 of the coins, `outf3` and `outint2`: stopped mid-run.
- Kuzushiji classes on seeds 8-12 (born, teacher, competitive learning, the rule): stopped
  before any run finished.
- Kannada-MNIST: the teacher's wiring in the sheet, and every lived arm.
- MNIST's pair test under competitive learning with no reward, seeds 5-7.
- The quiet way (`--read-freeze 3 --ticks 80`) on the chosen set: the recipe exists
  (`scripts/harderdata.sh quiet --sets kmnist`) and ran in the smoke run only. Lived arms
  cannot be read that way at all without a flag that saves a lived wiring.
- A rate-held check on any new set; a fine-zone sweep on Kuzushiji; shuffled-digit lives.
- The MNIST re-run through this harness (`HARDERDATA_RUN=check scripts/harderdata.sh full
  --sets mnist --tests class --seeds 5 --parts born,rule --arms out --coins 0`).

## Script lines

```
scripts/get_data.sh all                                   fashion, kmnist, kannada into data/
scripts/harderdata.sh full --sets kmnist --tests class    the second real-data test
scripts/harderdata.sh room --sets kmnist,fashion,kannada  born, plain learners, the teacher's bound
scripts/harderdata.sh screen --sets kmnist,fashion --parts compete,rule    20,000-digit lives
scripts/harderdata.sh full --sets kmnist,fashion --parts compete,rule      60,000 with coins
scripts/harderdata.sh quiet --sets kmnist                 the room read the quiet way
scripts/harderdata.sh full --table-only --sets mnist,kmnist,fashion,kannada
scripts/harderdata.sh smoke --quick                       every part, short, Fashion-MNIST
out/harderdata/mnist_from_main.sh                         MNIST's rows from earlier jobs' files
out/harderdata/refcheck.sh                                the four reference lives against main's
```

The teacher's model is about four times slower on this PC than on stevejobs (90 s a pass
against 25); the engine's lives are the other way round.

## Code

- `src/mnist/mnist_main.cpp`: `--data DIR` for any `blobmnist` command (sets `THONK_MNIST`
  and takes the flag out before the command sees it). No flag given: nothing changes.
- `scripts/get_data.sh`, `scripts/npz_to_idx.py`, `scripts/harderdata.sh`,
  `scripts/harderdata.py`, `scripts/harderdata_table.py` (reuse `everyevent.py`, `inside.py`,
  `ladder2.py`). The python model reads `THONK_MNIST`, as before.
- Checks: a build with no warnings; the four reference lives byte-identical
  (`out/harderdata/refcheck.sh`, run after the last code change); `scripts/harderdata.sh
  smoke --quick` end to end.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, new: **the live every-event rule is not an MNIST rule.** On
  Kuzushiji-MNIST's classes `out` reads 0.776 against born 0.733 and its coin 0.735 (3 of 3
  seeds; the teacher's wiring 0.830, competitive learning 0.792), with an outside output
  right 0.84 of the time. On pair tests it needs the wide fine zone on both sets. [likely: 3
  seeds, one new set with coins; Fashion-MNIST short lives only]
- `docs/findings.md`, new: **more room is not the same as an easier measurement.**
  Kuzushiji's classes have 2.3 times MNIST's room and about twice MNIST's sharpness (three
  seeds show 3% of the room against 4 to 7%); Fashion-MNIST at half ink has less room than
  MNIST. [hint: 3 seeds]
- `docs/ideas.md`: idea 185 stopped at tabling with the above. New: a code with greys for
  Fashion-MNIST; why the born sheet reads under the bits' own straight line on Fashion and
  Kannada; why competitive learning beats the rule on Kuzushiji and ties it on MNIST.
- `docs/plan.md`: item 11c stopped at tabling; the unfinished list above is what remains.
- `docs/task-map.md`, tier 11: Kuzushiji-MNIST classes, born 0.733, competitive learning
  0.792, the live rule 0.776, the teacher's wiring 0.830, a small net 0.86
  (`scripts/harderdata.sh full --sets kmnist --tests class`).
- `docs/predictions.md`: the predictions below and their scores.

## Predictions, scored

| prediction | outcome |
|---|---|
| Kuzushiji classes have the most room: 0.6 | yes (0.098) |
| a pair test has more room than every class test: 0.25 | no among the new sets (0.031, 0.043); MNIST's own pairs still have 0.106 |
| Kuzushiji's class room at least twice MNIST's: 0.65 | yes (2.3 times) |
| Kuzushiji born 0.780 / 0.600, teacher 0.890 / 0.680 | 0.733 / 0.576, 0.830 / 0.606: both lower, the pair room far smaller than guessed |
| Fashion born 0.800 / 0.680, teacher 0.835 / 0.770, room 0.035 / 0.090 | 0.785 / 0.712, 0.808 / 0.752, 0.023 / 0.043 |
| Kannada born 0.900 / 0.640 | 0.877 / 0.597; teacher not read |
| rates born: Fashion 44, Kuzushiji 37, Kannada 31 | 43, 34, 31.5 |
| the rule's best arm at least 0.02 over born on Kuzushiji classes: 0.7; over its coin 3 of 3: 0.75 | yes (+0.043; 3 of 3) |
| it closes 0.45 of the room | 0.44 |
| competitive learning 0.840, the best live arm 0.830 on Kuzushiji classes | 0.792, 0.776 |
| competitive learning above the rule's best arm on Kuzushiji classes: 0.5 | yes |
| the outside output right 0.86 on Kuzushiji, 0.80 on Fashion | 0.84; Fashion 0.75 at 20,000 digits, 60,000 not run |
| Fashion classes: best arm at least 0.01 over born: 0.5; `out` under born: 0.3; `outf3` at or above `out`: 0.6 | at 20,000 digits +0.012, `out` over born, `outf3` above `out`; not run at 60,000 |
| `outf3` the best arm on every set's pairs: 0.6 | Kuzushiji yes (2 seeds, level with `outint2`); Fashion yes at 20,000 |
| seeds for a tenth of the room: MNIST 3 / 3, Fashion 4 / 4, Kuzushiji 2 / 4 | 2 / 4, 5 / 3 (short lives), 2 / 22 |
| the quiet way shrinks the room by a quarter | not run |

I had the pair tests as the place the new sets would give room and they give the least: at
half size at half ink the new sets' pairs are hard for the small net too.

## Predictions (written and committed before any scoring run was read)

Seen before writing these: the docs named in the brief with their MNIST numbers (born 0.925
/ 0.677, the teacher's wiring 0.966 / 0.785, competitive learning 0.952 / 0.726, the live
rule 0.951 / 0.770), the code, and three things run here that are not scoring runs:

- The sets' own statistics: lines on at half ink per image (the sheet's sensor code `t28`):
  MNIST 104 of 784, Fashion-MNIST 247 (5% of images under 38, 5% over 463), Kuzushiji-MNIST
  151, Kannada-MNIST 64. Fashion-MNIST has 22% of its pixels between a quarter and three
  quarters ink (MNIST 5%, Kuzushiji 9%), so the half-ink code throws away more of it.
- `scripts/harderdata.sh smoke --quick` on Fashion-MNIST, seed 5: 3,000-digit lives, the
  reader fit on 5,000 digits, the teacher one pass on 10,000 digits, 2,000 test digits.
  Its sheet scores say nothing at that size (every arm 0.755 to 0.772 on the classes, 0.67
  to 0.71 on pairs). What it showed: every part runs; the born sheet fires at 45 spikes per
  1,000 cell-ticks on Fashion-MNIST (MNIST 33); the outside output is right 0.68 of the
  time after 3,000 digits and 99% of digits teach at fine 0.1.
- The plain learners on Fashion-MNIST, which that smoke run trains in full (60,000 digits):
  linear on the bits 0.777, on grey pixels 0.841; the two-layer net on the bits 0.808 (one
  pass), 0.837 (five); random projection 0.788. Pairs: linear 0.49, the net 0.791 / 0.820,
  random projection 0.706.

Nothing of Kuzushiji-MNIST or Kannada-MNIST has been run.

Guesses, test digits, seeds 5-7, the classes / the pair test:

| set | born | teacher's wiring in the sheet | room | competitive learning | best live arm |
|---|---|---|---|---|---|
| MNIST (known) | 0.925 / 0.677 | 0.966 / 0.785 | 0.041 / 0.108 | 0.952 / 0.726 | 0.951 / 0.770 |
| Fashion-MNIST | 0.800 / 0.680 | 0.835 / 0.770 | 0.035 / 0.090 | 0.815 / 0.715 | 0.810 / 0.740 |
| Kuzushiji-MNIST | 0.780 / 0.600 | 0.890 / 0.680 | 0.110 / 0.080 | 0.840 / 0.630 | 0.830 / 0.650 |
| Kannada-MNIST | 0.900 / 0.640 | 0.950 / 0.740 | 0.050 / 0.100 | | |

- **Which has the most room: Kuzushiji-MNIST's classes** (0.6). A pair test of some set
  has more room than every class test: 0.25. Fashion-MNIST's classes: 0.1. Kuzushiji's
  class room is at least twice MNIST's (0.082 or more): 0.65.
- Why: a straight line does badly on Kuzushiji (I guess 0.66 on the bits) and a small net
  well (0.85 after five passes), so there is something for features to add. Fashion-MNIST's
  errors are classes that look alike at half ink (shirt, T-shirt, pullover, coat); the net
  adds only 0.06 over the line there and the born sheet, a random nonlinear layer, already
  has some of it.
- Seeds for a tenth of the room (twice the standard error of the difference between two
  arms on the same seeds): MNIST 3 / 3, Fashion-MNIST 4 / 4, Kuzushiji-MNIST 2 / 4. The
  spread across seeds grows as the score falls from 0.95 toward 0.8 (I guess 0.004 on
  Kuzushiji's classes against 0.002 on MNIST), so the room has to grow faster than that.
- The firing rate while read, born: Fashion-MNIST 44, Kuzushiji 37, Kannada 31 (MNIST 33).
- **The live rule's gain holds on Kuzushiji's classes** (the best of `out`, `outf3`,
  `outint2` at least 0.02 over born): 0.7; that arm over its coin on 3 of 3 seeds: 0.75.
  It closes less of the room than on MNIST (0.63 there): I guess 0.45.
- **On Fashion-MNIST's classes the gain is small or missing**: the best arm at least 0.01
  over born: 0.5; `out` (fine 0.1) under born: 0.3. The rule needs an output that is right
  about nine times in ten and Fashion's outside output will be right about 0.80, so at fine
  0.1 nearly every digit teaches, which on MNIST was the failing case ("the answer on every
  digit: born"). `outf3` at or above `out` on Fashion's classes: 0.6.
- The outside output's share right in the last tenth of a 60,000-digit life: Fashion 0.80,
  Kuzushiji 0.86 on its training digits (its test digits are harder than its training
  digits, so the fresh reader on test will sit under that).
- On the pair tests `outf3` is the best arm on every set (0.6), as on MNIST.
- Competitive learning gains on every set's classes (0.8) and more than the live rule's
  best arm on Kuzushiji's classes (0.5).
- The quiet way (`--read-freeze 3 --ticks 80`) lifts born by more than it lifts the
  teacher's wiring on the chosen set, so the room shrinks by a quarter or more: 0.6.
