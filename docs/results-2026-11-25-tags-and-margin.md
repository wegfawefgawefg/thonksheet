# Barely correct, and action-feedback tags (2026-11-25)

Two mechanisms from the catalogue's "inside learning" list, on the best readout (learn mode 5,
evidence counters, balance, 128 motor cells per button):

1. **Barely correct** (entry 15, Baldassi et al. 2007; entry 16, Brader, Senn and Fusi 2007):
   the readout's missing case. On good, when the vote was close, move.
2. **Action-feedback tags** (entry 98, AGREL / AuGMEnT / BrainProp): the pressed button's
   cells send feedback back along their own wiring, the inner neurons that fed the answer are
   tagged, and the verdict moves the tagged neurons' hidden counters.

<!-- SHORT -->

## What was built

### The vote, and the margin rule (`src/blob/learn_margin.cpp`, `src/world/plug_vote.cpp`)

- The world already marks the pressed button's cells (`motor_acted`). It now also writes the
  step's vote: the pressed button's spikes, the runner-up's, and which cells are the
  runner-up's (`plug_mark_vote`, one line beside `plug_mark_acted` in `src/run/life.cpp` and
  in `src/mnist/mnist_sheet.cpp`; `plug_choice` keeps the last counts). Nothing reads them
  unless `margin_percent` is set.
- `margin_percent P`: a good is **thin** when the pressed button's lead over the runner-up
  is under P percent of the pressed button's spikes.
