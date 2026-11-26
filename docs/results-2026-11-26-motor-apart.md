# Motor cells regulated apart; a chain row a coin cannot pass (idea 184, 2026-11-26)

**Tabled part way (the project was put away while this ran). As far as the finished runs
go: the smallest change that frees the sector's bias from the motor cells without costing
the output is a second bias per sector for the motor cells (`--g.motor_apart 3`), but it
brings the one setting's hidden cells back only part way (13 to 21 per 1,024 on MNIST)
because what starves them is the tile's shared winner cut, and splitting that cut brings
them back and costs the online score 0.03 to 0.11, so nothing here is shown to be worth a
default; the chain row a coin cannot pass is four buttons scored on the opening press
(`--task chain --k 4 --chain-first 1`: the critic 0.96, everything else 0.29 or less, line
0.625).**

Queue item 2n of `docs/plan.md`, following `docs/results-2026-11-26-cap-default.md`. Two
things that job left: the output's motor cells share a sector's rate bias with the hidden
cells and starve them in the MNIST world, and the quick map's chain row is passed by a coin
on the first press. No default was changed; every new number is off.

## Short version

- **Part 1 was screened on 3 seeds (5-7); parts 2 and 4 were not run.** The whole map, the
  inside test, the fresh readers, the long lives and the lean are in the script and ran
  only in its smoke.
- **Taking motor cells out of the sector's count is not enough, and hurts.** Out of the
  count and still under the hidden cells' bias (out1), or under no bias (out2), they lose
  the +16 they sat under and fire more (base rule on MNIST: 216 to 256 and 263 per 1,024),
  and MNIST online falls 0.026 and 0.037 under the base rule and 0.023 under the one
  setting (out1).
- **A bias per kind of cell (own) leaves the output where it was**: the motor cells' own
  bias sits at its limit as the shared one did, MNIST online -0.009 / +0.005 / +0.008 (base
  / a2rucomp / best), the four toy rows within 0.02 in every cell (far3 under a2rucomp
  +0.018). The sector's bias is no longer pinned at +16 under base (1.00 of sectors to
  0.31) and the one setting (0.96 to 0.02).
- **But the one setting's hidden cells are not starved by the bias.** Freed, the bias goes
  to its lower limit on 9 in 10 sectors and the hidden cells still fire at 16 to 21 per
  1,024 with 0.12 to 0.17 silent. The motor cells fire at about 104 whatever their
  thresholds are. They are taking the tile's two winners.
- **Splitting the tile's winners by kind (`motor_winners`) brings the hidden cells back
  and costs the output.** One setting, MNIST: a cut of their own for motor cells (cut2)
  hidden 25.5, silent 0.084, online 0.661 (-0.054); motor cells out of the cut (cut) hidden
  33.8, online 0.603 (-0.112); with the bias by kind as well (own+cut) 0.636 (-0.078).
  Under a2rucomp every one of the five cut arms loses 0.025 to 0.033 and the hidden rate
  goes from 63 to 98: there the shared cut was holding competitive learning's own runaway.
  So the one setting scores better with its hidden cells quiet. Why was not looked at.
- **An honest higher target for motor cells (128) is out**: exclusive-or loses 0.06 to 0.13
  under a2rucomp and the one setting in all three such arms. It lifts reach (far3 under
  a2rucomp 0.930 to 0.99) and, with the bias still shared, MNIST under the one setting
  (+0.026).
- **The chain row.** Scored on the opening press alone, seeds 5-12, cap 64: with four
  buttons the critic opens with A on 0.958 (6 seeds), the base rule 0.056, a2rucomp 0.034,
  the one setting 0.018; with the cap lifted (the old row's coin) 0.218, 0.107, 0.292. The
  halfway line is 0.625, a coin is 0.25. With three buttons the one setting at cap 1000
  reaches 0.502 (one seed 0.84) against a line of 0.667, so four buttons it is. The row
  `chainfirst` is in `scripts/variants/oneset.txt` beside the old one, which is marked.
- **Should anything become the default: no.** own is safe on what was run and does not do
  the job it was for; the cut by kind does the job and loses score.

## The arms and what each costs in the machine

| arm | flags | what | cost |
|---|---|---|---|
| out1 | `--g.motor_apart 1` | motor cells out of the sector's count and target; they still feel its bias | one more AND per tile in the count; no state |
| out2 | `--g.motor_apart 2` | and out of the bias | out1, and for a tile with motor cells and a bias that is not 0 the 64-cell compare a second time, merged by the motor mask |
| own | `--g.motor_apart 3` | a second bias per sector for motor cells | out2, and per sector one more smoothed count, one more 16-bit bias, one more target; one more popcount per tile |
| walk128, walkonly, own128 | `--g.threshold_down_shift_motor 7` (with `--g.threshold_cap_motor 1000`, `--g.motor_target_per_1024 128`) | the walk's own cap and target for motor cells | the cap picked by the motor bit on a step up; one more random word per tick per tile with motor cells |
| cut, cut2 | `--g.motor_winners -1`, `2` | the tile's winners picked among the cells that are not motor cells; motor cells keep every spike, or have a cut of their own | the tile's cut run on two masks; no state |

All state is flat arrays per sector; nothing per neuron was added. With every number off
the fire step does one integer test per tick and one per tile. `learn_hop.cpp` (not mine to
touch) still reads the sector's bias for every cell, so with `hop_who 4` (motor cells taught
by the one-hop rule) under out2 or own its "near" test would be off by the difference of the
two biases. `blobbench`, one round only (the smoke): before 68,788 ticks a second with
learning off and 46,924 on; after, all off, 69,334 and 47,238; out1 66,660 / 45,334; out2
65,151 / 45,242; own 67,803 / 46,180. The five rounds taking turns were not run.

## Part 1: the screen

`scripts/motorapart.sh screen`: quick-map lives through `blobprobe capsplit`. The second
screen (the cut arms) finished its MNIST lives for a2rucomp (all five arms) and for the one
setting (cut, cut2, own+cut); own+cut2 and out1+cut2 under the one setting and every toy
row of the cut arms did not run.

Seeds 5,6,7. late: the life's online score (the quick map's), in brackets the lowest and highest seed and the mean difference to std, seed by seed. Rates per 1,024 cell-ticks over the last 2,000 steps. hidden silent: share of plain hidden cells with no spike in them; motor stuck: share of motor cells firing on over a quarter of the ticks; bias: share of sectors with the bias at its upper / lower limit; motor bias: the motor cells' own, mean (own arms; else 0).


mnist:

| column | arm | late (lowest-highest, less std) | hidden rate | motor rate | hidden silent | motor stuck | motor at 64+ | bias at +max / -max | motor bias | seeds |
|---|---|---|---|---|---|---|---|---|---|---|
| base | std | 0.210 (0.20-0.22) | 34.2 | 216.1 | 0.000 | 0.278 | 0.80 | 1.00 / 0.00 | 0.0 | 3 |
| base | out1 | 0.184 (0.16-0.20, -0.026) | 40.7 | 255.7 | 0.000 | 0.378 | 0.84 | 0.31 / 0.08 | 0.0 | 3 |
| base | out2 | 0.172 (0.14-0.20, -0.037) | 40.8 | 263.2 | 0.000 | 0.391 | 0.82 | 0.31 / 0.04 | 0.0 | 3 |
| base | own | 0.201 (0.20-0.20, -0.009) | 40.7 | 208.2 | 0.000 | 0.263 | 0.70 | 0.31 / 0.04 | 16.0 | 3 |
| base | own128 | 0.203 (0.20-0.21, -0.006) | 40.5 | 238.8 | 0.000 | 0.303 | 0.40 | 0.33 / 0.04 | 16.0 | 3 |
| base | walk128 | 0.196 (0.18-0.21, -0.014) | 40.3 | 124.1 | 0.000 | 0.000 | 0.88 | 0.29 / 0.04 | 0.0 | 3 |
| base | walkonly | 0.200 (0.19-0.21, -0.010) | 36.5 | 123.6 | 0.000 | 0.000 | 0.80 | 0.77 / 0.00 | 0.0 | 3 |
| a2rucomp | std | 0.841 (0.84-0.84) | 62.9 | 93.1 | 0.006 | 0.013 | 0.44 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | out1 | 0.841 (0.84-0.84, +0.000) | 64.6 | 83.3 | 0.004 | 0.009 | 0.40 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | out2 | 0.837 (0.83-0.85, -0.004) | 64.0 | 91.1 | 0.006 | 0.010 | 0.43 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | own | 0.846 (0.83-0.86, +0.005) | 63.5 | 92.5 | 0.005 | 0.015 | 0.43 | 1.00 / 0.00 | 16.0 | 3 |
| a2rucomp | own128 | 0.819 (0.81-0.82, -0.022) | 62.6 | 94.0 | 0.003 | 0.001 | 0.02 | 0.98 / 0.00 | -15.2 | 3 |
| a2rucomp | walk128 | 0.837 (0.83-0.84, -0.004) | 59.5 | 98.0 | 0.001 | 0.000 | 0.16 | 0.98 / 0.00 | 0.0 | 3 |
| a2rucomp | walkonly | 0.839 (0.83-0.84, -0.002) | 61.3 | 91.1 | 0.001 | 0.000 | 0.14 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | cut | 0.816 (0.80-0.82, -0.025) | 98.1 | 87.0 | 0.000 | 0.037 | 0.37 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | cut2 | 0.812 (0.81-0.82, -0.029) | 97.5 | 107.0 | 0.000 | 0.026 | 0.49 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | own+cut | 0.811 (0.79-0.83, -0.030) | 97.4 | 94.0 | 0.000 | 0.046 | 0.40 | 1.00 / 0.00 | 15.9 | 3 |
| a2rucomp | own+cut2 | 0.807 (0.79-0.82, -0.033) | 97.5 | 108.5 | 0.000 | 0.024 | 0.49 | 1.00 / 0.00 | 16.0 | 3 |
| a2rucomp | out1+cut2 | 0.811 (0.81-0.82, -0.029) | 97.7 | 105.9 | 0.000 | 0.023 | 0.48 | 1.00 / 0.00 | 0.0 | 3 |
| best | std | 0.715 (0.70-0.73) | 13.3 | 104.8 | 0.188 | 0.000 | 0.54 | 0.96 / 0.00 | 0.0 | 3 |
| best | out1 | 0.692 (0.68-0.70, -0.023) | 16.0 | 103.9 | 0.172 | 0.000 | 0.54 | 0.00 / 0.92 | 0.0 | 3 |
| best | out2 | 0.713 (0.70-0.74, -0.002) | 18.3 | 97.7 | 0.136 | 0.000 | 0.52 | 0.02 / 0.92 | 0.0 | 3 |
| best | own | 0.723 (0.71-0.73, +0.008) | 21.1 | 90.0 | 0.115 | 0.000 | 0.47 | 0.02 / 0.90 | 16.0 | 3 |
| best | own128 | 0.687 (0.66-0.71, -0.027) | 11.9 | 114.9 | 0.360 | 0.000 | 0.07 | 0.00 / 0.96 | -16.0 | 3 |
| best | walk128 | 0.705 (0.70-0.71, -0.010) | 16.1 | 104.1 | 0.114 | 0.000 | 0.35 | 0.02 / 0.90 | 0.0 | 3 |
| best | walkonly | 0.740 (0.73-0.75, +0.026) | 13.4 | 107.3 | 0.139 | 0.000 | 0.36 | 0.98 / 0.00 | 0.0 | 3 |
| best | cut | 0.603 (0.58-0.63, -0.112) | 33.8 | 152.0 | 0.084 | 0.056 | 0.70 | 0.92 / 0.00 | 0.0 | 3 |
| best | cut2 | 0.661 (0.65-0.67, -0.054) | 25.5 | 103.5 | 0.084 | 0.001 | 0.53 | 0.90 / 0.00 | 0.0 | 3 |
| best | own+cut | 0.636 (0.62-0.66, -0.078) | 35.4 | 135.3 | 0.084 | 0.052 | 0.62 | 0.29 / 0.48 | 16.0 | 3 |

echo2:

| column | arm | late (lowest-highest, less std) | hidden rate | motor rate | hidden silent | motor stuck | motor at 64+ | bias at +max / -max | motor bias | seeds |
|---|---|---|---|---|---|---|---|---|---|---|
| base | std | 1.000 (1.00-1.00) | 31.2 | 47.1 | 0.000 | 0.000 | 0.15 | 0.02 / 0.10 | 0.0 | 3 |
| base | out1 | 1.000 (1.00-1.00, +0.000) | 31.9 | 54.7 | 0.000 | 0.000 | 0.19 | 0.00 / 0.10 | 0.0 | 3 |
| base | out2 | 1.000 (1.00-1.00, +0.000) | 31.9 | 51.3 | 0.000 | 0.000 | 0.16 | 0.00 / 0.08 | 0.0 | 3 |
| base | own | 1.000 (1.00-1.00, +0.000) | 31.9 | 48.8 | 0.000 | 0.000 | 0.15 | 0.00 / 0.12 | 1.4 | 3 |
| base | own128 | 1.000 (1.00-1.00, +0.000) | 31.9 | 120.6 | 0.000 | 0.004 | 0.05 | 0.00 / 0.10 | -2.8 | 3 |
| base | walk128 | 1.000 (1.00-1.00, +0.000) | 31.9 | 113.2 | 0.000 | 0.000 | 0.11 | 0.00 / 0.17 | 0.0 | 3 |
| base | walkonly | 1.000 (1.00-1.00, +0.000) | 29.6 | 91.9 | 0.000 | 0.000 | 0.08 | 0.23 / 0.08 | 0.0 | 3 |
| a2rucomp | std | 1.000 (1.00-1.00) | 46.7 | 57.1 | 0.019 | 0.000 | 0.27 | 0.65 / 0.02 | 0.0 | 3 |
| a2rucomp | out1 | 1.000 (1.00-1.00, +0.000) | 47.7 | 57.9 | 0.020 | 0.000 | 0.27 | 0.75 / 0.00 | 0.0 | 3 |
| a2rucomp | out2 | 1.000 (1.00-1.00, +0.000) | 47.0 | 64.3 | 0.020 | 0.000 | 0.31 | 0.75 / 0.00 | 0.0 | 3 |
| a2rucomp | own | 1.000 (1.00-1.00, +0.000) | 48.0 | 56.0 | 0.019 | 0.000 | 0.26 | 0.71 / 0.00 | 3.8 | 3 |
| a2rucomp | own128 | 0.972 (0.94-1.00, -0.028) | 46.6 | 129.4 | 0.024 | 0.000 | 0.06 | 0.88 / 0.00 | -1.1 | 3 |
| a2rucomp | walk128 | 1.000 (1.00-1.00, +0.000) | 45.1 | 102.1 | 0.019 | 0.000 | 0.08 | 0.75 / 0.02 | 0.0 | 3 |
| a2rucomp | walkonly | 1.000 (1.00-1.00, +0.000) | 46.0 | 104.3 | 0.020 | 0.000 | 0.09 | 0.81 / 0.00 | 0.0 | 3 |
| best | std | 1.000 (1.00-1.00) | 27.0 | 44.7 | 0.138 | 0.000 | 0.16 | 0.08 / 0.77 | 0.0 | 3 |
| best | out1 | 1.000 (1.00-1.00, +0.000) | 27.2 | 43.9 | 0.138 | 0.000 | 0.16 | 0.04 / 0.79 | 0.0 | 3 |
| best | out2 | 1.000 (1.00-1.00, +0.000) | 27.4 | 40.0 | 0.138 | 0.000 | 0.13 | 0.04 / 0.81 | 0.0 | 3 |
| best | own | 1.000 (1.00-1.00, +0.000) | 27.4 | 38.7 | 0.136 | 0.000 | 0.14 | 0.06 / 0.79 | 0.8 | 3 |
| best | own128 | 1.000 (1.00-1.00, +0.000) | 26.2 | 66.0 | 0.145 | 0.000 | 0.00 | 0.04 / 0.83 | -4.0 | 3 |
| best | walk128 | 1.000 (1.00-1.00, +0.000) | 26.1 | 67.1 | 0.147 | 0.000 | 0.01 | 0.04 / 0.81 | 0.0 | 3 |
| best | walkonly | 1.000 (1.00-1.00, +0.000) | 26.1 | 64.7 | 0.145 | 0.000 | 0.01 | 0.10 / 0.75 | 0.0 | 3 |

bandit:

| column | arm | late (lowest-highest, less std) | hidden rate | motor rate | hidden silent | motor stuck | motor at 64+ | bias at +max / -max | motor bias | seeds |
|---|---|---|---|---|---|---|---|---|---|---|
| base | std | 0.771 (0.77-0.77) | 31.4 | 42.7 | 0.000 | 0.000 | 0.10 | 0.00 / 0.06 | 0.0 | 3 |
| base | out1 | 0.777 (0.77-0.79, +0.006) | 31.9 | 45.4 | 0.000 | 0.001 | 0.13 | 0.00 / 0.08 | 0.0 | 3 |
| base | out2 | 0.775 (0.77-0.78, +0.004) | 31.9 | 46.1 | 0.000 | 0.001 | 0.11 | 0.00 / 0.06 | 0.0 | 3 |
| base | own | 0.775 (0.76-0.78, +0.004) | 31.9 | 40.4 | 0.000 | 0.000 | 0.09 | 0.00 / 0.06 | 2.3 | 3 |
| base | own128 | 0.785 (0.78-0.79, +0.014) | 31.9 | 120.4 | 0.000 | 0.003 | 0.03 | 0.00 / 0.17 | -2.7 | 3 |
| base | walk128 | 0.787 (0.78-0.79, +0.016) | 31.8 | 113.0 | 0.000 | 0.000 | 0.07 | 0.00 / 0.21 | 0.0 | 3 |
| base | walkonly | 0.780 (0.77-0.79, +0.008) | 29.5 | 90.4 | 0.000 | 0.000 | 0.04 | 0.12 / 0.12 | 0.0 | 3 |
| a2rucomp | std | 1.000 (1.00-1.00) | 58.9 | 79.6 | 0.131 | 0.000 | 0.38 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | out1 | 1.000 (1.00-1.00, +0.000) | 58.8 | 80.0 | 0.129 | 0.001 | 0.38 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | out2 | 1.000 (1.00-1.00, +0.000) | 58.3 | 93.8 | 0.142 | 0.005 | 0.44 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | own | 1.000 (1.00-1.00, +0.000) | 58.9 | 81.7 | 0.133 | 0.001 | 0.39 | 1.00 / 0.00 | 4.0 | 3 |
| a2rucomp | own128 | 1.000 (1.00-1.00, +0.000) | 56.4 | 126.3 | 0.144 | 0.000 | 0.07 | 1.00 / 0.00 | -0.6 | 3 |
| a2rucomp | walk128 | 1.000 (1.00-1.00, +0.000) | 57.3 | 106.1 | 0.127 | 0.000 | 0.21 | 1.00 / 0.00 | 0.0 | 3 |
| a2rucomp | walkonly | 1.000 (1.00-1.00, +0.000) | 57.2 | 106.4 | 0.127 | 0.000 | 0.20 | 1.00 / 0.00 | 0.0 | 3 |
| best | std | 0.997 (1.00-1.00) | 26.2 | 36.1 | 0.139 | 0.000 | 0.07 | 0.02 / 0.85 | 0.0 | 3 |
| best | out1 | 0.994 (0.99-1.00, -0.003) | 26.4 | 34.8 | 0.139 | 0.000 | 0.06 | 0.06 / 0.85 | 0.0 | 3 |
| best | out2 | 0.991 (0.99-1.00, -0.006) | 26.5 | 32.9 | 0.139 | 0.000 | 0.06 | 0.02 / 0.88 | 0.0 | 3 |
| best | own | 0.994 (0.99-1.00, -0.003) | 26.4 | 33.3 | 0.138 | 0.000 | 0.06 | 0.02 / 0.88 | -0.9 | 3 |
| best | own128 | 1.000 (1.00-1.00, +0.003) | 25.4 | 60.7 | 0.150 | 0.000 | 0.00 | 0.04 / 0.90 | -4.0 | 3 |
| best | walk128 | 1.000 (1.00-1.00, +0.003) | 25.3 | 60.9 | 0.153 | 0.000 | 0.00 | 0.04 / 0.90 | 0.0 | 3 |
| best | walkonly | 1.000 (1.00-1.00, +0.003) | 25.2 | 58.5 | 0.148 | 0.000 | 0.00 | 0.04 / 0.79 | 0.0 | 3 |

