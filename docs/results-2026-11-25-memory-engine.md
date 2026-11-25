# A memory, not a sheet (idea 123), 2026-11-25

The owner asked: "are we limiting ourselves with this sheet of connections idea or no". This
is the test of one different answer: an engine with no sheet and no wiring, whose learning
is storing bit patterns and recalling them by content. Code in `src/memory/`, picked with
`blobrun --engine memory`, numbers with `--y.name value`, script `scripts/memory.sh`.

Short version: with one setting it passes every row of the suite except the place task, most
at 0.98 to 0.99 from the first window: delayed echo, beat, gap, order, exclusive-or, catch,
echo with 16 cues, a second task with the first kept, cartpole balanced 5 to 12 times longer
than a coin. It holds a cue across 4,000 silent ticks. It holds about 1,000 random cue-button
pairs and breaks before 2,000. It is, nearly, a lookup table of everything it has seen, and the one world in the
suite that asks for something it has never seen (place) is a coin per layout. A sheet in
front of it made everything worse.

## The design

At each tick the sensor cells the world wrote are read (only the attached sensor cells).
The input is cut into events where it changes. The address of "now" is ten fields of 1,024
bits: the code of the input now, how long it has lasted, and the code and length of each of
the last four events. A code is the XOR of fixed random rows, one row per sensor cell that
is on (each code bit is the parity of 16 random sensor cells). A length is a level code with
4 buckets per doubling of ticks, neighbouring buckets sharing most bits. The store is 256
tables. Each table reads 10 fixed bits of the address, and those bits are its slot (1,024
slots per table): 1 table kind in 10 reads only the code of now, each other kind reads 5
bits of now and 5 of one other field. So every frame touches exactly one slot per table, with
no search: Kanerva's memory with hashed locations, or CMAC tile coding over random bit
subsets. A slot holds 16 signed 8-bit counters (-31..31), one per button. A new frame is made
when the address changes (a new event, or now's length crosses a bucket) and after each
reward; it sums the counters of its 256 slots per button and drives the motor cells of the
largest. Buttons are learned from the world's `motor_acted` marks: the cells marked together
after a press are one button; a button nobody has pressed yet counts as a sum of 0, so a
context where everything known was bad tries a new one. Ties and exploring (a random button,
chance 2/256 to 32/256 falling as the running good rate rises) are drawn once per reward, so a
whole step acts one way. On good or bad the frames since the last reward (up to 100 ticks
back) are credited, those of the step just answered with the button the world says was
pressed, older ones with the button they drove: good adds 1 to that button's counter in each
active slot, bad takes 2 off, each (table, slot, button) once per reward. Frames from before
the last reward, up to 40 ticks back, are also moved, toward the sign of (reward minus the
running mean reward) with chance |reward - mean| / 2. A second, small store predicts the
next input: a hashed table from (input now, last event, ticks into now) to the input that
followed last time; it fills the shell's `predicted` map, which is what the sequence task
scores. Nothing is ever rewired, nothing is switched off, there is no phase. The setting is
the defaults in `src/memory/memory.hpp`; no flag changes between rows.

All of it is integer: XOR and OR of 64-bit words for the codes, a bit gather per table bit
for the slots, int8 adds for the sums, a hash multiply for the predictor. Deterministic for a
seed; byte-identical on this PC and stevejobs (checked on a delayed-echo life).

## What was tried and dropped (seeds 1-4)

| change | what it did |
| --- | --- |
| threshold code (OR of rows, 4 taps a bit) instead of parity | the same everywhere, place 0.29 (0.46 parity); parity kept |
| credit only frames since the last reward | everything at 0.98-0.99 except cartpole with the lean reward: 12 steps per fall, below a coin (23) |
| credit frames before the last reward with the plain reward | cartpole lean 185, echo 16 cues collapses to 0.08 (wrong answers get the next trial's good) |
| the same, centred on the mean reward, 100 ticks back | cartpole lean 136, sparse 380, echo 16 cues 0.85 |
| the same, 40 ticks back (kept) | cartpole lean 99, sparse 289, echo 16 cues 0.98, beat 0.99 |
| depth 3 (three past events) | the same rows; holds a cue across no other event. depth 4 kept: same or better on every row, holds one |
| 128 tables of 256 slots (512 KB of counters) | every discrete row unchanged; cartpole lean 62, sparse 155. Default kept for cartpole and pairs; this is the cheap version |
| a sheet in front (below) | worse on everything |

## Scorecard

`scripts/suite.sh memory --engine memory`, seeds 5-12, 8 lives a row, the suite's standard
life. Scores were run on stevejobs and are identical here; the capacity and speed runs were
repeated on this PC so the speed rows are from here (load about 38 from other agents). The
scorecard is `out/scorecards/memory.csv`. Best sheet: the `blob-readout` column of
`docs/scoreboard.md` unless named (frozen sheet with the evidence readout).

| row | memory | learning off | chance | best sheet |
| --- | --- | --- | --- | --- |
| echo k=2 | 0.994 ±0.002 | 0.50 | 0.47 | 1.00 |
| echo k=4 | 0.988 ±0.002 | 0.26 | 0.23 | 1.00 |
| echo k=8 | 0.986 ±0.002 | 0.12 | 0.11 | 0.84 |
| echo k=16 | 0.983 ±0.002 | 0.06 | 0.05 | 0.31 |
| capacity, largest k held (2..16) | 16 (all seeds) | 0 | 0 | 8 |
| shuffled echo k=2 | 0.993 ±0.001 | 0.50 | 0.48 | 1.00 |
| delayed echo k=2, 2 steps | 0.992 ±0.003 | 0.50 | 0.48 | 0.55 |
| beat, every 4 steps | 0.988 ±0.008 | 0.39 | 0.51 | 0.51 (minimal) |
| sequence, prediction | 1.000 | 0.34 | 0.50 | 1.00 (minimal; its learning-off twin also 1.00) |
| catch 4x4 | 0.991 ±0.003 | 0.25 | 0.25 | 0.32 |
| cartpole, lean reward (steps per fall) | 109 ±48 | 22 | 23 | 17.6 |
| cartpole, sparse reward (steps per fall) | 279 ±255 | 9 | 23 | 18.8 (minimal) |
| order | 0.992 ±0.003 | 0.50 | 0.48 | 0.69; 0.85-0.90 on the richer sheets |
| exclusive-or (both-not-either) | 0.993 ±0.001 | 0.50 | 0.48 | 0.88 |
| gap | 0.994 ±0.002 | 0.50 | 0.47 | 0.52; 0.93 on the richer sheets |
| place (a place never rewarded) | 0.628 ±0.339 | 0.50 | 0.48 | 0.48 (blob-main), 0.25 (readout) |
| echo, cue and button 1/2/3/5 tiles apart | 0.994 each | 0.50 | 0.48 | 1.00 / 0.91 / 0.85 / 0.59 |
| echo A at its end / B at its end | 0.994 / 0.988 | 0.50 / 0.25 | 0.47 / 0.23 | 1.00 / 0.99 |
| A retested after B, first window / end | 0.997 / 0.996 | 0.50 / 0.51 | 0.46 / 0.48 | 0.98 / 1.00 |
| echo before / after 400,000 ticks unplugged (first window, end) | 0.994 / 0.994, 0.993 | 0.50 | 0.47 | 1.00 / 1.00 |
| ticks/s in the echo k=2 life | 186,603 | 266,067 | | 63,337 (readout), 227,054 (minimal) |
| ticks/s, random input every tick, 8x8 / 16x16 / 32x32 tiles | 158,868 / 165,918 / 111,701 | | | readout 64,472 / 14,638 / 3,683; minimal 214,196 / 53,024 / 12,146 |

Rows that mean nothing for this engine: "cue readable at N tiles" reads activity spreading
through a sheet, and there is none, so it is 0 (should be shown n/a); the distance rows pass
because the memory has no geometry at all, not because it reaches; the room and aliveness
rows describe a sheet (the memory's "neurons" are only the sensor and motor cells, 0.5% of
the shell fire). Bytes per neuron is meaningless; the size is below.

Place per seed (5..12): 0.99, 0.01, 0.99, 0.49, 0.81, 0.64, 0.77, 0.33. Seeds 1-4: 0.46,
0.52, 0.49, 0.36. The held-out shape lands in slots shared with whatever stored pattern it
overlaps most, which depends on which places were held out; on some layouts that is the
right shape and on some the wrong one. That is a coin per layout, not generalization. Over
the twelve seeds the mean is 0.57.

## Against the fixed reference networks

The reference networks landed on main while this ran (`gru-fixed`, `mlp-fixed`,
docs/results-2026-11-25-fixed-net-reference.md; same suite, seeds 5-12, one setting). The
plain GRU already passes most of what the memory passes, a little higher (0.998-0.9998
where the memory sits at 0.98-0.99, which is its exploring floor of 2/256). Where they
differ:

| row | memory | gru-fixed | mlp-fixed |
| --- | --- | --- | --- |
| catch 4x4 | 0.991 | 0.722 | 0.493 |
| cartpole lean / sparse (steps per fall) | 109 / 279 | 12.0 / 14.2 | 17.3 / 22.3 |
| sequence, prediction | 1.000 | 0.50 | 0.50 |
| place | 0.63 (a coin per layout) | 0.16 | 0.16 |
| exclusive-or | 0.993 | 0.966 | 0.996 |
| delayed echo, beat, gap, order | 0.99 each | 0.999 each | 0.50, 0.64, 0.49, 0.50 |
| ticks/s in the echo k=2 life | 186,603 (integers) | 35,507 (floats) | 233,847 (floats) |

So the memory is the first design in the project that learns everything the GRU learns
with the same setting, and it also learns catch and cartpole, which the GRU does not (the
GRU's cartpole failure is the lean-reward lag below plus no credit across steps; the
memory's credit 40 ticks back is what carries it). The GRU reference said its walls are a
held-out place, a hold longer than its backprop window, and the cartpole reward. The memory
passes the second (4,000 ticks, any length of silence) and works around the third; the first
stands for both.

## The walls, against the sheets

| wall | sheets (best) | memory |
| --- | --- | --- |
| holding a cue across a gap | 20 ticks; a ringing sheet 60 at a third of the sharpness; nothing 150 | delayed echo 0.99 at 2, 10, 50 and 200 blank steps (40 to 4,000 ticks; seeds 5-12: 0.992, 0.992, 0.994, 0.998, learning off 0.49-0.51) |
| holding it across other events | not tested | depth 4 is the wall: none 0.993, one other line blinking in the gap 0.994, two 0.512, three 0.542 (learning off 0.48-0.50; seeds 5-12) |
| cue-button pairs | 8 one-hot cues, 16 at 0.31 | 16 one-hot at 0.98. Random 8-of-56-line patterns, each to one of 16 buttons (`memprobe pairs`, seeds 5-12, learning off 0.06): 16 pairs 0.983, 64 0.980, 256 0.901, 1,024 0.882, 2,048 0.376, 4,096 0.135. About 1,000 pairs held; it breaks between 1,024 and 2,048, when the 1,024 slots of each now-only table are shared by two patterns or more |
| a second task after the first, the first again | 0.99, first kept 0.98 | 0.988, first kept 0.997 |
| exclusive-or, order, gap | 0.88, 0.69-0.90, 0.52-0.93 | 0.993, 0.992, 0.994 |
| tries to learn echo, 2 cues | readout sheet: 2-3 errors (first 50 steps 0.94-0.96); minimal 700-2,200 steps; table 150 steps to never | 0.6 errors (0, 1, 0, 0, 2, 1, 0, 1), learned by step 1.4 on average |
| tries to learn echo, 8 cues | readout sheet: 0.8 after 3,600-11,200 steps, ends 0.71-0.84 | 62 errors, learned by step 128 (k=4: 10 errors, 24 steps; k=16: 360 errors, 583 steps) |
| a shape in a new place | 0.25-0.48 | a coin per layout (0.63 ±0.34; 0.01 to 0.99) |
| cartpole | below a coin | 109 (lean), 279 (sparse) steps per fall; coin 23; the scripted player never falls |
| catch | 0.32 | 0.99 |

"Learned" in the tries rows is the first step after which 20 steps hold at most one error
(`out/memory/tries.py`); the sheet rows are the first 50-step (k=2) or 100-step (k=8) window
at 0.9 or 0.8.

## The cartpole lean reward judges the step before

Found on the way and it matters for every engine: in `cartpole_step` the cart position and
the pole angle advance with the speeds from before the push (`cart_x += tau * cart_v`, then
`cart_v += ...`), and the lean reward compares the position and angle before and after. So
the reward of a step does not depend on that step's press at all; it judges the press one
step earlier. A rule that credits only the step it is answering learns noise from it: the
memory with that rule balanced for 12 steps per fall, below a coin (23), exactly where the
sheets sit (14 to 18). Letting credit reach 40 ticks back, two steps, took it to 99-119 on
seeds 1-4 and 109 on 5-12. The world should give the lean reward one step later, or from the
speeds after the push, before any engine is judged on that row again.

## The combination: the memory reading a sheet

`--y.sheet 1` puts a minimal-engine sheet held fixed in front of the memory, `--y.sheet 2` a
learning one (fed the same rewards). The world's sensor bits are copied onto the sheet every
tick and the code of now is made from the sheet's spikes in the tiles within one of the
sensor tiles (a cell on if it fired in the last 4 ticks), recomputed every frame; events are
still cut where the sensed input changes. Seeds 5-12:

| row | memory alone | memory on a fixed sheet | memory on a learning sheet |
| --- | --- | --- | --- |
| echo k=2 | 0.994 | 0.849 | 0.860 |
| echo k=8 | 0.986 | 0.638 | 0.817 |
| delayed echo | 0.992 | 0.621 | 0.524 |
| exclusive-or | 0.993 | 0.753 | 0.793 |
| catch | 0.991 | 0.373 | 0.477 |
| place | 0.628 | 0.358 | 0.334 |

Counting spikes over 16 ticks instead of 4 (seeds 1-4) was worse still (echo 0.56). A
spiking sheet gives the memory a noisy view of the same information: the same input no
longer gives the same code, so recall stops being exact, and the sheet offers nothing that
shares across places either (place goes down, not up). What the memory wants from a partner
is not mixing, which its own random code already does completely, but a code that is the
same for things that should be treated the same (a shape anywhere). None of our sheets makes
that yet.

## Speed and bytes

Default store: 4.97 MB. 4.19 MB of it is the counters (256 tables x 1,024 slots x 16
buttons x 1 byte), 0.56 MB the predictor (4,096 entries of a tag and 16 input words), 64 KB
the slots of the last 128 frames, 0.13 MB the code rows and level codes. The cheap version
(128 tables x 256 slots) is 1.27 MB, 0.71 MB without the predictor. None of it grows with the
sheet size; the ticks/s fall at larger sheets only because the shell's spike map is cleared
and counted every tick.

Per frame: 256 x 10 bit gathers and 256 x 16 int8 adds. A frame is made about 15 times in a
20-tick step (the length field crosses a bucket every tick at first), so in a life most ticks
do no store work at all: 187k ticks/s in the echo life, 159k with a new random input every
tick, on this PC with about 38 other jobs on it. A reward touches up to 256 slots per
credited frame.

How it would sit on other machines:

- GPU: the tables are independent, so one thread per table (or one warp per 32 tables) does
  the bit gather and reads one 16-byte line of counters; the per-button sums are a
  reduction. Many lives or many agents run as one batch, each frame touching 256 lines of
  16 bytes, which is a gather-heavy but small and regular access. Learning is the same
  addresses written back.
- FPGA: each table is one block RAM addressed directly by its 10 (or 8) code bits, which are
  wires from the address register, so all tables read in the same clock; 16 adder trees sum
  the int8 outputs. The cheap version needs 128 blocks of 256 x 128 bits (one 36 Kb block
  RAM each), well inside a mid-range part, and would make a frame a clock. The code is an XOR
  tree per bit over the sensor lines; the predictor is one more RAM with a hash in front.

## Exact lines

```
scripts/memory.sh suite                        # = scripts/suite.sh memory --engine memory
scripts/memory.sh task delayed-echo --delay 200 --steps 200000 --window 5000 --seed 5
scripts/memory.sh tries                        # echo k=2, the first 40 steps, seeds 5-12
scripts/memory.sh pairs 1024 5                 # one seed of the pairs ladder, and learning off
scripts/memory.sh hold 1 5                     # one other line blinking in the gap
scripts/memory.sh ladder                       # pairs 16..4096 and hold 0..3, seeds 5-12
scripts/memory.sh sheet place 5                # memory alone, on a fixed sheet, on a learning one
scripts/memory.sh speed                        # ticks/s with random input, and the store's bytes
scripts/memory.sh task echo --y.tables 128 --y.table_bits 8      # the cheap version
```

The tune tables (seeds 1-4) came from `out/memory/tune.py`, the delays from
`out/memory/delay.sh`, the k=2..16 tries from `out/memory/tries.py`, the sheets' tries from
`out/memory/sheet_tries.sh`; all in the worktree's `out/memory/`, not committed.

## What it means, plainly

Yes, the sheet of connections was limiting us, in a specific way. Every wall in
`docs/findings.md` except one was about storing and holding: a cue held 20 ticks, eight
pairs, slow learning of a single association, no carrying a second task. A table of small
counters found by a hash of "what is here now and what happened just before" takes all of
those down at once, learns from one or two presentations, and costs a few megabytes and a
few thousand integer operations per decision. The time walls fell because the memory keeps
events, not ticks: silence costs it nothing, however long, but each new event pushes an old
one out, and with four kept a cue survives one intervening blink and not two.

It is not general learning. On these worlds it is a lookup table of everything seen (the
current input, the last four events and their lengths), with counts per button. That is not
cheating by the bench's rules: nothing about any task is built in, one setting runs every
row, and it learns online from good and bad. But it means most of the suite only asks
"can it store and recall", which a hash table answers trivially. The one world that asks for
something never seen, place, gives a coin per layout. Cartpole is the closest to a real test
of generalizing (the place-coded state has about 1,300 cells and continuous dynamics), and
there it balances 5 to 12 times longer than a coin while the scripted player never falls.

So the next thing to learn is not storage. Two directions follow. First, the suite needs
worlds where nothing repeats exactly (noise on the input, held-out versions, real data such
as MNIST, idea 126) so that a memory and a learner can be told apart. Second, the right
organs look like the brain's own split: a store like this one beside something that makes
codes which are equal for things that should be treated the same. A random spiking sheet is
not that (it made the memory worse everywhere); a shared code such as copying one patch's
features to every place (idea 125) might be, and the memory would then supply the holding,
the one-shot learning and the capacity that the sheets lack.

## Rough

- The pairs world and the hold world live in `memprobe` (the engine's own folder), not in
  the world, so other engines cannot run them yet. They use the plug and the shell exactly as
  blobrun does. Worth moving into `src/world/` as tasks.
- Buttons are found from `motor_acted`; the learning-off twin still finds them, which is
  bookkeeping, not learning of the task.
- At most 16 buttons (the world's own most), 4 past events, 1,024 sensor cells read.
- The suite's reach, room and aliveness rows should be n/a for this engine; it has no
  sheet. Its reach rows print 0.
- The sheet-in-front code recomputes the code of now every frame, so it makes about as many
  frames as ticks in some tasks; it is a quick test, not a design.
- Speed numbers are from a loaded machine (load about 38).
