# Actor and critic (2026-11-25)

Barto, Sutton and Anderson (1983) balanced a pole with two neuron-like elements: an actor
told only by a critic that learned to predict how well things were going and broadcast the
change in its prediction. This job gives the sheet a critic in its own terms and tries it on
the acting worlds. It was built and tuned by a first agent (seeds 1-4) and finished by a
second: three faults fixed, the improvements tried, and the report on seeds 5-12 over every
world of `scripts/ladder.py`, beside the readout alone, the GRU, the memory engine, the
learning-off twin, coin flips and the scripted player.

## Short version

- **The one setting**, unchanged across all 17 worlds:
  `--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 5 --g.readout_balance 1
  --motor-spread 128 --g.critic_on 1 --g.readout_good 1` (the readout's rule, taught by the
  critic's surprise instead of raw good and bad, its good half on, the actor at 2^-5 a
  spike). The first agent's setting had `readout_shift 4`; halving the actor's chance was
  better or equal on every tuning world and was kept (below).
- **Report, seeds 5-12, 40,000 steps: the critic beats its twin on 15 of 17 worlds, and
  scores above the readout alone on 15 (below it only on the pendulum's energy reward).** It learns both bandits (1.00), the switching bandit (0.98, GRU
  0.79), the cued chain (0.99, GRU 0.01), track (0.88), track paid only on arrival (0.78),
  forage (0.84) and forage paid only on arrival (0.78, above the GRU's 0.62 and the memory
  engine's 0.70), mountain car with the energy reward (0.97), catch a little (0.39, GRU
  0.72, memory 0.99). It fails the chain with no stage cue (0.09), mountain car paid only
  at the goal (0.00) and the pendulum paid only upright (0.08, coin 0.05); the pendulum's
  energy reward is the one world where it is worse than the readout alone (0.16 against
  0.23).
- **Cartpole does not balance.** Steps per fall at 40,000 steps: lean 109, alive 30, sparse
  28, against coin 21 and twin 18; the GRU gets 875, 127 and 66, the memory engine 302, 10
  and 281, the scripted player never falls. Over 400,000 steps the lean reward rises to
  about 110 within the first 40,000-80,000 steps and stays there (ten slices below):
  more data does not take it further.
- **The critic knows how things are going.** Its level orders by steps to the next fall on
  all three cartpole rewards (rank correlation +0.21 to +0.33; the twin's same cells -0.11
  to 0.00), by the chain's stage, by mountain car's position (+0.40, +0.44; twin -0.13,
  +0.11), and by steps to the next good on arrival-only track and forage and the upright-
  only pendulum.
- **On cartpole the surprise, not the actor, is the limit** (a diagnostic, not a
  candidate: the world told the actor whether each press was the scripted player's).
  Driven by that clean verdict through the same actor, the sheet balances: 865 steps per
  fall (the readout alone, told it as a one-step reward: 1000, every seed); through the
  critic's own surprise made from the clean verdict, 601; from the lean reward, 89. So a
  linear readout of the sheet can represent balancing; what it is told is too thin.
- **Credit one step back**, the rung the more-worlds job found missing everywhere: the
  critic gets it on the cued chain, arrival-only track and arrival-only forage. It does not
  on mountain car paid at the goal (the critic never sees a reward: no goal is reached in
  the logged 10,000 steps), the pendulum paid upright (the level does rise before a good,
  but the reader cannot represent a swing-up: told the right press every step it still
  gets 0.08-0.14), or the uncued chain (not a credit
  problem: the stage is not seen and nothing holds the last press).
- **Not kept**: the wide critic (768 cells), the margin gate on the actor's good half, and
  a shorter actor trace. Each cost the bandit or the chain, and cartpole too.
- **Cost**: with the critic a life runs at about 12,000-13,000 ticks a second here against
  23,000-53,000 for the readout alone (loaded machine, rough): the step's end is now the
  larger part of the work.

## The three faults fixed

1. `--g.critic_on` with another engine: `src/run/life.cpp` gave the every-step reward on
   `real_blob(driver)`, true for the minimal, memory, GRU and vm engines too, which were
   then told valence 0 every step (the minimal engine scores 0 as bad). Now it asks
   `core_blob(driver)` (`src/run/main.cpp`: the full engine or its twin), and `blobrun`
   refuses `critic_on` with `--engine minimal|memory|mlp|gru|vm` and `--table`. The viewer
   (`src/view/sim_task.cpp`) has the same condition on `critic_on` alone; not edited here.
2. `--long` lives (`src/run/life_long.cpp`) did not give the critic its every-step reward.
   They do now, and mark the vote as `life.cpp` does. A 6,000-step `--long` chain life with
   the critic went 0.02 ... 0.00 before; now it gives the plain life's windows (0.83 ...
   0.93). `--long` pairs lives with the critic off are byte-identical to before.
