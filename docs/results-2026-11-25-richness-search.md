# What makes a sheet rich to read (2026-11-25)

Idea 98. Learning has turned out to be mostly selection at the output, so a sheet can be
judged with no learning rule: how readable are the features from its neurons? That takes
under two seconds a sheet, so hundreds can be tried.

## Method

`scripts/richness.sh run --samples 600`, then `report`, then `check --top 10`. 600 random
sheets of the full engine plus the default one, 4,096 neurons, learning off, no reward,
seed 1. Each sheet draws 22 genome numbers at random from a range (inhibition, thresholds,
leak, cooldown, wiring density, far links, delays, the branch bonus, noise, sector
regulation, pacemakers, weight steps, the connection cap). Each is scored with the feature
probe on four features, as readable from the tile next to the input (1.0 easy, under 0.3
lost). A number's effect is the rank correlation between it and the score over all sheets.
The ten best all-round sheets were then re-measured on seeds 2 and 3. The raw report is in
`docs/results-2026-11-25-richness-report.txt`; the samples are in `out/richness/`.

## Result

The default sheet is nowhere near the best.

| feature | worst sheet | median | best | default | ten best, fresh seeds |
| --- | --- | --- | --- | --- | --- |
| which cue | 1.2 | 8.5 | 19.6 | 8.5 | 12.0 to 18.6 |
| order | 0.34 | 3.7 | 8.4 | 2.2 | 5.8 to 8.2 |
| exclusive-or | 0.23 | 1.8 | 5.1 | 1.9 | 2.1 to 4.0 |
| gap | 0.31 | 2.0 | 7.0 | 1.4 | 2.3 to 6.6 |

The best sheets keep their lead on new seeds (default on those seeds: 9.2, 2.2, 2.2, 1.7).

What moves what (rank correlation over 601 sheets; under 0.15 left out):

| number | which | order | xor | gap |
| --- | --- | --- | --- | --- |
| longer local delays | | +0.31 | | +0.26 |
| denser local wiring | | | +0.29 | +0.30 |
| a bigger branch bonus | +0.16 | | +0.31 | |
| a higher branch threshold | | +0.16 | +0.22 | |
| more inhibitory neurons | +0.21 | +0.24 | | |
| lopsided weight steps (1, 4, 5) | +0.24 | +0.16 | | |
| slower leak | +0.22 | | | |
| slower sector regulation | -0.20 | -0.15 | | -0.18 |
| more far links | | | | -0.17 |

No measurable effect on any feature: inhibition gain, thresholds, cooldown, noise, the
activity target, pacemakers, far reach, the connection cap.

## Reading

- **The sheet we have been using is a mediocre one.** The best sheets read order and gap
  three to four times better and exclusive-or up to twice as well, on fresh seeds.
- **Delays for time, again.** The third independent result that delays are what make time
  readable.
- **The dendrite-branch bonus matters after all.** It is the strongest single lever on
  exclusive-or, which is exactly what a branch nonlinearity should buy. On Echo it measured
  as doing nothing. The early "does nothing" verdicts were about a task that did not need
  the mechanism, read through a readout that could not read.
- **Denser local wiring helps the hard features.**
- **Fast sector regulation, the cause of the pulse, slightly helps readability.** The pulse
  may be doing some good. Not what the Echo result suggested.
- No single number has a large effect: the best correlations are about 0.3. Richness comes
  from combinations, which is what a search finds and a one-at-a-time ablation does not.

## Not yet known

Whether a sheet that reads better also learns better through the real readout rule. That
test (the readout on the five richest sheets, tasks order, exclusive-or and gap) is running.

## Do richer sheets learn better? (added the same day)

The readout rule on a frozen sheet (`--g.learn_mode 5 --g.readout_evidence 1
--g.readout_shift 4 --g.readout_balance 1 --motor-spread 128`), on the default sheet and
five of the richest from the search (`scripts/variants/rich-sheets.txt`), seeds 5 to 8,
run with `scripts/sweep.py` on stevejobs. Late score; the learning-off twins are all 0.45
to 0.50.

