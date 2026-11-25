# Actor and critic (2026-11-25)

Barto, Sutton and Anderson (1983) balanced a pole with two neuron-like elements: an actor
told only by a critic that learned to predict how well things were going and broadcast the
change in its prediction. This job gives the sheet a critic in its own terms and tries it on
the acting worlds. **Stopped early** (the owner cut the number of agents): everything below
is on the tuning seeds 1-4 at 40,000 steps. No report seeds (5-12) and no million-step lives
were run for the sheet. What is missing, and the line that would run it, is at the end.

## Short version

- **Cartpole's lean reward had a one-step lag, and it is fixed here.** With the Euler order a
  push changed only the speeds, and the cart and pole moved by the old ones, so a step's lean
  reward did not depend on that step's press at all. Random presses, 2M steps: good came 0.218
  and 0.217 after a wrong and a right press (when the press before was wrong), 0.421 and
  0.419 (when it was right). The fix (`src/world/task_control.cpp`, speeds first: gym's
  semi-implicit option, the same as the more-worlds branch's `--cart-integrator semi`,
  a3342ae) makes it 0.135 against 0.239 and 0.288 against 0.528. It is unconditional here,
  since the old order is a bug. Every cartpole number from before this fix is on the old
  world. On the fixed world the scripted player never falls in a million steps and a coin
  that always presses falls every 20.8 steps (`out/critic/cart_players.cpp`).
- **The fix alone lets the reference networks balance.** The fixed GRU, seeds 5-12, 40,000
  steps, told only good and bad (`blobrun --engine gru`): cartpole lean 798 steps per fall
  (5 of 8 lives at 875 or more; a window with no fall scores 1,000), alive 73, sparse 77; the
  MLP 276, 96 and 44. So "nothing learns cartpole" was the world, not the learners.
- **With a critic the sheet learns all six acting worlds better than without**, seeds 1-4:

  | world (score) | learning off | readout alone | readout + critic | GRU (seeds 5-12, its own plug) |
  |---|---|---|---|---|
  | bandit (share on the best arm) | 0.51 | 0.77 | **1.00** | 1.00 |
  | chain (2 x chains paid / steps) | 0.18 | 0.78 | **0.97** | 0.01 (MLP 0.45) |
  | cartpole lean (steps per fall) | 18.4 | 19.8 | **89** (107 with a slower actor) | 798 |
  | cartpole alive | 17.2 | 25.6 | **32.6** | 73 |
  | cartpole sparse | 17.6 | 20.2 | **23.7** (26.6 reinforce form) | 77 |
  | catch (caught / dots) | 0.17 | 0.20 | **0.32** | 0.58 |

  "readout + critic" is the chosen setting (error rule driven by the surprise, good half on).
  The chain is the one place the sheet beats the reference: the GRU (and our readout without
  a critic) is never taught at the chain's first stage, because nothing is paid there; the
  critic's surprise is. Cartpole does **not** balance: 89 steps per fall at 40,000 steps,
  still rising (late minus early +78), against the GRU's 798.
- **The critic learns something real.** Its level rises with the number of steps until the
  next fall: rank correlation +0.26 (opponent critic) and +0.31 (one group), against -0.04
  for the same cells on the learning-off twin (seed 1, lean reward on the old world, last
  5,000 of 20,000 steps; not rerun on the fixed world). Mean level by steps to the next fall,
  opponent critic: -51 one step before a fall, -47 at 3-4, -33 at 5-8, -13 at 9-16, -7 at
  17-32.
- **Which actor form works: either, as long as positive surprise teaches.** The existing
  error rule driven by the surprise, with nothing on good (as the readout does on raw
  reward), stalls: bandit 0.61, chain 0.49. With a positive surprise also teaching (the
  pressed button gains, the others lose: `readout_good 1`) it reaches 1.00 and 0.97; the
  plain reinforce form (pressed button only, both ways) reaches 1.00 and 0.97 too. On
  cartpole lean the error form is far better (89 against 26); on sparse the reinforce form is
  a little better (26.6 against 23.7); elsewhere they tie. The error form with its good half
  is the setting chosen.
- **Exploration was not needed and costs.** A random press 1 step in 16 lowers the bandit to
  0.97 and the chain to 0.88, and helps only the reinforce form on cartpole lean (33 against
  26). Off in the chosen setting.

## What was built

