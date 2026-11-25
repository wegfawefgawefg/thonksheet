# Results: lose-shift (idea 53, 2026-11-24)

Idea 53: when bad arrives, the motor cells that just fired get harder to fire for a while,
so the blob tries a different answer next time. Aimed at the lock in
`docs/results-2026-11-24-ablations-and-rivals.md`: echo seed 3 and the crossed
shuffled-echo lives settle on one answer and keep giving it while punished. Two neighbours
were tried beside it: win-stay (on good, the cells that just fired get easier to fire) and
bad cut (bad may remove a strength-1 connection, but only into a motor cell that just
fired).

## Short version

- **Lose-shift does not fix crossed lives.** On fresh seeds the crossed lives score 0.56
  with it and 0.56 without. It takes the floor away (worst crossed life 0.04 -> 0.31; seed
  17 goes 0.04 -> 0.75) but it also takes away the crossed lives that had learned (seed 11
  0.94 -> 0.51). Everything ends near 0.55: half-learned, not locked and not solved.
  A 160k-step life does not change that (0.60 against 0.55, t 0.8; no crossed life with
  lose-shift ever reaches 0.9).
- **It does break seed 3's lock** (0.72 -> 1.00, cue 0 now presses its own button 99% of
  the time). But seed 3 is a tuning seed, and on the fresh echo seeds lose-shift costs
  -0.16 (t -2.4).
- **It costs stability.** Over 160k-step echo lives the learned blobs wander: windows under
  0.8 after first reaching 0.9 go from 3 to 16 per life, the worst window in any
  life's last tenth from 0.92 to 0.57.
- **Win-stay is the strongest result here, and it points the other way.** It makes echo
  k=2 sharper (0.976 against 0.940, all 8 seeds 0.94 or more, t 1.5 so not yet proven; steadier at the end of long lives,
  though with deeper dips earlier) and
  makes crossed lives lock far harder (0.56 -> 0.14, t -4.2). So the lock really is the
  reward rule reinforcing its own answer: give "stay" a push and it locks more.
- **Bad cut hurts echo k=2, straight lives and go-no-go** (-0.36 on echo k=2, t -9; echo
  k=4 unchanged) and is the only thing that
  moves cartpole: lose-shift plus bad cut gets 20.2 steps per fall against 13.7, but by
  doing nothing on about half the steps, which is how the untrained twin does it too.
- **Cartpole (lean reward, place code):** learning lowers steps per fall from 20.7 (twin)
  to 13.7. The blob is not pressing one button nearly always: late in life it presses the
  favoured button 58% of the time (twin 49%), the other 34%, none 9%. Lose-shift makes it
  more one-sided, not less (favoured button 70%, one seed 95%).
- **Prediction 11: wrong.** Crossed lives stay at 0.56, not above 0.8. The seed 3 half came
  true, on a tuning seed only.
- Off is byte-identical; speed unchanged.

## What was built

`src/blob/learn_lose_shift.cpp`, one hook line at the top of the good/bad step in
`tick_learn`. Genome numbers, all off by default:

| number | what it does |
|---|---|
| `lose_shift_steps` | on a bad event, add this many steps to the threshold of each motor cell that just fired (clamped at `threshold_cap`). The per-neuron regulation lowers it again by chance (one step per ~512 ticks while quiet). |
| `win_stay_steps` | on a good event, take this many steps off the same cells (floor 1). Regulation raises it again as they fire. |
| `lose_shift_fires` | "just fired" means at least this many spikes (1-3) in the 16-tick spike ring: a saturating count in three bit planes over the ring, motor bits only. |
| `bad_cut_on` | while bad is on, each strength-1 connection between recently co-active cells whose target is a recently fired motor cell goes, with the bad broadcast's chance. |

The threshold moves happen once per reward event (`pending_valence`, which blob_reward sets
and the tick clears). Cost: nothing in the hot loop; one pass over the motor tiles per
reward event, and for bad cut a pass like the bad broadcast's restricted to motor targets.
No floats, no per-synapse state, nothing new stored.

Checks:

- Off is byte-identical to b328cd9: `cmp` of blobrun csvs for echo k=2 seed 3 (with both
  baselines), shuffled echo seed 17, cartpole seed 1; again after merging the cartpole
  change from main.
