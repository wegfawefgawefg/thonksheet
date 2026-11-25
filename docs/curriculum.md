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

## The function battery

Worlds where the right button is a fixed function of what was shown, cheap to run and
graded in difficulty, so we can see exactly where a sheet stops (`scripts/functions.sh`,
results in `docs/results-2026-11-25-function-battery.md`). Four task kinds, one trial shape:
one blank step that parts it from the last trial, the steps that show the input, and an
answer step with the ask line on. Only the press on the answer step counts; presses before
it are ignored. The right button is good, any other bad, no move a miss. Score: right
answers over trials. Everything is integer, so the same flags give the same csv.

How inputs are drawn: uniformly, when that already gives every answer equally often
(parity, for instance); otherwise an answer is drawn first, uniformly among the answers
some input has, then an input with that answer, uniformly. So always pressing one button
scores one over the number of answers, and that is the chance level the ladder uses. Coin
flips score a little under it (ties are no move); the scripted player scores 1.00 on every
member (checked on all 79, 20,000 steps).

- **Points** (`--task points --fn F --fn-lines N`, N from 1 to 10). N cue lines, each on or
  off, shown together with the ask line for one step. Trial: 2 steps. Two buttons (button 1
  when F is true) for:
  - `and` (all on), `or` (any on), `parity` (an odd number on), `majority` (more than half
    on; half of an even N is not), `exactly` (exactly `--fn-k` on, default 1);
  - `table`: a truth table drawn once per life from the task seed, any but the two
    constant ones; `table-lin`: a random threshold unit (a weight of plus or minus 1 to 4 on
    every line, the bar at the weighted sum of one random input; redrawn if constant), so
    one threshold unit can always do it; `table-nonlin`: a random table that a perceptron
    on the lines cannot get in 2,000 passes, so one threshold unit cannot (exact for
    N up to 4; for larger N almost every table is like this anyway).
  And with more buttons: `count`, how many lines are on (N + 1 buttons); `first`, the index
  of the first line on (N buttons; all off is never shown). `points --fn parity
  --fn-lines 2` is both-not-either exactly: the same lines in the same places, the same
  draws, a byte-identical csv.
- **Number** (`--task number --fn F --fn-lines L --fn-bands K`). x, a whole number from 0
  to L - 1, lights one line of L (place code: one line per value). The lines sit on a sheet
  8 wide in a snake (left to right, then right to left on the next row), so x and x + 1
  are always neighbours; at 3x3 sensor groups only 5 fit a row, so the battery uses 2x2.
  f is worked out at the centre of x's share of [0, 1), u = (2x + 1) / 2L, in units of
  1/4096: `identity` u; `step` 0 below the middle, the top above (always 2 buttons);
  `tent` up to the middle and back down; `sine` and `cosine` 0.5 + 0.5 sin or cos of one
  full turn (a table of 128 values); `square` u squared. The answer is which of K equal
  bands of [0, 1) f falls in, one button each. `--fn-hold H`: H values of x, drawn once per
  life from the task seed, are never rewarded, and only trials on them are scored (as in
  place). A held value lies inside a band (both neighbours taught and in the same band, so
  filling in between taught values gives the right answer), and no two are side by side.
  Trial: 2 steps. With place coding every taught x is its own line, so on taught values
  this is a lookup a linear reader of the input lines can always do; the held-out values
  are the real question.
- **Two numbers** (`--task two-numbers --fn F --fn-lines L --fn-bands K`, L up to 32). x1
  and x2 the same way, x1's rows of the sheet above x2's. `sum` (x1 + x2), `diff` (x1 - x2,
  signed), `larger` (the larger of the two), `product`: the band of K over the result's
  whole range. `compare`: button 0 if x1 is larger, 1 if x2 is; equal pairs are never
  shown. Sum, difference and compare are a weighted sum of the input lines; product and
  larger need lines to be combined.
- **Past** (`--task past --fn F --fn-gap G`). Lines A, B and ask. Each item lights A or B
  (a coin flip) for one step; items are G blank steps apart, so (G + 1) x ticks-per-step
  ticks (20 x (G + 1) in the battery); the last item comes with the ask. `recall`: button
  1 if the first of 2 items was B (holding one item); `same`: the last 2 items are the same
  (exclusive-or in time); `majority` (more B than A) and `parity` (an odd number of B)
  over `--fn-items` items (default 3, up to 4). Trial: 1 + items + (items - 1) G steps.

The battery (`scripts/functions.py`, `members`) runs points with N = 2 to 8 (and, or at 2,
4, 8; parity at 2, 3, 4, 5, 6, 8; majority, exactly-1 and both tables at 3, 4, 5, 6, 8;
count at 2, 3, 4, 6, 8; first at 3, 4, 6, 8), identity at L = 8, 16, 32 and K = 2, 4, 8,
the other one-number functions at L = 16 (step K = 2, the rest K = 4), the same with 3
held-out values (identity also at K = 2), two numbers at L = 8 with K = 2 and 4 (compare
with 2), and the past functions at gaps 0 and 2: 79 members. Each is scored three ways:
(a) `blobprobe reader`, a ridge reader of every non-sensor neuron's spike count over the
answer step of a frozen sheet as born, fitted on the first 60% of 3,000 trials, the ridge
picked on the next 15%, scored on the last 25% (and on every held-out trial), beside the
same reader on the input lines of every step of the trial; (b) the readout rule on a
frozen sheet; (c) the minimal engine and table neurons with their own rules. A member is
"got" at a level when its late score is at least halfway from chance to 1.0.

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

