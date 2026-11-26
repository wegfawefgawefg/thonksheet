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

Seen after the predictions and before the report runs: a smoke run (one seed, 10,000
digits) and the picking runs on the validation slice (below). The rungs grew from them:
T6b, T6s, T7w, T7m, T7k, T7x, T7t and the T8 and T10 variants were added to split T7 after
the picks showed it was the step that fell; the S arms (one step alone from T2) were added
then too.

## Short version

**From backprop (0.980 / 0.860) down to the engine's rule, the rung that loses the most is
T7, the counter step (one count up or down with a small chance, into a 6-bit counter, on
each input that was on); the lowest rung that still clearly beats its coin control and its
born arm is T7 itself, and written into the real sheet it reads 0.948 / 0.723.**
(Model: 3 seeds every rung, 8 for T7, T7x, T8, T10; transplant 8 seeds, 5-12; test digits.)

- **The counter step costs the most, in order and alone.** From T6s (the same rule with
  float steps) to T7: MNIST 0.971 to 0.966, pairs 0.810 to 0.791 (8 seeds), and its margin
  over the coin falls from +0.021 / +0.048 to +0.006 / +0.021. Alone from the online
  backprop rung it costs 0.015 / 0.070, by far the largest single step.
- **It is the small chance, not the counting.** The same step counting every marked input
  (chance 1) into 9 to 12-bit counters reads as the float rule does: 0.970 / 0.813 (T7x,
  8 seeds; no counter passed 12 bits). Wider counters at the small chance (8 bits) or a
  chance per tick do not help. Written into the real sheet the every-event wiring reads
  **0.961 / 0.761** (8 of 8 seeds above born on both), above competitive learning's 0.952 /
  0.726 and near the perfect teacher's 0.966 / 0.785; on MNIST at 43 spikes per 1,000
  cell-ticks against the born 33 (pairs 34).
- **Everything above T7 is nearly free**: one pass online (one digit at a time), the hop
  through the cells, random feedback, a few +-1 wires, signs on the lines, no derivative,
  inputs as bits, the cell told only more or less: together 0.009 on MNIST and 0.050 on
  pairs, no single one over 0.005 / 0.017, and each still beats its coin (MNIST +0.008 to
  +0.035, pairs +0.042 to +0.068) and born (+0.011 to +0.013, +0.045 to +0.078) on every
  seed. ("Less data" with batches of 256 and Adam costs 0.009 / 0.056; that is the batch: one
  digit at a time, one pass costs 0.001 / 0.017.)
- **The engine's own choices cost the rest.** From the answer (T8, the engine's
  `hop_teach 3`) in place of the output's own signed errors: -0.004 / -0.002 on T7,
  -0.013 / -0.027 alone; on MNIST it leaves only +0.0015 over born (5 of 8 seeds). Only the
  engine's learners (T10, hidden excitatory cells) takes MNIST to born (-0.0004, 3 of 8),
  pairs keep +0.017. Told only good or bad for the pressed button learns nothing (T8v).
- **In the real sheet** (8 seeds each, every seed above born except one T10 seed on MNIST):
  T7's wiring 0.948 / 0.723, T8's 0.941 / 0.727, T10's 0.928 / 0.721; born 0.924 / 0.673;
  the engine's own rule 0.938 / 0.698 (hop doc; seed 5 re-run here, identical on MNIST);
  competitive learning 0.952 / 0.726. The model's engine-like bottom (T10) reads under the
  engine's own rule on MNIST and above it on pairs.

## The rungs

The cells are idea 165's rung 7b exactly (`scripts/ladder2_model.py`: the engine's cell at
constant drive, two settle steps, the engine's regulation with its bounds, top-counter
wiring at the born counts of each kind, fixed signs, reach). Only the teacher changes. From
T2 on the teacher is written out by hand (`scripts/ladder3_rule.py`); on one batch its
backprop matches autograd's to a relative 1.6e-7. Each rung adds to the one above it; side
arms branch where named.

