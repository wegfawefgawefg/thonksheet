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

## Invariance

The invariance battery (idea 101; `scripts/invariance.sh`). One task kind, `--task
inv-<kind>`. Each shows one of `--inv-shapes` things (2 by default; shapes up to 4, view
objects up to 3) in one version of one change; the button is which thing. Some versions are
rewarded. The others, the held-out set, are never rewarded (no good, no bad) and are the
only trials scored, so the score is carrying what was learned to versions never taught.
Versions are drawn evenly. The rewarded versions' score is kept apart (`taught_late` in a
summary). `--inv-hold 2,3` sets the held-out versions, `--inv-hold r4` draws 4 per life.
Coin flips score about 0.47 (ties are no move), the scripted player 1.00, on every kind.

**Picture kinds.** A 16x12 bitmap (`--sensor-side 1`, one sensor cell a line) and an ask
line. A trial is one blank step, then one step with the picture and the ask on; only that
step's press counts. The shapes are 4x4 templates of 7 pixels, each drawn at 2x2 lines a
pixel unless said: shape 0 `.### ##.. .#.. .#..`, shape 1 `...# #### .#.. .#..` (they share
5 pixels), shapes 2 and 3 for `--inv-shapes 4`. Every shape is chiral and none is a turn or
mirror image of another. The pair was chosen so a linear reader of the input lines cannot
carry them to a held-out version of any picture kind; most pairs it can.

- **position.** The shape's corner on a 2-line grid, 5 across by 3 down: 15 places. 4
  drawn per life are held out.
- **scale.** About the middle, 1x, 2x, 3x (1, 2, 3 lines a pixel). 1x is held out.
- **rotation.** About the middle, 0, 1/4, 1/2, 3/4 turn, plain and mirrored (versions 0-3
  plain, 4-7 mirrored). Half and three-quarter turns are held out, both ways.
- **bright.** Each pixel lights 4, 2 or 1 of its 2x2 lines. 1 is held out. A linear reader
  of the input lines carries this one (the lit lines are a subset of the taught ones); it
  stays as a control and because the readout does not.
- **clutter.** 0, 3, 8 or 16 extra 2x2 blocks on a 6x6 grid of blocks around and among the
  shape, never on its own pixels, drawn per trial. 8 and 16 are held out. Mostly carried
  by a linear reader too (added cells shift a linear sum only a little).
- **mixed** (place and scale at once). 1x or 2x at one of four places along the middle
  row, two left, two right. Taught: 1x on the left, 2x on the right; held out: 1x on the
  right, 2x on the left. Each place and each size was taught, only not together.
- **view.** Three small 3D objects, 8 points each on a 4x4x4 grid, a post with an arm (along
  x at the top, along z at the top, along x at the foot), seen straight on after turning
  about the vertical axis by k/8 of a turn, each point a 2x2 block. Object 0 seen from the
  front looks like object 1 seen from the side but for one point. Views 2, 3, 4 (a quarter
  to a half turn) are held out.

**With motion** (`--inv-motion P`, percent of all steps; the battery uses 50, stretches of
`--inv-motion-steps` 8). Whenever moving steps are under P percent, a stretch runs before
the next trial: one shape, no ask, no reward, drifting a notch each step through the
versions and bouncing at the ends: sliding a line (position), growing a quarter line
between 1x and 3x (scale), turning 1/16 (rotation and view), one line a pixel dimmer or
brighter, one clutter block more or less, sliding and growing at once (mixed). The drift
passes through held-out versions too; that is what a rule that ties together what follows
what has to learn from. Mirrored and plain never turn into each other.

**Time kinds.** A tune is three notes (A, B, C) in an order and then all three together
for one note, so every note was last on at the same moment: tune 0 is A B C ABC, tune 1 is C
B A ABC (2 and 3: B A C, A C B). A trial is a blank step, two steps in which the tune
plays tick by tick (one step for shift), and the ask step with every note off; only the
ask step's press counts. These sense every tick.

- **shift.** Lines A, B, C, ask (`--sensor-side 3`). Notes 3 ticks; the tune starts 0, 4
  or 8 ticks into its step. 4 is held out.
- **tempo.** Same lines. Notes 2, 4, 6 or 8 ticks, the tune ending as the ask comes on. 2
  (faster than any taught) is held out.
- **pitch.** A 16x8 bitmap (`--sensor-side 1`) whose columns are frequency lines, and the
  ask. Notes 6 ticks; note A is column 2v, B 2v + 2, C 2v + 4 for version v = 0..5. 2 drawn
  per life are held out.

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
