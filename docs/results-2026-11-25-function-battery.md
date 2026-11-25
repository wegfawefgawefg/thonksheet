# The function battery (2026-11-25) — UNFINISHED

Cheap worlds where the right button is a fixed function of what was shown, graded in
difficulty, each scored three ways. Definitions: `docs/curriculum.md`, "The function
battery". The job was wound up partway (the owner cut the number of agents): the code, the
checks and the parity ladder are done; most of levels (a) and (b) and half of the table
engine are not. What is missing and the line that finishes it are at the end.

## Short version

- **Parity stops at 2 lines, at every level.** Exclusive-or of 2 cues: the sheet as born
  carries it (linear reader 0.95) and the readout rule learns it (0.88). Parity of 3: the
  reader of the frozen sheet gets 0.58 (0.64 with 8,000 trials on seed 5), the rule 0.54,
  its twin 0.49. Parity of 4, 5, 6, 8: everything at chance (0.48-0.51). So 3-point parity
  is lost **in the sheet**, not at the readout: level (a) fails, so (b) has nothing to read.
- **Linear functions climb all the way.** The rule gets AND and OR of 8 lines (0.96, 0.99)
  and majority of 3 (0.99). A one-layer function of any width is easy; one extra order of
  interaction is the wall.
- **A richer sheet climbs one rung; a bigger one does not.** On parity of 3, the richest
  sheet from the richness search (rich4) lifts the reader from 0.58 to 0.86 and the rule
  from 0.54 to 0.76 (over the bar, 8 seeds); rich1 and rich2 lift the reader to 0.79 but
  the rule only to 0.59 and 0.64. Parity of 4 moves for no sheet (0.49-0.53). A sheet of
  16x16 tiles (4x the neurons) or 32x32 (16x, 4 seeds, partly run) does worse or the same
  (reader 0.53, 0.51; rule 0.54, 0.56).
- **The minimal engine and table neurons with their own rules get nothing**, not even AND
  of 2 lines (minimal 0.40 against its twin 0.39; table 0.69 against 0.53, under the bar
  of 0.75). Their default rules do not learn a two-step trial with an ask line at all.

## What was built

- Four task kinds at the end of `TaskKind` (`src/world/task_function*.cpp`): `points`,
  `number`, `two-numbers`, `past`, with flags `--fn`, `--fn-lines`, `--fn-bands`, `--fn-k`,
  `--fn-hold`, `--fn-gap`, `--fn-items`. Shared files touched: four lines per switch in
  `task.cpp`, a few lines in `task.hpp`, one hook and a usage line in `run/run_flags.cpp`,
  one dispatch in `probe/main.cpp`.
- `blobprobe reader` (`src/probe/reader.cpp`): level (a). A ridge reader of every non-sensor
  neuron's spike count over the answer step, frozen sheet as born, beside the same reader on
  the input lines of every step of the trial; taught trials split in time (60% fit, 15% pick
  the ridge, 25% test), held-out values scored apart. Builds the kernel in blocks so a
  32x32-tile sheet fits in memory.
- `scripts/functions.sh` / `functions.py`: the battery, 79 members, levels a, b, c, one
  table and a ladder; finished runs are kept and a rerun does only what is missing.
- `scripts/suite.py`: eight battery rows after the existing ones (not yet run in a suite).

Checks: `--task points --fn parity --fn-lines 2` is byte-identical to both-not-either, and
both-not-either's own 40,000-step csv is unchanged; every member's scripted player is 1.000
and coin just under 1/answers (`docs/notes-world.md`); the same flags repeat byte for byte.

## The battery table, as far as it ran

Seeds 5-12, 40,000 steps, window 1,000, late = mean of the last 4 windows. Bar = halfway
from chance (1/answers) to 1. a: reader of the sheet as born (input-line reader in
brackets). b: the readout rule (`--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift
4 --g.readout_balance 1 --motor-spread 128`), frozen sheet, its learning-off twin, coin.
c: minimal engine and table neurons, default rules, each with its twin. "-" not run yet.

