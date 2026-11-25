# MNIST (2026-11-25)

The first real dataset. One handwritten digit is shown on the sensor sheet for 20 ticks;
ten buttons; good if the right one wins, bad otherwise. Every accuracy below is on the
10,000 test digits unless it says "validation" (the last 10,000 training digits, with
training on the first 50,000). Settings were chosen on validation only.

## Short version

All on the test set, trained on the 60,000 training digits, one core of this PC. The
machine was shared: load average 36 to 38 on 32 threads (other agents' runs) for every
timed number here, so wall times are rough and on the slow side.

| learner | what it reads | test | wall time (training) |
| --- | --- | --- | --- |
| linear (softmax, 3 passes) | grey pixels | 0.922 | 1.5 s |
| linear (softmax, 3 passes) | the bits the sheet is shown (28x28 at half ink) | 0.912 | 1.1 s |
| two-layer, 100 hidden, backprop, 1 pass | the same bits | 0.955 | 1.5 s |
| two-layer, 100 hidden, backprop, 1 pass | grey pixels | 0.955 | 1.3 s |
| random binary projection, 4,096 units x 32 lines, + linear | the same bits | 0.943 | 18.6 s |
| **(a) ceiling**: linear reader of the never-taught sheet's spike counts | 3,072 non-sensor neurons | **0.924** | 48 s sheet + 16 s reader |
| the same reader on the bits of the same digits | 784 bits | 0.916 | 4 s |
| the same reader on every neuron, sensor cells included | 4,096 | 0.933 | 20 s reader |
| **(b) the sheet's own readout from good and bad**, best point | motor cells, 102 per button | **0.710** | 91 s (20,000 digits) |
| (b) after one minute of training | | **0.709** | 60 s (12,700 digits) |
| (b) seeds 5-8 at 20,000 digits (stevejobs, accuracy only) | | 0.694, 0.740, 0.705, 0.700 (mean 0.710) | |
| (b) seeds 5-8 at 60,000 digits, one pass | | 0.664, 0.725, 0.684, 0.683 (mean 0.689) | |
| starting point (a): default sheet, `t14` in one corner, motor region below | 3,840 non-sensor neurons | 0.856 (bits alone 0.885; what the motor cells hear 0.759) | 38 s sheet + 21 s reader |
| starting point: default sheet, `t14`, the readout job's rule | 24 per button | 0.098, 0.103, 0.092 at 5,000, 20,000, 60,000 digits | 25 s (20,000) |

In plain words:

- **The never-taught sheet holds as much as the pixels, a little more.** A linear reader of
  its spike counts gets 0.924, against 0.916 for the same reader on the bits it is shown and
  0.922 for a linear classifier on grey pixels. It passes the linear classifier, narrowly.
  It does not reach the two-layer net (0.955) or a plain random projection of the same
  bits (0.943). So for digits the sheet is a modest feature bank: it adds less than a
  random projection of its own input does.
- **The readout that learns from good and bad gets 0.71**, far under its own ceiling. It
  learns fast (0.60 after 1,000 digits, 0.71 after 5,000) and then stops; with more passes
  it slowly gets worse (0.69 at 60,000 digits, 0.64 at 120,000). This is the gap.
- **Nothing handles a moved digit** (table at the end). Moved by 2 pixels, everything keeps
  65 to 74% of its unmoved score; the sheet (ceiling 72%, own readout 74%) sits with the
  random projection (71%), a little above the linear classifier (65%). No invariance to
  speak of.

## How a digit becomes bits

Three codes, all in `src/world/mnist_data.cpp`:

- `t14`: 2x2 pixels averaged to 14x14, on at half ink. 196 lines. Fits the default sheet's
  16x16 sensor lattice.
- `g14`: 14x14 at four grey levels, each pixel a 2x2 group of lines lit as a thermometer at
  1/8, 3/8, 5/8, 7/8 ink, laid as one 28x28 sheet so neighbours stay neighbours.
- `t28`: every pixel, on at half ink. 784 lines.

Baselines on each (test):

| code | linear | two-layer, 1 pass | projection + linear |
| --- | --- | --- | --- |
| t14 | 0.889 | 0.926 | 0.932 |
| g14 | 0.929 | 0.953 | 0.950 |
| t28 | 0.912 | 0.955 | 0.943 |

The sheet uses `t28` (or `g14`; they tie on validation for the sheet, 0.917 against 0.920
for the ceiling and 0.711 against 0.717 for the readout). `t28` and `g14` need a 28x28
lattice: the sensor area is made the whole 64x64-cell sheet (`--g.sensor_w 64
--g.sensor_h 64`), still every second cell, so the default 4,096-neuron sheet holds it.
No wiring knows anything about digits.

## (a) The ceiling

`blobmnist ceiling`: the sheet frozen (learning off), shown each training digit for 20
ticks, every neuron's spike count over those ticks kept; a softmax reader (the same code as
the linear baseline: each feature centred and scaled, 10 passes) fitted on them and scored
on the test digits' counts.

