# One inner rule from tags and reverse replay (idea 155, 2026-11-25)

Two rules passed the first half of the inside-learning test: reverse replay (the walk,
docs/results-2026-11-25-reverse-replay.md) and action-feedback tags
(docs/results-2026-11-25-tags-and-margin.md). They are one idea in two forms: after an
answer, one step of signed credit goes from the motor cells back to the inner neurons that
fed them, into hidden counters, and connections are re-read from the counters. This job
merged them into one rule behind genome numbers, scored it against both parents, and ran
the inside-learning test, transfer and the MNIST ceiling.

## Short version

- **The merge is not better than its parents in a life.** The merged rule (the tags with
  the walk's spike-exact credit) ties the tags on exclusive-or (0.991) and eight cues
  (0.952), sits between them on order (0.682; tags 0.657, walk 0.673, readout 0.695;
  all within about 2 standard errors of each other), and loses to the walk on reach (3
  tiles 0.855 against 0.943, 5 tiles 0.757 against 0.876) and parity of 3 (0.592 against
  0.696). Echo at 2 tiles, the row to win, is still lost (0.823; readout 0.905; walk 0.885,
  tags 0.794). Spike-exact credit is not what gives the walk its reach.
- **It stays healthy.** At 40,000 steps it holds the target rate on every row (33.9 at 2
  tiles, where the walk is already at 53.1 with 3.1% of neurons stuck). The 160,000-step
  lives were not run (below).
- **The inside learns, and for reach and eight cues the credit is what does it.** A fresh
  output on the frozen lived sheet against a fresh output on the born sheet, seeds 5-12:
  exclusive-or 0.993 against 0.855, eight cues 0.963 against 0.813, parity of 3 0.693
  against 0.552, echo at 5 tiles 0.763 against 0.579. The same rule with its credit handed
  to as many random neurons that fired (the control both earlier jobs lacked) gets **none**
  of the gain on echo at 5 tiles (0.579) and eight cues (0.809), half on parity of 3
  (0.623), and most on exclusive-or (0.961): on exclusive-or "rewire whatever was busy" is
  most of it.
- **Transfer: yes, for the first time, between tasks that share features.** A fresh output
  on a sheet that lived exclusive-or learns parity of 3 at **0.856** against 0.552 on the
  born sheet (t +20; better than a sheet that lived parity of 3 itself, 0.693). A sheet that
  lived eight cues learns eight *other* cue-to-button pairs on the same cues at 0.950
  against 0.796 (t +10). Parity of A,B then parity of A,C, where C is a line the life never
  showed: 0.760 against 0.712 (t +4.5, small). A life alternating exclusive-or and eight
  cues every 1,000 steps leaves a sheet on which a fresh output reads exclusive-or at
  0.994, eight cues at 0.883 (born 0.813) and parity of 3, never in the mix, at 0.726
  (born 0.552).
- **MNIST: the lived sheet reads digits better than the born one, but not because of the
  credit, and not better than a random projection.** Linear reader of the frozen sheet
  (sensor cells left out), seeds 5-8: born 0.926, after 20,000 rewarded digits with the
  rule 0.938 (+0.012 ±0.001), after the same life with the readout alone 0.925. But the
  same life with good and bad from a coin gives 0.936, credit to random busy neurons 0.939,
  shuffled roles 0.933. The gain is spike-exact rewiring of busy neurons (the tags' window
  form changes 60% more connections and gains nothing: 0.927). All are under the random
  projection of the same bits (0.943) and under competitive learning on main (0.952).
- **The second hop was not tried**: the merged rule is not ahead of its parents, and the
  pieces table that should come first was not run.

## What was built

All behind genome numbers at the end of `Genome`, off by default; off is byte-identical
(the four merge-reference lives, csv and summary, ticks per second aside; and a tags life,
echo at 3 tiles, 4,000 steps, against the binary before). The tick is untouched.

The merge lives in the tags files (`src/blob/learn_tags.cpp`, `src/blob/tag_store.cpp`), not
in a third mechanism. Where each piece came from:

| piece | from | switch |
|---|---|---|
| contrast between buttons (chosen button's cells +, the others' -, weighted so all buttons count the same) | tags (the walk has it in its seeding) | `tag_mode 2` (1: chosen button only) |
| threshold on net credit per neuron | tags | `tag_min 4` |
| inhibitory neurons do not learn | tags | `tag_inh 0` |
| fixed born counts of excitatory, inhibitory and strong connections | tags | `tag_counts 0` (1: strength from the counter, drive grows; 2: one count, strength from the counter, nothing at zero: the walk's read-off) |
| verdict against a running mean of rewards, good and bad | tags | `tag_verdict 0` (1: only bad moves, the walk's error rule) |
| **spike-exact credit**: a button cell's feedback goes only along an edge whose source fired one slot delay before one of the cell's own spikes in the window; a tagged neuron moves counters only for sources that fired one delay before its own spikes | the walk | `tag_exact 1` (new) |
| growth: active unwired sources gain counters, so the fixed count can swap them in | the walk (`replay_new`); the tags already had it | `tag_grow 1` (0: only wired sources move) |
| control: each button cell draws its role at random (pressed with the pressed share) | the walk's `replay_shuffle` | `tag_shuffle 1` (new) |
| control: the tags handed to as many neurons drawn at random among those that fired in the step | new | `tag_random 1` (new) |

The rule chosen on seeds 1-4: `--g.tag_mode 2 --g.tag_inh 0 --g.tag_min 4 --g.tag_exact 1`
on the best readout (`--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4
--g.readout_balance 1`, 128 motor cells per button, 256 over all buttons for 8 and 16 cues).

Probes:
- `blobprobe transfer` (src/probe/lived_sheet.cpp): `--then-flags "<blobrun flags>"` (the
  second task in full), `--then-perm 1` (the buttons take one another's cells, a
  derangement from the seed: the same cues, other cue-to-button pairs), `--then-move L` (cue
  line L moves to the free sensor group nearest it: a line the life never showed), `--mix
  "<flags>" --mix-every N` (two tasks take turns on one blob; each turn a fresh task with
  task seed + turn; the readout learns throughout). The fresh readout now also stops the
  walk (`replay_depth 0`).
- `blobmnist ceiling --live N [--live-coin 1]` (src/mnist/mnist_ceiling.cpp): N training
  digits *with rewards* under the genome's own rules (here the readout with the stop-learning
  gate, plus the inner rule), then every motor cell's wiring put back as born, then frozen
  and read. `--live-coin 1`: good and bad from a coin. **How it differs from main's `--life`**
  (idea 158): `--life` is a reward-free life (no blob_reward at all), the motor cells are not
  reset, and `--life-shuffle` shuffles the pixels; `--live` is a rewarded life, which is the
  only kind in which a reward-driven inner rule moves anything, and it resets the motor
  cells so the reader cannot read the trained output cells. To merge them: one `--life N`
  with `--life-reward 0|1|coin` and the motor reset whenever the life was rewarded. Both are
  in the merged file now, side by side.

## 1. Choosing the setting (seeds 1-4)

40,000 steps, late score (last tenth of 40 windows). `scripts/inner.sh tune` and `tune2`.

| variant | xor | order | echo, 2 tiles | echo, 5 tiles | 8 cues |
|---|---|---|---|---|---|
| readout (frozen sheet) | 0.892 | 0.705 | 0.911 | 0.642 | 0.855 |
| tags | 0.996 | 0.711 | 0.833 | 0.803 | 0.952 |
| walk depth 1 | 0.966 | 0.655 | 0.858 | 0.823 | 0.926 |
| **exact** (chosen) | 0.990 | 0.693 | 0.767 | 0.765 | 0.960 |
| exact, min 2 | 0.996 | 0.677 | 0.661 | 0.775 | 0.969 |
| exact, min 8 | 0.949 | 0.690 | 0.837 | 0.676 | 0.964 |
| exact, step chance 2^-3 | 0.995 | 0.682 | 0.799 | 0.767 | 0.938 |
| exact, error verdict | 0.981 | 0.692 | 0.856 | 0.764 | 0.829 |
| exact, error verdict, step 2^-5 | 0.978 | 0.687 | 0.877 | 0.738 | 0.847 |
| exact, drive grows (`tag_counts 1`) | 0.681 | 0.687 | 0.905 | 0.756 | 0.200 |
| exact, error verdict, drive grows | 0.954 | 0.698 | **0.969** | 0.759 | 0.706 |
| tags, error verdict | 0.994 | 0.681 | 0.686 | 0.789 | 0.930 |

Learning-off twins 0.496, 0.493, 0.510, 0.489, 0.100.

- Among the merged candidates, "exact" and "exact, step 2^-3" tie on the mean of the five
  rows (0.835, 0.836); the tags parent is higher (0.859). "exact" was taken because it is
  the merge as specified, with nothing else changed.
- **Echo at 2 tiles is won only by giving up two pieces of the merge**: the walk's error
  verdict plus drive that can grow (strength from the counter) takes it to 0.969, above
  the readout (0.911) and both parents, but costs eight cues (0.706). With the running-mean
  verdict, drive that grows is a runaway on many buttons (8 cues 0.20, exclusive-or 0.68).
  The error verdict alone costs eight cues (0.83).
- Spike-exact credit with the running mean makes echo at 2 tiles *worse* than the tags'
  window credit, and falling during the life (late minus early -0.15).

## 2. Against both parents (seeds 5-12)

Every row 40,000 steps, 20 ticks a step, 3x3 cue groups; late score; paired t against the
readout. The parents' and twins' lives ran on stevejobs, the merged rule's here; the
parents reproduce the earlier docs to the third decimal (readout xor 0.883, tags xor 0.996,
walk depth 1 echo at 5 tiles 0.876). `scripts/inner.sh tables`.

| task | readout | tags | walk d1 | **inner** | inner vs readout | twin |
|---|---|---|---|---|---|---|
| exclusive-or | 0.883 | **0.996** | 0.985 | 0.991 | +0.108 ±0.014 (t 7.8) | 0.489 |
| order | **0.695** | 0.657 | 0.673 | 0.682 | -0.013 ±0.011 | 0.480 |
| gap | 0.519 | 0.505 | 0.510 | 0.509 | -0.010 ±0.006 | 0.488 |
| echo, 2 tiles | **0.905** | 0.794 | 0.885 | 0.823 | -0.083 ±0.018 (t -4.6) | 0.483 |
| echo, 3 tiles | 0.853 | 0.869 | **0.943** | 0.855 | +0.002 ±0.016 | 0.476 |
| echo, 5 tiles | 0.587 | 0.781 | **0.876** | 0.757 | +0.170 ±0.018 (t 9.2) | 0.473 |
| echo, 8 cues | 0.842 | **0.953** | 0.919 | 0.952 | +0.110 ±0.011 (t 9.8) | 0.116 |
| echo, 16 cues | 0.305 | **0.416** | 0.316 | 0.383 | +0.078 ±0.008 (t 10.0) | 0.047 |
| parity of 3 | 0.541 | 0.635 | **0.696** | 0.592 | +0.051 ±0.014 (t 3.8) | 0.490 |
| shuffled echo (16 lives) | 1.000 | 1.000 | 1.000 | 1.000 | 0 | 0.518 |
| delayed echo | 0.547 | 0.530 | 0.550 | 0.558 | +0.011 ±0.009 | 0.488 |

- The merge takes the tags' strengths (exclusive-or, eight cues, sixteen cues nearly) and
  none of the walk's (reach, parity of 3). Order: no parent and no merge beats the frozen
  sheet; the merge loses least (-0.013, noise). Echo at 2 tiles: still lost.
- The walk at depth 1 is the best rule on four rows here and the only one to keep echo at 2
  tiles (0.885, noise against the readout), but see health.
- No locks: shuffled echo 1.00 on all 16 lives with every rule.

Full tables with per-seed values: `out/inner/report.md`.

## 3. Health (40,000 steps, seeds 5-12)

Spikes per 1,024 per tick (target 32), stuck neurons, thresholds at their cap, share of
connections at strength 5:

| | readout | tags | walk d1 | inner |
|---|---|---|---|---|
| echo, 2 tiles | 32.7 / 0 / 1.0% / 0 | 34.1 / 0 / 2.9% / 0 | **53.1 / 3.1% / 10.0% / 3.9%** | 33.9 / 0 / 2.5% / 0 |
| echo, 3 tiles | 32.7 / 0 / 0.7% / 0 | 32.9 / 0 / 1.0% / 0 | 33.9 / 0.2% / 2.2% / 0.4% | 33.0 / 0 / 1.2% / 0 |
| echo, 5 tiles | 32.7 / 0 / 0.5% / 0 | 32.7 / 0 / 0.6% / 0 | 33.3 / 0.1% / 1.3% / 0.4% | 32.8 / 0 / 0.5% / 0 |
| exclusive-or | 33.0 / 0 / 1.1% / 0 | 33.3 / 0 / 1.7% / 0 | 34.8 / 0 / 3.3% / 0.2% | 34.5 / 0 / 3.0% / 0 |

The merged rule keeps the tags' calm: no stuck neurons, nothing at strength 5. The walk at
depth 1 is already running hot at 2 tiles within 40,000 steps. Connections per neuron at 2
tiles: 62.7 readout, 60.8 tags, 61.0 inner (born 62.7): about 2 per neuron lost with the
tags' store on that row, not looked into (the counts are held per tagged neuron, so the loss
is likely in the readout's motor cells).

**The 160,000-step lives were not run** (the long-life row of the brief is open).

## 4. Did the inside learn? (seeds 5-12)

`out/inner/transfer.sh out/inner/jobs-same.txt`: a 40,000-step life, then the inside frozen,
every motor cell back to its born wiring, the readout's counters wiped, and a fresh readout
(the plain frozen-sheet rule) learns the task for 20,000 steps. "random": the same life with
`tag_random 1` (the credit handed to as many random neurons that fired).

| task | born: first quarter / late | lived (inner): first quarter / late | lived - born | random control: late | random - born |
|---|---|---|---|---|---|
| exclusive-or | 0.685 / 0.855 | **0.911 / 0.993** | +0.138 ±0.009 (t 14.7) | 0.961 | +0.105 ±0.009 |
| eight cues | 0.594 / 0.813 | **0.836 / 0.963** | +0.150 ±0.012 (t 12.5) | 0.809 | -0.004 ±0.010 |
| parity of 3 | 0.531 / 0.552 | **0.580 / 0.693** | +0.141 ±0.028 (t 5.1) | 0.623 | +0.071 ±0.022 |
| echo, 5 tiles | 0.552 / 0.579 | **0.710 / 0.763** | +0.184 ±0.027 (t 6.8) | 0.579 | 0.000 ±0.005 |

- **Yes on all four.** The fresh output on the lived sheet is better on every row, from its
  first quarter.
- **The random-neuron control separates "rewiring" from "credit".** On echo at 5 tiles and
  eight cues, rewiring the same number of busy neurons gives nothing (0.579, 0.809): the
  gain is the credit. On exclusive-or most of the gain (0.105 of 0.138) is had by rewiring
  any busy neurons, as the walk's shuffled control hinted; the credit adds 0.03. Parity of 3
  is half and half.
- On parity of 3 the fresh output reads the lived sheet (0.693) better than the life itself
  did (0.592): the life's own readout had not caught up with its sheet.

## 5. Transfer

`out/inner/jobs-transfer.txt`, seeds 5-12; the same protocol, a different second task.

| life, then fresh output on | born | lived (inner) | lived - born |
|---|---|---|---|
| exclusive-or, then parity of 3 | 0.552 | **0.856** | +0.304 ±0.015 (t 20.3) |
| eight cues, then eight other cue-to-button pairs on the same cues (`--then-perm 1`) | 0.796 | **0.950** | +0.154 ±0.015 (t 10.2) |
| parity of A,B, then parity of A,C (`--then-move 1`) | 0.712 | 0.760 | +0.048 ±0.011 (t 4.5) |

- **This is the first transfer shown here.** A sheet that learned exclusive-or of lines A
  and B is a far better starting point for parity of A, B and C than the born sheet, and
  than a sheet that lived parity of 3 itself (0.693). Exclusive-or of A and B is the half of
  3-parity the born sheet cannot supply; the sheet now has it.
- Caveat on that row: the plug lays the 3-line task's line C where the 2-line task had its
  ask line, so C sits on a group the exclusive-or life saw lit on every answer step.
- Eight cues to new pairs: the cue features the life built serve any assignment of cues to
  buttons as well as the trained one (0.950 against 0.963 for the same pairs).
- Parity of A,C: **C is a sensor group the life never showed** (the world has no "parity of
  lines 0 and 1 of 3", and src/world was left alone), so only A's half can be shared; the
  gain is small but every seed but one is above born.
- No random-neuron control was run on the transfer rows.

### Mixed lives

`out/inner/jobs-mix.txt`: one life of 80,000 steps in which exclusive-or and eight cues take
turns every 1,000 steps on the same blob (a fresh task at each turn, the readout learning
throughout, 256 motor cells over the buttons for both); then the fresh output on each, and
on parity of 3, which was not in the mix. Seeds 5-12.

In the life, the last turn of each task: exclusive-or 0.971, eight cues 0.772 (eight cues
alone reached 0.952 in 40,000 steps).

| fresh output on | born | lived on the mix | lived - born | lived on that task alone (sections 4, 5) |
|---|---|---|---|---|
| exclusive-or (in the mix) | 0.855 | **0.994** | +0.138 ±0.008 (t 18.3) | 0.993 |
| eight cues (in the mix) | 0.813 | 0.883 | +0.070 ±0.010 (t 7.3) | 0.963 |
| parity of 3 (not in the mix) | 0.552 | 0.726 | +0.173 ±0.012 (t 15.0) | 0.856 after exclusive-or alone |

- One sheet carries both tasks' features: a fresh output reads exclusive-or as well as
  after a pure exclusive-or life, and eight cues better than born, though with half of what
  an eight-cue life gives (the eight-cue turns had half the steps and shared the sheet).
- The held-out task gains too (parity of 3, +0.17), less than after exclusive-or alone:
  what transfers is the exclusive-or half, diluted by the mix.

## 6. MNIST ceiling (seeds 5-8)

`out/inner/mnist_all.sh` (`blobmnist ceiling` with the MNIST job's sheet, the gated readout
`--g.readout_evidence 2 --g.readout_good 1 --g.margin_percent 20 --g.margin_win 2`, a
linear reader fitted on all 60,000 training digits, scored on the 10,000 test digits).
Before reading, a life of 20,000 rewarded digits with the rule shown, then the motor cells
back to their born wiring.

| life before the reader | reader of the sheet, sensor cells left out | vs born (paired) | every neuron | stream share right in the life | inner connection bits changed |
|---|---|---|---|---|---|
| none (born) | 0.9258 | | 0.9314 | | 0 |
| readout alone | 0.9251 | -0.0007 ±0.0013 | 0.9301 | 0.753 | 0 |
| **inner rule** | **0.9382** | **+0.0124 ±0.0013** | 0.9381 | 0.755 | 56,000 |
| inner rule, good and bad from a coin | 0.9357 | +0.0099 ±0.0008 | 0.9368 | 0.093 | 64,000 |
| inner rule, credit to random busy neurons | 0.9390 | +0.0132 ±0.0013 | 0.9375 | 0.754 | 74,000 |
| inner rule, roles shuffled | 0.9330 | +0.0072 ±0.0013 | 0.9351 | 0.756 | 49,000 |
| tags (window credit, not spike-exact) | 0.9265 | +0.0008 ±0.0008 | 0.9318 | 0.756 | 89,000 |

Against: random projection of the same bits 0.943; competitive learning on main 0.952.

- **The lived sheet reads digits better than the born one** (+0.012, every seed), the first
  reward-driven inner rule here to move the MNIST ceiling.
- **But the credit is not what moves it.** A life with uninformative rewards gets 80% of the
  gain, and credit handed to random busy neurons gets all of it. What lifts the reader is
  the spike-exact rewiring itself: each moved neuron re-chooses its inputs among the
  sources that fired just before it fired, a Hebbian step at born counts. The tags'
  window form moves more connections and lifts nothing.
- **It does not reach a random projection (0.943)** and is under competitive learning
  (0.952), which does the same kind of step on every cell without any reward.
- The online readout in the life is unchanged by the rule (0.755 against 0.753 on the stream).

## 7. Cost

- Memory: the tags' store, about 715 bytes per neuron (a 6-bit counter per possible source,
  704 bytes, plus counts and flags), made on the first verdict. Spike-exact credit adds no
  state.
- Speed: the tick is untouched. Mean ticks per second of the tuning lives, all on this PC,
  2 jobs at a time, loaded (rough): exclusive-or readout 56,400, tags 56,300, walk d1 57,500,
  exact 59,000; echo at 5 tiles 49,900, 47,200, 52,300, 50,900; eight cues 47,600, 44,500,
  50,100, 50,600. No difference beyond the noise of a shared machine. Not taken on a quiet
  machine.

## Not done

- **The piece-by-piece table on seeds 5-12** (`scripts/inner.sh pieces`,
  `scripts/variants/inner-pieces.txt`: no contrast, not spike-exact, no growth, strength
  free, no fixed counts, no threshold, inhibitory learning, error verdict with free
  strength, shuffled signs, random neurons; rows xor, order, echo at 2 and 5 tiles, 8 cues,
  parity of 3). Written, not run. What is known about the pieces comes from seeds 1-4
  (section 1: spike-exactness, threshold, verdict, drive) and from the two controls in
  sections 4 and 6.
- **The shuffled-sign and random-neuron controls in a life** (only in the inside-learning
  test and MNIST).
- **160,000-step lives** on echo at 3 and 5 tiles: `SEEDS=5-8 scripts/inner.sh long`, then
  `python3 out/inner/health.py out/inner/sweep scripts/variants/inner-report.txt 5-8 --
  --task echo --k 2 --distance 5 --ticks 20 --sensor-side 3 --motor-side 3 --steps 160000
  --window 4000`.
- **The second hop** (`--g.tag_hops 2 --g.tag_thin 1`, which with `tag_exact` already
  follows exact drivers). Not tried: the merged rule is not ahead of its parents, and the
  pieces table should come first.
- A random-neuron control on the transfer rows; a version of parity of A,C where C was
  shown in the life (needs a world function "parity of some lines of N", in src/world).

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, "Where it stands" / "Two more": add that **a sheet taught by one-step
  credit transfers to a task that shares its features**: exclusive-or, then parity of 3,
  fresh output 0.856 against 0.552 born (8 seeds, t +20); eight cues, then other pairs on the
  same cues 0.950 against 0.796. The line "Neither transfers to a task it was not rewarded
  on" should read "neither transfers to an unrelated task (order after exclusive-or); both
  transfer to a related one". [likely: one rule, one protocol, 8 seeds]
- Principle 29 (credit one step back reaches the inside): the random-neuron control says
  the credit carries the gain on reach and eight cues (control: none of it) and not on
  exclusive-or (control: three quarters of it). Grade stays likely.
- New hint: **spike-exact rewiring of busy neurons lifts the MNIST ceiling whatever the
  reward** (0.926 to 0.938 with real credit, 0.936 with a coin, 0.939 with random neurons);
  a window-based version does not. It agrees with competitive learning (idea 158) that the
  gain on digits is unsupervised.
- New hint: the walk's reach does not come from spike-exactness; with the tags' fixed
  counts, spike-exact credit behaves like the tags. Echo at 2 tiles is won only by the
  walk's error verdict plus drive that can grow (0.969, seeds 1-4), which costs eight cues.
- `docs/plan.md`, item B: done; the merge ties the tags, does not combine the parents'
  strengths; transfer shown; MNIST ceiling moved but below 0.943 and not by credit. Next:
  the pieces table, the long lives, and the walk's reach with fixed counts (error verdict +
  drive growing only on the side asked to fire less, or a per-button balance of "more").

## Script lines

```
scripts/inner.sh life both-not-either                     one life with the merged rule
scripts/inner.sh life echo --distance 5
scripts/inner.sh transfer both-not-either "--task points --fn parity --fn-lines 3"
scripts/inner.sh mnist                                    born, inner, readout-only, coin ceilings
scripts/inner.sh tune / tune2                             seeds 1-4
scripts/inner.sh tables                                   section 2, seeds 5-12
scripts/inner.sh pieces                                   not run
SEEDS=5-8 scripts/inner.sh long                           not run
```

Helpers and raw outputs (git-ignored, in the worktree's `out/inner/`): `transfer.sh` and the
`jobs-*.txt` lists, `transfer_sum.py`, `mnist.sh`, `mnist_all.sh`, `mnist_sum.py`,
`health.py`, `report_inner.sh`, `report_tables.sh`, `ident.sh`; results in `same/`,
`transfer/`, `mix/`, `mnist/`, `sweep/` (with stevejobs' cache merged in), `tune.md`,
`tune2.md`, `report.md`; the binaries before the change in `bin0/`, before the merge with
main in `bin1/`.

## Rough

- Settings were chosen on seeds 1-4 over five rows; the tags parent scored higher there
  than any merged candidate, and the merge was kept because it was the job.
- The parents' report lives ran on stevejobs (6 jobs at once, over its cap, started before
  the cap changed; the last rows ran at 2), the merged rule's here; results are
  byte-identical across the two machines, so the columns compare.
- Transfer and inside-learning rows are one probe protocol (40,000-step life, 20,000-step
  fresh output, late = last tenth); MNIST is 4 seeds.
- The mixed lives first ran with a wrong flag list and failed; rerun.
