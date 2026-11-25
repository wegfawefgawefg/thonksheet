# The rest of the map (2026-11-25): tiers 9 and 10, being shown, damage

UNFINISHED. The job was wound up part way through the runs (the owner cut the agent count).
What is measured: level (b), the readout rule on a frozen sheet, for 36 of the 42 world
settings on seeds 5-12; level (c) table neurons for 34 of them. Not measured on the full
seeds: level (a) (the linear reader as born, and the shortcut readers), the minimal engine,
the no-teacher reader after a life, odd-event, and both protocols (being shown, damage).
Single-seed and two-seed spot checks of those are below, marked. How to finish: at the end.

Worlds, definitions and how they are built: `docs/curriculum.md` ("The rest of the map"),
`docs/notes-world.md` ("The rest of the map"), `scripts/map_worlds.py`.

## What was built

- 21 worlds, each with an easy and a hard setting, all on one trial script
  (`src/world/task_script.cpp` + one file per family), integer and seeded: same/different side
  by side and one after the other, odd one out, bigger, analogy; how many, more or fewer; Reber
  grammar, balanced brackets, parts of a figure; noisy reward, rare cue, noisy lines; context
  line, unsignalled rule switch; copy, recall after a filled gap, n-back; no teacher: predict
  the next input, fill in a blanked part, notice a break in a cycle.
- Two life protocols: `--shown N` (the right button's cells made to fire for the last N ticks
  of each answer step, no reward, then a test with no help) and `--damage P` / `--damage-tile 1`.
- An engine hook for them: `blob.silenced` and `blob.forced`, null by default, ANDed out of /
  ORed into the fired word in the fire step of all three engines. Off, every engine's csv is
  byte-identical to before (checked: order, 3,000 steps, full, minimal, table, readout mode 5).
  No urge array was on main; `forced` is what being-shown uses.
- `blobprobe linear` reads the new worlds (`task.probe_marks`), and fits the same reader to the
  task's raw input lines over the last 16 steps and to their brightness (one feature per
  count): the shortcut check.
- `scripts/map.sh` scores everything three ways; `scripts/suite.py` has the rows (after the
  old ones, "map: ...").

## Coin and scripted player

From the level (b) lives, seeds 5-12, late score (mean of the last 4 of 40 windows). The
scripted perfect player scores 1.00 on every taught world. Coin flips (ties are no move):

| world | coin | world | coin | world | coin |
|---|---|---|---|---|---|
| same easy / hard | 0.50 / 0.50 | how-many easy / hard | 0.33 / 0.20 | noisy-lines easy / hard | 0.50 / 0.24 |
| same-then easy / hard | 0.49 / 0.49 | more-fewer easy / hard | 0.48 / 0.50 | context easy / hard | 0.50 / 0.25 |
| odd easy / hard | 0.32 / 0.24 | grammar easy / hard | 0.50 / 0.50 | switch easy / hard | 0.49 / 0.49 |
| bigger easy / hard | 0.50 / 0.48 | brackets easy / hard | 0.50 / 0.47 | copy easy / hard | 0.50 / 0.24 |
| analogy easy / hard | 0.49 / 0.30 | parts easy / hard | 0.24 / 0.16 | recall easy / hard | 0.50 / 0.25 |
| noisy-echo easy / hard | 0.50 / 0.50 | rare-cue easy / hard | 0.49 / 0.49 | n-back easy / hard | 0.50 / 0.49 |

