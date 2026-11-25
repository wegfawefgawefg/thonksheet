# Search leaderboard: main

Written by `scripts/search.sh report` on 2026-11-25 05:02. How to read it: docs/search.md.

Every candidate is one setting used unchanged on every world. Chosen and ordered on selection seeds 1 2 3; every score shown is from confirmation seeds 5 6 7 8, which played no part in choosing. A world's score is (late - the larger of the learning-off twin and chance) / (perfect - chance), held to 0..1 (probes: the reading scaled the same way); passed at 0.30 or more.

## Tried so far

| engine | candidates | reached rung 1 | rung 2 | confirmed | core hours |
| --- | --- | --- | --- | --- | --- |
| blob | 5 | 4 | 3 | 1 | 4.5 |
| minimal | 95 | 44 | 26 | 12 | 1.8 |
| table | 7 | 4 | 3 | 0 | 1.1 |

## Since the last report

Last report: 2026-11-25 04:43.
- blob: 2 new candidates (5 in all).
- minimal: 1 new candidates (95 in all).
- table: 6 new candidates (7 in all).
- new best on echo2 blob: `a4959c20c334` (confirmation 0.27, was nothing)
- new best on echo4 blob: `a4959c20c334` (confirmation 0.08, was nothing)
- new best on echo8 blob: `a4959c20c334` (confirmation 0.01, was nothing)
- new best on shuffled blob: `a4959c20c334` (confirmation 0.29, was nothing)
- new best on gonogo blob: `a4959c20c334` (confirmation 0.62, was nothing)
- new best on dist2 blob: `a4959c20c334` (confirmation 0.01, was nothing)
- new best on dist3 blob: `a4959c20c334` (confirmation 0.11, was nothing)
- new best on xor blob: `a4959c20c334` (confirmation 0.03, was nothing)
- new best on order blob: `a4959c20c334` (confirmation 0.02, was nothing)
- new best on gap blob: `a4959c20c334` (confirmation 0.00, was nothing)
- new best on delayed blob: `a4959c20c334` (confirmation 0.00, was nothing)
- new best on beat blob: `a4959c20c334` (confirmation 0.02, was nothing)
- new best on sequence blob: `a4959c20c334` (confirmation 0.00, was nothing)
- new best on f-xor blob: `a4959c20c334` (confirmation 0.25, was nothing)
- new best on f-order blob: `a4959c20c334` (confirmation 0.51, was nothing)
- new best on f-gap blob: `a4959c20c334` (confirmation 0.26, was nothing)
- new best on f-hold20 blob: `a4959c20c334` (confirmation 0.17, was nothing)

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
| `a4959c20c334` | blob | default | 3 of 13, mean 0.15 | 1 of 13, mean 0.11 | 0.27 | 0.08 | 0.01 | 0.29 | **0.62** | 0.01 | 0.11 | 0.03 | 0.02 | 0.00 | 0.00 | 0.02 | 0.00 |

## Best blob

On every world this engine runs (probes included for the full engine). The engine's defaults for comparison.

| candidate | engine | made as | selection | confirmation | echo2 | echo4 | echo8 | shuffled | gonogo | dist2 | dist3 | xor | order | gap | delayed | beat | sequence | f-xor | f-order | f-gap | f-hold20 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `a4959c20c334` | blob | default | 4 of 17, mean 0.19 | 2 of 17, mean 0.16 | 0.27 | 0.08 | 0.01 | 0.29 | **0.62** | 0.01 | 0.11 | 0.03 | 0.02 | 0.00 | 0.00 | 0.02 | 0.00 | 0.25 | **0.51** | 0.26 | 0.17 |

## Best minimal

On every world this engine runs (probes included for the full engine). The engine's defaults for comparison.

