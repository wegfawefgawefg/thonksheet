# Predictions

Written down before the results come in, so we can tell learning from hindsight. Each one
has a rough confidence and gets marked right, wrong or unclear when its result lands. Add
new predictions before starting an experiment, never after.

## 2026-11-24, before the ablation sweep, the rivals and table neurons

1. **Most mechanisms do nothing measurable on Echo-level tasks.** Branch bonus, pacemakers,
   surprise boost, far links and prediction learning can each be switched off with no change
   in score. (75%)
   - *Scored 2026-11-24: half right. No measurable effect: delays, branch bonus, varied
     neurons, noise, far redraw. Small or mixed: far slots, pacemakers, prediction learning,
     cooldown, sector regulation. Wrong on the surprise boost, which matters, though only
     because it is on 92-99% of the time and so acts as a constant doubling of the
     learning chances.*
2. **What does matter:** inhibition and regulation (staying alive), growth and pruning, and
   the good/bad rule itself. (75%)
   - *Scored: right. Inhibition, per-neuron threshold regulation, good, bad, growth and
     pruning each cost 0.1 to 0.46 when removed. Sector regulation does not matter.*
3. **A frozen blob with a learned readout matches or beats the full blob on Echo.** The
   inner learning is not yet helping the task. (65%)
   - *Scored: right for 2 cues (0.914 against 0.940, a tie) and shuffled echo; wrong for 4
     cues, where the readout stays at its twin's 0.16 and the full rule reaches 0.33. First
     evidence that learning inside the sheet does something.*
4. **Noise-is-the-experiment saturates less and does better on 4 cues, but learns slower**
   than the current rule, because far fewer events get graded. (55%)
   - *Scored: wrong. It loses: 0.81 on 2 cues, 0.12 on 4 cues, and very uneven across
     seeds. Alongside good/bad it changes nothing. Saturation was not measured.*
5. **Random flips solve cartpole, slowly.** (60%) **Some online brain-style rule solves
   cartpole in this round.** (35%)
   - *Scored: wrong on both so far. Flips are the only rule that improves on cartpole (19-20
     steps per fall against 10.6 for the brain rule) but that is still no better than coin
     flips (22-25). No rule learns to balance.*
6. **Table neurons learn Echo faster and sharper than the popcount blob, and carry a cue
   worse** until their wiring is tuned. (50%)
   - *Scored: half right. Sharper, yes: 0.68 on four cues against 0.31. Carrying worse,
     no: they learn with cue and button two rows apart (0.80) where the blob does not.*
7. **Delays matter for the timing tasks** (beat, delayed echo) **and not for Echo.** (70%)
   - *Scored: the Echo half is right (no effect). The timing half is untested: no rule
     learns delayed echo at all (0.44 against 0.43).*
8. **Removing the pulse helps a little and never hurts.** (60%)
   - *Scored: unclear. Sector regulation off moves scores by -0.07, -0.08, +0.04.*

## The larger guess

The core that ends up working will be three things: sparse activity that stays stable;
wiring by co-activity, which needs no reward and builds reach and features; and a sharp,
sparse credit rule close to the output, fed by something that pools thinly spread signals.
In other words a sheet that organises itself, with reinforcement mostly at the edge.

Where most ideas will fail: teaching neurons deep inside the sheet from reward alone. That
is the hard, unsolved part. The find worth the whole exercise would be any rule that makes
inner neurons measurably better for a task than unsupervised wiring already makes them.

## 2026-11-24, on richness against credit

9. **On today's small tasks the binding problem is credit, not richness.** (60%)
10. **A frozen sheet with a learned readout already supports order-of-two-cues and
    both-but-not-either,** and supports more of them after a reward-free life and on a
    bigger sheet. (55%) If this holds, the owner's thesis (richness turns learning into
    selection) is carrying real weight; if a frozen sheet fails these however large it is,
    the missing piece is credit reaching the inside.
    - *Scored 2026-11-24: 9 looks right. 10 is wrong as written: a frozen sheet with our
      readout scores chance on all four feature tasks. But an ideal reader finds order,
      exclusive-or and gap in the tile next to the input at birth. The richness is there;
      the readout is what fails. The size half is untested.*

## 2026-11-24, before lose-shift and slow credit

