# More worlds and long lives (idea 63, 2026-11-25)

The ladder jumped from Echo to cartpole, and lives of 40,000 steps are short by
reinforcement learning standards. This job adds the missing rungs, makes a life of a
million steps cheap, and runs every engine and rule up the ladder at both lengths.

## Short version

**Unfinished (stopped 2026-11-25 when agents were cut to four).** Measured: every world
at 40,000 steps, seeds 5-12, for the minimal engine, the full blob and the readout rule;
the table engine for 9 of 18 worlds (forage-cold on 5 seeds); a million steps only for
the minimal engine (16 worlds, seeds 5-8). Not run: the million-step lives of the full
blob, the table engine and the readout rule; the table learners on all seeds; the side
checks; the suite's new rows. How to finish: "Unfinished" at the end.

- **Built**: seven new worlds (bandit, switching bandit, chain with and without a stage
  cue, track shaped and arrival-only, forage warmer and arrival-only, mountain car and
  pendulum each with an energy and a goal reward) and three cartpole variants (wide
  starts, a relative reward, a semi-implicit step), all integer or 16.16, each checked with
  coin flips and a scripted player; episodes per life printed and in the summary;
  `--windows N` and `scripts/task.sh --long` (now `--million`); `scripts/ladder.py`; 14 suite rows after the
  old ones.
