# Calculation and answers that are patterns (ideas 133, 134, 2026-11-25)

The owner: "should we add various alu, multiplication compute style tasks? executing
programs, 3d transforms etc. would be cool if we can arbitrarily learn calculations."

## Short version

- **An answer can now be a pattern.** A task asks for B yes/no outputs at once; each output
  bit is an "off" and an "on" group of motor cells voted like two buttons. Graded per bit,
  the working readout rule learns it: copy of 2, 4, 8 bits reaches 0.99, 0.84, 0.78 bits
  right (coin 0.49, its learning-off twin 0.47), held-out cases 0.83, 0.80, 0.78. Graded
  once for the whole answer it cannot get past two bits: copy2 0.84, copy4 0.47 (its twin
  0.47), and every answer of three or more bits sits at its twin. That was predicted from
  the rule's arithmetic before the runs (below).
- **Per-bit grading needs no change in `src/blob/`.** The world delivers each output's
  verdict through the marks it already writes (which cells are the outputs' cells, which
  group won): before a bad, only the wrong bits' cells are marked, and the rule only moves
  marked cells. With two groups per bit, "this bit was wrong" is the same as "the other
  group was right": per-bit grading is being shown the answer, bit by bit. One good or bad
  for the whole answer is the real reinforcement setting, and this rule cannot use it for
  wide answers.
