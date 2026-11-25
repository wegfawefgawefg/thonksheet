# The fixed-network reference (2026-11-25)

The owner: "do we need ground truthing on these tasks? like compare to standard nn ... i
assume we will find a task that the standard nn cant learn but ours can. the thing is, you
often have to change arch to get a nn to learn a task right. which is kind of part of the
problem here. thats what were trying to engineer out."

So this is not the best network per task. It is two ordinary networks held to the rule our
designs are held to (docs/bench.md, "One setting for everything"): one size, one recipe, the
same stream through the same plug, told only good and bad, nothing changed between worlds.

## The answer, short

On today's suite, an ordinary GRU with one fixed setting learns every world any home-grown
engine learns, at the same or a higher score: of 26 comparable rows, 22 are learned by a
reference, 19 by some home-grown engine, none by a home-grown engine alone. It also learns
delayed echo, beat and gap, which none of our engines learns, and echo with 16 cues, order
and catch far better than ours. The plain MLP does everything that needs no memory.

The places where the fixed networks fail are where a per-task change would be the obvious
fix: no memory (MLP: add recurrence), a held-out place (both: add a convolution), a hold
longer than the backprop window (GRU: lengthen the window), a reward that judges the wrong
press (cartpole: fix the world or add a critic). Those are the walls a design of ours could
get past without being redesigned. Today none of our designs gets past any of them.

## What was built

`src/nn/`, run with `blobrun --engine mlp` or `--engine gru`. It stands behind the same
`Blob` shell as the other engines, so the suite, the plug and every task drive it
unchanged. Floats are allowed there (it is a yardstick, not a candidate for the hardware).
No library: about 600 lines of hand-written matrix code, single precision, no fused
multiply-add and a hand-written `exp`, so a seed gives the same csv on both machines
(checked: byte-identical on this PC and stevejobs).

- **What it sees.** Once per task step, the sensor lattice cells (256 of them, the whole
  sensor region) as bits on the step's first tick. The plug holds a step's lines for the
  whole step, so that is all the step shows; the good and bad lines of the last reward are
  on in those ticks too, exactly as the sheets see them. Unattached cells are 0. The input
  layer is the same 256 cells for every world.
- **What it does.** One score per button (16 output units, the most any task has; a task
  uses the first few). It samples a button from a softmax at temperature 1 and fires that
  button's motor cells on every tick of the step, so the plug's count picks it. It reads
  from the plug which cells belong to which button and when a step starts (no motor spike
  counted yet). It always presses: it cannot give "no move", which the sheets can by a tie.
- **What it learns from.** Only the good or bad the world broadcasts after a step, through
  the same `reward()` call the sheets get. REINFORCE: loss -(r - b) log p(a), b a running
  average of r (rate 0.01), one update per reward, credit to the step just judged only (no
  returns, no discount, no critic). Steps the world does not judge teach nothing.
- **mlp**: 256 inputs -> 128 relu -> 16 scores. 34,960 parameters.
- **gru**: 256 inputs -> a GRU of 128 (torch's equations) -> 16 scores, its state carried
  across steps and never reset, backprop through the last 8 steps. 150,288 parameters.
- **Initial weights**: torch's defaults (uniform in +-1/sqrt(fan-in); the GRU's in
  +-1/sqrt(128)).
- **Learning-off twin**: the same network at step size 0.
- **Checked**: finite-difference gradients (MLP worst relative error 1e-4, GRU 0.03 over
  228 sampled parameters, in float).

### The one setting

| number | value | how |
| --- | --- | --- |
| optimizer, step size | Adam (0.9, 0.999, 1e-8), 0.0001 | chosen on seeds 1-4, below |
| softmax temperature | 1 | fixed, not searched |
| baseline rate | 0.01 | fixed, not searched |
| GRU backprop window | 8 steps | given, not searched |
| hidden units | 128 | given |

