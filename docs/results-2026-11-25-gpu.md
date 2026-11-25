# The minimal engine on the GPU, many lives at once (2026-11-25)

`blobgpu` runs thousands of independent minimal-engine lives (different seeds, different
genome numbers) on the RX 9070 XT through Vulkan compute, headless, together with their Echo
task. Every life is the CPU engine bit for bit, given one switch on the CPU side
(`--m.rng_mode 1`, keyed random words). `scripts/gpu.sh` runs it.

Later the same day (sections from "Host worlds" on): every task in `src/world` runs with
the sheet on the GPU and the world's own code on the CPU, one exchange per step; the
evidence counters, far slots and delays are ported; the memory engine runs on the card at
12 to 16 million life-ticks a second, 30 to 58 times one core; and one call scores every
row of a settings file on a list of worlds. All of it bit-exact with the CPU engines.

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

## Host worlds: every task, with the sheet on the GPU (added later on 2026-11-25)

The Echo family in the shader is fast but it is four worlds. Host worlds run any task: the
sheet steps on the GPU, the world (the real task and plug code of `src/world`, unchanged)
on the CPU, and once per task step they exchange what each needs. No shader change per
task; one specialization constant (`HOST`) switches the shader.

- **One dispatch is one task step of every life**: the reward the host sends for the step
  just answered (applied first, where blobrun's loop has it: after the answer tick, before
  the next tick), the rest ticks owed, then the step's ticks with the sensor words the host
  wrote. Good and bad lines and rest ticks are counted on the GPU (the plug's timers).
- **What crosses, per life and step**: out, 16 motor counts (80 bytes, host-cached memory);
  in, the reward, the pressed button and the next step's sensor words (16 bytes plus
  tiles + 1 words, host-visible memory the shader reads once). The host turns counts into
  `plug_choice`, `task_step`, the window score, then `task_sense`, `plug_set_lines` and
  `plug_write` onto a scratch shell, whose sensor words go out.