## The rest of the map (tiers 9 and 10, being shown, damage; 2026-11-25)

The kinds of task `docs/task-map.md` had no world for. Code: `src/world/task_script.cpp`
runs every one of them as a trial script (a list of steps, each with the lines on, the
button wanted or none, and whether it is scored, rewarded, or kept by the offline reader);
one file per family draws the trials. All integer, all seeded by the task seed, so the same
flags give the same csv. The table of worlds and their two settings is
`scripts/map_worlds.py`; `scripts/map.sh` scores them all (`docs/notes-world.md`).

Shared rules. A trial of the relation, number, parts and memory worlds starts with one
blank step that parts it from the last (as in the feature tasks); the echo-shaped worlds
are one step per try with no blank, like echo. Only a step that wants a button counts;
presses on other steps are ignored. A right press is good, a wrong one bad, no move is a
miss with nothing said. Score: right over scored answers, unless said. Items in the
relation worlds are w-of-n patterns, so every item lights the same number of lines and
brightness says nothing. Held-out items (`--hold N`): N items drawn per life are never
rewarded; one trial in four is made only of them, and only those trials are scored, so the
score is carrying the relation to items never taught on. Whether an answer can be had
without the sheet is checked with the offline reader on the raw input lines of the last 16
steps and on their brightness (`docs/notes-world.md`, "The rest of the map").

### Relation

- **same** (`--item-lines n --item-on w`): two slots of n lines and the ask, shown together
  for one step. Button 0 same, 1 different (half each). Easy: n 4, w 1. Hard: n 6, w 3, 6 of
  the 20 items held out.
- **same-then**: one slot and the ask. Item A for a step, item B on the same lines, then
  the ask alone. Easy and hard as same.
- **odd-one** (`--k` slots): k slots and the ask, one step. All slots show one item but one,
  which shows another; press the odd slot. No slot alone tells (each item is as likely to be
  the odd one as the common one). Easy: 3 slots of 4 one-line items. Hard: 4 slots of 2-of-5
  items, 4 held out.
- **bigger** (`--item-lines` levels, `--diff-max`): two rows of levels, one line lit in each,
  and the ask, one step. Button 0 if the first is bigger, 1 if the second; never equal.
  Easy: 6 levels, any difference. Hard: 10 levels, always neighbours. It is linear in the
  input (the reader on the lines gets it), and one row alone tells a little at the ends (a
  value of 0 is never the bigger one): this is the easy relation.
- **analogy** (`--k` changes): two slots of 6 lines and the ask. A step with A in slot 1 and
  B in slot 2; then for each of k candidates a step with C in slot 1 and the candidate in
  slot 2; then the ask. B is one of k changes of A (shift right by one around the end,
  mirror, light the lowest unlit line); the candidates are the k changes of C in random
  order; press the candidate made by the change that made B. Items: 2-of-6 patterns that
  every change moves to something new. Easy: shift or mirror, 2 candidates. Hard: all three
  changes, 3 candidates, C from 3 held-out items. "Add" makes a brighter candidate and a
  brighter B, so on add trials brightness helps once A to B is seen; noted, not removed.

### Number (a 7x7 sheet at 2x2 cells per line; it does not fit at 3x3)

- **how-many** (`--k` most, `--varied`): 1 to k items, none touching another even at a
  corner, shown with the ask for one step; press button n - 1. Easy: 1 to 3 single cells
  (brightness gives it, by design). Hard: 1 to 5 rectangles of 1-3 by 1-3 cells whose total
  area is always 6 to 9 cells.
- **more-fewer**: a display, then a second one with the ask; button 1 if the second has more
  items, 0 if fewer (1 to 5 each, never equal). Easy: single cells. Hard: as how-many hard.

### Parts and wholes

- **grammar** (`--length` most): Reber's grammar on lines B T P S X V E and the ask, one
  symbol a step, then the ask. Allowed (half): a walk through the grammar, each fork a coin
  flip, at most `length` symbols. Broken: an allowed string with one inner symbol changed to
  another inner symbol so it is no longer allowed (same length, start and end). Button 0
  allowed, 1 broken. Easy: up to 8 symbols. Hard: up to 14.
- **brackets** (`--length`): lines ( and ) and the ask. Every string has `length` symbols,
  half of each, starts with ( and ends with ). Allowed: balanced. Counts, length, first and
  last symbol are the same for both answers. Easy: 8. Hard: 12.
- **parts** (`--k` kinds, `--varied`): parts are three-cell shapes (a bar down, a bar across,
  a corner). Easy: a 6x3 sheet, one of 2 kinds in each half at a fixed spot; press the
  button of the ordered pair (4 buttons). Hard: a 6x6 sheet, two of 3 kinds anywhere,
  touching along an edge, not overlapping; press the button of the unordered pair (6
  buttons); a figure that another pair could also make is drawn again. Always 6 cells lit.

