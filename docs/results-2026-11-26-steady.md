# Do the inner rules learn more from a steadier sheet? (2026-11-26)

Queue item 2j, idea 174, following `docs/results-2026-11-26-repeatable.md` (idea 168). There a
cell's count on a digit turned out to be about two fifths regulation and carry-over, with 0.66
spikes a digit; read with regulation frozen and the digit shown 80 ticks (the quiet read) the
born sheet goes 0.924 / 0.673 to 0.954 / 0.752. Competitive learning and the one-hop rule
decide what to wire from those same noisy spikes. This job gives the rules a steadier sheet
during the life and asks whether what they learn about digits grows: lived minus a life on
shuffled digits (and, for the one-hop rule, lived minus its coin), at matched firing rate, on
MNIST and the pair test, read the standard way and the quiet way.

## Predictions (written and committed before any scoring run was read)

Seen before writing these: the four docs this job follows, the code of `learn_compete.cpp`,
`regulate.cpp` and the MNIST harness, and two timing runs on 6,000 digits (no score of them
read as a result). Nothing below has been run at full size.

Every number is at matched rate (`--g.threshold_cap 1000` in the life and the read, as
compete2's "B5, cap"), means of seeds 5-8 unless said. MNIST / pairs. Known: competitive
learning, standard read, lived minus shuffled +0.0029 on MNIST (8 seeds); the pair test at cap
was never run.

| arm (life only; the read is the same for every arm) | lived - shuffled, standard read | quiet read |
|---|---|---|
| base: compete B5, 60,000 digits of 20 ticks | +0.003 / +0.010 | +0.005 / +0.015 |
| 1 shown 40 ticks, same digits (twice the ticks) | +0.003 / +0.010 | |
| 1 shown 40 ticks, same ticks (30,000 digits) | +0.002 / +0.008 | |
| 1 shown 80 ticks, same digits | +0.004 / +0.012 | +0.006 / +0.016 |
| 1 shown 80 ticks, same ticks (15,000 digits) | +0.002 / +0.007 | |
| 2 threshold regulation 8 times slower | +0.003 / +0.010 | |
| 2 thresholds frozen after 3,000 digits, sector bias to 32 | +0.001 / +0.005 | |
| 2 sector activity smoothed over 256 ticks | +0.003 / +0.010 | |
| 3 counted: the tile's 4 most active over a digit move, toward what was on in it | +0.004 / +0.012 | +0.006 / +0.018 |
| 4 compete_shift 6 / 7, 60,000 digits | +0.003 / +0.010 | |
| 4 compete_shift 6 / 7, 120,000 digits | +0.004 / +0.011 | |

Reasons. A competitive move is made per spike, toward the sources on at that spike. The pixel
lines are on for the whole digit, so the pixel part of what a winner moves toward is the same
whether the digit is shown 20 ticks or 80, and the counters already average over many spikes;
which cells win is the noisy part, and the counters average that too. So I expect the life's
noise to matter little to what is learned, unlike the read, where one digit's count is all
there is. Freezing thresholds takes away the handicap that keeps winners from taking
everything; I expect more silent cells and a worse lived sheet, not a better one.

- Lived minus shuffled grows by 0.003 or more on MNIST (standard read, 4 seeds) under any arm:
  0.25. By 0.01 or more on pairs under any arm: 0.3. The likeliest to do it: the counted rule
  (0.35 of the 0.25), then 80 ticks with the same digits.
- If an arm grows it, "same ticks" grows it less than "same digits", i.e. part of it is more
  experience, not steadier showing: 0.6.
- The quiet read shows a larger lived-minus-shuffled than the standard read on the base life:
  0.6 (MNIST), 0.65 (pairs).
- Thresholds frozen in the life with the bias to 32: the life's rate rises to 40-60 per 1,000
  cell-ticks and 5% or more of the hidden cells end silent on the read: 0.5. Slower threshold
  regulation and the slower bias keep the rate within 10% of target: 0.8.
- The counted rule, at matched moves (`compete_shift 2`, about one move a tile a digit, as B5
  at cap), reads within 0.005 of B5's lived sheet on MNIST: 0.6.
- The one-hop rule (errors from the answer, `hop_shift 9`, cap): lived minus coin is +0.003 /
  +0.008 on the base life; it grows by 0.003 or more on MNIST under any arm: 0.3 (likeliest:
  the slower rule over 120,000 digits). Longer showing does little for it, because its marks
  cover only the last 12 ticks of a digit: 0.7.
- Nothing here reaches the teacher's wiring (0.966 / 0.785 standard, 0.975 / 0.826 quiet):
  0.97.
