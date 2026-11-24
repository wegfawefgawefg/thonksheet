# Results: ablations and rivals (2026-11-24)

The design is frozen at 21 mechanisms. This doc takes each one out in turn, then runs the
rival learning rules from `docs/plan.md` on the same blob, plug and tasks. All numbers are
from seeds 11-18, which nothing was tuned on. Rival settings were picked on seeds 1-4.

## Short version

- **Needed** (taking it out hurts on two or more tasks): inhibition, per-neuron threshold
  regulation, the bad broadcast, the good broadcast, growth, pruning.
- **Surprise boost** also hurts when removed, but it is not acting on surprise: it is on for
  92-99% of ticks on echo, so it just doubles every learning chance.
- **No measurable difference** on any task: local delays, the branch bonus, varied neurons
  (threshold, leak and cooldown spreads), noise, far redraw.
- **Small or mixed**: prediction learning (helps on echo k=2 only), lopsided weights (linear
  0,1,2,3 is a bit worse everywhere), far slots, pacemakers, cooldown, sector regulation.
- **Nothing helps when removed** on a task that learns.
- **The frozen blob with a learned readout ties the full rule on echo k=2 and shuffled echo.**
  The full rule beats it only on echo k=4 (0.33 against 0.16, where the readout learns
  nothing).
- **Random flips into motor neurons learn echo k=2** (0.82 against 0.50 frozen), worse than
  the brain rule (0.94). On cartpole they are the only rule that improves anything.
- **Noise as the experiment (REINFORCE)** loses to the brain rule in place of it, and adds
  nothing alongside it.
- **Nothing learns delayed echo or cartpole.** On cartpole the brain rule ends worse than its
  frozen twin (10.6 steps per fall against 14.3), and every rule stays at or below a coin flip
  (about 22).
- **Shuffled echo splits by geometry.** Lives where the cue-to-button match is "straight"
  score 0.99; "crossed" lives average 0.51 and can lock onto the wrong answer (0.04). The
  crossed match needs a two-tile path, which matches the core's finding that learning
  reaches about one or two tiles. Seeds 1-4 are all straight, so the effort doc's "shuffled
  echo 0.96" was effectively plain echo.

## How it was run

`scripts/sweep.py` runs one task over a list of seeds for a file of named variants, at most
4 `nice -n 19` single-threaded `blobrun` lives at once, and prints the tables below. Each
variant's learning-off twin is the same command with `--g.learning_on 0`; variants that
differ only in learning numbers share one twin. Late is the mean of the last 4 of 40 windows
of 1,000 steps, early the first 4. "late vs base" is the paired difference to the first row,
seed by seed, with its standard error, and t is their ratio. With 8 seeds and about 100
comparisons, |t| near 2 can be chance; |t| of 3 or more, or the same sign on several tasks,
is a real effect.

```
cmake -S . -B build && nice -n 19 cmake --build build -j4
C="--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000"
for V in ablate rivals; do
  python3 scripts/sweep.py --name "echo k=2" --seeds 11-18 --variants scripts/variants/$V.txt -- --task echo --k 2 $C
  python3 scripts/sweep.py --name "echo k=4" --seeds 11-18 --variants scripts/variants/$V.txt -- --task echo --k 4 $C
  python3 scripts/sweep.py --name "shuffled echo k=2" --seeds 11-18 --variants scripts/variants/$V.txt -- --task shuffled-echo --k 2 $C
  python3 scripts/sweep.py --name "delayed echo k=2" --seeds 11-18 --variants scripts/variants/$V.txt -- --task delayed-echo --k 2 $C
done
python3 scripts/sweep.py --name cartpole --seeds 11-18 --variants scripts/variants/rivals.txt -- \
  --task cartpole --ticks 20 --sensor-side 2 --motor-side 3 --steps 100000 --window 1000
```

Each variant is a genome switch (`--g.name value`); the full list with what each line turns
off is `scripts/variants/ablate.txt`. Switches added for this: `sector_reg_on`,
`threshold_reg_on`, `noise_on`, `predict_on`, `good_on`, `bad_on`, `grow_on`, `prune_on`,
`far_redraw_on`, and `strong_log` (what a w1 bit counts: 2 gives 0,1,4,5, 1 gives 0,1,2,3).
With all of them left on, runs are byte-identical to before.

## Ablations

Paired difference to the baseline, late score, with t in brackets. Mechanism numbers are
those in `docs/design.md`.

| variant | echo k=2 | echo k=4 | shuffled echo k=2 | delayed echo k=2 |
|---|---|---|---|---|
| baseline | 0.940 | 0.326 | 0.688 | 0.439 |
| no-cooldown (3) | -0.117 (-2.6) | +0.076 (+1.1) | -0.080 (-0.6) | +0.005 (+0.9) |
| no-inhibition (4) | -0.463 (-27.3) | -0.115 (-2.6) | -0.218 (-2.1) | -0.022 (-0.7) |
| no-far (6) | -0.108 (-1.9) | -0.068 (-1.0) | -0.069 (-0.6) | -0.012 (-1.4) |
| no-delays (7) | +0.003 (+0.1) | -0.059 (-0.9) | -0.068 (-0.8) | -0.002 (-0.4) |
| no-branch (8) | -0.048 (-0.9) | -0.041 (-0.6) | -0.086 (-0.6) | +0.002 (+0.1) |
| linear-weights (9) | -0.092 (-3.2) | -0.080 (-1.8) | -0.058 (-0.4) | -0.013 (-1.5) |
| uniform-neurons (11) | -0.058 (-1.3) | +0.014 (+0.2) | +0.016 (+0.2) | +0.002 (+0.2) |
| no-pacemakers (12) | -0.122 (-2.1) | -0.026 (-0.4) | +0.017 (+0.2) | +0.013 (+1.6) |
| no-noise (13) | -0.013 (-0.3) | +0.008 (+0.2) | -0.105 (-1.0) | +0.019 (+1.2) |
| no-sector-reg (14) | -0.067 (-1.1) | -0.083 (-2.0) | +0.041 (+0.4) | -0.017 (-1.1) |
| no-threshold-reg (15) | -0.322 (-5.6) | -0.091 (-1.8) | -0.196 (-1.9) | +0.041 (+3.8) |
| no-predict (16) | -0.151 (-3.5) | -0.057 (-0.8) | +0.032 (+0.3) | +0.007 (+0.6) |
| no-good (17) | -0.079 (-2.6) | -0.129 (-2.3) | -0.348 (-2.3) | +0.005 (+0.6) |
| no-bad (17) | -0.361 (-5.0) | -0.152 (-2.6) | -0.256 (-1.7) | +0.024 (+1.8) |
| no-surprise (18) | -0.185 (-2.6) | -0.097 (-2.4) | -0.114 (-0.9) | +0.026 (+2.0) |
| no-grow (19) | -0.338 (-5.3) | -0.104 (-2.5) | -0.146 (-1.1) | +0.002 (+0.2) |
| no-prune (20) | -0.339 (-5.4) | -0.123 (-4.1) | -0.174 (-1.5) | +0.012 (+1.1) |
| no-far-redraw (6) | -0.028 (-0.8) | +0.011 (+0.2) | +0.083 (+0.8) | +0.002 (+0.2) |

