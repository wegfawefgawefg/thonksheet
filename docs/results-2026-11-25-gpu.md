# The minimal engine on the GPU, many lives at once (2026-11-25)

`blobgpu` runs thousands of independent minimal-engine lives (different seeds, different
genome numbers) on the RX 9070 XT through Vulkan compute, headless, together with their Echo
task. Every life is the CPU engine bit for bit, given one switch on the CPU side
(`--m.rng_mode 1`, keyed random words). `scripts/gpu.sh` runs it.

## What it is

- **One workgroup is one life.** 256 threads step one 4,096-neuron sheet; a dispatch steps
  every life of a group by up to N ticks. Lives never talk to each other, so a workgroup
  barrier is the only sync, and a dispatch can run many ticks.
- **Layout.** Every buffer is life-major, and inside a life it is the CPU engine's own
  layout, so one life's block is a straight copy of `MinimalBlob`: masks
  `((tile * 9 + slot) * 64 + bit)` as 64-bit words, thresholds one byte per neuron, the
  spike and noise rings (32 maps of 72 words), the inhibitory words. A small per-life
  struct holds the clock, the reward average, the task and the ticker. Per-life genome
  numbers sit in their own buffer (one struct a life), so a batch can be a population.
  The per-batch table (sensor cells, each line's cells, each button's cells) is shared.
- **64-bit words are 64-bit in GLSL** (`shaderInt64`); popcount is two 32-bit `bitCount`s on
  the halves. Shared-memory 64-bit `atomicOr` packs the fired bits.
