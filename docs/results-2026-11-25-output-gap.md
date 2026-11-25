# Why our output cannot read what the sheet holds (2026-11-25)

Queue item 2c, idea 161. A fresh linear reader, fit offline, gets 0.926 on MNIST from the
sheet as born and 0.952 from the sheet after a reward-free life under competitive learning.
The sheet's own output, learning online from good and bad, gets 0.71 (0.81 with the
close-vote gate) and drifts down on long lives. Where do the 0.2 go?

## Prediction (written before any ladder run)

The ladder: (1) float softmax, offline, 10 passes, every non-sensor cell; (2) the same,
online, one pass; (3) integer weights, online, told the right button; (4) the same, told only
good or bad; (5) each output unit hears only what a motor cell hears, a button is the vote
of its 102 cells; (6) one-bit connections chosen by evidence counters, at the engine's width
and step chance; (7) the engine.

Nothing seen before writing this except the numbers in the brief (0.926 / 0.952 offline,
0.71 / 0.81 online).

- The rung that loses the most, on the born sheet at 20,000 digits:
  - 5 to 6 (one-bit connections, a budget of about 33 per cell): probability 0.35;
  - 3 to 4 (good or bad only, with ten buttons): 0.25;
  - 6 to 7 (spikes, regulation, the sheet around the motor cells): 0.20;
  - 4 to 5 (local hearing, a vote of thresholded cells): 0.15;
  - 1 to 2 or 2 to 3: 0.05.
- Sizes, best guess: 1 to 2 about 0.01; 2 to 3 about 0.03; 3 to 4 about 0.06; 4 to 5
  about 0.05; 5 to 6 about 0.08; 6 to 7 about 0.05.
- The drift (lower at 120,000 than at 20,000 digits) first shows at rung 4 (the good half
  of the rule moves on every good, so it learns class means and the balance between
  buttons wanders): 0.5; at rung 6: 0.3; only at rung 7: 0.2.
- The lived sheet loses more of its lead than the born sheet on the way down (its extra
  0.026 is mostly gone by rung 6): 0.7.

How it scored (born sheet, seeds 5-12, 20,000 digits; the ladder below):

- The largest loss is 3 to 4, my second choice (0.25), not 5 to 6 (0.35). And it is not
  "good or bad only" as such: it is what the engine's rule does on a good. Its size is 0.28,
  where I guessed 0.06.
- Sizes: 1 to 2 was 0.02 (guessed 0.01); 2 to 3 0.04 (0.03); 3 to 4 0.28 (0.06); 4 to 5 a
  gain of 0.11 (guessed a loss of 0.05); 5 to 6 nothing (0.08); 6 to 7 nothing on the plain
  rule (0.05). Wrong almost everywhere past rung 3.
- The drift first at rung 4 (0.5): wrong. On the plain rule no rung of the copy drifts,
  only the engine; under the gate it starts at rung 5, the vote of cells.
- The lived sheet's lead gone by rung 6 (0.7): half right. It is gone at rung 3 already
  (integer weights: 0.856 against 0.864 born) and in the engine; but my copy's rung 6 reads
  the lived sheet 0.04 better than the born one, more than any rung from 3 to 5.

## Short version

MNIST test digits, seeds 5-12, born sheet, at 20,000 digits unless said.

- **Where the 0.23 goes.** A reader of every non-sensor cell fit offline: 0.926. The same
  online, one pass: 0.905. Integer weights told the right answer (the perceptron): 0.864.
  The same told only good or bad, moving on good the way the engine does: **0.577**, three
  or four buttons dead for good. The real motor cells with only what they hear: 0.602; as
  thresholded cells: 0.691; with one-bit connections chosen by the engine's counters: 0.692.
  The engine: 0.692. So one step holds the loss: **the rule's good half** (0.28 on the
  linear reader); local hearing costs nothing; thresholded cells win 0.09 of it back;
  one-bit counters cost nothing more; and on the plain rule the engine equals my copy of it,
  so nothing is left unnamed there.
- **Why the good half fails with ten buttons.** On every good the pressed button gains and
  the nine others lose a little. A winner wins more; and with accuracy over one half, goods
  outnumber bads, so a button that is seldom pressed is pushed down more often than up and
  sinks. The gate (move on good only when the vote was close) stops the first. Moving only
  the runner-up on a close good stops the second.
- **The fix, in the engine** (`--g.readout_good 2 --g.margin_percent 20 --g.margin_win 2`):
  **0.822, 0.845, 0.852** at 20,000, 60,000, 120,000 digits, against 0.692, 0.673, 0.649
  plain and 0.811, 0.799, 0.759 with the gate alone. No drift, still rising at 120,000;
  every seed between 0.847 and 0.856 at 120,000; no button goes quiet. With competitive
  learning on from the first digit: 0.816, 0.837, 0.844 (gate 0.811, 0.798, 0.708). Nothing
  changes per tick; a reward moves two buttons' cells instead of ten, so training per digit
  went from 3.2 ms (plain) and 2.0 ms (gate) to 1.2 ms.