- **Cartpole as built cannot be learned by any rule we have, for a reason in the world, not
  the blob.** Gym's Euler step moves the pole by the spin from before the push, so a step's
  reward (lean, relative, alive, a fall) is exactly independent of that step's press
  (good 0.322 after the right press, 0.321 after the wrong one) and judges the press 20
  ticks earlier, outside every rule's trace. A plain table that learns from each step's
  own reward gets 24 steps per fall on it (coin 22); Q-learning, which carries credit one
  step back, gets 118. `--cart-integrator semi` (gym's other option) makes lean judge the
  press (0.40 against 0.20); the one-step table then doubles (49) and Q-learning gets 583.
- **Where each rule stops (40,000 steps)**: the minimal engine learns nothing on any new
  world, not even the two-armed bandit; only Echo-shaped halves (the cued chain's second
  press on 3 of 8 seeds). The table engine is the best bandit player (0.89, 0.96 with four
  arms, 0.93 switching) and learns forage with warmer/colder a little (0.49). The full blob
  learns the bandit on most seeds and locks on the wrong arm on others (1.00 to 0.01). The
  readout rule learns the bandit (0.77), the switching bandit (0.77), the cued chain's
  second step (0.72), forage with warmer/colder (0.64), mountain car with the energy
  reward (0.94 of episodes reach the goal; every other rule 0.00), the pendulum's swing
  (0.23 upright, coin 0.05) and a little catch (0.32, coin 0.25); on the semi-implicit
  cartpole it beats its twin (20.0 against 18.2) but not coin (20.7).
- **Nothing learns anything that needs credit across steps**: chain without a stage cue
  (every rule locks onto one button and scores about 0), track and forage paid only on
  arrival, mountain car and the pendulum paid only at the goal, cartpole on its Euler
  step. That is the missing rung: one step of credit back.
- **A million steps changed no verdict for the minimal engine** on any of 16 worlds (every
  late score within 0.07 of its 40,000-step score; nothing crossed coin). The other rules'
  long lives were not run.
- **The readout rule stops at a matching point on noisy rewards.** It moves only on bad,
  so pressing the best arm (bad 20%) and the other (bad 80%) balance at 0.8 of presses on
  the best; it ends at 0.77 from the first windows on and never improves (late - early
  -0.005). Echo has no such point because the right answer is never bad.

## The worlds

Definitions in full: `docs/curriculum.md`, "More worlds". One line each:

| world | what it is | score | random | coin | scripted |
|---|---|---|---|---|---|
| bandit2 | no cue; 1 of 2 buttons pays 80% good, the other 20% | share of steps on the best | 0.50 | 0.47 | 1.00 |
| bandit4 | the same with 4 buttons | as above | 0.25 | 0.23 | 1.00 |
| switch2 | bandit2, the paying button moves every 1,000 steps | as above, second half of each block | 0.50 | 0.47 | 1.00 |
| chain | press A then B (drawn per life); stage 0 never paid; 2 lines show the stage | 2 x chains / steps | 0.33 | 0.31 | 1.00 |
| chain-nocue | the same, stage not shown (best memoryless player 0.50) | as above | 0.33 | 0.31 | 1.00 |
| track | marker and target on a 5-cell line (5x2 bitmap); left, right; nearer good, further bad | share of steps ending nearer | 0.50 | 0.48 | 1.00 |
| track-arrive | the same, good only on arrival, bad after 5 steps without | as above | 0.50 | 0.47 | 1.00 |
| forage | 5x5 wrapped grid seen from the animal; 4 moves; nearer good, further bad | share of steps ending nearer | 0.43 | 0.39 | 1.00 |
| forage-cold | the same, good only on reaching the food, bad after 8 steps without | as above | 0.43 | 0.39 | 1.00 |
| mcar | gym mountain car, 16.16; energy rose good, fell bad, goal good | share of episodes at the goal | 0.00 | 0.00 | 1.00 |
| mcar-goal | the same, good at the goal, bad when 200 steps run out | as above | 0.00 | 0.00 | 1.00 |
| pend | gym pendulum swing-up, 16.16; upright good, leaving it bad, else energy gap shrank good | share of steps upright | 0.05 | 0.06 | 0.86 |
| pend-goal | the same, only the upright part | as above | 0.05 | 0.06 | 0.86 |
| catch | the suite's catch 4x4 | caught / dots | 0.25 | 0.25 | 1.00 |
| cartpole | the suite's cartpole, lean reward, place code | steps per fall | 22.1 | 23.3 | no fall |
| cartpole-wide | starts within 1 m, 0.5 m/s, 0.1 rad, 0.5 rad/s | steps per fall | 20.9 | 21.1 | no fall |
| cartpole-rel | good when nearer home than the recent mean, bad when further | steps per fall | 22.1 | 23.3 | no fall |
| cartpole-semi | lean reward, semi-implicit step (a step's reward judges its own press) | steps per fall | 20.8 | 20.7 | no fall |

Random: a uniform random press every step, straight on the task, 200,000 steps. Coin:
the stand-in's coin flips through the plug (ties are no move), seeds 5-12 of the
standard life. Scripted: the perfect player (`task_perfect_move`).

What the scripted player does: presses the paying button (bandits); A then B (chain);
steps toward the target (track, forage, along the further axis); pushes the way the car
is already moving (mountain car, goal in 129 steps on average); pumps energy in or out by
the sign of the spin and, within 0.6 rad of the top, pushes against lean plus 0.3 spin
(pendulum; 0.86 because each episode starts at a random angle and the first part is spent
swinging up); the linear push rule of the old cartpole player.

## Does a step's reward judge that step's press?

Every rule we have credits what was active just before a reward. That only works if the
reward of a step depends on the press of that step. Checked straight on each world with
a uniform random player, 200,000 steps (`out/worlds/rewards.cpp`): the share of good and
bad steps after a press that matches the scripted player's ("right") and after one that
does not, and the same split by the previous step's press.

| world | this press right: good / bad | wrong: good / bad | last press right: good / bad | wrong: good / bad |
|---|---|---|---|---|
| bandit2 | 0.80 / 0.20 | 0.20 / 0.80 | 0.50 / 0.50 | 0.50 / 0.50 |
| track | 1.00 / 0.00 | 0.00 / 0.60 | 0.50 / 0.46 | 0.50 / 0.14 |
| forage | 1.00 / 0.00 | 0.24 / 0.47 | 0.41 / 0.52 | 0.43 / 0.29 |
| mountain car | 0.995 / 0.005 | 0.04 / 0.96 | 0.50 / 0.50 | 0.54 / 0.46 |
| pendulum | 0.89 / 0.10 | 0.07 / 0.93 | 0.47 / 0.52 | 0.49 / 0.50 |
| catch 4x4 | 0.17 / 0.17 | 0.00 / 0.34 | 0.08 / 0.25 | 0.03 / 0.31 |
| **cartpole, lean** | **0.322 / 0.677** | **0.321 / 0.678** | 0.42 / 0.58 | 0.22 / 0.78 |
| cartpole, relative | 0.53 / 0.47 | 0.53 / 0.47 | 0.55 / 0.45 | 0.50 / 0.50 |
| cartpole, alive | 0.955 / 0.045 | 0.954 / 0.046 | 0.97 / 0.03 | 0.94 / 0.06 |
| cartpole, lean, semi-implicit | 0.40 / 0.60 | 0.20 / 0.80 | 0.41 / 0.59 | 0.20 / 0.80 |
| cartpole, relative, semi-implicit | 0.55 / 0.45 | 0.49 / 0.51 | 0.56 / 0.44 | 0.48 / 0.52 |

**On cartpole as built, a step's reward says nothing about that step's press.** The step
is gym's Euler step: the push changes the speeds, and the cart and pole move by the speeds
from before the push. So where the pole ends a step, which is all `lean` (and `relative`,
`alive`, a fall) looks at, is fixed before the press is made. The reward judges the press
one step earlier, which is 20 ticks back at the standard 20 ticks a step: outside the
readout rule's 15-tick trace, the full blob's 6-tick "fired recently", the table engine's
16 ticks and the minimal engine's 20-tick credit window. Every cartpole number so far graded the rules on noise.
`--cart-integrator semi` (gym's other option: speeds first, then positions by the new
speeds) makes lean judge this press (0.40 against 0.20, half the bandit's contrast).
`relative` is weak either way (0.53 against 0.49 semi): a running mean that lags makes
"better than usual" mostly a fact about where the swing is, not about the push.

## What a plain table learner does (reference)

Two plain table learners straight on each world, seeing exactly the lines the plug lights
(numbers as their place-code band), 10% random presses: **one-step** keeps the mean
immediate reward per (what is seen, button), so it uses only what each step's own reward
says, as every rule on the sheet does; **Q** is Q-learning (discount 0.9, step 0.1), which
carries credit back. Seed 5 only (the all-seeds run, `out/worlds/tabular_all.sh`, was not
reached). With 10% random presses the ceiling on share scores is about 0.95.

| world | one-step 40k | one-step 1M | Q 40k | Q 1M | states |
|---|---|---|---|---|---|
| bandit2 | 0.95 | 0.95 | 0.95 | 0.95 | 1 |
| bandit4 | 0.92 | 0.93 | 0.92 | 0.92 | 1 |
| switch2 | 0.95 | 0.95 | 0.95 | 0.95 | 1 |
| chain | 0.64 | 0.63 | 0.91 | 0.92 | 2 |
| chain-nocue | 0.09 | 0.09 | 0.11 | 0.11 | 1 |
| track | 0.95 | 0.95 | 0.95 | 0.95 | 20 |
| track-arrive | 0.93 | 0.88 | 0.95 | 0.95 | 20 |
| forage | 0.93 | 0.94 | 0.93 | 0.94 | 24 |
| forage-cold | 0.58 | 0.58 | 0.82 | 0.94 | 24 |
| mcar | 1.00 | 0.95 | 1.00 | 0.99 | about 52 |
| mcar-goal | 0.00 | 0.00 | 0.00 | 0.03 | about 50 |
| pend | 0.28 | 0.23 | 0.27 | 0.25 | about 60 |
| pend-goal | 0.05 | 0.06 | 0.15 | 0.25 | about 60 |
| catch | 0.48 | 0.47 | 0.88 | 0.89 | 48 |
| cartpole (lean, Euler) | 24.0 | 20.1 | 118 | 305 | 126-773 |
| cartpole-wide | 20.4 | 20.1 | 129 | 264 | |
| cartpole-rel | 14.6 | 16.2 | 98 | 69 | |
| cartpole-semi | 48.7 | 36.4 | 583 | 548 | |
| cartpole sparse | 25.7 | 13.5 | 92 | 127 | |

What this says about the worlds: the bandits, track, forage with warmer/colder and
mountain car with the energy reward are solved by a table of immediate-reward counts in a
few thousand steps; they need no credit across steps. The cued chain, arrival-only track
and forage, catch and every cartpole need it: the one-step table stays near where the
sheet rules are, and Q-learning is far above. The uncued chain needs memory of the last
press, which neither table has (a greedy table with one state presses one button forever,
which is also what the sheet rules do). The pendulum is hard for both tables at this
coarseness (8 bands of 45 degrees); the readout rule's 0.23 is near the one-step table's
0.28. Mountain car paid only at the goal is the classic exploration problem: Q-learning
reaches the goal in 3% of episodes after a million steps.

## The ladder

Late score (mean of the last tenth of the windows) over the seeds, the learning-off
twin's in brackets. `**`: above the twin with t >= 3 and above coin and twin by 0.03 (2
steps per fall on cartpole); `*`: above the twin with t >= 2; `-`: below the twin with
t <= -2. Coin is the last rule's coin in the row (the readout's 128 spread motor cells
shift it by up to 0.02); each rule's own is in `scripts/ladder.py table`. Empty cells
were not run (table engine, stopped). forage-cold for the table engine is 5 seeds.

40,000 steps, seeds 5-12:

| world | coin | minimal | table | blob | readout |
|---|---|---|---|---|---|
| bandit2 | 0.49 | 0.40 (0.38) | 0.89 (0.43) ** | 0.74 (0.47) ** | 0.77 (0.39) ** |
| bandit4 | 0.24 | 0.20 (0.19) | 0.96 (0.37) ** | 0.45 (0.22) * | 0.47 (0.20) ** |
| switch2 | 0.49 | 0.45 (0.38) * | 0.93 (0.47) ** | 0.52 (0.44) * | 0.77 (0.49) ** |
| chain | 0.33 | 0.30 (0.15) * | 0.09 (0.17) | 0.20 (0.19) | 0.72 (0.24) ** |
| chain-nocue | 0.33 | 0.13 (0.13) | 0.02 (0.12) - | 0.01 (0.19) - | 0.00 (0.28) - |
| track | 0.49 | 0.27 (0.28) | 0.02 (0.20) - | 0.20 (0.31) | 0.31 (0.38) - |
| track-arrive | 0.49 | 0.28 (0.27) | 0.35 (0.22) | 0.33 (0.34) | 0.49 (0.40) * |
| forage | 0.41 | 0.34 (0.33) | 0.49 (0.39) ** | 0.41 (0.38) * | 0.64 (0.38) ** |
| forage-cold | 0.41 | 0.33 (0.33) | 0.39 (0.39) | 0.37 (0.38) | 0.41 (0.38) * |
| mcar | 0.00 | 0.00 (0.00) |  | 0.00 (0.00) | 0.94 (0.00) ** |
| mcar-goal | 0.00 | 0.00 (0.00) |  | 0.00 (0.00) | 0.00 (0.01) |
| pend | 0.05 | 0.05 (0.05) |  | 0.07 (0.07) | 0.23 (0.06) ** |
| pend-goal | 0.05 | 0.05 (0.05) |  | 0.03 (0.06) - | 0.01 (0.06) - |
| catch | 0.25 | 0.25 (0.26) |  | 0.25 (0.24) * | 0.32 (0.25) ** |
| cartpole | 22.3 | 17.2 (18.6) - |  | 14.4 (19.5) - | 17.6 (19.4) |
| cartpole-wide | 20.8 | 16.4 (17.3) - |  | 13.7 (18.2) - | 16.0 (17.4) - |
| cartpole-rel | 22.3 | 17.2 (19.0) - |  | 12.5 (19.1) - | 16.6 (19.2) - |
| cartpole-semi | 20.7 | 16.4 (17.4) | 15.5 (12.5) | 13.6 (18.4) - | 20.0 (18.2) * |

Per seed the full blob is bimodal where it learns: bandit2 1.00, 0.72, 0.01, 1.00, 0.54,
1.00, 0.68, 0.98; chain 0.00, 0.96, 0.00, 0.00, 0.34, 0.03, 0.24, 0.00. One full-blob life
learned the whole chain (0.96); the others lock. The minimal engine's chain is 0.59, 0.50
and 0.65 on three seeds and the twin's level on five: those three learned the cued second
press (with stage 0 at chance the score is about 0.6), not the chain.

Where each stops on the ladder (40,000 steps):

| rule | learns | stops at |
|---|---|---|
| minimal | (the cued half of chain on 3 of 8 seeds) | the bandit |
| table | bandits (to 0.96), switching bandit, forage warmer a little | chain, track (locks: 0.02); 9 worlds not run |
| blob | bandit on most seeds, forage warmer and catch barely | locks; mountain car, pendulum nothing; cartpole worse than coin |
| readout | bandits (matching point), switching, cued chain's second step, forage warmer, mountain car energy, pendulum swing, catch a little | anything paid later than the step: uncued chain, arrival-only, goal-only, cartpole |

## Long lives

Only the minimal engine's million-step lives were run (seeds 5-8, 100 windows of 10,000
steps; the twin and coin are the 40,000-step life's on the same seed, since nothing
changes with learning off):

