# The error sent one hop, to every hidden cell (2026-11-25)

Queue item 2b of `docs/plan.md`, idea 159. The owner: "do we need some sort of hyper
truncated bit backprop... maybe backprop isnt the issue but the whole requirement that it
goes all the way back through is fucked up right? also that its so heavy on compute."

Known in floats as direct feedback alignment (Nokland 2016) with an eligibility mark per
connection (e-prop, Bellec et al. 2020): every hidden cell gets the output's error through a
fixed random wire, no chain backwards. Here it is put in bits, online, in the recurrent
sheet, and scored by the inside-learning test (`scripts/inside.sh`).

## Prediction (written before any scoring run)

Seen before writing this: one smoke run only (validation, 2,000-digit taught life, told the
right button, 3,000 digits fitted; the numbers say nothing, it checked that the code runs):
the readout reached 0.6 right in its life, hidden cells' pixel likeness went 0.05 to 0.17,
the rate stayed at 39 spikes per 1,000 cell-ticks.

MNIST, test, 60,000-digit taught life, motor cells put back as born, fresh reader fit on
60,000 (born about 0.925; competitive learning 0.952; two-layer net 0.955):

| | told the right button (2) | told good or bad (1) |
|---|---|---|
| lived beats born by 0.005 or more | 0.55 | 0.35 |
| best guess, lived - born | +0.010 (range -0.005 to +0.025) | +0.004 (range -0.010 to +0.015) |
| lived beats its coin control by 0.005 or more | 0.45 | 0.30 |
| lived reaches competitive learning's 0.952 | 0.10 | 0.05 |
| lived reaches the two-layer net's 0.955 | 0.05 | 0.03 |

- With competitive learning in the same life: beats competitive learning alone by 0.003 or
  more: 0.30.
- Toys (fresh output on the lived sheet against the born sheet): both-not-either lifted by
  0.05 or more, mode 2: 0.6. Parity of 3 lifted by 0.05 or more: 0.35; parity of 4: 0.2.
- Calc held-out bits (xor4, add4) rise 0.03 or more over born: 0.5; over the coin control by
  0.03: 0.3.
- The online readout (`scripts/mnist.sh online`) with the rule on from the first digit is
  0.02 or more above off at some checkpoint (mean of seeds): 0.25.
- Cost: the rule adds under 20% to a taught life's ticks per second: 0.7 (the readout's own
  rewiring is most of a taught life's cost).

Reasons for the low numbers: one bit of error, only on bad steps (about a third of them once
the readout is at 0.7), sparse random wires, a counter step with chance 1/8, a fixed budget;
float DFA needs a full error vector on every example and many epochs to reach 0.98.
