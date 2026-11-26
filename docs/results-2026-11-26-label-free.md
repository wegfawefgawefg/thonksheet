# Label-free rule families, tried in the model before the engine (2026-11-26)

Queue item 4b, idea 173. **Backprop appears here only as a measuring stick**: as the labelled
teacher's number (idea 165) and as each family's upper bound (its objective optimised by
backprop under the cells' constraints, no labels). Every candidate is also run as a local
step, with no gradient through other cells, and it is the local form that counts.

The model is idea 165's bottom rung (7b): the real sheet's 3,072 non-sensor cells, their
reach, fixed signs, 1 and 4 at the born counts, the engine's cell and regulation
(`scripts/ladder2_model.py`, imported). A rule works on the wiring the way the engine does: a
counter per candidate input per cell (6 bits, starting at the born strength), integer steps,
the cell wired to its top counters at its born counts. Only hidden excitatory cells learn and
only their excitatory sources move (the engine's competitive learners). Online, one pass, read
at 20,000 and 60,000 digits.

## Predictions (written before any scoring run was read)

Seen before writing these: the numbers in the brief and in idea 165's doc; development runs
only, seed 5: a crash test of every family and control on 320-digit lives with a 3,000-digit
fit (readers all within 0.68 to 0.69 on pairs; meaningless at that size), and one 2,000-digit
competitive life with a 10,000-digit fit (0.911 against born 0.929 on the same small fit, rate
55 against 33); and the first `scripts/ladder4.sh smoke --quick` (10,000-digit fit), where only
the born arm and the two-looks bound (one pass on 10,000 digits) finished: born 0.929 / 0.723,
bound 0.928 / 0.733, born in the real sheet 0.900 / 0.642 (500-digit settle). No 20,000 or
60,000-digit life had been read.

Guesses, model, 60,000-digit life, seeds 5-7, fresh reader (MNIST / pairs; model born about
0.961 / 0.768):

| family | guess | over its shuffled life |
|---|---|---|
| 0 competitive learning | 0.962 / 0.780 | +0.003 / +0.010 |
| 1 is this real (ungated; gated the same within 0.003) | 0.961 / 0.770 | +0.001 / +0.003 |
| 2 predict a pixel outside | 0.958 / 0.760 | 0.000 / 0.000 |
| 3 two looks agree | 0.963 / 0.780, moved 1 pixel +0.010 over born (the best of all on moved digits) | +0.004 / +0.010 |
| 4 push apart (with 0) | family 0 within 0.003 | |
| 5 sliding threshold (BCM) | 0.962 / 0.775 | +0.002 / +0.007 |

- Calibration: the model's competitive learning lifts the model's born by less than the engine
  lifts the real sheet (+0.026 / +0.049), because the model's born is already 0.961 / 0.768
  and the model has no time: I guess +0.002 / +0.012, and that most of it is there on the
  shuffled life too: probability 0.6. That the model's version needs a change before it
  behaves like the engine's (rate up by a half or more, pixels inputs up from 7 to 15 or more,
  likeness up from 0.05 to 0.2 or more): probability 0.35 that it does NOT already.
- Upper bounds (backprop on the objective, no labels): real against made-up 0.963 / 0.780;
  predict a pixel 0.958 / 0.750; two looks (InfoNCE) 0.970 / 0.800, the best; push apart
  (autoencoder plus decorrelation) 0.961 / 0.750; BCM 0.960 / 0.760.
- Some family beats competitive learning in the real sheet on both tests (above 0.952 and
  0.726 written in with `--wiring-load`): probability 0.15. Family 0's own transplant reads
  under the engine's lived sheet (0.952 / 0.726) because it runs at the engine's rate, not
  twice it: 0.945 / 0.710, probability 0.65 that it is under on both.
- Some family beats its shuffled life by 0.005 or more on MNIST in the model: probability 0.3;
  on pairs: 0.4.

## Short version

**Of the label-free families, family 1, "is this real" (competitive learning on real digits,
the same step backwards on made-up ones), reads best: 0.954 / 0.767 in the model and 0.952 /
0.738 written into the real sheet (competitive learning 0.952 / 0.726, the labelled teacher
0.966 / 0.785, born 0.924 / 0.673); over its shuffled-digit control that is +0.022 / +0.060.**
(8 seeds, 5-12, lived and read with the tile's winners as the engine's best competitive
sheet is; every lived wiring written in with `blobmnist ceiling --wiring-load`, settle 3,000,
the tree's reader. Its shuffled control is a weak one, see below.)

- **On MNIST nothing label-free beats competitive learning; on pairs family 1 does, by about
  0.01.** Same settings, same seeds (8), seed by seed: family 1 minus the model's competitive
  learning +0.001 (5 of 8) on MNIST and **+0.011 (8 of 8, +0.005 to +0.018) on pairs**;
  family 1 minus the engine's own lived competitive sheet (0.952 / 0.728 here) -0.0005 (3 of
  8) and +0.010 (6 of 8). The teacher's 0.966 / 0.785 stays out of reach: competitive
  learning closes about two thirds of the MNIST gap between born and the teacher and half
  of the pairs gap; family 1 adds a tenth of the pairs gap and nothing on MNIST.
- **The model's reader cannot judge these rules; the real sheet can, and the model is a good
  simulator of the rule.** Idea 165's model reads every lived wiring under born (competitive
  learning 0.950 / 0.758 against born 0.960 / 0.770 in the model) and orders born, churn,
  shuffled and lived the other way round from the real sheet. Noise in the model's cells does
  not fix that, even at the real sheet's 0.6 self-agreement. But the wirings the model's rule
  makes read in the real sheet like the engine's own: the model's competitive learning with
  the tile's winners, written in and read with them, gives 0.950 / 0.727 against the
  engine's lived sheet 0.952 / 0.728, and its shuffled life 0.939 / 0.704 against 0.939 /
  0.705 (8 seeds). So every arm was written into the real sheet (a minute each), and that is the
  number used; the model's reader is kept in a table of its own.