These are the defaults of the `--n.` numbers in `src/nn/nn.hpp`. Nothing below sets any of
them per task.

Chosen on echo k=2, exclusive-or (both-not-either) and delayed echo (delay 2), seeds 1-4,
40,000-step lives at the suite's plug, both networks. The rule: the highest mean late score
over the six network-by-task cells.

| setting | mlp echo | mlp xor | mlp delayed | gru echo | gru xor | gru delayed | mean |
|---|---|---|---|---|---|---|---|
| **Adam 0.0001** | 1.00 | 1.00 | 0.50 | 1.00 | 0.93 | 1.00 | **0.904** |
| Adam 0.00003 | 1.00 | 0.96 | 0.50 | 1.00 | 0.94 | 1.00 | 0.899 |
| Adam 0.0003 | 1.00 | 1.00 | 0.50 | 1.00 | 0.88 | 1.00 | 0.896 |
| SGD 0.01 | 1.00 | 1.00 | 0.50 | 1.00 | 0.81 | 1.00 | 0.886 |
| SGD 0.03 | 1.00 | 1.00 | 0.50 | 1.00 | 0.75 | 1.00 | 0.876 |
| Adam 0.003 | 1.00 | 1.00 | 0.50 | 1.00 | 0.82 | 0.75 | 0.846 |
| Adam 0.001 | 1.00 | 1.00 | 0.50 | 0.88 | 0.88 | 0.75 | 0.833 |
| SGD 0.1 | 1.00 | 0.57 | 0.50 | 1.00 | 0.76 | 0.74 | 0.762 |
| Adam 0.01 | 0.91 | 0.69 | 0.50 | 1.00 | 0.50 | 0.62 | 0.704 |
| SGD 0.3 | 0.75 | 0.51 | 0.50 | 0.75 | 0.50 | 0.50 | 0.585 |

The top three are within one seed of each other. The MLP's 0.50 on delayed echo is the
expected wall (no memory). The GRU's exclusive-or below 1.00 is one seed in four locked at
0.75: the softmax saturates on a wrong answer for one of the four cases and REINFORCE's
gradient there, proportional to 1 - p, is exactly zero in float. At Adam 0.001 one GRU
Echo life learned 1.00 and then collapsed to 0.50 at step 30,000: Adam keeps taking full
steps when the gradient is tiny. Both are the plain rule's known failures, left in.

## The scorecards

`out/scorecards/mlp-fixed.csv` and `gru-fixed.csv`, seeds 5-12, the suite's standard life
(20 ticks a step, 3x3 groups, 40,000 steps). Late score, mean ± spread over 8 seeds;
learning-off twin in brackets. "told" is the labelled extra (below), not the reference.

