# Why credit by noise learns the exact opposite (idea 57, 2026-11-24)

The puzzle (docs/findings.md): "noise is the experiment" grades only neurons whose output
noise flipped, against a running average of reward. In the table engine it ends at 0.000
on six seeds of eight; turned over, it scores 1.00 on shuffled echo on all eight.

## The answer

The rule is only half of REINFORCE. On every reward that beats the average, each flipped
neuron's counter moves toward what noise made it do; on a worse one, back toward the
table's own answer. Almost no flip changes the answer (0.04% of them), so for nearly every
graded flip the reward was decided by the context the neuron was in, not by the flip. In
contexts the sheet gets right, the rule therefore pushes neurons away from their own
answers; in contexts it gets wrong, it pushes them toward their own answers. That leftover
term is the plain reward rule ("on good, more of what you did; on bad, less") turned over,
applied to whatever neurons noise happened to pick. It is far stronger than the real noise
signal, so the sheet unlearns right answers, digs in on wrong ones, and freezes once every
answer is wrong and every reward equals the average.

The missing half is the neurons noise did not flip. In node perturbation the noise has zero
mean and this term cancels by itself. A flip is one-sided, so its mean has to be taken out
by hand: also grade a sample of unflipped neurons, at the flip chance, the other way. With
that the table sheet stops heading for the wrong answer, and also stops learning: what is
left is the real noise signal, and in this sheet it is too weak to learn from.

Turning the sign over works because it turns the leftover term into the plain reward rule
on a random sample. That rule is a strong learner here (the table engine's default rule is
the same rule on every neuron). Noise plays no part in its success: the same rule on a
random sample of unflipped neurons, with no noise in it at all, scores 1.000 on seeds 1-4,
and its opposite scores 0.000.

Of the six suspects in the brief, none is the cause on its own. Timing (a), the vote (e)
and inhibition (d) are ruled out: the 16 graded ticks all lie inside the step that was
answered, and of the 3,517 graded flips that changed the answer, every one whose counter
could still move (3,381) moved it the useful way. Silenced cells (c) and non-motor cells (f) are
not it alone either: grading only fire-flips, only silencing flips, only motor cells or
only inner cells all still go wrong or fail. The nearest is (b), the baseline: a baseline
that knew each context's own expected reward would cancel the term, but the context here
is not just the cue. An oracle baseline per cue still lands on 0.000.

The other two engines differ. In the minimal engine the rule as built has the right sign:
turned over it collapses (echo 0.94 to 0.15), and with the mean taken out it still learns
(0.90), so there noise carries a real signal. In the full blob all three versions are
within noise of each other and a little below the default rule; its 0.01-0.07 lives were
the crossed shuffled-echo lock that every rule has, the default included.

## The counts that prove it

All from `tableprobe` (`src/table/probe/sign_probe.cpp`), which runs a table life with the
noise rule copied from `table_reward` and counts every graded flip. Its window scores are
byte-identical to `blobrun --table --t.learn_mode 1` (checked, echo seed 1, all 40
windows). The sheet is the one the noise rules were run on in the table results: K=4,
no bad damping, every flip graded (`--t.inputs 4 --t.bad_rise_shift 0 --t.reward_shift 0`).
Echo k=2, 40,000 steps of 20 ticks, seeds 1-4.

**Where the graded flips are**, as built, whole lives, seeds 1-4 together:

| | count | share |
| --- | --- | --- |
| graded flips | 8,866,406 | about 480 per reward that acted |
| of them on the 18 motor cells the plug reads | 42,336 | 0.48% |
| of those, flips that changed the answer | 3,517 | 8.3% of motor flips, 0.04% of all |
| flips that silenced a firing neuron (the rest made a quiet one fire) | 644,654 | 7.3% |

**Which way counters moved** on the read motor cells (useful: up for the right group, down
for the wrong one):

| | useful | harmful |
| --- | --- | --- |
| flips that changed the answer | 3,381 | 0 |
| flips that did not | 18,089 | 18,233 |

So the flips that matter teach the right thing every time, at the motor cells the
rule-arithmetic is not reversed, and the vote does not turn a spike's meaning around. But
they are drowned: for every flip that changed the answer there are about 2,500 that did not.

**Each part of the rule alone** (late score, seeds 1-4):

