# Search leaderboard: main

Written by `scripts/search.sh report` on 2026-11-25 04:43. How to read it: docs/search.md.

Every candidate is one setting used unchanged on every world. Chosen and ordered on selection seeds 1 2 3; every score shown is from confirmation seeds 5 6 7 8, which played no part in choosing. A world's score is (late - the larger of the learning-off twin and chance) / (perfect - chance), held to 0..1 (probes: the reading scaled the same way); passed at 0.30 or more.

## Tried so far

| engine | candidates | reached rung 1 | rung 2 | confirmed | core hours |
| --- | --- | --- | --- | --- | --- |
| minimal | 94 | 43 | 25 | 12 | 1.8 |
| blob | 3 | 2 | 1 | 0 | 4.0 |
| table | 1 | 1 | 1 | 0 | 0.2 |

## Since the last report

Last report: 2026-11-25 04:03.
- minimal: 89 new candidates (94 in all).
- blob: 3 new candidates (3 in all).
- table: 1 new candidates (1 in all).
- new best on echo2 minimal: `fb773ede6fdc` (confirmation 0.83, was 0.81)
- new best on shuffled minimal: `63e25b283e45` (confirmation 0.60, was 0.66)
- new best on gonogo minimal: `0e71ff916060` (confirmation 0.72, was 0.80)
- new best on gap minimal: `b5017acdcc2d` (confirmation 0.01, was 0.00)
- new best on beat minimal: `9e5de9d92e27` (confirmation 0.02, was 0.02)

## Best overall

On the life worlds (every engine runs them). Passed and mean are on the confirmation seeds.

| candidate | engine | made as | selection | confirmation | echo2 | echo4 | echo8 | shuffled | gonogo | dist2 | dist3 | xor | order | gap | delayed | beat | sequence |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `edfe1224c68d` | minimal | cross | 3 of 13, mean 0.21 | 3 of 13, mean 0.19 | **0.88** | 0.15 | 0.01 | **0.62** | **0.81** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.03 | 0.00 |
| `e63d979330c9` | minimal | default | 3 of 13, mean 0.20 | 3 of 13, mean 0.19 | **0.81** | 0.22 | 0.00 | **0.66** | **0.80** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.02 | 0.00 |
| `63e25b283e45` | minimal | cross | 3 of 13, mean 0.20 | 3 of 13, mean 0.19 | **0.84** | 0.13 | 0.01 | **0.60** | **0.82** | 0.00 | 0.00 | 0.00 | 0.00 | 0.01 | 0.00 | 0.03 | 0.00 |
| `1a24160baece` | minimal | mutant | 3 of 13, mean 0.20 | 3 of 13, mean 0.17 | **0.85** | 0.13 | 0.00 | **0.50** | **0.77** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.02 | 0.00 |
| `fb773ede6fdc` | minimal | cross | 3 of 13, mean 0.19 | 3 of 13, mean 0.18 | **0.83** | 0.13 | 0.00 | **0.56** | **0.79** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| `0e71ff916060` | minimal | cross | 3 of 13, mean 0.19 | 3 of 13, mean 0.17 | **0.89** | 0.05 | 0.01 | **0.46** | **0.72** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.05 | 0.00 |
| `9e5de9d92e27` | minimal | cross | 3 of 13, mean 0.19 | 3 of 13, mean 0.17 | **0.87** | 0.07 | 0.02 | **0.50** | **0.68** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.02 | 0.00 |
| `6679f5778b38` | minimal | mutant | 3 of 13, mean 0.19 | 3 of 13, mean 0.18 | **0.88** | 0.14 | 0.00 | **0.59** | **0.72** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |
| `b5017acdcc2d` | minimal | cross | 3 of 13, mean 0.19 | 3 of 13, mean 0.16 | **0.75** | 0.09 | 0.00 | **0.54** | **0.69** | 0.00 | 0.00 | 0.00 | 0.00 | 0.01 | 0.00 | 0.02 | 0.00 |
| `adb3d963556a` | minimal | evidenceC | 3 of 13, mean 0.14 | 4 of 13, mean 0.17 | **0.50** | **0.53** | 0.07 | **0.42** | **0.48** | 0.19 | 0.03 | 0.00 | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 |

