# Putting the winners together: the core preset (idea 65, 2026-11-25)

Idea 65 in `docs/ideas.md`, predictions 26, 27 and 28. The minimal engine is the base; the
pieces that earned a place elsewhere go in, and each is kept only if it earns its place in
the combination. After the coordinator's note of the same day (the owner: "online learning
for any task"; `docs/bench.md`, "One setting for everything" and "Nothing frozen"), `core` is
one fixed setting with the whole sheet learning all the time. Runs with the inside of the
sheet held fixed are reported beside it as a diagnostic only.

**Status: stopped part way (agents cut to four, 2026-11-25).** Measured: the readout port,
the choice of inner learning and of sheet properties (seeds 1-4), and `core` against its
frozen-inside twin and the plain minimal engine on 13 rows (seeds 5-12). Not done: the
piece-by-piece table on seeds 5-12, cartpole, shuffled straight/crossed, retention, idle,
the readability probe beyond one run, the cost table, the suite. How to finish each is at
the end.

## Short version

- **`core` = the minimal engine + the readout's error rule on 128 cells per button +
  co-activity wiring everywhere (the buttons' cells too) + two far slots per neuron + slot
  delays 1..8.** One setting, every row, the whole sheet learning all the time
  (`scripts/minimal.sh flags`). Left out: the minimal engine's own noise-graded reward rule,
  inner evidence counters, long-only delays (4..8).
- **The pieces combine.** Seeds 5-12: echo at 1/2/3/5 tiles 0.994 / 0.922 / 0.859 / 0.764,
  echo k=4/8/16 0.975 / 0.872 / 0.529, exclusive-or 0.720, delayed echo with the ask right
  after the cue 0.994, go-no-go 0.989. Against the full engine's frozen-sheet readout: ties
  at 1-3 tiles and k=4, better at 5 tiles (0.76 against 0.59), k=8 (0.87 against 0.84) and
  k=16 (0.53 against 0.31); worse on exclusive-or (0.72 against 0.88) and order (0.49
  against 0.70).
- **Frozen against learning, row by row: here inner learning (co-activity wiring) never made
  a row worse.** Holding the inside fixed costs 0.05 at 1 tile, 0.17-0.25 at 2-5 tiles, 0.16
  on exclusive-or, 0.18-0.25 on k=4/8/16, 0.13 on delayed0, 0.08 on go-no-go, and nothing
  measurable on order, gap and delayed echo with a gap (at chance either way). The full
  engine's fight (inner learning helped reach and hurt exclusive-or) does not appear with
  co-activity wiring as the inner rule. It does with the other two inner rules (seeds 1-4):
  the noise-graded reward rule costs reach 0.11 at 3 tiles and gains nothing; inner
  evidence counters cost 0.07-0.23 on reach, exclusive-or, k=4 and delayed0.
- **Time is still the wall.** Order, gap and delayed echo with a gap stay at chance with
  every combination tried, long delays 4..8 included. Delays do buy delayed echo with the
  ask right after the cue (0.61 to 0.99) and, with the readout, no longer cost the reflex.
- **The readout uses far slots where the old rule could not**: echo at 3 tiles 0.74 to 0.96
  (seeds 1-4); the plain engine with the same far slots stayed at 0.37.
- The minimal sheet as born is poorer to read than the full one: one linear-reader run
  (echo, 1 tile, seed 5): whole sheet 0.94, motors' view 0.91, against 1.00 and 1.00 for the
  full engine. The rest of that probe was not run.
- Speed: not measured cleanly. One smoke life of `core` under load ran at 31,000 ticks/s;
  the plain engine runs 200,000-230,000 in other sessions. Delays (2.2x) and far slots
  (1.3x) alone predict about 3x; prediction 28 is probably wrong.

## What was built

All in `src/minimal/` unless said; everything is behind `--m.` numbers that are off by
default, and with them off every csv is byte-identical to c0f8277 (10 lives compared with
`cmp`: echo k=2 and k=4, shuffled, far3 with far slots, delayed0 with delays, counters on
motor neurons and on every neuron, noise_pair, motor spread 128, co-activity off; checked
again after merging main).

- **The readout** (`minimal_readout.cpp`, `--m.readout 1`): learn mode 5's error rule from
  the full engine. The world marks which motor cells belong to each button and which belong
  to the button just pressed (the plug's existing `plug_mark_motors`/`plug_mark_acted`
  write into the shell's `motor_attached`/`motor_acted`, which the minimal engine now
  allocates when the readout is on; no world code changed). On bad the pressed button's
  cells lose evidence for every source they heard over the last `readout_ticks` ticks and
  the other buttons' cells gain it, one step per heard tick at chance 2^-`readout_shift`
  (4); sign-aware (gaining lowers an inhibitory source's counter); balanced (the other
  buttons move at a chance lowered by their number); nothing moves on good. "Heard" is what
  the cell's slot read on that tick, so it respects slot delays and far slots.
- **One counter store.** The readout moves `minimal_evidence.cpp`'s bitsliced counters
  (6 bits, wired sources born 4 above zero), and the cell's mask is read off them (the top
  48). Rows are made for every motor neuron when the readout is on; with `--m.ev_on 2` every
  neuron has a row and the inner evidence rule grades all but the buttons' cells.
