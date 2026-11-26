# The credit rules not yet tried, on the rule's ladder's bench (2026-11-26)

Queue item 6, idea 177 (ideas 91, 138, 139). **No rule here uses backprop.** Backprop appears
only as a diagnostic: how often a rule's teaching bit has the sign backprop's would have.

(Results are not in yet. This commit holds the rules as built and the predictions.)

## Predictions (written before any scoring run was read)

Seen before writing these: the three docs this builds on and their numbers (in the real
sheet: born 0.924 / 0.673, T7x 0.961 / 0.761, competitive learning 0.952 / 0.726, the
backprop teacher's wiring 0.966 / 0.785). From this job only mechanics runs
(`out/credit/dev/mech.py`: lives of 1,280 and 2,560 digits on seed 5, no reader, no real
sheet), which print for each arm the share of cell-digits taught, the share of "more" among
them, counter steps a digit, the largest counter, born inputs changed, and how often the
teaching bit has backprop's sign. What they showed:

- Agreement with backprop's sign in the first 1,280 digits: the one-hop rule 0.51, two
  phases 0.50 to 0.51, code with every cell perturbed 0.507, with 192 cells 0.53, with 12
  cells 0.63, payment 0.87 (MNIST; 1.00 on pairs, where the two read weights mirror each
  other), targets from the answer 0.52, tuned wires 0.51 to 0.52.
- Two phases with every spike an event: at a gain of 1/4 and above, nine "more" in ten and a
  largest counter of 8,000 after 2,560 digits; it will pass 16 bits. Its counters were given
  24 bits and the gain grid moved down to 1/16, 1/4, 1.
- A run takes 2 to 5 minutes here.

`scripts/credit.sh smoke --quick` had been started; its output was not read before this was
committed.

Guesses, the wiring written into the real sheet, seeds 5-7, MNIST / pairs:

| arm | guess | note |
|---|---|---|
| T7x again | 0.961 / 0.761 | the ladder's numbers; its coin 0.928 / 0.675 |
| hop (T7x through this bench's loop) | the same model reader as T7x to four places on seed 5: 0.8 | the check |
| P2s two phases, sign of the difference | 0.955 / 0.745 | the one-hop error, found by the cell's own counts |
| P2 two phases, every spike an event | 0.950 / 0.735 | runs to "more"; the pick will take the smallest gain |
| C2 code, every cell | 0.925 / 0.675 | within 0.003 of its coin on both |
| C2n192, C2n12 | 0.928 / 0.680, 0.927 / 0.678 | agreement over the life 0.51, 0.53, 0.62 for all, 192, 12 |
| C1 code, good or bad only | its coin within 0.003 | |
| pay1 the output pays the cells that fired | 0.960 / 0.755 | |
| pay passed back once more | 0.961 / 0.760 | within 0.003 of pay1 |
| payc passed back to one source | pay within 0.003 | |
| tgt a target from the answer | 0.945 / 0.690 | no cell can be a "same" detector |
| dtp wires the cell tuned itself | 0.958 / 0.750 | |
| hopb, dtpb (the bit passed back) | their rule within 0.003 | |
| d1n error one showing late, no trace | 0.928 / 0.675 | its coin's level |
| d1t2 marks last 2 showings | 0.955 / 0.745 | half the marks belong to another digit |
| d1t4, d3t4 marks last 4 | 0.948 / 0.725 both | the delay costs nothing inside the mark's life, the blur does |
| d0t4 error on time, marks last 4 | 0.950 / 0.730 | |

- **Some rule beats T7x in the real sheet on both tests** (seed by seed, mean above zero and
  at least two of three seeds on each test): 0.25. The likeliest: pay, hopb or dtp.
- pay or pay1 beats T7x on MNIST by 0.002 or more: 0.3; on pairs by 0.005 or more: 0.25.
- dtp beats T7x on both: 0.2. P2s beats T7x on both: 0.1.
- Credit by code beats its coin by 0.005 on either test at any number of cells perturbed:
  0.1.
- tgt beats born on MNIST: 0.85; on pairs: 0.5.
- The firing rate written in: every rule that learns sits between 33 and 46 spikes per
  1,000 cell-ticks (T7x: 43 on MNIST, 34 on pairs).

Which arms get 8 seeds (5-12), fixed now: born, T7x and its coin, and every rule that on
seeds 5-7 reads in the sheet within 0.005 of T7x or above it on either test, with its coin.
