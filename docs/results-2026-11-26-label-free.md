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
