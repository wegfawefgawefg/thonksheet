# The minimal engine (2026-11-24)

The full engine (`src/blob/`) has about 21 mechanisms. The owner agreed the elegant core is
probably three things and asked for a simpler design to be tried. This is it:
`src/minimal/`, run with `blobrun --engine minimal` or `scripts/minimal.sh`.

## Predictions

Written after the engine compiled and before any task was run on it.

1. **Echo k=2 at one tile apart: the minimal engine scores within 0.05 of the full engine**
   (mean late score over seeds 5 to 12). Confidence 50%.
2. **The noise-graded rule carries the result.** Replacing it with "grade everything
   recently active" costs more than 0.10 on Echo k=2; turning off the budget or wiring by
   co-activity each costs less than 0.05. Confidence 45%.
3. **It is at least 4x faster per tick than the full engine at 65,536 neurons on one core,
   with under half the bytes per neuron.** Confidence 75%.

## What it is

Plain words first, then the data.

- **Neuron.** 4,096 neurons on a 64x64 sheet, tiles of 8x8 (one 64-bit spike word per
  tile). A neuron hears its own tile and the 8 around it (576 possible sources) through
  one bit per source: connected or not. Each tick it counts excitatory spikes on its
  connections minus 4 x the inhibitory ones (from the previous tick only) and fires if
  the count reaches its threshold. No potential, no leak, no cooldown, no delays, no far
  links, no branches, no pacemakers, no sectors, no prediction learning, no surprise.
  25% of non-sensor neurons are inhibitory senders. Sensor neurons fire as the world says.
- **Keeping it alive.** On firing, a neuron raises its threshold by 1 with chance 1/16;
  every tick it lowers it by 1 with chance 1/512. Balance at 1 spike in 32 ticks. Nothing
  else.
- **Credit.** Noise is a sparse random word ORed into each tile's spikes, so the engine
  knows which spikes happened only because of noise (a second ring of 32 words per tile).
  A running average of reward (x256, an integer) is kept. When a reward arrives, every
  neuron whose latest spike in the last 20 ticks was a noise spike is graded: if the
  reward beat the average, it connects to one source that was active the tick before
  that spike; if worse, it disconnects from one. The chance of a grade halves each time
  the gap between reward and average halves, so a blob that is mostly right learns
  little from being right and a lot from being wrong. Neurons whose latest spike was
  driven by their inputs are not graded.
- **Budget.** Every neuron has a fixed number of connections. A gain costs a loss: a
  random connection whose source was not active (or, `--m.drop_mode 1`, one not marked
  useful since its last sweep, a clock version of least recently useful, which costs a
  second bit per source). A loss on bad is paid back with a random unwired, inactive
  source.
- **Wiring by co-activity** (switchable, `--m.cowire_on`). When a neuron fires, with
  chance 1/64 it swaps one connection for a source that was active the tick before,
  paying under the same budget. No reward involved.
- **Learning off** (the twin): no wiring change at all; thresholds and noise still run.
- **Switch added after the idle test** (`--m.cowire_needs_input 1`, off by default, so
  every number here was taken without it): co-activity wiring runs only while some sensor
  line is attached. See section 5.

## Keeping it alive: what threshold regulation alone does

Taken while settings were still being chosen (noise 2^-8); the same probe with the final
settings is in section 4. `scripts/minimal.sh probe alive` (fresh sheet, random bits on every sensor cell 1 in 8, or
`--input 0` for none; 30,000 ticks to settle, 65,536 measured; seed 1). Rate is spikes per
1,024 non-sensor neurons per tick, target 32.

| setting | input | rate | median / p99 / worst tick | Fano | pulse |
| --- | --- | --- | --- | --- | --- |
| inhibition gain 2, 24 connections | random | 31.5 | 15 / 318 / 584 | 401 | none |
| inhibition gain 2, 24 connections | none | 31.1 | - | 786 | none |
| inhibition gain 2, 192 connections | none | 34.2 | 4.5 / 997 / 1022 | 2550 | none |
| inhibition gain 4, 24 connections | random | 31.6 | 17 / 156 / 216 | 140 | 290 ticks, 0.11 |
| inhibition gain 4, 48 connections | random | 31.9 | 16 / 141 / 221 | 127 | 247 ticks, 0.11 |
| inhibition gain 4, 48 connections | none | 31.6 | 9 / 158 / 242 | 193 | 267 ticks, 0.17 |
| inhibition gain 4, 192 connections | random | 32.0 | 24 / 125 / 252 | 89 | none |
| full engine (default) | random | 33.1 | 31 / 72 / 107 | 25 | 24 ticks, 0.60 |

