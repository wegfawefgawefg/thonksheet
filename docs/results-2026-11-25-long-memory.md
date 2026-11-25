# Long memory (ideas 129, 130, 131; 2026-11-25)

Stopped early (the owner cut the agent count). What is measured is below, with what is
unfinished marked and the line that finishes it. Seeds 1 and 2 were used to choose
settings; seeds 5 and up are reported, and only seeds 5 to 8 got far (see each table).

Two different problems: holding something across a gap (activity), and keeping what was
learned while learning more (wiring). Worlds for both were built, the engines measured on
them, and mechanisms tried behind switches that are off by default and byte-identical when
off (checked: echo k4 table, echo plain blob, order with the evidence readout, all `cmp`
equal against 2616354).

"The readout" below is the full engine with the working evidence readout:
`--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1
--motor-spread 128`.

## The worlds

Defined in `docs/curriculum.md`, "Long memory"; code `src/world/task_long.cpp`; run with
`blobrun --long` (`src/run/life_long.cpp`) or `scripts/longlife.sh WORLD ENGINE`.

- **hold**: a cue, N blank steps, the ask (N = 1 to 5,000 steps = 20 to 100,000 ticks).
  `--hold-noise 1`: distractor lines flash in the gap. `--hold-noise 2`: the cue lines
  themselves flash, the real cue carries a mark line (needs a gate).
- **pairs**: a new pair every 400 steps (6 lit lines of 28; 376,740 patterns, none
  repeated, button a fixed hash of the pattern), the newest 4 taught side by side, each
  asked once more, unrewarded, at ages 100, 1,000, 10,000, 100,000, 1,000,000 steps. Score:
  the forgetting curve and "pairs held at once" (curve above chance, integrated over age,
  an age within two standard errors of chance counting 0).
- **once**: a pattern shown a single time with its answer on a hint line, asked once later.
- **rare**: 8 common patterns, 4 that come one step in 1,000.
- **drift**: 8 pairs; at a set step the odd ones move to the next button.

Coin and perfect (2M-step pairs lives, 3 seeds; the others 30k-step stand-in checks):

| world | bucket | coin | perfect |
| --- | --- | --- | --- |
| hold (2 cues) | ask | 0.46 | 1.00 |
| pairs | every age, teaching | 0.22 to 0.25 | 1.00 (held 2,500 per 1M steps) |
| once | ages, show, hint | 0.18 to 0.33 (few asks) | 1.00 |
| rare | common, rare | 0.23, 0.20 | 1.00 |
| drift | kept, changed | 0.23, 0.23 | 1.00 |

Coin is below 1/4 because ties are no move. Learning-off twins: readout 0.23 at every
age, table 0.19 to 0.23.

## The baseline

### Forgetting (pairs, 2M-step lives, mean (sd); raw in out/longmem/here/out/longmem/base
and out/mini/longmem/base)

| engine | seeds | teach | age 100 | 1,000 | 10,000 | 100,000 | 1,000,000 | held |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| readout (full engine) | 5-7 | 0.64 (0.04) | 0.44 (0.03) | 0.71 (0.04) | 0.28 (0.01) | 0.23 | 0.23 | 13.6 (2.6) |
| table neurons | 5-7 | 0.65 (0.03) | 0.44 (0.02) | 0.69 (0.04) | 0.25 (0.01) | 0.24 | 0.23 | 7.7 (0.7) |
| minimal engine (10M steps) | 5 | 0.25 | 0.20 | 0.27 | 0.20 | 0.20 | 0.19 | 0.3 |
| coin | | 0.23 to 0.24 | | | | | | 0 |

Age 100 is mid-teaching (each pair is taught for 1,600 steps), so age 1,000 is the best
point of the curve. Every engine has lost a pair by 10,000 steps (25 newer pairs) and
holds 8 to 14 pairs at once. The readout's teaching falls during its first 100k steps
(0.89 to 0.90 on the first window, 0.54 to 0.63 by 50k, seeds 1-2) and then sits at about
0.67 for the rest of 2M steps; table neurons stay at 0.60 to 0.67 throughout. The minimal
engine does not learn this world at all in 10M steps (teaching 0.23 to 0.26 in every
window). The 10M-step table life was stopped at 40% with its rows unflushed: nothing
from it.

### How long a cue is held (hold, 2 cues; right asks)

| engine | 20 ticks | 200 ticks | 1,000 ticks | seeds |
| --- | --- | --- | --- | --- |
| readout | 0.71 | 0.48 | 0.49 | 1 |
| table neurons | 0.43 to 0.45 | 0.44 to 0.45 | 0.43 | 1-2 |

