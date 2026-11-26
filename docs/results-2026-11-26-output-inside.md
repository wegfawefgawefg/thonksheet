# The blob's own output for the one-hop rule (2026-11-26)

Queue item 9a, idea 182, following `docs/results-2026-11-26-every-event.md` (idea 178).
**Backprop is a measuring stick here, never the project's method.**

(The answer and the tables come after the runs. This first commit holds the predictions.)

## Predictions (written and committed before any scoring run)

Seen before writing these: the three docs named in the brief with their numbers (the outside
whole-number output `outint2`: fresh reader 0.9444 / 0.7702 on validation and 0.9415 / 0.7677
on test, right 0.90 / 0.72 of the time online, teaching on 0.34 / 0.56 of the digits; the
softmax output 0.951 / 0.770 at its two fine zones; born 0.925 / 0.677; the engine's readout
right 0.68 online under its plain rule, 0.845 at 60,000 digits on a born sheet under the
runner-up rule with the gate; `ev5`, the plain readout's votes as the error, 0.932 / 0.720 on
validation), the code, and three 3,000-digit lives on validation seed 1 that say nothing
about scores (they showed that the ladder's form 4 with every option off gives form 3's
life to the digit, and that a life takes about five seconds at that length).

The ladder, as outside outputs of a live life (names as in `scripts/outputinside.py`); my
guess for the output's share right in the last tenth of an MNIST life, and for the fresh
reader after it, MNIST / pairs:

| rung | what changes | online | fresh reader |
|---|---|---|---|
| outint2 | the start: every cell, 16 bits, counts, margin perceptron | 0.90 (known) | 0.944 / 0.770 (known) |
| fld | a line is its motor cells, each reading the places it can hear | 0.91 | 0.944 / 0.765 |
| fldbit | a source is one bit: fired in the last 15 ticks | 0.895 | 0.942 / 0.760 |
| fld12 | 12-bit counters | as fldbit, within 0.003: 0.7 | |
| fld8 | 8-bit counters, every event | 0.86 | 0.935 / 0.740 |
| fld8s6 | 8 bits at the engine's chance, 2^-6 a spike | 0.84 | 0.935 / 0.735 |
| fldr1 | told good or bad only, the engine's plain rule | 0.60 | 0.927 / 0.700 |
| fldr2 | with the gate | 0.88 | 0.940 / 0.750 |
| fldr3 | runner-up and gate | 0.87 | 0.940 / 0.750 |
| com | a cell votes one bit (its sum over zero) | 0.85 | 0.935 / 0.740 |
| bit1 | one-bit wiring at the born count, drives summed | 0.75 | 0.932 / 0.720 |
| eng | the engine's readout, plain rule | 0.68 (known) | 0.932 / 0.720 (known) |
| enggate | the gate | 0.80 | 0.935 / 0.720 |
| engrun | runner-up and gate | 0.84 | 0.937 / 0.725 |
| engrunstr | and the top quarter strong | 0.85 | 0.937 / 0.725 |

- **The rung where the output falls**: one-bit wiring at a fixed count 0.45; the rule's good
  half 0.25 (known from the output-gap doc, and the gate mends it); cells that spike
  against a threshold 0.15; the counters' width and chance 0.10; the motor cells' fields or
  one bit a source 0.05.
- **The piece I expect to build**: each motor cell keeps a signed counter per source it can
  hear and its vote is the sum of its counters over the sources that fired, read at the
  verdict; a line's vote is its cells' sum; a margin perceptron told the right line.
- **That output inside the blob, alone**: right in the last tenth of an MNIST life 0.90
  (between 0.89 and 0.91: 0.6); above 0.90 as the mean of 8 seeds: 0.45. Pairs 0.71.
- **The closed loop, fresh reader** (test, seeds 5-12): MNIST 0.942, pairs 0.760. Within
  0.005 of `outint2` on MNIST (0.9415): 0.6; within 0.010 of it on pairs (0.768): 0.5.
  Reaches 0.951 on MNIST: 0.1; reaches 0.770 on pairs: 0.25. Above its coin on every seed
  of both tests: 0.8. Above a shuffled-digit life on every seed: 0.9.
- **One fine-zone setting for both tests** (within 0.005 of each test's own best on
  validation): 0.6.
- **Firing rate** of the closed loop while read: 37 to 41 per 1,000 cell-ticks on MNIST
  (0.7), 33 to 36 on pairs (0.7).
- **Payment along the output's own sources** in place of the hash wires: 0.944 / 0.765.
  Beats the hash wires on MNIST by 0.002 or more: 0.35; on pairs by 0.005 or more: 0.35;
  on both: 0.2. (The model's lead was +0.004 / +0.012 with an output that read every cell
  with float weights.)
- **Toys with the new output as the blob's output**: xor fresh output late 0.99 (0.95 or
  more: 0.7); parity of 3 0.90 (0.85 or more: 0.5). Two quick-map rows: within 0.03 of the
  old output's score: 0.5.
- **Speed**: a closed-loop MNIST life runs at 25,000 ticks a second or more here (the
  outside whole-number life: 39,400; a judged life under the old readout: 5,950): 0.6.
