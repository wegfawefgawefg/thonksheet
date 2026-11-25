# Curriculum

The ladder of things the blob is asked to do, easiest first. Every task talks to the blob
through the same plug (sensor lines, motor lines, good, bad; see `docs/design.md`). The blob
is never reset between tasks or between tries.

## How a task is scored

- A task reports a score per window of its own steps, over the blob's whole life on it.
- The headline number is **late minus early**: the mean of the last windows minus the mean
  of the first. A blob that is born good and learns nothing scores zero.
- Every result is shown next to two baselines run with the same genome and seed: learning
  switched off, and motors driven by coin flips.
- Tasks marked *shuffled* reassign which line means what at the start of each life. Nothing
  born-in can be right, so only learning scores. These are the ones a genome search will use.
- Task physics uses integers or fixed point, so a run is repeatable.

## Stage 0: staying alive

No task. The blob idles for a million ticks. Pass: activity in every sector stays near its
target, nothing dies, nothing saturates, no short loop takes over. Then the same with random
input, and with input that starts and stops.

## Stage 1: reflexes (one move)

1. **Echo.** One of K sensor lines is on. Pressing the matching motor line is good, any
   other is bad. Start with K = 2. One step is one try. No move is neither good nor bad but
   counts as a miss. Score: right presses over tries.
2. **Shuffled echo.** Same, but which button matches which line is shuffled per life.
3. **Delayed echo.** The cue goes off; the answer is wanted some ticks later. First test of
   holding something over time. A try is: one step with the cue, `delay` blank steps, then
   one step with an "ask" line on. Only the press on the ask step counts.
4. **Go / no-go.** Press on one cue, hold still on another. Two motor lines, press and rest;
   rest winning, a tie or silence is holding still. Holding still on no-go is good. Score:
   right answers over tries.
5. **Beat.** Press once every N ticks with no cue. First test of the sense of time. N is in
   task steps. Press and rest lines as in go/no-go. A press on the due step is good, a miss or
   an early press is bad. Score is balanced: the mean of the hit rate on due steps and the
   hold rate on the others, so pressing always or never both score 0.5.

## Stage 2: prediction and recognition

6. **Sequence.** A repeating pattern plays on the sensor lines. No reward. Score is surprise
   going down. Each line is on in half the pattern steps. Scored on the first tick of each
   step (when the input changes) as the share of attached sensor neurons predicted right, so
   any fixed or random guess scores 0.5 and knowing the pattern scores 1. Good and bad are
   left unattached.
7. **Shapes.** Small bitmaps appear on the sensor sheet at random places. Press the button
   for the shape. Tests whether a feature learned in one place carries to another.
8. **Counting.** N pulses, or N dots; press button N.
9. **Rhythm and tune.** Frequency lines over time; name the pattern. The sound-shaped task.
10. **Mirroring.** Copy a pattern from the sensor sheet onto a motor sheet; follow a moving
    target.

## Features nobody designed in

For the bench (`docs/bench.md`, idea 49). In each the answer depends on something no
single input line carries, so a readout of the input lines alone cannot score; the sheet
has to make the feature. All four share one trial shape: one blank step (every line off)
that parts it from the last trial, the steps that show it, then one step with the ask line
on. Only the press on the ask step counts; presses before it are ignored. Two buttons. The
right button is good, the other bad, no move a miss. Score: right answers over trials.
Coin flips score about 0.44 (ties are no move), the scripted player 1.00.

- **Order.** Lines A, B and ask. The first cue (A or B, a coin flip) comes on alone for
  `order_gap` steps (default 1), then the second joins it for one step, then the ask step
  with both off. Button 0 if A came first, 1 if B did. Both cues were last seen on the
  same step, so "which was on last" does not tell it; only the order they came on in does.
  Trial: 3 + `order_gap` steps.
- **Both-not-either.** Lines A, B and ask. A and B are each on or off by coin flip (four
  cases, equally likely), shown for one step with the ask line. Button 1 if exactly one is
  on, 0 if both or neither: exclusive or. No sum of the lines with fixed signs gets it; it
  needs an "A and B" feature. Trial: 2 steps.
- **Gap.** Lines cue and ask. The cue is on for one step, then `gap_short` (default 1) or
  `gap_long` (default 3) blank steps, a coin flip, then on for one more step, then the ask.
  Button 0 short, 1 long. The last blink is one step back either way; the answer is in how
  far back the first one was. Trial: 4 + gap steps.
- **Place.** A `place_size` square sheet (default 4x4 lines) and the ask line. One of two
  shapes, each two lines on a 2x2 corner, appears with the ask at one of (size - 1)^2 places
  (9 by default): "\" ((0,0) and (1,1)) is button 0, "/" ((1,0) and (0,1)) is button 1.
  Every single line is lit by both shapes at some place, so no one line tells the shape.
  `place_hold` places (3 by default) are drawn once per life from the task seed and never
  rewarded: trials there get no good or bad. Only those trials are scored, so the score is
  carrying a shape learned at some places to places it was never taught at. Trial: 2 steps;
  about a third of trials are scored.

## Reach