Reading, mechanism by mechanism:

- **Inhibition (4).** Without it nothing works: echo k=2 drops to 0.48 and so does its
  twin. Needed.
- **Per-neuron threshold regulation (15).** Out, learning nearly stops (echo k=2 0.62 against
  a twin of 0.58). Sector regulation does not stand in for it. Needed.
- **Bad (17).** The largest single learning signal on echo k=2 (-0.36). Needed.
- **Good (17).** Smaller on echo k=2 (-0.08) but the largest loss on shuffled echo (-0.35).
  Needed. Without good, three shuffled lives end at 0.00-0.11, all of them crossed lives
  (see below): bad alone drives them onto the wrong match.
- **Growth (19) and pruning (20).** Each costs about a third on echo k=2 and a tenth on k=4.
  Needed, both of them.
- **Surprise boost (18).** Out costs -0.19, -0.10, -0.11. But a probe shows `surprise_high`
  is on for 92-94% of ticks on echo k=2 and 97-99% on echo k=3 (seed 11): with only 36
  attached sensor neurons, the 80-per-1,024 line is always crossed. So it is a constant
  doubling of every learning chance, not a response to surprise. What the row says is that
  the learning chances want to be about twice the genome numbers.
- **Prediction learning (16).** Helps on echo k=2 (-0.15, t -3.5), nothing on k=4 or
  shuffled. Weak evidence it matters.
- **Lopsided weights (9).** Linear weights lose a little on every task (-0.09, -0.08,
  -0.06). Thresholds were set for 0,1,4,5, so part of that may be the thresholds.
- **Far slots (6), pacemakers (12), cooldown (3), sector regulation (14).** Small losses on
  one task each, none consistent. Far slots lean negative everywhere but never reach t 2.
  Cooldown is negative on k=2 and positive on k=4.
- **No measurable difference: local delays (7), branch bonus (8), varied neurons (11), noise
  (13), far redraw (6).** Every |t| is 1.3 or less on every task. Delays were the open
  question in the plan: on these tasks, all-delays-1 learns as well.
- **Delayed echo.** Nothing learns it: baseline 0.439 against a twin of 0.432, and no
  variant moves it. It scores under 0.5 because no-moves count as misses. The cue is gone 40
  ticks before the ask; the ring is 16 ticks and "recent" is 6. The two "helps when removed"
  entries in that column (threshold regulation, surprise boost) shift a bias in a blob that
  does not learn the task, so they are not evidence.

### echo k=2

`--task echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 0.940 ±0.061 | 0.498 ±0.194 | 0.442 ±0.186 | 0.257 ±0.121 |  |  |
| no-cooldown (3) | 0.823 ±0.126 | 0.601 ±0.187 | 0.222 ±0.202 | 0.074 ±0.164 | -0.117 ±0.045 | -2.6 |
| no-inhibition (4) | 0.478 ±0.050 | 0.471 ±0.049 | 0.007 ±0.046 | 0.023 ±0.033 | -0.463 ±0.017 | -27.3 |
| no-far (6) | 0.832 ±0.130 | 0.622 ±0.105 | 0.210 ±0.151 | 0.159 ±0.132 | -0.108 ±0.058 | -1.9 |
| no-delays (7) | 0.943 ±0.048 | 0.576 ±0.155 | 0.367 ±0.135 | 0.241 ±0.078 | +0.003 ±0.031 | +0.1 |
| no-branch (8) | 0.893 ±0.136 | 0.500 ±0.182 | 0.392 ±0.168 | 0.197 ±0.168 | -0.048 ±0.052 | -0.9 |
| linear-weights (9) | 0.849 ±0.120 | 0.526 ±0.169 | 0.323 ±0.193 | 0.197 ±0.117 | -0.092 ±0.028 | -3.2 |
| uniform-neurons (11) | 0.883 ±0.104 | 0.488 ±0.086 | 0.395 ±0.087 | 0.242 ±0.067 | -0.058 ±0.043 | -1.3 |
| no-pacemakers (12) | 0.819 ±0.132 | 0.569 ±0.145 | 0.249 ±0.236 | 0.123 ±0.166 | -0.122 ±0.059 | -2.1 |
| no-noise (13) | 0.928 ±0.083 | 0.509 ±0.205 | 0.419 ±0.269 | 0.196 ±0.147 | -0.013 ±0.045 | -0.3 |
| no-sector-reg (14) | 0.874 ±0.161 | 0.528 ±0.194 | 0.346 ±0.268 | 0.211 ±0.127 | -0.067 ±0.061 | -1.1 |
| no-threshold-reg (15) | 0.618 ±0.110 | 0.582 ±0.226 | 0.036 ±0.298 | 0.061 ±0.124 | -0.322 ±0.058 | -5.6 |
| no-predict (16) | 0.790 ±0.120 | 0.498 ±0.194 | 0.291 ±0.244 | 0.123 ±0.140 | -0.151 ±0.043 | -3.5 |
| no-good (17) | 0.861 ±0.111 | 0.498 ±0.194 | 0.363 ±0.226 | 0.224 ±0.122 | -0.079 ±0.031 | -2.6 |
| no-bad (17) | 0.580 ±0.207 | 0.498 ±0.194 | 0.081 ±0.079 | 0.017 ±0.042 | -0.361 ±0.072 | -5.0 |
| no-surprise (18) | 0.756 ±0.158 | 0.498 ±0.194 | 0.258 ±0.287 | 0.110 ±0.161 | -0.185 ±0.071 | -2.6 |
| no-grow (19) | 0.602 ±0.169 | 0.498 ±0.194 | 0.104 ±0.066 | 0.005 ±0.052 | -0.338 ±0.063 | -5.3 |
| no-prune (20) | 0.601 ±0.184 | 0.498 ±0.194 | 0.103 ±0.142 | 0.033 ±0.034 | -0.339 ±0.063 | -5.4 |
| no-far-redraw (6) | 0.913 ±0.079 | 0.498 ±0.194 | 0.415 ±0.233 | 0.217 ±0.149 | -0.028 ±0.037 | -0.8 |

Late score per seed, in seed order:

```
baseline           0.94 0.94 0.97 0.99 0.92 0.98 0.80 0.98
no-cooldown (3)    0.78 0.94 0.63 0.99 0.91 0.71 0.73 0.89
no-inhibition (4)  0.50 0.50 0.50 0.43 0.52 0.49 0.37 0.51
no-far (6)         0.99 0.96 0.67 0.92 0.73 0.74 0.92 0.72
no-delays (7)      0.98 0.97 0.83 0.95 0.95 0.92 0.97 0.97
no-branch (8)      0.98 0.62 0.99 0.95 0.99 0.76 0.90 0.97
linear-weights (9) 0.76 0.87 0.89 0.85 0.96 0.95 0.60 0.91
uniform-neurons (11) 0.64 0.95 0.90 0.95 0.96 0.86 0.91 0.89
no-pacemakers (12) 0.96 0.67 0.79 0.96 0.74 0.67 0.99 0.77
no-noise (13)      0.95 0.94 0.99 0.83 1.00 0.96 0.99 0.77
no-sector-reg (14) 0.98 0.94 0.95 0.64 0.59 0.98 0.92 0.98
no-threshold-reg (15) 0.66 0.71 0.52 0.46 0.71 0.58 0.76 0.53
no-predict (16)    0.82 0.63 0.98 0.72 0.81 0.93 0.73 0.69
no-good (17)       0.74 0.95 0.99 0.78 0.86 0.91 0.69 0.96
no-bad (17)        0.68 0.33 0.28 0.71 0.52 0.63 0.56 0.92
no-surprise (18)   0.64 0.61 0.90 0.76 0.93 0.68 0.96 0.55
no-grow (19)       0.61 0.33 0.42 0.60 0.64 0.65 0.67 0.90
no-prune (20)      0.40 0.59 0.45 0.61 0.45 0.73 0.60 0.97
no-far-redraw (6)  0.94 0.91 0.99 0.96 0.99 0.76 0.89 0.86
```

### echo k=4

`--task echo --k 4 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 0.326 ±0.127 | 0.158 ±0.051 | 0.167 ±0.122 | 0.115 ±0.090 |  |  |
| no-cooldown (3) | 0.401 ±0.121 | 0.223 ±0.069 | 0.178 ±0.162 | 0.126 ±0.159 | +0.076 ±0.067 | +1.1 |
| no-inhibition (4) | 0.210 ±0.087 | 0.175 ±0.054 | 0.036 ±0.131 | 0.049 ±0.065 | -0.115 ±0.045 | -2.6 |
| no-far (6) | 0.258 ±0.101 | 0.180 ±0.043 | 0.078 ±0.112 | -0.018 ±0.110 | -0.068 ±0.067 | -1.0 |
| no-delays (7) | 0.267 ±0.165 | 0.221 ±0.119 | 0.046 ±0.171 | 0.011 ±0.095 | -0.059 ±0.065 | -0.9 |
| no-branch (8) | 0.285 ±0.182 | 0.166 ±0.059 | 0.119 ±0.160 | 0.055 ±0.187 | -0.041 ±0.072 | -0.6 |
| linear-weights (9) | 0.245 ±0.162 | 0.164 ±0.073 | 0.081 ±0.103 | 0.057 ±0.127 | -0.080 ±0.044 | -1.8 |
| uniform-neurons (11) | 0.340 ±0.070 | 0.162 ±0.073 | 0.178 ±0.088 | 0.092 ±0.085 | +0.014 ±0.061 | +0.2 |
| no-pacemakers (12) | 0.299 ±0.141 | 0.222 ±0.066 | 0.077 ±0.176 | 0.084 ±0.130 | -0.026 ±0.063 | -0.4 |
| no-noise (13) | 0.333 ±0.104 | 0.164 ±0.052 | 0.169 ±0.082 | 0.105 ±0.092 | +0.008 ±0.033 | +0.2 |
| no-sector-reg (14) | 0.242 ±0.136 | 0.166 ±0.054 | 0.076 ±0.113 | 0.050 ±0.097 | -0.083 ±0.043 | -2.0 |
| no-threshold-reg (15) | 0.234 ±0.039 | 0.152 ±0.068 | 0.082 ±0.083 | 0.010 ±0.057 | -0.091 ±0.050 | -1.8 |
| no-predict (16) | 0.268 ±0.127 | 0.158 ±0.051 | 0.110 ±0.101 | 0.071 ±0.115 | -0.057 ±0.075 | -0.8 |
| no-good (17) | 0.197 ±0.122 | 0.158 ±0.051 | 0.039 ±0.115 | 0.067 ±0.121 | -0.129 ±0.056 | -2.3 |
| no-bad (17) | 0.173 ±0.090 | 0.158 ±0.051 | 0.015 ±0.089 | -0.003 ±0.020 | -0.152 ±0.059 | -2.6 |
| no-surprise (18) | 0.228 ±0.089 | 0.158 ±0.051 | 0.070 ±0.075 | 0.029 ±0.078 | -0.097 ±0.040 | -2.4 |
| no-grow (19) | 0.222 ±0.037 | 0.158 ±0.051 | 0.064 ±0.059 | -0.001 ±0.064 | -0.104 ±0.041 | -2.5 |
| no-prune (20) | 0.203 ±0.059 | 0.158 ±0.051 | 0.044 ±0.062 | 0.006 ±0.012 | -0.123 ±0.030 | -4.1 |
| no-far-redraw (6) | 0.337 ±0.151 | 0.158 ±0.051 | 0.178 ±0.127 | 0.121 ±0.110 | +0.011 ±0.065 | +0.2 |

