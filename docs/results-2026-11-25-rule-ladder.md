# The rule's ladder: which step from backprop down to the one-hop rule loses the features? (2026-11-25)

Queue item 2g, idea 167. **Backprop here is a measuring stick, never the project's method.**
The project's method stays local rules on bits. This job walks from idea 165's backprop
teacher down to the engine's one-hop rule one step at a time, in idea 165's offline model of
the sheet's 3,072 non-sensor cells at its bottom rung (7b: about 33 inputs a cell from its own
neighbourhood, strengths 1 and 4 at born counts, fixed signs, the engine's regulation), to
find which step loses what backprop finds.

## Predictions (written before any training run of this job was read)

Seen before writing these: idea 165's doc and numbers (T0 = 0.980 / 0.860, born 0.961 /
0.768 in the model; the real sheet 0.966 / 0.785 under the transplant, born 0.924 / 0.673),
the hop doc's real-sheet numbers (from the answer 0.9377 against coin 0.9317 on MNIST, 0.698
against 0.690 on pairs), and the engine's code (`src/blob/learn_hop.cpp`). No run of this
job had been started.

The rungs as planned (each adds to the one above; MNIST / pair test, fresh linear reader in
the model):

| rung | what changes | guess |
|---|---|---|
| T0 | idea 165's teacher: 12 passes, batches of 256, Adam, random start | 0.980 / 0.86 |
| T0b | the same, started from the born wiring (the engine starts there) | 0.979 / 0.855 |
| T1a | one pass of 60,000 (the engine's taught MNIST life is 60,000 digits) | 0.976 / 0.82 |
| T1b | 20,000 digits (side arm) | 0.972 / 0.80 |
| T2 | one digit at a time, plain steps, still full backprop | 0.972 / 0.80 |
| T2h | the error stops at the cell: no backprop through the cell-to-cell step | 0.971 / 0.795 |
| T3 | fixed random feedback wires (dense) in place of the output's weights | 0.968 / 0.78 |
| T4 | a few wires a cell, each +1 or -1 (the engine's: each line with chance 1/2) | 0.967 / 0.775 |
| T5 | each line's error as a sign, with a "fine" zone | 0.966 / 0.775 |
| T6 | no derivative: the step gated by "fired" (second arm: "near, did not fire") | 0.964 / 0.772 |
| T7 | one count up or down with a small chance on each input that was on, integer counters | 0.963 / 0.771 |
| T8 | one verdict (good or bad for the pressed button) / told the right button / from the answer every step | 0.961 / 0.768; 0.962 / 0.769; 0.963 / 0.772 |
| T9 | the sheet's own output rule | likely left out (from the answer, the output plays no part) |

- Coin controls (errors from another digit) from T3 down: T3 to T5 at least 0.004 (MNIST)
  and 0.01 (pairs) under their rung; from T6 down within 0.003 of their rung.
- **The rung that loses the most** (pairs, where the room is): T1, less data (0.35); T3,
  random feedback (0.25); T6 or T7, losing the derivative or the magnitude (0.25); any
  other (0.15). On MNIST every step is small (none over 0.006: 0.7).
- **The lowest rung that still clearly beats its coin and born**: T5 (0.35), T6 (0.3), T4 or
  higher (0.2), T7 or lower (0.15).
- The engine-like bottom in the model (T8, from the answer) beats its coin by 0.003 or more
  on MNIST: 0.55; on pairs by 0.005 or more: 0.4.
- The lowest working rung's wiring in the real sheet: MNIST 0.945, pairs 0.71; above
  competitive learning (0.952 / 0.726) on either test: 0.25.