- **Families, written into the real sheet as lived (no tile winners), 3 seeds, MNIST /
  pairs:** is this real 0.942 / 0.730, sliding threshold 0.943 / 0.720, push apart 0.942 /
  0.721, two looks 0.940 / 0.717, competitive learning 0.939 / 0.717, predict a pixel 0.932 /
  0.685. Churn reads born or under for every family (0.922 to 0.926 / 0.665 to 0.670).
- **The sliding threshold (BCM, family 5) is the cleanest margin over its shuffled life**
  because it is the one rule that does not raise the rate: written in, its sheet fires at 47
  and its shuffled life at 46 (born 33; competitive learning 81 against 46). At that matched
  rate lived beats shuffled by +0.0035 on MNIST and +0.012 on pairs, 8 of 8 seeds each
  (competitive learning at matched rate in the engine: +0.003 on MNIST, the compete2 doc).
  It is also the best on moved digits among the lived rules (moved 1 pixel 0.901, moved 2
  0.716; competitive learning 0.894 / 0.700, born 0.870 / 0.657).
- **Family 1's lead over its shuffled life is mostly a weak control.** On shuffled digits its
  made-up digits look like the real ones, so the step back cancels the step forward, the
  cells keep only 6 pixel inputs and the control reads near born (0.930 / 0.679 with the
  tile's winners). The fair comparison is competitive learning lived the same way: +0.001 /
  +0.011 with the tile's winners (8 seeds), +0.003 / +0.013 without (3 seeds).
  Its coin control (the "made-up" digit is another real digit, so the broadcast bit says
  nothing) moves no wiring: the steps cancel. So what family 1 adds is "away from what
  made-up digits have", and on pairs that is worth about +0.01.
- **Upper bounds (each family's objective trained by backprop, no labels, written into the
  real sheet):** two looks (InfoNCE) 0.946 / 0.720, autoencoder 0.947 / 0.685, autoencoder
  plus decorrelation 0.947 / 0.692, predict a pixel 0.944 / 0.697, real against made-up
  0.917 / 0.695, BCM's objective falls apart (0.806 / 0.573). All at rates 36 to 44. None
  passes competitive learning; the best objective is about what the best local rule reads.
  So for these families the objective is the limit, not the local step: the local rules
  reach their bounds or pass them (family 1's local form reads 0.942 / 0.730 where its
  objective trained by backprop reads 0.917 / 0.695).
- **Labels still buy what nothing here does.** The teacher's wiring reads 0.966 / 0.785 at
  the engine's rate.

## The table: written into the real sheet

Every lived wiring (60,000 digits, one pass) written into the real born sheet with
`blobmnist ceiling --wiring-load`, 3,000 digits of regulation with learning off, then the
inside-learning test's reader (fit 60,000, test 10,000); MNIST moved 1 and 2 pixels with
`--shift 1`. Mean (lowest-highest). "rate": hidden spikes per 1,000 cell-ticks while read.
"kept", "pixels", "spread": from the model's wiring (learners; MNIST life; pairs alike within
0.02 and 1.5 pixels). Lived minus its controls is seed by seed.

MNIST (born 0.924, moved 0.870 / 0.657, rate 33; 8 seeds):

