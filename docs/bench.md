# The bench

How ideas get tried. Agreed with the owner 2026-11-24.

## The goal the bench serves

Find out what makes general learning possible in a brain-like machine, and build the
smallest efficient version of it: a persistent thing you pump input into, which keeps
learning, has a sense of time, and forms features of any kind (in time, shape or place)
without the structure being designed in. The sheet, Echo and the current mechanisms are a
vehicle. An idea is judged by whether it moves toward that, not by one task score.

## The method

1. The coordinator writes an idea into `docs/ideas.md` with a prediction in
   `docs/predictions.md`.
2. One agent takes one idea, a small piece of the machine, in its own folder. It
   implements it, runs the suite, and writes a results doc. Short jobs, fresh agent each.
3. The coordinator reads the scorecard, merges or drops, records the result against the
   idea's number, and picks what to try next from what was learned.
4. Every so often the things that helped are run together, because some mechanisms may only
   work in combination.

## The suite

One script runs everything and writes one scorecard: `scripts/suite.sh <name> [blobrun
flags and --g.name value ...]`, into `out/scorecards/<name>.csv`; `--quick` is a smoke
check on 2 seeds. How each row is run: `docs/notes-world.md`, "The suite".
Fixed seeds: 1 to 4 are for choosing settings, 5 to 12 are for reporting. Every number is
shown beside the same engine with learning off and beside coin flips.

What it measures, in the owner's words where he gave them:

| property | test |
| --- | --- |
| learns at all | echo with 2, 4, 8 cues; shuffled echo |
| a sense of time | delayed echo; beat; sequence (surprise falling) |
| acting in a world | catch; cartpole |
| reach | cue identity readable at 1 to 7 tiles; echo with cue and button 1, 2, 3, 5 tiles apart |
| features nobody designed in | frozen sheet plus readout on: which of two cues came first; both but not either alone; a gap of N ticks; the same shape in a new place |
| keeps learning, keeps what it had | learn A, learn B, retest A; capacity as pairs grow |
| does not wreck itself idle | teach, unplug for a long stretch, retest |
| room | share of connections at top strength and of neurons at their cap after a long life |
| stays alive | rate against target, silent and stuck neurons, rhythm |
| fits the machine | ticks per second on one core at three sizes; bytes per neuron |

The scorecard is one row per test with mean and spread over the report seeds. All
scorecards are collected in `docs/scoreboard.md` (`scripts/scoreboard.sh`), one column per
engine or variant.

## Engines

An engine is one complete small learner. It lives in its own folder under `src/` and sits
behind the same `Blob` shell the plug and tasks already use (as `src/world/standin.cpp` and
the table neurons do), so the suite drives any engine unchanged. `blobrun --engine name`
picks it. A variant of an existing engine is a set of `--g.name value` numbers with a name.

An agent working on an engine edits only that engine's folder and adds its name to the
engine list. That is what lets many agents work at once without colliding.

## One setting for everything

The owner, 2026-11-25: "you often have to change arch to get a nn to learn a task right.
which is kind of part of the problem here. thats what were trying to engineer out."

So a design is one fixed set of numbers, and it is scored with that one set on every world
in `docs/task-map.md`. No flag may change from task to task except what belongs to the
world itself (how its input is laid on the sensor sheet, how many buttons it has). A design
that needs long delays for one task and short ones for another has not passed both; it has
to carry both at once. The headline number for any design is how many worlds it passes with
nothing changed between them.

The same rule applies to the reference we compare against. It is not "the best network for
each task". It is one ordinary network, one size, one training recipe, fed the same stream
through the same plug and told only good and bad, run on the whole map unchanged
(`mlp-fixed` and `gru-fixed` in the scoreboard). Where that fails without being redesigned
is where a design of ours could be doing something different in kind.

## Nothing frozen

A design that graduates learns everywhere, all the time: no part switched off, no training
phase and testing phase, no reset between tasks. Runs with the inside of the sheet held
fixed are a diagnostic, for finding where learning happens; they are never the design.
Scorecards say which they are. The reference numbers that matter are with everything
learning.

## Honesty

- Predictions before results. Nulls and failures stay on the list.
- Settings are chosen on seeds 1 to 4 and reported on 5 to 12.
- No wiring at birth that solves a task.
- If a plain method beats a brain-style one, that is a result to build on.
- A finding that only holds at a cost the hardware cannot pay is not a finding.

## Sharing the machine

The owner capped the CPU clock at about 4.6 GHz on 2026-11-24 because agents' runs were
taking it to 96 C. So: an agent runs at most 2 single-threaded jobs at once, always under
`nice -n 19`. Speed numbers taken while other agents are running, or after the cap, are
not comparable with earlier ones: say which it was beside every ticks-per-second figure,
and take the figures that matter on a quiet machine.