- Threshold regulation alone holds every neuron's average rate on target, with and
  without input, and leaves no silent or stuck neuron (0.0% both, every case above).
- With inhibition at gain 2 the sheet has seizures: quiet most of the time, then ticks
  where nearly the whole sheet fires. At gain 4 that stops: the worst tick is about a
  fifth of the sheet. That is a number, not a mechanism, so nothing was added.
- It is still bursty: slow swells (about 250 ticks, weak) where a tenth of the sheet
  fires for tens of ticks, then quiet. Fano 90 to 190 against the full engine's 25. The
  cause is threshold relaxation: thresholds are small integers (1 to 4 at 24 connections),
  they all drift down together until the sheet tips into a burst, the burst raises them,
  repeat. More connections make thresholds larger and finer and the swells weaker (Fano
  89 at 192, no measurable period).
- Tried and dropped: faster or slower regulation at the same balance (up/down shifts 1/6,
  2/7, 3/8: the period moves from 77 to 270 ticks, the swells stay); a one-tick refractory
  (a neuron that just fired cannot fire on input): no real change. Neither earned a place.

So the answer to "can one thing keep it alive" is yes, alive and not saturated, but not
calm. Whether the bursts matter for learning is not separately tested here.

## Settings chosen on seeds 1 to 4

`out/minimal/sweep.sh` style: Echo k=2, Echo k=4 and Echo with cue and button 2 tiles apart
(`far2`), seeds 1 to 4, late score (mean of the last 4 of 40 windows), learning on only.
Defaults then were budget 24, noise 2^-8, everything else as now.

| change | echo k=2 | echo k=4 | far2 |
| --- | --- | --- | --- |
| budget 24, noise 2^-7 | 0.68 | 0.26 | 0.46 |
| budget 24, noise 2^-8 | 0.86 | 0.29 | 0.44 |
| budget 24, noise 2^-9 | 0.98 | 0.31 | 0.43 |
| budget 48, noise 2^-7 | 0.78 | 0.28 | 0.45 |
| budget 48, noise 2^-8 | 0.94 | 0.32 | 0.44 |
| **budget 48, noise 2^-9 (chosen)** | **0.97** | **0.39** | **0.42** |
| budget 48, noise 2^-10 | 0.93 | 0.38 | 0.39 |
| budget 48, noise 2^-11 | 0.92 | 0.24 | 0.41 |
| budget 96, noise 2^-8 | 0.91 | 0.36 | 0.45 |
| budget 96, noise 2^-9 | 0.91 | 0.41 | 0.43 |
| budget 96, noise 2^-10 | 0.92 | 0.34 | 0.39 |
| budget 192, noise 2^-8 | 0.87 | 0.26 | 0.45 |
| budget 192, noise 2^-9 | 0.86 | 0.30 | 0.42 |
| chosen, grade chance 1/2 (learn_shift 1) | 0.95 | 0.24 | 0.43 |
| chosen, grade chance 1/4 | 0.90 | 0.21 | 0.44 |
| chosen, co-activity chance 2^-4 | 0.91 | 0.36 | 0.42 |
| chosen, co-activity chance 2^-8 | 0.94 | 0.40 | 0.39 |
| chosen, co-activity off | 0.79 | 0.37 | 0.39 |
| chosen, credit window 10 ticks | 0.97 | 0.37 | 0.44 |
| chosen, reward average over 2^3 | 0.95 | 0.41 | 0.43 |
| chosen, reward average over 2^8 | 0.89 | 0.32 | 0.43 |

Chosen: 48 connections, noise 2^-9, every graded neuron swaps (at the largest gap), wiring by
co-activity at 2^-6, credit window 20 ticks, reward average over about 32 rewards.
Spread over the four seeds was 0.005 to 0.05 on Echo k=2. Nothing learned at 2 tiles.

## 1. The task list

Seeds 5 to 12, one life each: 40,000 steps of 20 ticks (cartpole 100,000), window 1,000
steps, late = mean of the last 4 windows, mean over the 8 lives with the spread (sd) in
brackets. The full engine ran the same presets and seeds with its defaults.

