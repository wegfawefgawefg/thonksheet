# What a connection does over a few ticks, and what a neuron computes (2026-11-25)

Ideas 66 to 75. Ten mechanisms inside the sheet, each behind its own genome numbers (0 is
off and runs byte-identical), screened with no learning at all by how readable the
features are, then searched together with the old numbers, then the few that moved the
screen most run through the real readout rule. Idea 76 (table plus count) was not built.
The job was wound up early: what is unfinished, and the line that finishes it, is at the
end.

## Short version

- **Delayed echo is learned for the first time.** With the cue two blank steps (60 ticks)
  before the ask, the readout rule on a frozen sheet goes from 0.55 (default sheet) to
  **0.91 with tiring neurons** (`--g.tire_jump 4 --g.tire_shift 7`) and **0.84 with the
  slow second potential** (`--g.slow_leak_shift 6 --g.slow_share_shift 4`), every one of
  seeds 5-12, learning-off twins at 0.47 to 0.50.
- The same two also lift **order** (0.70 to 0.88 and 0.85; long delays gave 0.84) and
  **gap** (0.52 to 0.68 and 0.69; the linear reference on the default sheet was 0.63), and
  cost **both-not-either** 0.11 to 0.13 (0.88 to 0.78, 0.75).
- **Winner in the tile** (`--g.tile_winners 2`) takes both-not-either from 0.88 to **0.98**
  and costs a little on everything to do with time.
- Of the ten, the other seven do little or hurt in the screen. Tiring synapses on the
  sheet's own neurons help a little (order +0.3, gap +0.2); any-branch neurons help
  exclusive-or a little (+0.2 to +0.5); rebound, up/down states, veto and coincidence do
  nothing useful; coincidence on every neuron nearly silences the sheet.
- **The screen under-rates time mechanisms.** The probe's hold60 for tiring neurons rose
  only 0.22 to 0.54 ("lost" by its own scale) and the slow potential made order and gap
  read worse in the probe, yet both are the two biggest wins through the readout on the
  task's 20-tick steps. The probe's timing (6-tick cues, 3 against 9 tick gaps) is not the
  task's.

## The mechanisms and how each is stored

All in `src/blob/fire_dynamics.cpp`; the fire step (`fire.cpp`) calls them only when on.
Arrays are made at birth only when the mechanism is on.

| idea | genome numbers | what it does | state |
| --- | --- | --- | --- |
| 66 tiring synapses | `syn_tire_ticks` N, `syn_tire_keep_shift` s, `syn_sensors` | a spike from a neuron that also fired in the last N ticks reaches its targets only with chance 2^-s | a second spike ring (`sent`, 16 maps, 2 bytes per neuron) the hot loop reads instead of the real one; the mask is an OR of the last N maps ANDed with a random word |
| 67 warming synapses | `syn_warm_ticks` N, `syn_warm_shift` s, `syn_sensors` | a spike from a neuron that also fired in the last N ticks counts twice (with chance 2^-s) | `sent` plus a `warm` ring (2 more bytes per neuron); the slot loop runs a second time over `warm` |
| 68 tiring neurons | `tire_jump` J, `tire_shift` s | firing adds J to the neuron's threshold; it fades by 2^-s of what is left each tick (J 4, s 4: gone in about 40 ticks) | int16 per neuron, x16 |
| 69 slow potential | `slow_leak_shift` L, `slow_share_shift` S | a second sum of the same drive leaking by 2^-L per tick (L 5 to 8), never reset by a spike; 2^-S of it is added to the potential at the compare | int16 per neuron |
| 70 rebound | `rebound_level` R, `rebound_ticks` T, `rebound_drop` D | a potential below -R marks the neuron; for T ticks after it comes back up its threshold is D lower | uint8 timer per neuron |
| 71 up and down states | `updown_drop` D, `updown_floor` F | a neuron that fired is "up": its threshold is D lower until its potential falls below F | one bit per neuron (a word per tile) |
| 72 any-branch neurons | `any_branch_percent` | that share of neurons is driven by its best slot's excitation (with the branch bonus) less all its inhibition, not by the sum over slots | one bit per neuron, fixed at birth |
| 73 veto | `veto_percent` | in that share of neurons one inhibitory spike in a slot cancels that slot's excitation | one bit per neuron, fixed at birth |
| 74 coincidence across delays | `coincide_percent` | that share of neurons may fire only when two slots read at different delays both reach `branch_threshold` | one bit per neuron, fixed at birth; a 16-bit delay mask per neuron per tick, scratch |
| 75 winner in the tile | `tile_winners` k | after the compare only the k spikes of a tile furthest over threshold are kept (a binary search for the lowest bar that leaves at most k); losers keep their potential | none |