Where it started (validation, 10,000 digits fitted, default sheet, `t14`, motor region
below the sensor region):

| reader of | validation |
| --- | --- |
| sheet, sensor cells left out | 0.775 (first reader), 0.828 (better reader) |
| what the motor cells can hear (their tiles and the ones around) | 0.67 to 0.72 |
| the bits alone, same digits, same reader | 0.870 to 0.882 |

So at first the sheet lost information: 5 to 10 points under its own input, and the
motor cells could hear far less still. What moved it, one at a time (validation, 10,000
digits, better reader unless said):

| change | sheet, sensors left out | bits | note |
| --- | --- | --- | --- |
| default, first reader (each count scaled by its own spread, 3 passes) | 0.775 | 0.882 | |
| the reader: rare neurons not blown up by their tiny spread, 10 passes | 0.828 | 0.870 | half the first gap was the reader |
| 50,000 digits fitted instead of 10,000 (first reader) | 0.775 -> 0.822 | 0.895 | |
| 60 ticks a digit (first reader) | 0.833 | 0.882 | |
| 200 ticks a digit (5,000 fitted) | 0.831 | 0.870 | more ticks help, then level off |
| 40 ticks a digit | 0.864 | 0.870 | |
| inhibition gain 5 -> 2 | 0.848 | 0.870 | |
| 40 ticks and inhibition gain 2 | **0.871** | 0.870 | ties the bits |
| sector regulation off; threshold regulation off; noise off; gap 10 ticks; delays all 1 | +0.004 to +0.017 each (first reader) | | small |
| both regulations off | -0.03 | | |
| activity target 128 instead of 32 | -0.04 | | more spikes, but driven by the sheet, not the digit |
| the five richest sheets of the richness search | -0.01 to -0.06 | | they were tuned for time features: long delays |
| `t28` on the whole sheet, 20 ticks | **0.907** | 0.907 | ties |
| `t28`, 40 ticks, inhibition gain 2 | **0.917** | 0.907 | passes the bits |
| `g14`, 40 ticks, inhibition gain 2 | 0.920 | 0.920 | ties |
| `t28`, sparser born wiring (`local_connect_shift 4`) | 0.903 | 0.907 | |
| a reward-free pass of 10,000 digits with wiring by co-activity first | 0.910 (from 0.907); 0.913 with sparser wiring (from 0.903) | | a hair better; see below |

Why it lost information at first: spike counts are small (about 0.6 spikes a neuron in
20 ticks at the target rate of 3%), so each count is a noisy sample; more ticks and less
inhibition give each neuron more to say about the digit. And with the default layout half
the sheet (the motor region) is away from the image. On test, fit on all 60,000, the
default sheet reads 0.856 against 0.885 for its bits. With the digit spread over the whole
sheet, the frozen sheet keeps everything the bits have and adds a little: 0.924 against
0.916. That final sheet is the one the readout uses (20 ticks, inhibition gain 5, sparser
wiring); with 40 ticks and gain 2 the ceiling would be about a point higher (validation),
but the learned readout was worse with them (below). The `t28`/`g14` rows with 40 ticks
were run on stevejobs.

## (b) The readout learning from good and bad

`blobmnist online`: the task stream, learn mode 5 (nothing inside the sheet learns; only
the inputs of the motor cells), tested frozen at checkpoints on all 10,000 test digits.

### From the readout job's rule to something that learns