- **What is left**: the fixed engine is 0.04 under the told-the-answer perceptron at 20,000
  (0.025 at 120,000) and 0.10 under the offline reader. Under the gate my copy of the motor
  cell reads 0.1 lower than the real one, for a reason not found. And one rung is not fixed:
  **the sign**. A one-bit excitatory connection can only speak for a button; a linear reader
  limited to weights of zero or more loses 0.17 under the gate. Opponent cells (half of each
  button's cells vote against it) were built and cost in the engine.
- **The lived sheet's lead does not reach the output.** Offline 0.952 against 0.926 born;
  online float 0.925 against 0.905; but integer readers, the motor cells and the engine read
  the lived sheet no better than the born one (engine plain 0.693 / 0.692; fixed engine on
  the lived sheet 0.818 and 0.845 at 20,000 and 120,000, against 0.822 and 0.852 born). The lead is lost at the first integer
  rung (0.856 against 0.864); centring the counts gets about 0.01 of it back late (2 seeds).
- **A weighted vote on top** (`--g.readout_evidence_strong 2`, idea 147: each motor cell's
  best quarter of connections at strength 4): **0.837, 0.854, 0.863**, above the runner-up
  rule alone on every seed, free per tick.
- **Toy rows**: 16 cues 0.305 to 0.605 (0.723 with the weighted vote), 8 cues 0.842 to 0.940,
  order 0.695 to 0.779, echo k=2 and k=4 unchanged; **exclusive-or 0.883 to 0.793** (the
  gate's cost; with two buttons the new rule is the gate), 0.725 with the weighted vote.
  Reference lives byte-identical.

## The ladder

All numbers: test accuracy on the 10,000 MNIST test digits, mean of seeds 5-12, at 20,000,
60,000 and 120,000 digits of the online life (two passes of the 60,000 training digits).
Born: the sheet as born. Lived: the same sheet after 60,000 digits with no reward under
competitive learning (`--g.compete_on 1 --g.compete_shift 5`), then frozen. Rung 7 on the
lived sheet is the engine on that frozen sheet (`--life 60000 --life-freeze 1`). Each seed's
full lines are in `out/output-gap/ladder-born/` and `ladder-lived/`.

| rung | born 20k | 60k | 120k | lived 20k | 60k | 120k |
|---|---|---|---|---|---|---|
| 1  float softmax, offline | 0.926 (offline) |  |  | 0.952 (offline) |  |  |
| 2  float softmax, online | 0.905 | 0.920 | 0.924 | 0.925 | 0.942 | 0.947 |
| 3  integer, told the right button | 0.864 | 0.881 | 0.880 | 0.856 | 0.883 | 0.899 |
| 4n integer, good/bad, nothing on good | 0.401 | 0.423 | 0.438 | 0.439 | 0.502 | 0.523 |
| 4u integer, good/bad, engine's good half | 0.577 | 0.581 | 0.580 | 0.584 | 0.589 | 0.591 |
| 4g 4u, only on a close vote (gate) | 0.872 | 0.901 | 0.908 | 0.871 | 0.908 | 0.912 |
| 4r close good: pressed up, runner-up down | 0.869 | 0.887 | 0.893 | 0.766 | 0.801 | 0.816 |
| 4up 4u, weights never below zero | 0.528 | 0.532 | 0.533 | 0.571 | 0.584 | 0.586 |
| 4gp 4g, weights never below zero | 0.702 | 0.692 | 0.693 | 0.668 | 0.646 | 0.643 |
| 5s motor cells' fields, sum of drives | 0.602 | 0.605 | 0.603 | 0.606 | 0.610 | 0.610 |
| 5v motor cells' fields, graded vote | 0.691 | 0.691 | 0.694 | 0.627 | 0.632 | 0.637 |
| 5sg 5s, close-vote gate | 0.879 | 0.899 | 0.901 | 0.873 | 0.903 | 0.906 |
| 5vg 5v, close-vote gate | 0.845 | 0.851 | 0.823 | 0.829 | 0.842 | 0.834 |
| 6c counters as graded weights | 0.719 | 0.702 | 0.698 | 0.731 | 0.715 | 0.709 |
| 6b one-bit top counters (engine) | 0.692 | 0.686 | 0.680 | 0.731 | 0.722 | 0.714 |
| 6g 6b, close-vote gate | 0.721 | 0.726 | 0.725 | 0.762 | 0.768 | 0.765 |
| 6r 6b, close good: pressed up, runner-up down | 0.754 | 0.780 | 0.797 | 0.797 | 0.826 | 0.836 |
| 7  engine, plain | 0.692 | 0.673 | 0.649 | 0.693 | 0.679 | 0.653 |
| 7  engine, gate | 0.811 | 0.799 | 0.759 |  |  |  |
| 7  engine, runner-up + gate | 0.822 | 0.845 | 0.852 | 0.818 | 0.837 | 0.845 |

Seed spread at 20,000 (lowest to highest, born): rung 2 0.902-0.908, rung 3 0.851-0.876,
4u 0.534-0.611, 4g 0.864-0.883, 5sg 0.863-0.889, 5vg 0.833-0.854, 6b 0.629-0.729, 6r
0.713-0.787; engine plain 0.659-0.739, gate 0.800-0.820, runner-up 0.818-0.827.
(`python3 scripts/outgap_table.py out/output-gap --doc` prints this table and the spreads.)

Read down a column; each line adds one handicap to one above it:

- **1 to 2, online, one step a digit: 0.02.** Gone by 60,000 digits (0.920).
- **2 to 3, integer weights and the perceptron's step: 0.04** at every length.
- **3 to 4, told only good or bad: the good half is the whole loss.** The engine's rule
  (4u: on good the pressed button gains 16 and the nine others lose 1) reads 0.58 and never
  moves from there; three or four of the ten buttons go quiet within 5,000 digits and stay
  quiet, on every seed. Moving nothing on good (4n) is worse still (0.40: bad alone, with
  ten buttons, says too little). Moving on good only when the vote was close (4g, the gate)
  reads 0.872, the same as being told the answer (0.864), and passes it later (0.908 against
  0.880). Moving only the pressed button and the runner-up on a close good (4r) does the
  same (0.869, 0.893). Why the engine's good half fails: a button that wins gains on every good, so it
  wins more; with accuracy over one half, goods outnumber bads, so every button that is not
  pressed is pushed down more often than up and sinks.
- **4 to 5, each output is the real motor cells with only what they can hear: nothing.**
  The sum of the 102 cells' drives (5s 0.602, 5sg 0.879) reads as the free reader of every
  cell (4u 0.577, 4g 0.872). The motor cells' 11 slots carry what a digit holds. Turning each
  cell into a thresholded unit whose vote is how far it is over threshold (5v) *helps* the
  engine's rule (0.691 against 0.602): a cell's threshold follows its own drive, which caps
  the rich-get-richer. Under the gate it costs 0.034 (5vg 0.845 against 5sg 0.879).
- **5 to 6, one-bit connections chosen by counters at the engine's width and chance:
  nothing on the engine's rule** (6b 0.692 against 5v 0.691; counters used as graded weights,
  6c, 0.719). Under the gate my copy loses 0.12 (6g 0.721 against 5vg 0.845), but see the
  next line.
- **6 to 7, the engine itself: equal on the plain rule** at 20,000 (0.692 against 6b
  0.692), so on the plain rule no handicap is left unnamed there; later the engine drifts
  where the copy does not (0.649 against 0.680 at 120,000). **Under the gate and the
  runner-up rule the engine reads better than my copy**: 0.811 against 6g 0.721, 0.822
  against 6r 0.754. The copy's motor cell is a count reader (drive = wired spike counts, vote
  = how far over a threshold that follows the drive); the real cell fires tick by tick with
  a leak, a branch bonus and delays. Something in the real cell or the live sheet helps the
  gated rules and not the plain one. Not found. Candidates: the copy's vote is a smooth
  function of counts, so 31% of its goods count as close (34% with the runner-up rule)
  against 42% in the engine (the gate then learns from fewer, easier goods); the real cell responds to spikes that come
  together within a few ticks, which counts do not see; motor cells' own spikes feed back
  into the sheet. The ordering of the rules is the same in copy and engine (plain < gate <
  runner-up, and only the runner-up rule keeps rising).