```
scripts/minimal.sh seeds echo2 5 12          # likewise echo4 echo8 shuffled delayed catch cartpole
TAG=full scripts/minimal.sh seeds echo2 5 12 --engine blob --no-baselines
TAG=fullb scripts/minimal.sh seeds echo2 5 12 --engine blob    # full engine with its twin
```

| task | minimal | minimal, learning off | coin | full engine | full, learning off |
| --- | --- | --- | --- | --- | --- |
| echo k=2 | **0.942** (0.023) | 0.451 (0.039) | 0.472 | 0.874 (0.131) | 0.476 (0.157) |
| echo k=4 (chance 0.25) | **0.408** (0.078) | 0.200 (0.022) | 0.227 | 0.312 (0.077) | |
| echo k=8 (chance 0.125) | 0.062 (0.014) | 0.100 (0.011) | 0.112 | 0.094 (0.031) | |
| shuffled echo k=2 | **0.870** (0.105) | 0.392 (0.071) | 0.478 | 0.756 (0.329) | |
| delayed echo k=2, delay 2 | 0.239 (0.009) | 0.300 (0.029) | 0.477 | **0.438** (0.023) | |
| catch 6x6 | 0.162 (0.015) | 0.167 (0.013) | 0.167 | 0.161 (0.007) | |
| cartpole, steps per fall | 19.7 (0.9) | 19.9 (1.0) | 22.4 | 10.1 (1.2) | |

Catch and cartpole use `--sensor-side 2` (the bitmap and number rows do not fit at side 3).

- On Echo k=2, k=4 and shuffled echo the minimal engine beats the full one, and its
  seeds agree with each other (spread 0.02 against 0.13 on k=2; the full engine has two
  seeds at 0.63 and 0.67).
- Echo k=8: neither learns; the minimal engine ends below its own twin.
- Delayed echo: the minimal engine cannot do it and learning makes it worse than its twin
  (0.24 against 0.30). It has no memory longer than one tick: no potential, no delays.
  The answer has to be given 2 steps (40 ticks) after the cue is gone. The full engine is
  near chance too (0.44 against coin 0.48), so neither has a working memory here.
- Catch and cartpole: nothing learns in either engine. The full engine's cartpole is
  worse than coin (10 steps per fall against 22).

## 2. Reach: cue and button 1, 2, 3 tiles apart

How the lines were placed: the plug packs the cue lines in the bottom row of the sensor
area (cells y 24 to 28, tile row 3) and the buttons in the top row of the motor area. The
motor area is moved down by genome rect: `--g.motor_y 32 / 40 / 48 --g.motor_h 16` puts
the buttons in tile row 4, 5 or 6. Each cue's nearest button tile is then 1, 2 or 3 tiles
away (Chebyshev distance between tiles; horizontally the nearest pair is at most one
tile apart in all three, so the distance is the row gap). At 1 tile a motor neuron hears
the cue tile directly; at 2 it needs one relay; at 3, two. Presets far1, far2, far3.

| tiles apart | minimal | minimal, learning off | coin | full engine | full, learning off |
| --- | --- | --- | --- | --- | --- |
| 1 | **0.942** (0.023) | 0.451 (0.039) | 0.472 | 0.874 (0.131) | 0.476 (0.157) |
| 2 | 0.398 (0.023) | 0.370 (0.025) | 0.478 | 0.507 (0.130) | 0.438 (0.051) |
| 3 | 0.370 (0.023) | 0.360 (0.015) | 0.481 | **0.575** (0.100) | 0.427 (0.027) |

(far1 is byte-identical to echo2 for the minimal engine, as it should be: the motor rect
height does not move the lines.)

- **The minimal engine is a one-tile reflex and nothing more.** At 2 and 3 tiles it is
  within 0.03 of its twin, on every seed.
- **The full engine reaches further.** At 3 tiles it gains 0.15 over its twin (one seed
  at 0.80, the rest 0.44 to 0.62); at 2 tiles two seeds of eight learn (0.79, 0.64). It
  has far links (typical reach 3 tiles) and delays; the minimal engine has neither. That
  it does better at 3 tiles than at 2 fits the far links' reach.
- So the minimal engine is worse than the full one at distance. That is where it loses.

## 3. Which piece carries it

Each piece off in turn, seeds 5 to 12, learning on only (the twins are the same as above).
`TAG=nobudget scripts/minimal.sh seeds echo2 5 12 --no-baselines --m.budget_on 0`, and
`--m.cowire_on 0`, `--m.grade_mode 1`, `--m.drop_mode 1`, `--m.learn_shift 30`.

