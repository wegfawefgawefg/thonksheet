# The closed loop everywhere: the whole quick map, good and bad only, parity of 3 (2026-11-26)

Queue item 9b, idea 188, following `docs/results-2026-11-26-output-inside.md` (idea 182).
**Backprop is a measuring stick here, never the project's method.**

**The answer:** (written after the runs; see below the predictions)

## Predictions (written and committed before any scoring run was read)

Seen before writing these: the docs named in the brief with their numbers (the closed loop
0.956 / 0.759 and 0.961 / 0.776; the two map rows echo16 0.997 and MNIST 0.903; parity of 3
0.758 against 0.919; the cached columns base 6, a2rucomp 13, best 15 rows), the code, and
these runs, none of them a scoring run:

- Timing lives on seed 99, not read for their scores.
- Two-button check, seed 99, 4,000 steps: with `--g.out_judge 1`, `2` or `3` an xor life and
  a parity-of-3 life are byte for byte the told-the-right-line life (`cmp` on the window
  files). With two buttons "bad for the pressed line" names the other line, so the two
  rules are one rule. Only `out_judge 4` (no line up on bad) differs.
- One earlier toy life run again with this build (parity of 3, `inold`, seed 5): the same
  as the earlier job's file line for line.
- `scripts/loopmap.sh smoke --quick`: seed 5, 2,000-step lives on echo8, xor and bandit,
  one 4,000-step toy, one 3,000-digit MNIST life. Its numbers, which I did read: echo8 alone
  1.000, told good or bad (form 2) 0.987, with the loop 0.920; xor alone 0.648, loop 0.988;
  bandit told good or bad 0.988; the MNIST output told good or bad right 0.74 of the time
  in the last tenth of 3,000 digits.

### What the world can tell

The world's "right line" is the scripted player's move on a step that gets a verdict
(`task_perfect_move`, passed by `plug_hop_lines` only when the step is judged good or bad).

- 18 rows are questions with one right button, and the verdict is "the pressed button was
  (not) it": echo2, echo8, echo16, shuffled, far3, far5, xor, parity3, order, gap, delayed2,
  hold1000, noisy, copy2, inv-scale, inv-position, pairs, mnist. The world holds the answer
  and can name it.
- 13 of those have two buttons, so naming the right line adds nothing to good and bad.
  Five have more: echo8 (8), echo16 (16), noisy (4), pairs (4), mnist (10).
- 4 rows cannot be told: bandit, chain, cartpole, catch. There the scripted player's move
  is the thing to be found from pay (the bandit's hidden best arm, the chain's unpaid first
  press, the cart's controller reading the true state, the paddle's move on steps that get
  no verdict). Telling it would be copying the player, not the task. The told-the-right-line
  columns are not run there.

### Part 1: the map told the right line (18 rows, seeds 5-12)

`alone`: the blob's own output on a sheet in which nothing else learns. `loop`: the closed
loop. A row passes at halfway from chance to perfect and over its learning-off twin.

| row | alone | loop | passes with the loop |
|---|---|---|---|
| echo2, shuffled | 1.00 | 1.00 | yes (0.95) |
| echo8 | 0.99 | 0.99 | yes (0.95) |
| echo16 | 0.97 (known) | 0.997 (known) | yes |
| far3 | 0.93 | 0.97 | yes (0.85) |
| far5 | 0.75 | 0.88 | yes (0.6) |
| xor | 0.92 | 0.99 | yes (0.9) |
| parity3 | 0.70 | 0.82 | yes (0.7); alone no (0.7) |
| order | 0.70 | 0.74 | 0.4 |
| gap | 0.52 | 0.56 | 0.15 |
| delayed2 | 0.55 | 0.60 | 0.15 |
| hold1000 | 0.50 | 0.50 | 0.03 |
| noisy | 0.62 | 0.62 | 0.5 |
| copy2 | 0.65 | 0.70 | 0.3 |
| inv-scale, inv-position | 0.60, 0.50 | the same | 0.05 each |
| pairs | 0.72 | 0.62 | alone 0.6, loop 0.45 |
| mnist | 0.89 (known) | 0.90 (known) | yes |

- **Rows passed by the loop, of 18: 10** (9 to 12: 0.7). The cached columns on these 18
  rows: base 5, a2rucomp 12, best 13 by my count from the tables.
