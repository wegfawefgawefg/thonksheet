# Growing the minimal engine (2026-11-24)

Idea 55 in `docs/ideas.md`; predictions 13 and 14 in `docs/predictions.md`. The minimal
engine (`src/minimal/`, `docs/results-2026-11-24-minimal-engine.md`) is the base. Pieces from
the full engine go back in one at a time and stay only if they earn their place on the test
they are for. Each piece is behind its own `--m.` number and off by default; off is
byte-identical to before (csv diffs below).

How every table here was run, unless it says otherwise: `scripts/minimal.sh seeds PRESET 5 12
[flags]` (seeds 5-12, one life each, 40,000 steps of 20 ticks, window 1,000; late = mean of
the last 4 windows; mean over the 8 lives with the sd in brackets). Settings were chosen on
seeds 1-4 first. "diff" is the paired difference to the piece off, seed by seed, with its
standard error; with 8 seeds anything under about 2 standard errors is noise. Helpers:
`out/grow/sweep.sh` (rows of named flags) and `out/grow/pair.py` (paired differences), both
in the worktree's `out/grow/`, which git ignores.

## Short version

| piece | switch | the test it is for | result | kept |
| --- | --- | --- | --- | --- |
| 0. co-activity wiring only while plugged in | `cowire_needs_input` 1 | idle | 0.953 to 0.957 after 2M idle ticks, all connections kept; task runs byte-identical | yes, default |
| 1. far slot | `far_slots` 1 or 2 | reach, 2-5 tiles | no change (3 tiles 0.370 off, 0.372 one slot, 0.390 two; coin 0.48); 12 bytes and ~25% speed per slot | no |
| 2. slot delays | `delay_max` 8 | delayed echo, beat | +0.22 on delayed echo with the ask right after the cue; nothing with a gap or on beat; -0.20 on echo k=2 and shuffled; 9 bytes, ~2.2x slower | no |

Both rejected pieces put the information where it is needed (the cue at the buttons, the
cue still there at the ask) and the reward rule does not use it. For reach and for time,
the bottleneck is credit.

## Piece 0: co-activity wiring only while plugged in, by default

`cowire_needs_input` now defaults to 1 (idea 56). Echo k=2 and k=4, seeds 5-12: every csv
byte-identical to the old default (0.942 and 0.408), as expected, since a task always has
lines attached. `--m.cowire_needs_input 0` gives the old behaviour. **Kept.**

## Piece 1: far slots

