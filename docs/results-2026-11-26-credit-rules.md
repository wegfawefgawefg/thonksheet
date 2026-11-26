# The credit rules not yet tried, on the rule's ladder's bench (2026-11-26)

Queue item 6, idea 177 (ideas 91, 138, 139). **No rule here uses backprop.** Backprop appears
only as a diagnostic: how often a rule's teaching bit has the sign backprop's would have.

**The answer: yes, one. Paying backward along the output's own read wires (each line in error
pays or charges the fired cells it reads, by the sign of its read weight) reads 0.965 on
MNIST and 0.770 on the pair test written into the real sheet, against the one-hop
random-wire rule's 0.961 and 0.758 at the same firing rate (8 seeds: +0.004, 8 of 8;
+0.012, 7 of 8), and what it needs that the one-hop rule does not is a way back along every
read wire that carries the sign of that wire's weight. Nothing else beats the one-hop rule:
passing the payment one step further reads higher on pairs (0.789) only by firing half
again as much (needs each cell's own input slots used backward); two phases read 0.953 /
0.726 (need the digit shown twice); wires the cell tunes itself 0.955 / 0.765 (need ten
small counters a cell); a target from the answer alone reads under born, 0.916 / 0.610
(needs less than one hop: no output); credit by code wrecks the sheet, 0.773 / 0.641 (needs
no wires, and by how often its bit has backprop's sign about a hundred lives for payment's
one); and a mark that lasts two showings lets the one-hop rule learn from an error one
showing late at no loss (0.960 / 0.756), a mark of four showings for 0.007 / 0.009.**

(MNIST / pairs throughout. Born 0.924 / 0.673. Every number here is a wiring made in the
model and written into the real sheet unless it says "model". 8 seeds, 5-12, for born, T7x
and the rules near it; 3 seeds, 5-7, for the rest.)

## Predictions (written before any scoring run was read)

Seen before writing these: the three docs this builds on and their numbers (in the real
sheet: born 0.924 / 0.673, T7x 0.961 / 0.761, competitive learning 0.952 / 0.726, the
backprop teacher's wiring 0.966 / 0.785). From this job only mechanics runs
(`out/credit/dev/mech.py`: lives of 1,280 and 2,560 digits on seed 5, no reader, no real
sheet), which print for each arm the share of cell-digits taught, the share of "more" among
them, counter steps a digit, the largest counter, born inputs changed, and how often the
teaching bit has backprop's sign. What they showed:

- Agreement with backprop's sign in the first 1,280 digits: the one-hop rule 0.51, two
  phases 0.50 to 0.51, code with every cell perturbed 0.507, with 192 cells 0.53, with 12
  cells 0.63, payment 0.87 (MNIST; 1.00 on pairs, where the two read weights mirror each
  other), targets from the answer 0.52, tuned wires 0.51 to 0.52.
- Two phases with every spike an event: at a gain of 1/4 and above, nine "more" in ten and a
  largest counter of 8,000 after 2,560 digits; it will pass 16 bits. Its counters were given
  24 bits and the gain grid moved down to 1/16, 1/4, 1.
- A run takes 2 to 5 minutes here.

`scripts/credit.sh smoke --quick` had been started; its output was not read before this was
committed.

Guesses, the wiring written into the real sheet, seeds 5-7, MNIST / pairs:

| arm | guess | note |
|---|---|---|
| T7x again | 0.961 / 0.761 | the ladder's numbers; its coin 0.928 / 0.675 |
| hop (T7x through this bench's loop) | the same model reader as T7x to four places on seed 5: 0.8 | the check |
| P2s two phases, sign of the difference | 0.955 / 0.745 | the one-hop error, found by the cell's own counts |
| P2 two phases, every spike an event | 0.950 / 0.735 | runs to "more"; the pick will take the smallest gain |
| C2 code, every cell | 0.925 / 0.675 | within 0.003 of its coin on both |
| C2n192, C2n12 | 0.928 / 0.680, 0.927 / 0.678 | agreement over the life 0.51, 0.53, 0.62 for all, 192, 12 |
| C1 code, good or bad only | its coin within 0.003 | |
| pay1 the output pays the cells that fired | 0.960 / 0.755 | |
| pay passed back once more | 0.961 / 0.760 | within 0.003 of pay1 |
| payc passed back to one source | pay within 0.003 | |
| tgt a target from the answer | 0.945 / 0.690 | no cell can be a "same" detector |
| dtp wires the cell tuned itself | 0.958 / 0.750 | |
| hopb, dtpb (the bit passed back) | their rule within 0.003 | |
| d1n error one showing late, no trace | 0.928 / 0.675 | its coin's level |
| d1t2 marks last 2 showings | 0.955 / 0.745 | half the marks belong to another digit |
| d1t4, d3t4 marks last 4 | 0.948 / 0.725 both | the delay costs nothing inside the mark's life, the blur does |
| d0t4 error on time, marks last 4 | 0.950 / 0.730 | |

- **Some rule beats T7x in the real sheet on both tests** (seed by seed, mean above zero and
  at least two of three seeds on each test): 0.25. The likeliest: pay, hopb or dtp.
- pay or pay1 beats T7x on MNIST by 0.002 or more: 0.3; on pairs by 0.005 or more: 0.25.
- dtp beats T7x on both: 0.2. P2s beats T7x on both: 0.1.
- Credit by code beats its coin by 0.005 on either test at any number of cells perturbed:
  0.1.
- tgt beats born on MNIST: 0.85; on pairs: 0.5.
- The firing rate written in: every rule that learns sits between 33 and 46 spikes per
  1,000 cell-ticks (T7x: 43 on MNIST, 34 on pairs).

Which arms get 8 seeds (5-12), fixed now: born, T7x and its coin, and every rule that on
seeds 5-7 reads in the sheet within 0.005 of T7x or above it on either test, with its coin.

Seen after the predictions and before the report runs: the smoke run (one seed, 10,000
digits) and the two-phase picks. Added after the seeds 5-7 table had been read: the arm
`still` (nothing learns), the read at matched firing rate, the arms `hopf` and `payn` that
take payment's lead apart, coins for the two smaller code arms, and a corrected version of
the time arms (the first is under "suspect").

## Short version

- **Payment along the read's own wires beats the one-hop rule, and it is the wires that do
  it.** pay1 0.9649 / 0.7704 against T7x 0.9610 / 0.7583: +0.0039 on MNIST (8 of 8 seeds;
  +0.0016 to +0.0060) and +0.0121 on pairs (7 of 8; -0.0008 to +0.0229), at the same firing
  rate (41 / 33 spikes per 1,000 cell-ticks against 43 / 34) and the same pixels a cell,
  teaching half as many cell-digits (3.6% / 4.5% against 7.3% / 15%). With every sheet held
  at its target rate: +0.0031 (7 of 8) and +0.0138 (8 of 8). Taken apart (3 seeds): T7x
  gated by "fired" alone, as payment is, reads T7x's numbers (0.960 / 0.762); the read
  wires' signs under T7x's own gate read payment's (0.964 / 0.770). Payment's bit has
  backprop's sign nine times in ten, the random wires' six in ten. It lands 0.0014 under
  the backprop teacher's wiring on MNIST (0.9663) and 0.015 under on pairs (0.7853).
- **Passing the bit back one more step buys firing, not credit.** As fired it reads 0.9635
  / 0.7888 (payment), 0.9650 / 0.7893 (conserved), 0.9614 / 0.7917 (the one-hop bit): +0.03
  over T7x on pairs on every seed. But those sheets fire at 44 to 62 (T7x 34 to 43), their
  cells take 8 to 13 pixels instead of 6 or 7, and their coins gain as much: every rule's
  lead over its own coin on pairs is +0.086 to +0.087, pass-back or not. At matched rate
  payment passed back reads 0.952 / 0.768, no better than pay1 on pairs and 0.007 under it
  on MNIST; for the random-wire rule the pass-back is worth +0.012 on pairs and costs
  0.009 on MNIST.
- **Two phases learn less than one hop, and mostly what their coin learns.** 0.953 / 0.726:
  over born by +0.029 / +0.052, under T7x on every seed (-0.007 / -0.033), over its own
  coin by +0.007 / +0.022. Every spike as an event reads 0.931 / 0.686.
- **Wires the cell tunes itself are random wires.** 0.955 / 0.765: -0.006 on MNIST (0 of
  8), +0.006 on pairs (7 of 8), the same at matched rate. Their bit has backprop's sign
  0.59 / 0.55 of the time; the random wires' 0.60 / 0.56.
- **A target from the answer alone unlearns.** 0.916 / 0.610, under born on every seed.
- **Credit by code wrecks the sheet; with few cells perturbed it only stops wrecking.**
  All cells 0.773 / 0.641; 192 a digit 0.915 / 0.714 (coin 0.914 / 0.715); 12 a digit 0.948
  / 0.717 (coin 0.946 / 0.710). Good or bad only: 0.849 / 0.650, under its coin.
- **Time: a mark that lasts covers a late error.** Error one showing late, no trace: born
  (0.923 / 0.676). Marks of two showings: T7x's numbers (0.960 / 0.756; -0.0009 / -0.0025).
  Marks of four showings cost 0.006 to 0.008 / 0.006 to 0.012 whether the error is 0, 1 or
  3 showings late: the price is the other showings' marks, not the delay.
- **Coins are not at born.** A rule that teaches few cell-digits reads above born even on
  another digit's signal (two phases' coin +0.022 / +0.030, tuned wires' +0.011 / +0.020,
  the 12-cell code's +0.021 / +0.035); one that teaches many reads under (T7x's coin -0.005
  / -0.002). Writing the born wiring out through the bench changes nothing (0.926 /
  0.673). So "over born" flatters the gentle rules; "over its coin" is the margin to read.

## The table: written into the real sheet

Test digits; mean over seeds; "coin": the same rule on another digit's signal; "minus T7x":
seed by seed (seeds above); "rate": hidden spikes per 1,000 cell-ticks while read (born 33 /
33). MNIST / pairs in every cell. The model's own reader is in the last column and is not
used to rank. Raw: `out/credit/full/table.txt` (all seeds), `table-seeds5-7.txt`.

| rule | seeds | real sheet | coin | minus T7x | rate | needs beyond the one-hop rule | model |
|---|---|---|---|---|---|---|---|
| born | 8 | 0.9241 / 0.6728 | | | 33 / 33 | | 0.960 / 0.764 |
| still (nothing learns, written out) | 3 | 0.926 / 0.673 | | | 33 / 33 | | 0.960 / 0.764 |
| **T7x one hop, random wires** | 8 | **0.9610 / 0.7583** | 0.919 / 0.671 | | 43 / 34 | | 0.969 / 0.812 |
| **pay1 the output pays the cells that fired** | 8 | **0.9649 / 0.7704** | 0.926 / 0.684 | **+0.0039 (8/8) / +0.0121 (7/8)** | 41 / 33 | the read wires usable backward, carrying each weight's sign; a tally per cell | 0.974 / 0.821 |
| pay passed back once more | 8 | 0.9635 / 0.7888 | 0.940 / 0.702 | +0.0025 (7/8) / +0.0305 (8/8) | 55 / 48 | pay1's, and every cell's input slots usable backward | 0.973 / 0.825 |
| payc passed back to one source | 8 | 0.9650 / 0.7893 | | +0.0040 (8/8) / +0.0310 (8/8) | 48 / 44 | pay's, and a random word a paying cell | 0.974 / 0.829 |
| hopb T7x, then passed back | 8 | 0.9614 / 0.7917 | 0.934 / 0.704 | +0.0004 (4/8) / +0.0334 (8/8) | 62 / 54 | every cell's input slots usable backward | 0.970 / 0.820 |
| dtp wires the cell tunes itself | 8 | 0.9549 / 0.7645 | 0.935 / 0.693 | -0.0061 (0/8) / +0.0062 (7/8) | 46 / 35 | an 8-bit counter per answer line per cell; every line to every cell | 0.965 / 0.809 |
| dtpb dtp, then passed back | 8 | 0.9524 / 0.7826 | 0.944 / 0.709 | -0.0086 (0/8) / +0.0242 (8/8) | 63 / 52 | both | 0.959 / 0.816 |
| payn read wires' signs, T7x's gate (added) | 3 | 0.964 / 0.770 | | +0.0036 (3/3) / +0.0117 (3/3) | 40 / 33 | as pay1 | 0.973 / 0.818 |
| hopf T7x gated by fired alone (added) | 3 | 0.960 / 0.762 | | +0.0001 (1/3) / +0.0032 (2/3) | 44 / 35 | nothing | 0.970 / 0.810 |
| P2s two phases, sign | 3 | 0.953 / 0.726 | 0.946 / 0.704 | -0.0069 (0/3) / -0.0327 (0/3) | 38 / 35 | the digit shown twice, the answer held on the lines; the lines' activity in the cell's drive every tick; 3 bits a cell | 0.966 / 0.788 |
| P2 two phases, every spike | 3 | 0.931 / 0.686 | 0.914 / 0.662 | -0.0288 (0/3) / -0.0727 (0/3) | 33 / 32 | as P2s without the 3 bits; a counter step for every spike in both phases | 0.962 / 0.779 |
| tgt a target from the answer | 3 | 0.916 / 0.610 | 0.724 / 0.607 | -0.044 (0/3) / -0.148 (0/3) | 32 / 31 | less: no output at all | 0.957 / 0.709 |
| C2 code, every cell, two runs | 3 | 0.773 / 0.641 | 0.712 / 0.628 | -0.188 (0/3) / -0.118 (0/3) | 32 / 33 | no wires; a second run; one bit for the whole sheet; two random words a tile | 0.788 / 0.711 |
| C2n192 code, 192 cells a digit | 3 | 0.915 / 0.714 | 0.914 / 0.715 | -0.046 (0/3) / -0.045 (0/3) | 34 / 35 | as C2 | 0.928 / 0.778 |
| C2n12 code, 12 cells a digit | 3 | 0.948 / 0.717 | 0.946 / 0.710 | -0.012 (0/3) / -0.042 (0/3) | 37 / 35 | as C2 | 0.966 / 0.777 |
| C1 code, one run, good or bad | 3 | 0.849 / 0.650 | 0.881 / 0.652 | -0.111 (0/3) / -0.108 (0/3) | 50 / 54 | no wires, no second run | 0.873 / 0.711 |
| e1n error 1 showing late, no trace | 8 | 0.9232 / 0.6761 | | -0.0378 (0/8) / -0.0823 (0/8) | 36 / 34 | nothing | 0.951 / 0.759 |
| e1t2 error 1 late, marks last 2 | 8 | 0.9601 / 0.7559 | | -0.0009 (3/8) / -0.0025 (3/8) | 42 / 34 | a second mark word a slot | 0.970 / 0.808 |
| e3t4 error 3 late, marks last 4 | 8 | 0.9543 / 0.7490 | | -0.0067 (0/8) / -0.0093 (1/8) | 40 / 34 | four mark words a slot, or a 2-bit count-down a source | 0.966 / 0.802 |
| e1t4 error 1 late, marks last 4 | 3 | 0.952 / 0.746 | | -0.0083 (0/3) / -0.0122 (0/3) | 41 / 34 | as e3t4 | 0.967 / 0.797 |
| e0t4 error on time, marks last 4 | 3 | 0.954 / 0.752 | | -0.0062 (0/3) / -0.0059 (0/3) | 40 / 34 | as e3t4 | 0.967 / 0.802 |

Beside them, from their own docs (8 seeds): the backprop teacher's wiring 0.9663 / 0.7853 at
34 / 32; competitive learning 0.952 / 0.726 at about 65; the ladder's own T7x 0.9607 /
0.7607 (re-run here 0.9610 / 0.7583; torch on the two machines is not bit-identical).

Each rule over its own coin, seed by seed (the label's part of what it learned): T7x +0.042
/ +0.087, pay1 +0.039 / +0.086, pay +0.023 / +0.087, hopb +0.027 / +0.087, dtp +0.020 /
+0.072, dtpb +0.008 / +0.074, P2s +0.007 / +0.022, P2 +0.017 / +0.024, C2n12 +0.003 /
+0.007, C2n192 +0.001 / -0.001, C1 -0.032 / -0.002; every seed above for all but the last
three. (tgt's +0.19 on MNIST and C2's +0.06 are a coin that wrecks the sheet worse.)

**At matched rate** (added: the same wirings read with `--g.threshold_cap 1000`, so
regulation can hold every sheet at its target; all read at 31 spikes per 1,000 cell-ticks;
`out/credit/full/table-cap.txt`):

| rule | seeds | real sheet at matched rate | coin | minus T7x |
|---|---|---|---|---|
| born | 8 | 0.9218 / 0.6682 | | |
| T7x | 8 | 0.9557 / 0.7530 | 0.903 / 0.667 | |
| **pay1** | 8 | **0.9588 / 0.7668** | 0.916 / 0.678 | **+0.0031 (7/8) / +0.0138 (8/8)** |
| pay | 8 | 0.9516 / 0.7682 | 0.915 / 0.687 | -0.0041 (0/8) / +0.0152 (8/8) |
| payc | 8 | 0.9555 / 0.7683 | | -0.0002 (4/8) / +0.0153 (8/8) |
| hopb | 8 | 0.9463 / 0.7652 | 0.906 / 0.684 | -0.0094 (0/8) / +0.0121 (8/8) |
| dtp | 8 | 0.9489 / 0.7591 | 0.924 / 0.688 | -0.0068 (0/8) / +0.0061 (8/8) |
| dtpb | 3 | 0.934 / 0.762 | 0.930 / 0.690 | -0.022 (0/3) / +0.008 (3/3) |
| payn | 3 | 0.958 / 0.769 | | +0.0016 (3/3) / +0.0151 (3/3) |
| hopf | 3 | 0.955 / 0.757 | | -0.0017 (0/3) / +0.0034 (2/3) |
| P2s | 3 | 0.945 / 0.720 | 0.935 / 0.695 | -0.011 (0/3) / -0.034 (0/3) |

## Why each falls where it does

1. **Two phases (idea 91).** With fixed wires from the lines, the difference between the
   two phases' drive is the wires times the output's error: the one-hop rule's signal,
   found by the cell's own count instead of sent. A silent cell near its bar is easier to
   push over than a firing cell is to hold back, so a nudge that moves many counts says
   "more" four times in five and the wiring runs away; one small enough to stay balanced
   (1/16) moves a count on 0.5% to 1.3% of cell-digits, a tenth of what T7x teaches.
2. **Credit by code (idea 138).** One bit for the whole sheet cannot say which cell (next
   section). And a spike forced into a silent cell mostly hurts, so two bits in three are
   "less", every input that is ever on runs down, and the cells end wired to pixels that
   are never on (25 pixel inputs a cell on MNIST, born 7).
3. **Payment (idea 139).** The win is where the earnings enter: the output paying along the
   wires it reads by. Cells paying cells, the bucket brigade proper, makes the cells that
   feed paid cells take more pixels and fire more, and with firing held equal adds nothing.
4. **A target for each cell.** With fixed wires and no threshold on the way back, the
   target less the cell's activity is the wires times the error: the one-hop rule again.
   A target from the answer alone keeps pushing after the output is right and asks cells
   for what they cannot detect. A way back the cell learns for itself is as good as random
   wires: a cell that fires more for "3" is not thereby one the output reads positively.

By what the signal carries: the sign of the read's own weight > a fixed random wire = the
cell's own tuning > the same error found by two phases > one outcome bit against a code.
The ladder's float rungs said the same in the model (the output's own weights 0.978
against random feedback 0.973 on MNIST); this is that step in integer counters, in the
real sheet.

## Scaling of credit by code

One outcome bit for the whole sheet a digit; n cells perturbed at once (a forced or vetoed
spike each); a cell learns only on the digits its own code took effect. Seeds 5-7.

| cells perturbed a digit | bit has backprop's sign (MNIST / pairs) | cell-digits taught | real sheet | its coin | minus coin |
|---|---|---|---|---|---|
| 12 | 0.62 / 0.62 | 0.2% | 0.948 / 0.717 | 0.946 / 0.710 | +0.0028 (3/3) / +0.0071 (3/3) |
| 192 | 0.53 / 0.53 | 3.5% | 0.915 / 0.714 | 0.914 / 0.715 | +0.0006 (1/3) / -0.0013 (1/3) |
| 3,072 | 0.51 / 0.51 | 52% | 0.773 / 0.641 | 0.712 / 0.628 | +0.061 / +0.013 (both wrecked) |

The margin of the bit over a coin flip goes 0.12, 0.03, 0.01: about one over the root of
the number of cells perturbed (the cell's own effect is one among n). The steps a cell gets
go up as n. So the evidence a counter gathers over a life (cell-digits taught times the
margin squared) hardly depends on n: 2.9, 3.2 and 5.2 in 100,000 a cell-digit for 12, 192
and 3,072 cells. Payment's bit has a margin of 0.40 on 3.6% of cell-digits: 580 in 100,000,
a hundred to two hundred times more. By that count code would need a hundred lives or more
to gather what payment gathers in one, and the textbook says the factor grows with the
number of cells. (The count is unfair to the one-hop rule, whose random wires agree with
backprop only 0.60 of the time and teach by being the same wires every digit.) That is the
known weakness and it shows here as a null at every n, with damage on top when n is large
because the bits are lopsided ("less" two times in three).

## Time: the smallest honest test

The bench has no time in it and the model's cells keep nothing between digits. So the test
puts time in the one place a rule can use it without the cells: **the error for a digit
arrives K showings late**, while other digits are being shown. The rule is T7x. A mark is
set on an input when it is on while the cell is near its bar or fired, and lasts L showings
(L marks words a slot, or a count-down per source); when the late bit arrives every live
mark counts. The output layer still learns on time.

| arm | error late by | marks last | real sheet | minus T7x |
|---|---|---|---|---|
| T7x | 0 | 1 | 0.9610 / 0.7583 | |
| e1n (8 seeds) | 1 | 1 (no trace) | 0.9232 / 0.6761 (born 0.9241 / 0.6728) | -0.038 / -0.082 |
| e1t2 (8) | 1 | 2 | 0.9601 / 0.7559 | -0.0009 (3/8) / -0.0025 (3/8) |
| e0t4 (3) | 0 | 4 | 0.954 / 0.752 | -0.0062 / -0.0059 |
| e1t4 (3) | 1 | 4 | 0.952 / 0.746 | -0.0083 / -0.0122 |
| e3t4 (8) | 3 | 4 | 0.9543 / 0.7490 | -0.0067 (0/8) / -0.0093 (1/8) |

What it shows: every-event counters ride out the other showings' marks. With two showings'
marks live half the steps belong to the wrong digit and the wiring reads as T7x's; with
four, three quarters do and it costs under 0.01; the delay itself costs nothing inside the
mark's life. What it does not show: memory. Nothing holds a digit in the cells' activity,
no answer depends on an earlier input, the delay is fixed and counted in showings, and the
digits in between are unrelated to the late error. It tests the rule's mark, not credit
through the sheet's own dynamics (what e-prop's trace is for). That needs the engine, on
the delayed rows.

## The bench and the rules as run

The bench is the rule's ladder's (`scripts/ladder3_rule.py`, imported): idea 165's model of
the sheet's 3,072 non-sensor cells at its bottom rung, a counter per candidate input per
cell starting at the born strength, the cell wired to its top counters at its born counts,
rewired and regulated every 64 digits, one pass of 60,000 digits one at a time, and a
softmax output layer that learns by its own delta rule. T7x on it is the one-hop rule: the
output's signed error on each line (+1, -1, or 0 when under 0.1) sent to each cell on fixed
random wires (each line reaches a cell with chance 1/2, as +1 or -1), the cell told "more"
or "less" by the sign of the sum if it was near its bar or fired, and then **every input
that was on moves its counter one count** (chance 1, 16-bit counters).

Every rule here keeps that last step and the same marks (an input that was on in the
digit), and changes only where the cell's "more / less / nothing" comes from. All integer,
every event counted. `scripts/credit_rule.py`.

| arm | where the cell's bit comes from |
|---|---|
| T7x | the ladder's own code, run again on the same seeds |
| hop | T7x through this job's loop: the check that this bench is that bench |
| **P2s** two phases, sign | The output lines reach the cells as plain inputs on the same fixed +-1 wires, at a gain g (drive a tick at full rate). Free phase: the lines carry the output's own answer (its ten rates). Clamped phase: they carry the right answer. Both phases settle twice (so the nudge also spreads one step through the cells' own wires). The cell compares its two counts: more in the clamped phase, "more"; less, "less". No error is computed anywhere and no gate is needed |
| **P2** two phases, every spike | The same phases; every spike a cell makes in the clamped phase counts its marked inputs up, every spike in the free phase counts them down (contrastive Hebbian as written). The cell remembers nothing between phases |
| **C2** code, two runs | Each cell has a code bit for the digit: force one more spike, or veto one. A clean run, then a run with every cell's code applied; the output says only whether the answer's line rose or fell. A cell whose code took effect and helped does more of it ("more" if it added a spike), one that hurt does the reverse. C2n192, C2n12: only 192 or 12 cells perturbed a digit |
| **C1** code, one run | One run with the codes applied; the outcome is only whether the pressed button was right |
| **pay1** payment, one step | The output pays backward along its own read wires: a line that should have been higher pays the fired cells it reads with a positive weight and charges those it reads with a negative one; a line that should have been lower the reverse. A cell that fired and was paid on balance is told "more", charged "less". Only the sign of each read weight is used |
| **pay** payment, passed back | pay1, and each paid or charged cell passes its bit to the wired cells whose firing let it fire (those that fired in the first settle step; through an inhibitory wire the bit flips, a strong wire counts 4). Those cells step their pixel inputs |
| **payc** conserved | pay, but a paying cell pays exactly one of those sources, drawn by chance |
| **tgt** a target from the answer | The answer alone on the fixed wires: +1 the cell should fire for this answer, -1 it should be silent, no wire no target. Near the bar and silent with target "fire": "more"; fired with target "silent": "less". The output plays no part |
| **dtp** tuned wires | Difference targets with a learned way back: each cell keeps an 8-bit up/down counter per answer line (+3 if it fired when that line was the answer, -1 if not). A line's wire into the cell is +1 if that counter is above the cell's mean counter, -1 if below. Then the output's signed errors arrive on those wires as in T7x |
| hopb, dtpb | T7x or dtp, then the bit passed back once as in pay |
| **e1n, e1t2, e1t4, e3t4, e0t4** time | T7x with the error arriving 1, 1, 1, 3, 0 showings late and a mark on each input (it was on while the cell was near or fired) that lasts 1, 2, 4, 4, 4 showings; every live mark counts when the late bit (the sign of the wires' sum) arrives. An input marked in two showings counts twice |
| still (added) | nothing learns: the born wiring as this bench writes it out, to see what the writing alone does |
| hopf, payn (added) | T7x gated by "fired" alone; the read wires' signs gated as T7x is. Together they say whether payment's lead is its wires or its gate |

**Coin controls**: the same rule with the teaching signal of another digit: T7x, dtp and
the time arms take another digit's line errors (the ladder's ring of 256); two phases take
another digit's output and answer on the lines; code takes another digit's outcome; payment
another digit's payments from the output; tgt another digit's answer. The no-trace arm e1n
is the time arms' control (a late error on this digit's marks). In every coin the cell's
own part (its gate, its code, its marks) is this digit's.