Validation, 2,000 digits scored, seed 1. The readout job's rule (`--g.learn_mode 5
--g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128`) on
the default sheet with `t14`:

| step | validation at 20,000 digits | what was wrong |
| --- | --- | --- |
| the readout job's rule, default sheet (24 motor cells a button: the 256 of the motor region) | 0.10 (chance; test 0.10 at 20,000 and 0.09 at 60,000) | five or six buttons never pressed: their cells wire nothing (the counter bug found below) |
| motor cells on the odd cells of the whole sheet, between the sensor cells: 102 a button | 0.11 | |
| ... and good also moves (`readout_good 1`: the right button gains) | 0.27 at 10,000 digits, 0.37 at 20,000 | four buttons (2, 6, 7, 9) dead |
| found: their cells had 160 to 260 counters at the +31 clamp, more than their budget of about 60, and the rewire dropped the whole level that did not fit, so they wired nothing | | a bug that only shows with many buttons |
| `readout_evidence 2`: that level fills the rest of the budget | 0.34 | all alive, but saturated: thresholds at 62 of 64 |
| sources wired at strength 1 (`readout_evidence_strong 0`), 8-bit counters, chance 2^-6 | 0.41 | |
| sparser born wiring (`local_connect_shift 4`, about 33 connections a cell) | 0.63 | |
| `t28` on the whole sheet | 0.70 | |
| both | **0.71** | the rule used below |

The rule: `--g.learn_mode 5 --g.readout_evidence 2 --g.readout_evidence_strong 0
--g.readout_evidence_bits 8 --g.readout_shift 6 --g.readout_balance 1 --g.readout_good 1`.
The sheet: `--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1
--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4`.

### Each thing tried on top of that, one at a time (validation at 20,000 digits; base 0.711)

| change | validation |
| --- | --- |
| more motor cells a button: 51 -> 102 | 0.621 -> 0.711 |
| a bigger sheet (16x16 tiles, sensors every 4th cell, 409 motor cells a button) | 0.647, and 5 times slower |
| larger evidence counters: 6 bits -> 8 bits (strength 1, chance 2^-6) | 0.236 -> 0.41 (on the earlier sheet) |
| chance 2^-5 / 2^-7 instead of 2^-6 | 0.694 / 0.704 |
| 40 ticks a digit and inhibition gain 2 (best for the ceiling) | 0.605 |
| long delays off (all 1 tick) | 0.711 |
| 5 quiet ticks between digits | 0.680 |
| balance rounded to the nearest power of two (1/8 instead of 1/16 for nine others) | 0.27 (on the earlier sheet, against 0.41) |
| no move on good (`readout_good 0`) | 0.14 (on the earlier sheet) |
| threshold regulation off | 0.13 (on the earlier sheet) |
| a reward-free pass over 10,000 digits with wiring by co-activity first | 0.640 |
| excitatory sources only (`readout_evidence 3`) | 0.711 (button 5 no longer silent; same score) |
| ... and the reward-free pass | 0.703 |
| `g14` instead of `t28` | 0.717 |

What helped: putting the motor cells among the image, letting good move the counters,
the counter fix, strength 1, sparser wiring, more cells a button. What did not: a bigger
sheet, more ticks, delays off, a gap, the reward-free pass (worse for the readout, a hair
better for the ceiling: wiring by co-activity reshapes the sheet in a way a linear reader
can use and this readout cannot), excitatory-only (same).

### As a function of digits seen (test, seed 1, the rule above)

| digits seen | training time | test | the training stream's own score |
| --- | --- | --- | --- |
| 1,000 | 4.7 s | 0.599 | 0.454 |
| 5,000 | 24 s | 0.706 | 0.682 |
| 12,700 (one minute) | 60 s | 0.709 | 0.702 |
| 20,000 | 91 s | 0.710 | 0.703 |
| 60,000 (one pass) | 266 s | 0.687 | 0.693 |
| 120,000 (two passes) | 535 s | 0.640 | 0.651 |
| 180,000 (three passes) | 813 s | 0.635 | 0.630 |

With `readout_evidence 3` (excitatory sources only; every button alive) on test, seed 1:
0.719 at 20,000 digits (105 s), 0.708 at 60,000 (301 s), 0.658 at 120,000 (600 s). A
point better at the peak, and the decline is a little slower, but it is still there. Not
put in `scripts/mnist.sh` because the tables above were run with 2.

Training ran at 25,000 to 38,000 ticks a second under the load above (60,000 alone).
Best is reached by about 5,000 digits (24 s); after 20,000 it declines. One button (5) is
never pressed in this life: fives are 9% of the test digits, all wrong. With
`readout_evidence 3` that button is alive, at the same score on validation (0.711).

## Shift test

Trained as above, scored on the test digits moved by 1 and by 2 pixels (mean of right,
left, down, up):

| learner | not moved | moved 1 | moved 2 |
| --- | --- | --- | --- |
| linear on grey pixels | 0.922 | 0.857 | 0.602 |
| linear on the bits | 0.912 | 0.850 | 0.597 |
| two-layer on the bits | 0.955 | 0.895 | 0.646 |
| random projection on the bits | 0.943 | 0.888 | 0.666 |
| ceiling: linear reader of the sheet | 0.924 | 0.869 | 0.661 |
| the sheet's own readout, after 20,000 digits | 0.713 | 0.666 | 0.526 |
| the sheet's own readout, after 180,000 digits | 0.635 | 0.595 | 0.461 |

Nothing here copes with a move, as expected. Kept at 2 pixels, as a share of the unmoved
score: linear 65%, two-layer 68%, projection 71%, sheet ceiling 72%, the sheet's readout
74%. A frozen local sheet gives a little tolerance (a neuron hears a patch, not a pixel),
about what a random projection gives.

## Reading

- **The thesis holds at the level of the ceiling, barely.** A sheet nobody taught,
  read by a linear reader, passes a linear classifier on the raw pixels (0.924 against
  0.922) and its own input bits (0.916). It is not rich in the sense that matters for
  MNIST: a random binary projection of the bits, with no recurrence and no spikes, does
  better (0.943), and so does a two-layer net in one second.
- **Selection from good and bad does not get there yet.** 0.71 where a reader could get
  0.92. Learning is fast and then stalls and drifts. The pieces that worked on Echo with
  eight cues did not carry to ten buttons without fixes (a counter bug that silenced
  buttons, saturation at strength 4, a balance that drains), and what is left is a
  readout made of spiking cells with binary, equal-strength inputs chosen by counters,
  whose firing is regulated back to target. Each button's answer is a vote of about a
  hundred such cells. That is a much weaker reader than a softmax over 3,072 graded
  weights.
- **Where the sheet sits matters more than how it is tuned.** Spreading the digit and the
  motor cells over the whole sheet was worth more than any genome number.
- Against the owner's goal (beat the 2-layer net in under a minute, never taught): the
  ceiling is 3 points short of the 2-layer net and the learned readout 25 points short.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, headline: MNIST is the first real data. A never-taught sheet read
  by a linear reader gets 0.924 on the test set (linear classifier on pixels 0.922, on the
  same bits 0.912-0.916; two-layer net 0.955; random projection of the bits 0.943). The
  readout learning from good and bad gets 0.71 (seeds 5-8: 0.69 to 0.74), best within
  5,000-20,000 digits, then drifting down. Selection does not reach what is there.
- `docs/findings.md`, principle 8 ("the sheet is already rich"): qualify. For digits the
  frozen sheet holds what its input holds plus about a point; a random binary projection
  of the same bits adds three. Rich for timing features, plain for shape.
- `docs/findings.md`, principle 16 ("a decision needs many cells"): second result, on ten
  buttons: 51 to 102 cells a button, 0.62 to 0.71.
- `docs/findings.md`, principle 14 (evidence before commitment): add the failure with many
  buttons: with ten buttons the counters pile up at the clamp, and the rewire as built
  dropped a whole level that did not fit the budget, silencing buttons for good (fixed as
  `readout_evidence 2`). Strength 4 saturates the cells (thresholds at the cap); strength
  1 and 8-bit counters are needed at ten buttons.
- `docs/findings.md`, new hint: moving on good matters with many buttons
  (`readout_good 1`: 0.14 to 0.41 on the same sheet), the opposite of the two-to-four-cue
  result, because with ten buttons "bad" says little about which one was right.
- `docs/findings.md`, new hint: layout beats tuning for real input. Spreading the digit
  over the whole sheet and putting the motor cells among it (odd cells between the sensor
  cells) took the ceiling from 0.856 to 0.924 and the readout from chance to 0.7.
- `docs/findings.md`, walls, "A new place": the shift test (2 pixels keeps 65-74% for
  everything, sheet included). No invariance.
- `docs/task-map.md`, tier 11: "MNIST: never-taught sheet + linear reader 0.924 (passes
  the linear classifier on pixels, 0.922); the readout from good and bad 0.71; two-layer
  net 0.955 not reached." Milestone "MNIST": first target met only by the offline reader,
  not by a readout that hears good and bad.
- `docs/notes-world.md`: the `mnist` task, `--mnist-code/--mnist-first/--mnist-count`, the
  data path (`data/mnist` or `$THONK_MNIST`), `blobmnist`, and the bigger line limits.
- `genome.hpp` (core owner): document `readout_evidence 2` and `3` and `readout_balance 2`
  beside the existing values. Consider making `readout_evidence 2` the behaviour of 1: the
  old rewire also under-wires whenever counters tie at the cut, which may cost something
  on Echo too (not measured).

## What I would try next

1. Find the drift: the readout peaks at 5,000-20,000 digits and loses 2 to 7 points by
   180,000. Counters at 8 bits are far from the clamp, so look at threshold regulation of
   the motor cells and at which sources come and go.
2. A graded readout: a cell's wired sources at strengths from their counters (1, 4, 5 by
   rank) instead of all at one, so a cell is a weighted vote, not a count.
3. Let the readout learn on a margin: also update on good when the right button only just
   won (the perceptron with margin), since good is where the label is.
4. A sheet built for shape rather than time: the richness search scored on digits (the
   ceiling runs in under a minute per sheet on validation), not on order and gap.
5. For invariance, the place wall: shared wiring between patches, which nothing here has.

## Exact script lines

```
scripts/get_mnist.sh                    the data, checked
scripts/mnist.sh                        baselines, ceiling, online, then the table
scripts/mnist.sh baselines --mnist-code t14
scripts/mnist.sh ceiling                (fit 60,000, test, shift test)
scripts/mnist.sh online                 (checks 1,000 to 180,000 digits and at 60 s, shift test)
scripts/mnist.sh online --val 1 --checks 5000,20000 --test-count 2000 --diag 1
scripts/mnist.sh watch                  the viewer (owner only; opens a window)
```

The tuning sweeps are `out/mnist/csweep.py` over the variant files in `out/mnist/` (not
committed; `out/` is ignored), each a list of `name: flags` run by `blobmnist ceiling` or
`blobmnist online` with `--val 1`.

## What was built

- `scripts/get_mnist.sh`: the four idx files from the PyTorch S3 mirror (Google's CVDF
  mirror second), md5 and size checked, into `data/mnist/` (git-ignored).
- `src/world/mnist_data.*`: loader, the three codes, a seeded permutation (a Feistel
  network, so the order needs no table).
- `src/world/task_mnist.cpp`: task kind `mnist` (last in the enum). The good and bad lines
  are left unattached, so the last answer is never on the sheet while the next digit is
  shown. `blobrun --task mnist --mnist-code t28 ...` runs the training stream (its window
  score is the training stream's, not a test). `TASK_MAX_SHEET` and
  `PLUG_MAX_SENSOR_LINES` 256 -> 1024, `PLUG_MAX_SPREAD` 1024 -> 4096.
- `src/mnist/` (`blobmnist`): `baselines`, `ceiling`, `online` (`--checks`,
  `--check-seconds`, `--pretrain`, `--shift`, `--val`, `--gap`, `--diag`). The learners
  (`mnist_learn.cpp`) use floats; nothing in `src/blob/` does.
- Core, small and behind new values of existing numbers (1 unchanged):
  `readout_evidence 2` (the level that does not fit fills the budget), `readout_evidence 3`
  (2, excitatory sources only), `readout_balance 2` (nearest power of two).

## Rough

- Wall times were taken with the machine at load 36 to 38 (other agents); quiet-machine
  numbers would be faster for everything, the sheet most.
- The seed spread for (b) is from stevejobs (byte-identical results, slower clock).
- The ceiling's reader is softmax by SGD with a fixed recipe chosen on validation; a
  stronger reader could find a little more.
- The viewer draws the packed motor groups' frames (top row), not the spread motor cells.
  The digit itself shows on the sheet (checked with `blobview --capture` on lavapipe).
- `readout_balance 2` and `readout_evidence 2/3` are not described in `genome.hpp` (the
  contract; only the core owner edits it). Their comments are in `learn_readout.cpp` and
  `learn_evidence.cpp`.
- The decline after 20,000 digits is not explained. Counters at 8 bits are far from their
  clamp (-30 to +40 at 20,000 digits), so it is not the clamp.
