# The error sent one hop, to every hidden cell (2026-11-25)

Queue item 2b of `docs/plan.md`, idea 159. The owner: "do we need some sort of hyper
truncated bit backprop... maybe backprop isnt the issue but the whole requirement that it
goes all the way back through is fucked up right? also that its so heavy on compute."

Known in floats as direct feedback alignment (Nokland 2016) with an eligibility mark per
connection (e-prop, Bellec et al. 2020): every hidden cell gets the output's error through a
fixed random wire, no chain backwards. Here it is put in bits, online, in the recurrent
sheet, and scored by the inside-learning test (`scripts/inside.sh`).

## Prediction (written before any scoring run)

Seen before writing this: one smoke run only (validation, 2,000-digit taught life, told the
right button, 3,000 digits fitted; the numbers say nothing, it checked that the code runs):
the readout reached 0.6 right in its life, hidden cells' pixel likeness went 0.05 to 0.17,
the rate stayed at 39 spikes per 1,000 cell-ticks.

MNIST, test, 60,000-digit taught life, motor cells put back as born, fresh reader fit on
60,000 (born about 0.925; competitive learning 0.952; two-layer net 0.955):

| | told the right button (2) | told good or bad (1) |
|---|---|---|
| lived beats born by 0.005 or more | 0.55 | 0.35 |
| best guess, lived - born | +0.010 (range -0.005 to +0.025) | +0.004 (range -0.010 to +0.015) |
| lived beats its coin control by 0.005 or more | 0.45 | 0.30 |
| lived reaches competitive learning's 0.952 | 0.10 | 0.05 |
| lived reaches the two-layer net's 0.955 | 0.05 | 0.03 |

- With competitive learning in the same life: beats competitive learning alone by 0.003 or
  more: 0.30.
- Toys (fresh output on the lived sheet against the born sheet): both-not-either lifted by
  0.05 or more, mode 2: 0.6. Parity of 3 lifted by 0.05 or more: 0.35; parity of 4: 0.2.
- Calc held-out bits (xor4, add4) rise 0.03 or more over born: 0.5; over the coin control by
  0.03: 0.3.
- The online readout (`scripts/mnist.sh online`) with the rule on from the first digit is
  0.02 or more above off at some checkpoint (mean of seeds): 0.25.
- Cost: the rule adds under 20% to a taught life's ticks per second: 0.7 (the readout's own
  rewiring is most of a taught life's cost).

Reasons for the low numbers: one bit of error, only on bad steps (about a third of them once
the readout is at 0.7), sparse random wires, a counter step with chance 1/8, a fixed budget;
float DFA needs a full error vector on every example and many epochs to reach 0.98.

## What was built

**The rule** (`src/blob/learn_hop.cpp`, 300 lines; genome numbers, all off by default):

