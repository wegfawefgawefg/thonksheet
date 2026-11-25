# Why our output cannot read what the sheet holds (2026-11-25)

Queue item 2c, idea 161. A fresh linear reader, fit offline, gets 0.926 on MNIST from the
sheet as born and 0.952 from the sheet after a reward-free life under competitive learning.
The sheet's own output, learning online from good and bad, gets 0.71 (0.81 with the
close-vote gate) and drifts down on long lives. Where do the 0.2 go?

## Prediction (written before any ladder run)

The ladder: (1) float softmax, offline, 10 passes, every non-sensor cell; (2) the same,
online, one pass; (3) integer weights, online, told the right button; (4) the same, told only
good or bad; (5) each output unit hears only what a motor cell hears, a button is the vote
of its 102 cells; (6) one-bit connections chosen by evidence counters, at the engine's width
and step chance; (7) the engine.

Nothing seen before writing this except the numbers in the brief (0.926 / 0.952 offline,
0.71 / 0.81 online).

- The rung that loses the most, on the born sheet at 20,000 digits:
  - 5 to 6 (one-bit connections, a budget of about 33 per cell): probability 0.35;
  - 3 to 4 (good or bad only, with ten buttons): 0.25;
  - 6 to 7 (spikes, regulation, the sheet around the motor cells): 0.20;
  - 4 to 5 (local hearing, a vote of thresholded cells): 0.15;
  - 1 to 2 or 2 to 3: 0.05.
- Sizes, best guess: 1 to 2 about 0.01; 2 to 3 about 0.03; 3 to 4 about 0.06; 4 to 5
  about 0.05; 5 to 6 about 0.08; 6 to 7 about 0.05.
- The drift (lower at 120,000 than at 20,000 digits) first shows at rung 4 (the good half
  of the rule moves on every good, so it learns class means and the balance between
  buttons wanders): 0.5; at rung 6: 0.3; only at rung 7: 0.2.
- The lived sheet loses more of its lead than the born sheet on the way down (its extra
  0.026 is mostly gone by rung 6): 0.7.
