# The search

A script that tries settings of every engine for days with nobody watching, scores each one
on a short map of worlds with nothing changed between worlds, and keeps what works. The
owner, 2026-11-25: "ideally i could just set you off for a week, you try a zillion different
random mechanics. we stumble upon some stuff that works for all of our toy tasks, then we
latch onto it and focus on scale."

It is a search over combinations of the numbers the engines already have. It cannot invent
a mechanism; it can find which of the existing ones, at which settings, work together. The
one way it can find something new in kind is a number that is itself a rule (a rule table
kept as hex, below).

## Running it

Everything goes through `scripts/search.sh`; its header lists every line.

```
scripts/search.sh run                      this PC, 2 jobs under nice -n 19, until stopped
scripts/search.sh run --hours 8            stop by itself after 8 hours
scripts/search.sh status                   how far, the best so far, the last log lines
scripts/search.sh report                   write docs/search-leaderboard.md
scripts/search.sh stop                     finish the runs in flight, then stop
scripts/search.sh check minimal            are a space file's names and tags right

scripts/search.sh mini start --jobs 8 --hours 48    the second machine (~/thonk-search)
scripts/search.sh mini status | log | stop | kill | pull
```

`mini start` copies this checkout there, builds it, and starts the search under `nohup
caffeinate`, so it keeps going after the ssh session ends. `mini pull` copies its folder
back to `out/search/mini-main/` and merges it into `out/search/main/`; run `report` after.
Both machines can work on the same search: run it on both and `pull` now and then (merging
only appends what is missing). Results are byte-identical between the machines.

Everything a search has done is in `out/search/<name>/`: `results.jsonl` (one line per
finished run, keyed by its exact command), `candidates.jsonl`, `decisions.jsonl` (each
race decision and why), `run.log`. All three are append-only. A crash, a reboot or a lost
connection loses only the runs in flight; start it again and it carries on. Nothing is ever
run twice: a command already in `results.jsonl` is never run again, by either machine.

The search does not start a run while the one-minute load is over `--max-load` (default:
the number of cores), except to keep `--busy-jobs` going (default 1), so it backs off when
the machine is busy. On the second machine `mini start` uses `--max-load 21 --busy-jobs 4`:
4 jobs while other agents load it, up to 8 when it is quieter.

## What a candidate is

One engine and one full set of that engine's numbers and plug settings (ticks per step,
sensor and motor group sides, reward and rest ticks, motor spread). It is used unchanged on
every world (docs/bench.md, "One setting for everything"). A world's own flags (its task,
its layout) come last on the command line and win; nothing else may differ between worlds.

Frozen-inside settings are not in the space ("Nothing frozen", docs/bench.md): the full
engine's learn modes 1 and 5 and flips limited to the motor cells.

## Scoring: the quick map