### Chance

- **noisy-echo** (`--right-pct`): echo k=2, but a right press is rewarded only `right-pct`
  times in 100 and punished otherwise, a wrong one the other way. Scored on the true answer.
  Easy 80, hard 60.
- **rare-cue** (`--rare N`): echo k=2, cue 1 comes one try in N. Score balanced: the mean of
  the two cues' hit rates, so always pressing button 0 scores 0.5. Easy 10, hard 40.
- **noisy-lines** (`--k`, `--item-lines`, `--flip-pct`): k fixed patterns on 8 lines, half
  lit (for two, one is the other inverted), drawn per life; every step each line is flipped
  with chance `flip-pct` in 100. Easy: 2 patterns, 10. Hard: 4 patterns, 25.

### Context

- **context** (`--k`, `--context-early`): k cues and two context lines. Context 0: cue i is
  button i; context 1: button (i + 1) mod k. Neither line alone tells. Easy: k 2, context and
  cue on the same step (exclusive-or in a new dress). Hard: k 4, the context alone a step
  before the cue.
- **switch-rule** (`--switch-every N`): echo k=2 whose rule moves on (cue i means button
  i + shift) every N tries, no signal. Scored over all tries, so it measures how fast the
  change is followed. Easy 500, hard 50.

### Longer memory

Output across several steps is a run of answer steps, one press each, each judged and
rewarded at once; the plug is unchanged.

- **copy** (`--k`, `--length` most): k symbol lines and a go line. A blank, 2 to `length`
  symbols one a step, then the go line for as many steps: on the i-th go step press the i-th
  symbol. The go steps look the same, so the sheet must keep its place. Scored per press.
  Easy: k 2, always 2 symbols. Hard: k 4, 2 to 6.
- **recall** (`--k`, `--fill`): k symbol lines, a mark line, the ask. A blank, the target
  with the mark, `fill` steps each showing a random symbol, then the ask: press the marked
  one. Easy: k 2, 1 between. Hard: k 4, 4 between.
- **n-back** (`--k`, `--back`): a stream of k symbols with no blanks; each step press 1 if
  this symbol is the one `back` steps ago, else 0; half match, a non-match is any other
  symbol. Easy: k 4, 1 back. Hard: k 4, 2 back.

### No teacher (no good or bad at all)

Symbols are 2-of-4 patterns on 4 lines with every line lit by half of them, so "all off" or
"as before" predicts half the cells right. The world's own score uses the sheet's
predictions of its sensor cells (what each sensor cell would have done from the sheet's
input, `blob.predicted`); the minimal engine keeps none, so it has no score here. The offline
reader reads the answer from the sheet's activity on steps the world marks.

- **predict-next** (`--k`, `--markov-order`): the next symbol is a fixed function of the last
  one (order 1) or of the last two (order 2; the last alone never fixes it) 7 times in 8,
  any other symbol otherwise. Score: share of sensor cells predicted right on each step's
  first tick (as sequence). Reader label: the next symbol; the best possible is 0.875. Easy:
  order 1. Hard: order 2.
- **fill-in** (`--k`, `--item-lines`): k half-lit patterns on 8 lines, drawn per life; one step
  in 4 the second half is blanked. Score: over the blanked steps' ticks, the share of hidden
  cells whose own prediction is on when they should be on, less the share on when they should
  be off (0 for any blind guess, 1 for a perfect fill; checked: 1.00 when the scripted player
  can see the cells). Reader label: which pattern, on blanked steps (the visible half gives it,
  so the reader is a weak test here; the score is the real one). Easy: k 2. Hard: k 6.
- **odd-event** (`--k`, `--rare`): k symbols in a cycle drawn per life; one step in `rare`
  shows another symbol instead. Score: wrong predictions per sensor cell on break steps less
  on regular ones (0 when the break goes unnoticed). Reader label: break or not, on every
  break and on the regular step one cycle after it (half each). No single step's symbol
  tells. Easy: k 4. Hard: k 6. Both one step in 8.

### Being shown the answer

`--shown N` on any button task: for `--steps` the task runs with no good or bad; on every
answer step the right button's motor cells are made to fire for the last N ticks
(`blob.forced`, written by the world before each tick and ORed into the fired word in every
engine's fire step; no urge array was on main). Then `--retest` steps with no help and no
good or bad. Reported: the test's first window, its mean (the headline) and its end. The
learning-off twin sees the same. Rows: echo k=2, echo k=4, both-not-either, N 6.

### Damage

`--damage P`: the task for `--steps`, then P in 100 of the sheet's non-sensor neurons (motor
cells included) are silenced for the rest of the life (`blob.silenced`, ANDed out of the
fired word in every engine's fire step; drawn from the seed), then the task again for
`--retest` steps, learning on. `--damage-tile 1` silences every non-sensor cell of the tile
holding the first cue line instead. Reported: before (end of the first phase), at once (the
first window after) and after the retest (its end). Rows: echo k=2 at 5, 20 and 50 in 100
and the cue tile; both-not-either at 20.
