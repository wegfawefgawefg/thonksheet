# One setting with everything that worked (plan item A, 2026-11-26)

The mechanisms found one at a time on 2026-11-25 (the output rule, credit one step back,
tiring neurons, a slow potential, time and hold cells, winners in the tile, the critic, the
good half, inhibition kept local, the rich2 sheet), plus the two that landed while this ran
(tags from the chosen action, the margin gate), each run alone and then together on one quick
map of 22 worlds, with one setting unchanged on every world, beside the GRU, the memory
engine and the vm engine.

## Short version

- **They hold together, mostly.** One setting, unchanged on all 22 worlds, passes **15** of
  them on the report seeds (5-12): the output rule with the margin gate, time and hold cells,
  tiring neurons, two winners per tile, and one step of the backward walk on bad steps
  (`scripts/variants/best.txt`). The plain output rule passes 6. The GRU passes 16, the
  memory engine 16, the vm engine 14.
- A row passes when its late score is at least halfway from chance to perfect and above its
  own learning-off twin by more than 2 standard errors (8 seeds, paired).
- **What the one setting still cannot do** (fails, report seeds): cartpole (17 steps per
  fall, a coin gets 21, the GRU 703), the two-step chain (0.15; only the critic gets it,
  0.96), parity of 3 lines (0.64), noisy lines (0.31, chance 0.23), the held-out scale and
  place (0.60 and 0.47, no better than chance or the base; nothing passes these, the
  references included), and keeping pairs (0.53, the memory engine 0.98).