`scripts/search/quick-map.txt`. A life world is one `blobrun` life of 10,000 steps on one
seed, plus its learning-off twin (a separate cached run: candidates that differ only in
learning numbers share one twin). Its score is

    (late - the larger of the twin's late and chance) / (perfect - chance), held to 0..1

so beating a twin that sits below chance does not count. A probe world (full engine only)
is one `blobprobe feature` reading of the sheet as born, at the tile next to the input,
scaled the same way between a "lost" reading and a fully readable one. A world is passed
at 0.3 or more (mean over the seeds in play). The headline fitness is the number of worlds
passed; the tie-break is the mean score over worlds.

Seeds 1, 2, 3 choose; seeds 5 to 8 confirm. The file refuses to load if they overlap.
Every score on the leaderboard is from the confirmation seeds, and candidates are chosen and
ordered by their selection seeds only.

## Racing

Each candidate climbs rungs, each a superset of the one before:

| rung | worlds | seeds | goes on if |
| --- | --- | --- | --- |
| 0 | the fast worlds (marked `fast`; else the 3 cheapest life worlds) | 1 | in the best 40% of its engine at rung 0 |
| 1 | all | 1 | in the best 25%, or the best so far on some world or tier |
| 2 | all | 1, 2, 3 | among the 3 best of its engine, or the best on some world or tier |
| 3 | all | 1, 2, 3 and 5 to 8 | (confirmed; reported, never used to choose) |

A candidate scoring nothing stops, and so does one whose raw scores at a rung are exactly
those of an earlier candidate (a mutation that changed nothing that acts). With fewer than
10 earlier candidates at a rung (5 at rung 2) everything that scores goes on. Each engine's
defaults and the named starts (`scripts/search/starts.txt`) always go through to rung 3, so
the leaderboard always shows them for comparison. Within a rung the cheapest worlds run
first (measured cost).

## The search itself

New candidates go to the engine that has used the least machine time so far. 20% are
fresh (the defaults with about half the numbers drawn anew from their ranges), 25% a
crossover of two parents (each number from either; a rule entry by entry) with maybe one
change, the rest a mutant of one parent with 1 to 4 numbers changed (a shift or small
integer mostly moves one step; a quarter of the time it is drawn anew; a switch flips; a
choice changes; a rule has 1 to 8 entries changed). Parents are picked by tournament of
three from the pool: the 12 best of the engine (furthest rung first, then fitness there),
plus the best candidate on each world, so a setting that solves one hard world keeps
breeding even if it fails the rest. Numbers that cannot act (a `when` tag says so) are held
at their defaults and are never the ones mutated.

## Adding a world

Add a line to `scripts/search/quick-map.txt`:

```
# name   tier kind  zero  full  fast engines  flags
gap5     4    life  0.50  1.00  -    all      --task gap --gap-long 5
```

`zero` is chance for a life world (1/k for k buttons), `full` is perfect. Then stop and
start the search (`mini stop`, wait for `mini status` to say not running, `mini start`).
New candidates get the world; the defaults, the starts and the current parents are topped
up with it; old candidates keep their scores. Only the new world's runs cost anything.
Budget first (docs/bench.md, "Keep the loop fast"): a new world goes in by taking the place
of one, or by raising `budget` on purpose.

## Adding a mechanism's number

Add a line to the engine's space file, `scripts/search/space-<engine>.txt`:

```
--m.new_thing   switch  0..1  0   learn  when=--m.ev_on
```

The default must be the engine's own default, so that every earlier command stays the
same and nothing is rerun. Kinds: `switch`, `int lo..hi`, `shift lo..hi` (a chance of
2^-x; mutation moves it a step), `choice a,b,c`, `fixed` (listed, not searched), `rule N`.
Tags: `learn` if the learning-off twin ignores it (wrong tags corrupt the shared twin),
`plug` for plug flags, `when=FLAG` or `when=FLAG:a,b` if it only acts while another number
is off its default or has one of those values. Then run `scripts/search.sh check ENGINE`:
it runs every number once and checks every `learn` tag against the twin and every `when`
against the life, at the defaults. Restart the search to pick the line up.

### A rule as data

```
--rule-file   rule   1024   none   learn   seeds=rules/*.hex
```

A table of 1,024 4-bit entries, kept by the candidate as a string of 1,024 hex digits,
**one digit per entry, entry 0 first**, on one line. When a run needs it the search writes
it to `out/search/<name>/rules/<hash>.hex` and passes `--rule-file <that path>`; the cache
key holds `rule:<hash>` instead of the path, so two machines agree. A fresh rule is one of
the seed files (half the time), random (a quarter) or none (the engine's own rule).
Mutation changes 1 to 8 entries; crossover takes each entry from either parent. Named
files seed the population through `starts.txt` (`--rule-file @rules/name.hex`). The line
is in `space-blob.txt`, commented out until blobrun takes `--rule-file`.

## A batch evaluator

`--batch program:PATH[:ENGINES[:SIZE]]` hands up to SIZE lives at a time to an external
program (for `blobgpu`, thousands of minimal-engine lives at once), in a slot of its own
beside the CPU jobs. It gets a csv `key,args` (blobrun's flags) and writes `key,early,late,
error`. Its rows are recorded exactly as if blobrun had run them, so it must give
blobrun's numbers for the same command; one that does not belongs in a search of its own
`--name`. If a whole batch fails, the CPU takes everything over. `scripts/search/batch.py`.

## Reading the leaderboard

`docs/search-leaderboard.md`, written by `scripts/search.sh report`:

- **Tried so far**: candidates per engine, how many reached each rung, core hours.
- **Since the last report**: new candidates, new best per world.
- **Best overall** (life worlds, all engines), **best per engine** (with the defaults
  beside), **best per tier**, **best per world**. Columns "selection" and "confirmation"
  are `passed of worlds, mean`; per-world cells are confirmation scores, bold when passed.
- **What a candidate costs**: seconds per world per machine against the budget, one pass
  of the map per engine (OVER BUDGET when it is), candidates per core hour against the last
  report.
- **Worlds: what each one tells us**: per world the share of candidates passing it, the
  best ever, and the world it tracks most closely. Flagged to cut: a world 95% of
  candidates pass, one nobody has passed, one that tracks another at 0.9 or more.
- **Which numbers matter**: rank correlation of each number with each world's score (for
  a choice, the gap between one value's mean score and the mean). Numbers with no effect
  on any world after 200 candidates are flagged for removal from the space file.
- **The settings**: every confirmed candidate as flags.

## What it costs

Seconds of one core for one world on one seed (a life and its twin; a probe), this PC,
2026-11-25, at load 37 on 32 threads (every agent's runs going at about half speed; quiet,
roughly half these):

| engine | life world | probe | one pass of the map (13 life worlds, 4 probes) |
| --- | --- | --- | --- |
| full | 13.5 s | 2 s | 184 s |
| minimal | 4 s | - | 53 s |
| table | 27 s | - | 351 s |

On the second machine the full engine is about 3.6 times slower than this PC, minimal
about 0.7, table about 1.3 times faster (AGENTS.md). Per candidate, the rungs cost one
pass times: rung 0 about 0.2 (three worlds), rung 1 one, rung 2 three, rung 3 seven, less
whatever is cached (a twin is shared by every candidate with the same non-learning
numbers; a probe by every candidate with the same sheet). Most candidates stop at rung 0
or 1. The small case below managed 141 candidates per core hour on the minimal engine with
three worlds; the report gives the live figure per engine for the real map.

## The small case that proved it

`scripts/search/small-map.txt`: Echo with 2 cues, with 4 cues, and with cue and button two
tiles apart, minimal engine only, no named starts, 2 jobs on this PC (loaded), 1.2 core
hours, 163 candidates (68 reached rung 1, 36 rung 2, 22 confirmed). It was killed with
`kill -9` once and with TERM once and restarted each time: 1,562 runs recorded, 1,562
distinct commands, every line from before each kill still there unchanged.

On confirmation seeds 5 to 8 the defaults pass 1 of the 3 worlds (echo2 0.81, echo4 0.22,
distance 2 0.00; mean 0.34). Eight of the ten best by selection pass 2 of 3: for example
`--m.down_shift 10 --sensor-side 2 --m.threshold_max 9` gives echo2 0.48, echo4 0.41;
`--m.down_shift 10 --m.up_shift 5 --sensor-side 2` gives 0.60, 0.32. Nothing passes
distance 2 (best 0.19). Two warnings it gave at once: the two best by selection fell back to
1 of 3 on confirmation (three selection seeds flatter whoever ranks top), and the mean
score did not beat the defaults (echo4 was bought with echo2).

## Blind spots

- It finds only what the quick map rewards. A world not on it, a property no world tests
  (keeping a first task after a second, holding still when unplugged, speed) is invisible.
- Short lives. 10,000 steps; a mechanism that needs a long life to show itself (slow
  growth, anything that wins only after 40,000 steps) is missed or punished.
- One seed decides rung 0 and rung 1; a lucky seed can carry a weak candidate up a rung
  and an unlucky one can stop a good one. Rung 2 uses three seeds; confirmation four more.
  The small case showed the top of the selection ranking regressing on confirmation;
  read the confirmation columns, not the order.
- Racing on the fast worlds first: a setting that fails echo but solves something hard
  dies at rung 0 unless it also scores on the fast worlds.
- Correlations of numbers with scores are taken over a population bred from good parents,
  not a random sample, so they are hints. A number that matters only together with
  another may show nothing alone.
- Fitness counts worlds passed; partial progress on a world nobody passes only shows in
  the tie-break mean.
- The cache trusts that a command means the same thing on every commit. If an engine's
  behaviour at an existing setting changes (a fix, a new default), old rows are stale: run
  that engine's search under a new `--name`, or say so in the report.
- Adding a world makes old candidates' fitness at that rung count the new world as 0 until
  topped up, so for a while newcomers pass more easily.