Kinds (72 to 74) are drawn from their own random stream so the rest of the sheet is born
the same. 72 to 74 share one per-slot path (`split_add_slot`), branch-free like the plain
one; 68 to 71 and 75 share one leak-add-compare path.

Off is byte-identical to 5133942: blobrun order (mode 0), echo in mode 5 with delays 4 to
8, echo with all delays 1, echo in mode 4, blobprobe feature gap and xor, blobbench stage0,
all `cmp`-equal.

## a. The screen (seeds 1-4, no learning) - complete

`scripts/dynamics.sh screen`, then `scripts/dynamics.sh table`. Each variant and seed: the
feature probe on the sheet as born (learning off, no reward, 2,000 tries after 5,000
ticks), readable from the tile next to the input (1.0 easy, under 0.3 lost); and a
3,000-step learning-off Echo life for the aliveness line (spikes per 1,024 per tick, target
32; share of neurons silent; share stuck). The default row is mean ± seed-to-seed spread.
Every other row is its change against the default, seed by seed (same seed, same birth),
± standard error; `*` is 2 standard errors or more. Two tiles out
(`out/dynamics/screen-table-d2.md`) says the same, smaller.

"sheet only" rows are `syn_sensors 0`: the world's sensor lines send as they are. With
sensors included, tiring synapses mostly halve the input and warming doubles it, which is
what those rows measure, not a sheet mechanism.