So the readout holds a cue about 20 ticks, and table neurons cannot answer even then
(their learning grades the last 16 ticks before the reward, which do not include the cue).
Unfinished: the baseline hold rows on seeds 5-12 for table and minimal, and the once,
rare and drift baselines for every engine (queued on stevejobs, stopped).

## Holding (activity)

### Integrator cells (catalogue 99, 180): `--g.hold_percent 10`, `--t.hold_percent 10`

A share of excitatory inner neurons each count onsets of one sensor cell near them, with no
leak, fire while the count is above 0, and clear on any good or bad. Exempt from noise and
the rate bar. They hold "which cue" for as long as the trial lasts, and say nothing about
when.

### Time cells, the log-spaced timeline (idea 131, catalogue 62): `--g.time_percent 10`

Each sensor cell keeps two counters: ticks since its last onset and since the onset
before. A time cell has a sensor cell near it, a bin [2^(4+k), 2^(5+k)) ticks with k
below `time_levels` (14, up to 262,144 ticks), and which of the two onsets it times; it
fires while that count is in its bin. No reset; it carries which cue and how long ago.
8 bytes per sensor cell and 6 per time cell; set after the fire step, one hook.

The first form of idea 131, slow OR-maps or sampled maps refreshed every 2^k ticks that
inputs of table neurons can read (`--t.timeline_levels 10 --t.timeline_percent 10`, with
or without `--t.timeline_source 1` to read sensor cells), did nothing: hold at 200 and
1,000 ticks stayed 0.44 to 0.45 (seeds 1-2). The block refresh blurs the phase, the OR
of a busy inner cell is all ones, and few inputs land in the right level.

### Seeds 1-2 (choosing), the readout

| row | plain | integrator 10% | time 10% | time 10% + integrator 10% |
| --- | --- | --- | --- | --- |
| delayed echo | 0.56 | 1.00 | 0.92 | 1.00 |
| gap | 0.51 | | 0.82 (0.98, 0.67) | 0.99 |
| order | 0.71 | | 0.86 (0.99, 0.72) | 0.98 |
| echo k=8 | 0.86 | | 0.82 | 0.97 |
| exclusive-or | | | 0.88 | 0.96 |
| hold 20 ticks | 0.71 | 1.00 | | |
| hold 200 ticks | 0.48 | 1.00 | | |
| hold 1,000 ticks | 0.49 | 1.00 | 0.78 | 0.99 |
| hold 5,000 ticks | | | 0.57 | 0.98 |

Integrator cells at 5% were as good on hold up to 1,000 ticks (0.99) and delayed echo
(1.00); 10% was kept. Table neurons with integrator cells: hold 20 ticks 0.73 and 0.80,
200 ticks 0.85 and 0.49, 1,000 ticks 0.58 (0.71 in the second half), against 0.43 to 0.45
plain.

### Seeds 5-6 (reported; unfinished), the readout, late score

"hold2" is the one setting proposed: time 10% + integrator 10%.

| row | plain | hold2 | integrator only | ring sheet |
| --- | --- | --- | --- | --- |
| echo k=2 | 1.00 (2) | 1.00 (2) | 1.00 | 0.96 |
| echo k=8 | 0.83 (2) | 0.96 (2) | 0.97 | 0.75 |
| exclusive-or | 0.92 (2) | 0.93 (2) | 0.95 | 0.61 |
| delayed echo | 0.53 (2) | 1.00 (1) | 1.00 | |
| gap | 0.53 (2) | 0.98 (1) | 0.49 | |
| order | 0.68 (2) | 0.99 (1) | 0.57 | |

(n) is the number of seeds where it is not 1. On seed 5 integrator cells alone cost order
0.68 to 0.57; with time cells beside them order is 0.99. Nothing in this table is worse
with hold2 than plain. Unfinished: seeds 7-12 of this table, hold at every gap on seeds
5-12, distractors (`--hold-noise 1, 2`), 20,000 and 100,000 ticks.

### A region that holds by itself

The ringing sheet from the 500-sheet search (docs/results-2026-11-25-richness-search.md),
with the readout: hold at 20 ticks 0.51 and 60 ticks 0.52 (seed 1, plain 0.71), and on seed
5 it is worse than the default sheet on echo k=2 (0.96), echo k=8 (0.75) and exclusive-or
(0.61). It rings, but the readout cannot use the ringing. A two-region sheet (one part
ringing, one mixing) needs per-region genome numbers in birth.cpp and fire.cpp, which other
agents own; not tried.

