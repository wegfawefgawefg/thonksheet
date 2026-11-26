# The closed loop everywhere: the whole quick map, good and bad only, parity of 3 (2026-11-26)

Queue item 9b, idea 188, following `docs/results-2026-11-26-output-inside.md` (idea 182).
**Backprop is a measuring stick here, never the project's method.**

**The job was stopped at tabling. Screens are finished; most 8-seed report runs are not.
Every number below says which seeds it is from.**

**The answer, as far as the finished runs carry it: on the screen (seeds 1-3) the closed
loop passes 12 of the 18 rows where the world has a right line to name, as one setting
(base 5, a2rucomp 12, best 13 on those rows at seeds 5-12; only 6 of the 18 were finished
at seeds 5-12, 5 of them passes), and told only good or bad it passes 13 of 21 on the
screen (chain not counted); an output that hears only good and bad needs one thing more
than the told one, a second line to step up on a bad verdict, and that line has to be drawn
by chance (the runner-up locks: the two lines swap and swap back; 16 cues 0.48 and MNIST
online 0.61 against 0.88 and 0.86 for a chance line, 0.97 and 0.89 told), which costs one
random number a bad verdict and nothing else, and with it the cells learn as much as when
told the right line (MNIST, validation, 2 seeds: fresh reader 0.9546 against 0.9554, born
0.9259); and parity of 3 did not fall as learning: each sheet is read best by the kind of
output that taught it (8 seeds: the old arm's sheet 0.919 by a fresh old readout and 0.870
by a fresh output of the new kind, the loop's 0.758 and 0.879), and with the two knobs
validation had already picked for MNIST the loop's sheet reads 0.878 by the old readout
and 0.965 by its own kind.**

## Short version

- **With two buttons, told the right line and told good or bad are the same rule.** Bad for
  one line names the other. Checked: 78 of 78 lives the same window for window (13 rows,
  seeds 1-3, the output alone and the loop). So hole (1) of the brief is a hole on five
  rows only (echo8, echo16, noisy, pairs, mnist) and on the four acting rows.
- **The map, told the right line** (screen, seeds 1-3): the loop passes 12 of 18: echo2,
  echo8, echo16, shuffled, far3, far5, xor, parity3, order, noisy, inv-scale, mnist. The
  output alone on a sheet that does not learn also passes 12: the same without parity3,
  with copy2. Fails for both: gap, delayed2, hold1000, inv-position, pairs.
- **Inside the passes.** The cells' learning makes the pass on one row, parity3 (0.721
  alone, 0.841 with the loop). It adds on echo16 (+0.025), xor (+0.074), mnist (+0.007).
  It costs copy2 its pass (0.758 to 0.695), and costs gap (-0.044) and inv-position
  (-0.176, to 0.352, under chance). Every other pass is the output's read alone. order
  passes thinly (0.76, 0.55 of the way). inv-scale (0.80 alone, 0.88 loop; no column
  passed it before) is scored on the smallest size, which sits inside the two taught
  sizes: the read carries over by shared pixels; inv-position, where nothing is shared,
  stays at chance. I would not call inv-scale invariance.
- **Good and bad only.** `--g.out_judge 2`. Good and close: the pressed line up, the
  runner-up down. Good and clear: nothing. Bad: the pressed line down and a line drawn by
  chance up. Screen and 8-seed numbers below. The runner-up as the line that goes up locks;
  the lowest line is slow; no line up (the discipline broken) is level for the output at
  12 bits and 20,000 to 60,000 steps but teaches the cells less (0.9460 against 0.9546).
- **What the cells should hear on a bad verdict: both lines, the guess included.** Fresh
  reader on MNIST validation: both lines 0.9546, nothing on bad 0.9470, the pressed line
  alone 0.9431. A lone "lower" is worse than silence. My reading, not tested: a pair of
  lines teaches only the cells that tell the two apart; one line alone pushes every cell
  it reaches the same way.