- **The queue stays full.** After a first step waited for in parts that grow from 64 lives
  (which measures a life's step), the lives are cut into parts of about `--dispatch-ms`
  (two at least from 512 lives), each on its own command buffer and fence; while the card
  runs the parts queued behind it, the host answers the part that finished and queues its
  next step. Host threads (at most 4) only when one would take over 4 ms: on this loaded
  machine a new nice 19 thread can wait milliseconds to start, which made 4 threads slower
  than one at 1,024 lives (2.4M against 5.9M life-ticks/s before the queue).
- **Several worlds in one batch** (`--tasks echo:k=8,order,gap,catch:catch-size=4/4`): every
  row of a settings file runs on every world of the list; each world has its own table
  block (good, bad and attached cells, buttons) on the GPU and its own plug on the host.
  `key=value` is any blobrun flag for that world alone (`/` separates its values); ticks,
  rest and reward ticks are the batch's. Echo-family tasks run as host worlds with `--host`.

Files: `src/gpu/gpu_host.cpp` (worlds, the host's half of an exchange),
`src/gpu/gpu_host_run.cpp` (the loop, the queue, the check), the `HOST` paths of
`shaders/gpu/minimal_life.comp`.

### What is bit-exact, and how it was checked

`--check` with host worlds runs the CPU engine beside every life, stepped by the real ticker
(`cpu_life_step`, blobrun's loop with each step's reward held to the start of the next call,
where the GPU has it), and after every step compares the whole sheet, the ring of the
step's ticks, the host's task against the CPU ticker's task, and the reward and move the
host answered; at the end the window scores. `scripts/gpu.sh check host` runs, on lavapipe
with validation and then on the GPU with validation:

| case | what it covers |
| --- | --- |
| host-echo | echo k=2 as a host world |
| host-order, -gap, -both, -catch, -cartpole, -sequence, -place | each world with its twin (catch 4x4; cartpole lean reward, place code, 2x2 lines) |
| host-worlds | two settings rows (grade_mode, budget, noise, learn_shift, sign, cowire, credit, avg_shift, up_shift, inhibitory share, budget size) on five worlds at once, with twins, 3 rest ticks and 4-tick reward lines |
| evidence | six evidence rows and a plain one on two worlds, with twins |
| far-delays | six far-slot and delay rows (with and without evidence) and a plain one on three worlds |
| memory, memory-worlds | the memory engine: four store shapes on four worlds; eight worlds with twins |

All identical, every step, on lavapipe and on the GPU, zero validation messages. The nine
in-shader cases still pass unchanged.

After main's new worlds landed (the relation, grammar, memory, number, function and acting
batteries), every one of the 44 task kinds except MNIST ran the same check, one life and
its twin, 150 steps, one rest tick, at the largest group side that fits of 3, 2, 1
(`scripts/gpu.sh sweep`, `sweep memory`): all 44 identical with the minimal engine on lavapipe
and on the GPU; 43 identical with the memory engine on both (fill-in refused, see below).
The host worlds copy the ticker's step answer, so a change to `src/run/task_ticker.cpp`
needs the same change in `host_exchange` (main's fill-in counts did); babbling is refused
until then.
At the merge with the five wound-up branches (2026-11-25) the rest of what the ticker and
the life loop gained is refused the same way rather than copied (`host_world_supported` in
`gpu_host.cpp`, also for the Echo family in the shader): the life protocols (`--shown` and
`--damage` withhold the reward; `--long` and the older ones have phases), the critic
(`--g.critic_on`, which rewards every step), and the invariance battery's tunes (inv-shift,
inv-tempo, inv-pitch, which the ticker senses on every tick of a step). The minimal core's
readout (`--m.readout`, `--m.inner_reward 0`) is not ported and is refused too. The battery's
picture kinds and `--bitmap-map logpolar` go through the plug code as they are. With these
in, `scripts/gpu.sh check` (9 in-shader and 13 host-world cases, lavapipe then the card)
was identical every tick, zero validation messages, no amdgpu kernel lines.

`scripts/gpu.sh against` runs four seeds (5-8) and their twins for 2,000 steps on the GPU
and in blobrun and compares the window csvs as text (16 cases; B and the catch case below
were run by hand the same way). Identical as text to
`blobrun --engine minimal --m.rng_mode 1`: echo, order, gap, both-not-either, catch 4x4,
cartpole (lean, place code); evidence settings A (echo k=4) and C (order) and B
(both-not-either), C with read-off 1 and ev_quiet 2 on catch; far slots (echo k=4); delays
8 (gap); far slots, delays and evidence together (order). Identical as text to
`blobrun --engine memory`: echo k=4 (lavapipe and GPU), catch, cartpole, sequence, delayed
echo.

## The grown-back pieces (host worlds)

Evidence counters (`ev_on` and every `ev_` number: counters on the motor cells or on every
neuron, the three read-off rules, quiet and co-activity variants), far slots (`far_slots`,
`far_percent`, `far_reach`, `far_delay_per_tile`) and delays (`delay_max`,
`delay_percent`) are each behind their own genome numbers, so a settings row turns them on.
A batch with any of them runs as host worlds; specialization constants (`SLOTS`, `DELAY`,
`EV`, `EV_BITS`, `EV_ROWS`) size the layout for the most any row asks for, and each life
uses what its own genome says inside them (a life with fewer far slots has empty ones,
which pick exactly as the CPU does over its own). A plain batch compiles to the same
shader as before.

- **Keyed draws.** The CPU's evidence reward drew its per-tile words from whatever stream
  the last draw site left, which depends on the order tiles are visited. Two new sites
  (`src/minimal/minimal_keyed.hpp`): per tile, which counter neurons move and which quiet
  ones move the other way; per neuron, the read-off's tie picks. `rng_mode 0` is unchanged:
  six 4,000-step lives (evidence A, B, C settings, ev_on 2 with far slots and delays, far
  slots with drop_mode 1, noise_pair with ev_on) give byte-identical window csvs before and
  after. Far slots and delays needed no new site.
- **Births on the GPU** draw far tiles and delays in the CPU's order (`set_far_and_delays`,
  then the wiring over the life's own slots); evidence counters start from the wiring.
- **Registers.** The read-off first kept arrays of slot words: 256 VGPRs with spills, the
  whole shader 2.5 times slower even for plain lives. Made again from the counters each
  time, it is 84 VGPRs. One array is left (the pool the tie picks draw from): without it
  lavapipe crashed in its JIT code, at an access whose descriptor it chose per lane; with
  it both agree with the CPU.
- **Where it costs.** The fire loop with delays reads each slot's word from the ring at its
  own delay instead of the shared map; far slots read one more word per neuron. Evidence
  on every neuron (ev_on 2) is dominated by the read-off after each co-activity bump and
  each graded neuron (a binary search over the counters, 6 x 9 slots x 6 planes).

## The memory engine on the GPU (idea 151)

`--engine memory` runs `src/memory/` on the card through the same host worlds. One
workgroup per life: a frame's tables (the 10-bit gather per table, the counter sums) and a
reward's credit (one thread per table, each (table, slot, button) once per reward) are
spread over the threads; events, the code of now, the choice, the next-input store and
every random draw run on thread 0 in the CPU engine's order, so no draw needed keying.
Births stay on the CPU (`memory_birth`, at most 4 threads) and are copied up. The store's
shape can differ per settings row (`y.name` columns: tables, table_bits, depth, past_bits,
now_only, code, credit, exploring, predictor). Not on the GPU: `y.sheet` (a sheet in front;
it made the memory worse everywhere) and `y.fade_shift` (it draws in table order).
4.9 MB a life on the device (4 MB of it the counters), so 2,048 lives take 10 GB.

**The card helps, by a lot.** The memory engine is mostly table lookups and small adds, and
most ticks do no store work at all; one workgroup spends most of a tick waiting on a few
dependent reads, so many lives at once fill the card. Echo k=2, 500-step lives: 61,000
life-ticks/s for one life, 4.7M at 64, 13.2M at 256, 12.1M at 1,024 and 12.2M at 2,048,
against 394,000 on one core in the same session (load not recorded; the memory results doc
had 187,000 at load 38); catch 4x4 15.9M (core 299,000),
cartpole 11.5M (312,000), sequence 37M (727,000) at 1,024 lives: 30 to 50 times one core.
The host worlds are not the limit: 0.04 s of thread time against 0.85 s at 1,024 lives.
Files: `shaders/gpu/memory_life.comp`, `memory_store.glsl`, `memory_layout.glsl`,
`src/gpu/gpu_memory.cpp`.

## How it joins the search

A batch is a population, and with host worlds it is a population on a whole list of worlds
at once. `scripts/gpu.sh settings in.csv out.csv [flags]`:

- **in.csv**: a header, then one row per life. Columns: `seed`, and any minimal-genome
  number by its name (`noise_shift`, `budget`, `ev_on`, `far_slots`, `delay_max`;
  `m.noise_shift` also works), a sheet number as `g.name` (`g.inhibitory_percent`), or with
  `--engine memory` a memory number as `y.name` (`y.tables`, `y.depth`). Columns not given
  take the command line's values. Eight seeds of one design are eight rows.
- **One world** (`--task --k ...`): out.csv has the same columns, then `late,early` (and
  `twin_late,twin_early` with `--twin`), late and early as blobrun reports them (mean of the
  last and first tenth of the windows, `--edge N` to set).
- **A list of worlds** (`--tasks echo:k=8,order,gap,catch:catch-size=4/4,...`): every row runs
  on every world in one batch, and out.csv has the settings columns, then `late_<world>` and,
  with `--twin`, `twin_late_<world>` for each world (`echo:k=8` is the column `echo_k8`).
  `key=value` is any blobrun flag for that world alone; ticks, rest and reward ticks are the
  batch's. `--windows PATH` writes every window with a world column.
- `--engine memory` scores memory designs the same way.
- A row the GPU cannot run as written (drop_mode 1, noise_pair, y.sheet, y.fade_shift, another
  sheet layout than the first row's) stops the run with the row and the reason. A batch is as
  slow as its slowest row (every life steps in lockstep), so rows with ev_on 2 are best
  batched apart.

As run: 32 minimal rows (plain, evidence, far slots, delays mixed) on six worlds with
twins, 3,000-step lives (384 lives): 24 s. 64 memory rows on eight worlds with twins
(1,024 lives), 3,000 steps: 3.6 s.

## Speed, every path

`scripts/gpu.sh speed-host`: 500-step lives, 20 ticks a step, 3x3 groups, seeds 1..N, RX
9070 XT; life-ticks per second summed over all lives, timed over the stepping. The CPU
column is the same flags on one core of this PC (`blobgpu --device cpu`, the real life
loop, keyed words, birth not counted). Taken late on 2026-11-25 with the machine quieter
than earlier (load 6 to 8), so these CPU figures are about twice the loaded ones above.

| path | 1 life | 64 | 1,024 | most | (lives) | one CPU core | most / core |
| --- | --- | --- | --- | --- | --- | --- | --- |
| minimal, echo k=2 in the shader | 41,400 | 2.78M | 9.05M | 9.71M | 16,384 | 172,000 | 56 |
| minimal, echo k=2 as a host world | 20,400 | 1.82M | 6.88M | 7.44M | 16,384 | 169,000 | 44 |
| minimal, catch 4x4 (host) | 24,400 | 1.90M | 7.03M | 7.21M | 16,384 | 173,000 | 42 |
| evidence A, echo k=4 (motor cells) | 12,300 | 855,000 | 3.78M | 5.19M | 16,384 | 118,000 | 44 |
| evidence C, echo k=4 (every neuron) | 6,000 | 267,000 | 542,000 | 596,000 | 4,096 | 24,000 | 25 |
| far slots 2, echo k=4 | 12,700 | 930,000 | 2.93M | 3.09M | 16,384 | 125,000 | 25 |
| delays up to 8, echo k=4 | 9,000 | 821,000 | 2.24M | 2.28M | 16,384 | 82,000 | 28 |
| memory engine, echo k=2 | 62,100 | 4.51M | 12.3M | 12.3M | 2,048 | 383,000 | 32 |
| memory engine, catch 4x4 | 95,400 | 4.67M | 16.0M | 16.2M | 2,048 | 278,000 | 58 |

- **A host world costs about a fifth against the shader's own Echo** (7.4M against 9.7M at
  16,384): dispatches are one step (20 ticks) long instead of up to 4,096 ticks, so each
  carries its start (every life's small state) and its drain. One life alone is worse (a
  dispatch and a fence per step: 20,000 against 41,000), and at 64 lives the card is mostly
  idle either way.
- **Where the time goes in host-world mode** (echo k=2, 16,384 lives, 500 steps, CPU load
  33): 163.8M life-ticks in 21.95 s wall; the host waited on the card 20.4 s; the host
  worlds took 1.12 s of thread time (0.13 microseconds a life and step: plug_choice,
  task_step, task_sense, plug lines onto a scratch shell) and the exchange 0.21 s (copying
  counts out of and words into mapped memory), both while the card ran the parts queued
  behind. So the card is the limit; the world and the exchange are 6% of the wall clock
  and hidden behind it. Before the queue (each part waited for) the same batch ran at 6.1M,
  and with 4 host threads at 1,024 lives at 2.4M (thread starts under nice 19 on a loaded
  machine).
- **The grown-back pieces cost about what they cost on the CPU.** Evidence on the motor
  cells keeps pace with the plain sheet's share (5.2M against 7.4M at 16,384, as the CPU's
  118,000 against 169,000); evidence on every neuron is 8% of plain here and 14% on the CPU,
  the one piece where the card is relatively worse (its read-off is serial work per
  neuron). Far slots and delays break the tile-wide source word the fire loop relies on,
  and cost 2.4 and 3.3 times.