| world | coin | minimal, 1M steps |
|---|---|---|
| bandit2  |  0.47  |  0.41 (0.41) |
| bandit4  |  0.23  |  0.21 (0.20) |
| switch2  |  0.47  |  0.43 (0.38) * |
| chain  |  0.31  |  0.37 (0.15) * |
| chain-nocue  |  0.31  |  0.13 (0.13) |
| track  |  0.47  |  0.28 (0.27) |
| track-arrive  |  0.47  |  0.28 (0.27) * |
| forage  |  0.39  |  0.34 (0.33) * |
| forage-cold  |  0.39  |  0.32 (0.33) |
| mcar  |  0.00  |  0.00 (0.00) |
| pend  |  0.07  |  0.05 (0.05) |
| catch  |  0.25  |  0.26 (0.25) |
| cartpole  |  23.5  |  17.4 (18.5) - |
| cartpole-wide  |  20.4  |  16.5 (17.0) |
| cartpole-rel  |  23.5  |  17.2 (18.7) |
| cartpole-semi  |  21.0  |  16.4 (17.4) |

No verdict changed. Episodes per life at a million steps (minimal engine): cartpole
57,754 falls (about 17 steps a fall), catch 333,333 dots, mountain car 5,000 episodes,
track 46,708 targets: far more episodes than RL papers report for cartpole, with no change
in score. For the minimal engine, data was not what was missing. Whether the readout
rule's cartpole-semi (twin +1.8 at 40,000, late minus early +1.6) keeps rising over a
million steps is the most useful unrun life.

