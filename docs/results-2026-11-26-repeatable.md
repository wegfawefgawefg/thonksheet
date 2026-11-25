# Why the sheet does not answer the same digit the same way twice (2026-11-26)

Queue item 2h, idea 168. Idea 165 found that a real cell's spike counts over the same test
digits, run from two starting states, correlate only 0.61 with themselves (0.59 on the pair
test), and that a noise-free model of the same wiring reads 0.04 higher on MNIST and 0.08 to
0.10 higher on pairs. This job looks for where that comes from, and whether one cheap change
gives a repeatable code without breaking the time rows.

## Predictions (written and committed before any scoring run was read)

Seen before writing these: idea 165's doc (0.61 / 0.59; a 20-tick gap and noise off each
moved the born MNIST reader by about 0.004) and the tick code (`fire.cpp`, `regulate.cpp`,
`birth.cpp`). Nothing of this job has been run yet.

The measure here is not quite idea 165's: the same sheet reads the test digits, sees one more
training digit, and reads them again (`--twice 1`), so the two passes are 10,001 digits
apart rather than two whole separate runs. I expect it to read within 0.03 of 0.61.

My reading of the tick code: the per-cell threshold is a random walk held to the target by a
weak pull. Up one with chance 1/16 on a spike, down one with chance 1/512 a tick; if one
threshold step changes a cell's firing chance by about 0.005 a tick, the walk's spread is
about 2.5 steps with a memory of about 3,000 ticks (150 digits), on thresholds of 4 to 12.
So between two passes over the test digits every cell's threshold is somewhere else by a
step or two, and a cell that fires 0 to 2 times a digit is very sensitive to that. The sector
bias (one step a tick, within -16 to 16) carries the previous digits' activity into the next.
Potentials (leak shift up to 4), cooldowns and the 15-tick spike ring carry the last digit.

Guesses, seed 5, born sheet (self-correlation on MNIST; born reader MNIST / pairs; base 0.61,
0.924 / 0.673):

| arm | self-corr | reader |
|---|---|---|
| base (my measure) | 0.61 | 0.924 / 0.673 |
| 1 no recurrent excitation (pixels and inhibitory cells kept) | 0.70 | 0.91 / 0.64 |
| 1 cells hear only pixels | 0.85 | 0.90 / 0.60 |
| 1 no inhibition (gain 0) | 0.55 | 0.90 / 0.64 |
| 1 inhibition with no delay | 0.64 | 0.926 / 0.68 |
| 2 thresholds and sector bias frozen after settling | **0.75** | 0.935 / 0.70 |
| 2 thresholds alone frozen | 0.71 | |
| 2 sector bias alone frozen | 0.65 | |
| 3 potentials and cooldowns cleared every digit | 0.66 | 0.928 / 0.68 |
| 3 and the spike ring cleared too | 0.70 | |
| 3 gap of 100 quiet ticks | 0.64 | 0.928 / 0.68 |
| 4 pacemakers off | 0.62 | |
| 4 noise off | 0.62 | 0.926 |
| 4 both | 0.63 | |
| 5 every delay 1 | 0.63 | 0.92 / 0.67 |
| 6 digit shown 40 ticks, counted over all | 0.70 | 0.935 / 0.69 |
| 6 shown 80 ticks | 0.78 | 0.94 / 0.70 |
| 6 only the last 10 of 20 ticks counted | 0.60 | 0.92 / 0.67 |
| 7 a pixel line fires every second tick | 0.62 | 0.92 / 0.67 |
| 8 target rate half (16 per 1,024) | 0.60 | 0.920 / 0.68 |
| 8 target rate a quarter (8 per 1,024) | 0.57 | 0.91 / 0.68 |
| 8 wider cells (`wire_pixel_shift 2`) | 0.66 | 0.935 / 0.70 |
| added: per-cell threshold regulation off from birth (sector bias alone) | 0.70 | 0.93 / 0.69 |
| added: threshold regulation 8 times slower (up 1/128, down 1/4,096) | 0.63 | |
| added: winners per tile (2) | 0.65 | |

- **The arm that explains most, of those that keep the sheet whole: regulation frozen
  during the read, self-correlation 0.75** (0.6 that it is the largest of the sheet-whole
  arms). The largest of all: cells that hear only pixels, 0.85. Slower threshold regulation
  does little (0.63): it slows the walk but, by my estimate, leaves its spread the same.
- On the transplant the arms move the same way, a little less (it is already more
  input-driven: 0.966 / 0.785).
- No single cheap change lifts the born MNIST reader by more than 0.015 (0.7); pairs move
  more than MNIST for every arm that lifts self-correlation (0.6).
- Time rows: with thresholds unregulated from birth, delayed echo and order hold within
  0.05 (0.6); the arms that cut state (reset, pixels only) are diagnostics and are not run
  on the time rows.
