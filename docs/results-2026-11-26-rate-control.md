# A rate control that does not fight the rest (idea 175, 2026-11-26)

Queue item 2k of `docs/plan.md`. The standard control (`src/blob/regulate.cpp`): each cell's
threshold rises with chance 2^-4 on a spike and falls with chance 2^-9 a tick (floor 1, cap
`threshold_cap` 64), and each sector's bias steps one toward the target every tick (within
±16). Three jobs found it in the way: it jitters a read, takes back a loop that stays lit,
and fails at both ends (cells pinned at the cap on the prediction sheet, 12 in 100 silent
under the one setting).

## Predictions (written and committed before any scoring run was read)

Seen before writing these: the three results docs named in the job, `regulate.cpp`, and two
smoke runs of the new health probe (`blobprobe ratehealth`, the standard control only, 2,000
and 3,000 steps, seed 5). The second showed the quick map's MNIST world under the base output
rule running at four times the target with the bias pinned at +16, 11 in 100 hidden
thresholds at the cap of 64 and half the motor cells firing on over a quarter of the ticks.
No candidate has been run.

The candidates (genome numbers at the end of the genome, all off by default; flags as run):

| name | what | flags |
|---|---|---|
| std | the standard control | (none) |
| e | the cap lifted | `--g.threshold_cap 1000` |
| a | dead band: a cell's threshold steps once per 2,048 ticks, only if it fired under a quarter or over four times its share; no per-spike walk | `--g.rate_band_shift 11 --g.threshold_cap 1000` |
| a8 | the same, band to eight times | a + `--g.rate_band_high 3` |
| b | critical period: the walk for the first 131,072 ticks, then thresholds stop | `--g.threshold_reg_until 131072 --g.threshold_cap 1000` |
| ba | critical period, then the dead band | b + `--g.rate_band_shift 11` |
| c | inhibition in proportion: each tile is its own sector; its bias is a slow part (one step with chance 2^-8 a tick) plus 8 threshold steps per target's worth of activity over the target, counted over about 4 ticks; the per-cell walk as now | `--g.sector_tiles 1 --g.sector_smooth_shift 2 --g.sector_gain 8 --g.sector_step_shift 8 --g.threshold_cap 1000` |
| d | slow sector bias: activity averaged over 256 ticks, one step with chance 2^-6 a tick; the per-cell walk as now | `--g.sector_smooth_shift 8 --g.sector_step_shift 6 --g.threshold_cap 1000` |
| ac | a and c together | both |
| ad | a and d together | both |

Pass lines, set now:

1. Alive: after 160,000 steps on (i) the base output rule, (ii) base with competitive
   learning, (iii) the one setting (echo at 3 tiles and delayed echo of 2): rate 0.75 to 1.33
   of target, hidden thresholds at the cap under 1 in 100, silent and stuck under 2 in 100 on
   (i) and (ii) and no worse than the standard control on (iii); stage 0 (idle, random input,
   input on and off) with no dead sector and no saturated tick; through a change (the MNIST
   world, input off 2,000 ticks, back on) the first 100 ticks back at most 1.5 times the rate
   before the gap.
2. Repeatable: the standard read of the born sheet, 8 seeds, at least half way from the
   standard to the frozen read on both tests: MNIST 0.931 or more, pairs 0.684 or more.
3. The latch: the hand-made latch with the cap at 1000 (the rate free to be held) votes 0.90
   or more at 8 blanks with the hidden rate at most 1.33 of target over the read and the next
   cue replacing it (vote 0.95 or more at blank 1); and delayed echo at 2 steps with
   competitive learning living under the candidate no more than 0.03 under the standard
   control.
