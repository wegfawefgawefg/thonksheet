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
reader. "coin": the rule on, every error sent to a random line. Seeds 5-8; one setting, not
tuned (the defaults above). Rule numbers per life on xor: about 500 verdicts reach the rule
(the live readout is right almost at once), about 940 cells taught per verdict, 10 counter
steps per taught cell, 400 re-picks per verdict.

| task (fresh output, late) | born | off | **told the right button (m2)** | told good or bad (m1) | m1, and good teaches | coin (m2's errors on random lines) |
|---|---|---|---|---|---|---|
| both-not-either (xor) | 0.857 (0.698) | 0.876 (0.735) | **0.998 (0.960)** | 0.982 (0.829) | 0.915 | 0.933 (0.770) |
| parity of 3 | 0.542 (0.525) | 0.550 (0.532) | **0.824 (0.711)** | 0.681 (0.596) | 0.608 | 0.589 (0.544) |
| parity of 4 | 0.473 | 0.488 | 0.494 | 0.473 | | 0.475 |

Per seed (5, 6, 7, 8): xor m2 0.999, 0.997, 0.998, 0.998 against born 0.839, 0.895, 0.856,
0.836 and coin 0.933, 0.934, 0.933, 0.931; parity 3 m2 0.826, 0.855, 0.816, 0.800 against
born 0.528, 0.561, 0.561, 0.518 and coin 0.582, 0.588, 0.575, 0.609. Every seed is above its
coin control on xor and parity 3 in both teaching modes.

- **Yes on the toys: the inside comes to carry the product.** A readout that has never seen
  the task reads exclusive-or from the lived sheet at 0.96 in its first quarter and 0.998 at
  the end (born 0.70 and 0.86): the same as tags (0.961 / 0.998) on the same test, reached
  by a different route (tags follow the button's own wiring back one hop; this reaches
  every hidden cell through random wires). Parity of 3, which only the richest sheet
  carried (finding 33), goes 0.54 to 0.82.
- **Told the right button teaches more than good or bad** (parity 3: 0.82 against 0.68), as
  expected: on a two-button task "the pressed one was wrong" already names the right one,
  so the gap is in how many cells move (a +1 line and a -1 line reach more cells than one),
  not in what they are told.
- **Good should teach nothing.** Letting good say "the pressed button should have been
  higher" costs both tasks (xor 0.98 to 0.92, parity 3 0.68 to 0.61). Finding 39 again.
- **The coin control moves the toys too** (xor 0.86 to 0.93, parity 3 0.54 to 0.59): churn
  of the wiring with no information is worth something, as the shuffled control was for
  competitive learning and the unsigned half was for reverse replay. The errors are worth
  far more (+0.065 and +0.235 over the coin).
- **Parity of 4: no**, also with a four times longer life (`scripts/hop.sh toy toys-long
  --settings off,m2,m2coin --tasks parity4 --seeds 5,6 --steps 160000 --window 4000`: off
  0.475, m2 0.478, coin 0.498). The live readout stays at chance through the life too.

**Which cells should count as near** (m2, seeds 5-8; `scripts/hop.sh toy toys-near0
--settings m2 --tasks xor,parity3 --g.hop_near 0`, and `toys-nofired ... --g.hop_fired 0`):

| marks from | xor | parity 3 |
|---|---|---|
| cells at or over the bar only (fired, or over it but held back): Hebbian with a sign | 0.997 | 0.849 |
| fired, or within a quarter of the bar under it (the default) | 0.998 | 0.824 |
| **within a quarter of the bar under it, not fired** | 0.997 | **0.893** |

On parity 3 the cells that came near and did not fire are the better teachers (0.893, every
seed 0.886 to 0.903, against 0.80 to 0.86 for the default): "fire more" has the most to say
to a cell that almost fired, as the float rule's pseudo-derivative says. On xor everything
reaches 0.997.

## Step 3: MNIST (`scripts/inside.sh NAME --teach 1|2 ...`)

All numbers: test set, fresh linear reader of the non-sensor cells fit on 60,000 training
digits, after a 60,000-digit taught life (the readout of `scripts/mnist.sh online` learning
from good and bad, the rule hearing the errors), motor cells put back as born before the
read. "rate": hidden spikes per 1,000 cell-ticks at the end of the life (born 31 to 33).

**The output's own learning does not reach the reader.** The same taught life with the rule
off (`scripts/inside.sh teachoff --teach 2 --only mnist --kinds lived`; the readout gets
0.68 and 0.70 right in its life): lived 0.9245 and 0.9246 against born 0.9255 and 0.9243
(seeds 1, 2). Equal within a thousandth; wiring and rate as born.