## Predictions 24 and 25

- **24 ("the new readout learns the bandit and the two-step chain, and still fails
  cartpole", 60%)**: half right. Bandit: right (0.77, twin 0.39; though it stops at a
  matching point, not near 1). Chain: wrong as meant. It learns the cued chain's second
  press (0.72), which is an Echo; it learns nothing of the first press, which is the
  credit-across-two-decisions part, and without the stage cue it scores 0.00. Cartpole:
  right (17.6 against coin 22.3 on the Euler step; 20.0 against 20.7 on the semi-implicit
  one).
- **25 ("longer lives alone do not rescue cartpole for the old rules", 70%)**: right for
  the minimal engine (17.4 at a million steps against 17.2 at 40,000, coin 23.5, on 4
  seeds); unclear for the full blob and the table engine, whose long lives were not run.

## Reading

- **The owner's question, "are we using enough data for cartpole?"**: not the problem
  first. Cartpole as built hands every rule a reward about the press before the one it is
  grading. A million steps (58,000 falls) did nothing for the minimal engine, and the
  plain one-step table, which is what our rules amount to, gets coin on it at any length.
- **The missing rung is credit one step back.** Every world that a one-step table solves,
  at least one sheet rule learns (mostly the readout rule). Every world that needs credit
  across steps, nothing learns: the cued chain's first press, arrival-only track and
  forage, goal-only mountain car and pendulum, catch beyond a little, Euler cartpole.
  Idea 62 (credit walks backward) and any trace that spans one task step (20 ticks, not
  6 or 15) are the direct tests; the chain is the smallest world for them.
