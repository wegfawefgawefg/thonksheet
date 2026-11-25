# Scoreboard

Written by `scripts/scoreboard.sh` from `out/scorecards/`; do not edit by hand. One column per engine or variant (`scripts/suite.sh <name>`). A cell is the mean over the report seeds, ± the spread (standard deviation), and in brackets the same engine with learning off. Chance is coin flips. Scores are the late score, the mean of the last tenth of the life's windows, unless the test says otherwise. What each test is: `docs/bench.md`, `docs/curriculum.md`, `docs/notes-world.md`.

- **blob-main**: flags `(none)`; seeds 5 6 7 8 9 10 11 12; commit 7ce8bc7, 2026-11-24; 56 min, 2 runs at a time; frozen readout: yes.
- **blob-readout**: flags `--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128`; seeds 5 6 7 8 9 10 11 12; commit e733173, 2026-11-25; 55 min, 2 runs at a time; frozen readout: yes.
- **blob-rich2**: flags `--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --g.inhibitory_percent 34 --g.inhibit_gain 3 --g.threshold_min 6 --g.threshold_max 14 --g.cooldown_max 8 --g.local_connect_shift 2 --g.strong_shift 1 --g.far_percent 36 --g.far_reach_tiles 2 --g.local_delay_min 3 --g.local_delay_max 11 --g.branch_threshold 6 --g.branch_bonus 5 --g.noise_shift 9 --g.sector_target_per_1024 25 --g.sector_smooth_shift 6 --g.pacemakers_per_1024 11 --g.fan_in_max 103 --motor-spread 128`; seeds 5 6 7 8 9 10 11 12; commit bdf4df2, 2026-11-25; 97 min, 2 runs at a time; frozen readout: yes.
- **gru-fixed**: flags `--engine gru`; seeds 5 6 7 8 9 10 11 12; commit integrated main of 2026-11-25 (run on stevejobs), 2026-11-24; 83 min, 2 runs at a time; frozen readout: not in this engine.
- **gru-told**: flags `--engine gru --n.teach 1`; seeds 5 6 7 8 9 10 11 12; commit c8d0a78 (run on stevejobs), 2026-11-25; 42 min, 2 runs at a time, on stevejobs; frozen readout: not in this engine.
- **memory**: flags `--engine memory`; seeds 5 6 7 8 9 10 11 12; commit integrated main of 2026-11-25 (run on stevejobs), 2026-11-24; 46 min, 2 runs at a time; frozen readout: not in this engine.
- **minimal**: flags `--engine minimal`; seeds 5 6 7 8 9 10 11 12; commit 66416e1, 2026-11-24; 11 min, 2 runs at a time; frozen readout: not in this engine.
- **mlp-fixed**: flags `--engine mlp`; seeds 5 6 7 8 9 10 11 12; commit integrated main of 2026-11-25 (run on stevejobs), 2026-11-24; 9 min, 2 runs at a time; frozen readout: not in this engine.
- **mlp-told**: flags `--engine mlp --n.teach 1`; seeds 5 6 7 8 9 10 11 12; commit c8d0a78 (run on stevejobs), 2026-11-25; 4 min, 2 runs at a time, on stevejobs; frozen readout: not in this engine.
- **table**: flags `--table`; seeds 5 6 7 8 9 10 11 12; commit b32208f, 2026-11-24; 63 min, 2 runs at a time; frozen readout: not in this engine.
- **vm**: flags `--engine vm`; seeds 5 6 7 8 9 10 11 12; commit 00b2278, 2026-11-25; 54 min, 2 runs at a time; frozen readout: not in this engine.

Speed figures: single core under nice -n 19, CPU clock capped at about 4.6 GHz, other agents' runs on the machine: not comparable with quiet runs.

