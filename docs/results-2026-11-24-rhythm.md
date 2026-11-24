# Why the blob pulses (2026-11-24)

Seen in the viewer: the whole sheet's activity rises and falls about every 20 ticks. The
owner's guess was the time a signal takes to go round the sheet. The other guess was the
sector regulator correcting late and overshooting.

## Method

`scripts/rhythm.sh` (the probe is `src/probe/rhythm.cpp`). One life per line: 4,096 neurons
unless said, seed 1, random bits on every sensor cell (1 in 8) unless said, 30,000 ticks to
settle, then the number of neurons that fired is recorded for 65,536 ticks. Period is the
lag at which that count best repeats; strength is the correlation at that lag (1 is a
perfect repeat, under 0.05 counts as none). One genome number is changed per line.

## Result

| changed | period (ticks) | strength | spread (Fano) |
| --- | --- | --- | --- |
| nothing (baseline) | 24 | 0.60 | 20 |
| no input at all | 24 | 0.64 | 32 |
| sheet 4x4 tiles (1,024 neurons) | 21 | 0.27 | 9 |
| sheet 16x16 tiles (16,384 neurons) | 25 | 0.64 | 34 |
| all local delays 1 | 19 | 0.31 | 20 |
| local delays 4 to 8 | 37 | 0.68 | 17 |
| sector smoothing 4 ticks (default 16) | 17 | 0.50 | 11 |
| sector smoothing 64 ticks | 42 | 0.11 | 12 |
| sector bias off | 38 | 0.08 | 9 |
| cooldown 1 for all | 26 | 0.35 | 20 |
| cooldown 6 to 8 | 20 | 0.57 | 15 |
| no pacemakers | 24 | 0.52 | 18 |
| inhibition gain 2 (default 5) | 77 | 0.40 | 80 |
| inhibition gain 8 | 19 | 0.23 | 6 |
| learning off | 24 | 0.60 | 19 |

## Reading

- **It is not a lap of the sheet.** A sheet four times wider (4 to 16 tiles) moves the
  period from 21 to 25 ticks. A lap would have quadrupled it.
- **It is the sector regulator hunting.** With the sector bias off the rhythm is gone
  (0.08). With slow smoothing it is nearly gone (0.11). With faster smoothing the period
  shortens (17). The regulator sees activity late, corrects, overshoots, and corrects back.
- **Local delays set part of the loop time.** Period 19, 24, 37 for delays of 1, 1 to 4,
  4 to 8, and longer delays make the rhythm stronger.
- **Not the cause:** input, pacemakers, cooldown, learning.
- **Inhibition sets how violent it is.** Weak inhibition gives slow, large bursts.
- From the viewer job: while a blob is learning Echo the pulse fades out; its learning-off
  twin keeps it at 24 ticks.

One seed per line. The differences that carry the reading are large, but this wants
repeating over seeds before anything is built on it.

## Not decided

Whether to keep the rhythm. Brains have rhythms from the same kind of loop and may use them
as a clock. Ours costs: the count of spikes per tick swings about 20 times more than
independent firing would. Slower sector smoothing removes it and keeps the rate on target.

## Does the pulse matter for learning?

Echo with 2 cues, `scripts/task.sh echo --seed N --task-seed N`, late score, with the
default smoothing (pulse present) and with `--g.sector_smooth_shift 6` (pulse gone):

| seed | pulse present | pulse gone |
| --- | --- | --- |
| 1 | 0.89 | 0.91 |
| 2 | 0.84 | 0.76 |
| 3 | 0.72 (no better than learning off) | 0.97 |
| 4 | 0.95 | 0.97 |
| mean | 0.85 | 0.90 |

Removing the pulse does not hurt Echo and may help; seed 3, the one that failed, now
learns. Four seeds is too few to call it. The ablation sweep should include this setting.

## Seen in the viewer, not yet explained

After 600,000 ticks of Echo, 87% of all connections are at the top strength, nearly every
neuron is at the cap of 96 connections, and many thresholds sit at their cap. The blob is
learning by saturating. That leaves little room to learn a second thing. The capture is
`captures/echo-tick600000.png`.