| family, as lived (no tile winners) | seeds | lived | moved 1 / 2 | rate | churn | shuffled (rate) | lived - shuffled | kept, pixels, spread |
|---|---|---|---|---|---|---|---|---|
| 0 competitive learning | 3 | 0.939 (0.936-0.941) | 0.894 / 0.700 | 81 | 0.925 | 0.937 (46) | +0.003, 2/3 | 0.10, 13.3, 2.1 |
| 1 is this real | 8 | **0.942 (0.938-0.944)** | 0.896 / 0.702 | 81 | 0.922 | 0.929 (34) | +0.013, 8/8 | 0.09, 17.2, 2.4 |
| 2 predict a pixel outside | 3 | 0.932 (0.930-0.936) | 0.879 / 0.670 | 38 | 0.923 | 0.915 (32) | +0.017, 3/3 | 0.23, 9.9, 3.0 |
| 3 two looks agree | 3 | 0.940 (0.939-0.941) | 0.895 / 0.701 | 82 | 0.923 | 0.938 (47) | +0.002, 3/3 | 0.09, 14.1, 2.1 |
| 4 push apart (with 0) | 3 | 0.942 (0.939-0.944) | 0.897 / 0.702 | 81 | 0.923 | 0.939 (46) | +0.003, 2/3 | 0.10, 13.9, 2.1 |
| 5 sliding threshold (BCM) | 8 | **0.942 (0.937-0.943)** | **0.901 / 0.716** | **47** | 0.924 | 0.938 (46) | +0.0035, 8/8 | 0.08, 17.1, 2.6 |
| 1, coin control | 3 | 0.932 | 0.877 / 0.670 | 35 | | | | 1.00, 7.4, 4.1 |

Pairs (born 0.673, rate 33; 8 seeds):

| family, as lived | seeds | lived | rate | churn | shuffled (rate) | lived - shuffled |
|---|---|---|---|---|---|---|
| 0 competitive learning | 3 | 0.717 (0.714-0.720) | 80 | 0.667 | 0.703 (45) | +0.014, 3/3 |
| 1 is this real | 8 | **0.726 (0.716-0.733)** | 81 | 0.664 | 0.674 (34) | +0.051, 8/8 |
| 2 predict a pixel outside | 3 | 0.685 (0.683-0.686) | 36 | 0.669 | 0.655 (32) | +0.030, 3/3 |
| 3 two looks agree | 3 | 0.717 (0.715-0.718) | 82 | 0.665 | 0.703 (45) | +0.014, 3/3 |
| 4 push apart (with 0) | 3 | 0.721 (0.715-0.728) | 80 | 0.670 | 0.709 (44) | +0.012, 3/3 |
| 5 sliding threshold (BCM) | 8 | 0.713 (0.699-0.722) | 44 | 0.665 | 0.702 (44) | +0.012, 8/8 |
| 1, coin control | 3 | 0.686 | 34 | | | |

With the engine's best settings: the tile's winners in the life (at most 4 spikes a tick in a
tile, in the model as at most 80 a digit; arm `.cap4`) and in the read (`--g.tile_winners 4`),
as the engine's competitive sheet (B5) is lived and read:

| | seeds | MNIST | moved 1 / 2 | rate | pairs | rate | MNIST / pairs shuffled life | lived - shuffled |
|---|---|---|---|---|---|---|---|---|
| born, read with the tile's winners | 8 | 0.926 (0.920-0.929) | 0.871 / 0.659 | 33 | 0.677 (0.670-0.682) | 32 | | |
| the engine's competitive learning (B5), lived in the sheet | 8 | 0.952 (0.950-0.954) | 0.914 / 0.737 | 64 | 0.728 (0.722-0.737) | 66 | 0.939 / 0.705 | +0.013 / +0.023, 8/8 |
| model's competitive learning (family 0) | 8 | 0.950 (0.947-0.955) | 0.911 / 0.730 | 64 | 0.727 (0.719-0.734) | 66 | 0.939 / 0.704 | +0.012 / +0.023, 8/8 |
| model's is this real (family 1) | 8 | **0.952 (0.950-0.953)** | 0.911 / 0.730 | 63 | **0.738 (0.731-0.746)** | 65 | 0.930 / 0.679 | +0.022 / +0.060, 8/8 |
| model's sliding threshold (family 5) | 3 | 0.947 (0.945-0.949) | 0.908 / 0.727 | 44 | 0.723 (0.719-0.727) | 43 | 0.938 / 0.703 | +0.008 / +0.020, 3/3 |

Seed by seed, 8 seeds, with the tile's winners (MNIST / pairs; seeds above; range):

| difference | MNIST | pairs |
|---|---|---|
| family 1 - family 0 | +0.0011, 5/8 (-0.002 to +0.003) | **+0.0112, 8/8 (+0.005 to +0.018)** |
| family 1 - the engine's competitive sheet | -0.0005, 3/8 (-0.002 to +0.001) | +0.0102, 6/8 (-0.004 to +0.022) |
| family 0 - the engine's competitive sheet | -0.0016, 2/8 (-0.006 to +0.003) | -0.0010, 4/8 (-0.017 to +0.010) |
| family 1 - born (plain read) | +0.027, 8/8 | +0.065, 8/8 |

