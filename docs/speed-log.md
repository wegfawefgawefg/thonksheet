# Speed log

How fast the loop is, measured on purpose and dated, so a slowdown shows. See "Keep the loop
fast" in `docs/bench.md`. One row per audit. Say what else was running on the machine.

| date | what | figure | machine and load | note |
| --- | --- | --- | --- | --- |
| 2026-11-24 | full engine tick, 4,096 neurons, learning off / on | 73,812 / 51,557 ticks/s | this PC, one core, quiet, before the clock cap | first build |
| 2026-11-24 | minimal engine, echo life, learning on | 206,000 ticks/s | this PC, one core, load about 10, clock capped | 4.6x the full engine in the same session |
| 2026-11-24 | table neurons (8 inputs), 4,096 neurons, learning on | 38,600 ticks/s | this PC, one core, load about 7 | |
| 2026-11-24 | full suite, full engine, 192 runs | 56 min | this PC, 2 jobs, shared | |
| 2026-11-24 | full suite, minimal engine | 11 min | this PC, 2 jobs, shared | |
| 2026-11-24 | full suite, table neurons | 63 min | this PC, 2 jobs, shared | |
| 2026-11-25 | richness screen, one sheet, four features | 1.8 s | this PC, 2 jobs | no learning rule involved |
| 2026-11-25 | echo life, 20,000 steps: full / minimal / table | 34 s / 2.5 s / 7.9 s | stevejobs, one core | here: 9.4 s / 1.8 s / 10.2 s |