## Memory per life, the grown-back pieces and the memory engine

Plain minimal engine 329 KB. Evidence A 437 KB, evidence C (6-bit counters on every
neuron) 1.95 MB; a far slot adds 48 KB (its mask words and source tiles), delays 36 KB (a
byte per neuron and slot). Memory engine 4.9 MB (4 MB
counters, 0.5 MB next-input store). Groups keep every buffer under 1 GB.

## Scripts

```
scripts/gpu.sh                    # small safe batch: 64 lives + twins, echo k=2, 2,000 steps
scripts/gpu.sh check              # 9 in-shader and 13 host-world cases, lavapipe then GPU
scripts/gpu.sh check host         # only the host-world cases (evidence, far, delays, memory)
scripts/gpu.sh sweep              # every task kind as a host world, checked (sweep memory too)
scripts/gpu.sh against            # 16 cases against blobrun as text (needs blobrun to build)
scripts/gpu.sh run --lives 4096 --task shuffled-echo --steps 40000 --twin
scripts/gpu.sh run --lives 1024 --task cartpole --sensor-side 2 --cart-reward lean --twin
scripts/gpu.sh run --lives 1024 --engine memory --task catch --catch-size 4 4 --twin
scripts/gpu.sh settings in.csv out.csv --task echo --k 4 --steps 40000 --window 1000 --twin
scripts/gpu.sh settings in.csv out.csv --tasks echo:k=8,order,gap,catch:catch-size=4/4 --twin
scripts/gpu.sh settings in.csv out.csv --engine memory --tasks echo:k=8,order,gap --twin
scripts/gpu.sh speed              # the in-shader speed table
scripts/gpu.sh speed-host         # every path at 1, 64, 1,024 lives and the most that fit
scripts/gpu.sh big                # 16,384 lives + twins, echo k=2, 40,000 steps
```

