# Results: babbling and balanced good/bad (ideas 60 and 61, 2026-11-24)

Idea 60: now and then the world urges one randomly chosen button for a whole task step
(motor babbling), and the engine's ordinary reward rules learn from what follows. Idea 61:
at the plug, thin out whichever of good and bad has been commoner lately, so the two arrive
equally often. Both are built once, at the plug, so all three engines get them.

## Short version

- **Babbling helps where a sheet is stuck, and costs a little where it is not.** The one
  clean gain is in the minimal engine: crossed shuffled-echo lives 0.671 -> 0.766 (steady,
  t 5.8) or 0.751 (fading, t 2.9), echo k=4 0.400 -> 0.473 (fading, t 3.9), and echo k=8
  0.078 -> 0.188 (fading, t 7.1; coin 0.109, twin 0.110). That k=8 row is the first score
  above chance on eight cues in any engine.
- **Full blob: crossed lives 0.559 -> 0.650 with fading babble, t 0.9: noise.** It lifts the
  worst life (seed 17, 0.04 -> 0.71) and costs straight lives nothing, but no crossed life
  is better on average by more than the spread. Echo k=2 drops 0.06-0.08 (t -1.4 to -1.6),
  two tiles apart 0.06 (t -1.6 to -1.8).
- **Table engine: nothing to break and something to lose.** Its crossed lives are already
  at 0.92; babbling leaves them there (fading) or costs 0.07 (steady). Echo k=4 drops
  0.82 -> 0.62 with fading babble (t -2.0).
- **Too much babbling silences the minimal engine.** At 12-25% of steps its motor cells
  get so used to being urged that rate regulation raises their thresholds and they fall
  quiet between babbles (54% of its own steps make no move at 25%); scores fall to 0.25.
  The settings kept are 3% (steady) or a fading rate.
- **Balance does what it says and helps nothing.** On cartpole's lean reward bad goes from
  0.73 to 0.27 per step, level with good, in every engine. Steps per fall do not rise:
  blob 13.7 -> 13.0 (t -1.4), minimal 16.5 -> 17.4 (t +1.3), table 16.2 -> 14.7 (t -2.3);
  coin is 23.3. Under the sparse reward good is so rare that balance throws away about 97%
  of the bad on falls, and the blob (16.3 -> 11.1, t -4.8) and the table (13.3 -> 10.4,
  t -4.0) get worse. On echo k=2 it costs the minimal engine 0.26 (t -19.5) by thinning
  the good it lives on. Catch: everything at coin.
- **Fading babble raises cartpole steps per fall, but not by learning.** Minimal lean
  16.5 -> 19.0 (t 5.1), sparse 18.6 -> 20.5 (t 7.1); table lean 16.2 -> 17.8 (t 2.0). The
  learning-off twins rise as much or more (minimal 18.5 -> 21.2, 19.3 -> 22.1): the random
  pushes themselves help, as coin flips (23.3) already show.
- **Prediction 21: wrong** (crossed lives above 0.8 in at least two engines: no engine gets
  there; the minimal engine reaches 0.77, the blob 0.65; the table was there already).
  **Prediction 22: wrong** (nothing reaches 25 steps per fall under the lean reward).
- Off is byte-identical in all three engines; speed unchanged.
- **Unfinished** (stopped when the agent count was cut): the long-life stability check, and
  idea 61 on catch and echo k=2/k=4 in the table engine. See "Unfinished" at the end.

## What was built

One shell array and one plug file, so every engine gets both ideas.

- `Blob::urge` (end of `src/blob/blob.hpp`): one word per tile, written by the world before
  each tick: cells it urges to fire this tick. Each engine allocates it at birth (zeroed)
  and reads it in one line of its fire step:
  - full blob (`fire.cpp`): ORed into the noise word, so an urged cell fires on top of its
    inputs if it is rested (cooldown is respected), and the noise ring marks it as a spike
    that came from outside. The default brain rule learns from it like any other recent
    spike.
  - minimal (`minimal_tick.cpp`): ORed into the noise word. Its default rule grades only
    spikes that came from noise, so here an urged spike is an experiment the rule grades.
    Without that the rule would never grade an urged cell at all.
  - table (`table_tick.cpp`): ORed into the spikes. Its default rule moves each graded
    neuron's used counter toward what it did, so an urged spike counts as "fired".
  - The stand-in (coin, perfect) leaves it null and is never urged.
