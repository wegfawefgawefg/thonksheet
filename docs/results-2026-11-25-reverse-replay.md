# Credit walks backward through the recorded ticks (idea 62, 2026-11-25)

When the readout from idea 58 is wrong, its error is walked back through the last 16
spike maps along the edges that carried a spike, and the neurons on those paths move hidden
evidence counters for their own inputs. This is the first thing in the project that teaches
neurons inside the sheet from an outcome and makes the readout better rather than worse.

## Short version

- **It helps reach, exclusive-or and eight cues; it costs order and gap a little.** Seeds
  5-12, the walk at depth 2 (the setting chosen on seeds 1-4) against the frozen-sheet
  readout it sits on:

  | task | frozen sheet + readout | walk, depth 2 | walk, depth 1 | t (depth 2) |
  |---|---|---|---|---|
  | echo k=2, 2 tiles | 0.905 | 0.945 | | +2.3 |
  | echo k=2, 3 tiles | 0.853 | 0.928 | 0.943 | +3.4 |
  | echo k=2, 5 tiles | 0.587 | 0.802 | **0.876** | +7.1 |
  | both-not-either | 0.883 | **0.985** | 0.985 | +8.0 |
  | echo k=8 | 0.842 | **0.924** | | +11.2 |
  | order | 0.695 | 0.664 | 0.673 | -3.5 |
  | gap | 0.519 | 0.491 | | -3.1 |
  | order, delays 4-8 | 0.839 | 0.840 | | +0.1 |
  | gap, delays 4-8 | 0.620 | 0.614 | | -0.5 |
  | echo k=4 | 0.997 | 0.996 | | -0.8 |
  | echo k=16 | 0.305 | 0.302 | | -0.5 |
  | shuffled echo | 1.000 | 1.000 | | |

  Learning-off twins 0.47-0.49 (k=2), 0.25, 0.12, 0.05. Exclusive-or at 0.985 is above the
  whole-sheet linear reader of the born sheet (0.97); echo at 5 tiles was the reach wall
  ("past about five tiles nothing arrives").
- **The inside really is a better sheet for the task it was walked on.** After a walked life,
  with the motor cells put back to birth and a fresh readout learning on the frozen walked
  sheet: exclusive-or 0.997 against 0.874 on the born sheet (t +9.2). A linear reader of
  the walked sheet reads exclusive-or from a single tile at 1.000 (born 0.878) and the echo
  cue 3 tiles away from the motor tiles at 0.998 (born 0.915). It does not carry over to a
  different task: walked on order, exclusive-or is 0.860 against 0.873; walked on echo at 3
  tiles, order there falls 0.620 to 0.570.
- **The sign carries information, but some of the gain is just churn.** The same spikes
  walked with random signs (the control) get part of the gain on exclusive-or (0.948 at
  depth 2, 0.946 at depth 1) and little on reach (5 tiles: 0.669 and 0.641 against 0.802 and
  0.876). The real walk beats the shuffled one on reach, exclusive-or and eight cues (t from
  3.0 to 11.4); on order and gap the two are the same (both hurt).
- **Depth: one step is best, and more is worse.** Depth 1 does as well as 2 everywhere and
  better at 5 tiles (0.876, 0.802, 0.724, 0.663 for depths 1-4). Deeper walks feed
  themselves: the rewired neurons fire together, the walk widens (depth-4 spikes per walk
  grew from 79 to 1,310 over one life), and echo at 3 tiles rises and then falls. Credit
  past one step is not needed for what was gained here and hurts.
- Health, cost and prediction 23: below. Prediction 23 is right.

## 1-3. The tasks, against the frozen-sheet readout (seeds 5-12)

Settings chosen on seeds 1-4 (tables at the end): depth 2, `replay_shift 4`, `replay_thin
1`, growing on. Each row is `scripts/sweep.py` over `scripts/variants/replay.txt`, 40,000
steps, the learning-off twin beside it. Late score (mean of the last 4 of 40 windows);
paired difference to the frozen-sheet readout with its standard error. Shuffled: the same
walk, each motor spike's sign drawn at random.

