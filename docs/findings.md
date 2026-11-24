# Findings

What we think we know, and how well. This is the product: principles, with the evidence
behind each. A finding moves up a grade only when a second, different experiment agrees.

Grades: **solid** (two or more independent results), **likely** (one clean result),
**hint** (one result, few seeds or one engine).

## Principles that hold so far

1. **A neuron must pay for what it keeps.** [solid] A fixed budget of connections, where
   gaining one costs another, is the piece that carries learning in the minimal engine
   (Echo drops 0.17 to 0.23 without it and the sheet saturates). The full engine, which has
   no budget, ends with 87 to 95% of connections at top strength. In the table engine only
   the entry just used moves, which is the same discipline by another route, and it does not
   saturate either.
2. **Inhibition and each neuron keeping its own rate are what keep a sheet alive.** [solid]
   Removing either is the largest loss in the ablation table (-0.46, -0.32). The minimal
   engine stays on target for 4M ticks with only these two. Regulating by sector is not
   needed and is what causes the pulse.
3. **Wiring toward what fires together helps, and needs no reward.** [solid] It extends how
   far a cue can be read from 3 tiles to 4, and to 6 over a long life, with no reward at
   all; it is worth about 0.15 on Echo in the minimal engine and -0.34 when removed from
   the full one.
4. **But it must stop when nothing is plugged in.** [likely] Left running idle it rewires
   the sheet to noise and everything learned is lost (0.95 to 0.33). Gated on input being
   attached, nothing is lost (0.95 to 0.96).
5. **Memory that is specific to the situation holds more.** [likely] Table neurons, where a
   neuron stores a separate answer for each pattern of its inputs, learn four cues (0.68)
   where both popcount engines manage 0.31 to 0.41, and reach one tile further. The cost
   is no speed gain on a CPU.
6. **The sheet makes its own variety.** [likely] Switching injected noise off changes
   nothing in the full engine; rules that grade only injected-noise spikes lose in two
   engines out of three.
7. **Most of what we first built does nothing on reflex tasks.** [solid] Delays, the
   dendrite-branch bonus, varied neurons, far-link redraw and injected noise can each be
   removed with no measurable change, and a four-mechanism engine beats the
   twenty-one-mechanism one on Echo. Delays and far links cannot be judged yet: no task
   that needs them is learned by anything.

8. **The sheet is already rich; reading it is the hard part.** [likely] With nothing
   learned and no reward, order, exclusive-or and gap length can all be read from the
   neurons a tile or two from the input by an ideal reader (1.3 to 3.4, where 1.0 is easy).
   The same features score at chance when a frozen sheet's readout has to learn them. This
   is the owner's richness thesis, half confirmed: the features are there; selection is
   what we cannot yet do.
9. **Delays give a sense of time.** [likely] They do nothing measurable on Echo. On
   time features they are most of the story: gap length reads 0.43 with all delays at 1 and
   4.0 with delays of 4 to 8 ticks; order goes 1.5 to 5.4.

10. **Far links are what give reach.** [likely] The full engine learns a little with cue
    and button three tiles apart (0.58); with its far links removed it does not (0.45).
11. **More bits of evidence per thing learned helps.** [hint] Table neurons needed 4-bit
    counters to learn four cues (2-bit gave 0.56 on two). The popcount engines, with one or
    two bits per connection, hold less. Not yet tested head to head.

## Walls

- **Reading a thin signal. This is the main wall.** [solid: four independent results] An
  ideal reader gets order, exclusive-or and gap from the tile next to the input, and which
  cue is on from three tiles away; our rules get chance. Giving the minimal engine far
  slots puts the cue at the buttons three tiles away, and giving it delays keeps the cue's
  trace alive through a gap, and in both cases the score does not move. The information
  arrives, spread thinly over many neurons, and a rule that flips one-bit connections on
  single rewards cannot add it up.

- **Reach.** Learning works across one tile, sometimes two (table neurons), not three. The
  cue's identity is still readable at three or four tiles, so past one tile the failure is
  mostly credit.
- **Time.** Nothing learns delayed echo. Nothing connects a reward to anything older than
  a few ticks.
- **Locking on.** A sheet settles on one answer and keeps giving it while punished: Echo
  seed 3, crossed shuffled-echo lives, probably cartpole.
- **Acting in a world.** Nothing learns catch or cartpole. On cartpole learning makes the
  full engine worse than a coin.
- **More than four things.** Eight cues: nothing above chance, in any engine.

## Puzzles

- **Credit by noise learns exactly the wrong answer.** In the table engine it ends at 0.00
  on six seeds of eight, and with its sign reversed it scores 1.00 on shuffled echo on
  eight of eight, the best shuffled result of any rule. The full engine's noise rule also
  had lives at 0.01 to 0.07. A learner that is exactly wrong has found the signal; we do
  not know why the sign is backwards.