- **One tick in the shader**, in blobrun's order: the ticker writes the lines (good, bad,
  the task's), the random words for noise, regulation and co-activity are made (four jobs
  per tile spread over the threads), every neuron counts its 9 slots against its threshold,
  each tile's new map goes into the ring, thresholds move, co-activity swaps, motor spikes
  are counted, and on a step's last tick thread 0 does the task's step (choice, good or bad,
  the window tally) and the reward's running average; then the graded neurons swap, each in
  parallel (a swap only touches its own neuron's masks).
- **The world is in the shader too**: echo with k cues, shuffled echo, delayed echo and
  both-not-either, with the plug's lines, the choice (most spikes, tie is no move), good
  and bad lines for `--reward-ticks`, rest ticks, cue offset, the window scores.
- **Births are on the GPU** as well (`minimal_birth.comp`, one thread per life drawing the
  life's own stream in the CPU's order). 16,384 births: 34 s on one CPU thread, 0.6 s here.

Files: `src/gpu/` (host: headless Vulkan, the batch, births and world tables, the CPU
reference, flags, csv), `shaders/gpu/` (`minimal_life.comp` the ticks, `minimal_birth.comp`,
`minimal_learn.glsl` the swaps, `minimal_world.glsl` the task, `minimal_random.glsl`,
`minimal_layout.glsl`). CMake block at the end of the root file, skipped without Vulkan or
with `THONK_NO_VIEW`.

## The random words

The CPU engine draws every random word from one xoshiro stream per life, in a fixed order
that depends on what happened (how many neurons fired, which were picked). A GPU cannot make
that order. So the CPU engine got a switch, `--m.rng_mode 1` (src/minimal/minimal_keyed.hpp):
every draw site restarts the stream from a key of (life, tick, what is drawn, tile or neuron,
age) and then draws in the plain code's order. Six sites: the noise word per tile, the
regulation words per tile, the co-activity word per tile, a co-activity swap per neuron, the
grading word per tile and age, a graded swap per neuron and age. Birth still uses the life's
own stream. The shader makes the same words.

- `rng_mode 0` (the default) is unchanged: four lives of 4,000 steps (echo; shuffled with
  grade_mode 1 and no budget; both-not-either with evidence counters and noise_pair; echo
  k=4 with far slots, delays and drop_mode 1) give byte-identical window csvs before and
  after the change.
- `rng_mode 1` learns as well (echo k=2 seed 1, 20,000 steps: 0.97 late), but its lives are
  other lives than mode 0's: compare designs within one mode.
- On the CPU, mode 1 costs about 25% of tick speed (three reseeds per tile per tick).

## What is bit-exact, and how it was checked

`blobgpu --check` steps the CPU engine alongside, driven by the real plug, task and ticker
code (`src/run/task_ticker.cpp`, the same loop as blobrun's life), and after every 16 ticks
compares: the spike and noise maps of each of those 16 ticks, the whole rings, every mask
word, every threshold, the reward average and the task's dice, cue and phase; before the
first tick it compares the newborn sheets (masks, thresholds, inhibitory words); at the end
the window scores. `scripts/gpu.sh check` runs nine cases, first on lavapipe with
validation, then on the GPU with validation:

| case | what it covers |
| --- | --- |
| echo2 | echo k=2, 2 lives |
| shuffled-twin | shuffled echo with learning-off twins |
| settings-echo4, -delayed, -both | four lives with different genome numbers each (grade_mode 0/1, budget on/off, budget 24 to 200, noise 2^-4 to 2^-11, learn_shift 0 to 2, noise_sign -1, cowire off, cowire_needs_input 0, credit 5 to 30, avg_shift 0 to 8, up_shift 2 to 5, inhibitory 10 to 40%) on echo k=4, delayed echo, both-not-either |
| rest-offset | echo k=3, cue offset 1, 3 rest ticks, good/bad lines 4 ticks |
| tiles16 | a 16x16-tile sheet (16,384 neurons), 8 ticks a step |
| workgroup64, workgroup1024 | other thread counts per life |

All nine: **identical, every tick, on lavapipe and on the GPU, zero validation messages.**
Separately, window csvs of three shuffled-echo lives (1,500 steps) from lavapipe are
identical as text to `blobrun --engine minimal --m.rng_mode 1`. (Full-length lives against
blobrun: below.)

Not identical: the stats counters the CPU keeps (gained, lost, graded, cowired) are not kept
on the GPU; nothing reads them in a life.

## Speed

`scripts/gpu.sh speed`: echo k=2, 20 ticks a step, 3x3 groups, 2,000 steps a life (40,000
ticks), seeds 1..N, births on the GPU. Life-ticks per second summed over all lives, timed
over the stepping; lives per second over the whole batch (births, upload, stepping, read
back). RX 9070 XT, 256 threads a life. The CPU was loaded the whole time (load average 34 to
38 on 32 threads, other agents' runs), which barely touches the GPU figures and pulls the
CPU ones down.

| lives | GPU, learning on | GPU, learning off | lives/s on | lives/s off |
| --- | --- | --- | --- | --- |
| 1 | 46,100 | 58,000 | 0.94 | 1.13 |
| 64 | 2.89M | 3.58M | 58 | 68 |
| 1,024 | 9.41M | 11.0M | 223 | 258 |
| 16,384 | 9.48M | 10.95M | 234 | 269 |

The CPU engine on one core, same task, same session (`blobgpu --device cpu`, the real
life loop, birth not counted):

| | learning on | learning off |
| --- | --- | --- |
| rng_mode 0 (today's runs), load 38 | 104,000 | 136,000 |
| rng_mode 0, earlier in the session at load about 20 (blobrun life) | 200,000 to 214,000 | |
| rng_mode 1 (keyed words), load 38 | 65,000 to 104,000 (noisy) | 132,000 |

- **The GPU is faster per life-tick than 16 cores would be.** At the best single-core speed
  seen here (214,000), 16 cores would give at most 3.4M life-ticks/s if all ran at full
  speed at once, which they do not; the GPU gives 9.5M with learning on, 2.8 times that,
  the work of about 44 such cores. Against today's loaded single core it is 91 cores.
- **It saturates at about 1,000 lives.** One life alone is slower than on a CPU core (one
  workgroup of 256 threads, about 46,000 ticks a second); 64 lives use a fraction of the
  card.
- **Largest batch run:** `scripts/gpu.sh big`, 16,384 lives and their 16,384 learning-off
  twins (32,768 lives, 10.8 GB), echo k=2, 40,000 steps each (800,000 ticks, the length
  scripts/minimal.sh uses): 26.2 billion life-ticks in 40 minutes, 10.9M life-ticks/s, no
  amdgpu line in the kernel log. Late score over the 16,384 seeds 0.947 (sd 0.021; 97.1%
  at or above 0.9, none below 0.8), twins 0.459 (sd 0.056); the CPU results doc had 0.942
  on seeds 5 to 12. Seeds 5 to 12 of this batch, all 40 windows each, are identical as
  text to `blobrun --engine minimal --m.rng_mode 1` run on the CPU.

Where the time goes (ablations, timing only): the fire loop is most of it (with the fire
loop removed the rest runs at 44M); reading every mask whether its source tile spiked or
not is 4 times slower, so skipping silent source tiles carries most of the speed; masks
forced into cache are only 19% faster. What helped: fewer registers (108 to 60 VGPRs,
+15%), issuing all of a neuron's mask reads before using them (+8%), the random words
made in parallel before the fire loop (+7%). What did not: wave32 against wave64, smaller
dispatch groups, 512 threads a life (+0 to 5%, within noise), and a column copy of the
masks read one word per spike for sparse source words (slower, 5.2 to 6.0M, and twice the
memory; the attempt is kept as out/gpu/columns-try/columns.diff in the worktree only).

## Memory per life

329 KB on the device for a 4,096-neuron life: masks 288 KB, rings 36 KB, thresholds 4 KB,
the rest under 1 KB. The CPU engine allocates the same (82 bytes a neuron). 16,384 lives
take 5.4 GB; with twins 10.8 GB.

## How it joins the search

A batch is a population. `scripts/gpu.sh settings in.csv out.csv [flags]`:

- **in.csv**: a header, then one row per life. Columns: `seed`, and any minimal-genome
  number by its name (`noise_shift`, `budget`, `m.noise_shift` also works) or a sheet number
  as `g.name` (`g.inhibitory_percent`). Columns not given take the command line's values.
  Eight seeds of one design are eight rows.
- **out.csv**: the same columns, then `late,early` (and `twin_late,twin_early` with
  `--twin`), late and early as blobrun reports them (mean of the last and first tenth of
  the windows, `--edge N` to set).
- The task and plug come from the command line (`--task --k --steps --window ...`), so a
  design scored on several worlds is one call per world with the same in.csv.
- A row the shader cannot run as written (far slots, delays, evidence counters,
  drop_mode 1, noise_pair, another sheet layout than the first row's) stops the run with
  the row and the reason.

## Worlds that stay on the CPU

Only the Echo family is in the shader. For the others the sheet would still step on the GPU
and the world on the CPU, exchanging once per task step: before a step the host writes each
life's line word (one bit per sensor line, 4 bytes a life) and, after the last step, its
valence; the shader runs the step's ticks (plug_write and plug_count are already there) and
leaves each life's motor counts (16 ints); the CPU world turns counts into a move, a valence
and the next lines. 16,384 lives exchange about 1 MB per step. A step of 16,384 lives is
about 35 ms of GPU time, so a world under about 2 microseconds a step per life keeps up on
one core; to hide it fully, split the batch in two halves and let the CPU step one half's
worlds while the GPU ticks the other.

## Scripts

```
scripts/gpu.sh                    # small safe batch: 64 lives + twins, echo k=2, 2,000 steps
scripts/gpu.sh check              # nine cases, lavapipe then GPU, against the CPU engine
scripts/gpu.sh run --lives 4096 --task shuffled-echo --steps 40000 --twin
scripts/gpu.sh settings in.csv out.csv --task echo --k 4 --steps 40000 --window 1000 --twin
scripts/gpu.sh speed              # the speed table
scripts/gpu.sh big                # 16,384 lives + twins, echo k=2, 40,000 steps
```

## GPU safety, as run

Every new or changed shader ran first on lavapipe (`LP_NUM_THREADS=2`, validation on, zero
messages, the --check cases) before the GPU. Dispatches start at one tick and grow toward
50 ms (never more than 4,096 ticks); the longest dispatch seen was under 100 ms. Every fence
wait has a 10 s timeout; a lost device or a timeout stops the run. One GPU process at a
time; the kernel log was checked after the first run at each new scale: no amdgpu lines.
No window, no surface, `DISPLAY` unset for every run.

## Not ported yet

- Far slots, slot delays, evidence counters, drop_mode 1, noise_pair (the grown-back pieces
  and ablations). They stop the run with a message.
- Tasks other than the Echo family (see above for how they would join).
- The idle and retention protocols, aliveness and rhythm measures, the stats counters.

## What next

1. The host-world mode above, so every world on the task map runs with the sheet on the
   GPU (the world code stays the CPU's; only lines and counts cross).
2. Port the grown-back pieces the search will vary: evidence counters (bitsliced already,
   they move per graded neuron like the swaps), then far slots and delays (these break the
   tile-wide source word the fire loop relies on, so they cost more here than on the CPU).
3. When the learning restructure on the CPU side lands (readout rule, evidence switches),
   key its new draw sites the same way and port them; `--check` will say where they differ.
4. Different tasks per life in one batch (the task numbers move from the batch table into
   the per-life struct), so one batch can be a whole quick map.