| world | chance | mlp-fixed | gru-fixed | mlp told | gru told |
|---|---|---|---|---|---|
| echo k=2 | 0.47 | 1.00 (0.49) | 1.00 (0.49) | 1.00 | 1.00 |
| echo k=4 | 0.23 | 1.00 (0.25) | 1.00 (0.25) | 1.00 | 1.00 |
| echo k=8 | 0.11 | 1.00 (0.12) | 1.00 (0.12) | 1.00 | 1.00 |
| echo k=16 | 0.05 | 1.00 (0.06) | 1.00 (0.06) | 1.00 | 1.00 |
| shuffled echo | 0.48 | 1.00 (0.50) | 1.00 (0.50) | 1.00 | 1.00 |
| delayed echo, 2 steps | 0.48 | 0.50 (0.50) | 1.00 (0.50) | 0.50 | 1.00 |
| beat, every 4 steps | 0.51 | 0.64 ±0.01 (0.50) | 1.00 (0.50) | 0.52 | 0.50 |
| sequence (prediction) | 0.50 | 0.50 | 0.50 | 0.50 | 0.50 |
| catch 4x4 | 0.25 | 0.49 ±0.01 (0.25) | 0.72 ±0.01 (0.26) | 0.50 | 0.73 |
| cartpole, lean (steps per fall) | 23.3 | 17.3 ±7.7 (21.9) | 12.0 ±4.8 (20.9) | 1000 | 1000 |
| cartpole, sparse (steps per fall) | 23.3 | 22.3 ±1.2 (21.6) | 14.2 ±2.2 (21.1) | 758 | 969 |
| echo, cue and button 1/2/3/5 tiles apart | 0.47-0.48 | 1.00 all | 1.00 all | 1.00 all | 1.00 all |
| order | 0.48 | 0.50 (0.50) | 1.00 (0.50) | 0.50 | 1.00 |
| exclusive-or (both-not-either) | 0.48 | 1.00 (0.50) | 0.97 ±0.09 (0.50) | 1.00 | 1.00 |
| gap | 0.47 | 0.49 (0.50) | 1.00 (0.49) | 0.49 | 1.00 |
| place, never-rewarded place | 0.47 | 0.16 ±0.10 (0.50) | 0.16 ±0.12 (0.49) | 0.18 | 0.21 |
| echo A at its end / B at its end | 0.47 / 0.23 | 1.00 / 1.00 | 1.00 / 1.00 | 1.00 / 1.00 | 1.00 / 1.00 |
| echo A retested after B, first window | 0.46 | 1.00 | 1.00 | 1.00 | 1.00 |
| echo before / after unplugged (first window) | 0.47 | 1.00 / 1.00 | 1.00 / 1.00 | 1.00 / 1.00 | 1.00 / 1.00 |

GRU exclusive-or: seven seeds 1.00, seed 5 locked at 0.75. Rows the reference cannot take
part in are left out: reach (cue readable at N tiles), room, aliveness, ticks per second
and bytes per neuron are measurements of a sheet, and the frozen-sheet rows need a readout
mode (`blobrun` refuses `--frozen-readout` for these engines, like for minimal and table).
Sequence is a prediction score with no reward; the reference predicts nothing, so it sits
at 0.50 by construction.

## Tier by tier, against the best home-grown engine

Best home-grown is the best of `blob-main`, `blob-readout` (the evidence readout),
`minimal` and `table` in docs/scoreboard.md. "Learned" means beating both the learning-off
twin and chance by 0.05 and by two standard errors (`scripts/reference.sh compare`).

| tier | world | mlp | gru | best home-grown | ahead | per-task change that would fix the failure |
|---|---|---|---|---|---|---|
| 1 association | echo k=2, 4 | 1.00 | 1.00 | 1.00 readout | even | |
| 1 | echo k=8 | 1.00 | 1.00 | 0.84 readout | reference | |
| 1 | echo k=16 | 1.00 | 1.00 | 0.31 readout | reference | |
| 1 | shuffled echo | 1.00 | 1.00 | 1.00 readout | even | |
| 2 combination | exclusive-or | 1.00 | 0.97 | 0.88 readout (rich2 sheet 0.94, seeds 5-8) | reference | |
| 4 time | delayed echo, 2 steps | 0.50 | 1.00 | 0.55 readout (not learned) | reference | mlp: recurrence |
| 4 | beat | 0.64 | 1.00 | 0.51 (none learned) | reference | mlp: recurrence |
| 4 | order | 0.50 | 1.00 | 0.69 readout (rich2 0.85) | reference | mlp: recurrence |
| 4 | gap | 0.49 | 1.00 | 0.52 (none learned; rich2 0.93, seeds 5-8) | reference | mlp: recurrence |
| 4 | delayed echo, 7+ steps (diagnostic, below) | - | 0.49-0.73 | not run (fails at 2) | neither | gru: a longer backprop window |
| 5 space | place never rewarded | 0.16 | 0.16 | 0.25-0.48 (none learned) | neither | a convolution (shared weights over position) |
| 7 acting | catch 4x4 | 0.49 | 0.72 | 0.32 readout | reference | gru: credit for earlier moves (returns) |
| 7 | cartpole, lean | 17 | 12 | 18 (none learned) | neither | the world: its reward judges the press before (below) |
| 7 | cartpole, sparse | 22 | 14 | 19 (none learned) | neither | returns or a critic (credit 25 steps late) |
| 8 keeping | second task, first kept, idle | 1.00 | 1.00 | 0.98-1.00 readout | even | |
| 10 no teacher | sequence prediction | n/a | n/a | 1.00 minimal, same as its learning-off twin | neither | a prediction objective |
| - reach | echo with cue and button 2, 3, 5 tiles apart | 1.00 | 1.00 | 0.91, 0.85, 0.59 readout | reference (not a fair row) | |