| sheet | order | exclusive-or | gap |
| --- | --- | --- | --- |
| default | 0.70 | 0.90 | 0.52 |
| rich1 | 0.58 | 0.90 | 0.49 |
| rich2 | 0.85 | 0.94 | **0.93** |
| rich3 | 0.61 | 0.94 | 0.53 |
| rich4 | 0.61 | **0.99** | 0.51 |
| rich6 | **0.90** | 0.94 | 0.69 |

- **Yes, for some of them, and by a lot.** Gap, which nothing had learned (0.52), is 0.93 on
  rich2, every seed between 0.91 and 0.96. Order goes from 0.70 to 0.85 and 0.90.
  Exclusive-or reaches 0.99 on rich4.
- **rich2 is one setting that does all three**: order 0.85, exclusive-or 0.94, gap 0.93.
  Until now the time tasks needed their own delay settings.
- **The screen is not a reliable predictor.** rich1 and rich3 read order and gap far better
  than the default in the probe and learn them worse in the task. The probe's timing (a few
  ticks) is not the task's (steps of 20 ticks). A sheet has to be confirmed on the task.
- What rich2 and rich6 share: long local delays (3 to 11 and 4 to 12 ticks), a high branch
  threshold with a large bonus (6 and 5), a third or more of neurons inhibitory with a
  gentler inhibition gain (2 or 3), more connections born strong, more far links, slower
  sector regulation.

The full suite on rich2 with the readout is running as scorecard `blob-rich2`.

## Can a sheet hold a cue longer? (the hold search, same day)

500 random sheets scored on the `hold` feature: cue A or B for 6 ticks, then nothing, read
20, 60 or 150 ticks later (`scripts/richness.sh --name hold --features
hold20,hold60,hold150,which run --samples 500`). The raw report is in
`docs/results-2026-11-25-hold-report.txt`. The ten sheets with the longest holds were then
re-measured on seeds 2, 3 and 4.

| sheet | hold at 20 | at 60 | at 150 | which cue |
| --- | --- | --- | --- | --- |
| default | 1.10 | 0.23 | 0.03 | 10.7 |
| longest hold at 60 (a) | 1.69 | 3.86 | 0.22 | 3.7 |
| longest hold at 60 (b) | 0.60 | 3.39 | 0.45 | 3.5 |
| best at 150 | 1.96 | 1.45 | 0.76 | 7.1 |

- **Sixty ticks can be had from the existing numbers.** Two sheets keep the cue easy to read
  (3.4 to 3.9) three times longer than the default can hold anything, on seeds they were
  not chosen on.
- **It costs the sheet its sharpness.** Those sheets tell the two cues apart a third as
  well while the cue is on (3.5 against 10.7). What holds is fewer and weaker inhibitory
  neurons, very dense local wiring, a high activity target: a sheet that keeps ringing.
  What mixes well is the opposite.
- **Nothing holds 150 ticks reliably.** The best is 0.76.
- So holding and mixing want different sheets. One region tuned to ring, beside one tuned
  to mix, is the obvious next design (idea 112), and tuning numbers will not get past about
  a hundred ticks: longer holds need a mechanism (ideas 111, 131, cells that do not leak).

Sheet (a): `--g.inhibitory_percent 16 --g.inhibit_gain 2 --g.threshold_min 7
--g.threshold_max 19 --g.leak_shift_max 2 --g.cooldown_max 3 --g.local_connect_shift 1
--g.far_percent 37 --g.far_reach_tiles 2 --g.local_delay_min 3 --g.local_delay_max 7
--g.branch_bonus 3 --g.noise_shift 9 --g.sector_target_per_1024 67
--g.sector_smooth_shift 5 --g.pacemakers_per_1024 5`
