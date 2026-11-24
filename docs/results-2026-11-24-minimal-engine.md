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

## Keeping it alive: what threshold regulation alone does

`scripts/minimal.sh probe alive` (fresh sheet, random bits on every sensor cell 1 in 8, or
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
