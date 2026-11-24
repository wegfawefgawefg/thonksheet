# Core notes

Handoff for `src/blob/` and `src/bench/`, first blob effort (2026-11-24).

## Files

- `blob/birth.cpp`: `blob_birth`, `blob_free`, `blob_reward`, far tile draw, fan-in count.
- `blob/fire.cpp`: integrate and fire, the hot loop.
- `blob/regulate.cpp`: sector bias, per-neuron thresholds, surprise counts.
- `blob/learn_signal.cpp`: sensor prediction learning, the good/bad broadcast.
- `blob/learn_wiring.cpp`: growth, pruning, far slot redraw.
- `blob/tick.cpp`: `blob_tick`, the order of the steps.
- `blob/random_words.hpp`: xoshiro256**, sparse words, `pick_bit`.
- `blob/weight_levels.hpp`: one-level moves on the two bit planes.
- `blob/tick_steps.hpp`: core-only declarations, `slot_source`, `slot_read`.
- `bench/`: `blobbench` with three modes, plus `--g.name value` to override any genome number.

```
blobbench [speed] [--tiles 8] [--learn 0|1] [--seconds-ms 400] [--g.name v ...]
blobbench stage0 [--ticks 1000000] [--window 50000] [--csv 1] [--g.name v ...]
blobbench learn  [--layout 0..3] [--cues 2] [--lines 8] [--group 8] [--window 20]
                 [--gap 10] [--trials 20000] [--report 2000] [--seed 1] [--learn 0|1]
                 [--spread 1] [--g.name v ...]
```

`stage0` writes `stage0_sectors.csv` (phase, tick, sector, spikes per tick per 1,024, bias)
into the current directory.

## Header changes (blob.hpp, genome.hpp)

- Weights are slot-major inside a tile. Index with `blob_weight_index(neuron, slot)`;
  `blob_weight` uses it. The old `w0[neuron * SLOTS + slot]` is gone.
- `far_delay` replaced by `slot_delay`, one per neuron per slot at the same
  `blob_weight_index`. A tick reads `spikes[(now + 1 - delay) & 15]` for that slot.
- The 16 spike maps are one block, `spike_stride` words apart; `spikes[h]` still points at
  map h. `blob_free` frees the block through `spikes[0]`.
- New Blob fields: `pacemaker` (word per tile), `sector_target`, `surprise_level`,
  `spike_stride`.
- New functions: `blob_detach(blob, neuron)`, `blob_detach_all(blob)`,
  `blob_weight_index(neuron, slot)`.
- `stats.surprise`: attached sensor neurons whose prediction was wrong, on the latest tick.
- Genome added: `sensor_stride`, `motor_stride`, `local_delay_min/max`, `learning_on`,
  `pacemaker_period_min/max`, `far_connect_shift`, `threshold_cap`,
  `sector_smooth_shift`, `prune_bit_shift`.
- Genome defaults changed: sensor rect 0,0,32x32 stride 2; motor rect 32,32,32x32 stride 2;
  `inhibitory_percent` 20 -> 25; `inhibit_gain` 2 -> 5; `sector_target_per_1024` 40 -> 32;
  `reward_shift` 2 -> 4.

## Speed

`blobbench speed`, one core (`taskset -c 15`), Ryzen 9950X, gcc 13 -O3 -march=native.
"on" feeds random input to every sensor line and rewards every 50 ticks.

| neurons | delays 1..4, off | on | all delays 1, off | on |
|---|---|---|---|---|
| 1,024 | 295,938 | 210,999 | 383,483 | 255,312 |
| 4,096 | 73,812 | 51,557 | 90,626 | 59,097 |
| 16,384 | 16,312 | 12,724 | 21,097 | 15,330 |
| 65,536 | 4,069 | 3,148 | 5,069 | 3,763 |