- **The ladder's "told good or bad teaches the cells nothing" (T8v) does not hold for this
  output**: +0.029 over born on validation, 2 of 2 seeds, within 0.001 of the told loop.
  Not confirmed on the test digits: that run was not started.
- **Acting rows, good and bad only**: bandit passes (0.972, 8 seeds). cartpole 21 steps a
  fall alone and 26 with the loop (a coin 21), catch 0.42 and 0.47 (halfway is 0.63; seeds
  1-3), chain 0.000 (reported, not counted). The rule has no credit over steps.
- **Parity of 3**: see the answer. The earlier guess (the old answer was spikes the cells
  could hear) is a part: the old arm with its cells deaf to motor cells reads 0.884 from
  0.938 (3 seeds). Giving the loop spiking motor cells that learn does not help (0.735).
- **One store**: designed and a 25-line version built (`compete_store`); not run.

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

## What was built

All off by default; the new genome numbers are at the end of the genome and of
`genome_args.cpp`. With them off the four reference lives are byte-identical
(`out/loopmap/refcheck.sh`, run on the last code).

- **`out_judge`: the blob's own output told only good or bad** (`learn_output.cpp`,
  `output_judge`). The world passes the pressed line as an efference copy, as it does for
  the old readout (`blob_output_verdict` gained that argument; `plug_hop.cpp` passes it);
  good or bad arrives as it always did. With `out_judge` on the output never looks at the
  right line. Good and the runner-up within `out_margin` steps' worth: the pressed line up,
  the runner-up down. Good won clear: nothing. Bad: the pressed line down and one other
  line up: `1` the runner-up, `2` a line drawn by chance, `3` the lowest line, `4` none (a
  control that breaks one-down-for-one-up). A tie between other lines goes to the next
  line round from the pressed one. A pattern answer (the calc battery) is not handled.
- **`out_tell`: what the cells hear on a bad verdict** (`hop_teach 6` reads the output's
  moves). `0` both lines that moved, `1` only the pressed line, `2` nothing.
