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
| 4 | time | order; gap; delayed echo; beat; sequence; functions of the last few cues | order 0.70 to 0.84; a cue is held 20 ticks; everything needing a longer hold fails |
| 5 | space and what stays the same | the invariance battery: place, scale, turn, thickness, clutter, viewpoint | place fails (0.25); the rest being built |
| 6 | sound | the invariance battery's tune, tempo and pitch rows | being built |
| 7 | acting | bandit; switching bandit; chain; track; catch; forage; mountain car; pendulum; cartpole; snake | catch 0.32 (chance 0.25); cartpole below a coin; the rest being built |
| 8 | keeping and adding | second task after the first; retest; idle; capacity; a changed rule; damage | second task 0.99 with the first kept at 0.98; idle loses nothing |
| 9 | relation, number, parts and wholes, chance, context | same/different; odd one out; counting; grammar; noisy answers; context switch | being built |
| 10 | no teacher | predict the next input; fill in a missing part; notice the odd one | hole |
| 11 | real data | MNIST digits; spoken digits or tones; characters of text | being built (MNIST first) |
| 12 | the long game | a maze; snake on a big board; several tasks interleaved for a whole life | hole |

## Milestones worth naming

- **Every row of tiers 1 to 4 on one set of rules.** Not yet: nothing holds a cue past 20
  ticks.
- **MNIST.** A linear classifier on the raw pixels gets about 92% and a small two-layer
  network about 97%, each in well under a minute on a CPU. The first target is to pass the
  linear classifier with a sheet that was never taught and a readout that only ever hears
  good and bad; the second is the two-layer network. Neither of those baselines can cope
  with a digit that is moved or turned; tier 5 is where a sheet could do something they
  cannot.
- **Cartpole balanced.** A classic control task learned from good and bad alone.
- **One life, every tier.** The same sheet meeting worlds from each tier in turn without
  being reset.

## Holes

Kinds of task with no world yet: learning with no teacher (tier 10); the long game (tier
12); output that is a whole pattern or an amount instead of a button; being shown the answer
(imitation); damage to the sheet. Each needs a world before any design can be said to work
on "any task".