- The learning-off twins of every variant are window-for-window identical to the baseline
  twin (cartpole lean/place seeds 1-4, from the sweep cache), so the report tables share
  one twin, run with the baseline.

Speed, `blobbench speed --tiles 8 --seconds-ms 2000` (4,096 neurons), `taskset -c 13`, five
rounds alternating the old build (b328cd9) and this one. **Shared machine, clock capped,
other agents' runs going (load 8-9).**

| build | learning off, ticks/s | learning on, ticks/s |
|---|---|---|
| before (b328cd9) | 65,670 64,159 62,910 62,745 60,858 | 46,390 45,294 45,315 45,319 43,687 |
| after, switches off | 65,753 65,504 62,973 63,779 63,074 | 46,147 46,329 45,174 45,348 44,922 |
| after, all three on | | 46,266 45,656 45,310 44,941 45,142 |

No measurable difference. (The bench's "on" rewards every 50 ticks with a random sign, so
all three mechanisms do real work.)

## How straight and crossed lives were told apart

As in the ablation doc: a shuffled-echo life is crossed when the task's draw puts cue 1 on
button 0. That depends only on the task seed, so it is computed from the world's splitmix64
(`scripts/variants/lose_shift_split.py`). It agrees with the ablation doc for seeds 11-18
(straight 12, 15, 18). Seeds 1-4 are all straight; 6, 7, 25, 26 are crossed.

## Choosing settings (tuning seeds only)

Echo k=2 on seeds 1-4; shuffled echo k=2 on seeds 1-4 (straight) and 6, 7, 25, 26
(crossed). No twins. "f2" means `lose_shift_fires 2`. Late score:

| variant | echo k=2 | shuffled, straight | shuffled, crossed | worst crossed |
|---|---|---|---|---|
| baseline | 0.851 | 0.916 | 0.490 | 0.01 |
| ls2 | 0.766 | 0.843 | 0.557 | 0.50 |
| ls4 | 0.785 | 0.779 | 0.555 | 0.52 |
| ls8 | 0.724 | 0.728 | 0.559 | 0.48 |
| ls4 f2 | 0.873 | 0.872 | 0.483 | 0.01 |
| ls8 f2 | 0.892 | 0.907 | 0.584 | 0.42 |
| ls16 f2 | **0.949** | **0.944** | **0.642** | 0.53 |
| ls24 f2 | 0.811 | 0.951 | 0.522 | 0.45 |
| ls32 f2 | 0.880 | 0.882 | 0.485 | 0.32 |
| ls8 f3 | 0.928 | 0.868 | 0.481 | 0.00 |
| ls16 f3 | 0.958 | 0.756 | 0.709 | 0.54 |
| ls32 f3 | 0.902 | 0.828 | 0.551 | 0.30 |
| ws1 | 0.881 | 0.833 | 0.024 | 0.00 |
| ws2 | 0.963 | 0.881 | 0.151 | 0.01 |
| cut | 0.607 | 0.634 | 0.500 | 0.45 |
| ls8 f2 ws2 | 0.830 | 0.754 | 0.454 | 0.00 |
| ls8 f2 cut | 0.492 | 0.592 | 0.535 | 0.43 |

With fires 1 the bump lands on many motor cells that had nothing to do with the answer (at
the target rate of 32 per 1,024 per tick, about 40% of cells fire at least once in 16
ticks; an estimate, not measured) and straight lives lose. Fires 2 keeps the bump on the cells that
did the pressing. Picked: lose-shift = 16 steps, fires 2; win-stay = 2 steps (fires 1 alone,
2 when combined with lose-shift, since the two share `lose_shift_fires`); bad cut on.
Echo per seed for ls16 f2 was 0.98 0.84 1.00 0.98 against 0.89 0.84 0.72 0.95: seed 3
from 0.72 to 1.00.

```
C="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"
for T in lose-shift-tune lose-shift-tune2 lose-shift-tune3; do
  python3 scripts/sweep.py --jobs 2 --no-twin --name tune --seeds 1-4,6,7,25,26 \
    --variants scripts/variants/$T.txt -- --task shuffled-echo --k 2 $C
  python3 scripts/sweep.py --jobs 2 --no-twin --name tune --seeds 1-4 \
    --variants scripts/variants/$T.txt -- --task echo --k 2 $C
done
python3 scripts/variants/lose_shift_split.py --seeds 1-4,6,7,25,26 \
  --variants scripts/variants/lose-shift-tune.txt -- --task shuffled-echo --k 2 $C
```