## Best minimal

On every world this engine runs (probes included for the full engine). The engine's defaults for comparison.

| candidate | engine | made as | selection | confirmation | echo2 | echo4 | echo8 | shuffled | gonogo | dist2 | dist3 | xor | order | gap | delayed | beat | sequence |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `edfe1224c68d` | minimal | cross | 3 of 13, mean 0.21 | 3 of 13, mean 0.19 | **0.88** | 0.15 | 0.01 | **0.62** | **0.81** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.03 | 0.00 |
| `e63d979330c9` | minimal | default | 3 of 13, mean 0.20 | 3 of 13, mean 0.19 | **0.81** | 0.22 | 0.00 | **0.66** | **0.80** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.02 | 0.00 |
| `63e25b283e45` | minimal | cross | 3 of 13, mean 0.20 | 3 of 13, mean 0.19 | **0.84** | 0.13 | 0.01 | **0.60** | **0.82** | 0.00 | 0.00 | 0.00 | 0.00 | 0.01 | 0.00 | 0.03 | 0.00 |
| `1a24160baece` | minimal | mutant | 3 of 13, mean 0.20 | 3 of 13, mean 0.17 | **0.85** | 0.13 | 0.00 | **0.50** | **0.77** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.02 | 0.00 |
| `fb773ede6fdc` | minimal | cross | 3 of 13, mean 0.19 | 3 of 13, mean 0.18 | **0.83** | 0.13 | 0.00 | **0.56** | **0.79** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |

## Best blob

On every world this engine runs (probes included for the full engine). The engine's defaults for comparison.

Nothing confirmed yet.

## Best table

On every world this engine runs (probes included for the full engine). The engine's defaults for comparison.

Nothing confirmed yet.

## Best per tier

Per tier, the confirmed candidate best on that tier's worlds on selection seeds, with its confirmation score there.

| tier | candidate | engine | confirmation in the tier |
| --- | --- | --- | --- |
| 1 | `edfe1224c68d` | minimal | 3 of 7, mean 0.35 |
| 2 | `cfa62dda63ea` | minimal | 0 of 1, mean 0.08 |
| 4 | `b5017acdcc2d` | minimal | 0 of 5, mean 0.00 |

## Best per world

Per world and engine, the confirmed candidate with the best selection score there; its confirmation score, and its raw late score beside the twin's (confirmation seeds, mean).

| world | engine | candidate | confirmation | late | twin |
| --- | --- | --- | --- | --- | --- |
| echo2 | minimal | `fb773ede6fdc` | 0.83 | 0.915 | 0.395 |
| echo4 | minimal | `adb3d963556a` | 0.53 | 0.646 | 0.193 |
| echo8 | minimal | `adb3d963556a` | 0.07 | 0.185 | 0.101 |
| shuffled | minimal | `63e25b283e45` | 0.60 | 0.802 | 0.375 |
| gonogo | minimal | `0e71ff916060` | 0.72 | 0.990 | 0.629 |
| dist2 | minimal | `adb3d963556a` | 0.19 | 0.594 | 0.375 |
| dist3 | minimal | `adb3d963556a` | 0.03 | 0.482 | 0.344 |
| xor | minimal | `cfa62dda63ea` | 0.08 | 0.518 | 0.374 |
| order | minimal | `e63d979330c9` | 0.00 | 0.345 | 0.408 |
| gap | minimal | `b5017acdcc2d` | 0.01 | 0.434 | 0.421 |
| delayed | minimal | `e63d979330c9` | 0.00 | 0.258 | 0.282 |
| beat | minimal | `9e5de9d92e27` | 0.02 | 0.498 | 0.478 |
| sequence | minimal | `e63d979330c9` | 0.00 | 1.000 | 1.000 |

## What a candidate costs

Seconds of one core for one world on one seed (the life and its twin, or one probe), mean over every run so far. Budget for one pass of the map: **200 s**.

| minimal | kmmm4.local | wegbox |
| --- | --- | --- |
| echo2 | - | 4.5 |
| echo4 | - | 4.8 |
| echo8 | - | 4.8 |
| shuffled | - | 4.5 |
| gonogo | - | 4.8 |
| dist2 | - | 4.9 |
| dist3 | - | 5.0 |
| xor | - | 4.3 |
| order | - | 4.0 |
| gap | - | 3.9 |
| delayed | - | 3.9 |
| beat | - | 4.1 |
| sequence | - | 3.9 |
| one pass | **0** | **57** |