| rung | what changes |
|---|---|
| T0 | idea 165's teacher: backprop through the output and both settle steps, Adam (0.002, cosine), batches of 256, 12 passes of 60,000, a random start |
| T0b | the same, started from the born wiring (every rung below starts there, as the engine does) |
| T1a | one pass of 60,000 (the engine's taught MNIST life is 60,000 digits, not 20,000) |
| T1b | 20,000 digits (side arm) |
| T2 | one digit at a time, plain steps: no Adam, no momentum, no batch; one pass. The output learns by its own delta rule (a step of 0.3 / (1 + the squared size of its input)) |
| T2h | the error stops at the cell: no backprop through the cell-to-cell settle step |
| T3 | the error reaches a cell through fixed random wires from the output's error lines (dense, Gaussian), not through the output's weights |
| T4 | a few wires a cell: each line reaches a cell with chance 1/2, +1 or -1 (the engine's `hop_wire_shift 1`: about 5 of MNIST's 10 lines, 1 of the pair test's 2) |
| T5 | each line's error a sign: +1, -1, or 0 when under 0.1 ("fine") |
| T6 | no derivative: the step gated by "near the bar or fired" (the engine's `hop_near 2`, `hop_fired 1`). Side arms: T6f "fired", T6n "near, did not fire" |
| T6b | an input counts as a bit (on or not), not as its share of the drive (a cell input's count / 20) |
| T6s | the cell is told only "more" or "less" (the sign of its summed error) |
| T7 | the engine's step: one count up or down with chance 2^-s on each input that was on, integer counters of -31..31 born at their strengths 0, 1, 4, rewired to the top counters at born counts (ties to the wired). Side arms: T7w counters of -127..127; T7m, T7k, T7x counters of 9, 12, 16 bits counting every event (chance 1); T7t a chance per tick the input was on |
| T8 | from the answer, every step: each line's target less its running rate (the engine's `hop_teach 3`). Side arms: T8v told only good or bad for the pressed button (bad: it should be lower); T8r on bad also the right button higher; T8t, T8x the same error on T7t, T7x |
| T9 | the sheet's own output rule: **left out**. Under the engine's error (T8) the output plays no part in what the cells learn; under T8v and T8r it does, and the model's softmax output is stronger than the engine's readout, so those arms are if anything optimistic |
| T10 | only the cells the engine teaches learn: hidden excitatory cells (1,530 of 3,072); motor, inhibitory and pacemaker cells keep their born wiring. Side arms T10t, T10x on T8t, T8x |
| S5 to S10 | one step alone from T2 (online, plain steps, backprop's error): S5 signs, S6 / S6f no derivative, S7 counter steps (6-bit, chance 2^-7), S8 from the answer, S10 only the engine's learners |

Why this order and not the brief's: "less data" turned out to be two things, the number of
digits and the number of steps, and T2 separates them. The hop has to stop before random
feedback (feedback alignment has no second hop), so T2h is its own rung. T7 was split after
the picks showed it was where the score fell. T10 comes last because it is the engine's
restriction, not part of the rule.

Every rung from T3 down has a **coin control**: the same rung, the cells given the error of
another digit (drawn from the last 256 digits' errors; the output still learns from its own).
A rung has **learned** only by what it reads above both its coin and the born arm (rung 7b
with the real born wiring, nothing trained).

Every run is read as idea 165 reads: the cells frozen, all 60,000 training digits shown once,
a fresh linear reader of the 3,072 counts (centred and scaled as the tree's reader), scored
on the 10,000 test digits. The pair test is `--pair 2 --pair-mix 1` (same class or not,
interleaved), as in idea 165.

## Picking the step (validation slice, seed 1)

Each rung's step (the score's step for grad rungs, the counter's chance 2^-s for count rungs)
was picked on the validation slice (train on digits 0-49,999, score 50,000-59,999), seed 1,
from a grid of three, extended by one where the best sat at an edge on the rungs the
conclusion rests on. Rule: the best of min(minus coin, minus born); without a coin, the best
reader; within 0.0005 the higher reader, then the smaller step. Picks (MNIST / pairs): T2
100 / 300, T3 1 / 1, T4 0.3 / 1, T5 0.3 / 0.3, T6 0.3 / 0.3, T6f 0.3 / 1, T6n 0.1 / 0.3, T6b
1 / 1, T6s 0.03 / 3, T7 2^-3 / 2^-5, T7w 2^-7 / 2^-5, T7m, T7k, T7x chance 1 (no step), T7t
2^-7 / 2^-7, T8 2^-7 / 2^-5, T8v 2^-5 / 2^-9, T8r 2^-5 / 2^-3, T8t 2^-11 / 2^-7, T8x 2^-6 /
1, T10 2^-5 / 2^-5, T10t 2^-7 / 2^-7, T10x 2^-3 / 2^-3. Still at a grid's edge: T6s, T8t
(MNIST), T8r (pairs), T2 and the S arms (their readers are flat across the grid). The step
mattered little for grad rungs (MNIST within a rung 0.001 to 0.003 across the grid) and a
lot for count rungs (T7 on pairs: 0.797 at 2^-3, 0.780 at 2^-9). The whole pick table is
`out/ladder3/pick/table.txt`.

## The ladder (test digits, seeds 5, 6, 7)

Fresh linear reader, mean (lowest-highest). "coin": the same rung with another digit's
errors. "- coin", "- born": seed by seed, mean (seeds above). "own": the output layer's own
score. "changed": born inputs a cell no longer wires (of the 31.4 the model can see);
"pixels": pixel inputs a cell (born 6.9); "active": share of cell-digits with a spike (born
0.22). `out/ladder3/full/table.txt`. T0 is idea 165's 8 seeds; re-run here on seed 5 it
gives the same numbers (0.9796 / 0.8530, its seed 5: 0.9796 / 0.853).

MNIST:

| rung | reader | coin | - coin | - born | own | changed | pixels | active |
|---|---|---|---|---|---|---|---|---|
| born arm | 0.9604 (0.959-0.961) | | | | | 0 | 6.9 | 0.22 |
| T0 (idea 165) | 0.9795 | | | +0.019 | 0.978 | | 15.2 | 0.15 |
| T0b from the born wiring | 0.9791 (0.978-0.980) | | | +0.0187 (3/3) | 0.978 | 22.7 | 14.8 | 0.16 |
| T1a one pass, batches, Adam | 0.9709 (0.970-0.972) | | | +0.0105 (3/3) | 0.942 | 16.1 | 9.3 | 0.21 |
| T1b 20,000 digits | 0.9675 (0.966-0.969) | | | +0.0071 (3/3) | 0.906 | 12.2 | 8.2 | 0.18 |
| T2 one at a time, plain steps | 0.9786 (0.978-0.980) | | | +0.0182 (3/3) | 0.965 | 26.0 | 16.5 | 0.24 |
| T2h the error stops at the cell | 0.9778 (0.977-0.978) | | | +0.0174 (3/3) | 0.962 | 25.7 | 15.4 | 0.24 |
| T3 random feedback, dense | 0.9729 (0.973-0.973) | 0.9651 | +0.0078 (3/3) | +0.0125 (3/3) | 0.945 | 24.6 | 15.8 | 0.29 |
| T4 a few +-1 wires | 0.9723 (0.971-0.974) | 0.9648 | +0.0075 (3/3) | +0.0119 (3/3) | 0.943 | 23.3 | 15.6 | 0.28 |
| T5 line errors as signs | 0.9720 (0.972-0.973) | 0.9523 | +0.0197 (3/3) | +0.0116 (3/3) | 0.940 | 25.8 | 16.1 | 0.28 |
| T6 near or fired | 0.9731 (0.973-0.974) | 0.9503 | +0.0228 (3/3) | +0.0127 (3/3) | 0.942 | 26.8 | 16.8 | 0.28 |
| T6f fired | 0.9723 (0.971-0.973) | 0.9511 | +0.0212 (3/3) | +0.0119 (3/3) | 0.939 | 26.3 | 16.8 | 0.29 |
| T6n near, not fired | 0.9733 (0.972-0.975) | 0.9628 | +0.0105 (3/3) | +0.0129 (3/3) | 0.935 | 17.3 | 14.4 | 0.25 |
| T6b inputs as bits | 0.9714 (0.971-0.972) | 0.9363 | +0.0351 (3/3) | +0.0110 (3/3) | 0.946 | 28.6 | 7.1 | 0.25 |
| T6s the cell told more or less | 0.9712 (0.971-0.972) | 0.9506 | +0.0206 (3/3) | +0.0108 (3/3) | 0.943 | 23.8 | 6.7 | 0.25 |
| **T7 counter steps (6-bit, 2^-3)** | **0.9661 (0.965-0.967)** | 0.9587 | +0.0074 (3/3) | +0.0057 (3/3) | 0.909 | 28.3 | 5.2 | 0.22 |
| T7w counters of 8 bits | 0.9658 (0.966-0.966) | 0.9618 | +0.0041 (3/3) | +0.0054 (3/3) | 0.901 | 25.6 | 4.5 | 0.23 |
| T7m 9 bits, every event | 0.9694 (0.969-0.970) | 0.9538 | +0.0156 (3/3) | +0.0090 (3/3) | 0.934 | 28.6 | 7.1 | 0.24 |
| T7k 12 bits, every event (= T7x) | 0.9697 (0.969-0.970) | 0.9471 | +0.0226 (3/3) | +0.0093 (3/3) | 0.946 | 28.6 | 6.9 | 0.25 |
| T7t a chance per tick on | 0.9686 (0.968-0.970) | 0.9634 | +0.0052 (3/3) | +0.0082 (3/3) | 0.926 | 27.6 | 11.1 | 0.23 |
| **T8 from the answer** | **0.9626 (0.961-0.965)** | 0.9553 | +0.0072 (3/3) | +0.0022 (2/3) | 0.882 | 27.3 | 5.6 | 0.22 |
| T8v told good or bad (pressed) | 0.9594 (0.959-0.960) | 0.9581 | +0.0012 (2/3) | -0.0010 (1/3) | 0.884 | 25.4 | 5.7 | 0.25 |
| T8r told the right button on bad | 0.9671 (0.964-0.969) | 0.9624 | +0.0047 (3/3) | +0.0067 (3/3) | 0.899 | 26.0 | 4.5 | 0.23 |
| T8t (on T7t) | 0.9633 (0.961-0.965) | 0.9607 | +0.0026 (3/3) | +0.0029 (3/3) | 0.909 | 25.2 | 12.4 | 0.24 |
| T8x (on T7x) | 0.9579 (0.955-0.960) | 0.9475 | +0.0104 (3/3) | -0.0025 (0/3) | 0.902 | 27.6 | 5.9 | 0.26 |
| **T10 only the engine's learners** | **0.9603 (0.958-0.962)** | 0.9573 | +0.0030 (3/3) | -0.0001 (1/3) | 0.884 | 14.4 | 6.5 | 0.18 |
| T10t (on T8t) | 0.9627 (0.963-0.963) | 0.9580 | +0.0047 (3/3) | +0.0023 (3/3) | 0.891 | 14.4 | 6.5 | 0.18 |
| T10x (on T8x) | 0.9654 (0.963-0.968) | 0.9552 | +0.0101 (3/3) | +0.0050 (3/3) | 0.924 | 14.4 | 6.0 | 0.23 |

Pair test:

| rung | reader | coin | - coin | - born | own | changed | pixels | active |
|---|---|---|---|---|---|---|---|---|
| born arm | 0.7643 (0.757-0.769) | | | | | 0 | 6.9 | 0.22 |
| T0 (idea 165) | 0.8599 | | | +0.092 | 0.840 | | 16.6 | 0.12 |
| T0b from the born wiring | 0.8598 (0.857-0.864) | | | +0.0955 (3/3) | 0.843 | 23.0 | 16.1 | 0.13 |
| T1a one pass, batches, Adam | 0.8039 (0.803-0.805) | | | +0.0396 (3/3) | 0.746 | 14.7 | 10.8 | 0.19 |
| T1b 20,000 digits | 0.7765 (0.768-0.784) | | | +0.0121 (3/3) | 0.645 | 8.9 | 9.0 | 0.18 |
| T2 one at a time, plain steps | 0.8427 (0.840-0.846) | | | +0.0784 (3/3) | 0.803 | 26.9 | 15.1 | 0.15 |
| T2h the error stops at the cell | 0.8370 (0.833-0.840) | | | +0.0727 (3/3) | 0.789 | 26.5 | 12.3 | 0.16 |
| T3 random feedback, dense | 0.8424 (0.840-0.844) | 0.7741 | +0.0683 (3/3) | +0.0780 (3/3) | 0.805 | 22.4 | 14.3 | 0.24 |
| T4 a few +-1 wires | 0.8371 (0.832-0.842) | 0.7726 | +0.0645 (3/3) | +0.0728 (3/3) | 0.799 | 16.3 | 12.4 | 0.23 |
| T5 line errors as signs | 0.8275 (0.826-0.830) | 0.7758 | +0.0517 (3/3) | +0.0631 (3/3) | 0.786 | 16.6 | 12.4 | 0.25 |
| T6 near or fired | 0.8234 (0.819-0.828) | 0.7601 | +0.0633 (3/3) | +0.0591 (3/3) | 0.786 | 17.4 | 12.3 | 0.24 |
| T6f fired | 0.8249 (0.819-0.832) | 0.7617 | +0.0632 (3/3) | +0.0606 (3/3) | 0.787 | 17.7 | 12.7 | 0.24 |
| T6n near, not fired | 0.8180 (0.812-0.827) | 0.7760 | +0.0420 (3/3) | +0.0537 (3/3) | 0.766 | 14.7 | 11.8 | 0.24 |
| T6b inputs as bits | 0.8114 (0.807-0.817) | 0.7637 | +0.0477 (3/3) | +0.0471 (3/3) | 0.763 | 18.1 | 5.8 | 0.22 |
| T6s the cell told more or less | 0.8098 (0.803-0.815) | 0.7618 | +0.0479 (3/3) | +0.0454 (3/3) | 0.769 | 18.1 | 5.8 | 0.22 |
| **T7 counter steps (6-bit, 2^-5)** | **0.7885 (0.779-0.794)** | 0.7682 | +0.0203 (3/3) | +0.0241 (3/3) | 0.703 | 17.8 | 5.1 | 0.22 |
| T7w counters of 8 bits | 0.7923 (0.790-0.796) | 0.7672 | +0.0251 (3/3) | +0.0279 (3/3) | 0.733 | 17.8 | 5.1 | 0.23 |
| T7m 9 bits, every event | 0.8036 (0.798-0.810) | 0.7634 | +0.0402 (3/3) | +0.0393 (3/3) | 0.709 | 18.1 | 6.1 | 0.22 |
| T7k 12 bits, every event (= T7x) | 0.8118 (0.808-0.817) | 0.7620 | +0.0498 (3/3) | +0.0475 (3/3) | 0.766 | 18.1 | 5.9 | 0.23 |
| T7t a chance per tick on | 0.7955 (0.789-0.804) | 0.7622 | +0.0332 (3/3) | +0.0311 (3/3) | 0.705 | 17.8 | 8.0 | 0.22 |
| **T8 from the answer** | **0.7908 (0.790-0.792)** | 0.7652 | +0.0256 (3/3) | +0.0265 (3/3) | 0.700 | 21.2 | 4.9 | 0.22 |
| T8v told good or bad (pressed) | 0.7678 (0.762-0.775) | 0.7651 | +0.0027 (2/3) | +0.0034 (2/3) | 0.673 | 14.0 | 6.3 | 0.23 |
| T8r told the right button on bad | 0.7769 (0.771-0.784) | 0.7708 | +0.0061 (2/3) | +0.0126 (3/3) | 0.667 | 17.9 | 5.2 | 0.22 |
| T8t (on T7t) | 0.7886 (0.787-0.790) | 0.7615 | +0.0271 (3/3) | +0.0242 (3/3) | 0.698 | 21.3 | 7.2 | 0.21 |
| T8x (on T7x) | 0.7856 (0.777-0.792) | 0.7635 | +0.0220 (3/3) | +0.0212 (3/3) | 0.734 | 21.6 | 5.0 | 0.22 |
| **T10 only the engine's learners** | **0.7903 (0.787-0.792)** | 0.7724 | +0.0179 (3/3) | +0.0259 (3/3) | 0.723 | 10.7 | 6.2 | 0.22 |
| T10t (on T8t) | 0.7863 (0.783-0.791) | 0.7649 | +0.0214 (3/3) | +0.0220 (3/3) | 0.720 | 10.7 | 7.4 | 0.22 |
| T10x (on T8x) | 0.7937 (0.785-0.802) | 0.7671 | +0.0266 (3/3) | +0.0294 (3/3) | 0.739 | 10.8 | 6.1 | 0.21 |

The rungs the conclusion rests on, 8 seeds (5-12; `out/ladder3/key/table.txt`):

| rung | MNIST | coin | - coin | - born | pairs | coin | - coin | - born |
|---|---|---|---|---|---|---|---|---|
| born arm | 0.9606 (0.958-0.963) | | | | 0.7677 (0.760-0.774) | | | |
| T7 | 0.9663 (0.965-0.969) | 0.9600 | +0.0063 (8/8) | +0.0057 (8/8) | 0.7912 (0.779-0.797) | 0.7707 | +0.0205 (8/8) | +0.0235 (8/8) |
| T7x every event | 0.9699 (0.969-0.971) | 0.9466 | +0.0234 (8/8) | +0.0093 (8/8) | 0.8131 (0.808-0.817) | 0.7630 | +0.0501 (8/8) | +0.0454 (8/8) |
| T8 from the answer | 0.9621 (0.959-0.965) | 0.9565 | +0.0056 (8/8) | +0.0015 (5/8) | 0.7895 (0.785-0.794) | 0.7680 | +0.0215 (8/8) | +0.0218 (8/8) |
| T10 the engine's learners | 0.9602 (0.958-0.963) | 0.9580 | +0.0022 (6/8) | -0.0004 (3/8) | 0.7851 (0.774-0.790) | 0.7712 | +0.0140 (8/8) | +0.0174 (8/8) |

One step alone from T2 (online, plain steps, the error by backprop; 3 seeds, no coin):

| alone from T2 | MNIST | minus T2 | pairs | minus T2 |
|---|---|---|---|---|
| T2 itself | 0.9786 | | 0.8427 | |
| S5 line errors as signs | 0.9770 | -0.002 | 0.8408 | -0.002 |
| S6 no derivative, near or fired | 0.9801 | +0.002 | 0.8351 | -0.008 |
| S6f no derivative, fired | 0.9792 | +0.001 | 0.8444 | +0.002 |
| **S7 counter steps (6-bit, 2^-7)** | **0.9641** | **-0.015** | **0.7732** | **-0.070** |
| S8 from the answer | 0.9657 | -0.013 | 0.8159 | -0.027 |
| S10 only the engine's learners | 0.9679 | -0.011 | 0.8228 | -0.020 |

(S7's gate is backprop's surrogate slope, which is never zero, so every cell is taught on
every digit: a harsher test of the counter step than T7, where only cells near the bar are.)

## What the ladder says

- **The counter step is where the features go, on both tests, in order and alone.** From T6s
  (the cell told only more or less, float steps, inputs as bits) to T7 (the same, each step
  one count with a small chance into a 6-bit counter): MNIST 0.971 to 0.966, pairs 0.810 to
  0.791 (8 seeds), and the margin over the coin from +0.021 / +0.048 to +0.006 / +0.021.
  Alone from T2 it costs 0.015 / 0.070, the largest single step by far.
- **It is the chance, not the counting.** The same step counting every marked input (chance
  1) into a counter wide enough reads as T6s does: 12 bits 0.970 / 0.813 (no counter went
  past 12 bits, so 16 bits gives the same runs); 9 bits 0.969 / 0.804. A wider counter at
  the small chance does not help (8 bits: 0.966 / 0.792), nor a chance per tick on (T7t
  0.969 / 0.796). With a chance of 2^-s a counter moves on about 2^-s of the times its input
  is marked; the walk is mostly coin flips, and a 6-bit counter cannot hold the many steps
  it would take for the drift to show above them. The picks show the same: the smaller the
  chance, the closer T7 sits to its coin (pairs 2^-3 +0.027, 2^-7 +0.010, 2^-9 +0.002).
- **Everything above T7 is nearly free.** One pass of 60,000 one digit at a time (0.979 /
  0.843 against T0's 0.980 / 0.860), the hop through the cells (-0.001 / -0.006), random
  feedback (-0.005 / +0.005), a few +-1 wires (-0.001 / -0.005), signs on the lines (0 /
  -0.010), no derivative (+0.001 / -0.004), inputs as bits (-0.002 / -0.012), the cell told
  only more or less (0 / -0.002). The largest are random feedback on MNIST and bits and
  signs on pairs. Of the gates, "near or fired" and "fired" are equal; "near, did not fire"
  alone learns less (pairs 0.818, its coin margin about two thirds).
- **"Less data" was the batch.** One pass in batches of 256 with Adam (T1a: 234 steps) costs
  0.009 / 0.056; the same pass one digit at a time (T2) costs 0.001 / 0.017. 20,000 digits
  in batches costs more again (0.012 / 0.083).
- **The engine's error (from the answer) is the second loss.** On T7: -0.004 / -0.002; alone
  from T2: -0.013 / -0.027; on every-event counters (T8x) -0.012 / -0.026, and on MNIST
  that takes the sheet under born. The output's own signed errors are worth more than the
  answer's in the model, where the output is a good softmax; the hop doc found the reverse
  in the engine, whose readout is right two times in three.
- **Told only good or bad for the pressed button learns nothing** (T8v +0.001 / +0.003 over
  its coin; under born on MNIST). Told the right button on bad steps keeps a little (T8r).
- **Only the engine's learners (T10) takes MNIST to born** (-0.0004, 3 of 8) and keeps pairs
  (+0.017 over born, +0.014 over the coin, 8 of 8). Half the cells never move (14.4 inputs
  changed a cell on average, against 27).
- **The churn is visible and the coins carry it.** Every rung from T2 down changes 23 to 29 of
  a cell's 31 born inputs (T10: 14, the learners' share); the coins change as many. The
  rungs that work beat their coins at the same churn. With bit marks the coins read far
  under born on MNIST (T6b's coin 0.936: random errors with bit marks scramble the cells),
  so "minus born" is the safer margin there.
- **Pixels a cell tell the story of bits and of T7.** Grad rungs with inputs as values keep
  15 to 17 pixels a cell (T0: 15); with inputs as bits 6 to 7; T7 5.2, under the born 6.9.
  The engine's real rule in the real sheet ends at 5.7 (seed 5, below), the same loss. A
  bit mark weighs a cell input that spiked once like a pixel that was on for all 20 ticks,
  and the candidates are mostly cells (370 a cell against 103 pixels).

## In the real sheet

The wirings written into the real born sheet (`blobmnist ceiling --wiring-load`, 3,000
digits of regulation with learning off, then the inside-learning test's reader), seeds 5-12,
beside the born sheet read the same way. Hidden spikes per 1,000 cell-ticks in brackets.

MNIST:

| seed | born | T7 | T7x every event | T8 from the answer | T10 the engine's learners |
|---|---|---|---|---|---|
| 5 | 0.9228 [33.0] | 0.9475 [36.0] | 0.9589 [42.0] | 0.9427 [34.9] | 0.9301 [32.8] |
| 6 | 0.9232 [32.9] | 0.9480 [35.6] | 0.9605 [42.6] | 0.9409 [34.8] | 0.9286 [32.6] |
| 7 | 0.9267 [32.8] | 0.9493 [35.8] | 0.9607 [42.8] | 0.9399 [35.3] | 0.9246 [32.3] |
| 8 | 0.9221 [33.1] | 0.9447 [35.9] | 0.9582 [42.7] | 0.9398 [34.8] | 0.9262 [33.0] |
| 9 | 0.9235 [33.3] | 0.9454 [36.2] | 0.9611 [44.5] | 0.9392 [35.7] | 0.9260 [33.3] |
| 10 | 0.9225 [32.9] | 0.9484 [35.8] | 0.9622 [43.7] | 0.9414 [35.5] | 0.9303 [32.9] |
| 11 | 0.9258 [33.2] | 0.9473 [36.3] | 0.9613 [43.3] | 0.9451 [35.2] | 0.9283 [32.9] |
| 12 | 0.9265 [33.1] | 0.9490 [35.4] | 0.9628 [43.5] | 0.9422 [35.2] | 0.9290 [32.8] |
| mean | 0.9241 | **0.9475** (+0.0233, 8 of 8) | **0.9607** (+0.0366, 8 of 8) | **0.9414** (+0.0173, 8 of 8) | **0.9279** (+0.0038, 7 of 8) |

Pair test:

| seed | born | T7 | T7x every event | T8 from the answer | T10 the engine's learners |
|---|---|---|---|---|---|
| 5 | 0.6787 [32.7] | 0.7296 [33.2] | 0.7629 [33.9] | 0.7284 [33.1] | 0.7203 [33.4] |
| 6 | 0.6801 [32.5] | 0.7186 [33.1] | 0.7597 [33.6] | 0.7246 [33.6] | 0.7195 [33.2] |
| 7 | 0.6635 [32.4] | 0.7230 [33.3] | 0.7625 [34.2] | 0.7238 [32.9] | 0.7189 [33.2] |
| 8 | 0.6758 [32.6] | 0.7330 [33.6] | 0.7590 [33.6] | 0.7362 [33.1] | 0.7233 [33.6] |
| 9 | 0.6720 [32.9] | 0.7235 [34.3] | 0.7621 [34.1] | 0.7302 [34.2] | 0.7231 [34.0] |
| 10 | 0.6731 [32.5] | 0.7184 [33.7] | 0.7594 [33.7] | 0.7282 [33.7] | 0.7196 [33.5] |
| 11 | 0.6695 [32.8] | 0.7255 [33.6] | 0.7646 [33.6] | 0.7290 [33.3] | 0.7259 [33.3] |
| 12 | 0.6696 [32.8] | 0.7120 [33.4] | 0.7551 [33.8] | 0.7172 [33.3] | 0.7196 [33.6] |
| mean | 0.6728 | **0.7229** (+0.0502, 8 of 8) | **0.7607** (+0.0879, 8 of 8) | **0.7272** (+0.0544, 8 of 8) | **0.7213** (+0.0485, 8 of 8) |

Beside them (8 seeds, from their own docs): the perfect teacher's wiring (idea 165) 0.9663 /
0.7853; competitive learning 0.952 / 0.726 at about 65 spikes per 1,000 cell-ticks; the
engine's one-hop rule from the answer 0.9377 (coin 0.9317) / 0.698 (coin 0.690, 4 seeds).

The engine's own rule in the real sheet, one confirming seed (`scripts/ladder3.sh full
--parts engine --seeds 5`, the hop doc's `target` arm): MNIST 0.9379 against its coin 0.9328
(the hop doc's seed 5: the same, 0.9379 and 0.9328); pairs 0.7039 against 0.6912. Pixels a
hidden cell 5.7 (MNIST), born inputs kept 0.19.

- **The model's order holds in the real sheet.** T7x (every event) > T7 > T8 > T10 on MNIST,
  every rung above born on every seed but one (T10, seed 7). The model's pair numbers run
  high as idea 165 warned (T7 0.791 in the model, 0.723 in the sheet), but the margins over
  born grow in the sheet on pairs (+0.050 against the model's +0.024 for T7).
- **Every-event counters take the real sheet past competitive learning on both tests**
  (0.961 / 0.761 against 0.952 / 0.726) and to within 0.006 / 0.025 of the perfect teacher,
  with a rule that is local, one hop, sign-only and integer. On MNIST the transplanted sheet
  runs at 43 spikes per 1,000 cell-ticks (the born sheet 33, competitive learning about 65;
  the cells take more pixels and regulation cannot hold them all at target); on pairs at 34.
- **The engine-like bottom (T10) is close to the engine's own rule**: 0.928 against 0.938 on
  MNIST, 0.721 against 0.698 on pairs. The two differ in the details the model leaves out
  (one mark a digit, rewiring every 64 digits, the pair step 2^-5 against the engine's
  2^-9) and in where they learned: the model's cells are noise-free, the real sheet's are
  not. That the model's bottom reads above the engine on pairs is a hint, not a result.
- The transplant is the number to quote: T7 is the lowest rung that clearly works and reads
  0.948 / 0.723; T8, the first to lose its MNIST margin in the model (+0.0015 over born, 5
  of 8), still reads 0.941 / 0.727 in the sheet; T10, the first to fail in the model on
  MNIST, still reads 0.928 (+0.004, 7 of 8) in the sheet. So "fails" in the model at the
  bottom is a few thousandths over born in the sheet.

## Hardware

The lowest rung that works, T7, needs nothing in the cell that the engine's one-hop rule
does not already have: a signed 6-bit counter for each possible source (704 a neuron, 11
slots of 64, kept as bytes in the tag store: 704 bytes, 88 words), eight bytes of bounds, the
one-word-per-tile "near" map for each tick of the window, and error wires that cost no
memory (a hash of cell and line gives each wire and its sign). Its step is the engine's: a
random word ANDed down to 2^-3 or 2^-5 over the marked sources of a slot, then one count up
or down. What T7 has that the engine's T8 and T10 do not is outside the cell: each output
line's error is the sign of the output's own miss with a "fine" zone (2 bits a line a
verdict, 20 bits a digit on MNIST, against hop_teach 3's target and running rate per line),
which needs an output that reports how sure it is per line, not only which button it
pressed; and every non-sensor cell learns, not only the hidden excitatory ones. The
every-event counters that recover most of what T7 loses (T7m, T7k: chance 1) cost 9 to 12
bits a source instead of 6: kept as bit-planes, one 64-bit word per plane per slot, that is
9 to 12 planes x 11 slots = 99 to 132 words a neuron against the tag store's 88, 1.1 to 1.5
times; a step adds a slot's marked-source word into its planes with a ripple carry (two word
operations a plane, under 30 a slot, and no random word), and decrements the same way.
What grows is the number of steps: every marked source moves on every verdict, 8 to 32 times
the engine's counter steps at 2^-3 to 2^-5 (512 times hop_teach 3's 2^-9), and with them the
rewire checks; that is work per verdict, not per tick, and nothing on the tick's path.

## What is real, what is suspect

- Real: T7 as the step that loses most, on both tests, in order (3 seeds a rung, 8 for T7)
  and alone (S7); the every-event counters recovering it (T7x 8 seeds, every seed above T7
  on both tests; T7m 3 seeds); a wider counter at the small chance not recovering it.
- Real: idea 165's teacher reproduced (T0 seed 5 identical; from the born start 0.9791 /
  0.8598 against 0.9795 / 0.8599); the engine's own rule reproduced on seed 5.
- Suspect: the model's pair numbers run 0.08 to 0.10 high (idea 165); the transplant is the
  number to quote.
- Suspect: steps picked on one validation seed from grids of three; T6s, T8t (MNIST), T8r
  (pairs) at a grid's edge. Count rungs are sensitive to the step, so T7's numbers could
  move a little with a finer grid; the every-event arms have no step to pick.
- Suspect: the model's T7 is not the engine's step in every detail: one mark a digit, not a
  mark per tick within a 12-tick window (T7t is the per-tick version); counters rewired
  every 64 digits, not when a step crosses a bound; regulation every 64 digits; the output a
  softmax layer with a normalised delta rule, not the motor cells' readout.
- The online teacher rewires every 64 digits; within those its score steps add up exactly
  as digit-by-digit steps would (the wiring, so the cells, do not change between rewires),
  but a rewire after every digit was not run.
- Runs went to both machines (torch is not bit-identical across them; each run is one
  machine's).

## Not done

- T9, the sheet's own output rule in the model (why above).
- Each step alone from T0 (12 passes, Adam, batches): written (`A0` to `A8` in
  `ladder3_train.py`) but not run; the S arms run each step alone from T2 instead, which is
  cheaper and is where the rule's own steps start.
- The brief's "2 to 4 wires a cell": the engine's own rate (each line with chance 1/2) was
  used instead.
- Every-event counters in the real engine (`hop_shift 0` with wider tag counters): the model
  says that is the lever; the engine's tag store is 6 bits (`tag_bits` goes to 8 and is
  stored in bytes).
- A finer step grid for the count rungs; a rewire after every digit; the transplant of T6s.

## Script lines

Outputs in `out/ladder3/NAME/`; a finished run is kept, so a line run again only runs what is
missing. On stevejobs the same lines ran with `THONK_MNIST=$HOME/thonk-ladder2/out/mnist
LADDER_VENV=$HOME/thonk-ladder2/out/venv` (folder `thonk-ladder3`); here with `LADDER_VENV`
set to the main checkout's `out/ladder2/venv`.

```
scripts/ladder3.sh pick --val --seeds 1 --arms T2@3,T2@10,T2@30,T3@0.3,T3@1,T3@3,...    the picks
scripts/ladder3.sh full --arms born,T1a,T1b,T2,T2h,T3,T4,T5,T6,S5,S6,S6f,S7,S8,S10        seeds 5-7
scripts/ladder3.sh full --arms T0b,T6f,T6n,T6b,T6s,T7,T7w,T7m,T7k,T7x,T7t,T8v,T8r,T8,T8t,T8x,T10,T10t,T10x
scripts/ladder3.sh full --seeds 5 --arms T0 --coins 0             idea 165's teacher, one seed
scripts/ladder3.sh full --parts engine --seeds 5                  the engine's rule, real sheet
scripts/ladder3.sh key --seeds 5-12 --arms born --coins 0         the born arm, 8 seeds
scripts/ladder3.sh key --seeds 5-12 --arms T7,T7x,T8,T10 --transplant T7,T7x,T8,T10 --parts model,transplant
scripts/ladder3.sh full --table-only                              print a finished run
scripts/ladder3.sh smoke --quick                                  one seed, 10,000 digits, every part
```

An online run is about 45 seconds on stevejobs and 2 to 4 minutes here (one thread); a
12-pass autograd run about 5 minutes there; a transplant one to two minutes here.

## Code

- `scripts/ladder3.sh`, `ladder3.py` (runner, picks), `ladder3_train.py` (one run; the
  arms), `ladder3_rule.py` (the hand-written teacher), `ladder3_table.py`. They reuse
  `ladder2_data.py` (bits, pairs, the reader), `ladder2_model.py` (rung 7b),
  `ladder2_train.py` (idea 165's teacher, the stats, the wiring writer) and `ladder2.py`
  (the sheet's flags, the describe job, the job pool). No C++ changed.
- Checks: clean build (`nice -n 19 cmake -S . -B build && nice -n 19 cmake --build build
  -j4`, no warnings from this tree's code); the four reference lives byte-identical
  (`out/ladder3/refcheck.sh`, as `out/from-agents/hop/refcheck.sh`);
  `scripts/ladder3.sh smoke --quick` end to end.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, finding 52 ("the teaching rule is what falls short") and "Where it
  stands": **the step of the rule that loses the features is the counter step's small
  chance.** In idea 165's model, backprop's error can be cut down rung by rung to a
  one-hop, random-wire, sign-only, derivative-free, bit-marked, "more or less" rule and
  keep most of what backprop finds (MNIST 0.971 against 0.980, pairs 0.810 against 0.860,
  every seed above coin and born); turning each step into one count with chance 2^-3 to
  2^-5 into a 6-bit counter loses most of the rest (0.966 / 0.791, 8 seeds); counting every
  marked input into 9 to 12-bit counters recovers it (0.970 / 0.813, 8 seeds). In the real
  sheet the 6-bit rule's wiring reads 0.948 / 0.723 and the every-event wiring 0.961 /
  0.761, past competitive learning (0.952 / 0.726) and near the perfect teacher (0.966 /
  0.785), 8 seeds, every seed above born. [likely: one model, 8 seeds for the key rungs,
  the transplant 8 seeds; the every-event rule not yet run in the engine itself]
- `docs/findings.md`, under the one-hop rule: the engine's choices cost the rest: errors from
  the answer (`hop_teach 3`) in place of the output's own signed errors (-0.004 / -0.002 on
  T7; -0.013 / -0.027 alone), and only hidden excitatory cells learning (MNIST to born).
  Told only good or bad for the pressed button teaches nothing. [likely, model]
- New finding candidate: **a bit mark loses pixels.** Marks that are bits (as the engine's
  are) weigh a cell input that spiked once like a pixel on for 20 ticks; the model's cells
  fall from 15 to 17 pixels to 5 to 7, and the engine's real rule ends at 5.7, under the
  born 7.2. Costs 0.002 / 0.012 in the model. [hint: model, 3 seeds]
- New finding candidate: **"less data" is the batch, not the digits**: one pass of 60,000
  one digit at a time is within 0.001 / 0.017 of 12 passes in batches. [hint: 3 seeds]
- `docs/plan.md`: item 2g done (this doc). Next candidates, in order: (1) every-event
  counters in the engine: `hop_shift 0` with 9 to 12-bit counters (bit-plane counters per
  slot, a genome switch, core owner), scored by the inside test and the transplant; (2) the
  one-hop rule taught by the output's own signed errors per line with a "fine" zone, which
  needs item 9's graded readout; (3) the hop rule for motor and inhibitory cells too; (4)
  value marks (a cell input's mark weighted by how often it spiked) if (1) leaves the
  pixel loss.
- `docs/ideas.md`: idea 167 done, with the above; new ideas: bit-plane counters for the hop
  rule's counters (the hardware form of chance 1); marks worth their drive.
- `docs/predictions.md`: the predictions above and their scores.
- `docs/notes-world.md` (or wherever scripts are listed): `scripts/ladder3.sh`.

## Predictions, scored

| prediction (MNIST / pairs) | outcome |
|---|---|
| T0 0.980 / 0.86 | 0.980 / 0.853 (seed 5; idea 165's 8 seeds 0.980 / 0.860) |
| T0b 0.979 / 0.855 | 0.979 / 0.860 |
| T1a 0.976 / 0.82 | 0.971 / 0.804 |
| T1b 0.972 / 0.80 | 0.968 / 0.777 |
| T2 0.972 / 0.80 | 0.979 / 0.843: one digit at a time is far better than I thought |
| T2h 0.971 / 0.795 | 0.978 / 0.837 |
| T3 0.968 / 0.78 | 0.973 / 0.842 |
| T4 0.967 / 0.775 | 0.972 / 0.837 |
| T5 0.966 / 0.775 | 0.972 / 0.828 |
| T6 0.964 / 0.772 | 0.973 / 0.823 |
| T7 0.963 / 0.771 | 0.966 / 0.791 (8 seeds) |
| T8 verdict / right / answer 0.961 / 0.768; 0.962 / 0.769; 0.963 / 0.772 | 0.959 / 0.768; 0.967 / 0.777; 0.962 / 0.790 |
| T9 left out | left out |
| coins: T3-T5 at least 0.004 / 0.01 under their rung | right (0.008 to 0.020 / 0.052 to 0.068) |
| coins: from T6 down within 0.003 of their rung | wrong for T6 to T7 (T6 0.023 / 0.063, T7 0.006 / 0.021); right only for T8v and T10 on MNIST |
| the rung that loses most: T1 0.35, T3 0.25, T6 or T7 0.25, other 0.15 | T7 (in the 0.25 bucket); T1's loss is the batch, not the digits |
| on MNIST no step over 0.006: 0.7 | right on the main line from T2 down (largest 0.005, T3 and T7); T1a with batches 0.009 |
| the lowest clearly working rung: T5 0.35, T6 0.3, T4 or higher 0.2, T7 or lower 0.15 | T7 (the 0.15 bucket) |
| T8 beats its coin by 0.003 on MNIST: 0.55; by 0.005 on pairs: 0.4 | both happened (+0.0056, +0.0215, 8 of 8) |
| the lowest working rung in the real sheet MNIST 0.945, pairs 0.71 | T7: 0.948 / 0.723, close |
| above competitive learning on either test: 0.25 | T7 no (0.948 / 0.723 against 0.952 / 0.726); the every-event side arm yes on both (0.961 / 0.761) |

I underrated the float rules everywhere below T2 (0.005 to 0.05 too low) and did not see
that the counter step would be the one place they break, nor that the cause would be the
chance rather than the integers.