- `margin_win 1` (Baldassi's rule R2): on a thin good, each pressed cell's *wired, active,
  excitatory* sources gain one counter step (chance 2^-readout_shift per tick a source
  fired). They are already wired, so nothing visible changes; they are only held deeper.
- `margin_rival 1`: the runner-up's cells push their *unwired*, active, excitatory sources
  down (the 0/1-input form of R2, hidden only); `margin_rival 2`: the runner-up loses its
  active inputs as on bad, and rewires.
- `margin_win 2` (the stop-learning gate): with `readout_good 1` (on good the pressed button
  gains and the others lose, as the MNIST job uses), that move runs only on a thin good; a
  comfortable good moves nothing.
- Hook: `tick.cpp`, beside `learn_readout`. Off (`margin_percent 0`) is byte-identical.

### Tags (`src/blob/learn_tags.cpp`, `src/blob/tag_store.cpp`)

- **Feedback.** At a verdict, every cell of the pressed button sends feedback back along its
  wired connections to the sources that were active in the step (the readout's 15-tick
  window). Each edge adds the sender's sign to the source's feedback, flipped when the
  source is inhibitory (it pushed against the answer). This is BrainProp's
  phi_m = sum_n w_nm phi_n g_m with w replaced by the sign of a wired edge and g by "fired
  in the window". `tag_mode 2` ("contrast") also lets the other buttons' cells send the
  opposite sign, weighted so all buttons count the same. A neuron whose summed feedback is
  at least `tag_min` gets a plus tag, at most -`tag_min` a minus tag: one bit each.
- **Second hop** (`tag_hops 2`): each tagged neuron sends its own sign one step further back,
  each edge with chance 2^-`tag_thin` (1/2).
- **The verdict.** Against a running mean of rewards (x256, over about 32 verdicts, as the
  noise rule): how far the verdict is from expected is the chance the event moves anything at
  all (so goods and bads balance in expectation). Better than expected: a plus-tagged neuron
  moves its active inputs' counters toward firing (excitatory up, inhibitory down), a minus-
  tagged one away; worse does the reverse. Chance 2^-`tag_shift` per active source.
- **Counters and wiring.** Every neuron gets the readout's store: a signed 6-bit counter per
  possible source (11 slots x 64, one byte each), made on first use from its born strengths.
  After a move it wires the top counters at exactly its born count of excitatory sources,
  its born count of inhibitory sources, and as many of each strong (strength 4) as it was
  born with; ties keep what is wired, then draw by chance. (A first version kept one count
  for both kinds: tagged neurons swapped inhibitory inputs for excitatory ones and the sheet
  ran at 1.8 times its target rate with 15% of thresholds at the cap; a second version let
  the count fall when counters went to zero and lost 4 connections per neuron in a life.)
- **What is never tagged**: sensor cells (the world drives them) and the motor lattice (the
  readout owns it). `tag_inh 0` also leaves inhibitory neurons out entirely.
- **How long a tag lasts**: tags are computed at the verdict from the spike ring; in every
  world here the verdict comes on the tick after the decision, so "tag at the decision, use
  at the verdict" and "compute at the verdict" are the same thing. A tag is kept until the
  next verdict (for the viewer) and never used twice.
- **Far slots**: feedback runs back along far slots too (`tag_far 1`, kept). Without it
  nothing changed beyond noise in the first tuning round (xor 0.947 against 0.944, echo at 3
  tiles 0.749 against 0.753), so it stayed as the more complete form.
- **Inhibitory neurons**: in the chosen setting (`tag_inh 0`) they are never tagged and never
  rewired, and feedback does not pass through them. With them tagged (opposite sign) and
  learning, order fell to 0.62 and k=8 to 0.92 on seeds 1-4 against 0.71 and 0.95 without.
- `tag_verdict 1` (added late, a diagnostic beside the reverse replay): an error rule in
  place of the running mean: every bad moves, no good does.
- Hook: `tick.cpp`, before `learn_readout` (the feedback runs along the wiring that made the
  answer). Off (`tag_mode 0`) is byte-identical.

### Probes (`src/probe/lived_sheet.cpp`)

- `blobprobe transfer --life "<blobrun flags>" [--born 1] --then TASK`: lives the life,
  freezes the inside (mode 5, no tags, no margin), puts every motor cell's wiring back as
  born, wipes the readout's counters, and lets that fresh readout learn TASK. `--born 1`
  skips the life.
- `blobprobe feature ... --life "<blobrun flags>"`: the feature probe on the lived sheet.

Off is byte-identical: five lives (modes 0, 1, 5, 6), csv and summary equal to the binary
before (`out/tags/ident.sh`), and the feature probe as born identical; checked again after
merging main.

## Choosing the settings (seeds 1-4)

`out/tags/tune.sh tags|tags2|margin|margin2` (sweep tables in `out/tags/*tune*.log`). Every
life is 40,000 steps, 20 ticks a step, 3x3 cue groups, 128 motor cells a button (256 over
all buttons for 8 and 16 cues); late = mean of the last 4 of 40 windows.

**Tags, round 1** (feedback from the chosen button only unless "contrast"; inhibitory neurons
tagged; `tag_min 1`, `tag_shift 4`):

| variant | xor | order | echo, 3 tiles | echo k=8 |
|---|---|---|---|---|
| readout (the thing to beat) | 0.892 | 0.705 | 0.857 | 0.855 |
| tags | 0.944 | 0.513 | 0.753 | 0.621 |
| tags, contrast | 0.990 | 0.618 | 0.794 | 0.919 |
| tags, 2 hops | 0.884 | 0.491 | 0.715 | 0.458 |
| tags, contrast, 2 hops | 0.985 | 0.484 | 0.775 | 0.891 |
| tags, slow (2^-6) | 0.968 | 0.563 | 0.774 | 0.731 |
| tags, no inhibitory | 0.971 | 0.651 | 0.804 | 0.729 |
| tags, no far slots | 0.947 | 0.558 | 0.749 | 0.582 |
| tags, min 4 | 0.973 | 0.585 | 0.826 | 0.910 |
| mode 6 (the old inner rule) | 0.557 | 0.552 | 0.962 | 0.672 |

**Tags, round 2** (all contrast):

| variant | xor | order | echo, 3 tiles | echo k=8 |
|---|---|---|---|---|
| contrast | 0.990 | 0.618 | 0.794 | 0.919 |
| contrast, no inhibitory | 0.995 | 0.713 | 0.812 | 0.952 |
| contrast, no inhibitory, slow | 0.993 | 0.719 | 0.783 | 0.961 |
| **contrast, no inhibitory, min 4** | **0.996** | **0.711** | **0.858** | **0.952** |
| contrast, no inhibitory, slow, min 4 | 0.990 | 0.691 | 0.835 | 0.955 |
| contrast, no inhibitory, very slow (2^-8) | 0.958 | 0.688 | 0.799 | 0.961 |

Chosen: `--g.tag_mode 2 --g.tag_inh 0 --g.tag_min 4` (shift 4, one hop, far slots on).
What the rounds say: feedback from the chosen button alone is not enough (it hurts order
0.19 and k=8 0.23); the contrast with the other buttons (the readout's own finding 15, "the
output must know which action was taken", pushed one step inward) is what makes tags help.
Inhibitory neurons learning is what cost order. A second hop never helped. Requiring four
chosen-minus-other edges before a neuron is tagged spares reach.

**Margin, round 1** (on good, Baldassi's deepening; echo k=8, seeds 1-4): margin 5% 0.879,
10% 0.884, 20% 0.857, 40% 0.794 against 0.855; the runner-up pushing its unwired sources
down (`margin_rival 1`) 0.866, its active inputs down visibly (`margin_rival 2`) 0.733, the
runner-up alone 0.804. The runner-up half never helped, so it is not reported further.

**Margin, round 2**: never, always, or only-when-thin moves on good, with and without the
counter fix (`readout_evidence 2`):

| variant | xor | order | echo k=8 | echo k=16 |
|---|---|---|---|---|
| readout | 0.892 | 0.705 | 0.855 | 0.322 |
| readout, counter fix | 0.873 | 0.697 | 0.856 | 0.318 |
| fix, good always (`readout_good 1`) | 0.526 | 0.536 | 0.567 | 0.472 |
| fix, deepen 10% | 0.876 | 0.525 | 0.888 | 0.381 |
| fix, gate 10% | 0.797 | 0.740 | 0.940 | 0.541 |
| **fix, gate 20%** | 0.786 | **0.788** | 0.884 | **0.635** |
| deepen 10% (no fix) | 0.853 | 0.525 | 0.884 | 0.375 |
| gate 20% (no fix) | 0.761 | 0.774 | 0.882 | 0.649 |

And on MNIST (`blobmnist online`, the MNIST job's sheet and rule, validation on the last
10,000 training digits, seed 1, at 20,000 digits): never on good 0.204; always 0.719 (the
MNIST job's setting); deepen 10/20/40% (never otherwise) 0.42, 0.41, 0.43; **gate 5/10/20/
40/80%: 0.694, 0.808, 0.816, 0.741, 0.719** (at 80% every good is thin, so it equals always).

Chosen: the gate at 20% with the counter fix (`--g.readout_evidence 2 --g.readout_good 1
--g.margin_percent 20 --g.margin_win 2`), best on MNIST, order and 16 cues; 10% was better
on 8 cues. The deepening rule (Baldassi's R2 as asked: `--g.margin_percent 10`, nothing else
on good) is reported beside it. The counter fix changes nothing at 8 or 16 cues here
(0.856 against 0.855, 0.318 against 0.322): with 32 or 16 cells per button no cell has more
counters at the clamp than its budget. The margin rule's gain does not depend on it (gate
20% 0.649 without, 0.635 with).

## The report (seeds 5-12)

`ROWS=... scripts/tags.sh tables` (sweep tables in `out/tags/report-here.log` and, for 16
cues, stevejobs' `out/report-many.log`). "readout" and "mode 6" rows are the readout job's
own runs, imported into this sweep's cache after two of them were rerun here and matched
byte for byte. "replay" is the reverse-replay job's report (docs/results-2026-11-25-
reverse-replay.md), the same seeds, the same lives and the same readout underneath (its
frozen column equals this one to the third decimal on every row); quoted, not rerun.

| task | readout | mode 6 | **tags** | gate | tags + gate | deepen | always on good | replay d2 / d1 | twin |
|---|---|---|---|---|---|---|---|---|---|
| exclusive-or | 0.883 | 0.627 | **0.996** (+0.113, t 8.2) | 0.804 | 0.992 | 0.828 | | 0.985 / 0.985 | 0.489 |
| order | 0.695 | 0.539 | 0.657 (-0.037, t -2.1) | **0.775** (+0.080, t 7.5) | 0.722 | 0.521 | | 0.664 / 0.673 | 0.480 |
| gap | 0.519 | 0.495 | 0.505 | 0.520 | 0.534 | 0.474 | | 0.491 | 0.488 |
| echo, 2 tiles | 0.905 | **0.993** | 0.794 (-0.112, t -6.5) | 0.873 | 0.751 | 0.911 | | 0.945 | 0.483 |
| echo, 3 tiles | 0.853 | **0.967** | 0.869 (+0.016, t 1.0) | 0.811 | 0.837 | 0.832 | | 0.928 / 0.943 | 0.476 |
| echo, 5 tiles | 0.587 | **0.871** | 0.781 (+0.194, t 13.3) | 0.598 | 0.767 | 0.569 | | 0.802 / 0.876 | 0.473 |
| echo, 8 cues | 0.842 | | **0.953** (+0.111, t 9.9) | 0.851 | 0.787 | 0.879 | 0.552 | 0.924 | 0.116 |
| echo, 16 cues | 0.305 | | 0.416 (+0.111, t 9.0) | 0.604 (+0.299) | **0.631** (+0.325) | 0.359 | 0.438 | 0.302 | 0.047 |
| shuffled echo, 16 lives | 1.000 | | 1.000 | 1.000 | 0.821 | 1.000 | 0.708 | 1.000 | 0.518 |

Spread over 8 seeds: 0.002 to 0.07; the t columns are paired against the readout, seed by
seed. Full tables with per-seed values: `out/tags/report-here.log`.

### Retention: echo A, then echo B on new cues and buttons, then A again

`out/tags/protocol.py` over `blobrun --task echo --retention echo --steps 40000 --window 200
--retest 10000` (the suite's row), seeds 5-12, 128 cells a button:

| variant | A at its end | B at its end | A retested, first window | A retested, end |
|---|---|---|---|---|
| readout | 1.000 | 0.994 | 0.983 | 1.000 |
| mode 6 | 1.000 | 0.979 | 0.978 | 1.000 |
| tags | 1.000 | 0.979 | 0.959 (±0.013) | 1.000 |
| gate | 1.000 | 0.952 | 0.887 (±0.035) | 1.000 |
| tags + gate | 0.824 | 0.802 | 0.988 | 0.997 |
| deepen | 1.000 | 0.992 | 0.989 | 1.000 |

A second task after the first is learned by all; tags keep a little less of A (0.96 against
0.98 at once), the gate less again (0.89); everything is back at 1.00 by the end of the
retest.

## Barely correct: what it does

**Pairs held** (late score against the line halfway from chance to perfect: 0.56 for 8
cues, 0.53 for 16):

| | 8 cues | 16 cues | held |
|---|---|---|---|
| readout | 0.842 | 0.305 | 8 |
| deepen (Baldassi's rule as asked) | 0.879 (+0.037, t 7.0) | 0.359 (+0.054, t 9.3) | 8 |
| always on good | 0.552 | 0.438 | 4 (8 falls under the line) |
| **gate** (good moves only on a thin vote) | 0.851 (peak 0.91) | **0.604** (+0.299, t 26) | **16** |
| tags | **0.953** | 0.416 | 8 |
| tags + gate | 0.787 (peak 0.89) | **0.631** | 16 |

32 and 64 cues could not be run: the world caps a task at 16 buttons (`TASK_MAX_MOTORS`,
src/world/task.hpp; `--k 32` is quietly clamped to 16, which the first k=32 sweep showed by
giving the k=16 numbers to the last digit), and the sheet's 256 motor cells would leave 8 and
4 per button. Raising the cap also changes the reference networks' output size
(`NN_OUT = TASK_MAX_MOTORS` in src/nn/nn.hpp), so it is left to the world owner.

- **Deepening alone (the brief's mechanism) is a small gain where there are many buttons and
  a large loss on order.** Pairs: +0.04 at 8, +0.05 at 16. But order falls from 0.695 to
  0.521 (every seed, t -18.7), exclusive-or 0.883 to 0.828, gap and reach a little. Deepening
  the sources that carried a thin win holds whatever won, including what won by chance; on a
  two-button task with a weak feature (order) that is enough to stop learning (late minus
  early -0.016).
- **The stop-learning gate is the form that matters.** "Good moves too" was the MNIST job's
  finding for ten buttons; here, "good moves, but only when the vote was close":
  - 16 cues: 0.305 to 0.604 (every seed +0.21 to +0.31). Always moving on good gets 0.438.
  - order: 0.695 to 0.775 (t 7.5): the best order score without long delays so far.
  - 8 cues: same late score, but reached much sooner (0.8 at 2,000 steps against 17,375) and
    then drifting down (peak 0.91, late 0.85; one seed falls to 0.65).
  - exclusive-or 0.883 to 0.804 and echo at 3 tiles 0.853 to 0.811: worse.
  - always moving on good is far worse than either on two to eight buttons (0.53 to 0.71).
- **The lock does not come back** with either form: shuffled echo is 1.00 on all 16 lives
  (9 crossed: seeds 6, 7, 11, 13, 14, 16, 17, 19, 20) with deepening and with the gate.
  Always moving on good settles every life, crossed or straight, at 0.70 (not a lock on one
  button: that would be 0.5), so finding 17 holds for unconditional strengthening.
- **Tries to learn** (mean steps until a window first reaches the level; "(n+)": n lives
  never did, counted as 40,000):

  | 8 cues | to 0.5 | to 0.7 | to 0.8 | to 0.9 |
  |---|---|---|---|---|
  | readout | 2,000 | 6,625 | 17,375 | 39,875 (7+) |
  | deepen | 1,625 | 4,000 | 9,750 | 32,375 (4+) |
  | gate | 1,000 | 1,625 | 2,000 | 4,750 |
  | tags | 1,625 | 2,750 | 4,000 | 8,125 |
  | tags + gate | 1,000 | 1,625 | 2,000 | 11,500 (2+) |
  | always on good | 1,875 | (8+) | (8+) | (8+) |

  | exclusive-or | to 0.7 | to 0.8 | to 0.9 |
  |---|---|---|---|
  | readout | 2,875 | 7,250 | 26,000 (1+) |
  | tags | 2,375 | 3,000 | 4,500 |
  | gate | 3,000 | 10,125 (1+) | 37,625 (7+) |
  | mode 6 | 19,625 (3+) | 37,500 (7+) | (8+) |

### MNIST, and the drift (the MNIST job's question)

`out/tags/mnist.sh` (the MNIST job's sheet and rule; only the good half changes), seed 1, test
digits (all 10,000) at each check, three passes:

| digits seen | always on good (MNIST job) | gate 20% | gate 10% |
|---|---|---|---|
| 1,000 | 0.599 | 0.639 | 0.533 |
| 5,000 | 0.706 | **0.798** | 0.756 |
| 20,000 | 0.710 | **0.808** | 0.799 |
| 60,000 | 0.687 | 0.784 | 0.775 |
| 120,000 | 0.640 | 0.713 | 0.658 |
| 180,000 | 0.635 | 0.699 | 0.587 |

("always" is the MNIST job's table; rerun here to 20,000 digits it matched to four
decimals.) The gate lifts the online readout by ten points at its peak (0.71 to 0.81, the
linear reader of the same sheet gets 0.92), and learns faster (0.80 at 5,000 digits). **It
does not stop the drift.** From the peak to three passes it loses 11 points (always: 7.5;
gate 10%: 21). The drift is buttons dying: at 180,000 digits the gate-10% life presses 4 and
8 on 69 and 72 of 10,000 test digits (1,000 each would be right); the gate-20% life presses
2, 4 and 8 on 521, 369 and 469. A comfortable good already moves nothing under the gate, so
whatever wears those buttons down happens on thin goods and on bads. The same early peak and
slow fall shows on 8 cues (peak 0.91, late 0.85) and, with tags, on shuffled echo. So "stop
learning when the margin is comfortable" is not the fix for the drift; something that keeps
every button's counters from running down (a floor per button, or balance by how often a
button is right) is the next guess.

## Tags: what they do

- **Exclusive-or: 0.883 to 0.996**, every seed 0.99 or 1.00, above the linear reader of the
  born sheet (0.97) and above the backward walk (0.985). It is learned five times sooner
  (0.9 at 4,500 steps against 26,000).
- **Eight cues 0.842 to 0.953**, sixteen 0.305 to 0.416, echo at 5 tiles 0.587 to 0.781.
- **Order -0.037 (t -2.1, borderline), echo at 2 tiles -0.112, gap and echo at 3 tiles
  unchanged.** The 2-tile loss is real (every seed lower) and odd beside the 3- and 5-tile
  results; tuning was on 3 tiles, where the chosen setting was neutral.
- **Against mode 6 (the old inner rule):** tags win every feature and capacity row
  (exclusive-or 0.996 against 0.627, order 0.657 against 0.539) and lose every reach row
  (2, 3, 5 tiles: 0.79, 0.87, 0.78 against 0.99, 0.97, 0.87). Mode 6 grows wiring toward what
  fires together, which is what carries a cue across tiles; tags only re-choose among a
  neuron's born possible sources, one hop from the motor cells, and keep its count.
- **No lock**: shuffled echo 1.00 on all 16 lives.

### Did the inside learn? The feature probe on the lived sheet

`blobprobe feature --cue-x 2 --cue-y 3` (the tile where the task's cue lines sit), learning
off, on the sheet as born and after a 40,000-step exclusive-or life with tags; seeds 5-12.
The probe's number per distance from the cue tile (1.0 is easy to read, under 0.3 lost):

| feature | sheet | cue tile | 1 tile | 2 tiles | 3 tiles | 4 tiles |
|---|---|---|---|---|---|---|
| exclusive-or | born | 1.56 | 1.42 | 0.59 | 0.21 | 0.15 |
| exclusive-or | after tags | **2.71** | **2.37** | **1.25** | **0.60** | 0.44 |
| order | born | 2.78 | 2.62 | 0.90 | 0.40 | 0.21 |
| order | after tags (exclusive-or life) | 2.64 | 2.50 | 1.06 | 0.59 | 0.42 |

Every seed's exclusive-or at the cue tile is higher after the life (+1.15 ±0.11). The sheet
now computes the feature it was rewarded for, and carries it two to three times further.
Order, never rewarded in that life, is not damaged near the cue and is carried a little
further. (The probe is an ideal reader per tile; it does not say a readout can use it. The
fresh-readout check below does.)

<!-- FRESH -->

## Both together

Tags plus the gate is not the sum. It keeps tags' exclusive-or (0.992) and the gate's 16
cues (0.631, the best), but on 8 cues, shuffled echo and retention it learns fast and then
falls: on shuffled echo 14 of 16 lives are at 1.00 and then, somewhere between 4,000 and
25,000 steps, collapse to 0.54-0.72 and only partly recover (late 0.821), 8 cues peaks at 0.89 and ends at 0.79, retention's first task
ends at 0.82. Moving on good (even only when thin) plus moving the inside on good (tags,
against the running mean) is two positive loops at once; each alone is stable on those
rows.

<!-- LONG -->

## How close are tags to backprop?

BrainProp (Pozzi, Bohté and Roelfsema 2020, read for this) is a layered rate network. The
network picks one output s; only s sends feedback, through feedback weights equal to the
forward weights: phi_m = sum_k w_km g_k phi_k, with g the slope of the unit's activation
(zero when the unit is off, so feedback does not pass through silent units). The update is
dw_ij = delta * phi_i * g_i * y_j with delta = r - y_s, the reward against the chosen
output's own estimate. For the loss (r - y_s)^2 / 2 that is exactly backprop's update, per
sample, for that one output.

What survives here, piece by piece:

- **Feedback weights = forward weights**: yes, for free. The feedback runs back along the
  very connections that exist, so there is no weight-transport problem; but only the sign of
  a connection is used (excitatory +, inhibitory -), not its strength (1, 4 or 5).
- **g, the slope**: replaced by "fired in the step's window". For a ReLU unit that is the
  same gate (on or off); for a spiking neuron it ignores how close to threshold it was.
- **phi, the amount of feedback**: summed as a count of chosen-minus-other edges, then cut to
  one bit at a line of 4. The magnitude is gone; above the line every tagged neuron gets
  the same step.
- **y_j, the presynaptic activity**: "fired at all in the window", one bit.
- **delta**: one bit (better or worse than a running mean of all rewards), with its size
  turned into the chance that the event moves anything. Expected value right, but the mean
  is global, not the chosen output's estimate for this input (which is what makes BrainProp's
  delta a per-sample error).
- **Contrast (`tag_mode 2`)**: the other buttons also send feedback, with the opposite sign.
  That is not BrainProp; it is closer to backprop of a softmax/error over all outputs (the
  readout's own error rule, one step in), and it is the version that works.
- **Depth**: one hop. BrainProp's equivalence holds at any depth in a feedforward net; here a
  second hop never helped (as with the backward walk, where depth 1 was best).
- **The weight change**: a counter step with chance 2^-4, then a top-k read-off at fixed
  counts. That is not a gradient step; it is a sign-and-rank update with a hard budget.
- **Recurrence and time**: the sheet is recurrent with delays; credit goes to any spike in a
  15-tick window, not along the causal path. BrainProp has no time.

So the honest description: for one hidden layer (the neurons feeding the motor cells), tags
are a one-bit, sign-only, budgeted version of the gradient of the chosen-output error, with
a global baseline. That is close to backprop in its direction for that layer and far from it
in everything that makes backprop a precise optimizer (magnitudes, depth, per-input error,
the chain through time). It is a backward pass of one step, along existing wiring, at
decision time: closer to backprop than anything else in the engine, including the backward
walk, which follows spikes rather than the chosen action's wiring.

## Tags and the backward walk: the same thing?

Both start at the motor cells with the readout's sign (chosen button one way, the others the
other), step one hop back along existing connections, and move the hidden counters of the
neurons reached for their active inputs; both are best at one step; both are reported on
the same lives. They differ in three ways:

| | tags | backward walk (replay) |
|---|---|---|
| what is followed | wired connections from the button's cells to any source active in the step | the exact spikes that fed each motor spike (source fired at age + delay) |
| when | every verdict, against a running mean (good and bad) | bad steps only (the readout's error) |
| who is credited | neurons whose net chosen-minus-other edge count is at least 4; inhibitory left out | every walked spike, with its sign, inhibitory included |
| wiring after | exact born counts of excitatory sources, inhibitory sources, and strong ones of each | the born count of connections (one count for both kinds); strength read from the counter (1, 4 or 5), so drive can grow |

On the same rows: tags are better on exclusive-or (0.996 against 0.985) and eight cues
(0.953 against 0.924) and sixteen (0.416 against 0.302); the walk is better on reach (2 tiles
0.945 against 0.794; 3 tiles 0.943 against 0.869; 5 tiles 0.876 against 0.781); both cost
order a little (0.66 against 0.67) and gap nothing measurable.

To see which difference matters, tags were run as an error rule like the walk's
(`tag_verdict 1`: every bad moves, no good does), everything else unchanged; seeds 5-12, not
tuned (a diagnostic, so it was run straight on the report seeds; `scripts/variants/
tags-error.txt`):

| task | readout | tags | tags, error only | walk d1 (d2) |
|---|---|---|---|---|
| exclusive-or | 0.883 | 0.996 | 0.993 | 0.985 (0.985) |
| order | 0.695 | 0.657 | 0.646 | 0.673 (0.664) |
| echo, 2 tiles | 0.905 | 0.794 | 0.682 | (0.945) |
| echo, 3 tiles | 0.853 | 0.869 | 0.805 | 0.943 (0.928) |
| echo, 5 tiles | 0.587 | 0.781 | **0.851** | 0.876 (0.802) |
| echo, 8 cues | 0.842 | 0.953 | 0.941 | (0.924) |

Making tags an error rule brings 5 tiles to the walk's level (0.85 against 0.88) and changes
nothing on exclusive-or, order or 8 cues, but makes 2 and 3 tiles worse and unstable (late
minus early negative, one 2-tile life at 0.51). So timing of the verdict is not what gives
the walk its reach at 2-3 tiles. The remaining differences are that the walk follows the
exact spikes (source fired one delay before the motor spike), credits inhibitory neurons,
and lets a neuron's drive grow (strength from the counter). My reading: **they are the same
mechanism** (one step of signed credit from the action's cells, inward, into hidden counters,
read off under a budget) **in two dressings**, and each dressing's choices decide where it
wins: the exact-spike walk with growable strength carries a cue across tiles and runs hot
over long lives; the wiring-based tag with fixed counts is calmer and better on the
conjunction and on many cues. Neither helps order or gap. A merged rule would take the
walk's spike-exact credit and the tags' fixed E/I and strong counts.

On the runaway: <!-- RUNAWAY -->