What it is. `--m.far_slots N` (0, 1 or 2) gives each neuron N extra slots after its 9 local
ones. Each slot gets a source tile at birth with chance `--m.far_percent` (sensor neurons
never), drawn as the full engine draws its far tiles: distance 2 + a draw below
`--m.far_reach` (3), extended by another such draw with chance 1/4 at a time, then a point
on the square ring at that distance; off the sheet means 8 tries, then no tile. A far slot
holds only excitatory senders (its mask is ANDed with the tile's non-inhibitory bits), so
its drive is one AND + popcount per neuron, gathered from the neuron's own tile word. It is
one more slot under the same budget of 48: birth wiring spreads the 48 over all allowed
slots, and every gain, loss and co-activity swap picks among all slots alike. Cost: 4 bytes
for the tile and 8 for the mask word per slot per neuron.

Presets for reach: far1-far3 as before; added far4 (`--g.sensor_y 10 --g.sensor_h 16`, cue
in tile row 2) and far5 (`--g.sensor_y 2 --g.sensor_h 16`, cue in tile row 1), both with the
buttons in tile row 6 (`--g.motor_y 48 --g.motor_h 16`), checked with `--show-plug`.

### Choosing on seeds 1-4 (learning on only)

`out/grow/sweep.sh out/grow/far_tune.txt 1 4 --no-baselines` (and `far_tune2.txt`). Late
score, mean of seeds 1-4.

| far slots | share | reach | far2 | far3 | far5 |
| --- | --- | --- | --- | --- | --- |
| 0 (off) | | | 0.424 | 0.376 | 0.357 |
| 1 | 15% | 3 | 0.422 | 0.386 | 0.361 |
| 1 | 50% | 3 | 0.429 | 0.386 | 0.365 |
| 1 | 100% | 3 | 0.414 | 0.379 | 0.349 |
| 1 | 50% | 5 | | 0.353 | 0.352 |
| 1 | 100% | 1 | | 0.399 | |
| 2 | 15% | 3 | | 0.363 | |
| 2 | 50% | 3 | | 0.367 | |
| 2 | 100% | 3 | | 0.406 | |
| 0, grading every recent spike (`grade_mode 1`) | | | | 0.436 | |
| 1, 15%, grading every recent spike | | | | 0.449 | |
| 1, 50%, grading every recent spike | | | | 0.457 | |
| 2, 50%, grading every recent spike | | | | 0.435 | |

Coin flips score 0.48 here. Nothing moves the score by more than the spread between seeds
(0.01 to 0.05). Grading every recent spike raises far3 to 0.44-0.46, with or without far
slots, but that is still under coin: as in the minimal-engine doc it makes the motors busier
(fewer no-moves), not right. 1 slot at 50% was taken to the report seeds as the best
plain row, and 2 slots at 100% as the most far wiring.

### Seeds 5-12

`out/grow/sweep.sh out/grow/far_report.txt 5 12`, i.e. for example
`TAG=f1p50 scripts/minimal.sh seeds far3 5 12 --m.far_slots 1 --m.far_percent 50`.

| task | off | off, learning off | 1 far slot, 50% | its learning-off twin | diff (on - off) | coin |
| --- | --- | --- | --- | --- | --- | --- |
| echo k=2 (1 tile) | 0.942 (0.023) | 0.451 (0.039) | 0.959 (0.018) | 0.384 (0.028) | +0.018 (se 0.009) | 0.472 |
| echo k=4 | 0.408 (0.078) | 0.200 (0.022) | 0.388 (0.039) | 0.177 (0.028) | -0.020 (se 0.032) | 0.227 |
| 2 tiles apart | 0.398 (0.023) | 0.370 (0.025) | 0.409 (0.037) | 0.356 (0.028) | +0.011 (se 0.010) | 0.478 |
| 3 tiles apart | 0.370 (0.023) | 0.360 (0.015) | 0.372 (0.031) | 0.346 (0.016) | +0.002 (se 0.010) | 0.481 |
| 4 tiles apart | 0.358 (0.011) | 0.358 (0.010) | 0.358 (0.017) | 0.363 (0.014) | +0.000 (se 0.005) | 0.481 |
| 5 tiles apart | 0.355 (0.014) | 0.359 (0.014) | 0.351 (0.015) | 0.354 (0.008) | -0.004 (se 0.006) | 0.481 |

Two far slots on every neuron (`--m.far_slots 2 --m.far_percent 100`):

| task | off | 2 far slots, 100% | its twin | diff | coin |
| --- | --- | --- | --- | --- | --- |
| 3 tiles apart | 0.370 (0.023) | 0.390 (0.035) | 0.335 (0.010) | +0.020 (se 0.012) | 0.481 |
| 5 tiles apart | 0.355 (0.014) | 0.338 (0.023) | 0.340 (0.019) | -0.017 (se 0.008) | 0.481 |

Per seed, 3 tiles, 1 slot: 0.35 0.34 0.41 0.37 0.41 0.34 0.41 0.35 (off: 0.35 0.35 0.39 0.40
0.39 0.34 0.36 0.38).

- **No reach.** At 2, 3, 4 and 5 tiles the far slot changes the score by -0.02 to +0.02,
  all inside 2 standard errors, and every row stays below coin flips. The best case, 2 far
  slots on every neuron at 3 tiles, is +0.02 (t 1.6) and 0.39 against coin's 0.48.
- **Echo k=2 at one tile: +0.018 (t 2.1),** on the line between noise and a small real
  gain; six seeds of eight up or level. Its twin is lower (0.38 against 0.45), so the far
  slot changes the born sheet. Echo k=4 -0.02, noise.

On the reflex tasks, for completeness (`out/grow/far_echo.txt`):

| | echo k=2 | echo k=4 | shuffled echo |
| --- | --- | --- | --- |
| off | 0.942 | 0.408 | 0.870 |
| 1 far slot, 50% | 0.959 (+0.018, se 0.009) | 0.388 (-0.020, se 0.032) | 0.854 (-0.016, se 0.013) |
| 2 far slots, 100% | 0.955 (+0.013, se 0.013) | 0.396 (-0.013, se 0.031) | 0.893 (+0.024, se 0.011) |

All within about 2 standard errors and of both signs: the far slot neither helps nor hurts
the reflex.

### Carry against credit

Does the far slot bring the cue to the buttons at all? `scripts/minimal.sh probe trace` (new,
`src/minimal/probe/probe_trace.cpp`) teaches for `--steps`, freezes learning, then runs
`--test-steps` more and asks, for every tick of a trial and every neuron, how well cue 0
and cue 1 can be told apart from that neuron's spike chance; the gaps (in units of the
spread, chance taken off) are summed over a group, as `blobprobe carry` does over tiles. It
is an ideal reader's number, an upper bound: 1.0 is easy to read, under 0.3 is lost. Here
on echo at 3 tiles, seed 5, 20,000 frozen test steps, per tick, averaged over the step's 20
ticks:

