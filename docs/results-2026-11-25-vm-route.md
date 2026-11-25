# The vm route: the owner's vector machine, in bits, taught without gradients

Idea 135. 2026-11-25. Engine `src/vm/`, `blobrun --engine vm`, `scripts/vm.sh`.

## The answer first

**Does it converge on anything? Yes.** The bit version, taught by local rules and no
gradient, learns and stays learned with one setting: delayed echo across 20 silent steps
(400 ticks) goes from 0.71 in the first 1,000 steps to 1.00 in every window after, on all 8
report seeds; echo with 16 cues, both-not-either, delayed echo at every gap, serial
addition 0.88, a cue held through 8 distracting events 0.76. With the input held still it
settles on one set of entries (a fixed point); trials with the same cue walk the same path
through the bank; different cues walk disjoint ones.

**Why his did not, in his words "stuck in minor loops".** It reproduces from his code: with
256 ops and nearest-op lookup the state sits on one op forever, with or without input, at
birth and after training; with his refractory lockout of 4 or 8 ticks it cycles through
exactly 5 or 9 ops, whatever the input does. The op added each tick is as large as the
state, so the state becomes its nearest op, and that op is again the nearest. There is no
"do nothing", the input is a small part of what chooses the op, and no loss reaches which
op is chosen.

**What made the bit version converge**, each checked by switching it off: an operation
leaves untouched bits alone (so a still state is a fixed point, which is the hold); the
input is overwritten into half of the address every tick (with the input weighing little,
as in his machine, everything is at a coin); and the input writes into the state through
the bank, by events written in one go with no reward (without them everything is at a
coin and the walk hardly depends on the input). One winner per tick, as in his machine, is
enough once those hold (delayed echo 0.94 and 0.95, order 0.77 and 0.86, choice seeds). The refractory lockout is not what
prevents loops.

**What it does that the sheets cannot**: hold a thing over any silence (the sheets hold
about 20 ticks; this holds 400 and more, exactly: 0 bits of the state change in the gap),
and learn a pair in one go. **What it does that the memory engine (a hashed lookup of the
last four events) cannot**: keep a cue through more distracting events than a fixed depth
(4 events 0.86 against 0.50; 8 events 0.76 against 0.50), carry a computed value further
back (serial addition 0.88 against 0.69), answer patterns never rewarded by nearness
(0.78 against 0.45), and hold through input noise (0.94 against 0.50 at 3% of cells
flipped). At 8 distractors it also beats the reference GRU (0.58), and at a 20-step gap
(GRU 0.49).