11. **Lose-shift fixes the lock.** Making the motor cells that just earned a bad harder to
    fire for a while lifts the crossed shuffled-echo lives from 0.51 to above 0.8 and stops
    seed 3 locking onto one button. (55%)
    - *Scored: wrong. 0.561 against 0.559. It fixed seed 3, a tuning seed, and nothing on
      fresh ones.*
12. **A slow "fired recently" bit lets cartpole beat coin flips.** Credit that reaches back
    hundreds of ticks, at a lower chance, gets steps per fall above 25. (40%) It also lets
    something learn delayed echo. (45%)

## 2026-11-24, before growing the minimal engine

13. **One far slot gives the minimal engine reach.** Echo with cue and button 3 tiles apart
    goes from 0.37 (no better than its twin) to above 0.55. (55%)
14. **Delays let it learn delayed echo.** With slot delays of 1 to 8 ticks the minimal engine
    beats its twin on delayed echo by more than 0.15. (35%) More likely delays alone are not
    enough and it also needs credit that reaches back in time.
    - *Scored: 13 wrong (0.372 with a far slot against 0.370 without). 14 wrong on delayed
      echo as the suite defines it (0.23), right only when the ask follows the cue at once
      (0.63 against 0.37). The guess that delays alone would not be enough was right, but
      for a different reason than I gave: the trace is inside the credit window, and the
      rule still cannot use it.*

## 2026-11-24, before chasing the reversed sign

15. **The reversed sign is a bookkeeping slip, not a principle.** The cause will turn out to
    be which neurons count as "flipped by noise" or which way a quiet neuron is graded, and
    once fixed the rule will be ordinary: no better than the default. (60%) If instead the
    reversed rule keeps beating the default on shuffled echo in a second engine, it is a
    real finding about what noise spikes mean in a sheet with inhibition.
    - *Scored: right. A slip in the arithmetic. Reversed only works because it is the plain
      rule applied to a random sample; in the minimal engine reversing collapses it.*

## 2026-11-24, before the readout work

16. **A plain error rule at the motor cells learns order and exclusive-or on a frozen
    sheet** to above 0.75, with the motor cells within a tile of the cue. (55%)
17. **Spreading a button's motor cells through the sheet matters more than the rule.**
    (45%)
    - *Scored: both wrong. The plain error rule gets 0.53 and 0.57; it took evidence
      counters to reach 0.70 and 0.88. Spreading cells does nothing for the old rule and
      adds a little to the new one, mostly through their number.*

## 2026-11-24, before evidence counters

18. **Hidden evidence counters give the minimal engine reach.** With far slots on, Echo at 3
    tiles goes from 0.37 to above 0.6. (50%)
19. **They also raise how much it holds:** echo with 4 cues from 0.41 to above 0.6, and
    something above chance at 8 cues. (45%)
20. **They let it use the delay trace:** delayed echo above 0.6 with delays on. (35%)
    - *Scored: 18 wrong as stated, right in direction (0.54 to 0.60, every seed up). 19
      right with counters on every neuron (0.63; 8 cues above chance), wrong for motor
      cells only. 20 wrong: at chance among presses.*

## 2026-11-24, before decision-sized experiments and balanced reward

21. **Nudging a whole button now and then breaks the lock.** Crossed shuffled-echo lives go
    from about 0.55 to above 0.8 in at least two of the three engines. (50%)
22. **Balancing good and bad lets something beat a coin on cartpole.** At least one engine
    ends above 25 steps per fall under the lean reward. (35%)

## 2026-11-25, before reverse replay and more worlds

23. **Walking credit backward teaches the inside.** With the readout from idea 58, letting
    its error move counters on inner neurons along the recorded ticks beats the frozen
    sheet on order or exclusive-or by more than 0.05, and does not make reach worse. (40%)
24. **The new readout learns the bandit and the two-step chain, and still fails cartpole.**
    (60%)
25. **Longer lives alone do not rescue cartpole** for the old rules: a million steps ends
    no better than 40,000. (70%)

## 2026-11-25, before putting the winners together

26. **The combined engine keeps the reflex and the reach together:** Echo at one tile above
    0.9 and at three tiles above 0.8, in the minimal engine. (55%)
27. **It reads the features too:** exclusive-or above 0.8 and order above 0.65 with the sheet
    frozen. (50%) The minimal sheet has no delays or far links by default, so it may be a
    poorer sheet to read than the full one.
28. **It stays fast:** within 2x of the plain minimal engine's ticks per second in an Echo
    life. (60%)

