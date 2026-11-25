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
| 2026-11-25 | audit: machine load with 21 agents running | this PC load 32 on 32 threads; stevejobs load 36 to 48 on 14 cores (26 busy runs, all ours: 7 agents at up to 4 jobs each) | both | Compute, not ideas or usage, is the limit. On stevejobs every run is going at about half speed. More agents now only slow each other; new compute-heavy jobs wait until some finish, and the GPU port is the next real gain. |

| 2026-11-25 | minimal engine on the GPU (blobgpu), echo k=2, 2,000-step lives, learning on / off | 9.48M / 10.95M life-ticks/s at 16,384 lives; 234 / 269 lives/s for the whole batch | this PC, RX 9070 XT; CPU load 34 to 38 | one core same session: 104,000 / 136,000 (200,000+ at load 20). Saturates by 1,024 lives. docs/results-2026-11-25-gpu.md |
| 2026-11-25 | blobgpu big: 16,384 lives + twins, 40,000 steps each | 40 min, 10.9M life-ticks/s, 13.7 lives/s | this PC, RX 9070 XT | bit-identical to blobrun --m.rng_mode 1 on the seeds checked |
