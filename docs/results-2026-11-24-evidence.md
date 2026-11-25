# Evidence before commitment: hidden counters per source (2026-11-24)

Idea 59 in `docs/ideas.md`; predictions 18, 19 and 20 in `docs/predictions.md`. The target is
the wall in `docs/findings.md` called "Reading a thin signal": with far slots the cue reaches
the buttons three tiles away and with delays its trace survives the gap, and the minimal
engine's reward rule (one-bit connections flipped by chance on single rewards) does not use
either.

## Short version

- **Counters do read a thin signal, and it costs the reflex.** With counters on every
  neuron, echo with cue and button 3 tiles apart goes from 0.39 to 0.60 (far slots on, coin
  0.48, every seed up), echo with 4 cues from 0.41 to 0.63, and echo with 8 and 16 cues ends
  above chance for the first time in any engine (0.19 against 0.125; 0.087 against 0.0625).
  Exclusive-or (both-not-either) goes from 0.26 to 0.58 with counters on the motor neurons.
- **The price is echo k=2 at one tile: 0.94 down to 0.73,** on every seed and in every
  setting that gives the gains. No setting tried gets both.
- **Time is not fixed.** Delayed echo with a gap stays at chance among presses (0.49): the
  score rises from 0.24 to 0.39 only because the blob presses on the ask step more often.
- **What carries it** is grading every spike, not only noise spikes: counters fed only by
  noise spikes (the plain rule's credit with evidence added) keep the reflex (0.93) and
  change almost nothing else. Counter width matters less than expected (6 bits best; 4 and 8
  within noise on most rows). Counters on every neuron, not only the motor neurons, are what
  lift echo k=4.
- **Cost:** counters on the motor neurons only: 32 bytes more per neuron (114 against 82)
  and 24% slower in an echo life; on every neuron: 411 more bytes per neuron (493, 6x the
  engine) and 5.8x slower in an echo life, all of it in the reward path. With learning off
  every setting ticks at the same speed: the hot loop is unchanged.

## What was built

`src/minimal/minimal_evidence.cpp`, switched by `--m.ev_on` (0 off, the default; 1 the motor
neurons; 2 every non-sensor neuron). Off is byte-identical to before (csv `cmp` on echo k=2,
k=4, shuffled, far3 with 2 far slots, delayed with delays 1..8, seed 5, against the binary
from before this work; checked again after merging main).

- **The counters.** Every possible source of a counter neuron (576 local, 64 per far slot)
  has a counter of `ev_bits` bits (2 to 8), offset binary, so 2^(bits-1) is zero. They are
  stored bitsliced: for each slot, `ev_bits` 64-bit words, word i holding bit i of the 64
  sources' counters. "Add one to every heard source" is a ripple of word ops (an AND, an XOR
  per plane, saturating at the top or bottom); "which counters are at least v" is a compare
  of `ev_bits` word ops. Same shape for every counter neuron, no floats, chance is a sparse
  random word as before.