| what is graded | seeds 1-4 | mean |
| --- | --- | --- |
| as built (mode 1) | 0.30 0.00 0.00 0.00 | 0.08 |
| turned over (mode 2) | 1.00 1.00 1.00 0.98 | 0.99 |
| mean taken out (mode 3, the fix) | 0.50 0.55 0.46 0.50 | 0.50 |
| only flips that changed the answer (oracle) | 1.00 1.00 0.49 0.50 | 0.75 |
| plain rule on a random sample of unflipped neurons, no noise in the rule | 1.00 1.00 1.00 1.00 | 1.00 |
| the same, turned over | 0.00 0.00 0.00 0.00 | 0.00 |
| as built, only the 18 read motor cells | 0.00 0.16 0.47 0.48 | 0.28 |
| as built, only inner cells | 0.37 0.36 0.38 0.37 | 0.37 |
| as built, only flips that made a neuron fire | 0.14 0.43 0.00 0.22 | 0.20 |
| as built, only flips that silenced one | 0.00 0.15 0.37 0.34 | 0.21 |
| as built, with an oracle running average per cue | 0.00 0.00 0.24 0.00 | 0.06 |
| learning off (twin, seeds 5-12 for scale) | | 0.48 |

How to read it: the rule as built is the sum of two terms. The flips that change the answer
are the real signal; alone they learn (1.00 where such flips exist; on seeds 3 and 4 one
button group wins every step by more than one spike, so no single flip ever changes the
answer and there is nothing to learn from). Everything else is the leftover term, and its
algebra is fixed: on a flip, "toward what you did" is the opposite of "toward the table's
answer". So the leftover is the plain rule on a random sample, turned over, and that alone
lands on 0.000 on four of four seeds, exactly as the rule as built does. Taking the mean
out removes it and the score goes to chance (0.50), not to 0 and not to 1.

**How it gets there** (seed 1, as built, windows of 500 steps): for 3,000 steps it sits
near 0.5, pressing mostly one button for both cues. Then within 1,000 steps it turns
cue-specific and wrong (cue 0 to button 1, cue 1 to button 0) and stays at 0.000 from step
4,000: every reward is bad, the average reaches -1, and the gap that sets how often the rule
acts is 0, so nothing moves again. (On seed 1 a few late windows wobble back up to 0.43.)

## The fix

The smallest change that sets the sign right for the right reason is to take the flip's
mean out of the rule. Table `--t.learn_mode 3`: as mode 1, plus every unflipped neuron is
graded with the chance a flip has (2^-noise_shift, times the grading chance), with the same
formula, which for an unflipped neuron means toward the table's answer on better than
average. For a neuron in a given context the expected push from flips that did not matter
is then zero; what is left is the push from flips that did. It costs one extra random word
per tile per graded tick.

The same change in the other two engines: `--g.noise_learn_pair 1` (blob) and
`--m.noise_pair 1` (minimal) grade a sample of neuron-ticks that stayed quiet, and had no
later spike in the window, at the noise chance, the other way. `--g.noise_learn_sign -1`
and `--m.noise_sign -1` turn the rule over. All four numbers default to the rule as it
was, and runs are byte-identical to 41b8b90 (checked: blob mode 0 and mode 3, minimal,
table modes 0 and 1, shuffled echo seed 3).

Tuning, seeds 1-4, echo k=2, mode 3: noise 2^-5 0.50, 2^-7 (as built) 0.50, 2^-9 0.59,
2^-10 0.57, 2^-11 0.40 (one seed 0.98, three below chance); grading chance 1/4 0.53.
Noise 2^-9 was kept as a second row (`noise_fixed_n9`, with its own twin).

What the fix does not do: it does not make the rule learn in the table sheet (next
section), and it does not stop the lock. A rule graded against its own average freezes
when every reward equals the average, so a life that drifts into "always wrong" stays
there: mode 3 still ends at 0.000 on four echo k=4 lives and one shuffled life of eight
(seed 8 on k=4: cue 0 to button 3, cues 1-3 to button 0, every press wrong). With four
buttons most of the frozen states are wrong ones.

## Scores, all three engines