No-teacher worlds (seed 1, 20,000 steps, stand-in): predict-next coin 0.50, perfect 1.00 (the
stand-in sees its input; a real predictor's ceiling is about 0.92); fill-in coin 0.00, perfect
0.00 (the stand-in cannot see hidden cells; with the cells shown it scores 1.00, which checks
the count); odd-event coin 0.00, perfect 0.00 (it predicts its input exactly, so nothing
surprises it). Rare-cue is scored balanced, so "always button 0" is 0.50.

## The three levels (what exists)

(b) the readout rule on a frozen sheet (`--g.learn_mode 5 --g.readout_evidence 1
--g.readout_shift 4 --g.readout_balance 1 --motor-spread 128`, the same for every world), its
learning-off twin, and t of the paired difference over 8 seeds; (c) table neurons with their
defaults and their twin. No-teacher rows: (b) is the plain full engine (design rule) and the
world's own score, not the readout. "-": not run.

| world | coin | (b) rule | twin | t | (c) table | twin | t |
|---|---|---|---|---|---|---|---|
| same-easy | 0.50 | **0.66** | 0.49 | 29.0 | 0.43 | 0.47 | -2.9 |
| same-hard (held out) | 0.50 | 0.53 | 0.48 | 3.5 | 0.42 | 0.46 | -2.2 |
| same-then-easy | 0.49 | 0.53 | 0.48 | 12.8 | 0.42 | 0.47 | -4.3 |
| same-then-hard | 0.49 | 0.51 | 0.50 | 0.9 | 0.45 | 0.45 | -0.3 |
| odd-easy | 0.32 | 0.39 | 0.31 | 9.7 | 0.29 | 0.31 | -1.6 |
| odd-hard | 0.24 | 0.21 | 0.23 | -2.0 | 0.22 | 0.22 | 0.1 |
| bigger-easy | 0.50 | **0.97** | 0.45 | 60.4 | 0.55 | 0.47 | 2.4 |
| bigger-hard | 0.48 | **0.68** | 0.48 | 14.8 | 0.45 | 0.47 | -1.3 |
| analogy-easy | 0.49 | 0.47 | 0.48 | -0.2 | 0.42 | 0.46 | -2.6 |
| analogy-hard | 0.30 | 0.32 | 0.31 | 0.2 | 0.30 | 0.32 | -2.8 |
| how-many-easy | 0.33 | 0.37 | 0.32 | 8.2 | 0.29 | 0.31 | -2.1 |
| how-many-hard | 0.20 | 0.18 | 0.19 | -0.4 | 0.19 | 0.18 | 1.6 |
| more-fewer-easy | 0.48 | **0.70** | 0.48 | 20.0 | 0.42 | 0.48 | -9.9 |
| more-fewer-hard | 0.50 | 0.56 | 0.48 | 10.1 | 0.43 | 0.48 | -6.4 |
| grammar-easy | 0.50 | 0.48 | 0.48 | 0.3 | 0.42 | 0.46 | -3.0 |
| grammar-hard | 0.50 | 0.48 | 0.48 | 0.5 | 0.45 | 0.48 | -1.4 |
| brackets-easy | 0.50 | 0.51 | 0.48 | 2.1 | 0.44 | 0.47 | -3.0 |
| brackets-hard | 0.47 | 0.52 | 0.48 | 2.5 | 0.42 | 0.48 | -4.1 |
| parts-easy | 0.24 | 0.48 | 0.24 | 6.1 | 0.26 | 0.19 | 3.5 |
| parts-hard | 0.16 | 0.16 | 0.15 | 1.3 | 0.16 | 0.15 | 0.4 |
| noisy-echo-easy (80) | 0.50 | **0.77** | 0.52 | 7.2 | 0.68 | 0.45 | 2.7 |
| noisy-echo-hard (60) | 0.50 | 0.57 | 0.52 | 1.4 | 0.46 | 0.45 | 0.4 |
| rare-cue-easy (1 in 10) | 0.49 | **1.00** | 0.54 | 8.5 | **0.87** | 0.44 | 5.5 |
| rare-cue-hard (1 in 40) | 0.49 | **1.00** | 0.55 | 7.5 | 0.74 | 0.44 | 3.0 |
| noisy-lines-easy | 0.50 | **0.99** | 0.53 | 18.8 | 0.75 | 0.43 | 7.3 |
| noisy-lines-hard | 0.24 | 0.32 | 0.24 | 4.4 | 0.25 | 0.22 | 3.3 |
| context-easy | 0.50 | **0.93** | 0.49 | 59.4 | 0.51 | 0.47 | 1.4 |
| context-hard | 0.25 | 0.29 | 0.24 | 2.2 | 0.24 | 0.22 | 1.7 |
| switch-easy (500) | 0.49 | **0.94** | 0.49 | 171 | 0.57 | 0.47 | 5.0 |
| switch-hard (50) | 0.49 | 0.64 | 0.49 | 24.5 | 0.45 | 0.47 | -2.2 |
| copy-easy | 0.50 | 0.66 | 0.47 | 19.7 | 0.41 | 0.48 | -4.9 |
| copy-hard | 0.24 | 0.21 | 0.24 | -5.9 | 0.23 | 0.22 | 0.2 |
| recall-easy | 0.50 | 0.69 | 0.47 | 12.3 | 0.42 | 0.47 | -4.7 |
| recall-hard | 0.25 | 0.20 | 0.22 | -2.5 | - | - | - |
| n-back-easy | 0.50 | **0.71** | 0.48 | 45.1 | - | - | - |
| n-back-hard | 0.49 | 0.50 | 0.48 | 4.0 | - | - | - |
| predict-easy (design rule, share predicted) | 0.50 | 0.48 | 0.52 | -0.7 | - | - | - |
| predict-hard | 0.50 | 0.57 | 0.46 | 1.4 | - | - | - |
| fill-in-easy (hit less false alarm) | 0.00 | 0.14 | -0.06 | 4.2 | - | - | - |
| fill-in-hard (6 seeds) | 0.00 | 0.02 | -0.04 | 1.2 | - | - | - |

Not run on the full seeds: odd-event, the shown and damage rows, every level (a) number, every
minimal-engine number, table for recall-hard and n-back and later.

Reading (b), with the caveat that a t of 3 to 10 on a small gain is a real but tiny effect:

- **Learned well (0.9 or more)**: bigger-easy, rare cue (both, even 1 in 40), noisy-lines-easy,
  context-easy (exclusive-or in a new dress, like both-not-either's 0.88), switch-easy.
- **Learned part way (0.6 to 0.8)**: same-easy 0.66, bigger-hard 0.68, more-fewer-easy 0.70,
  noisy-echo at 80 in 100 (0.77), switch every 50 (0.64), copy of 2 (0.66), recall past one
  other cue (0.69), 1-back (0.71). Copy-easy, recall-easy and 1-back are the first things in
  this repo held over a step or more that a readout learns: all are a hold of one step.
- **At chance**: analogy (both), grammar (both), brackets (both), how-many-hard, odd-hard,
  same-then-hard, parts-hard, copy-hard, recall-hard, context with the context a step early,
  2-back. Held-out relations (same-hard 0.53, odd-hard 0.21) are at or near chance: nothing
  carries a relation to items it was not taught on.
- how-many-easy is 0.37 (chance 0.33) although brightness gives the answer exactly (the
  reader on brightness gets 1.00 on seed 5): a middle count needs a non-monotone readout of
  one quantity, which a sum of evidence per button cannot make.
- Table neurons with their defaults learn only the echo-shaped worlds (rare cue, noisy lines,
  noisy echo 80, switch every 500, parts-easy a little) and are at or below their twin on
  every relation, number, grammar and memory world.
- No teacher: the design rule's own predictions do not get better than its twin on predict-
  next; fill-in-easy shows a small positive fill (0.14 against -0.06, t 4.2), the only hint
  that the sheet's own wiring fills in a missing part.

## Spot checks of level (a) (not the report seeds)

Seed 5, 1,200 probe steps after 5,000 settling ticks; full engine as born; chance for the
reader is 1/classes. Sheet = whole-sheet reader; input = the reader on the raw input lines
of the last 16 steps (a perfect memory, linear); bright = on brightness alone.

| world | sheet | best 3x3 | input | bright |
|---|---|---|---|---|
| same-easy | 0.70 | 0.89 | 0.51 | 0.45 |
| bigger-easy | 0.99 | 1.00 | 0.99 | 0.56 |
| how-many-easy | 0.62 | 0.64 | 0.59 | 1.00 |
| how-many-hard | 0.25 | 0.34 | 0.27 | 0.37 |
| predict-easy | 0.86 | 0.87 | 0.85 | 0.45 |
| odd-event-easy | 0.62 | 0.70 | 0.62 | 0.62 |

Two seeds, quick lives (`out/map/smoke-quick`): rare-cue-easy sheet 1.00, input 0.99;
fill-in-easy 1.00 everywhere (the visible half names the pattern, as designed; the world's own
fill score is the real test); being shown echo k=2: test 0.51 for the readout rule, the
minimal engine and table neurons, each equal to its twin to the last digit: no engine learns
anything from being shown. Damage, 20 in 100 silenced after echo k=2: the readout rule 1.00
before and 1.00 at once; the minimal engine 0.90 to 0.62, table 0.83 to 0.53.

Shortcuts checked by design and by the input reader: relation items all light the same
number of lines (brightness constant); same-easy's input reader is at chance (0.51), so the
sheet's 0.70 is the sheet's own conjunctions; bigger is linear in the input (0.99), as
intended for the easy relation; how-many-easy is brightness by design, how-many-hard's area
band still leaks a little (0.37 against chance 0.20); brackets keep counts, length and ends
equal across answers. Not yet run on the full seeds, so these are not report numbers.

## What the failing kinds seem to need (for a reader designing mechanisms)

The current sheet plus the readout rule learns anything that is a fixed weighting of
features the sheet already has at the moment of the answer, including thin ones (a cue one
try in 40) and noisy ones (reward true 80 in 100, lines flipped 10 in 100). It is nowhere near
four kinds. **Anything carried over more than about one step** (copy of more than two, recall
past four cues, 2-back, the context shown a step early, grammar and brackets, which need the
whole string): the sheet keeps a cue for about a step and then it is gone, so these need a
store that is written on purpose and held (a latch, a slow population, or wiring that makes
a cue's trace last for a chosen number of steps) and, for copy, a pointer to the place in a
sequence. **Relations on items never taught** (same-hard, odd-hard, analogy): the rule learns
item-specific conjunctions, so what is learned on one pair says nothing about another; this
needs a comparison that is the same for every item, a "this line matches that line" feature
built once and shared across lines, which is the same missing sharing as the place wall.
**Counting over size and place** (how-many hard, and even easy at 0.37): the answer is a
non-monotone function of one pooled quantity, and the readout adds evidence per button
linearly, so it needs either an output that can pick a middle value (a number line of
buttons with neighbours inhibiting, or thresholded pooling units) or units that count
objects rather than cells. **No teacher**: the design rule's own predictions do not improve on
predict-next and barely on fill-in; a predictive rule that is scored and kept by how well it
predicts (the sheet's own surprise as the teacher) has not been built.

## Unfinished, and how to finish it

Raw outputs: `out/map/map/` on this PC (level (b) runs, the table runs copied in from the
second machine); the second machine's own folder is `~/thonk-map/out/map/map/` (pulled to
`out/mini/map/map/`). `out/map/map/table.txt` is the table as printed. Partial runs killed
mid-way were deleted, so `--resume` redoes them.

```
scripts/map.sh map --resume --levels b                 the rest of (b): odd-event, shown, damage (2 jobs)
scripts/map.sh map --resume --levels a                 every (a) number and the learned reader
scripts/map.sh map --resume --levels c                 the minimal engine and the rest of table
MINI_DIR=thonk-map scripts/mini.sh push
MINI_DIR=thonk-map scripts/mini.sh bg mapac 'python3 scripts/map.py map --resume --levels a,c --jobs 4'
scripts/map.sh map --print                             the table, once every run is in one folder
```

Before resuming on the second machine, copy `out/map/map/` there (or run `--levels a,c` only
there and copy back with `scripts/mini.sh pull` into `out/mini/`, then into `out/map/map/`).
Cost seen: a level (b) life took about 50 s here with 2 jobs (400 runs, about 3 h); the
second machine was at load 60-85 on 14 cores and did about 1.7 table runs a minute. The
whole set is about 1,600 runs.

## Rough

- Level (a) for held-out worlds fits and tests the reader only on held-out trials (it says
  whether the relation is readable among those items, not whether a reader taught on others
  carries over).
- The reader is ridge regression to one-hot labels with the largest output winning: it cannot
  pick a middle class from one quantity (how-many-easy's input reader 0.59 although the
  count is linear in the lines). One feature per count fixes that for brightness only.
- Fill-in's reader label is given by the visible half; only the world's own fill score tests
  filling in.
- Analogy's "add a cell" change makes B and the right candidate brighter.
- The minimal engine keeps no predictions, so its no-teacher scores mean nothing (shown as
  "-"); the suite's old sequence row reads 1.00 for it for the same reason.
- The hard settings were chosen by hand, not tuned on seeds 1-4.

## What should change elsewhere (not edited here)

- `docs/findings.md`: a new hint (one engine, the readout rule): the frozen sheet plus the
  readout learns thin and noisy versions of association (a cue one try in 40 at 1.00, reward
  true 80 in 100 at 0.77, flipped lines 0.99) and a context that flips a rule (0.93), and holds
  a cue for one step (copy of 2 0.66, recall past one 0.69, 1-back 0.71), but nothing with a
  hold of two steps or more, no relation on untaught items, no grammar, no analogy, no count
  over size. The "Time" wall gains these rows. Being shown the answer teaches no engine
  (quick check only); the readout rule shrugs off 20 in 100 of the sheet silenced (quick).
- `docs/task-map.md`: updated here for tiers 9 and 10 and the holes.
- `docs/ideas.md`: candidates from the paragraph above: a written-on-purpose hold (latch) for
  copy/recall/n-back/grammar; a shared match detector for relations on new items; an output
  that can choose a middle value; surprise-scored prediction for the no-teacher tier.