- **Rules that move only on bad have two traps the new worlds expose.** A noisy reward
  leaves them at a matching point (bandit 0.77). A press that gets no reward at all (into
  the end of the track, stage 0 of the chain, the uncued chain stuck on one button) never
  moves them, so they can sit on it forever (chain-nocue 0.00, track 0.31 below its own
  twin). The full blob, which strengthens on good, locks instead (bandit 0.01 on one seed).
- **A no-cue bandit is a different skill from Echo for the minimal engine.** It learns
  Echo at 0.94 and the bandit not at all (0.40, coin 0.47). A guess, not tested: a neuron
  on a fixed budget can move weight between inputs (selection, which is Echo) but not
  raise its own total drive (a bias, which is the bandit).
- **The readout rule is the only rule that learns a control task**: mountain car with the
  energy reward, 0.94 of episodes at the goal from 0.00. It is the shape it can do: the
  right push is the sign of the speed, and the energy reward judges the press of that step.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, wall "Acting in a world": cartpole's Euler step makes every cartpole
  reward judge the previous press (measured: good 0.322 vs 0.321 by this press), so no
  immediate-credit rule could have learned it; with the semi-implicit step the readout
  beats its twin (20.0 vs 18.2) but not coin (20.7). The readout rule learns mountain car
  with an energy reward (0.94 at the goal, every other rule 0.00) and the pendulum's swing
  (0.23 vs 0.05). A million steps changed nothing for the minimal engine on 16 worlds.