| blob | kmmm4.local | wegbox |
| --- | --- | --- |
| echo2 | 141.0 | - |
| echo4 | 156.9 | - |
| echo8 | 149.7 | - |
| shuffled | 148.7 | - |
| gonogo | 155.4 | - |
| dist2 | 152.9 | - |
| dist3 | 149.0 | - |
| xor | 152.9 | - |
| order | 151.8 | - |
| gap | 147.1 | - |
| delayed | 143.5 | - |
| beat | 153.6 | - |
| sequence | 156.4 | - |
| f-xor | 18.3 | - |
| f-order | 29.6 | - |
| f-gap | 25.3 | - |
| f-hold20 | 29.5 | - |
| one pass | **2061 OVER BUDGET** | **0** |

| table | kmmm4.local | wegbox |
| --- | --- | --- |
| echo2 | 35.8 | - |
| echo4 | 64.2 | - |
| echo8 | 39.0 | - |
| shuffled | 54.9 | - |
| gonogo | 36.5 | - |
| dist2 | 36.8 | - |
| dist3 | 50.4 | - |
| xor | 38.7 | - |
| order | 46.1 | - |
| gap | 36.2 | - |
| delayed | 65.5 | - |
| beat | 51.6 | - |
| sequence | 35.8 | - |
| one pass | **592 OVER BUDGET** | **0** |

Candidates per hour per core (every candidate of the engine over every second its runs took, any host, all rungs):

| engine | candidates | core hours | per core hour | last report |
| --- | --- | --- | --- | --- |
| minimal | 94 | 1.8 | 52 | 10 |
| blob | 3 | 4.0 | 1 | - |
| table | 1 | 0.2 | 6 | - |

## Worlds: what each one tells us

Share of candidates passing each world on the first selection seed, and the world its scores track most closely (rank correlation over candidates).

| minimal | candidates | passing | best ever | tracks most closely |
| --- | --- | --- | --- | --- |
| echo2 | 93 | 67% | 0.98 | gonogo +0.64 |
| echo4 | 93 | 4% | 0.55 | echo2 +0.44 |
| echo8 | 43 | 0% | 0.13 | order +0.54 |
| shuffled | 43 | 93% | 1.00 | echo2 +0.62 |
| gonogo | 43 | 93% | 0.92 | echo2 +0.64 |
| dist2 | 43 | 0% | 0.27 | dist3 +0.64 |
| dist3 | 43 | 0% | 0.03 | xor +0.70 |
| xor | 94 | 0% | 0.02 | dist3 +0.70 |
| order | 43 | 0% | 0.00 | xor +0.61 |
| gap | 43 | 0% | 0.02 | dist3 +0.40 |
| delayed | 43 | 0% | 0.00 | dist3 +0.46 |
| beat | 43 | 0% | 0.07 | echo8 -0.23 |
| sequence | 43 | 0% | 0.00 | - +0.00 |

| blob | candidates | passing | best ever | tracks most closely |
| --- | --- | --- | --- | --- |
| echo2 | 2 | 100% | 1.00 | - +0.00 |
| echo4 | 3 | 67% | 1.00 | - +0.00 |
| echo8 | 2 | 50% | 0.57 | - +0.00 |
| shuffled | 1 | 100% | 0.71 | - +0.00 |
| gonogo | 1 | 100% | 0.33 | - +0.00 |
| dist2 | 1 | 0% | 0.00 | - +0.00 |
| dist3 | 2 | 50% | 0.85 | - +0.00 |
| xor | 2 | 0% | 0.27 | - +0.00 |
| order | 1 | 0% | 0.00 | - +0.00 |
| gap | 2 | 0% | 0.00 | - +0.00 |
| delayed | 2 | 0% | 0.04 | - +0.00 |
| beat | 2 | 0% | 0.05 | - +0.00 |
| sequence | 1 | 0% | 0.00 | - +0.00 |
| f-xor | 2 | 0% | 0.25 | - +0.00 |
| f-order | 2 | 100% | 0.55 | - +0.00 |
| f-gap | 2 | 0% | 0.22 | - +0.00 |
| f-hold20 | 2 | 0% | 0.23 | - +0.00 |