| variant | which | order | xor | gap | hold20 | hold60 | hold150 | rate | silent | stuck |
|---|---|---|---|---|---|---|---|---|---|---|
| default | 9.94 ±0.76 | 3.24 ±0.21 | 1.79 ±0.06 | 1.61 ±0.10 | 1.16 ±0.30 | 0.22 ±0.08 | 0.08 ±0.07 | 31.9 | 0.000 | 0.000 |
| syn tire 2 ticks, keep 1/2 | -4.09 ±0.20* | -1.43 ±0.07* | -0.71 ±0.03* | +0.16 ±0.10 | -0.48 ±0.08* | -0.12 ±0.06 | -0.01 ±0.02 | 31.9 | 0.000 | 0.000 |
| syn tire 4 ticks, keep 1/2 | -3.87 ±0.18* | -1.37 ±0.10* | -0.70 ±0.05* | +0.29 ±0.15 | -0.50 ±0.09* | -0.14 ±0.06* | -0.03 ±0.04 | 31.9 | 0.000 | 0.000 |
| syn tire 8 ticks, keep 1/2 | -3.71 ±0.16* | -1.29 ±0.10* | -0.69 ±0.05* | +0.27 ±0.14 | -0.49 ±0.07* | -0.18 ±0.05* | -0.03 ±0.05 | 31.9 | 0.000 | 0.000 |
| syn tire 4 ticks, keep 1/8 | -7.40 ±0.29* | -2.42 ±0.12* | -1.50 ±0.05* | -0.30 ±0.14* | -0.88 ±0.13* | -0.13 ±0.05* | -0.04 ±0.05 | 31.9 | 0.000 | 0.000 |
| syn tire 4 ticks, sheet only | +0.45 ±0.03* | +0.18 ±0.02* | +0.03 ±0.01* | +0.13 ±0.01* | -0.04 ±0.02 | -0.05 ±0.04 | -0.03 ±0.04 | 31.9 | 0.000 | 0.000 |
| syn tire 8 ticks, sheet only | +0.71 ±0.09* | +0.29 ±0.02* | +0.01 ±0.01 | +0.24 ±0.03* | -0.10 ±0.02* | -0.08 ±0.04 | -0.04 ±0.04 | 31.9 | 0.000 | 0.000 |
| syn tire 4 ticks, keep 1/8, sheet only | +0.74 ±0.07* | +0.26 ±0.02* | +0.09 ±0.01* | +0.18 ±0.03* | -0.18 ±0.03* | -0.11 ±0.04* | -0.03 ±0.02 | 31.9 | 0.000 | 0.000 |
| syn warm 2 ticks | +5.71 ±0.48* | +1.65 ±0.06* | +0.50 ±0.03* | +0.11 ±0.03* | +0.48 ±0.23* | -0.02 ±0.05 | -0.01 ±0.05 | 32.0 | 0.000 | 0.000 |
| syn warm 4 ticks | +4.70 ±0.50* | +1.11 ±0.04* | +0.39 ±0.04* | +0.13 ±0.11 | +0.43 ±0.21* | +0.01 ±0.08 | +0.01 ±0.02 | 32.4 | 0.000 | 0.000 |
| syn warm 8 ticks | +3.90 ±0.39* | +0.61 ±0.04* | +0.24 ±0.04* | +0.45 ±0.10* | +0.16 ±0.17 | +0.00 ±0.08 | -0.03 ±0.03 | 32.7 | 0.000 | 0.000 |
| syn warm 4 ticks, 1/2 | +2.21 ±0.24* | +0.61 ±0.04* | +0.13 ±0.01* | -0.06 ±0.06 | +0.33 ±0.16* | -0.01 ±0.08 | -0.05 ±0.04 | 32.2 | 0.000 | 0.000 |
| syn warm 4 ticks, sheet only | -0.80 ±0.12* | -0.55 ±0.03* | -0.24 ±0.03* | -0.39 ±0.02* | -0.10 ±0.04* | -0.06 ±0.06 | -0.04 ±0.04 | 32.4 | 0.000 | 0.000 |
| syn warm 8 ticks, sheet only | -1.16 ±0.14* | -0.94 ±0.04* | -0.38 ±0.02* | -0.15 ±0.08 | -0.27 ±0.04* | -0.10 ±0.04* | -0.05 ±0.04 | 32.7 | 0.000 | 0.000 |
| tire neuron 1 | +0.11 ±0.05* | +0.06 ±0.02* | +0.01 ±0.01 | +0.07 ±0.01* | -0.01 ±0.02 | -0.02 ±0.06 | -0.01 ±0.04 | 31.9 | 0.000 | 0.000 |
| tire neuron 2 | +0.40 ±0.06* | +0.14 ±0.03* | +0.02 ±0.00* | +0.20 ±0.05* | -0.04 ±0.03 | -0.04 ±0.05 | +0.02 ±0.07 | 31.9 | 0.000 | 0.000 |
| tire neuron 4 | +0.64 ±0.10* | +0.24 ±0.02* | -0.00 ±0.01 | +0.38 ±0.09* | -0.07 ±0.04 | -0.11 ±0.04* | -0.02 ±0.03 | 31.8 | 0.000 | 0.000 |
| tire neuron 8 | +0.99 ±0.13* | +0.28 ±0.09* | -0.05 ±0.03 | +0.62 ±0.13* | -0.05 ±0.06 | -0.09 ±0.07 | -0.01 ±0.06 | 31.7 | 0.000 | 0.000 |
| tire neuron 4, fade 6 | +0.42 ±0.16* | +0.42 ±0.06* | -0.17 ±0.02* | +0.40 ±0.09* | +0.19 ±0.09* | +0.14 ±0.04* | -0.01 ±0.04 | 31.8 | 0.000 | 0.000 |
| tire neuron 8, fade 6 | +0.17 ±0.18 | +0.53 ±0.09* | -0.36 ±0.05* | +0.62 ±0.14* | +0.54 ±0.10* | +0.10 ±0.04* | -0.01 ±0.04 | 31.6 | 0.000 | 0.000 |
| tire neuron 4, fade 7 | +0.17 ±0.17 | +0.42 ±0.03* | -0.24 ±0.03* | +0.45 ±0.10* | +0.27 ±0.07* | +0.32 ±0.04* | -0.01 ±0.05 | 31.8 | 0.000 | 0.000 |
| slow 6, share 1/64 | -0.02 ±0.08 | -0.08 ±0.01* | +0.00 ±0.00 | -0.01 ±0.04 | +0.06 ±0.02* | +0.07 ±0.08 | +0.01 ±0.03 | 31.9 | 0.000 | 0.000 |
| slow 6, share 1/32 | -0.29 ±0.12* | -0.20 ±0.02* | -0.09 ±0.02* | -0.12 ±0.01* | +0.08 ±0.03* | +0.16 ±0.05* | +0.07 ±0.04 | 31.9 | 0.000 | 0.000 |
| slow 6, share 1/16 | -1.49 ±0.14* | -0.65 ±0.02* | -0.35 ±0.04* | -0.40 ±0.03* | -0.09 ±0.06 | +0.25 ±0.04* | +0.13 ±0.05* | 31.7 | 0.000 | 0.000 |
| slow 6, share 1/8 | -3.82 ±0.12* | -1.67 ±0.05* | -0.84 ±0.04* | -0.97 ±0.05* | -0.41 ±0.11* | +0.25 ±0.04* | +0.11 ±0.07 | 35.4 | 0.000 | 0.001 |
| slow 7, share 1/32 | -1.58 ±0.15* | -0.67 ±0.03* | -0.38 ±0.03* | -0.38 ±0.02* | -0.34 ±0.07* | +0.11 ±0.04* | +0.11 ±0.03* | 31.7 | 0.000 | 0.000 |
| slow 7, share 1/16 | -4.19 ±0.05* | -1.61 ±0.06* | -0.91 ±0.04* | -0.97 ±0.03* | -0.69 ±0.14* | +0.13 ±0.03* | +0.12 ±0.04* | 34.2 | 0.000 | 0.001 |
| slow 8, share 1/128 | -0.19 ±0.11 | -0.12 ±0.01* | -0.09 ±0.02* | -0.04 ±0.03 | -0.09 ±0.02* | +0.01 ±0.05 | +0.03 ±0.04 | 31.9 | 0.000 | 0.000 |
| slow 8, share 1/64 | -1.36 ±0.12* | -0.56 ±0.01* | -0.36 ±0.03* | -0.28 ±0.02* | -0.42 ±0.04* | -0.04 ±0.04 | +0.03 ±0.04 | 31.7 | 0.000 | 0.000 |
| rebound 8, drop 4 | +0.05 ±0.11 | -0.16 ±0.03* | -0.12 ±0.01* | -0.07 ±0.02* | -0.07 ±0.08 | -0.08 ±0.07 | -0.04 ±0.06 | 32.2 | 0.000 | 0.000 |
| rebound 16, drop 4 | -0.03 ±0.12 | -0.08 ±0.02* | -0.08 ±0.01* | -0.04 ±0.02 | -0.05 ±0.04 | -0.04 ±0.05 | +0.01 ±0.05 | 32.1 | 0.000 | 0.000 |
| rebound 16, drop 8 | +0.01 ±0.08 | -0.25 ±0.02* | -0.20 ±0.03* | -0.15 ±0.01* | -0.13 ±0.09 | -0.00 ±0.14 | -0.03 ±0.06 | 32.4 | 0.000 | 0.000 |
| rebound 32, drop 8, 8 ticks | +0.07 ±0.12 | +0.01 ±0.01 | -0.08 ±0.01* | -0.13 ±0.03* | -0.08 ±0.06 | -0.10 ±0.06 | +0.01 ±0.04 | 32.0 | 0.000 | 0.000 |
| updown 2 | -0.18 ±0.05* | -0.04 ±0.01* | -0.00 ±0.01 | -0.05 ±0.02* | +0.03 ±0.01* | -0.12 ±0.02* | +0.03 ±0.06 | 31.9 | 0.000 | 0.000 |
| updown 4 | -0.44 ±0.07* | -0.11 ±0.01* | -0.02 ±0.00* | -0.17 ±0.02* | +0.02 ±0.03 | -0.06 ±0.05 | +0.04 ±0.03 | 32.0 | 0.000 | 0.000 |
| updown 8 | -1.06 ±0.14* | -0.35 ±0.03* | -0.07 ±0.03* | -0.37 ±0.02* | +0.09 ±0.05 | -0.08 ±0.05 | -0.03 ±0.04 | 32.1 | 0.000 | 0.000 |
| updown 4, floor -8 | -0.63 ±0.07* | -0.24 ±0.02* | -0.06 ±0.02* | -0.16 ±0.03* | +0.12 ±0.05* | -0.02 ±0.06 | -0.05 ±0.03 | 32.0 | 0.000 | 0.000 |
| any-branch 25% | -0.12 ±0.09 | +0.07 ±0.04 | +0.18 ±0.03* | +0.23 ±0.03* | -0.03 ±0.08 | -0.06 ±0.06 | -0.06 ±0.04 | 31.6 | 0.000 | 0.000 |
| any-branch 50% | -0.20 ±0.06* | +0.04 ±0.09 | +0.36 ±0.05* | +0.15 ±0.10 | -0.18 ±0.07* | -0.11 ±0.04* | -0.03 ±0.03 | 31.4 | 0.000 | 0.000 |
| any-branch 100% | -0.41 ±0.14* | -0.16 ±0.09 | +0.49 ±0.03* | -0.15 ±0.08 | -0.41 ±0.13* | -0.10 ±0.06 | -0.03 ±0.05 | 31.2 | 0.000 | 0.000 |
| veto 25% | -0.14 ±0.02* | +0.01 ±0.02 | +0.02 ±0.01* | +0.09 ±0.02* | +0.01 ±0.02 | -0.10 ±0.06 | -0.02 ±0.05 | 31.8 | 0.000 | 0.000 |
| veto 50% | -0.42 ±0.04* | +0.00 ±0.05 | +0.04 ±0.01* | +0.09 ±0.06 | -0.06 ±0.05 | -0.08 ±0.06 | -0.04 ±0.04 | 31.7 | 0.000 | 0.000 |
| veto 100% | -0.72 ±0.08* | -0.04 ±0.06 | +0.03 ±0.01* | +0.15 ±0.08 | -0.14 ±0.06* | -0.10 ±0.04* | -0.04 ±0.03 | 31.6 | 0.000 | 0.000 |
| coincide 25% | -0.05 ±0.05 | -0.16 ±0.04* | -0.10 ±0.03* | -0.08 ±0.05 | -0.10 ±0.07 | -0.11 ±0.03* | -0.03 ±0.04 | 31.9 | 0.000 | 0.000 |
| coincide 50% | +0.36 ±0.09* | -0.02 ±0.20 | +0.09 ±0.05 | -0.19 ±0.08* | -0.55 ±0.09* | -0.13 ±0.04* | -0.05 ±0.04 | 31.4 | 0.000 | 0.000 |
| coincide 100% | -6.07 ±0.54* | -2.41 ±0.16* | +2.84 ±0.13* | -1.22 ±0.09* | -1.09 ±0.15* | -0.17 ±0.04* | -0.04 ±0.04 | 2.7 | 0.000 | 0.000 |
| winners 2 | +1.91 ±0.08* | +0.45 ±0.11* | +0.96 ±0.09* | -0.83 ±0.02* | -0.29 ±0.13* | -0.05 ±0.04 | -0.03 ±0.04 | 28.0 | 0.000 | 0.000 |
| winners 4 | +0.95 ±0.15* | +0.14 ±0.02* | +0.04 ±0.03 | +0.02 ±0.03 | +0.04 ±0.01* | -0.01 ±0.06 | -0.01 ±0.03 | 31.2 | 0.000 | 0.000 |
| winners 8 | +0.15 ±0.03* | +0.03 ±0.02 | -0.01 ±0.01 | +0.07 ±0.02* | -0.02 ±0.02 | -0.07 ±0.06 | -0.04 ±0.03 | 31.8 | 0.000 | 0.000 |
| winners 16 | +0.04 ±0.04 | +0.01 ±0.02 | +0.00 ±0.01 | +0.06 ±0.01* | +0.02 ±0.02 | -0.02 ±0.08 | -0.01 ±0.04 | 31.9 | 0.000 | 0.000 |

