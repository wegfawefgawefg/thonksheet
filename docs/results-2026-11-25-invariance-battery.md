# The invariance battery: baseline (2026-11-25)

Idea 101. The owner: "scale invariance, rotational invariance, position invariance, time
invariance, frequency invariance etc. we should have envs for testing all of these and keep
trying more mechanics until we find something that can do all of them." This job built the
worlds and the measuring stick, not mechanisms. Definitions: `docs/curriculum.md`,
"Invariance". Stopped early when the owner cut the number of agents: what is missing is
listed under "Unfinished", with the lines that finish it.

## Short version

- Ten task kinds, each with versions that are rewarded and versions never rewarded (the
  only ones scored). Seven picture kinds (position, scale, rotation, bright, clutter, mixed,
  view) also have a "with motion" variant, where half the life is unrewarded stretches of
  one shape drifting through every version. Coin flips score 0.49-0.50 over whole lives,
  the scripted player 1.00, on every row.
- **Nothing learns a held-out version of position, rotation, the tunes (shift, tempo,
  pitch), or the turning 3D view**, at any level, in any engine, including a plain MLP and
  GRU trained by backprop behind the same plug. Scale, mixed and clutter are partly carried
  (0.6-0.8); dim shapes are carried by everything except our readout.
- **Motion does nothing yet** (as it should: nothing in any engine ties together what
  follows what). With a frozen sheet it costs 0.00-0.05, because half the life has no
  trials.
- **The suite's place world is a trap**, not just hard: a reader that remembers the nearest
  taught picture gets 0.23 on it, a per-cell vote 0.09. That is the 0.16-0.25 every learner
  scores. The battery's picture rows are not traps by the same test; the three time rows
  are, unavoidably (below), so their scores must be read against the nearest-pattern number.
