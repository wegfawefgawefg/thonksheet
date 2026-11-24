# Scoreboard

Written by `scripts/scoreboard.sh` from `out/scorecards/`; do not edit by hand. One column per engine or variant (`scripts/suite.sh <name>`). A cell is the mean over the report seeds, ± the spread (standard deviation), and in brackets the same engine with learning off. Chance is coin flips. Scores are the late score, the mean of the last tenth of the life's windows, unless the test says otherwise. What each test is: `docs/bench.md`, `docs/curriculum.md`, `docs/notes-world.md`.

- **blob-main**: flags `(none)`; seeds 5 6 7 8 9 10 11 12; commit 7ce8bc7, 2026-11-24; 56 min, 2 runs at a time; frozen readout: yes.
- **minimal**: flags `--engine minimal`; seeds 5 6 7 8 9 10 11 12; commit 66416e1, 2026-11-24; 11 min, 2 runs at a time; frozen readout: not in this engine.
- **table**: flags `--table`; seeds 5 6 7 8 9 10 11 12; commit b32208f, 2026-11-24; 63 min, 2 runs at a time; frozen readout: not in this engine.

Speed figures: single core under nice -n 19, CPU clock capped at about 4.6 GHz, other agents' runs on the machine: not comparable with quiet runs.

| test | chance | blob-main | minimal | table |
| --- | --- | --- | --- | --- |
| **learns at all** | | | | |
| echo k=2 | 0.47 | 0.87 ±0.14 (0.48) | 0.94 ±0.02 (0.45) | 0.90 ±0.08 (0.44) |
| echo k=4 | 0.23 | 0.31 ±0.08 (0.15) | 0.41 ±0.08 (0.20) | 0.68 ±0.29 (0.21) |
| echo k=8 | 0.11 | 0.09 ±0.03 (0.11) | 0.06 ±0.02 (0.10) | 0.12 ±0.00 (0.12) |
| shuffled echo k=2 | 0.48 | 0.76 ±0.35 (0.49) | 0.87 ±0.11 (0.39) | 0.78 ±0.15 (0.47) |
| **a sense of time** | | | | |
| delayed echo k=2, 2 steps | 0.48 | 0.44 ±0.02 (0.45) | 0.24 ±0.01 (0.30) | 0.41 ±0.01 (0.46) |
| beat, every 4 steps | 0.51 | 0.50 ±0.02 (0.51) | 0.51 ±0.01 (0.49) | 0.44 ±0.01 (0.48) |
| sequence, 8 lines x 4 (prediction) | 0.50 | 0.43 ±0.04 (0.57) | 1.00 ±0.00 (1.00) | 0.50 ±0.01 (0.50) |
| sequence, surprise falling (late - early) | 0.00 | -0.01 ±0.01 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) |
| **acting in a world** | | | | |
| catch 4x4 | 0.25 | 0.25 ±0.02 (0.24) | 0.25 ±0.01 (0.26) | 0.25 ±0.01 (0.25) |
| cartpole, lean reward (steps per fall) | 23.3 | 14.4 ±1.64 (19.5) | 17.2 ±1.33 (18.6) | 16.5 ±1.57 (13.9) |
| cartpole, sparse reward (steps per fall) | 23.3 | 15.9 ±3.10 (19.5) | 18.8 ±0.77 (19.7) | 13.9 ±2.09 (13.6) |
| **reach** | | | | |
| cue readable how far (tiles) |  | 4.38 ±0.74 (3.38) | 2.62 ±0.74 (2.25) | 2.75 ±1.04 (2.75) |
| cue readable at 1 tiles |  | 16.9 ±1.04 (9.54) | 4.43 ±0.22 (2.05) | 3.93 ±0.48 (3.93) |
| cue readable at 2 tiles |  | 2.95 ±0.22 (1.90) | 1.18 ±0.21 (0.56) | 1.17 ±0.45 (1.17) |
| cue readable at 3 tiles |  | 0.93 ±0.25 (0.48) | 0.33 ±0.19 (0.16) | 0.28 ±0.15 (0.28) |
| cue readable at 4 tiles |  | 0.77 ±0.45 (0.23) | 0.16 ±0.15 (0.11) | 0.14 ±0.11 (0.14) |
| cue readable at 5 tiles |  | 0.23 ±0.13 (0.11) | 0.11 ±0.11 (0.08) | 0.12 ±0.14 (0.12) |
| cue readable at 6 tiles |  | 0.09 ±0.06 (0.06) | 0.10 ±0.10 (0.05) | 0.10 ±0.11 (0.10) |
| cue readable at 7 tiles |  | 0.06 ±0.04 (0.05) | 0.10 ±0.11 (0.04) | 0.07 ±0.09 (0.07) |
| echo k=2, cue and button 1 tiles apart | 0.47 | 0.87 ±0.14 (0.48) | 0.94 ±0.02 (0.45) | 0.90 ±0.08 (0.44) |
| echo k=2, cue and button 2 tiles apart | 0.48 | 0.51 ±0.14 (0.44) | 0.40 ±0.02 (0.37) | 0.81 ±0.24 (0.48) |
| echo k=2, cue and button 3 tiles apart | 0.48 | 0.51 ±0.06 (0.45) | 0.36 ±0.02 (0.36) | 0.53 ±0.19 (0.47) |
| echo k=2, cue and button 5 tiles apart | 0.48 | 0.48 ±0.03 (0.43) | 0.35 ±0.01 (0.35) | 0.43 ±0.01 (0.46) |
| **features nobody designed in** | | | | |
| control: echo k=2 (frozen sheet, readout learns) | 0.47 | 0.79 ±0.32 (0.48) | n/a | n/a |
| order: which of two cues came on first (frozen sheet, readout learns) | 0.48 | 0.44 ±0.02 (0.44) | n/a | n/a |
| both-not-either: exactly one of two cues (frozen sheet, readout learns) | 0.48 | 0.46 ±0.02 (0.46) | n/a | n/a |
| gap: short or long gap between two blinks (frozen sheet, readout learns) | 0.47 | 0.47 ±0.04 (0.45) | n/a | n/a |
| place: shape at a place never rewarded (frozen sheet, readout learns) | 0.47 | 0.49 ±0.03 (0.44) | n/a | n/a |
| order: which of two cues came on first (whole blob learns) | 0.48 | 0.42 ±0.04 (0.44) | 0.37 ±0.02 (0.39) | 0.45 ±0.03 (0.46) |
| both-not-either: exactly one of two cues (whole blob learns) | 0.48 | 0.45 ±0.01 (0.46) | 0.26 ±0.03 (0.36) | 0.55 ±0.23 (0.46) |
| gap: short or long gap between two blinks (whole blob learns) | 0.47 | 0.48 ±0.03 (0.45) | 0.43 ±0.06 (0.40) | 0.44 ±0.04 (0.47) |
| place: shape at a place never rewarded (whole blob learns) | 0.47 | 0.48 ±0.03 (0.44) | 0.41 ±0.03 (0.39) | 0.39 ±0.07 (0.46) |
| **keeps learning, keeps what it had** | | | | |
| echo A at the end of learning it | 0.47 | 0.87 ±0.14 (0.48) | 0.94 ±0.02 (0.45) | 0.90 ±0.08 (0.44) |
| echo B (2 new cues and buttons) at its end | 0.23 | 0.13 ±0.15 (0.18) | 0.11 ±0.01 (0.14) | 0.96 ±0.05 (0.18) |
| echo A retested after B, first window | 0.46 | 0.81 ±0.19 (0.48) | 0.43 ±0.06 (0.50) | 0.61 ±0.15 (0.46) |
| echo A retested after B, end | 0.48 | 0.89 ±0.16 (0.47) | 0.93 ±0.03 (0.43) | 0.90 ±0.12 (0.45) |
| capacity: largest k held (2..16) | 0.00 | 1.50 ±0.93 (0.25) | 2.00 ±0.00 (0.00) | 3.50 ±0.93 (0.00) |
| echo k=16 | 0.05 | 0.06 ±0.03 (0.06) | 0.03 ±0.01 (0.06) | 0.06 ±0.00 (0.05) |
| **does not wreck itself idle** | | | | |
| echo before unplugging | 0.47 | 0.87 ±0.14 (0.48) | 0.94 ±0.02 (0.45) | 0.90 ±0.08 (0.44) |
| echo after unplugged, first window | 0.47 | 0.87 ±0.13 (0.49) | 0.41 ±0.08 (0.46) | 0.91 ±0.10 (0.43) |
| echo after unplugged, end | 0.48 | 0.81 ±0.15 (0.48) | 0.93 ±0.04 (0.45) | 0.92 ±0.08 (0.45) |
| **room** | | | | |
| connections per neuron, end of echo k=2 life |  | 93.9 ±0.25 (63.1) | n/a | n/a |
| share of connections at strength 1, end of echo k=2 life |  | 0.04 ±0.03 (0.76) | n/a | n/a |
| share of connections at strength 4, end of echo k=2 life |  | 0.05 ±0.04 (0.24) | n/a | n/a |
| share of connections at strength 5 (top), end of echo k=2 life |  | 0.90 ±0.06 (0.00) | n/a | n/a |
| share of neurons at the connection cap, end of echo k=2 life |  | 0.92 ±0.05 (0.02) | n/a | n/a |
| share of neurons with threshold at cap, end of echo k=2 life |  | 0.19 ±0.04 (0.00) | n/a | n/a |
| **stays alive** | | | | |
| firing rate over target, end of echo k=2 life |  | 1.57 ±0.21 (1.02) | 1.00 ±0.01 (1.00) | 1.11 ±0.10 (2.16) |
| share of silent neurons, end of echo k=2 life |  | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) |
| share of stuck neurons, end of echo k=2 life |  | 0.00 ±0.01 (0.00) | 0.00 ±0.00 (0.00) | 0.02 ±0.01 (0.07) |
| rhythm period (ticks, 0 none), end of echo k=2 life |  | 57.9 ±9.19 (24.4) | 263 ±19.8 (251) | 4.50 ±11.2 (160) |
| rhythm strength, end of echo k=2 life |  | 0.38 ±0.21 (0.56) | 0.19 ±0.05 (0.19) | 0.11 ±0.23 (1.00) |
| **fits the machine** | | | | |
| ticks/s in the echo k=2 life |  | 42971 ±1330 (63809) | 227054 ±1898 (259103) | 39152 ±132 (40441) |
| ticks/s at 8x8 tiles, random input |  | 53462 | 214196 | 40502 |
| ticks/s at 16x16 tiles, random input |  | 13241 | 53024 | 9833 |
| ticks/s at 32x32 tiles, random input |  | 3396 | 12146 | 2443 |
| bytes per neuron (32x32 tiles) |  | 210 | 84.6 | 181 |