The bounds, written in the same way (3 seeds, no tile winners; the objective trained by
backprop on the model, no labels, 6 passes):

| objective (family) | MNIST in the model | MNIST, real sheet | moved 1 / 2 | rate | pairs in the model | pairs, real sheet | rate |
|---|---|---|---|---|---|---|---|
| autoencoder (0) | 0.961 | 0.947 | 0.904 / 0.714 | 42 | 0.746 | 0.685 | 42 |
| real against made-up (1) | 0.943 | 0.917 | 0.854 / 0.645 | 36 | 0.756 | 0.695 | 37 |
| predict a pixel (2) | 0.955 | 0.944 | 0.902 / 0.708 | 44 | 0.749 | 0.697 | 39 |
| two looks, InfoNCE (3) | 0.955 | 0.946 | **0.911 / 0.734** | 39 | 0.769 | **0.720** | 38 |
| autoencoder and decorrelation (4) | 0.964 | 0.947 | 0.904 / 0.714 | 39 | 0.758 | 0.692 | 39 |
| BCM's objective (5) | 0.782 | 0.806 | 0.728 / 0.517 | 33 | 0.649 | 0.572 | 33 |

Engine's competitive learning without tile winners (A, 3 seeds): 0.945 / 0.723 at 74 and 75;
its shuffled life 0.938 / 0.704.

Read across:

- **The rate is half the story, again.** Families 0, 1, 3 and 4 all double the rate written
  in (80 against 33, the thresholds at their cap) and read alike on MNIST (0.939 to 0.942).
  Their shuffled lives, at 46, get within 0.003 of them, except family 1's. The tile's
  winners bring the rate to the engine's 63 and lift families 0 and 1 by 0.01 on MNIST
  (family 5 by 0.005, at 44).
- **Family 5 gets the same MNIST at half the rate.** 0.942 at 47 against competitive
  learning's 0.939 at 81; on moved digits it is the best lived rule. On pairs it is 0.004
  under competitive learning.
- **Family 1 is the only one that pays on pairs:** +0.013 over competitive learning as lived
  (3 seeds), +0.011 with the tile's winners (8 of 8 seeds). On MNIST it is competitive
  learning.
- **Predicting a pixel (family 2) and two looks (family 3) add nothing local.** Family 2 keeps
  the rate but its cells take few pixels (10) and stay loose (spread 3.0); family 3 is family
  0 by every number (its second look moved one pixel changes which sources are on very
  little). Push apart (4) is family 0 within the spread.
- **The objectives are no better than the local rules.** The best bound, two looks by
  InfoNCE, reads 0.946 / 0.720 at a rate of 38, about family 0's numbers, and is the best of
  everything on moved digits; the autoencoders 0.947 / 0.685 to 0.692. Real against made-up
  by backprop (tile goodness, Forward-Forward's form) is worse than its local form (0.917 /
  0.695 against 0.942 / 0.726): the trained cells learn a few strong detectors of what is
  wrong with made-up digits (5 pixel inputs a cell). BCM's objective as a loss runs away
  (a few cells saturate) and is no bound at all; its local form with a count threshold is
  fine. So for these families "the local step fails to reach its objective" is not what
  happens: the objectives themselves are weak.

## The model's own reads (not trusted; see calibration)

Fresh linear reader of the model's 3,072 cells after the life (60,000 digits; 20,000 within
0.003 everywhere), MNIST / pairs. Model born 0.960 / 0.770 (8 seeds).

| family | seeds | lived | moved 1 | rate | churn | shuffled | lived - shuffled |
|---|---|---|---|---|---|---|---|
| 0 | 3 | 0.950 / 0.758 | 0.912 | 52 | 0.960 / 0.762 | 0.955 / 0.761 | -0.005 / -0.003 |
| 0, tile cap | 8 | 0.955 / 0.763 | 0.922 | 52 | | 0.955 / 0.761 | 0.000 / +0.003 |
| 1 | 8 | 0.951 / 0.764 | 0.915 | 54 | 0.961 / 0.766 | 0.960 / 0.765 | -0.009 / 0.000 |
| 1, tile cap | 8 | 0.954 / 0.767 | 0.921 | 53 | | 0.960 / 0.767 | -0.006 / -0.001 |
| 2 | 3 | 0.958 / 0.761 | 0.929 | 33 | 0.961 / 0.764 | 0.959 / 0.761 | -0.001 / -0.001 |
| 3 | 3 | 0.950 / 0.761 | 0.912 | 53 | 0.960 / 0.764 | 0.955 / 0.756 | -0.005 / +0.005 |
| 4 | 3 | 0.952 / 0.759 | 0.913 | 52 | 0.960 / 0.764 | 0.955 / 0.760 | -0.003 / -0.001 |
| 5 | 8 | 0.954 / 0.761 | 0.924 | 34 | 0.961 / 0.766 | 0.955 / 0.761 | -0.001 / 0.000 |
| 1, coin control | 3 | 0.962 / 0.769 | 0.935 | 32 | | | |