Tiers 3 (magnitude), 6 (sound), 9, 11 and 12 have no world on main yet; tier 0 (staying
alive) is a sheet's own property.

The reach rows are not a fair comparison: the networks' input layer is wired to every
sensor cell, so "distance" does not exist for them. They are listed because the scorecard
has them.

### Where the fixed reference does better than every home-grown engine

Echo with 8 and 16 cues; exclusive-or; delayed echo; beat; order; gap; catch: eight worlds
by a clear margin, with nothing changed between them. Echo with cue and button 2, 3 and 5
tiles apart as well, but by construction. The MLP alone already beats every home-grown
engine on echo k=8 and k=16, exclusive-or, catch and beat.

### Where a home-grown engine does better than the fixed reference

On the late score: nowhere in today's suite. On the held-out place the home-grown engines
sit nearer chance (0.25 to 0.48) than the networks (0.16), but none of them learned it:
that is being less wrong, not right.

On speed of learning: the evidence readout (`blob-readout`) reaches its level on the
association rows within the first 1,000-step window (echo k=2, k=4, shuffled, distance 1
and 2), where the reference takes 2,000 to 3,000 steps (table below). That is the one
measurable edge, and it is partly the reference's small step size (Adam 0.0003 learns
faster and locks more often).

On a sequence prediction row the minimal engine scores 1.00, but so does its learning-off
twin: nothing is learned there, by anything.

### What a per-task redesign would fix

Every failure of the fixed networks has an obvious per-task fix, which is the owner's
point:

- **MLP, every time row**: add recurrence. That is exactly the GRU, which then passes all
  of them.
- **Both, held-out place**: a convolution, which shares what one place learned with every
  other place. Without it both networks invert: 0.16, worse than chance, probably because at
  a never-rewarded place the shape lights cells that meant the other shape at neighbouring
  training places (not checked cell by cell).
- **GRU, a hold longer than 7 steps**: a longer backprop window (diagnostic below).
- **GRU, exclusive-or seed 5**: an entropy bonus, or another step size.
- **Cartpole**: a world fix first (the lean reward grades the wrong press); then returns or
  a critic for the sparse reward.
- **Sequence**: a prediction objective (a different loss, not just another size).

### The hold diagnostic

Delayed echo with longer delays, the fixed GRU, seeds 5-8 (`scripts/reference.sh hold`). A trial is
the cue, `delay` blank steps, the ask; the cue is delay + 1 steps before the answer.

| delay (steps) | 4 | 6 | 7 | 10 | 16 |
|---|---|---|---|---|---|
| gru late, per seed | 1.00 1.00 1.00 1.00 | 1.00 1.00 1.00 1.00 | 0.73 0.49 0.50 0.61 | 0.44 0.50 0.48 0.48 | 0.48 0.49 0.53 0.58 |
| learning off | 0.47-0.51 | 0.46-0.53 | 0.48-0.50 | 0.47-0.52 | 0.46-0.58 |

