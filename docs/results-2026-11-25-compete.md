# Competitive learning, and the inside-learning test as a script (2026-11-25)

Queue items 1 and 2 of `docs/plan.md`. The one question: can a local rule, with no reward,
make the inside of the sheet learn features? Part 1 is the test as one script
(`scripts/inside.sh`); part 2 is the first reward-free candidate scored by it.

## Prediction (written before any scoring run)

Seen before writing this: two smoke runs only (validation, 2,000-digit life, 3,000 digits
fitted): competitive learning lifted the reader 0.885 to 0.899 and the pixel likeness of a
cell's sources 0.05 to 0.25; and the first full run of the script on the old co-activity
rule, seed 1: born 0.924, lived 0.936, shuffled control 0.925.

- MNIST, test, 60,000-digit life, fresh reader fit on 60,000: the lived sheet beats the born
  sheet's 0.924 by at least 0.005: probability 0.7. Best guess +0.010 (0.934), range +0.000 to
  +0.020. It does not reach the random projection's 0.943: probability 0.8.
- It beats its shuffled control by at least 0.005: probability 0.6.
- It beats the co-activity rule's lived sheet (0.936 on seed 1): probability 0.35.
- Calc (xor4, parity4, add4): no member's held-out column moves by more than 0.03:
  probability 0.75. The rule pulls cells toward input lines that are on together; these
  members need products of lines far apart, which nothing here rewards.

(results below)