("fade 6" and "fade 7" are `tire_shift 6` and `7`; the variants file calls them "slow fade"
and "slower fade".) Every sheet stays at its target rate with no silent or stuck neurons,
except coincidence on every neuron (rate 2.7 against 32: the sheet almost stops; its
exclusive-or gain comes with that). Nothing reaches hold150.

The paired errors are small because each variant is born from the same seed as its
default; many starred changes are a few hundredths and real but too small to matter.

## b. The search over old and new numbers together - unfinished (105 of about 450 sheets)

`scripts/dynamics.sh search` (300 planned on stevejobs, draw seed 1; 150 on this PC, draw
seed 2), then `report`. Each new mechanism is off in half the sheets; its secondary numbers
are always drawn. Stopped when the job was wound up at 105 sheets (92 from stevejobs, 13
from here), merged in `out/richness/dynamics.csv`; report in
`out/dynamics/search-report.txt`. `check` (fresh seeds for the best sheets) was not run.
Ranks now share ties (the new numbers are 0 in half the sheets); that moves the old
search's correlations by up to 0.04 (strong_log 0.24 to 0.28).

Median over the sheets with each mechanism on / off, tile next to the input:

| number | which | order | xor | gap | hold20 | hold60 | hold150 |
|---|---|---|---|---|---|---|---|
| syn_tire_ticks | 4.34/7.59 | 1.73/3.22 | 0.69/1.67 | 1.08/1.63 | 0.36/0.52 | 0.08/0.13 | 0.07/0.08 |
| syn_warm_ticks | 5.68/6.76 | 1.98/2.81 | 1.09/1.22 | 1.12/1.41 | 0.37/0.47 | 0.12/0.10 | 0.07/0.07 |
| tire_jump | 7.06/5.97 | 2.66/2.10 | 1.47/0.93 | 1.63/1.12 | 0.37/0.48 | 0.09/0.12 | 0.06/0.07 |
| slow_leak_shift | 5.98/7.84 | 2.01/3.17 | 0.93/1.54 | 0.95/1.85 | 0.35/0.54 | 0.10/0.12 | 0.07/0.07 |
| rebound_level | 6.55/6.47 | 2.32/2.30 | 1.19/1.18 | 1.39/1.30 | 0.38/0.50 | 0.10/0.12 | 0.07/0.07 |
| updown_drop | 6.41/6.47 | 2.38/2.29 | 1.20/1.12 | 1.33/1.24 | 0.49/0.37 | 0.12/0.10 | 0.07/0.06 |
| any_branch_percent | 6.32/6.72 | 2.17/2.35 | 1.14/1.44 | 1.38/1.22 | 0.38/0.51 | 0.09/0.13 | 0.07/0.06 |
| veto_percent | 6.73/5.98 | 2.35/2.25 | 1.22/1.07 | 1.27/1.33 | 0.43/0.46 | 0.12/0.10 | 0.07/0.06 |
| coincide_percent | 5.68/7.06 | 2.46/2.19 | 0.98/1.28 | 1.25/1.39 | 0.36/0.52 | 0.10/0.12 | 0.07/0.07 |
| tile_winners | 6.45/6.70 | 2.41/2.29 | 1.15/1.22 | 1.29/1.31 | 0.41/0.50 | 0.11/0.11 | 0.07/0.07 |