Late score per seed, in seed order:

```
baseline           0.28 0.25 0.33 0.12 0.26 0.46 0.53 0.37
no-cooldown (3)    0.28 0.48 0.66 0.36 0.42 0.31 0.34 0.36
no-inhibition (4)  0.25 0.00 0.25 0.20 0.26 0.24 0.25 0.24
no-far (6)         0.35 0.29 0.11 0.31 0.32 0.08 0.30 0.30
no-delays (7)      0.48 0.38 0.11 0.14 0.26 0.17 0.50 0.10
no-branch (8)      0.41 0.16 0.11 0.40 0.11 0.10 0.55 0.43
linear-weights (9) 0.29 0.05 0.22 0.16 0.03 0.41 0.34 0.47
uniform-neurons (11) 0.26 0.41 0.27 0.46 0.33 0.37 0.30 0.31
no-pacemakers (12) 0.40 0.17 0.11 0.46 0.18 0.27 0.49 0.31
no-noise (13)      0.36 0.23 0.21 0.28 0.26 0.39 0.47 0.47
no-sector-reg (14) 0.23 0.14 0.16 0.20 0.07 0.29 0.37 0.50
no-threshold-reg (15) 0.27 0.25 0.20 0.26 0.18 0.28 0.18 0.24
no-predict (16)    0.35 0.09 0.36 0.46 0.16 0.23 0.16 0.34
no-good (17)       0.18 0.09 0.30 0.20 0.08 0.43 0.07 0.21
no-bad (17)        0.38 0.10 0.12 0.16 0.17 0.10 0.16 0.20
no-surprise (18)   0.33 0.11 0.22 0.21 0.10 0.34 0.26 0.25
no-grow (19)       0.23 0.20 0.21 0.22 0.18 0.30 0.21 0.21
no-prune (20)      0.15 0.13 0.17 0.17 0.21 0.26 0.31 0.22
no-far-redraw (6)  0.44 0.04 0.23 0.50 0.26 0.45 0.40 0.37
```

### shuffled echo k=2