Nothing that moved its wiring reads above the model's born. Every family with structure
(pixel likeness 0.17 to 0.31 against born 0.03 to 0.05) reads under its own churn, in the
model only.

## Calibration: family 0 in the model against the engine

**The rule.** The model's competitive learning makes what the engine's makes (MNIST, seeds 5-7):

| | pixels | likeness | spread | born kept | rate, model | rate written in / lived |
|---|---|---|---|---|---|---|
| engine, compete A (no tile winners) | 15.4 | 0.25 | 2.55 | 0.24 | | 74 |
| engine, compete B5 (tile winners) | 17.6 | 0.25 | 2.54 | 0.24 | | 63 |
| engine, shuffled life | 26.2 | 0.05 | 4.14 | 0.25 | | 46 |
| model, family 0 | 13.3 | 0.31 | 2.06 | 0.10 | 52 | 81 |
| model, family 0 with the tile cap | 13.8 | 0.30 | 2.15 | 0.10 | 52 | 79 (64 read with tile winners) |
| model, family 0, shuffled life | 27.8 | 0.05 | 4.16 | 0.12 | 33 | 46 |

Half the inputs become pixels that are on together and close together, the rate climbs as
the thresholds hit their cap, the shuffled life fills the cells with unrelated pixels. The
model's rule goes further (born inputs kept 0.10 against 0.24; the shape is there after
1,000 digits) and a little tighter. Written into the real sheet and read as the engine's
lived sheet is read, it gives the engine's numbers: 0.950 / 0.727 against 0.952 / 0.728,
shuffled 0.939 / 0.704 against 0.939 / 0.705 (8 seeds; seed by seed -0.002 and -0.001, 2 and
4 of 8 above). Lived over born in the real sheet: +0.028 / +0.055 for the engine, +0.026 /
+0.054 for the model's rule; about half of it there on the shuffled life too (+0.015 /
+0.03 both), as the brief said of the engine.

**The reader.** The model's own reader does not behave like the real sheet's. It reads family
0's lived wiring under born (0.950 against 0.960; moved 1 pixel 0.912 against 0.935), its
shuffled life in between, its churn at born; the real sheet reads lived 0.939, shuffled
0.937, churn 0.925, born 0.924. Upside down, on both tests, for every family with structure.

Noise in the model's cells does not explain it (`ladder4.py noise`, seed 5, 2,000-digit lives,
10,000-digit fit):

| noise in the model's cells | a cell against a second draw | born | family 0 lived | shuffled | churn |
|---|---|---|---|---|---|
| none | 1 | 0.931 | 0.912 | 0.920 | 0.928 |
| on the drive, 0.25 of the threshold | 0.66-0.74 | 0.916 | 0.898 | 0.908 | 0.912 |
| on the drive, 0.5 | 0.41-0.49 | 0.897 | 0.875 | 0.884 | 0.890 |
| every count a Poisson draw | 0.60 (real sheet 0.61) | 0.915 | 0.895 | 0.906 | 0.911 |
| (real sheet, same kind of wirings, smoke) | | 0.900 | 0.913 | 0.914 | 0.898 |

With the born cells exactly as unreliable as the real sheet's, the model still puts the lived
wiring last. What the real sheet rewards in a pixel-heavy cell is something of its time:
pixel lines are on all 20 ticks; cell inputs arrive as spikes through delays of 1 to 4 ticks
and inhibition at 5 times. The model's cell (constant drive, two settle steps) has none of
it. I did not find the piece.

**What I changed:** not the model. Its reader is reported but not used to choose; every arm
and control was written into the real sheet, which costs about a minute a wiring, and the
real sheet's reader decides. The model is the rule's simulator, and family 0 says it is a
faithful one. The brief's "write only the best into the real sheet" became "write them all".

## The best rule as a cell would do it in bits

Family 1, "is this real", is competitive learning plus one broadcast bit and one more step:

- State: nothing new per cell or per tile. The engine's tag store (a 6-bit counter per
  candidate source, 704 a cell) and its read-off (top counters at the born counts), as
  `learn_compete.cpp` has them. One bit for the whole sheet per digit: real or made up.