| variant | echo k=2 | echo k=4 | shuffled | 2 tiles apart |
| --- | --- | --- | --- | --- |
| all three pieces (default) | **0.942** (0.023) | **0.408** (0.078) | **0.870** (0.105) | 0.398 (0.023) |
| budget off (gains and losses unpaired) | 0.761 (0.062) | 0.235 (0.025) | 0.636 (0.137) | 0.420 (0.024) |
| wiring by co-activity off | 0.806 (0.042) | 0.395 (0.032) | 0.707 (0.110) | 0.376 (0.013) |
| grade every recent spike, not only noise | 0.956 (0.018) | 0.317 (0.016) | 0.779 (0.209) | 0.472 (0.016) |
| budget paid with least recently useful | 0.862 (0.096) | 0.375 (0.033) | 0.789 (0.158) | 0.311 (0.016) |
| reward rule off (co-activity only) | 0.625 (0.194) | 0.238 (0.072) | 0.419 (0.268) | 0.396 (0.066) |
| learning off | 0.451 (0.039) | 0.200 (0.022) | 0.392 (0.071) | 0.370 (0.025) |

- **The budget carries the most.** Without it the score falls on every reflex task: 0.18
  on k=2, 0.17 on k=4 (to chance), 0.23 on shuffled.
- **The reward rule is necessary but not sufficient alone.** With co-activity only, seeds
  scatter from 0.37 to 0.89 on k=2: co-activity amplifies whatever cue-to-button bias a
  seed is born with, right or wrong. The reward rule picks the right one.
- **Wiring by co-activity helps on two tasks of three**: 0.14 on k=2 and 0.16 on shuffled,
  nothing on k=4.
- **Grading only noise spikes is the smallest piece.** On k=2 grading every recent spike
  does as well (0.956 against 0.942). It costs 0.09 on k=4 and on shuffled (the latter
  with a spread of 0.21). At 2 tiles grading everything gives 0.47 against a twin of 0.37,
  but coin is 0.48: it makes the motors busier (fewer no-moves), not right.
- **Least recently useful is worse than random** for paying the budget: 0.08 lower on k=2,
  0.09 lower at 2 tiles, and it costs a second bit per source. Random stays.

So prediction 2 is wrong: the noise-graded rule is not what carries the result. Ranked by
what the score loses when each is taken out: budget, then wiring by co-activity, then the
noise restriction. All three earn their place on at least one task; none is sufficient.

## 4. Saturation and aliveness after a long life

`scripts/minimal.sh probe life --seed 5` (Echo k=2, 800,000 ticks; `--steps 200000` for
4,000,000) and `scripts/minimal.sh probe alive [--input 0]`, each also with
`--engine full`. The last 65,536 ticks of the life are measured. Silent: firing under 1/16
of the target rate. Stuck: firing more than half the ticks.

| | minimal, 800k | minimal, 4M | minimal, seed 6, 800k | minimal, budget off, 800k | full, 800k |
| --- | --- | --- | --- | --- | --- |
| late Echo score | 0.913 | 0.918 | 0.931 | 0.798 | 0.902 |
| connections per neuron (median, range) | 48 (48-48) | 48 (48-48) | 48 (48-48) | 429 (191-534) of 576 | 96 (65-96) |
| neurons at connection cap | (budget) | (budget) | (budget) | - | 99.0% |
| connections at top strength | (one bit) | (one bit) | (one bit) | (one bit) | 95.0% |
| threshold p10 / median / p90 / max | 3 / 5 / 13 / 25 | 3 / 5 / 13 / 24 | 3 / 5 / 13 / 23 | 4 / 7 / 16 / 28 | 7 / 49 / 64 / 64 |
| thresholds at cap | 0% | 0% | 0% | 0% | 18.7% |
| rate per 1024 (target 32) | 31.6 | 31.8 | 31.6 | 31.9 | 47.1 |
| silent / stuck | 0% / 0% | 0% / 0% | 0% / 0% | 0% / 0% | 1.5% / 0% |
| Fano, pulse | 139, none to speak of (40 ticks, 0.10) | 148, 240 ticks 0.10 | 164, 39 ticks 0.14 | 162, none | 26, 60 ticks 0.06 |
| connections changed over the life | 1.9M gained, 1.9M lost | 9.1M / 9.1M | 1.9M / 1.9M | 1.6M / 0.2M | |