xor:

| column | arm | late (lowest-highest, less std) | hidden rate | motor rate | hidden silent | motor stuck | motor at 64+ | bias at +max / -max | motor bias | seeds |
|---|---|---|---|---|---|---|---|---|---|---|
| base | std | 0.893 (0.85-0.94) | 31.8 | 40.1 | 0.000 | 0.000 | 0.11 | 0.04 / 0.10 | 0.0 | 3 |
| base | out1 | 0.906 (0.89-0.93, +0.013) | 32.1 | 45.3 | 0.000 | 0.000 | 0.16 | 0.02 / 0.02 | 0.0 | 3 |
| base | out2 | 0.911 (0.90-0.92, +0.018) | 32.1 | 43.0 | 0.000 | 0.000 | 0.13 | 0.02 / 0.02 | 0.0 | 3 |
| base | own | 0.881 (0.85-0.91, -0.012) | 32.1 | 39.1 | 0.000 | 0.000 | 0.11 | 0.00 / 0.12 | 2.1 | 3 |
| base | own128 | 0.909 (0.88-0.93, +0.016) | 32.0 | 118.6 | 0.000 | 0.001 | 0.04 | 0.00 / 0.04 | -3.0 | 3 |
| base | walk128 | 0.882 (0.81-0.92, -0.011) | 32.0 | 111.7 | 0.000 | 0.000 | 0.06 | 0.00 / 0.06 | 0.0 | 3 |
| base | walkonly | 0.908 (0.87-0.93, +0.015) | 30.1 | 89.1 | 0.000 | 0.000 | 0.02 | 0.23 / 0.10 | 0.0 | 3 |
| a2rucomp | std | 0.993 (0.98-1.00) | 35.9 | 51.1 | 0.000 | 0.000 | 0.25 | 0.44 / 0.00 | 0.0 | 3 |
| a2rucomp | out1 | 0.989 (0.97-1.00, -0.004) | 36.5 | 51.4 | 0.000 | 0.000 | 0.24 | 0.38 / 0.00 | 0.0 | 3 |
| a2rucomp | out2 | 0.980 (0.94-1.00, -0.013) | 36.5 | 58.6 | 0.000 | 0.000 | 0.30 | 0.40 / 0.00 | 0.0 | 3 |
| a2rucomp | own | 0.995 (0.99-1.00, +0.002) | 36.3 | 51.0 | 0.000 | 0.000 | 0.24 | 0.38 / 0.00 | 3.9 | 3 |
| a2rucomp | own128 | 0.865 (0.85-0.88, -0.129) | 37.0 | 125.8 | 0.000 | 0.000 | 0.04 | 0.56 / 0.00 | -3.4 | 3 |
| a2rucomp | walk128 | 0.895 (0.88-0.91, -0.099) | 36.9 | 108.5 | 0.000 | 0.000 | 0.05 | 0.54 / 0.00 | 0.0 | 3 |
| a2rucomp | walkonly | 0.871 (0.86-0.88, -0.122) | 36.8 | 108.3 | 0.000 | 0.000 | 0.05 | 0.56 / 0.00 | 0.0 | 3 |
| best | std | 0.924 (0.86-0.96) | 28.9 | 40.9 | 0.127 | 0.000 | 0.15 | 0.15 / 0.63 | 0.0 | 3 |
| best | out1 | 0.928 (0.88-0.96, +0.005) | 29.1 | 41.0 | 0.127 | 0.000 | 0.14 | 0.10 / 0.71 | 0.0 | 3 |
| best | out2 | 0.913 (0.82-0.96, -0.011) | 29.2 | 38.4 | 0.127 | 0.000 | 0.12 | 0.17 / 0.60 | 0.0 | 3 |
| best | own | 0.934 (0.89-0.96, +0.010) | 29.3 | 36.7 | 0.127 | 0.000 | 0.10 | 0.19 / 0.63 | 1.5 | 3 |
| best | own128 | 0.836 (0.76-0.88, -0.088) | 28.1 | 66.9 | 0.127 | 0.000 | 0.00 | 0.08 / 0.65 | -4.0 | 3 |
| best | walk128 | 0.815 (0.71-0.87, -0.108) | 28.2 | 65.0 | 0.128 | 0.000 | 0.00 | 0.12 / 0.63 | 0.0 | 3 |
| best | walkonly | 0.867 (0.81-0.91, -0.057) | 28.0 | 63.8 | 0.127 | 0.000 | 0.00 | 0.17 / 0.56 | 0.0 | 3 |

far3:

| column | arm | late (lowest-highest, less std) | hidden rate | motor rate | hidden silent | motor stuck | motor at 64+ | bias at +max / -max | motor bias | seeds |
|---|---|---|---|---|---|---|---|---|---|---|
| base | std | 0.886 (0.80-0.95) | 31.8 | 34.6 | 0.000 | 0.000 | 0.04 | 0.00 / 0.02 | 0.0 | 3 |
| base | out1 | 0.889 (0.81-0.96, +0.003) | 32.0 | 35.5 | 0.000 | 0.000 | 0.04 | 0.00 / 0.04 | 0.0 | 3 |
| base | out2 | 0.889 (0.81-0.95, +0.003) | 31.9 | 34.4 | 0.000 | 0.000 | 0.04 | 0.00 / 0.10 | 0.0 | 3 |
| base | own | 0.886 (0.81-0.95, -0.000) | 31.9 | 33.2 | 0.000 | 0.000 | 0.03 | 0.00 / 0.00 | 1.1 | 3 |
| base | own128 | 0.918 (0.86-0.97, +0.033) | 31.8 | 118.1 | 0.000 | 0.003 | 0.01 | 0.02 / 0.06 | -3.7 | 3 |
| base | walk128 | 0.903 (0.83-0.95, +0.017) | 31.8 | 115.1 | 0.000 | 0.000 | 0.04 | 0.00 / 0.17 | 0.0 | 3 |
| base | walkonly | 0.930 (0.86-0.97, +0.044) | 29.7 | 76.8 | 0.000 | 0.000 | 0.01 | 0.12 / 0.08 | 0.0 | 3 |
| a2rucomp | std | 0.930 (0.92-0.94) | 47.4 | 47.3 | 0.009 | 0.000 | 0.21 | 0.75 / 0.02 | 0.0 | 3 |
| a2rucomp | out1 | 0.951 (0.94-0.96, +0.022) | 47.7 | 49.0 | 0.011 | 0.001 | 0.23 | 0.81 / 0.02 | 0.0 | 3 |
| a2rucomp | out2 | 0.961 (0.95-0.98, +0.031) | 47.5 | 58.7 | 0.011 | 0.001 | 0.31 | 0.81 / 0.02 | 0.0 | 3 |
| a2rucomp | own | 0.948 (0.93-0.97, +0.018) | 47.9 | 48.2 | 0.011 | 0.000 | 0.21 | 0.81 / 0.00 | 3.9 | 3 |
| a2rucomp | own128 | 0.992 (0.99-1.00, +0.063) | 46.9 | 123.4 | 0.020 | 0.000 | 0.02 | 0.79 / 0.00 | -4.0 | 3 |
| a2rucomp | walk128 | 0.995 (0.99-1.00, +0.065) | 47.2 | 98.7 | 0.010 | 0.000 | 0.03 | 0.77 / 0.00 | 0.0 | 3 |
| a2rucomp | walkonly | 0.994 (0.99-1.00, +0.064) | 47.1 | 98.6 | 0.011 | 0.000 | 0.05 | 0.79 / 0.04 | 0.0 | 3 |
| best | std | 0.991 (0.98-1.00) | 26.9 | 33.7 | 0.133 | 0.000 | 0.06 | 0.00 / 0.88 | 0.0 | 3 |
| best | out1 | 0.991 (0.98-1.00, -0.000) | 26.8 | 34.9 | 0.134 | 0.000 | 0.08 | 0.00 / 0.88 | 0.0 | 3 |
| best | out2 | 0.993 (0.98-1.00, +0.001) | 27.2 | 28.9 | 0.134 | 0.000 | 0.05 | 0.00 / 0.85 | 0.0 | 3 |
| best | own | 0.988 (0.97-1.00, -0.003) | 27.0 | 31.5 | 0.133 | 0.000 | 0.04 | 0.00 / 0.92 | -2.0 | 3 |
| best | own128 | 0.991 (0.98-1.00, -0.000) | 25.8 | 54.8 | 0.134 | 0.000 | 0.00 | 0.02 / 0.90 | -4.0 | 3 |
| best | walk128 | 0.991 (0.98-1.00, -0.001) | 25.9 | 53.5 | 0.133 | 0.000 | 0.00 | 0.00 / 0.92 | 0.0 | 3 |
| best | walkonly | 0.990 (0.98-0.99, -0.002) | 25.8 | 54.2 | 0.134 | 0.000 | 0.00 | 0.00 / 0.85 | 0.0 | 3 |

Arms: std: the default: motor cells in the sector's count, under its bias (`no flags`); out1: motor cells out of the sector's count and target; they still feel its bias (`--g.motor_apart 1`); out2: motor cells out of the sector's count and of its bias (`--g.motor_apart 2`); own: a bias per kind of cell: the motor cells' own, toward 32 per 1,024 (`--g.motor_apart 3`); own128: a bias and a walk of their own for motor cells, both toward 128 per 1,024 (`--g.motor_apart 3 --g.motor_target_per_1024 128 --g.threshold_down_shift_motor 7`); walk128: motor cells out of the count, with their own walk: target 128, cap 1000 (`--g.motor_apart 1 --g.threshold_down_shift_motor 7 --g.threshold_cap_motor 1000`); walkonly: their own walk alone (target 128, cap 1000), still in the sector's count (`--g.threshold_down_shift_motor 7 --g.threshold_cap_motor 1000`); m64: the cap by kind: 1000 for the rest, 64 for motor cells (the cap doc's probe arm) (`--g.threshold_cap 1000 --g.threshold_cap_motor 64`); out1h: out1 with the cap by kind: the rest at 1000, motor cells at 64 (`--g.motor_apart 1 --g.threshold_cap 1000 --g.threshold_cap_motor 64`); c1000: cap 1000 for every cell (`--g.threshold_cap 1000`); cut: the tile's winners by kind: motor cells out of the tile's cut (they keep every spike) (`--g.motor_winners -1`); cut2: the tile's winners by kind: the motor cells have a cut of their own, 2 a tile (`--g.motor_winners 2`); own+cut: own, and motor cells out of the tile's cut (`--g.motor_apart 3 --g.motor_winners -1`); own+cut2: own, and the motor cells' own cut of 2 (`--g.motor_apart 3 --g.motor_winners 2`); out1+cut2: out1, and the motor cells' own cut of 2 (`--g.motor_apart 1 --g.motor_winners 2`)

## Part 3: the chain row

`scripts/motorapart.sh chain` (run on stevejobs). `--chain-first 1` is new in the chain
world: the score is the share of chains opened by pressing A at once. "Anything but the
paid button" and "each button in turn" get at most 1 / (k - 1) on it; only knowing which
press was paid for a step later gets more. With four buttons the "another button" column
adds two buttons. Missing: the critic with four buttons on seeds 11 and 12 and the critic at
cap 1000 with four buttons.

The chain, each form, seeds 5,6,7,8,9,10,11,12, the last 2,000 steps. A at once: of the stage-0 steps that open a chain (the step after a stage-1 step), the share on which A was pressed; this is the score of `--chain-first 1`. A less another: A's share less the share of the buttons that are neither A nor B, with its standard error. chains paid x 2 / steps: the old row's score on the same life.

k2 (`--task chain  --chain-first 1`): a coin on the opening press 0.500; "anything but B" or "each button in turn" at most 1.000; the halfway line from the coin to perfect 0.750.

| setting | A at once (lowest-highest) | B / another button / nothing at once | A less another | chains paid x 2 / steps | chains opened | A at once per seed |
|---|---|---|---|---|---|---|
| base | 0.033 (0.00-0.10) | 0.96 / 0.00 / 0.01 | +0.03 ±0.01 | 0.31 | 306 | 0.00 0.01 0.03 0.03 0.03 0.04 0.10 0.02 |
| base@1000 | 0.368 (0.21-0.42) | 0.61 / 0.00 / 0.02 | +0.37 ±0.02 | 0.72 | 721 | 0.33 0.21 0.42 0.37 0.42 0.42 0.37 0.38 |
| a2rucomp | 0.000 (0.00-0.00) | 0.00 / 0.00 / 0.00 | +0.00 ±0.00 | 0.00 | 0 | 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.00 |
| a2rucomp@1000 | 0.067 (0.01-0.12) | 0.92 / 0.00 / 0.01 | +0.07 ±0.01 | 0.52 | 525 | 0.12 0.09 0.10 0.01 0.06 0.08 0.07 0.01 |
| best | 0.008 (0.00-0.06) | 0.36 / 0.00 / 0.00 | +0.01 ±0.01 | 0.15 | 151 | 0.06 0.01 0.00 0.00 0.00 0.00 0.00 0.00 |
| best@1000 | 0.766 (0.14-1.00) | 0.22 / 0.00 / 0.01 | +0.77 ±0.12 | 0.81 | 811 | 0.93 0.34 0.98 1.00 0.99 0.14 0.93 0.84 |
| critic | 0.950 (0.90-0.97) | 0.05 / 0.00 / 0.00 | +0.95 ±0.01 | 0.96 | 976 | 0.97 0.90 0.95 0.97 0.96 0.96 0.93 0.95 |
| critic@1000 | 0.789 (0.68-0.88) | 0.20 / 0.00 / 0.01 | +0.79 ±0.03 | 0.86 | 903 | 0.88 0.68 0.83 0.87 0.69 0.77 0.75 0.83 |