`--task shuffled-echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 0.688 ±0.354 | 0.462 ±0.206 | 0.226 ±0.365 | 0.126 ±0.235 |  |  |
| no-cooldown (3) | 0.609 ±0.255 | 0.399 ±0.240 | 0.210 ±0.208 | 0.101 ±0.146 | -0.080 ±0.141 | -0.6 |
| no-inhibition (4) | 0.470 ±0.067 | 0.481 ±0.046 | -0.011 ±0.092 | 0.022 ±0.031 | -0.218 ±0.105 | -2.1 |
| no-far (6) | 0.619 ±0.290 | 0.402 ±0.208 | 0.216 ±0.220 | 0.084 ±0.144 | -0.069 ±0.118 | -0.6 |
| no-delays (7) | 0.620 ±0.269 | 0.457 ±0.197 | 0.163 ±0.179 | 0.066 ±0.112 | -0.068 ±0.088 | -0.8 |
| no-branch (8) | 0.602 ±0.269 | 0.439 ±0.193 | 0.163 ±0.274 | 0.162 ±0.167 | -0.086 ±0.135 | -0.6 |
| linear-weights (9) | 0.630 ±0.384 | 0.427 ±0.186 | 0.203 ±0.366 | 0.124 ±0.195 | -0.058 ±0.130 | -0.4 |
| uniform-neurons (11) | 0.704 ±0.144 | 0.387 ±0.087 | 0.318 ±0.136 | 0.179 ±0.118 | +0.016 ±0.104 | +0.2 |
| no-pacemakers (12) | 0.705 ±0.193 | 0.402 ±0.191 | 0.303 ±0.178 | 0.248 ±0.131 | +0.017 ±0.096 | +0.2 |
| no-noise (13) | 0.583 ±0.333 | 0.453 ±0.213 | 0.130 ±0.314 | 0.038 ±0.242 | -0.105 ±0.106 | -1.0 |
| no-sector-reg (14) | 0.729 ±0.250 | 0.430 ±0.219 | 0.299 ±0.323 | 0.184 ±0.200 | +0.041 ±0.106 | +0.4 |
| no-threshold-reg (15) | 0.492 ±0.087 | 0.504 ±0.261 | -0.013 ±0.276 | 0.047 ±0.133 | -0.196 ±0.104 | -1.9 |
| no-predict (16) | 0.720 ±0.195 | 0.462 ±0.206 | 0.258 ±0.211 | 0.182 ±0.106 | +0.032 ±0.101 | +0.3 |
| no-good (17) | 0.340 ±0.294 | 0.462 ±0.206 | -0.122 ±0.188 | -0.122 ±0.142 | -0.348 ±0.151 | -2.3 |
| no-bad (17) | 0.432 ±0.248 | 0.462 ±0.206 | -0.030 ±0.138 | -0.005 ±0.036 | -0.256 ±0.150 | -1.7 |
| no-surprise (18) | 0.574 ±0.369 | 0.462 ±0.206 | 0.112 ±0.311 | 0.034 ±0.184 | -0.114 ±0.129 | -0.9 |
| no-grow (19) | 0.542 ±0.176 | 0.462 ±0.206 | 0.080 ±0.168 | 0.010 ±0.114 | -0.146 ±0.128 | -1.1 |
| no-prune (20) | 0.514 ±0.161 | 0.462 ±0.206 | 0.052 ±0.141 | 0.004 ±0.014 | -0.174 ±0.116 | -1.5 |
| no-far-redraw (6) | 0.771 ±0.166 | 0.462 ±0.206 | 0.309 ±0.153 | 0.176 ±0.087 | +0.083 ±0.098 | +0.8 |

Late score per seed, in seed order:

```
baseline           0.94 1.00 0.55 0.36 0.98 0.67 0.04 0.98
no-cooldown (3)    0.11 0.75 0.57 0.61 0.94 0.48 0.58 0.84
no-inhibition (4)  0.50 0.50 0.50 0.37 0.52 0.51 0.35 0.51
no-far (6)         0.54 0.98 0.55 0.12 0.74 0.35 0.71 0.94
no-delays (7)      0.62 0.91 0.70 0.77 0.72 0.55 0.01 0.68
no-branch (8)      0.26 0.98 0.55 0.60 0.60 0.28 0.57 0.98
linear-weights (9) 0.68 0.84 0.01 0.05 0.96 0.78 0.73 0.98
uniform-neurons (11) 0.71 0.98 0.59 0.75 0.81 0.63 0.53 0.63
no-pacemakers (12) 0.76 0.98 0.40 0.59 0.95 0.59 0.62 0.74
no-noise (13)      0.58 0.79 0.52 0.64 0.98 0.01 0.23 0.92
no-sector-reg (14) 0.89 0.99 0.48 0.86 0.99 0.34 0.52 0.76
no-threshold-reg (15) 0.49 0.61 0.50 0.51 0.49 0.50 0.30 0.53
no-predict (16)    0.54 0.88 0.59 0.70 0.96 0.54 0.56 0.99
no-good (17)       0.11 0.32 0.27 0.06 0.54 0.00 0.57 0.85
no-bad (17)        0.42 0.12 0.45 0.55 0.38 0.20 0.39 0.94
no-surprise (18)   0.54 0.88 0.59 0.07 0.94 0.00 0.61 0.95
no-grow (19)       0.47 0.53 0.84 0.55 0.54 0.29 0.39 0.73
no-prune (20)      0.35 0.56 0.43 0.40 0.59 0.42 0.50 0.86
no-far-redraw (6)  0.94 0.63 0.70 0.65 0.98 0.64 0.63 0.98
```

### delayed echo k=2

`--task delayed-echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 0.439 ±0.023 | 0.432 ±0.024 | 0.006 ±0.041 | -0.003 ±0.042 |  |  |
| no-cooldown (3) | 0.444 ±0.017 | 0.422 ±0.020 | 0.022 ±0.026 | -0.008 ±0.035 | +0.005 ±0.006 | +0.9 |
| no-inhibition (4) | 0.416 ±0.091 | 0.292 ±0.101 | 0.124 ±0.129 | 0.020 ±0.087 | -0.022 ±0.032 | -0.7 |
| no-far (6) | 0.427 ±0.020 | 0.452 ±0.015 | -0.025 ±0.024 | -0.007 ±0.033 | -0.012 ±0.008 | -1.4 |
| no-delays (7) | 0.437 ±0.016 | 0.435 ±0.021 | 0.002 ±0.024 | -0.014 ±0.034 | -0.002 ±0.004 | -0.4 |
| no-branch (8) | 0.441 ±0.029 | 0.448 ±0.020 | -0.007 ±0.029 | -0.017 ±0.031 | +0.002 ±0.014 | +0.1 |
| linear-weights (9) | 0.426 ±0.024 | 0.448 ±0.022 | -0.022 ±0.040 | -0.017 ±0.027 | -0.013 ±0.009 | -1.5 |
| uniform-neurons (11) | 0.441 ±0.014 | 0.425 ±0.020 | 0.016 ±0.025 | 0.005 ±0.019 | +0.002 ±0.012 | +0.2 |
| no-pacemakers (12) | 0.452 ±0.020 | 0.434 ±0.015 | 0.017 ±0.017 | -0.004 ±0.031 | +0.013 ±0.008 | +1.6 |
| no-noise (13) | 0.457 ±0.027 | 0.437 ±0.017 | 0.020 ±0.028 | 0.013 ±0.040 | +0.019 ±0.016 | +1.2 |
| no-sector-reg (14) | 0.422 ±0.035 | 0.428 ±0.016 | -0.007 ±0.043 | -0.032 ±0.029 | -0.017 ±0.015 | -1.1 |
| no-threshold-reg (15) | 0.479 ±0.014 | 0.454 ±0.030 | 0.025 ±0.031 | 0.008 ±0.033 | +0.041 ±0.011 | +3.8 |
| no-predict (16) | 0.446 ±0.022 | 0.432 ±0.024 | 0.013 ±0.025 | -0.001 ±0.029 | +0.007 ±0.012 | +0.6 |
| no-good (17) | 0.443 ±0.021 | 0.432 ±0.024 | 0.011 ±0.036 | -0.005 ±0.030 | +0.005 ±0.008 | +0.6 |
| no-bad (17) | 0.463 ±0.032 | 0.432 ±0.024 | 0.030 ±0.051 | 0.001 ±0.025 | +0.024 ±0.014 | +1.8 |
| no-surprise (18) | 0.465 ±0.028 | 0.432 ±0.024 | 0.032 ±0.035 | 0.027 ±0.037 | +0.026 ±0.013 | +2.0 |
| no-grow (19) | 0.441 ±0.020 | 0.432 ±0.024 | 0.008 ±0.032 | 0.008 ±0.028 | +0.002 ±0.010 | +0.2 |
| no-prune (20) | 0.450 ±0.020 | 0.432 ±0.024 | 0.018 ±0.033 | 0.002 ±0.025 | +0.012 ±0.010 | +1.1 |
| no-far-redraw (6) | 0.441 ±0.030 | 0.432 ±0.024 | 0.008 ±0.040 | 0.001 ±0.036 | +0.002 ±0.015 | +0.2 |