- Per spike of a hidden excitatory cell, with chance 2^-5: if the digit is real, +1 for every
  excitatory source its slots read active on that spike, and -1 with chance 1/4 for each
  silent source whose counter is above zero (the engine's step); if the digit is made up,
  -1 for every source read active. Re-pick when a bound is crossed (the engine's cheap
  read-off).
- Per digit: nothing.
- The world's side: it shows a made-up digit after each real one (a third each: the pixels
  shuffled within the digit, the top half of this digit on the bottom half of another, this
  digit's halves swapped) and sets the bit. That doubles the digits shown, and is the real
  cost: in the sheet's world a made-up input has to come from somewhere (a later version
  could make it from the sheet itself: its own replay, or its input scrambled by a fixed
  wiring).
- With the tile's winners in the life and the read, as the engine's best competitive sheet.

In the engine this is about 10 lines in `learn_compete.cpp` (the sign of the up-step and no
down-step when the bit is clear) and a flag in the MNIST world; not built here.

Family 5, the runner-up (same MNIST at half the rate): per cell one more small counter, a
running mean of its count squared (moved by chance toward count^2 each digit), and its count
for the digit (3 bits). At the digit's end: count above the mean, the competitive step for
the sources active during the digit; fired but not above, -1 for them. Needs "active during
the digit" per source (a 20-tick OR of the slot reads, one word a slot) and a step at the
digit's end, which the engine does not have yet.

## The families as they were run

All on the model's counters: 6-bit, starting at the born strength, integer steps rounded by
chance from the batch's expected steps, the cell wired to its top counters at its born
counts, re-picked every 32 digits. Moves with chance 2^-5 a spike (c 2^-5 a digit for a cell
with count c), as the engine's `compete_shift 5`.

- **0 competitive learning:** a cell that fires: +1 for the sources active on its spike
  (pixels on; a cell source with chance count/20), -1 with chance 1/4 for silent sources
  above zero.
- **1 is this real:** a made-up digit after each real one, a broadcast bit; family 0's step
  on real digits, -1 for the active sources on made-up ones (same chance a spike). (1g,
  gated by a per-tile level, is written but was not run.)
- **2 predict a pixel outside:** each cell a fixed target pixel in its reach, not wirable,
  not born-wired, on in 10% to 60% of digits; its guess is whether it fired; a wrong guess,
  with chance 1/8, moves its active sources' counters by the error (+1 the pixel was on, -1
  it was off).
- **3 two looks agree:** each digit shown twice, the second look moved one pixel in a drawn
  direction with 2% of its pixels flipped; a cell that fired on one look takes family 0's
  step toward the sources active on the other. (Its shuffled control shuffles both looks by
  the same permutation; a first version shuffled the first look and moved it, which teaches
  pixel neighbours; dropped, kept in `out/ladder4/f3-moved-control/`.)
- **4 push apart:** family 0, and a source active and wired to two or more fired cells of a
  tile is stepped down (chance 1/4 a move) by each of them.
- **5 sliding threshold (BCM):** a running mean of count squared per cell (rate 0.01 a batch);
  count above it, family 0's step; fired but not above, -1 for active sources.

Why family 5: idea 165's teacher made sparse, selective cells (a spike on 14% of digits,
born 22%) at the engine's rate with about 15 pixels each, and competitive learning makes
cells that fire on more digits at twice the rate. BCM is the textbook label-free rule for
selectivity. It was coded before any scoring run was read; the choice came from idea 165's
results, not this job's.

## What is real, what is suspect

- Real: the calibration (family 0's wiring in the real sheet reads the engine's own numbers
  within 0.002 at the engine's settings, 8 seeds, both tests); every arm's transplant above
  born and churn (3 or 8 seeds, every seed); family 1's pairs lead over competitive learning
  with the tile's winners (+0.011, 8 of 8 seeds, the smallest +0.005); family 5's
  matched-rate margin over its shuffled life (8 of 8 seeds, both tests, small on MNIST).
- Real: the model's reader inverting the real sheet's order, and noise not fixing it.
- Suspect: family 1's "+0.022 / +0.060 over its shuffled life" (+0.013 / +0.051 without the
  tile's winners) as a measure of learning. Its
  shuffled control is weaker than the others' (the rule pushes pixels out when made-up and
  real look alike); the fair comparison is family 0 lived the same way.
- Suspect: the bounds. Six passes of backprop with the model's straight-through gradients,
  on a model whose reader does not rank wirings the way the real sheet does; trained to
  the model, read in the sheet. "The objectives are weak" holds for these losses as
  written. BCM's loss ran away; it is not a bound.