- **Inner rules, each switchable, each leaving the buttons' cells to the readout:** the
  noise-graded reward rule (`--m.inner_reward`, on by default as before), co-activity wiring
  (`--m.cowire_on`; `--m.readout_cowire 1` lets the buttons' cells co-wire too, through their
  counters), inner evidence counters (`--m.ev_on 2`).
- `--m.delay_min`: local slot delays drawn in min..max (the full engine's "delays 4 to 8").
  `--m.ev_read 3`: the top budget of counters above zero (never triggered in practice, kept
  as a switch). `--m.readout_inh 0`: inhibitory sources' counters do not move (worse).
- `blobrun --frozen-readout --engine minimal` was refused; it now means the readout with
  nothing inside learning (`minimal_readout_only`, a 4-line change in `src/run/main.cpp`).
- `minimalprobe linear` (`src/minimal/probe/probe_linear.cpp`): blobprobe linear's ridge
  reader for either engine, on a real task, with `--teach-steps N` to learn before reading.
  Needs `src/probe/linear_fit.cpp` linked into minimalprobe (one line in `CMakeLists.txt`).
  The minimal probes now mark the pressed button before a reward, as blobrun does, and
  `probe life` prints what the buttons' cells wire.
- `scripts/minimal.sh`: `core` before any command runs it with core's flags; `flags`
  prints them; `core suite` runs the suite; presets dist1-5, go, retention, idle.

## The readout on a frozen minimal sheet first (seeds 1-2, then 1-4)

`out/core/sw.sh BIN out/core/v/r1..r3.txt 1-2 ...` (variant files in `out/core/v/`; r1.txt
was later overwritten with the report variants). Echo k=2 at one tile, readout alone, inside
frozen: 0.73 with 128 cells per button, 0.79 with 32, 0.71 packed; the full engine's readout
gets 1.00 there. Not fixed by 8-bit counters, shift 6, a moving line or above-zero read-off,
a cap at zero, or excitatory-only moves (worse, 0.69). A 20-tick window helps (0.82). What
fixes it is letting the sheet learn: co-activity wiring inside gives 0.99. `minimalprobe
life` on a frozen life: the buttons' cells end with 18 of 48 connections inhibitory (25% of
senders are) and presses biased 0.65/0.31. A guess, not tested: the bursty minimal sheet
(Fano 175 against the full engine's 25) puts swells into both buttons' counts, and errors
made in swells push the counters around.

## Which inner learning (seeds 1-4)

Readout on 128 cells per button, 20-tick window; `out/core/t4.md` (with two far slots and
delays 1..8, "B") and `out/core/t1.md` (without). Late score, mean of 4 seeds.

| with far slots + delays 1..8 | 1 tile | 3 tiles | xor | order | gap | delayed0 | delayed 2 | k=4 |
|---|---|---|---|---|---|---|---|---|
| inside frozen | 0.950 | 0.638 | 0.603 | 0.491 | 0.509 | 0.876 | 0.491 | 0.799 |
| + co-activity wiring (B) | 0.993 | 0.914 | 0.728 | 0.500 | 0.522 | 0.997 | 0.497 | 0.971 |
| B, the buttons' cells co-wire too (core) | 0.993 | 0.887 | 0.729 | 0.496 | 0.538 | 0.993 | 0.502 | 0.980 |
| B + noise-graded reward rule | 0.994 | 0.808 | 0.729 | 0.500 | 0.524 | 0.996 | 0.481 | 0.974 |
| B + inner evidence counters (ev_on 2) | 0.928 | 0.685 | 0.600 | 0.474 | 0.504 | 0.932 | 0.478 | 0.899 |

Paired against B: the reward rule -0.106 (se 0.045) at 3 tiles, nothing elsewhere; inner
evidence -0.065 (0.021), -0.230 (0.032), -0.128 (0.023), -0.065 (0.007), -0.073 (0.008) on
1 tile, 3 tiles, xor, delayed0, k=4; the buttons' cells co-wiring is within noise on every
row (-0.027 at 3 tiles, se 0.025) and was taken so that nothing is switched off. Without far
slots and delays the order is the same (`t1.md`, 1 tile / 3 tiles / xor / k=4: frozen
0.82 / 0.53 / 0.71 / 0.91; co-activity 0.99 / 0.74 / 0.73 / 0.95; reward rule + co-activity
0.99 / 0.50 / 0.75 / 0.94; inner evidence 0.93 / 0.65 / 0.55 / 0.92).

## Which sheet properties (seeds 1-4, readout + co-activity wiring)

`out/core/t2.md`, `out/core/t3.md`.

| | 1 tile | 3 tiles | xor | order | gap | delayed0 | delayed 2 | k=4 |
|---|---|---|---|---|---|---|---|---|
| neither | 0.990 | 0.744 | 0.731 | 0.489 | 0.534 | 0.612 | 0.495 | 0.952 |
| delays 1..8 | 0.985 | 0.566 | 0.736 | 0.504 | 0.537 | 0.992 | 0.489 | 0.933 |
| delays 1..8, half the slots | 0.992 | 0.588 | 0.747 | 0.497 | 0.510 | 0.985 | 0.480 | 0.956 |
| delays 1..4 | 0.964 | 0.593 | 0.728 | 0.481 | 0.502 | 0.947 | 0.505 | 0.935 |
| delays 4..8 | 0.983 | 0.523 | 0.675 | 0.498 | 0.538 | 0.992 | 0.479 | 0.912 |
| 1 far slot, 50% | 0.997 | 0.964 | 0.729 | 0.489 | 0.533 | 0.623 | 0.478 | 0.962 |
| 2 far slots, 100% | 0.997 | 0.955 | 0.722 | 0.484 | 0.549 | 0.612 | 0.503 | 0.986 |
| **2 far slots + delays 1..8** | 0.993 | 0.914 | 0.728 | 0.500 | 0.522 | 0.997 | 0.497 | 0.971 |
| 2 far slots + delays 1..8, half | 0.995 | 0.941 | 0.733 | 0.489 | 0.524 | 0.987 | 0.491 | 0.977 |
| 1 far slot + delays 1..8 | 0.983 | 0.832 | 0.730 | 0.491 | 0.530 | 0.977 | 0.479 | 0.954 |
| 2 far slots + delays 4..8 | 0.991 | 0.926 | 0.688 | 0.497 | 0.550 | 0.989 | 0.484 | 0.927 |

- Far slots: reach (3 tiles 0.74 to 0.96) and k=4 (+0.03); nothing on time. In idea 55 the
  same far slots bought nothing: the readout can use what they carry.
- Delays: delayed0 0.61 to 0.99, and they no longer cost the reflex (0.99; in idea 55 they
  cost 0.20). Alone they cost reach (-0.18); with far slots much less (-0.05). Long-only
  delays (4..8) cost exclusive-or and k=4 and help nothing. Order, gap, delayed echo with a
  gap: no setting moves them.
- Kept: two far slots on every neuron and delays 1..8 on every slot (the half-slot version
  is within noise of it; the plainer one was taken).

## core against its parents, seeds 5-12

`out/core/report.sh out/core/blobrun-v6 out/core/v/r1.txt 5-12 out/core/rep1.md` (table in
`out/core/rep1.md`, runs in `out/core/sweep/`). Late score (mean of the last 4 of 40 windows
of 1,000 steps), mean over 8 lives; core's learning-off twin in brackets (the same born
sheet serves both core rows). Full engine: frozen full sheet, learn mode 5 with evidence,
balance, 128 cells (`docs/results-2026-11-24-readout.md`; `out/scorecards/blob-readout.csv`
on main). Coin flips not rerun: 0.47-0.50 for two buttons, 0.23 / 0.11 / 0.05 for 4 / 8 / 16
(`out/scorecards/minimal.csv`).