| member | bar | a sheet (inputs) | b rule / twin / coin | minimal / twin | table / twin |
|---|---|---|---|---|---|
| and N=2 | 0.75 | - | **1.00** / 0.46 / 0.50 | 0.40 / 0.39 | 0.69 / 0.53 |
| and N=4 | 0.75 | - | **0.96** / 0.47 / 0.50 | 0.38 / 0.39 | 0.56 / 0.51 |
| and N=8 | 0.75 | - | **0.96** / 0.47 / 0.50 | 0.40 / 0.40 | 0.58 / 0.50 |
| or N=2 | 0.75 | - | **1.00** / 0.45 / 0.50 | 0.40 / 0.38 | 0.71 / 0.51 |
| or N=4 | 0.75 | - | **0.99** / 0.45 / 0.50 | 0.38 / 0.37 | 0.63 / 0.51 |
| or N=8 | 0.75 | - | **0.99** / 0.46 / 0.50 | 0.40 / 0.37 | 0.70 / 0.50 |
| parity N=2 (= both-not-either) | 0.75 | **0.95** (0.53) | **0.88** / 0.49 / 0.49 | 0.26 / 0.36 | 0.55 / 0.46 |
| parity N=3 | 0.75 | 0.58 (0.60) | 0.54 / 0.49 / 0.49 | 0.35 / 0.37 | 0.42 / 0.47 |
| parity N=4 | 0.75 | 0.51 (0.52) | 0.48 / 0.48 / 0.49 | 0.36 / 0.38 | 0.44 / 0.47 |
| parity N=5 | 0.75 | 0.50 (0.51) | 0.48 / 0.48 / 0.49 | 0.37 / 0.38 | 0.43 / 0.48 |
| parity N=6 | 0.75 | 0.51 (0.51) | 0.47 / 0.49 / 0.48 | 0.36 / 0.38 | 0.42 / 0.47 |
| parity N=8 | 0.75 | 0.49 (0.49) | 0.48 / 0.48 / 0.49 | 0.36 / 0.38 | 0.43 / 0.47 |
| majority N=3 | 0.75 | - | **0.99** / 0.49 / 0.50 | 0.41 / 0.39 | 0.51 / 0.48 |
| majority N=4, 5 | 0.75 | - | - | 0.39, 0.39 / 0.39, 0.39 | 0.52, 0.43 / 0.50, 0.48 |

Minimal engine on every other member (all 79 ran; none reaches its bar, none is more than
0.21 over its twin): exactly-1 0.31-0.33; both tables 0.32-0.41; count 0.05-0.24 (chance
0.11-0.33); first 0.10-0.28; one number 0.09-0.55 (identity L=8 K=4 the best gain, 0.45
against 0.29); held-out x 0.13-0.36; two numbers 0.14-0.50; past 0.46-0.66 (majority of 3,
gap 0, 0.66 against 0.45). Full per-member numbers: `out/functions/main-table.txt`.

## The ladder so far

| family | a: sheet as born | b: readout rule | c: minimal | c: table |
|---|---|---|---|---|
| and | not run | N=8 (all) | none | none (best 0.71 at N=2) |
| or | not run | N=8 (all) | none | none |
| parity | **N=2**, fails at 3 | **N=2**, fails at 3 | none | none |
| majority | not run | N=3 (4+ not run) | none | none |
| exactly-1, tables, count, first | not run | not run | none | not run |
| number, two numbers, past | not run | not run | none | not run |

Where parity is lost: level (a) fails at N=3, so the sheet does not carry the answer; the
readout is not the limit. One reading: the default sheet makes pairwise conjunctions (the
branch bonus, finding 20) but little of the third-order term that 3-parity needs (parity of
a, b, c is a + b + c - 2(ab + bc + ca) + 4abc; with pairs only, a linear reader cannot get
it). That is a guess consistent with the numbers, not tested.

## Bigger and richer sheets (parity of 3 and 4)

Levels a and b, seeds 5-12 except 32x32 (seeds 5-8 for a; b only 2 seeds of N=3 finished).

| sheet | parity 3: a / b (twin) | parity 4: a / b (twin) |
|---|---|---|
| default 8x8 tiles | 0.58 / 0.54 (0.49) | 0.51 / 0.48 (0.48) |
| rich1 | 0.79 / 0.59 (0.47) | 0.52 / 0.47 (0.48) |
| rich2 | 0.79 / 0.64 (0.48) | 0.53 / 0.49 (0.48) |
| rich3 | 0.72 / 0.56 (0.48) | 0.49 / 0.48 (0.48) |
| **rich4** | **0.86 / 0.76** (0.48) | 0.53 / 0.48 (0.48) |
| rich6 | 0.66 / 0.54 (0.48) | 0.50 / 0.48 (0.48) |
| 16x16 tiles | 0.53 / 0.54 (0.49) | 0.49 / 0.48 (0.49) |
| 32x32 tiles | 0.51 / 0.56 (0.48), b on 2 seeds | 0.50 / not run |

- **Richness, not size.** Four of five rich sheets carry parity of 3 better than the
  default (0.66-0.86 against 0.58); rich4 carries it well enough for the rule to learn it
  (0.76). The owner's thesis holds one rung further on a richer sheet: when the sheet has
  the feature, the readout selects it. But no sheet carries parity of 4, so richness as
  found by the search buys one order of interaction, not a ladder.
- **A bigger sheet adds nothing.** The extra tiles grow away from both the input and the
  output regions (their cell positions are fixed), so they add neurons that hear the cues
  weakly; the whole-sheet reader then has 4x or 16x more noisy features for the same 1,800
  fitting trials and does slightly worse. Size as these flags give it is not richness.
- In rich1 and rich2 the reader reaches 0.79 but the rule only 0.59-0.64: there the
  readout, not the sheet, is the limit (level a passes, b fails). In rich4 both pass.