| candidate | engine | made as | selection | confirmation | echo2 | echo4 | echo8 | shuffled | gonogo | dist2 | dist3 | xor | order | gap | delayed | beat | sequence |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `edfe1224c68d` | minimal | cross | 3 of 13, mean 0.21 | 3 of 13, mean 0.19 | **0.88** | 0.15 | 0.01 | **0.62** | **0.81** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.03 | 0.00 |
| `e63d979330c9` | minimal | default | 3 of 13, mean 0.20 | 3 of 13, mean 0.19 | **0.81** | 0.22 | 0.00 | **0.66** | **0.80** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.02 | 0.00 |
| `63e25b283e45` | minimal | cross | 3 of 13, mean 0.20 | 3 of 13, mean 0.19 | **0.84** | 0.13 | 0.01 | **0.60** | **0.82** | 0.00 | 0.00 | 0.00 | 0.00 | 0.01 | 0.00 | 0.03 | 0.00 |
| `1a24160baece` | minimal | mutant | 3 of 13, mean 0.20 | 3 of 13, mean 0.17 | **0.85** | 0.13 | 0.00 | **0.50** | **0.77** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.02 | 0.00 |
| `fb773ede6fdc` | minimal | cross | 3 of 13, mean 0.19 | 3 of 13, mean 0.18 | **0.83** | 0.13 | 0.00 | **0.56** | **0.79** | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 | 0.00 |

## Best table

On every world this engine runs (probes included for the full engine). The engine's defaults for comparison.

Nothing confirmed yet.

## Best per tier

Per tier, the confirmed candidate best on that tier's worlds on selection seeds, with its confirmation score there.

| tier | candidate | engine | confirmation in the tier |
| --- | --- | --- | --- |
| 1 | `edfe1224c68d` | minimal | 3 of 7, mean 0.35 |
| 2 | `a4959c20c334` | blob | 0 of 2, mean 0.14 |
| 4 | `a4959c20c334` | blob | 1 of 8, mean 0.12 |

## Best per world

Per world and engine, the confirmed candidate with the best selection score there; its confirmation score, and its raw late score beside the twin's (confirmation seeds, mean).

| world | engine | candidate | confirmation | late | twin |
| --- | --- | --- | --- | --- | --- |
| echo2 | blob | `a4959c20c334` | 0.27 | 0.653 | 0.463 |
| echo2 | minimal | `fb773ede6fdc` | 0.83 | 0.915 | 0.395 |
| echo4 | blob | `a4959c20c334` | 0.08 | 0.277 | 0.139 |
| echo4 | minimal | `adb3d963556a` | 0.53 | 0.646 | 0.193 |
| echo8 | blob | `a4959c20c334` | 0.01 | 0.127 | 0.111 |
| echo8 | minimal | `adb3d963556a` | 0.07 | 0.185 | 0.101 |
| shuffled | blob | `a4959c20c334` | 0.29 | 0.439 | 0.465 |
| shuffled | minimal | `63e25b283e45` | 0.60 | 0.802 | 0.375 |
| gonogo | blob | `a4959c20c334` | 0.62 | 0.848 | 0.533 |
| gonogo | minimal | `0e71ff916060` | 0.72 | 0.990 | 0.629 |
| dist2 | blob | `a4959c20c334` | 0.01 | 0.463 | 0.417 |
| dist2 | minimal | `adb3d963556a` | 0.19 | 0.594 | 0.375 |
| dist3 | blob | `a4959c20c334` | 0.11 | 0.558 | 0.458 |
| dist3 | minimal | `adb3d963556a` | 0.03 | 0.482 | 0.344 |
| xor | blob | `a4959c20c334` | 0.03 | 0.470 | 0.459 |
| xor | minimal | `cfa62dda63ea` | 0.08 | 0.518 | 0.374 |
| order | blob | `a4959c20c334` | 0.02 | 0.432 | 0.407 |
| order | minimal | `e63d979330c9` | 0.00 | 0.345 | 0.408 |
| gap | blob | `a4959c20c334` | 0.00 | 0.424 | 0.430 |
| gap | minimal | `b5017acdcc2d` | 0.01 | 0.434 | 0.421 |
| delayed | blob | `a4959c20c334` | 0.00 | 0.437 | 0.427 |
| delayed | minimal | `e63d979330c9` | 0.00 | 0.258 | 0.282 |
| beat | blob | `a4959c20c334` | 0.02 | 0.498 | 0.511 |
| beat | minimal | `9e5de9d92e27` | 0.02 | 0.498 | 0.478 |
| sequence | blob | `a4959c20c334` | 0.00 | 0.446 | 0.566 |
| sequence | minimal | `e63d979330c9` | 0.00 | 1.000 | 1.000 |
| f-xor | blob | `a4959c20c334` | 0.25 | 1.750 | - |
| f-order | blob | `a4959c20c334` | 0.51 | 3.200 | - |
| f-gap | blob | `a4959c20c334` | 0.26 | 1.780 | - |
| f-hold20 | blob | `a4959c20c334` | 0.17 | 1.295 | - |

