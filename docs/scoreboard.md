# Scoreboard

Written by `scripts/scoreboard.sh` from `out/scorecards/`; do not edit by hand. One column per engine or variant (`scripts/suite.sh <name>`). A cell is the mean over the report seeds, ± the spread (standard deviation), and in brackets the same engine with learning off. Chance is coin flips. Scores are the late score, the mean of the last tenth of the life's windows, unless the test says otherwise. What each test is: `docs/bench.md`, `docs/curriculum.md`, `docs/notes-world.md`.

- **blob-main**: flags `(none)`; seeds 5 6 7 8 9 10 11 12; commit 7ce8bc7, 2026-11-24; 56 min, 2 runs at a time; frozen readout: yes.

Speed figures: single core under nice -n 19, CPU clock capped at about 4.6 GHz, other agents' runs on the machine: not comparable with quiet runs.

| test | chance | blob-main |
| --- | --- | --- |
| **learns at all** | | |
| echo k=2 | 0.47 | 0.87 ±0.14 (0.48) |
| echo k=4 | 0.23 | 0.31 ±0.08 (0.15) |
| echo k=8 | 0.11 | 0.09 ±0.03 (0.11) |
| shuffled echo k=2 | 0.48 | 0.76 ±0.35 (0.49) |
| **a sense of time** | | |
| delayed echo k=2, 2 steps | 0.48 | 0.44 ±0.02 (0.45) |
| beat, every 4 steps | 0.51 | 0.50 ±0.02 (0.51) |
| sequence, 8 lines x 4 (prediction) | 0.50 | 0.43 ±0.04 (0.57) |
| sequence, surprise falling (late - early) | 0.00 | -0.01 ±0.01 (0.00) |
| **acting in a world** | | |
| catch 4x4 | 0.25 | 0.25 ±0.02 (0.24) |
| cartpole, lean reward (steps per fall) | 23.3 | 14.4 ±1.64 (19.5) |
| cartpole, sparse reward (steps per fall) | 23.3 | 15.9 ±3.10 (19.5) |
| **reach** | | |
| cue readable how far (tiles) |  | 4.38 ±0.74 (3.38) |
| cue readable at 1 tiles |  | 16.9 ±1.04 (9.54) |
| cue readable at 2 tiles |  | 2.95 ±0.22 (1.90) |
| cue readable at 3 tiles |  | 0.93 ±0.25 (0.48) |
| cue readable at 4 tiles |  | 0.77 ±0.45 (0.23) |
| cue readable at 5 tiles |  | 0.23 ±0.13 (0.11) |
| cue readable at 6 tiles |  | 0.09 ±0.06 (0.06) |
| cue readable at 7 tiles |  | 0.06 ±0.04 (0.05) |
| echo k=2, cue and button 1 tiles apart | 0.47 | 0.87 ±0.14 (0.48) |
| echo k=2, cue and button 2 tiles apart | 0.48 | 0.51 ±0.14 (0.44) |
| echo k=2, cue and button 3 tiles apart | 0.48 | 0.51 ±0.06 (0.45) |
| echo k=2, cue and button 5 tiles apart | 0.48 | 0.48 ±0.03 (0.43) |
| **features nobody designed in** | | |
| control: echo k=2 (frozen sheet, readout learns) | 0.47 | 0.79 ±0.32 (0.48) |
| order: which of two cues came on first (frozen sheet, readout learns) | 0.48 | 0.44 ±0.02 (0.44) |
| both-not-either: exactly one of two cues (frozen sheet, readout learns) | 0.48 | 0.46 ±0.02 (0.46) |
| gap: short or long gap between two blinks (frozen sheet, readout learns) | 0.47 | 0.47 ±0.04 (0.45) |
| place: shape at a place never rewarded (frozen sheet, readout learns) | 0.47 | 0.49 ±0.03 (0.44) |
| order: which of two cues came on first (whole blob learns) | 0.48 | 0.42 ±0.04 (0.44) |
| both-not-either: exactly one of two cues (whole blob learns) | 0.48 | 0.45 ±0.01 (0.46) |
| gap: short or long gap between two blinks (whole blob learns) | 0.47 | 0.48 ±0.03 (0.45) |
| place: shape at a place never rewarded (whole blob learns) | 0.47 | 0.48 ±0.03 (0.44) |
| **keeps learning, keeps what it had** | | |
| echo A at the end of learning it | 0.47 | 0.87 ±0.14 (0.48) |
| echo B (2 new cues and buttons) at its end | 0.23 | 0.13 ±0.15 (0.18) |
| echo A retested after B, first window | 0.46 | 0.81 ±0.19 (0.48) |
| echo A retested after B, end | 0.48 | 0.89 ±0.16 (0.47) |
| capacity: largest k held (2..16) | 0.00 | 1.50 ±0.93 (0.25) |
| echo k=16 | 0.05 | 0.06 ±0.03 (0.06) |
| **does not wreck itself idle** | | |
| echo before unplugging | 0.47 | 0.87 ±0.14 (0.48) |
| echo after unplugged, first window | 0.47 | 0.87 ±0.13 (0.49) |
| echo after unplugged, end | 0.48 | 0.81 ±0.15 (0.48) |
| **room** | | |
| connections per neuron, end of echo k=2 life |  | 93.9 ±0.25 (63.1) |
| share of connections at strength 1, end of echo k=2 life |  | 0.04 ±0.03 (0.76) |
| share of connections at strength 4, end of echo k=2 life |  | 0.05 ±0.04 (0.24) |
| share of connections at strength 5 (top), end of echo k=2 life |  | 0.90 ±0.06 (0.00) |
| share of neurons at the connection cap, end of echo k=2 life |  | 0.92 ±0.05 (0.02) |
| share of neurons with threshold at cap, end of echo k=2 life |  | 0.19 ±0.04 (0.00) |
| **stays alive** | | |
| firing rate over target, end of echo k=2 life |  | 1.57 ±0.21 (1.02) |
| share of silent neurons, end of echo k=2 life |  | 0.00 ±0.00 (0.00) |
| share of stuck neurons, end of echo k=2 life |  | 0.00 ±0.01 (0.00) |
| rhythm period (ticks, 0 none), end of echo k=2 life |  | 57.9 ±9.19 (24.4) |
| rhythm strength, end of echo k=2 life |  | 0.38 ±0.21 (0.56) |
| **fits the machine** | | |
| ticks/s in the echo k=2 life |  | 42971 ±1330 (63809) |
| ticks/s at 8x8 tiles, random input |  | 53462 |
| ticks/s at 16x16 tiles, random input |  | 13241 |
| ticks/s at 32x32 tiles, random input |  | 3396 |
| bytes per neuron (32x32 tiles) |  | 210 |