## Keeping (wiring)

### Decay as room: `--g.readout_decay N`, `--t.decay N`

On each update of a motor cell, N random evidence counters of that cell step once toward
0 (locked ones are spared). Seeds 1-2, pairs, 100k steps, the readout:

| setting | teach | age 100 | 1,000 | 10,000 | held | echo k=8 |
| --- | --- | --- | --- | --- | --- | --- |
| plain | 0.65 | 0.46 | 0.76 | 0.30 | 9.3 | 0.86 |
| decay 1 | 0.66 | 0.41 | 0.73 | 0.24 | 8.2 | |
| decay 2 | 0.73 | 0.51 | 0.81 | 0.26 | 9.8 | 0.71 |
| decay 8 | 0.88 | 0.75 | 0.91 | 0.26 | 11.8 | 0.41 |
| decay 8, idle sources only | 0.85 | 0.72 | 0.90 | 0.27 | 11.6 | 0.42 |
| decay 32, idle only | 0.92 | 0.85 | 0.96 | 0.25 | 12.8 | 0.21 |

Decay keeps the readout learning for the whole life (teaching 0.86 to 0.91 in every window
with decay 8, where plain sinks from 0.90 to 0.54 to 0.63). The cause of the fall is
clutter, not saturation: after 30k steps 68% of counters are non-zero without decay and 38%
with it, and only 1.4% sit at an end of their range; no motor cell goes dark. But decay
fast enough to make room erases the slow evidence that echo with 8 cues builds over 40k
steps (0.86 to 0.41 at decay 8, 0.71 at decay 2), and on seed 5 it costs exclusive-or 0.92
to 0.81. No decay rate is good for both, so decay is not part of the proposed setting.
`--t.decay` on table neurons was built and not run.

### The freezer (idea 130): `--g.readout_lock_levels`, `_shift`, `_hold`, `_from`, `_fade`; `--t.lock_levels`, `_shift`, `_hold`

A counter pushed further at the end of its range (or, with `_from F`, at least F strong)
deepens a lock (up to 3); a move against a lock of depth d passes with chance
2^-(3d) and opens it; decay spares locks; with `_fade S` a decay pick lowers a lock with
chance 2^-S.

| setting (seeds) | teach | 1,000 | 10,000 | held | echo k=8 |
| --- | --- | --- | --- | --- | --- |
| readout, lock 3 (1-2) | 0.60 | 0.64 | 0.28 | 6.8 | |
| readout, lock 3 + decay 8 (1-2) | 0.87 | 0.90 | 0.24 | 11.6 | |
| readout, lock from 4 + decay 8 (1) | 0.71 | 0.78 | 0.31 | | 0.43 |
| readout, lock from 4, fade 2, decay 8 (1) | 0.82 | 0.89 | 0.32 | | 0.38 |
| readout, lock from 8, fade 2, decay 8 (1) | 0.83 | 0.84 | 0.24 | | 0.39 |
| table, plain (1) | 0.65 | 0.70 | 0.23 | 7.7 | |
| table, lock 1 (1) | 0.62 | 0.68 | 0.24 | 7.3 | |
| table, lock 3 (1) | 0.44 | 0.47 | 0.26 | 3.7 | |

Null. Locking trades learning new pairs for a hair more at 10,000 steps (0.31 to 0.32
against 0.24 to 0.30) and does not rescue echo k=8 from decay. The reading: the readout
holds about 10 to 14 pairs at once whatever is frozen; forgetting here is a capacity limit
(new pairs' sources displace old ones from the cell's connection budget), so freezing can
only choose which pairs to keep, not keep more. Keeping old pairs while learning new ones
needs more room, not stiffer counters.

## What now holds how long, and keeps how much

- Holds: with time and integrator cells the readout answers a 2-cue hold after 1,000 and
  5,000 ticks at 0.98 to 0.99 (seeds 1-2), delayed echo at 1.00, gap at 0.98 to 0.99 and
  order at 0.98 to 0.99 (seeds 1-2 and seed 5). Untested: 20,000 and 100,000 ticks, and
  distractors.
- Keeps: 8 to 14 pairs at once in every engine, each gone by 10,000 steps. Decay makes the
  readout keep learning at 0.86 to 0.91 for the whole life but costs echo k=8. Nothing
  kept a pair 100,000 steps.

## Cost