Late score per seed, in seed order:

```
baseline           0.45 0.42 0.42 0.45 0.42 0.48 0.45 0.42
no-cooldown (3)    0.43 0.43 0.44 0.47 0.43 0.47 0.45 0.44
no-inhibition (4)  0.49 0.45 0.50 0.42 0.43 0.47 0.23 0.34
no-far (6)         0.45 0.41 0.43 0.46 0.43 0.43 0.41 0.40
no-delays (7)      0.45 0.42 0.43 0.46 0.42 0.46 0.43 0.43
no-branch (8)      0.44 0.43 0.46 0.43 0.50 0.43 0.44 0.40
linear-weights (9) 0.42 0.39 0.40 0.46 0.44 0.45 0.41 0.43
uniform-neurons (11) 0.43 0.45 0.44 0.43 0.46 0.43 0.42 0.46
no-pacemakers (12) 0.44 0.43 0.44 0.45 0.45 0.47 0.48 0.47
no-noise (13)      0.41 0.45 0.49 0.44 0.49 0.44 0.45 0.48
no-sector-reg (14) 0.42 0.38 0.45 0.41 0.49 0.42 0.41 0.40
no-threshold-reg (15) 0.49 0.51 0.46 0.48 0.47 0.47 0.47 0.48
no-predict (16)    0.45 0.44 0.45 0.42 0.48 0.43 0.46 0.42
no-good (17)       0.42 0.44 0.42 0.46 0.47 0.46 0.45 0.41
no-bad (17)        0.44 0.46 0.44 0.50 0.46 0.48 0.41 0.50
no-surprise (18)   0.49 0.44 0.44 0.47 0.45 0.44 0.51 0.48
no-grow (19)       0.44 0.43 0.47 0.45 0.46 0.44 0.43 0.40
no-prune (20)      0.45 0.44 0.42 0.44 0.49 0.45 0.46 0.44
no-far-redraw (6)  0.42 0.43 0.44 0.47 0.50 0.42 0.43 0.42
```

## Seed 3, and why shuffled echo splits in two

The gate's seed 3 (echo k=2) scored 0.72 with learning and 0.72 without. They get there in
different ways. Choices per cue, from a probe that runs the same life:

```
learning off, every 10k steps alike:  cue 0: b0 0.58  b1 0.30  none 0.12
                                      cue 1: b0 0.10  b1 0.82  none 0.08
learning on, steps 30-40k:            cue 0: b0 0.37  b1 0.45  none 0.18
                                      cue 1: b0 0.00  b1 1.00
```

With learning on, button 1's group takes over: cue 1 is right every time and cue 0 mostly
presses button 1 too. Run for 160,000 steps it wanders between 0.54 and 0.80 and never
settles. The learning-off twin happens to have a born-in lean that is right more often than
chance on both cues. So seed 3 is a blob whose learning locked onto one button.

The same lock shows up much more in shuffled echo, and it follows the cue-to-button match.
The plug puts the cue lines at x = 12 and 18 and the buttons at x = 0 and 6, a tile row
below. In tiles: cue 0 sits over tiles 1-2, cue 1 over tile 2, button 0 under tile 0,
button 1 under tiles 0-1. The straight match (cue 0 to button 0, cue 1 to button 1, which is
plain echo) needs only one-tile paths. The crossed match needs cue 1 to reach button 0, two
tiles away. Seeds 11-18 split 3 straight (12, 15, 18) and 5 crossed:

| rule | straight lives | crossed lives | worst crossed life |
|---|---|---|---|
| brain | 0.99 | 0.51 | 0.04 |
| brain, reward_shift 2 | 0.99 | 0.64 | 0.60 |
| readout | 0.97 | 0.57 | 0.01 |
| flips any | 0.61 | 0.44 | 0.35 |
| flips motor | 0.86 | 0.56 | 0.34 |
| noise instead | 0.88 | 0.23 | 0.01 |
| noise plus | 0.89 | 0.54 | 0.18 |
| brain, good/bad lines silent | 0.97 | 0.34 | 0.12 |

Every rule has it, the readout included, so it comes from where the lines sit, not from one
learning rule. It is not the good/bad sensor lines, which sit right above the buttons:
silencing them (`--reward-ticks 0`, the broadcast unchanged) leaves the split. Seed 17
(crossed) learns the straight match and keeps strengthening it while punished every step
(cue 0 to button 0 rises from 0.55 to 0.87 of presses over the life); with good and bad both
off it stays near its born-in lean. So the reward rule drives it there: crossed lives feed
the one-tile path more than the two-tile one, and once that path is strong, bad can only take
it down to strength 1 while growth keeps rebuilding it from co-activity.

This matches `docs/notes-core.md` (learning reaches about one or two tiles). It also means:

- **Seeds 1-4 are all straight.** The effort doc's "shuffled echo 0.96" (seed 1) tested the
  same match as plain echo.
- **Plain echo k=2 is the one-hop case.** The gate holds for paths of one tile.

## Rivals

Each rival is a `learn_mode` on the same blob:

- **1, readout.** Nothing inside learns. Only connections into motor neurons move: the
  good/bad rule, growth and pruning, restricted to motor targets. No prediction learning.
- **2, random flips** (`src/blob/learn_flips.cpp`). Flip `flip_count` weight-plane bits into
  any non-sensor neuron (scope 0) or only into motor neurons (scope 1). Live
  `flip_window_ticks`. Keep them if the summed reward over the window beat the running mean of
  past windows, else flip them back. Nothing else learns. It runs inside the blob from
  `blob_reward`, so the runner did not change.
- **3, noise is the experiment** (`src/blob/learn_noise.cpp`). The fire step keeps a ring of
  spikes that came from noise alone. On a reward, compare it with a running mean of rewards
  (integer, x256). Better than the mean strengthens the recently active inputs of neurons
  whose latest spike in the last 16 ticks was noise; worse weakens them (down to 1, as bad
  does). Since rewards are +1 or -1, the sign always matches the reward; the mean sets how
  often an event acts (chance = |reward - mean| / 2), so a blob that is almost always right
  stops pushing on its good events. This replaces the good/bad rule; prediction, growth and
  pruning stay. **4** is the same alongside the good/bad rule.