- **The sign.** A one-bit excitatory connection can only add evidence for a button; only an
  inhibitory source can count against. A linear reader allowed no weight below zero loses
  0.17 under the gate (4gp 0.702 against 4g 0.872) and 0.05 on the plain rule (4up). The
  engine wires inhibitory sources too (`readout_evidence 2`), which is part of the way back.
  Opponent cells (half of each button's cells vote against it and learn the other way, the
  critic's trick) were tried in the engine and cost (below), so this rung is not fixed here.

Where the drift starts (20,000 against 120,000): rung 2 rises (0.905 to 0.924); rung 3 is
flat; 4u and 5v are flat (they stop learning within a few thousand digits) and 6b loses
0.012; the engine on the plain rule falls 0.043. Under the gate, the vote of cells falls
(5vg 0.851 to 0.823 from 60,000) and the engine falls 0.052; the free and summed readers do
not (4g, 5sg rise). Under the runner-up rule nothing falls at any rung: 4r, 6r and the
engine all still rise at 120,000.

The lived sheet, rung by rung (lived minus born, at 20,000 / 120,000): offline +0.026;
online float +0.020 / +0.023; integer told the answer -0.008 / +0.019; the gate +0.000 /
+0.004; the runner-up linear reader -0.10 / -0.08 (it does badly on the lived sheet, not
explained); the motor cells' summed fields +0.00, their graded vote -0.06; my copy of the
engine's counters +0.04 on every rule; the engine itself +0.001 plain and -0.004 / -0.007
with the runner-up rule. So the lead is lost where the float readers stop: at integer
weights on raw counts. The lived cells fire about twice as often as the born ones and much
less evenly (the compete job: 63 against 33 spikes per 1,000 cell-ticks), and a float reader
centres and scales each one. Centring the integer readers' counts (3c, 4gc, 4rc; running
mean per cell, seeds 5 and 6 only) gives back about 0.01 on both sheets at 120,000 (lived
3c 0.912 against 3 0.898; born 0.895 against 0.879) and nothing at 20,000: part of it, not
the whole.