3. With everything off the four reference lives (`out/merge-ref`: echo; both-not-either,
   mode 5; minimal; table) are byte-identical after every change here and after merging
   main.

## The report (seeds 5-12, 40,000 steps)

Late score (mean of the last 4 windows of 1,000 steps) over 8 seeds. `**`: above the
critic's own twin with t >= 3; `*` t >= 2; `-` below with t <= -2. Twin, coin and scripted
are from the critic's calls (each rule's own twin is within 0.03 of it). Every life:
`--ticks 20 --sensor-side 3 --motor-side 3` (2x2 sensor groups and the place code for
forage, mountain car, pendulum and cartpole), the worlds as in `scripts/ladder.py`. GRU and
memory are the fixed references on their own plug (no spread motor cells). On cartpole the
score is steps per fall, a window with no fall scoring 1,000; the scripted player never
falls.

| world | critic | twin | coin | readout alone | GRU | memory | scripted |
|---|---|---|---|---|---|---|---|
| bandit2 | 1.00 ** | 0.39 | 0.49 | 0.77 | 1.00 | 0.99 | 1.00 |
| bandit4 | 1.00 ** | 0.20 | 0.24 | 0.47 | 1.00 | 0.98 | 1.00 |
| switch2 | 0.98 ** | 0.49 | 0.49 | 0.77 | 0.79 | 0.99 | 1.00 |
| chain | 0.99 ** | 0.24 | 0.33 | 0.72 | 0.01 | 0.00 | 1.00 |
| chain-nocue | 0.09 - | 0.28 | 0.33 | 0.00 | 0.01 | 0.00 | 1.00 |
| track | 0.88 ** | 0.38 | 0.49 | 0.31 | 0.99 | 0.00 | 1.00 |
| track-arrive | 0.78 ** | 0.40 | 0.49 | 0.49 | 0.89 | 0.98 | 1.00 |
| forage | 0.84 ** | 0.38 | 0.41 | 0.64 | 0.98 | 0.77 | 1.00 |
| forage-cold | 0.78 ** | 0.38 | 0.41 | 0.41 | 0.62 | 0.70 | 1.00 |
| mcar | 0.97 ** | 0.00 | 0.00 | 0.94 | 1.00 | 1.00 | 1.00 |
| mcar-goal | 0.00 | 0.01 | 0.00 | 0.00 | 0.00 | 0.06 | 1.00 |
| pend | 0.16 ** | 0.06 | 0.05 | 0.23 | 0.22 | 0.23 | 0.86 |
| pend-goal | 0.08 * | 0.06 | 0.05 | 0.01 | 0.04 | 0.07 | 0.86 |
| catch | 0.39 ** | 0.25 | 0.25 | 0.32 | 0.72 | 0.99 | 1.00 |
| cartpole (lean) | 109 ** | 18.2 | 20.7 | 20.0 | 875 | 302 | no fall |
| cartpole-alive | 30.2 ** | 17.7 | 20.7 | 21.4 | 127 | 9.8 | no fall |
| cartpole-sparse | 27.8 ** | 18.1 | 20.7 | 22.3 | 66 | 281 | no fall |

Per seed, critic: cartpole lean 102, 88, 112, 160, 107, 177, 17, 107 (seed 11 never
learned); alive 24-34; sparse 23-35; catch 0.34-0.42; chain 0.98-0.99 on every seed;
forage-cold 0.65-0.83; chain-nocue 0.03-0.25.

Reading it:

- **Where the critic is the best thing here**: the cued chain (0.99; the GRU and the memory
  engine 0.00-0.01, the readout 0.72), forage paid only on arrival (0.78 against 0.62 and
  0.70), and the switching bandit beside the GRU (0.98 against 0.79; memory 0.99).
- **Where it lifts the sheet from nothing to most of the way**: track (0.31 to 0.88; the
  readout sat below its twin), arrival-only track (0.49 to 0.78), forage (0.64 to 0.84), the
  bandits' matching point is gone (0.77 to 1.00; 0.47 to 1.00 with four arms).
- **Where it is far behind the references**: catch (0.39; GRU 0.72, memory 0.99) and every
  cartpole (lean 109 against 875 and 302). The memory engine's twin and score of 0.00 on
  chain, chain-nocue and track are real: it pressed nothing there (0-2 episodes a life).
- **Where it costs**: the pendulum with the energy reward, 0.16 against the readout's 0.23
  (all three references about 0.22). The uncued chain is below its twin (0.09 against 0.28):
  it settles on one button, as every rule here does.

Which worlds still rise at the end of 40,000 steps (mean of windows 31-40 minus 21-30, over
the seeds): cartpole lean +44 (t 2.1), catch +0.029 (t 3.7), forage +0.014 (t 6.7). Not
rising: every other world, cartpole alive (-0.7) and sparse (-1.0) included. Those three
got the long lives.

## Long lives (seeds 5-12, 400,000 steps, ten slices)

`--steps 400000 --window 1000 --no-baselines`; the worlds still rising at 40,000, plus the
references on all four cartpole and catch worlds. Mean of the windows in each tenth of the
life (40,000 steps), over 8 seeds; the last column is the late score. Twin and coin as at
40,000 steps (learning off does not change).

| world | rule | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| cartpole lean | critic | 66 | 116 | 119 | 112 | 110 | 117 | 116 | 116 | 118 | 109 |
| cartpole lean | readout alone | 18.5 | 20.5 | 20.8 | 20.8 | 20.9 | 21.0 | 20.9 | 20.8 | 20.7 | 20.9 |
| cartpole lean | GRU | 615 | 919 | 932 | 924 | 925 | 891 | 865 | 825 | 835 | 895 |
| cartpole lean | memory | 270 | 282 | 279 | 286 | 302 | 289 | 306 | 280 | 283 | 307 |
| cartpole alive | GRU | 92 | 127 | 113 | 109 | 113 | 133 | 120 | 98 | 111 | 94 |
| cartpole alive | memory | 9.6 | 14.7 | 17.9 | 20.1 | 25.0 | 25.8 | 29.5 | 31.1 | 31.1 | 31.9 |
| cartpole sparse | GRU | 46 | 98 | 152 | 145 | 140 | 130 | 151 | 125 | 124 | 146 |
| cartpole sparse | memory | 241 | 267 | 273 | 270 | 282 | 268 | 283 | 281 | 262 | 268 |
| catch | critic | 0.34 | 0.41 | 0.42 | 0.42 | 0.42 | 0.42 | 0.42 | 0.41 | 0.40 | 0.41 |
| catch | GRU | 0.68 | 0.73 | 0.74 | 0.73 | 0.72 | 0.72 | 0.70 | 0.69 | 0.68 | 0.68 |
| catch | memory | 0.99 | 0.99 | 0.99 | 0.99 | 0.99 | 0.99 | 0.99 | 0.99 | 0.99 | 0.99 |
| forage | critic | 0.79 | 0.86 | 0.86 | 0.86 | 0.87 | 0.87 | 0.87 | 0.87 | 0.87 | 0.87 |

- **Cartpole lean: the critic levels off at about 110-120 steps per fall after the first
  80,000 steps and stays there** for the remaining 320,000 (6,066 falls a life). Per seed
  at the end: 85, 92, 126, 116, 104, 131, 101, 120; seed 11, which had not learned by
  40,000 steps (17), got there later. It does not balance: against the GRU's 895 and the
  memory engine's 307 it is a factor of 8 and 3 short, and against coin's 21 it is 5 times
  better. The readout alone stays at coin (20.9) for the whole life.
- Catch levels at 0.41-0.42 by the second slice (GRU 0.68, memory 0.99). Forage at 0.87.
- Alive and sparse were not rising at 40,000 and got no long critic life. The references'
  long lives on them: GRU 94 and 146, memory 32 and 268.