Settings were picked on seeds 1-4 (echo k=2; tables at the end): readout at the default
`reward_shift` 4 (2, 4 and 6 within noise), flips 64 bits per 1,000 ticks (16 and 32 were
worse; 4 learned nothing), noise instead at `noise_learn_shift` 4, noise plus at 2. The brain
rule also got one tuned row: `reward_shift` 2 was best on seeds 1-4.

| rule | echo k=2 | echo k=4 | shuffled echo k=2 | delayed echo k=2 | cartpole |
|---|---|---|---|---|---|
| brain (default) | 0.940 | 0.326 | 0.688 | 0.439 | 10.6 |
| brain reward_shift 2 | -0.055 (-0.9) | -0.018 (-0.6) | +0.081 (+0.8) | -0.002 (-0.1) | +0.404 (+0.7) |
| readout | -0.026 (-0.8) | -0.168 (-2.9) | +0.031 (+0.2) | -0.005 (-0.3) | +0.890 (+1.6) |
| flips any 64/1000 | -0.383 (-5.4) | -0.140 (-3.1) | -0.188 (-1.5) | -0.008 (-0.6) | +8.201 (+6.2) |
| flips motor 64/1000 | -0.120 (-4.0) | -0.072 (-1.4) | -0.015 (-0.2) | +0.008 (+0.7) | +9.424 (+5.5) |
| noise instead s4 | -0.128 (-1.7) | -0.202 (-3.7) | -0.217 (-2.2) | -0.008 (-0.4) | -0.149 (-0.2) |
| noise plus s2 | -0.053 (-1.1) | -0.048 (-0.8) | -0.020 (-0.2) | -0.001 (-0.1) | +0.435 (+1.0) |

Reading:

- **Readout ties the full rule wherever the full rule works at k=2.** Echo k=2 0.91 against
  0.94 (t -0.8), shuffled 0.72 against 0.69. So on two buttons the inner learning (prediction
  learning, and good/bad, growth and pruning on inner neurons) adds nothing measurable. On
  echo k=4 the readout learns nothing at all (0.158, exactly its twin) while the full rule
  gets 0.326. That is the only place the inner learning earns its keep.
- **Random flips learn, slowly.** Into motor neurons they reach 0.82 on echo k=2 (twin
  0.50), clearly below the brain rule. Flips anywhere barely learn (0.56). They are the only
  rule that improves cartpole.
- **Noise as the experiment loses.** In place of good/bad it is worse on every task that
  learns, and very seed-dependent (shuffled lives at 0.01-0.07, echo k=4 below its own twin).
  Alongside good/bad it is indistinguishable from good/bad alone. One likely reason: at
  `noise_shift` 10 only about 4 of 4,096 neurons fire from noise per tick, and the noise
  ablation shows the brain rule does not need noise either.
- **The tuned brain row did not hold up.** `reward_shift` 2 was +0.11 on seeds 1-4 and is
  -0.06 on echo k=2 here (it does help the crossed shuffled lives: 0.64 against 0.51).
- **Delayed echo**: no rule learns it.
- **Cartpole**: no rule learns to balance. The brain rule, the readout and both noise rules
  end at 10.5-11.5 steps per fall, below their frozen twin (14.3) on average. Flips reach 18.8-20.0,
  4.5-5.7 above their twin on average (t 5.5-6.2 against the brain rule), but still not above
  a coin flip, which falls every 22-25
  steps (seeds 1 and 2 here, 23 in `docs/notes-world.md`). The reward is a good every 25
  steps alive and a bad on a fall; good/bad acts on the last 6 ticks, about a third of one
  step, while a fall comes from pushes several steps earlier. Why the brain rule ends
  worse than frozen was not looked into.

### echo k=2

`--task echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| brain (default) | 0.940 ±0.061 | 0.498 ±0.194 | 0.442 ±0.186 | 0.257 ±0.121 |  |  |
| brain reward_shift 2 | 0.885 ±0.146 | 0.498 ±0.194 | 0.387 ±0.253 | 0.190 ±0.171 | -0.055 ±0.062 | -0.9 |
| readout | 0.914 ±0.074 | 0.498 ±0.194 | 0.416 ±0.188 | 0.272 ±0.159 | -0.026 ±0.035 | -0.8 |
| flips any 64/1000 | 0.558 ±0.208 | 0.498 ±0.194 | 0.060 ±0.050 | 0.037 ±0.061 | -0.383 ±0.071 | -5.4 |
| flips motor 64/1000 | 0.821 ±0.084 | 0.498 ±0.194 | 0.322 ±0.152 | 0.277 ±0.155 | -0.120 ±0.030 | -4.0 |
| noise instead s4 | 0.813 ±0.245 | 0.498 ±0.194 | 0.315 ±0.212 | 0.191 ±0.145 | -0.128 ±0.075 | -1.7 |
| noise plus s2 | 0.887 ±0.098 | 0.498 ±0.194 | 0.389 ±0.180 | 0.188 ±0.120 | -0.053 ±0.048 | -1.1 |

Late score per seed, in seed order:

```
brain (default)    0.94 0.94 0.97 0.99 0.92 0.98 0.80 0.98
brain reward_shift 2 0.64 1.00 0.88 0.99 1.00 0.69 1.00 0.90
readout            0.92 0.86 0.93 0.94 0.76 0.96 0.99 0.96
flips any 64/1000  0.59 0.23 0.32 0.67 0.65 0.60 0.49 0.89
flips motor 64/1000 0.71 0.83 0.74 0.87 0.89 0.80 0.76 0.96
noise instead s4   0.80 0.37 0.94 0.97 0.99 1.00 0.49 0.93
noise plus s2      0.82 0.83 0.83 0.73 0.95 0.96 0.99 0.98
```

### echo k=4

`--task echo --k 4 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| brain (default) | 0.326 ±0.127 | 0.158 ±0.051 | 0.167 ±0.122 | 0.115 ±0.090 |  |  |
| brain reward_shift 2 | 0.307 ±0.115 | 0.158 ±0.051 | 0.149 ±0.109 | 0.130 ±0.080 | -0.018 ±0.032 | -0.6 |
| readout | 0.158 ±0.089 | 0.158 ±0.051 | -0.000 ±0.083 | -0.011 ±0.045 | -0.168 ±0.058 | -2.9 |
| flips any 64/1000 | 0.186 ±0.058 | 0.158 ±0.051 | 0.028 ±0.027 | 0.023 ±0.034 | -0.140 ±0.046 | -3.1 |
| flips motor 64/1000 | 0.253 ±0.075 | 0.158 ±0.051 | 0.095 ±0.088 | 0.092 ±0.088 | -0.072 ±0.051 | -1.4 |
| noise instead s4 | 0.124 ±0.107 | 0.158 ±0.051 | -0.034 ±0.070 | -0.021 ±0.054 | -0.202 ±0.054 | -3.7 |
| noise plus s2 | 0.278 ±0.169 | 0.158 ±0.051 | 0.120 ±0.129 | 0.039 ±0.152 | -0.048 ±0.061 | -0.8 |

Late score per seed, in seed order:

```
brain (default)    0.28 0.25 0.33 0.12 0.26 0.46 0.53 0.37
brain reward_shift 2 0.29 0.24 0.30 0.28 0.10 0.47 0.45 0.32
readout            0.29 0.03 0.25 0.14 0.16 0.19 0.06 0.13
flips any 64/1000  0.25 0.07 0.19 0.18 0.16 0.18 0.18 0.26
flips motor 64/1000 0.22 0.25 0.38 0.30 0.16 0.24 0.30 0.17
noise instead s4   0.16 0.03 0.16 0.11 0.03 0.01 0.14 0.34
noise plus s2      0.29 0.07 0.20 0.23 0.20 0.25 0.33 0.65
```

### shuffled echo k=2

`--task shuffled-echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| brain (default) | 0.688 ±0.354 | 0.462 ±0.206 | 0.226 ±0.365 | 0.126 ±0.235 |  |  |
| brain reward_shift 2 | 0.769 ±0.184 | 0.462 ±0.206 | 0.307 ±0.226 | 0.179 ±0.131 | +0.081 ±0.099 | +0.8 |
| readout | 0.719 ±0.334 | 0.462 ±0.206 | 0.257 ±0.321 | 0.199 ±0.184 | +0.031 ±0.126 | +0.2 |
| flips any 64/1000 | 0.500 ±0.208 | 0.462 ±0.206 | 0.038 ±0.057 | 0.039 ±0.054 | -0.188 ±0.126 | -1.5 |
| flips motor 64/1000 | 0.673 ±0.210 | 0.462 ±0.206 | 0.211 ±0.211 | 0.186 ±0.201 | -0.015 ±0.070 | -0.2 |
| noise instead s4 | 0.471 ±0.450 | 0.462 ±0.206 | 0.009 ±0.400 | 0.010 ±0.229 | -0.217 ±0.098 | -2.2 |
| noise plus s2 | 0.668 ±0.242 | 0.462 ±0.206 | 0.206 ±0.213 | 0.111 ±0.170 | -0.020 ±0.114 | -0.2 |

Late score per seed, in seed order:

```
brain (default)    0.94 1.00 0.55 0.36 0.98 0.67 0.04 0.98
brain reward_shift 2 0.60 0.97 0.60 0.69 1.00 0.66 0.64 1.00
readout            0.96 0.92 0.58 0.66 0.99 0.01 0.64 0.99
flips any 64/1000  0.42 0.25 0.59 0.41 0.68 0.35 0.42 0.89
flips motor 64/1000 0.70 0.82 0.75 0.34 0.89 0.61 0.40 0.88
noise instead s4   0.87 0.64 0.01 0.03 1.00 0.07 0.16 1.00
noise plus s2      0.69 0.76 0.62 0.61 0.96 0.18 0.59 0.94
```

### delayed echo k=2

`--task delayed-echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| brain (default) | 0.439 ±0.023 | 0.432 ±0.024 | 0.006 ±0.041 | -0.003 ±0.042 |  |  |
| brain reward_shift 2 | 0.437 ±0.020 | 0.432 ±0.024 | 0.004 ±0.028 | -0.018 ±0.021 | -0.002 ±0.014 | -0.1 |
| readout | 0.433 ±0.034 | 0.432 ±0.024 | 0.001 ±0.037 | -0.008 ±0.024 | -0.005 ±0.018 | -0.3 |
| flips any 64/1000 | 0.430 ±0.027 | 0.432 ±0.024 | -0.002 ±0.033 | -0.006 ±0.019 | -0.008 ±0.015 | -0.6 |
| flips motor 64/1000 | 0.447 ±0.020 | 0.432 ±0.024 | 0.014 ±0.024 | 0.014 ±0.025 | +0.008 ±0.011 | +0.7 |
| noise instead s4 | 0.431 ±0.039 | 0.432 ±0.024 | -0.002 ±0.038 | -0.008 ±0.032 | -0.008 ±0.019 | -0.4 |
| noise plus s2 | 0.437 ±0.028 | 0.432 ±0.024 | 0.005 ±0.037 | -0.005 ±0.030 | -0.001 ±0.016 | -0.1 |

Late score per seed, in seed order:

```
brain (default)    0.45 0.42 0.42 0.45 0.42 0.48 0.45 0.42
brain reward_shift 2 0.41 0.44 0.43 0.43 0.47 0.41 0.44 0.46
readout            0.44 0.43 0.45 0.39 0.50 0.40 0.41 0.45
flips any 64/1000  0.38 0.44 0.43 0.43 0.48 0.42 0.44 0.42
flips motor 64/1000 0.45 0.45 0.44 0.43 0.48 0.44 0.46 0.42
noise instead s4   0.41 0.41 0.41 0.44 0.48 0.37 0.49 0.44
noise plus s2      0.40 0.46 0.42 0.42 0.47 0.43 0.42 0.47
```

### cartpole

`--task cartpole --ticks 20 --sensor-side 2 --motor-side 3 --steps 100000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| brain (default) | 10.6 ±1.4 | 14.3 ±3.5 | -3.682 ±3.244 | -0.438 ±1.465 |  |  |
| brain reward_shift 2 | 11.0 ±1.7 | 14.3 ±3.5 | -3.278 ±3.629 | -0.516 ±1.998 | +0.404 ±0.613 | +0.7 |
| readout | 11.5 ±2.2 | 14.3 ±3.5 | -2.792 ±3.067 | -0.043 ±1.471 | +0.890 ±0.560 | +1.6 |
| flips any 64/1000 | 18.8 ±3.3 | 14.3 ±3.5 | 4.519 ±3.351 | 3.077 ±4.597 | +8.201 ±1.313 | +6.2 |
| flips motor 64/1000 | 20.0 ±4.3 | 14.3 ±3.5 | 5.742 ±7.097 | 3.862 ±7.918 | +9.424 ±1.702 | +5.5 |
| noise instead s4 | 10.5 ±1.8 | 14.3 ±3.5 | -3.831 ±4.205 | -3.100 ±3.622 | -0.149 ±0.794 | -0.2 |
| noise plus s2 | 11.0 ±1.5 | 14.3 ±3.5 | -3.246 ±3.032 | -0.634 ±1.412 | +0.435 ±0.439 | +1.0 |

Late score per seed, in seed order:

```
brain (default)    13 10 12 9.75 11 9.34 9.38 9.94
brain reward_shift 2 14 9.73 11 14 10 11 9.37 9.69
readout            13 9.83 14 13 15 9.35 9.38 9.60
flips any 64/1000  14 15 22 20 21 21 16 21
flips motor 64/1000 19 20 18 14 20 18 25 28
noise instead s4   9.31 9.51 9.46 9.93 14 9.53 9.38 13
noise plus s2      12 11 13 13 11 9.30 9.38 9.42
```

## Tried and found nothing

- **Rest ticks.** good/bad acts for 4 ticks after the step, while the next cue is already
  on, so its "recent" window mixes two steps. `--rest-ticks 6` (quiet ticks after each
  reward, an existing plug flag) changed nothing on seeds 1-4 of shuffled echo (brain -0.03,
  t -0.6). Not run further.
- **Silent good/bad lines** (`--reward-ticks 0`): echo k=2 -0.02 (t -0.4), shuffled -0.11
  (t -1.1). The broadcast does the work; the sensor lines add little or nothing.

### silent lines, echo k=2

`--task echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 0.940 ±0.061 | 0.498 ±0.194 | 0.442 ±0.186 | 0.257 ±0.121 |  |  |
| silent reward lines | 0.921 ±0.100 | 0.505 ±0.219 | 0.416 ±0.208 | 0.199 ±0.101 | -0.019 ±0.044 | -0.4 |

