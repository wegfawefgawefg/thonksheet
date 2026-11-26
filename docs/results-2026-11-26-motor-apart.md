# Motor cells regulated apart; a chain row a coin cannot pass (idea 184, 2026-11-26)

**(The one-sentence answer goes here when the runs are in.)**

Queue item 2n of `docs/plan.md`, following `docs/results-2026-11-26-cap-default.md`. Two
things that job left: the output's motor cells share a sector's rate bias with the hidden
cells and starve them in the MNIST world, and the quick map's chain row is passed by a coin
on the first press.

## Predictions (written and committed before any scoring run of this job was read)

Seen before writing these: the three results docs named in the job, `regulate.cpp`,
`regulate_rate.cpp`, `fire.cpp`, the chain world, the output rule's code. Run before writing
them, none of it a scoring run: the four reference lives (byte-identical with every new
number off); seven 3,000-step lives of the three-button chain, seed 5, one per new code
path, to see that each runs (their scores, 0.00 to 0.22, say nothing at that length; what
they showed: the engine's `threshold_cap_motor 64` under cap 1000 gives the same life as the
cap doc's probe arm to the last digit, and motor cells given their own walk toward 128 while
still in the sector's count push its bias to +8.6 with a third of the sectors at the limit
within 3,000 steps); three 3,000-step lives of the chain world with the new score; and the
script's smoke (`scripts/motorapart.sh smoke --quick`: seed 5, tiny lives), of whose tables I
read only the last two: long lives cut to 1,000 to 2,000 steps under out1, and one round of
blobbench. The smoke's map part runs two rows at full length for seed 5; I did not read it.

The arms (flags in `scripts/motorapart.py`), all at the default cap of 64 unless said:

| arm | what | cost in the machine |
|---|---|---|
| std | the default: motor cells in the sector's count, under its bias | |
| out1 | motor cells out of the sector's count and target; they still feel its bias | one more AND per tile in the count; no state |
| out2 | out of the count and of the bias (nothing added to a motor cell's threshold) | out1, and the compare done a second time for a tile's motor cells |
| own | a bias per kind of cell: each sector keeps a second bias for its motor cells, toward 32 | out2, and per sector one more count, one more 16-bit bias, one more step |
| own128 | own, with the motor cells' bias and walk both toward 128 | own, and one more random word per tile with motor cells |
| walk128 | out1, and the motor cells' own walk: target 128, cap 1000 | out1, and that random word |
| walkonly | the motor cells' own walk alone, still in the sector's count | that random word |

What I expect, MNIST world (20,000 digits, seeds 5-7; std is the cap doc's cap-64 row: base
0.214 with hidden 34 and motor 210 per 1,024; a2rucomp 0.840, 63, 93; best 0.716, 13, 105
with 19 in 100 hidden cells silent):

1. **The one setting's hidden cells come back under out1, out2 and own alike**: hidden rate
   25 or more per 1,024 and silent hidden cells 0.10 or less in all three: 0.75. Its online
   score rises by 0.015 or more under at least two of the three: 0.6 (my guess: 0.74).
2. **Under the base rule and a2rucomp little moves** in those three arms, because their
   hidden cells are not starved (base: 34; a2rucomp: 63, over the target by their own
   doing, so the bias stays at its limit whoever is counted): online within 0.02 of std for
   both columns under own: 0.7; under out1: 0.55 (the motor cells lose the +16 they sat
   under and fire more; the cap doc's "held as born" arm says far more motor spikes hurt the
   base rule).
3. **own keeps the motor cells where they were** (its own bias at the limit in the MNIST
   world, motor rate within 20 in 100 of std in every column): 0.7. So of the three it is
   the one that changes the output least.
4. **An honest target for motor cells costs the base rule** (walk128 or own128: base MNIST
   online under 0.195): 0.65; under a2rucomp and best they are within 0.02 of std or
   better: 0.5.
5. **walkonly pins the bias** (bias at +max on over half the sectors in the MNIST world
   under every column) and starves hidden cells more than std (hidden rate under std's in
   at least two columns): 0.7.
6. **The four toy rows do not move**: echo of 2, bandit, exclusive-or and far3 within 0.02
   of std under out1, out2 and own in every column but one cell at most: 0.6. The cell I
   trust least: far3 under a2rucomp.
7. **The fresh reader after an MNIST life** (linear reader of the non-sensor cells): under
   the one setting it is higher with the hidden cells back (out1 or own less std +0.005 or
   more): 0.55; under base and a2rucomp within 0.005: 0.7.
8. **The best arm is own or out1**: own 0.45, out1 0.35, out2 0.1, one of the honest-target
   arms 0.1.

The whole map and the inside test with the best arm (seeds 5-12):

9. Rows passed stay 6 / 13 / 15 or better (base / a2rucomp / best), with none of
   exclusive-or, 16 cues, order and MNIST lower by 0.03 or more: 0.5. MNIST online under
   the one setting +0.02 or more: 0.55.
10. The inside test: every one of the six cells (born, competitive learning, the one-hop
    rule; MNIST, pairs) within 0.005 of std: 0.6. The lives of that test that are not
    judged have no output rule driving the motor cells, so there is little to change; the
    pair test's taught life is the one that may move.
11. That I end up saying it should be the default: 0.3. That I say it should be added to the
    one setting's flags and not to the genome's defaults: 0.4.

The chain row (`--chain-first 1`: the share of chains opened by pressing A at once; a coin
gets 1 / k, "anything but B" 1 / (k - 1)):

12. The critic opens with A on 0.85 or more with three buttons at cap 64: 0.75; with four:
    0.6.
13. The base rule at cap 1000 (the cap doc's coin) opens with A on 0.25 to 0.45 with three
    buttons and 0.15 to 0.35 with four: 0.75. At cap 64 under 0.25 with either: 0.7.
14. The one setting at cap 64 opens with A under 0.3 with three and four buttons: 0.7; at
    cap 1000 with three buttons 0.45 to 0.62 (the cap doc saw 0.54): 0.7.
15. So only the critic passes the new row (halfway line 0.667 with three buttons, 0.625 with
    four): 0.7.

The one setting's lean to A (three buttons, cap 1000, seeds 1-12, A's share of opening
presses less the third button's; the cap doc: +0.19 ±0.09 over all stage-0 steps):

16. My guess at the mechanism, not credit over a step: the integrator cells keep the stage-0
    line's cells firing through stage 1 (they clear only on a verdict), the output rule's
    bad half gives "the other buttons" those inputs when a wrong button is pressed on stage
    1, and tiring neurons make that wrong button C more often than A (A's cells have just
    fired), so A gains stage-0 inputs more often than C does. If so: the lean goes (A less
    the third +0.05 or under) without time and hold cells: 0.5; without tiring neurons: 0.4;
    without the margin gate: 0.3; the base rule with time and hold cells alone shows +0.10
    or more: 0.3. No single piece removes it: 0.3.
17. That I can name where it comes from with a control that confirms it: 0.35.