| table | candidates | passing | best ever | tracks most closely |
| --- | --- | --- | --- | --- |
| echo2 | 1 | 0% | 0.14 | - +0.00 |
| echo4 | 1 | 100% | 0.72 | - +0.00 |
| echo8 | 1 | 0% | 0.00 | - +0.00 |
| shuffled | 1 | 100% | 0.99 | - +0.00 |
| gonogo | 1 | 100% | 0.82 | - +0.00 |
| dist2 | 1 | 0% | 0.16 | - +0.00 |
| dist3 | 1 | 0% | 0.00 | - +0.00 |
| xor | 1 | 0% | 0.00 | - +0.00 |
| order | 1 | 0% | 0.02 | - +0.00 |
| gap | 1 | 0% | 0.00 | - +0.00 |
| delayed | 1 | 0% | 0.00 | - +0.00 |
| beat | 1 | 0% | 0.00 | - +0.00 |
| sequence | 1 | 0% | 0.00 | - +0.00 |

Candidates to cut from the quick map (judged after 50 candidates):

- xor (minimal): no candidate has passed it (best 0.02)

## Which numbers matter

Rank correlation of each number with each world's score on the first selection seed (|r| 0.15 or more shown). For a choice (a mode), the largest difference between one value's mean score and the mean of all. Mutants of good parents are over-represented, so these are hints, not measurements.

| minimal (94 candidates) | echo2 | echo4 | echo8 | shuffled | gonogo | dist2 | dist3 | xor | order | gap | delayed | beat | sequence |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| --motor-side | +0.31 | . | . | . | . | . | . | -0.05 | . | . | . | . | . |
| --reward-ticks | -0.22 | -0.29 | . | . | . | . | . | +0.17 | . | . | . | . | . |
| --motor-spread | +0.06 | . | . | . | . | . | . | . | . | . | . | . | . |
| --motor-spread-even | . | . | . | . | . | -0.30 | -0.33 | . | . | -0.24 | . | +0.36 | . |
| --g.inhibitory_percent | -0.33 | -0.15 | . | -0.22 | -0.47 | . | . | . | -0.37 | . | . | -0.22 | . |
| --m.budget | -0.23 | -0.19 | -0.30 | . | +0.17 | . | . | . | -0.33 | -0.25 | -0.18 | . | . |
| --m.budget_on | +0.24 | . | . | . | . | -0.16 | -0.16 | . | -0.16 | . | . | -0.16 | . |
| --m.inhibit_gain | . | . | +0.16 | . | . | +0.24 | . | +0.24 | +0.16 | . | . | . | . |
| --m.threshold_min | -0.30 | -0.23 | +0.26 | -0.17 | -0.17 | +0.24 | +0.20 | +0.18 | +0.21 | . | . | . | . |
| --m.threshold_max | -0.25 | -0.28 | +0.16 | -0.27 | . | +0.23 | +0.35 | +0.21 | +0.18 | . | +0.17 | . | . |
| --m.up_shift | -0.31 | -0.29 | -0.21 | -0.35 | -0.33 | . | . | +0.36 | . | . | . | -0.16 | . |
| --m.down_shift | -0.17 | . | -0.19 | -0.26 | -0.22 | -0.26 | -0.26 | . | -0.19 | -0.22 | -0.19 | . | . |
| --m.noise_shift | . | . | +0.21 | +0.25 | . | -0.26 | -0.41 | -0.22 | . | . | -0.45 | . | . |
| --m.credit_ticks | +0.19 | +0.22 | . | . | +0.19 | -0.36 | -0.20 | -0.21 | -0.26 | . | . | . | . |
| --m.learn_shift | -0.24 | -0.27 | -0.17 | -0.29 | . | +0.15 | . | . | . | . | . | -0.19 | . |
| --m.avg_shift | . | . | +0.19 | +0.31 | +0.17 | . | . | -0.24 | . | -0.23 | -0.19 | -0.15 | . |
| --m.grade_mode | +0.16 | . | +0.45 | +0.16 | +0.22 | +0.38 | +0.61 | +0.37 | +0.46 | . | . | -0.29 | . |
| --m.drop_mode | -0.21 | . | . | -0.28 | -0.29 | +0.28 | +0.32 | . | +0.21 | . | +0.23 | . | . |
| --m.cowire_on | +0.31 | +0.15 | . | . | . | . | . | -0.28 | . | . | . | . | . |
| --m.cowire_shift | . | . | . | -0.30 | -0.33 | . | +0.18 | . | . | . | +0.18 | . | . |
| --m.cowire_needs_input | . | . | -0.20 | -0.21 | . | . | -0.24 | . | -0.17 | . | . | . | . |
| --m.far_slots | . | . | . | . | . | -0.32 | . | . | -0.21 | +0.30 | . | . | . |
| --m.delay_max | -0.38 | -0.33 | . | -0.39 | -0.41 | . | . | . | . | . | . | +0.19 | . |
| --m.delay_percent | +0.19 | +0.21 | . | . | . | . | . | . | . | . | . | . | . |
| --m.ev_on | +0.05 | . | . | . | . | . | . | . | . | . | . | . | . |
| --m.ev_bits | -0.26 | . | +0.35 | -0.31 | -0.29 | +0.35 | +0.19 | . | +0.29 | . | +0.30 | -0.28 | . |
| --m.ev_init | . | +0.20 | +0.35 | -0.31 | -0.29 | +0.35 | +0.19 | . | +0.29 | . | +0.30 | -0.28 | . |
| --m.ev_fired_min | -0.24 | -0.26 | . | . | . | . | . | . | . | . | . | . | . |
| --m.ev_quiet_shift | -0.25 | -0.22 | . | . | . | . | . | +0.28 | . | . | . | . | . |
| --m.ev_shift | -0.23 | -0.19 | . | . | . | . | . | . | . | . | . | . | . |
| --m.ev_inh | . | . | . | . | . | . | . | +0.16 | . | . | . | . | . |
| --m.ev_cowire | +0.17 | +0.15 | . | . | . | . | . | -0.21 | . | . | . | . | . |
| --m.noise_sign | +0.06 | . | . | . | . | . | . | . | . | . | . | . | . |
| --m.noise_pair | -0.18 | . | . | -0.28 | -0.31 | -0.21 | . | . | . | +0.20 | . | . | . |