| task | frozen sheet + readout | walk | shuffled | walk - frozen | shuffled - frozen | walk - shuffled | twin |
|---|---|---|---|---|---|---|---|
| order | 0.695 | 0.664 | 0.668 | -0.030 ±0.009 | -0.027 ±0.011 | -0.003 ±0.007 | 0.480 |
| both-not-either | 0.883 | 0.985 | 0.948 | **+0.101 ±0.013** | +0.065 ±0.019 | +0.037 ±0.012 | 0.489 |
| gap | 0.519 | 0.491 | 0.499 | -0.028 ±0.009 | -0.020 ±0.006 | -0.008 ±0.009 | 0.488 |
| order, delays 4-8 | 0.839 | 0.840 | 0.828 | +0.001 ±0.009 | -0.011 ±0.010 | +0.012 ±0.010 | 0.480 |
| gap, delays 4-8 | 0.620 | 0.614 | 0.625 | -0.006 ±0.012 | +0.005 ±0.016 | -0.011 ±0.016 | 0.479 |
| echo k=2, 2 tiles | 0.905 | 0.945 | 0.929 | +0.039 ±0.017 | +0.024 ±0.004 | +0.015 ±0.016 | 0.483 |
| echo k=2, 3 tiles | 0.853 | 0.928 | 0.880 | **+0.074 ±0.022** | +0.027 ±0.010 | +0.047 ±0.016 | 0.476 |
| echo k=2, 5 tiles | 0.587 | 0.802 | 0.669 | **+0.215 ±0.030** | +0.082 ±0.019 | +0.133 ±0.039 | 0.473 |
| echo k=4 | 0.997 | 0.996 | 0.996 | -0.001 ±0.001 | -0.001 ±0.001 | 0.000 ±0.001 | 0.247 |
| echo k=8 (256 cells) | 0.842 | 0.924 | 0.871 | **+0.082 ±0.007** | +0.029 ±0.004 | +0.053 ±0.005 | 0.116 |
| echo k=16 (256 cells) | 0.305 | 0.302 | 0.326 | -0.003 ±0.006 | +0.021 ±0.006 | -0.024 ±0.003 | 0.047 |
| shuffled echo | 1.000 | 1.000 | 1.000 | 0 | 0 | 0 | 0.523 |

- **Reach is where it matters most.** Echo with the cue 5 tiles from the buttons goes from
  0.59 to 0.80 (0.88 at depth 1), every seed above every frozen seed. The readout alone
  cannot hear that far; one layer of inner neurons that learns which of its inputs to keep
  brings the cue to it.
- **Exclusive-or goes to 0.985** (0.97-1.00 on every seed), above the linear reader of the
  born sheet (0.97).
- **Eight cues: 0.84 to 0.92.** Sixteen cues (16 cells per button): no change, and there
  the shuffled walk is better than the real one, the one place the sign hurts.
- **Order and gap lose about 0.03, and the shuffled walk loses the same.** With long delays
  both are ties. Whatever hurts them is the rewiring itself, not the sign of the credit.
- **No locks**: shuffled echo stays at 1.00 on all eight seeds.

## 4. Depth (seeds 5-12)

`scripts/variants/replay-depth.txt`; depth 1 also with shuffled signs
(`replay-depth1.txt`).

| task | frozen | depth 1 | depth 2 | depth 3 | depth 4 | depth 1 shuffled |
|---|---|---|---|---|---|---|
| order | 0.695 | 0.673 | 0.664 | 0.671 | 0.675 | 0.676 |
| both-not-either | 0.883 | 0.985 | 0.985 | 0.978 | 0.986 | 0.946 |
| echo k=2, 3 tiles | 0.853 | **0.943** | 0.928 | 0.817 | 0.819 | 0.873 |
| echo k=2, 5 tiles | 0.587 | **0.876** | 0.802 | 0.724 | 0.663 | 0.641 |

- **More depth does not help anywhere, and on reach it hurts**, steadily: 0.88, 0.80,
  0.72, 0.66 at 5 tiles. At 3 tiles depths 3 and 4 rise early and fall late (late minus
  early -0.04). Exclusive-or does not care (0.98-0.99 at every depth).