Fresh sheets, settled 30,000 ticks then 65,536 measured, seed 1:

| | rate | median / p99 / worst tick | Fano | pulse | silent / stuck |
| --- | --- | --- | --- | --- | --- |
| minimal, random input | 31.8 | 13 / 149 / 242 | 158 | 265 ticks, 0.11 | 0% / 0% |
| minimal, no input | 31.6 | 6 / 170 / 263 | 242 | 265 ticks, 0.24 | 0% / 0% |
| full, random input | 33.1 | 31 / 72 / 107 | 25 | 24 ticks, 0.60 | 0.1% / 0% |
| full, no input | 32.6 | 31 / 77 / 109 | 32 | 24 ticks, 0.64 | 0% / 0% |

- **The minimal engine does not saturate, by construction.** Every neuron holds exactly
  48 connections after 4 million ticks, while about 9 million connections came and went.
  Its thresholds sit well inside their range (median 5, none at the cap). It is on its
  target rate to within 1%, nothing is silent or stuck, and the score at 4M ticks is the
  same as at 800k.
- **Without the budget it saturates like the full engine:** a median of 429 of the 576
  possible sources wired after 800k ticks, against the full engine's 99% of neurons at
  their cap and 95% of connections at top strength.
- **The full engine, during Echo, overshoots its target** (47 against 32) with 19% of
  thresholds at the cap. The minimal engine does not.
- **The full engine pulses (24 ticks, strength 0.6) and the minimal engine does not**, but
  the minimal engine is burstier: its spike count per tick swings 5 to 8 times more
  (Fano 140 to 240 against 25 to 32). In a life the 40-tick "pulse" is the task's own
  two-step rhythm, and weak.
- Least recently useful (`--m.drop_mode 1`, seed 5): late 0.672 and much burstier (Fano
  630, worst tick 52% of the sheet). Another reason random stays.

## 5. Idle: teach, unplug, retest

`scripts/minimal.sh probe idle --seed N` (and `--engine full`): teach Echo k=2 for 800,000
ticks, test 2,000 steps with learning frozen, unplug every line and run 2,000,000 ticks
with learning on and nothing written or rewarded, plug back in, test 2,000 steps frozen
again. Seeds 5 to 12, means. "Kept" is the share of connections present before the idle
stretch that are still there after it.

| | frozen score before | frozen score after | connections kept |
| --- | --- | --- | --- |
| minimal (default) | 0.953 | **0.333** | 10.8% |
| minimal, co-activity only while plugged in (`--m.cowire_needs_input 1`) | 0.953 | **0.957** | 100.0% |
| minimal, co-activity 16x slower (`--m.cowire_shift 10`) | 0.867 | 0.566 | 33.3% |
| full engine | 0.840 | 0.858 | 98.5% (92.8 to 100) |

- **The minimal engine as built forgets everything when idle.** Wiring by co-activity
  keeps swapping connections at the same rate with nothing plugged in, and with no reward
  nothing anchors what was learned: after 2M ticks about 11% of the taught connections
  remain, close to the 8% a fresh random draw of 48 from 576 would share. The score falls
  below its twin (no-moves). Every one of the 8 seeds.
- **The full engine keeps it,** because its churn (growth and pruning) never touches strong
  connections, and 95% of its connections are strong.
- **Fix, measured:** co-activity wiring runs only while at least one sensor line is
  attached. That is the design doc's own rule ("change needs a reason", idle is the quiet
  phase), costs one OR over the tile words per tick, and changes nothing while a task is
  plugged in (the before scores are identical, seed by seed). With it the engine keeps
  everything. It is a switch, off by default, so every number above was taken without it.
  It makes idling trivially safe by not changing anything; it does not show that the
  engine could keep learning unsupervised while idle without forgetting. Slowing
  co-activity down instead costs learning and still loses two thirds.

## 6. Speed and size

`scripts/minimal.sh probe speed` and `scripts/minimal.sh probe speed --engine full`, one
job at a time, same session, one core, random bits on the sensor cells (1 in 8) and a
random reward every 50 ticks (learning on). The CPU is capped at about 4.6 GHz and other
agents were running (load average about 10 on 32 threads), so these numbers are only good
beside each other. Two passes agreed within 2%; the first is shown.