Late score per seed, in seed order:

```
baseline           0.94 0.94 0.97 0.99 0.92 0.98 0.80 0.98
silent reward lines 0.94 0.76 0.98 0.76 1.00 0.99 0.95 0.99
```

### silent lines, shuffled echo k=2

`--task shuffled-echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 11-18

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| baseline | 0.688 ±0.354 | 0.462 ±0.206 | 0.226 ±0.365 | 0.126 ±0.235 |  |  |
| silent reward lines | 0.576 ±0.387 | 0.449 ±0.228 | 0.127 ±0.397 | 0.069 ±0.290 | -0.112 ±0.099 | -1.1 |

Late score per seed, in seed order:

```
baseline           0.94 1.00 0.55 0.36 0.98 0.67 0.04 0.98
silent reward lines 0.58 0.95 0.14 0.67 0.98 0.17 0.12 0.99
```

## Tuning runs (seeds 1-4, echo k=2, no twins)

### tune echo k=2

`--task echo --k 2 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds 1-4

| variant | late | learning off | late - off | late - early | late vs base | t |
|---|---|---|---|---|---|---|
| brain | 0.851 ±0.100 | - | - | 0.186 ±0.132 |  |  |
| brain reward_shift 2 | 0.960 ±0.038 | - | - | 0.158 ±0.046 | +0.109 ±0.058 | +1.9 |
| brain reward_shift 6 | 0.847 ±0.220 | - | - | 0.141 ±0.271 | -0.004 ±0.070 | -0.1 |
| readout | 0.941 ±0.062 | - | - | 0.177 ±0.148 | +0.089 ±0.065 | +1.4 |
| readout reward_shift 2 | 0.915 ±0.077 | - | - | 0.151 ±0.101 | +0.064 ±0.075 | +0.9 |
| readout reward_shift 6 | 0.955 ±0.035 | - | - | 0.163 ±0.126 | +0.104 ±0.065 | +1.6 |
| flips any 4/1000 | 0.504 ±0.150 | - | - | -0.001 ±0.029 | -0.347 ±0.124 | -2.8 |
| flips any 16/1000 | 0.521 ±0.155 | - | - | 0.031 ±0.030 | -0.330 ±0.125 | -2.6 |
| flips any 4/4000 | 0.503 ±0.162 | - | - | -0.004 ±0.031 | -0.349 ±0.129 | -2.7 |
| flips motor 4/1000 | 0.545 ±0.138 | - | - | 0.033 ±0.061 | -0.306 ±0.117 | -2.6 |
| flips motor 16/1000 | 0.693 ±0.100 | - | - | 0.209 ±0.097 | -0.158 ±0.093 | -1.7 |
| flips motor 4/4000 | 0.508 ±0.185 | - | - | -0.003 ±0.008 | -0.343 ±0.141 | -2.4 |
| noise only s2 | 0.702 ±0.183 | - | - | 0.141 ±0.081 | -0.149 ±0.118 | -1.3 |
| noise only s0 | 0.773 ±0.258 | - | - | 0.205 ±0.069 | -0.078 ±0.160 | -0.5 |
| noise only s4 | 0.847 ±0.234 | - | - | 0.303 ±0.083 | -0.005 ±0.157 | -0.0 |
| noise only s2 noise8 | 0.625 ±0.228 | - | - | -0.029 ±0.291 | -0.226 ±0.086 | -2.6 |
| noise plus s2 | 0.917 ±0.089 | - | - | 0.225 ±0.140 | +0.066 ±0.081 | +0.8 |
| noise plus s0 | 0.860 ±0.094 | - | - | 0.156 ±0.111 | +0.009 ±0.018 | +0.5 |
| noise plus s2 noise8 | 0.741 ±0.147 | - | - | -0.035 ±0.269 | -0.110 ±0.079 | -1.4 |

Late score per seed, in seed order:

```
brain              0.89 0.84 0.72 0.95
brain reward_shift 2 0.94 0.92 1.00 0.98
brain reward_shift 6 0.92 0.98 0.52 0.97
readout            0.95 0.85 1.00 0.96
readout reward_shift 2 0.82 0.89 1.00 0.96
readout reward_shift 6 0.97 0.95 0.99 0.91
flips any 4/1000   0.49 0.51 0.69 0.33
flips any 16/1000  0.54 0.54 0.69 0.32
flips any 4/4000   0.53 0.51 0.69 0.29
flips motor 4/1000 0.56 0.53 0.72 0.38
flips motor 16/1000 0.75 0.63 0.80 0.59
flips motor 4/4000 0.49 0.52 0.73 0.28
noise only s2      0.75 0.45 0.89 0.72
noise only s0      1.00 0.53 0.99 0.57
noise only s4      0.93 0.99 0.97 0.50
noise only s2 noise8 0.46 0.67 0.44 0.93
noise plus s2      0.79 0.94 0.99 0.94
noise plus s0      0.89 0.90 0.72 0.93
noise plus s2 noise8 0.82 0.91 0.60 0.64
```

Flip counts, same seeds:

| variant | late |
|---|---|
| flips motor 16/1000 | 0.693 |
| flips motor 32/1000 | 0.719 |
| flips motor 64/1000 | 0.786 |
| flips motor 32/400 | 0.783 |
| flips any 64/1000 | 0.546 |

## Speed

`blobbench speed --tiles 8` (4,096 neurons), one pinned core. On a quiet machine, before
the switches: off 72,326 / 73,429 ticks/s, on 51,839 / 52,167; after the switches: off
74,887 / 74,573, on 52,947 / 52,926. After the rival code, with another agent's 4 runs on
the machine, the old build (a23f5a0) and the new one alternated on the same core, 5 runs
each: off 64,720 64,580 64,934 63,960 64,166 against 63,474 64,440 60,890 62,930 64,975; on
45,509 45,588 45,752 45,622 43,266 against 44,459 45,251 44,232 44,947 44,607. Within noise,
at most 1-2% slower (one extra store per tile for the noise ring, a variable shift in place
of `* 4`).

## Rough

- 8 seeds per cell and about 100 comparisons. The t column is a guide, not a test.
- "uniform neurons" sets threshold 8, leak 2, cooldown 2: near the middle of each spread,
  not exactly the mean.
- Linear weights keep thresholds tuned for 0,1,4,5.
- The readout rival still lets growth, pruning and far redraw run on motor neurons. A
  strictly level-only readout was not run.
- Flips choose any motor neuron, not only the 18 the plug reads, so most motor flips land
  on unread neurons. The blob does not know which motor neurons are read.
- `blob.fan_in` is not updated by flips (only growth reads it, and growth is off in that
  mode).
- No sweep reran the coin baseline per seed; the cartpole coin figure is from seeds 1-2 and
  the world notes.