## What a candidate costs

Seconds of one core for one world on one seed (the life and its twin, or one probe), mean over every run so far. Budget for one pass of the map: **200 s**.

| blob | kmmm4.local | wegbox |
| --- | --- | --- |
| echo2 | 141.0 | 25.7 |
| echo4 | 156.9 | 21.0 |
| echo8 | 149.7 | 27.6 |
| shuffled | 148.7 | 31.4 |
| gonogo | 155.4 | 27.8 |
| dist2 | 152.9 | 29.3 |
| dist3 | 149.0 | 28.4 |
| xor | 152.9 | 26.5 |
| order | 151.8 | 21.9 |
| gap | 147.1 | 27.8 |
| delayed | 143.5 | 26.0 |
| beat | 153.6 | 30.4 |
| sequence | 156.4 | 17.9 |
| f-xor | 18.3 | 5.9 |
| f-order | 29.6 | 6.7 |
| f-gap | 25.3 | 2.2 |
| f-hold20 | 29.5 | 4.1 |
| one pass | **2061 OVER BUDGET** | **361 OVER BUDGET** |

| minimal | kmmm4.local | wegbox |
| --- | --- | --- |
| echo2 | - | 4.5 |
| echo4 | 12.6 | 4.8 |
| echo8 | 16.1 | 4.8 |
| shuffled | 10.9 | 4.5 |
| gonogo | 13.6 | 4.8 |
| dist2 | 15.6 | 4.9 |
| dist3 | 17.4 | 5.0 |
| xor | 9.9 | 4.3 |
| order | 15.2 | 4.0 |
| gap | 18.2 | 3.9 |
| delayed | 10.0 | 3.9 |
| beat | 22.9 | 4.1 |
| sequence | 24.0 | 3.9 |
| one pass | **186** | **57** |

| table | kmmm4.local | wegbox |
| --- | --- | --- |
| echo2 | 41.5 | - |
| echo4 | 48.8 | - |
| echo8 | 47.9 | - |
| shuffled | 50.3 | - |
| gonogo | 46.1 | - |
| dist2 | 48.9 | - |
| dist3 | 42.1 | - |
| xor | 38.8 | - |
| order | 48.2 | - |
| gap | 44.7 | - |
| delayed | 49.4 | - |
| beat | 46.9 | - |
| sequence | 43.8 | - |
| one pass | **597 OVER BUDGET** | **0** |

Candidates per hour per core (every candidate of the engine over every second its runs took, any host, all rungs):

| engine | candidates | core hours | per core hour | last report |
| --- | --- | --- | --- | --- |
| blob | 5 | 4.5 | 1 | 1 |
| minimal | 95 | 1.8 | 51 | 52 |
| table | 7 | 1.1 | 6 | 6 |

## Worlds: what each one tells us

Share of candidates passing each world on the first selection seed, and the world its scores track most closely (rank correlation over candidates).

| blob | candidates | passing | best ever | tracks most closely |
| --- | --- | --- | --- | --- |
| echo2 | 5 | 60% | 1.00 | echo4 +1.00 |
| echo4 | 5 | 40% | 1.00 | echo2 +1.00 |
| echo8 | 4 | 50% | 0.57 | echo2 +1.00 |
| shuffled | 4 | 75% | 0.98 | f-hold20 +0.95 |
| gonogo | 3 | 100% | 0.92 | echo2 +1.00 |
| dist2 | 4 | 50% | 0.96 | echo2 +1.00 |
| dist3 | 4 | 50% | 0.85 | echo2 +1.00 |
| xor | 5 | 0% | 0.27 | gap -1.00 |
| order | 4 | 0% | 0.05 | f-xor +0.95 |
| gap | 4 | 0% | 0.00 | xor -1.00 |
| delayed | 4 | 0% | 0.04 | gonogo +1.00 |
| beat | 3 | 0% | 0.05 | echo2 -1.00 |
| sequence | 3 | 33% | 0.44 | f-xor +1.00 |
| f-xor | 4 | 25% | 0.61 | sequence +1.00 |
| f-order | 4 | 100% | 1.00 | sequence +1.00 |
| f-gap | 4 | 50% | 0.99 | sequence +1.00 |
| f-hold20 | 4 | 0% | 0.23 | sequence -1.00 |