| neurons | minimal, learning off | minimal, on | full, off | full, on | minimal / full (on) |
| --- | --- | --- | --- | --- | --- |
| 4,096 | 231,072 | 206,393 | 64,004 | 44,439 | 4.6x |
| 65,536 | 13,347 | 11,790 | 3,535 | 2,880 | 4.1x |
| 1,048,576 | 337 | 311 | 127 | 93 | 3.3x |

Ticks per second. With learning off the ratio is 3.6x, 3.8x, 2.7x.

| | minimal | full |
| --- | --- | --- |
| bytes per neuron | 82 (72 mask, 1 threshold, 8 for the two 32-deep rings, the rest per tile) | 208 (176 for two weight planes, 11 delays, 8 far tiles, 9 per-neuron state, ring) |
| bytes per neuron with least recently useful | 155 | |
| AND+popcounts per neuron per tick | 18 (9 slots x excitatory, inhibitory) | 44 plus a delay pick per slot |
| lines of code, engine only (no blank or comment-only lines) | 543 in `src/minimal/` | 1,047 in `src/blob/` |
| mechanisms | 4 (popcount neuron, threshold regulation, noise-graded swaps under a budget, co-activity swaps) | about 21 |

At 1M neurons both fall out of cache (75 MB of masks against 218 MB of weights), and the
gap narrows to 3.3x.

## Reading

In plain words:

- **On the one thing the full engine does well, the reflex, the minimal engine does it
  better with a fifth of the mechanisms, 4x the speed and 40% of the memory.** Echo k=2
  0.94 against 0.87, Echo k=4 0.41 against 0.31, shuffled echo 0.87 against 0.76, and
  much steadier across seeds. That is the headline, with the limits below.
- **It does nothing beyond a one-tile reflex.** At 2 and 3 tiles it does not learn; the
  full engine learns a little at 3 tiles, very likely through its far links. Delayed echo,
  catch and cartpole: no learning in either engine, and the minimal one has no memory past
  one tick at all.
- **The budget is the piece that matters most**, not the noise rule. Fixed fan-in with
  swaps is what keeps the reflex learnable on more cues and keeps the sheet from
  saturating; without it the engine saturates (429 of 576 sources wired) and scores drop
  by about 0.2. Wiring by co-activity is second. Grading only noise spikes helps on k=4
  and shuffled but not k=2.
- **Threshold regulation alone keeps it alive** (rate on target, nothing silent or stuck,
  no saturation) once inhibition is strong enough (gain 4), but it is bursty.
- **Idle exposed a real flaw**: unanchored co-activity wiring erases learning when
  nothing is plugged in. Gating it on input fixes that at no cost.

## Predictions, scored

1. Echo k=2 within 0.05 of the full engine: **wrong**, in the good direction: 0.942
   against 0.874, 0.07 better.
2. The noise-graded rule carries it: **wrong**. Grading everything recently active does as
   well on k=2 (0.956 against 0.942) and loses 0.09 on k=4 and shuffled; the budget carries
   the most (0.18 to 0.23 lost without it), and co-activity wiring loses 0.14 on k=2.
3. At least 4x faster per tick at 65,536 neurons with under half the bytes: **right with
   learning on** (4.1x, 82 against 208 bytes), **just short with learning off** (3.8x).

## What to try next

- Reach is the gap. The cheapest test is one far slot (a tenth word per neuron, one source
  tile drawn at birth) under the same budget, to see whether the full engine's 3-tile
  result comes from far links.
- A memory longer than a tick for delayed echo: the smallest is a potential that keeps the
  last tick's drive (one shift), measured against delayed echo only.
- Make `cowire_needs_input` the default, then test whether it can keep learning on
  reward-free input (input plugged in, no reward) without forgetting.
- The bursts: try a per-tile inhibition proportional to the tile's last spike count (one
  popcount per tile) only if bursts turn out to hurt something measurable.
- Echo k=8 fails in both engines; with 8 buttons a step's 20 ticks of noise rarely find
  the right one. Worth one run with 40 ticks per step before anything else.

## Rough

- Echo is a reflex test; the minimal engine was tuned on it (seeds 1-4), the full engine
  was tuned by its owner on the same task. Neither was tuned on catch or cartpole.
- The full engine's twin was run only on Echo k=2 and the two far presets; elsewhere the
  minimal engine's twin and coin are the baselines.
- The scratchpad folder is shared with other agents in this session; a `sweep.sh` of mine
  there overwrote one with the same name and another agent then ran it with its own tags
  (its runs failed at once, empty output). My sweep helpers now live in `out/minimal/`.
