# A harder real-data test with more room (2026-11-26)

Queue item 11c, idea 185. **Backprop is a measuring stick here, never the project's method.**
MNIST leaves 0.041 between the born sheet and the teacher's bound and the pair test 0.108, so
every comparison of rules has needed 8 seeds. This job puts other data sets in MNIST's own
file format through the same sheet and asks which gives the most room, and whether the live
every-event rule (docs/results-2026-11-26-every-event.md) gains there too.

(The answer, short version and tables are written below the predictions once the runs are
in.)

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
