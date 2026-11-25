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