- **Where the cells' learning adds to the output alone** (the loop over the output alone
  by 2 standard errors): xor, parity3, far5, echo16, mnist: at least four of those five:
  0.6. Where it costs: pairs (the counters of old pairs go stale as the cells re-pick):
  0.5.
- The time rows (order, gap, delayed2, hold1000, copy2) stay where the base column is: the
  output reads the last 15 ticks and the one-hop rule's marks are 12 ticks long. At most
  one of the five passes: 0.7.

### Part 2: good and bad only

The rule (`--g.out_judge`): good and the runner-up within the margin: the pressed line up,
the runner-up down; good won clear: nothing; bad: the pressed line down and one other line
up. Forms for "one other line": 1 the runner-up, 2 a line drawn by chance, 3 the lowest
line, 4 none (the control that breaks one-down-for-one-up).

- **Form 1 (the runner-up) can lock** on a row with more than two buttons: the pressed
  line and the runner-up swap, and on the next bad they swap back, so a third line is never
  tried while it sits more than a step under both. I expect form 1 under form 2 on echo16
  by 0.10 or more: 0.6.
- Form 2 on the five rows with more than two buttons, the output alone (told the right line
  in brackets): echo8 0.97 (0.99), echo16 0.85 (0.97), noisy 0.50 (0.62), pairs 0.60 (0.72),
  mnist online 0.80 (0.89). Passes echo8, echo16 and mnist: 0.7.
- Form 4 (bad moves one line only) is worse than form 2 on mnist online by 0.05 or more:
  0.6.
- The acting rows, good and bad only: bandit 0.95 and passes (0.8); catch at its twin, no
  pass (0.85); cartpole under 30 steps a fall (0.85); chain reported, not counted.
- **Rows passed told only good and bad, of 21 (chain left out): 11** (10 to 12: 0.6).
- **MNIST told only good or bad, the 60,000-digit life and a fresh reader** (validation):
  the output right 0.86 online (told the right line: 0.927); the cells the loop teaches
  read 0.945 (born 0.926, told the right line 0.955). The ladder's finding that told good
  or bad teaches the cells nothing (T8v) no longer holds, meaning the reader is 0.010 or
  more over born on validation: 0.7. On the test digits, over its coin on every one of 8
  seeds: 0.6. The reason I expect it to hold up: on a good that was close the two lines
  named are the right line and its nearest rival, the same two the told output names; only
  on a bad is the line told "higher" a guess.
- Telling the cells only of the pressed line on bad (`out_tell 1`) reads within 0.003 of
  telling them both lines: 0.5; better by more: 0.3.

### Part 3: parity of 3

Read by the same fresh old readout, the every-event arm (`old`) 0.919, the closed loop
(`inold`) 0.758. The two lives differ in: who answers (and so when it is wrong), when the
cells are told (bad steps only, against bad and close steps), and whether the motor cells
learn (in the old arm their spikes carry the answer and hidden cells can hear them; under
the new output the motor cells stay as born). An arm for each:

| arm | what | my guess |
|---|---|---|
| in2old | the new output answers, the cells told only on bad steps (`hop_teach 2`) | 0.80; 0.88 or more: 0.3 |
| old5 | the old readout answers, the cells told on bad and close steps by its spikes' margin | 0.86; 0.82 or less: 0.35 |
| olddeaf | the old arm, the cells may not count or wire a motor cell (`hop_deaf 1`) | 0.88; 0.80 or less: 0.3 |
| incells | the loop, the motor cells also learn by the old readout's rule (`out_cells 1`) | 0.82; 0.88 or more: 0.35 |
| inm0old, inm1old | the loop with no margin, with one step's worth | 0.76, 0.76; either 0.88 or more: 0.2 |
| inwold | the loop with a 10-tick window and more learner kinds | 0.78; 0.88 or more: 0.15 |

- No single arm accounts for it (none of the loop's arms reaches 0.88 and neither of the
  old readout's arms falls to 0.82): 0.3.
- A change that reads 0.88 or more on parity of 3 and costs MNIST no more than 0.003 and the
  pair test no more than 0.005 on validation (3 seeds): 0.3.

### Part 4: one counter store

- Competitive learning beside the loop with the two stores as they are (`loopcomp`) is
  worse than the better of the two alone on far5 or on parity3: 0.7.
- The smallest one-store version gets the better of the two alone on both rows: 0.2.