| task | core (twin) | core, inside frozen | frozen - core (se) | plain minimal | full engine, frozen + readout |
|---|---|---|---|---|---|
| echo k=2, 1 tile | **0.994** (0.476) | 0.945 | -0.049 (0.007) | 0.942 | 1.000 |
| echo k=2, 2 tiles | 0.922 (0.472) | 0.753 | -0.168 (0.019) | 0.398 | 0.905 |
| echo k=2, 3 tiles | 0.859 (0.467) | 0.647 | -0.212 (0.024) | 0.360 | 0.853 |
| echo k=2, 5 tiles | **0.764** (0.461) | 0.515 | -0.249 (0.011) | 0.354 | 0.587 |
| echo k=4 | 0.975 (0.220) | 0.798 | -0.177 (0.005) | 0.408 | 0.997 |
| echo k=8 | **0.872** (0.094) | 0.625 | -0.248 (0.008) | 0.062 | 0.842 |
| echo k=16 | **0.529** (0.046) | 0.328 | -0.201 (0.011) | 0.031 | 0.305 |
| both-not-either | 0.720 (0.480) | 0.559 | -0.161 (0.003) | 0.259 | **0.883** |
| order | 0.491 (0.475) | 0.483 | -0.008 (0.002) | 0.368 | **0.695** |
| gap | 0.545 (0.489) | 0.518 | -0.027 (0.008) | 0.427 | 0.519 |
| delayed echo, ask right after | **0.994** (0.468) | 0.868 | -0.126 (0.014) | 0.410 | not run |
| delayed echo, 2 blank steps | 0.490 (0.462) | 0.481 | -0.009 (0.009) | 0.239 | 0.547 |
| go-no-go | 0.989 (0.499) | 0.907 | -0.082 (0.025) | 0.976 | not run |
| cartpole, lean, place | unfinished | unfinished | | 17.2 | 17.6 |