```
scripts/minimal.sh probe trace --task echo --k 2 --g.motor_y 48 --g.motor_h 16 --seed 5 \
    --task-seed 5 --test-steps 20000 --steps 40000 [--m.far_slots 2 --m.far_percent 100]
```

| | motor neurons, per tick | whole sheet, per tick | frozen score |
| --- | --- | --- | --- |
| off, as born (`--steps 1`) | 0.03-0.15 | 3.2-3.9 | 0.360 |
| 2 far slots 100%, as born | 0.21-0.32 | 2.5-2.9 | 0.334 |
| off, after 40,000 steps | 0.11-0.27 | 6.4-7.2 | 0.356 |
| 1 far slot 50%, after 40,000 steps | 0.30-0.97 | 6.8-8.5 | 0.378 |
| 2 far slots 100%, after 40,000 steps | 0.74-0.84 | 5.4-6.3 | 0.381 |

- **The far slot does carry.** Without it the cue is lost at the motor neurons (under 0.3
  per tick, before and after learning). With it the motor neurons read the cue 3 to 4 times
  better, and learning (co-activity wiring plus the reward rule) raises that further: 0.3
  at birth to 0.8 after a life with two slots. Over a 20-tick step that is plenty for an
  ideal reader.
- **The reward rule does not turn it into the right press.** The frozen score stays at
  0.36-0.38, the same as without far slots. The cue is at the buttons; which button fires
  more is not tied to it. At 3 tiles, with far slots, the problem is credit, not carrying.
  That matches the carry probe's reading for the full engine.

One seed; it is a mechanism check, not a score.

### Why the full engine reaches and this does not

The minimal-engine doc guessed that the full engine's 0.58 at 3 tiles comes from its far
links. Checked, on the full engine, seeds 5-12:

```
TAG=full scripts/minimal.sh seeds far3 5 12 --engine blob
TAG=fullnofar scripts/minimal.sh seeds far3 5 12 --engine blob --g.far_percent 0
TAG=fullnoredraw scripts/minimal.sh seeds far3 5 12 --engine blob --g.far_redraw_on 0
```

| full engine, 3 tiles apart | late | learning off | per seed |
| --- | --- | --- | --- |
| default (far links on 15% of excitatory neurons) | 0.575 (0.100) | 0.427 | 0.80 0.52 0.55 0.57 0.44 0.62 0.53 0.57 |
| no far links | 0.449 (0.012) | 0.427 | 0.47 0.43 0.46 0.43 0.46 0.45 0.45 0.45 |
| far links never redrawn | 0.549 (0.073) | 0.427 | 0.69 0.48 0.53 0.54 0.44 0.59 0.52 0.61 |