- `docs/findings.md`, wall "Time": nothing learns a world that needs credit one step back
  (the cued chain's first press, arrival-only track and forage, goal-only mountain car and
  pendulum); a one-step table fails these too and Q-learning solves them.
- `docs/findings.md`, wall "Locking on": the error rule has its own traps: a press never
  punished is never left (uncued chain 0.00, track below its twin), and on noisy rewards it
  settles at a matching point (bandit 0.77, not about 1).
- `docs/findings.md`, principle 5 (situation-specific memory): the table engine is also the
  best bandit learner (0.89, 0.96 with four arms, 0.93 switching): a second result.
- `docs/ideas.md`, idea 63: built; results here; unfinished runs queued (below).
  cartpole-semi should replace the Euler cartpole as the main cartpole row. The cued chain
  is the smallest test for idea 62 and for a trace spanning a whole task step. New idea:
  let the readout rule move on good too when rewards are noisy (`--g.readout_good 1`; the
  side check was queued, not run).
- `docs/predictions.md`: 24 half right, 25 right for the minimal engine (texts above).
- `docs/task-map.md` tier 7: "best so far" from the 40,000-step table above.

## Script lines

```
cmake -S . -B build && nice -n 19 cmake --build build -j4
scripts/task.sh bandit                         one world, one life, beside twin, coin, scripted
scripts/task.sh cartpole --cart-integrator semi --million --engine minimal  a million steps
python3 scripts/ladder.py run --rules minimal              every world, seeds 5-12, 40,000 steps
python3 scripts/ladder.py run --rules readout --long --seeds 5-8 --worlds cartpole,catch
python3 scripts/ladder.py table                            the tables below
scripts/suite.sh NAME --only worlds                        only the suite's new rows
```