- **Readable.** `blobrun --reach`: `blobprobe carry`'s measure
  (`docs/results-2026-11-24-carry.md`). The sensor cells of the sensor region's corner tile are split into cue A and
  cue B; one of them is on per block of 20 ticks, no reward. How well A and B can be told
  apart from the neurons at each distance in tiles; reach is the farthest still at 0.3.
- **Echo at a distance.** Any task with `--distance N`: the plug moves the motor lines,
  then the sensor lines, away from each other by whole tiles until the tile row (or column)
  of the cue cell nearest the motors and that of the motor cell nearest the cues are N
  apart. The default genome packs them 1 tile apart; the most it allows is 7.

## Stage 3: control

11. **Catch.** Move a paddle under a falling dot. A 6x6 bitmap; the dot falls a row per
    step; left and right lines, no move stays. Caught is good, missed is bad. Score: caught
    over dots.
12. **Cartpole.** Balance. Gym constants in 16.16 fixed point; the four state numbers
    become six threshold lines each. Left and right lines, no move pushes nothing. A fall is
    bad and restarts the cart, never the blob; every 25 steps up is good. Score: steps per
    fall.
13. **Mountain car, pendulum swing-up, acrobot.** The classic set.
14. **Snake** on a 10x10 board: head, food and body countdown lines.
15. **Flying.** Hold height with thrust; then a lander.
16. **Jumping.** Clear gaps by timing a jump.
17. **Walking.** A small 2D walker with joint motors. Hard; last.

## More worlds: the rungs between Echo and cartpole (idea 63)

Built 2026-11-25 (`src/world/task_bandit.cpp`, `task_move.cpp`, `task_classic.cpp`; all
integers or 16.16 fixed point; `scripts/task.sh <world>`, the whole set with
`scripts/ladder.py`). No move is always a move of its own: it is never good or bad by
itself unless said. "Coin" is the stand-in's coin flips through the plug (ties are no
move), "scripted" the perfect player. Results: `docs/results-2026-11-25-more-worlds.md`.

- **Bandit** (`bandit`, `--k` buttons, default 2). No cue: one line that is always on and
  carries nothing. One button, drawn per life, pays good on 80% of presses and bad
  otherwise (`--bandit-good`); every other button pays good on 20% (`--bandit-poor`).
  Score: share of steps on the best button. Episodes: presses. A table of two counts per
  button solves it in tens of presses.
- **Switching bandit** (`switch-bandit`). The same, but every `--switch-every` steps
  (1,000) the paying button moves to another one. Score: share of steps on the best
  button over the second half of each block only. Episodes: blocks. A table of counts
  that forgets (a running mean) solves it; one that never forgets does not.
- **Chain** (`chain`, `--k` buttons). A and B are two buttons drawn per life. Stage 0:
  pressing A moves to stage 1, anything else stays; never a reward. Stage 1: B is good,
  another button bad, no move nothing; then back to stage 0. So the press that starts a
  chain is only ever paid one step later. `--chain-cue 1` (default): two lines show the
  stage; `0`: one line always on, so the stage must be remembered (the best memoryless
  player scores 0.5). Score: 2 x chains paid / steps (A, B, A, B, ... is 1.0).
  Episodes: chains paid. A table of rewards per stage and button learns stage 1 and never
  stage 0 (no reward there ever); it takes credit one step back (Q-learning with any
  discount, or a trace) to learn stage 0. Four table entries.
- **Track** (`track`). A `--track-size` x 2 bitmap (5): the target's cell on the top row,
  the marker's on the bottom. Buttons left, right; no move or a move into an end stays.
  `--track-reward shaped` (default): good when a step ends nearer the target, bad when
  further. `arrive`: good on arrival only, bad if `track-size` steps pass without one. On
  arrival the target jumps to another cell. Score: share of steps that ended nearer (an
  arrival counts). Episodes: targets. Shaped, the right button is target-left-or-right of
  the marker: linear in the lines (weight +i on target cell i, -i on marker cell i).
- **Forage** (`forage`, needs `--sensor-side 2`). A `--forage-size` square grid (5) that
  wraps at the edges, seen from the animal, which is always the middle cell; the bitmap
  shows only the food. Buttons up, down, left, right. `--forage-warmer 1` (default): good
  when a step ends nearer the food (grid steps, the short way round), bad when further;
  `0`: good on reaching it, bad if 2 x (size - 1) steps pass (the food moves). The food
  reappears at another random cell. Score: share of steps that ended nearer. Episodes:
  targets. Warmer, it is echo with 24 cues and one or two right buttons each.
- **Mountain car** (`mountain-car`, `--sensor-side 2 --number-code place`). The gym
  constants (position -1.2 to 0.6, speed at most 0.07, push 0.001, gravity 0.0025
  cos 3x, goal 0.5) in 16.16. Position and speed are 8 place-coded lines each. Buttons
  push left, right. Episodes of at most 200 steps, new ones at rest between -0.6 and -0.4.
  `--classic-reward energy` (default): a step is good when the car's energy (height plus
  speed) rose, bad when it fell, the goal good. `goal`: good at the goal, bad when the 200
  steps run out. Score: share of episodes that reach the goal. The push alone cannot
  climb: the car must be rocked. Under the energy reward the right button is the sign of
  the speed, readable from the speed lines.