So the guess was right for the full engine: without far links it is back at its twin
(paired -0.13, 7 of 8 seeds down); redrawing empty far slots adds little (-0.03 when off).
Its far links are fixed at birth just like these. What differs is what learns on them: the
full engine strengthens every connection whose source and target both fired in the last 6
ticks on good (a whole chain at once, in strength steps of 1, 4, 5, into a potential that
integrates over ticks), and it starts each far slot with about 12 connections. The minimal
rule grades only neurons whose latest spike was noise, one swap each, so a two-hop chain
(cue to far relay to button) has to be found one link at a time by neurons that happened
to misfire, and here it is not found. Not tested further; it is the obvious next question
(below).

### Cost

`scripts/minimal.sh probe speed [flags]`, one job at a time, same session, capped CPU,
other agents running (load average about 10 on 32 threads), so only good beside each
other. Ticks per second with learning on (off in brackets); the full table with every
piece is `out/grow/speed2.txt`.

| | 4,096 neurons | 65,536 | 1,048,576 | bytes per neuron |
| --- | --- | --- | --- | --- |
| before this work (the old binary) | 205,035 (229,406) | 11,916 (13,392) | 313 (335) | 82 |
| pieces off (now) | 208,549 (231,327) | 11,996 (13,326) | 313 (347) | 82 |
| 1 far slot, 50% | 157,718 (173,869) | 9,127 (10,395) | 273 (294) | 94 |
| 2 far slots, 100% | 139,277 (152,432) | 8,379 (8,857) | 225 (234) | 106 |

With everything off the engine runs as fast as before (its own plain fire loop; a first
version that went through the general loop lost 18% and was replaced). One far slot costs
about 25% of the speed and 12 bytes per neuron; two, about 33% and 24 bytes.

### Verdict

**Not kept.** `far_slots` stays 0. It carries the cue to the buttons but the reward rule
does not use it, so it buys no score at 2-5 tiles; the +0.018 on plain Echo k=2 is at the
edge of noise and not what the piece is for.

## Piece 2: slot delays

