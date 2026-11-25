# Competitive learning, followed up: rate, churn, products, cost, time (2026-11-25)

Queue item 2a of `docs/plan.md`, following `docs/results-2026-11-25-compete.md`. The claims
tested are findings 43, 44 and 47.

## Predictions (written before any scoring run)

Seen before writing this: speed runs only (no reader was fitted on a full life). On a
6,000-digit MNIST life, seed 5: the rule off with tile winners runs 53,700 ticks a second at
33 spikes per 1,000 cell-ticks; the born sheet pushed to 63 by a higher target (no rule)
43,700; the rule (B5) 26,700 at 63; the rule at cap 1000 41,100 at 33. So the higher rate
alone costs about 1.23 times. And 81 % of the rule's updates cross a bound (78 % change the
wiring), so skipping re-picks that change nothing cannot save much on its own.

1. MNIST, 8 seeds, rate held at target (cap 1000) in life and read, tile winners on: lived
   minus shuffled is 0.005 or more: probability 0.3. Without tile winners: 0.65.
2. MNIST, the churn control (same moves toward random sources): lived minus churn is 0.005 or
   more, as the sheet fires: probability 0.85; at matched rate: 0.75.
3. Calc, 8 seeds: held-out xor4 survives its shuffled control (lived minus shuffled at least
   0.02 on the mean and above on at least 6 of 8 seeds): probability 0.55. Held-out add4 the
   same: 0.4.
4. The cheap read-off gets the cost under 1.2 times (MNIST life ticks per second, rule on
   against off, both with tile winners): at the rule's own rate: 0.1 (the rate alone is
   1.23); with both held at target rate (cap 1000): 0.6.
5. Competitive learning plus the one-hop rule is worse than the one-hop rule alone on
   held-out xor4 again on 4 seeds (by 0.03 or more): 0.75. The tile winners, not the
   competition for counters, are most of that: 0.4.
6. Time rows (delayed echo, order, gap; whole blob learns): rule on against off moves none
   of them by 0.03 or more: 0.6.
