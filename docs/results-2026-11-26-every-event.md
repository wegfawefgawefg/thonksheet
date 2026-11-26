# Every-event counters in the engine's one-hop rule (2026-11-26)

Queue item 2l, idea 178, following `docs/results-2026-11-25-rule-ladder.md` (idea 167).
**Backprop is a measuring stick here, never the project's method.** The ladder found, in an
offline model of the cells, that the one-hop rule keeps most of what backprop finds if its
hidden counters step on every marked input instead of with a small chance, and that wiring,
written into the real sheet, reads 0.961 on MNIST and 0.761 on the pair test. Nobody had run
that rule inside the engine. This job does.

## Predictions (written and committed before any scoring run)

Seen before writing these: the three docs named above with their numbers (written-in 0.961 /
0.761; born 0.924 / 0.673; the engine's rule from the answer 0.938 / 0.698, its coin 0.932 /
0.690; competitive learning 0.952 / 0.726; the model's rungs), the code, and two things run
here that are not scoring runs:

- A 12,000-digit taught life on MNIST and on the pair test with the old rule, to see what
  the motor lines' spike counts look like (needed to define the sheet's own error). The
  readout in that life is right 0.66 of the time on MNIST and 0.49 on pairs (chance); the
  right line holds about 17% of the lines' spikes on MNIST when it wins, so "share of the
  spikes less the answer" calls the right line too low on every digit.
- Eleven 2,000-digit lives (validation seed 1, reader fit on 1,000 digits), one per new
  code path, to see that each runs. Their scores say nothing (0.78 to 0.83, born at that
  fit size is about there too). What they showed: with every-event steps a taught cell
  moves about 100 counters a verdict; born inputs kept fall to 0.09 to 0.14 within 2,000
  digits (the old rule: 0.77); the rate rises to 43 in some arms; a judged life runs at
  about 5,100 ticks a second with every-event counters against 5,600 with the old rule.

The arms (names as in `scripts/everyevent.py`), all on 12-bit plane counters stepped on every
marked input unless said:

| arm | what |
|---|---|
| old | the engine's rule as it was: from the answer (`hop_teach 3`), chance 2^-9, 6 bits |
| ev3 | piece 1 alone: every event, from the answer, hidden excitatory cells learn |
| ev3i, ev3all | piece 3 on it: inhibitory and pacemaker cells learn too; motor cells too |
| ev4, ev5 | piece 2 with the sheet's own output: each line's signed error from the lines' spikes, by share (4) or by margin (5) |
| out, outi, outall | piece 2 with the model's output: an outside softmax layer's signed error per line (a measuring stick, floats); `outall` is the model's T7x rule run live |
| out6all | the same errors and learners on the old counters (6 bits, chance 2^-3): the model's T7 |

Guesses, MNIST / pair test, test digits, seeds 5-12, fresh linear reader:

| arm | guess | chances |
|---|---|---|
| ev3 | 0.943 / 0.712 | above old (0.938 / 0.698) by 0.003 / 0.005 or more: 0.6 / 0.6; above its coin by 0.005 / 0.010 or more: 0.6 / 0.5 |
| ev3all | 0.940 / 0.715 | above ev3 on MNIST: 0.35 (the model's T8x fell under born there); on pairs: 0.5 |
| ev4 or ev5, the better | 0.940 / 0.712 | above ev3 on MNIST by 0.002 or more: 0.3; on pairs within 0.005 of ev3: 0.7 (the readout is at chance there, so its error is the answer) |
| outall | 0.950 / 0.735 | above its coin by 0.010 / 0.020 or more: 0.7 / 0.6; above competitive learning (0.952 / 0.726): 0.35 / 0.6 |
| out6all | 0.940 / 0.705 | under outall on both tests: 0.75 |

- **The best live arm against the written-in 0.961 / 0.761**: within 0.005 on MNIST: 0.2;
  within 0.010 on pairs: 0.2; reaches both numbers: 0.07. My best guess for the gap that
  stays: 0.010 on MNIST, 0.025 on pairs.
- **Which piece gives the most when added to the engine's rule** (on pairs, where the room
  is): every-event counters 0.4; the outside output's error 0.35; who learns 0.1; none
  clearly 0.15.
- The sheet's own output's error (ev4, ev5) is worth less than the outside output's on
  MNIST: 0.75.
- The best arm's firing rate on MNIST while read is between 40 and 48 per 1,000 cell-ticks:
  0.6 (the written-in sheet: 43).
- Piece 4 (re-pick every 64 verdicts, or one mark a digit) lifts `outall` by 0.003 or more
  on MNIST on the validation slice: 0.3.
- Toys with every-event counters, told the answer on bad steps: parity of 3 fresh output
  0.80 (the old rule 0.847), 0.80 or more: 0.5; xor 0.99 or more: 0.7; calc xor4 held-out
  0.82 or more (old 0.871): 0.5.
- The sheet's own readout during a judged MNIST life (last tenth) is lower with every-event
  counters than with the rule off by 0.03 or more: 0.6 (the cells under it keep changing).
- A judged MNIST life with every-event plane counters runs at 85% or more of the old rule's
  ticks a second: 0.7.

Why not higher: the written-in wiring was learned on noise-free cells with one clean mark a
digit and then met the real sheet once; the live rule has to learn through the sheet's own
noise, with marks from a 12-tick window, re-picking after every digit, and regulation
chasing the changing cells. And the engine's rule read under its own model twin on pairs
(0.698 against 0.721), so some live cost is already known.