Strongest rank correlations of the new numbers (|r| 0.15 or more): syn_tire_ticks -0.33
to -0.40 on which, order and xor, -0.31 on hold60; slow_leak_shift -0.24 to -0.38 on the
four plain features; coincide_percent -0.22 to -0.30; updown_floor -0.22 to -0.28;
syn_sensors (1 = sensors tire too) -0.26 which, -0.34 xor; tire_jump +0.18 xor; veto
+0.16 hold60, +0.23 hold150; updown_drop +0.16 hold20, +0.17 hold60. The old levers are
the same as before (longer delays, denser wiring), plus far reach +0.17 to +0.23 on the
hold columns and far_percent -0.28 on hold150.

Over these 105 sheets: which median 6.5 (best 19.4), order 2.3 (6.8), xor 1.2 (3.9), gap
1.3 (4.8), hold20 0.46 (1.85), hold60 0.11 (2.03), hold150 0.07 (0.55); the default sheet
9.8, 3.4, 1.7, 1.6, 1.6, 0.11, 0.12. The best all-rounder reads hold60 0.78; no sheet reads
hold150 above 0.55. With up to ten mechanisms drawn at once the median sheet is worse than
the default; tiring neurons are the one mechanism whose sheets are better on order, xor
and gap in combination. Too few sheets to say more.

