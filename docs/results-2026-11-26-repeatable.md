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

## Short version

**A cell's answer to the same digit correlates 0.61 with itself; with regulation held still
during the read and the digit shown 80 ticks it correlates 0.88 (0.54 before, by this doc's
measure), the born reader goes 0.924 / 0.673 to 0.954 / 0.752 and the transplant 0.966 /
0.785 to 0.975 / 0.826; the time rows do not survive the longer showing as a setting for a
life (every step 80 ticks: the best setting's order row falls to chance, 0.994 to 0.493, and
copy loses 0.153), so it is a way to read a sheet, not a fix.** (MNIST / pair test, the fresh
reader on the test digits, 8 seeds, 5-12, every seed above its base on every column.)

- **The state that hurts is regulation moving and what one digit leaves for the next, not
  the recurrent wiring, inhibition, delays or pacemakers.** Cut recurrent excitation or all
  recurrence, give inhibition no delay, set every delay to 1, take the pacemakers out: the
  born MNIST reader moves by 0.000 to 0.009 and a cell's self-correlation by -0.02 to +0.04
  (seed 5). Freezing the thresholds and the sector bias after settling lifts the born reader
  by 0.014 on MNIST and 0.021 on pairs (8 of 8 seeds). A sheet held still (regulation
  frozen; potentials, cooldowns and the spike ring cleared at each digit; no noise, no
  pacemakers) answers every digit exactly the same way and reads **0.952 / 0.735 born and
  0.975 / 0.824 transplanted** (8 seeds): it closes 0.028 of the 0.037 between the born
  sheet and idea 165's noise-free model on MNIST, and 0.062 of 0.095 on pairs. Held still
  and shown 80 ticks (seed 5) it reads 0.956 / 0.756 born and 0.976 / 0.836 transplanted,
  within 0.004 to 0.018 of the model given the same wiring and digits (0.961 / 0.765, 0.980
  / 0.854, idea 165's match table).
- **The rest is small counts.** A cell fires 0.66 times a digit. Shown 80 ticks (four times
  the spikes) a cell is 0.69 self-correlated instead of 0.54 and the born reader gains 0.021
  (MNIST) and 0.044 (pairs), 8 of 8 seeds. One noise spike in 1,024 ticks takes a held-still
  sheet from 1.00 to 0.74, with or without recurrence (0.74 when cells hear only pixels), yet
  costs the reader 0.002: self-correlation overstates what noise does to a reader.
- **A quiet gap makes it worse.** 100 quiet ticks after every digit: during the gap
  regulation lowers thresholds and bias, the next digit lands on a hair-trigger sheet (63
  spikes per 1,000 cell-ticks against 33), self-correlation falls to 0.45. Idea 165's 20-tick
  gap pointed the wrong way.
- **The two cheap levers idea 165 left do not pay in the real sheet.** A per-cell target of
  half or a quarter (19 and 12 spikes per 1,000 cell-ticks) moves the born reader by +0.005 /
  +0.003 on MNIST and +0.015 / +0.007 on pairs, the transplant by +0.000 / -0.002 and +0.000
  / -0.011, and makes cells less repeatable (seed 5). Wider cells (`wire_pixel_shift 2`):
  born +0.015 on MNIST at 1.7 times the firing rate, -0.003 on pairs; idea 165's model
  trained on the wide sheet reads 0.981 / 0.851 and its transplant 0.967 / 0.772 (normal:
  0.980 / 0.854, transplant 0.967 / 0.786): the fan-in does not hold the real sheet back.
- **What it is: a finding about how the sheet is read, and one engine-level lever that is
  not free.** Freezing regulation and clearing carry-over are not things a sheet does in a
  life; they show where a reader's noise comes from. The one engine setting that moves the
  same way (no per-cell threshold regulation, the sector bias allowed to 32) lifts the born
  reader by 0.014 / 0.010 (seed 5), keeps every time row of the best setting and costs it
  exclusive-or 0.115 (below). Showing a digit longer works on every MNIST column and costs
  four times the ticks per answer; as a life's step length it breaks order and copy.

## How it was measured

Every number is `blobmnist ceiling` as idea 165 ran it (`scripts/mnist.sh`'s sheet,
`--scale 2 --passes 10 --lr-1000 1 --views 1`, `--settle 3000`, `--fit 60000`, the 10,000 test
digits; the pair test is `--pair 2 --pair-mix 1`), on the born sheet and on idea 165's
transplanted wiring (`--wiring-load`, the bottom rung of its model), with one change, and
`--twice 1`:

- (a) **self-correlation**: after the test digits the sheet sees one more training digit and
  reads the test digits again; for each non-sensor cell, the correlation over the 10,000
  digits of its count in the first pass with its count in the second, mean over the cells
  that spiked in both passes (all but a few in every arm; "kept" in `table.txt`).
- (b) **reader**: the inside-learning test's fresh reader of the 3,072 non-sensor cells, fit
  on the training digits, scored on the first pass over the test digits.
- (c) **rate and active**: spikes per 1,000 cell-ticks counted, and the share of cell-digits
  with a spike, on the first pass.

**This is not quite idea 165's measure, and the two disagree in a way that says something.**
Idea 165 ran two whole separate sheets (3,000 and 3,001 digits of settling) and compared their
test rows; `scripts/repeat.sh check` does that here and gets idea 165's 0.609 (MNIST) and
0.594 (pairs), seed 5. Reading one sheet twice, 10,001 digits apart, gives 0.536 and 0.520.
The two separate sheets saw the same 60,000 training digits, and their slow state
(thresholds, sector bias) is partly entrained by that stream; one sheet read twice has had
10,000 more digits of drift between its passes. With regulation frozen the two measures
split the other way: two sheets frozen at different moments differ for the whole read, one
frozen sheet read twice does not. The reader fits and scores within one sheet, so the
one-sheet measure is the one that matches what a reader suffers.

Both measures, seed 5 (`scripts/repeat.sh check`; one sheet read twice in brackets):

| arm | MNIST born | MNIST transplant | pairs born | pairs transplant |
|---|---|---|---|---|
| base | 0.609 (0.536) | 0.694 (0.612) | 0.594 (0.520) | 0.547 (0.463) |
| freeze | 0.504 (0.664) | 0.673 (0.793) | 0.571 (0.660) | 0.414 (0.662) |
| freeze+show80 | 0.648 (0.853) | | 0.760 (0.892) | |
| show80 | 0.795 (0.694) | | 0.790 (0.687) | |
| still | 0.540 (1.000) | | 0.637 (1.000) | |

By idea 165's own measure the longer showing alone lifts 0.61 / 0.59 to 0.80 / 0.79, and is the
largest single change by either measure. Two held-still sheets answer differently (0.54) only because their thresholds and biases were
frozen at different moments; each of them answers itself exactly. The reader, fit and scored
in one sheet, gains in both cases.

## Every arm (seed 5)

Self-correlation / reader, test digits. Born: the born sheet; transplant: idea 165's seed-5
wiring. Last column: spikes per 1,000 cell-ticks counted on the born sheet, MNIST / pairs
(the standard sheet holds 33). `scripts/repeat.sh arms`; `out/repeat/arms/table.txt`.

| arm | MNIST born | MNIST transplant | pairs born | pairs transplant | rate |
|---|---|---|---|---|---|
| base: the standard sheet | 0.54 / 0.9228 | 0.61 / 0.9666 | 0.52 / 0.6787 | 0.46 / 0.7857 | 33 / 33 |
| rec1: no excitation from other cells (pixels, inhibition kept) | 0.52 / 0.9259 | 0.55 / 0.9632 | 0.51 / 0.6972 | 0.43 / 0.7928 | 32 / 32 |
| rec2: cells hear only pixels | 0.58 / 0.9318 | 0.59 / 0.9527 | 0.57 / 0.7103 | 0.46 / 0.7564 | 33 / 33 |
| noinh: no inhibition (gain 0) | 0.64 / 0.9193 | 0.65 / 0.9527 | 0.57 / 0.6620 | 0.46 / 0.7437 | 37 / 36 |
| inhfast: inhibition with no delay | 0.54 / 0.9229 | 0.61 / 0.9680 | 0.52 / 0.6928 | 0.46 / 0.7901 | 33 / 33 |
| freeze: thresholds and sector bias frozen after settling | 0.66 / 0.9387 | 0.79 / 0.9685 | 0.66 / 0.6844 | 0.66 / 0.7995 | 28 / 40 |
| freezeth: thresholds frozen after settling | 0.62 / 0.9330 | 0.74 / 0.9702 | 0.60 / 0.6946 | 0.55 / 0.7982 | 33 / 33 |
| freezebias: sector bias frozen after settling | 0.63 / 0.9302 | 0.70 / 0.9693 | 0.58 / 0.6842 | 0.47 / 0.7849 | 30 / 32 |
| reset: potentials and cooldowns cleared every digit | 0.55 / 0.9284 | 0.61 / 0.9671 | 0.54 / 0.6907 | 0.43 / 0.7924 | 33 / 32 |
| resetring: and the spike ring cleared too | 0.57 / 0.9322 | 0.63 / 0.9703 | 0.55 / 0.6898 | 0.44 / 0.7994 | 33 / 32 |
| gap100: 100 quiet ticks after every digit | 0.45 / 0.9221 | 0.54 / 0.9681 | 0.43 / 0.6525 | 0.33 / 0.7822 | 63 / 62 |
| paceoff: pacemakers off | 0.54 / 0.9258 | 0.61 / 0.9657 | 0.52 / 0.6776 | 0.46 / 0.7869 | 33 / 33 |
| noiseoff: noise off | 0.55 / 0.9266 | 0.63 / 0.9655 | 0.54 / 0.6800 | 0.49 / 0.7870 | 33 / 33 |
| quiet: pacemakers and noise off | 0.55 / 0.9263 | 0.63 / 0.9655 | 0.54 / 0.6867 | 0.49 / 0.7866 | 33 / 33 |
| delay1: every delay 1 | 0.55 / 0.9301 | 0.62 / 0.9687 | 0.53 / 0.6907 | 0.46 / 0.7977 | 33 / 33 |
| show40: digit shown 40 ticks | 0.63 / 0.9413 | 0.67 / 0.9725 | 0.62 / 0.7140 | 0.55 / 0.8153 | 33 / 33 |
| show80: digit shown 80 ticks | 0.69 / 0.9462 | 0.70 / 0.9744 | 0.69 / 0.7222 | 0.61 / 0.8281 | 33 / 33 |
| last10: only the last 10 of 20 ticks counted | 0.46 / 0.9238 | 0.56 / 0.9681 | 0.45 / 0.6776 | 0.42 / 0.7892 | 35 / 35 |
| every2: a pixel line fires every second tick | 0.37 / 0.9091 | 0.50 / 0.9617 | 0.36 / 0.6510 | 0.35 / 0.7619 | 32 / 31 |
| target16: target rate half (16 per 1,024) | 0.53 / 0.9282 | 0.63 / 0.9667 | 0.51 / 0.6935 | 0.45 / 0.7860 | 19 / 19 |
| target8: target rate a quarter (8 per 1,024) | 0.49 / 0.9262 | 0.63 / 0.9645 | 0.47 / 0.6859 | 0.44 / 0.7743 | 12 / 12 |
| wide: wider cells (`wire_pixel_shift 2`; transplant: the model trained on the wide sheet) | 0.69 / 0.9378 | 0.65 / 0.9671 | 0.66 / 0.6755 | 0.41 / 0.7724 | 56 / 55 |
| *added* thregoff: no per-cell threshold regulation, from birth | 0.60 / 0.9398 | 0.70 / 0.9659 | 0.57 / 0.6985 | 0.48 / 0.7889 | 59 / 58 |
| *added* thregoff32: the same, sector bias up to 32 | 0.56 / 0.9368 | 0.67 / 0.9672 | 0.54 / 0.6886 | 0.45 / 0.7833 | 39 / 39 |
| *added* slowreg: threshold regulation 8 times slower | 0.54 / 0.9233 | 0.62 / 0.9647 | 0.53 / 0.6785 | 0.47 / 0.7853 | 33 / 33 |
| *added* slowbias: sector activity smoothed over 256 ticks, not 16 | 0.56 / 0.9262 | 0.57 / 0.9638 | 0.53 / 0.6780 | 0.35 / 0.7760 | 34 / 34 |
| *added* thregoff32+slowbias | 0.52 / 0.9267 | 0.55 / 0.9632 | 0.48 / 0.6908 | 0.29 / 0.7828 | 40 / 40 |
| *added* winners2: 2 winners per tile | 0.52 / 0.9298 | 0.62 / 0.9677 | 0.52 / 0.6904 | 0.50 / 0.7908 | 32 / 32 |
| *added* still: frozen; potentials, cooldowns, ring cleared each digit; no noise, no pacemakers | 1.00 / 0.9530 | 1.00 / 0.9765 | 1.00 / 0.7356 | 1.00 / 0.8250 | 15 / 24 |
| *added* still+noise: the same with noise on | 0.74 / 0.9507 | 0.85 / 0.9733 | 0.73 / 0.7232 | 0.65 / 0.8181 | 20 / 18 |
| *added* still+noise+rec2: and cells hearing only pixels | 0.74 / 0.9536 | 0.84 / 0.9641 | 0.82 / 0.7459 | 0.75 / 0.7745 | 20 / 26 |
| *added* still+show80 | 1.00 / 0.9557 | 1.00 / 0.9762 | 1.00 / 0.7556 | 1.00 / 0.8360 | 22 / 46 |
| freeze+show80 (the best two) | 0.85 / 0.9544 | 0.92 / 0.9748 | 0.89 / 0.7481 | 0.80 / 0.8338 | 23 / 31 |
| *added* freeze+reset | 0.69 / 0.9471 | 0.83 / 0.9728 | 0.69 / 0.7132 | 0.65 / 0.8115 | 18 / 22 |
| *added* thregoff+show40 | 0.71 / 0.9503 | 0.79 / 0.9721 | 0.68 / 0.7311 | 0.61 / 0.8145 | 59 / 58 |

Why the added arms: the tick code gives every cell two slowly moving numbers besides its
wiring (its threshold and its sector's bias), so the regulation arms test whether a real
setting can do what freezing does (thregoff, thregoff32, slowreg, slowbias). Winners per tile
are the one existing mechanism that caps how many cells answer. The still arms take every
source of state and chance out at once and put them back one at a time, which is the
cleanest way to split the model-to-sheet gap; still is exactly 1.00 by construction and is
the check that nothing else moves.

Read down it:

- **Wiring dynamics (rec1, rec2, noinh, inhfast, delay1): little.** Removing recurrent
  excitation (rec1) or all recurrence (rec2) lifts the born reader by 0.003 and 0.009 on
  MNIST and 0.019 and 0.032 on pairs, but costs the transplant (its wiring uses cells hearing
  cells): rec1 -0.003 / +0.007, rec2 -0.014 / -0.029. No inhibition makes cells more
  repeatable (0.64) and reads worse everywhere. Inhibition with no delay: +0.000 on MNIST,
  +0.014 / +0.004 on pairs. Every delay 1: +0.007 / +0.012 born.
- **Regulation (freeze, freezeth, freezebias): the largest single diagnostic.** Thresholds
  and bias each carry about half. Eight times slower threshold regulation does nothing
  (0.54, 0.9233): it slows the walk but leaves its size, as predicted. No per-cell threshold
  regulation from birth lifts the born MNIST reader (0.940) but at 1.8 times the target
  rate, because the sector bias reaches its cap of 16; with the cap at 32 the rate is 39 and
  the reader 0.937. A slower sector bias does not help (0.926) and costs the pair transplant.
- **Carry-over (reset, resetring): small alone, larger with regulation frozen.** Potentials
  and cooldowns cleared: +0.006 / +0.012; the ring too: +0.009 / +0.011. Frozen and cleared
  (freeze+reset): 0.947 / 0.713, more than the two apart.
- **Noise and pacemakers: almost nothing** on the reader (-0.001 to +0.008).
- **Counting (show40, show80, last10): the largest real lever.** 40 ticks: 0.941 / 0.714; 80:
  0.946 / 0.722; the transplant 0.974 / 0.828 at 80. Counting only the last 10 of 20 ticks
  loses repeatability and gains nothing: the start of a digit is not the noisy part.
- **Sparser input (every2): worse.** A pixel line firing every second tick halves the drive;
  the sheet becomes less repeatable (0.37) and reads worse (0.909). Saturating drive is not
  the source of the chaos.
- **Winners per tile (2): +0.007 / +0.012 born**, nothing on the transplant.

## The arms the conclusion rests on (8 seeds)

Mean (lowest to highest seed); `scripts/repeat.sh arms --arms base,freeze,show80,freeze+show80,still
--seeds 5-12 --table-only`. Seeds 5-7 of the transplant are idea 165's wirings; seeds 8-12
were trained again with idea 165's model (`scripts/repeat.sh wiring`, on stevejobs; the model
scores 0.980 / 0.854 to 0.866, as idea 165's did), because those files were not kept. The base
transplant reads 0.9664 / 0.7831 with them (idea 165: 0.9663 / 0.7853).

| arm | kind | MNIST self-corr | MNIST reader | pairs self-corr | pairs reader | rate M / P |
|---|---|---|---|---|---|---|
| base | born | 0.541 | 0.9241 (0.9221-0.9267) | 0.525 | 0.6728 (0.6635-0.6801) | 33 / 33 |
| base | transplant | 0.617 | 0.9664 (0.9643-0.9673) | 0.462 | 0.7831 (0.7737-0.7962) | 35 / 32 |
| freeze | born | 0.679 | 0.9376 (0.9349-0.9395) | 0.655 | 0.6938 (0.6844-0.6979) | 36 / 39 |
| freeze | transplant | 0.797 | 0.9688 (0.9676-0.9720) | 0.663 | 0.7927 (0.7815-0.8020) | 34 / 57 |
| show80 | born | 0.695 | 0.9453 (0.9433-0.9484) | 0.690 | 0.7168 (0.7079-0.7222) | 34 / 33 |
| show80 | transplant | 0.712 | 0.9742 (0.9727-0.9753) | 0.608 | 0.8206 (0.8060-0.8329) | 36 / 32 |
| **freeze+show80** | born | **0.878** | **0.9535 (0.9520-0.9552)** | **0.874** | **0.7516 (0.7478-0.7595)** | 39 / 39 |
| **freeze+show80** | transplant | **0.932** | **0.9752 (0.9739-0.9765)** | **0.837** | **0.8259 (0.8116-0.8353)** | 42 / 55 |
| still | born | 1.000 | 0.9519 (0.9502-0.9530) | 1.000 | 0.7348 (0.7299-0.7456) | 24 / 28 |
| still | transplant | 1.000 | 0.9748 (0.9732-0.9765) | 1.000 | 0.8241 (0.8153-0.8368) | 26 / 33 |

Paired by seed, against base: freeze +0.014 / +0.021 born, +0.002 / +0.010 transplanted;
shown 80 ticks +0.021 / +0.044 born, +0.008 / +0.038 transplanted; both +0.029 / +0.079 born,
+0.009 / +0.043 transplanted; held still +0.028 / +0.062 born, +0.008 / +0.041 transplanted.
Every one is above base on every seed except freeze on the pair transplant (one seed
-0.001). Frozen and shown 80 ticks against shown 80 ticks alone: +0.008 / +0.035 born, +0.001
/ +0.005 transplanted. The born pair test gains most from both together: 0.752, past the
held-still sheet (0.735).

## The cost side: the time rows

The rows that need state, on the one setting (`best`, docs/results-2026-11-26-one-setting.md,
which passes all six), with the change on, seeds 5-12, each life beside its learning-off twin
(`python3 scripts/repeat.py time`; the best column is the cached one). Freezing regulation and
clearing carry-over have no form a life can use, so the two changes run are the real ones:
every step shown 80 ticks (`--ticks 80`, the counting lever) and no per-cell threshold
regulation with the sector bias to 32 (the regulation lever).

| row | best | best, every step 80 ticks | best, no threshold regulation, bias to 32 |
|---|---|---|---|
| far5 | 0.933 | 0.876* (-0.057 ±0.033) | 0.953* (+0.020 ±0.006) |
| xor | 0.936 | 0.922* (-0.014 ±0.023) | 0.821* (-0.115 ±0.016) |
| order | 0.994 | **0.493 (-0.501 ±0.033)** | 0.986* (-0.009 ±0.007) |
| gap | 0.897 | 0.779* (-0.118 ±0.081) | 0.900* (+0.003 ±0.014) |
| delayed2 | 1.000 | 1.000* (+0.000 ±0.000) | 1.000* (+0.000 ±0.000) |
| copy2 | 0.897 | 0.745* (-0.153 ±0.042) | 0.895* (-0.003 ±0.008) |

`*` passes (halfway from chance to perfect and above its twin by 2 standard errors); in
brackets the paired difference to best and its standard error.

- **Every step 80 ticks long kills order (0.994 to 0.493, chance) and costs copy (-0.153) and
  the gap row (-0.118); delayed echo holds.** A step four times as long asks the sheet to
  carry its state four times as many ticks. So counting longer is a way to read a sheet on
  an input that holds still, not a setting for a life: a finding, not a fix. Where an input
  holds still (a digit) it pays; where the answer is in what changed between steps it does
  not, unless the time cells' scales move with it (not tried).
- **No per-cell threshold regulation keeps every time row** (order -0.009, gap +0.003,
  delayed echo 0, copy -0.003, far5 +0.020) **and costs exclusive-or 0.115.** It is the one
  engine change here that the time rows do not mind, and it gives the born MNIST reader
  +0.014 and pairs +0.010 (seed 5); the exclusive-or loss rules it out as a default.

## Hardware cost

What I recommend changes how the sheet is read, not the cell: count a digit over 80 ticks
instead of 20, and hold regulation still while a reader's counts are being taken. Holding
regulation still adds no bits (one flag for the whole sheet) and removes work: no threshold
step (two random words per tile and a walk over their set bits) and no sector-bias step per
tick, about 2% and 6% of a tick at 4,096 cells by `blobbench` here (68,700 ticks/s; 70,300
without threshold regulation; 72,800 without either; rough, this PC is temperature-pinned).
Counting longer adds no bits to a cell (a reader's count of a cell over 80 ticks still fits in
a byte: with a cooldown of at least one tick a cell fires at most 40 times) and no work per
tick, but every answer takes four times the ticks, so four times the work per digit; 40 ticks
gets about two thirds of the gain for twice the work (0.941 / 0.714 born, seed 5). The engine
setting that does part of the same (no per-cell threshold regulation, sector bias to 32) saves
the threshold step and would let a cell's threshold be its born 4 to 12 in 4 bits instead of a
16-bit word, but costs the best setting exclusive-or 0.115; I do not recommend it as a default.

## What is real, what is suspect

- Real (8 seeds, every seed): reading longer and regulation held still during the read both
  lift the reader on both tests, born and transplanted, and the held-still sheet comes within
  0.01 of idea 165's model on MNIST. The wiring's own dynamics (recurrence, inhibition's
  delay, delays, pacemakers) explain little: seed 5 only, but every one of those arms moves
  the born MNIST reader by less than 0.01.
- Seed 5 only: every arm in the big table except the five above. Steps of 0.005 or less there
  are within the seed spread (0.002 to 0.005 on MNIST, 0.01 to 0.02 on pairs for the 8-seed
  arms) and are called nothing.
- Suspect: self-correlation is a correlation of mostly 0 and 1 counts; it moves with the firing
  rate and with the measure (one sheet twice against two sheets, above). The reader is the
  number to trust; self-correlation says where to look.
- Suspect: arms that change the firing rate (thregoff 59, wide 56, gap100 63, target16 19,
  target8 12, still 15 to 28) mix "more repeatable" with "more or fewer spikes". Freeze and
  show80 keep the rate within 33 to 42 on MNIST; freeze runs at 39 to 57 on the pair test,
  because the bias freezes wherever settling left it.
- The transplant wirings of seeds 8-12 are retrained, not idea 165's own files (above).
- Freezing is a snapshot of a random walk: two sheets frozen at different moments differ for
  the whole read (the two-sheet check). For a reader within one sheet that does not matter;
  for a rule that learns across a life it would.
- "Inhibition with no delay" is done in the MNIST harness, not the engine: before every tick
  the older maps of the spike ring take the newest map's inhibitory spikes. Learning is off
  while the sheet is read and nothing else reads old maps then, so it is exact for the read,
  but it is not a setting a life can use. A genome switch for it in `fire.cpp` cost 9% to 18%
  of tick speed with the switch off (`blobbench`, 4,096 cells: 66,000 to 69,000 ticks/s
  before, 55,000 to 61,000 after, two forms tried), so it was taken out.

## Not done

- A spiking version of idea 165's model, for what is left (0.004 to 0.018, seed 5, with the
  sheet held still and shown 80 ticks).
- A teacher trained at a lower rate: only the real sheet's regulation was changed, with the
  transplant trained at the engine's rate.
- The wiring-dynamics and lever arms on 8 seeds.
- The two changes on the base setting's time rows: only the best setting, whose time rows
  pass, was run.
- A local rule that learns with regulation held still over its counted window, or counts over
  a longer window: the obvious next test for every inside rule (below).

## Script lines

Outputs in `out/repeat/NAME/`, one file a run, kept; a line run again only runs what is
missing.

```
scripts/repeat.sh arms                                                   every arm, seed 5, both tests, born and transplant
scripts/repeat.sh arms --arms base,freeze,show80,freeze+show80,still --seeds 6-12 --kinds born
scripts/repeat.sh arms --arms base,freeze,show80,freeze+show80,still --seeds 6-12 --kinds transplant
scripts/repeat.sh arms --arms base,freeze,show80,freeze+show80,still --seeds 5-12 --table-only
LADDER_VENV=... scripts/repeat.sh wiring arms --seeds 8-12              transplant wirings for seeds 8-12 (stevejobs)
LADDER_VENV=... scripts/repeat.sh wide arms                             the wide cells' wiring (stevejobs)
scripts/repeat.sh check arms --arms base,freeze --tasks mnist,pair       idea 165's two-sheet measure (stevejobs)
scripts/repeat.sh check arms --arms still,freeze+show80 --kinds born --tasks mnist,pair
scripts/oneset.sh rep-best-show80 --rows order,gap,delayed2,copy2,xor,far5 --seeds 5-12 BEST --ticks 80
scripts/oneset.sh rep-best-thregoff32 --rows order,gap,delayed2,copy2,xor,far5 --seeds 5-12 BEST --g.threshold_reg_on 0 --g.sector_bias_max 32
python3 scripts/repeat.py time                                           the time-row table above
scripts/repeat.sh smoke --quick                                          two arms on few digits, end to end
```

BEST is the `best:` line of `scripts/variants/best.txt`. On stevejobs the same lines ran with
`THONK_MNIST`, `LADDER_VENV` and `REPEAT_WIRING` set (`MINI_DIR=thonk-repeat`); the
thregoff32 time rows ran there (its cache is `out/oneset/cache-repeat-mini.jsonl` in this
worktree).

## Code

- `src/mnist/mnist_repeat.cpp` (new): the probes of `blobmnist ceiling`, every one off unless
  its flag is given: `--twice`, `--rec-cut`, `--delays-one`, `--pace-off`, `--read-freeze`,
  `--reset-digit`, `--input-every`, `--count-from`, `--inh-fast`.
- `src/mnist/mnist_ceiling.cpp` (small: the flag names and three calls),
  `src/mnist/mnist_sheet.cpp` (the per-digit and per-tick hooks, each behind a field that is
  0 by default), `src/mnist/mnist_run.hpp` (the fields and declarations).
- Nothing in `src/blob/`, `src/run/` or `src/world/`. Clean build, no new warnings; the four
  reference lives byte-identical (`out/repeat/refcheck.sh`, a copy of
  `out/from-agents/hop/refcheck.sh` pointed at the main checkout's `out/merge-ref/`).
- `scripts/repeat.sh`, `scripts/repeat.py`.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, new finding: **the sheet's cells answer a digit differently each time
  mostly because regulation moves during the read and because a digit's 20 ticks hold few
  spikes; recurrence, inhibition's delay, delays, noise and pacemakers explain little.** Held
  still (regulation frozen, carry-over cleared, no noise or pacemakers) the born sheet reads
  0.952 / 0.735 against idea 165's noise-free model's 0.961 / 0.768; regulation frozen and
  the digit shown 80 ticks reads 0.954 / 0.752 born, 0.975 / 0.826 transplanted. [likely: 8
  seeds, both tests]
- `docs/findings.md`, idea 165's new finding ("0.6 like themselves from another starting
  state"): add the measure. 0.61 is two separate sheets; one sheet read twice is 0.54; with
  regulation frozen the two measures go opposite ways. "About two fifths of what a cell does
  is state" stands, and the state is mostly the regulation's. [likely]
- `docs/findings.md`, idea 165's "the best code is sparse and per-cell regulation forbids
  it": in the real sheet a lower per-cell target does not help (born +0.005 / +0.015 at half,
  transplant +0.000) and wider cells do not help the transplant (0.967 / 0.772). [hint: seed 5]
- `docs/findings.md`: a quiet gap between inputs makes the sheet less repeatable (regulation
  lowers thresholds in the quiet; the next input bursts at twice the rate). [hint: seed 5]
- `docs/plan.md`: item 2h done. Candidates for the queue, in order: (1) decide whether the
  inside-learning test reads with regulation frozen and 80-tick digits; if so every yardstick
  moves (born 0.954 / 0.752; transplant 0.975 / 0.826) and the competitive-learning and hop
  results should be read again the same way before they are compared; (2) the inside rules
  learn from per-digit spike counts that are mostly regulation's noise: competitive learning
  and the one-hop rule with regulation held still during the counted window, or with the
  threshold step only between inputs; (3) no per-cell threshold regulation on the whole
  quick map, to see where else it costs besides exclusive-or.
- `docs/ideas.md`, new: (a) regulation on a slower clock that steps only between inputs (the
  real-engine form of "frozen during the read"; cost: one gate per tick); (b) a threshold with
  2 hidden fraction bits, stepped by quarters: by the reasoning in the predictions a smaller
  step shrinks the walk, which a slower step does not (slowreg changed nothing); (c) a reader
  of longer windows as the plug's default for real data.
- `docs/notes-world.md`: `blobmnist ceiling --twice | --rec-cut | --delays-one | --pace-off |
  --read-freeze | --reset-digit | --input-every | --count-from | --inh-fast`;
  `scripts/repeat.sh`.
- `docs/predictions.md`: the predictions above and their scores (below).

## Predictions, scored

| prediction | outcome |
|---|---|
| my one-sheet measure within 0.03 of 0.61 | 0.54: wrong (the two measures differ; above) |
| rec1 0.70; rec2 (pixels only) 0.85, the largest of all | 0.52; 0.58: wrong, the worst miss |
| noinh 0.55; inhfast 0.64 | 0.64; 0.54: wrong both ways |
| freeze 0.75, reader 0.935 / 0.70, the largest of the arms that keep the sheet whole (0.6) | 0.66 (0.68 on 8 seeds), reader 0.939 / 0.684 (8 seeds 0.938 / 0.694): the largest of the engine-state arms, but showing longer matched it (0.69) |
| freezeth 0.71; freezebias 0.65 | 0.62; 0.63 |
| reset 0.66, reader 0.928 / 0.68; resetring 0.70 | 0.55, 0.928 / 0.691; 0.57 |
| gap100 0.64, reader 0.928 / 0.68 | 0.45, 0.922 / 0.653: wrong direction |
| paceoff, noiseoff, both 0.62, 0.62, 0.63 | 0.54, 0.55, 0.55: small, as guessed |
| delay1 0.63, reader 0.92 / 0.67 | 0.55, 0.930 / 0.691 |
| show40 0.70, 0.935 / 0.69; show80 0.78, 0.94 / 0.70 | 0.63, 0.941 / 0.714; 0.69, 0.946 / 0.722: readers better than guessed |
| last10 0.60, 0.92 / 0.67 | 0.46, 0.924 / 0.678 |
| every2 0.62, 0.92 / 0.67 | 0.37, 0.909 / 0.651: wrong |
| target16 0.60, 0.920 / 0.68; target8 0.57, 0.91 / 0.68 | 0.53, 0.928 / 0.694; 0.49, 0.926 / 0.686 |
| wide 0.66, 0.935 / 0.70 | 0.69, 0.938 / 0.676 |
| thregoff 0.70, 0.93 / 0.69 | 0.60, 0.940 / 0.699 |
| slowreg 0.63 (slows the walk, same spread) | 0.54, reader unchanged: right that it does nothing |
| winners2 0.65 | 0.52 |
| the transplant moves the same way, a little less | self-correlation moves more on the transplant, the reader less: half right |
| no single change lifts the born MNIST reader by more than 0.015 (0.7) | showing 80 ticks +0.021: wrong; freeze +0.014 |
| pairs move more than MNIST for every arm that lifts self-correlation (0.6) | for freeze, show40, show80, still: yes; for wide and no inhibition: no |
| no threshold regulation keeps delayed echo and order within 0.05 (0.6) | order, gap, delayed echo, copy within 0.01 of best; exclusive-or (not a time row) -0.115: right on the time rows |

I expected the wiring's dynamics (recurrence, the strong delayed inhibition) to be the chaos;
they are not. The regulation I named second is most of it, and counting longer, which I rated
as a modest lever, is the largest real one.