The wall is the window: up to 7 steps back (140 ticks) is learned on every seed; 8 steps
back, one step outside the window, only partly on two seeds of four; 11 and 17 steps back
not at all. The state could carry the cue further; the credit cannot reach it. This
is a concrete world where an unchanged network fails and only a per-task change (a longer
window) fixes it. Our engines today hold a cue about 20-60 ticks (1-3 steps), so the fixed
GRU's 140 ticks is still well beyond them.

## Only good and bad, against being told (the labelled extra)

The same two networks, same setting, with `--n.teach 1`: on every step the world judges,
cross-entropy on the right button (the perfect player's move) replaces good or bad. Steps
the world does not judge teach nothing. (A first version taught on every step that has a
right answer, which leaked the held-out places' answers: place 1.00. Fixed before these
runs.)

- **No gap at all on everything both learn**: echo 2-16, shuffled, exclusive-or, distances,
  retention, idle are 1.00 either way. For these worlds "only good and bad" costs nothing.
- **Cartpole is the big gap**: told the perfect player's press, the MLP balances (1000 steps
  per fall with the lean reward, the most a window can show; 758 with the sparse one).
  With good and bad it is worse than a coin. The network can do it; the reward cannot teach
  it (see "Suspect worlds").
- **Catch**: 0.50 told against 0.49 with good and bad for the MLP. The ceiling there is the
  always-press output and the one-step credit, not the reward.
- **Held-out place** stays below chance told (MLP 0.18): it is the network, not the signal.
- **Beat** is lower told (0.52) than with good and bad (0.64): the right move on three
  steps of four is "rest", which the perfect player gives as "no move", so those steps
  teach nothing.
- **The GRU told** gets exclusive-or 1.00 on every seed, so seed 5's lock at 0.75 with good
  and bad is the reward rule's, not the network's. Cartpole 1000 (lean) and 969 (sparse);
  catch 0.73 (0.72 with good and bad); held-out place 0.21; beat 0.50 for the reason above.

## Cost, side by side

Same life (40,000 steps, 20 ticks each), wall time of one life, this PC, under nice with
other agents' runs on the machine, the home-grown numbers from their own suite runs (also
loaded): read them as rough. Steps to learn: the first 1,000-step window at 90% of the way
from chance to the life's own late score, median over the seeds that learned it (how many
in brackets).

| world | mlp | gru | blob-main | blob-readout | minimal | table |
|---|---|---|---|---|---|---|
| echo k=2 | 2k (8/8), 3.5 s | 2k (8/8), 23 s | 7k (8/8), 19 s | 1k (8/8), 13 s | 4k (8/8), 3.5 s | 9k (8/8), 20 s |
| echo k=4 | 3k (8/8) | 3k (8/8) | 26k (5/8) | 1k (8/8) | 16k (7/8) | 15k (6/8) |
| echo k=8 | 4k (8/8) | 5k (8/8) | not learned | 12k (8/8) | not learned | not learned |
| shuffled echo | 2k (8/8), 3.7 s | 2k (8/8), 23 s | 11k (5/8), 19 s | 1k (8/8), 13 s | 7k (8/8), 3.5 s | 3k (7/8), 21 s |
| exclusive-or | 11k (8/8), 1.8 s | 12k (8/8), 14 s | not learned | 10k (8/8), 15 s | not learned | 10k (2/8), 20 s |
| delayed echo | not learned | 3k (8/8), 6 s | not learned | 9k (4/8), 16 s | not learned | not learned |
| order | not learned | 3k (8/8), 7 s | not learned | 14k (8/8), 15 s | not learned | not learned |
| gap | not learned | 5k (8/8), 5 s | not learned | 8k (1/8), 15 s | 11k (1/8), 3.4 s | not learned |
| beat | 3k (8/8), 2 s | 3k (8/8), 7 s | not learned | not learned | not learned | not learned |
| catch | 7k (8/8), 1.4 s | 8k (8/8), 8 s | not learned | 3k (5/8), 19 s | not learned | not learned |

The networks' time depends on how many steps are judged (one update per reward): delayed
echo and order are judged once per trial of 4 steps and run faster.