- So for the critic, as for the minimal engine in the more-worlds job, more data is not
  what is missing.

## Does the critic know anything

`scripts/critic.sh track WORLD --seed S` (`blobprobe critic`, the last 10,000 steps of a
40,000-step life, the one setting, the twin with `--learning 0` reading the same cells).
Seeds 5 and 6.

Cartpole, the critic's mean level (spikes, opponent half subtracted) by steps until the
next fall:

| reward, seed | 1 | 2 | 3-4 | 5-8 | 9-16 | 17-32 | 33-64 | 65+ | rank corr | twin's |
|---|---|---|---|---|---|---|---|---|---|---|
| lean, 5 | -68 | -68 | -54 | -37 | -17 | -1 | 8 | 20 | +0.33 | -0.11 |
| lean, 6 | -58 | -53 | -42 | -27 | -8 | 2 | 11 | 8 | +0.21 | -0.01 |
| alive, 5 | -22 | -21 | -20 | -17 | -8 | -2 | -1 | -2 | +0.24 | -0.10 |
| alive, 6 | -32 | -30 | -27 | -23 | -15 | -7 | -10 | -12 | +0.27 | -0.01 |
| sparse, 5 | -33 | -31 | -30 | -28 | -25 | -20 | -14 | -13 | +0.26 | -0.06 |
| sparse, 6 | -45 | -46 | -45 | -43 | -39 | -31 | -25 | -19 | +0.26 | +0.00 |

The twin's cells move the other way and a tenth as much (lean seed 5: 6.1 one step before a
fall, 2.4 at 65+). How the surprise came out on lean: spread 24-27 spikes; a step the world
called bad was a bad surprise 70-73% of the time, a good one good 71-73% (the first agent's
64% and 79%, on the old cartpole). On alive and sparse a bad step (a fall) is heard as bad
58-87% of the time, and the lone goods of alive and sparse are heard as good 33% and 83-89%.

The chain: mean level at stage 1 (A to press, nothing paid) 66 and 53, at stage 2 (B pays)
74 and 61 (seeds 5, 6). The gap of about 8 spikes is the surprise for pressing A, which is
above the margin of 4: that is how the first press is taught. Twin: 2 and -6, -0.5 and 1.6.

Mountain car (energy reward): rank correlation of level with position +0.44 and +0.40 (twin
+0.11 and -0.13); level -48 and -33 at the left end, 89 and 53 near the goal. With energy
itself it does not correlate (+0.06, -0.09): it learned "where the car is", not the energy
the reward pays for.

## On cartpole: the surprise, not the actor (a diagnostic)

The first agent could not tell whether cartpole was held back by a noisy surprise or by
what the actor (a linear readout of the sheet) can resolve. `blobprobe critic --oracle`
replaces the world's valence by "was this press the scripted player's" (+1 or -1), which is
computed from the world's true state. **It is a diagnostic only, never a candidate**: the
world is giving the answer. Seeds 1-4, lean, 40,000 steps, late steps per fall, with the
first agent's setting (`readout_shift 4`):

| what teaches the button cells | per seed | mean |
|---|---|---|
| the critic's surprise from the lean reward (the setting) | 89, 105, 118, 44 | 89 |
| the critic's surprise, made from the clean verdict (`--oracle 2`) | 521, 875, 488, 521 | 601 |
| the clean verdict itself as the surprise, one step late, the actor's 4-step trace (`--oracle 1`) | 875, 1000, 1000, 583 | 865 |
| the readout alone, told the clean verdict as a one-step reward | 1000, 1000, 1000, 1000 | 1000 |

So the sheet can represent balancing: a linear reader of it holds the pole for whole
windows when told the right press. The actor's trace costs a little (1000 to 865), the
critic's own level noise more (865 to 601), and the lean reward, as the critic turns it into
a surprise, the most (601 to 89). The wall on cartpole is how much the surprise says about
each press. A wider critic was the first agent's answer to the noise; it was worse (below).

The same diagnostic on the two goal-only worlds the critic fails (seeds 1, 2, the final
setting, late score):