**What it cannot do**: count time (a still state cannot tell a short gap from a long one;
gap and beat at a coin), credit a press judged a step later (two-step at a coin; the
memory gets 0.99), learn a gate on what to keep (so 8 distractors is 0.76 and serial
addition 0.88, not 1.00), handle 32 buttons and fine holds with one setting, generalise to
a shape in a new place, or run fast (about a fifth of the memory engine's speed). The GRU
beats it on most rows.

**Does it belong with a sheet?** Not in front of one: its events want an input that looks
the same every time it comes. Beside one, offering the sheet a held value, maybe; untested
(section 7).

SUITE-SUMMARY

## 1. Why the original did not learn

The owner's machine (`thonksheet-library/repos/vvm`, June 2026):
`S(t+1) = normalize(deadzone(S(t) + scale * op))`, `op` sampled from the 8 entries of a
1,024 x 256 float bank nearest to `S(t)` by dot product. Trained by plain SGD with momentum
on the chosen op only (one step, or truncated backprop through the window with `--bptt`),
plus a weak "rejection" term that pushes a chosen op away from a state it served badly. No
AdamW in the code; the seed spec named it, the trainer never used it.

What he tried, from his docs: copy-input, delayed-copy, linear-2, basis-4, xor,
alternating-bit, sine-next, MNIST 0/1 and 10-class, a dozen anti-collapse and stability
knobs (refractory lockout, tangent transition, rejection, usage-aware rejection, retention,
anchoring to the best bank, LR schedules, wider class registers). Best result: MNIST 0 vs 1
at 80 to 87% balanced accuracy, where a linear reader on the raw pixels is near 100%.

What I ran (his code, copied to `out/vm/vvm-copy`, release build, nothing changed):

| run | what it shows |
| --- | --- |
| `vvm run --steps 16` (no training) | from tick 1 the state is 0.73 to 0.89 aligned with one op and stays on it: op 699 for 15 ticks running; with sampling, the same op 179 for 9 of 16 ticks |
| delayed-copy, 256-d, 1,024 ops, 30 epochs, lr 0 | test loss 0.006615 |
| the same, lr 0.1 | test loss 0.006588 (0.4% better than untrained; a random unit vector scores 0.0078) |
| the same with `--bptt`, window 16 | 0.006584 |
| bench-tasks, 16-d, 64 ops | delayed-copy: train loss 0.0081, test 0.0652, untrained test 0.0655: the training vectors are memorised, a new one is not held at all. alternating-bit 50%, xor 75% |

The owner's own account: "we got it stuck in minor loops ... i initialized it with like 256
ops. and it would just loop 7-15 and never learn anything at all." That reproduces. A small
driver (`src/vm/vvm/vvm_walk.cpp`, linked against his unchanged library) walks his machine with
256 ops for 600 ticks after 100 to settle, from 5 starting states, with no input, with one
input held, and with two inputs taking turns every 8 ticks:

| his machine, 256 ops | no input | input held | input switching every 8 ticks |
| --- | --- | --- | --- |
| at birth, nearest op | 1 op, period 1 (all 5 starts) | 1 op, period 1 | **1 op, period 1**: the input never changes the op |
| at birth, nearest op, refractory lockout 4 ticks | 5 ops, period 5 | 5.2 ops, period 5.2 | 17.8 ops, period 38 |
| at birth, refractory lockout 8 ticks | 9 ops, period 9 | 11 ops, period 14 | 24 ops, period 37 |
| trained (30 epochs delayed-copy, `--hard-retrieval`) | 1 op, period 1 | 1 op, period 1 | 1 op, period 1 |
| at birth, sampled from the top 8 (his default) | 39 ops, no cycle, top op 13% of ticks | 20 ops, top op 32% | 33 ops, top op 21% |
| trained, sampled | 42 ops, top op 14% | 19 ops, top op 24% | 32 ops, top op 22% |

His training runs report the same: hard retrieval uses 34 of 256 ops with one op chosen 144
times an epoch. So "loops of 7 to 15" is the refractory lockout's own period: the state sits
on its nearest op, the lockout forbids it for N ticks, the walk steps to the next nearest
and back, a cycle of N + 1 ops, whatever the input does. Without the lockout it is a cycle
of one. Sampling turns the cycle into a random walk over about 20 to 40 ops that again does
not depend on the input. Learning does not pull it out because nothing in the loss reaches
which op is chosen (point 4 below), and the input is added into a state that the next op
then half-overwrites (point 1).

My account of the failure, in order of weight:

1. **The state falls onto an op and stays there.** The op added is a unit vector of the
   same size as the state, so each tick replaces about half of what the state held. The op
   nearest the state is the one most like it, so adding it pulls the state further toward
   it, and the next lookup finds the same op. Within two ticks the input is gone and the
   state is that op, give or take the deadzone. The machine is a quantiser onto its own
   1,024 prototypes. Holding anything is only possible as "which op did it fall into",
   about 10 bits, and only for things that were trained in as ops. That is exactly the
   bench-tasks row: the 16 training vectors held, a new vector not at all. His "mini-loop"
   diagnosis and the refractory lockout were circling this.
2. **There is no way to do nothing.** Every tick some op is added at full size. A register
   machine's commonest instruction, "leave it", does not exist, so memory decays by a fixed
   factor per tick whatever is learned.
3. **The op is both the address and the effect.** He saw this himself (the key/value
   conflict). Training an op to move the state toward a target moves where it is found,
   so a useful op wanders away from the states that needed it.
4. **Nothing teaches which op is chosen.** The gradient reaches only the chosen op's
   content, never the lookup. Nothing forces different states to find different ops; the
   rejection term was the only route pressure, and it is a weak heuristic.
5. **The default trainer credits one step.** Without `--bptt` an op is trained to make the
   next state look like the final target, so a sequence of ops never forms.

So it is not that gradients cannot pass the sampled lookup (they were never asked to). The
dynamics were wrong before learning started: a contraction onto prototypes, with no idle
operation, has no memory to train.

## 2. The bit version

### What it is

One state of bits, in three parts, and a bank of 2,048 entries. Defaults below; every number
is a `--v.name value` flag. One setting for every row of this doc and the suite.

- **X, the world's part** (1,024 bits), overwritten every tick: each of the 256 sensor cells
  that is on ORs a fixed random code of 16 bits into X. A cue line (9 cells) lights about
  140 bits.
- **M, the machine's own part**, one bit per sheet cell that is neither sensor nor motor
  (3,584 bits on the default sheet). A bit keeps its value until an operation sets or clears
  it.
- **O, the output**, one bit per motor cell (256). The plug reads it as it reads any engine's
  motor spikes.
- **An entry** is an address (a bit mask over X and M: density 1/8 over X, 1/32 over M, so
  X and M weigh about equally), an operation (a set mask and a clear mask over M), and a
  signed 5-bit evidence counter per motor cell. Every entry has the same shape.

One tick:

1. The world's code goes into X.
2. Every entry's match is `popcount(address AND state)`. A histogram of the matches gives
   the bar that exactly 16 entries clear (ties at the bar broken from a random start).
3. Events (below): if the input just changed, its detector is found (or written).
4. The operations of the winners that did not fire on the tick before (refraction, as in
   production systems: an entry acts once, when it starts to match) are ORed: a bit is set where some winner sets it and none
   clears it, cleared where some clears and none sets, left alone on a conflict or when no
   one touches it. The event detector's operation is ORed in too, winner or not. A homeostat thins the sets (AND with a random word) while M is above
   1/8 of its bits on, and the clears while below.
5. Each motor cell sums the winners' evidence and fires if the sum beats a fresh 9-bit
   random number (a rate code), or by chance 1/16.

Learning, all local to entries, all integer:

- **Evidence (finding 15's error rule).** On bad, every entry that fired in the last 20
  ticks moves its counters one step: down on the pressed button's cells, up on the other
  buttons' cells. On good, nothing.
- **Answer writing (one-shot).** On every outcome the quietest of 16 sampled entries is
  rewritten: its address becomes a sample of the state's on bits (as many per part as a
  random address has), its evidence becomes the answer the outcome implies at strength 8
  (good: the pressed button; bad: the other buttons, sharing it), its operation is none.
  Next time the state is near this one, this entry matches far better than any random one
  and answers.
- **Events (no reward).** When X has settled on a pattern (at least 8 bits on, three
  quarters the same as on the tick before, and less than three quarters the same as the
  last event's pattern), and no
  winner is already a detector of it (an entry that writes something, whose X address is
  at least half the size of what is on and three quarters lit), the quietest of 16 sampled entries becomes one:
  its address is the input's on bits (no M part), its operation a fresh random code: set
  1/16 of M, clear 1/2 of M. Whenever the input changes to one it knows, its detector makes
  that mark again, whether or not it is among the 16 winners. So each event leaves its own
  mark in M, and every later event halves what is left of the earlier ones. Marks fade per
  event, not per tick.
- **Inner rule, as specified (off).** On bad, entries that fired since the last outcome
  lose each mask bit with chance 1/8. Off by default; see below.

What has no counterpart in the owner's machine, and is what made it work: an operation can
leave a bit alone, so a still state is a fixed point; the address and the effect are
separate masks; the lookup takes 16 entries, not one; and the bank is written by
outcomes and by events, not moved by small steps.

### What changed along the way (choice seeds, 10,000-step lives, standard plug)

| version | echo k=2 | echo k=8 | delayed echo, gap 2 | order | gap | both-not-either | what the probe showed |
| --- | --- | --- | --- | --- | --- | --- | --- |
| v1 (seeds 1, 2): random operations, answers written with half the state's bits | 1.00 | 0.00 | 0.49 | 0.52 | 0.50 | | the written entries matched on M won every tick whatever the input; M alike between trials with the same and different cue: 0.615 / 0.619 |
| v2 (seed 1, probe): addresses sampled to a random address's size, events with sparse codes (clear 1/16) | | | 0.52 | | | | the event codes never erased each other: M identical in every trial (1.000 / 1.000) |
| (seeds 1, 2) events clear half of M | 1.00 | 0.00 | 0.72 | 0.75 | 0.47 | 1.00 | M same / different cue: 0.44 / 0.15; delayed echo 1.00 on its first seed, gap 20 1.00 on two |
| (seeds 1, 2) output as a rate code (fixes ties) | 1.00 | **1.00** | 0.98 | 0.72 | 0.52 | | echo k=8 was 0.00 because the seven unpressed buttons gained equally on every bad, all saturated, tied, and a tie is no move, which is never judged: frozen |
| (seeds 1 to 4 from here) inner rule off, the default since | | 1.00 | **1.00** | 0.76 | 0.49 | 0.98 | with it on, order seed 1 had 37 entries left firing and M frozen: the inner rule stripped the event entries, which fire before every outcome, right or wrong |
| 8 winners instead of 16 | | 1.00 | 0.73 | 0.48 | | 0.48 | |
| 32 winners | | 1.00 | 1.00 | 0.66 | | 1.00 | |
| events clear 1/4 of M | | | 1.00 | 0.52 | | 0.98 | |
| events clear 1/8 of M | | | 1.00 | 0.61 | | 0.97 | |
| half of M slides one bit per tick (a clock that loses nothing) | | 1.00 | 1.00 | 0.46 | 0.47 | 1.00 | the slid copies are too faint against the rest of M |
| **refraction (kept)** | | 1.00 | 0.96 | **0.90** | 0.47 | 0.98 | before it, at the A+B step the event entries for A alone and B alone also matched fully and wrote both marks every tick, whatever came first (M same/different order: 0.53 / 0.51 at that step) |
| answer entries no longer count as an input's detector (a bug fix, kept) | | 1.00 | 0.96 | 0.90 | 0.49 | 0.97 | in two-step the second step's event entry was evicted and never written again: M changed 0 bits at the second step |
| credit window 40 ticks (two steps; this row and the next before the fix above) | 1.00 | 1.00 | 0.95 | 0.86 | | 0.96 | two-step 0.14 |
| credit 40, refraction off | | | 1.00 | 0.66 | | | two-step 0.54 (3 seeds of 4 learn it): no one setting had both two-step and order |
| **an input change makes its detector's mark even when the detector is not a winner (kept)** | | 1.00 | **1.00** | 0.81 | | 0.94 | before it, answer entries for the ask out-matched the ask's own event entry, so after learning the ask's mark was no longer made (M changed 0 bits at the ask step). Also: distract 4 0.72 (seed 1) to 0.84, serial-add 0.62 to 0.83, majority held out 0.74 |
| **an event waits for the input to settle, by nearness (kept)** | | 1.00 | 1.00 | 0.77 | | | distract 4 0.81, serial-add 0.84, majority held out 0.78; under input noise (section 3) delayed echo 0.48 to 0.94 |
| events also keyed on context in M (`event_m 1`; tried before the two rows above) | | | 0.48 | 0.46 | | | seed 1 only: every input in a new context is a new event, the bank turns over, nothing is reused; serial-add 0.64, distract 0.51 |

Blank cells were not run for that version. Gap 20 reached 1.00 on both of seeds 1 and 2 as soon
as events cleared half of M.

## 3. Scores

One setting (the defaults in `src/vm/vm.hpp`) for every row. Seeds 5 to 12, the standard life
(20 ticks a step, 3x3 line groups, 40,000 steps, windows of 1,000), late score (mean of the
last tenth of the windows). The vm beside its learning-off twin and coin flips; the memory
engine (`--engine memory`, idea 123) and the fixed reference GRU (`--engine gru`) beside it,
each with its own one setting. `scripts/vm.sh rows TAG beyond,hold,... 5-12 [--engine
memory]`. Raw runs in `out/vm/rows/{vm3,memory,gru}/`.

### What a lookup of the recent past cannot do (the coordinator's three)

(a) distracting events in the gap, (b) a held value that is computed, never shown, (c)
cases never rewarded. New worlds in `src/world/task_beyond_lookup.cpp`; place is the suite's.

| world | vm | vm, learning off | coin | memory | gru |
| --- | --- | --- | --- | --- | --- |
| distract, 1 event in the gap | 1.00 ±0.00 | 0.46 | 0.47 | 0.99 ±0.00 | 1.00 ±0.00 |
| distract, 2 events | 1.00 ±0.00 | 0.46 | 0.48 | 0.99 ±0.00 | 1.00 ±0.00 |
| distract, 4 events | 0.86 ±0.04 | 0.46 | 0.47 | 0.50 ±0.02 | 1.00 ±0.00 |
| distract, 8 events | 0.76 ±0.06 | 0.47 | 0.48 | 0.50 ±0.02 | 0.58 ±0.10 |
| serial-add (sum bit, carry held) | 0.88 ±0.02 | 0.46 | 0.47 | 0.69 ±0.01 | 0.92 ±0.02 |
| majority, held-out patterns only | 0.78 ±0.09 | 0.45 | 0.49 | 0.45 ±0.17 | 1.00 ±0.00 |
| place, held-out places only | 0.21 ±0.10 | 0.46 | 0.47 | 0.63 ±0.34 | 0.17 ±0.11 |

- **(a) Distractors.** The vm loses the cue gradually, a half per event, so it still has
  0.86 after 4 events and 0.76 after 8. The memory's depth is 4 events: 0.99 up to 2, a
  coin at 4. The GRU is perfect up to 4 and 0.58 at 8. **At 8 distracting events the vm is
  the best of the three.**
- **(b) A computed held value.** Serial addition with the carry passing through 3 pairs in
  4: the memory is held to about what its 4-event window sees (0.69); the vm 0.88; the GRU
  0.92. The vm does not hold a carry bit (section 4); it reads the carry off the fading
  marks of the last several pairs, which reaches further back than 4 events.
- **(c) Never rewarded.** Majority of 5 lines, scored only on 6 of 32 patterns never
  rewarded: the vm 0.78 (nearness: a held-out pattern is close to rewarded ones with the
  same answer), the memory 0.45 (its codes are parities, so a new pattern is a stranger),
  the GRU 1.00. Place (a shape at a place never rewarded) defeats the vm and the GRU, both
  below a coin (0.21, 0.17: what they learned in the rewarded places points the wrong way),
  and splits the memory (0.63 ±0.34).

### Holding, steps, capacity

| world | vm | vm, learning off | coin | memory | gru |
| --- | --- | --- | --- | --- | --- |
| delayed echo, gap 0 | 1.00 ±0.00 | 0.46 | 0.48 | 0.99 ±0.00 | 1.00 ±0.00 |
| delayed echo, gap 2 | n/a | n/a | n/a | 0.99 ±0.00 | 1.00 ±0.00 |
| delayed echo, gap 5 | 1.00 ±0.00 | 0.45 | 0.48 | 0.99 ±0.00 | 1.00 ±0.00 |
| delayed echo, gap 20 | 1.00 ±0.00 | 0.46 | 0.47 | 0.99 ±0.01 | 0.49 ±0.05 |
| order | n/a | n/a | n/a | 0.99 ±0.00 | 1.00 ±0.00 |
| gap | n/a | n/a | n/a | 0.99 ±0.00 | 1.00 ±0.00 |
| both-not-either | n/a | n/a | n/a | 0.99 ±0.00 | 1.00 ±0.00 |
| echo, 32 cues | 0.03 ±0.01 | 0.02 | 0.03 | 0.05 ±0.01 | 0.80 ±0.06 |
| two-step | 0.20 ±0.06 | 0.21 | 0.23 | 0.99 ±0.00 | 0.15 ±0.07 |

- Delayed echo is held perfectly at every gap tried, 20 steps (400 silent ticks) included;
  the GRU fails at 20 (0.49): its training window does not reach back that far.
- Order, gap and both-not-either: the suite (below).
- **Echo with 32 cues fails** (0.03, a coin), and so does the memory (0.05); the GRU 0.80.
  At 2x2 line groups (needed to fit 32 lines) a cue is 4 cells. See section 5.
- **Two-step fails** (0.20, a coin 0.23): the first press is only credited by an outcome
  one step later, and the vm's credit window is one step. The memory, which credits the
  frames of the whole trial with the buttons they drove, gets 0.99; the GRU 0.15.

### Input noise (a sheet in front, the cheap version)

The memory agent found a spiking sheet in front of a lookup made every row worse, because
noise breaks exact matching. The cheap test of whether a nearness match tolerates it:
`--input-noise N` flips each sensor cell with chance N/1,024 per tick, the same for every
engine. Seeds 5 to 8, 10,000-step lives (run on the second machine).

| | echo 8 cues: vm / memory / gru | delayed echo, gap 2 | order |
| --- | --- | --- | --- |
| no noise | 1.00 / 0.98 / 0.99 | 1.00 / 0.99 / 1.00 | 0.74 / 0.99 / 1.00 |
| 3% of cells flipped a tick | 1.00 / 0.99 / 0.99 | **0.94** / 0.50 / 1.00 | 0.47 / 0.48 / 1.00 |
| 10% | 0.97 / 0.79 / 0.96 | **0.71** / 0.51 / 1.00 | 0.49 / 0.50 / 0.98 |

Before events waited for the input to settle, the vm fell like the memory (delayed echo
0.48 at 3%): every noisy tick was a new event that cleared half of M. Since, it holds
through noise the memory cannot; order is still lost. The GRU does not notice the noise.

### The whole suite, one setting

`scripts/suite.sh vm --engine vm` (`scripts/vm.sh suite`), seeds 5 to 12, 54 minutes, 2 runs
at a time; scorecard `out/scorecards/vm.csv`. Beside it the memory engine, the fixed GRU
and the best sheet (`blob-readout`, frozen sheet with the evidence readout) from their own
scorecards. Late scores; chance is coin flips.

| row | vm | vm, learning off | chance | memory | gru-fixed | best sheet |
| --- | --- | --- | --- | --- | --- | --- |
| echo k=2, 4, 8, 16 | 1.00, 1.00, 1.00, 1.00 | 0.46, 0.22, 0.10, 0.05 | 0.47 ... 0.05 | 0.99 ... 0.98 | ... 1.00 | 1.00, 1.00, 0.84, 0.31 |
| capacity, largest k held | **16** (all seeds) | 0 | 0 | 16 | | 8 |
| shuffled echo | 1.00 | 0.46 | 0.48 | 0.99 | | 1.00 |
| delayed echo, 2 steps | **1.00** | 0.45 | 0.48 | 0.99 | 1.00 | 0.55 |
| beat, every 4 steps | 0.42 ±0.01 | 0.50 | 0.51 | 0.99 | 1.00 | 0.50 |
| sequence (prediction) | 0.50 (no predictor) | 0.50 | 0.50 | | | 0.57 |
| catch 4x4 | 0.54 ±0.05 | 0.25 | 0.25 | 0.99 | 0.72 | 0.32 |
| cartpole, lean (steps per fall) | 20.5 | 22.8 | 23.3 | 109 | 12.0 | 17.6 |
| cartpole, sparse | 43 ±31 | 22.8 | 23.3 | 279 | 14.2 | 14.2 |
| echo, cue and button 1, 2, 3, 5 tiles apart | 1.00 at every distance | | 0.47 | | | 1.00, 0.91, 0.85, 0.59 |
| order | 0.87 ±0.12 | 0.45 | 0.48 | 0.99 | 1.00 | 0.69 |
| both-not-either | 0.96 ±0.02 | 0.46 | 0.48 | 0.99 | 0.97 | 0.88 |
| gap | 0.48 | 0.45 | 0.47 | 0.99 | 1.00 | 0.52 |
| place (never-rewarded places) | 0.21 ±0.10 | 0.46 | 0.47 | 0.63 ±0.34 | 0.16 | 0.25 |
| echo B after A | 1.00 | 0.21 | 0.23 | | | 0.99 |
| echo A retested after B, first window / end | 0.96 / 1.00 | 0.45 | 0.46 | 1.00 | 1.00 | 0.98 / 1.00 |
| echo after 400,000 ticks unplugged, first window | **1.00** | 0.43 | 0.47 | 0.99 | 1.00 | 1.00 |
| ticks/s in the echo life (loaded machine) | 47,000 | | | 187,000 | 36,000 | 63,000 |
| ticks/s at 8x8, 16x16, 32x32 tiles | 49,100, 17,900, 4,900 | | | | | |
| bytes per sheet cell (32x32 tiles) | 782 | | | | | |

The suite's reach rows ("cue readable at N tiles") read the shell's sheet, where the vm's M
bits are laid on cells with no geometry, so they say nothing about this engine; distance
does not exist for it (echo is 1.00 at 5 tiles). Its "stays alive" rows likewise are the M
bits on the sheet (77% of cells silent: X and the bank are not on the sheet). Beat is below
a coin: with no input between beats the state is a fixed point and cannot count steps.

MAP-PLACEHOLDER

## 4. Inside it

### Does it fall into loops, as his did

The same measurement as for his machine (section 1), on the bit version: `vmprobe --walk`,
the walk with the input held for 1,200 ticks (200 to settle, 1,000 recorded), at birth and
after a 10,000-step life, seeds 1 and 2. A "set" is the 16 winners of one tick; "period up
to ties" counts two sets as the same when they share 12 of 16 (ties at the bar are broken by
chance, so exact repeats are rarer). All from `scripts/vm_ablate.sh`.

| | input blank | one line held | two lines taking turns every 20 ticks | the two lines' winners alike |
| --- | --- | --- | --- | --- |
| at birth | 1 set of 16 entries, period 1 | 1 set, period 1 | 672 sets over 216 entries, no period | 0.01 |
| after delayed echo (seeds 1, 2) | 3 and 1 sets over 17 and 16 entries, period 1 up to ties | 11 and 3 sets over 25 and 18 entries, period 1 up to ties | 41 and 40 sets, 44 and 57 entries | 0.00, 0.03 |
| after order (seeds 1, 2) | 1 and 6 sets over 16 and 20 entries, period 1 up to ties | 5 and 1 sets over 17 and 16 entries, period 1 up to ties | 66 and 64 sets, 57 and 52 entries | 0.00, 0.14 |

So with the input held it does settle, at once, on one set or a few that differ only by
ties among 16 to 25 entries: a fixed point. That is the hold, and it is meant: a bit nobody
sets or clears keeps its value, so a still input gives a still state. The difference from
his machine is what happens when the input moves: his stayed on the same op whatever the
input; here the two inputs get two different sets of entries (sharing 0 to 14% of them).
The walk follows the world.

Entries that fired at least once over a 12,000-step life: 1,150 to 1,200 of 2,048 on
delayed echo, 1,270 to 1,630 on order, all 2,048 on echo with 8 cues and on serial-add
(20,000-step probes). In the last 2,000 steps of a delayed-echo life, 367. The bank is not
collapsing onto a few entries; it is used, and turned over by writing.

### What prevents the collapse: each piece switched off

Same lives (10,000 steps, seeds 1 and 2, the final setting), share of presses right while
still learning over the last 2,000 steps (`vmprobe`'s "right"; 0.5 is a coin), and the
walk after learning order.

| switched off | delayed echo | order | entries used over the life (delayed echo, order) | walk after order, lines taking turns: sets / entries, alike | what it shows |
| --- | --- | --- | --- | --- | --- |
| nothing (the design) | 1.00, 1.00 | 0.85, 0.80 | 1,154, 1,271 | 66 / 57, 0.00 | |
| one winner per tick, as in his machine (`active 1`) | 0.94, 0.95 | 0.77, 0.86 | 219, 274 | 2 / 2, 0.00, period 40 = the input's | one operation a step is enough: no loop, the one winner follows the input |
| refraction | 1.00, 1.00 | 0.64, 0.51 | 1,097, 1,575 | 70 / 69, 0.13 | not what prevents loops (held input: 95 sets over 22 entries at birth, wandering); it is what makes order work |
| the density homeostat | 1.00, 1.00 | 0.50, 0.51 | 747, 809 | 21 / 33, 0.25 | without it M fills and order is lost |
| **the input weighing as much as M** (address density over X 1/128 instead of 1/8) | **0.53, 0.49** | **0.49, 0.49** | 2,041, 2,041 | 573 / 703, 0.10 | with the input a small part of the match nothing is learned and the bank churns; this is his machine's arrangement, an input added into a state that dominates the lookup |
| **events** (the input never writes into M) | **0.45, 0.53** | **0.50, 0.49** | 462, 357 | 11 / 21, **0.77** | nothing holds the cue, and the walk hardly depends on the input: the closest this machine comes to his loop |
| answer writing | 0.97, 0.94 | 0.73, 0.76 | 1,773, 1,742 | 614 / 237, 0.04 | evidence alone learns most of it, more slowly |
| marks made by detectors that are not winners | 0.97, 0.97 | 0.92, 0.79 | 1,939, 1,962 | 105 / 46, 0.04 | about even here; it matters for serial-add and distract (section 2) |
| settling (any changed bit is an event) | 1.00, 1.00 | 0.82, 0.79 | 1,397, 1,741 | 218 / 86, 0.22 | the same without noise; it is what holds under input noise (section 3) |
| all learning (the twin) | 0.44, 0.44 | 0.44, 0.44 | 1,865, 1,876 | 315 / 86, 0.14 | |

The two pieces that matter are the two his machine lacked: **the input weighs as much as the
machine's own bits in choosing what runs** (X is overwritten every tick and holds half the
address bits), and **the input writes into the state through the bank** (events), so the
state carries what happened. With both, a single operation per step is enough. Refraction
(his lockout) is not what prevents loops here; it only decides whether an operation repeats
while it keeps matching.

### The walk through the bank on learned tasks

`scripts/vm.sh probe delayed-echo --delay 2 --show 2 --seed 5` (20,000 steps learning, 2,000
recorded, everything still learning; files in `out/mini/final/` from the second machine,
results there are byte-identical). Right on 500 of 500 asks. Two trials with the same cue,
the 16 winners at a few ticks of each step:

```
trial 0, cue 0, pressed 0, good             trial 1, cue 0, pressed 0, good
 cue   tick 1  31 262 278 293 342 ... 1788   cue   tick 1  31 207 262 278 293 ... 1788
       tick 3  262 293 342 368 ... 2018             tick 3  262 278 293 342 ... 2018
 blank         17 70 262 342 368 ... 2018    blank         17 70 262 293 342 ... 2018
 blank         17 70 262 342 368 ... 2018    blank         17 70 262 342 368 ... 2018
 ask   tick 1  115 262 293 368 406 544 ...   ask   tick 1  115 262 293 368 406 544 ...
       tick 20 31 207 262 278 293 ...               tick 20 31 262 278 293 368 ...
```

The cue step starts on the entries that know the cue, the cue's event entry makes its mark
on the second tick (once the input has settled), and the winners move to a set that
matches the new M; through both blank steps the set is the same, give or take a tie; the
ask brings in the ask's entries and the answer entries written on earlier asks with this
cue. Two trials with the same cue take the same path (they converge from the first tick:
the cue's mark overwrites most of what the last trial left). Measured over all 500 trials:

| step of the trial | winners alike, same cue / other cue | M alike, same cue / other cue | M bits changed since the step before |
| --- | --- | --- | --- |
| cue | 0.72 / 0.00 | 0.91 / 0.32 | |
| blank 1 | 0.71 / 0.00 | 0.91 / 0.32 | 0 |
| blank 2 | 0.71 / 0.00 | 0.91 / 0.32 | 0 |
| ask | 0.53 / 0.00 | 0.81 / 0.58 | 200 |

Different cues send the state down disjoint paths (winners alike 0.00); the same cue gives
a path that repeats. 367 distinct entries fired in the 2,000 recorded steps.

**How it holds a cue across a gap: a fixed point.** With delay 20 (`--delay 20`, 91 asks,
all right) the table is the same for all 20 blank steps: M changes 0 bits over 400 silent
ticks, M alike 0.91 within a cue and 0.28 across, winners alike 0.75 and 0.00, and 89 of
90 trials sit on one winner set through the whole gap. Not a cycle, not a chain: nothing
in the gap matches an entry that writes, so nothing changes.

**Distractors.** With 4 distracting events (`--task distract`, right 0.81) the cue's mark
halves at each one: M alike same / other cue is 0.59 / 0.10 after the cue, then 0.35 / 0.19,
0.36 / 0.29, 0.39 / 0.34, 0.42 / 0.38 after the fourth. The answer is read off what is left.

**Order** (right 0.65 on this seed at 20,000 steps): after the first cue M is 0.75 / 0.27
alike, after "both" 0.72 / 0.71; the order survives in the share of each cue's mark, a small
part of M, which is why it is the weakest of the holds.

**Serial addition** (right 0.92 of presses on this seed): M changes 270 to 360 bits every
step, the marks of the last few pairs, and every one of 2,048 entries fired at some point:
no one bit of M is the carry. The likeliest reading is that the answer entries read the
carry off the pattern of the last few pairs' marks: a soft lookup of the recent past that
reaches further back than the memory engine's four events.

## 5. Where it breaks

1. **It does not compute a held value; it fades one.** Nothing in the bank learns "if the
   carry is set and this pair comes, set the carry". Operations are written by events, keyed
   on the input alone, and never changed by outcomes (the inner rule as specified stripped
   the event marks and was switched off; keying events on context too, `event_m 1`, makes
   every input in a new context a new event, the bank turns over and every row falls to a
   coin). So M is a record of recent events, each halved by every later one. That is enough
   for serial addition at 0.88 and for 8 distractors at 0.76, and it is why neither is 1.00:
   a learned gate (keep this, ignore that) is what the GRU has and this does not. The
   register machine, multi-bit operations with several internal steps, the calculation
   battery: not tried (not on main), and I expect them to fail for this reason.
2. **No sense of time.** A still input is a fixed point, so the state cannot count blank
   ticks. Gap (short or long blank between two blinks) is at a coin; beat in the suite. A
   ring of M that slides one bit a tick (a permutation, so nothing is lost) was tried and
   did not help: the slid copies were too faint for the answer entries' match.
3. **Late credit.** Evidence moves only on the entries that fired in the last 20 ticks (one
   step). Two-step (the first press judged one step later) stays at a coin. A 40-tick
   window with refraction off learned it to 0.54 on three seeds of four, and cost order.
4. **Many buttons.** The error rule never raises the pressed button and raises all the
   others on every bad, so with 32 buttons the right one never stands out: echo 32 at a
   coin. Letting good move evidence too (`--v.ev_good 1`: on good the pressed cells gain
   and the others lose) learns echo 32 to 1.00 on all four choice seeds, keeps echo 2 and 8,
   shuffled echo (no lock-in) and delayed echo at 1.00, serial-add 0.83 and majority 0.81,
   but drops order from 0.77 to 0.60 and distract 4 from 0.81 to 0.65 (seeds 1-4, 10,000
   steps). So there is no one setting yet with both many buttons and the fine holds.
5. **Order is the weakest hold** (0.74 to 0.85 on choice seeds; the suite row below). "A
   then A+B" leaves a small share of A's mark against the shared A+B mark. Under input
   noise it is lost.
6. **A new place.** Place is below a coin (0.21), like the sheets and the GRU: what is
   learned about a shape in the rewarded places does not carry, and points the wrong way.
7. **Speed.** About a fifth of the memory engine's ticks per second: every entry is compared
   with the state every tick (section 6).

## 6. Speed, bytes, hardware

Memory, default sheet (4,096 cells), from `vm_bytes`: addresses 2,048 x 72 words (1.18 MB),
set and clear masks 2 x 2,048 x 56 words (1.84 MB), evidence 2,048 x 256 bytes (0.52 MB),
stamps and match counts 20 KB, the state 608 bytes. 3.57 MB in all, about 870 bytes per
sheet cell. The suite's heap measure is in the scorecard.

Work per tick: 147,000 ANDs and popcounts (every entry against the state), one histogram of
2,048 small numbers, 16 x 56 x 2 word ORs for the operations, 16 x 256 byte adds for the vote.
The match is nearly all of it, and it reads the whole address table every tick, so the
engine is bound by how fast 1.2 MB can be streamed: about 26,000 to 31,000 ticks a second
per life here under load (other agents' runs, CPU capped; not a quiet figure), against
about 110,000 to 145,000 for the memory engine and the GRU on the same lives. Learning costs
almost nothing: an outcome scans 2,048 stamps and moves a few hundred evidence rows; an
event rewrites one entry.

On other hardware the shape is the textbook one for content-addressable memory:

- **FPGA**: the address table is 9.4 Mbit of block RAM, the state is broadcast, each entry is
  an AND and a popcount tree, the 16 winners come from a histogram (a counter per match
  value) and a second pass. At 64 entries a cycle a tick is about 40 cycles, so roughly
  7 million ticks a second at 300 MHz for one machine. A ternary CAM does the match in one
  cycle.
- **GPU**: one tick is a 2,048 x 4,608 bit matrix times a bit vector, the binary-network
  kernel, but each tick depends on the last, so one machine leaves the GPU idle. Run many
  machines (seeds, worlds) side by side, one per block, and it is a good fit.
- **CPU**: hashing the address so that only a few entries are touched (as the memory engine
  does) would cut the work 100-fold, at the price of exact nearness; that trade is the
  difference between the two engines (section 3).

Deterministic for a seed (two runs, same csv, checked).

## 7. Does it belong with a sheet

Not as built. A sheet's offer is a rich mix of the input (exclusive-or, order, gap read off
neurons a tile or two from the input; finding 8). The vm's input path is the reverse of
mixing: it wants an input to look the same every time it comes, because an event is "an
input no detector knows", and a sheet's spikes are never the same twice. The memory agent
found the same for its lookup (a sheet in front made every row worse). What the vm offers a
sheet is the part a sheet lacks: a state that holds over any silence and an answer stored in
one go. Two ways they could fit, untested:

- sheet in front, through a cleaner: only the sheet cells that are reliably on for an input
  (a slow vote over a step) go into X. Then a sheet feature like "A came before B" becomes
  one more event the vm can hold.
- vm beside, not in front: the vm reads the raw input as now; its M bits are offered to the
  sheet as extra input lines, so the sheet's readout can use a held value. That is the
  "organs" idea (124), and it puts the hold where the sheet cannot make one.

The input-noise test in section 3 says how much a nearness match tolerates before either is
worth building.

## 8. What should change in the shared docs

For the coordinator (I have not edited them):

- **docs/findings.md**, new principles:
  - "A state that is the address of its next step holds, once an operation can leave bits
    alone and the input writes into the state." [likely: one engine, eight seeds, every gap
    0 to 20 at 1.00, and the ablation] The owner's vvm had neither: adding a unit-size op to
    a unit state makes the state fall onto its nearest op and stay (one op forever, or a
    cycle of lockout + 1 ops, whatever the input). Switching off either piece in the bit
    version puts it at a coin.
  - "Forgetting by event, not by tick, degrades gracefully under distraction." [hint] Each
    event halves earlier marks: 0.86 at 4 distracting events and 0.76 at 8, where a
    4-event lookup is a coin and the GRU 0.58 at 8.
  - "Matching by nearness generalises to unseen inputs a parity-hashed lookup cannot."
    [hint] Majority of 5 on held-out patterns: 0.78 against 0.45. Place: both fail.
  - "An exact-change event detector is broken by input noise; settling by nearness fixes
    it for holds." [hint] Delayed echo at 3% cell noise 0.48 -> 0.94; the memory 0.50.
- **docs/findings.md, walls**: time (a still state cannot count; gap and beat at a coin for
  the vm), late credit (two-step), a learned gate on what to keep (serial addition 0.88,
  8 distractors 0.76, where a gate would be 1.00), and the trade-off between many buttons
  (good must move evidence) and fine holds.
- **docs/task-map.md**: tier 4's "a cue is still held only about 20 ticks" is out of date
  (the memory and the vm both hold delayed echo 400 silent ticks). Add the three new
  worlds: distract (tier 4, holding against distraction), serial-add (a held value that is
  computed, not shown; tier 3 or the calculation battery), majority with held-out patterns
  (tier 2, "generalises" apart from "remembers", idea 149). Add a row kind "input noise"
  under "how much of it there is" (`--input-noise`).
- **docs/ideas.md, idea 135**: tried; result this doc. New ideas it suggests: a learned
  gate on event marks (keep / do not clear), credited by outcomes, for distractors and
  carries; a slow clock that only advances on blank input, for gap and beat.

## Rough

- The suite's frozen-readout rows are n/a for this engine; its "stays alive" rows are on
  the shell's sheet, where M bits sit on the free cells and X and the bank are not shown.
- The probes and the ablation are on seeds 1 and 2 (choice seeds), 10,000-step lives, and
  score "presses right" over the last 2,000 steps, not the suite's late window.
- vmprobe's "alike" tables compare trials of equal length only; for gap (two lengths) the
  "other answer" column is empty.
- Distract, serial-add and majority are mine, written for this test; the calculation
  battery and register-machine worlds were not on main.
- `--input-noise` and the three worlds touch shared files (`src/world/plug.*`,
  `src/world/task.*`, `src/run/run_flags.cpp`); small and additive, merged by hand with main
  once already.
- Speed figures are with other agents' runs on both machines.