| | mlp | gru | blob-main | blob-readout | minimal | table |
|---|---|---|---|---|---|---|
| size | 34,960 floats | 150,288 floats | 4,096 neurons | 4,096 neurons | 4,096 neurons | 4,096 neurons |
| bytes | 140 KB weights, 560 KB with gradient and Adam's moments | 600 KB weights, 2.4 MB training state | 860 KB (210 per neuron) | 870 KB (212) | 347 KB (85) | 742 KB (181) |
| work per step | one pass of 256 bits x 128 and an update of every weight | 8 GRU steps back and an update of every weight | 20 ticks of 4,096 neurons | same | same | same |
| hardware | floats, backprop, global updates | the same, and a stored history | bits, local rules | bits | bits | bits |

The sheets are 4,096 neurons (8x8 tiles). Per byte they are in the same range as the MLP
and smaller than the GRU's training state; per life the MLP is as fast as the minimal
engine and the GRU as slow as the full one. Nothing here is a hardware-honest number for
the networks: they are floats with backprop through time, which the bench rules out for a
design.

## Suspect worlds (calibration)

Worlds that no reference, no home-grown engine and no offline linear reader learns. The
world's author should check each is learnable as built.

1. **cartpole, lean reward.** Nothing learns it, and learning makes every learner worse
   than a coin (MLP 17, GRU 12 steps per fall, against 22 for the same networks with
   learning off). Told the right press, the MLP balances (1000). The cause is known on the
   more-worlds branch (commit a3342ae, not on main): with the Euler step, a step's lean
   reward is independent of that step's press and grades the press before it. That branch
   adds `--cart-integrator semi` and a `cartpole-semi` row. Until it lands, the lean row
   measures nothing a learner can use.
2. **cartpole, sparse reward.** Good every 25 steps up, bad on a fall. Nothing learns it
   with good and bad; told, the MLP gets 758 and the GRU 969. Learnable, but only by something that
   carries credit 25 steps back. Fair as a slow-credit world; it should say so.
3. **place, a place never rewarded.** Nothing learns it; both networks and the readout go
   below chance (0.16, 0.25), told or not. Learnable only with a prior for position (a
   convolution). The inversion is worth checking: if a held-out place's shape lights cells
   that meant the other shape at neighbouring training places (likely, not checked), a
   learner that only reads pixels is pushed toward the wrong answer, and chance is the
   mark for "no sharing", not the floor. No linear reader was run on it.
4. **sequence prediction.** Not learned by anything. The minimal engine scores 1.00 but so
   does its learning-off twin, and the reference cannot take part. As a learning
   measure the row says nothing today.

Gap is no longer on this list: the fixed GRU learns it (1.00), and the rich2 sheet gets
0.93 (docs/results-2026-11-25-richness-search.md), above the linear reader's 0.63.

## Script lines

```
scripts/reference.sh one echo                       one life, mlp
scripts/reference.sh one delayed-echo --engine gru --delay 2
scripts/reference.sh select                         the selection grid, seeds 1-4
scripts/reference.sh suite                          mlp-fixed and gru-fixed (5 and 26 min here)
scripts/reference.sh told                           mlp-told and gru-told, the labelled extra
scripts/reference.sh compare                        reference against the home-grown scorecards
scripts/reference.sh hold                           the gru on delayed echo, delay 4 to 16
scripts/suite.sh mlp-fixed --engine mlp             what `suite` runs
scripts/suite.sh gru-fixed --engine gru
scripts/suite.sh mlp-told --engine mlp --n.teach 1
```

The selection was run as `scripts/sweep.py --seeds 1-4 --no-twin` over the grid in
`scripts/reference.sh select`. The told suites ran on stevejobs, the reference suites
here; results are byte-identical between the two machines.

## Plain-words reading