## Report runs

Fresh seeds: shuffled echo 11-24 (7 straight, 7 crossed), everything else 11-18. The
variants are `scripts/variants/lose-shift.txt`; the baseline row and its learning-off twin
come from `lose-shift-base.txt`, and every variant shares that twin (see Checks). With 8
seeds a difference under about 2 standard errors is noise. t is the paired difference to
the baseline over its standard error.

```
C="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"
sw() { # name seeds task args
  python3 scripts/sweep.py --jobs 2 --name "$1" --seeds $2 --variants scripts/variants/lose-shift-base.txt -- "${@:3}"
  python3 scripts/sweep.py --jobs 2 --no-twin --name "$1" --seeds $2 --variants scripts/variants/lose-shift.txt -- "${@:3}"
}
sw "shuffled echo k=2" 11-24 --task shuffled-echo --k 2 $C
sw "echo k=2" 11-18 --task echo --k 2 $C
sw "echo k=4" 11-18 --task echo --k 4 $C
sw "go-no-go" 11-18 --task go-no-go $C
sw "cartpole lean place" 11-18 --task cartpole --cart-reward lean --number-code place \
  --ticks 20 --sensor-side 2 --motor-side 3 --steps 40000 --window 1000
python3 scripts/variants/lose_shift_split.py --seeds 11-24 \
  --variants scripts/variants/lose-shift.txt -- --task shuffled-echo --k 2 $C
```

### All tasks, late score (cartpole in steps per fall), with t against the baseline

| variant | shuffled straight | shuffled crossed | echo k=2 | echo k=4 | go-no-go | cartpole |
|---|---|---|---|---|---|---|
| learning off (twin) | | | 0.498 | 0.158 | 0.564 | 20.7 |
| baseline | 0.932 | 0.559 | 0.940 | 0.326 | 0.860 | 13.7 |
| lose-shift | 0.896 (-0.7) | 0.561 (+0.0) | 0.779 (-2.4) | 0.331 (+0.1) | 0.864 (+0.1) | 13.0 (-0.8) |
| win-stay | 0.925 (-0.1) | **0.135 (-4.2)** | 0.976 (+1.5) | 0.256 (-1.8) | 0.912 (+0.6) | 10.7 (-4.6) |
| bad cut | 0.611 (-8.8) | 0.583 (+0.3) | 0.581 (-9.0) | 0.354 (+0.6) | 0.642 (-4.3) | 16.0 (+3.6) |
| lose-shift + win-stay | 0.827 (-2.1) | 0.549 (-0.1) | 0.808 (-2.4) | 0.335 (+0.3) | 0.849 (-0.2) | 12.2 (-3.1) |
| lose-shift + bad cut | 0.544 (-13.2) | 0.466 (-0.9) | 0.603 (-5.7) | 0.295 (-0.6) | 0.570 (-4.2) | 20.2 (+13.2) |

