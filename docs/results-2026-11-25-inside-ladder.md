# The inside's ladder: what can these cells hold under the best teacher? (2026-11-25)

Queue item 2e, idea 165. **Backprop here is a measuring stick, not the method.** The project's
method stays local rules on bits. This job asks only what the sheet's cells can represent when
the teacher is as good as a teacher can be, so that a shortfall can be put on the rule or on
the cell.

## Predictions (written before any training run was read)

Seen before writing these: the engine's numbers below; the tree's reader against my reader on
the born sheet's own spike counts (seed 5: MNIST 0.9248 tree, 0.923 mine; pairs 0.6924,
0.6965); and the model's born arms on seed 5, nothing trained: the real born wiring in the
model reads **0.961 on MNIST and 0.766 on the pair test** (rung 7b, the engine's rate curve),
against 0.925 and 0.692 for the same wiring in the real sheet. One trained development run
(validation, seed 1, 2 epochs) was started before this was written; its output was not read.

Guesses, fresh linear reader, test set (MNIST / pair test):

| rung | guess |
|---|---|
| 0 unconstrained net (3,072 relu, full fan-in) | 0.982 / 0.89 |
| 1 count units, full fan-in | 0.980 / 0.88 |
| 2 born fan-in (33), learned top-K of the pixels | 0.975 / 0.80 |
| 3a one-bit strengths, sign free | 0.970 / 0.78 |
| 3b strengths 1 and 4 at born counts | 0.972 / 0.79 |
| 4 cells hear cells, 2 settle steps | 0.975 / 0.82 |
| 5 fixed sign per source | 0.968 / 0.78 |
| 6 within reach | 0.965 / 0.77 |
| 7a activity at the engine's rate | 0.965 / 0.77 |
| 7b the engine's rate curve (bottom) | 0.970 / 0.79 (range 0.96-0.98 / 0.77-0.84) |
| bottom rung, no labels (autoencoder) | 0.962 / 0.77 |

- The rung that costs the most: 5, the fixed sign (pixels can only excite) 0.35; 6, reach
  0.25; 2, the fan-in 0.15; 7a/7b, the rate 0.15; 3, strengths 0.05; 4, 0.05.
- The transplant lands within 0.02 of the model's bottom rung: 0.25 (the born wiring already
  loses 0.036 and 0.074 between model and sheet). It beats the real born sheet (0.925,
  0.692): 0.6.