- **Probes for parity of 3**, test knobs and not proposals: `out_cells 1` (beside the new
  output the motor cells still learn by the old readout's rule), `hop_deaf 1` (the one-hop
  rule's cells may not count or wire a motor cell). `blobprobe --after "out_on 1 ..."` now
  works on a sheet born without the output (`after_life.cpp`).
- **`compete_store 1`** (`learn_compete.cpp`, `move_cell_planes`): competitive learning's
  counts go into the one-hop rule's bit planes, one store and one re-pick. Built, compiles,
  **never run**.

## Hardware: what good and bad only costs

- State: nothing per cell or per source. Two numbers a blob (the pressed line, a flag).
- Work a verdict: one more pass over the lines' votes (at most 64 compares) for the
  runner-up. The move is the told output's: two lines, a ripple carry or borrow a slot a
  cell. It moves on the same verdicts the told output moves on (every bad, every close
  good).
- Chance: one number a bad verdict for form 2. None per source, none per cell.
- What it gives up: with more than two buttons a bad verdict does not say which line was
  right, so the line that steps up is a placeholder that keeps the lines' sum where it
  was. The output learns by ruling lines out, one a bad verdict.
- Why not the runner-up: pressed down and runner-up up is a swap, and the next bad swaps
  them back. A third line is never tried while it sits more than a step under both.

## Part 1: the map told the right line

`alone`: `--g.learn_mode 5 --motor-spread 128 --g.out_on 1 --g.out_top 1`. `loop`: that
with `--g.hop_on 1 --g.hop_bits 12 --g.hop_shift 0 --g.hop_teach 6`. Twin: the same with
learning off (every vote zero, the lowest line pressed), one twin for both columns.

Screen, seeds 1-3 (`scripts/loopmap.sh mapscreen`); `*` passes. base and best are the
cached columns at the same seeds (a2rucomp was never run on seeds 1-3).

| row | base | best | alone | loop | loop - alone (standard error) |
|---|---|---|---|---|---|
| echo2 | 1.000* | 1.000* | 1.000* | 1.000* | +0.000 |
| echo8 | 0.826* | 0.986* | 1.000* | 1.000* | +0.000 |
| echo16 | 0.291 | 0.937* | 0.969* | 0.994* | +0.025 (0.007) |
| shuffled | 1.000* | 1.000* | 1.000* | 1.000* | +0.000 |
| far3 | 0.847* | 0.985* | 0.999* | 1.000* | +0.001 (0.001) |
| far5 | 0.673 | 0.963* | 0.968* | 1.000* | +0.032 (0.030) |
| xor | 0.878* | 0.947* | 0.926* | 1.000* | +0.074 (0.016) |
| parity3 | 0.552 | 0.659 | 0.721 | 0.841* | +0.120 (0.038) |
| order | 0.672 | 0.994* | 0.767* | 0.761* | -0.007 (0.013) |
| gap | 0.513 | 0.954* | 0.554 | 0.509 | -0.044 (0.021) |
| delayed2 | 0.544 | 1.000* | 0.603 | 0.601 | -0.002 (0.039) |
| hold1000 | 0.398 | 1.000* | 0.419 | 0.459 | +0.040 (0.007) |
| noisy | 0.281 | 0.352 | 0.652* | 0.665* | +0.014 (0.009) |
| copy2 | 0.637 | 0.980* | 0.758* | 0.695 | -0.063 (0.014) |
| inv-scale | 0.681 | 0.548 | 0.797* | 0.878* | +0.081 (0.057) |
| inv-position | 0.513 | 0.469 | 0.529 | 0.352 | -0.176 (0.027) |
| pairs | 0.322 | 0.489 | 0.500 | 0.583 | +0.083 (0.044) |
| mnist | 0.222 | 0.707* | 0.891* | 0.898* | +0.007 (0.004) |
| rows passed, of 18 | 5 | 13 | 12 | 12 | |

Seeds 5-12, the six rows that finished (`scripts/loopmap.sh map --table-only`):

| row | base | a2rucomp | best | alone | loop |
|---|---|---|---|---|---|
| echo2 | 1.000* | 1.000* | 1.000* | 1.000* | 1.000* |
| echo8 | 0.801* | 0.925* | 0.976* | 1.000* | 1.000* |
| echo16 | 0.266 | 0.620* | 0.955* | 0.966* | 0.997* |
| shuffled | 1.000* | 1.000* | 1.000* | 1.000* | 1.000* |
| hold1000 | 0.506 | 0.508 | 0.998* | 0.498 | 0.498 |
| mnist | 0.214 | 0.838* | 0.714* | 0.893* | 0.903* |

The other twelve rows are in the cache only in part. The four acting rows were not run
told the right line, on purpose.

## Part 2: good and bad only

Screen, seeds 1-3, the rows where it is not the told rule (`scripts/loopmap.sh
judgescreen`). Forms: 1 the runner-up up on bad, 2 a chance line, 3 the lowest line, 4 no
line. alone / loop.

| row | told | form 1 | form 2 | form 3 | form 4 |
|---|---|---|---|---|---|
| echo8 | 1.000 / 1.000 | 0.672 / 0.747 | 1.000 / 1.000 | 1.000 / 1.000 | 1.000 / 1.000 |
| echo16 | 0.969 / 0.994 | 0.478 / 0.484 | 0.883 / 0.855 | 0.595 / 0.548 | 0.673 / 0.621 |
| noisy | 0.652 / 0.665 | 0.651 / 0.652 | 0.614 / 0.618 | 0.603 / 0.617 | 0.615 / 0.613 |
| pairs | 0.500 / 0.583 | 0.350 / 0.406 | 0.467 / 0.550 | 0.439 / 0.522 | 0.456 / 0.506 |
| mnist (online, 20,000 digits) | 0.891 / 0.898 | 0.613 / 0.553 | 0.863 / 0.877 | 0.692 / 0.701 | 0.869 / 0.854 |
| bandit | not told | 0.975 / 0.969 | the same | the same | 0.990 / 0.997 |
| cartpole (steps a fall) | not told | 20 / 28 | the same | the same | 25 / 19 |
| catch | not told | 0.423 / 0.470 | the same | the same | 0.404 / 0.490 |
| chain | not told | 0.000 | | | |

What the cells hear on bad, form 2, the loop: both lines / only the pressed line / nothing:
echo16 0.855 / 0.820 / 0.776; mnist online 0.877 / 0.847 / 0.860; pairs 0.550 / 0.561 /
0.439.

Form 2, seeds 5-12, the rows that finished (`scripts/loopmap.sh judge --table-only`):

| row | told alone | told loop | good or bad alone | good or bad loop | twin |
|---|---|---|---|---|---|
| echo8 | 1.000* | 1.000* | 1.000* | 1.000* | 0.119 |
| echo16 | 0.966* | 0.997* | 0.889* | 0.866* | 0.061 |
| mnist | 0.893* | 0.903* | 0.864* | 0.874* | 0.099 |
| bandit | | | 0.972* | 0.972* | 0.375 |
| cartpole | | | 21 | 26 | 8 |
| chain | | | 0.000 | 0.000 | 0.000 |

Under good and bad the cells cost echo16 (-0.023, standard error 0.010) where told they
add (+0.030), and add on mnist either way (+0.010). noisy, pairs and catch did not finish
at seeds 5-12.

Rows the loop passes told only good or bad, screen seeds 1-3, of 21: the 8 two-button passes above
(echo2, shuffled, far3, far5, xor, parity3, order, inv-scale), echo8, echo16, noisy, mnist,
bandit: **13**. Fails: gap, delayed2, hold1000, copy2, inv-position, pairs, cartpole, catch.

**MNIST told only good or bad: a 60,000-digit life, then a fresh linear reader**
(validation, seeds 1 and 2; `scripts/loopmap.sh judgesheet`). "own": the output's share
right in the last tenth of the life.

| arm | reader | own | share of digits that taught |
|---|---|---|---|
| born | 0.9259 | | |
| told the right line, the output alone | 0.9256 | 0.909 | |
| told the right line, the loop (`intope`) | 0.9554 | 0.927 | 0.18 |
| good or bad form 2, the output alone | 0.9283 | 0.878 | |
| **good or bad form 2, the loop** | **0.9546** | 0.901 | 0.27 |
| the same, the cells hear only the pressed line on bad | 0.9431 | 0.891 | 0.30 |
| the same, the cells hear nothing on bad | 0.9470 | 0.897 | 0.17 |
| form 2 with a 10-tick window and inhibitory and pacemaker learners | 0.9586 | 0.915 | 0.25 |
| form 4 (no line up), the output alone | 0.9255 | 0.886 | |
| form 4, the loop | 0.9460 | 0.893 | 0.29 |
| form 1 (the runner-up), the loop | 0.9497 | 0.628 | 0.48 |
| form 3 (the lowest), the loop | 0.9442 | 0.776 | 0.45 |

The two seeds are within 0.004 of each other for every arm. Even the locked output
(form 1, right 0.63 of the time) teaches the cells to 0.950: what the cells need is the
two lines on close goods, which every form names correctly.

## Part 3: parity of 3

A 40,000-step life, the inside frozen, the motor cells put back as born, a fresh output
learns the task for 20,000 steps; its late score. `scripts/loopmap.sh parity` (seeds
5-12), `parityscreen` (seeds 5-7).

| the life (seeds 5-12) | read by a fresh old readout | read by a fresh output of the new kind | the answering output in the life |
|---|---|---|---|
| none (born) | 0.552 | 0.719 | |
| old: the old readout answers, the rule told the right button on bad steps | **0.919** | 0.870 | 0.909 |
| in: the closed loop | 0.758 | **0.879** | 0.867 |
| inw: the loop with `--g.hop_who 3 --g.out_window 10` | 0.878 | **0.965** | 0.962 |

| difference | mean | seeds above | range |
|---|---|---|---|
| in - old, both read by the old readout | -0.160 | 0 of 8 | -0.221 to -0.080 |
| old - in, both read by the new output | -0.009 | 4 of 8 | -0.140 to +0.097 |
| old: read by the new output - by the old readout | -0.048 | 1 of 8 | -0.086 to +0.009 |
| in: read by the new output - by the old readout | +0.121 | 8 of 8 | +0.093 to +0.140 |
| inw - in, read by the old readout | +0.120 | 8 of 8 | +0.040 to +0.209 |
| inw - in, read by the new output | +0.086 | 8 of 8 | +0.015 to +0.176 |
| inw read by the old readout - old read by the old readout | -0.040 | 1 of 8 | -0.135 to +0.006 |
| inw read by the new output - old read by the old readout | +0.047 | 7 of 8 | -0.001 to +0.073 |

- **The fall is a mismatch between the sheet and the reader that scored it.** Read by a
  fresh output of the new kind the old arm's sheet scores 0.870 and the loop's 0.879. The
  old arm's sheet is the only one the old readout reads better than the new output does;
  every other sheet, the born one included, the new output reads better by 0.09 to 0.17.
  Each loop ends with a sheet its own output reads at about the score it had in the life.
- **The setting validation had picked for MNIST mends most of it and costs nothing**: the
  output-inside doc's second report has it at +0.005 / +0.016 on MNIST and the pair test,
  8 of 8 seeds each. So no separate cost check was run.

A difference an arm, seeds 5-7, read by a fresh old readout. "taught": the share of the
life's steps that sent the cells an error.

| arm | what differs | read | life | taught |
|---|---|---|---|---|
| old | | 0.938 | 0.928 | 0.06 |
| in | | 0.764 | 0.884 | 0.21 |
| olddeaf | old, the cells may not count or wire a motor cell | 0.884 | 0.859 | 0.11 |
| indeaf | in, the same | 0.779 | 0.850 | 0.23 |
| incells | in, the motor cells also learn by the old readout's rule | 0.735 | 0.806 | 0.24 |
| in2old | in, the cells told only on bad steps (`hop_teach 2`) | 0.807 | 0.886 | 0.10 |
| old5 | old, the cells told on bad and close steps, by a quarter of the spikes | 0.684 | 0.659 | 0.46 |
| inf1old | in, the cells' own zone one step's worth | 0.800 | 0.906 | 0.14 |
| in64old | in, the cells re-pick together every 64 verdicts | 0.831 | 0.904 | 0.18 |
| in2deaf | in, told only on bad steps and deaf | 0.814 | 0.895 | 0.10 |
| inm1old | in, margin one step's worth | 0.770 | 0.877 | 0.18 |
| inm0old | in, no margin: the newborn output never moves | 0.540 | 0.507 | 0 |
| inallold | in, inhibitory and pacemaker cells learn too | 0.868 | 0.947 | 0.15 |
| inw10old | in, a 10-tick window | 0.795 | 0.939 | 0.15 |
| inwold | in, both | 0.867 | 0.966 | 0.11 |
| in2wold | both, and told only on bad steps | 0.887 | 0.977 | 0.05 |
| oldall | old, inhibitory and pacemaker cells learn too | 0.902 | 0.950 | 0.05 |

Read by a fresh output of the new kind, the same seeds: old 0.883, olddeaf 0.916, in 0.889,
in2 0.907, inw 0.965.

- **The earlier guess is a part, about a third.** Deaf to motor cells the old arm reads
  0.884 by the old readout (2 of 3 seeds lower) and 0.916 by the new output (higher). Some
  of what the old readout reads in its own sheet is its own echo. Spiking motor cells that
  learn beside the new output do not help (0.735).
- **When the cells are told is a smaller part** (+0.043 told on bad steps only, 3 of 3).
- **The margin is not it.**
- **Who learns is the largest lever** (+0.104, 3 of 3).
- The arms line up by "taught": the less often the cells were still being taught, the
  better the old readout reads the sheet. Cause and effect not separated.

## Part 4: one counter store (design; nothing run)

Four rules re-pick a hidden cell's wiring, from three stores: the tag store's bytes (the
tags, competitive learning, the one-hop rule at `hop_bits 0`; 704 bytes a cell,
`tag_rewire`), the backward walk's own bytes (704 a cell, its own read-off), and the
one-hop rule's bit planes (132 words a cell at 12 bits, `hop_planes_rewire`). Each re-pick
writes the whole of the cell's wiring from its own counters, so a cell under two stores
has whichever rule moved last.

The design: one store in bit planes and one re-pick.

- State: per cell per slot B words, plane 0 the lowest bit of 64 counters, the last the
  sign. 8 planes are the tag store's size (88 words); 12 are 132. The byte stores go.
- Every rule steps the same counters by its own rule, a word of sources at a time (a
  ripple carry or borrow, an AND and an XOR a plane). Competitive learning: a winner's
  active sources up; a silent source above zero (no sign bit, an OR of the planes) down by
  chance. The one-hop rule: the marked sources toward the teaching bit. Tags and the walk:
  the tagged sources by the verdict.
- One re-pick, the radix select there is, after any step that moved a counter.
- A weight a rule in powers of two: step plane j instead of plane 0. Free in planes, and
  likely needed: a winner steps on one spike in 32, the one-hop rule on a fifth of the
  verdicts with every marked source.
- The other form, a store a rule and a re-pick on their bit-sliced sum (a ripple add a
  slot), costs 88 to 132 words a rule and lets a rule's counts be dropped or aged alone. I
  would build the single store first.
- Not settled: the walk's read-off maps counter size to strength by its own lines; in one
  store it would take the born counts as the others do.

Built: `--g.compete_store 1` (competitive learning's steps into the one-hop planes).
Recipe: `scripts/loopmap.sh store` (gap, delayed2, parity3, seeds 1-3; two stores, one
store in planes, one store in bytes). **Not run.** Rows chosen because a2rucomp passes gap
and delayed2 (0.938, 0.919) and the loop does not (0.51, 0.60).

## What is suspect

- Most of the map is 3 seeds (1-3), on half-length lives. Form 2 was picked on those seeds.
- The MNIST good-or-bad result is validation, 2 seeds, no coin, no test digits.
- The twin presses the lowest line always, so "over its twin" is easy; the halfway line is
  the binding half of the pass rule on every row here.
- order passes at 0.55 of the way and inv-scale by shared pixels; I would count the loop's
  screen passes as 10 solid and 2 thin.
- A good-or-bad column on a two-button row reads the told life (the tables' code does
  this); the check was window scores at seeds 1-3, not full traces at seeds 5-12.
- `hop_deaf` also takes away a cell's born inputs from motor cells at its first re-pick, so
  it is more than "cannot hear the answer".
- The fresh output "of the new kind" is told the right line; with two buttons that is the
  same information as good and bad.
- "A lone lower is worse than silence because it is not a contrast" is a reading.
- Runs were split over this PC and stevejobs; no life was run on both.

## Not finished at tabling

- The map at seeds 5-12: 12 of 18 told rows, and noisy, pairs, catch told good or bad.
  Partial lives are in `out/oneset/cache-loopmap.jsonl` and `cache-loopmap-mini.jsonl`;
  `scripts/loopmap.sh map` and `judge` run what is missing.
- `judgereport`: MNIST told good or bad on the test digits, seeds 5-12, with its coin. Not
  started. The told arms' files are already copied into `out/loopmap/judgereport/`.
- `judgebits`: the broken-discipline form at 8 and 6 bits. Not started. My guess that the
  discipline matters for the output only when counters are narrow or lives long is untested.
- `store`: not started. `compete_store` has never run a life.
- `mapknobs`: the loop with the validation knobs on the map. Not started.
- `parity`: `olddeaf` on seeds 8-12. Not started (it has seeds 5-7).
- `paritycost` and the `intopecells` arm: dropped, the mend found has its cost on record.
- The shared twin was not compared against the earlier job's per-column twin.
- `scripts/loopmap.sh smoke --quick` ran end to end before the last recipes were added;
  not run again on the last script.
- `docs/design.md` text for the new numbers: not written.

## Predictions, scored against what finished

| prediction | outcome |
|---|---|
| loop passes 10 of 18 (9 to 12) | 12 on seeds 1-3; not known at 5-12 |
| passes with the loop: far3, far5, xor, parity3 | yes, all four (seeds 1-3) |
| order 0.4, noisy 0.5, copy2 0.3, pairs 0.45 | order and noisy pass, copy2 and pairs do not |
| gap, delayed2, hold1000, inv rows fail | yes, but inv-scale passes: I had it at 0.05 |
| cells add on at least four of xor, parity3, far5, echo16, mnist | four at 2 standard errors (far5 +0.032 is not) |
| cells cost pairs | no (+0.083); they cost copy2, gap and inv-position, which I did not guess |
| at most one time row passes | yes: order |
| form 1 under form 2 on echo16 by 0.10 | yes: 0.48 against 0.88 |
| form 2: echo8 0.97, echo16 0.85, noisy 0.50, pairs 0.60, mnist 0.80 | 1.000, 0.883, 0.614, 0.467, 0.863 |
| form 4 worse than form 2 on mnist online by 0.05 | no: level alone (0.869, 0.863); worse on echo16 (0.67 against 0.88) |
| bandit passes, catch and cartpole do not | yes |
| 11 of 21 told good or bad | 13 on seeds 1-3 |
| MNIST told good or bad: output 0.86 online, reader 0.945; 0.010 over born | 0.901, 0.9546; +0.029 over born |
| over its coin on the test digits | not run |
| telling only the pressed line within 0.003: 0.5; better: 0.3 | wrong: worse by 0.011 |
| in2old 0.80; old5 0.86; olddeaf 0.88; incells 0.82; inm1 0.76; inwold 0.78 | 0.807; 0.684; 0.884; 0.735; 0.770; 0.867 |
| no single arm accounts for it: 0.3 | true of the arms I listed; the cross read, which I did not list, does |
| a change that reads 0.88 and costs MNIST and pairs nothing: 0.3 | 0.878 on 8 seeds with the validation knobs |
| Part 4 | not run |

What I had wrong: I treated the old readout's score as the measure of what the cells
learned. It measures how well the cells suit the old readout.

## Script lines

```
scripts/loopmap.sh mapscreen --table-only --detail    the map told the right line, seeds 1-3
scripts/loopmap.sh map                    the same, seeds 5-12 (runs what is missing)
scripts/loopmap.sh judgescreen --table-only           good or bad, every form, seeds 1-3
scripts/loopmap.sh same --table-only      two buttons: good-or-bad lives against told lives
scripts/loopmap.sh judge                  told and good-or-bad side by side, seeds 5-12
scripts/loopmap.sh judgesheet --table-only            MNIST told good or bad, validation
scripts/loopmap.sh judgereport            the same on the test digits (not started)
scripts/loopmap.sh parity --table-only    parity of 3, both readers, seeds 5-12
scripts/loopmap.sh parityscreen --table-only          a difference an arm, seeds 5-7
scripts/loopmap.sh judgebits              not started
scripts/loopmap.sh store                  not started
scripts/loopmap.sh smoke --quick
out/loopmap/refcheck.sh                   the four reference lives against main's
```

The good-or-bad loop's flags for any blobrun or blobmnist line: `--g.learn_mode 5 --g.out_on
1 --g.out_top 1 --g.out_judge 2 --g.hop_on 1 --g.hop_bits 12 --g.hop_shift 0 --g.hop_teach
6`. The MNIST data link was removed from this worktree; the lines need `data/mnist` or
`THONK_MNIST`.

## Code

- `src/blob/genome.hpp`, `genome_args.cpp`: `out_judge`, `out_tell`, `out_cells`,
  `hop_deaf`, `compete_store`, at the end, all 0.
- `src/blob/learn_output.cpp`, `output_state.hpp`: `output_judge`; `blob_output_verdict`
  takes the pressed lines.
- `src/blob/blob.hpp` (that signature), `src/world/plug_hop.cpp` (passes them).
- `src/blob/tick.cpp`: `output_judge` before the one-hop rule; `out_cells`.
- `src/blob/hop_state.hpp`: `hop_deaf` in `hop_usable`.
- `src/blob/learn_compete.cpp`: `move_cell_planes` (`compete_store`; never run).
- `src/probe/after_life.cpp`: the lines' array for `--after "out_on 1"`.
- `scripts/loopmap.sh`, `scripts/loopmap.py` (reuse `oneset.py`, `everyevent.py`,
  `outputinside.py`, `hop2.py`); `out/loopmap/`.
- Checks on the last code: the build has no warning from these files; the four reference
  lives byte-identical; one earlier toy life the same line for line (run before
  `compete_store` and `out_tell 2` went in).

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, new: **with two buttons, told the right line and told good or bad
  are one rule** (78 of 78 lives the same). [solid]
- `docs/findings.md`, new: **good and bad are enough for the blob's own output and for the
  cells it teaches, if a bad verdict moves the pressed line down and a chance line up.**
  MNIST online 0.864 alone and 0.874 with the loop against 0.893 and 0.903 told (8 seeds,
  20,000 digits); fresh reader 0.9546 against 0.9554 (validation, 2 seeds). The runner-up
  as the second line locks (0.61, 16 cues 0.48). [likely: the reader on 2 validation seeds,
  no coin, no test digits]
- `docs/findings.md`, T8v ("told good or bad teaches the cells nothing"): add "for the
  blob's own output it teaches as much as the right line does, on validation".
- `docs/findings.md`, "one line down for one line up": add that it holds for what the cells
  hear: both lines 0.9546, the pressed line alone 0.9431, nothing on bad 0.9470. [hint: 2
  seeds; the reason is a reading]
- `docs/findings.md`, "parity of 3 loses": replace with **each sheet is read best by the
  kind of output that taught it**; the table of four above. [solid for the cross read: 8
  seeds; the echo part 3 seeds]
- `docs/findings.md`, "Where it stands": the closed loop passes 12 of the 18 told rows on
  the screen seeds (parity3 by the cells, the rest by the output's read); it does not hold
  anything over steps (gap, delayed2, hold1000, copy2, catch, cartpole). [screen: 3 seeds]
- `docs/ideas.md`: idea 188 stopped at tabling, with the above. New: (a) the loop inside
  the one setting (time and hold cells, tiring) for the time rows; (b) `compete_store` and
  the one-store design, untested; (c) why the loop takes inv-position under chance; (d)
  the chance line on bad drawn only among lines near the top; (e) whether "taught share"
  causes the old readout's read.
- `docs/plan.md`: item 9b stopped at tabling. Open: the 8-seed map, `judgereport`,
  `judgebits`, `store`.
- `docs/predictions.md`: the table above.
- `docs/task-map.md`: told the right line or good and bad, the blob's own output passes
  association to 16 cues, reach to 5 tiles, xor, noisy lines and online MNIST alone; parity
  of 3 needs the cells; bandit passes on good and bad; nothing held over steps passes.