(The straight-column t values are from the same paired calculation over the 7 straight
seeds; shuffled echo's learning-off twin over all 14 seeds is 0.463.)

### Shuffled echo k=2, straight and crossed lives

Seeds 11-24. Straight 12, 15, 18, 21, 22, 23, 24; crossed 11, 13, 14, 16, 17, 19, 20.
Mean ± standard error.

| variant | straight | crossed | worst crossed | crossed vs base | t |
|---|---|---|---|---|---|
| baseline | 0.932 ±0.028 | 0.559 ±0.113 | 0.04 |  |  |
| lose-shift | 0.896 ±0.039 | 0.561 ±0.054 | 0.31 | +0.002 ±0.131 | +0.0 |
| win-stay | 0.925 ±0.050 | 0.135 ±0.076 | 0.00 | -0.424 ±0.101 | -4.2 |
| bad cut | 0.611 ±0.037 | 0.583 ±0.031 | 0.42 | +0.024 ±0.093 | +0.3 |
| lose-shift + win-stay | 0.827 ±0.035 | 0.549 ±0.045 | 0.38 | -0.010 ±0.129 | -0.1 |
| lose-shift + bad cut | 0.544 ±0.023 | 0.466 ±0.040 | 0.36 | -0.093 ±0.102 | -0.9 |

Late score per seed, seeds 11-24 in order (crossed: 11, 13, 14, 16, 17, 19, 20):

```
baseline              0.94 1.00 0.55 0.36 0.98 0.67 0.04 0.98 0.54 0.83 0.82 0.92 0.99 0.84
lose-shift            0.51 0.84 0.57 0.31 0.85 0.53 0.75 0.99 0.56 0.70 0.91 0.71 0.99 0.98
win-stay              0.50 0.63 0.33 0.05 0.98 0.00 0.02 0.96 0.01 0.03 0.99 0.98 0.94 0.99
bad cut               0.66 0.64 0.55 0.65 0.56 0.63 0.42 0.61 0.56 0.60 0.42 0.72 0.66 0.66
lose-shift + win-stay 0.38 0.80 0.68 0.39 0.69 0.53 0.59 0.75 0.64 0.62 0.85 0.90 0.96 0.84
lose-shift + bad cut  0.61 0.51 0.36 0.43 0.56 0.62 0.41 0.60 0.44 0.38 0.48 0.65 0.52 0.50
```

What the blob presses over the last 4,000 steps (one-off probe, same lives):

```
seed 17 (crossed; right is cue 0 -> b1, cue 1 -> b0)
  baseline     cue 0: b0 0.89 b1 0.07   cue 1: b0 0.01 b1 0.98   (the straight match, wrong 94%)
  lose-shift   cue 0: b1 1.00           cue 1: b0 0.49 b1 0.38   (half of the crossed match)
  win-stay     cue 0: b0 0.95 b1 0.03   cue 1: b0 0.01 b1 0.99   (straight match, wrong 97%)
  learning off cue 0: b0 0.26 b1 0.63   cue 1: b0 0.25 b1 0.64
seed 11 (crossed)
  baseline     cue 0: b1 0.99           cue 1: b0 0.88 b1 0.06   (learned the crossed match)
  lose-shift   cue 0: b1 0.99           cue 1: b0 0.02 b1 0.92   (one button for both cues)
  win-stay     cue 0: b1 0.96           cue 1: b0 0.05 b1 0.93   (one button for both cues)
```

So lose-shift does make the blob leave a wrong answer (seed 17 drops the straight match and
gets cue 0 right every time), but it does not hold the crossed match once found: seed 11,
which found it without help, ends pressing one button for both cues.

### echo k=2

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 0.940 ±0.061 | 0.498 ±0.194 | 0.442 ±0.186 | 0.257 ±0.121 |  |  |
| lose-shift | 0.779 ±0.172 | - | - | 0.191 ±0.153 | -0.162 ±0.067 | -2.4 |
| win-stay | 0.976 ±0.022 | - | - | 0.319 ±0.162 | +0.036 ±0.023 | +1.5 |
| bad cut | 0.581 ±0.110 | - | - | -0.038 ±0.151 | -0.359 ±0.040 | -9.0 |
| lose-shift + win-stay | 0.808 ±0.113 | - | - | 0.190 ±0.190 | -0.133 ±0.056 | -2.4 |
| lose-shift + bad cut | 0.603 ±0.129 | - | - | 0.004 ±0.164 | -0.338 ±0.059 | -5.7 |

```
baseline              0.94 0.94 0.97 0.99 0.92 0.98 0.80 0.98
lose-shift            0.98 0.78 0.64 0.67 0.99 0.53 0.71 0.93
win-stay              0.97 0.99 1.00 0.99 1.00 0.94 0.97 0.96
bad cut               0.48 0.56 0.64 0.73 0.40 0.69 0.62 0.54
lose-shift + win-stay 0.94 0.82 0.69 0.79 0.67 0.80 1.00 0.75
lose-shift + bad cut  0.61 0.59 0.71 0.60 0.38 0.52 0.82 0.59
```

### echo k=4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 0.326 ±0.127 | 0.158 ±0.051 | 0.167 ±0.122 | 0.115 ±0.090 |  |  |
| lose-shift | 0.331 ±0.150 | - | - | 0.130 ±0.111 | +0.005 ±0.063 | +0.1 |
| win-stay | 0.256 ±0.116 | - | - | 0.016 ±0.095 | -0.070 ±0.040 | -1.8 |
| bad cut | 0.354 ±0.051 | - | - | 0.028 ±0.051 | +0.028 ±0.045 | +0.6 |
| lose-shift + win-stay | 0.335 ±0.097 | - | - | 0.074 ±0.061 | +0.010 ±0.036 | +0.3 |
| lose-shift + bad cut | 0.295 ±0.055 | - | - | 0.033 ±0.091 | -0.030 ±0.053 | -0.6 |

```
baseline              0.28 0.25 0.33 0.12 0.26 0.46 0.53 0.37
lose-shift            0.48 0.19 0.36 0.41 0.03 0.32 0.41 0.43
win-stay              0.20 0.27 0.23 0.21 0.09 0.21 0.44 0.41
bad cut               0.36 0.38 0.25 0.39 0.30 0.37 0.40 0.38
lose-shift + win-stay 0.41 0.24 0.20 0.26 0.30 0.43 0.40 0.44
lose-shift + bad cut  0.34 0.29 0.22 0.36 0.28 0.23 0.36 0.28
```

### go-no-go

`--task go-no-go` with the same plug flags, seeds 11-18.

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 0.860 ±0.162 | 0.564 ±0.183 | 0.295 ±0.238 | 0.105 ±0.167 |  |  |
| lose-shift | 0.864 ±0.169 | - | - | 0.244 ±0.161 | +0.004 ±0.056 | +0.1 |
| win-stay | 0.912 ±0.151 | - | - | 0.213 ±0.138 | +0.052 ±0.082 | +0.6 |
| bad cut | 0.642 ±0.044 | - | - | -0.010 ±0.059 | -0.217 ±0.051 | -4.3 |
| lose-shift + win-stay | 0.849 ±0.154 | - | - | 0.220 ±0.182 | -0.011 ±0.060 | -0.2 |
| lose-shift + bad cut | 0.570 ±0.055 | - | - | -0.081 ±0.095 | -0.289 ±0.068 | -4.2 |

```
baseline              0.98 0.88 0.85 0.95 0.57 0.65 0.99 1.00
lose-shift            1.00 0.57 0.97 0.87 0.65 0.87 0.99 1.00
win-stay              0.56 0.90 1.00 0.99 0.88 0.98 1.00 1.00
bad cut               0.62 0.63 0.68 0.64 0.60 0.61 0.73 0.63
lose-shift + win-stay 0.74 0.62 0.95 1.00 0.82 0.69 0.98 1.00
lose-shift + bad cut  0.55 0.55 0.65 0.48 0.64 0.55 0.56 0.57
```

### Cartpole, lean reward, place code

`--task cartpole --cart-reward lean --number-code place --ticks 20 --sensor-side 2
--motor-side 3 --steps 40000 --window 1000`, seeds 11-18. Steps per fall, higher is better.
Coin flips: about 23.5 (seed 11, from the cartpole commit on main).

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 13.7 ±0.7 | 20.7 ±4.4 | -6.924 ±4.777 | 0.975 ±0.774 |  |  |
| lose-shift | 13.0 ±2.1 | - | - | -0.958 ±4.009 | -0.743 ±0.916 | -0.8 |
| win-stay | 10.7 ±1.6 | - | - | -1.109 ±1.250 | -2.997 ±0.646 | -4.6 |
| bad cut | 16.0 ±1.3 | - | - | -0.216 ±1.977 | +2.234 ±0.618 | +3.6 |
| lose-shift + win-stay | 12.2 ±1.1 | - | - | -1.841 ±2.669 | -1.589 ±0.516 | -3.1 |
| lose-shift + bad cut | 20.2 ±1.7 | - | - | 0.057 ±1.979 | +6.451 ±0.490 | +13.2 |

```
baseline              13 14 13 15 14 14 13 13
lose-shift            14 13 14 12 9.99 12 17 13
win-stay              9.55 9.39 12 9.39 12 11 14 9.53
bad cut               17 15 15 14 18 17 16 16
lose-shift + win-stay 14 13 13 12 11 12 12 11
lose-shift + bad cut  20 19 20 23 22 19 21 19
```

Press shares over the last 4,000 steps, mean over seeds 11-18 (b0 pushes left, b1 right;
"favoured" is whichever button that life pressed more):

| variant | none | favoured button | other button | most one-sided life | good / bad per step |
|---|---|---|---|---|---|
| learning off | 0.13 | 0.49 | 0.38 | 0.64 (seed 17) | 0.29 / 0.71 |
| baseline | 0.09 | 0.58 | 0.34 | 0.73 (seed 15) | 0.27 / 0.73 |
| lose-shift | 0.11 | 0.70 | 0.19 | 0.95 (seed 15) | 0.27 / 0.73 |
| bad cut | 0.14 | 0.50 | 0.36 | 0.58 (seed 14) | 0.27 / 0.73 |
| lose-shift + bad cut | 0.47 | 0.29 | 0.24 | 0.32 (seed 17) | 0.26 / 0.74 |

Per seed, share of the favoured button: learning off 0.51 0.45 0.50 0.44 0.48 0.45 0.64
0.47; baseline 0.57 0.57 0.59 0.54 0.73 0.47 0.57 0.58; lose-shift 0.58 0.66 0.43 0.81 0.95
0.77 0.67 0.72. The favoured button is b1 (right) in 7 of 8 baseline lives.

So the coordinator's reading is half right. Learning does push cartpole toward one button
(7 of 8 baseline lives favour a button more than their twin does, 58% against 49% on
average), but it is far from "nearly always"; the drop from 20.7 to 13.7 steps per fall comes
with a lean of about 20 points, not a lock. Lose-shift makes the lean stronger, not weaker.
Lose-shift plus bad cut recovers the twin's figure by making no move on about half the
steps (a tie or silence counts as no move, which applies no force), not by balancing.
Note that under the lean reward bad comes on about 73% of steps whatever the blob does, so
lose-shift fires on most steps here and acts more like a constant drag on the motor cells
than a response to a mistake.

## Stability over a long life

Echo k=2, 160,000 steps, seeds 11-18, windows of 1,000 steps (160 windows; last tenth = 16
windows).

```
python3 scripts/sweep.py --jobs 2 --no-twin --name "echo k=2, 160k steps" --seeds 11-18 \
  --variants scripts/variants/lose-shift-long.txt -- --task echo --k 2 --ticks 20 \
  --sensor-side 3 --motor-side 3 --steps 160000 --window 1000
python3 scripts/variants/lose_shift_split.py --no-split --stable --seeds 11-18 \
  --variants scripts/variants/lose-shift-long.txt -- --task echo --k 2 --ticks 20 \
  --sensor-side 3 --motor-side 3 --steps 160000 --window 1000
```

| variant | last tenth | lowest window in last tenth | lowest window after first reaching 0.9 | windows under 0.8 after first reaching 0.9 |
|---|---|---|---|---|
| baseline | 0.980 ±0.005 | 0.960 ±0.010 (min 0.92) | 0.748 ±0.049 | 3.0 |
| lose-shift | 0.892 ±0.050 | 0.855 ±0.057 (min 0.57) | 0.677 ±0.049 | 16.4 |
| win-stay | 0.987 ±0.005 | 0.953 ±0.023 (min 0.80) | 0.590 ±0.039 | 7.0 |
| lose-shift + win-stay | 0.904 ±0.036 | 0.827 ±0.053 (min 0.64) | 0.568 ±0.037 | 41.4 |

(mean ± standard error over seeds; every life reached 0.9 at some point.)

```
last tenth per seed
baseline              0.96 0.96 0.97 0.99 0.99 1.00 0.99 0.98
lose-shift            0.96 0.97 0.85 0.99 0.98 0.60 1.00 0.78
win-stay              0.99 0.99 0.96 1.00 0.98 1.00 0.98 1.00
lose-shift + win-stay 0.92 0.70 1.00 0.93 0.81 0.91 1.00 0.96
```

Yes, lose-shift makes a learned blob wander. In echo every wrong answer earns a bad, and a
learned blob still makes a few; each one now knocks 16 steps off the pressing cells'
excitability, so the right group sometimes loses to the other one, earns another bad, and
the knock goes on. Seed 16 ends at 0.60 after being at 1.00. Win-stay by itself ends
steadier than the baseline in the last tenth but dips deeper earlier in life.

### Crossed lives over a long life

Does lose-shift just need longer? The 7 crossed lives of the report set, 160,000 steps:

```
python3 scripts/sweep.py --jobs 2 --no-twin --name "crossed, 160k" --seeds 11,13,14,16,17,19,20 \
  --variants scripts/variants/lose-shift-crossed-long.txt -- --task shuffled-echo --k 2 \
  --ticks 20 --sensor-side 3 --motor-side 3 --steps 160000 --window 1000
```

| variant | last tenth | worst life | lowest window, last tenth | lives that ever reached 0.9 |
|---|---|---|---|---|
| baseline | 0.546 ±0.060 | 0.21 | 0.480 (min 0.11) | 2 of 7 |
| lose-shift | 0.600 ±0.032 | 0.54 | 0.546 (min 0.46) | 0 of 7 |

```
baseline    0.55 0.70 0.58 0.64 0.52 0.62 0.21   (seeds 11, 13, 14, 16, 17, 19, 20)
lose-shift  0.54 0.55 0.57 0.63 0.78 0.58 0.56
```

No. With 4x the life the crossed lives are still near 0.55-0.60 either way (+0.05, t 0.8,
noise). The baseline's two good crossed lives do not keep it either: seed 11 was 0.94 at
40k steps and is 0.55 at 160k. Lose-shift keeps every life off the floor (worst 0.54
against 0.21) and none ever reaches 0.9.

## Reading

The lock is real and it is the reward rule's own doing: win-stay, which only adds weight to
whatever was just rewarded, makes the crossed lives lock almost completely (0.14) and makes
the straight ones sharper. Lose-shift does what it says: it unseats a held wrong answer
(seed 17, seed 3). But a cell-level "try something else" cannot tell which cue it was
answering, so it unseats right answers too, and the blob ends at roughly half right on
crossed lives whether it had found the crossed match or not. On plain echo it costs
learned blobs their steadiness. The crossed match needs a two-tile path (ablation doc), and
nothing here builds that path; lose-shift only makes the one-tile answer less sticky.

Bad cut shows why bad stops at strength 1: allowed to disconnect even only into motor cells,
it takes echo k=2 from 0.94 to 0.58. The connections it cuts are the ones that carry the
answer.

## Prediction 11

"Lose-shift lifts the crossed shuffled-echo lives from 0.51 to above 0.8 and stops seed 3
locking onto one button." **Wrong.** Crossed lives: 0.559 -> 0.561 (t 0.0) on fresh seeds.
Seed 3 does stop locking (0.72 -> 1.00, both cues answered right), but seed 3 was a tuning
seed and the same setting costs fresh echo seeds 0.16.

## Rough

- Tuning used 4 straight and 4 crossed lives and 4 echo seeds, and the chosen setting's
  tuning gains (echo +0.10, crossed +0.15) did not carry to fresh seeds. Tuning sets this
  small pick noise.
- The press probe is a one-off program (blobrun's life loop plus a tally of moves per cue
  over the last tenth); it is not in the repo. It ran the same lives as the sweep (same
  seeds and flags).
- "Just fired" counts spikes over the 16-tick ring, which at reward time holds the last 15
  ticks of the 20-tick step and the first tick of the next. A step is 20 ticks, so the
  first 5 ticks of the step are missed.
- Win-stay with lose-shift uses lose-shift's fires setting (2); alone it uses 1. They share
  one number.
- The motor cells bumped include motor cells the plug does not read (the blob does not know
  which are read).
- Cartpole coin figure is from one seed (main's cartpole commit), not rerun here.
- `scripts/sweep.py`'s LEARNING_ONLY set does not know the four new genome numbers, so a
  sweep with twins runs a separate (identical) twin per variant. Here that was avoided by
  running the twin once with the baseline. If the coordinator wants it, add
  `lose_shift_steps`, `win_stay_steps`, `lose_shift_fires`, `bad_cut_on` to that set (scripts
  is another agent's area, so I did not).

## Next

- The crossed lives need the two-tile path built, and "try another answer" does not build
  it. The next thing to try against crossed lives is a credit rule that reaches two tiles:
  causal credit (idea 3) or pooling relays (idea 48). Lose-shift as made here works on
  cells, not on (cue, answer) pairs, so it cannot by itself keep a found crossed match.
- Win-stay as a standalone: +0.036 on echo k=2 (t 1.5) with every seed at 0.94 or more and
  the steadiest long lives. Worth a run on more seeds and tasks, knowing it deepens the lock
  wherever the first answer is the wrong one.
- Under the lean cartpole reward bad comes 73% of the time; a reward-minus-expectation
  baseline (idea 4) would make bad mean "worse than usual" and is the obvious pairing for
  lose-shift there.