| world | the clean verdict as the surprise | the readout alone, told it as a reward | critic | scripted |
|---|---|---|---|---|
| mountain car, goal only | 0.41, 0.54 | | 0.00 | 1.00 |
| pendulum, upright only | 0.08, 0.14 | 0.19, 0.16 | 0.08 | 0.86 |

Mountain car's pumping is learnable by the actor once something tells it which press was
right: what the critic lacks there is any reward to learn from. The pendulum's swing-up is
not learned even when the right press is given every step: on this plug (8 lines per number,
bands of 45 degrees) the sheet's reader cannot represent the scripted player's rule, so the
pendulum is a wall of the reader, not of credit.

## Credit one step back, per world

The more-worlds job found that nothing learned a world that pays a step after the press
that earned it. The critic is the standard answer. From the report table and the probes
(seeds 5, 6; "goods" are the world's good steps in the last 10,000):

| world | critic | readout alone | does it get it | where it breaks |
|---|---|---|---|---|
| chain, cued (first press never paid) | 0.99 | 0.72 | yes | - |
| track, arrival only | 0.78 | 0.49 | yes (GRU 0.89, memory 0.98) | level rises toward an arrival (rank -0.21) and the surprise is good the step before one (+0.39, +0.42 mean sign) |
| forage, arrival only | 0.78 | 0.41 | yes, the best rule here | level rises toward the food (rank -0.40, -0.33) |
| mountain car, goal only | 0.00 | 0.00 | no | the critic never sees a reward: 0 goals in 10,000 steps (the twin 0 and 1). Told the right press, the actor reaches the goal in 0.41-0.54 of episodes, so it is an exploration problem, not a credit one |
| pendulum, upright only | 0.08 | 0.01 | barely (twin 0.06, t 2.5) | the critic sees it (8% of steps good) and its level moves earlier (10-20 spikes far off, 55-64 two steps before; rank -0.38, -0.45), and the surprise is good on the way in (+0.13 to +0.19 three to eight steps before). The chain breaks at the actor: even told the right press every step it gets 0.08-0.14 (the readout 0.16-0.19, scripted 0.86) |
| chain, no stage cue | 0.09 | 0.00 | no | not a credit problem: the stage is not on any line and nothing holds the last press, so the same input asks for A and for B. It settles on one button (stage 2 is reached 2-8% of steps) |

## How the setting was chosen

The first agent's tuning (seeds 1-4, 40,000 steps, its own plug: catch 6x6, the bandit
before the merge) chose the form of the rule:

- The error rule driven by the surprise with nothing on good (as the readout does on raw
  reward) stalls: bandit 0.61, chain 0.49. With the good half on (`readout_good 1`) both
  reach 1.00 and 0.97. The plain reinforce form (pressed button only, both ways) ties on
  the bandit and chain and is far worse on cartpole lean (26 against 89).
- A random press 1 step in 16 lowers the bandit (0.97) and the chain (0.88). Off.
- A one-group critic (no opponent half) breaks the chain on 2 of 4 seeds. The opponent
  half stays on.
- Critic reward 32 spikes instead of 16: lean 70 against 89; margin 0: 73; margin 12: 92.

## Making it better (seeds 1-4, 40,000 steps, the ladder's worlds)

Each change against the first agent's setting (`readout_shift 4`), kept only if it helps
and does not cost the bandit or the chain. `out/critic2/tune/`.

| setting | bandit2 | chain | lean | alive | sparse | catch |
|---|---|---|---|---|---|---|
| first agent's (shift 4) | 0.998 | 0.971 | 89.2 | 32.6 | 23.7 | 0.366 |
| wide critic (`critic_wide 1`, 768 cells) | 0.991 | 0.838 | 32.7 | | | |
| margin gate on the actor's good half, 20% | 0.836 | 0.794 | 55.7 | | | |
| **slower actor (`readout_shift 5`)** | **1.000** | **0.994** | **107.0** | 32.3 | **28.2** | **0.406** |
| slower still (shift 6) | 1.000 | 0.992 | 107.8 | 32.2 | 30.4 | 0.402 |
| shift 5, actor trace 2 steps | 0.998 | 0.993 | 86.3 | 29.3 | | |
| shift 5, actor trace 1 step | 0.998 | 0.990 | 8.5 | | | |