That is about 2.7e8 to 3.9e8 neuron updates per second with learning off. History of the
4,096 number (learning off): neuron-major masks 16,510; slot-major + packed flags 93,485;
varied delays with select passes 51,693; varied delays with an AVX-512 permute 70,146 (the
only intrinsics, with a plain fallback). The hot loop is vectorized by the compiler
(VPOPCNTQ across a tile's 64 neurons).

## Stage 0

`blobbench stage0`, 4,096 neurons, default genome, seed 1: 1M ticks idle, 1M with random
input (each sensor fires with chance 1/8 per tick), 1M with input in bursts of 200-5,200
ticks and unplugged gaps of the same length. 44.5 s.

- Activity per sector, window means: 30 to 36 spikes per tick per 1,024 (target 32). No
  sector ever silent, no tick with more than a quarter firing, no tick with none firing.
- Fano factor of the per-tick spike count: 21 to 36 (1 would be independent firing). Some
  shared rhythm, no bursts.
- Short loops: overlap of the spike map with the map 2 to 15 ticks earlier peaks at lag 5,
  3.4% to 3.9%. Independent firing at this rate gives about 1.6%. No loop takes over.
- Idle: strong connections untouched (15.05 per neuron for the whole million ticks), weak
  ones settle at about 43 per neuron with growth and pruning balanced (about 188k each per
  250k ticks). Nothing strengthened, nothing weakened while idle.
- With input, prediction learning moves 225k up and 310k down per 250k ticks, and strong
  connections drop to 14.4 per neuron.

Changes to get there:

- `inhibit_gain` 2 -> 5 and `inhibitory_percent` 20 -> 25. With 2 and 20 the blob burst:
  fano 340 to 590, 2% to 5% of ticks with over a quarter firing, the same 17% of neurons
  firing again 4 ticks later. Inhibition times its share has to roughly match excitation.
- `sector_target_per_1024` 40 -> 32. The per-neuron threshold rule balances at
  1024 * 2^(up - down) = 32. With 40 the sector bias sat pinned at -16 and the real rate
  was 31 to 34 anyway; at 32 the two agree and the bias moves.
- Pruning takes each idle weak connection with chance 2^-prune_bit_shift instead of all of
  them, so its loss scales with the number of idle weak connections and balances growth.

## Learning check

`blobbench learn`: K cues, each held on 8 random sensor cells for a 20-tick window; K groups
of 8 random motor cells; good when the group that matches the cue fired most in the window
(ties broken by coin), bad otherwise; 10 quiet ticks after. Which group matches which cue is
shuffled per seed. No born-in wiring between them. The same genome and seed run with
`learning_on = 0` is the baseline. Numbers are hit % over the last two report windows.

| setup | seeds | trials | learning on | off |
|---|---|---|---|---|
| layout 1: 1,024 neurons, sensor tile row right next to the motor tile row, 2 cues | 6 | 16k | 89.9 | 48.1 |
| same, 4 cues (chance 25) | 3 | 16k | 75.1 | 26.7 |
| layout 2: one empty tile row between them, 2 cues | 3 | 40k | 79.0 | 48.8 |
| layout 2, all local delays 1 | 3 | 40k | 68.7 | 46.2 |
| layout 3: 4,096, default areas, lines and groups from the 16x16 corners where the areas touch | 6 | 20k | 61.4 | 53.5 |
| layout 0: 4,096, default areas, lines and groups anywhere in them | 3 | 60k | 51.6 | 47.4 |

One curve, layout 1, seed 2 (learning off stays at 32-40 because the born-in wiring happens
to favour the wrong group):

```
trials   2000  4000  6000  8000 10000 12000 14000 16000
on       63.3  75.9  89.5  94.0  88.0  94.1  93.3  91.2
off      34.3  34.5  35.7  33.7  31.6  34.8  39.9  35.5
```

So: it learns over one and two hops, clearly, on every seed but one at one hop (seed 5:
59.8 vs 33.5). Across the default areas, where a cue and a motor group are often 3 to 6
tiles apart, it does not learn in 60k trials. In the corner setup it is mixed: one seed of
six clearly (92.7 vs 69.5), the rest within a few points.

`--spread 1` shows why: per tile, how much its neurons' rate differs between cue 0 and cue
1. With learning off the difference is about 20 spikes per 1,000 ticks inside the sensor
area, 7 one tile out and 1 to 2 (noise) at the motor area. Cue information fades by about 3x
per tile.

What was tried on the default layout (12k trials, 3 seeds, all 50 to 55%, no effect):
`far_percent` 50, `sector_target_per_1024` 16, `recent_ticks` 8, `reward_ticks` 8,
`reward_shift` 6, all delays 1, groups of 32, 32 lines per cue.

The one rule change that mattered: bad used to take connections down to 0. The up/down walk
then had an absorbing floor, and the busiest pairs (the paths out of the cue) were cut
first; with learning on, the cue spread near the sensors fell from about 20 to 4. Now bad
stops at strength 1 and the spread rises to 30 to 55 near the sensors with learning on.

## Rough or unsolved

- Learning reaches about two tiles. The world plug packs task lines at the top-left of the
  sensor area, the far corner from the motor area; on the default layout that will likely
  read as "does not learn". Putting lines near where the areas touch, or overlapping the
  areas, is worth trying in the plug before concluding anything about the blob.
- The reward rule moves every recently co-active connected pair in the blob, about 1% of all
  connections per reward. Most of that is noise that drifts unrelated weights toward the
  middle level.
- Sector bias still pins at +-16 in some sectors when input arrives (the sensor sectors get
  busy); the per-neuron thresholds then carry the load slowly.
- The surprise boost never engaged: in stage 0 random input about 9 of 256 attached lines
  are guessed wrong per tick, 36 per 1,024, under the `surprise_high_per_1024` line of 80.
- Pacemakers, far redraw and the surprise boost were not looked at separately.
- Sleep (mechanism 21) is not built.

## Next

- A causal eligibility for good/bad: source active in the ticks before the target fired,
  not both anywhere in the window. Cheap with the ring: the map the slot's delay read when
  the target fired.
- Make cue information carry further: sparser background (lower target) together with a
  stronger feed-forward from sensor cells, or far slots that start from the input area.
- Run the world's echo with lines near the motor area to compare with this check.
- Intrinsics for the far-slot gather if the far slots become a bigger share of the tick.