All behind genome numbers that are off by default; off is byte-identical (cmp of blobrun csvs
for cartpole lean/place mode 5, echo mode 0 and echo mode 5, 3,000 steps, against c226d4b;
the sums differ only in ticks per second). New files: `src/blob/critic.hpp`,
`src/blob/learn_critic.cpp`, `src/blob/learn_actor.cpp`, `src/probe/critic_track.cpp`,
`src/world/task_rungs.cpp`, `scripts/critic.sh`, `scripts/critic_tables.sh`,
`scripts/critic_track.py`, `scripts/variants/critic.txt`.

- **The critic.** A group of ordinary cells: the motor region's cells halfway between the
  motor lattice's cells (256 by default; `critic_wide 1` takes every non-motor cell there,
  768, built but not tested). They hear the same tiles the buttons hear. Each keeps the
  readout's evidence counters (`learn_evidence.cpp`, one line there lets critic cells have
  rows) and wires the top of its budget. Its **level** is its cells' spike count over the
  step's last 15 ticks. `critic_opponent 1` (default): half the cells, a checkerboard, count
  against and learn the other way, so what every cell shares (the sheet's rhythm, its overall
  rate) cancels. In the first version (one group, and the mean bug below) the level's spread
  was 104 spikes and the surprise's 97, and a world bad step was heard as bad 47% of the time
  (chance); with both fixed the surprise's spread was 30 and bad was heard as bad 64%.
- **The surprise.** At the end of each step: reward of the step before (against its running
  mean, `critic_center`, in 16 fractional bits; first built in 8 bits, which rounded the mean
  to -1 and made every bad step count as nothing) times `critic_reward` (16 spikes), plus the
  level now, minus the level a step ago. Beyond `critic_margin` (4 spikes) it is good or bad,
  else nothing. Integers throughout; no floats in `src/blob/`. The running mean of reward is
  the average-reward form of TD: the level is spikes and cannot go negative enough to soak up
  a reward that is bad 70% of the time, so the mean is taken out first.
- **The critic's learning.** On a good surprise every critic cell gains evidence for the
  sources active in the step before (opponent cells lose it), on a bad one the reverse; the
  steps before that count too, the chance halving each step back (`critic_trace_steps` 4,
  `critic_fade` 1). Activity per step is kept as a 4-bit spike count per neuron in bit planes
  (8 steps kept), and a plane worth 2^p spikes moves with 2^p times the chance, so a source
  moves in proportion to how much it fired, as the readout's tick-by-tick rule does, for a
  sixth of the random draws.
- **The actor.** The readout's button cells, through the same counters, taught by the
  surprise instead of raw good and bad, for the presses of the last 4 steps (chance halving
  each step back; `actor_trace_steps`, `actor_fade`). The pressed button of each kept step is
  a copy of the world's mark (`motor_acted`). `actor_form 0`: the error rule (bad: pressed
  lose, others gain, balanced; good: nothing, or with `readout_good 1` pressed gain, others
  lose). `actor_form 1`: reinforce (pressed only, both ways).