| test | chance | blob-main | blob-readout | blob-rich2 | gru-fixed | gru-told | memory | minimal | mlp-fixed | mlp-told | table | vm |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **learns at all** | | | | | | | | | | | | |
| echo k=2 | 0.47 | 0.87 ±0.14 (0.48) | 1.00 ±0.00 (0.52) | 0.89 ±0.03 (0.49) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.99 ±0.00 (0.50) | 0.94 ±0.02 (0.45) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.90 ±0.08 (0.44) | 1.00 ±0.00 (0.46) |
| echo k=4 | 0.23 | 0.31 ±0.08 (0.15) | 1.00 ±0.00 (0.25) | 0.90 ±0.03 (0.22) | 1.00 ±0.00 (0.25) | 1.00 ±0.00 (0.25) | 0.99 ±0.00 (0.26) | 0.41 ±0.08 (0.20) | 1.00 ±0.00 (0.25) | 1.00 ±0.00 (0.25) | 0.68 ±0.29 (0.21) | 1.00 ±0.00 (0.22) |
| echo k=8 | 0.11 | 0.09 ±0.03 (0.11) | 0.84 ±0.03 (0.12) | 0.69 ±0.07 (0.10) | 1.00 ±0.00 (0.12) | 1.00 ±0.00 (0.12) | 0.99 ±0.00 (0.12) | 0.06 ±0.02 (0.10) | 1.00 ±0.00 (0.12) | 1.00 ±0.00 (0.12) | 0.12 ±0.00 (0.12) | 1.00 ±0.00 (0.10) |
| shuffled echo k=2 | 0.48 | 0.76 ±0.35 (0.49) | 1.00 ±0.00 (0.52) | 0.89 ±0.05 (0.53) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.99 ±0.00 (0.50) | 0.87 ±0.11 (0.39) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.78 ±0.15 (0.47) | 1.00 ±0.00 (0.46) |
| **a sense of time** | | | | | | | | | | | | |
| delayed echo k=2, 2 steps | 0.48 | 0.44 ±0.02 (0.45) | 0.55 ±0.03 (0.49) | 0.89 ±0.08 (0.52) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.99 ±0.00 (0.50) | 0.24 ±0.01 (0.30) | 0.50 ±0.01 (0.50) | 0.50 ±0.02 (0.50) | 0.41 ±0.01 (0.46) | 1.00 ±0.00 (0.45) |
| beat, every 4 steps | 0.51 | 0.50 ±0.02 (0.51) | 0.50 ±0.02 (0.51) | 0.53 ±0.02 (0.50) | 1.00 ±0.00 (0.50) | 0.50 ±0.00 (0.50) | 0.99 ±0.01 (0.39) | 0.51 ±0.01 (0.49) | 0.64 ±0.01 (0.50) | 0.52 ±0.00 (0.50) | 0.44 ±0.01 (0.48) | 0.42 ±0.01 (0.50) |
| sequence, 8 lines x 4 (prediction) | 0.50 | 0.43 ±0.04 (0.57) | 0.57 ±0.02 (0.57) | 0.55 ±0.02 (0.55) | 0.50 ±0.00 (0.50) | 0.50 ±0.00 (0.50) | 1.00 ±0.00 (0.34) | 1.00 ±0.00 (1.00) | 0.50 ±0.00 (0.50) | 0.50 ±0.00 (0.50) | 0.50 ±0.01 (0.50) | 0.50 ±0.00 (0.50) |
| sequence, surprise falling (late - early) | 0.00 | -0.01 ±0.01 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (-0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) |
| **acting in a world** | | | | | | | | | | | | |
| catch 4x4 | 0.25 | 0.25 ±0.02 (0.24) | 0.32 ±0.01 (0.25) | 0.30 ±0.02 (0.25) | 0.72 ±0.01 (0.26) | 0.73 ±0.01 (0.26) | 0.99 ±0.00 (0.25) | 0.25 ±0.01 (0.26) | 0.49 ±0.01 (0.25) | 0.50 ±0.01 (0.25) | 0.25 ±0.01 (0.25) | 0.54 ±0.05 (0.25) |
| cartpole, lean reward (steps per fall) | 23.3 | 14.4 ±1.64 (19.5) | 17.6 ±0.44 (19.4) | 17.0 ±0.22 (17.9) | 798 ±331 (19.8) | 1000 ±0.00 (20.9) | 302 ±114 (21.3) | 17.2 ±1.33 (18.6) | 276 ±45.5 (20.4) | 1000 ±0.00 (21.9) | 16.5 ±1.57 (13.9) | 20.5 ±0.67 (22.8) |
| cartpole, sparse reward (steps per fall) | 23.3 | 15.9 ±3.10 (19.5) | 14.2 ±1.52 (18.9) | 13.7 ±1.14 (18.3) | 77.1 ±27.0 (20.0) | 969 ±57.9 (21.1) | 281 ±195 (8.29) | 18.8 ±0.77 (19.7) | 44.2 ±18.9 (20.8) | 758 ±176 (21.6) | 13.9 ±2.09 (13.6) | 43.5 ±30.9 (22.8) |
| **reach** | | | | | | | | | | | | |
| cue readable how far (tiles) |  | 4.38 ±0.74 (3.38) | 3.38 ±0.52 (3.38) | 2.25 ±0.46 (2.25) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 2.62 ±0.74 (2.25) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 2.75 ±1.04 (2.75) | 7.00 ±0.00 (5.25) |
| cue readable at 1 tiles |  | 16.9 ±1.04 (9.54) | 9.54 ±0.40 (9.54) | 11.8 ±0.84 (11.8) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 4.43 ±0.22 (2.05) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 3.93 ±0.48 (3.93) | 16.5 ±18.9 (0.27) |
| cue readable at 2 tiles |  | 2.95 ±0.22 (1.90) | 1.90 ±0.13 (1.90) | 1.10 ±0.09 (1.10) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 1.18 ±0.21 (0.56) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 1.17 ±0.45 (1.17) | 14.7 ±6.46 (0.24) |
| cue readable at 3 tiles |  | 0.93 ±0.25 (0.48) | 0.48 ±0.08 (0.48) | 0.25 ±0.15 (0.25) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.33 ±0.19 (0.16) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.28 ±0.15 (0.28) | 23.1 ±16.2 (0.37) |
| cue readable at 4 tiles |  | 0.77 ±0.45 (0.23) | 0.23 ±0.14 (0.23) | 0.10 ±0.04 (0.10) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.16 ±0.15 (0.11) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.14 ±0.11 (0.14) | 34.3 ±41.0 (0.30) |
| cue readable at 5 tiles |  | 0.23 ±0.13 (0.11) | 0.11 ±0.08 (0.11) | 0.08 ±0.03 (0.08) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.11 ±0.11 (0.08) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.12 ±0.14 (0.12) | 23.0 ±11.6 (0.39) |
| cue readable at 6 tiles |  | 0.09 ±0.06 (0.06) | 0.06 ±0.05 (0.06) | 0.07 ±0.04 (0.07) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.10 ±0.10 (0.05) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.10 ±0.11 (0.10) | 36.4 ±45.8 (0.29) |
| cue readable at 7 tiles |  | 0.06 ±0.04 (0.05) | 0.05 ±0.04 (0.05) | 0.06 ±0.06 (0.06) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.10 ±0.11 (0.04) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.07 ±0.09 (0.07) | 25.2 ±8.60 (0.29) |
| echo k=2, cue and button 1 tiles apart | 0.47 | 0.87 ±0.14 (0.48) | 1.00 ±0.00 (0.52) | 0.89 ±0.03 (0.49) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.99 ±0.00 (0.50) | 0.94 ±0.02 (0.45) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.90 ±0.08 (0.44) | 1.00 ±0.00 (0.46) |
| echo k=2, cue and button 2 tiles apart | 0.48 | 0.51 ±0.14 (0.44) | 0.91 ±0.03 (0.48) | 0.84 ±0.02 (0.48) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.99 ±0.00 (0.50) | 0.40 ±0.02 (0.37) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.81 ±0.24 (0.48) | 1.00 ±0.00 (0.45) |
| echo k=2, cue and button 3 tiles apart | 0.48 | 0.51 ±0.06 (0.45) | 0.85 ±0.04 (0.48) | 0.82 ±0.06 (0.48) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.99 ±0.00 (0.50) | 0.36 ±0.02 (0.36) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.53 ±0.19 (0.47) | 1.00 ±0.00 (0.45) |
| echo k=2, cue and button 5 tiles apart | 0.48 | 0.48 ±0.03 (0.43) | 0.59 ±0.04 (0.47) | 0.50 ±0.02 (0.47) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.99 ±0.00 (0.50) | 0.35 ±0.01 (0.35) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.43 ±0.01 (0.46) | 1.00 ±0.00 (0.46) |
| **features nobody designed in** | | | | | | | | | | | | |
| control: echo k=2 (frozen sheet, readout learns) | 0.47 | 0.79 ±0.32 (0.48) | 0.79 ±0.17 (0.52) | 0.66 ±0.29 (0.49) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| order: which of two cues came on first (frozen sheet, readout learns) | 0.48 | 0.44 ±0.02 (0.44) | 0.49 ±0.02 (0.48) | 0.48 ±0.03 (0.49) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| both-not-either: exactly one of two cues (frozen sheet, readout learns) | 0.48 | 0.46 ±0.02 (0.46) | 0.49 ±0.02 (0.49) | 0.50 ±0.02 (0.46) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| gap: short or long gap between two blinks (frozen sheet, readout learns) | 0.47 | 0.47 ±0.04 (0.45) | 0.49 ±0.02 (0.49) | 0.47 ±0.02 (0.48) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| place: shape at a place never rewarded (frozen sheet, readout learns) | 0.47 | 0.49 ±0.03 (0.44) | 0.49 ±0.01 (0.47) | 0.49 ±0.03 (0.48) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| order: which of two cues came on first (whole blob learns) | 0.48 | 0.42 ±0.04 (0.44) | 0.69 ±0.04 (0.48) | 0.86 ±0.02 (0.49) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.99 ±0.00 (0.50) | 0.37 ±0.02 (0.39) | 0.50 ±0.01 (0.50) | 0.50 ±0.02 (0.50) | 0.45 ±0.03 (0.46) | 0.87 ±0.12 (0.45) |
| both-not-either: exactly one of two cues (whole blob learns) | 0.48 | 0.45 ±0.01 (0.46) | 0.88 ±0.04 (0.49) | 0.94 ±0.02 (0.46) | 0.97 ±0.09 (0.49) | 1.00 ±0.00 (0.49) | 0.99 ±0.00 (0.50) | 0.26 ±0.03 (0.36) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.55 ±0.23 (0.46) | 0.96 ±0.02 (0.46) |
| gap: short or long gap between two blinks (whole blob learns) | 0.47 | 0.48 ±0.03 (0.45) | 0.52 ±0.02 (0.49) | 0.89 ±0.06 (0.48) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.99 ±0.00 (0.50) | 0.43 ±0.06 (0.40) | 0.49 ±0.02 (0.49) | 0.49 ±0.01 (0.49) | 0.44 ±0.04 (0.47) | 0.48 ±0.02 (0.45) |
| place: shape at a place never rewarded (whole blob learns) | 0.47 | 0.48 ±0.03 (0.44) | 0.25 ±0.08 (0.47) | 0.30 ±0.08 (0.48) | 0.16 ±0.12 (0.49) | 0.21 ±0.14 (0.49) | 0.63 ±0.34 (0.50) | 0.41 ±0.03 (0.39) | 0.16 ±0.10 (0.50) | 0.18 ±0.13 (0.50) | 0.39 ±0.07 (0.46) | 0.21 ±0.10 (0.46) |
| **keeps learning, keeps what it had** | | | | | | | | | | | | |
| echo A at the end of learning it | 0.47 | 0.87 ±0.14 (0.48) | 1.00 ±0.00 (0.52) | 0.89 ±0.03 (0.49) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.99 ±0.00 (0.50) | 0.94 ±0.02 (0.45) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.90 ±0.08 (0.44) | 1.00 ±0.00 (0.46) |
| echo B (2 new cues and buttons) at its end | 0.23 | 0.13 ±0.15 (0.18) | 0.99 ±0.00 (0.22) | 0.83 ±0.04 (0.25) | 1.00 ±0.00 (0.25) | 1.00 ±0.00 (0.25) | 0.99 ±0.00 (0.25) | 0.11 ±0.01 (0.14) | 1.00 ±0.00 (0.25) | 1.00 ±0.00 (0.25) | 0.96 ±0.05 (0.18) | 1.00 ±0.00 (0.21) |
| echo A retested after B, first window | 0.46 | 0.81 ±0.19 (0.48) | 0.98 ±0.01 (0.56) | 0.90 ±0.05 (0.52) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.50) | 0.43 ±0.06 (0.50) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.61 ±0.15 (0.46) | 0.96 ±0.02 (0.45) |
| echo A retested after B, end | 0.48 | 0.89 ±0.16 (0.47) | 1.00 ±0.00 (0.53) | 0.93 ±0.02 (0.50) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.51) | 0.93 ±0.03 (0.43) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.90 ±0.12 (0.45) | 1.00 ±0.00 (0.45) |
| capacity: largest k held (2..16) | 0.00 | 1.50 ±0.93 (0.25) | 8.00 ±0.00 (0.00) | 8.00 ±0.00 (0.00) | 16.0 ±0.00 (0.00) | 16.0 ±0.00 (0.00) | 16.0 ±0.00 (0.00) | 2.00 ±0.00 (0.00) | 16.0 ±0.00 (0.00) | 16.0 ±0.00 (0.00) | 3.50 ±0.93 (0.00) | 16.0 ±0.00 (0.00) |
| echo k=16 | 0.05 | 0.06 ±0.03 (0.06) | 0.31 ±0.04 (0.05) | 0.24 ±0.04 (0.04) | 1.00 ±0.00 (0.06) | 1.00 ±0.00 (0.06) | 0.98 ±0.00 (0.06) | 0.03 ±0.01 (0.06) | 1.00 ±0.00 (0.06) | 1.00 ±0.00 (0.06) | 0.06 ±0.00 (0.05) | 1.00 ±0.00 (0.05) |
| **does not wreck itself idle** | | | | | | | | | | | | |
| echo before unplugging | 0.47 | 0.87 ±0.14 (0.48) | 1.00 ±0.00 (0.52) | 0.89 ±0.03 (0.49) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.99 ±0.00 (0.50) | 0.94 ±0.02 (0.45) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.90 ±0.08 (0.44) | 1.00 ±0.00 (0.46) |
| echo after unplugged, first window | 0.47 | 0.87 ±0.13 (0.49) | 1.00 ±0.00 (0.56) | 0.91 ±0.03 (0.54) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.99 ±0.00 (0.52) | 0.41 ±0.08 (0.46) | 1.00 ±0.00 (0.51) | 1.00 ±0.00 (0.51) | 0.91 ±0.10 (0.43) | 1.00 ±0.00 (0.43) |
| echo after unplugged, end | 0.48 | 0.81 ±0.15 (0.48) | 1.00 ±0.00 (0.53) | 0.88 ±0.02 (0.49) | 1.00 ±0.00 (0.49) | 1.00 ±0.00 (0.49) | 0.99 ±0.00 (0.50) | 0.93 ±0.04 (0.45) | 1.00 ±0.00 (0.50) | 1.00 ±0.00 (0.50) | 0.92 ±0.08 (0.45) | 1.00 ±0.00 (0.46) |
| **room** | | | | | | | | | | | | |
| connections per neuron, end of echo k=2 life |  | 93.9 ±0.25 (63.1) | 60.3 ±0.10 (63.1) | 97.9 ±0.23 (98.8) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| share of connections at strength 1, end of echo k=2 life |  | 0.04 ±0.03 (0.76) | 0.75 ±0.00 (0.76) | 0.49 ±0.00 (0.52) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| share of connections at strength 4, end of echo k=2 life |  | 0.05 ±0.04 (0.24) | 0.25 ±0.00 (0.24) | 0.51 ±0.00 (0.48) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| share of connections at strength 5 (top), end of echo k=2 life |  | 0.90 ±0.06 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| share of neurons at the connection cap, end of echo k=2 life |  | 0.92 ±0.05 (0.02) | 0.01 ±0.00 (0.02) | 0.68 ±0.01 (0.72) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| share of neurons with threshold at cap, end of echo k=2 life |  | 0.19 ±0.04 (0.00) | 0.01 ±0.00 (0.00) | 0.02 ±0.00 (0.01) | n/a | n/a | n/a | n/a | n/a | n/a | n/a | n/a |
| **stays alive** | | | | | | | | | | | | |
| firing rate over target, end of echo k=2 life |  | 1.57 ±0.21 (1.02) | 1.02 ±0.00 (1.02) | 1.13 ±0.02 (1.11) | 0.15 ±0.00 (0.15) | 0.15 ±0.00 (0.15) | 0.15 ±0.00 (0.15) | 1.00 ±0.01 (1.00) | 0.15 ±0.00 (0.15) | 0.15 ±0.00 (0.15) | 1.11 ±0.10 (2.16) | 3.66 ±0.19 (5.09) |
| share of silent neurons, end of echo k=2 life |  | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 1.00 ±0.00 (1.00) | 1.00 ±0.00 (1.00) | 1.00 ±0.00 (1.00) | 0.00 ±0.00 (0.00) | 1.00 ±0.00 (1.00) | 1.00 ±0.00 (1.00) | 0.00 ±0.00 (0.00) | 0.77 ±0.03 (0.37) |
| share of stuck neurons, end of echo k=2 life |  | 0.00 ±0.01 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.02 ±0.01 (0.07) | 0.15 ±0.02 (0.23) |
| rhythm period (ticks, 0 none), end of echo k=2 life |  | 57.9 ±9.19 (24.4) | 24.8 ±0.89 (24.5) | 56.4 ±1.51 (56.2) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 263 ±19.8 (251) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 4.50 ±11.2 (160) | 0.00 ±0.00 (0.88) |
| rhythm strength, end of echo k=2 life |  | 0.38 ±0.21 (0.56) | 0.58 ±0.07 (0.56) | 0.59 ±0.09 (0.56) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.19 ±0.05 (0.19) | 0.00 ±0.00 (0.00) | 0.00 ±0.00 (0.00) | 0.11 ±0.23 (1.00) | 0.02 ±0.00 (0.04) |
| **fits the machine** | | | | | | | | | | | | |
| ticks/s in the echo k=2 life |  | 42971 ±1330 (63809) | 63337 ±1291 (62607) | 34093 ±3541 (38312) | 36449 ±728 (1278390) | 30268 ±889 (1004570) | 391204 ±17175 (526336) | 227054 ±1898 (259103) | 1181780 ±97167 (5673130) | 879206 ±181888 (4223710) | 39152 ±132 (40441) | 46968 ±1024 (45879) |
| ticks/s at 8x8 tiles, random input |  | 53462 | 64472 | 45985 | 2841280 | 1328000 | 333405 | 214196 | 2815460 | 1561970 | 40502 | 49100 |
| ticks/s at 16x16 tiles, random input |  | 13241 | 14638 | 10406 | 697386 | 223255 | 224865 | 53024 | 692647 | 379024 | 9833 | 17880 |
| ticks/s at 32x32 tiles, random input |  | 3396 | 3683 | 2209 | 169694 | 54052 | 94966 | 12146 | 169897 | 100944 | 2443 | 4949 |
| bytes per neuron (32x32 tiles) |  | 210 | 212 | 212 | 45.4 | 45.4 | 79.8 | 84.6 | 16.7 | 16.7 | 181 | 782 |
| **map: relation** | | | | | | | | | | | | |
| same-easy: same or different, side by side, 4 one-line items | 0.47 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.51) |  |  |  |
| same-hard: same or different, side by side, 3-of-6 items, 6 never taught | 0.46 |  |  |  | 0.87 ±0.04 (0.51) |  | 0.65 ±0.08 (0.50) |  | 0.96 ±0.01 (0.50) |  |  |  |
| same-then-easy: same or different, one after the other, 4 items | 0.47 |  |  |  | 0.96 ±0.06 (0.50) |  | 0.99 ±0.00 (0.51) |  | 0.49 ±0.02 (0.49) |  |  |  |
| same-then-hard: same or different, one after the other, 6 never taught | 0.47 |  |  |  | 0.72 ±0.10 (0.49) |  | 0.99 ±0.01 (0.51) |  | 0.49 ±0.03 (0.49) |  |  |  |
| odd-easy: odd one out of 3, 4 one-line items | 0.31 |  |  |  | 0.97 ±0.04 (0.33) |  | 0.99 ±0.00 (0.33) |  | 0.99 ±0.00 (0.33) |  |  |  |
| odd-hard: odd one out of 4, 2-of-5 items, 4 never taught | 0.22 |  |  |  | 0.48 ±0.06 (0.24) |  | 0.27 ±0.05 (0.25) |  | 0.70 ±0.08 (0.26) |  |  |  |
| bigger-easy: which of two is bigger, 6 levels | 0.47 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.51) |  |  |  |
| bigger-hard: which of two is bigger, 10 levels, always neighbours | 0.48 |  |  |  | 0.98 ±0.03 (0.50) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| analogy-easy: A:B as C:which, shift or mirror, 2 candidates | 0.48 |  |  |  | 0.50 ±0.02 (0.50) |  | 0.52 ±0.02 (0.50) |  | 0.51 ±0.01 (0.50) |  |  |  |
| analogy-hard: A:B as C:which, shift, mirror or add, C never taught | 0.29 |  |  |  | 0.33 ±0.04 (0.33) |  | 0.33 ±0.03 (0.33) |  | 0.33 ±0.02 (0.32) |  |  |  |
| **map: number** | | | | | | | | | | | | |
| how-many-easy: how many dots, 1 to 3 | 0.31 |  |  |  | 0.90 ±0.01 (0.33) |  | 0.95 ±0.01 (0.33) |  | 0.83 ±0.01 (0.34) |  |  |  |
| how-many-hard: how many items, 1 to 5, mixed sizes, area matched | 0.18 |  |  |  | 0.35 ±0.01 (0.20) |  | 0.34 ±0.01 (0.20) |  | 0.37 ±0.02 (0.20) |  |  |  |
| more-fewer-easy: more or fewer dots than last time, 1 to 5 | 0.47 |  |  |  | 0.98 ±0.00 (0.50) |  | 0.80 ±0.01 (0.51) |  | 0.78 ±0.01 (0.50) |  |  |  |
| more-fewer-hard: more or fewer than last time, mixed sizes | 0.47 |  |  |  | 0.78 ±0.01 (0.50) |  | 0.65 ±0.02 (0.50) |  | 0.70 ±0.02 (0.50) |  |  |  |
| **map: parts** | | | | | | | | | | | | |
| grammar-easy: Reber string allowed or not, up to 8 symbols | 0.48 |  |  |  | 0.70 ±0.04 (0.49) |  | 0.66 ±0.03 (0.51) |  | 0.51 ±0.02 (0.51) |  |  |  |
| grammar-hard: Reber string allowed or not, up to 14 symbols | 0.47 |  |  |  | 0.60 ±0.06 (0.50) |  | 0.61 ±0.02 (0.48) |  | 0.51 ±0.03 (0.50) |  |  |  |
| brackets-easy: brackets balanced or not, 8 symbols | 0.49 |  |  |  | 0.99 ±0.01 (0.50) |  | 0.85 ±0.03 (0.49) |  | 0.49 ±0.03 (0.50) |  |  |  |
| brackets-hard: brackets balanced or not, 12 symbols | 0.51 |  |  |  | 0.90 ±0.02 (0.49) |  | 0.76 ±0.03 (0.51) |  | 0.50 ±0.05 (0.50) |  |  |  |
| parts-easy: two parts of two kinds at fixed spots: the ordered pair | 0.23 |  |  |  | 1.00 ±0.00 (0.25) |  | 0.99 ±0.00 (0.24) |  | 1.00 ±0.00 (0.26) |  |  |  |
| parts-hard: two parts of three kinds anywhere, touching: the pair | 0.15 |  |  |  | 0.26 ±0.02 (0.16) |  | 0.86 ±0.02 (0.16) |  | 0.33 ±0.02 (0.17) |  |  |  |
| **map: chance** | | | | | | | | | | | | |
| noisy-echo-easy: echo, reward true 80 in 100 | 0.47 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.49) |  |  |  |
| noisy-echo-hard: echo, reward true 60 in 100 | 0.47 |  |  |  | 1.00 ±0.01 (0.49) |  | 0.73 ±0.04 (0.50) |  | 1.00 ±0.00 (0.49) |  |  |  |
| rare-cue-easy: a cue one try in 10 (balanced score) | 0.47 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| rare-cue-hard: a cue one try in 40 (balanced score) | 0.47 |  |  |  | 1.00 ±0.00 (0.48) |  | 1.00 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| noisy-lines-easy: 2 patterns on 8 lines, 10 in 100 flipped | 0.48 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.51) |  | 1.00 ±0.00 (0.50) |  |  |  |
| noisy-lines-hard: 4 patterns on 8 lines, 25 in 100 flipped | 0.23 |  |  |  | 0.66 ±0.05 (0.25) |  | 0.61 ±0.09 (0.25) |  | 0.66 ±0.05 (0.25) |  |  |  |
| **map: context** | | | | | | | | | | | | |
| context-easy: context line flips the rule, shown with the cue | 0.47 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| context-hard: 4 cues, the context a step before the cue | 0.24 |  |  |  | 0.89 ±0.08 (0.25) |  | 0.99 ±0.00 (0.24) |  | 0.49 ±0.01 (0.25) |  |  |  |
| switch-easy: rule changes every 500 tries, no signal | 0.48 |  |  |  | 0.75 ±0.00 (0.50) |  | 0.86 ±0.02 (0.50) |  | 0.51 ±0.04 (0.50) |  |  |  |
| switch-hard: rule changes every 50 tries, no signal | 0.48 |  |  |  | 0.50 ±0.01 (0.50) |  | 0.56 ±0.01 (0.50) |  | 0.53 ±0.05 (0.50) |  |  |  |
| **map: memory** | | | | | | | | | | | | |
| copy-easy: copy 2 cues of 2 back in order | 0.48 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.87 ±0.01 (0.50) |  | 0.50 ±0.01 (0.50) |  |  |  |
| copy-hard: copy 2 to 6 cues of 4 back in order | 0.22 |  |  |  | 0.55 ±0.01 (0.25) |  | 0.25 ±0.01 (0.25) |  | 0.25 ±0.01 (0.25) |  |  |  |
| recall-easy: the marked cue of 2, one other between | 0.47 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.49) |  | 0.50 ±0.01 (0.50) |  |  |  |
| recall-hard: the marked cue of 4, four others between | 0.24 |  |  |  | 0.99 ±0.01 (0.25) |  | 0.65 ±0.02 (0.24) |  | 0.25 ±0.01 (0.25) |  |  |  |
| n-back-easy: same as 1 step ago, 4 symbols | 0.48 |  |  |  | 0.97 ±0.06 (0.50) |  | 0.99 ±0.00 (0.50) |  | 0.50 ±0.00 (0.51) |  |  |  |
| n-back-hard: same as 2 steps ago, 4 symbols | 0.48 |  |  |  | 0.94 ±0.08 (0.50) |  | 0.99 ±0.00 (0.50) |  | 0.50 ±0.01 (0.50) |  |  |  |
| **map: no teacher** | | | | | | | | | | | | |
| predict-easy: next of 4 symbols from the last one (7 in 8) | 0.50 |  |  |  | 0.50 ±0.00 (0.50) |  | 0.85 ±0.00 (0.40) |  | 0.50 ±0.00 (0.50) |  |  |  |
| predict-hard: next of 4 symbols from the last two (7 in 8) | 0.50 |  |  |  | 0.50 ±0.00 (0.50) |  | 0.84 ±0.03 (0.56) |  | 0.50 ±0.00 (0.50) |  |  |  |
| fill-in-easy: 2 patterns on 8 lines, half blanked 1 step in 4 | -0.00 |  |  |  | 0.00 ±0.00 (0.00) |  | 0.01 ±0.01 (0.00) |  | 0.00 ±0.00 (0.00) |  |  |  |
| fill-in-hard: 6 patterns on 8 lines, half blanked 1 step in 4 | -0.00 |  |  |  | 0.00 ±0.00 (0.00) |  | 0.00 ±0.00 (0.00) |  | 0.00 ±0.00 (0.00) |  |  |  |
| odd-event-easy: a cycle of 4 symbols broken 1 step in 8 | 0.00 |  |  |  | 0.00 ±0.00 (0.00) |  | 0.47 ±0.01 (-0.21) |  | 0.00 ±0.00 (0.00) |  |  |  |
| odd-event-hard: a cycle of 6 symbols broken 1 step in 8 | 0.00 |  |  |  | 0.00 ±0.00 (0.00) |  | 0.42 ±0.01 (-0.11) |  | 0.00 ±0.00 (0.00) |  |  |  |
| **map: shown** | | | | | | | | | | | | |
| shown-echo2: echo k=2: shown the answer, then tested (no reward), teach_end | 0.47 |  |  |  | 0.49 ±0.02 (0.49) |  | 0.50 ±0.00 (0.50) |  | 0.50 ±0.01 (0.50) |  |  |  |
| shown-echo2: echo k=2: shown the answer, then tested (no reward), test_start | 0.47 |  |  |  | 0.49 ±0.04 (0.49) |  | 0.49 ±0.02 (0.49) |  | 0.51 ±0.03 (0.51) |  |  |  |
| shown-echo2: echo k=2: shown the answer, then tested (no reward), test_mean | 0.48 |  |  |  | 0.49 ±0.01 (0.49) |  | 0.50 ±0.01 (0.50) |  | 0.50 ±0.01 (0.50) |  |  |  |
| shown-echo4: echo k=4: shown the answer, then tested (no reward), teach_end | 0.23 |  |  |  | 0.25 ±0.01 (0.25) |  | 0.25 ±0.01 (0.25) |  | 0.25 ±0.01 (0.25) |  |  |  |
| shown-echo4: echo k=4: shown the answer, then tested (no reward), test_start | 0.21 |  |  |  | 0.26 ±0.04 (0.26) |  | 0.23 ±0.03 (0.23) |  | 0.27 ±0.02 (0.27) |  |  |  |
| shown-echo4: echo k=4: shown the answer, then tested (no reward), test_mean | 0.23 |  |  |  | 0.25 ±0.00 (0.25) |  | 0.25 ±0.00 (0.25) |  | 0.25 ±0.01 (0.25) |  |  |  |
| shown-xor: both-not-either: shown the answer, then tested (no reward), teach_end | 0.48 |  |  |  | 0.49 ±0.01 (0.49) |  | 0.50 ±0.01 (0.50) |  | 0.50 ±0.01 (0.50) |  |  |  |
| shown-xor: both-not-either: shown the answer, then tested (no reward), test_start | 0.45 |  |  |  | 0.51 ±0.04 (0.51) |  | 0.48 ±0.06 (0.48) |  | 0.49 ±0.08 (0.49) |  |  |  |
| shown-xor: both-not-either: shown the answer, then tested (no reward), test_mean | 0.48 |  |  |  | 0.50 ±0.01 (0.50) |  | 0.50 ±0.01 (0.50) |  | 0.50 ±0.01 (0.50) |  |  |  |
| **map: damage** | | | | | | | | | | | | |
| damage5: echo k=2, then 5 in 100 cells silenced, before | 0.47 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.49) |  |  |  |
| damage5: echo k=2, then 5 in 100 cells silenced, after_start | 0.47 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.52) |  | 1.00 ±0.00 (0.51) |  |  |  |
| damage5: echo k=2, then 5 in 100 cells silenced, after_end | 0.46 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| damage20: echo k=2, then 20 in 100 cells silenced, before | 0.47 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.49) |  |  |  |
| damage20: echo k=2, then 20 in 100 cells silenced, after_start | 0.47 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.52) |  | 1.00 ±0.00 (0.51) |  |  |  |
| damage20: echo k=2, then 20 in 100 cells silenced, after_end | 0.46 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| damage50: echo k=2, then 50 in 100 cells silenced, before | 0.47 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.49) |  |  |  |
| damage50: echo k=2, then 50 in 100 cells silenced, after_start | 0.47 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.52) |  | 1.00 ±0.00 (0.51) |  |  |  |
| damage50: echo k=2, then 50 in 100 cells silenced, after_end | 0.46 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| damage-tile: echo k=2, then the cue tile silenced, before | 0.47 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.49) |  |  |  |
| damage-tile: echo k=2, then the cue tile silenced, after_start | 0.47 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.52) |  | 1.00 ±0.00 (0.51) |  |  |  |
| damage-tile: echo k=2, then the cue tile silenced, after_end | 0.46 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| damage20-xor: both-not-either, then 20 in 100 cells silenced, before | 0.48 |  |  |  | 0.97 ±0.09 (0.49) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| damage20-xor: both-not-either, then 20 in 100 cells silenced, after_start | 0.45 |  |  |  | 0.97 ±0.07 (0.51) |  | 0.99 ±0.02 (0.51) |  | 1.00 ±0.00 (0.49) |  |  |  |
| damage20-xor: both-not-either, then 20 in 100 cells silenced, after_end | 0.48 |  |  |  | 0.97 ±0.09 (0.49) |  | 0.99 ±0.00 (0.51) |  | 1.00 ±0.00 (0.50) |  |  |  |
| **fixed functions (the battery)** | | | | | | | | | | | | |
| points: parity of 3 lines | 0.48 |  |  |  | 0.78 ±0.06 (0.50) |  | 0.99 ±0.00 (0.50) |  | 0.96 ±0.00 (0.49) |  |  |  |
| points: parity of 4 lines | 0.47 |  |  |  | 0.50 ±0.01 (0.49) |  | 0.99 ±0.00 (0.50) |  | 0.90 ±0.01 (0.50) |  |  |  |
| points: majority of 5 lines | 0.48 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  |  |  |
| points: how many of 4 lines are on (5 buttons) | 0.18 |  |  |  | 0.88 ±0.08 (0.20) |  | 0.99 ±0.00 (0.20) |  | 0.89 ±0.05 (0.20) |  |  |  |
| number: band of sine(x), 16 lines, 4 bands, x never rewarded | 0.24 |  |  |  | 0.36 ±0.08 (0.27) |  | 0.17 ±0.17 (0.24) |  | 0.35 ±0.04 (0.25) |  |  |  |
| two numbers: product above or below half, 8 lines each | 0.47 |  |  |  | 0.99 ±0.00 (0.50) |  | 0.99 ±0.00 (0.50) |  | 0.99 ±0.00 (0.50) |  |  |  |
| past: the first of 2 items, 2 blank steps between | 0.47 |  |  |  | 1.00 ±0.00 (0.51) |  | 0.99 ±0.00 (0.51) |  | 0.50 ±0.01 (0.50) |  |  |  |
| past: the last 2 items the same, back to back | 0.48 |  |  |  | 1.00 ±0.00 (0.49) |  | 0.99 ±0.00 (0.50) |  | 0.50 ±0.01 (0.50) |  |  |  |
| **more worlds (idea 63)** | | | | | | | | | | | | |
| bandit, 2 buttons (share on the best) | 0.47 |  |  |  | 1.00 ±0.00 (0.51) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.51) |  |  |  |
| bandit, 4 buttons (share on the best) | 0.23 |  |  |  | 1.00 ±0.00 (0.25) |  | 0.98 ±0.00 (0.25) |  | 1.00 ±0.00 (0.25) |  |  |  |
| switching bandit, every 1,000 steps (best share, late in each block) | 0.47 |  |  |  | 0.88 ±0.13 (0.51) |  | 0.99 ±0.00 (0.50) |  | 0.61 ±0.01 (0.50) |  |  |  |
| chain A then B, stage shown (2 x chains / steps) | 0.31 |  |  |  | 0.01 ±0.00 (0.33) |  | 0.00 ±0.00 (0.00) |  | 0.45 ±0.03 (0.33) |  |  |  |
| chain A then B, stage not shown (2 x chains / steps) | 0.31 |  |  |  | 0.01 ±0.00 (0.33) |  | 0.00 ±0.00 (0.00) |  | 0.03 ±0.01 (0.33) |  |  |  |
| track 5, nearer good (share of steps nearer) | 0.48 |  |  |  | 0.99 ±0.01 (0.50) |  | 0.00 ±0.00 (0.00) |  | 1.00 ±0.00 (0.49) |  |  |  |
| track 5, arrival only (share of steps nearer) | 0.47 |  |  |  | 0.91 ±0.02 (0.50) |  | 0.98 ±0.01 (0.30) |  | 0.88 ±0.07 (0.50) |  |  |  |
| forage 5x5, warmer (share of steps nearer) | 0.39 |  |  |  | 0.97 ±0.03 (0.43) |  | 0.77 ±0.31 (0.49) |  | 1.00 ±0.00 (0.43) |  |  |  |
| forage 5x5, arrival only (share of steps nearer) | 0.39 |  |  |  | 0.60 ±0.03 (0.43) |  | 0.70 ±0.02 (0.42) |  | 0.53 ±0.01 (0.43) |  |  |  |
| mountain car, energy reward (share of episodes at the goal) | 0.00 |  |  |  | 1.00 ±0.00 (0.00) |  | 1.00 ±0.01 (0.00) |  | 1.00 ±0.00 (0.00) |  |  |  |
| pendulum swing-up, energy reward (share of steps upright) | 0.06 |  |  |  | 0.22 ±0.01 (0.06) |  | 0.23 ±0.04 (0.05) |  | 0.19 ±0.01 (0.05) |  |  |  |
| cartpole, lean reward, wide starts (steps per fall) | 19.4 |  |  |  | 538 ±192 (18.4) |  | 98.7 ±27.1 (20.0) |  | 250 ±92.6 (19.1) |  |  |  |
| cartpole, relative reward (steps per fall) | 21.1 |  |  |  | 865 ±111 (19.7) |  | 75.8 ±24.5 (21.4) |  | 424 ±97.4 (20.6) |  |  |  |
| cartpole, lean reward, semi-implicit: a step's reward judges its press (steps per fall) | 21.1 |  |  |  | 798 ±331 (19.8) |  | 302 ±114 (21.3) |  | 276 ±45.5 (20.4) |  |  |  |
| **invariance** | | | | | | | | | | | | |
| position: a shape at a place never rewarded | 0.47 |  |  |  | 0.37 ±0.09 (0.51) |  | 0.34 ±0.22 (0.50) |  | 0.37 ±0.08 (0.51) |  |  |  |
| position: on the rewarded versions | 0.48 |  |  |  | 0.93 ±0.04 (0.50) |  | 0.99 ±0.00 (0.50) |  | 0.99 ±0.00 (0.50) |  |  |  |
| scale: a shape at a size never rewarded (1x) | 0.46 |  |  |  | 0.55 ±0.04 (0.49) |  | 0.47 ±0.13 (0.50) |  | 0.53 ±0.02 (0.50) |  |  |  |
| scale: on the rewarded versions | 0.48 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| rotation: a shape turned in a way never rewarded | 0.48 |  |  |  | 0.48 ±0.02 (0.50) |  | 0.46 ±0.11 (0.50) |  | 0.48 ±0.02 (0.50) |  |  |  |
| rotation: on the rewarded versions | 0.46 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.51) |  | 1.00 ±0.00 (0.50) |  |  |  |
| bright: a shape dimmer than ever rewarded (control) | 0.47 |  |  |  | 0.96 ±0.02 (0.50) |  | 0.50 ±0.02 (0.50) |  | 0.95 ±0.01 (0.50) |  |  |  |
| bright: on the rewarded versions | 0.47 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.49) |  | 1.00 ±0.00 (0.50) |  |  |  |
| clutter: a shape in more clutter than ever rewarded | 0.47 |  |  |  | 0.89 ±0.01 (0.49) |  | 0.50 ±0.02 (0.50) |  | 0.89 ±0.01 (0.51) |  |  |  |
| clutter: on the rewarded versions | 0.48 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.86 ±0.01 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| mixed: a size at a place never rewarded together | 0.48 |  |  |  | 0.52 ±0.02 (0.51) |  | 0.53 ±0.05 (0.51) |  | 0.54 ±0.02 (0.50) |  |  |  |
| mixed: on the rewarded versions | 0.47 |  |  |  | 0.99 ±0.00 (0.49) |  | 0.99 ±0.00 (0.51) |  | 0.99 ±0.00 (0.50) |  |  |  |
| view: a 3D object from angles never rewarded | 0.48 |  |  |  | 0.67 ±0.03 (0.51) |  | 0.38 ±0.12 (0.50) |  | 0.61 ±0.05 (0.51) |  |  |  |
| view: on the rewarded versions | 0.47 |  |  |  | 1.00 ±0.00 (0.50) |  | 0.99 ±0.00 (0.50) |  | 1.00 ±0.00 (0.50) |  |  |  |
| shift: a tune starting at a time never rewarded | 0.48 |  |  |  | 0.50 ±0.03 (0.49) |  | 0.99 ±0.01 (0.50) |  | 0.50 ±0.02 (0.50) |  |  |  |
| shift: on the rewarded versions | 0.47 |  |  |  | 0.75 ±0.01 (0.50) |  | 0.99 ±0.00 (0.49) |  | 0.50 ±0.02 (0.50) |  |  |  |
| tempo: a tune faster than ever rewarded | 0.47 |  |  |  | 0.48 ±0.03 (0.50) |  | 0.99 ±0.00 (0.49) |  | 0.50 ±0.03 (0.50) |  |  |  |
| tempo: on the rewarded versions | 0.47 |  |  |  | 0.67 ±0.03 (0.50) |  | 0.99 ±0.00 (0.49) |  | 0.49 ±0.01 (0.50) |  |  |  |
| pitch: a tune at a pitch never rewarded | 0.48 |  |  |  | 0.25 ±0.07 (0.49) |  | 0.29 ±0.16 (0.50) |  | 0.50 ±0.03 (0.50) |  |  |  |
| pitch: on the rewarded versions | 0.48 |  |  |  | 0.76 ±0.05 (0.51) |  | 0.94 ±0.04 (0.49) |  | 0.52 ±0.02 (0.52) |  |  |  |
| position + motion (half the life drifting), held out | 0.48 |  |  |  | 0.37 ±0.07 (0.50) |  | 0.43 ±0.13 (0.50) |  | 0.37 ±0.05 (0.52) |  |  |  |
| scale + motion (half the life drifting), held out | 0.49 |  |  |  | 0.55 ±0.04 (0.50) |  | 0.48 ±0.03 (0.49) |  | 0.53 ±0.04 (0.50) |  |  |  |
| rotation + motion (half the life drifting), held out | 0.48 |  |  |  | 0.46 ±0.03 (0.51) |  | 0.56 ±0.15 (0.51) |  | 0.45 ±0.02 (0.48) |  |  |  |
| bright + motion (half the life drifting), held out | 0.48 |  |  |  | 0.93 ±0.02 (0.49) |  | 0.58 ±0.16 (0.50) |  | 0.92 ±0.02 (0.47) |  |  |  |
| clutter + motion (half the life drifting), held out | 0.49 |  |  |  | 0.88 ±0.02 (0.50) |  | 0.49 ±0.02 (0.48) |  | 0.89 ±0.02 (0.50) |  |  |  |
| mixed + motion (half the life drifting), held out | 0.48 |  |  |  | 0.53 ±0.02 (0.51) |  | 0.49 ±0.09 (0.50) |  | 0.52 ±0.03 (0.51) |  |  |  |
| view + motion (half the life drifting), held out | 0.47 |  |  |  | 0.68 ±0.04 (0.51) |  | 0.42 ±0.16 (0.51) |  | 0.57 ±0.03 (0.50) |  |  |  |