| minimal | candidates | passing | best ever | tracks most closely |
| --- | --- | --- | --- | --- |
| echo2 | 95 | 67% | 0.98 | shuffled +0.62 |
| echo4 | 95 | 4% | 0.55 | echo2 +0.44 |
| echo8 | 44 | 0% | 0.13 | order +0.50 |
| shuffled | 44 | 93% | 1.00 | echo2 +0.62 |
| gonogo | 44 | 93% | 0.92 | echo2 +0.61 |
| dist2 | 44 | 0% | 0.27 | dist3 +0.65 |
| dist3 | 44 | 0% | 0.03 | xor +0.69 |
| xor | 95 | 0% | 0.02 | dist3 +0.69 |
| order | 44 | 0% | 0.00 | xor +0.61 |
| gap | 44 | 0% | 0.02 | dist3 +0.42 |
| delayed | 44 | 0% | 0.00 | dist3 +0.47 |
| beat | 44 | 0% | 0.07 | echo8 -0.25 |
| sequence | 44 | 0% | 0.00 | - +0.00 |

| table | candidates | passing | best ever | tracks most closely |
| --- | --- | --- | --- | --- |
| echo2 | 6 | 33% | 0.97 | gonogo +1.00 |
| echo4 | 6 | 33% | 0.72 | shuffled +1.00 |
| echo8 | 3 | 0% | 0.00 | echo2 -0.50 |
| shuffled | 3 | 67% | 0.99 | echo4 +1.00 |
| gonogo | 3 | 67% | 0.96 | echo2 +1.00 |
| dist2 | 3 | 0% | 0.16 | echo4 +1.00 |
| dist3 | 3 | 0% | 0.00 | echo4 -1.00 |
| xor | 6 | 0% | 0.00 | shuffled -1.00 |
| order | 3 | 0% | 0.02 | echo4 +1.00 |
| gap | 3 | 0% | 0.03 | echo4 -1.00 |
| delayed | 3 | 0% | 0.00 | echo4 -1.00 |
| beat | 3 | 0% | 0.00 | sequence +1.00 |
| sequence | 4 | 0% | 0.00 | beat +1.00 |

Candidates to cut from the quick map (judged after 50 candidates):

- xor (minimal): no candidate has passed it (best 0.02)

## Which numbers matter

Rank correlation of each number with each world's score on the first selection seed (|r| 0.15 or more shown). For a choice (a mode), the largest difference between one value's mean score and the mean of all. Mutants of good parents are over-represented, so these are hints, not measurements.

blob: 5 candidates, too few yet.