- **Exploration** (`explore_shift`; the plug's nudge is not on main): with chance 2^-shift a
  step, a random button for the next step, made by holding either the last press's cells or
  every other button's quiet for a step (their cooldown); `explore_scale` makes it rarer
  where the critic's level stands above its mean.
- **The hooks.** With `critic_on` the world calls `blob_reward` at the end of every step,
  valence 0 included, after marking the press: one condition in `src/run/life.cpp` and one in
  the viewer's `src/view/sim_task.cpp`; `blob_reward` hands the step to `critic_step` and
  `blob_free` frees its block (`src/blob/birth.cpp`). The raw valence then never reaches
  `learn_readout`. Blob gets one pointer at its end; Genome gets 14 numbers at its end.
- **Bandit and chain** (`src/world/task_rungs.cpp`, `--task bandit`, `--task chain`): idea
  63's worlds were not on main, so they are written here to the same definitions and
  defaults as the more-worlds branch (bandit 80/20, chain with the stage shown). Drop this
  file for that branch's when it lands; the enum entries will conflict by name on purpose.
  Done at the merge (2026-11-25): `task_rungs.cpp` is gone and `--task bandit` and `--task
  chain` are `src/world/task_bandit.cpp`. Same definitions; the bandit draws its best arm
  with one draw instead of two, so bandit lives differ from the numbers here by seed (a
  2,000-step critic life: late 0.998 here, 1.000 on main's); chain and cartpole lives are
  byte-identical. Cartpole's default is now the semi-implicit step;
  `--cart-integrator euler` keeps the old one.
- **`blobprobe critic`** logs per step the critic cells' level, the surprise, the move, the
  valence and whether the step ended in a fall; `scripts/critic_track.py` bins the level by
  steps until the next fall. With `--learning 0` it reads the same cells on the twin.

## Tuning (seeds 1-4, 40,000 steps, 128 motor cells per button)

Every number from the sweep tables in `out/critic/round-a.md`, `out/critic/round-b.md` and
`out/critic/mini/round-c-lean.md` (late = mean of the last 4 windows of 1,000 steps; twin in
brackets). Round A's cartpole rows were stopped when the lag was found; bandit and chain were
unaffected.

| variant | bandit | chain | lean | alive | sparse | catch |
|---|---|---|---|---|---|---|
| learning off | 0.51 | 0.18 | 18.4 | 17.2 | 17.6 | 0.17 |
| readout alone | 0.77 | 0.78 | 19.8 | 25.6 | 20.2 | 0.20 |
| critic, error rule, nothing on good | 0.61 | 0.49 | | | | |
| critic, error rule + good (**chosen**) | **1.00** | **0.97** | **89.2** | 32.6 | 23.7 | **0.32** |
| critic, reinforce | 1.00 | 0.97 | 25.9 | 32.8 | 26.6 | 0.31 |
| chosen + exploration 1/16 | 0.97 | | 71.5 | 30.1 | 25.2 | 0.28 |
| reinforce + exploration 1/16 | 0.97 | 0.88 | 33.1 | 32.0 | 27.6 | 0.28 |
| reinforce, one step of credit (TD(0)) | 1.00 | | 14.8 | 33.7 | 27.4 | 0.30 |
| error rule, nothing on good, TD(0) | 0.69 | 0.59 | | | | |
| error rule, nothing on good, reward 32 | 0.61 | 0.49 | | | | |
| error rule, nothing on good, one group | 0.60 | 0.21 | | | | |

Per-seed lean, chosen: 89 105 118 44. On cartpole lean, round C around the chosen setting
(mini, stopped at 29 of 36 runs):

| change to the chosen setting | lean |
|---|---|
| none | 89.2 (89 105 118 44) |
| actor chance 2^-3 (faster) | 55.0 |
| actor chance 2^-5 (slower) | **107.0** (85 155 103 85) |
| critic reward 32 spikes | 69.6 |
| margin 0 | 73.1 |
| margin 12 | 91.6 |
| critic chance 2^-3, 2^-5 | not finished |

The slower actor is better on lean (+18, t 1.1, noise at 4 seeds); it was not tried on the
other worlds, so the chosen setting keeps 2^-4. The one-group critic (no opponent half)
breaks the chain on 2 of 4 seeds (0.00), which is why the opponent half is the default.

## The reference networks on the same worlds (seeds 5-12, 40,000 steps)

`out/critic/mini/ref-5-12.md`: `blobrun --engine mlp|gru`, their own plug (no
`--motor-spread`), fixed cartpole.

| world | MLP | GRU | twins |
|---|---|---|---|
| bandit | 1.000 | 1.000 | 0.51 |
| chain | 0.447 | **0.007** | 0.33 |
| cartpole lean | 276 | 798 | 20 |
| cartpole alive | 96 | 73 | 20 |
| cartpole sparse | 44 | 77 | 21 |
| catch | 0.31 | 0.58 | 0.17 |

The GRU's chain is worse than its twin: it learns stage 1 (B is paid) and, untaught at stage
0, settles on pressing B there too, which never starts a chain. The reference credits only
the step just judged; the critic is exactly what it lacks. (The reference doc's catch 0.72 is
a different plug; on this plug the GRU gets 0.58.)

## Where the chain breaks on cartpole

Cartpole improves with the critic (lean 20 to 89, more on a slower actor) but is far from
balanced and from the GRU. What was measured of each link:

1. **The critic learns.** Its level is ordered by distance to a fall (above), so it does carry
   "how good from here".
2. **Its surprise is still noisy.** On lean (old world, seed 1) the surprise's spread was 30
   spikes against a reward worth 16: a world bad step was heard as a bad surprise 64% of the
   time, a good one as good 79%. Doubling the reward's weight (32 spikes) made lean worse
   (70 against 89), so the noise is not simply the reward being too quiet; more likely the
   level's own step-to-step noise. The wide critic (768 cells, 3x the signal for 1.7x the
   noise) is built for this and not yet measured.
3. **The actor uses it**: the bandit and chain go to 1.00 and 0.97 on the same surprise, and
   lean rises 4.5x. Whether it is the actor's resolution (what a linear readout of the sheet
   can tell about the state) or the surprise's noise that caps cartpole was not separated.