## Side checks

- Parity of 3 with 8,000 reader trials instead of 3,000 (seed 5): 0.64 against 0.58. The
  signal is thin, not absent; the reader is somewhat data-limited, which would also limit
  an online rule.
- The input-line reader (least squares on the lines of every step) is a weak baseline: on
  exclusive-or it sits at chance as it should, but on multi-answer members it can miss
  linear functions (count of 3: 0.51), and on held-out values it scores 0 (a never-lit line
  has no weight). Read it only as "1.0 means the raw lines already give it".
- Sine with 3 held-out values (seed 5, reader only): the sheet as born gets 0.67 on values
  it never saw (chance 0.25), the input lines 0.00. Neighbouring lines' activity overlaps
  enough to fill in between taught values. Not yet run at level b.

## Not finished, and how to finish it

Raw outputs: `out/functions/main/{a,b,min,tab}/` (per member and seed), the table so far
`out/functions/main-table.txt`, rich and size runs `out/functions/{rich1,rich2,rich3,
rich4,rich6,size16,size32}/`, the smoke run `out/functions/smoke-quick/`. Done: level a
for parity (48 runs); level b for and, or, parity, majority 3 (101 of 632); minimal 632 of
632; table 114 of 632; the rich and 16x16 runs; 32x32 partly. Every command keeps what has
run and does the rest (an empty `-sum.csv` from a cut run is redone):

```
scripts/functions.sh main --levels b          the rest of level b (531 runs, ~36 s each here)
scripts/functions.sh main --levels a          the rest of level a (584 runs, ~9 s each)
scripts/functions.sh main --levels c          the rest of table neurons (518 runs, ~41 s each)
scripts/functions.sh size32 --levels ab --members 'points-parity-n-[34]$' --seeds 5-8 \
    --g.tiles_w 32 --g.tiles_h 32
scripts/functions.sh main --table-only        the whole table and ladder
```

Level c ran on stevejobs (`MINI_DIR=thonk-functions scripts/mini.sh push`, then
`scripts/mini.sh bg fn-c 'python3 scripts/functions.py main --levels c --jobs 4'`; its
outputs were pulled into `out/functions/main/`). That machine ran at a load of 40-80 on 14
cores during this job, so table runs there were slow.

The rich and size runs: `scripts/functions.sh rich4 --levels ab --members
'points-parity-n-[34]$' <rich4's genome numbers from scripts/variants/rich-sheets.txt>`
(and the same for rich1, 2, 3, 6), `scripts/functions.sh size16 --levels ab --members
'points-parity-n-[34]$' --g.tiles_w 16 --g.tiles_h 16`.

Once level a is in: run the size and richness experiment on the other members that fail
at the default sheet (tables without a threshold unit, past same and parity, count).

## What should change elsewhere (not edited here)

- `docs/findings.md`: a new wall, "Order of interaction": exclusive-or of 2 is carried and
  learned, parity of 3 is not carried by the default sheet (reader 0.58) and parity of 4 by
  no sheet tried; linear functions of 8 lines are learned (and, or 0.96-0.99, majority of 3
  0.99). Finding 20 (richness can be tuned) gets a second, independent result: the richest
  sheet takes parity of 3 from 0.58 to 0.86 readable and 0.54 to 0.76 learned [likely].
  Finding 18 (size does not break it) gets a qualifier: size does not help either; a 4x or
  16x sheet grown away from the input and output regions reads parity of 3 no better.
  The headline's "holds for everything a linear reader can get out of the sheet" stands,
  with the limit now measured: the sheet as born holds pairwise features, not third order.
- `docs/findings.md`, the minimal and table engines: with their default rules they learn
  none of the battery, AND of 2 included, on two-step trials with an ask line.
- `docs/task-map.md`: add the four kinds (points, number, two-numbers, past) and
  `blobprobe reader`.
- `docs/ideas.md`: ideas worth a number: (1) a sheet built for third-order features
  (three-input conjunctions, a second branch level) judged on parity of 3 and 4; (2) the
  richness search scored on parity of 3 through `blobprobe reader` (a cheap, sharp target);
  (3) growing the sheet between the input and the output instead of away from them.

## Rough

- Most of the table is not run; the ladder above is for and, or, parity, majority only.
- The reader's chance is 1/answers and it always answers; the lives' coin scores lower
  (ties are no move). Comparable in units, not in conditions (as in the readout doc).
- `table-nonlin` uses a perceptron check (2,000 passes): exact for N up to 4, almost
  surely right beyond.
- Number kinds use 2x2 sensor groups (the battery's other kinds 3x3), so the families are
  not on the same plug.
- In a held-out life the taught values' score is kept in tally[2]/[3] and not printed; the
  battery reads taught scores from the life without holding out.
- The 32x32 level-b numbers are 2 seeds.
- Speeds here were taken with other agents' runs on the machine.