| minimal (95 candidates) | echo2 | echo4 | echo8 | shuffled | gonogo | dist2 | dist3 | xor | order | gap | delayed | beat | sequence |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| --motor-side | +0.31 | . | . | . | . | . | . | -0.05 | . | . | . | . | . |
| --reward-ticks | -0.22 | -0.29 | . | . | . | . | . | +0.17 | . | . | . | . | . |
| --motor-spread | +0.05 | . | . | . | . | . | . | . | . | . | . | . | . |
| --motor-spread-even | . | . | . | . | . | -0.29 | -0.33 | . | . | -0.23 | . | +0.36 | . |
| --g.inhibitory_percent | -0.34 | -0.17 | . | -0.22 | -0.49 | . | . | . | -0.33 | . | . | -0.20 | . |
| --m.budget | -0.23 | -0.18 | -0.31 | . | +0.15 | . | . | . | -0.33 | -0.24 | -0.17 | . | . |
| --m.budget_on | +0.24 | . | . | . | . | -0.16 | -0.17 | . | -0.16 | . | . | -0.17 | . |
| --m.inhibit_gain | . | . | +0.21 | . | +0.19 | +0.16 | . | +0.24 | . | -0.15 | . | . | . |
| --m.threshold_min | -0.30 | -0.24 | +0.26 | -0.17 | -0.17 | +0.23 | +0.20 | +0.18 | +0.20 | . | . | . | . |
| --m.threshold_max | -0.26 | -0.29 | . | -0.27 | . | +0.24 | +0.35 | +0.21 | +0.19 | . | +0.17 | . | . |
| --m.up_shift | -0.31 | -0.29 | -0.21 | -0.34 | -0.33 | . | . | +0.36 | . | . | . | -0.15 | . |
| --m.down_shift | -0.19 | . | -0.19 | -0.26 | -0.22 | -0.26 | -0.26 | . | -0.19 | -0.22 | -0.19 | . | . |
| --m.noise_shift | . | . | +0.19 | +0.25 | . | -0.25 | -0.41 | -0.22 | . | . | -0.45 | . | . |
| --m.credit_ticks | +0.19 | +0.21 | . | . | +0.17 | -0.33 | -0.18 | -0.21 | -0.25 | . | . | . | . |
| --m.learn_shift | -0.24 | -0.26 | -0.17 | -0.29 | . | +0.16 | . | . | . | . | . | -0.18 | . |
| --m.avg_shift | . | . | +0.19 | +0.31 | +0.16 | . | . | -0.24 | . | -0.22 | -0.18 | -0.15 | . |
| --m.grade_mode | +0.16 | . | +0.43 | +0.16 | +0.20 | +0.39 | +0.61 | +0.38 | +0.46 | . | . | -0.27 | . |
| --m.drop_mode | -0.21 | . | . | -0.28 | -0.29 | +0.28 | +0.32 | . | +0.22 | . | +0.23 | . | . |
| --m.cowire_on | +0.31 | +0.15 | . | . | . | . | . | -0.28 | . | . | . | . | . |
| --m.cowire_shift | . | . | . | -0.30 | -0.32 | . | +0.18 | . | . | . | +0.18 | . | . |
| --m.cowire_needs_input | . | . | -0.20 | -0.21 | . | . | -0.23 | . | -0.17 | . | . | +0.15 | . |
| --m.far_slots | . | . | . | . | . | -0.30 | . | . | -0.21 | +0.30 | . | . | . |
| --m.delay_max | -0.37 | -0.32 | . | -0.39 | -0.41 | . | . | . | . | . | . | +0.20 | . |
| --m.delay_percent | +0.19 | +0.21 | . | . | . | . | . | . | . | . | . | . | . |
| --m.ev_on | +0.05 | . | . | . | . | . | . | . | . | . | . | . | . |
| --m.ev_bits | -0.26 | . | +0.34 | -0.31 | -0.29 | +0.34 | +0.19 | . | +0.29 | . | +0.30 | -0.28 | . |
| --m.ev_init | -0.16 | . | +0.34 | -0.31 | -0.29 | +0.34 | +0.19 | . | +0.29 | . | +0.30 | -0.28 | . |
| --m.ev_fired_min | -0.24 | -0.27 | . | . | . | . | . | . | . | . | . | . | . |
| --m.ev_quiet_shift | -0.25 | -0.22 | . | . | . | . | . | +0.28 | . | . | . | . | . |
| --m.ev_shift | -0.22 | -0.19 | . | . | . | . | . | . | . | . | . | . | . |
| --m.ev_inh | . | . | . | . | . | . | . | +0.16 | . | . | . | . | . |
| --m.ev_cowire | +0.17 | . | . | . | . | . | . | -0.21 | . | . | . | . | . |
| --m.noise_sign | +0.06 | . | . | . | . | . | . | . | . | . | . | . | . |
| --m.noise_pair | -0.18 | . | . | -0.28 | -0.31 | -0.20 | . | . | . | +0.20 | . | . | . |

minimal, no effect seen on any world (times it was off its default):

`--m.far_percent` (3), `--m.ev_read` (3), `--m.ev_quiet` (2)

table: 7 candidates, too few yet.

Numbers to cut from the space files (no effect after 200 or more candidates of their engine):

- none flagged yet

## The settings

Every candidate named above, as flags (only what differs from the engine's defaults; a rule table is in out/search/<name>/rules/<hash>.hex).

- `a4959c20c334` blob default: `(defaults)`
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