- **Log-polar map** (`--bitmap-map logpolar`): at level (a), held-out scale goes from 0.61
  to 0.91 (sheet) and rotation from 0.45 to 0.73. It gives scale away entirely to a linear
  reader of the input (0.51 to 1.00), so with the map on, the scale row stops testing
  invariance. Position gets worse (0.40 to 0.42 sheet, 0.48 to 0.43 motors' view), as it must.

## The rows

Picture kinds: a 16x12 bitmap at `--sensor-side 1`. Two 7-pixel shapes that differ in two
pixels, `.### ##.. .#.. .#..` and `...# #### .#.. .#..`, drawn at 2x2 lines a pixel unless
the change is size. Time kinds: three notes and a chord; tune 0 is A B C ABC, tune 1 C B A
ABC.

| row | versions | held out (scored) |
|---|---|---|
| place (the suite's) | 2x2 diagonal pair at 9 places on 4x4 | 3 drawn per life |
| position | 15 places on a 2-line grid | 4 drawn per life |
| scale | 1x, 2x, 3x about the middle | 1x |
| rotation | 4 quarter turns x plain/mirrored | 1/2 and 3/4 turns, both |
| bright | 4, 2, 1 lines of each pixel's 2x2 | 1 (control: free for linear readers) |
| clutter | 0, 3, 8, 16 extra 2x2 blocks | 8, 16 |
| mixed | 1x, 2x at 2 left and 2 right places | 1x right, 2x left |
| view | 3D post-and-arm objects, 8 angles about the vertical | 3 in a row (1/4 to 1/2 turn) |
| shift | tune starts 0, 4, 8 ticks into its step | 4 |
| tempo | notes 2, 4, 6, 8 ticks | 2 (faster than any taught) |
| pitch | tune on columns 2v, 2v+2, 2v+4, v = 0..5 | 2 drawn per life |
| + motion | the picture rows, 50% of steps drifting, 8-step stretches | as the row |

## The baseline: three levels

Seeds 5-12, mean of 8. Level (a): `blobprobe invariance`, the sheet as born, a ridge reader
fitted on trials of the rewarded versions, scored on later trials of the same versions
("same", is it readable) and on held-out versions ("held", does the code carry over);
chance 0.50 (the reader always answers). Level (b): the readout (learn_mode 5, evidence,
128 cells a button), frozen sheet, 40,000 steps. Level (c): the whole engine learning
(full engine: learn_mode 6 with the same readout; minimal and table: their defaults;
mlp and gru: the fixed networks). Lives score late (last tenth); held = held-out versions,
taught = rewarded ones. The learning-off twin scores 0.46-0.51 held out on every row and
coin 0.48-0.51.

### Full engine

| row | a input s/h | a nearest held | a sheet s/h | a motors' view s/h | b held | b taught | c held | c taught |
|---|---|---|---|---|---|---|---|---|
| place | 0.93/0.17 | (0.23, exact) | 0.99/0.24 | 0.76/0.30 | 0.25 | 0.74 | 0.36 | 0.58 |
| position | 1.00/0.42 | not run | 0.96/0.40 | 0.77/0.48 | 0.49 | 0.74 | 0.49 | 0.65 |
| scale | 1.00/0.51 | not run | 1.00/0.61 | 1.00/0.68 | 0.69 | 0.99 | 0.77 | 0.91 |
| rotation | 1.00/0.50 | not run | 1.00/0.45 | 0.99/0.36 | 0.36 | 0.94 | 0.35 | 0.81 |
| bright | 1.00/1.00 | 1.00 | 1.00/1.00 | 1.00/0.52 | 0.58 | 0.98 | 0.63 | 0.97 |
| clutter | 1.00/0.91 | 0.88 | 1.00/0.90 | 0.98/0.80 | 0.75 | 0.95 | 0.67 | 0.79 |
| mixed | 1.00/0.50 | 0.58 | 1.00/0.55 | 0.93/0.77 | 0.72 | 0.88 | 0.63 | 0.77 |
| view | 1.00/0.43 | not run | 1.00/0.56 | 1.00/0.77 | 0.57 | 0.97 | 0.33 | 0.89 |
| shift | 1.00/0.06 | (0.00, sim) | 0.80/0.75 | 0.80/0.72 | 0.59 | 0.56 | 0.47 | 0.48 |
| tempo | 1.00/0.00 | (0.00, sim) | 0.93/0.47 | 0.90/0.47 | 0.45 | 0.71 | 0.46 | 0.50 |
| pitch | 1.00/0.07 | 0.38 (5 seeds) | 0.74/0.55 | 0.69/0.56 | 0.47 | 0.51 | 0.47 | 0.48 |
| position + motion | | | | | 0.48 | 0.70 | 0.48 | 0.62 |
| scale + motion | | | | | 0.68 | 0.99 | 0.65 | 0.81 |
| rotation + motion | | | | | 0.36 | 0.90 | 0.34 | 0.78 |
| bright + motion | | | | | 0.53 | 0.95 | 0.60 | 0.95 |
| clutter + motion | | | | | 0.72 | 0.92 | 0.63 | 0.73 |
| mixed + motion | | | | | 0.68 | 0.84 | 0.66 | 0.76 |
| view + motion | | | | | 0.56 | 0.95 | 0.33 | 0.82 |

Spreads (sd over 8 seeds) of the held-out late score: 0.02-0.06 on most rows; scale 0.09
(b) and 0.13 (c), view 0.11 (b) and 0.09 (c), place 0.08 (b). Level (a) is the same for
the motion rows (nothing learns while the probe runs).

Motion minus plain, paired by seed, held out: level (b) -0.00 to -0.05 (bright t -4.5,
clutter -7.4, mixed -4.3; the rest |t| < 1); level (c) scale -0.12 (t -2.3), clutter -0.04
(t -2.8), mixed +0.03 (t +2.8), the rest |t| < 1.5. The losses are fewer trials, not motion.

### Other engines, level (c), and the minimal and table sheets at level (a)

| row | minimal a sheet s/h | minimal c held/taught | table a sheet s/h | table c held/taught | mlp c held/taught | gru c held/taught |
|---|---|---|---|---|---|---|
| place | 0.94/0.19 | 0.41/0.40 | 0.89/0.20 | 0.39/0.47 | 0.16/1.00 | 0.16/0.96 |
| position | 0.84/0.42 | 0.36/0.39 | 0.82/0.42 | 0.44/0.43 | 0.37/0.99 | 0.37/0.93 |
| scale | 0.96/0.52 | 0.23/0.37 | 0.96/0.58 | 0.42/0.45 | 0.53/1.00 | 0.55/1.00 |
| rotation | 0.96/0.41 | 0.40/0.36 | 0.91/0.45 | 0.42/0.46 | 0.48/1.00 | 0.48/1.00 |
| bright | 0.95/0.67 | 0.29/0.41 | 0.95/0.85 | 0.47/0.46 | 0.95/1.00 | 0.96/1.00 |
| clutter | 0.71/0.73 | 0.41/0.25 | 0.95/0.81 | 0.44/0.43 | 0.89/1.00 | 0.89/1.00 |
| mixed | 0.84/0.55 | 0.38/0.38 | 0.86/0.51 | 0.45/0.43 | 0.54/0.99 | 0.52/0.99 |
| view | 0.94/0.52 | 0.43/0.40 | not run | 0.43/0.43 | 0.61/1.00 | 0.67/1.00 |
| shift | 0.51/0.51 | 0.27/0.35 | not run | 0.43/0.44 | 0.50/0.50 | 0.50/0.75 |
| tempo | 0.51/0.50 | 0.44/0.43 | not run | 0.45/0.44 | 0.50/0.49 | 0.48/0.67 |
| pitch | 0.50/0.51 | 0.44/0.44 | 0.62/0.48 | 0.43/0.44 | 0.50/0.52 | 0.25/0.76 |
| motion rows | | 0.24-0.43 / 0.24-0.41 | | 0.43-0.46 / 0.42-0.44 | as plain | as plain |

The minimal and table engines, as configured here (one sensor cell a line, packed 3x3
buttons, their own defaults), learn **none** of the rows, not even the rewarded versions;
their rows fail before invariance comes into it. Their sheets as born read the pictures
(0.82-0.96 same) but not the tunes (0.50-0.62: no delays). mlp and gru motion rows: within
0.02 of plain except view (mlp 0.57 against 0.61).

### Which level fails, per kind (full engine)

- **position, rotation**: already at (a). Not even the sheet as born carries a shape to
  a new place or turn (0.40, 0.45; motors' view 0.48, 0.36), the readout learns the taught
  versions (0.74, 0.94) and is at or below chance on the rest. Rotation is below chance at
  every level (0.36): the held-out half turn of one shape looks more like the other shape.
  The fixed networks fail the same way (0.37, 0.48).
- **scale (small)**: partly carried at every level: input 0.51, sheet 0.61, motors' view
  0.68, readout 0.69, whole engine 0.77. The sheet blurs, and blur carries a little size.
  The fixed networks get less (0.53-0.55). Not solved anywhere.
- **mixed**: (a) whole sheet 0.55 but motors' view 0.77; (b) 0.72, (c) 0.63. Each size and
  place was taught separately, so a local reader of the far tiles carries some of it.
- **view**: (a) 0.56 (view 0.77), (b) 0.57, (c) 0.33: the whole engine learning makes the
  held-out views worse than chance. GRU 0.67, MLP 0.61.
- **bright** (control): free at (a) for any linear reader (1.00), lost at the motors'
  view (0.52): a dim shape does not reach the motor cells' neighbourhood. So the readout
  fails it (0.58) while the fixed networks, which read the lines directly, pass (0.95).
  This is a reach/gain failure of our sheet, not a missing invariance.
- **clutter**: mostly free (input 0.91, sheet 0.90); readout 0.75, whole engine 0.67; the
  fixed networks 0.89.
- **shift**: the sheet as born carries it (0.75 held against 0.80 same: its own smoothing
  in time), but the readout does not even learn the taught starts (0.56) and the GRU learns
  them (0.75) without carrying them (0.50).
- **tempo**: fails at (a): taught speeds read 0.93, the faster one 0.47. Readout learns
  taught 0.71, held 0.45. GRU the same shape (0.67, 0.48).
- **pitch**: weak at (a) even taught (0.74, held 0.55); nothing learns it (readout taught
  0.51); the GRU learns taught (0.76) and goes to 0.25 held out, the trap direction.

## The place world is a trap (checked cell by cell)

`out/invariance/place_trap.py` prints every shape's cells at every place. "\" at (x, y)
lights (x, y) and (x+1, y+1); "/" lights (x+1, y) and (x, y+1). Every picture shares a cell
with the other shape at about twice as many places as with its own (inner places: 4 against
2; edges: 3 against 1; corners: 2 against 0 or 1). And over all 9 places every inner cell is
lit equally often by both shapes (2 and 2; edge cells 1 and 1), so holding a place out
removes exactly one "own shape" vote from each of its two cells and tips both toward the
other shape. Over all 84 ways to hold out 3 places: the nearest taught picture is right
0.23 of the time, a per-cell vote 0.09. The measured scores (our engines 0.19-0.41, the MLP
and GRU 0.16, the linear reader of the input 0.17) are this trap, not a failure to share.
The coordinator's reference agent guessed right. Place should not be read as "position
invariance fails"; the battery's position row is the test (nearest-pattern and input
linear at 0.42, at chance-ish).

The battery's own rows by the same test (nearest taught input pattern, held out):
bright 1.00, clutter 0.88, mixed 0.58 (8 seeds); pitch 0.38 (5 seeds); position, scale,
rotation, view not run (unfinished, below; their input linear reader is 0.42-0.51, and the
shape pair was chosen with a class-mean reader at 0.50 on all four, so they are not
expected to be traps). The time kinds are traps by construction: in a Python check
(`out/invariance/tune_trap.py`) of all 15 pairs of note orders, with or without the closing
chord, a tune shifted, stretched or transposed by more than a note shares no (line, tick)
with its own class at the taught versions and some with the other tune, so the nearest
pattern is the other tune (0.00 for every pair on shift; 0.00-0.50 on tempo; 0.03-0.34 on
pitch). Shifts shorter than a note were tried and are free (1.00). So for shift, tempo and
pitch, 0.50 is not the floor of a memoriser: the floor is the nearest-pattern number, and
any score above it is something carried. The sheet as born gets 0.75 on shift against a
nearest of 0.00.

## The log-polar test (level a, full engine, seeds 5-12)

`--bitmap-map logpolar` lays the 16x12 bitmap out as 16 angles by 8 rings about its middle
(ring edges 1.44 times apart; `src/world/plug_map.cpp`, integer tables).

| row | plain: input / nearest / sheet / view (held) | log-polar: input / nearest / sheet / view (held) |
|---|---|---|
| scale | 0.51 / - / 0.61 / 0.68 | **1.00 / 1.00 / 0.91** / 0.65 |
| rotation | 0.50 / - / 0.45 / 0.36 | 0.72 / 0.75 / **0.73** / 0.62 |
| position | 0.42 / - / 0.40 / 0.48 | 0.48 / 0.38 / 0.42 / 0.43 |
| mixed | 0.50 / 0.58 / 0.55 / 0.77 | 0.58 / 0.75 / 0.54 / 0.54 |
| view | 0.43 / - / 0.56 / 0.77 | 0.65 / 0.34 / 0.57 / 0.51 |

Same-version reading stays 0.97-1.00 everywhere. What it says:

- In log-polar, the small shape is not a slide the reader must undo: the map's inner rings
  sample the centre pixels repeatedly, and a linear reader of the mapped lines gets the
  held-out size right outright (1.00). The scale row with the map on tests nothing; with
  the map, a battery would need scale held out at the large end, or a coarser map.
- Rotation becomes a slide of 4 lines a quarter turn and is partly carried (0.73), not
  solved: a linear reader does not undo a slide either.
- Position and the place-plus-size mix lose: the map is about one centre.
- So the map does what the brain's map is said to do for size, about one fixation point,
  and needs eye movements (idea 105) to be about anything else. It does not remove the
  need for something that carries a pattern along a slide.

## Redesigns (what had to change before the battery measured anything)

Checked at level (a) on seed 5 with the input lines read beside the sheet. A row where a
linear reader of the input lines carries over does not test invariance.

- **The shapes.** The first pair (two shared pixels) left scale open (input lines carried
  every size at 1.00). A second pair (five shared) closed it for 3x but opened rotation
  (0.99). A search over all pairs of the 96 chiral 7-pixel 4x4 shapes (12 up to turning and
  mirroring), with the reader in its strong-ridge limit (a class-mean reader on
  standardized lines, which is what the probe picks on noiseless pictures;
  `out/invariance/pair_search.py`), found no pair for which the input lines fail every
  scale split: 2x and 3x held out are always carried. Pairs that differ in one or two
  pixels fail with 1x held out and also fail position, rotation and mixed. The battery uses
  one. So scale holds out the small size.
- **Brightness** is carried at 1.00 by a linear reader for any shapes (a subset of lit
  cells keeps the sign of a linear sum). Kept, as a control, because our readout fails it.
- **Clutter** with 0-8 blocks around the shape was carried at 1.00; raised to 3-16 blocks
  among the gaps it is still 0.91. Kept as a near-control.
- **Shift**: a 40-tick window left the earliest tune 28 ticks before the ask and the sheet
  had lost it (same 0.55); starts 2 ticks apart were free (1.00). Now one 20-tick window,
  starts 0, 4, 8.
- **Tempo**: the middle speed held out was carried by the sheet (0.79 against 0.86 same);
  the fastest held out fails, and is the default.
- **Pitch**: 12 pitches, 4-cell 4-tick notes read only 0.56 on taught pitches; now 6
  pitches, 8-cell columns, 6-tick notes (0.74).
- **View**: two views held out next to taught ones were carried at 0.82; three in a row
  give 0.56.

## Unfinished

- **Nearest-pattern column at level (a)** for position, scale, rotation, view, shift, tempo
  (the re-run was stopped at 20 of 88 runs; pitch seeds 10 and 11 were cut off mid-write
  and have no level (a) numbers now; pitch's level (a) above is seeds 5-9 and 12). Finish:
  `scripts/invariance.sh blob --levels a` (88 probe runs, about 25 minutes here at 2 jobs).
  Levels (b) and (c) are complete and untouched by it (`--resume` would skip finished runs
  but does not know about the new column, so run it without).
- **Table engine level (a)** for view, shift, tempo and the last pitch seeds (stopped at 200
  of 232). Finish on stevejobs: `MINI_DIR=thonk-invariance scripts/mini.sh bg inv-table
  'python3 scripts/invariance.py table --table --jobs 2 --resume'`. Level (c) is complete.
- Not started: the battery at `--inv-shapes 4`; motion with the whole sheet learning for
  longer lives; the suite rows (added to `scripts/suite.py`, not yet run in any scorecard).

## Script lines

```
scripts/invariance.sh blob                       full engine, levels a b c (149 min here, 2 jobs)
scripts/invariance.sh blob --quick               2 seeds, short lives (4 min)
scripts/invariance.sh minimal --engine minimal   levels a and c (68 min on stevejobs, 2 jobs)
scripts/invariance.sh table --table              (about 3 hours on stevejobs, loaded)
scripts/invariance.sh gru --engine gru           level c only (23 min on stevejobs); mlp 10 min
scripts/invariance.sh logpolar --levels a --rows position,scale,rotation,mixed,view --bitmap-map logpolar
scripts/invariance.sh show rotation              the pictures as text; any kind
scripts/watch.sh inv-rotation --inv-motion 50    in the viewer (owner)
```

Scorecards: `out/scorecards/invariance-{blob,minimal,table,gru,mlp,logpolar}.csv` (table
partial). Raw runs: `out/invariance/blob/` here; minimal and table on stevejobs in
`~/thonk-invariance/out/invariance/` (copied to `out/mini/invariance/`), gru, mlp and
logpolar in `~/thonk-invariance2/out/invariance/` (copied to `out/mini2/invariance/`).
Pilots and design checks: `out/invariance/pilot/`, `pair_search.py`, `tune_trap.py`,
`place_trap.py`.

## Reading

- The battery does what it was for: every kind except brightness and clutter defeats a
  linear reader of the input, the sheet as born reads every row's taught versions, and the
  held-out versions are lost somewhere between the input and the answer. That gives every
  later mechanism a row-by-row bar.
- Our sheet with no sharing fails position, rotation, tempo, pitch and the view, as
  designed. It partly carries small size, the size-place mix and time shift on its own,
  through blur in space and in time. Neither is solved anywhere.
- Ordinary fixed networks trained by backprop fail the same rows. Weight sharing, not
  backprop, is what a convolution adds; neither engine here has it. These rows measure the
  missing prior, not learning power.
- Motion is in place and measured at zero for everything, which is the baseline idea 102
  (the trace rule) and 110 (views tied by time) will be scored against.
- Where the motors can hear matters for invariance too: the motors' view often carries more
  than the whole sheet (mixed 0.77 against 0.55, view 0.77 against 0.56), because a reader
  of distant tiles sees a blurred picture. And a dim shape never reaches them (0.52), which
  is a gain problem (idea 108) before it is an invariance problem.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, wall "A new place": the 0.25 on place is a trap in the world, not a
  failure to share (nearest picture 0.23, per-cell vote 0.09, every learner 0.16-0.41).
  Restate the wall from the battery: a shape at a new place 0.49, a new turn 0.36, a faster
  tune 0.45, a new view 0.33-0.57, on the full engine at levels (b) and (c), and the same
  for a GRU (0.37, 0.48, 0.48, 0.67). Add: small size is partly carried (0.69-0.77), and a
  dim shape is lost at the motor cells (0.58) though any linear reader of the lines gets it.
- `docs/findings.md`, the headline: "a shape in a place it was never rewarded (0.25, below
  chance)" should cite the trap.
- `docs/task-map.md`: the invariance tier is built (`--task inv-*`, ten kinds, motion
  variants); note place's trap, and that the time kinds' floor is the nearest-pattern
  number, not 0.50.
- `docs/ideas.md`: idea 101 built and baselined; idea 104 (maps where the change is a
  slide) first test done: log-polar gives size away at the input (1.00), carries half of
  turning (0.73), costs position; idea 102 and 110 now have their worlds (the motion rows)
  and their baseline (motion changes nothing, -0.05 to +0.03). New candidate: a world fix
  for place (equal overlap either way) or retire it in favour of inv-position.
- `docs/bench.md`: the suite now has battery rows after the old ones (17 jobs, about 40
  more minutes a suite at 2 jobs).
