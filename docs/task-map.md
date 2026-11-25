# The task map

The goal, in the owner's words (2026-11-25): "to find a set of rules that works on any
task, any task. so we need representation of all the types of tasks ... list possible
pattern shapes / domains, come up with varied envs for all of them, and then we just bash
our designs against them looking for something that works." And: "obviously as we solve
basic ones we need to graduate."

This file is the map: every kind of task we can name, the worlds that stand for it, and
where each stands. Exact task definitions live in `docs/curriculum.md`. Scores live in
`docs/scoreboard.md`. A kind of task with no world yet is a hole in the map; holes are
listed, not hidden.

## How a task can differ

A task is a choice along five lines. A rule that "works on any task" has to cope with every
value on every line, so the suite needs each value somewhere.

1. **What structure the pattern has.**
   - association: this cue, that answer
   - combination: the answer is a function of several things at once (and, or, parity)
   - magnitude: the answer depends on how much, smoothly (ramps, sine)
   - space: shape, and what stays the same when it moves, grows, turns, thickens, or is seen
     from another side
   - time: order, how long, how far apart, rhythm, something to hold over a gap, a sequence
   - sound: a pattern over frequency lines, and what stays the same when it is transposed or
     played faster
   - relation: same or different, odd one out, bigger than, A is to B as C is to what
   - number: how many
   - parts and wholes: things made of things, grammar, nesting
   - chance: answers that are right only most of the time, rare events, noise
   - context: the same cue means different things depending on something else
2. **How it is taught.**
   - nothing at all (only the input: predict it, find its structure)
   - good or bad after every answer
   - good or bad late, after several steps
   - by being shown the answer
   - by a verdict for each output (with two-valued outputs this is being shown the answer)
   - by acting on a world that answers back
3. **What has to come out.** One of a few buttons; one of many; an amount; a sequence of
   presses; a press at the right moment; a whole pattern.
4. **How much of it there is.** Two things to tell apart, or ten, or a thousand; clean or
   noisy; toy or real data.
5. **What has to survive.** Learning a second thing; coming back to the first; a long idle;
   a changed rule; damage to the sheet; a bigger sheet.

## The tiers

A design graduates a tier when, on 8 fresh seeds, it clearly beats its own learning-off twin
and chance on most of the tier's worlds, and does not lose a tier it had. Graduation is
recorded in `docs/scoreboard.md`. The tiers are ordered by what they need, not by difficulty
alone: a design may pass tier 4 before tier 3.

| tier | what it asks | worlds | best so far |
| --- | --- | --- | --- |
| 0 | stay alive: stable activity with input, without, and through changes | stage 0 runs, the aliveness rows of the suite | passed by all three engines |
| 1 | association | echo with 2, 4, 8, 16 cues; shuffled echo; go/no-go | passed: the evidence readout gets 2 to 8 cues, 16 at 0.31 |
| 2 | combination | exclusive-or; the function battery's N-point patterns, counting | exclusive-or 0.88; the rest being built |
| 3 | magnitude | functions of a number: step, tent, sine, cosine, sums, products | being built |
| 4 | time | order; gap; delayed echo; beat; sequence; functions of the last few cues | on one richer sheet (rich2) with nothing changed between them: order 0.85, gap 0.93; a cue is still held only about 20 ticks, so delayed echo and beat fail |
| 5 | space and what stays the same | the invariance battery: place, scale, turn, thickness, clutter, viewpoint; the seeing battery: name the shape, track a moving thing, count, a thing behind a cover | place fails (0.25); the rest being built |
| 6 | sound | the invariance battery's tune, tempo and pitch rows | being built |
| 7 | acting | bandit; switching bandit; chain; track; catch; forage; mountain car; pendulum; cartpole; snake | catch 0.32 (chance 0.25); cartpole below a coin; the rest being built |
| 8 | keeping and adding | second task after the first; retest; idle; capacity; a changed rule; damage | second task 0.99 with the first kept at 0.98; idle loses nothing |
| 9 | relation, number, parts and wholes, chance, context | same/different side by side and in turn; odd one out; bigger; analogy; how many; more or fewer; Reber grammar; brackets; parts of a figure; noisy reward; rare cue; noisy lines; context line; unsignalled switch; copy; recall past others; n-back (`scripts/map.sh`) | readout rule, frozen sheet: chance (1.00 at 1 in 40), noisy lines 0.99, bigger 0.97, context 0.93, switch every 500 0.94; same 0.66, more-fewer 0.70, copy of 2 0.66, recall past 1 0.69, 1-back 0.71; chance on analogy, grammar, brackets, counting over size, every relation on untaught items, every hold of 2 steps or more. Unfinished: no linear-reader or minimal-engine numbers yet (docs/results-2026-11-25-rest-of-the-map.md) |
| 10 | no teacher | predict the next input; fill in a missing part; notice the odd one (`scripts/map.sh`) | built; the design rule's own predictions do not beat its twin on predict-next (0.48 against 0.52); fill-in 0.14 against -0.06; odd-event not yet run |
| 11 | real data | MNIST digits; spoken digits or tones; characters of text | MNIST: a linear reader of the never-taught sheet 0.924 (linear on pixels 0.922, two-layer net 0.955); after a reward-free life under competitive learning 0.952; the sheet's own readout from good and bad 0.71 to 0.81, the same with or without that rule |
| 12 | the long game | a maze; snake on a big board; several tasks interleaved for a whole life | hole |