k3 (`--task chain --k 3 --chain-first 1`): a coin on the opening press 0.333; "anything but B" or "each button in turn" at most 0.500; the halfway line from the coin to perfect 0.667.

| setting | A at once (lowest-highest) | B / another button / nothing at once | A less another | chains paid x 2 / steps | chains opened | A at once per seed |
|---|---|---|---|---|---|---|
| base | 0.050 (0.00-0.15) | 0.86 / 0.07 / 0.02 | -0.02 ±0.05 | 0.20 | 202 | 0.04 0.00 0.00 0.06 0.14 0.01 0.01 0.15 |
| base@1000 | 0.316 (0.11-0.54) | 0.38 / 0.25 / 0.05 | +0.06 ±0.08 | 0.53 | 532 | 0.38 0.11 0.41 0.27 0.54 0.31 0.29 0.22 |
| a2rucomp | 0.035 (0.00-0.28) | 0.19 / 0.01 / 0.02 | +0.02 ±0.02 | 0.01 | 10 | 0.00 0.00 0.00 0.00 0.00 0.00 0.00 0.28 |
| a2rucomp@1000 | 0.101 (0.07-0.15) | 0.68 / 0.18 / 0.03 | -0.08 ±0.05 | 0.40 | 402 | 0.07 0.15 0.07 0.12 0.09 0.09 0.11 0.10 |
| best | 0.119 (0.00-0.64) | 0.56 / 0.05 / 0.02 | +0.07 ±0.07 | 0.20 | 197 | 0.06 0.64 0.00 0.00 0.19 0.00 0.00 0.06 |
| best@1000 | 0.502 (0.20-0.84) | 0.18 / 0.27 / 0.05 | +0.23 ±0.12 | 0.65 | 645 | 0.37 0.20 0.60 0.84 0.71 0.52 0.49 0.28 |
| critic | 0.945 (0.91-0.99) | 0.05 / 0.00 / 0.01 | +0.94 ±0.01 | 0.96 | 973 | 0.98 0.92 0.99 0.97 0.93 0.91 0.94 0.92 |
| critic@1000 | 0.703 (0.63-0.83) | 0.26 / 0.03 / 0.01 | +0.68 ±0.03 | 0.81 | 868 | 0.70 0.64 0.81 0.72 0.63 0.83 0.65 0.65 |

k4 (`--task chain --k 4 --chain-first 1`): a coin on the opening press 0.250; "anything but B" or "each button in turn" at most 0.333; the halfway line from the coin to perfect 0.625.

| setting | A at once (lowest-highest) | B / another button / nothing at once | A less another | chains paid x 2 / steps | chains opened | A at once per seed |
|---|---|---|---|---|---|---|
| base | 0.056 (0.00-0.15) | 0.78 / 0.13 / 0.03 | -0.07 ±0.03 | 0.19 | 192 | 0.03 0.06 0.00 0.03 0.15 0.03 0.13 0.01 |
| base@1000 | 0.218 (0.10-0.35) | 0.36 / 0.36 / 0.06 | -0.14 ±0.04 | 0.37 | 375 | 0.25 0.13 0.21 0.10 0.35 0.21 0.16 0.33 |
| a2rucomp | 0.034 (0.00-0.18) | 0.37 / 0.08 / 0.01 | -0.05 ±0.08 | 0.04 | 39 | 0.00 0.00 0.00 0.00 0.00 0.18 0.00 0.09 |
| a2rucomp@1000 | 0.107 (0.05-0.22) | 0.63 / 0.23 / 0.04 | -0.12 ±0.06 | 0.35 | 351 | 0.17 0.05 0.05 0.17 0.08 0.06 0.22 0.07 |
| best | 0.018 (0.00-0.05) | 0.50 / 0.08 / 0.03 | -0.06 ±0.05 | 0.07 | 75 | 0.05 0.03 0.05 0.00 0.00 0.00 0.00 0.02 |
| best@1000 | 0.292 (0.07-0.58) | 0.11 / 0.50 / 0.10 | -0.21 ±0.09 | 0.50 | 498 | 0.15 0.27 0.40 0.28 0.38 0.07 0.19 0.58 |
| critic | 0.958 (0.94-0.99) | 0.04 / 0.00 / 0.00 | +0.96 ±0.01 | 0.97 | 980 | 0.95 0.94 0.96 0.94 0.99 0.96 - - |

- The old two-button row read this way: the base rule at cap 1000 opens with A on 0.37 and
  scores 0.72 by chains paid, the pass the cap doc took apart.
- The one setting at cap 1000 does open with A well over a coin with two buttons (0.766, 6
  of 8 seeds at 0.84 or more) and with three (0.502 against 0.27 for the third button, A
  less the third +0.23 ±0.12), and not with four (0.292 against 0.25 each for the two
  others). Where that comes from is part 4, which did not run.

## What is real, what is suspect

Real: the mechanism (shared bias pinned by motor cells; shared winner cut taken by motor
cells) is read straight off the rates and the bias in 3 seeds that agree closely. The chain
table is 8 seeds.

Suspect: every part 1 score is 3 seeds; differences under 0.02 there are not results (own's
-0.009 under the base rule among them). The cut arms were never run on the toy rows. The
`chainfirst` row was run through the quick map only in the smoke (seed 5, not read); its
pass line here is worked out from 1 / k, not from the map's calibration. A player that
repeats its press of two steps before would open with A every time; nothing in the sheet
was seen doing that, and learning it needs the same credit.

## Not finished at tabling

- Part 1: the fresh readers (`reader`); own+cut2 and out1+cut2 under the one setting; the
  toy rows for the cut arms; 8 seeds for anything.
- Part 2: the whole map with an arm; the inside test; the long lives (`alive`); what would
  be remade (`bash out/motorapart/refarm.sh --g.motor_apart 3` is written, not run).
- Part 3: the new row on the map for the standard columns with twins; the critic's two
  missing seeds.
- Part 4: the lean (`lean`: the one setting less each piece, seeds 1-12). Not started.
- Speed over five rounds. Why the one setting's output does better with quiet hidden cells.