- `src/world/plug_babble.cpp` (+ `.hpp`), settings `BabbleSettings` at the end of
  `RunFlags`, state in the task ticker. It uses the world's own splitmix64, seeded from the
  task seed, so the blob's generator is untouched, and the urge never looks at the right
  answer.
  - `--babble N`: at a step's start, with chance N/1024, pick one of the task's buttons
    uniformly at random. For every choosing tick of that step, each of its motor cells is
    urged with chance `--babble-fire F`/256 (default 64). Rest ticks are not urged.
  - `--babble-fade 1`: the chance is scaled by the running share of bad among rewarded
    steps that were not babbled (forgets at 2^-6 per step), so babbling fades as the blob
    gets things right and comes back when it is stuck on a wrong answer.
  - `--balance 1`: running rates of good and bad per step (x65536, forget at 2^-6 per step,
    `--balance-shift`). When the commoner one arrives it is passed on with chance
    rarer/commoner, otherwise dropped (no broadcast, no good/bad line, no rest ticks). The
    task still scores every step.
  - With any of these on (or `--signal-report 1`), the summary gets the late score without
    babbled steps (`late_clean`: right/tries with those steps' tallies taken out), how often
    the urged button won, the share of the blob's own steps with no move, and good and bad
    per step as the task gave them and as the blob got them; `<stem>-clean.csv` has the
    clean windows.
- `scripts/sweep.py`: `--out DIR` for the cache; reads the `table` column when there is no
  `blob` one, and gives a `--table` twin `--t.learning_on 0` as well (the table sheet has its
  own switch, and `--g.learning_on 0` alone left the table twin learning); caches the clean
  windows and signal counts when a run writes them. Unknown plug flags in a variant pass
  through as pairs, as before, so every flag there must take one value.
- `scripts/babble.sh` runs everything here; `scripts/variants/babble_split.py` prints the
  tables from the cache; variants are `scripts/variants/babble-tune.txt`, `babble.txt` and
  `balance.txt`.

Engine edits, all additive: `blob.hpp` +1 field at the end of `Blob`; `birth.cpp`,
`minimal_birth.cpp`, `table_birth.cpp` one allocation and one free each; `fire.cpp`,
`minimal_tick.cpp`, `table_tick.cpp` one changed line each.

### Checks

- Off is byte-identical to 3d970d6, all three engines: `cmp` of blobrun csvs (blob,
  learn_off, coin, perfect) and of summaries without the ticks/s line, for shuffled echo
  seed 3, cartpole lean/place seed 1 and echo k=4 seed 5 (`out/babble/ident.sh`, rerun after
  each code change). `--signal-report 1` alone also leaves the csv identical.
- The report baselines reproduce earlier docs exactly: blob shuffled echo straight 0.932 /
  crossed 0.559, echo k=2 0.940, cartpole lean 13.7 (twin 20.7), as in the lose-shift doc.
- An urged button wins its step 99-100% of the time at fire 64/256 in every engine (91%
  in the blob at 16/256).

### Speed

`blobrun --speed --speed-ms 2000` (random input, learning on), three rounds alternating
the 3d970d6 build and this one with babbling off. **Shared machine (load 5.3-5.6, other
agents' runs going), clock capped.** Ticks per second, range over the three rounds:

| engine | build | 8x8 tiles | 16x16 tiles | 32x32 tiles |
|---|---|---|---|---|
| blob | before | 52,734-53,393 | 12,996-13,104 | 3,260-3,412 |
| blob | after | 53,262-53,455 | 12,880-13,111 | 3,347-3,393 |
| minimal | before | 213,844-214,993 | 52,484-52,936 | 12,016-12,074 |
| minimal | after | 218,873-220,622 | 53,965-54,498 | 12,343-12,531 |
| table | before | 40,214-40,622 | 9,915-9,954 | 2,440-2,461 |
| table | after | 40,367-40,555 | 9,929-9,990 | 2,414-2,425 |

No measurable cost (the minimal engine's 2-3% gain is code layout, not the change). Bytes
per neuron unchanged at the printed precision (one more word per 64 neurons). Babbling on
costs nothing in the tick: the plug writes a few words on babbled steps only.

## Idea 60: babbling

### Choosing settings (tuning seeds only)

Shuffled echo k=2 on seeds 1-4 (all straight) and 6, 7, 25, 26 (crossed); echo k=2 on
seeds 1-4. 40,000-step lives, no twins. `cN fF` is `--babble N --babble-fire F` (N steps in
1024 babbled; each urged cell urged with chance F/256 per tick); `fadeN` is
`--babble N --babble-fade 1` at fire 64. Late score, all steps:

| setting | blob straight | blob crossed | blob echo | minimal straight | minimal crossed | minimal echo | table straight | table crossed | table echo |
|---|---|---|---|---|---|---|---|---|---|
| off | 0.916 | 0.490 | 0.851 | 0.941 | 0.745 | 0.972 | 0.912 | 0.845 | 0.897 |
| c32 f64 | 0.756 | 0.656 | 0.910 | 0.927 | **0.810** | 0.921 | 0.844 | 0.829 | 0.941 |
| c128 f64 | 0.790 | 0.537 | 0.843 | 0.729 | 0.586 | 0.721 | 0.799 | 0.864 | 0.825 |
| c256 f64 | 0.673 | 0.517 | 0.797 | 0.297 | 0.298 | 0.289 | 0.702 | 0.610 | 0.810 |
| c128 f16 | 0.869 | 0.520 | 0.777 | 0.857 | 0.711 | 0.872 | 0.874 | 0.914 | 0.889 |
| c128 f256 | 0.826 | 0.500 | 0.804 | 0.256 | 0.252 | 0.257 | 0.787 | 0.869 | 0.822 |
| fade256 | **0.895** | **0.668** | 0.815 | **0.972** | 0.762 | 0.949 | 0.913 | **0.926** | 0.870 |
| fade512 | 0.866 | 0.512 | 0.940 | 0.951 | 0.469 | 0.948 | 0.935 | 0.834 | 0.902 |

Picked, one setting for all three engines: **steady = `--babble 32 --babble-fire 64`**
(3% of steps) and **fade = `--babble 256 --babble-fade 1`** (up to 25% of steps, scaled by
the recent share of bad). Fade256 is the only setting that is best or near best on crossed
lives in all three engines without costing straight ones; c32 is the only steady rate the
minimal engine survives. Seed 3 (the echo seed that locks, a tuning seed): blob 0.72 off,
0.79 steady, 1.00 fade; minimal 0.98 / 0.93 / 0.95; table 0.92 / 0.97 / 0.95.

At 12% or more steady babbling the minimal engine falls to 0.25-0.29 on both tasks. Its
urged motor cells fire so much that the per-neuron regulation raises their thresholds, and
between babbles they are silent: at 25% babbling, 54% of its own steps (seed 1, echo k=2)
end with no move, against 4% with babbling off. The full blob and the table sheet do not
do this (the blob respects cooldown; the table regulates on its own output, not on spikes).

```
scripts/babble.sh tune
scripts/babble.sh summary-tune
```

### Report runs

Seeds 11-24 for shuffled echo (straight 12, 15, 18, 21, 22, 23, 24; crossed 11, 13, 14,
16, 17, 19, 20, told apart as in the lose-shift doc), 11-18 for the rest. 40,000 steps of
20 ticks, 3x3 groups, windows of 1,000, late = last 4 windows. Each variant has its own
learning-off twin, babbled the same way. "Unbabbled" is the score with babbled steps left
out. Mean ± standard error over seeds; t is the paired difference to off over its standard
error; with 8 seeds (7 for the split) |t| under 2 is noise. Coin flips (the stand-in, same
seeds): shuffled 0.477, echo k=2 0.475, k=4 0.230, k=8 0.109, 2 tiles apart 0.476, go-no-go
0.499.

```
scripts/babble.sh report
scripts/babble.sh coin
scripts/babble.sh summary
```

#### Shuffled echo k=2, straight and crossed lives

| engine | variant | straight | crossed | crossed, unbabbled | worst crossed | crossed vs off | t | twin straight | twin crossed |
|---|---|---|---|---|---|---|---|---|---|
| blob | off | 0.932 ±0.028 | 0.559 ±0.113 | 0.559 | 0.04 | | | 0.511 | 0.416 |
| blob | steady | 0.819 ±0.063 | 0.614 ±0.044 | 0.616 | 0.52 | +0.055 | +0.5 | 0.494 | 0.411 |
| blob | fade | 0.929 ±0.024 | 0.650 ±0.059 | 0.653 | 0.39 | +0.091 | +0.9 | 0.493 | 0.423 |
| minimal | off | 0.956 ±0.005 | 0.671 ±0.024 | 0.671 | 0.55 | | | 0.481 | 0.318 |
| minimal | steady | 0.913 ±0.003 | **0.766 ±0.018** | 0.775 | 0.72 | +0.095 | **+5.8** | 0.454 | 0.320 |
| minimal | fade | 0.950 ±0.006 | **0.751 ±0.024** | 0.758 | 0.70 | +0.080 | **+2.9** | 0.420 | 0.330 |
| table | off | 0.931 ±0.028 | 0.924 ±0.036 | 0.924 | 0.74 | | | 0.522 | 0.490 |
| table | steady | 0.863 ±0.030 | 0.858 ±0.038 | 0.870 | 0.70 | -0.065 | -1.2 | 0.522 | 0.490 |
| table | fade | 0.901 ±0.035 | 0.925 ±0.032 | 0.929 | 0.82 | +0.001 | +0.0 | 0.523 | 0.495 |

Straight lives against off: blob steady -0.113 (t -1.6), fade -0.004 (t -0.1); minimal
steady -0.043 (t -8.0), fade -0.006 (t -0.6); table steady -0.069 (t -1.4), fade -0.030
(t -0.7).

Late score per seed, 11 to 24 (crossed: 11, 13, 14, 16, 17, 19, 20):

```
blob     off     0.94 1.00 0.55 0.36 0.98 0.67 0.04 0.98 0.54 0.83 0.82 0.92 0.99 0.84
blob     steady  0.54 0.95 0.55 0.61 0.96 0.57 0.64 0.64 0.52 0.86 0.91 0.98 0.60 0.69
blob     fade    0.90 0.97 0.59 0.39 0.94 0.59 0.71 0.99 0.63 0.74 0.91 0.80 0.97 0.91
minimal  off     0.70 0.94 0.63 0.55 0.95 0.75 0.71 0.96 0.70 0.67 0.95 0.95 0.95 0.98
minimal  steady  0.76 0.91 0.74 0.72 0.92 0.87 0.76 0.93 0.79 0.73 0.90 0.91 0.92 0.91
minimal  fade    0.75 0.96 0.74 0.74 0.94 0.89 0.70 0.96 0.70 0.74 0.97 0.94 0.96 0.93
table    off     0.98 0.85 1.00 0.86 1.00 1.00 0.98 1.00 0.74 0.90 1.00 0.87 0.84 0.96
table    steady  0.84 0.94 0.86 0.79 0.78 0.87 0.98 0.75 0.98 0.70 0.91 0.81 0.92 0.92
table    fade    0.99 0.89 0.98 0.86 0.85 0.82 1.00 0.98 0.83 0.99 1.00 0.83 1.00 0.76
```

In the blob, fade lifts the two worst crossed lives (17: 0.04 -> 0.71; 19: 0.54 -> 0.63)
and lowers two that had learned (11: 0.94 -> 0.90, 20: 0.83 -> 0.74). In the minimal engine
every crossed life goes up with steady babbling, from 0.55-0.75 to 0.72-0.87. Crossed lives
that were under 0.8 and end above it with babbling: minimal seed 16 (0.75 -> 0.87 steady,
0.89 fade) and table seed 19 (0.74 -> 0.98 steady, 0.83 fade); none in the blob.

#### The other tasks

Late score (all steps), with the score on unbabbled steps and the paired t against off:

| task | engine | off | steady | t | fade | t | fade, unbabbled | t | twin (off / fade) | coin |
|---|---|---|---|---|---|---|---|---|---|---|
| echo k=2 | blob | 0.940 | 0.857 | -1.6 | 0.881 | -1.4 | 0.888 | -1.3 | 0.498 / 0.492 | 0.475 |
| echo k=2 | minimal | 0.949 | 0.911 | -4.3 | 0.966 | +1.9 | 0.967 | +2.1 | 0.495 / 0.429 | |
| echo k=2 | table | 0.915 | 0.935 | +0.7 | 0.918 | +0.1 | 0.921 | +0.1 | 0.507 / 0.509 | |
| echo k=4 | blob | 0.326 | 0.316 | -0.2 | 0.360 | +0.9 | 0.369 | +1.1 | 0.158 / 0.176 | 0.230 |
| echo k=4 | minimal | 0.400 | 0.459 | +3.0 | **0.473** | **+3.9** | **0.497** | **+5.1** | 0.212 / 0.215 | |
| echo k=4 | table | 0.822 | 0.713 | -0.9 | 0.617 | -2.0 | 0.627 | -1.9 | 0.248 / 0.248 | |
| echo k=8 | blob | 0.096 | 0.100 | +0.2 | 0.111 | +1.0 | 0.106 | +0.6 | 0.102 / 0.109 | 0.109 |
| echo k=8 | minimal | 0.078 | 0.117 | +2.0 | **0.188** | **+7.1** | **0.202** | **+7.3** | 0.108 / 0.110 | |
| echo k=8 | table | 0.121 | 0.121 | -0.2 | 0.119 | -0.5 | 0.117 | -0.9 | 0.128 / 0.127 | |
| 2 tiles apart | blob | 0.556 | 0.494 | -1.8 | 0.490 | -1.6 | 0.486 | -1.6 | 0.443 / 0.443 | 0.476 |
| 2 tiles apart | minimal | 0.419 | 0.426 | +0.9 | 0.394 | -3.3 | 0.382 | -4.7 | 0.364 / 0.345 | |
| 2 tiles apart | table | 0.858 | 0.797 | -0.7 | 0.878 | +0.4 | 0.879 | +0.4 | 0.469 / 0.471 | |
| go-no-go | blob | 0.860 | 0.858 | -0.0 | 0.856 | -0.1 | 0.863 | +0.1 | 0.564 / 0.552 | 0.499 |
| go-no-go | minimal | 0.973 | 0.959 | -2.9 | 0.977 | +0.8 | 0.980 | +1.4 | 0.617 / 0.584 | |
| go-no-go | table | 0.906 | 0.921 | +0.4 | 0.952 | +1.2 | 0.955 | +1.4 | 0.532 / 0.532 | |

Share of steps babbled with fade (it follows the share of bad): echo k=2 0.05 / 0.01 / 0.02
(blob / minimal / table), k=4 0.16 / 0.10 / 0.11, k=8 0.22 / 0.18 / 0.22, 2 tiles apart
0.11 / 0.11 / 0.07, go-no-go 0.04 / 0.01 / 0.02. Steady is 0.031 everywhere.

The minimal engine's k=8 baseline (0.078) is below its twin (0.108): lives freeze on wrong
answers, as the noise-sign doc found for k=4 ("a rule graded against its own average
freezes when every reward equals the average"). Fading babble breaks that freeze, since
one babble in eight is the right button and earns a good the rule can grade, and the score
goes to 0.19: above coin (0.109) on every seed (lowest 0.15).

Per seed, 11 to 18:

```
minimal echo k=8  off     0.05 0.05 0.11 0.07 0.12 0.07 0.09 0.07
                  steady  0.13 0.18 0.06 0.10 0.17 0.09 0.09 0.12
                  fade    0.21 0.19 0.17 0.17 0.17 0.24 0.20 0.15
minimal echo k=4  off     0.52 0.40 0.34 0.39 0.42 0.36 0.37 0.40
                  steady  0.48 0.42 0.46 0.45 0.48 0.48 0.48 0.42
                  fade    0.47 0.46 0.46 0.48 0.47 0.49 0.47 0.47
table echo k=4    off     0.99 0.78 0.96 0.64 0.62 1.00 0.83 0.75
                  steady  0.25 0.23 0.96 0.86 0.71 0.85 0.96 0.87
                  fade    0.26 0.99 0.95 0.28 0.42 0.94 0.73 0.36
```

The minimal engine's k=4 lives all end at 0.46-0.49 with fading babble: the bad lives
come up and the best one (seed 11, 0.52) comes down. In the table sheet babbling knocks
some k=4 lives that had learned (0.99, 0.64) down to near a coin (0.25-0.28).

### Reading (idea 60)

An experiment the size of a decision does reach the reward rule: the urged button wins its
step 99-100% of the time and every engine learns from what follows, so the mechanism works.
What it buys depends on whether the sheet was stuck. Where a rule freezes on wrong answers
(the minimal engine on k=4 and k=8, its crossed lives, the blob's worst crossed life),
babbling unfreezes it, because a forced right answer earns a good the rule can act on.
Where a sheet had learned, babbling only adds wrong answers to learn from, and in the blob
and the table sheet it costs a few points (more on table k=4). Fading babble keeps most of
the gain and loses most of the cost, as intended.

It does not build the two-tile path the crossed lives need (lose-shift doc): the blob's
crossed lives stay near 0.6 on average, and the minimal engine's rise to 0.77, not above
0.8. "Try the other answer" is not enough on its own; the credit for that answer still has
to reach two tiles.

### Stability of a learned life: UNFINISHED

Echo k=2, 160,000 steps, seeds 11-18, all three engines, off / steady / fade: not run. To
run it and print the table (last tenth, lowest window in the last tenth, lowest window
after first reaching 0.9, windows under 0.8 after that, on all windows and on unbabbled
windows):

```
scripts/babble.sh long
scripts/babble.sh summary      # the "echo k=2, 160,000 steps" tables
```

The closest evidence so far is the 40,000-step echo k=2 lives above: with steady/fade
babbling the blob ends at 0.86/0.88 against 0.94 (t -1.6/-1.4), the minimal engine at
0.91/0.97 against 0.95, the table at 0.94/0.92 against 0.92. On a learned life fading
babble is rare (1-5% of steps), which is why it costs little.

## Idea 61: balanced good and bad

`--balance 1`, alone and with idea 60's fading babble (`--babble 256 --babble-fade 1`).
Seeds 11-18, 40,000 steps of 20 ticks, windows of 1,000. Cartpole: `--task cartpole
--number-code place --sensor-side 2 --motor-side 3`, lean or sparse reward; catch: `--task
catch --catch-size 4 4` at 3x3 groups. Each row has its own learning-off twin. Good and bad
are per step: "raw" as the task gave them, "sent" as the blob got them. Coin flips:
cartpole 23.3 steps per fall (good 0.32 / bad 0.68 per step under lean, 0.014 / 0.045
under sparse), catch 0.256.

```
scripts/babble.sh balance
scripts/babble.sh summary
```

### Cartpole, lean reward (steps per fall, higher is better)

| engine | variant | late | twin | good raw | bad raw | good sent | bad sent | vs off | t |
|---|---|---|---|---|---|---|---|---|---|
| blob | off | 13.7 ±0.3 | 20.7 | 0.273 | 0.726 | 0.273 | 0.726 | | |
| blob | balance | 13.0 ±0.6 | 21.6 | 0.272 | 0.727 | 0.272 | 0.269 | -0.7 | -1.4 |
| blob | fade | 14.4 ±0.4 | 18.1 | 0.279 | 0.721 | 0.279 | 0.721 | +0.6 | +1.2 |
| blob | balance + fade | 14.5 ±0.6 | 20.3 | 0.277 | 0.723 | 0.277 | 0.274 | +0.7 | +1.1 |
| minimal | off | 16.5 ±0.3 | 18.5 | 0.273 | 0.727 | 0.273 | 0.727 | | |
| minimal | balance | 17.4 ±0.4 | 18.8 | 0.267 | 0.732 | 0.267 | 0.264 | +0.9 | +1.3 |
| minimal | fade | 19.0 ±0.2 | 21.2 | 0.263 | 0.736 | 0.263 | 0.736 | +2.4 | +5.1 |
| minimal | balance + fade | 19.2 ±0.3 | 21.6 | 0.264 | 0.736 | 0.263 | 0.260 | +2.6 | +4.4 |
| table | off | 16.2 ±0.3 | 12.9 | 0.305 | 0.694 | 0.305 | 0.694 | | |
| table | balance | 14.7 ±0.8 | 13.0 | 0.285 | 0.714 | 0.285 | 0.282 | -1.6 | -2.3 |
| table | fade | 17.8 ±0.6 | 14.1 | 0.309 | 0.691 | 0.309 | 0.691 | +1.5 | +2.0 |
| table | balance + fade | 15.5 ±0.6 | 14.3 | 0.290 | 0.709 | 0.290 | 0.287 | -0.7 | -1.0 |

Fade rows babble on 17-18% of steps. The table sheet is the only engine whose learning beats
its own twin on cartpole (16.2 against 12.9), and it is still below a coin.

### Cartpole, sparse reward

| engine | variant | late | twin | good raw | bad raw | good sent | bad sent | vs off | t |
|---|---|---|---|---|---|---|---|---|---|
| blob | off | 16.3 ±1.0 | 21.4 | 0.006 | 0.065 | 0.006 | 0.065 | | |
| blob | balance | 11.1 ±0.4 | 21.2 | 0.002 | 0.084 | 0.002 | 0.002 | -5.2 | -4.8 |
| blob | fade | 15.5 ±0.9 | 19.2 | 0.005 | 0.067 | 0.005 | 0.067 | -0.8 | -0.6 |
| blob | balance + fade | 11.9 ±0.4 | 18.6 | 0.001 | 0.082 | 0.001 | 0.001 | -4.4 | -5.9 |
| minimal | off | 18.6 ±0.4 | 19.3 | 0.008 | 0.057 | 0.008 | 0.057 | | |
| minimal | balance | 18.3 ±0.2 | 19.4 | 0.008 | 0.056 | 0.008 | 0.007 | -0.3 | -0.6 |
| minimal | fade | 20.5 ±0.4 | 22.1 | 0.011 | 0.049 | 0.011 | 0.049 | +1.9 | +7.1 |
| minimal | balance + fade | 20.6 ±0.4 | 22.3 | 0.011 | 0.049 | 0.011 | 0.010 | +1.9 | +4.3 |
| table | off | 13.3 ±0.2 | 12.9 | 0.007 | 0.073 | 0.007 | 0.073 | | |
| table | balance | 10.4 ±0.6 | 12.8 | 0.001 | 0.096 | 0.001 | 0.001 | -3.0 | -4.0 |
| table | fade | 15.2 ±0.8 | 14.6 | 0.008 | 0.062 | 0.008 | 0.062 | +1.9 | +2.3 |
| table | balance + fade | 12.6 ±1.1 | 14.8 | 0.002 | 0.083 | 0.002 | 0.002 | -0.7 | -0.6 |

### Catch 4x4 and echo (blob and minimal; table UNFINISHED)

| task | engine | variant | late | twin | good raw | bad raw | good sent | bad sent | vs off | t |
|---|---|---|---|---|---|---|---|---|---|---|
| catch | blob | off | 0.242 | 0.233 | 0.083 | 0.250 | 0.083 | 0.250 | | |
| catch | blob | balance | 0.249 | 0.236 | 0.082 | 0.251 | 0.082 | 0.079 | +0.006 | +0.8 |
| catch | blob | fade | 0.242 | 0.244 | 0.083 | 0.251 | 0.083 | 0.251 | -0.000 | -0.1 |
| catch | blob | balance + fade | 0.252 | 0.234 | 0.083 | 0.251 | 0.083 | 0.080 | +0.009 | +1.1 |
| catch | minimal | off | 0.248 | 0.254 | 0.085 | 0.248 | 0.085 | 0.248 | | |
| catch | minimal | balance | 0.247 | 0.254 | 0.085 | 0.249 | 0.085 | 0.081 | -0.001 | -0.4 |
| catch | minimal | fade | 0.255 | 0.245 | 0.084 | 0.249 | 0.084 | 0.249 | +0.007 | +0.9 |
| catch | minimal | balance + fade | 0.248 | 0.254 | 0.085 | 0.249 | 0.085 | 0.082 | -0.000 | -0.0 |
| echo k=2 | blob | off | 0.940 | 0.498 | 0.851 | 0.114 | 0.851 | 0.114 | | |
| echo k=2 | blob | balance | 0.930 | 0.503 | 0.880 | 0.097 | 0.091 | 0.092 | -0.010 | -0.3 |
| echo k=2 | blob | balance + fade | 0.958 | 0.502 | 0.874 | 0.097 | 0.091 | 0.092 | +0.018 | +0.5 |
| echo k=2 | minimal | off | 0.949 | 0.495 | 0.926 | 0.029 | 0.926 | 0.029 | | |
| echo k=2 | minimal | balance | **0.690** | 0.468 | 0.693 | 0.115 | 0.114 | 0.115 | -0.259 | **-19.5** |
| echo k=2 | minimal | balance + fade | 0.682 | 0.425 | 0.678 | 0.124 | 0.122 | 0.124 | -0.267 | -16.3 |
| echo k=4 | blob | off | 0.326 | 0.158 | 0.256 | 0.617 | 0.256 | 0.617 | | |
| echo k=4 | blob | balance | 0.295 | 0.162 | 0.271 | 0.592 | 0.264 | 0.261 | -0.031 | -0.6 |
| echo k=4 | blob | balance + fade | 0.290 | 0.175 | 0.275 | 0.607 | 0.260 | 0.257 | -0.036 | -0.5 |
| echo k=4 | minimal | off | 0.400 | 0.212 | 0.360 | 0.448 | 0.360 | 0.448 | | |
| echo k=4 | minimal | balance | 0.382 | 0.216 | 0.359 | 0.452 | 0.322 | 0.320 | -0.018 | -0.6 |
| echo k=4 | minimal | balance + fade | 0.496 | 0.212 | 0.475 | 0.360 | 0.335 | 0.337 | +0.096 | +5.1 |

The fade-only echo rows are in the idea 60 table. Minimal echo k=4 with balance and fade
(0.496) is close to fade alone (0.473); their difference was not tested.

UNFINISHED: table catch 4x4 (stopped part way; finished runs are cached), and table echo
k=2 and k=4 with balance (their off and fade rows exist from idea 60). To finish:

```
scripts/babble.sh balance      # cached runs are skipped; only the table rows are left
scripts/babble.sh summary
```

### Reading (idea 61)

Balance is exact at the plug: after it, good and bad arrive equally often in every engine
and task. It helps nothing. Under the lean reward bad was common, but thinning it did not
make it informative: what is wrong on cartpole is not how often bad comes but that one
step's push decides little about the next lean. Under the sparse reward and on echo, the
commoner signal was the useful one (bad on falls; the good of a learned echo), and throwing
most of it away costs. A running-average baseline (reward minus what is usual, idea 4)
keeps every event and is the better form of the same idea.

## Predictions

- **21. "Nudging a whole button now and then breaks the lock. Crossed shuffled-echo lives
  go from about 0.55 to above 0.8 in at least two of the three engines." Wrong.** Crossed
  lives: blob 0.559 -> 0.650 (fade, t 0.9), minimal 0.671 -> 0.766 (steady, t 5.8), table
  0.924 -> 0.925 (fade; it never had the lock). No engine moves a locked set of lives above
  0.8. Right in spirit for one engine: in the minimal engine babbling reliably loosens the
  lock, and on k=8 it breaks a freeze outright.
- **22. "Balancing good and bad lets something beat a coin on cartpole. At least one engine
  ends above 25 steps per fall under the lean reward." Wrong.** Best with balance: minimal
  17.4 (19.2 with fade too, and its twin with fade gets 21.6). Coin is 23.3.

## What should change in other docs (for the coordinator)

- `docs/findings.md`, principle 12: add that a decision-sized experiment does reach the
  rule (the urged button wins 99-100% of its steps) and unfreezes rules that froze on wrong
  answers: minimal engine k=8 0.078 -> 0.188 (coin 0.109), k=4 0.40 -> 0.47, crossed lives
  0.67 -> 0.77, all t above 2.9 on fresh seeds. One engine, so **likely** at most.
- Wall "Locking on": babbling (fading form) is the first thing that lifts locked lives
  without wrecking learned ones, but it does not get crossed lives above 0.8 in any engine;
  the crossed lock needs the two-tile path, as the lose-shift doc said. The table sheet has
  no crossed lock (0.92).
- Wall "More than four things": the minimal engine with fading babble is above chance on
  eight cues (0.19 against 0.11 for a coin), the first such score; still far from learned.
- Wall "Acting in a world": balanced reward does not help cartpole in any engine and hurts
  under the sparse reward. Fading babble raises steps per fall, but its learning-off twin
  rises as much, so that is random pushing, not learning. The table sheet beats its own
  twin on cartpole (16.2 against 12.9), still below a coin.
- A caution worth a line: rate regulation turns heavy babbling into silence in the minimal
  engine (with 12% or more of steps urged, its motor cells stop firing on their own).
- `docs/ideas.md`: 60 *tried*: loosens locks in the minimal engine, small or negative
  elsewhere; fading form preferred; stability check unfinished. 61 *tried: null; hurts
  under sparse reward and in the minimal engine's echo*; try idea 4's running-average
  baseline instead.
- `docs/predictions.md`: 21 wrong, 22 wrong (text above).
- `docs/notes-world.md`: `scripts/sweep.py` now reads the table engine (column `table`,
  twin `--t.learning_on 0`). Before this, a `--table` sweep failed on the column, and a twin
  given only `--g.learning_on 0` would still have learned.

## Next

- Babbling in the minimal engine together with idea 59's evidence counters, on k=4, k=8 and
  crossed lives: babbling supplies right answers to a frozen rule, and the counters could
  hold the weak evidence those rare answers give.
- A babble rate set per cue (bad share per cue), so a life with one cue right and one wrong
  babbles only on the wrong one, which is what crossed lives need. The button must still be
  picked at random.
- Keep urged spikes out of the regulation count (or babble below its reach), so heavier
  babbling does not silence the minimal engine.
- For cartpole, reward minus its running average (idea 4) instead of thinning.

## Unfinished, and how to resume

Stopped 2026-11-25 when the agent count was cut. Everything else above is measured.

1. Stability over a long life (all three engines): `scripts/babble.sh long`, then
   `scripts/babble.sh summary`.
2. Idea 61 in the table engine on catch 4x4 and echo k=2/k=4: `scripts/babble.sh balance`
   (finished runs are cached and skipped).

Both use `out/babble/blobrun-sweep`, a copy of this branch's blobrun (`scripts/babble.sh`
rebuilds and copies it if it is missing). Raw outputs, all in this worktree and not in git:
`out/babble/sweep/` (cache.jsonl, plus a csv, -sum and -clean file per run),
`out/babble/coin/`, `out/babble/ident/`, `out/babble/speed/result.txt`, and the logs
`out/babble/{tune,report,balance,coin}.log`.

## Rough

- Settings were chosen on 4 straight and 4 crossed tuning lives and 4 echo seeds. Tuning
  this small picks noise: the blob's fade256 crossed gain was +0.18 on tuning seeds and
  +0.09 on report seeds.
- One setting serves all three engines; per-engine settings might do better (the table
  sheet would want less babbling).
- "Unbabbled" scores remove the babbled steps' right/tries from each window; the steps
  right after a babble still carry its effect.
- The minimal engine counts an urged spike as a noise spike, so its noise-graded rule
  grades it. The blob also marks urged spikes in its noise ring (read only by its rival
  rules); the table sheet does not mark them in its flip ring (read only by its noise
  rules). Rival and noise rules were not run with babbling.
- The blob respects cooldown on urged cells; the other two engines have no cooldown.
- The learning-off twin is babbled too; under fade it babbles more than the learning life,
  since it is wrong more often.
- Speed was measured on a shared, loaded machine (load 5.3-5.6); the report runs later
  shared it with load up to about 33, which slows runs but does not change results.
- Fade's running bad share starts at one half, so every life babbles at about half the
  maximum rate for its first few hundred steps.