### The fixed reference

One ordinary network setting run on the same rows (`scripts/reference.sh`, idea 117): a
GRU of 128 units, backprop through 8 steps, told only good and bad. It is the bar for
"learnable": tier 1 all 1.00 (16 cues included), exclusive-or 0.97, tier 4's delayed echo,
beat, order and gap all 1.00, catch 0.72, tier 8 1.00. It fails the place never rewarded
(0.16), holds longer than 8 steps, and cartpole. A world the reference fails is either
truly hard or badly made; cartpole's "lean" reward and the place world are marked suspect
until checked.

Since the integration (2026-11-25) the GRU, the plain net and the memory engine have every
row in `docs/scoreboard.md`, on the fixed cartpole (GRU 798 steps per fall on lean, 77 on
sparse; memory 302 and 281). By tier, the GRU: tier 2 parity of 3 lines 0.78, of 4 lines
0.50 (the plain net 0.96 and 0.90); tier 7 bandits 1.00, track and forage 0.9 to 1.0,
mountain car 1.00, chain 0.01, pendulum 0.22; tier 9 same or different 0.87 to 1.00, odd
one out 0.97 and 0.48, bigger 1.00, brackets 0.99 and 0.90, grammar 0.70 and 0.60, copy
1.00 and 0.55, recall 1.00 and 0.99, n-back 0.97 and 0.94; tier 10 noisy and rare cues
1.00, context 1.00 and 0.89, switching 0.75 and 0.50. No reference learns analogy, the
shown-the-answer rows, fill-in, or any held-out place, turn or size. Those rows are
suspect until something passes them.

### The memory engine

`blobrun --engine memory` (idea 123) passes tiers 1, 2 (exclusive-or), 4 (order, gap,
delayed echo, beat), 7 (catch 0.99; cartpole 109 and 279 steps per fall) and 8 with one
setting, by storing what it has seen. So "every row of tiers 1 to 4 on one set of rules"
is met, by a lookup table, and the milestone needs sharpening: rows where the asked case
was never shown (idea 149). The place world is the only such row today and it fails there.

### After the wind-up (2026-11-25, night)

Best per tier with the mechanisms that just landed (each still to be confirmed on the full
seeds and, above all, together under one setting):
- tier 4 (time): delayed echo 0.91 to 1.00, order 0.88 to 0.99, gap 0.68 to 0.98, with a
  slow variable in the neuron or with time and hold cells. Beat and sequence unrun.
- tier 5 and 6 (what stays the same): the invariance battery exists; every engine and both
  references fail the held-out rows. The old place row is retired as a trap.
- tier 7 (acting): with a critic, bandit 1.00, chain 0.97, cartpole 89 steps per fall;
  cartpole now uses the fixed step by default.
- tier 2 and 3: and, or, majority to 8 lines; parity to 2.
- tier 9 and 10: 21 worlds built; association under noise and context learned, the rest
  at chance.
- tier 8 (keeping): 8 to 14 pairs held, gone by 10,000 steps.

## Milestones worth naming

- **Every row of tiers 1 to 4 on one set of rules.** Not yet: nothing holds a cue past 20
  ticks.
- **MNIST.** A linear classifier on the raw pixels gets about 92% and a small two-layer
  network about 97%, each in well under a minute on a CPU. The first target is to pass the
  linear classifier with a sheet that was never taught and a readout that only ever hears
  good and bad; the second is the two-layer network. Neither of those baselines can cope
  with a digit that is moved or turned; tier 5 is where a sheet could do something they
  cannot.
  *Where it stands:* the first target is met only by an offline reader (0.924), not by a
  readout that hears only good and bad (0.71). Moved by 2 pixels every learner, plain ones
  included, keeps about two thirds of its score.
- **Cartpole balanced.** A classic control task learned from good and bad alone.
- **One life, every tier.** The same sheet meeting worlds from each tier in turn without
  being reset.

## Holes

Kinds of task with no world yet: the long game (tier 12); output that is an amount
instead of a button; a changed rule on the *same* lines and buttons (today's second
task uses new ones, so nothing interferes). Each needs a world before any design can be said to work on
"any task". Built 2026-11-25 (`scripts/map.sh`, docs/curriculum.md "The rest of the map"):
no teacher (tier 10), being shown the answer (`--shown N`) and damage to the sheet
(`--damage P`, `--damage-tile 1`); their full-seed runs are unfinished (quick check: no
engine learns from being shown; the readout rule loses nothing to 20 in 100 silenced).
Output over several steps is now covered by copy (one press a step).
Output that is a whole pattern is covered by the calculation battery (`--task calc`,
`scripts/calc.sh`, docs/results-2026-11-25-calculation.md): with a verdict per bit the
working rule learns pattern answers that are weighted sums of the input lines at small
sizes, and nothing that needs a product of distant lines; with one verdict for the whole
answer it holds two bits. Every member holds out a quarter of its cases, so this is also
the first battery a lookup table fails (memory: seen 0.88 to 0.99, held out near the most
common answer).