- **Why deeper walks go wrong** (`blobprobe replay`, echo at 3 tiles, seed 1, depth 4):
  early in the life a walk names about 100 motor spikes and 80-110 at each depth below,
  with about 40 cancelling. By the end the same walk names 230 motor spikes and 700, 1,100,
  1,300, 1,300 at depths 1 to 4, 1,270 of them named with both signs, and 200 neurons are
  rewired per walk. The neurons the walk rewires are wired toward sources that were active
  at the same ticks, so they fire together more, so the next walk finds more drivers at
  each step: it feeds itself. Each step past the first is thinned by half; without that
  (depth 2, `replay_thin 0`, seeds 1-4) echo at 3 tiles also rises and falls. A quarter of
  the inner connections had moved by the end of that life, against 0.5% after 6,000 steps
  of a depth-2 walk on order.
- **Depth 1 is the better setting.** It was within noise of depth 2 on seeds 1-4 and was
  not chosen; on the report seeds it is as good or better on every task. It is also the
  version where the sign matters most: at 5 tiles, 0.876 against 0.641 shuffled.

So "credit walks backward" here works as one step: the neurons that fed the motor cells at
the moment of the mistake change what they listen to. The readout plus one taught layer is
a two-layer learner; the third layer down adds nothing and the fourth costs.

## 5. Is the inside really better?