Time and integrator cells: no change measurable in ticks per second under the shared load
(the readout ran 30k to 39k ticks/s with and without them); state is 8 bytes per sensor
cell plus 6 bytes per cell made, made on first use. The integrator and time cells replace
those neurons' own spikes, so 20% of excitatory inner neurons stop computing. Decay: N
random counter steps per motor-cell update. Locks: one byte per evidence counter.

## Against the memory engine on main

`blobrun --engine memory` holds across 4,000 silent ticks by counting events. These worlds
would test it where counting is not enough: `--hold-noise 2` (the cue lines flash in the
gap, so a count of events says nothing about which was marked), pairs (a new pair every
400 steps for ever: more pairs than any fixed number of slots, so the forgetting curve
shows what it evicts), drift (the stored answer for half the pairs becomes wrong at once;
does it overwrite, or keep both), and once (a single showing). Not run.

## What should change in other docs

- docs/findings.md, wall "Time": add that time cells (log-spaced, per sensor cell) with
  integrator cells take delayed echo from 0.53 to 1.00, gap from 0.53 to 0.98 and order
  from 0.68 to 0.99 through the existing readout with nothing changed between tasks, and
  hold a cue 5,000 ticks (hint: seeds 1-2 and one or two report seeds). Add a new wall,
  "Keeping": every engine holds 8 to 14 pairs at once and loses each by 10,000 steps; the
  readout's learning falls from 0.90 to 0.65 as old evidence clutters its counters; decay
  restores it but costs echo k=8; the freezer does nothing (likely); the minimal engine
  does not learn the pairs world. Under principles: "the readout must forget to keep
  learning, and no forgetting rate suits both a stream and a slow task" (hint).
  The ringing sheet: holds, but the readout reads it worse (hint).
- docs/task-map.md: tier 4 "best so far" becomes the readout with time and integrator
  cells (delayed echo 1.00, gap 0.98, order 0.99, beat untested). Tier 8 gets the pairs,
  once, rare, drift worlds and the line "8 to 14 pairs at once, gone by 10,000 steps".
  Tier 12 (the long game) gets the 10M-step life protocol. The milestone "every row of
  tiers 1 to 4 on one set of rules" is close: beat and sequence are the rows not yet run.
- docs/ideas.md: 129 built; 130 built, null; 131 built, null as slow maps, works as time
  cells; 111 (a place that holds): the ringing sheet does not help the readout. New ideas:
  more room for the readout (a larger or growing budget per motor cell, or a separate
  store of pairs, so keeping does not need forgetting), and time cells and integrators on
  inner neurons' outputs, not only on sensor cells.

## How to resume

Everything is in this worktree: `out/longmem/` holds the job lists and helpers
(`make_jobs.py`, `make_score.py`, `runq.py`, `summarize.py`), dev runs (`dev1` to
`dev7`), `out/longmem/here/out/longmem/base` and `.../score` (this PC) and
`out/mini/longmem/base` (stevejobs).

- Finish the readout scoring on seeds 5-12 (506 jobs, about 800M ticks; the sorted list
  runs the main comparisons first; finished jobs are skipped):
  `cd out/longmem/here && python3 ../runq.py jobs-score-blob2s.txt 2`
  (binary at `out/longmem/here/bin/score2/blobrun`, built from this branch).
- Table neurons scoring and the table and minimal baselines (stevejobs):
  `MINI_DIR=thonk-longmem scripts/mini.sh bg q 'python3 out/jobs/runq.py out/jobs/jobs-base2.txt 4'`,
  then the same with `out/jobs/jobs-score-table-mini.txt`.
- One routine 10M-step life: `scripts/longlife.sh pairs table --seed 5` (about 5 hours on
  stevejobs under load; the minimal engine took 6,310 s there).
- Owner-facing lines: `scripts/longlife.sh hold blob --hold-steps 5000 --steps 2000000
  --g.time_percent 10 --g.hold_percent 10`, `scripts/longlife.sh pairs blob --g.readout_decay 8`.

## Rough

- The 10M-step table life left no rows because the csv was only flushed when its buffer
  filled; `life_long.cpp` now flushes each window.
- `pairs held` leans on straight lines between ages a decade apart; the 1,000 to 10,000
  band dominates it.
- Integrator and time cells take their source from sensor cells. That is a choice of
  architecture (memory fed from the input, as in AuGMEnT), not of task, but it is a prior.
- Integrator cells clear on any reward, which is the trial boundary in every world here;
  a world with reward mid-trial would wipe them.
- The table-engine timeline encoding was widened from 4 to 5 bits of input code; csvs are
  unchanged.