## c. The readout rule on a frozen sheet (seeds 5-12)

`scripts/dynamics.sh readout` (variants `scripts/variants/dynamics-readout.txt`): learn
mode 5, evidence counters, balance, 128 motor cells per button, 40,000 steps of 20 ticks.
Late score (mean of the last tenth of the windows) ± seed spread; "off" is the same sheet
with learning off; "vs default" is paired by seed ± standard error.

| task | default | tiring neurons 4, fade 7 | slow 6, share 1/16 | winners 2 | off (all four) |
|---|---|---|---|---|---|
| order | 0.695 ±0.036 | **0.875** ±0.029 (+0.180 ±0.010) | **0.851** ±0.023 (+0.156 ±0.015) | 0.665 ±0.029 (-0.030 ±0.012) | 0.48 to 0.49 |
| both-not-either | 0.883 ±0.040 | 0.776 ±0.042 (-0.108 ±0.008) | 0.749 ±0.053 (-0.134 ±0.015) | **0.978** ±0.009 (+0.095 ±0.013) | 0.48 to 0.49 |
| gap | 0.519 ±0.017 | **0.682** ±0.025 (+0.162 ±0.011) | **0.687** ±0.037 (+0.168 ±0.015) | 0.499 ±0.023 (-0.020 ±0.010) | 0.48 to 0.50 |
| delayed echo, delay 0 | 0.990 ±0.004 | 0.921 ±0.025 (-0.068 ±0.008) | 0.958 ±0.015 (-0.032 ±0.005) | 0.993 ±0.005 (+0.003 ±0.002) | 0.47 to 0.49 |
| delayed echo, delay 1 | 0.798 ±0.044 | **0.930** ±0.018 (+0.133 ±0.016) | **0.876** ±0.030 (+0.078 ±0.020) | 0.738 ±0.043 (-0.060 ±0.017) | 0.47 to 0.48 |
| delayed echo, delay 2 | 0.547 ±0.025 | **0.910** ±0.020 (+0.363 ±0.011) | **0.836** ±0.019 (+0.290 ±0.012) | 0.521 ±0.021 (-0.025 ±0.010) | 0.47 to 0.50 |
| echo k=8 | unfinished | | | | |