- Every echo row beats the plain minimal engine (by 0.05 to 0.81) and the full engine's
  readout at 5 tiles and 8 and 16 cues; ties it at 1 to 3 tiles and k=4.
- Inner learning (co-activity wiring) is worth something on every row the sheet can do at
  all, most on reach and many cues (0.17-0.25). Where frozen is at chance (order, gap,
  delayed 2) learning does not change it.
- Exclusive-or is the one row where the minimal sheet falls well short of the full one
  (0.72 against 0.88). Three of its four cases are a plain sum of the two lines, so 0.75 is
  what a reader without a "both on" feature gets; the minimal sheet near the input may have
  little of that feature to read. Not checked with the probe.

## How readable the minimal sheet is

Only one run was done before the stop: `minimalprobe linear` (4,000 trials, ridge reader),
echo k=2 at one tile, seed 5, as born: minimal whole sheet 0.941, motors' view 0.907, best
3x3 block 0.967, best tile 0.840; full engine 1.000 on all four (`out/core/lin/`). Even the
plain cue is not cleanly in the minimal sheet as born, which fits the frozen readout's
0.73-0.95. The full probe (5 tasks x 7 sheets: minimal as born, delays 1..8, delays 4..8,
two far slots, after a co-activity-only life, full, full with delays 4..8) is
`out/core/lin.sh out/core/minimalprobe-v6 5 6`, not run.

## Cost

- Bytes per neuron, by the engine's own count (`minimal_bytes`) for the 4,096-neuron echo
  sheet, not printed by a run: about 155 against 82 for the plain engine (masks 88 for 11
  slots, far tiles 8, delays 11, counters on the 256 motor neurons 33, row index 4, rings and
  thresholds about 10).
- Lines (engine only, no blank or comment-only lines): 1,220 in `src/minimal/` against 1,078
  at c0f8277.
- Ticks per second: not measured (load 33-37 on 32 threads all session). One 2,000-step
  smoke life of `core` ran at 31,057 ticks/s; the plain engine's suite life ran 227,054 in
  another session. With everything off the tick is the same code as before (the plain fire
  loop is untouched; co-activity gets one AND per tile) and runs byte-identical.

## Predictions

- **26. Echo at one tile above 0.9 and at three tiles above 0.8, in the minimal engine.**
  Right: 0.994 and 0.859 (seeds 5-12), whole sheet learning. Frozen inside it is wrong at
  three tiles (0.647).
- **27. Exclusive-or above 0.8 and order above 0.65 with the sheet frozen.** Wrong: frozen
  0.559 and 0.483; whole sheet learning 0.720 and 0.491. The prediction's caveat was right:
  the minimal sheet is the poorer sheet to read.
- **28. Within 2x of the plain minimal engine's ticks per second in an Echo life.** Not
  measured cleanly; one loaded smoke run says about 7x, and delays and far slots alone
  predict about 3x. Probably wrong.