## Script lines

```
scripts/motorapart.sh smoke --quick        every part, tiny, seed 5 (ran end to end, exit 0)
scripts/motorapart.sh screen               part 1, the first seven arms (ran)
scripts/motorapart.sh screen --arms cut,cut2,own+cut,own+cut2,out1+cut2 --cols a2rucomp,best   (stopped part way)
scripts/motorapart.sh chain                part 3 (ran on stevejobs, 182 of 192 lives)
scripts/motorapart.sh reader               not run; the same for: map --arms own, inside --arms own, alive --arms own, lean, speed
scripts/motorapart.sh table                every table from what has run
bash out/motorapart/refcheck.sh            the four reference lives against out/merge-ref
```

Raw outputs: `out/motorapart/look/` (the screen's probe runs), `chain/` (182),
`table-screen.md`, `table-chain.md`. `out/oneset/` holds copies of the main checkout's
caches (`cache-main.jsonl` is its `cache.jsonl`) and this job's own `cache.jsonl` (the
smoke's lives only). Nothing is running on stevejobs; the folder `~/thonk-motorapart` is
still there.

## Checks

Every new number off: the four reference lives are byte-identical (`bash
out/motorapart/refcheck.sh`, run after the last build). Every file that includes a touched
header was built again: no warning. The engine's `threshold_cap_motor 64` under cap 1000
gives the cap doc's probe arm to the last digit (a 3,000-step chain life).

## Files touched

- Shared: `src/blob/genome.hpp` and `src/blob/genome_args.cpp` (six numbers at the end),
  `src/blob/fire.cpp` (an include, one flag, the motor compare call, the winners call),
  `src/world/task.hpp` (`chain_first`), `src/run/run_flags.cpp` and
  `src/run/run_flags_worlds.hpp` (`--chain-first`), `scripts/variants/oneset.txt` (the old
  chain row marked, the row `chainfirst` added: old columns show it as missing until it is
  run; dropping that one line undoes it).
- Mine: `src/blob/regulate.cpp`, `src/blob/regulate_rate.cpp`, `src/blob/regulate_rate.hpp`,
  `src/world/task_bandit.cpp`, `src/probe/cap_split.cpp`.
- New: `scripts/motorapart.sh`, `scripts/motorapart.py`, `scripts/motorapart_more.py`.
- Not touched: the readout files, `learn_hop.cpp`, `hop_lines.cpp`, `mnist_outside.cpp`.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, the one setting on MNIST (replacing "motor cells should not be counted
  in the sector's rate"): the hidden cells are starved by the tile's shared winner cut, not
  by the bias. With a bias per kind the bias goes to its lower limit and they fire at 21;
  with the cut split they come back (25 to 35, silent 0.08) and MNIST online falls 0.05 to
  0.11. [likely: 3 seeds]
- `docs/findings.md`, new: motor cells out of the sector's count without a bias of their
  own fire more and cost MNIST online 0.02 to 0.04; a second bias per sector for them costs
  nothing seen. Under a2rucomp the shared cut holds the hidden cells' own runaway (63 to 98
  per 1,024 when split). [likely: 3 seeds]
- `docs/findings.md`, the chain: on the opening press with four buttons only the critic is
  over the line (0.96; all else 0.29 or less; 8 seeds, the critic 6). [real]
- `docs/ideas.md`: 184 tabled part way (above). New: why the one setting's output wants
  quiet hidden cells on MNIST; a motor target of 128 lifts reach and breaks exclusive-or.
- `docs/plan.md`: item 2n not finished; the list above is what is left.
- `docs/predictions.md`: the scoring below.
- `docs/task-map.md`: the chain row is `chainfirst` from here on; the old row's passes at a
  lifted cap do not count.

## Predictions, scored (those whose runs finished)

| prediction | outcome |
|---|---|
| 1. the one setting's hidden cells back (25 or more, silent 0.10 or less) under out1, out2, own; online +0.015 under two | wrong: 16, 18, 21 with 0.17, 0.14, 0.12 silent; online -0.023, -0.002, +0.008 |
| 2. base and a2rucomp within 0.02 under own; under out1 | own yes (-0.009, +0.005); out1 no (base -0.026) |
| 3. own keeps the motor cells within 20 in 100 | yes: 208 of 216, 92.5 of 93.1, 90 of 105; its bias at the limit |
| 4. an honest target costs the base rule (under 0.195) | no: 0.196 and 0.203; under a2rucomp and best own128 loses 0.022 and 0.027 |
| 5. walkonly pins the bias and starves hidden cells more | pins yes (0.77, 1.00, 0.98 of sectors); starves no (one column of three) |
| 6. toy rows within 0.02 under out1, out2, own but one cell | own every cell; out1 and out2 one cell each, far3 under a2rucomp, and it went up (+0.022, +0.031) |
| 8. the best arm is own (0.45) | yes, as far as it goes |
| 12. the critic opens with A on 0.85 or more | yes: 0.945 with three buttons, 0.958 with four |
| 13. the base rule at cap 1000 0.25 to 0.45 and 0.15 to 0.35; at cap 64 under 0.25 | yes: 0.316, 0.218; 0.050, 0.056 |
| 14. the one setting at cap 64 under 0.3; at cap 1000 with three buttons 0.45 to 0.62 | yes: 0.119, 0.018; 0.502 |
| 15. only the critic passes the new row | yes |
| 18. cut and cut2 alone leave hidden cells under 22 | wrong: 33.8 and 25.5 (own+cut2 did not run) |
| 19. out of the cut: motor over 150 and the score under std's | cut yes (152, -0.112); own+cut 135, -0.078 |
| 20. a2rucomp within 0.02 for the five cut arms | wrong: -0.025 to -0.033 |
| 7, 9, 10, 11, 16, 17 | not run |

What I had wrong going in: I took the bias for what starves the hidden cells. It is the
winner cut, and the output does not want them back.

## Predictions (written and committed before any scoring run of this job was read)

Seen before writing these: the three results docs named in the job, `regulate.cpp`,
`regulate_rate.cpp`, `fire.cpp`, the chain world, the output rule's code. Run before writing
them, none of it a scoring run: the four reference lives (byte-identical with every new
number off); seven 3,000-step lives of the three-button chain, seed 5, one per new code
path, to see that each runs (their scores, 0.00 to 0.22, say nothing at that length; what
they showed: the engine's `threshold_cap_motor 64` under cap 1000 gives the same life as the
cap doc's probe arm to the last digit, and motor cells given their own walk toward 128 while
still in the sector's count push its bias to +8.6 with a third of the sectors at the limit
within 3,000 steps); three 3,000-step lives of the chain world with the new score; and the
script's smoke (`scripts/motorapart.sh smoke --quick`: seed 5, tiny lives), of whose tables I
read only the last two: long lives cut to 1,000 to 2,000 steps under out1, and one round of
blobbench. The smoke's map part runs two rows at full length for seed 5; I did not read it.

The arms (flags in `scripts/motorapart.py`), all at the default cap of 64 unless said:

| arm | what | cost in the machine |
|---|---|---|
| std | the default: motor cells in the sector's count, under its bias | |
| out1 | motor cells out of the sector's count and target; they still feel its bias | one more AND per tile in the count; no state |
| out2 | out of the count and of the bias (nothing added to a motor cell's threshold) | out1, and the compare done a second time for a tile's motor cells |
| own | a bias per kind of cell: each sector keeps a second bias for its motor cells, toward 32 | out2, and per sector one more count, one more 16-bit bias, one more step |
| own128 | own, with the motor cells' bias and walk both toward 128 | own, and one more random word per tile with motor cells |
| walk128 | out1, and the motor cells' own walk: target 128, cap 1000 | out1, and that random word |
| walkonly | the motor cells' own walk alone, still in the sector's count | that random word |

What I expect, MNIST world (20,000 digits, seeds 5-7; std is the cap doc's cap-64 row: base
0.214 with hidden 34 and motor 210 per 1,024; a2rucomp 0.840, 63, 93; best 0.716, 13, 105
with 19 in 100 hidden cells silent):

1. **The one setting's hidden cells come back under out1, out2 and own alike**: hidden rate
   25 or more per 1,024 and silent hidden cells 0.10 or less in all three: 0.75. Its online
   score rises by 0.015 or more under at least two of the three: 0.6 (my guess: 0.74).
2. **Under the base rule and a2rucomp little moves** in those three arms, because their
   hidden cells are not starved (base: 34; a2rucomp: 63, over the target by their own
   doing, so the bias stays at its limit whoever is counted): online within 0.02 of std for
   both columns under own: 0.7; under out1: 0.55 (the motor cells lose the +16 they sat
   under and fire more; the cap doc's "held as born" arm says far more motor spikes hurt the
   base rule).
3. **own keeps the motor cells where they were** (its own bias at the limit in the MNIST
   world, motor rate within 20 in 100 of std in every column): 0.7. So of the three it is
   the one that changes the output least.
4. **An honest target for motor cells costs the base rule** (walk128 or own128: base MNIST
   online under 0.195): 0.65; under a2rucomp and best they are within 0.02 of std or
   better: 0.5.
5. **walkonly pins the bias** (bias at +max on over half the sectors in the MNIST world
   under every column) and starves hidden cells more than std (hidden rate under std's in
   at least two columns): 0.7.
6. **The four toy rows do not move**: echo of 2, bandit, exclusive-or and far3 within 0.02
   of std under out1, out2 and own in every column but one cell at most: 0.6. The cell I
   trust least: far3 under a2rucomp.
7. **The fresh reader after an MNIST life** (linear reader of the non-sensor cells): under
   the one setting it is higher with the hidden cells back (out1 or own less std +0.005 or
   more): 0.55; under base and a2rucomp within 0.005: 0.7.
8. **The best arm is own or out1**: own 0.45, out1 0.35, out2 0.1, one of the honest-target
   arms 0.1.

The whole map and the inside test with the best arm (seeds 5-12):

9. Rows passed stay 6 / 13 / 15 or better (base / a2rucomp / best), with none of
   exclusive-or, 16 cues, order and MNIST lower by 0.03 or more: 0.5. MNIST online under
   the one setting +0.02 or more: 0.55.
10. The inside test: every one of the six cells (born, competitive learning, the one-hop
    rule; MNIST, pairs) within 0.005 of std: 0.6. The lives of that test that are not
    judged have no output rule driving the motor cells, so there is little to change; the
    pair test's taught life is the one that may move.
11. That I end up saying it should be the default: 0.3. That I say it should be added to the
    one setting's flags and not to the genome's defaults: 0.4.

The chain row (`--chain-first 1`: the share of chains opened by pressing A at once; a coin
gets 1 / k, "anything but B" 1 / (k - 1)):

12. The critic opens with A on 0.85 or more with three buttons at cap 64: 0.75; with four:
    0.6.
13. The base rule at cap 1000 (the cap doc's coin) opens with A on 0.25 to 0.45 with three
    buttons and 0.15 to 0.35 with four: 0.75. At cap 64 under 0.25 with either: 0.7.
14. The one setting at cap 64 opens with A under 0.3 with three and four buttons: 0.7; at
    cap 1000 with three buttons 0.45 to 0.62 (the cap doc saw 0.54): 0.7.
15. So only the critic passes the new row (halfway line 0.667 with three buttons, 0.625 with
    four): 0.7.

The one setting's lean to A (three buttons, cap 1000, seeds 1-12, A's share of opening
presses less the third button's; the cap doc: +0.19 ±0.09 over all stage-0 steps):

16. My guess at the mechanism, not credit over a step: the integrator cells keep the stage-0
    line's cells firing through stage 1 (they clear only on a verdict), the output rule's
    bad half gives "the other buttons" those inputs when a wrong button is pressed on stage
    1, and tiring neurons make that wrong button C more often than A (A's cells have just
    fired), so A gains stage-0 inputs more often than C does. If so: the lean goes (A less
    the third +0.05 or under) without time and hold cells: 0.5; without tiring neurons: 0.4;
    without the margin gate: 0.3; the base rule with time and hold cells alone shows +0.10
    or more: 0.3. No single piece removes it: 0.3.
17. That I can name where it comes from with a control that confirms it: 0.35.

Added after the first screen was read and before the second ran. The first screen showed
that under the one setting the bias is not what starves the hidden cells: with the motor
cells out of the count the bias goes to its lower limit on 9 in 10 sectors and the hidden
cells still fire at 16 to 21 per 1,024, and the motor cells fire at 104 whatever their
thresholds do. The tile's winners (2 a tile, shared by every cell of the tile) are what the
motor cells take. So a sixth number, `motor_winners` (the tile's cut by kind of cell), and
five more arms: cut (motor cells out of the tile's cut), cut2 (a cut of their own, 2 a
tile), own+cut, own+cut2, out1+cut2.

18. Under the one setting in the MNIST world own+cut2 brings the hidden cells to 27 or more
    per 1,024 with 0.03 or fewer silent: 0.7; its online score 0.73 or more: 0.5. cut and
    cut2 alone (the bias still shared) leave the hidden cells under 22: 0.6.
19. Motor cells out of the cut altogether (cut, own+cut) fire at over 150 per 1,024 there
    and the online score falls under std's: 0.6.
20. Under a2rucomp (4 winners a tile) the MNIST online score stays within 0.02 for all five
    arms: 0.6; the toy rows within 0.02 for own+cut2 in both columns but one cell at most:
    0.55.