- **When they move.** Only on a reward, with the plain rule's chance (reward against its
  running average; the gap sets the chance) plus `ev_shift`. A graded neuron's heard sources
  go up on better-than-average and down on worse. Who is graded (`ev_spikes`): 0 a neuron
  whose latest spike in the 20-tick window was noise (the plain rule's choice), 1 a neuron
  whose latest spike was any spike, 2 every spike in the window (sources ORed).
  `ev_fired_min` asks for at least N spikes. The mirror (`ev_quiet` 1): a counter neuron that
  did not spike at all in the window moves the sources it heard on the last tick the other
  way (on bad it should have fired, on good it was right to stay quiet). `ev_inh 1` moves an
  inhibitory source's counter the opposite way to an excitatory one.
- **The read-off** (`ev_read`). After a neuron's counters move, its mask is rebuilt from
  them: 0, the top `budget` (48) counters, by a binary search for the line; at the line,
  sources already wired are kept first, the rest filled or trimmed at random (so the budget
  holds exactly and ties do not churn); 1, every counter at or above a line the neuron moves
  one step per update toward 48 connections; 2, every counter above zero (no budget).
- **At birth** every counter sits at zero and each wired source at `ev_init` above it, so a
  new source needs `ev_init` + 1 net good rewards before it can push out an idle wired one.
- **Co-activity wiring** on a counter neuron (`ev_cowire` 1) adds one to the counter of one
  unwired source heard on the last tick and reads the mask again; 0 swaps the mask as before
  (and the read-off may undo it later). On other neurons it is unchanged.
- The plain rule still runs on every neuron that has no counters. The tick never reads the
  counters; the masks it reads are rewritten only on a reward or a co-activity event.

`minimalprobe life` now also prints the late press shares and the bad rate, so a life that
ends pressing one button shows up directly.

## Settings, chosen on seeds 1-4

`out/evidence/sweep.sh out/evidence/tuneN.txt 1 4 --no-baselines` (tune1 to tune6; logs
beside them). Late score, mean of seeds 1-4, learning on only. "far3 f" is echo 3 tiles apart
with 2 far slots on every neuron (`--m.far_slots 2 --m.far_percent 100`), "delayed0 d" the
ask right after the cue with delays 1..8 (`--m.delay_max 8`).

| setting | echo2 | echo4 | far3 f | delayed0 d |
| --- | --- | --- | --- | --- |
| off | 0.972 | 0.387 | 0.406 | 0.606 |
| motor, 4 bits, noise spikes, mirror | 0.721 | 0.285 | 0.429 | 0.662 |
| motor, 4 bits, latest spike, mirror | 0.715 | 0.368 | 0.528 | 0.677 |
| motor, 4 bits, every spike, mirror | 0.663 | 0.297 | 0.547 | 0.729 |
| motor, 4 bits, noise spikes, no mirror | 0.870 | 0.309 | 0.413 | |
| every neuron, 4 bits, noise spikes, mirror | 0.598 | 0.223 | 0.349 | 0.668 |
| motor, 8 bits (init 8), latest spike | 0.677 | 0.339 | 0.501 | |
| **A: motor, 6 bits (init 4), latest spike, mirror** | 0.747 | 0.461 | 0.543 | 0.637 |
| motor, 6 bits, latest, slower (`ev_shift 2`) | 0.696 | 0.390 | 0.517 | |
| motor, 8 bits, latest, slower | 0.815 | 0.410 | 0.388 | |
| motor, 6 bits, latest, co-activity swaps the mask | 0.704 | 0.361 | 0.532 | |
| motor, 6 bits, fired = 2+ spikes | 0.681 | 0.394 | 0.540 | |
| motor, 6 bits, fired = 3+ spikes | 0.553 | 0.407 | 0.476 | |
| motor, 6 bits, init 8 | 0.707 | 0.345 | 0.552 | |
| motor, 6 bits, moving line | 0.795 | 0.388 | 0.527 | |
| motor, 6 bits, above zero (no budget) | 0.720 | 0.346 | 0.513 | |
| motor, 6 bits, inhibitory flipped, latest spike | 0.595 | 0.376 | 0.401 | |
| motor, 6 bits, inhibitory flipped, noise spikes, no mirror | 0.950 | 0.418 | 0.371 | 0.642 |
| **B: motor, 4 bits (init 2), inhibitory flipped, noise spikes, no mirror** | 0.970 | 0.346 | 0.442 | |
| motor, 8 bits, the same as B | 0.946 | 0.377 | 0.420 | |
| **C: A on every neuron** (checked after the ablation, see below) | 0.735 | 0.654 | 0.567 | 0.572 |

Things learned while choosing:

- **Two families, a straight trade.** Grading only noise spikes (B) keeps the reflex but
  barely moves anything else; grading every neuron that spiked (A) buys reach and costs
  echo k=2 about 0.2. No knob tried closes the gap: slower updates, wider counters, a higher
  bar for "fired", a larger birth offset, a moving line, flipping inhibitory sources.
- **Why A costs the reflex, as far as I can tell:** a motor neuron of the losing button that
  spiked once in the window is graded as if it had fired the press, so on a good step its
  sources (the right cue among them) go up too. The noise-only rule does not have this
  problem because a noise spike is an experiment by that neuron alone. Requiring 2 or more
  spikes to count as fired did not help (0.68, 0.55), so this is a guess, not a finding.
- 4-bit counters saturate: after a 40,000-step life with A at 4 bits, 17.8% of a motor
  neuron's counters sit at the top, so the top 48 are mostly ties and the read-off falls
  back to "keep what is wired".
- C (A on every neuron) was first seen in the parts ablation on seeds 5-12 (echo4 0.63). It
  was then run on seeds 1-4 before being reported anywhere else (row above: echo4 0.654,
  far3 0.567), so it is reported as a chosen setting; the reader should know the order.

Report settings: **A** `--m.ev_on 1 --m.ev_bits 6 --m.ev_init 4 --m.ev_spikes 1`,
**B** `--m.ev_on 1 --m.ev_bits 4 --m.ev_init 2 --m.ev_inh 1 --m.ev_spikes 0 --m.ev_quiet 0`,
**C** `--m.ev_on 2 --m.ev_bits 6 --m.ev_init 4 --m.ev_spikes 1`.

## The tasks, seeds 5-12

`out/evidence/sweep.sh out/evidence/report.txt 5 12` (and `report_c.txt`,
`report_shuf.txt`, `report_c_shuf.txt`; C's echo2, echo4, far3 and delayed0 rows are the
parts sweep's `xaall` rows, the same flags). Each row is for example

```
scripts/minimal.sh seeds far3 5 12 --m.far_slots 2 --m.far_percent 100                      # off
scripts/minimal.sh seeds far3 5 12 --m.far_slots 2 --m.far_percent 100 --m.ev_on 1 \
    --m.ev_bits 6 --m.ev_init 4 --m.ev_spikes 1                                            # A
```

Late score (mean of the last 4 of 40 windows of 1,000 steps), mean over 8 lives (sd); diff
is paired against off, seed by seed, with its standard error. The learning-off twin is the
same life with or without counters (birth draws nothing new and the counters never move
with learning off; checked: the far3 A row with twins gives 0.335, the off row's twin), so
one twin column serves all.

### 1. Reach: echo k=2 with cue and button 1, 2, 3, 5 tiles apart (far slots on past 1)

| tiles apart | off | twin | coin | A | diff | B | diff | C | diff |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 0.942 (0.023) | 0.451 | 0.472 | 0.730 (0.037) | -0.212 (0.019) | 0.925 (0.036) | -0.017 (0.016) | 0.735 (0.038) | -0.207 (0.019) |
| 2 | 0.447 (0.050) | 0.346 | 0.478 | 0.571 (0.045) | +0.124 (0.021) | 0.441 (0.100) | -0.006 (0.045) | **0.618** (0.069) | +0.171 (0.032) |
| 3 | 0.390 (0.035) | 0.335 | 0.481 | 0.539 (0.020) | +0.149 (0.014) | 0.468 (0.031) | +0.078 (0.014) | **0.597** (0.053) | +0.207 (0.019) |
| 5 | 0.338 (0.023) | 0.340 | 0.481 | 0.480 (0.018) | +0.142 (0.008) | 0.404 (0.023) | +0.066 (0.010) | 0.510 (0.072) | +0.172 (0.029) |

Per seed at 3 tiles, A - off: +0.11 +0.19 +0.16 +0.12 +0.10 +0.21 +0.18 +0.13; C - off:
+0.18 +0.18 +0.28 +0.25 +0.12 +0.23 +0.17 +0.25.

Part of every gain is pressing more often: a tie or silence is "no move" and scores as a
miss, and coin flips score 0.48, not 0.5. So the press shares were taken
(`out/evidence/presses.sh`, `minimalprobe life` per seed, last tenth of the life):

| 3 tiles, seeds 5-12 | no move | bad | right among presses |
| --- | --- | --- | --- |
| off | 0.30 | 0.31 | 0.56 |
| A | 0.17 | 0.29 | 0.65 |
| C | 0.12 | 0.29 | 0.68 |

Counters both make the blob press more and make its presses more often right (0.56 to 0.65
or 0.68, against 0.5 for a guess). At 3 tiles this is the first reach the minimal engine has
shown. At 5 tiles A and C are level with coin: the gain there is mostly pressing.

### 2. How many pairs it holds

| cues | off | twin | coin | A | diff | B | diff | C | diff |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 4 (chance 0.25) | 0.408 (0.078) | 0.200 | 0.227 | 0.393 (0.056) | -0.015 (0.041) | 0.321 (0.065) | -0.087 (0.027) | **0.630** (0.071) | +0.222 (0.046) |
| 8 (chance 0.125) | 0.062 (0.014) | 0.100 | 0.112 | **0.213** (0.042) | +0.150 (0.016) | 0.097 (0.023) | +0.034 (0.008) | 0.187 (0.011) | +0.124 (0.006) |
| 16 (chance 0.0625) | 0.031 (0.005) | 0.061 | 0.053 | **0.096** (0.014) | +0.065 (0.006) | 0.049 (0.007) | +0.018 (0.004) | 0.087 (0.011) | +0.056 (0.005) |

- With counters on every neuron, echo k=4 goes 0.41 to 0.63 (every seed up, +0.03 to
  +0.47). The table engine's 0.68 is the only higher number on this task.
- With 8 cues A is right on 0.25 of its presses (no move 0.16, bad 0.62; chance 0.125),
  presses spread over all 8 buttons (0.07 to 0.19 each). It is the first engine above chance
  at 8 cues, and k=16 is above chance too. Far from held, but no longer nothing.
- Counters on the motor neurons alone (A) do not raise k=4.

### 3. Delayed echo, delays on (1..8)

| blank steps | off | twin | coin | A | diff | B | diff | C | diff |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 0 (ask right after the cue) | 0.629 (0.041) | 0.367 | 0.479 | **0.786** (0.037) | +0.157 (0.022) | 0.653 (0.071) | +0.024 (0.023) | 0.590 (0.122) | -0.039 (0.050) |
| 1 | 0.227 (0.011) | 0.304 | 0.472 | 0.360 (0.025) | +0.133 (0.007) | 0.257 (0.018) | +0.030 (0.008) | 0.363 (0.029) | +0.136 (0.010) |
| 2 | 0.236 (0.017) | 0.296 | 0.477 | 0.392 (0.033) | +0.156 (0.014) | 0.301 (0.033) | +0.065 (0.018) | 0.368 (0.038) | +0.132 (0.013) |

- With the ask right after the cue, A lifts the score from 0.63 to 0.79.
- With a gap the scores rise by 0.13 to 0.16 but stay well under coin, and the press shares
  say why: with 2 blank steps, off is right on 0.50 of its ask presses (no move 0.52 of
  asks), A on 0.49 (no move 0.21). The whole gain is answering more often. **Nothing learns
  delayed echo with a gap.**

### 4. Features nobody designed in (cue and buttons one tile apart)

| task | off | twin | coin | A | diff | B | diff | C | diff |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| order | 0.368 (0.017) | 0.394 | 0.477 | 0.421 (0.009) | +0.053 (0.006) | 0.401 (0.021) | +0.033 (0.011) | 0.425 (0.018) | +0.057 (0.011) |
| both-not-either | 0.259 (0.029) | 0.356 | 0.476 | **0.575** (0.038) | +0.316 (0.016) | 0.441 (0.088) | +0.182 (0.035) | 0.465 (0.030) | +0.206 (0.018) |
| gap | 0.427 (0.061) | 0.400 | 0.469 | 0.474 (0.012) | +0.046 (0.024) | 0.436 (0.040) | +0.009 (0.014) | 0.443 (0.017) | +0.016 (0.018) |
| order, delays 1..8 | 0.448 (0.020) | 0.383 | 0.477 | 0.462 (0.006) | +0.014 (0.007) | 0.441 (0.015) | -0.007 (0.011) | 0.385 (0.036) | -0.063 (0.016) |
| gap, delays 1..8 | 0.449 (0.038) | 0.380 | 0.469 | 0.456 (0.017) | +0.007 (0.012) | 0.452 (0.026) | +0.003 (0.012) | 0.407 (0.028) | -0.042 (0.018) |

- **Exclusive-or is learned in part with counters on the motor neurons.** 0.575 against coin
  0.476, every seed +0.26 to +0.39 over off. Press shares: off is right on 0.37 of its ask
  presses (worse than a guess), A on 0.66. A sum of the input lines with fixed signs cannot
  give exclusive-or, so the motor neurons are reading a feature the sheet makes; this is the
  first feature task any whole-blob rule has scored above coin on.
- Order and gap stay at or under coin, with or without delays.

### 5. Shuffled echo, straight and crossed lives, and the lock

Seeds 5-20 (16 lives: 9 crossed, 7 straight; which is which follows from the task seed, the
same rule the ablation doc used). Late score from `minimalprobe life` (it matches blobrun's
per seed).

| | straight lives | crossed lives | worst crossed life |
| --- | --- | --- | --- |
| off | 0.950 (0.016) | 0.690 (0.066) | 0.55 |
| A | 0.726 (0.036) | 0.713 (0.075) | 0.55 |
| B | 0.908 (0.094) | 0.726 (0.103) | 0.49 |
| C | 0.746 (0.056) | 0.687 (0.058) | 0.60 |

- **Counters do not fix the crossed lives**: 0.69 off, 0.71 A, 0.73 B, 0.69 C, all
  noise-sized differences. A and C make straight lives nearly as poor as crossed ones (the
  reflex cost again).
- **No life ends pressing one button.** In every life of off, A and B the late presses are
  split between the two buttons (each 0.30 to 0.53 of steps); C's presses were not counted,
  but a life pressing one button scores at most 0.5 and C's worst life is 0.60. The minimal engine's "lock" is
  a crossed life that answers right 55-80% of the time, not a blob stuck on one button.

### 6. Cartpole (lean reward, place lines, steps per fall)

| | off | twin | coin | A | B | C |
| --- | --- | --- | --- | --- | --- | --- |
| steps per fall | 17.3 (0.7) | 18.7 | 22.4 | 19.0 (0.8) | 20.7 (0.8) | 17.7 (0.5) |

B and A fall less often than off (+3.3 and +1.7, both well over 2 standard errors) but all
stay under coin flips. Nothing learns cartpole.

## 7. What it costs

### Tries to learn echo k=2

`out/evidence/sweep.sh out/evidence/tries.txt 5 12` (windows of 200 steps; the lives are the
same as with windows of 1,000), then `out/evidence/tries.py`: the first window at or above a
level.

| echo k=2, steps until a window reaches | 0.6 | 0.7 | 0.8 | late |
| --- | --- | --- | --- | --- |
| off | 500 | 1,050 | 1,600 | 0.942 |
| A | 275 | 1,500 | 2,675 (one seed 10,200) | 0.730 |
| B | 400 | 900 | 1,425 | 0.925 |
| C | 275 | 2,150 | 6,575 | 0.735 |
| echo k=4, steps to 0.4: off 4,225, C 600 | | | | |

A and C start faster (counters move every heard source at once) and then stall at about
0.73; they reach 0.8 only in lucky windows. B learns echo k=2 at the plain rule's pace. On
echo k=4, C gets to 0.4 seven times sooner than off.

### Long life and idle: nothing that worked got worse

`scripts/minimal.sh probe life --seed 5 [--steps 200000] <A or C flags>` (800,000 and
4,000,000 ticks of echo k=2; outputs in `out/evidence/life/`):

| | off, 800k (grow doc) | A, 800k | A, 4M | C, 800k | C, 4M |
| --- | --- | --- | --- | --- | --- |
| late echo score | 0.913 | 0.724 | 0.718 | 0.736 | 0.728 |
| connections per neuron | 48 (48-48) | 48 (48-48) | 48 (48-48) | 48 (48-48) | 48 (48-48) |
| threshold p10 / median / p90 / max | 3 / 5 / 13 / 25 | 3 / 5 / 9 / 18 | 3 / 5 / 9 / 20 | 2 / 4 / 6 / 19 | 2 / 4 / 7 / 18 |
| rate per 1024 (target 32) | 31.6 | 31.6 | 31.6 | 31.4 | 31.5 |
| silent / stuck | 0% / 0% | 0% / 0% | 0% / 0% | 0% / 0% | 0% / 0% |
| Fano, pulse | 139, weak | 134, 240 ticks 0.19 | 145, 240 ticks 0.10 | **48, none** | **51, none** |
| counters at the bottom / top | | 0.2% / 17.7% | 0.4% / 21.8% | 1.2% / 4.5% | 2.1% / 4.0% |

The budget holds exactly, the rate is on target, nothing is silent or stuck, and the score
does not drift between 800k and 4M ticks. With counters on every neuron the sheet is much
calmer (Fano 48 against 134-139, no swells), a side effect nobody asked for. Motor counters
at 6 bits still pile up at the top (18-22% of counters), which may be part of the echo2
ceiling.

`scripts/minimal.sh probe idle --seed N <flags>` for N in 5..12 (`out/evidence/idle.sh`):
teach echo k=2, test frozen 2,000 steps, 2M idle ticks unplugged, test again.

| | frozen score before | after | connections kept |
| --- | --- | --- | --- |
| off | 0.953 | 0.957 | 100.0% |
| A | 0.646 | 0.647 | 100.0% |
| C | 0.640 | 0.642 | 100.0% |

Idle is safe with counters: nothing moves without a reward or input. (The frozen scores of A
and C are 0.08 under their learning scores of 0.73; part of what they do while learning is
the counters moving step by step.)

### Speed and bytes

`scripts/minimal.sh probe speed [flags]`, one job at a time, same session, one core, capped
CPU with other agents running (load average about 9.5 on 32 threads), so these are good only
beside each other. Random input, a random reward every 50 ticks when learning is on.
Off was run first and last; the two passes agree within 3%.

| | 4,096 neurons, off / on | 65,536, off / on | 1,048,576, off / on | bytes per neuron |
| --- | --- | --- | --- | --- |
| counters off | 233,712 / 208,358 | 13,428 / 11,590 | 334 / 309 | 82 |
| A: motor neurons, 6 bits | 233,315 / 208,024 | 13,574 / 11,932 | 336 / 308 | 87 here, 114 in the echo sheet |
| every neuron, 4 bits | 231,389 / 53,536 | 13,503 / 3,354 | 342 / 117 | 358-375 |
| C: every neuron, 6 bits | 233,506 / 42,368 | 13,401 / 2,559 | 344 / 95 | 493-519 |
| every neuron, 8 bits | 232,528 / 31,961 | 13,388 / 1,945 | 338 / 72 | 628-663 |

- **The tick is not slower.** With learning off every setting runs at the same speed (the
  hot loop never reads a counter).
- **Motor neurons only costs nothing measurable here** (the speed sheet's motor region is
  small) and 24% in a real echo life, where a reward comes every 20 ticks: 168,889 against
  220,724 ticks/s (seed 5, blobrun's own timing).
- **Every neuron costs a lot in the reward path:** 4.9x slower with a reward every 50 ticks,
  5.8x in an echo life (37,916 ticks/s). The quiet mirror touches about half the sheet on
  every reward, and each touched neuron re-reads its whole mask (a binary search over 6
  compares of 9 x 6 words). Not optimised; see "what to try next".
- **Bytes:** a counter neuron holds 9 slots x bits x 8 bytes: 432 at 6 bits (5.3x the
  engine's 82). Motor neurons only (256 of 4,096 in the echo sheet) adds 32 bytes per neuron
  (27 counters, 4 the row index, the rest lines and per-tile words). The row index (4 bytes
  per neuron) is waste when every neuron has counters and could go.

## 8. Which part carries it

Seeds 5-12, learning on only, `out/evidence/sweep.sh out/evidence/parts.txt 5 12`. Diff is
paired against the engine with counters off (for the co-activity-off rows, see the note).

| variant | echo2 | echo4 | far3 f | delayed0 d |
| --- | --- | --- | --- | --- |
| off | 0.942 | 0.408 | 0.390 | 0.629 |
| A (motor, 6 bits) | 0.730 (-0.21) | 0.393 (-0.02) | 0.539 (+0.15) | 0.786 (+0.16) |
| A, 4 bits | 0.717 (-0.23) | 0.393 (-0.02) | 0.516 (+0.13) | 0.688 (+0.06) |
| A, 8 bits | 0.770 (-0.17) | 0.444 (+0.04) | 0.443 (+0.05) | 0.698 (+0.07) |
| A on every neuron (C) | 0.735 (-0.21) | **0.630 (+0.22)** | **0.597 (+0.21)** | 0.590 (-0.04) |
| A, moving line instead of top 48 | 0.788 (-0.15) | 0.415 (+0.01) | 0.521 (+0.13) | 0.795 (+0.17) |
| A, every counter above zero, no budget | 0.716 (-0.23) | 0.353 (-0.06) | 0.485 (+0.10) | 0.560 (-0.07) |
| A, co-activity off | 0.666 (-0.28) | 0.370 (-0.04) | 0.512 (+0.12) | 0.571 (-0.06) |
| A, co-activity swaps the mask | 0.728 (-0.21) | 0.407 (-0.00) | 0.526 (+0.14) | 0.780 (+0.15) |
| off, co-activity off | 0.806 (-0.14) | 0.395 (-0.01) | 0.362 (-0.03) | 0.494 (-0.14) |
| B (motor, 4 bits, noise spikes) | 0.925 (-0.02) | 0.321 (-0.09) | 0.468 (+0.08) | 0.653 (+0.02) |
| B, 6 bits | 0.955 (+0.01) | 0.412 (+0.00) | 0.450 (+0.06) | 0.628 (-0.00) |
| B, 8 bits | 0.948 (+0.01) | 0.402 (-0.01) | 0.476 (+0.09) | 0.697 (+0.07) |
| B on every neuron | 0.657 (-0.29) | 0.311 (-0.10) | 0.465 (+0.08) | 0.547 (-0.08) |
| B, co-activity off | 0.478 (-0.46) | 0.288 (-0.12) | 0.459 (+0.07) | 0.576 (-0.05) |

Standard errors of these diffs are 0.01 to 0.06 (full list in `out/evidence/parts.log` and
the pair helper); rows within 0.05 of each other on echo4 or delayed0 are not separable.

- **Grading every spike, not counters as such, carries the reach.** B (counters fed by
  noise spikes only) moves far3 by +0.06 to +0.09 at any width; A (counters fed by every
  neuron's latest spike) by +0.13 to +0.15. Counters with noise-only credit behave like the
  plain rule (echo2 0.93-0.96).
- **Every neuron, not motor only, carries echo k=4** (+0.22 against -0.02) and adds a little
  reach (+0.21 against +0.15), at the same echo2 cost. It loses the delayed0 gain (three
  seeds of eight at 0.45-0.48, spread 0.12).
- **Width:** 6 bits is best for A at far3 and delayed0; 4 bits is close; 8 bits loses most
  of the reach (+0.05), likely because evidence takes longer to cross the birth offset of
  8. With noise-only credit width does nothing measurable.
- **The budget read-off matters, the exact form less.** Without a budget (every counter
  above zero) A loses half its reach and its delayed0 gain; a moving line toward 48 is as
  good as the exact top 48, and kinder to echo2 (0.79 against 0.73, se 0.02).
- **Co-activity wiring is not what carries it.** With co-activity off in both, counters
  still add +0.15 at far3 (0.512 against 0.362) and cost echo2 0.14 (0.666 against 0.806).
  Co-activity helps delayed0 in both engines (off: 0.63 against 0.49).

## Predictions, scored

18. **Hidden evidence counters give the minimal engine reach (3 tiles with far slots, 0.37
    to above 0.6).** **Wrong as stated, right in direction.** 0.39 to 0.54 with counters on
    the motor neurons, to 0.597 with counters on every neuron (just under the bar), every
    seed up, and above coin (0.48); right among presses 0.56 to 0.65-0.68. First reach in
    this engine, and not free: echo k=2 falls 0.21.
19. **They raise how much it holds: echo k=4 from 0.41 to above 0.6, and something above
    chance at 8 cues.** **Right with counters on every neuron** (0.630; seeds 1-4 0.654), and
    8 cues are above chance in both A and C (0.21 and 0.19 against 0.125). Wrong for counters
    on the motor neurons only (k=4 0.39).
20. **They let it use the delay trace: delayed echo above 0.6 with delays on.** **Wrong.**
    0.39 (A) and 0.37 (C) with two blank steps, and right on 0.49 of ask presses: chance.
    Only the no-gap version improves (0.63 to 0.79).

## Reading

In plain words:

- **The wall moved, a little, for reach and for holding more, not for time.** Letting many
  rewards add up per source, and grading every neuron that spiked rather than only noise
  spikes, gets the minimal engine its first reach at 3 tiles, its first exclusive-or, k=4 to
  0.63 and 8 and 16 cues above chance. The thin signal is usable once evidence is summed.
- **It is a trade, not a free gain.** The same rule that reads the thin signal costs the
  one-tile reflex 0.2 and makes straight shuffled lives as poor as crossed ones. The noise
  rule's virtue (a noise spike is a clean experiment by one neuron) is exactly what is lost
  when every spike is graded, and the noise rule's vice (too few experiments to sum a weak
  signal) is what the counters cannot fix on their own: counters fed by noise spikes only
  change almost nothing.
- **Time stays the wall.** The delay trace reaches the motor neurons (grow doc) and counters
  still cannot turn it into the right answer after a gap; the gain on delayed echo with a gap
  is answering more often, at chance.
- The lock: no minimal-engine life presses one button while punished, with or without
  counters; crossed lives answer 55-80% right and counters do not change that.

## What this suggests for docs/findings.md (not edited here)

Main's findings moved while this ran (the readout work, idea 58, landed findings 14 and 15
and a new headline). Against that version:

- **Finding 14 (evidence before commitment), "a second test is running":** this is that
  test, and it agrees in a second engine and a different setting: the whole minimal sheet
  learning from reward (no error rule, no frozen sheet), with counters, reads the cue at 3
  tiles (0.39 to 0.60), holds 4 cues (0.41 to 0.63), gets exclusive-or above coin (0.26 to
  0.58), and lifts 8 and 16 cues above chance. It can go to solid for reach and capacity,
  with two limits worth writing in: counters fed only by noise-spike credit change nothing
  measurable (the credit feeding them has to be broad), and in this engine the gain costs
  the one-tile reflex 0.2 (0.94 to 0.73).
- **Finding 15 (the output must know which action was taken):** consistent. My best guess
  for the reflex cost is exactly that the reward rule here grades every motor neuron that
  spiked, including the losing button's; the readout's error rule does not have that
  problem. Not tested here.
- **Finding 11 (more bits of evidence, hint):** 6 bits is best, 4 close, 8 worse on reach
  (slower to cross the birth offset), and width does nothing when credit is noise-only.
  Width is not the main lever; summing at all is.
- **Finding 12 (one neuron's experiment almost never changes the answer):** a second engine
  agrees: noise-spike credit, even summed in counters, moves nothing past one tile.
- **Wall "Reading a thin signal: broken at the output, standing inside":** counters on every
  neuron (inside the sheet too) are what lift echo k=4 (+0.22, against -0.02 with counters
  on the motor neurons only), so evidence counters inside the sheet do help capacity under a
  reward rule. They do not help time.
- **Wall "Time":** unchanged and now tested with counters: delayed echo with a gap is right
  on 0.49 of ask presses with delays and counters on.
- **Wall "Locking on":** in the minimal engine no life, with or without counters, ends
  pressing one button while punished; crossed shuffled lives answer 55-80% right, and
  counters do not change them (0.69 off, 0.71-0.73 on).
- **Cartpole:** counters on the motor neurons (B) fall less often than off (20.7 against
  17.3 steps) but stay under coin flips (22.4).

## Rough

- Settings A and B were chosen on seeds 1-4 among about 40 rows; C was spotted on the report
  seeds in the ablation and only then run on seeds 1-4 (where it held). Treat C's echo4 as
  one clean result plus one replication, not two independent ones.
- The press-share tables come from `minimalprobe life` (last tenth of the life), not from
  blobrun's last 4 windows; scores differ in the second decimal.
- "Right among presses" for tasks whose trials span several steps is worked out from the
  per-step bad rate and the score; it assumes no move is never bad, which is how the tasks
  score.
- The learning-off twin was run once per row (with counters off); with counters on it is the
  same life by construction, and that was checked on one row only (far3 A).
- Speed numbers come from a shared, capped machine; only same-session comparisons mean
  anything.