minimal, no effect seen on any world (times it was off its default):

`--m.far_percent` (3), `--m.ev_read` (3), `--m.ev_quiet` (2)

blob: 3 candidates, too few yet.

table: 1 candidates, too few yet.

Numbers to cut from the space files (no effect after 200 or more candidates of their engine):

- none flagged yet

## The settings

Every candidate named above, as flags (only what differs from the engine's defaults; a rule table is in out/search/<name>/rules/<hash>.hex).

- `e63d979330c9` minimal default: `(defaults)`
- `cfa62dda63ea` minimal evidenceA: `--m.ev_bits 6 --m.ev_init 4 --m.ev_on 1 --m.ev_spikes 1`
- `adb3d963556a` minimal evidenceC: `--m.ev_bits 6 --m.ev_init 4 --m.ev_on 2 --m.ev_spikes 1`
- `0bf64bf422ca` minimal fresh: `--m.avg_shift 4 --m.cowire_shift 9 --m.credit_ticks 6 --m.drop_mode 1 --m.learn_shift 3 --m.noise_shift 6 --m.threshold_cap 128 --m.threshold_max 5 --m.up_shift 6 --motor-side 2 --ticks 16`
- `b5017acdcc2d` minimal cross: `--m.delay_max 2`
- `63e25b283e45` minimal cross: `--m.credit_ticks 21`
- `6679f5778b38` minimal mutant: `--m.credit_ticks 21 --m.noise_shift 10`
- `fb773ede6fdc` minimal cross: `--m.noise_shift 10`
- `9e5de9d92e27` minimal cross: `--motor-side 4`
- `1a24160baece` minimal mutant: `--m.avg_shift 6`
- `edfe1224c68d` minimal cross: `--g.inhibitory_percent 23`
- `0e71ff916060` minimal cross: `--g.inhibitory_percent 23 --motor-side 4`