- **Error lines, in bits.** At each verdict the world names the motor lines the blob pressed
  and the lines that were right, as two bit masks (`plug_hop_lines`, `src/world/plug_hop.cpp`;
  one call beside `plug_mark_acted` in `src/run/life.cpp`, `src/probe/lived_sheet.cpp`,
  `src/mnist/mnist_sheet.cpp`, and in the calc probe's taught life). For a pattern answer
  each graded output bit is two lines: the group that won is pressed, the group that should
  have won is right. The rule turns that into an error of -1, 0 or +1 per line:
  - `hop_teach 1`, told good or bad: on bad the pressed line, if it was not right, is -1.
    Nothing else is known.
  - `hop_teach 2`, told the right line: also, on bad, the right line is +1.
  - `hop_good 1`: on good the pressed line is +1 (otherwise good teaches nothing).
- **One hop.** Every hidden excitatory cell (not sensor, motor, pacemaker or inhibitory) has
  a fixed wire from each error line with chance 2^-`hop_wire_shift` (default 1: half the
  lines), with a sign; both are bits of a hash of the cell number, the line and a salt drawn
  once per blob. No memory. The cell's teaching bit is the sign of the sum of wire times
  error over the lines: "fire more for what was just shown", "fire less", or (a sum of 0)
  nothing.
- **Near threshold, cheaply.** After every fire step, one word per tile: the learners whose
  potential is at most `hop_near` eighths of their bar under the bar (bar = threshold plus
  sector bias; default 2, a quarter), whether or not they fired, plus (`hop_fired 1`) those
  that fired. It is stored in a ring of 16 maps beside the spike ring (16 x 65 words for the
  whole sheet). This is the bit form of the float rule's pseudo-derivative: it is 1 in a
  band under and at the threshold, 0 far below. Cells over the bar that did not fire (lost
  the tile's competition, or in their cooldown) count as near.
- **Marks.** Nothing is stored per connection. At a verdict, a connection's mark is
  computed from the two rings: its source was on (at that slot's own delay) on some tick
  within `hop_window` ticks before the verdict (default 12) on which its cell was near.
  That is the same as a per-connection trace "my source was on while my cell was near
  firing" that lasts the window, at the cost of one word per tile per tick.
- **Move.** Each marked source's hidden counter (the tag store the tags and competitive
  learning use, 6 bits signed) steps by the teaching bit with chance 2^-`hop_shift` (default
  3): "fire more" raises excitatory sources and lowers inhibitory ones, "fire less" the
  reverse. Unwired sources gain and lose too, which is how a cell rewires.
- **Rewire only when it can matter.** Each learner keeps 8 bytes of bounds on its counters:
  the lowest wired and highest unwired counter, the lowest strong and highest weak, each for
  excitatory and inhibitory sources. A step can change what the cell wires only if an
  unwired counter climbs past the lowest wired one, a wired one drops under the highest
  unwired one, or the same between strong and weak. Only then does the cell re-pick its top
  counters at its born counts (`tag_rewire`, finding 38). Bounds are kept loose in the safe
  direction between re-picks, and made again (one pass over the row) after any re-pick,
  including those by competitive learning or tags (`hop_stale`, one line in
  `tag_store.cpp`).
- **The coin control** (`hop_coin 1`): every error goes to a line drawn at random instead
  of its own. Same verdicts, same number of errors, same signs, so the rule moves just as
  much; nothing about the input is in which line was wrong. This is "labels replaced by
  random ones" at the rule; the readout in that life still learns the real labels, and is
  reset before the read anyway.

**The test** (small edits):

- `blobmnist ceiling --life N --life-teach 1|2` (`src/mnist/mnist_ceiling.cpp`, 12 lines):
  the life is judged as `blobmnist online` judges it, so the readout learns (its rule's
  flags are given), the genome's `hop_teach` is set to the flag, and after the life the
  motor cells' wiring goes back to as born and the readout's counters are dropped
  (`blob_motor_reborn`, `src/blob/motor_reborn.cpp`, born again from the same genome and
  seed, as `blobprobe transfer` does). Hidden cells may still listen to motor cells, but
  those now fire as born, so nothing the output learned reaches the reader. Each tenth of
  the life prints the readout's share right and the rule's counts.
- `blobprobe calc --life N --life-teach 1|2`: the same for the calculation battery (a
  verdict per bit).
- `scripts/inside.sh NAME --teach 1|2 ...` adds the readout rule of `scripts/mnist.sh` and
  the coin control to the MNIST part.
- `scripts/hop.sh toy|calc NAME ...`: the toys through `blobprobe transfer` (a fresh output
  on the lived sheet; tags doc) and the calc battery, born against lived against coin.

## Step 1: toys (a fresh output on the lived sheet)

`scripts/hop.sh toy toys` (and the variant lines below). A 40,000-step life on the task with
the tags job's readout (`--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4
--g.readout_balance 1`, 128 motor cells a button) and the rule; then `blobprobe transfer`
freezes the inside, puts every motor cell back as born, wipes the readout's counters, and a
fresh readout learns the same task for 20,000 steps. The number is that fresh readout's late
score (last tenth); in brackets its first quarter. "off" is the same life with the readout
learning and the rule off: the check that the output's own learning does not reach the
reader. "coin": the rule on, every error sent to a random line. Seeds 5-12 (parity of 4 and
"m1, and good teaches": 5-8); one setting, not tuned (the defaults above). Rule numbers per life on xor: about 500 verdicts reach the rule
(the live readout is right almost at once), about 940 cells taught per verdict, 10 counter
steps per taught cell, 400 re-picks per verdict.

| task (fresh output, late) | born | off | **told the right button (m2)** | told good or bad (m1) | m1, and good teaches | coin for m2 | coin for m1 |
|---|---|---|---|---|---|---|---|
| both-not-either (xor) | 0.855 (0.685) | 0.866 (0.726) | **0.998 (0.958)** | 0.983 (0.843) | 0.915 | 0.930 (0.772) | 0.871 (0.687) |
| parity of 3 | 0.552 (0.531) | 0.562 (0.542) | **0.816 (0.709)** | 0.667 (0.599) | 0.608 | 0.587 (0.548) | 0.613 (0.586) |
| parity of 4 | 0.473 | 0.488 | 0.494 | 0.473 | | 0.475 | |

Per seed (5 to 12): xor m2 0.999, 0.997, 0.998, 0.998, 0.997, 0.999, 0.996, 0.997 against
born 0.839, 0.895, 0.856, 0.836, 0.885, 0.814, 0.852, 0.866 and coin 0.933, 0.934, 0.933,
0.931, 0.933, 0.899, 0.944, 0.931; parity 3 m2 0.826, 0.855, 0.816, 0.800, 0.802, 0.795,
0.838, 0.793 against born 0.528, 0.561, 0.561, 0.518, 0.591, 0.553, 0.543, 0.562 and coin
0.582, 0.588, 0.575, 0.609, 0.602, 0.581, 0.589, 0.574. Every seed is above its coin
control on xor and parity 3 in both teaching modes (m1 against its own coin: xor 0.988,
0.985, 0.962, 0.995, 0.986, 0.971, 0.983, 0.994 against 0.897, 0.842, 0.863, 0.915, 0.856,
0.854, 0.936, 0.808; parity 3 0.671, 0.690, 0.684, 0.678, 0.678, 0.706, 0.596, 0.632
against 0.653, 0.671, 0.580, 0.565, 0.642, 0.631, 0.559, 0.602).

- **Yes on the toys: the inside comes to carry the product.** A readout that has never seen
  the task reads exclusive-or from the lived sheet at 0.96 in its first quarter and 0.998 at
  the end (born 0.70 and 0.86): the same as tags (0.961 / 0.998) on the same test, reached
  by a different route (tags follow the button's own wiring back one hop; this reaches
  every hidden cell through random wires). Parity of 3, which only the richest sheet
  carried (finding 33), goes 0.55 to 0.82.
- **Told the right button teaches more than good or bad** (parity 3: 0.82 against 0.67), as
  expected: on a two-button task "the pressed one was wrong" already names the right one,
  so the gap is in how many cells move (a +1 line and a -1 line reach more cells than one),
  not in what they are told.
- **Good should teach nothing.** Letting good say "the pressed button should have been
  higher" costs both tasks (seeds 5-8: xor 0.98 to 0.92, parity 3 0.68 to 0.61). Finding 39 again.
- **The coin control moves the toys too** (xor 0.86 to 0.93, parity 3 0.54 to 0.59): churn
  of the wiring with no information is worth something, as the shuffled control was for
  competitive learning and the unsigned half was for reverse replay. The errors are worth
  far more (m2: +0.068 and +0.229 over its coin; m1: +0.112 and +0.054).
- **Parity of 4: no**, also with a four times longer life (`scripts/hop.sh toy toys-long
  --settings off,m2,m2coin --tasks parity4 --seeds 5,6 --steps 160000 --window 4000`: off
  0.475, m2 0.478, coin 0.498). The live readout stays at chance through the life too.

- **The live readout learns the toys too, in the same life.** The life's own readout with
  the rule on ends at 0.997 on xor (off 0.883) and 0.797 on parity 3 (off 0.541), seeds 5-12
  ("life's own readout" in `out/hop/toys/table.txt`). On these worlds the rule is not only
  a better sheet for a later reader: the readout that is learning beside it uses it.
- **The rate does not move on the toys.** One life each, seed 5 (`blobrun --task
  both-not-either ... --g.hop_on 1 --g.hop_teach 2 --out ...`, the toy life above):
  xor 33.8 spikes per 1,024 neuron-ticks against 32.9 off; parity 3 34.0 against 33.3.
  Thresholds at the cap 2.3% against 1.0%.

**At the MNIST setting** (a counter step with chance 1/128 instead of 1/8, `scripts/hop.sh
toy toys-s7 --settings m2,m2coin,m1 --tasks xor,parity3 --g.hop_shift 7`, seeds 5-8): the
same 40,000-step life moves the inside less. xor m2 0.977, m1 0.943, coin 0.911; parity 3
m2 0.634, m1 0.664, coin 0.587. Still above the coin on every mean, but 16 times fewer
steps buy a third of the gain on parity 3.

**Which cells should count as near** (m2; `scripts/hop.sh toy toys-near0 --settings m2
--tasks xor,parity3 --g.hop_near 0`, and `toys-nofired ... --g.hop_fired 0`):

| marks from | xor | parity 3 |
|---|---|---|
| cells at or over the bar only (fired, or over it but held back): Hebbian with a sign (seeds 5-8) | 0.997 | 0.849 |
| fired, or within a quarter of the bar under it (the default; seeds 5-12) | 0.998 | 0.816 |
| **within a quarter of the bar under it, not fired** (seeds 5-12) | 0.997 | **0.897** |

On parity 3 the cells that came near and did not fire are the better teachers (0.897, every
seed 0.876 to 0.928, against 0.793 to 0.855 for the default): "fire more" has the most to say
to a cell that almost fired, as the float rule's pseudo-derivative says. On xor everything
reaches 0.997.

## Step 2: the calculation battery (held-out cases)

`scripts/hop.sh calc calc --seeds 5,6 --life 400000`: a taught life of 400,000 ticks (20,000
trials, a verdict per output bit, the readout learning), motor cells put back as born, then
`blobprobe calc`'s ridge reader per output bit on the frozen sheet. Bits right, seen /
held-out (held-out cases are never rewarded in the life). Seeds 5 and 6, untuned defaults;
the last row is marks from near-but-not-fired cells only (`scripts/hop.sh calc calc-nf
--seeds 5,6 --life 400000 --settings m2,m2coin --g.hop_fired 0`).

| | xor4, seed 5 | xor4, seed 6 | add4, seed 5 | add4, seed 6 | parity4, seed 5 | parity4, seed 6 |
|---|---|---|---|---|---|---|
| input lines alone | 0.53 / 0.41 | 0.53 / 0.34 | 0.60 / 0.49 | 0.62 / 0.45 | 0.61 / 0.25 | 0.58 / 0.23 |
| born | 0.648 / 0.635 | 0.669 / 0.631 | 0.634 / 0.566 | 0.623 / 0.563 | 0.673 / 0.098 | 0.660 / 0.100 |
| off (readout alone in the life) | 0.659 / 0.647 | 0.681 / 0.642 | 0.657 / 0.544 | 0.653 / 0.526 | 0.723 / 0.152 | 0.588 / 0.152 |
| **told the answer (m2)** | **0.919 / 0.889** | **0.863 / 0.854** | **0.739 / 0.652** | **0.738 / 0.667** | 0.843 / 0.026 | 0.788 / 0.020 |
| told good or bad per bit (m1) | 0.806 / 0.792 | 0.772 / 0.754 | 0.682 / 0.625 | 0.683 / 0.617 | 0.721 / 0.160 | 0.612 / 0.135 |
| coin (m2's errors on random lines) | 0.718 / 0.685 | 0.714 / 0.649 | 0.661 / 0.566 | 0.661 / 0.542 | 0.705 / 0.182 | 0.630 / 0.013 |
| m2, near and not fired | 0.914 / 0.900 | 0.896 / 0.902 | 0.734 / 0.675 | 0.728 / 0.628 | 0.802 / 0.031 | 0.809 / 0.008 |
| its coin | 0.706 / 0.663 | 0.753 / 0.710 | 0.677 / 0.620 | 0.691 / 0.606 | 0.729 / 0.178 | 0.667 / 0.081 |

- **The held-out columns move, and the coin does not explain it.** xor4 held-out 0.63 to
  0.87 (0.90 with near-not-fired marks), against 0.67 for the coin; add4 0.565 to 0.660,
  against 0.554. These members need exclusive-ors of lines that sit apart, which the born
  sheet does not carry (finding 41); after the life a linear reader gets them on cases never
  rewarded. For comparison, competitive learning (no reward, its own born sheet with tile
  winners) reached xor4 held-out 0.73 and 0.77, add4 0.63 and 0.63 (compete doc).
- Told good or bad per bit (m1) gets about half of it (xor4 0.77).
- Parity of 4: the seen column rises (0.67 to 0.82) and held-out falls to 0.02: every held
  case's nearest seen case has the opposite answer, so a reader that learned more of the
  seen cases gets these more wrong. Nothing about parity 4 is generalised, as on the toy.

## Step 3: MNIST (`scripts/inside.sh NAME --teach 1|2 ...`)

All numbers: test set, fresh linear reader of the non-sensor cells fit on 60,000 training
digits, after a 60,000-digit taught life (the readout of `scripts/mnist.sh online` learning
from good and bad, the rule hearing the errors), motor cells put back as born before the
read. "rate": hidden spikes per 1,000 cell-ticks at the end of the life (born 31 to 33).

**The output's own learning does not reach the reader.** The same taught life with the rule
off (`scripts/inside.sh teachoff --teach 2 --only mnist --kinds lived`; the readout gets
0.68 and 0.70 right in its life): lived 0.9245 and 0.9246 against born 0.9255 and 0.9243
(seeds 1, 2). Equal within a thousandth; wiring and rate as born.

**Tuning, seeds 1 and 2** (test digits; a handful of settings; mean of 2 seeds, each seed in
`out/hop/inside/NAME/`). Born: 0.9255 and 0.9243 (seeds 1, 2), the same sheet with the rule
off in a taught life 0.9245 and 0.9246. "pixels": pixel inputs per hidden excitatory cell
(born 7.3 of 34); "kept": share of born inputs still wired; "likeness": the compete doc's
pixel likeness (born 0.05). Each line is `scripts/inside.sh NAME --teach T --g.hop_on 1
FLAGS --only mnist`.

| NAME | teach | flags | lived | coin | lived - coin | shuffled | rate lived / coin | pixels | kept | likeness | verdicts | re-picks |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| hop2 | 2 | (defaults: shift 3) | 0.9220 | 0.9325 | -0.0105 | 0.9173 | 30 / 36 | 3.7 | 0.10 | 0.22 | 19,600 | 1.27 M |
| hop2nf | 2 | `--g.hop_fired 0` | 0.9244 | 0.9348 | -0.0104 | | 31 / 38 | 4.3 | 0.11 | 0.21 | 20,700 | 1.12 M |
| hop2good | 2 | `--g.hop_good 1` | 0.9158 | | | | 29 | 3.5 | 0.09 | 0.21 | 60,000 | 2.93 M |
| hop2s5 | 2 | `--g.hop_shift 5` | 0.9317 | 0.9342 | -0.0025 | | 34 / 38 | 5.1 | 0.13 | 0.22 | 20,500 | 0.61 M |
| **hop2s7** | 2 | `--g.hop_shift 7` | **0.9355** | 0.9329 | +0.0026 | | 39 / 38 | 5.8 | 0.22 | 0.18 | 19,400 | 0.25 M |
| hop2s9 | 2 | `--g.hop_shift 9` | 0.9342 | 0.9309 | +0.0034 | | 38 / 36 | 6.5 | 0.39 | 0.12 | 20,300 | 0.09 M |
| hop1 | 1 | (defaults: shift 3) | 0.9210 | 0.9233 | -0.0023 | | 30 / 29 | 4.5 | 0.12 | 0.20 | 20,200 | 0.88 M |
| hop1s5 | 1 | `--g.hop_shift 5` | 0.9331 | 0.9281 | +0.0050 | | 35 / 34 | 5.4 | 0.16 | 0.22 | 19,200 | 0.46 M |
| **hop1s7** | 1 | `--g.hop_shift 7` | **0.9336** | 0.9303 | +0.0033 | | 38 / 38 | 5.9 | 0.26 | 0.18 | 17,600 | 0.20 M |

## Step 4: on from the first digit of the online life

`scripts/mnist.sh online --seed S --checks 1000,5000,20000,60000,120000 --g.hop_on 1
--g.hop_teach 2 --g.hop_shift 7` (and `--g.hop_teach 1 --g.hop_shift 5`), seeds 5 and 6,
against the same life with the rule off (the compete job's runs of the same command, same
binary behaviour; `out/hop/online/`). The sheet's own readout, test digits, mean of 2 seeds
(each seed in brackets):

| digits seen | 1,000 | 5,000 | 20,000 | 60,000 | 120,000 |
|---|---|---|---|---|---|
| off | 0.556 (0.588, 0.523) | 0.713 (0.696, 0.730) | 0.718 (0.693, 0.743) | 0.693 (0.659, 0.727) | 0.667 (0.653, 0.681) |
| on, told the right button, shift 7 | 0.567 (0.583, 0.550) | 0.697 (0.701, 0.694) | 0.702 (0.699, 0.705) | 0.679 (0.674, 0.683) | 0.637 (0.626, 0.649) |
| on, told good or bad, shift 5 | 0.532 (0.564, 0.501) | 0.649 (0.605, 0.693) | 0.673 (0.645, 0.701) | 0.651 (0.625, 0.678) | 0.619 (0.606, 0.632) |
| competitive learning on (compete doc) | 0.539 | 0.686 | 0.723 | 0.697 | 0.655 |

No help to the readout. Told the right button it is the off curve within seed noise, a
little lower late; told good or bad it is lower at every checkpoint (0.04 by 5,000 digits).
Training time for 120,000 digits: off 375 s, on (shift 7) 383 s, on (good or bad, shift 5)
384 s (+2%).

## Cost

On this PC (shared with other agents, nice 19, rough):

| | rule off | rule on | |
|---|---|---|---|
| `blobbench speed --tiles 8 --g.learn_mode 5` (no verdicts: the near maps alone) | 66,700 | 61,600 | -7% (first form: 56,400, -16%) |
| taught MNIST life, shift 7 (`blobmnist ceiling --life-teach 2` life lines) | 6,440 | 6,320 | -2% |
| taught MNIST life, shift 3 | 6,440 | 6,150 | -5% |
| the online MNIST life, 120,000 digits, shift 7 (training seconds) | 375 s | 383 s | -2% |
| toy life, xor (about 500 verdicts in 40,000 steps) | 55,900 | 56,600 | 0 |
| toy life, parity 3 (shift 3, 4,300 verdicts) | 43,400 | 37,300 | -14% |

(ticks per second unless said.) A taught MNIST life is slow with or without the rule: the
readout's own re-picks (`evidence_rewire_as`) are 58% of its time (perf, shift 3); the rule's
re-picks (`select_top`) 6%, its verdict work 3%, the near maps 3%.

What the rule does per verdict on MNIST: about 370 cells taught (a third of the 1,548
learners), at shift 7 a counter step for one marked source in 128, and 13 re-picks (shift
3: 65). The bounds are what keep re-picks rare: without them every taught cell would
re-pick at every verdict (370 per verdict at 4 microseconds each, about 75 microseconds a
tick, several times the tick itself). Memory: the tag store (704 bytes a neuron, shared
with competitive learning and tags), 9 bytes a neuron of bounds, and 16 x 65 words of near
maps.