**A fresh readout on the walked sheet.** `blobrun --retention B`: task A for 40,000 steps
with the walk on (or off, or shuffled); at tick 800,000 (the end of A) the walk stops for
good, the motor cells go back to their born wiring and the readout's counters are cleared
(`--g.replay_until 800000 --g.replay_reset 1`); then a fresh readout learns task B for
40,000 steps on the frozen sheet as it now is. Scored on B's windows (`sweep.py --windows
40:80`), `scripts/variants/replay-relearn.txt`, seeds 5-12:

| A, then B | born sheet | walked sheet | shuffle-walked sheet | walked - born |
|---|---|---|---|---|
| both-not-either, then both-not-either | 0.874 | **0.997** | 0.983 | +0.123 ±0.013 (t +9.2) |
| order, then order | 0.688 | 0.680 | 0.672 | -0.007 ±0.010 |
| order, then both-not-either | 0.873 | 0.860 | 0.857 | -0.013 ±0.009 |
| echo 3 tiles, then order (3 tiles) | 0.620 | 0.570 | 0.642 | -0.050 ±0.038 |

Unfinished: the same for echo then echo with new cues at 3 and 5 tiles (`out/replay/extra2.sh`
in the worktree, the two `sw --windows 40:80` lines with `--task echo --distance 3|5
--retention echo`); stopped before any run finished.

**The linear reader on the walked sheet.** `blobprobe linear` after the same 800,000 ticks
(walk on, off or shuffled), motor cells reset, all learning off (`--g.replay_reset 2`),
4,000 trials of the task's own timing, seeds 5-8 (`out/replay/probe.sh`; the reference in
docs/results-2026-11-24-readout.md was taken the same way on seeds 5-8). Share right on
held-out trials; in brackets the paired difference to the born sheet:

| task | sheet | whole sheet | motors' view | best 3x3 | best single tile | mean tile, top motor rows |
|---|---|---|---|---|---|---|
| order | born | 0.905 | 0.873 | 0.847 | 0.726 | 0.610 |
| order | walked | 0.893 (-0.012 ±0.004) | 0.873 (0.000) | 0.845 | 0.731 | 0.611 |
| order | shuffle-walked | 0.910 (+0.006) | 0.879 (+0.006) | 0.851 | 0.724 | 0.616 |
| both-not-either | born | 0.978 | 0.970 | 0.994 | 0.878 | 0.587 |
| both-not-either | walked | 1.000 (+0.023) | 1.000 (+0.030) | 1.000 | **1.000** (+0.122 ±0.014) | 0.718 |
| both-not-either | shuffle-walked | 0.999 (+0.022) | 0.999 (+0.030) | 1.000 | 0.996 (+0.118) | 0.658 |
| echo, 3 tiles | born | 1.000 | 0.915 | 1.000 | 1.000 | 0.635 |
| echo, 3 tiles | walked | 1.000 | **0.998** (+0.083 ±0.010) | 1.000 | 1.000 | 0.754 |
| echo, 3 tiles | shuffle-walked | 1.000 | 0.968 (+0.053) | 1.000 | 1.000 | 0.670 |

("Top motor rows": the mean single-tile score over tile rows 4 and 5, the half of the motor
region nearest the cues.)

- **Yes, for the task it was walked on.** Exclusive-or is now readable from one tile at
  1.000 (born: 0.878 at best): the walk made neurons that compute it. The echo cue 3 tiles
  away is now in the motor tiles' own neighbourhood (0.998 against 0.915), which is reach
  moved into the sheet, not into the readout.
- **Not in general.** It does not carry to another task (order after exclusive-or, order
  after echo), and on order itself the walked sheet is no better. The walk specializes the
  sheet; walked on echo, it got worse for order there on 6 of 8 seeds.
- **The shuffled walk changes the sheet too**, toward the same task (exclusive-or from one
  tile at 0.996), because it rewires exactly the neurons that were active on that task's
  trials. On reach the shuffled sheet gets about two thirds of the probe gain and much less
  of the task gain. Part of what the walk does is unsigned: "rewire what was busy when the
  answer was wrong".

## 6. Does it wreck the sheet?

**At the end of the 40,000-step report lives** (from the runs' summary files, seeds 5-12),
spikes per 1,024 per tick against a target of 32, and the share of stuck neurons:

| task | frozen | depth 1 | depth 2 | depth 3 | depth 4 | depth 2 shuffled |
|---|---|---|---|---|---|---|
| echo, 2 tiles | 32.7 / 0 | | **63.9 / 4.9%** | | | 32.8 / 0 |
| echo, 3 tiles | 32.7 / 0 | 33.9 / 0.2% | 43.8 / 1.8% | 65.6 / 5.6% | 80.4 / 8.0% | 32.8 / 0 |
| echo, 5 tiles | 32.7 / 0 | 33.3 / 0.1% | 36.9 / 0.7% | 45.8 / 2.2% | 57.1 / 4.3% | 32.7 / 0 |
| both-not-either | 33.0 / 0 | 34.8 / 0 | 35.9 / 0 | 37.3 / 0.2% | 37.8 / 0.2% | 33.9 / 0 |
| order, gap, k=4, 8, 16 | 31.4-32.7 / 0 | | 31.4-32.7 / 0 | | | same |

No silent neurons anywhere. Connections per neuron stay at 62.8 (the fixed count holds).

**A long life** (160,000 steps, seeds 5-8, `out/replay/health.sh`):

| | echo 3 tiles, frozen | walk depth 2 | shuffled | order, frozen | walk depth 2 | shuffled |
|---|---|---|---|---|---|---|
| late score | 0.863 | **0.805** | 0.889 | 0.718 | 0.682 | 0.678 |
| spikes per 1,024 (target 32) | 32.8 | **112** | 33.0 | 32.6 | 32.5 | 32.7 |
| stuck neurons | 0 | **12.9%** | 0 | 0 | 0 | 0 |
| threshold at its cap | 0.6% | **30.8%** | 1.2% | 0.6% | 0.7% | 1.1% |
| connections at strength 1 / 4 / 5 | 73 / 27 / 0% | 46 / 27 / **28%** | 73 / 27 / 1% | 72 / 28 / 0% | 72 / 28 / 0% | 69 / 30 / 1% |
| inner connections moved since birth | 0 | 37% | 8% | 0 | 7% | 13% |

- **On reach tasks the real walk runs away over a long life.** The credit keeps asking the
  paths that carry the cue for more activity; counters climb, strengths go to 5, the walked
  neurons fire together, the next walk is wider (on seed 5 the depth-2 walk grew from 85
  motor spikes and 100 / 80 at depths 1-2 per walk to 270 and 930 / 1,460), and thresholds
  hit their cap so regulation can no longer hold the rate. The score dips (0.76 at 100,000
  steps on seed 5) and partly comes back. The shuffled walk, with the same amount of
  rewiring in the same places, stays on target: the runaway comes from the sign being
  consistent, which is also what makes it useful.
- **On order nothing runs away**; the walk only costs its 0.03.
- **Where the wiring moves**: in the motor half of the sheet (tile rows 4-7) and the two rows
  above it, almost none in the cue's own rows (gained connections per inner neuron by row,
  echo walk: 0.5, 14.7, 19.1, 32.4, 35.2, 35.7, 32.1, 16.9).
- **Idle check** (echo at 3 tiles, teach 40,000 steps, unplug 200,000 ticks, retest 10,000;
  seeds 5-12): walk 0.928 before, 0.921 at the start of the retest; frozen 0.853, 0.890.
  Nothing moves while unplugged (no reward, no walk), so nothing is lost.
- **One fix tried, unfinished** (seeds 1-4 tuning, stopped after 7 of 20 long lives;
  `out/replay/budget/`): `replay_strong_budget 1` keeps each walked neuron's count of strong
  connections fixed and allows no strength 5. On seed 1 depth 1 without it also runs away
  over 160,000 steps (74.5 spikes per 1,024, 6% stuck, late 0.75); with it the rate holds
  (33.9) but the score falls further (0.57). Depth 2 with it: rate 59.9, late 0.81 (without:
  118, 0.76). So capping strength is not enough: rewiring toward sources that fire at the
  same ticks makes the sheet synchronous by itself. A hint from one or two seeds.

## 7. Cost

- **Memory**: 704 bytes per neuron of counters, plus 4 bytes (connection and strong
  counts), plus the born wiring kept for `replay_reset` (176) and the walk's maps (8):
  about 890 bytes per neuron, against 210 for the whole full engine
  (docs/scoreboard.md). Without the born copy and bit-sliced (6 planes of one word per
  slot) the counters would be 528 bytes. Made on first use, only when the walk is on.
- **Speed**: the tick is unchanged; the walk runs only on bad steps. Mean ticks per second of
  the report lives, the two variants interleaved in the same sweep on a loaded machine (other
  agents running, clock capped), so rough: frozen readout 52,600, walk depth 2 51,700,
  shuffled 50,000 (96 lives each); about 2% slower. Depth 1, 3 and 4 ran later under a
  different load and are not comparable (45,400, 37,000, 32,700); depths 3 and 4 are slower
  because their walks widen. A runaway long life ran at 15,700 against 29,400 frozen.
  Not taken on a quiet machine.

## Prediction 23

"Walking credit backward teaches the inside. With the readout from idea 58, letting its
error move counters on inner neurons along the recorded ticks beats the frozen sheet on order
or exclusive-or by more than 0.05, and does not make reach worse. (40%)"

**Right**, with a caveat. Exclusive-or +0.101 ±0.013 (0.883 to 0.985), seeds 5-12; reach is
better, not worse (+0.04, +0.07, +0.22 at 2, 3, 5 tiles). Order is worse (-0.03). The
caveat: about two thirds of the exclusive-or gain is also had by walking the same spikes
with random signs, and on reach tasks the walk runs away in a life four times as long.

## Reading

- **Teaching the inside from outcomes works here, one layer deep.** The neurons that were
  feeding the motor cells at the moment of a mistake change which of their inputs they keep,
  by evidence, in the direction the error asks for. That carries the cue to buttons 5 tiles
  away (0.59 to 0.88 at depth 1), builds neurons that compute exclusive-or (a single tile
  reads it at 1.00 afterwards), and lifts eight cues to 0.92. The sheet keeps it: a fresh
  readout on the walked sheet gets 0.997 on exclusive-or.
- **Deep credit is not what did it.** One step back is the best depth; each further step is
  worse on reach, because a walk that follows the paths it just strengthened gets wider every
  time. A backward pass through a recurrent spiking sheet compounds.
- **Where the credit goes wrong**:
  - on order and gap the walk and its shuffled control lose the same 0.03, so it is not the
    sign: rewiring the neurons that are busy in the answer window wears away the slow,
    spread-out traces those tasks read (and with long delays nothing is lost). The 15-tick
    ring also never reaches back to the cues, which come one and two steps (20-40 ticks)
    earlier.
  - on reach, the consistent sign is a positive loop: more activity is asked for, granted,
    and asked for again. Over 160,000 steps the rate goes to 3.5 times its target.
  - sixteen cues: the shuffled walk beats the real one (+0.024); with 16 cells per button the
    error names the pressed button's few cells against fifteen others, and the signal is
    mostly noise.
- **Part of the effect is not credit at all**: rewiring whatever was active on the trials
  helps exclusive-or almost as much as the real walk. That says the born sheet is short of
  conjunction neurons for the task at hand, and any rewiring that focuses on the task's
  active neurons makes more of them.

## What should change in docs/findings.md (for the coordinator; not edited here)

- The wall "Reading a thin signal: broken at the output, standing inside" no longer stands as
  written: teaching inner neurons from the readout's error, one step back along the spikes
  that carried the answer, beats the frozen sheet on exclusive-or (0.88 to 0.99), echo at
  3 tiles (0.85 to 0.94) and 5 tiles (0.59 to 0.88) and eight cues (0.84 to 0.92), seeds
  5-12, and the walked sheet itself is better for that task (fresh readout 0.997, linear
  reader from one tile 1.00). It still hurts order and gap a little (-0.03) and runs away on
  reach tasks in long lives. Grade: likely (one engine, one clean result).
- The wall "Reach": echo at 5 tiles is now learned (0.88 at depth 1); the line "past about
  five tiles nothing arrives to be read" is out of date.
- The wall "More than four things": eight cues 0.92 with the walk; sixteen cues 0.30 with or
  without it (chance 0.06).
- A new principle candidate, [hint]: **credit one step back, not many.** Depth 1 beat 2, 3
  and 4 on reach; deeper walks feed themselves.
- A new wall: **a consistent teaching signal inside a recurrent sheet is a positive loop.**
  Over a long life the walk on reach tasks takes the rate to 3.5x its target; regulation by
  threshold cannot hold it and capping strengths did not fix it (one seed). Principle 2
  (rate regulation keeps a sheet alive) has a limit here.
- Principle 14 (evidence before commitment) gets its second, independent use: the same
  counters, on inner neurons.

## What should change in docs/ideas.md and docs/predictions.md

- Idea 62: tried, works one step back. Exclusive-or 0.99, echo at 5 tiles 0.88, eight cues
  0.92 on a sheet taught by the readout's error; depth past 1 is worse; runs away on reach
  in long lives; part of the gain is unsigned rewiring (the shuffled control).
  docs/results-2026-11-25-reverse-replay.md
- Prediction 23: right (above).

## What I would try next

1. Make depth 1 the setting and stop the loop: the walk's counter steps zero-sum per neuron
   (every up paired with a down on another source of the same neuron), or the walked
   neurons' own threshold raised when they gain, so a taught path cannot add drive. The
   strong-count budget alone did not (one seed).
2. Run the long-life health and the five report tasks at depth 1 with whatever fixes the
   loop; that is the version to keep.
3. Order and gap: credit that reaches the cue steps. The ring is 16 ticks; the cues were 20
   to 40 ticks earlier. A slow trace (idea 52) on the walked neurons, or a longer ring for
   the walk only.
4. Sixteen cues: thin the "other buttons" side harder, or walk only from the pressed
   button's cells.
5. The walk with the design rule off the motor cells in mode 6, to see whether the inner
   design rule and the walk can live together.

## Unfinished, and how to resume

- Echo then echo with new cues, 3 and 5 tiles (experiment 5 at a distance): `bash
  out/replay/extra2.sh` in the worktree (or the two `sw --windows 40:80 ... --task echo
  --distance 3|5 --retention echo --retest 1000` lines in the same form as in `scripts/replay.sh
  tables`).
- The strength-budget tuning, long lives on echo at 3 tiles, seeds 1-4 (7 of 20 done):
  `bash out/replay/budget_tune.sh` (uses a build in `out/replay/build2`; `scripts/build.sh`
  then `build/blobrun` works the same now that the code is committed).
- A speed figure on a quiet machine, walk off and on: `nice -n 19 ./build/blobrun --task
  echo --distance 3 --ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000
  --seed 5 --g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1
  --motor-spread 128 --no-baselines [--g.replay_depth 1]`.

Raw outputs (in the worktree, git-ignored): `out/replay/sweep/` (every life, cache.jsonl
and each life's csv and summary), `out/replay/tune1.md`, `tune2.md`, `tables.md`,
`extra.md` (the tables as printed), `out/replay/probe/` (linear reader),
`out/replay/health/` (long lives, diag, idle), `out/replay/budget/` (the fix, partial),
`out/replay/diag-*.txt`; summarizers `out/replay/probe_sum.py`, `health_sum.py`,
`speed.py`, `vs-shuffle.sh`.

## Script lines

```
scripts/build.sh --target blobrun blobprobe
scripts/replay.sh life echo --distance 5                   the walk (depth 2), one life
scripts/replay.sh life echo --distance 5 --g.replay_depth 1
scripts/replay.sh diag echo --distance 3 --g.replay_depth 4 what the walk does per window
scripts/replay.sh probe both-not-either --seed 5           linear reader on the walked sheet
scripts/replay.sh probe both-not-either --seed 5 --g.replay_depth 0   ... on the born sheet
scripts/replay.sh tables                                    every report table, seeds 5-12
```

`tables` runs `scripts/sweep.py` over `scripts/variants/replay.txt`, `replay-many.txt` (8
and 16 cues, 256 cells), `replay-depth.txt`, `replay-relearn.txt` (with `--windows 40:80`),
each task with `--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000`, seeds
5-12, twins beside each. The depth-1 control is `replay-depth1.txt` over order,
both-not-either, echo at 3 and 5 tiles. Tuning was `replay-tune.txt` and `replay-tune2.txt`
on seeds 1-4.

## Rough

- Depth 2 was chosen on seeds 1-4 where depth 1 tied it; on the report seeds depth 1 is
  better. The headline numbers are depth 2 (the pre-chosen setting) with depth 1 beside.
- The linear-reader probe ran on seeds 5-8 only; relearn and the main tables on 5-12.
- The shuffled control gives each walked motor spike a random sign, so it also changes how
  much "more" and "less" credit there is, not only which.
- Speed numbers are from a loaded machine.
- The fix (`replay_strong_budget`) is committed but only partly measured; it is off by
  default.
- Retention's second phase moves echo cues (cue offset), so "echo then echo" is new cues, not
  the same task (and it is unfinished anyway).

## Tuning tables (seeds 1-4)

Late score, 40,000 steps; t against the frozen-sheet readout.

| variant | order | both-not-either | echo 3 tiles |
|---|---|---|---|
| frozen sheet + readout | 0.705 | 0.892 | 0.857 |
| depth 1 | 0.655 (-10.2) | 0.966 (+4.0) | 0.953 (+5.0) |
| depth 2 | 0.646 (-3.7) | 0.983 (+3.5) | 0.949 (+3.8) |
| depth 3 | 0.650 (-5.5) | 0.986 (+4.3) | 0.870 (+0.4) |
| depth 4 | 0.651 (-4.7) | 0.986 (+3.8) | 0.778 (-2.0) |
| depth 2, shift 2 | 0.635 (-4.0) | 0.958 (+1.7) | 0.819 (-1.3) |
| depth 2, shift 6 | 0.689 (-1.5) | 0.954 (+1.9) | 0.928 (+3.4) |
| depth 2, no growing | 0.685 (-1.2) | 0.955 (+3.6) | 0.906 (+3.1) |
| depth 2, quiet cells walk | 0.472 (-24.9) | 0.506 (-11.9) | 0.493 (-17.7) |
| depth 2, no thinning | 0.652 (-3.0) | 0.984 (+3.9) | 0.778 (-1.4) |
| depth 2, signs shuffled | 0.671 (-2.4) | 0.962 (+3.5) | 0.872 (+1.2) |
| depth 4, no growing | 0.692 (-1.4) | 0.956 (+3.9) | 0.905 (+3.0) |
| depth 4, shift 6 | 0.661 (-3.9) | 0.979 (+3.3) | 0.939 (+3.2) |
| depth 2, shift 6, no growing | 0.676 (-2.3) | 0.905 (+0.9) | 0.893 (+2.7) |

Learning-off twins 0.493, 0.496, 0.475. The full printed tables with per-seed scores are in
`out/replay/tune1.md`, `tune2.md`, `tables.md` and `extra.md` in the worktree.

## What was built

On top of the best frozen-sheet readout (learn mode 5, evidence counters at the motor
cells, balance, 128 motor cells per button), `--g.replay_depth N` turns on a walk. Files:
`src/blob/learn_replay.cpp` (the walk), `src/blob/replay_wire.cpp` (inner counters and how
they are read off into connections), `src/blob/replay.hpp` (the state, one malloc block).
One hook in `tick_learn` (before `learn_readout`, so the walk reads the connections that
carried the spikes), one `free` in `blob_free`, one `ReplayState*` at the end of `Blob`,
genome numbers at the end of `Genome`. All off by default; off is byte-identical (order with
the mode 5 best rule, echo at 2 tiles with mode 0, 3,000 steps, csv `cmp` against fb18e8c).

How one walk goes, on every bad step (the readout moves nothing on good, so neither does the
walk):

1. **Seeds.** Every spike a motor cell made in the readout's window (the 15 ticks before the
   reward) is a walked spike: (cell, age, sign). Pressed button's cells: "should have been
   less active"; the other buttons' cells: "more". The other buttons are thinned by their
   number, as the readout's balance does.
2. **Step back.** For a walked spike of neuron n at age a, each slot k reads
   `spikes[age a + delay(n,k)][source tile] AND connected(n,k)`: the spikes n's fire step
   actually read. Those (source, older age) are the next walked spikes, same sign through an
   excitatory source, the other sign through an inhibitory one. Sensor cells (driven by the
   world) and motor cells (the readout's) are not walked into. A spike named with both
   signs at the same age cancels. Past the first step each walked spike goes on with chance
   2^-`replay_thin` (1: half).
3. **Teach.** Each walked spike past the motor cells moves its neuron's own counters for the
   sources active one delay earlier still (`spikes[a + delay]`), one step with chance
   2^-`replay_shift` per active source: wanted more, excitatory sources go up and
   inhibitory down; wanted less, the reverse. With `replay_new 1` active sources that are not
   wired also move (so the walk can grow connections).
4. **Read off.** After the walk every neuron whose counters moved is rewired from them.

It is word ANDs on the spike ring, per walked spike per slot, and only on bad steps.

### Decisions

- **A neuron that did not fire.** Only spikes are walked. A cell that should have fired more
  walks back from the spikes it did make (its drivers should have been more active then too).
  The other option, walking from every tick of the window for those cells with their active
  connected sources as "would have helped" (`replay_quiet 1`), was tried on seeds 1-4 and
  wrecks every task (order 0.47, exclusive-or 0.51, echo at 3 tiles 0.49): it seeds about
  2,200 spikes per walk instead of 200, and after 40,000 steps 43% of the inner connections
  had been replaced.
- **Counters for every possible source.** Each neuron gets a signed 6-bit counter for all 704
  possible sources (11 slots x 64), one byte each: 704 bytes per neuron, made on first use.
  Bit-sliced (6 planes of one word per slot) it would be 528. Counters for the current
  connections plus a few candidates would be smaller but are a list of different length per
  neuron, which the machine rules forbid; the full row is flat and lets the walk grow
  connections.
- **The fixed count.** A neuron's row starts at its wiring when first touched, at 4x the
  strength (1 -> 4, 4 -> 16), so reading an untouched row off gives back the born wiring
  exactly. It keeps the number of connections it had then: the highest counters are wired;
  at the cut, a source wired now beats one that is not, so a single step on a new source
  does not evict anything. Nothing at zero or below is wired, so a neuron can end with fewer.
  Strength comes from the counter (1..9: 1, 10..19: 4, 20 and up: 5).
- **Growing.** Allowed by default (`replay_new 1`). Without it (only existing connections'
  counters move, so only strengths and removals) the gains are smaller on seeds 1-4 and the
  loss on order is smaller too (tables below).
- **Thinning.** Each step past the first keeps half of the walked spikes. Without thinning
  echo at 3 tiles falls during the life (late minus early -0.08).

Also built: `blobprobe replay` (`src/probe/replay_diag.cpp`) prints, per window, the score,
spikes walked at each depth per walk, conflicts, counter steps, rewires, and at the end how
many inner connections changed since birth and in which tile rows. `blobprobe linear` now
marks the pressed button before a reward, so the mode 5 rules work in it with `--learning 1`.
`replay_until`, `replay_reset` let a life stop the walk at a tick, put the motor cells back
to their born wiring and clear the readout's counters (and with 2, stop all learning), for
experiment 5. `replay_shuffle 1` is a control: the same motor spikes walked, each with a
random sign. `scripts/sweep.py` got `--windows A:B` (score one phase of a longer life) and the
replay numbers in its learning-only list.