## What was built

**The measuring tool** (`blobmnist ladder`, `src/mnist/mnist_ladder.cpp`,
`mnist_ladder_linear.cpp`, `mnist_ladder_cells.cpp`; floats allowed, nothing in
`src/blob/`). The sheet (scripts/mnist.sh's sheet, with `--g.tile_winners 4` so born and
lived are the same sheet) is frozen and shown 60,000 training digits of the task's stream and
the 10,000 test digits once. Each neuron's spike count per digit is kept twice: over all 20
ticks (what a reader reads) and over the last 15 (the window the engine's rule learns from).
Online readers then go through that recording twice (120,000 digits; the second pass in a
fresh order) and are scored on all 10,000 test digits at 1,000, 5,000, 20,000, 60,000 and
120,000. The rungs:

| rung | reader |
|---|---|
| 1 | float softmax, offline, 10 passes, every non-sensor cell (the ceiling's own reader and recipe) |
| 2 | the same, online, one step per digit, constant step size |
| 3 | integer weights, online, told the right button: the multiclass perceptron (on a miss the right button gains the digit's counts, the pressed one loses them) |
| 4 | the same, told only good or bad. On bad the pressed button loses 16 and the nine others gain 1 (the engine's balance). What moves on a good: 4n nothing; 4u the pressed button gains 16 and the others lose 1 (the engine's `readout_good 1`); 4g the same only when the vote was close (lead under 20% of the winner's score: the engine's gate); 4r on a close vote the pressed button gains and only the runner-up loses. 4up and 4gp: 4u and 4g with no weight allowed below zero |
| 5 | a button is its 102 motor cells; each cell has integer weights on exactly the sources it can hear (its 9 local tiles and 2 far tiles, sensor cells included, 704 places), starts from its born wiring and learns by the rule of 4. 5s: a button scores the sum of its cells' drives; 5v: each cell gives its drive over its own threshold, in its own spread, a stand-in for a spike count (the threshold follows the cell's running mean and spread so it is over it on the 42% of digits the real motor cells fire on); 5sg, 5vg: with the gate |
| 6 | the engine's own learning in counts: per cell an 8-bit counter on each of its 704 places, starting at the born strengths; each tick a source fired in the last 15 is one chance of a step (2^-6 for the pressed button's cells, 2^-10 for the others), excitatory and inhibitory sources moving opposite ways; the vote read as 5v. 6c: the counters used as graded weights; 6b: one-bit wiring of the top counters at the cell's born count, ties filled from a random place (the engine's `readout_evidence 2`); 6g: with the gate; 6r: the runner-up rule with the gate |
| 7 | the engine: `blobmnist online`, the same sheet and rule (`scripts/outgap.sh online`) |

**The engine options** (all genome numbers that default to off; off is byte-identical):

- `readout_good 2` (`learn_readout.cpp`): on a good the pressed button's cells gain and only
  the runner-up's cells lose, at the pressed button's rate; the other eight do not move. It
  reads the runner-up from the vote marks the gate already writes, so it is used with
  `--g.margin_percent 20` (and `--g.margin_win 2` for the gate).
- `readout_opponent 1` (`blob/opponent.hpp`, `learn_readout.cpp`, `world/plug.cpp`,
  `plug_vote.cpp`): motor cells on odd columns of the motor lattice vote against their button
  and learn the other way; the vote is a signed count.