- **The wide critic** (every non-motor cell of the motor region, 768): worse everywhere,
  the chain worst (0.84). More cells did not mean a cleaner level.
- **The margin gate as the actor's good half** (built here: the vote kept per kept step in
  `CriticState`; with `margin_percent` set under a critic, a good surprise moves a step's
  press only when its vote lead was under that share): bandit 0.84, chain 0.79, lean 56.
  Under a critic the good half is what escapes the matching point, and gating it brings the
  point back. Not kept. The tags job's gate helped many-cue echo; it does not carry over.
- **A slower actor** (the actor's chance 2^-5 instead of 2^-4; under a critic only the
  actor reads `readout_shift`, since the raw valence never reaches the readout): better on
  five worlds, equal on alive. Kept. Shift 6 ties shift 5; the smaller change was kept.
- **A shorter actor trace**: 2 steps costs lean and alive; 1 step makes cartpole worse than
  its twin (8.5). The press that matters on cartpole is more than one step behind the
  verdict.

## What was built (both agents)

All behind genome numbers off by default; off is byte-identical. Files:
`src/blob/critic.hpp`, `src/blob/learn_critic.cpp`, `src/blob/learn_actor.cpp`,
`src/probe/critic_track.cpp`, `scripts/critic.sh`, `scripts/critic_track.py`,
`scripts/critic_tables.sh`, `scripts/variants/critic.txt`; the critic, GRU and memory rules
in `scripts/ladder.py`.

- **The critic.** A group of ordinary cells: the motor region's cells halfway between the
  motor lattice's cells (256). They hear the same tiles the buttons hear, keep the
  readout's evidence counters and wire the top of their budget. Its **level** is their
  spike count over the step's last 15 ticks; half of them (a checkerboard,
  `critic_opponent 1`) count against and learn the other way, so what every cell shares
  cancels.
- **The surprise.** At the end of each step: the reward of the step before (against its
  running mean, 16 fractional bits) times `critic_reward` (16 spikes), plus the level now,
  minus the level a step ago. Beyond `critic_margin` (4) it is good or bad, else nothing.
- **The critic's learning.** On a good surprise every critic cell gains evidence for the
  sources active in the steps before (4 steps, the chance halving each step back; opponent
  cells the other way); on a bad one the reverse. Activity per step is kept as a 4-bit spike
  count per neuron in bit planes, 8 steps.
- **The actor.** The readout's button cells, through the same counters, taught by the
  surprise for the presses of the last 4 steps (the chance halving each step back). The
  error form with the good half: on bad the pressed button's cells lose their active
  sources and the others gain them (balanced); on good the reverse.
- **Votes per step and the margin gate** (this job): `CriticState.vote`, read only with
  `margin_percent` under a critic. Not in the setting.
- **The oracle** (this job): `CriticState.oracle`, set only by `blobprobe critic --oracle`,
  never by a genome number. A diagnostic.
- **The hooks.** With `critic_on` the world calls `blob_reward` every step, valence 0
  included, after marking the press and the vote: `src/run/life.cpp`,
  `src/run/life_long.cpp` (this job) and the viewer's `src/view/sim_task.cpp`; only for the
  full engine (this job).
- **Probe.** `blobprobe critic` logs per step the level, the surprise and its sign, the
  move, the valence, whether the step ended in a fall, and (this job) the chain's stage
  and the 16.16 position and speed; `scripts/critic_track.py` bins the level by steps to
  the next fall and to the next good, by the chain's stage, and by mountain car's position
  and energy.
- **Cartpole's lean reward had a one-step lag** (first agent): the Euler step made a
  step's reward independent of that step's press. The semi-implicit step is now the
  default; every cartpole number before it is on the old world.

## Cost

Per step with the critic on: a 4-plane count of the last 15 tick maps (65 words x 15), a
level from popcounts, and on each surprise (about 70% of steps) an update of 256 critic
cells over 4 kept steps and up to 256 button cells, each followed by a rewire of the cell.
Memory: 8 steps of planes and presses (about 25 KB) and evidence rows for the critic cells
(180 KB). Speed, rough (this PC, 3 jobs and other agents running): critic lives 11,800
ticks a second on cartpole, 13,200-13,300 on catch and forage; the readout alone 23,300,
41,700 and 53,400; the twins about 65,000. The critic is 2 to 4 times the cost of the
readout alone, and nearly all of it is the step's end (the tick is unchanged). The obvious
saving is to rewire a critic cell less often than every surprise. Measure with `blobbench`
on a quiet machine before relying on these.