Seeds 5-12, 40,000 steps of 20 ticks, 3x3 groups, late score = mean of the last 4 of 40
windows. The learning-off twin is in brackets where it was run; versions without one share
the twin of the same sheet (the table's noise versions share `plain_k4`'s, the blob's and
the minimal engine's share `default`'s). Shuffled echo is split by the life's match: seeds
6, 7 and 11 are crossed (cue 0 to button 1), the other five straight. "2 rows apart" is the
table results' layout: cue lines on tile row 0, buttons on tile row 2 (`--g.sensor_h 8
--g.motor_y 16 --g.motor_h 8`), the same for all three engines. t is the paired difference
to the engine's default divided by its standard error; with 8 seeds (5 or 3 for the split)
|t| under 2 is noise.

**Table neurons.** `default` is the chosen sheet (K=8, bad damping). `plain_k4` is the
default rule on the sheet the noise versions use (K=4, no damping), so it is their fair
comparison.

| version | echo k=2 | echo k=4 | shuffled, straight | shuffled, crossed | 2 rows apart |
|---|---|---|---|---|---|
| default (K=8) | 0.90 (0.44) | 0.68 (0.21) | 0.75 (0.44) | 0.84 (0.52) | 0.80 (0.50) |
| plain_k4 | 0.99 (0.48) | 0.25 (0.23) | 1.00 (0.49) | 0.98 (0.44) | 0.58 (0.49) |
| noise as built (mode 1) | 0.08 | 0.01 | 0.00 | 0.22 | 0.40 |
| noise reversed (mode 2) | 0.87 | 0.22 | 1.00 | 1.00 | 0.53 |
| noise fixed (mode 3) | 0.45 | 0.07 | 0.40 | 0.47 | 0.46 |
| noise fixed, noise 2^-9 | 0.48 (0.45) | 0.09 (0.23) | 0.40 (0.47) | 0.46 (0.40) | 0.38 (0.49) |

Per seed, 5 to 12:

```
echo2     default          0.82 0.88 0.95 0.98 0.95 0.76 0.94 0.94
echo2     plain_k4         1.00 1.00 1.00 0.91 1.00 1.00 1.00 1.00
echo2     noise_as_built   0.00 0.00 0.47 0.00 0.00 0.16 0.00 0.00
echo2     noise_reversed   0.99 1.00 0.99 1.00 0.00 1.00 1.00 1.00
echo2     noise_fixed      0.48 0.47 0.45 0.27 0.53 0.46 0.46 0.49
echo2     noise_fixed_n9   0.47 0.49 0.37 0.50 0.53 0.45 0.43 0.60
echo4     default          0.73 0.78 0.24 0.71 0.95 0.24 0.99 0.78
echo4     plain_k4         0.26 0.26 0.25 0.27 0.25 0.26 0.24 0.25
echo4     noise_as_built   0.00 0.01 0.07 0.00 0.00 0.00 0.00 0.00
echo4     noise_reversed   0.29 0.26 0.24 0.26 0.25 0.00 0.25 0.22
echo4     noise_fixed      0.21 0.01 0.25 0.00 0.00 0.00 0.09 0.00
echo4     noise_fixed_n9   0.21 0.00 0.20 0.00 0.00 0.13 0.00 0.21
shuffled  default          0.82 0.84 0.70 0.48 0.83 0.79 0.98 0.85
shuffled  plain_k4         1.00 0.99 0.95 1.00 1.00 0.99 1.00 1.00
shuffled  noise_as_built   0.00 0.00 0.38 0.00 0.00 0.00 0.28 0.00
shuffled  noise_reversed   1.00 1.00 1.00 1.00 1.00 1.00 0.99 1.00
shuffled  noise_fixed      0.51 0.51 0.45 0.00 0.53 0.46 0.47 0.47
shuffled  noise_fixed_n9   0.70 0.58 0.40 0.36 0.53 0.43 0.41 0.00
apart2    default          0.65 0.96 0.83 0.80 0.45 0.77 0.98 1.00
apart2    plain_k4         0.48 0.46 0.86 0.98 0.47 0.47 0.46 0.47
apart2    noise_as_built   0.47 0.47 0.46 0.00 0.44 0.45 0.42 0.49
apart2    noise_reversed   0.46 0.45 1.00 0.52 0.45 0.45 0.45 0.46
apart2    noise_fixed      0.47 0.46 0.47 0.46 0.46 0.46 0.46 0.44
apart2    noise_fixed_n9   0.35 0.41 0.37 0.35 0.38 0.39 0.38 0.39
```

- As built it lands on the exact wrong answer on every task it can reach: 6 of 8 echo
  lives, 7 of 8 k=4 lives (k=4 chance is 0.25, so 0.00 is pressing a wrong button every
  step), 6 of 8 shuffled lives.
- Reversed is the plain rule: within noise of `plain_k4` everywhere (echo 0.87 against 0.99,
  the gap is one life locked at 0.000; k=4 0.22 against 0.25; shuffled 1.00 against 0.99;
  2 rows 0.53 against 0.58). Neither does k=4 on this K=4 sheet.
- Fixed is at or below its twin everywhere: the rule stops pointing the wrong way and
  learns nothing.

**Minimal engine.** `default` is its noise-graded rule as built. `plain` is its existing
ablation `--m.grade_mode 1` (grade every recent spike, not only noise ones).

| version | echo k=2 | echo k=4 | shuffled, straight | shuffled, crossed | 2 rows apart |
|---|---|---|---|---|---|
| default (noise, as built) | 0.94 (0.45) | 0.41 (0.20) | 0.95 (0.44) | 0.74 (0.31) | 0.42 (0.39) |
| noise reversed | 0.15 | 0.10 | 0.11 | 0.02 | 0.38 |
| noise fixed (pair) | 0.90 | 0.29 | 0.94 | 0.47 | 0.41 |
| plain (every recent spike) | 0.96 | 0.32 | 0.94 | 0.51 | 0.47 |

| against default | echo k=2 | echo k=4 | straight | crossed | 2 rows |
|---|---|---|---|---|---|
| noise reversed | -0.80 (t -41) | -0.31 (-8.6) | -0.84 (-107) | -0.72 (-24) | -0.04 (-4.0) |
| noise fixed | -0.04 (-2.5) | -0.12 (-3.5) | -0.01 (-0.8) | -0.27 (-12) | -0.01 (-1.4) |
| plain | +0.01 (+1.7) | -0.09 (-2.9) | -0.01 (-0.5) | -0.23 (-10) | +0.05 (+7.2) |

```
echo2     default          0.91 0.93 0.97 0.92 0.97 0.91 0.95 0.96
echo2     noise_fixed      0.91 0.88 0.93 0.94 0.93 0.91 0.87 0.86
echo2     noise_reversed   0.17 0.15 0.15 0.15 0.16 0.09 0.23 0.06
echo2     plain            0.97 0.94 0.96 0.97 0.96 0.91 0.97 0.97
echo4     default          0.31 0.48 0.39 0.27 0.46 0.44 0.52 0.40
echo4     noise_fixed      0.26 0.32 0.33 0.29 0.27 0.24 0.25 0.34
echo4     noise_reversed   0.12 0.12 0.18 0.10 0.03 0.03 0.12 0.07
echo4     plain            0.34 0.30 0.30 0.31 0.31 0.35 0.31 0.32
shuffled  default          0.95 0.73 0.79 0.96 0.97 0.92 0.70 0.94
shuffled  noise_fixed      0.88 0.47 0.48 0.95 0.97 0.92 0.47 0.96
shuffled  noise_reversed   0.09 0.02 0.01 0.12 0.10 0.10 0.03 0.11
shuffled  plain            0.91 0.48 0.54 0.94 0.94 0.95 0.51 0.96
apart2    default          0.45 0.42 0.44 0.44 0.39 0.44 0.43 0.37
apart2    noise_fixed      0.40 0.44 0.41 0.43 0.36 0.40 0.45 0.38
apart2    noise_reversed   0.39 0.42 0.37 0.36 0.38 0.39 0.41 0.34
apart2    plain            0.47 0.48 0.48 0.50 0.47 0.47 0.47 0.45
```

- The minimal engine's failing lives are not exactly-wrong ones. Its three weakest
  shuffled lives (0.73, 0.79, 0.70) are exactly the three crossed lives, and they press the
  right button most of the time; that is the crossed-geometry problem every rule has.
- Here the rule as built has the right sign. Turned over it goes to the wrong answer
  (shuffled 0.01-0.12, echo 0.06-0.23).
- With the mean taken out it still learns echo (0.90) and straight shuffled echo (0.94):
  in this engine noise carries a real signal. The leftover term helps rather than hurts
  here: removing it costs 0.12 on k=4 and 0.27 on crossed lives, about what the plain rule
  loses too. Why the leftover helps in a popcount sheet and hurts in a table sheet was not
  traced; see "Rough".
- Echo k=2 the mean-free rule is 0.04 behind (t -2.5).

**Full blob.** `default` is the brain rule (mode 0). The noise versions are mode 3 ("noise
instead", the rivals' tuned `--g.noise_learn_shift 4`).

| version | echo k=2 | echo k=4 | shuffled, straight | shuffled, crossed | 2 rows apart |
|---|---|---|---|---|---|
| default (brain rule) | 0.87 (0.48) | 0.31 (0.15) | 0.92 (0.52) | 0.48 (0.45) | 0.51 (0.46) |
| noise as built | 0.66 | 0.20 | 0.74 | 0.37 | 0.50 |
| noise reversed | 0.79 | 0.13 | 0.66 | 0.39 | 0.45 |
| noise fixed (pair) | 0.79 | 0.18 | 0.74 | 0.31 | 0.42 |

| against default | echo k=2 | echo k=4 | straight | crossed | 2 rows |
|---|---|---|---|---|---|
| noise as built | -0.21 (t -2.8) | -0.11 (-1.9) | -0.18 (-1.3) | -0.11 (-1.1) | -0.02 (-0.2) |
| noise reversed | -0.09 (-1.7) | -0.18 (-3.5) | -0.27 (-2.4) | -0.09 (-0.5) | -0.06 (-1.5) |
| noise fixed | -0.08 (-1.5) | -0.13 (-2.2) | -0.18 (-1.6) | -0.18 (-1.2) | -0.09 (-1.3) |

```
echo2     default          0.90 0.99 0.63 0.67 0.96 0.97 0.94 0.94
echo2     noise_as_built   0.70 0.47 0.44 0.62 0.96 0.91 0.80 0.37
echo2     noise_fixed      0.98 1.00 0.54 0.55 0.54 0.87 0.87 1.00
echo2     noise_reversed   0.85 0.96 0.79 0.52 0.96 0.64 0.73 0.85
echo2     off (default)    0.54 0.51 0.32 0.42 0.55 0.79 0.45 0.22
echo4     default          0.46 0.29 0.36 0.29 0.19 0.36 0.28 0.25
echo4     noise_as_built   0.14 0.07 0.14 0.24 0.39 0.42 0.16 0.03
echo4     noise_fixed      0.16 0.32 0.23 0.02 0.37 0.27 0.07 0.01
echo4     noise_reversed   0.25 0.29 0.04 0.05 0.29 0.08 0.04 0.01
echo4     off (default)    0.18 0.18 0.10 0.05 0.24 0.25 0.16 0.06
shuffled  default          0.96 0.01 0.50 0.97 0.99 0.68 0.94 1.00
shuffled  noise_as_built   0.70 0.04 0.20 0.43 0.98 0.95 0.87 0.64
shuffled  noise_fixed      0.50 0.07 0.35 0.53 0.96 0.74 0.50 0.95
shuffled  noise_reversed   0.78 0.21 0.45 0.70 0.60 0.77 0.52 0.43
shuffled  off (default)    0.57 0.34 0.59 0.44 0.56 0.77 0.42 0.23
apart2    default          0.59 0.60 0.58 0.47 0.45 0.58 0.36 0.45
apart2    noise_as_built   0.85 0.42 0.56 0.24 0.77 0.43 0.19 0.51
apart2    noise_fixed      0.66 0.49 0.22 0.50 0.68 0.40 0.10 0.34
apart2    noise_reversed   0.71 0.55 0.32 0.47 0.40 0.42 0.24 0.46
apart2    off (default)    0.43 0.44 0.51 0.48 0.49 0.43 0.41 0.51
```

- The three noise versions are within noise of each other on every task and all at or
  below the brain rule. Turning the rule over neither breaks it nor helps.
- The blob's lives at 0.01-0.07 were crossed shuffled lives (seed 6: 0.04 as built, 0.07
  fixed, 0.21 reversed) and the brain rule has the same life at 0.01. That is the crossed
  lock from the ablation doc, not the sign.
- Spreads are wide (single lives from 0.37 to 0.98 on echo k=2), so the blob says little
  either way.

## What it means

In the terms of docs/findings.md:

- **It is a slip in the rule's arithmetic, not a fact about noise in a sheet with
  inhibition and a largest-count vote.** Grading only the neurons noise flipped is not
  REINFORCE unless the flip's mean is taken out. The left-over term is "move neurons away
  from their own answer where things go well, toward it where they go badly" in the table
  sheet, and it outweighs the real signal by about 2,500 flips to one. The table sheet has
  no inhibition, and the flips that do reach the vote are read with the right sign every
  time, so neither inhibition nor the vote is involved.
- **The reversed rule's success is the plain reward rule's success.** On a random sample of
  neurons, with no noise in it, the plain rule scores 1.000 on seeds 1-4 and the reversed
  noise rule matches `plain_k4` on every task. Nothing about it is specific to noise. It
  does not carry to a second engine: in the minimal engine the reversed rule collapses, and
  in the blob it is no better.
- **Whether noise carries usable credit depends on the engine.** In the table sheet the
  correct noise rule learns nothing in 40,000 steps: a single flipped spike changes the
  largest-count answer on 8% of motor flips at best and on none when one button group
  dominates. In the minimal engine the correct rule still learns echo (0.90). So the
  finding "the sheet makes its own variety; noise-graded rules lose" should be split: the
  as-built noise rule in the table engine was broken, not beaten.
- **The lock is general.** Any rule graded against its own running average freezes when
  every reward equals the average; the fixed rule still freezes on all-wrong lives (4 of 8
  on echo k=4). This is the "Locking on" wall from another side.

What rule follows: if noise is to be the experiment, grade unflipped neurons too, the other
way, at the flip chance (mode 3 / `noise_learn_pair` / `noise_pair`). It is cheap (one more
random word per tile per graded tick) and it is the only form whose sign means what it
says. On these tasks it does not beat the plain rule in any engine.

## What to try next

1. Give the noise rule bigger experiments in the table sheet: flip a whole motor group or
   hold one flip for a whole step, so one experiment can change the answer. The real
   signal is there (the oracle learns 1.00) but too rare.
2. Find out why the minimal engine's noise signal is strong: count, as here, the share of
   its noise spikes that change the answer, and why its leftover term helps.
3. Make the reversed table rule honest: it is the plain rule on a sample, so name it that
   and drop the noise from it; then it no longer needs noise at all.
4. Break the freeze: something that still acts when every reward equals the average
   (idea 53, lose-shift, is aimed at this).

## Run lines

```
scripts/sign.sh probe            # the diagnosis table above, seeds 1-4 (tableprobe)
scripts/sign.sh probe-one 1      # one diagnosed life with the first 40 rewards in full
scripts/sign.sh scores table     # score tables, seeds 5-12, with twins
scripts/sign.sh scores minimal
scripts/sign.sh scores blob
scripts/sign.sh summary          # print the three score tables from out/sign/
```

The tuning rows used `out/sign/prun.sh LABEL echo 1 4 --t.inputs 4 --t.bad_rise_shift 0
--t.reward_shift 0 --t.learn_mode 3 --k 2 --t.noise_shift N` (helper, not in git). Raw
logs: `out/sign/` (not in git).

## Rough

- The table noise versions run on the K=4 sheet the table results used for them, not on
  the chosen K=8 sheet; `plain_k4` is their comparison. On K=8 with bad damping they were
  not run.
- "Useful" for a motor counter move is judged against the current cue; a table entry can
  be shared by both cues when the cell's inputs do not see the cue, so the split is a
  guide.
- The fix in the blob and the minimal engine is approximate: it samples quiet neuron-ticks
  that had no later spike at the noise chance, which matches the noise rule's "latest spike
  was noise" only on average. The blob's version also skips neurons that the noise rule
  itself graded on that reward.
- A per-cue oracle baseline did not fix the table rule, so the reward differs between a
  neuron's contexts within one cue. That it is because inputs carry the sheet's own answer
  as it builds is likely but was not measured.
- Why the leftover term helps the minimal engine and hurts the table sheet was not traced.
  Two differences that could matter: the minimal engine's noise only adds spikes (the
  table also silences), and a popcount connection gained from an active source changes the
  neuron in every context where that source is on, where a table entry is one context.
- Speed was not measured; mode 3 adds one sparse word per tile per graded tick, only on a
  reward.