**Scoring**: every wiring written into the real born sheet (`blobmnist ceiling
--wiring-load`, 3,000 digits of regulation with learning off, the inside-learning test's
reader, fit on 60,000 and scored on the 10,000 test digits), MNIST and the pair test
(`--pair 2 --pair-mix 1`). The model's own reader is given beside it and not used to rank.

## What each rule needs in hardware

All of them keep T7x's store: a signed counter per candidate source (704 a neuron, 11 slots
of 64), counted on every event, 9 to 12 bits as bit-planes (99 to 132 words a neuron); a
step adds a slot's marked-source word into the planes; no random word. What differs:

| rule | wires | phases a taught digit | kept per cell | from the output | random words | work beyond T7x's |
|---|---|---|---|---|---|---|
| T7x one hop | none stored: a hash of cell and line gives each wire and its sign; used once a verdict | 1 | nothing (the "near" map, one word a tile a tick of the window) | each line's signed error with a "fine" zone, 2 bits a line | none | - |
| P2s two phases, sign | the same hash wires, but carrying the lines' activity into the cell's drive on the tick's path (two ANDs and popcounts of a 10-bit word a cell a tick, at 1 tick in 16) | 2: the digit shown twice as long, the answer held on the lines the second time | its count from the free phase (3 bits) | nothing computed; the lines must be clampable | none (a tick counter for the 1 in 16) | twice the ticks; no "near" map |
| P2 two phases, every spike | as P2s | 2 | nothing: one phase bit for the whole sheet | as P2s | none | a counter step for every spike of every cell in both phases (about nine times T7x's steps) |
| C2 code, two runs | none | 2: a clean run and a run with the codes | a bit: my code took effect | one bit for the whole sheet: the answer's line rose or fell (the output keeps one level per line between the runs) | two a tile a digit (which cells, which way) | a forced or vetoed spike: an OR or AND-NOT on the fire word |
| C1 code, one run | none | 1 | the same bit | one bit: good or bad | two a tile a digit | the same |
| pay1 payment | the output's own read wires, used backward: for each line in error, "fired and read with a positive weight" and "fired and read with a negative weight" as whole words, tallied per cell over the lines (a bit-sliced count, about 4 planes) | 1 | nothing | each line's signed error (as T7x) and the sign of every read weight (the read's own store) | none | a tally per cell over the lines a verdict; no "near" map |
| pay passed back | pay1's, and each cell's own input slots used backward: a paid cell writes into its fired sources' tally (a scatter: about 11 slot words a paying cell, some 200 cells a digit) | 1 | a small signed tally (about 4 planes a tile) | as pay1 | none | the scatter, a few thousand word operations a digit; a second counter step for the sources |
| payc conserved | as pay | 1 | as pay | as pay1 | one a paying cell (pick the source) | as pay |
| tgt target from the answer | hash wires from the answer's lines only | 1 | nothing | nothing: no output is needed | none | less than T7x |
| dtp tuned wires | every line reaches every cell (2 bits a line broadcast); the wire's sign is the cell's own | 1 | an 8-bit up/down counter per line (10 bytes on MNIST), one stepped a digit; each wire's sign taken from them when the cell rewires | each line's signed error (as T7x), and the answer | none | four ANDs and popcounts of a 10-bit word a cell a verdict |
| time, marks that last L showings | as T7x | 1 | - | as T7x, late | none | L mark words a slot instead of one (L = 4: 33 more words a neuron) and L steps a verdict; or a 2-bit count-down per source (22 words a neuron) |

## Picking (validation slice, seed 1)

Every-event counters have no step to pick. The only number picked is the two-phase gain
(the drive a tick that an output line at full rate adds to a cell): lives on digits
0-49,999, the wiring written into the real sheet and read on 50,000-59,999 (`blobmnist
ceiling --val 1`), the best real-sheet read taken. Gains 1/256, 1/64, **1/16**, 1/4, 1:
P2s 0.945 / 0.710, 0.954 / 0.723, **0.956 / 0.730**, 0.955 / 0.684, 0.698 / 0.505; P2 (no
1/256) 0.925 / 0.668, **0.932 / 0.692**, 0.893 / 0.634, 0.793 / 0.510. On the same slice
born 0.928 / 0.678, T7x 0.9625 / 0.7655. Both picks are inside the grid. The model's own
reader would have picked 1/64 for P2 (0.963 against 0.958): the model's reader and the real
sheet disagreeing once more. Nothing else was picked: code's perturbation is one spike,
the tuned wires' counter steps were set once to balance at a cell that fires on one digit
in four, and the time arms' delays and mark lives were fixed before any run.

## Payment's first step as the sheet would do it in bits

Not built; the form the model's rule points at.

- Kept: T7x's counters (12 bits with the sign). Per tile two bit-sliced tallies, "paid" and
  "charged" (about 4 planes each, 8 words a tile), cleared at each verdict. No hash wires,
  no "near" map.
- At a verdict, for each line whose error is not "fine", for each motor cell of that line
  (102 a button on this sheet), for each slot: the sources that fired in the window, ANDed
  with the slot's wired mask, split by the sources' kind (excitatory pushes the line up,
  inhibitory down: the read weight's sign is the source's own bit). Line too low: the
  excitatory ones into "paid", the inhibitory into "charged", by tile; too high, the
  reverse. Then each hidden cell that fired compares its tallies (more, less, or nothing)
  and takes T7x's step.
- Work: motor cells of the lines in error times 11 slots times a few word operations a
  verdict; nothing on the tick's path, no random word, no second phase.
- Open: the model's output reads every cell with a float weight. A motor cell reads the
  sources it has wired, with fixed signs. Whether that read is good enough to pay by is the
  first thing to find out; it leans on queue item 9 (the output).

## What is real, what is suspect

- Real: this bench is the ladder's. `hop` (T7x through this job's loop) on stevejobs gives
  the ladder's archived T7x for seed 5 to the digit: model 0.9691 / 0.8094, real sheet
  0.9589 / 0.7629. T7x re-run here on 8 seeds: 0.9610 / 0.7583 (the ladder's 0.9607 /
  0.7607).
- Real: pay1 over T7x (8 seeds, both tests, as fired and at matched rate; the take-apart
  arms, 3 seeds, say it is the wires). The pass-back's lead on pairs as fired, and that it
  goes with 30% to 60% more firing there. Two phases under T7x, tuned wires level with
  it, a target from the answer and code under born (every seed the same side).
- Suspect: **the output.** The model's output is a softmax layer with a float weight of
  either sign on every cell and a normalised delta rule; it is right 0.95 of the time on
  MNIST by the end of a life. Payment uses the sign of those weights. The engine's read is
  motor cells with wired sources and their own rule, right two times in three (the ladder
  doc). T7x needs the output's errors; payment needs its errors and its weights. Whether
  payment keeps its lead along the engine's read is not shown (the ladder left its rung
  T9, the sheet's own output rule, out for the same reason).
- Suspect: everything is a wiring made in the noise-free model and read in the sheet; no
  rule here has run in the engine. The matched-rate read raises the threshold cap in the
  read only (all sheets do then read at 31.2 to 31.4).
- Suspect: the first version of the time arms (d1n, d1t2, d1t4, d3t4, d0t4; 3 seeds, in
  `out/credit/full/`): a mark counted only if the cell had also been near its bar on the
  digit the error belonged to, which a cell cannot know. It flattered marks of four
  showings (0.955 / 0.759 against the corrected 0.953 / 0.749) and let the no-trace arm
  read above born (0.937 / 0.691). The corrected arms are the ones reported.
- Suspect: why gentle coins read above born. My guess (steps only move counters of inputs
  that were on, so dead inputs leave) was not checked.
- Suspect: one setting each. Two phases: fixed random wires, softmax rates on the lines,
  one gain for all cells from a grid of five on one validation seed. Tuned wires: counter
  steps (+3 / -1) set by hand. Code: one perturbation size.
- Three seeds for the arms away from T7x. Lives ran on stevejobs except T7x, born, the two
  phase arms and `still`; every read ran here. Torch on the two is not bit-identical.
- Largest counters in any life (seeds 5-7): T7x 1,420, pay1 1,208, pay 2,663, hopb 3,170,
  dtp 1,279, P2s 3,344, marks of four showings 8,073, the full code 12,703; tgt sits at the
  16-bit bound on pairs. So 12 bits with the sign for pay1 as for T7x, 13 for the
  pass-back arms, 14 for marks of four showings.

## Not done

- Any of this in the engine, and payment along a read like the engine's (a motor cell's
  wired sources with fixed signs, the engine's readout rule).
- Float versions as upper bounds: every arm is integer, every event counted.
- The rest of the bucket brigade (wealth kept per cell, "the poor are replaced"); a coin
  for the conserved payment; the take-apart arms and `still` on more than 3 seeds.
- Two phases with wires that mirror the read (equilibrium propagation proper), more settle
  steps, a nudge sized per cell. Target propagation with a learned non-linear way back.
  Code with a weighted outcome (the textbook estimator) or with few learners.
- A time test with memory in it (the label of an earlier digit; the pair's halves one after
  the other): the model's cells hold nothing between digits. Marks that fade by weight
  rather than expire; delays that vary.

## Script lines

Outputs in `out/credit/NAME/`; a finished run is kept. Here with `LADDER_VENV` set to the
main checkout's `out/ladder2/venv`; on stevejobs (folder `thonk-credit`, lives only) with
`THONK_MNIST` and `LADDER_VENV` set as the ladder's and `--parts model --jobs 2`, the
results copied into `out/credit/full/` before the reads. A life takes about a minute there
and 3 to 5 here (one thread); a real-sheet read under a minute here.

```
scripts/credit.sh smoke --quick                                       one seed, 10,000 digits, every family, end to end
scripts/credit.sh pick --val --seeds 1 --coins 0 --arms T7x,P2s@0.00390625,P2s@0.015625,P2s@0.0625,P2s@0.25,P2s@1,P2@0.015625,P2@0.0625,P2@0.25,P2@1
scripts/credit.sh full --seeds 5-12 --arms born,T7x                   the reference, 8 seeds
scripts/credit.sh full --seeds 5-7                                    every arm with its coin, lives and reads
scripts/credit.sh full --seeds 8-12 --arms pay1,pay,payc,hopb,dtp,dtpb          the arms near T7x
scripts/credit.sh full --seeds 8-12 --coins 0 --arms e1t2,e1n,e3t4
scripts/credit.sh full --seeds 5-7 --coins 0 --arms payn,hopf,C2n12coin,C2n192coin,hop
scripts/credit.sh full --seeds 5-7 --parts transplant --arms still,T7x,pay1,pay,payc,hopb,dtp,dtpb,P2s --read cap "--g.threshold_cap 1000"
scripts/credit.sh full --seeds 8-12 --parts transplant --arms T7x,pay1,pay,payc,hopb,dtp --read cap "--g.threshold_cap 1000"
scripts/credit.sh full --seeds 5-12 --table-only                      the table (out/credit/full/table.txt, table.md)
scripts/credit.sh full --seeds 5-12 --table-only --read cap "--g.threshold_cap 1000"     the matched-rate table (table-cap.txt)
```

## Code

`scripts/credit.sh`, `credit.py` (runner, picks), `credit_train.py` (the arms, one run),
`credit_rule.py` (the rules and the life's loop), `credit_table.py`; they import the
`ladder2*.py` and `ladder3*.py` files unchanged. No C++ changed. Checks: `scripts/credit.sh
smoke --quick` end to end; `hop` against the ladder's T7x (above).

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, under the one-hop rule (finding 52, the ladder's findings) and "Where
  it stands": **the wires the error comes back on matter a little, and the right ones are
  the read's own.** With every-event counters, the output paying back along its own read
  wires (the sign of each read weight, to the cells that fired) makes a wiring that reads
  0.965 / 0.770 in the real sheet against the one-hop random-wire rule's 0.961 / 0.758 at
  the same firing rate (+0.004, 8 of 8 seeds; +0.012, 7 of 8; at matched rate +0.003 and
  +0.014); the backprop teacher's wiring reads 0.966 / 0.785. [likely: 8 seeds, two tests,
  model lives written into the sheet; not in the engine; leans on the model's good output]
- New finding: **passing a teaching bit back one more step along the cells' own wires buys
  firing, not credit.** As fired +0.02 to +0.03 on pairs (8 of 8 seeds, three rules) at 30%
  to 60% more firing; its coin gains as much; at matched rate it adds nothing to payment
  and costs MNIST 0.007. [likely: 8 seeds]
- New finding: **two phases, a target from the answer, wires a cell tunes itself, and
  credit by code do not beat the one-hop rule** (0.953 / 0.726; 0.916 / 0.610; 0.955 /
  0.765; 0.773 / 0.641). [likely for tuned wires, 8 seeds; hint for the rest, 3 seeds, one
  setting each]
- New finding: **a mark that lasts covers a late error.** The one-hop rule with the error
  one showing late and marks of two showings reads as with no delay (0.960 / 0.756, 8
  seeds); marks of four cost under 0.01 at delays of 0 to 3; with no trace a late error
  teaches nothing. A test of the rule's mark only, no memory. [likely: 8 seeds, model]
- New finding candidate: **a coin control is not born.** Rules that teach few cell-digits
  read +0.01 to +0.035 above born on another digit's signal; rules that teach many read
  under. Give "over its coin" beside "over born". [hint: five coins, cause not checked]
- `docs/plan.md`: item 6 done. Next, in order: (1) payment's first step in the engine once
  the every-event one-hop rule (idea 178) is in: the same counters, the error brought back
  along the motor cells' own wired sources instead of the hash wires, scored against idea
  178's rule; it depends on the output (item 9). (2) Marks of two showings (in ticks: a
  longer window) for the one-hop rule on the delayed rows. Not worth building: two phases,
  credit by code, a target from the answer, tuned wires, the pass-back.
- `docs/ideas.md`: idea 177 done; ideas 91 and 138 tried in the model, do not beat one hop;
  idea 139 split: the output paying along its read works, cells paying cells adds nothing.
  New: payment along the motor cells' wired sources in bits; why gentle coins read above
  born; marks of two showings for late errors.
- `docs/predictions.md`: the predictions above and their scores. `docs/notes-world.md` (or
  wherever scripts are listed): `scripts/credit.sh`.

## Predictions, scored

| prediction (real sheet, MNIST / pairs) | outcome |
|---|---|
| T7x again 0.961 / 0.761, its coin 0.928 / 0.675 | 0.9610 / 0.7583 (8 seeds); coin 0.919 / 0.671, lower than guessed |
| hop gives T7x's model reader to four places on seed 5: 0.8 | yes, and the real sheet's too (against the ladder's archived run on the same machine) |
| P2s 0.955 / 0.745 | 0.953 / 0.726: pairs too high |
| P2 0.950 / 0.735, the pick takes the smallest gain | 0.931 / 0.686; the pick was the middle of the grid. Too high |
| C2 0.925 / 0.675, within 0.003 of its coin | 0.773 / 0.641, its coin worse. Wrong: I did not see that the bits would be lopsided and wreck the sheet |
| C2n192 0.928 / 0.680, C2n12 0.927 / 0.678 | 0.915 / 0.714, 0.948 / 0.717: both off, because I put coins at born |
| C1 within 0.003 of its coin | under its coin by 0.03 on MNIST |
| pay1 0.960 / 0.755 | 0.965 / 0.770: better than guessed |
| pay 0.961 / 0.760, within 0.003 of pay1; payc within 0.003 of pay | 0.9635 / 0.789: MNIST yes; pairs +0.018 over pay1, by firing more. payc yes |
| tgt 0.945 / 0.690; beats born on MNIST 0.85, on pairs 0.5 | 0.916 / 0.610, under born on both. Wrong |
| dtp 0.958 / 0.750 | 0.955 / 0.765 |
| hopb, dtpb their rule within 0.003 | on MNIST yes; on pairs +0.033 and +0.018, by firing more |
| no trace, error one late 0.928 / 0.675 | 0.923 / 0.676 |
| marks of 2, error one late 0.955 / 0.745 | 0.960 / 0.756: no loss, where I expected one |
| marks of 4, error 1 or 3 late, 0.948 / 0.725 both; on time 0.950 / 0.730 | 0.952 / 0.746, 0.954 / 0.749; 0.954 / 0.752: equal to each other as guessed, higher than guessed |
| some rule beats T7x on both tests: 0.25; likeliest pay, hopb or dtp | yes: pay1 (and pay and payc as fired). Not hopb (pairs only), not dtp (pairs only) |
| pay or pay1 beats T7x on MNIST by 0.002: 0.3; on pairs by 0.005: 0.25 | both happened (pay1 +0.0039, +0.0121) |
| dtp beats T7x on both: 0.2; P2s: 0.1 | no; no |
| code beats its coin by 0.005 on either test at any number of cells: 0.1 | by the letter yes (12 cells, pairs +0.007, 3 of 3; all cells +0.06 on MNIST between two wrecked sheets); nothing I would call learning |
| every rule that learns fires at 33 to 46 | wrong for every pass-back arm (44 to 63) |

I put the win in the right family and the wrong member (I expected the second hop to be
what payment added; it was the first), did not expect coins above born or a target from
the answer under it, and underrated the marks.
