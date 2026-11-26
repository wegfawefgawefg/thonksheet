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