## GPU safety, as run

Every new or changed shader ran first on lavapipe (`LP_NUM_THREADS=2`, validation on, zero
messages, the --check cases) before the GPU, then on the GPU at a few lives with
validation and a 300 s timeout, then at scale. In-shader dispatches start at one tick and
grow toward 50 ms (never more than 4,096 ticks); host-world dispatches are one step, the
first ones in parts of 64 lives growing toward 50 ms; the longest dispatch printed in any run
of this work was 47 ms. Every fence wait has a 10 s timeout; a lost device or a timeout stops
the run. One GPU process at a time; the kernel log was checked after the first run at each
new scale: no amdgpu lines. No window, no surface, `DISPLAY` unset for every run.

Two things looked wrong on the way, neither on the card: lavapipe crashed (a segfault in its
JIT code, at an access whose buffer descriptor it chose per lane) on one form of the
evidence read-off, so the read-off keeps one small array; and the first form of the
read-off took the card's shader to 256 VGPRs with spills. On the card itself nothing
misbehaved.

## Not ported yet

- drop_mode 1 and noise_pair (the minimal engine's ablations), y.sheet and y.fade_shift (the
  memory engine's). They stop the run with a message.
- Babbling and balance (ideas 60, 61), the shown, damage and withheld phases, and the
  retention and idle protocols: the host worlds run one plain life per world.
- Fill-in worlds (hidden lines) with the memory engine: its predictions stay on the GPU. The
  minimal engine never predicts, so its fill-in counts are made on the host and match.
- MNIST was not run (its data is not in this worktree).
- The aliveness and rhythm measures, the stats counters.

## What next

1. Run the search's quick map through `--tasks` (one call per batch of candidates, both
   engines), with the held-out worlds as a second call.
2. Fill-in for the memory engine: the hidden lines' cells as two more mask words per step,
   counted on the GPU after each tick.
3. Longer dispatches for host worlds that need no answer every step (a world with no
   reward and no decisions could run several steps per dispatch), and two groups per
   dispatch to cut the start and drain cost.
4. Evidence on every neuron: a cheaper read-off (the line read-off, ev_read 1, needs no
   binary search) if the search finds that form worth the cost.