## Cost

Per step with the critic on: one 4-plane count of the last 15 tick maps (65 words x 15), a
level from popcounts, and on a surprise an update of 256 critic cells and up to 256 button
cells (one sparse random word per plane per slot per kept step) plus a rewire of each touched
cell. Memory: 8 steps x 5 words-per-tile maps (about 25 KB), and evidence rows for the 256
critic cells (180 KB). Speed was not measured on a quiet machine (both machines were
saturated by other agents all day): one 10,000-step cartpole life (200,000 ticks) took 21 s
with the critic here at load 35, about 10,000 ticks per second; the readout alone was not
timed beside it. The tick itself is unchanged; the cost is all at the step's end. Measure
with `blobbench` on a quiet machine before merging.

## Unfinished, and how to finish it

- The report: seeds 5-12 at 40,000 steps for every world, the readout alone against the
  critic (and the reinforce form, and the rich2 sheet): `scripts/critic.sh tables` (or
  `scripts/critic_tables.sh`; `JOBS=4` on the mini).
- The million-step lives, cartpole in ten slices: `STEPS=1000000 WORLDS="lean alive sparse
  catch" JOBS=4 scripts/critic_tables.sh` on the mini (`MINI_DIR=thonk-critic
  scripts/mini.sh push` first); the slices are the csv windows grouped by 100. Not run.
- The critic's level against distance to a fall on the fixed world, every reward form:
  `scripts/critic.sh track cartpole --seed 5` (and `--cart-reward alive`, `sparse`).
- Round C's critic-chance rows and the wide critic: `bash out/critic/tune.sh
  out/critic/v-d.txt D` (seeds 1-4; `WORLDS=...` to choose worlds).
- The viewer: `scripts/watch.sh cartpole --motor-spread 128 --g.learn_mode 5
  --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 --g.critic_on 1
  --g.readout_good 1` builds and is wired; it was not run (agents never open a window).

## Rough

- Tuning used 4 seeds; differences under about 2 standard errors are noise.
- The critic-tracks-distance figures are from one seed on the old (lagged) cartpole.
- The viewer's reward hook now also marks the pressed button on every reward, which main's
  viewer already did; with the critic it calls the readout watch every step.
- Exploration silences cells through their cooldown, which also stops them firing into the
  sheet for that step.
- `scripts/sweep.py`: the 14 critic numbers were added to its learning-only set, so twins are
  shared.
- Raw outputs: `out/critic/` (sweep caches `sweep/`, `sweep2/`; tables `round-a.md`,
  `round-b.md`; the mini's in `out/critic/mini/`; the lag check `lean_check.cpp`; the
  players `cart_players.cpp`), and on stevejobs `~/thonk-critic/out/`.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, wall "Acting in a world": cartpole's lean reward judged the press before
  the one it followed (fixed here, semi-implicit step); on the fixed world the reference GRU
  balances it (798) and the sheet with a critic reaches 89 (tuning seeds, 40,000 steps).
  With a critic the sheet learns the bandit (1.00) and the chain (0.97, where the GRU gets
  0.01), and catch rises 0.20 to 0.32. A new principle candidate, [hint] until the report
  seeds run: **an actor told good and bad needs a critic's surprise, and the surprise must
  teach on good as well as bad** (the error rule's "nothing on good", right for one-step
  tasks, stalls the bandit at 0.61 and the chain at 0.49 under a critic). And: **a critic made
  of spiking cells needs an opponent half**, or the sheet's common swings drown it.
- `docs/findings.md`, finding 17 ("do not strengthen your own answer on good"): under a
  critic's surprise the good half is needed; the lock it caused came from raw good, which a
  critic centres.
- `docs/task-map.md`, tier 7: bandit 1.00 and chain 0.97 with the critic (tuning seeds);
  cartpole on the fixed world: lean 89, alive 33, sparse 24 against coin 21 and the GRU 798,
  73 and 77; catch 0.32 on this plug (GRU 0.58). Mark every cartpole number before the fix as
  the lagged world.
- `docs/ideas.md`: idea 90 (value cell) and the actor-critic entry (64): tried, kept, behind
  `critic_on`; next on it: the wide critic, a slower actor, and the report seeds.
  `docs/predictions.md`: none was written for this job.
- The more-worlds branch: its `--cart-integrator semi` should become the only integrator, as
  here; its bandit and chain replace `task_rungs.cpp`.