- Suspect: which made-up digits carry family 1. Not split (see below).
- The firing rate is still the confound it was: families 0, 1, 3, 4 written in run at 80
  (63 with the tile's winners), born at 33. Only family 5 and the bounds stay near 45.
- The model's lives: expected steps rounded by chance (lower variance than the engine's),
  the wiring re-picked every 32 digits, the tile's winners as a cap on a digit's spikes, not
  on a tick's. A cell's silent-sensor inputs (not in the model's columns) are released at its
  first move, which adds about 0.3 pixel inputs a cell to every lived arm (the coin control
  shows it is worth +0.008 / +0.013 in the real sheet on its own: 0.932 / 0.686, born-wired
  otherwise).
- Torch on the two machines is not bit-identical; each run is one machine (MNIST lives of
  seeds 5 and 6, the coin lives and families 1 and 5 with the tile cap here; pairs, MNIST
  seed 7, family 0 with the tile cap, the bounds and seeds 8-12 on stevejobs; every
  transplant and engine life here).

## Not done

- Which made-up digits family 1 needs (shuffled pixels, spliced, halves swapped one at a
  time). The coin control says it needs made-up ones, not which.
- Family 1 gated by the tile (1g): written, not run.
- Family 1 or 5 in the engine (bits): not built; the transplant is the only real-sheet
  number.
- Family 5 with the tile cap at 8 seeds (3 seeds only); the bounds with the tile's winners.
- The pieces of the model's misjudgement (a spiking model with delays and inhibition).
- A rate-matched read of families 0, 1, 3, 4 (the threshold cap raised, compete2's lever).
- Longer than one pass, other learning rates, other batch sizes: one setting per family.

## Script lines

Outputs in `out/ladder4/NAME/`, finished runs kept. On stevejobs the same lines with
`THONK_MNIST=$HOME/thonk-ladder2/out/mnist LADDER_VENV=$HOME/thonk-ladder2/out/venv`.

```
scripts/ladder4.sh main --tasks mnist --parts model --arms f0,f3,f1,f2,f4,f5 --jobs 2     every family, seeds 5-7 (pairs on stevejobs)
scripts/ladder4.sh main --tasks mnist,pair --parts bound --jobs 2                       the bounds (stevejobs)
scripts/ladder4.sh main --tasks mnist,pair --seeds 5-12 --parts engine --jobs 3         the engine's own competitive lives
scripts/ladder4.sh main --tasks pair --parts transplant --transplant-arms f0,f1,f2,f3,f4,f5 --jobs 2
scripts/ladder4.sh main --tasks mnist,pair --parts transplant --transplant-arms Bauto,Breal,Bpixel,Blooks,Bapart,Bbcm --ctrls life
scripts/ladder4.sh main --tasks mnist,pair --parts model --arms f0.cap4,f1.cap4,f5.cap4 --ctrls life,shuffled
scripts/ladder4.sh main --tasks mnist,pair --parts transplant --transplant-arms f0.cap4,f1.cap4,f5.cap4 --ctrls life,shuffled --read tw4 "--g.tile_winners 4"
scripts/ladder4.sh main --tasks mnist,pair --parts model,transplant --arms f1.coin1 --transplant-arms f1.coin1 --ctrls life
scripts/ladder4.sh main --tasks mnist,pair --seeds 8-12 --parts model,transplant --arms f1,f5 --transplant-arms f1,f5
scripts/ladder4.sh main --tasks mnist,pair --seeds 8-12 --parts model --arms f1.cap4,f0.cap4 --ctrls life,shuffled
scripts/ladder4.sh main --tasks mnist,pair --seeds 8-12 --parts transplant --transplant-arms f1.cap4,f0.cap4 --ctrls life,shuffled --read tw4 "--g.tile_winners 4"
scripts/ladder4.sh main --seeds 5-12 --table-only                                       the tables (out/ladder4/main/table.txt)
scripts/ladder4.sh smoke --quick                                                       one seed, 2,000-digit lives, small fit
out/ladder2/venv/bin/python scripts/ladder4.py noise mnist 0,0.25,0.5 out/ladder4/main/sheet-5.txt   the noise probe (-1: Poisson counts)
```

A model life takes 3 to 8 minutes here (one thread; two looks and "is this real" show two
digits a step), 1 to 2 on stevejobs; a transplant about a minute here.

## Code

- `scripts/ladder4.sh`, `ladder4.py` (runner: lives, bounds, transplants, the engine's own
  lives, the noise probe), `ladder4_life.py` (a life in the model and its read),
  `ladder4_rules.py` (the counter store and the six rules), `ladder4_data.py` (streams,
  made-up digits, second looks, moved digits, target pixels, likeness), `ladder4_bound.py`
  (the objectives by backprop), `ladder4_table.py`, `ladder4_noise.py`. Idea 165's
  `ladder2_data.py`, `ladder2_model.py`, `ladder2_train.py` are imported, not changed.
- No C++ changed. A clean build; `scripts/ladder4.sh smoke --quick` runs end to end.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, findings 43, 44, 47 and "Where it stands": **a second label-free rule
  that teaches the inside: competitive learning on real digits plus the same step backwards
  on made-up digits (a broadcast "real" bit).** With the tile's winners, written into the
  real sheet: 0.952 / 0.738 against competitive learning's 0.950 / 0.727 (the engine's 0.952
  / 0.728), +0.011 on pairs 8 of 8 seeds, level on MNIST. The teacher's 0.966 / 0.785 stays
  far off. [likely: 8 seeds, two tests, transplant only, not yet in bits]
- New finding: **idea 165's model is a faithful simulator of a wiring rule but not a judge of
  it.** The model's competitive learning, written into the real sheet, reads the engine's
  own numbers within 0.002 (both tests, 8 seeds, with the engine's settings); the model's
  own reader ranks born above every lived wiring and churn above shuffled above lived, the
  reverse of the real sheet, and noise in its cells at the real sheet's 0.6 self-agreement
  does not change that. Score wiring rules by `--wiring-load` into the real sheet, not by
  the model. [likely: 8 seeds for the calibration, 3 for the inversion, six families]
- New finding: **label-free objectives trained by backprop under the cells' constraints read
  no better than competitive learning in the real sheet** (autoencoder, autoencoder with
  decorrelation, InfoNCE on two looks, predict a pixel, real against made-up: 0.917 to 0.947
  on MNIST, 0.685 to 0.720 on pairs, against 0.952 / 0.728). The labelled teacher's 0.966 /
  0.785 is labels' doing, not an optimiser's. [hint: 3 seeds, 6 passes]
- Finding 47 and the compete2 numbers: a rule that keeps the rate (BCM's sliding threshold)
  gets competitive learning's MNIST at 47 spikes per 1,000 instead of 81 and beats its own
  shuffled life at matched rate by +0.0035 / +0.012, 8 seeds. [likely]
- `docs/plan.md`: item 4b done; item 4 ("is this real") done in its local form (family 1).
  Next candidates: family 1 in bits in the engine (with the tile's winners); which made-up
  digits it needs; where the made-up digits come from in a world that does not make them
  (the sheet's own replay); family 5 in bits (needs a step at a digit's end).
- `docs/ideas.md`: idea 173 scored. New: made-up inputs from the sheet's own replay as the
  "not real" half of family 1; a spiking model to find what the static model misses.
- `docs/predictions.md`: the predictions above and their scores.
- `docs/notes-world.md`: `scripts/ladder4.sh`.

## Predictions, scored

Model, 60,000 digits, seeds 5-7 (families 1 and 5: 8 seeds), MNIST / pairs:

| prediction | outcome |
|---|---|
| 0: 0.962 / 0.780, over shuffled +0.003 / +0.010 | 0.950 / 0.758, -0.005 / -0.003. Wrong: under born in the model |
| 1: 0.961 / 0.770, +0.001 / +0.003 | 0.951 / 0.764, -0.009 / 0.000. Wrong on MNIST |
| 2: 0.958 / 0.760, 0 / 0 | 0.958 / 0.761, -0.001 / -0.001. Right |
| 3: 0.963 / 0.780, moved 1 pixel +0.010 over born, the best on moved digits | 0.950 / 0.761, moved 1 -0.023 under born, no better than 0 on moved digits. Wrong |
| 4: family 0 within 0.003 | +0.001 / +0.001. Right |
| 5: 0.962 / 0.775, +0.002 / +0.007 | 0.954 / 0.761, -0.001 / 0.000. Wrong |
| calibration: the model's 0 lifts born by +0.002 / +0.012, most of it on shuffled too (0.6) | -0.010 / -0.009: the model's reader puts every lived wiring under born. Wrong |
| the model's 0 behaves like the engine's without a change (0.35) | the rule yes (rate, pixels, likeness; and its transplant reads the engine's numbers); the reader no. Half right |
| bounds: real 0.963 / 0.780; pixel 0.958 / 0.750; looks 0.970 / 0.800 (the best); apart 0.961 / 0.750; BCM 0.960 / 0.760 | 0.943 / 0.756; 0.955 / 0.749; 0.955 / 0.769 (best on pairs, not MNIST); 0.964 / 0.758; 0.782 / 0.649. Mostly too high |
| some family beats competitive learning in the real sheet on both tests, above 0.952 and 0.726 (0.15) | family 1 with the tile's winners: 0.952 / 0.738, level on MNIST, +0.011 on pairs. Pairs yes, MNIST no |
| family 0's transplant under the engine's 0.952 / 0.726 on both, about 0.945 / 0.710 (0.65) | 0.939 / 0.717 as lived: right; with the engine's settings 0.950 / 0.727, the engine's own |
| some family beats its shuffled life by 0.005 in the model on MNIST (0.3); on pairs (0.4) | no on both (best +0.001 and +0.005) |

I expected the model to judge as the sheet does and the objectives to be good; it does not,
and they are not.
