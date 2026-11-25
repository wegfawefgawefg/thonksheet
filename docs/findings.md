# Findings

What we think we know, and how well. This is the product: principles, with the evidence
behind each. A finding moves up a grade only when a second, different experiment agrees.

Grades: **solid** (two or more independent results), **likely** (one clean result),
**hint** (one result, few seeds or one engine).

## The headline (2026-11-25)

**The output can now learn online, from good and bad alone, things no input line carries.**
In a test with the inside of the sheet held fixed as born (a way to find where learning
happens, not a goal), a rule at the motor cells learns exclusive-or of two cues (0.88), which cue came first (0.70,
0.84 with long delays), Echo with the cue three tiles from the button (0.85), four cues
(1.00) and eight (0.84), and shuffled Echo with no locked lives (1.00 on eight seeds of
eight). The same sheet with the old readout scored chance on all of these. The owner's
thesis, that a rich enough sheet turns learning into selection, holds for everything a
plain linear reader can get out of the sheet. docs/results-2026-11-24-readout.md

**What this is not.** The goal is a brain that learns everywhere, all the time, on any task
(the owner, 2026-11-25: "i didnt say it needs to be never taught ... the whole point is it
should be able to learn new things right? run nonstop. online learning for any task"). The
readout does learn that way: it never stops, it picks up a second task without losing the
first. The inside does not yet. Held fixed, it can only offer what it was born with, which
is why a shape in a new place, a long gap and sixteen cues fail. Letting the old inner rule
run alongside helps reach and hurts the feature tasks. Making the inside learn usefully,
online, is the open problem and the next target.

The full suite on that rule (`blob-readout` in docs/scoreboard.md, 8 seeds, sheet frozen)
agrees and adds: a second task is learned after the first (0.99) with the first still there
(0.98 at once); eight cue-button pairs held, and sixteen at 0.31 where chance is 0.06;
nothing lost after being unplugged (1.00 before, 1.00 after); Echo five tiles away 0.59;
catch 0.32 where chance is 0.25. Still failing: delayed echo (0.55), beat, gap (0.52),
cartpole (below a coin), and a shape in a place it was never rewarded (0.25, below chance).

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
   is no speed gain on a CPU. In the suite they are also the only engine that learns a
   second task after the first (two new cues and buttons: 0.96, against 0.13 and 0.11) and
   still has the first afterwards (0.61 at once, 0.90 after a short retest).
6. **The sheet makes its own variety.** [likely] Switching injected noise off changes
   nothing in the full engine. Grading only injected-noise spikes learns Echo in the minimal
   engine (0.90 when built correctly) but nothing in the table engine.
7. **Most of what we first built does nothing on reflex tasks.** [solid] Delays, the
   dendrite-branch bonus, varied neurons, far-link redraw and injected noise can each be
   removed with no measurable change, and a four-mechanism engine beats the
   twenty-one-mechanism one on Echo. Delays and far links cannot be judged yet: no task
   that needs them is learned by anything.

8. **The sheet is already rich; reading it is the hard part.** [solid: four seeds, tight] With nothing
   learned and no reward, order, exclusive-or and gap length can all be read from the
   neurons a tile or two from the input by an ideal reader (1.3 to 3.4, where 1.0 is easy).
   The same features score at chance when a frozen sheet's readout has to learn them. This
   is the owner's richness thesis, half confirmed: the features are there; selection is
   what we cannot yet do.
9. **Delays give a sense of time.** [solid: four seeds, tight] They do nothing measurable on Echo. On
   time features they are most of the story: gap length reads 0.43 with all delays at 1 and
   4.0 with delays of 4 to 8 ticks; order goes 1.5 to 5.4.

10. **Far links are what give reach.** [likely] The full engine learns a little with cue
    and button three tiles apart (0.58); with its far links removed it does not (0.45).
11. **More bits of evidence per thing learned helps.** [hint] Table neurons needed 4-bit
    counters to learn four cues (2-bit gave 0.56 on two). The popcount engines, with one or
    two bits per connection, hold less. Not yet tested head to head.

12. **One neuron's experiment almost never changes the answer.** [likely] In the table
    engine only 1 graded noise flip in 2,500 changed which button won. Those that did all
    moved their counters the right way, and grading only them reaches 1.00 on the seeds
    where a single flip can matter. So credit from single-neuron noise is correct but far
    too rare; an experiment has to be the size of a decision.
13. **A rule that grades against its own average must count everything it averaged over.**
    [solid] Grading only the neurons noise flipped is half of the textbook rule. The
    missing half (the unflipped neurons, the other way) is thousands of times larger than
    the real signal, so the rule as first built learned the exact opposite (0.00 on six
    seeds of eight; 1.00 with its sign reversed). This was a slip, not a principle.

14. **Evidence before commitment.** [solid for reach and for holding more: two engines] A connection that
    flips on one reward cannot add up a thin signal. Give each possible source a hidden
    counter (6 bits), move it a step on each reward, and wire the sources with the most
    evidence. On a frozen sheet this takes exclusive-or from 0.57 to 0.79 and Echo at three
    tiles from 0.57 to 0.74 over the same rule without counters, and it keeps improving at
    the end of a life where the plain rule has stopped.
    The second test, in the minimal engine with the whole sheet learning: Echo at three
    tiles 0.39 to 0.60, four cues 0.41 to 0.63, exclusive-or 0.26 to 0.58, eight cues above
    chance for the first time in that engine. Two limits. The credit feeding the counters
    has to be broad (every neuron that spiked; noise-only credit does nothing). And without
    the error rule of finding 15 it costs the plain reflex 0.2 (0.94 to 0.73). Width is not
    the lever (6 bits best, 4 close, 8 worse); summing evidence at all is.
15. **The output must know which action was taken.** [likely] The rule that works is an
    error rule: on bad, the cells of the button that was pressed lose their recently active
    inputs and the cells of the other buttons gain them; on good nothing is strengthened.
    A cell that only knows whether it fired itself gets roughly half the benefit. The
    world has to tell the motor cells which button won. That is one bit per cell, and it
    is the one part of the rule that is not strictly local.
16. **A decision needs many cells.** [likely] 32 or more motor cells per button, instead of
    9, adds 0.02 to 0.13. It is the count that matters, to beat spike noise, more than
    where the cells sit.
17. **Do not strengthen your own answer on good.** [likely] The rule above never does, and
    it has no locked lives: shuffled Echo is 1.00 on every seed, crossed ones included,
    where the old rule averaged 0.56.

18. **Size does not break it, so far.** [hint] Echo scores the same with the sheet grown 16
    times around the same task (minimal engine 0.94, 0.93, 0.92 at 4,096, 16,384 and 65,536
    neurons). Three seeds, one task, input and output areas unchanged.

19. **Counters inside the sheet help it hold more.** [hint] In the minimal engine, counters
    on the motor cells alone leave four cues at 0.39; counters on every neuron lift it to
    0.63, every seed up. This is the first sign of a reward rule making neurons inside the
    sheet more useful. It costs memory (82 to 493 bytes per neuron) and reward-time work.

20. **Richness can be tuned, and ours was not.** [likely] Among 600 random sheets the best
    read order and gap three to four times better than the default and exclusive-or up to
    twice as well, on fresh seeds. No single number does it (best rank correlation 0.3):
    longer delays for time, denser local wiring, the dendrite-branch bonus for
    exclusive-or, more inhibitory neurons. Whether richer sheets learn better through the
    readout is being tested.
21. **A mechanism can only be judged on a task that needs it, through a readout that can
    read.** [solid] Delays, far links and the branch bonus all measured as doing nothing on
    Echo. Delays turned out to carry time, far links reach, and the branch bonus
    exclusive-or.

## Walls

- **Reading a thin signal: broken at the output, standing inside.** An error rule moving
  evidence counters at the motor cells reads most of what a linear reader can (see the
  headline). It has not been tried on neurons inside the sheet. When the whole sheet learns
  alongside it, reach gets better (0.97 at three tiles) and the feature tasks get worse
  (exclusive-or 0.88 down to 0.63): the old inner rule fights the new readout.
- **Reach.** Three tiles is now learned (0.85 frozen, 0.97 with the sheet learning). Past
  about five tiles nothing arrives to be read.
- **Time.** Nothing learns delayed echo, now also with evidence counters and with delays
  on: answers rise only because it presses more, and among presses it is at chance. Order
  is learned by the new readout (0.70 to 0.84); a gap in time before the question is not.
- **Locking on.** A sheet settles on one answer and keeps giving it while punished: Echo
  seed 3 and crossed shuffled-echo lives. The cause is the reward rule strengthening its
  own answer: making that stronger (win-stay) deepens the lock (0.56 to 0.14), and making
  the last answer harder to give (lose-shift) only trades stuck lives for unstable ones.
  Crossed lives need a two-tile path, so this is the thin-signal wall again. Cartpole is
  not a lock: the blob leans to one side (0.58 of presses) without being stuck.
  Any rule graded against its own average also freezes once every reward equals the
  average: an all-wrong life stops moving.
- **A new place.** A shape learned in some places is answered wrongly in a place never
  rewarded (0.25 where chance is 0.49). Nothing in the sheet shares what one patch learned
  with another. This is the missing prior the owner started from on day one (a convolution
  shares; this sheet does not).
- **Acting in a world.** Nothing learns catch or cartpole. On cartpole learning makes the
  full engine worse than a coin.
- **More than four things.** Eight cues are now learned (0.84) by the new readout on the
  full engine. Sixteen is untested with it.