- `readout_halve 1`: a counter reaching the clamp halves its cell's whole row.
  `readout_keep_mul N`: a motor cell wires N times its born count.
  `readout_evidence_strong 2`: the top quarter of what a cell wires at strength 4, the rest
  at 1 (a weighted vote, idea 147). All in `learn_evidence.cpp`.
- `blobmnist online --life N [--life-freeze 1]`: a reward-free life first (rung 7 on the lived
  sheet), and it prints the share of goods that were close votes.

## The fix: on a close good, only the runner-up loses

The ladder says the loss is in what the rule does on a good. The gate (already in) stops
moving on comfortable goods and takes the engine from 0.69 to 0.81, but on a close good it
still pushes all nine other buttons down, so over a long life the buttons drain and it falls
(0.81 to 0.76 by 120,000). `readout_good 2` keeps the gate and changes who pays: on a close
good the pressed button's cells gain and only the runner-up's cells lose, at the same rate;
the other eight do not move. Bad is unchanged.

`scripts/outgap.sh online rival --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2`,
test accuracy, seeds 5-12:

| sheet | rule | 1,000 | 5,000 | 20,000 | 60,000 | 120,000 |
|---|---|---|---|---|---|---|
| born | plain (`scripts/mnist.sh online`'s rule) | 0.572 | 0.673 | 0.692 | 0.673 | 0.649 |
| born | gate | 0.627 | 0.791 | 0.811 | 0.799 | 0.759 |
| born | **runner-up + gate** | 0.587 | 0.769 | **0.822** | **0.845** | **0.852** |
| born | **runner-up + gate + top quarter strong** | 0.616 | 0.795 | **0.837** | **0.854** | **0.863** |
| competition on from the first digit | plain | 0.558 | 0.660 | 0.690 | 0.672 | 0.647 |
| competition on from the first digit | gate | 0.603 | 0.789 | 0.811 | 0.798 | 0.708 |
| competition on from the first digit | **runner-up + gate** | 0.582 | 0.757 | **0.816** | **0.837** | **0.844** |
| competition on from the first digit | runner-up + gate + top quarter strong | COMPSTRONG |
| lived, then frozen | plain | 0.568 | 0.669 | 0.693 | 0.679 | 0.653 |
| lived, then frozen | runner-up + gate | 0.554 | 0.758 | 0.818 | 0.837 | 0.845 |

(every sheet has `--g.tile_winners 4`; competition is `--g.compete_on 1 --g.compete_shift 5`;
top quarter strong is `--g.readout_evidence_strong 2`.)

- Every seed is above the gate at 60,000 and 120,000 (runner-up 0.847 to 0.856 at 120,000;
  gate 0.731 to 0.774), and above plain everywhere past 1,000 digits.
- **No drift**: still rising at 120,000 on both sheets; no button goes quiet on any seed
  (plain: 1 or 2 quiet buttons from the start on 7 of 8 seeds; the gate with competition:
  one on one seed by 120,000; `scripts/outgap_quiet.py`).
- It learns more slowly at first (0.769 against 0.791 at 5,000) and passes the gate by
  20,000.
- 42% of goods count as close (0.40 to 0.44 on every seed), so 58% of goods move nothing.
- Without the gate the runner-up rule only stops the drift (seed 5, 2,000 test digits:
  0.700, 0.701, 0.706 at 20,000, 60,000, 120,000; plain on that seed 0.739, 0.713, 0.690).
  The two parts do two jobs: the gate stops the winners getting richer, the runner-up rule
  stops the other buttons draining.
- **The lived sheet still does not help the output.** With competition on from the first
  digit the fixed output reads 0.844, a hair under the born sheet's 0.852; a fresh linear
  reader gets 0.952 from such a sheet and 0.926 from the born one. On the frozen lived sheet
  it reads 0.818, 0.837, 0.845 (born 0.822, 0.845, 0.852). The ladder (above) says where the
  lead is lost.

**A weighted vote on top (idea 147).** `readout_evidence_strong 2` wires the top quarter of
each motor cell's connections (by counter) at strength 4 and the rest at 1, so a cell's
vote weighs its best evidence four times. With the runner-up rule and gate: 0.837, 0.854,
0.863, above the runner-up rule alone on every one of the 8 seeds at 20,000 (0.832 to 0.842
against 0.818 to 0.827) and at 120,000 (0.856 to 0.868 against 0.847 to 0.856), and quicker
early (0.795 at 5,000, level with the gate alone). Free per tick: the w1 plane is read
already. `scripts/outgap.sh online rivalstrong --g.readout_good 2 --g.margin_percent 20
--g.margin_win 2 --g.readout_evidence_strong 2` (run on stevejobs; byte-identical there).

What else was tried in the engine, seed 5, 2,000 test digits (the gate alone on that seed,
all 10,000 test digits: 0.801, 0.785, 0.747 at 20,000, 60,000, 120,000; runner-up + gate:
0.815, 0.836, 0.851; `out/output-gap/quick/`):

| change | 20,000 | 60,000 | 120,000 | verdict |
|---|---|---|---|---|
| opponent cells, plain good half (`--g.readout_opponent 1`) | 0.698 | 0.687 | | worse than plain (0.739); a button dies |
| opponent cells + gate | 0.739 | 0.717 | | worse than the gate |
| opponent cells + runner-up + gate | 0.761 | 0.786 | | worse than runner-up alone |
| runner-up + gate + halve a full counter row (`--g.readout_halve 1`) | 0.815 | 0.832 | 0.834 | worse at 120,000: the clamp is not the drift |
| runner-up + gate + twice the budget (`--g.readout_keep_mul 2`) | 0.827 | 0.837 | 0.849 | the same |
| runner-up + gate + top quarter strong (`--g.readout_evidence_strong 2`, idea 147) | 0.827 | 0.836 | 0.858 | kept: +0.01 on 8 seeds (above) |
| runner-up, no gate | 0.700 | 0.701 | 0.706 | stops the drift only |
| runner-up + gate + centred steps (`--g.readout_center 1`) | 0.788 | 0.824 | 0.841 | worse |
| the same on the frozen lived sheet, against it without centring | 0.796 / 0.819 | 0.846 / 0.834 | 0.850 / 0.851 | no help |

The rungs this leaves: under the runner-up rule the engine (0.822 at 20,000; 0.837 with the
weighted vote) is 0.04 (0.027) under the integer perceptron told the answer (rung 3, 0.864)
and 0.10 (0.09) under the offline reader. At 120,000 it is 0.028 (0.017) under rung 3
(0.880). The rest is online learning with integer steps (rungs 1 to 3, 0.06) and the 0.05
between a free reader and the motor cells' vote (4r 0.869 against 7 0.822). The sign rung
(no evidence against) is not fixed: opponent cells cost in the engine, and I do not know
why the copy says 0.17 and the engine does not pay it; the inhibitory sources the engine
wires may already carry most of it.

## Toy rows

The working setting (`--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4
--g.readout_balance 1`, 40,000 steps of 20 ticks, 3x3 groups, 128 motor cells a button, 256
over all for 8 and 16 cues), and the same with the gate alone and with the runner-up rule
and the gate. Late score (mean of the last 4 of 40 windows), seeds 5-12; the learning-off
twin in brackets. `scripts/outgap.sh toys NAME FLAGS`.

| | echo k=2 | k=4 | k=8 | k=16 | both-not-either | order |
|---|---|---|---|---|---|---|
| working | 1.000 (0.519) | 0.997 (0.247) | 0.842 (0.116) | 0.305 (0.047) | **0.883** (0.489) | 0.695 (0.480) |
| gate (`--g.readout_good 1 --g.margin_percent 20 --g.margin_win 2`) | 1.000 | 0.898 | 0.866 | 0.609 | 0.793 | 0.779 |
| **runner-up + gate** (`--g.readout_good 2 --g.margin_percent 20 --g.margin_win 2`) | 1.000 | **0.993** | **0.940** | 0.605 | 0.793 | **0.779** |
| runner-up + gate + top quarter strong (`--g.readout_evidence_strong 2`) | 1.000 | 0.995 | 0.957 | **0.723** | 0.725 | 0.767 |

- With two buttons the runner-up is the other button, so the two rules are the same rule:
  the lives on echo k=2, both-not-either and order are identical to the gate's.
- Where there are more buttons the runner-up part is what keeps the small rows: k=4 0.898
  with the gate alone (one or two seeds fall), 0.993 with the runner-up rule; k=8 0.866
  against 0.940. Sixteen cues: both double the working setting (0.305 to 0.61).
- **The cost is exclusive-or**: 0.883 to 0.793, the gate's known cost (the tags job had
  0.80). Order gains 0.08. Moving on every good (the working setting has
  `readout_good 0`, nothing on good) is what exclusive-or wants and what many buttons cannot
  afford; this setting does not solve both.
