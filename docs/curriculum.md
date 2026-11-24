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
   other is bad. Start with K = 2.
2. **Shuffled echo.** Same, but which button matches which line is shuffled per life.
3. **Delayed echo.** The cue goes off; the answer is wanted some ticks later. First test of
   holding something over time.
4. **Go / no-go.** Press on one cue, hold still on another.
5. **Beat.** Press once every N ticks with no cue. First test of the sense of time.

## Stage 2: prediction and recognition

6. **Sequence.** A repeating pattern plays on the sensor lines. No reward. Score is surprise
   going down.
7. **Shapes.** Small bitmaps appear on the sensor sheet at random places. Press the button
   for the shape. Tests whether a feature learned in one place carries to another.
8. **Counting.** N pulses, or N dots; press button N.
9. **Rhythm and tune.** Frequency lines over time; name the pattern. The sound-shaped task.
10. **Mirroring.** Copy a pattern from the sensor sheet onto a motor sheet; follow a moving
    target.

## Stage 3: control

11. **Catch.** Move a paddle under a falling dot.
12. **Cartpole.** Balance.
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

## Stage 5: genome search

Many blobs, each living a full life across unseen, shuffled tasks. Score is late minus
early. Not started until one hand-set blob has visibly learned something in stages 1 to 3;
with nothing learning there is nothing for a search to climb.