- **The critic does not combine.** Alone it is the only thing that learns the chain (0.96)
  and lifts cartpole (77), but it costs exclusive-or 0.27, order 0.15, reach 0.19, and with
  time and hold cells it breaks every time row (the cells never clear: the critic swallows
  the world's verdict). Three glue versions narrowed the cost; none removed it. It is not in
  the one setting.
- **Health over 160,000 steps**: no runaway with the walk inside the one setting (rate 0.89
  of target on echo at 3 tiles; the walk alone 1.24). But **12% of neurons are silent** at
  the end of a long life under the one setting, where the base has none.
- **Inside learning, a little**: after a life under the one setting, a fresh output on the
  frozen lived sheet beats a fresh output on the sheet as born at 5 tiles (0.933 against
  0.891, +0.042 ±0.012) and, borderline, on exclusive-or (0.941 against 0.915, +0.026
  ±0.013). Same task, not cases never shown.
- Lives are 20,000 steps, half the standard 40,000 (the 1,000-tick hold keeps 40,000), to fit
  the time budget; the output rule's scores at 20,000 are within 0.05 of 40,000 on every row.

## The quick map

`scripts/variants/oneset.txt` (rows `row NAME: flags`, columns `NAME: flags`),
`scripts/oneset.sh NAME [flags]`, `scripts/oneset.py`. Every row runs on `--ticks 20
--sensor-side 3 --motor-side 3 --steps 20000 --window 1000` with its own world flags after.

| row | world | what it asks |
|---|---|---|
| echo2, echo8, echo16 | `--task echo --k 2/8/16` | association, 2 to 16 cues |
| shuffled | `--task shuffled-echo` | association that is remapped |
| far3, far5 | `--task echo --distance 3/5` | reach: cue 3 or 5 tiles from the buttons |
| xor | `--task both-not-either` | combination of two |
| parity3 | `--task points --fn parity --fn-lines 3` | combination of three |
| order, gap | `--task order`, `--task gap` | time features |
| delayed2 | `--task delayed-echo --delay 2` | hold 40-60 ticks |
| hold1000 | `--task delayed-echo --delay 50 --steps 40000` | hold about 1,000 ticks |
| bandit, chain | `--task bandit`, `--task chain` | acting; chain is paid one step late |
| cartpole | `--task cartpole --sensor-side 2 --cart-reward lean --number-code place` | control (fixed integrator) |
| catch | `--task catch --catch-size 4 4` | acting on a moving dot |
| noisy | `--task noisy-lines --k 4 --item-lines 8 --flip-pct 25` | association under noise |
| copy2 | `--task copy --k 2 --length 2` | holding one step, output over steps |
| inv-scale, inv-position | `--task inv-scale/inv-position --sensor-side 1` | held-out versions (scored only on them) |
| pairs | `--task pairs --sensor-side 2 --pair-every 100` | keeping: a new pair every 100 steps, probed later unrewarded |
| mnist | `--task mnist --sensor-side 1 --mnist-code t28` + the 64x64 sensor and motor region | 20,000 digits, each seen once: the online score on unseen digits |

Chance and perfect come from coin flips and the scripted player (memory-engine lives with
baselines, seeds 5-12). The data path for MNIST is `data/mnist` or `$THONK_MNIST`.

What was cut to fit the budget: lives halved to 20,000 steps (checked: base on seeds 1-4,
same lives, late at 20,000 against 40,000 is within 0.05 on every row and the same rows pass);
the pairs world runs a pair every 100 steps instead of 400 (at 400 the late windows hold too
few probes to score); hold1000 keeps 40,000 steps. No row was dropped.

How long a pass takes (all measured, both machines shared with other agents' runs):
- stevejobs, 6 jobs, load 20 to 65: the base column with its twins (352 lives plus 8
  calibration lives) took 64 minutes. At 40,000 steps the same was heading for about 2.4
  hours, which is why lives were halved. A column that changes only learning numbers shares
  the twins and runs half the lives. Under the new cap (2 jobs there) a pass is several hours;
  the full engine belongs on this PC.
- this PC, 3 jobs: the one setting with twins (352 lives) 35 minutes; a column that shares
  twins (176 lives) 20 minutes; seeds 1-4 alone 9 to 10 minutes.
- So the 40-minute budget at 6 jobs on stevejobs is met only for columns that share twins, or
  at that machine's quieter load. On this PC at 3 jobs it is met.

Where a world could not be plugged without changing a group size: MNIST needs the sensor
region to be the whole sheet (`--g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1
--g.motor_w 64 --g.motor_h 64`; 784 lines do not fit the default region). These are written in
the MNIST row, the same for every column. `--motor-spread 128` is the setting's for every
row; where fewer cells fit it gives all that fit (MNIST: 102 a button; 16 cues: 16).

## Baselines and each mechanism alone (seeds 5-12)

Late score; `*` passes; in brackets the paired difference to the base with its standard error,
`+`/`-` at 2 standard errors or more. Base: `--g.learn_mode 5 --g.readout_evidence 2
--g.readout_shift 4 --g.readout_balance 1 --motor-spread 128`. Every mechanism column is the
base plus its own flags.

| column | flags on top of the base | rows passed |
|---|---|---|
| base (output rule, frozen sheet) | | **6** |
| gru | `--engine gru` (not on the base) | **16** |
| memory | `--engine memory` | **16** |
| vm | `--engine vm` | **14** |
| replay1 | `--g.replay_depth 1` | 8 |
| tags | `--g.tag_mode 2 --g.tag_inh 0 --g.tag_min 4` | 8 |
| tire | `--g.tire_jump 4 --g.tire_shift 7` | 9 |
| slow | `--g.slow_leak_shift 6 --g.slow_share_shift 4` | 8 |
| timehold | `--g.time_percent 10 --g.hold_percent 10` | **12** |
| winners2 | `--g.tile_winners 2` | 7 |
| critic | `--g.critic_on 1 --g.readout_good 1` | 6 |
| critickw | critic with glue: `--g.critic_on 1 --g.critic_keep_world 1 --g.actor_good 1` | 6 |
| good | `--g.readout_good 1` | 1 |
| margin | `--g.readout_good 1 --g.margin_percent 20 --g.margin_win 2` | 8 |
| inhlocal | `--g.wire_inhib_reach -3` | 6 |
| rich2 | the rich2 sheet's numbers | 10 |
| c3tw (best without the walk) | | 14 |
| **best** | `scripts/variants/best.txt` | **15** |

Which rows each mechanism helps and costs against the base (2 standard errors or more; the
full tables with every number are `scripts/oneset.sh table base,replay1,... --se`, copied in
the appendix):

- **replay1** (the walk, depth 1): helps echo8 +0.05, far3 +0.09, far5 +0.26, xor +0.08,
  parity3 +0.10; costs order -0.04, noisy -0.04, MNIST -0.05.
- **tags**: helps echo8 +0.13, echo16 +0.08, far5 +0.20, xor +0.11, parity3 +0.08, chain
  +0.36, catch +0.06; costs order -0.04, delayed2 -0.02, copy2 -0.05, inv-scale -0.05,
  inv-position -0.03, MNIST -0.04. Tags and the walk are near twins: both lift reach and
  conjunction; tags more on many cues and the chain, the walk more at 3 tiles.
- **tire** (tiring neurons): helps order +0.15, gap +0.15, delayed2 +0.34, chain +0.32,
  copy2 +0.21; costs xor -0.09.
- **slow** (slow potential): helps order +0.14, gap +0.16, delayed2 +0.26; costs echo8 -0.19,
  echo16 -0.08, far3 -0.08, xor -0.15, the chain (to 0.00), MNIST -0.11. Worse than tiring
  neurons everywhere.
- **timehold** (time and hold cells): the largest single gain. Helps echo8 +0.14, echo16
  +0.17, far3 +0.14, far5 +0.34, xor +0.04, order +0.23, gap +0.34, delayed2 +0.46, hold1000
  +0.49, catch +0.16, copy2 +0.19; costs the chain (0.31 to 0.00), inv-scale -0.12, MNIST
  -0.02. It lifts reach because hold cells keep the cue's identity on for the whole trial.
- **winners2**: helps far3 +0.06, xor +0.09, parity3 +0.10, chain +0.16, catch +0.05; costs
  noisy -0.04, copy2 -0.04.
- **critic** (as its doc gives it): helps bandit +0.23, chain +0.66 (0.96, the only column
  that learns it), cartpole +60 (77 steps per fall), noisy +0.22, pairs +0.13, MNIST +0.33;
  costs echo2 -0.03, far3 -0.19, xor -0.27, order -0.15, inv-scale -0.11 and more.
- **good** (readout_good alone): locks. echo2 0.70, shuffled 0.71, xor 0.53. Passes 1 row.
  Finding 17 again.
- **margin** (the good half only on a close vote): helps echo8 +0.09, echo16 +0.29, order
  +0.07, bandit +0.23, catch +0.11, noisy +0.19, copy2 +0.07, pairs +0.24, MNIST +0.19; costs
  far3 -0.04, xor -0.07, cartpole -2. No locks.
- **inhlocal**: helps parity3 +0.06, gap +0.08; costs echo8 -0.14, far3 -0.09, inv-scale -0.13.
- **rich2**: helps xor, parity3, order, gap +0.35, delayed2 +0.33, copy2; costs echo2 -0.11
  (shuffled -0.10), echo8 -0.14, far5 -0.08, inv-scale -0.16, MNIST -0.08.

## Putting them together (seeds 1-4)

Greedy, one piece at a time: at each step every candidate was added to the current setting
and run on seeds 1-4; the step kept the one that passed most rows, and a piece was kept only
if it did not cost more rows than it gained. The order candidates were tried in came from
the singles' gains on the report seeds (only the order; every keep or drop was decided on
seeds 1-4). Tables: `scripts/oneset.sh table c1,c1margin,... --seeds 1-4` (appendix).

| step | setting | rows passed (1-4) | candidates tried at this step (rows) |
|---|---|---|---|
| 0 | base | 6 | |
| 1 | + time and hold cells (c1) | 12 | |
| 2 | + margin gate (c1margin) | **13** | tags 12, winners 12, walk 12, tiring 12, critic 3, critic glued 7, critic glued with margin 16: 11 |
| 3 | + tiring neurons (c2tire) | **15** | winners 15, tags 13, walk 4 (runaway, see fights), walk on bad only 14 |
| 4 | + two winners per tile (c3tw) | 15 | tags 14, walk on bad only 15 |
| 5 | + the walk on bad only (best) | 15 | tags 14, inhibition kept local 14 |

At step 4 winners and the walk tied at 15; winners went first because their gains were
larger (xor +0.13 against +0.12, copy2 +0.05 against +0.01, MNIST +0.10 against +0.01).
Steps 4 and 5 add no row on seeds 1-4: winners lift exclusive-or +0.13, parity3 +0.07, copy2
+0.05 and MNIST +0.10 against noisy -0.08; the walk adds parity3 +0.02 and inv-scale +0.03
against noisy -0.06 and bandit -0.005. The walk was kept on a tie because it is the piece
that changes the inside, which is the question the plan asks; on the report seeds it is worth
far5 +0.04, xor +0.03, catch +0.05 (catch passes) against noisy -0.05 and pairs -0.03, one
row more than c3tw (15 against 14).

## Which pieces fight, and why

- **The critic swallows the world's verdict.** With `critic_on` the world's good and bad
  never reach `pending_valence`; the surprise teaches the actor instead. So the backward walk
  never runs, and the integrator (hold) cells, which clear on any good or bad, never clear:
  they count every onset of the life and fire for good. Time and hold cells with the critic:
  3 rows on seeds 1-4 (order, gap, delayed2, hold1000 all to chance). Glue
  `critic_keep_world 1` lets the verdict through as well; that fixes the clearing.
- **The critic's surprise is noise on steps the world does not judge.** Even glued, the
  actor teaches the presses on cue and blank steps by a surprise whose spread (about 30
  spikes, critic doc) is larger than a reward (16). In the time worlds most steps are such
  steps: with glue `critic_keep_world 1 --actor_good 1 --actor_unjudged 1` (the surprise
  teaches only presses the world left unjudged) exclusive-or drops 0.95 to 0.63, order 0.97
  to 0.57, hold1000 1.00 to 0.52; a margin of 16 spikes halves the damage (xor 0.70, order
  0.66) and still costs a row. The chain needs exactly this credit for an unjudged press, so
  the chain and the time worlds want opposite things from the same signal. A critic that is
  quiet when nothing is at stake is what is missing.
- **The walk runs away on every good once good moves the output.** `learn_replay` walks on
  good whenever `readout_good` is on, and the margin gate needs `readout_good`. Time and hold
  cells + margin + the walk: echo2 1.00 to 0.63, shuffled to 0.62, 4 rows on seeds 1-4. Glue
  `replay_bad_only 1` (walk on bad only): back to 14-15 rows.
- **Tiring neurons cost exclusive-or; winners pay it back.** On the margin setting tiring
  neurons take xor 0.87 to 0.80; two winners per tile then take it to 0.93 (seeds 1-4), and
  the pair keeps order 1.00 and delayed2 1.00. This is the fight the dynamics doc predicted,
  settled.
- **Tags and the margin gate**: on the time-and-hold setting tags cost reach (far5 -0.18,
  far3 -0.07) and gain xor/parity; as the next piece after tiring neurons they lose a row
  (catch) and cost copy2. No third piece settled it: with tiring neurons and winners present
  tags still cost far3 -0.07 and order -0.03 and gain noisy +0.10. In this setting the walk is
  the better inner rule, by one row.
- **Time cells and the held-out scale**: every setting with time and hold cells loses
  inv-scale (0.70 to 0.50-0.60). Not understood; the cells are fed from the sensor cells, so
  a shape seen at a new size starts a different set of cells.
- **The chain under slow traces**: time and hold cells, and the slow potential, take the
  chain from 0.31 to 0.00. A guess: the stage line's trace lingers into the other stage, so
  the output answers B in both stages and never starts a chain. Not checked.

## The one setting on the map (seeds 5-12)

`scripts/oneset.sh best`. Beside the base, the critic and the three references; in brackets
the paired difference to the base and its standard error.

| row | base | best | critic | gru | memory | vm |
|---|---|---|---|---|---|---|
| echo2 | 1.000* | 1.000* | 0.975* | 1.000* | 0.993* | 1.000* |
| echo8 | 0.801* | 0.976* (+0.176 ±0.013) | 0.841* | 0.998* | 0.984* | 1.000* |
| echo16 | 0.266 | 0.955* (+0.689 ±0.012) | 0.387 | 0.992* | 0.982* | 1.000* |
| shuffled | 1.000* | 1.000* | 0.979* | 1.000* | 0.993* | 1.000* |
| far3 | 0.847* | 0.975* (+0.128 ±0.016) | 0.654 | 1.000* | 0.993* | 1.000* |
| far5 | 0.576 | 0.933* (+0.357 ±0.011) | 0.550 | 1.000* | 0.993* | 1.000* |
| xor | 0.882* | 0.936* (+0.054 ±0.009) | 0.616 | 0.988* | 0.993* | 0.951* |
| parity3 | 0.561 | 0.642 (+0.081 ±0.011) | 0.520 | 0.518 | 0.994* | 0.829* |
| order | 0.676 | 0.994* (+0.318 ±0.012) | 0.529 | 0.998* | 0.993* | 0.742* |
| gap | 0.509 | 0.897* (+0.387 ±0.032) | 0.485 | 0.994* | 0.995* | 0.476 |
| delayed2 | 0.544 | 1.000* (+0.456 ±0.007) | 0.519 | 0.999* | 0.993* | 1.000* |
| hold1000 | 0.506 | 0.998* (+0.492 ±0.022) | 0.482 | 0.495 | 0.998* | 1.000* |
| bandit | 0.773* | 0.995* (+0.223 ±0.003) | 1.000* | 1.000* | 0.988* | 0.767* |
| chain | 0.306 | 0.150 (-0.155 ±0.130) | **0.964*** | 0.010 | 0.000 | 0.000 |
| cartpole (steps per fall) | 17 | 17 (-0.5 ±0.6) | 77 | **703*** | 298 | 32 |
| catch | 0.298 | 0.671* (+0.373 ±0.031) | 0.315 | 0.723* | 0.991* | 0.523 |
| noisy | 0.268 | 0.307 (+0.039 ±0.026) | 0.490 | 0.666* | 0.598 | 0.394 |
| copy2 | 0.638 | 0.897* (+0.259 ±0.022) | 0.610 | 0.998* | 0.837* | 0.964* |
| inv-scale | 0.699 | 0.602 (-0.097 ±0.057) | 0.585 | 0.569 | 0.470 | 0.514 |
| inv-position | 0.487 | 0.471 (-0.016 ±0.023) | 0.495 | 0.388 | 0.347 | 0.256 |
| pairs | 0.273 | 0.525 (+0.252 ±0.039) | 0.406 | 0.338 | 0.977* | 0.746* |
| mnist | 0.214 | 0.714* (+0.499 ±0.007) | 0.546* | 0.733* | 0.158 | 0.118 |
| **rows passed** | **6** | **15** | **6** | **16** | **16** | **14** |

Chance per row, in table order (coin flips): 0.47, 0.11, 0.05, 0.48, 0.48, 0.48, 0.47,
0.48, 0.48, 0.47, 0.48, 0.47, 0.48, 0.30, 21, 0.25, 0.23, 0.47, 0.47, 0.48, 0.25, 0.09.

- The one setting passes every row the GRU passes except cartpole and noisy, and one the GRU
  does not (hold1000, past the GRU's 8-step window). It
  passes MNIST's halfway line (0.71 online on unseen digits; the GRU 0.73), where the memory
  and vm engines sit near chance.
- Its per-seed MNIST is tight (±0.016); the MNIST results doc's tuned readout got 0.71 on the
  test set with its own per-world settings; here the same 0.71 comes from the one setting.
  A test-set number for the one setting was not taken (below).

## Health after a long life

`bash out/oneset/health.sh "base replay1 best" 3`: 160,000 steps (3.2M ticks), seeds 5-8, no
twin. End of life, mean of 4 seeds.

| setting | world | late | rate / target | silent | stuck | strength 5 | threshold at cap | ticks/s |
|---|---|---|---|---|---|---|---|---|
| base | echo, 3 tiles | 0.862 | 1.03 | 0 | 0.00007 | 0 | 0.008 | 53,600 |
| base | delayed echo | 0.552 | 0.98 | 0 | 0 | 0 | 0.004 | 52,000 |
| walk alone (depth 1) | echo, 3 tiles | 0.977 | **1.24** | 0 | 0.011 | 0.013 | 0.053 | 60,600 |
| walk alone | delayed echo | 0.579 | 0.98 | 0 | 0 | 0.004 | 0.004 | 48,600 |
| **best** | echo, 3 tiles | **0.987** | 0.89 | **0.126** | 0.005 | 0.002 | 0.018 | 26,800 |
| **best** | delayed echo | **1.000** | 0.90 | **0.122** | 0.008 | 0 | 0.021 | 28,800 |

- **No runaway in the one setting.** The walk alone at depth 1 is already running warm on
  reach after 160,000 steps (1.24 of target, 5% of thresholds at the cap, 1.3% of connections
  at strength 5; the replay doc's depth 2 reached 3.5x). Inside the one setting the same walk
  ends at 0.89 of target with 0.2% at strength 5: tiring neurons and two winners per tile cap
  how many neurons can fire together, which is what the walk fed on.
- **But 12% of neurons are silent** at the end of the life, on both worlds, and the rate sits
  10% under target. Not diagnosed. Likely winners: with two spikes kept per 64-neuron tile per
  tick, the rate target is exactly the cap, and neurons that never win the compare never fire
  (the time and hold cells replace a further 20% of excitatory neurons' own spikes, which may
  count as silent when their inputs are quiet). Worth checking before anything else uses this
  setting for long lives.
- Speed: the one setting runs at about half the base's ticks per second (busy machine, rough).

## Does the inside learn?

`bash out/oneset/inside.sh 3`, seeds 5-12. A life of 20,000 steps with the one setting; then
`blobprobe transfer --keep-output 1` freezes the inside (the walk stops; time, hold, tiring
and winner cells keep running, they are dynamics, not learning), puts every motor cell's
wiring back as born, wipes the output's counters, and a fresh output (the same rule, margin
gate included) learns the same task for 20,000 steps. Against the same fresh output on the
sheet as born. Late score of the fresh output; first quarter of its life beside it.

| row | lived sheet | born sheet | lived - born | first quarter, lived / born |
|---|---|---|---|---|
| xor | 0.941 | 0.915 | +0.026 ±0.013 | 0.854 / 0.770 |
| far5 | 0.933 | 0.891 | **+0.042 ±0.012** | 0.907 / 0.865 |

- **Yes, a little, on the task it lived.** At 5 tiles the lived sheet is better for a fresh
  output by 3.5 standard errors, and on exclusive-or by 2.0 (borderline); both learn faster on
  the lived sheet (+0.08 and +0.04 in the first quarter).
- It is much less than the walk showed on the plain frozen sheet (exclusive-or 0.874 born to
  0.997 lived, replay doc): here the born sheet is already a better sheet (0.915 on xor with
  time cells, tiring neurons and winners), and the walk runs only on bad steps, which the
  margin setting makes rarer.
- This is the same task after the life, not cases never shown; transfer to another task and
  the random-projection comparison of the plan's test were not run.
- The first try of this test ran the fresh output without the margin gate (the probe's
  default: good always moves), which locks; those runs are discarded
  (`out/oneset/inside-bad/`), and `--keep-output 1` was added for this.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, "Where it stands": a new line: the mechanisms combine; one setting
  (output rule + margin gate + time and hold cells + tiring neurons + two winners + one step of
  the walk on bad) passes 15 of 22 quick-map worlds on report seeds where the output rule alone
  passes 6, the GRU 16, the memory engine 16, the vm engine 14. It fails cartpole, the chain,
  parity of 3, noisy lines, keeping pairs and both held-out rows.
- `docs/findings.md`, principle 30 (holding needs a slow variable): time and hold cells are
  the largest single piece (6 rows alone) and also lift reach (far5 0.58 to 0.92) because hold
  cells keep the cue on for the trial. Second result for "winners pay back what tiring
  neurons cost on exclusive-or" (xor 0.80 to 0.93 with both).
- `docs/findings.md`, principle 31 (an actor needs a critic): the critic is the only route to
  the chain, and it does not combine: its surprise is noise on steps nothing is at stake, and
  the time worlds are mostly such steps. A new wall: "a critic that is quiet when nothing is
  at stake". Also record the plumbing fact: `critic_on` hides the world's verdict from every
  other rule (the walk, the integrator cells).
- `docs/findings.md`, principle 17: readout_good alone locks on the quick map (echo2 0.70,
  xor 0.53); the margin gate is the version that works (third result).
- `docs/findings.md`, principle 29 (credit one step back): inside the one setting the walk
  does not run away over 160,000 steps; tiring neurons and winners hold it. The walk must not
  run on good (glue `replay_bad_only`).
- `docs/plan.md`: item A done (this doc). Next for the one setting: the silent 12%; a critic
  gated by stakes; the chain; the inv-scale cost of time cells; the test-set MNIST number.
- `docs/ideas.md`: new ideas: a critic whose surprise teaches only when its own level moved
  more than its usual noise (stakes); hold cells fed by inner neurons, not only sensor cells
  (to stop the inv-scale cost); a rate-aware winners rule (k from the target, not fixed 2).
- `docs/predictions.md`: none was written for this job.
- `docs/task-map.md`: "Every row of tiers 1 to 4 on one set of rules" is now met by a sheet,
  not only by the memory engine, on the quick map's tier 1-4 rows (echo to 16, shuffled,
  reach to 5, xor, order, gap, delayed, a 1,000-tick hold), except parity of 3.

## Main has moved on

Main gained, after this branch's last merge (0e363b4): the runner-up output rule
(`readout_good 2`), the one-hop rule, the competitive-learning follow-up, prediction cells, the
inside's ladder and the transplant flags. Every result here was made without them. Files
both sides changed, likely to collide on merge: `src/blob/genome.hpp` and
`src/blob/genome_args.cpp` (new numbers at the end on both sides), `scripts/sweep.py`
(LEARNING_ONLY), `src/blob/critic.hpp`, `src/blob/learn_actor.cpp`,
`src/blob/learn_critic.cpp`, `src/blob/birth.cpp`, `src/probe/lived_sheet.cpp` (main
changed 180 lines; this branch added `--keep-output` and the replay freeze there).

## Glue added (all off by default, byte-identical when off)

- `critic_keep_world 1`: under the critic the world's good and bad still reach the readout, the
  walk and the integrator cells (`src/blob/birth.cpp`, `blob_reward`).
- `actor_good 1`: a good surprise teaches the actor's good half without `readout_good`
  (`learn_actor.cpp`).
- `actor_unjudged 1`: the surprise teaches only presses the world left without a verdict
  (`learn_actor.cpp`, a `judged` flag per kept step in `critic.hpp`, set in
  `learn_critic.cpp`).
- `replay_bad_only 1`: the backward walk runs on bad only, even with `readout_good`
  (`learn_replay.cpp`). This one is in the one setting.
- `blobprobe transfer`: now also stops the walk when it freezes the inside, and takes
  `--keep-output 1` to keep the life's margin gate for the fresh output (default unchanged).

Checked after each: the four reference lives in `out/merge-ref/` (`--task echo --steps 4000
--window 1000 --seed 5`, the same with the readout on both-not-either, `--engine minimal`,
`--table`) `cmp`-equal, plus a critic cartpole life and a replay life with readout_good
equal to the build before.

## Not done

- The MNIST test-set accuracy of the one setting (`blobmnist online` with best's flags); the
  quick map's MNIST row is the online score on unseen training digits.
- A 40,000-step (standard) pass of the one setting; everything here is 20,000 steps.
- Why 12% of neurons go silent in long lives under the one setting.
- A critic version that combines (the three glue tries are above); the critic doc's own report
  seeds are another agent's job now.
- The slow potential, rich2 and inhibition kept local were tried only as singles and (inhlocal)
  as a fifth piece; rich2 was not tried in combination.
- The singles were not run on seeds 1-4; the order of trying them came from the report seeds.

## Script lines

```
scripts/build.sh --target blobrun blobprobe
scripts/oneset.sh best                                   the one setting on the quick map, seeds 5-12
scripts/oneset.sh base --seeds 1-4                       any column, the tuning seeds
scripts/oneset.sh mine --g.learn_mode 5 ...             any flags
scripts/oneset.sh table base,best,gru,memory,vm --se     side by side, with standard errors
JOBS=3 scripts/oneset.sh best                            3 at a time (this PC's cap)
bash out/oneset/health.sh "base replay1 best" 3          long-life health (worktree helper)
bash out/oneset/inside.sh 3                              the inside-learning test (worktree helper)
```

On stevejobs: `MINI_DIR=thonk-oneset scripts/mini.sh push`, then
`THONK_MNIST=$HOME/thonk-mnist/data/mnist python3 scripts/oneset.py run NAME --jobs 2`.
Raw lives and the cache: `out/oneset/` here and `~/thonk-oneset/out/oneset/` there (pulled as
`out/oneset/cache-mini.jsonl`).

## Rough

- Lives are half length. The output rule's scores barely move between 20,000 and 40,000, but
  slower pieces (counters still rising at the end) may be under-rated.
- hold1000 and pairs score few asks per window; their chance column comes from 8 coin lives
  and still wobbles (pairs 0.25, hold 0.47).
- The pass line uses the coin as chance; on rows where the twin sits under the coin (catch,
  the chain) passing the twin is easy and the halfway line is the binding one.
- The walk was kept on a tie on seeds 1-4.
- The critic was run with the flags its results doc gives (`--g.critic_on 1 --g.readout_good
  1`) on report seeds as a single; its own report-seed job is still running elsewhere.
- Speed numbers come from shared machines.

## Appendix: tables

Singles against the base, seeds 5-12, each paired difference with its standard error:
`out/oneset/singles-a-se.md`, `out/oneset/singles-b-se.md` (and the rich2 column by
`scripts/oneset.sh table base,rich2 --se`). Steps of the combination, seeds 1-4:
`out/oneset/step2.md` to `step5.md` and `out/oneset/path.md`. They are reproduced from the
cache by the `table` lines above; the numbers in this doc were copied from them.