What it is. `--m.delay_max D` gives every slot of every neuron one byte, its delay in ticks:
the slot reads the spike map that many ticks back (1 is the old reading). Local slots draw
the smaller of two draws in 1..D, so short delays are common, as the full engine does; with
`--m.delay_percent P` only P% of local slots draw one and the rest keep 1. A far slot's delay
is its distance in tiles times `--m.far_delay_per_tile` (1), capped at D. The fire loop
builds each source tile's column of the last D maps and picks each neuron's word with one
AVX-512 permute per 8 neurons (the full engine's trick); a slot whose whole column is empty
is skipped. Learning names a spike by its age and grades what the neuron heard at its own
delays, so "the sources active just before the spike" stays exact. The spike ring is 32
maps, so the credit window is capped at 31 - D ticks (20 is unaffected up to D = 11).

The tasks (`docs/curriculum.md`), 20 ticks per step:

- delayed echo, `--delay N`: a cue step (ticks 0-19), N blank steps, an ask step; only the
  press on the ask step counts. Presets delayed0 (N = 0, the ask right after the cue: the
  answer needs what was heard 1-20 ticks ago), delayed1 (N = 1, 21-40 ticks ago) and delayed
  (N = 2, the list's task, 41-60 ticks ago).
- beat, `--period N`: no cue, press on every Nth step. Balanced score, so always or never
  pressing is 0.5. Presets beat2 and beat4.

### Choosing on seeds 1-4

`out/grow/sweep.sh out/grow/delay_tune.txt 1 4` and `delay_tune2.txt`. Late score (its
learning-off twin in brackets). The second table was run after `delay_percent` was added,
which takes one more random draw per slot at birth, so its d8 rows differ a little from the
first.

| delays | delayed0 | delayed (N=2) | beat2 | beat4 |
| --- | --- | --- | --- | --- |
| off | 0.353 (0.332) | 0.255 (0.320) | 0.512 (0.486) | 0.508 (0.486) |
| 1..4 | 0.484 (0.354) | 0.232 (0.283) | 0.504 (0.491) | 0.504 (0.487) |
| 1..8 | 0.628 (0.376) | 0.237 (0.300) | 0.520 (0.486) | 0.508 (0.495) |
| 1..15 | 0.622 (0.407) | 0.230 (0.306) | 0.496 (0.489) | 0.501 (0.487) |

Coin: delayed0 0.478, delayed 0.491, beat 0.49.

| delays | echo k=2 | delayed0 |
| --- | --- | --- |
| off | 0.972 (0.467) | 0.353 (0.332) |
| 1..8, every slot | 0.732 (0.480) | 0.606 (0.381) |
| 1..8, half the slots | 0.792 (0.492) | 0.565 (0.360) |
| 1..8, a quarter of the slots | 0.887 (0.504) | 0.495 (0.346) |
| 1..4, every slot | 0.848 (0.492) | |
| 1..15, a quarter of the slots | 0.876 (0.497) | 0.486 (0.351) |

- **Delays let it learn delayed echo when the gap is within their reach.** With the ask
  right after the cue step (delayed0), 1..8 lifts the score from 0.35 to 0.61-0.63, about
  0.25 over its twin and well over coin, on every seed. 1..15 is no better than 1..8.
- **They do nothing for the list's delayed echo (two blank steps, 40 ticks)** and nothing for
  beat.
- **They cost Echo.** Every setting loses 0.08 to 0.24 on echo k=2. The fewer slots carry a
  delay, the smaller the loss and the smaller the gain on delayed0: a straight trade, no
  setting gets both. In Echo the cue changes every step, so a delayed slot spends the first
  ticks of each step hearing the previous cue.

1..8 on every slot was taken to the report seeds (it is the prediction's setting and the
best on delayed0).

### Seeds 5-12

`out/grow/sweep.sh out/grow/delay_report.txt 5 12`, e.g.
`TAG=d8 scripts/minimal.sh seeds delayed0 5 12 --m.delay_max 8`.

| task | off | off, learning off | delays 1..8 | its twin | diff (on - off) | coin |
| --- | --- | --- | --- | --- | --- | --- |
| delayed echo, ask right after (delayed0) | 0.410 (0.037) | 0.329 (0.014) | **0.629** (0.041) | 0.367 (0.031) | **+0.219** (se 0.025) | 0.479 |
| delayed echo, 1 blank step (delayed1) | 0.237 (0.009) | 0.295 (0.022) | 0.227 (0.011) | 0.304 (0.023) | -0.010 (se 0.007) | 0.472 |
| delayed echo, 2 blank steps (delayed) | 0.239 (0.009) | 0.300 (0.029) | 0.236 (0.017) | 0.296 (0.015) | -0.003 (se 0.008) | 0.477 |
| beat, every 2nd step | 0.496 (0.024) | 0.481 (0.012) | 0.521 (0.014) | 0.490 (0.009) | +0.025 (se 0.013) | 0.510 |
| beat, every 4th step | 0.506 (0.010) | 0.489 (0.006) | 0.502 (0.015) | 0.496 (0.006) | -0.004 (se 0.008) | 0.506 |
| echo k=2 | 0.942 (0.023) | 0.451 (0.039) | 0.738 (0.062) | 0.439 (0.051) | **-0.204** (se 0.020) | 0.472 |
| echo k=4 | 0.408 (0.078) | 0.200 (0.022) | 0.327 (0.063) | 0.198 (0.034) | -0.081 (se 0.047) | 0.227 |
| shuffled echo k=2 | 0.870 (0.105) | 0.392 (0.071) | 0.681 (0.124) | 0.382 (0.086) | **-0.188** (se 0.050) | 0.478 |

Per seed, delayed0 with delays: 0.62 0.64 0.65 0.72 0.57 0.60 0.63 0.61 (off: 0.44 0.35 0.37
0.41 0.46 0.46 0.40 0.39).

- **Delays earn something real on delayed0:** +0.22 (t 8.7), every seed up, 0.26 over its
  twin and 0.15 over coin. With the ask right after the cue step, a slot reading up to 8
  ticks back hears the cue during the first ticks of the ask step, and the reward rule
  wires that up.
- **Nothing on the list's delayed echo or one blank step** (21-60 ticks of gap), and nothing
  on beat (+0.025 on every 2nd step is 1.9 standard errors and still level with coin).
- **They cost the reflex:** echo k=2 -0.20 (t -10, every seed down), shuffled -0.19, echo
  k=4 -0.08 (noise-sized but the same sign).
- Learning makes delayed echo with a gap worse than its twin (0.24 against 0.30), with or
  without delays. A likely reason, not checked: at chance a press earns as much bad as
  good, silence earns nothing, so it drifts toward not pressing on the ask step, which the
  score counts as a miss.

### How long the cue survives, and where it is lost

`scripts/minimal.sh probe trace --task delayed-echo --delay 2 --seed 5 --task-seed 5
--test-steps 40000 --steps 40000 [--m.delay_max 8 | --engine full]` (`--steps 1 --window 1`
for as born). 10,000 frozen trials. Ideal-reader number per tick, averaged over 4-tick
bins: the whole sheet, then the 36 motor neurons. Cue on at ticks 0-19, blank 20-59, ask
60-79, reward after tick 79; the credit window covers ticks 60-79.

| ticks | off, as born | off, taught | delays 1..8, taught | full engine, taught |
| --- | --- | --- | --- | --- |
| 16-19 (cue on) | 6.5 / 1.8 | 15.9 / 5.4 | 18.7 / 6.5 | 13.0 / 4.3 |
| 20-23 | 4.4 / 1.0 | 6.7 / 2.3 | 17.2 / 6.5 | 11.7 / 3.5 |
| 24-27 | 2.7 / 0.3 | 1.2 / 0.3 | 11.8 / 5.2 | 7.7 / 2.1 |
| 32-35 | 2.0 / 0.4 | 0.7 / 0.0 | 8.0 / 3.7 | 6.6 / 1.8 |
| 40-43 | 1.3 / 0.2 | 0.1 / 0.1 | 5.5 / 2.5 | 5.6 / 1.7 |
| 52-55 | 0.4 / 0.0 | 0.3 / 0.1 | 3.3 / 1.5 | 3.3 / 0.9 |
| 60-63 (ask) | 0.3 / 0.1 | 0.2 / 0.0 | 2.5 / 1.1 | 2.9 / 0.9 |
| 76-79 | 2.0 / 0.4 | 1.0 / 0.3 | 1.6 / 0.5 | 1.4 / 0.3 |
| frozen score | 0.294 | 0.256 | 0.230 | 0.456 |

(sheet / motor). Full tables in `out/grow/trace_*.txt`.

- **Without delays the cue's trace in activity is gone about 20 ticks after the cue goes
  off** (sheet under 0.3 from tick 40, motor neurons from tick 24 once taught). Learning
  sharpens the cue while it is on (16 against 6.5) and makes it die faster. One tick of
  memory per hop, as built.
- **With delays 1..8 the trace lasts the whole gap.** The sheet still reads the cue at 2.5
  per tick when the ask comes, and the motor neurons at 1.1, falling to 0.5 by the end of
  the ask step: over the 20 ask ticks that is plenty for an ideal reader. Delay loops keep
  it echoing. The full engine (potential, leak, delays) holds a trace of the same size.
- **Yet neither presses right.** Delays score 0.23 on that trial, the full engine 0.46
  (coin 0.48). The cue is present, in the credit window, at the motor neurons; the rule does
  not turn it into the right button.
- A side find: without delays the cue comes back a little during the ask (0.3 to 2.0 on
  the sheet over ticks 60-79) after being gone for 20 ticks. The only state that can carry
  it is the thresholds: neurons that fired for the cue raised theirs and are still a little
  harder to fire when the ask line wakes the sheet. A fatigue memory, weak, not used.

So for delayed echo with a gap the missing piece is **credit, not memory**, once delays are
in: the trace survives into the ask step and into the credit window, and the reward rule
cannot pick it out. Without delays it is both: the trace is gone before the ask. A credit
rule that only reaches further back in time would not fix it by itself either, since the
answer has to be produced during the ask step, from whatever is still there.

### Cost

From the same speed session as piece 1: delays 1..8 on every slot run at 94,542 ticks/s
learning on at 4,096 neurons (99,107 off) against 208,549 (231,327) with the piece off,
about 2.2x slower; 5,476 against 11,996 at 65,536; 212 against 313 at 1M. 91 bytes per
neuron against 82 (one byte per slot). A first version that looked each delay up per
neuron ran at 65k; the AVX-512 permute from the full engine and skipping empty columns
brought it to 99k.

### Verdict

**Not kept as a default.** `delay_max` stays 1. Delays do earn their place on one timing
task, delayed echo with the ask right after the cue (+0.22), and they make the cue's trace
last through a 40-tick gap, but they cost the reflex 0.2 on echo k=2 and shuffled echo, and
they do nothing for the list's delayed echo or for beat. No setting tried (fewer delayed
slots, shorter or longer delays) keeps the gain without the loss. The switch stays for the
next credit experiment, where it is the memory half.

## Kept pieces together: the whole task list

Only piece 0 was kept, so "together" is the engine as it now stands by default
(`cowire_needs_input 1`, far slots and delays off). Seeds 5-12, `scripts/minimal.sh seeds
PRESET 5 12` (TAG `grown` for the last three: `out/grow/list.txt`). Cartpole now runs as
`--task cartpole --cart-reward lean --number-code place --sensor-side 2` (the `cartpole`
preset; `cartsparse` is the old one).

| task | now | learning off | coin | before this work |
| --- | --- | --- | --- | --- |
| echo k=2 | 0.942 (0.023) | 0.451 (0.039) | 0.472 | 0.942, csv identical |
| echo k=4 | 0.408 (0.078) | 0.200 (0.022) | 0.227 | 0.408, csv identical |
| echo k=8 | 0.062 (0.014) | 0.100 (0.011) | 0.112 | 0.062 |
| shuffled echo k=2 | 0.870 (0.105) | 0.392 (0.071) | 0.478 | 0.870 |
| delayed echo, delay 2 | 0.239 (0.009) | 0.300 (0.029) | 0.477 | 0.239 |
| catch 6x6 | 0.162 (0.015) | 0.167 (0.013) | 0.167 | 0.162 |
| cartpole, lean reward, place lines (steps per fall) | 17.3 (0.7) | 18.7 (1.6) | 22.4 | not run before; seed 5 csv identical to the old binary |

Nothing changed, as it should not: with a task plugged in, piece 0 does nothing, and the
other pieces are off. Cartpole with the new reward and lines learns nothing either (17.3
steps per fall, below its twin and below coin flips).

## Idle and long-life checks

`scripts/minimal.sh probe idle --seed N` for N in 5..12 (now with the default
`cowire_needs_input 1`): frozen score before the 2M idle ticks 0.953, after 0.957, 100.0%
of connections kept on every seed. The same as the switch gave in the minimal-engine doc;
the forgetting is fixed by default now.

`scripts/minimal.sh probe life --seed 5` (800,000 ticks), `--seed 6`, and `--seed 5 --steps
200000` (4M): late 0.913, 0.931, 0.918; every neuron at exactly 48 connections; thresholds
p10/median/p90 3/5/13, none at the cap; rate 31.6-31.8 against 32; nothing silent or stuck.
Identical to before.

The budget also holds with the pieces switched on (seed 5, 800,000 ticks): 2 far slots on
every neuron, every neuron at 48 (21% of them in far slots, 94% of neurons with a far tile;
late 0.993); delays 1..8, every neuron at 48 (late 0.588, the Echo cost again).

## Predictions, scored

13. **One far slot gives the minimal engine reach (3 tiles from 0.37 to above 0.55).**
    **Wrong.** 0.372 with one slot at 50%, 0.390 with two on every neuron, against 0.370
    off and coin 0.48; nothing at 2, 4 or 5 tiles either. The far slot does carry the cue to
    the buttons (motor readability 0.2 to 0.8 per tick), and far links are what give the
    full engine its 3-tile result (0.575, 0.449 without them), but the minimal reward rule
    does not use them.
14. **Delays let it learn delayed echo (beat its twin by more than 0.15).** **Wrong on the
    list's delayed echo** (delay 2: 0.236 against a twin of 0.296), **right in a weaker
    form** on delayed echo with the ask right after the cue (0.629 against a twin of 0.367,
    +0.26). The prediction's fallback was right: delays alone are not enough. But what the
    probe shows is not that the credit window is too short in time: with delays the cue's
    trace survives into the ask step and into the credit window, at the motor neurons, and
    still is not used. The missing piece is credit, not memory.

## Reading

- **Piece 0 is in:** co-activity wiring waits for input, idle forgetting is gone, nothing
  else moved.
- **Neither far slots nor delays earn a default place.** Far slots buy nothing at 2-5
  tiles. Delays buy +0.22 on the easiest delayed echo and cost 0.2 on the reflex.
- **Both did their mechanical job.** Far slots carry the cue to the buttons; delays keep
  it alive through a 40-tick gap. In both cases the information is where it needs to be,
  at the motor neurons, inside the credit window, and the score does not move. That puts
  the bottleneck in one place for both reach and time: **the reward rule cannot pick a thin,
  spread-out signal out of many sources.** At one tile the cue is 9 strong sensor bits next
  door and the rule finds it; anywhere else it is a weak bias over many neurons and it does
  not. This is the larger guess in `docs/predictions.md` ("teaching neurons deep inside the
  sheet from reward alone is the hard part") showing up at the motor neurons themselves.
- The full engine reaches 3 tiles with far links where this does not, and its rule differs
  in kind: it strengthens every recently co-active connected pair on good (chains at once,
  graded strengths, a potential that sums over ticks). That contrast is the cheapest lead.

## What to try next

- **A credit rule that can use a thin signal**, tried on far3 with far slots on and on
  delayed echo with delays on, since those two now have the information in place. The
  cheapest candidates, in the engine's style: (a) a motor neuron that fired on a good step
  swaps toward a source that was active *across* the step (an OR of the step's maps), not
  only the tick before one noise spike; (b) grade every recent spike of motor neurons only
  (grading all recent spikes everywhere made motors busy, not right); (c) a two-bit weight
  per connection (the full engine's strength levels) so a pair can be reinforced several
  times before the budget moves it.
- **Delays only where time matters.** The Echo loss comes from delayed slots hearing the
  previous cue. Try delays on the far slots only (`far_delay_per_tile` with local delays
  at 1), or on inhibitory neurons only, and check echo k=2 first.
- Use the threshold fatigue memory on purpose? It is already there and costs nothing; a
  probe on how long it lasts would say whether it is worth anything.

## Rough

- One trace seed (5) for each probe table; the readability numbers are an ideal reader's
  upper bound, not what the rule can get.
- The far slot is drawn once at birth and never moves. The full engine's redraw added
  only 0.03 at 3 tiles, so it was not built here.
- `far_percent` counts every non-sensor neuron, inhibitory ones too (their far slot still
  holds only excitatory senders); the full engine gives far slots to excitatory neurons
  only.
- The credit window cap (31 - delay_max) means delay_max 15 ran with a 16-tick window
  instead of 20.
- Speed numbers are from a capped, shared machine (load about 10); only the same-session
  comparisons above mean anything.
- An edit to `scripts/minimal.sh` while one of its runs was going made bash run a garbled
  last line after the lives had finished (an error on the summary only; the lives and their
  files were complete). Do not edit that script while it runs.