Per seed, delayed echo delay 2, late: default 0.56 0.51 0.57 0.53 0.56 0.54 0.53 0.58;
tiring neurons 0.92 0.93 0.90 0.92 0.90 0.87 0.90 0.94; slow 0.82 0.82 0.82 0.85 0.81 0.86
0.85 0.86. Full tables with per-seed numbers: `out/dynamics/readout.log`.

In delayed echo the cue is on for one step (20 ticks), then `delay` blank steps, then an
ask line that is the same for both cues; only the cue says which button is right. At
delay 2 the cue went off 40 to 60 ticks before the ask. The default sheet with this
readout already gets delay 0 (0.99, a cue 20 ticks back) and part of delay 1 (0.80).

## d. Cost

4,096 neurons, one core, learning off, best of 3 rounds with the configurations
interleaved, `blobbench speed`; the machine was busy with other agents' runs (load 10 to
20 of 32), so only the ratios mean anything. Bytes per neuron from `blobrun --speed` at
8x8 tiles (heap taken at birth).

| mechanism (setting) | ticks/s | against default | bytes per neuron |
|---|---|---|---|
| default | 69,800 | 1.00 | 215 |
| syn tire (4 ticks) | 68,300 | 0.98 | 218 |
| syn warm (4 ticks) | 38,700 | 0.55 | 220 |
| tiring neurons (4) | 53,900 | 0.77 | 217 |
| slow potential (8, 1/64) | 63,700 | 0.91 | 217 |
| rebound (16) | 59,400 | 0.85 | 216 |
| up/down (4) | 49,900 | 0.72 | 216 |
| any-branch (50%) | 42,900 | 0.61 | 216 |
| veto (50%) | 45,400 | 0.65 | 216 |
| coincidence (50%) | 28,500 | 0.41 | 216 |
| winners (4) | 44,600 | 0.64 | 215 |

Everything off is no slower than before the change (same session: 64.5k and 65.4k ticks/s
before, 69.9k and 69.4k after, at 4,096; 3.66k / 3.41k before and 3.95k / 3.91k after at
65,536). Warming runs the slot loop twice. Tiring neurons and the slow potential are one
int16 per neuron and a pass of plain arithmetic over each tile; neither has been tuned
for speed (the compare path with extras is a separate loop per mechanism).

## Reading

- **Holding a thing in mind needs a slow variable in each neuron, and a cheap one does it.**
  A neuron's threshold that jumps when it fires and fades over about a hundred ticks
  (`tire_jump 4`, `tire_shift 7`) leaves a trace of which neurons the cue drove, and the
  readout can read the cue from that trace a minute of ticks later: delayed echo at delay
  2 from 0.55 to 0.91. A second potential leaking 64 times slower does nearly as well
  (0.84). Both are one 16-bit number per neuron. This is what brains are thought to do
  with adaptation currents and slow synaptic currents.
- **The same slow variable gives order and gap.** Order 0.70 to 0.88, gap 0.52 to 0.68 (above
  what a linear reader got from the default sheet, 0.63). Time features are about
  remembering, and these are better at it than long delays.
- **Slow memory costs fast mixing.** Both-not-either loses 0.11 to 0.13 with either, and
  delay 0 loses a little. Winner-in-the-tile does the opposite: both-not-either 0.98, small
  losses on time. Whether winners plus tiring neurons together keep both was not run.