The ladder in this doc was run as `python3 scripts/ladder.py run --rules R [--long
--seeds 5-8] --worlds ... --out-dir out/worlds/ladder --blobrun <a frozen copy of
build/blobrun>`, two at a time under `nice -n 19`, rules minimal, table, blob and readout
(`--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1
--motor-spread 128`), each world's flags as in `scripts/ladder.py` (`WORLDS`). Every life:
`--ticks 20 --sensor-side 3 --motor-side 3` (2x2 sensor groups and the place code for
forage, mountain car, pendulum and cartpole), 40,000 steps in windows of 1,000, or
1,000,000 in 100 windows. The throwaway checks are in `out/worlds/` (not committed):
`rewards.cpp` (the timing table), `tabular.cpp` and `tabular_all.sh` (the table
learners), `side.sh` (side checks), each built with
`g++ -std=c++20 -O2 -Isrc out/worlds/X.cpp src/run/run_flags.cpp src/world/task*.cpp`.

## Rough

- The run was stopped part way (the owner cut the number of agents). See "Unfinished".
- The machine was loaded (load average about 32 on 32 cores) for most of the runs; no speed
  numbers are taken here.
- Track and forage scores count steps that ended nearer, so a rule that presses into the
  end of the track scores low even when it reaches many targets (the readout on track
  seed 5 reached 6,395 targets against coin's 3,922, yet scores 0.36 against 0.49).
- The pendulum's energy reward has a dead band of 0.01 and the upright band is 0.4 rad; the
  scripted player gets 0.86, not 1.0, and both table learners only about 0.27, so at 8
  lines per number it may be too coarse to be a fair rung.
- `relative` cartpole judges the press weakly even on the semi-implicit step (0.55 against
  0.49): a lagging mean makes "better than usual" mostly about where the swing is.
- The idea-61 balanced-reward plug option from the other agent had not landed on main, so
  it was not tried on cartpole.
- The viewer shows the new worlds with generic labels ("motor 0..3", "number 0"); naming
  them is a `src/view/sim_task.cpp` change for the viewer owner. Checked with `--capture`
  (lavapipe) for forage and pendulum.

## Unfinished, and how to resume

Raw outputs: `out/worlds/ladder/standard/` and `out/worlds/ladder/long/` (one csv, -sum.csv
and .txt per world, rule and seed); `out/worlds/combined.py` prints the tables above.
`ladder.py run` skips every call already finished, so each line below only does what is
missing:

```
python3 scripts/ladder.py run --rules table --out-dir out/worlds/ladder
python3 scripts/ladder.py run --rules readout --long --seeds 5-8 --worlds cartpole,cartpole-semi,catch --out-dir out/worlds/ladder
python3 scripts/ladder.py run --rules blob,table --long --seeds 5-8 --worlds cartpole,cartpole-semi --out-dir out/worlds/ladder
python3 scripts/ladder.py run --rules readout --long --seeds 5-8 --worlds mcar,pend,bandit2,chain,forage,track,switch2 --out-dir out/worlds/ladder
python3 scripts/ladder.py run --rules blob,table --long --seeds 5-8 --worlds catch,bandit2 --out-dir out/worlds/ladder
bash out/worlds/tabular_all.sh > out/worlds/tabular_all.txt
bash out/worlds/side.sh bandit2-good1 --task bandit --g.readout_good 1
bash out/worlds/side.sh bandit2-sure --task bandit --bandit-good 100 --bandit-poor 0
scripts/suite.sh worlds-check --quick --only worlds
```

(`side.sh` uses the frozen binary `out/worlds/blobrun-ladder2`; the ladder lines default to
`build/blobrun`.) The suite's 14 new rows are 14 more standard lives per seed beside about
28 in the old suite, so a full suite takes about half as long again (from the 40,000-step
run times here: about 35 more minutes for the full blob, 5 for the minimal engine, an hour
for the table engine, at 2 jobs). The new rows were not run through the suite itself.