- The weighted vote adds on many cues (16 cues 0.723, the best this row has had) and costs
  exclusive-or another 0.07 (0.725). (On these toys the working setting wires everything at
  strength 4, `readout_evidence_strong 1`; the weighted form puts three quarters back at 1.)
- These toys run with `readout_evidence 1` as the working setting does; the MNIST runs use 2.

## Cost

Nothing in the tick changed: the rule runs once per reward, and the vote is read per step.
(The plug's per-tick count gains one test of a flag per button when `readout_opponent` is
off; `blobbench` times `blob_tick` alone, which is untouched, so it was not rerun.) What a
reward costs is where the difference is, and it shrinks: training time for 120,000 digits
(2.4 million ticks) on this PC, `train s` in the run files (seeds 5-12; the runner-up rule
seeds 5-8, since 9-12 ran on stevejobs; this PC is shared and pinned when hot, so rough):

| rule | seconds | per digit | cells moved on a good |
|---|---|---|---|
| plain | 377 to 389 | 3.2 ms | all 1,020 (pressed up, others down), every good |
| gate | 234 to 255 | 2.0 ms | all 1,020, on a close good (42% of goods) |
| runner-up + gate | 139 to 141 | 1.2 ms | 204 (two buttons), on a close good |

About 0.4 ms of each digit is the 20 ticks themselves; the rest is the readout's update (a
chance word per slot per tick of the window, then a rewire over 704 counters, for every
cell moved). In the hot loop's terms: the runner-up rule does the same AND and popcount
work per tick and a fifth of the counter work per good. `readout_evidence_strong 2` uses
the w1 plane, which the hot loop already reads, so it is free per tick too; its rewire adds
one pass over the wired counters.