## What should change elsewhere (for the coordinator; not edited here)

- `docs/findings.md`, principle 3 (co-activity wiring): a third result. Beside an
  error-rule readout it is the inner rule that pays: holding the inside fixed costs
  0.05-0.25 on every echo row and 0.16 on exclusive-or, and it cost no row. Wall "Reading a
  thin signal ... the old inner rule fights the new readout": in the minimal engine the
  fight comes from the noise-graded reward rule (-0.11 at 3 tiles) and inner evidence
  counters (-0.07 to -0.23), not from co-activity wiring.
- Principle 7, findings 9-10 (delays, far links): with the readout both earn places on tasks
  in the minimal engine: far slots 3 tiles 0.74 to 0.96; delays delayed0 0.61 to 0.99, and
  no longer cost the reflex.
- Finding 14's reflex cost of counters (0.94 to 0.73) is gone with the error rule at the
  buttons (0.994), which fits the guess that it came from grading the losing button's cells.
- Walls: "Reach": 5 tiles 0.76. "More than four things": 8 cues 0.87, 16 cues 0.53. "Time":
  unchanged, now also with long delays. New: exclusive-or is weaker on the minimal sheet
  (0.72) than on the full one (0.88).
- `docs/ideas.md`, idea 65: tried, partly finished; `core` as above.

## What I would try next

1. Finish the list below, the piece table first.
2. Time: the minimal neuron has no memory but its delays; order and gap need a state held
   for 20-60 ticks. The full engine's potential with a slow leak is the cheapest candidate,
   tried against order only.
3. Exclusive-or: run the readability probe; if the "both on" signal is missing, look at the
   threshold range.
4. Speed: profile `core` on a quiet machine; the readout's heard lookups (256 cells x 11
   slots x 20 ticks per bad) and the delay permute are the suspects.

## Unfinished, and the lines that finish it

From the worktree root. Binaries are copies in `out/core/`; every sweep reuses
`out/core/sweep/cache.jsonl`, so a rerun only does what is missing.

```
# the cartpole row of core / frozen / plain, seeds 5-12 (the other 13 rows are cached)
out/core/report.sh out/core/blobrun-v6 out/core/v/r1.txt 5-12 out/core/rep1.md
# piece by piece on seeds 5-12: +reward rule, +inner evidence, -far, -delays, buttons not co-wiring
out/core/report2.sh out/core/blobrun-v6 out/core/v/r2.txt 5-12 out/core/rep2.md --no-twin
# shuffled echo, 16 lives; split straight/crossed with crossed() in scripts/variants/lose_shift_split.py
python3 scripts/sweep.py --jobs 2 --blobrun out/core/blobrun-v6 --out-dir out/core/sweep --seeds 5-20 \
    --name shuffled --variants out/core/v/r1.txt -- --engine minimal --ticks 20 --sensor-side 3 \
    --motor-side 3 --steps 40000 --window 1000 --task shuffled-echo --k 2
# readability of each sheet
out/core/lin.sh out/core/minimalprobe-v6 5 6
# the suite once (retention, idle, capacity, cost rows); check no other suite runs first
scripts/minimal.sh core suite
# speed, quiet machine, same session
scripts/minimal.sh probe speed; scripts/minimal.sh core task echo --no-baselines
```

## Script lines

```
cmake -S . -B build && nice -n 19 cmake --build build -j4
scripts/minimal.sh flags                            core's flags
scripts/minimal.sh core seeds dist3 5 12            one row of the table above
scripts/minimal.sh core seeds both 5 12 --m.cowire_on 0     the frozen-inside diagnostic
scripts/minimal.sh core probe life --seed 5         a life, then what the buttons' cells wire
scripts/minimal.sh probe linear --task both-not-either --seed 5 [--engine full]
```

## Rough

- Settings chosen on seeds 1-4 (about 60 rows over 8 tasks); report rows are seeds 5-12.
  "Buttons' cells co-wire too" was chosen on a rule (nothing switched off), not a difference.
- `--frozen-readout --engine minimal` means the readout with nothing inside learning; the
  full engine's `--frozen-readout` means its old learn mode 1, so the suite's "frozen sheet,
  readout learns" rows mean different things for the two engines.
- The full-engine parent was not rerun; its delayed0 and go-no-go were never run.
- Coin flips were not rerun; the learning-off twin is the baseline beside every number.
- Speed and bytes are not measured by a run.