4. The prediction sheet (`scripts/loops.sh pred`'s side 2, wide lattice, rule on): rate at
   most 1.25 of target, thresholds at the cap under 1 in 100, the bias at +max under half the
   time.
5. The quick map, seeds 5-8: on each of base, a2rucomp and best, no fewer rows passed than
   the column without the candidate, and none of exclusive-or, 16 cues, order and MNIST
   lower by 0.03 or more.

Guesses (pass or fail on each of the five, and a probability that the candidate passes all
five):

| candidate | 1 alive | 2 read | 3 latch | 4 pred | 5 map | all five |
|---|---|---|---|---|---|---|
| std | fail (MNIST world, one setting) | fail | fail | fail | pass | 0 |
| e | fail (the gap jump) | fail | fail | pass | pass | 0.02 |
| a | fail (the bias still falls in the gap) | pass | pass | fail | pass | 0.10 |
| a8 | fail | pass | pass | fail | pass | 0.05 |
| b | fail (drift after the period) | pass | fail | fail | fail (exclusive-or, as thregoff32) | 0.02 |
| ba | pass | pass | pass | fail | fail | 0.05 |
| c | pass | fail | fail | pass | pass | 0.05 |
| d | fail | fail | fail | pass | pass | 0.03 |
| ac | pass | pass | pass | pass | fail | 0.20 |
| ad | fail | pass | pass | fail | pass | 0.08 |

- **Passes most: ac (dead band and inhibition in proportion), 4 of 5** (0.4 that it is the
  one that passes most; 0.2 that something passes all five). The dead band removes the
  walk's jitter and leaves a latched loop alone while it averages under four times the
  target; the tile's own inhibition, being proportional and not an integral, does not wind
  up over a cue and stops bursts at once, but it may cost the quick map a row because the
  motor tiles lose the fast bias the readout leans on (the loops job: bias held at 0 cost
  delayed echo at 2 steps 0.23).
- The standard read under the best candidate: MNIST 0.934, pairs 0.688.
- The hand-made latch under the best candidate: vote 0.93 at 8 blanks at a hidden rate of 36.
- **A learned latch holds 8 steps (delayed echo at 8 steps 0.70 or more with competitive
  learning and strength from the counter, seeds 5-12): 0.2.** Strength from the counter ran
  the rate away under the standard control; under a control that tolerates a lit loop it may
  instead run the loops into each other (two cues' loops not kept apart).
- The 12 in 100 silent under the one setting: mostly time and integrator cells, whose spikes
  come from their own rule (0.6); if so no rate control changes it.

## Short version

**The candidate that passes most is e, the cap lifted (2 of 5; tied with c, which is e plus
per-tile inhibition in proportion and passes the same two): alive yes, the standard read
0.9218 / 0.6682 (was 0.924 / 0.673), the hand-made latch 0.59 at 8 blanks with the rate held,
the other sheet's rate 31.8 per 1,024, the quick map 7 / 12 / 16 rows of 22 for base /
a2rucomp / best (was 6 / 13 / 15; seeds 5-12); a learned latch was not tried, because no
candidate passed the latch.**

- **No rate control here does all three things the job asked for, and the reason is the
  same each time: what jitters the read and takes back the latch is also what competitive
  learning and the output need.** Every candidate that leaves the cells freer (a dead band,
  a critical period) or the sector bias slower loses delayed echo at 2 steps with
  competitive learning living under it: 0.96 under the standard control, 0.48 to 0.80 under
  every candidate but the lifted cap (0.90). The per-cell walk is competitive learning's
  "conscience" (DeSieno 1988: a unit that wins too often is handicapped, so every unit gets
  a share); the fast sector bias is a clock the readout leans on (the loops job: bias held
  at 0 cost the same row 0.23).
- **The standard read moves toward the frozen read only when regulation in effect stops.**
  The dead band reads 0.936 / 0.691 and with inhibition in proportion 0.937 / 0.695 (frozen
  0.938 / 0.694, 8 seeds), but only because on the MNIST sheet their sector bias sits at its
  +16 limit (rate 42 to 46 per 1,000 cell-ticks against 33): the bias is pinned, so frozen.
  With the bias given room to hold the rate (to 64) the dead band reads 0.928 / 0.674; with
  the bias slow as well, 0.932 / 0.680. The one read gain at the target rate is the
  critical period (the walk stops after 131,072 ticks, so the thresholds are frozen through
  the read): 0.931 / 0.690 at 32 per 1,000 cell-ticks, at the half-way line; it lets the
  prediction sheet run (54 per 1,024) and loses delayed echo (0.51 at 2 steps).
- **The hand-made latch survives best under the dead band with a slow bias (a5ms): vote
  0.87 / 0.80 / 0.74 at 4 / 8 / 16 blanks at a hidden rate of 38 (standard, cap lifted: 0.86
  / 0.60 / 0.53 at 32).** The loop stays lit (150 per 1,000 cell-ticks at 8 blanks, the rival
  loop 1 to 6). It is the same candidate that loses delayed echo entirely (0.49 at 2 steps).
- **Lifting the cap is the one change that only helps the sheet's health, and it should be
  the default, with two costs named.** Under the standard control at cap 64, after 160,000
  steps: the MNIST world runs at 3.1 times the target under the base output rule (10 in 100
  cells stuck, 10 in 100 at the cap) and 2.6 times with competitive learning (27 in 100 at the
  cap, 14 in 100 hidden cells silent); echo at 3 tiles with competitive learning at 1.5 times
  (20 in 100 at the cap); the prediction sheet at 2.1 times. At cap 1000 every one of these
  holds at 0.99 to 1.02 with nothing silent, stuck or capped (seeds 5-8; the standard
  control's long lives seeds 5 and 6). On the quick map it **passes
  the two-step chain under the one setting (0.150 to 0.811, seeds 5-12; under the base rule
  0.306 to 0.720)**, a row only the critic passed before. What it costs: MNIST online -0.034 /
  -0.100 / -0.043 (base / a2rucomp / best) and, with competitive learning, reach (far3 0.925
  to 0.721, far5 0.841 to 0.600, 8 seeds). At cap 64 those rows ran with the rate run away
  (the long lives); that the rate is what they gained from is likely but not separated.
- **The one setting's 12 in 100 silent cells are time and integrator cells** (silent share
  of plain hidden cells 0.001; of the integrator and time cells 0.90 to 0.93): they fire by
  their own rule, mostly on long time bins that a 16,000-tick count does not reach. No rate
  control changes it; it is not a fault.
- The prediction sheet holds at 32 per 1,024 under the per-cell walk with the cap lifted (e,
  c, d: 31.8 to 31.9). Every candidate that frees the cells lets it run (37 to 76), most of
  all with the bias limited to 16, which then has no authority left (a, ac, a8, ad, ba: 64
  to 76, the bias at +16 most of the time).
- A quiet gap still lands the next input on a fast sheet under every candidate (born MNIST
  sheet, 100 quiet ticks a digit: 61 to 103 per 1,000 cell-ticks against 33); under the slow
  bias it is worse (92 to 103), because the slow average drifts down through every gap, and
  on the MNIST world with learning the first 100 ticks back run at 1.65 (d) and 2.9 (a5ms)
  times the rate before a 2,000-tick gap, against 1.1 for the standard control.

## How it was measured

`scripts/ratecontrol.sh` (`ratecontrol.py`, `ratecontrol_tests.py`). Pass lines are the ones
written with the predictions.

1. **Alive**: `blobprobe ratehealth` (new): the life, then 820 more steps counting each
   cell's spikes (rate, silent, stuck as blobrun counts them, split into plain hidden,
   integrator and time cells, motor cells; hidden thresholds at the cap; sector bias), then
   2,000 ticks unplugged and 100 steps back, with the rate per 100 ticks. Lives: echo at 3
   tiles and delayed echo of 2 for 160,000 steps, and the quick map's MNIST world for 20,000
   digits (the change is read there: on the 3-tile worlds unplugging changes the rate by
   under 5 in 100), each under base, base with competitive learning (a2comp) and the one
   setting (best). Seeds 5 and 6 (e and c 5-8), on stevejobs and this PC. Stage 0 (`blobbench stage0`, 200,000 ticks a
   phase): no dead sector and no saturated tick for any candidate.
2. **Read**: `blobmnist ceiling --twice 1` on the born sheet as `scripts/repeat.sh` runs it,
   MNIST and the pair test, seeds 5-12; plus MNIST with 100 quiet ticks after every digit
   (`--gap 100`), seeds 5-8.
3. **Latch**: `blobprobe looptrace --loop-strong 1` after a 2-step delayed-echo life with
   competitive learning, the life and the read both under the candidate (its genome numbers
   on the probe's line, so the read runs under them too); seeds 5-8. The standard control's
   row is the loops job's "loop inputs all strong, cap 1000" (lived at 64, lifted after).
   Delayed echo at 2, 3, 4, 8 steps: 40,000-step lives with competitive learning under the
   candidate (the standard control's are the timerule job's lives).
4. **Pred**: `scripts/loops.sh pred`'s side 2 wide-lattice sheet, rule on, under the
   candidate; seeds 5-8.
5. **Map**: `scripts/oneset.py` with the candidate's numbers added to base, a2rucomp and
   best, each life beside its learning-off twin under the same candidate; seeds 5-8 (e also
   9-12).

The quick screen ran tests 2 to 4 on every candidate first; alive and the map, which take
hours, ran on those that held the rate.

## The table

| candidate | 1 alive: rate (lowest-highest of target), silent / stuck (i, ii), at cap, gap jump, stage 0 | 2 read: MNIST / pairs (self-corr, rate); MNIST with 100 quiet ticks a digit, at rate | 3 latch: vote b1 / b4 / b8 / b16, rate; delayed echo 2 / 3 / 4 / 8 | 4 pred: rate, at cap, bias at +max | 5 map: rows passed with (without) base / a2rucomp / best; worst of the four | passes |
|---|---|---|---|---|---|---|
| std | 0.85-3.08, 0.097 / 0.103, 0.275, 1.10, 0 | 0.9241 / 0.6728 (0.541, 33); gap 0.9221 at 63 | 1.00 / 0.86 / 0.60 / 0.53, 32.1; 0.96 / 0.56 / 0.50 / 0.48 | 66.0, 0.294, 0.82 | 6 (6) / 13 (13) / 15 (15); +0.000 | 0 |
| e | 0.85-1.02, 0.000 / 0.000, 0.000, 1.06, 0 **pass** | 0.9218 / 0.6682 (0.525, 31); gap 0.9239 at 61 | 1.00 / 0.79 / 0.59 / 0.55, 32.1; 0.90 / 0.68 / 0.48 / 0.48 | 31.8, 0.000, 0.14 **pass** | 7 (6) / 12 (13) / 16 (15); -0.100 | 2 |
| a | - | 0.9358 / 0.6911 (0.664, 46); gap 0.9346 at 69 **pass** | 0.96 / 0.60 / 0.57 / 0.56, 62.2; 0.48 / 0.48 / 0.48 / 0.49 | 64.8, 0.000, 0.81 | - (6) / - (13) / - (15); +0.000 | 1 |
| a8 | - | 0.9389 / 0.6960 (0.658, 58); gap 0.9352 at 69 **pass** |   -   /   -   /   -   /   -  ,   -  ;   -   /   -   /   -   /   -   | 75.5, 0.000, 0.94 | - (6) / - (13) / - (15); +0.000 | 1 |
| b | - | 0.9310 / 0.6897 (0.623, 32); gap 0.9325 at 61 | 1.00 / 0.79 / 0.62 / 0.62, 62.1; 0.51 / 0.48 / 0.48 / 0.48 | 54.0, 0.000, 0.58 | - (6) / - (13) / - (15); +0.000 | 0 |
| ba | - | 0.9315 / 0.6908 (0.622, 32); gap 0.9314 at 61 **pass** | 1.00 / 0.90 / 0.81 / 0.76, 61.8; 0.59 / 0.49 / 0.49 / 0.47 | 67.8, 0.000, 0.83 | - (6) / - (13) / - (15); +0.000 | 1 |
| c | 0.83-1.01, 0.000 / 0.000, 0.000, 1.23, 0 **pass** | 0.9253 / 0.6750 (0.525, 31); gap 0.9251 at 63 | 1.00 / 0.90 / 0.64 / 0.50, 31.9; 0.80 / 0.56 / 0.53 / 0.48 | 31.9, 0.000, 0.00 **pass** | 6 (6) / 10 (13) / 16 (15); -0.124 | 2 |
| d | 0.85-1.03, 0.000 / 0.000, 0.000, 1.65, 0 | 0.9250 / 0.6740 (0.589, 31); gap 0.9215 at 92 | 1.00 / 0.80 / 0.62 / 0.51, 32.3; 0.71 / 0.52 / 0.48 / 0.47 | 31.8, 0.000, 0.00 **pass** | - (6) / - (13) / - (15); +0.000 | 1 |
| ac | - | 0.9372 / 0.6945 (0.627, 42); gap 0.9362 at 66 **pass** | 0.99 / 0.64 / 0.61 / 0.59, 62.3; 0.48 / 0.48 / 0.49 / 0.48 | 66.7, 0.000, 0.83 | - (6) / - (13) / - (15); +0.000 | 1 |
| ad | - | 0.9358 / 0.6896 (0.696, 46); gap 0.9357 at 103 **pass** |   -   /   -   /   -   /   -  ,   -  ;   -   /   -   /   -   /   -   | 66.2, 0.000, 0.82 | - (6) / - (13) / - (15); +0.000 | 1 |
| a64 | - | 0.9283 / 0.6759 (0.598, 32); gap 0.9328 at 64 | 1.00 / 0.84 / 0.69 / 0.61, 40.5; 0.68 / 0.48 / 0.49 / 0.48 | 43.8, 0.000, 0.29 | - (6) / - (13) / - (15); +0.000 | 0 |
| a5 | - | 0.9278 / 0.6741 (0.596, 32); gap 0.9354 at 64 | 1.00 / 0.76 / 0.64 / 0.60, 36.9; 0.66 / 0.54 / 0.50 / 0.49 | 37.0, 0.000, 0.12 **pass** | - (6) / - (13) / - (15); +0.000 | 1 |
| a5m | 0.86-1.58, 0.018 / 0.000, 0.000, 1.23, 0 | 0.9272 / 0.6767 (0.588, 32); gap 0.9304 at 63 | 1.00 / 0.84 / 0.66 / 0.59, 39.6; 0.70 / 0.52 / 0.50 / 0.48 | 37.9, 0.000, 0.14 **pass** | - (6) / - (13) / - (15); +0.000 | 1 |
| a5ms | 0.87-1.51, 0.037 / 0.005, 0.000, 2.92, 0 | 0.9319 / 0.6795 (0.621, 34); gap 0.9308 at 100 | 1.00 / 0.87 / 0.80 / 0.74, 38.3; 0.49 / 0.51 / 0.49 / 0.50 | 40.0, 0.000, 0.15 **pass** | - (6) / - (13) / - (15); +0.000 | 1 |

`pass` marks a test passed by the lines above. Read: MNIST / pairs, a cell's
self-correlation and the rate per 1,000 cell-ticks in brackets; "gap": MNIST with 100 quiet
ticks a digit and the rate then. Latch: the hand-made latch's vote at blank 1 / 4 / 8 / 16 and
the hidden rate over the read; then delayed echo with competitive learning under the
candidate at 2 / 3 / 4 / 8 steps (standard: 0.96 / 0.56 / 0.50 / 0.48 on these seeds).
Pred: rate per 1,024, share of hidden thresholds at the cap, share of sector-ticks at +max.
Map: rows passed with the candidate (without it) on base / a2rucomp / best, seeds 5-8, and the
worst change of exclusive-or, 16 cues, order and MNIST.

Alive in detail, mean of the seeds, at the end of the life: setting (base, comp: base with
competitive learning, best: the one setting), world; rate over target; silent and stuck shares
of non-sensor cells; hidden thresholds at the cap; silent share of plain hidden cells; silent
share of integrator and time cells; the gap jump; late score:

- std base far3: 1.013, 0.000, 0.000, 0.005, 0.000,   -  , 0.965, 0.909
- std base delayed2: 1.008, 0.000, 0.000, 0.002, 0.000,   -  , 1.025, 0.560
- std base mnist: 3.084, 0.000, 0.103, 0.098, 0.000,   -  , 1.105, 0.206
- std comp far3: 1.523, 0.011, 0.001, 0.197, 0.012,   -  , 1.150, 0.985
- std comp delayed2: 1.095, 0.000, 0.000, 0.035, 0.000,   -  , 1.055, 0.986
- std comp mnist: 2.562, 0.097, 0.014, 0.275, 0.145,   -  , 1.000, 0.217
- std best far3: 0.853, 0.129, 0.004, 0.007, 0.001, 0.929, 1.025, 0.988
- std best delayed2: 0.936, 0.126, 0.009, 0.004, 0.001, 0.901, 0.950, 1.000
- std best mnist: 1.364, 0.143, 0.005, 0.001, 0.144, 0.610, 1.035, 0.708
- e base far3: 1.002, 0.000, 0.000, 0.000, 0.000,   -  , 1.000, 0.795
- e base delayed2: 1.005, 0.000, 0.000, 0.000, 0.000,   -  , 0.990, 0.551
- e base mnist: 0.996, 0.000, 0.000, 0.000, 0.000,   -  , 1.065, 0.171
- e comp far3: 1.013, 0.000, 0.000, 0.000, 0.000,   -  , 1.030, 0.771
- e comp delayed2: 1.001, 0.000, 0.000, 0.000, 0.000,   -  , 1.240, 0.945
- e comp mnist: 0.990, 0.000, 0.000, 0.000, 0.000,   -  , 0.927, 0.175
- e best far3: 0.850, 0.125, 0.004, 0.000, 0.000, 0.920, 1.022, 0.982
- e best delayed2: 0.919, 0.121, 0.009, 0.000, 0.000, 0.897, 0.965, 1.000
- e best mnist: 1.018, 0.057, 0.004, 0.000, 0.000, 0.590, 1.022, 0.672
- c base far3: 0.990, 0.000, 0.000, 0.000, 0.000,   -  , 1.167, 0.839
- c base delayed2: 0.993, 0.000, 0.000, 0.000, 0.000,   -  , 1.080, 0.540
- c base mnist: 1.000, 0.000, 0.000, 0.000, 0.000,   -  , 1.230, 0.173
- c comp far3: 0.990, 0.000, 0.000, 0.000, 0.000,   -  , 1.160, 0.720
- c comp delayed2: 0.984, 0.000, 0.000, 0.000, 0.000,   -  , 1.047, 0.774
- c comp mnist: 1.007, 0.000, 0.000, 0.000, 0.000,   -  , 0.953, 0.176
- c best far3: 0.833, 0.125, 0.004, 0.000, 0.000, 0.920, 1.042, 0.968
- c best delayed2: 0.891, 0.121, 0.009, 0.000, 0.000, 0.897, 0.973, 1.000
- c best mnist: 1.009, 0.057, 0.004, 0.000, 0.000, 0.590, 1.058, 0.678
- d base far3: 1.007, 0.000, 0.000, 0.000, 0.000,   -  , 1.275, 0.837
- d base delayed2: 1.005, 0.000, 0.000, 0.000, 0.000,   -  , 1.260, 0.554
- d base mnist: 1.005, 0.000, 0.000, 0.000, 0.000,   -  , 1.650, 0.170
- d comp far3: 0.990, 0.000, 0.000, 0.000, 0.000,   -  , 0.830, 0.734
- d comp delayed2: 0.982, 0.000, 0.000, 0.000, 0.000,   -  , 0.765, 0.768
- d comp mnist: 1.012, 0.000, 0.000, 0.000, 0.000,   -  , 1.230, 0.185
- d best far3: 0.850, 0.127, 0.004, 0.000, 0.000, 0.916, 1.020, 0.988
- d best delayed2: 0.933, 0.125, 0.009, 0.000, 0.000, 0.901, 0.975, 1.000
- d best mnist: 1.030, 0.061, 0.005, 0.000, 0.000, 0.610, 1.120, 0.705
- a5m base far3: 1.024, 0.000, 0.000, 0.000, 0.000,   -  , 1.025, 0.856
- a5m base delayed2: 1.027, 0.000, 0.000, 0.000, 0.000,   -  , 1.000, 0.577
- a5m base mnist: 1.171, 0.000, 0.000, 0.000, 0.000,   -  , 1.235, 0.151
- a5m comp far3: 1.204, 0.002, 0.000, 0.000, 0.002,   -  , 1.115, 0.768
- a5m comp delayed2: 1.262, 0.018, 0.000, 0.000, 0.020,   -  , 1.030, 0.671
- a5m comp mnist: 1.578, 0.000, 0.000, 0.000, 0.000,   -  , 1.175, 0.184
- a5m best far3: 0.862, 0.129, 0.005, 0.000, 0.001, 0.927, 1.020, 0.990
- a5m best delayed2: 0.923, 0.126, 0.009, 0.000, 0.001, 0.901, 0.970, 1.000
- a5m best mnist: 1.052, 0.061, 0.005, 0.000, 0.000, 0.610, 1.105, 0.667
- a5ms base far3: 1.066, 0.000, 0.000, 0.000, 0.000,   -  , 1.310, 0.874
- a5ms base delayed2: 1.069, 0.000, 0.000, 0.000, 0.000,   -  , 0.950, 0.539
- a5ms base mnist: 1.151, 0.000, 0.000, 0.000, 0.000,   -  , 2.925, 0.172
- a5ms comp far3: 1.340, 0.037, 0.005, 0.000, 0.039,   -  , 1.070, 0.708
- a5ms comp delayed2: 1.129, 0.002, 0.000, 0.000, 0.002,   -  , 1.075, 0.490
- a5ms comp mnist: 1.508, 0.001, 0.000, 0.000, 0.001,   -  , 1.350, 0.167
- a5ms best far3: 0.870, 0.128, 0.005, 0.000, 0.000, 0.922, 1.015, 0.996
- a5ms best delayed2: 0.937, 0.125, 0.009, 0.000, 0.000, 0.901, 0.980, 1.000
- a5ms best mnist: 1.100, 0.061, 0.005, 0.000, 0.001, 0.610, 1.130, 0.667

The quick map for the cap lifted, seeds 5-12 (`out/ratecontrol/map-e-5-12.md`; in brackets
the paired difference to the base column):

| row | base | e-base | a2rucomp | e-a2rucomp | best | e-best |
|---|---|---|---|---|---|---|
| echo2 | 1.000* | 0.973* (-0.027 ±0.009-) | 1.000* (+0.000 ±0.000) | 0.948* (-0.052 ±0.015-) | 1.000* (+0.000 ±0.000) | 1.000* (+0.000 ±0.000) |
| echo8 | 0.801* | 0.802* (+0.001 ±0.004) | 0.925* (+0.124 ±0.013+) | 0.910* (+0.109 ±0.010+) | 0.976* (+0.176 ±0.013+) | 0.988* (+0.187 ±0.011+) |
| echo16 | 0.266 | 0.267 (+0.001 ±0.006) | 0.620* (+0.354 ±0.014+) | 0.628* (+0.362 ±0.011+) | 0.955* (+0.689 ±0.012+) | 0.949* (+0.683 ±0.011+) |
| shuffled | 1.000* | 0.977* (-0.023 ±0.008-) | 1.000* (+0.000 ±0.000) | 0.953* (-0.047 ±0.012-) | 1.000* (+0.000 ±0.000) | 1.000* (+0.000 ±0.000) |
| far3 | 0.847* | 0.797* (-0.050 ±0.014-) | 0.925* (+0.079 ±0.017+) | 0.721 (-0.126 ±0.020-) | 0.975* (+0.128 ±0.016+) | 0.962* (+0.115 ±0.017+) |
| far5 | 0.576 | 0.565 (-0.011 ±0.004-) | 0.841* (+0.265 ±0.029+) | 0.600 (+0.024 ±0.011+) | 0.933* (+0.357 ±0.011+) | 0.901* (+0.325 ±0.013+) |
| xor | 0.882* | 0.851* (-0.032 ±0.012-) | 0.997* (+0.115 ±0.012+) | 0.995* (+0.113 ±0.011+) | 0.936* (+0.054 ±0.009+) | 0.961* (+0.079 ±0.011+) |
| parity3 | 0.561 | 0.548 (-0.013 ±0.010) | 0.831* (+0.270 ±0.011+) | 0.883* (+0.322 ±0.012+) | 0.642 (+0.081 ±0.011+) | 0.641 (+0.080 ±0.015+) |
| order | 0.676 | 0.677 (+0.001 ±0.013) | 0.733 (+0.057 ±0.029) | 0.843* (+0.167 ±0.020+) | 0.994* (+0.318 ±0.012+) | 0.996* (+0.319 ±0.012+) |
| gap | 0.509 | 0.518 (+0.009 ±0.011) | 0.938* (+0.429 ±0.022+) | 0.924* (+0.415 ±0.013+) | 0.897* (+0.387 ±0.032+) | 0.885* (+0.376 ±0.037+) |
| delayed2 | 0.544 | 0.553 (+0.009 ±0.011) | 0.919* (+0.375 ±0.019+) | 0.925* (+0.381 ±0.016+) | 1.000* (+0.456 ±0.007+) | 1.000* (+0.456 ±0.007+) |
| hold1000 | 0.506 | 0.485 (-0.021 ±0.032) | 0.508 (+0.002 ±0.029) | 0.476 (-0.030 ±0.026) | 0.998* (+0.492 ±0.022+) | 1.000* (+0.494 ±0.022+) |
| bandit | 0.773* | 0.756* (-0.017 ±0.004-) | 1.000* (+0.227 ±0.003+) | 0.960* (+0.188 ±0.005+) | 0.995* (+0.223 ±0.003+) | 0.984* (+0.211 ±0.003+) |
| chain | 0.306 | 0.720* (+0.415 ±0.052+) | 0.000 (-0.306 ±0.054-) | 0.525 (+0.219 ±0.061+) | 0.150 (-0.155 ±0.130) | 0.811* (+0.506 ±0.134+) |
| cartpole | 17 | 23 (+5.477 ±0.859+) | 13 (-4.450 ±0.244-) | 16 (-1.114 ±0.388-) | 17 (-0.502 ±0.633) | 18 (+0.500 ±0.659) |
| catch | 0.298 | 0.333 (+0.034 ±0.011+) | 0.366 (+0.067 ±0.016+) | 0.372 (+0.073 ±0.013+) | 0.671* (+0.373 ±0.031+) | 0.681* (+0.383 ±0.024+) |
| noisy | 0.268 | 0.272 (+0.004 ±0.007) | 0.218 (-0.051 ±0.033) | 0.321 (+0.053 ±0.012+) | 0.307 (+0.039 ±0.026) | 0.337 (+0.069 ±0.039) |
| copy2 | 0.638 | 0.636 (-0.003 ±0.015) | 0.927* (+0.289 ±0.019+) | 0.886* (+0.248 ±0.013+) | 0.897* (+0.259 ±0.022+) | 0.903* (+0.265 ±0.024+) |
| inv-scale | 0.699 | 0.654 (-0.045 ±0.020-) | 0.719 (+0.020 ±0.047) | 0.527 (-0.172 ±0.026-) | 0.602 (-0.097 ±0.057) | 0.586 (-0.113 ±0.056-) |
| inv-position | 0.487 | 0.504 (+0.017 ±0.007+) | 0.536 (+0.049 ±0.025) | 0.537 (+0.051 ±0.028) | 0.471 (-0.016 ±0.023) | 0.458 (-0.029 ±0.018) |
| pairs | 0.273 | 0.265 (-0.008 ±0.027) | 0.358 (+0.085 ±0.026+) | 0.427 (+0.154 ±0.027+) | 0.525 (+0.252 ±0.039+) | 0.525 (+0.252 ±0.047+) |
| mnist | 0.214 | 0.180 (-0.034 ±0.006-) | 0.838* (+0.624 ±0.005+) | 0.738* (+0.524 ±0.011+) | 0.714* (+0.499 ±0.007+) | 0.671* (+0.457 ±0.008+) |
| rows passed | 6 | 7 | 13 | 12 | 15 | 16 |

`*` passes (halfway from chance to perfect and above its twin by 2 standard errors); in brackets the paired difference to base, `+`/`-` at 2 standard errors.

Inhibition in proportion (c) on the quick map, seeds 5-8: base 6 rows (6), a2rucomp 10 (13:
it loses echo of 2 0.181, gap 0.270, order 0.124, copy 0.166, far3 0.209), best 16 (15: the
chain again, 0.298 to 0.897; MNIST -0.038).

## Why nothing passes the latch and keeps competitive learning

The same lever does both. The hand-made latch is a set of loop cells firing at five to eight
times the target for as long as a cue is held, and with two cues each loop cell is lit about
half the time: on average three to four times its share. Any control that holds each cell to
its share over the long run takes it back (the standard walk at cap 1000, c, d: 0.59 to 0.64 at
8 blanks); a control that lets a cell sit at four times its share keeps it (a5ms 0.80). But
competitive learning needs exactly that per-cell control: without it a few cells win every
time and the rest never fire, the tile's winners fill at four a tick, the sheet runs at twice
the target (a, ac: 62 per 1,024, 40 in 100 thresholds at the floor) or, with the bias given
room, holds the rate while half the thresholds sink to the floor (a5, a64, a5ms). Delayed echo
at 2 steps goes from 0.96 to 0.48-0.70 in every such life. Keeping the walk on the motor cells
(a5m) gives back a little (0.70). Slowing only the sector bias (d) or adding per-tile
inhibition on a slow bias (c) keeps the walk but costs the same row 0.16 to 0.25, as holding the
bias at 0 did in the loops job: the readout uses the fast bias.

So the two jobs this control does, equalising cells for competitive learning and keeping a
sector's spikes on time for the readout, are the jobs that also end a held loop and jitter a
read. A latch that is to live with per-cell equalisation has to be one whose cells fire at
about twice their share when lit and not at all otherwise, which the hand-made latch is not.

## Hardware

- e, the cap lifted: nothing new; thresholds are already 16-bit words and the cap is one compare.
- a, the dead band: a spike counter per cell. With windows of 2^11 ticks a cell's share of the
  target is 64 spikes, the band is 16 to 256, so a counter that stops at 257 needs 9 bits (the
  code keeps 16). Per tick: one increment per spike (a walk over the set bits of the fired
  word, about 2 a tile). Per window, staggered so one tile in 2,048 closes each tick: one
  compare per cell and at most one threshold step. No random numbers: it removes the walk's
  five random words per tile per tick (four ANDed for 2^-4, one for 2^-9).
- b, the critical period: one compare of the tick count; after it no per-cell work at all.
- c, inhibition in proportion: per tile (each tile its own sector) the smoothed count that
  exists already, a 16-bit slow part, and a reciprocal of the target fixed at birth (or a
  shift for a power-of-two target). Per tile per tick: a subtract, a multiply and a shift, and
  the slow step's chance (one random word per 64 tiles a tick at 2^-8).
- d, the slow sector bias: existing numbers; one random word per 64 sectors a tick.

`blobbench` (8 tiles, this PC under the temperature governor, rough), with every new number
off, three interleaved rounds against the branch point: base 65,200-68,700 ticks/s with
learning off and 46,200-47,800 with it on; this branch 68,900-69,400 and 47,000-48,200 at the
first commit; at the last, base 66,700-68,000 and this branch 68,400-69,200 off, 45,800-46,600
and 47,400-47,800 on. Nothing in `fire.cpp` changed. A clean build (`build-clean`) printed one
warning line, from no file this job touched (each touched file rebuilt alone: none); the four
reference lives are byte-identical (`bash out/ratecontrol/refcheck.sh`).

## What is real, what is suspect

Real:
- Lifting the cap holds the rate on every sheet and life tried (0.99 to 1.02 of target at the
  end of the life, nothing at the cap, seeds 5-8), where cap 64 lets the MNIST world and competitive
  learning run to 1.5 to 3.1 times with a fifth to a quarter of thresholds capped (seeds 5, 6).
- The chain passes under the one setting with the cap lifted: 0.150 to 0.811 on seeds 5-12
  (above its learning-off twin by more than 2 standard errors); under the base output rule
  0.306 to 0.720. Under c too (0.897, seeds 5-8).
- Every candidate that frees the cells or slows the bias loses delayed echo at 2 steps with
  competitive learning (4 seeds each, every candidate 0.16 to 0.48 down).
- The read reaches the frozen read only with the bias pinned (8 seeds).

Suspect:
- The chain result is large and new. A row paid one step late passing when the cap is
  lifted says the cap of 64 was locking the output; that is a guess, not looked at.
- Alive used two seeds (5, 6) for std, d, a5m and a5ms and four for e and c; the alive test
  was not run for the candidates that failed the screen (a, ac, a8, ad, a64, a5, b, ba).
- The dead band's read gain comes with the bias pinned and the rate at 42 to 46; the
  repeatability job's "thregoff" read 0.940 at rate 59. Some of it is rate.
- The latch's vote is fitted on half the trials and is noisy at 4 seeds (a5ms 0.75 to 0.89
  by seed at 8 blanks).
- Candidates were set by hand; the screen chose a5, a64, a5m and a5ms after seeing a and ac
  fail (written in the script's list).
- c's gain (8 steps per target's worth) and d's speeds were not tuned.

## Not done

- The learned latch (nothing passed the latch); `scripts/ratecontrol.sh learned NAME` is
  written and ran only in the smoke check.
- Winners per tile set from the rate error (idea 170): the silent cells it was for are time
  and integrator cells, so it was not built.
- c on seeds 9-12; the quick map for d and the dead-band family; the alive test for the
  candidates that failed the screen first.
- Why the cap of 64 blocks the chain; why the cap of 64 helps MNIST online and reach under
  competitive learning (the rate, by the long lives, but not separated).
- A rate-matched read of the dead band (its read at 32 per 1,000 is a5 / a64, which lose it).

## Script lines

```
scripts/ratecontrol.sh smoke --quick                 every test on std and ac, tiny (ran end to end)
scripts/ratecontrol.sh std,e,a,ac,a5,a64,a5m,a5ms,c,d,b,ba --tests read,pred,latch    the screen
scripts/ratecontrol.sh a8,ad --tests read,pred
scripts/ratecontrol.sh e,c --tests map               quick map, seeds 5-8
scripts/ratecontrol.sh e --tests map --seeds 9-12    the cap lifted on the report seeds
MINI_DIR=thonk-ratecontrol scripts/mini.sh bg rc 'THONK_MNIST=$HOME/thonk-hop2/data/mnist scripts/ratecontrol.sh std,e,c,d,a5m,a5ms --tests alive --seeds 5,6 --jobs 2'
scripts/ratecontrol.sh e,c --tests alive --seeds 7,8 (e on stevejobs, c here)
scripts/ratecontrol.sh table                         the table from what has run
scripts/ratecontrol.sh learned e                     the learned latch under a candidate (only --quick ran)
bash out/ratecontrol/refcheck.sh                     the four reference lives against out/merge-ref
```

Alive ran on stevejobs; its outputs were pulled into `out/ratecontrol/alive/`. New quick-map
lives are in `out/oneset/cache.jsonl` of this worktree (the main checkout's caches were copied
in as `cache-main.jsonl` and the rest).

## Predictions, scored

| prediction | outcome |
|---|---|
| ac passes most, 4 of 5 (0.4) | wrong: ac passes 1 (the read, with its bias pinned); e and c pass 2 |
| something passes all five (0.2) | no; the most is 2 |
| the standard read under the best candidate 0.934 / 0.688 | e: 0.922 / 0.668, no better than the standard |
| the hand-made latch under the best candidate 0.93 at 8 blanks, rate 36 | e: 0.59 at 32; the best latch, a5ms, 0.80 at 38 |
| a learned latch holds 8 steps (0.2) | not tried: no candidate passed the latch |
| the one setting's silent 12 in 100 are time and integrator cells (0.6) | right: plain hidden cells 0.001 silent, the memory cells 0.90 to 0.93 |
| std: fails alive, read, latch, pred; passes the map | right on all four tested |
| e: fails alive (gap jump), read, latch; passes pred and map | alive passes (jump 1.06), map fails (MNIST -0.10 under a2rucomp, a2rucomp 13 to 12 rows); read, latch, pred right |
| a: passes read and latch, fails alive and pred | read and pred right; latch wrong (0.57, rate 62, delayed echo 0.48) |
| b: passes read, fails latch, pred and map | latch and pred right; read at the line (0.9310 against 0.931, pairs 0.690 passes); map not run |
| ba: passes alive, read and latch, fails pred and map | read and pred right; latch wrong (0.81 at 8 blanks but at rate 62) |
| c: passes alive, pred and map, fails read and latch | alive, pred, read, latch right; map wrong (a2rucomp 13 to 10) |
| d: fails alive, read, latch; passes pred and map | alive (gap jump 1.65), read, latch, pred right; map not run |
| ac: passes alive, read, latch and pred, fails the map | read right; latch and pred wrong (rate 62 to 67) |

I expected the dead band to keep competitive learning whole and only stop the jitter. It is
the reverse: the per-cell walk is what competitive learning stands on, and the read improves
only where the bias is in effect frozen.

## What should change elsewhere (for the coordinator; not edited here)

- `src/blob/genome.hpp`: `threshold_cap` default 1000 (a core-owner decision; byte-identity of
  the reference lives would change, so `out/merge-ref` would be made again). Every result that
  leans on reach with competitive learning, MNIST online or the chain should be read again
  with it.
- `docs/findings.md`, new finding (likely, 4 to 8 seeds): **per-cell rate control is
  competitive learning's conscience and the fast sector bias is the readout's clock; the same
  two are what jitter a read and take back a held loop.** Freeing cells (dead band, critical
  period) or slowing the bias costs delayed echo at 2 steps 0.16 to 0.48 with competitive
  learning; the only reads at the frozen level come with the bias pinned.
- `docs/findings.md`: the cap of 64 binds in long lives of the MNIST world (3.1 times the
  target, 10 in 100 stuck) and of competitive learning (1.5 to 2.6 times, a fifth to a quarter
  capped); lifted, the rate holds everywhere. The chain passes under the one setting with it
  lifted (0.15 to 0.81, 8 seeds), and MNIST online and reach with competitive learning lose
  (they had leaned on the runaway rate). [likely]
- `docs/findings.md`, the one setting's "12 in 100 silent": they are time and integrator
  cells, not a rate-control fault. [real: by kind, 4 seeds]
- `docs/plan.md`: item 2k done with this doc. Queue: (a) the chain with the cap lifted, on the
  critic's rows and on seeds 1-4 (why does it pass?); (b) a latch built to live with per-cell
  equalisation (loops whose lit rate is about twice the share, held up by the tile's
  inhibition rather than by strong inputs), before any learned latch; (c) the cap default
  decided.
- `docs/ideas.md`: 175 done (above). 170 (winners from the rate error): its premise (the
  silent 12 in 100) is gone; park it. New: a loop that is bistable at twice its share.
- `docs/predictions.md`: the predictions above, scored.
- `docs/notes-world.md` (or wherever probes are listed): `blobprobe ratehealth`.

## Files that may collide with other branches

- Shared: `src/blob/regulate.cpp` (`regulate_thresholds` takes the shifts and a motor-only
  flag; new `regulate_cells`; a five-line block in `regulate_one_sector`), `src/blob/genome.hpp`
  (eight numbers at the end), `src/blob/genome_args.cpp` (eight lines at the end),
  `src/blob/blob.hpp` (one pointer at the end of `Blob`), `src/blob/birth.cpp` (one include,
  one `rate_free` call), `src/probe/main.cpp` (one declaration, one table line).
- New: `src/blob/regulate_rate.cpp`, `src/blob/regulate_rate.hpp`, `src/probe/rate_health.cpp`,
  `scripts/ratecontrol.sh`, `scripts/ratecontrol.py`, `scripts/ratecontrol_tests.py`,
  `scripts/ratecontrol_latch.py`.
- Not touched: `fire.cpp`, `learn_compete.cpp`, `scripts/ladder3*`, `scripts/ladder4*`,
  `loops.py`, `loops_probe.py`, `repeat.py`, `oneset.py`, `timerule_lib.py` (imported only).