## How each number was made

```
scripts/critic.sh tables                           the report (scripts/ladder.py; JOBS=3)
scripts/critic.sh tables --seeds 1-4               the same on the tuning seeds
scripts/critic.sh track cartpole --seed 5          the level against steps to a fall
scripts/critic.sh track cartpole --seed 5 --cart-reward sparse
scripts/critic.sh track chain --seed 5             the level by the chain's stage
scripts/critic.sh track mountain-car --seed 5      the level against position and energy
scripts/critic.sh track cartpole --seed 1 --oracle 1     the diagnostic (not a candidate)
python3 scripts/ladder.py run --life 400k --rules critic --worlds cartpole,catch,forage
```

This job ran them through frozen binaries and helper scripts in `out/critic2/` (the
ladder in `out/critic2/ladder/`, tuning in `out/critic2/tune/`, probes in
`out/critic2/probe/` and `probe5/`, the table builder `doc_table.py`): `report.sh`,
`tune1.sh`, `tune2.sh`, `long3.sh`, `probes.sh` with `mkprobes.py`, `refcheck.sh`. The GRU
and memory references and the probes ran on stevejobs (`~/thonk-critic2/out/`).

## Rough

- Four tuning seeds; differences under about two standard errors there are noise (lean's
  spread is 30 steps per fall). Shift 5 against 4 is ahead on five worlds out of six, each
  by little.
- Track and forage score steps that ended nearer, so pressing into the track's end scores
  low even when targets are reached.
- The oracle runs used the first agent's setting (shift 4), not the final one.
- The tracking figures are two seeds per world.
- The memory engine pressed nothing on chain, chain-nocue and track; not looked into.
- Speeds were taken on a loaded machine.
- The viewer's every-step hook still fires for any engine with `critic_on`.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, "Where it stands", the critic line: on the report seeds the critic
  learns 15 of 17 ladder worlds above its twin with one setting: bandits 1.00, switching
  0.98, cued chain 0.99 (GRU 0.01), track 0.88, arrival-only track 0.78 and forage 0.78
  (GRU 0.62), mountain car 0.97; cartpole lean 109 steps per fall at 40,000 steps and
  about 110 at 400,000 (GRU 875, memory 302): it does not balance, and longer lives do
  not help.
- `docs/findings.md`, principle 31: up to [likely] (8 report seeds, 17 worlds). Add: the
  slower actor helps; the margin gate on the actor's good half costs (bandit 0.84, chain
  0.79), so finding 39's gate holds for raw good, not for a critic's good; a wide critic is
  worse.
- `docs/findings.md`, wall "Acting in a world": rewrite. Cartpole is learned to 109 (lean)
  and stops; the sheet can represent balancing (865 to 1000 steps per fall when told the
  right press), so the wall is the surprise's information per press, not the reader.
  Credit one step back is reached by the critic on the cued chain, arrival-only track and
  arrival-only forage; not on goal-only mountain car (never reached: exploration) and
  upright-only pendulum (the critic sees it coming; the actor does not follow).
- `docs/findings.md`, wall "Time" (the more-worlds job's note): credit one step back is no
  longer missing everywhere; the uncued chain is a memory problem, not a credit one.
- `docs/task-map.md`, tier 7: the report table above as "best of ours" for every world the
  critic leads among the sheet's rules (all but pend, where the readout alone 0.23 is
  better), with the GRU and memory beside it.
- `docs/ideas.md`, idea 64 / 90 (critic): kept as the acting rule, setting above; next:
  more information per press for cartpole (a critic over more of the sheet did not do it;
  a per-press advantage, e.g. a critic level per button, is the obvious next), a cheaper
  step end, and exploration for goal-only worlds.
- `docs/predictions.md`: none was written for this job.
- The viewer's owner: `src/view/sim_task.cpp:100` should give the every-step reward only
  for the full engine.