## Reference lives

The four merge-reference lives (`out/merge-ref/`: echo; both-not-either with the readout;
the minimal engine; table neurons) are byte-identical to the ones made before this job: on
the branch after the last code change (`out/output-gap/refcheck-branch.txt`) and again after
merging main and rebuilding (`out/output-gap/refcheck-merged.txt`). The check is
`out/output-gap/refcheck.sh` (the compete job's checker, one life at a time): `cmp` on the
four csvs and on the -sum files without their speed line. The runner-up rule also gives the
same life before and after the merge (seed 5, 0.5930 and 0.7535 at 1,000 and 5,000).

## Unfinished

- **Why the real motor cell beats my copy under the gate** (0.811 against 0.721; 0.822
  against 0.754 with the runner-up rule), while they agree on the plain rule. Candidates
  named above; none tested. Until then rung 6 is trustworthy for the plain rule and only
  for the ordering of rules under the gate.
- **The sign rung is not fixed.** Opponent cells cost in the engine (one seed, three forms).
  Not tried: opponent cells with their own threshold regulation per half, or a second
  "against" counter per source.
- **The lived sheet's lead does not reach any integer reader or the engine.** Centring the
  counts gives about 0.01 back to the integer readers (2 seeds) and nothing in the engine's
  crude form (`readout_center`, one seed). The lived sheet fires about twice as often as the
  born one; a reader that scales each source by its own spread is the float readers' one
  advantage not yet tried in bits.
- The runner-up rule has been run to 120,000 digits only; the drift it removes could come
  back later.
- **Exclusive-or** pays 0.09 for the gate. Not addressed.
- The copy's rung 5v/6 stand-in for threshold regulation (mean plus z spreads over 128
  digits) is mine, not the engine's; it is why 5v helps the plain rule, and it may be part of
  why rung 6 differs under the gate.
- The fixed rule was not run on the calculation battery, the critic's worlds or the rest of
  the suite.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, "Where it stands" or the MNIST headline: the output's gap is mostly
  its good half. A ladder of readers on the recorded sheet (offline 0.926, online 0.905,
  integer perceptron told the answer 0.864, the engine's good-and-bad rule 0.58, the motor
  cells with their real fields and one-bit counters 0.69, the engine 0.69; seeds 5-12, 20,000
  digits) has one large step, 0.28, at the rule's good half: moving on every good (the motor
  cells' own thresholds then win back 0.1 of it). With
  `--g.readout_good 2 --g.margin_percent 20 --g.margin_win 2` the online readout reads 0.822,
  0.845, 0.852 at 20,000, 60,000, 120,000 digits (was 0.692, 0.673, 0.649; the gate alone
  0.811, 0.799, 0.759), no drift, no quiet buttons, and costs less per digit.
- New principle (likely; MNIST 8 seeds and the toy rows): **with many buttons, a good should
  move only the competitors.** On a close good the pressed button gains and the runner-up
  loses; the others do not move. Pushing every other button down on every good drains the
  ones that are seldom pressed (with accuracy over one half goods outnumber bads), and moving
  on comfortable goods makes winners richer. This restates principles 17 and 39 for many
  buttons: 17 (never strengthen on good) was right for 2 buttons and wrong for 10 (the MNIST
  job); 39 (only when the vote was close) is half of it; the other half is who pays.
- Ideas 146 and 157 (the drift): answered for MNIST. The drift is the good half pushing the
  nine others down; the runner-up rule removes it to 120,000 digits on every seed. Not the
  counter clamp (halving full rows made it worse), not the budget.