- **Pendulum swing-up** (`pendulum`, as mountain car). The gym pendulum (angle 0 upright,
  gravity 10, torque 2, spin at most 8, 0.05 s steps) in 16.16; angle and spin 8 lines
  each. Episodes of 200 steps from a random angle and spin within 1. Upright is within
  0.4 rad. Energy reward: a step ending upright is good, falling out is bad; elsewhere
  good when the energy gap to upright shrank, bad when it grew. `goal`: only the upright
  part. Score: share of steps upright. The scripted player (pump, then a linear push near
  the top) gets 0.86, not 1.0, because each episode starts somewhere random.
- **Cartpole variants** (fairness). `--cart-start wide`: the cart within 1 m of the middle,
  speed and spin within 0.5, lean within 0.1 rad (half way to a fall); the scripted player
  still never falls. `--cart-reward relative`: good when the step ended nearer upright and
  centred than the running mean (1/16 per step) of recent steps, bad when further, so good
  and bad come about equally often (0.53 / 0.47 for a coin). `--cart-integrator semi`:
  see the notes on cartpole below; it is what makes a step's reward judge that step's push.

Checked scores, 200,000 steps straight on the task (uniform random press, no ties) and
through the plug (coin, 20,000 steps, ties are no move):

| world | random | coin | scripted |
|---|---|---|---|
| bandit k=2 | 0.50 | 0.47 | 1.00 |
| bandit k=4 | 0.25 | 0.24 | 1.00 |
| switching bandit k=2 | 0.50 | 0.46 | 1.00 |
| chain, stage shown / not | 0.33 | 0.29 | 1.00 |
| track 5, shaped / arrive | 0.50 | 0.46 / 0.47 | 1.00 |
| forage 5x5, warmer / arrival | 0.43 | 0.38 | 1.00 |
| mountain car (goal share) | 0.00 | 0.00 | 1.00 (129 steps an episode) |
| pendulum (upright share) | 0.05 | 0.05 | 0.86 |
| cartpole wide starts (steps per fall) | 20.9 | 20.0 | no fall |
| cartpole relative | 22.1 | 23.9 | no fall |

## Stage 4: one life, many tasks

- Switch tasks without a reset and see what survives.
- Leave a task, come back later, compare with before.
- Unplug everything for a long stretch, plug back in, compare (the idle check in
  `docs/design.md`).

As built in `blobrun` (one life, the blob never reset, learning on throughout):

- **Retention** (`--retention B`). Task A for `--steps`, then task B for `--steps`, then A
  again for `--retest` steps. B takes A's task flags, but an echo-family B has its cues and
  buttons after A's (cue offset = A's offset + k), so `--task echo --retention echo` teaches
  two new cue-button pairs on new lines; one learner can hold both. During B, a press on
  one of A's buttons is wrong. Reported: A at the end of its phase (mean of the last tenth
  of its windows), A in the first window of the retest, A at the end of the retest, and B
  at its end. B uses the same task seed plus one; A's retest uses A's seed, so a shuffled
  task keeps its mapping.
- **Idle** (`--idle N`). Task A for `--steps`, then N ticks with every line off and
  unattached (no good or bad either), then A again for `--retest` steps. Reported: before
  (end of A), and the first window and the end of the retest.
- **Capacity** (`--capacity`). Echo with 2, 4, 8 and 16 cues, each a separate life. A k
  is held when its late score is at least halfway from chance (1/k) to perfect:
  0.75, 0.625, 0.5625, 0.53. Reported: the largest k held.

## Stage 5: genome search

Many blobs, each living a full life across unseen, shuffled tasks. Score is late minus
early. Not started until one hand-set blob has visibly learned something in stages 1 to 3;
with nothing learning there is nothing for a search to climb.

## Notes on cartpole (2026-11-24)

The first cartpole gave good only every 25 steps up and bad on a fall, which was a guess
and a poor one. `--cart-reward` now chooses: `sparse` (that), `alive` (good on every step
up, as in the classic task), or `lean` (every step: good if pole and cart ended nearer
upright-and-centred than they began, bad if further). Numbers also had a flaw: lines lit
from the left as the value rose, so "leaning left" was only an absence of activity.
`--number-code place` lights the one line whose band holds the value. The scripts use
`lean` and `place` for cartpole. `sparse` stays as the harder version for slow-credit ideas.

2026-11-25: the cartpole step is gym's Euler step, which moves the cart and pole by the
speeds from before the push; the push changes only the speeds. So where the pole ends a
step does not depend on that step's press at all, and `lean` (and `relative`) judge the
press one step earlier. Measured with a random player: after the scripted player's press
lean is good 0.322 of the time, after the other press 0.321; by the previous press it is
0.42 against 0.22. A rule that credits what was active just before the reward is graded
on noise. `--cart-integrator semi` (gym's other option: speeds first, then positions by
the new speeds) fixes that: lean then judges this press (0.40 against 0.20). Mountain car
and the pendulum use the semi-implicit step already.