Today's suite cannot tell "unchanged network" apart from "our sheet" in the direction the
owner hoped. An ordinary GRU, sized once and set once on three worlds, does everything the
suite asks except the held-out place and cartpole, and nothing of ours learns anything it
does not. So the suite is mostly measuring how far our engines are behind a plain
network, not where they are different in kind.

The owner's point about changing the architecture holds, and the data shows where: the
MLP needs recurrence for anything in time; both need a convolution for a new place; the
GRU needs a longer window for a hold past 7 steps; cartpole needs a better reward or a
critic. Those four are the worlds where "one design, nothing changed" is a real test,
because the standard answer is a per-task redesign. The suite has only one of them
(place) as a row today. To find "a task the standard nn cant learn but ours can", the map
needs more of these: holds of tens to hundreds of steps, shapes and sounds in new places
and sizes, changed rules, several tasks interleaved in one life, rewards that come late.
And our designs would have to pass them, which none does yet.

## What should change elsewhere (for the coordinator; not edited here)

- **docs/findings.md, the headline**: add that a plain GRU with one fixed setting, told only
  good and bad through the same plug, learns every suite row any of our engines learns, at
  the same or a higher score, plus delayed echo, beat and gap (1.00), and order 1.00, echo
  with 16 cues 1.00 and catch 0.72 where ours get 0.69, 0.31 and 0.32;
  the plain MLP does all of it except the time rows. Our engines lead nowhere on score; the
  evidence readout learns association rows faster (under 1,000 steps against 2,000-3,000).
  The walls the fixed networks hit (held-out place, a hold past the backprop window,
  cartpole's reward) are where a design could differ in kind.
- **docs/findings.md, the walls "A new place" and "Time"**: the fixed networks also fail a
  new place (0.16), so that wall is a property of learning from pixels without a prior,
  not of our sheet. Time is not a wall for a plain GRU up to 7 steps.
- **docs/task-map.md**: a "fixed reference" column in the tier table: tier 1 passed (2-16
  cues), tier 2 passed (exclusive-or 1.00 MLP), tier 4 passed by the GRU (delayed echo,
  beat, order, gap) and failed past 7 steps, tier 5 failed (place), tier 7 catch 0.72 and
  cartpole failed (world), tier 8 passed. Under "Holes": a long-hold world (delayed echo at
  10, 50, 200 steps), and a changed-rule world on the same buttons (today's retention task
  puts B on new lines and buttons, so nothing interferes and every learner keeps A).
- **docs/task-map.md, cartpole**: mark the lean row suspect until the semi-implicit step
  lands; the sparse row as a late-credit world.
- **The suite**: add delayed echo at a long delay (10 or more steps) as the first row a
  fixed GRU fails for a known reason.
- **docs/scoreboard.md**: rerun `scripts/scoreboard.sh` to add `mlp-fixed` and `gru-fixed`
  (and `mlp-told`, `gru-told`, the labelled extra, also committed in `out/scorecards/`).

## Rough

- The networks decide once per step from the step's first tick. That uses the plug's step
  clock and button grouping, which the sheets do not get directly (the readout rule does get
  the pressed button). It is the standard set-up for a network and was asked for, but it
  is an advantage the sheets do not have.
- They cannot give "no move". In catch and beat that costs them; in echo it does not.
- With rest ticks after a reward (`--rest-ticks`, 0 in the suite), each rest tick would look
  like a new step to the network. Not used anywhere today.
- Credit goes to the judged step only. A standard actor-critic or returns would do better on
  catch and sparse cartpole; that would be a per-task recipe choice, so it was left out.
- Wall times were taken with other agents' runs on the machine, for every engine.
- The batteries named in the brief (function, invariance, reinforcement worlds, the rest of
  the map, MNIST) were not on main when these runs finished; main was merged (bf894a9) and
  rerun results are identical. When they land: `scripts/suite.sh mlp-fixed --engine mlp
  --resume` and the same for `gru-fixed` run only the new rows.