- Idea 147 (weighted vote): built as `readout_evidence_strong 2` (top quarter at 4). On top
  of the runner-up rule, MNIST +0.015 at 20,000 and +0.011 at 120,000 on every seed (0.837,
  0.863); 16 cues 0.605 to 0.723; exclusive-or 0.793 to 0.725. A trade, not a free gain.
- Idea 161: the ladder is the answer to "where the 0.2 goes" (this doc). Two rungs are not
  closed: the sign (a linear reader with no weight below zero loses 0.17 under the gate;
  opponent cells did not get it back in the engine), and the 0.05 between a free reader and
  the motor cells' vote under the gate.
- Principle 41 and the compete result: the output still does not use the lived sheet's lead
  (competition on from the first digit, fixed output: 0.844 against 0.852 born). The next
  question for the output is that one: the lead survives float readers (online 0.925
  against 0.905) and is lost at the first integer one; scaling each source by its own spread
  is the next thing to try in bits.
- `scripts/mnist.sh online`: consider the runner-up + gate rule as its rule (it would change
  that script's numbers; left as is here).
- Toy rows: with the fix, 16 cues 0.305 to 0.605 and 8 cues 0.842 to 0.940, order 0.695 to
  0.779, exclusive-or 0.883 to 0.793 (the gate's known cost: with two buttons the runner-up
  rule is the gate: the lives are identical). With the gate alone k=4 falls to 0.898 and
  k=8 is 0.866: the runner-up part is what keeps rows with more than two buttons.
- `docs/design.md` and `genome.hpp` (core owner): `readout_good 2`, `readout_opponent`,
  `readout_halve`, `readout_keep_mul`, `readout_evidence_strong 2`; `blob/opponent.hpp`; the
  plug's signed vote (`plug.signed_vote`, `motor_gross`, `last_gross`) in `src/world/`.
- `docs/notes-world.md`: `blobmnist ladder`, `blobmnist online --life N --life-freeze 1`,
  `scripts/outgap.sh`.
- `docs/predictions.md`: the four predictions at the top and how they scored.
- `docs/plan.md`: item 2c done. Next for the output: the runner-up rule on the critic (the
  actor's good half), on the calculation battery's many outputs (principle 42: one verdict
  teaching many outputs is the same question of who pays), and on long lives past 120,000.

## Script lines

Each writes `out/output-gap/KIND-NAME/`; a finished seed is not run again; `--seeds 5-12`
and `--jobs 3` are the defaults.

```
scripts/outgap.sh ladder born                                  the ladder, born sheet
scripts/outgap.sh ladder lived --life 60000 --g.compete_on 1 --g.compete_shift 5
scripts/outgap.sh ladder born --seeds 1 --rungs 6b,6g,6r,6go,6h,6s   one seed, chosen readers
scripts/outgap.sh online born                                  rung 7, the plain rule
scripts/outgap.sh online gate --g.margin_percent 20 --g.margin_win 2
scripts/outgap.sh online rival --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2
scripts/outgap.sh online compete --g.compete_on 1 --g.compete_shift 5
scripts/outgap.sh online competegate --g.compete_on 1 --g.compete_shift 5 --g.margin_percent 20 --g.margin_win 2
scripts/outgap.sh online competerival --g.compete_on 1 --g.compete_shift 5 --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2
scripts/outgap.sh online lived --life 60000 --life-freeze 1 --g.compete_on 1 --g.compete_shift 5
scripts/outgap.sh online livedrival --life 60000 --life-freeze 1 --g.compete_on 1 --g.compete_shift 5 --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2
scripts/outgap.sh toys working
scripts/outgap.sh toys gate --g.readout_good 1 --g.margin_percent 20 --g.margin_win 2
scripts/outgap.sh toys rival --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2
scripts/outgap.sh online rivalstrong --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2 --g.readout_evidence_strong 2
scripts/outgap.sh online competerivalstrong --g.compete_on 1 --g.compete_shift 5 --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2 --g.readout_evidence_strong 2
scripts/outgap.sh toys rivalstrong --g.readout_good 2 --g.margin_percent 20 --g.margin_win 2 --g.readout_evidence_strong 2
scripts/outgap.sh table                                        every finished run
python3 scripts/outgap_table.py out/output-gap --doc           the ladder table above
python3 scripts/outgap_quiet.py out/output-gap/online-gate     buttons gone quiet, per seed
```

One ladder is about 15 minutes (most of it rung 6, which rewires all 1,020 cells every
digit, as the engine does); one online run about 7 minutes (12 with a life).
The one-seed engine screens: `out/output-gap/quick.sh NAME FLAGS` (seed 5, 2,000 test
digits; not a script for the owner, it is `scripts/outgap.sh online` with fewer test
digits).