- **The probe screen is a poor judge of time mechanisms.** Its hold60 for tiring neurons
  was 0.54 (under its own "easy" line) and the slow potential read worse on order and gap
  in the probe, yet those two won by the most on the tasks. The probe's cue is 6 ticks and
  its gaps 3 or 9 ticks; the tasks' are 20-tick steps. For time, screen at the task's
  timing (`blobprobe linear --task ...`) or straight through the readout.
- **Most of the family is null or harmful as built**: rebound, up and down states, veto,
  coincidence across delays, warming synapses on the sheet's own neurons. Tiring synapses
  only help when kept off the world's lines, and then a little. Any-branch neurons help
  exclusive-or a little. In random combinations most of them make sheets worse.

## Unfinished, and how to resume

1. Echo k=8 through the readout (the run was 8 of 64 done; the sweep cache keeps them):
   `scripts/dynamics.sh readout` (the six finished tables come from the cache in seconds).
2. How long the hold lasts: delayed echo at delays 2, 4 and 8, default, tiring neurons,
   slow potential, both together, and tiring neurons with `tire_shift 8`:
   `scripts/dynamics.sh hold` (variants `scripts/variants/dynamics-hold.txt`, not run).
3. The search: 105 sheets of about 450 planned. More sheets, with a fresh draw seed so
   none repeat: `scripts/dynamics.sh search --samples 300 --draw-seed 3 --jobs 2`, then
   `scripts/dynamics.sh report` and `scripts/dynamics.sh check --top 10 --seeds 2,3`. The
   rows from this job are in `out/richness/dynamics.csv` (merged from stevejobs'
   `out/richness/dynamics.csv` and this PC's `out/richness/dynamics-pc.csv`).
4. Not tried: tiring neurons plus winners together on both-not-either and delayed echo;
   tiring neurons and the slow potential in the full suite; the mechanism on the minimal
   engine.

Raw outputs (worktree, not committed): `out/dynamics/screen.csv` (every probe and
aliveness row), `out/dynamics/screen-table.md`, `out/dynamics/screen-table-d2.md`,
`out/dynamics/readout.log`, `out/dynamics/sweep/` (run csvs and cache),
`out/dynamics/search-report.txt`, `out/richness/dynamics*.csv`, `out/dynamics/ident/`
(byte-identity check outputs).

## What should change elsewhere (for the coordinator; not edited here)

- docs/findings.md: a new principle, likely (one clean result, two mechanisms agreeing,
  8 seeds): "Holding a thing across a gap needs a slow variable in each neuron. A threshold
  that jumps on firing and fades over ~100 ticks, or a second potential that leaks 64
  times slower, lets the readout learn delayed echo at delay 2 (0.55 to 0.91 and 0.84) and
  lifts order (0.70 to 0.88) and gap (0.52 to 0.68); it costs both-not-either about 0.1."
  The "Time" wall: delayed echo is now learned up to a 60-tick gap; how far beyond is
  untested. The headline's "still failing: delayed echo, gap" should point here. Principle
  21 gains a case: the richness probe under-rates time mechanisms because its timing is
  not the tasks'.
- docs/findings.md principle 9 ("delays give a sense of time"): tiring neurons beat long
  delays on order through the readout (0.88 against 0.84) and add delayed echo, which
  delays never did.
- docs/ideas.md: 68 and 69 *tried: delayed echo delay 2 0.55 to 0.91 / 0.84, order and gap
  up, both-not-either down ~0.1*; 75 *tried: both-not-either 0.88 to 0.98 at k=2, small
  losses on time*; 66 *tried: harmful on the world's lines, small help on the sheet's own*;
  67, 70, 71, 73, 74 *tried: null or harmful in the screen, not run through the readout*;
  72 *tried: xor +0.2 to +0.5 in the screen, not run through the readout*; 76 not built.
  New idea: the slow variable plus winners together, to keep exclusive-or and time at once.
- docs/design.md: the ten mechanisms and their storage (the table at the top of this doc).

## Rough

- Strengths were chosen by hand, 3 to 5 each, on seeds 1-4; the readout settings are the
  first ones tried, not tuned. Delayed echo might go higher with tuning.
- The screen used `tries 2000`, the search `tries 1500`, as the earlier search did.
- Tiring and warming synapses read a separate "sent" ring; learning rules still read the
  real spikes.
- Winner-in-the-tile losers keep their potential; any-branch neurons ignore inhibition in
  other slots only through its sum; these are choices, not the only versions.
- Cost figures are from a busy machine.