- **The sheet as born carries what a linear reader of its input lines carries, and
  little more.** Over 56 members the frozen-sheet reader and the reader of the raw input
  lines agree within a few hundredths almost everywhere. Members whose answer bits are
  weighted sums of input lines (copy, not, and, or, compare, max, increment's high bits,
  multiplication's top and bottom bits, quarter turns, mirrors, scaling in 2D, turning in
  3D) are read at 0.9 to 1.00 and generalise to held-out cases. Members that need a product
  of lines that are not next to each other fail at the sheet already: the sum bits of
  addition and subtraction at every width, exclusive-or of 4 and 8 bit pairs, parity of 4
  and more, the ALUs, moving by an offset, turning by an angle, programs of two steps. The
  sheet adds nonlinearity only between lines that sit together (exclusive-or of a0, b0
  0.54 to 0.66, increment's a0 xor a1 0.72 to 0.91; with the operands laid side by side,
  addition's low bit 0.81 to 0.90).
- **Where the rule stops** (56 members, a verdict per bit): 7 learned with held-out cases
  too (copy 2, 4, 8; not; compare2; a one-instruction program; three points turned); 3
  learned on seen cases only, all tiny case sets (inc2, max2, parity2); 16 carried by the
  sheet and not reached by the rule (sparse answers: turns and mirrors 0.84 where "all
  off" gets 0.75; wider inputs: compare and increment at 4 and 8 bits); 30 not carried
  by the sheet at all. On the widest answers (8 and 9 bits out, 14 to 16 cells a group) the
  rule falls below its twin (add8 0.38 against 0.45): groups wired to inhibitory sources
  fall silent.
- **Carry.** The carry out of an addition is a weighted sum of the inputs (a + b >= 2^N):
  the input reader gets it at every width (0.96 to 1.00) and the rule learns it above its
  twin up to 6 bits (0.82, 0.73, 0.72, 0.58 for N = 2, 3, 4, 6; 0.41 at 8). Every sum bit,
  the low one included, needs an exclusive-or of lines N apart; the sheet does not carry
  those (0.49 to 0.77) and the rule does not learn them (0.50 to 0.57). Carry stops before
  the carry: not at a chain length, at the first sum bit.
- **Programs.** At the ask step the sheet still holds the instruction one step (20 ticks)
  back at 0.96 per bit, at every length. A one-instruction program (a load: a copy) is
  learned (0.81); from two instructions on, the register is a nonlinear function of two
  instructions and nothing gets it: the linear reader of every instruction at once 0.67,
  the sheet 0.67, the rule 0.51, mode 6 0.53. The memory engine gets 0.77 of whole answers
  on seen programs of two steps and 0.41 from three steps on, held out the same; it stops on
  the number of programs (5,000 of length 3), not on its depth of 4 events, so there is no
  break at 5. The reference networks: 0.25 and below.
- **Seen against held out.** Where the sheet and the rule get a member, they get its
  held-out cases too (copy, not, compare, turns: held-out within 0.05 to 0.16 of seen,
  except on tiny case sets): a rule, not a table. The memory engine is the other way round:
  0.88 to 0.99 whole answers on seen cases of 33 of the 42 members with a button form, and
  at or near the most common answer on held-out cases (add2 0.11, xor4 0.05, move4 0.06,
  angle2 0.04), except comparison (0.54 to 0.68). The reference networks generalise
  comparison (gru 0.95 held-out at 4 and 8 bits) and little else. Parity is the cleanest
  held-out world: the nearest seen case always has the other answer.
- **Other engines.** The minimal engine learns none of the battery; table neurons beat
  their twin a little on one- and two-bit answers only. The sheet learning too (mode 6) is
  a little worse than frozen everywhere.
- **The battery takes** about 9 hours on 2 jobs here for everything (1,000 full-engine
  lives), `--quick` 46 minutes on stevejobs with 2 jobs.

## What was built

- **An answer that is a pattern** (idea 134). A task can ask for B yes/no outputs at once
  (`TaskShape.pattern_bits`). Each output bit has an "off" group and an "on" group of motor
  cells (motor lines 2b and 2b + 1), voted like two buttons over the ask step: more spikes
  wins, a tie is no answer and counts as wrong. With `--motor-spread 128` the 256 motor
  cells are dealt to the 2B lines, so a group has 256 / 2B cells (64 at B = 2, 14 at B = 9).
  `src/world/plug_pattern.cpp`; definitions in `docs/curriculum.md`, "Calculation".
- **Two gradings, and what each needs from the world.**
  - *Whole answer* (`--calc-grade whole`): good if every bit is right, else bad. The world
    gives one bit of verdict, plus, for the readout rule, the same efference copy the
    buttons get: which cells belong to the outputs and which group of each won.
  - *Per bit* (`--calc-grade bit`): the world gives each output its own verdict, delivered
    through the marks: before the bad, only the wrong bits' cells are marked (and of each
    the group that won as pressed), so the readout's error step, which moves nothing on
    good, lands on those outputs alone. No change in `src/blob/` was needed: the rule
    already moves only marked cells. (Had the rule also moved on good, a mixed verdict
    would need a third mark per cell; with the working rule it does not arise.)
  - The minimal engine and table neurons read no marks, so for them both gradings are one
    good or bad for the whole answer.
- **A button per answer** (`--calc-form buttons`): so the engines that press one button
  (memory, mlp, gru) can be scored, every member with at most 16 possible answers gets one
  button per answer, graded whole. A held-out case whose answer no seen case gives is not
  scored in this form (its button is never rewarded, so nothing could know it): copy, not,
  increment, the quarter turn and mirror have no held-out column there.
- **The battery** (idea 133): `blobrun --task calc --calc OP`, 28 kinds in integers, run
  at 56 sizes, each with a quarter of its input cases held out for the whole life (never
  rewarded, scored apart; `--calc-hold`), and optionally held out in balls
  (`--calc-ball 1`: every case one line flip from a held case is held too, so the nearest
  seen case is two flips away). `src/world/task_calc.cpp`, `calc_ops.cpp`.
- **The offline readers**, `blobprobe calc`: a ridge reader per output bit fitted to the
  frozen sheet as born, and one fitted to the input lines alone (every step's lines at
  once), on seen cases, scored on later trials of seen cases and on every held-out trial;
  the nearest stored seen trial (what storing gets); the best constant answer; and, for
  programs, the sheet asked at the ask step for the instruction one step back.
- `scripts/calc.sh NAME [--quick]` runs it all and prints the tables
  (`out/calc/NAME/table.txt`); six members are in `scripts/suite.py` after every existing
  row.

How to read the numbers: "bits" is the share of output bits right, "whole" the share of
whole answers right. The best constant answer matters: place-coded geometry answers are
mostly off, so answering "all off" already gets 0.75 of the bits on a 4x4 grid; and and or
of 4 bits get 0.75 the same way. The ladder counts a level as getting a member only when it
is at least halfway from that constant to perfect.

## Prediction written before the whole-answer runs

The working readout rule moves nothing on good. Under one good or bad for the whole answer,
on every bad each output's winning group loses and its other group gains, the bits that
were right included. For one output bit with accuracy p, pushes toward right come at a
rate 1 - p (the bit was wrong) and pushes toward wrong at p - P (the bit was right but some
other bit was not; P is the chance the whole answer is right). They balance at
p = (1 + P) / 2. With B bits that each sit at p and fail independently, P = p^B, so:
B = 1 or 2 can reach 1.0, B = 3 settles near 0.62, B = 4 near 0.54, more bits near 0.5,
however readable the answer is in the sheet. Moving on good as well (readout_good 1: on a
good every output's winner gains) adds P to the pushes toward right, so the balance is
p = 1/2 + P; with P = p^B that has no stopping point below 1 up to B = 4 and one near 0.6
from B = 5. So: whole-answer grading with the working rule should fail from 3 output bits
on; with moves on good, from about 5; past that only a verdict per bit can teach it. This
ignores that a bit's pushes differ in size and that bits are not independent; it is the
shape of the argument, not a number to hold the runs to.

**Result** (seeds 5-12, the full engine, learn mode 5 unless said; first size of each kind
and every size of copy; bits right on seen cases, late; the minimal engine and table
neurons with their own learning-off twin in brackets; they read no marks, so this is one
good or bad for the whole answer):

| member | bits out | constant | a verdict per bit | one for the whole answer | the same, moving on good too | sheet learning too (mode 6), per bit | minimal engine, whole | table neurons, whole |
|---|---|---|---|---|---|---|---|---|
| copy2 | 2 | 0.50 | 0.99 | 0.84 | 0.72 | 0.93 / 0.54 | 0.47 (0.46) | 0.55 (0.47) |
| copy4 | 4 | 0.50 | 0.84 | 0.47 | 0.58 | - / - | - (-) | - (-) |
| copy8 | 8 | 0.50 | 0.78 | 0.40 | 0.39 | - / - | - (-) | - (-) |
| add2 | 3 | 0.54 | 0.64 | 0.49 | 0.58 | 0.61 / 0.45 | 0.45 (0.45) | 0.50 (0.50) |
| sub2 | 2 | 0.50 | 0.57 | 0.51 | 0.57 | 0.53 / 0.35 | 0.46 (0.47) | 0.49 (0.47) |
| mul2 | 4 | 0.78 | 0.79 | 0.46 | 0.76 | 0.79 / 0.69 | 0.44 (0.45) | 0.52 (0.47) |
| inc2 | 3 | 0.58 | 0.94 | 0.55 | 0.71 | 0.87 / 0.35 | 0.45 (0.45) | 0.53 (0.54) |
| compare2 | 1 | 0.62 | 0.84 | 0.84 | 0.61 | 0.74 / 0.73 | 0.48 (0.48) | 0.60 (0.50) |
| and4 | 4 | 0.75 | 0.73 | 0.46 | 0.49 | 0.73 / 0.72 | 0.45 (0.45) | 0.50 (0.50) |
| or4 | 4 | 0.75 | 0.79 | 0.46 | 0.56 | 0.75 / 0.74 | 0.45 (0.45) | 0.50 (0.47) |
| xor4 | 4 | 0.50 | 0.49 | 0.45 | 0.45 | 0.47 / 0.45 | 0.45 (0.46) | 0.49 (0.48) |
| not4 | 4 | 0.50 | 0.85 | 0.46 | 0.51 | 0.80 / 0.74 | 0.46 (0.46) | 0.49 (0.48) |
| shl4 | 4 | 0.69 | 0.71 | 0.46 | 0.51 | 0.71 / 0.68 | 0.45 (0.46) | 0.50 (0.52) |
| rotl4 | 4 | 0.50 | 0.59 | 0.45 | 0.46 | 0.58 / 0.56 | 0.44 (0.46) | 0.49 (0.49) |
| ones4 | 3 | 0.69 | 0.73 | 0.50 | 0.69 | 0.71 / 0.58 | 0.46 (0.47) | 0.53 (0.54) |
| parity2 | 1 | 0.50 | 0.95 | 0.95 | 0.58 | 0.94 / 0.00 | 0.47 (0.46) | 0.66 (0.48) |
| alu42 | 2 | 0.56 | 0.55 | 0.50 | 0.58 | 0.53 / 0.47 | 0.47 (0.47) | 0.51 (0.49) |
| alu82 | 2 | 0.56 | 0.55 | 0.51 | 0.58 | 0.53 / 0.49 | 0.47 (0.47) | 0.51 (0.48) |
| program3x1 | 3 | 0.50 | 0.81 | 0.54 | 0.71 | 0.72 / 0.59 | 0.47 (0.46) | 0.50 (0.49) |
| program3x2 | 3 | 0.51 | 0.51 | - | - | 0.53 / 0.51 | - (-) | - (-) |
| program3x3 | 3 | 0.51 | 0.50 | - | - | 0.51 / 0.51 | - (-) | - (-) |
| program3x4 | 3 | 0.51 | 0.49 | - | - | 0.52 / 0.52 | - (-) | - (-) |
| program3x5 | 3 | 0.51 | 0.50 | - | - | 0.52 / 0.51 | - (-) | - (-) |
| program3x6 | 3 | 0.51 | 0.51 | - | - | 0.51 / 0.51 | - (-) | - (-) |
| program3x7 | 3 | 0.51 | 0.51 | - | - | 0.52 / 0.51 | - (-) | - (-) |
| max2 | 2 | 0.69 | 0.84 | 0.65 | 0.71 | 0.77 / 0.65 | 0.47 (0.47) | 0.55 (0.47) |
| sort32 | 6 | 0.69 | 0.74 | 0.39 | 0.39 | 0.71 / 0.69 | 0.42 (0.44) | 0.49 (0.45) |
| kth2x4 | 2 | 0.50 | 0.58 | 0.50 | 0.62 | 0.55 / 0.54 | 0.47 (0.47) | 0.48 (0.49) |
| turn4 | 8 | 0.75 | 0.84 | 0.39 | 0.39 | 0.79 / 0.68 | 0.42 (0.43) | 0.49 (0.48) |

- **The prediction held in shape.** One verdict for the whole answer is as good as one per
  bit for a one-bit answer (compare2 0.84 and 0.84, parity2 0.95 and 0.95), loses some at
  two bits (copy2 0.84 against 0.99; max2 0.65 against 0.84) and fails from three bits on:
  every member with 3 or more output bits is at or below its learning-off twin under
  whole-answer grading (0.39 to 0.55), however well it learns per bit (inc2 0.94, not4
  0.85, turn4 0.84).
- **Moving on good as well helps at 3 and 4 bits and not past them**, as predicted (inc2
  0.55 to 0.71, copy4 0.47 to 0.58, program3x1 0.54 to 0.71, mul2 0.46 to 0.76, which is
  its constant answer), and is back at 0.39 at 6 and 8 bits. It is worse at one bit
  (compare2 0.84 to 0.61, parity2 0.95 to 0.58), as the readout results found for buttons.
  It never reaches per-bit grading's numbers.
- **The minimal engine learns none of the battery** (0.42 to 0.48, its twin 0.43 to 0.47):
  it takes only one good or bad and its rule has no way to aim it at an output. **Table
  neurons** learn a little where the answer is one or two bits (compare2 0.60 against
  0.50, parity2 0.66 against 0.48, copy2 0.55 against 0.47, max2 0.55 against 0.47) and
  nothing wider. Giving them per-bit grading would need them to read the marks, a change
  in their own folders; nothing else was needed to run them (the plug reads pattern
  answers for every engine).
- **The sheet learning too** (learn mode 6, a verdict per bit) is a little worse than the
  frozen sheet on every member (copy2 0.93 against 0.99, inc2 0.87 against 0.94, compare2
  0.74 against 0.84, turn4 0.79 against 0.84) and worse on held-out cases (copy2 0.54
  against 0.83); nothing it fails frozen does it learn. The inner rule, as in the readout
  results, reshapes the sheet away from what the readout was using.

## The battery, bits right (seen / held-out), seeds 5-12

Probe columns: seeds 5-12, 3,000 ask steps each, frozen sheet as born, the reader fitted on
seen cases. Rule columns: the full engine, learn mode 5 (sheet frozen, the working rule at
the motor cells), a verdict per bit, 40,000 steps, late = mean of the last 4 of 40 windows.
"constant" is the best constant answer, "nearest" the nearest stored seen trial on held-out
cases. Whole answers right are in the last column.

| member | lines in / bits out | constant | nearest seen, held-out | linear reader of the input lines | linear reader of the sheet | working rule, a verdict per bit | its twin | coin | whole answers right (rule) |
|---|---|---|---|---|---|---|---|---|---|
| copy2 | 2/2 | 0.50 | 0.50 | 1.00 / 1.00 | 1.00 / 1.00 | 0.99 / 0.83 | 0.50 | 0.49 | 0.99 / 0.74 |
| copy4 | 4/4 | 0.50 | 0.97 | 1.00 / 1.00 | 1.00 / 1.00 | 0.84 / 0.80 | 0.47 | 0.49 | 0.45 / 0.39 |
| copy8 | 8/8 | 0.50 | 1.00 | 1.00 / 1.00 | 1.00 / 1.00 | 0.78 / 0.78 | 0.46 | 0.48 | 0.12 / 0.12 |
| add2 | 4/3 | 0.54 | 0.48 | 0.75 / 0.37 | 0.81 / 0.42 | 0.64 / 0.44 | 0.48 | 0.49 | 0.25 / 0.07 |
| add3 | 6/4 | 0.52 | 0.61 | 0.69 / 0.43 | 0.71 / 0.52 | 0.56 / 0.48 | 0.47 | 0.49 | 0.10 / 0.05 |
| add4 | 8/5 | 0.51 | 0.77 | 0.62 / 0.49 | 0.64 / 0.56 | 0.52 / 0.50 | 0.47 | 0.49 | 0.04 / 0.03 |
| add6 | 12/7 | 0.50 | 0.82 | 0.58 / 0.56 | 0.59 / 0.58 | 0.44 / 0.44 | 0.46 | 0.48 | 0.00 / 0.00 |
| add8 | 16/9 | 0.50 | 0.74 | 0.55 / 0.55 | 0.55 / 0.55 | 0.38 / 0.38 | 0.45 | 0.48 | 0.00 / 0.00 |
| sub2 | 4/2 | 0.50 | 0.33 | 0.63 / 0.17 | 0.71 / 0.25 | 0.57 / 0.32 | 0.49 | 0.49 | 0.32 / 0.08 |
| sub4 | 8/4 | 0.50 | 0.73 | 0.53 / 0.39 | 0.56 / 0.50 | 0.49 / 0.46 | 0.47 | 0.48 | 0.06 / 0.04 |
| sub8 | 16/8 | 0.50 | 0.73 | 0.50 / 0.50 | 0.51 / 0.51 | 0.39 / 0.39 | 0.46 | 0.48 | 0.00 / 0.00 |
| mul2 | 4/4 | 0.78 | 0.79 | 0.95 / 0.78 | 0.95 / 0.76 | 0.79 / 0.65 | 0.48 | 0.49 | 0.39 / 0.22 |
| mul3 | 6/6 | 0.71 | 0.78 | 0.83 / 0.76 | 0.83 / 0.75 | 0.69 / 0.64 | 0.47 | 0.49 | 0.13 / 0.12 |
| mul4 | 8/8 | 0.67 | 0.79 | 0.75 / 0.72 | 0.74 / 0.71 | 0.60 / 0.60 | 0.47 | 0.48 | 0.03 / 0.02 |
| inc2 | 2/3 | 0.58 | 0.29 | 1.00 / 0.46 | 1.00 / 0.46 | 0.94 / 0.40 | 0.48 | 0.49 | 0.84 / 0.00 |
| inc4 | 4/5 | 0.59 | 0.68 | 0.89 / 0.65 | 0.93 / 0.70 | 0.77 / 0.60 | 0.48 | 0.49 | 0.25 / 0.06 |
| inc8 | 8/9 | 0.56 | 0.98 | 0.89 / 0.88 | 0.92 / 0.91 | 0.72 / 0.72 | 0.47 | 0.48 | 0.05 / 0.05 |
| compare2 | 4/1 | 0.62 | 0.69 | 0.97 / 0.69 | 0.98 / 0.77 | 0.84 / 0.76 | 0.48 | 0.49 | 0.84 / 0.76 |
| compare4 | 8/1 | 0.53 | 0.88 | 0.98 / 0.95 | 0.94 / 0.91 | 0.71 / 0.70 | 0.49 | 0.49 | 0.71 / 0.70 |
| compare8 | 16/1 | 0.50 | 0.86 | 0.98 / 0.97 | 0.92 / 0.92 | 0.62 / 0.62 | 0.49 | 0.49 | 0.62 / 0.62 |
| and4 | 8/4 | 0.75 | 0.99 | 1.00 / 1.00 | 1.00 / 0.99 | 0.73 / 0.73 | 0.48 | 0.49 | 0.23 / 0.23 |
| or4 | 8/4 | 0.75 | 1.00 | 1.00 / 1.00 | 1.00 / 1.00 | 0.79 / 0.78 | 0.46 | 0.49 | 0.34 / 0.32 |
| xor4 | 8/4 | 0.50 | 0.98 | 0.54 / 0.38 | 0.66 / 0.63 | 0.49 / 0.47 | 0.47 | 0.49 | 0.06 / 0.05 |
| not4 | 4/4 | 0.50 | 0.97 | 1.00 / 1.00 | 1.00 / 1.00 | 0.85 / 0.80 | 0.47 | 0.49 | 0.52 / 0.37 |
| xor8 | 16/8 | 0.50 | 0.85 | 0.50 / 0.50 | 0.55 / 0.55 | 0.39 / 0.39 | 0.46 | 0.48 | 0.00 / 0.00 |
| shl4 | 6/4 | 0.69 | 0.90 | 0.83 / 0.75 | 0.87 / 0.79 | 0.71 / 0.67 | 0.48 | 0.49 | 0.26 / 0.21 |
| shl8 | 11/8 | 0.72 | 0.95 | 0.80 / 0.79 | 0.79 / 0.79 | 0.66 / 0.66 | 0.46 | 0.48 | 0.07 / 0.06 |
| rotl4 | 6/4 | 0.50 | 0.86 | 0.72 / 0.62 | 0.77 / 0.67 | 0.59 / 0.55 | 0.48 | 0.49 | 0.14 / 0.13 |
| ones4 | 4/3 | 0.69 | 0.47 | 0.85 / 0.58 | 0.83 / 0.52 | 0.73 / 0.57 | 0.47 | 0.49 | 0.37 / 0.16 |
| ones8 | 8/4 | 0.67 | 0.54 | 0.80 / 0.74 | 0.78 / 0.72 | 0.68 / 0.67 | 0.49 | 0.48 | 0.18 / 0.17 |
| parity2 | 2/1 | 0.50 | 0.00 | 1.00 / 0.00 | 1.00 / 0.00 | 0.95 / 0.01 | 0.47 | 0.50 | 0.95 / 0.01 |
| parity4 | 4/1 | 0.50 | 0.00 | 0.67 / 0.21 | 0.65 / 0.15 | 0.57 / 0.28 | 0.47 | 0.50 | 0.57 / 0.28 |
| parity8 | 8/1 | 0.50 | 0.00 | 0.54 / 0.40 | 0.52 / 0.45 | 0.49 / 0.47 | 0.49 | 0.49 | 0.49 / 0.47 |
| alu42 | 6/2 | 0.56 | 0.56 | 0.62 / 0.40 | 0.66 / 0.46 | 0.55 / 0.45 | 0.49 | 0.49 | 0.31 / 0.21 |
| alu44 | 10/4 | 0.56 | 0.81 | 0.53 / 0.52 | 0.57 / 0.56 | 0.50 / 0.50 | 0.48 | 0.49 | 0.07 / 0.07 |
| alu82 | 7/2 | 0.56 | 0.74 | 0.63 / 0.54 | 0.66 / 0.56 | 0.55 / 0.50 | 0.49 | 0.48 | 0.31 / 0.25 |
| alu84 | 11/4 | 0.53 | 0.85 | 0.61 / 0.60 | 0.61 / 0.60 | 0.51 / 0.50 | 0.47 | 0.49 | 0.07 / 0.06 |
| program3x1 | 5/3 | 0.50 | 0.88 | 1.00 / 1.00 | 1.00 / 1.00 | 0.81 / 0.74 | 0.49 | 0.49 | 0.47 / 0.37 |
| program3x2 | 5/3 | 0.51 | 0.75 | 0.67 / 0.66 | 0.67 / 0.63 | 0.51 / 0.51 | 0.48 | 0.49 | 0.14 / 0.13 |
| program3x3 | 5/3 | 0.51 | 0.73 | 0.67 / 0.68 | 0.66 / 0.66 | 0.50 / 0.49 | 0.48 | 0.49 | 0.12 / 0.12 |
| program3x4 | 5/3 | 0.51 | 0.67 | 0.67 / 0.68 | 0.66 / 0.67 | 0.49 / 0.50 | 0.48 | 0.48 | 0.13 / 0.12 |
| program3x5 | 5/3 | 0.51 | 0.63 | 0.67 / 0.67 | 0.66 / 0.65 | 0.50 / 0.48 | 0.48 | 0.49 | 0.12 / 0.12 |
| program3x6 | 5/3 | 0.51 | 0.60 | 0.67 / 0.67 | 0.66 / 0.66 | 0.51 / 0.49 | 0.47 | 0.49 | 0.13 / 0.10 |
| program3x7 | 5/3 | 0.51 | 0.59 | 0.67 / 0.67 | 0.66 / 0.66 | 0.51 / 0.50 | 0.47 | 0.49 | 0.14 / 0.14 |
| max2 | 4/2 | 0.69 | 0.78 | 0.93 / 0.84 | 0.97 / 0.79 | 0.84 / 0.73 | 0.48 | 0.49 | 0.70 / 0.51 |
| max4 | 8/4 | 0.62 | 0.94 | 0.88 / 0.84 | 0.88 / 0.86 | 0.68 / 0.67 | 0.46 | 0.49 | 0.21 / 0.19 |
| sort32 | 6/6 | 0.69 | 0.86 | 0.89 / 0.82 | 0.90 / 0.83 | 0.74 / 0.71 | 0.47 | 0.48 | 0.15 / 0.12 |
| kth2x4 | 10/2 | 0.50 | 0.96 | 0.69 / 0.67 | 0.71 / 0.70 | 0.58 / 0.58 | 0.49 | 0.49 | 0.34 / 0.33 |
| turn4 | 8/8 | 0.75 | 0.87 | 1.00 / 1.00 | 1.00 / 1.00 | 0.84 / 0.70 | 0.46 | 0.48 | 0.17 / 0.03 |
| mirror4 | 8/8 | 0.75 | 0.87 | 1.00 / 1.00 | 1.00 / 1.00 | 0.84 / 0.72 | 0.47 | 0.48 | 0.17 / 0.03 |
| move4 | 16/8 | 0.75 | 0.87 | 0.75 / 0.75 | 0.75 / 0.72 | 0.56 / 0.54 | 0.47 | 0.48 | 0.01 / 0.00 |
| scale4 | 11/8 | 0.75 | 0.87 | 1.00 / 0.93 | 1.00 / 0.95 | 0.82 / 0.73 | 0.46 | 0.48 | 0.16 / 0.07 |
| turn3d3 | 9/9 | 0.67 | 0.98 | 1.00 / 1.00 | 1.00 / 1.00 | 0.81 / 0.79 | 0.46 | 0.48 | 0.13 / 0.09 |
| move3d3 | 18/9 | 0.67 | 0.96 | 0.67 / 0.67 | 0.69 / 0.67 | 0.43 / 0.43 | 0.46 | 0.48 | 0.00 / 0.00 |
| angle2 | 18/10 | 0.80 | 0.80 | 0.82 / 0.74 | 0.82 / 0.75 | 0.69 / 0.65 | 0.46 | 0.48 | 0.01 / 0.01 |
| turn3pts2 | 12/12 | 0.50 | 0.99 | 1.00 / 1.00 | 1.00 / 1.00 | 0.76 / 0.74 | 0.44 | 0.47 | 0.03 / 0.02 |


Each output bit of the arithmetic members and programs: "Carry, exactly" below; every
member's bits are in `out/calc/main/table.txt` (section C).

## One button per answer: storing against a rule (whole answers right, seen / held-out)

All engines told only good and bad for the whole answer, one button per possible answer,
seeds 5-12, 40,000 steps. memory: `--engine memory` (a store of the last few events); gru
and mlp: the fixed reference networks. The working rule's bit-form column is from the
table above (a verdict per bit, which is a stronger teacher).

| member | most common answer | nearest (probe) | memory | gru | mlp | working rule, button form | working rule, bit form | coin |
|---|---|---|---|---|---|---|---|---|
| copy2 | 0.25 | 1.00 / 0.00 | 0.99 / - | 1.00 / - | 1.00 / - | 0.90 / - | 0.99 / 0.74 | 0.24 |
| copy4 | 0.06 | 1.00 / 0.88 | 0.98 / - | 0.81 / - | 0.92 / - | - / - | 0.45 / 0.39 | 0.05 |
| add2 | 0.25 | 1.00 / 0.00 | 0.99 / 0.11 | 0.55 / 0.13 | 0.79 / 0.08 | 0.12 / 0.08 | 0.25 / 0.07 | 0.11 |
| add3 | 0.12 | 1.00 / 0.10 | 0.98 / 0.05 | 0.19 / 0.03 | 0.29 / 0.04 | - / - | 0.10 / 0.05 | 0.06 |
| sub2 | 0.25 | 1.00 / 0.00 | 0.99 / 0.21 | 0.60 / 0.01 | 0.88 / 0.03 | 0.26 / 0.15 | 0.32 / 0.08 | 0.25 |
| sub4 | 0.06 | 1.00 / 0.31 | 0.95 / 0.04 | 0.08 / 0.03 | 0.09 / 0.03 | - / - | 0.06 / 0.04 | 0.05 |
| mul2 | 0.44 | 1.00 / 0.47 | 0.98 / 0.19 | 0.47 / 0.66 | 0.55 / 0.56 | - / - | 0.39 / 0.22 | 0.06 |
| inc2 | 0.25 | 1.00 / 0.00 | 0.99 / - | 1.00 / - | 1.00 / - | - / - | 0.84 / 0.00 | 0.12 |
| inc4 | 0.06 | 1.00 / 0.22 | 0.98 / - | 0.81 / - | 0.92 / - | - / - | 0.25 / 0.06 | 0.05 |
| compare2 | 0.62 | 1.00 / 0.69 | 0.99 / 0.59 | 1.00 / 0.80 | 1.00 / 0.79 | - / - | 0.84 / 0.76 | 0.49 |
| compare4 | 0.53 | 1.00 / 0.88 | 0.99 / 0.68 | 0.96 / 0.95 | 0.95 / 0.93 | 0.83 / 0.82 | 0.71 / 0.70 | 0.49 |
| compare8 | 0.50 | 0.86 / 0.86 | 0.60 / 0.54 | 0.95 / 0.95 | 0.95 / 0.94 | - / - | 0.62 / 0.62 | 0.49 |
| and4 | 0.32 | 1.00 / 0.98 | 0.88 / 0.35 | 0.31 / 0.33 | 0.36 / 0.34 | - / - | 0.23 / 0.23 | 0.05 |
| or4 | 0.32 | 1.00 / 0.98 | 0.88 / 0.32 | 0.32 / 0.31 | 0.45 / 0.40 | - / - | 0.34 / 0.32 | 0.05 |
| xor4 | 0.06 | 1.00 / 0.91 | 0.95 / 0.05 | 0.08 / 0.02 | 0.10 / 0.04 | 0.05 / 0.05 | 0.06 / 0.05 | 0.05 |
| not4 | 0.06 | 1.00 / 0.88 | 0.97 / - | 0.83 / - | 0.90 / - | - / - | 0.52 / 0.37 | 0.06 |
| shl4 | 0.23 | 1.00 / 0.67 | 0.98 / 0.19 | 0.38 / 0.37 | 0.46 / 0.37 | - / - | 0.26 / 0.21 | 0.06 |
| rotl4 | 0.06 | 1.00 / 0.60 | 0.98 / 0.06 | 0.19 / 0.00 | 0.24 / 0.04 | - / - | 0.14 / 0.13 | 0.06 |
| ones4 | 0.38 | 1.00 / 0.00 | 0.98 / 0.25 | 0.67 / 0.40 | 0.81 / 0.19 | - / - | 0.37 / 0.16 | 0.12 |
| ones8 | 0.27 | 1.00 / 0.00 | 0.89 / 0.15 | 0.27 / 0.25 | 0.40 / 0.30 | - / - | 0.18 / 0.17 | 0.06 |
| parity2 | 0.50 | 1.00 / 0.00 | 0.99 / 0.26 | 0.91 / 0.00 | 1.00 / 0.00 | 1.00 / 0.00 | 0.95 / 0.01 | 0.50 |
| parity4 | 0.50 | 1.00 / 0.00 | 0.99 / 0.38 | 0.72 / 0.22 | 0.87 / 0.15 | - / - | 0.57 / 0.28 | 0.50 |
| parity8 | 0.50 | 1.00 / 0.00 | 0.99 / 0.28 | 0.54 / 0.40 | 0.57 / 0.41 | - / - | 0.49 / 0.47 | 0.49 |
| alu42 | 0.33 | 1.00 / 0.31 | 0.99 / 0.28 | 0.43 / 0.23 | 0.58 / 0.23 | - / - | 0.31 / 0.21 | 0.25 |
| alu44 | 0.13 | 0.95 / 0.46 | 0.72 / 0.11 | 0.13 / 0.12 | 0.13 / 0.12 | - / - | 0.07 / 0.07 | 0.06 |
| alu82 | 0.33 | 1.00 / 0.54 | 0.99 / 0.21 | 0.38 / 0.29 | 0.48 / 0.28 | - / - | 0.31 / 0.25 | 0.24 |
| alu84 | 0.10 | 0.85 / 0.56 | 0.57 / 0.13 | 0.11 / 0.10 | 0.13 / 0.12 | - / - | 0.07 / 0.06 | 0.06 |
| program3x1 | 0.12 | 1.00 / 0.62 | 0.98 / - | 0.99 / - | 0.99 / - | - / - | 0.47 / 0.37 | 0.11 |
| program3x2 | 0.13 | 1.00 / 0.49 | 0.77 / 0.35 | 0.25 / 0.14 | 0.25 / 0.17 | 0.10 / 0.09 | 0.14 / 0.13 | 0.11 |
| program3x3 | 0.13 | 0.61 / 0.41 | 0.45 / 0.41 | 0.18 / 0.19 | 0.20 / 0.19 | - / - | 0.12 / 0.12 | 0.12 |
| program3x4 | 0.13 | 0.36 / 0.34 | 0.42 / 0.43 | 0.15 / 0.15 | 0.16 / 0.15 | - / - | 0.13 / 0.12 | 0.12 |
| program3x5 | 0.13 | 0.29 / 0.28 | 0.41 / 0.40 | 0.13 / 0.16 | 0.15 / 0.16 | - / - | 0.12 / 0.12 | 0.11 |
| program3x6 | 0.13 | 0.24 / 0.25 | 0.40 / 0.39 | 0.14 / 0.13 | 0.16 / 0.15 | - / - | 0.13 / 0.10 | 0.12 |
| program3x7 | 0.13 | 0.22 / 0.23 | 0.41 / 0.40 | 0.13 / 0.13 | 0.14 / 0.15 | - / - | 0.14 / 0.14 | 0.12 |
| max2 | 0.44 | 1.00 / 0.63 | 0.99 / 0.27 | 0.89 / 0.41 | 0.97 / 0.44 | 0.36 / 0.31 | 0.70 / 0.51 | 0.24 |
| max4 | 0.12 | 1.00 / 0.81 | 0.92 / 0.19 | 0.27 / 0.16 | 0.33 / 0.21 | - / - | 0.21 / 0.19 | 0.05 |
| kth2x4 | 0.25 | 0.99 / 0.93 | 0.96 / 0.50 | 0.48 / 0.44 | 0.50 / 0.46 | 0.25 / 0.23 | 0.34 / 0.33 | 0.25 |
| turn4 | 0.06 | 1.00 / 0.00 | 0.98 / - | 0.98 / - | 0.99 / - | 0.10 / - | 0.17 / 0.03 | 0.06 |
| mirror4 | 0.06 | 1.00 / 0.00 | 0.98 / - | 0.99 / - | 0.99 / - | - / - | 0.17 / 0.03 | 0.05 |
| move4 | 0.06 | 1.00 / 0.00 | 0.95 / 0.06 | 0.11 / 0.02 | 0.14 / 0.02 | - / - | 0.01 / 0.00 | 0.05 |
| scale4 | 0.12 | 1.00 / 0.15 | 0.98 / 0.03 | 0.79 / 0.19 | 0.85 / 0.19 | - / - | 0.16 / 0.07 | 0.05 |
| angle2 | 0.12 | 1.00 / 0.05 | 0.98 / 0.04 | 0.27 / 0.03 | 0.37 / 0.07 | - / - | 0.01 / 0.01 | 0.07 |


- **Storing is what the memory engine does, and the battery shows it.** On seen cases it
  is the best learner here by far: 0.88 to 0.99 of whole answers on 33 of the 42 members,
  including add, sub, xor, the ALUs at 2 bits, move and the angle turn, where nothing else
  passes 0.6. On held-out cases it is at or near the most common answer everywhere except
  comparison (0.54 to 0.68) and kth (0.50, where a neighbouring input often has the same
  k-th number). That is the picture the coordinator expected.
- **The reference networks generalise only what is a weighted sum of the inputs, and only
  some of that.** gru and mlp get comparison at 0.95 held-out (4 and 8 bits), mul2 at 0.56
  to 0.66 (its answers are mostly zero), and nothing else held-out above the constant. On
  seen cases they learn the readable members (copy, not, increment, turn, mirror, scale at
  0.79 to 1.00) and little of the rest (0.08 to 0.5).
- **The working rule in the button form** (one button per answer, whole verdict), on 10 members
  (seeds 5-12): it learns comparison better than in the bit form (compare4 0.83 seen, 0.82 held-out,
  against 0.71 and 0.70) and parity2 and copy2 (1.00, 0.90), and is at chance on everything with
  more than a few answers (add2 0.12, turn4 0.10, program3x2 0.10: 8 to 16 buttons sharing 256
  cells, and one verdict for the whole answer). It never stores the way the memory engine does.
- **The bit form with a verdict per bit is the stronger teacher**, and on held-out cases it
  is what generalises: compare2 0.76, compare4 0.70 held-out whole answers, max2 0.51, the
  readable members' bits within 0.05 to 0.16 of seen. It never stores: on seen cases of
  add2 it gets 0.25 whole answers where the memory engine gets 0.99.

## Held out in balls

`--calc-ball 1`: each held-out case brings every case one line flip away along, so no
held-out case has a seen case one flip away (only for case sets of 4,096 or fewer). Whole
answers right on held-out cases for the nearest-seen reader and the one-button engines,
bits right for the rule and the sheet reader; ball 0 (the main battery) / ball 1. Rule
with ball 1 on 11 members only.

| member | nearest held-out, ball 0 / 1 | memory held, 0 / 1 | gru held, 0 / 1 | mlp held, 0 / 1 | rule (bit form) held bits, 0 / 1 | sheet reader held bits, 0 / 1 |
|---|---|---|---|---|---|---|
| copy2 | 0.00 / 0.00 | - / - | - / - | - / - | 0.83 / - | 1.00 / 0.34 |
| copy4 | 0.88 / 0.80 | - / - | - / - | - / - | 0.80 / - | 1.00 / 1.00 |
| add2 | 0.00 / 0.00 | 0.11 / 0.08 | 0.13 / 0.07 | 0.08 / 0.09 | 0.44 / 0.47 | 0.42 / 0.49 |
| add3 | 0.10 / 0.14 | 0.05 / 0.04 | 0.03 / 0.04 | 0.04 / 0.05 | 0.48 / - | 0.52 / 0.52 |
| sub2 | 0.00 / 0.00 | 0.21 / 0.14 | 0.01 / 0.05 | 0.03 / 0.07 | 0.32 / 0.35 | 0.25 / 0.35 |
| sub4 | 0.31 / 0.25 | 0.04 / 0.05 | 0.03 / 0.02 | 0.03 / 0.02 | 0.46 / - | 0.50 / 0.51 |
| mul2 | 0.47 / 0.55 | 0.19 / 0.26 | 0.66 / 0.33 | 0.56 / 0.33 | 0.65 / 0.65 | 0.76 / 0.72 |
| inc2 | 0.00 / 0.00 | - / - | - / - | - / - | 0.40 / - | 0.46 / 0.33 |
| inc4 | 0.22 / 0.24 | - / - | - / - | - / - | 0.60 / - | 0.70 / 0.69 |
| compare2 | 0.69 / 0.60 | 0.59 / 0.35 | 0.80 / 0.59 | 0.79 / 0.68 | 0.76 / - | 0.77 / 0.63 |
| compare4 | 0.88 / 0.85 | 0.68 / 0.59 | 0.95 / 0.95 | 0.93 / 0.94 | 0.70 / 0.68 | 0.91 / 0.91 |
| compare8 | 0.86 / 0.86 | 0.54 / 0.54 | 0.95 / 0.95 | 0.94 / 0.94 | 0.62 / - | 0.92 / 0.92 |
| and4 | 0.98 / 0.89 | 0.35 / 0.31 | 0.33 / 0.31 | 0.34 / 0.33 | 0.73 / - | 0.99 / 0.97 |
| or4 | 0.98 / 0.97 | 0.32 / 0.40 | 0.31 / 0.37 | 0.40 / 0.46 | 0.78 / - | 1.00 / 1.00 |
| xor4 | 0.91 / 0.77 | 0.05 / 0.04 | 0.02 / 0.01 | 0.04 / 0.03 | 0.47 / 0.45 | 0.63 / 0.56 |
| not4 | 0.88 / 0.80 | - / - | - / - | - / - | 0.80 / - | 1.00 / 1.00 |
| shl4 | 0.67 / 0.58 | 0.19 / 0.24 | 0.37 / 0.31 | 0.37 / 0.40 | 0.67 / - | 0.79 / 0.77 |
| rotl4 | 0.60 / 0.48 | 0.06 / 0.05 | 0.00 / 0.03 | 0.04 / 0.02 | 0.55 / - | 0.67 / 0.63 |
| ones4 | 0.00 / 0.16 | 0.25 / 0.27 | 0.40 / 0.27 | 0.19 / 0.22 | 0.57 / - | 0.52 / 0.47 |
| ones8 | 0.00 / 0.06 | 0.15 / 0.11 | 0.25 / 0.30 | 0.30 / 0.19 | 0.67 / - | 0.72 / 0.70 |
| parity2 | 0.00 / 0.32 | 0.26 / 1.00 | 0.00 / 1.00 | 0.00 / 0.97 | 0.01 / - | 0.00 / 0.32 |
| parity4 | 0.00 / 0.20 | 0.38 / 0.35 | 0.22 / 0.19 | 0.15 / 0.19 | 0.28 / 0.24 | 0.15 / 0.20 |
| parity8 | 0.00 / 0.12 | 0.28 / 0.27 | 0.40 / 0.30 | 0.41 / 0.27 | 0.47 / - | 0.45 / 0.29 |
| alu42 | 0.31 / 0.21 | 0.28 / 0.16 | 0.23 / 0.23 | 0.23 / 0.19 | 0.45 / 0.45 | 0.46 / 0.47 |
| alu44 | 0.46 / 0.39 | 0.11 / 0.12 | 0.12 / 0.14 | 0.12 / 0.13 | 0.50 / - | 0.56 / 0.54 |
| alu82 | 0.54 / 0.52 | 0.21 / 0.29 | 0.29 / 0.28 | 0.28 / 0.28 | 0.50 / - | 0.56 / 0.56 |
| alu84 | 0.56 / 0.50 | 0.13 / 0.12 | 0.10 / 0.10 | 0.12 / 0.11 | 0.50 / - | 0.60 / 0.59 |
| program3x1 | 0.62 / 0.06 | - / - | - / - | - / - | 0.74 / - | 1.00 / 1.00 |
| program3x2 | 0.49 / 0.43 | 0.35 / 0.34 | 0.14 / 0.09 | 0.17 / 0.15 | 0.51 / 0.48 | 0.63 / 0.59 |
| program3x3 | 0.41 / 0.41 | 0.41 / 0.41 | 0.19 / 0.19 | 0.19 / 0.19 | 0.49 / - | 0.66 / 0.67 |
| program3x4 | 0.34 / 0.34 | 0.43 / 0.43 | 0.15 / 0.15 | 0.15 / 0.15 | 0.50 / - | 0.67 / 0.67 |
| program3x5 | 0.28 / 0.28 | 0.40 / 0.40 | 0.16 / 0.16 | 0.16 / 0.16 | 0.48 / - | 0.65 / 0.66 |
| program3x6 | 0.25 / 0.25 | 0.39 / 0.39 | 0.13 / 0.13 | 0.15 / 0.15 | 0.49 / - | 0.66 / 0.66 |
| program3x7 | 0.23 / 0.22 | 0.40 / 0.40 | 0.13 / 0.13 | 0.15 / 0.15 | 0.50 / - | 0.66 / 0.66 |
| max2 | 0.63 / 0.65 | 0.27 / 0.21 | 0.41 / 0.35 | 0.44 / 0.51 | 0.73 / 0.71 | 0.79 / 0.71 |
| max4 | 0.81 / 0.81 | 0.19 / 0.20 | 0.16 / 0.17 | 0.21 / 0.18 | 0.67 / - | 0.86 / 0.85 |
| kth2x4 | 0.93 / 0.90 | 0.50 / 0.43 | 0.44 / 0.38 | 0.46 / 0.40 | 0.58 / 0.57 | 0.70 / 0.69 |
| turn4 | 0.00 / 0.00 | - / - | - / - | - / - | 0.70 / 0.70 | 1.00 / 1.00 |
| mirror4 | 0.00 / 0.00 | - / - | - / - | - / - | 0.72 / - | 1.00 / 1.00 |
| move4 | 0.00 / 0.00 | 0.06 / 0.06 | 0.02 / 0.02 | 0.02 / 0.02 | 0.54 / - | 0.72 / 0.73 |
| scale4 | 0.15 / 0.13 | 0.03 / 0.03 | 0.19 / 0.19 | 0.19 / 0.19 | 0.73 / - | 0.95 / 0.95 |
| angle2 | 0.05 / 0.05 | 0.04 / 0.04 | 0.03 / 0.03 | 0.07 / 0.07 | 0.65 / - | 0.75 / 0.75 |

- **Balls change little.** Where a held-out case's nearest seen neighbour one flip away
  gave the answer away (copy4, not4, xor4, and4), the nearest-seen reader drops 0.06 to
  0.14; the engines and the sheet reader move by a few hundredths either way. Place-coded
  members (the geometry) are unchanged: two different points already differ in two lines.
- **On tiny case sets a ball is most of the set** (parity2: three of four cases held, one
  seen; copy2 and inc2 likewise), so the ball-1 numbers there mean nothing.
- The held-out picture of the main battery stands: storing does not reach held-out cases
  except where neighbouring inputs share the answer (comparison, and the per-bit
  majority of neighbours on bitwise members), and the readers and the rule do wherever the
  answer is a weighted sum of the inputs.

## The ladder

Which sizes each level gets, per kind. A level gets a member when its share right is at
least halfway from the best constant answer to perfect (0.75 where the constant is 0.5)
and, for a learner, 0.05 over its learning-off twin; a held-out column also counts a score
within 0.1 of a passing seen score. "none": tried and failed at every size; "-": not run.
Sizes: bits per operand (copy, arithmetic, bit operations, ALUs, lookup), instructions on
a 3-bit register (program 3xL), grid side (geometry; angle: radius; turn3: three points
on a 2x2 grid).

**Bits right** (whole-answer grading, mode 6, minimal and table on the first size of each
kind only):

| op | nearest seen, held-out | inputs reader | inputs reader, held-out | sheet reader | sheet reader, held-out | rule, per bit | rule, held-out | rule, whole-answer grading | rule, sheet learning (mode 6) | mode 6, held-out | minimal engine | table neurons |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| copy | 4/8 | 2/4/8 | 2/4/8 | 2/4/8 | 2/4/8 | 2/4/8 | 2/4/8 | 2 | 2 | none | none | none |
| add | 4/6 | none | none | 2 | none | none | none | none | none | none | none | none |
| sub | none | none | none | none | none | none | none | none | none | none | none | none |
| mul | none | 2 | none | 2 | none | none | none | none | none | none | none | none |
| inc | 8 | 2/4/8 | 8 | 2/4/8 | 8 | 2 | none | none | 2 | none | none | none |
| compare | 4/8 | 2/4/8 | 4/8 | 2/4/8 | 4/8 | 2 | 2 | 2 | none | none | none | none |
| and | 4 | 4 | 4 | 4 | 4 | none | none | none | none | none | none | none |
| or | 4 | 4 | 4 | 4 | 4 | none | none | none | none | none | none | none |
| xor | 4/8 | none | none | none | none | none | none | none | none | none | none | none |
| not | 4 | 4 | 4 | 4 | 4 | 4 | 4 | none | 4 | 4 | none | none |
| shl | 4/8 | none | none | 4 | 4 | none | none | none | none | none | none | none |
| rotl | 4 | none | none | 4 | 4 | none | none | none | none | none | none | none |
| ones | none | 4 | none | none | none | none | none | none | none | none | none | none |
| parity | none | 2 | none | 2 | none | 2 | none | 2 | 2 | none | none | none |
| alu4 | 4 | none | none | none | none | none | none | none | none | none | none | none |
| alu8 | 4 | none | none | none | none | none | none | none | none | none | none | none |
| program | 3x1 | 3x1 | 3x1 | 3x1 | 3x1 | 3x1 | 3x1 | none | none | none | none | none |
| max | 4 | 2/4 | 2/4 | 2/4 | 4 | 2 | none | none | none | none | none | none |
| sort3 | 2 | 2 | 2 | 2 | 2 | none | none | none | none | none | none | none |
| kth | 2x4 | none | none | none | none | none | none | none | none | none | none | none |
| turn | none | 4 | 4 | 4 | 4 | none | none | none | none | none | none | none |
| mirror | none | 4 | 4 | 4 | 4 | none | none | - | - | - | - | - |
| move | none | none | none | none | none | none | none | - | - | - | - | - |
| scale | none | 4 | 4 | 4 | 4 | none | none | - | - | - | - | - |
| turn3d | 3 | 3 | 3 | 3 | 3 | none | none | - | - | - | - | - |
| move3d | 3 | none | none | none | none | none | none | - | - | - | - | - |
| angle | none | none | none | none | none | none | none | - | - | - | - | - |
| turn3 | pts2 | pts2 | pts2 | pts2 | pts2 | pts2 | pts2 | - | - | - | - | - |

**Whole answers right** (the one-button engines in the button form):

| op | nearest seen, held-out | rule (bit form) | rule, held-out | memory | memory, held-out | gru | gru, held-out | mlp | mlp, held-out |
|---|---|---|---|---|---|---|---|---|---|
| copy | 4/8 | 2 | 2 | 2/4 | - | 2/4 | - | 2/4 | - |
| add | none | none | none | 2/3 | none | none | none | 2 | none |
| sub | none | none | none | 2/4 | none | none | none | 2 | none |
| mul | none | none | none | 2 | none | none | none | none | none |
| inc | 8 | 2 | none | 2/4 | - | 2/4 | - | 2/4 | - |
| compare | 4/8 | 2 | 2 | 2/4 | none | 2/4/8 | 4/8 | 2/4/8 | 4/8 |
| and | 4 | none | none | 4 | none | none | none | none | none |
| or | 4 | none | none | 4 | none | none | none | none | none |
| xor | 4 | none | none | 4 | none | none | none | none | none |
| not | 4 | none | none | 4 | - | 4 | - | 4 | - |
| shl | 4/8 | none | none | 4 | none | none | none | none | none |
| rotl | 4 | none | none | 4 | none | none | none | none | none |
| ones | none | none | none | 4/8 | none | none | none | 4 | none |
| parity | none | 2 | none | 2/4/8 | none | 2 | none | 2/4 | none |
| alu4 | none | none | none | 2/4 | none | none | none | none | none |
| alu8 | 4 | none | none | 2/4 | none | none | none | none | none |
| program | 3x1 | none | none | 3x1/3x2 | none | 3x1 | none | 3x1 | none |
| max | 4 | none | none | 2/4 | none | 2 | none | 2 | none |
| sort3 | none | none | none | - | - | - | - | - | - |
| kth | 2x4 | none | none | 2x4 | none | none | none | none | none |
| turn | none | none | none | 4 | - | 4 | - | 4 | - |
| mirror | none | none | none | 4 | - | 4 | - | 4 | - |
| move | none | none | none | 4 | none | none | none | none | none |
| scale | none | none | none | 4 | none | 4 | none | 4 | none |
| turn3d | 3 | none | none | - | - | - | - | - | - |
| move3d | 3 | none | none | - | - | - | - | - | - |
| angle | none | none | none | 2 | none | none | none | none | none |
| turn3 | pts2 | none | none | - | - | - | - | - | - |

**Where each member stops** (the working rule graded per bit; "a fails": the sheet as born
does not carry it to the linear reader; "b fails": it does, and the rule does not reach
it; "held-out fails": learned on seen cases, not on held-out ones):

| member | constant | sheet seen / held | rule seen / held | where it stops |
|---|---|---|---|---|
| copy2 | 0.50 | 1.00 / 1.00 | 0.99 / 0.83 | learned, held-out too |
| copy4 | 0.50 | 1.00 / 1.00 | 0.84 / 0.80 | learned, held-out too |
| copy8 | 0.50 | 1.00 / 1.00 | 0.78 / 0.78 | learned, held-out too |
| add2 | 0.54 | 0.81 / 0.42 | 0.64 / 0.44 | in the sheet, the readout does not find it (b fails) |
| add3 | 0.52 | 0.71 / 0.52 | 0.56 / 0.48 | not in the sheet (a fails) |
| add4 | 0.51 | 0.64 / 0.56 | 0.52 / 0.50 | not in the sheet (a fails) |
| add6 | 0.50 | 0.59 / 0.58 | 0.44 / 0.44 | not in the sheet (a fails) |
| add8 | 0.50 | 0.55 / 0.55 | 0.38 / 0.38 | not in the sheet (a fails) |
| sub2 | 0.50 | 0.71 / 0.25 | 0.57 / 0.32 | not in the sheet (a fails) |
| sub4 | 0.50 | 0.56 / 0.50 | 0.49 / 0.46 | not in the sheet (a fails) |
| sub8 | 0.50 | 0.51 / 0.51 | 0.39 / 0.39 | not in the sheet (a fails) |
| mul2 | 0.78 | 0.95 / 0.76 | 0.79 / 0.65 | in the sheet, the readout does not find it (b fails) |
| mul3 | 0.71 | 0.83 / 0.75 | 0.69 / 0.64 | not in the sheet (a fails) |
| mul4 | 0.67 | 0.74 / 0.71 | 0.60 / 0.60 | not in the sheet (a fails) |
| inc2 | 0.58 | 1.00 / 0.46 | 0.94 / 0.40 | learned on seen cases only (held-out fails) |
| inc4 | 0.59 | 0.93 / 0.70 | 0.77 / 0.60 | in the sheet, the readout does not find it (b fails) |
| inc8 | 0.56 | 0.92 / 0.91 | 0.72 / 0.72 | in the sheet, the readout does not find it (b fails) |
| compare2 | 0.62 | 0.98 / 0.77 | 0.84 / 0.76 | learned, held-out too |
| compare4 | 0.53 | 0.94 / 0.91 | 0.71 / 0.70 | in the sheet, the readout does not find it (b fails) |
| compare8 | 0.50 | 0.92 / 0.92 | 0.62 / 0.62 | in the sheet, the readout does not find it (b fails) |
| and4 | 0.75 | 1.00 / 0.99 | 0.73 / 0.73 | in the sheet, the readout does not find it (b fails) |
| or4 | 0.75 | 1.00 / 1.00 | 0.79 / 0.78 | in the sheet, the readout does not find it (b fails) |
| xor4 | 0.50 | 0.66 / 0.63 | 0.49 / 0.47 | not in the sheet (a fails) |
| not4 | 0.50 | 1.00 / 1.00 | 0.85 / 0.80 | learned, held-out too |
| xor8 | 0.50 | 0.55 / 0.55 | 0.39 / 0.39 | not in the sheet (a fails) |
| shl4 | 0.69 | 0.87 / 0.79 | 0.71 / 0.67 | in the sheet, the readout does not find it (b fails) |
| shl8 | 0.72 | 0.79 / 0.79 | 0.66 / 0.66 | not in the sheet (a fails) |
| rotl4 | 0.50 | 0.77 / 0.67 | 0.59 / 0.55 | in the sheet, the readout does not find it (b fails) |
| ones4 | 0.69 | 0.83 / 0.52 | 0.73 / 0.57 | not in the sheet (a fails) |
| ones8 | 0.67 | 0.78 / 0.72 | 0.68 / 0.67 | not in the sheet (a fails) |
| parity2 | 0.50 | 1.00 / 0.00 | 0.95 / 0.01 | learned on seen cases only (held-out fails) |
| parity4 | 0.50 | 0.65 / 0.15 | 0.57 / 0.28 | not in the sheet (a fails) |
| parity8 | 0.50 | 0.52 / 0.45 | 0.49 / 0.47 | not in the sheet (a fails) |
| alu42 | 0.56 | 0.66 / 0.46 | 0.55 / 0.45 | not in the sheet (a fails) |
| alu44 | 0.56 | 0.57 / 0.56 | 0.50 / 0.50 | not in the sheet (a fails) |
| alu82 | 0.56 | 0.66 / 0.56 | 0.55 / 0.50 | not in the sheet (a fails) |
| alu84 | 0.53 | 0.61 / 0.60 | 0.51 / 0.50 | not in the sheet (a fails) |
| program3x1 | 0.50 | 1.00 / 1.00 | 0.81 / 0.74 | learned, held-out too |
| program3x2 | 0.51 | 0.67 / 0.63 | 0.51 / 0.51 | not in the sheet (a fails) |
| program3x3 | 0.51 | 0.66 / 0.66 | 0.50 / 0.49 | not in the sheet (a fails) |
| program3x4 | 0.51 | 0.66 / 0.67 | 0.49 / 0.50 | not in the sheet (a fails) |
| program3x5 | 0.51 | 0.66 / 0.65 | 0.50 / 0.48 | not in the sheet (a fails) |
| program3x6 | 0.51 | 0.66 / 0.66 | 0.51 / 0.49 | not in the sheet (a fails) |
| program3x7 | 0.51 | 0.66 / 0.66 | 0.51 / 0.50 | not in the sheet (a fails) |
| max2 | 0.69 | 0.97 / 0.79 | 0.84 / 0.73 | learned on seen cases only (held-out fails) |
| max4 | 0.62 | 0.88 / 0.86 | 0.68 / 0.67 | in the sheet, the readout does not find it (b fails) |
| sort32 | 0.69 | 0.90 / 0.83 | 0.74 / 0.71 | in the sheet, the readout does not find it (b fails) |
| kth2x4 | 0.50 | 0.71 / 0.70 | 0.58 / 0.58 | not in the sheet (a fails) |
| turn4 | 0.75 | 1.00 / 1.00 | 0.84 / 0.70 | in the sheet, the readout does not find it (b fails) |
| mirror4 | 0.75 | 1.00 / 1.00 | 0.84 / 0.72 | in the sheet, the readout does not find it (b fails) |
| move4 | 0.75 | 0.75 / 0.72 | 0.56 / 0.54 | not in the sheet (a fails) |
| scale4 | 0.75 | 1.00 / 0.95 | 0.82 / 0.73 | in the sheet, the readout does not find it (b fails) |
| turn3d3 | 0.67 | 1.00 / 1.00 | 0.81 / 0.79 | in the sheet, the readout does not find it (b fails) |
| move3d3 | 0.67 | 0.69 / 0.67 | 0.43 / 0.43 | not in the sheet (a fails) |
| angle2 | 0.80 | 0.82 / 0.75 | 0.69 / 0.65 | not in the sheet (a fails) |
| turn3pts2 | 0.50 | 1.00 / 1.00 | 0.76 / 0.74 | learned, held-out too |

Of 56 members: 7 learned with held-out cases too (copy 2, 4, 8; not; compare2; a
one-instruction program; three points turned); 3 learned on seen cases only, all of them
tiny case sets where a quarter held out is 1 to 4 cases (inc2, max2, parity2); 16 carried
by the sheet and missed by the rule, most of them sparse answers where the rule beats the
twin but not halfway from the constant (turn4 0.84 against 0.75), or wider members whose
input lines reach three shelf rows from the motor region (compare at 4 and 8 bits an
operand: 8 and 16 input lines; increment at 4 and 8); 30 not carried by the sheet at all.

- **Largest size each level still gets.** Sheet reader: copy 8, compare 8, increment 8,
  max 4, sort3 2, quarter turn, mirror and scale 4x4, 3D turn 3x3x3, and/or/not 4,
  multiplication 2, addition 2 (only through its readable carry bit), parity 2, programs
  of 1 step. The rule, per bit: copy 8, not 4, increment 2, compare 2, max 2, parity 2,
  programs of 1, three points. Whole-answer grading: copy 2 and the one-bit answers.
  Minimal engine and table neurons: nothing (table neurons beat their twin on one- and
  two-bit answers without reaching halfway). The memory engine, seen: almost everything
  small; held-out: nothing. gru and mlp held-out: comparison at 4 and 8.

## Carry, exactly

Per output bit, seen cases: linear reader of the input lines / linear reader of the sheet
/ the working rule (a verdict per bit). Bit 0 is the lowest; the last bit of add is the
carry out.

| member | bits |
|---|---|
| add2 | 0 0.64/0.77/0.57 | 1 0.62/0.68/0.52 | 2 1.00/0.97/0.82 |
| add3 | 0 0.63/0.73/0.52 | 1 0.58/0.62/0.50 | 2 0.57/0.59/0.48 | 3 0.96/0.93/0.73 |
| add4 | 0 0.54/0.63/0.51 | 1 0.53/0.57/0.47 | 2 0.53/0.52/0.46 | 3 0.53/0.54/0.45 | 4 0.98/0.91/0.72 |
| add6 | 0 0.51/0.65/0.43 | 1 0.49/0.50/0.42 | 2 0.52/0.52/0.42 | 3 0.51/0.50/0.41 | 4 0.51/0.52/0.41 | 5 0.51/0.51/0.42 | 6 0.97/0.89/0.58 |
| add8 | 0 0.51/0.54/0.36 | 1 0.50/0.51/0.39 | 2 0.50/0.52/0.37 | 3 0.51/0.50/0.37 | 4 0.49/0.49/0.37 | 5 0.50/0.51/0.40 | 6 0.51/0.49/0.38 | 7 0.49/0.50/0.37 | 8 0.97/0.88/0.41 |
| sub2 | 0 0.64/0.78/0.57 | 1 0.62/0.65/0.56 |
| sub4 | 0 0.54/0.65/0.52 | 1 0.55/0.55/0.48 | 2 0.53/0.52/0.47 | 3 0.51/0.53/0.47 |
| mul2 | 0 1.00/0.99/0.81 | 1 0.87/0.86/0.66 | 2 0.99/0.98/0.76 | 3 0.96/0.98/0.93 |
| mul3 | 0 1.00/0.99/0.81 | 1 0.81/0.81/0.65 | 2 0.65/0.66/0.54 | 3 0.73/0.76/0.58 | 4 0.84/0.84/0.69 | 5 0.92/0.94/0.89 |
| inc4 | 0 1.00/1.00/0.96 | 1 0.72/0.91/0.52 | 2 0.86/0.85/0.67 | 3 0.90/0.92/0.76 | 4 0.96/0.98/0.94 |
| inc8 | 0 1.00/1.00/0.86 | 1 0.54/0.79/0.53 | 2 0.75/0.73/0.56 | 3 0.87/0.86/0.68 | 4 0.94/0.94/0.79 | 5 0.97/0.97/0.81 | 6 0.98/0.98/0.76 | 7 0.99/0.99/0.53 | 8 1.00/1.00/0.99 |
| xor4 | 0 0.54/0.66/0.52 | 1 0.52/0.70/0.51 | 2 0.54/0.54/0.47 | 3 0.55/0.75/0.48 |
| ones4 | 0 0.67/0.65/0.54 | 1 0.92/0.86/0.72 | 2 0.96/0.97/0.93 |
| parity4 | 0 0.67/0.65/0.57 |
| program3x1 | 0 1.00/1.00/0.64 | 1 1.00/1.00/0.89 | 2 1.00/1.00/0.88 |
| program3x2 | 0 0.69/0.68/0.51 | 1 0.65/0.67/0.52 | 2 0.66/0.66/0.51 |
| program3x4 | 0 0.70/0.69/0.49 | 1 0.66/0.65/0.49 | 2 0.66/0.65/0.49 |
| program3x7 | 0 0.69/0.67/0.50 | 1 0.66/0.65/0.51 | 2 0.66/0.66/0.51 |

- **The carry out is a weighted sum of the inputs** (a + b >= 2^N is one threshold over
  the lines with weights 1, 2, 4, ...), so the plain reader of the input lines gets it at
  every width (0.96 to 1.00) and the rule learns it at small widths (0.82 at N = 2, 0.72 at
  4, 0.58 at 6, 0.41 at 8, where the 18 lines of a 9-bit answer leave 14 cells a group and
  the group falls silent).
- **Every sum bit is an exclusive-or of lines N apart.** Bit 0 of a + b is a0 xor b0, and
  a0 and b0 sit N lines apart on the shelf (all of a, then all of b). The sheet carries a
  little of it at N = 2 (0.77, the lines two shelf groups apart) and less as they move
  apart (0.73, 0.63, 0.65, 0.54); the rule gets 0.57, 0.52, 0.51 and then chance. Bit 1 and
  up need that exclusive-or and the carry into it, a three-way term: the sheet does not
  have it at any width (0.49 to 0.68). So carry stops before the carry: the first sum bit
  already fails, and the chain is never reached. This agrees with the function battery's
  finding that the sheet carries pairs and not three-way terms.
- **Increment shows the same thing from the other side.** Bit k of a + 1 is a_k xor (all
  of a_0 .. a_k-1 on); for k >= 2 that is mostly just a_k, so those bits are read and
  learned (0.86 to 0.99); bit 1 is a0 xor a1, two neighbouring lines, and there the sheet
  does carry the pair (0.91 at N = 4, 0.79 at N = 8, where the input lines alone give 0.72
  and 0.54) but the rule does not pick it up (0.52, 0.53).
- **Subtraction** fails the same way (its low bit is the same exclusive-or).
- **Multiplication** at 2 bits is mostly readable (bits 0 and 3 are and-functions: 0.99,
  0.98), its middle bits are exclusive-ors of products and are not.
- The layout matters, and the world decides it: with `--calc-lay 1` each a_k sits next to
  b_k. Sheet reader (seeds 5-8, both layouts) and the rule (5-12), bits right per output
  bit, seen cases, apart -> side by side:

  | member | sheet reader | the rule |
  |---|---|---|
  | add2 | 0.77 0.70 0.97 -> 0.90 0.79 0.98 | 0.57 0.52 0.82 -> 0.62 0.53 0.81 |
  | add3 | 0.74 0.61 0.59 0.91 -> 0.86 0.63 0.60 0.92 | 0.52 0.50 0.48 0.73 -> 0.64 0.50 0.49 0.71 |
  | add4 | 0.64 0.57 0.52 0.56 0.90 -> 0.81 0.56 0.56 0.56 0.91 | 0.51 0.47 0.46 0.45 0.72 -> 0.61 0.47 0.46 0.46 0.70 |
  | sub2 | 0.77 0.67 -> 0.90 0.66 | 0.57 0.56 -> 0.66 0.55 |
  | xor4 | 0.67 0.69 0.53 0.75 -> 0.81 0.77 0.77 0.55 | 0.52 0.51 0.47 0.48 -> 0.62 0.49 0.50 0.47 |

  Side by side, the sheet carries an exclusive-or of a pair at 0.77 to 0.90 when the pair
  shares a shelf row (the last pair of xor4 is split across two rows: 0.55), so the low sum
  bit becomes readable at every width tried (0.81 to 0.90). The bits that need the carry,
  a three-way term, stay at 0.56 to 0.79. The rule gains on the low bit only (0.10 to 0.13)
  and stays well under what the sheet offers.

## Programs, exactly

A 3-bit register; ops load, add, xor, shift left (the first is always a load); one
instruction a step, the ask line on with the last. Seeds 5-12.

| program length | the sheet still has the last instruction (bits, whole) | linear reader of the input lines, all steps at once | linear reader of the sheet | nearest seen, held-out bits | rule per bit, seen / held | memory, whole, seen / held | gru, whole, seen / held | mlp, whole, seen |
|---|---|---|---|---|---|---|---|---|
| 1 | - | 1.00 | 1.00 | 0.88 | 0.81 / 0.74 | 0.98 / - | 0.99 / - | 0.99 |
| 2 | 0.98, 0.91 | 0.67 | 0.67 | 0.75 | 0.51 / 0.51 | 0.77 / 0.35 | 0.25 / 0.14 | 0.25 |
| 3 | 0.96, 0.81 | 0.67 | 0.66 | 0.73 | 0.50 / 0.49 | 0.45 / 0.41 | 0.18 / 0.19 | 0.20 |
| 4 | 0.96, 0.81 | 0.67 | 0.66 | 0.67 | 0.49 / 0.50 | 0.42 / 0.43 | 0.15 / 0.15 | 0.16 |
| 5 | 0.96, 0.81 | 0.67 | 0.66 | 0.63 | 0.50 / 0.48 | 0.41 / 0.40 | 0.13 / 0.16 | 0.15 |
| 6 | 0.96, 0.82 | 0.67 | 0.66 | 0.60 | 0.51 / 0.49 | 0.40 / 0.39 | 0.14 / 0.13 | 0.16 |
| 7 | 0.96, 0.82 | 0.67 | 0.66 | 0.59 | 0.51 / 0.50 | 0.41 / 0.40 | 0.13 / 0.13 | 0.14 |

(Whole answers: chance is 1/8, the most common answer 0.13.)

- **Holding is not what stops it.** At the ask step the sheet still carries the previous
  instruction, op and value, at 0.96 of its bits (0.81 to 0.91 of whole instructions) at
  every length. One step back is 20 ticks, inside the 20 ticks the findings say a cue is
  held; two steps back was not asked.
- **Combining is.** From two instructions on, the register is a nonlinear function of two
  instructions (the value of the load, plus or xor the value of the next one): no weighted
  sum of every line of every step gets more than 0.67 of its bits, and the sheet gets the
  same 0.67. The rule, which can only add up what the sheet offers, is at chance from
  length 2.
- **The memory engine does not break at 5 steps; it breaks at 3, on the number of programs.** It
  stores whole answers for the 200 programs of length 2 (0.77 seen, 0.35 held out), but there are
  5,000 programs of length 3 and about 7,500 rewarded trials in a life (a trial is now 4 steps), so
  a program rarely comes twice; from length 3 on, seen and held-out are the same (0.41 to 0.45) and
  stay there to length 7. What it keeps getting is what its last few events decide alone (a final
  load fixes the register, a quarter of programs). Its depth (4 past events) never comes into play
  here because nothing is learned that needs more than the last one or two.
- **The reference networks do worse than the memory engine on programs** (gru 0.25 at
  length 2, chance from 3), with the same good and bad.
- **With the sheet learning too** (learn mode 6, a verdict per bit), programs do no
  better: length 1 0.72 (0.81 frozen), lengths 2 to 7 0.51 to 0.53 bits, 0.14 to 0.15 whole
  answers (chance 0.13). The inner rule does not build the combination either.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/task-map.md`, Holes: "output that is a whole pattern" has worlds now (the
  calculation battery, `--task calc`), and tier 3 (or a new row "calculation") can say:
  the full engine with the working rule, graded per bit, learns pattern answers that are
  weighted sums of their input lines (copy, not, compare, max, quarter turns and mirrors,
  scaling) at small sizes, and nothing that needs a product of distant lines (sums,
  differences, exclusive-or of bit pairs, parity of 4 or more, the ALUs, moving by an
  offset, programs of two or more steps). One good or bad for the whole answer holds two
  bits, not four. The amount half of that hole is still a hole.
- `docs/task-map.md`, "How a task can differ", line 2: "the world tells each output
  whether it was right" is a teaching mode of its own, between "good or bad after every
  answer" and "being shown the answer"; with two-valued outputs it is the latter.
- `docs/findings.md`, a principle candidate [likely, one engine]: **the frozen sheet is a
  linear-plus-pairs feature bank.** Over 56 calculation worlds the reader of the sheet as
  born equals the reader of the raw input lines within a few hundredths, except for
  exclusive-ors of lines that sit together (a0 xor b0 in xor4: 0.54 to 0.66; a0 xor a1
  in increment: 0.72 to 0.91). Together with the
  function battery (pairs, not three-way terms) this says what "rich" means for this sheet
  today: it gives the readout little beyond its input, and nothing at a distance.
- `docs/findings.md`, a principle candidate [likely]: **one verdict cannot teach many
  outputs with an error rule that moves only on bad.** Predicted from the drift balance
  before the runs (above), and seen: one verdict per answer equals one per bit at one
  output bit, loses a little at two, and is at the learning-off twin from three bits on
  (0.39 to 0.55 where per-bit grading gets up to 0.94); moving on good as well rescues 3
  and 4 bits partly and not 6 or 8. A wide answer needs a verdict per output, or a rule
  that can use one good or bad for many outputs at once.
- `docs/findings.md`, walls: "Reading a thin signal" gains a consequence: the readout's reach (about
  two tiles) sets which output bits of a pattern can be learned; the bits whose input lines sit
  furthest from the motor region are learned worst (copy8: 0.90 for the first, 0.50 for the eighth).
  And "more than four things": an answer of 8 or 9 bits, with 14 to 16 motor cells a group, learns
  below its learning-off twin (add8 0.38 against 0.45): the groups fall silent. With main's
  `readout_evidence 3` (wire only excitatory sources, made for ten-button MNIST) the drop below the
  twin goes (add8 0.38 to 0.44, sub8 and xor8 0.39 to 0.43, move3d3 0.43 to 0.48; twins 0.45 to
  0.46) but copy8 falls from 0.78 to 0.66 (seeds 5-12): part of the silencing is groups wired to
  inhibitory sources, and the cure costs where there was something to learn.
- `docs/findings.md`, against the memory engine (ideas 149, 150): the calculation battery
  is the held-out test it asked for. Storing gets 0.88 to 0.99 of whole answers on seen
  cases of most small members and at or near chance on held-out ones; the one family where
  storing generalises a little is comparison (0.59 to 0.68), because neighbouring inputs
  mostly share the answer. Parity is the cleanest held-out world: the nearest seen case
  always has the other answer (nearest reader 0.00).
- `docs/ideas.md`, ideas 133 and 134: built; results here. Next for them: a layout that
  puts operands' bits side by side is a world choice and changes what is readable (see
  carry); a pattern readout with more motor cells per output (the motor region holds 256,
  and 2B groups share them).
- `docs/predictions.md`: the drift prediction above, with its result.

## How long it takes

On this PC, `nice -n 19`, 2 jobs at a time, under load from other agents (so rough):

- one full-engine life of 40,000 steps with its learning-off twin, coin and perfect
  player: about 1 minute (bit grading); about 30 s without baselines;
- the memory engine, gru, mlp with their twins: 5 to 15 s a life;
- one probe (3,000 ask steps, settle, four readers): 10 to 40 s;
- the whole battery as `scripts/calc.sh main` runs it: about 1,000 full-engine lives and
  2,000 short ones, about 9 hours on 2 jobs here (minimal and table neurons ran on
  stevejobs: 336 lives in 2.3 hours on 4 jobs under heavy load);
- `--quick` (2 seeds, 4,000-step lives, 770 runs): 46 minutes on stevejobs with 2 jobs
  under other agents' load.

## Rough

- **Bits right flatters sparse answers.** A place-coded answer is mostly off, so "all
  off" gets 0.75 of a 4x4 point's bits and 0.80 of the angle member's; and and or of 4
  bits likewise 0.75. Read the "constant" column first; the ladder uses it.
- **The button form cannot score held-out cases of one-to-one members** (their answer's
  button is never rewarded), so memory, gru and mlp have no held-out column for copy,
  not, increment, the quarter turn and mirror; and members with more than 16 answers
  (8-bit copy, add4 and up, sort, 3D, three points) have no button form at all.
- **Tiny case sets make held-out a test of bias, not of rule.** copy2 has 4 cases and one
  held out; add2 has 16 and 4. Three seen cases do not pin down "copy" for any learner;
  the held-out score on such members (the linear reader of the inputs gets 0.37 on add2,
  0.00 on parity2) says which way a learner extrapolates, not whether it has the rule.
- **Operands are laid out all of a, then all of b**, so the bits a sum or an exclusive-or
  pairs sit N lines apart, and the higher a bit, the further its lines are from the motor
  region. Both hurt. `--calc-lay 1` puts them side by side: then the sheet reads the low
  sum bit at 0.81 to 0.90; the carry bits do not move.
- **The motor region is fixed at 256 cells**, so a 9-bit answer has 14 cells a group,
  below the 32 the readout results found a decision needs.
- Everything in the main battery is seeds 5-12. Ball-1 held-out runs: the fast engines on
  5-12, the probe on 5-8, the rule on 11 members. The working rule in the button form ran
  on 10 members only (`scripts/calc.sh` runs it on all 42 by default).
- whole-answer grading, moving on good, mode 6, minimal and table neurons ran on the
  first size of each kind only (plus every size of copy, and mode 6 on every program
  length).
- Coin and learning-off columns are the rule's; the other engines' twins are in
  `out/calc/main/table.txt`.
- A world's held-out set for case spaces over 4,096 is a hash test per case (1 in 4); with
  65,536 cases and 15,000 rewarded trials most "seen" cases are met once, so seen and
  held-out converge on big members whatever the learner does.

## Script lines

```
scripts/calc.sh main                       the battery, seeds 5-12 (probe, the rule per bit,
                                           whole-answer grading, mode 6, minimal, table,
                                           memory, gru, mlp)
scripts/calc.sh main --quick               2 seeds, 4,000-step lives
scripts/calc.sh main --only arith,program --configs probe,bit
scripts/calc.sh ball1 --configs memory,gru,mlp,probe --calc-ball 1
scripts/calc.sh main --table-only          the tables from what has run
scripts/suite.sh NAME                      the suite, with six calculation rows at the end
```
