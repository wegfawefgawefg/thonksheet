# The threshold cap lifted: decide the default (idea 180, 2026-11-26)

**No: `threshold_cap` should stay 64 for now, because at 1000 the walk holds every cell to
its share, which is what MNIST online and reach were running over (the output's motor cells
for MNIST online, competitive learning's winners for reach), no one change beside the lifted
cap gives both back (a target of 64 gives back MNIST online and costs 8 cues and
exclusive-or; keeping the motor cells at 64, or a cap of 128 for every cell, keeps MNIST
online and still costs reach with competitive learning 0.07 to 0.20), and the chain that
1000 passes is not credit over a step of time.**

Queue item 2m of `docs/plan.md`, following `docs/results-2026-11-26-rate-control.md`. Every
cell's threshold walks up on a spike and down slowly, capped at `threshold_cap` (64). The
rate-control job found that at 64 long lives run away and at 1000 they hold, that the
two-step chain passes with the cap lifted, and that MNIST online and reach with competitive
learning lose. This job asks why, and whether 1000 becomes the default. Nothing in
`src/blob/` was changed and the default was not touched; everything ran with flags.

## Predictions (written and committed before any scoring run of this job was read)

Seen before writing these: the three results docs named in the job, `regulate.cpp`, the chain
world's code (`src/world/task_bandit.cpp`), and the quick-map lives already in the caches (the
rate-control job's and the older ones): per seed, the chain, MNIST, far3 and far5 rows of
base, a2rucomp and best at both caps with their end-of-life health. Nothing of this job has
been run.

What the cached lives already say, which the guesses lean on:
- The chain under the base rule fires at 1.02 of the target at both caps, with 0.8 in 100
  thresholds at the cap at 64. So on that row it is not the rate; it is a few pinned cells.
- Under the base rule at cap 1000 the chain is 0.67 to 0.75 on every one of 8 seeds (learning
  off: 0.25). That is too even to be eight separate discoveries.
- The base rule has nothing that sends credit back a step. It learns only on a bad verdict,
  and the first press of the chain gets no verdict.

The guess about the chain: the chain here has two buttons, A then B. The output rule learns
B on the paid step (stage 1) from its own verdicts. The per-cell walk holds every motor cell
to the same share of spikes, so the button the rule drives on stage 1 is pushed down on
stage 0, and with two buttons the other one is A. With the cap at 64 the motor cells sit at
the cap and the walk cannot do this. So: a side effect of per-cell sharing on a two-button
world, not credit over a step of time.

1. The cap split by kind of cell (a probe that caps motor cells and the rest apart), base
   rule, seeds 5-12: lifting it on the motor cells alone passes the chain (0.60 or more):
   0.7. Lifting it on the hidden cells alone does not (0.40 or less): 0.7.
2. Motor cells' thresholds held as born (no walk on them) with the cap at 1000: the chain is
   0.40 or less: 0.65.
3. The chain with three buttons at cap 1000 (base rule): on stage 0 the presses that are not
   B split between A and the third button, A's share of those under 0.65: 0.6. (Real credit
   would take A's share toward 1.)
4. The chain with the stage line taken away (`--chain-cue 0`) at cap 1000, base: 0.60 or
   more: 0.35.
5. Seeds 1-4, cap 1000: base chain 0.65 to 0.78 again: 0.8; the one setting's mean 0.60 or
   more: 0.6.
6. The other rows only a critic passed or lifted (cartpole, noisy lines): the cap lifted
   passes neither on seeds 1-4, as on 5-12: 0.9.
7. Whether it is real credit over a step of time: no, 0.75.

MNIST online and reach:
8. At cap 1000 with the target raised to the rate the cap-64 life ran at, a2rucomp's MNIST
   online comes back to within 0.02 of 0.838: 0.35. The same for base (0.214) and best
   (0.714), all three: 0.25.
9. At cap 64 with the rate held another way (the sector bias given room,
   `--g.sector_bias_max 64`), a2rucomp's MNIST online stays within 0.02 of 0.838: 0.5. So
   my leaning is that the cap itself (cells that are let fire more than their share, and
   whose thresholds stand still) is worth more than the extra spikes.
10. Capping only the motor cells at 64 (the rest at 1000) gives a2rucomp's MNIST online
   back to within 0.02: 0.45.
11. Reach with competitive learning (a2rucomp far3, 0.925 at cap 64, 0.721 at 1000): some
   honest target of 64 or less at cap 1000 gets 0.85 or more: 0.5.

The map, the inside test, speed, the answer:
12. The quick map at cap 1000 plus what part 2 finds: rows passed at least 7 / 13 / 16 (base
   / a2rucomp / best) with none of exclusive-or, 16 cues, order and MNIST lower than cap 64
   by 0.03 or more: 0.3.
13. The inside test at cap 1000: competitive learning on the pair test 0.698 (0.726 at cap
   64, born 0.673); the one-hop rule on the pair test 0.688; born 0.668. (MNIST at cap 1000
   is already known from the compete2 and hop docs: 0.935 and 0.935 against born 0.922.)
14. Speed: `blobbench` with the cap at 1000 within 3 in 100 of the default: 0.9.
15. That I end up saying 1000 should be the default: 0.6.

## Short version

- **The chain's pass is a side effect, not credit over a step.** The second press (B, the
  paid one) is learned at either cap. At 64 the first press is stuck on B too: B's motor
  cells sit near the cap, out-vote A's on the unpaid stage, and a press there gets no
  verdict, so nothing corrects it. With the cap lifted the walk raises B's cells (mean
  threshold 42 to 102) until the two buttons are level on the unpaid stage, and the first
  press becomes a coin: A on 0.56 of those steps under the base rule. A coin first press
  with a learned second scores 0.72, over the row's line of 0.65. With three buttons the
  first press goes to A no more than to the third button (0.37 and 0.40; the critic, which
  does send credit back: 0.95 and 0.00). It is the motor cells' cap (lifted on them alone
  0.71, on the rest alone 0.33), not the rate (32 per 1,024 at both caps). The one setting
  has a small real lean to A on top (0.49 against 0.35), a quarter of the critic's. Seeds
  1-4 say the same; cartpole and noisy lines do not pass.
- **What the cap of 64 gives is room over a cell's share, and two kinds of cell use it.**
  MNIST online uses the output's motor cells: under the base rule they fire at 6.5 times
  the target while the hidden cells sit on it, and capping the motor cells alone gives the
  whole score back (0.219 of 0.214; cap 1000: 0.171). Reach with competitive learning uses
  the winners among the hidden cells as well: with the rate held at 37 per 1,024 and the cap
  still on it keeps 0.926, and with an honest 59 and the cap off it gets 0.771.
- **A higher honest target gives back part.** The target is two numbers (the sector's, and
  the walk's own up and down chances); moved together to 64 they give back MNIST online
  under a2rucomp (0.826 of 0.840) and more under the one setting (0.758 against 0.716), and
  reach under the base rule. They do not give back reach with competitive learning (0.771
  of 0.921) or the base rule's MNIST, the chain no longer passes (0.24), and on the map the
  base rule loses 8 cues 0.22 and exclusive-or 0.16. Not a default.
- **Between 64 and 1000:** a cap of 128, on the whole map (seeds 5-12), passes 7 / 13 / 15
  rows against cap 64's 6 / 13 / 15 and cap 1000's 7 / 12 / 16. It keeps MNIST online
  (+0.016 / +0.006 / +0.031 for base / a2rucomp / best), keeps echo and the bandit whole,
  and gains what 1000 gains under competitive learning (order +0.14 and passed, parity of 3
  +0.08, noisy lines +0.11). It passes the chain the same side-effect way under the base
  rule only. It costs reach with competitive learning 0.07 and 0.11 (far5 falls under its
  line) and held-out scale 0.22. It takes off about half of the runaway, not all: at 20,000
  digits the MNIST world runs 53 / 56 / 40 per 1,024 where cap 64 runs 93 / 73 / 44, and in
  long lives still 1.2 to 1.8 times the target.
- **The whole map, seeds 5-12, cap 64 to cap 1000:** rows passed 6 to 7 (base), 13 to 12
  (a2rucomp), 15 to 16 (best). Every row gained is the chain or, under a2rucomp, order.
  Lost or lower beyond 2 standard errors: echo of 2 and shuffled (1.000 to 0.95 to 0.98
  under base and a2rucomp), reach, the bandit, held-out scale, MNIST online.
- **The inside test, rate honestly held:** competitive learning's 0.952 / 0.726 (MNIST /
  pairs) was read at 64 to 66 spikes per 1,000 cell-ticks. At cap 1000 it reads 0.935 /
  0.704 at 31, the born rate, still over born on every seed (+0.013 / +0.033). The one-hop
  rule keeps its numbers (0.935 / 0.696), so with the rate held the two are level on MNIST
  and 0.008 apart on the pair test. Born: 0.925 / 0.677 at cap 64, 0.921 / 0.670 at 1000.
- **Speed:** the cap costs nothing (blobbench medians 67,600 and 68,600 ticks a second with
  learning off, 47,600 and 47,800 with it on, inside the rounds' spread).
- **So: keep 64.** The fault the rate job found is real (in the MNIST world under the one
  setting the motor cells run at 105 per 1,024, the shared sector bias pins, and the hidden
  cells are starved to 13 with 19 in 100 silent), but 1000 for every cell is not its fix.
  The fix that fits the findings is a cap for motor cells apart from the rest, or motor
  cells out of the sector's count: a core change, with the numbers here to start from. If
  the default must move before that, 128, not 1000: it is a fair trade (MNIST online, order
  and parity for reach and held-out scale under competitive learning), not a fix.

## 1. Why the chain passes with the cap lifted

The chain (`--task chain`): two buttons A and B, two lines that show the stage. On stage 0
pressing A moves to stage 1 and anything else stays, with no verdict either way. On stage 1
B is good and another button bad, then back to stage 0. Score: chains paid times 2 over
steps (A, B, A, B is 1.0). A coin gets 0.30, so the pass line (halfway to perfect) is 0.65.

`scripts/capdefault.sh chain` lives the quick map's chain life through `blobprobe capsplit`
(new, `src/probe/cap_split.cpp`): blobrun's life loop, with the cap set apart for motor cells
and for the rest, and over the last 2,000 steps a count of what was pressed on each stage.
Checked against blobrun: with no caps of its own the probe gives the cached lives' window
scores to the last digit on all 22 rows (`scripts/capdefault.sh probecheck`), and caps of 64
on every kind under a genome cap of 1000 give the cap-64 life.

Each cell: late score (lowest to highest seed); share of stage-0 steps on which A / B /
another button was pressed; share of stage-1 steps on which B was pressed. Seeds 5-12.

| arm | base | a2rucomp | best |
|---|---|---|---|
| c64 | 0.306 (0.06-0.49); 0.19 / 0.79 / 0.00; 1.00 | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - | 0.150 (0.00-0.60); 0.11 / 0.89 / 0.00; 1.00 |
| c1000 | 0.720 (0.67-0.75); 0.56 / 0.42 / 0.00; 1.00 | 0.525 (0.47-0.56); 0.36 / 0.63 / 0.00; 1.00 | 0.811 (0.31-1.00); 0.74 / 0.25 / 0.00; 1.00 |
| motorfree | 0.706 (0.64-0.76); 0.55 / 0.43 / 0.00; 1.00 | 0.921 (0.79-1.00); 0.87 / 0.12 / 0.00; 1.00 | 0.748 (0.13-1.00); 0.66 / 0.33 / 0.00; 1.00 |
| hiddenfree | 0.325 (0.05-0.49); 0.20 / 0.78 / 0.00; 1.00 | 0.440 (0.26-0.57); 0.29 / 0.70 / 0.00; 1.00 | 0.150 (0.00-0.62); 0.11 / 0.89 / 0.00; 1.00 |
| holdmotor | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - | 0.001 (0.00-0.00); 0.00 / 1.00 / 0.00; 1.00 | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - |
| bias64 | 0.391 (0.04-0.55); 0.25 / 0.73 / 0.00; 1.00 | 0.002 (0.00-0.01); 0.00 / 1.00 / 0.00; 1.00 | 0.048 (0.00-0.34); 0.03 / 0.97 / 0.00; 1.00 |
| bias256 | 0.391 (0.04-0.55); 0.25 / 0.73 / 0.00; 1.00 | 0.149 (0.00-0.44); 0.09 / 0.89 / 0.00; 1.00 | 0.086 (0.00-0.65); 0.06 / 0.93 / 0.00; 1.00 |
| c128 | 0.730 (0.68-0.79); 0.58 / 0.40 / 0.00; 1.00 | 0.577 (0.41-0.75); 0.41 / 0.57 / 0.00; 1.00 | 0.388 (0.00-0.95); 0.32 / 0.67 / 0.00; 1.00 |
| c256 | 0.735 (0.68-0.79); 0.58 / 0.40 / 0.00; 1.00 | 0.555 (0.50-0.62); 0.39 / 0.61 / 0.00; 1.00 | 0.859 (0.69-0.99); 0.77 / 0.21 / 0.00; 1.00 |
| t64 | 0.235 (0.08-0.52); 0.14 / 0.85 / 0.00; 1.00 | 0.086 (0.00-0.35); 0.05 / 0.95 / 0.00; 1.00 | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; 1.00 |
| t128 | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - |

Arms: c64 the default; c1000 the cap lifted; motorfree: cap 1000 on motor cells, 64 on the
rest; hiddenfree: 64 on motor cells, 1000 on the rest; holdmotor: cap 1000, motor cells'
thresholds put back as born every tick (no walk on them); bias64, bias256: cap 64, the sector
bias given room to 64 or 256 so that it can hold the rate; c128, c256: caps between; t64,
t128: cap 1000 with the target rate at 64 or 128 (part 2).

The motor cells, mean of seeds 5-12 (threshold: mean over a button's 128 motor cells at the
end of the life; vote: spikes of a button's cells in a step; rate per 1,024 cell-ticks,
sensor cells left out):

| column | arm | threshold A / B | motor cells at 64+ | vote on stage 0, A / B | vote on stage 1, A / B | rate |
|---|---|---|---|---|---|---|
| base | c64 | 24 / 42 | 0.07 | 79 / 95 | 68 / 176 | 32.4 |
| base | c1000 | 17 / 102 | 0.31 | 69 / 65 | 51 / 114 | 32.0 |
| a2rucomp | c64 | 35 / 44 | 0.24 | 106 / 153 | - / - | 60.0 |
| a2rucomp | c1000 | 11 / 102 | 0.31 | 56 / 68 | 37 / 122 | 32.1 |
| best | c64 | 17 / 39 | 0.11 | 71 / 112 | 62 / 169 | 28.5 |
| best | c1000 | 14 / 130 | 0.32 | 66 / 56 | 55 / 125 | 29.0 |

What this shows:

- **The second press is learned in every column at either cap** (B on stage 1: 1.00
  wherever stage 1 is reached). The output rule gets a verdict there. Nothing about the cap
  changes that.
- **At cap 64 the first press is stuck on B.** Learning B on stage 1 also lifts B's vote on
  stage 0 (the hidden cells fire much alike on the two stages: cosine 0.85 under the base
  rule), B's cells out-vote A's on stage 0 (95 spikes to 79), and B is pressed on 0.79 of
  stage-0 steps (0.89 under the one setting, 1.00 under a2rucomp). A B press on stage 0 gets
  no verdict, so nothing ever corrects it.
- **With the cap lifted the walk levels the two buttons on stage 0.** B's cells walk up to a
  mean threshold of 102 (130 under the one setting; the highest cell 376), where at 64 they
  sat at 42 with 7 to 24 in 100 motor cells at the cap. B's vote on stage 0 comes down to
  A's (65 to 69). The first press is then about a coin: A on 0.56 of stage-0 steps under the
  base rule, on every seed (0.67 to 0.75 late). A coin on the first press with a learned
  second press scores 2 / (1/0.56 + 1) = 0.72, over the pass line of 0.65.
- **It is the motor cells.** The cap lifted on motor cells alone: 0.706 (base), 0.748
  (best), 0.921 (a2rucomp). On every other cell alone: 0.325, 0.150, 0.440, as at cap 64.
  Motor thresholds held as born: 0.000 in every column, B on every step.
- **It is not the rate.** Under the base rule the chain life fires at 32.0 to 32.4 per 1,024
  at both caps. At cap 64 with the rate held by the sector bias: base 0.391 (as cap 64),
  a2rucomp 0.149 at a rate of 32.2 (bias256; at cap 64 it runs at 60.0 and scores 0.000).
- **A cap of 128 or 256 does the same** under the base rule (0.730 and 0.735): the levelling
  needs B's cells at about 100, not 1000. The one setting needs 256 (0.388 and 0.859).
- **It goes away when sharing is weaker.** With the target rate at 64 (cap 1000) the chain is
  0.235 (base), 0.086 (a2rucomp) and 0.000 (best); at 128, 0.000 in all three.

Is it credit over a step of time? Three and four buttons, and no stage line, seeds 5-12; the
critic column (`--g.critic_on 1 --g.readout_good 1`) for what real credit looks like:

| arm | base | a2rucomp | best | critic |
|---|---|---|---|---|
| c64 | 0.306 (0.06-0.49); 0.19 / 0.79 / 0.00; 1.00 | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - | 0.150 (0.00-0.60); 0.11 / 0.89 / 0.00; 1.00 | 0.964 (0.94-0.98); 0.95 / 0.05 / 0.00; 0.99 |
| c1000 | 0.720 (0.67-0.75); 0.56 / 0.42 / 0.00; 1.00 | 0.525 (0.47-0.56); 0.36 / 0.63 / 0.00; 1.00 | 0.811 (0.31-1.00); 0.74 / 0.25 / 0.00; 1.00 | 0.861 (0.81-0.91); 0.82 / 0.17 / 0.00; 0.95 |
| k3-64 | 0.202 (0.04-0.50); 0.12 / 0.63 / 0.21; 1.00 | 0.011 (0.00-0.08); 0.01 / 0.86 / 0.13; 1.00 | 0.197 (0.00-0.50); 0.12 / 0.71 / 0.14; 1.00 | 0.957 (0.93-0.99); 0.95 / 0.04 / 0.00; 0.98 |
| k3-1000 | 0.531 (0.38-0.71); 0.37 / 0.18 / 0.40; 1.00 | 0.402 (0.27-0.49); 0.25 / 0.29 / 0.42; 1.00 | 0.645 (0.44-0.81); 0.49 / 0.10 / 0.35; 1.00 | 0.808 (0.76-0.87); 0.77 / 0.20 / 0.02; 0.93 |
| k4-1000 | 0.375 (0.23-0.47); 0.23 / 0.13 / 0.57; 1.00 | 0.351 (0.23-0.44); 0.21 / 0.23 / 0.50; 1.00 | 0.499 (0.39-0.62); 0.34 / 0.04 / 0.54; 1.00 | - |
| k3-motorfree | 0.477 (0.36-0.65); 0.32 / 0.19 / 0.45; 1.00 | 0.516 (0.00-0.90); 0.39 / 0.13 / 0.44; 1.00 | 0.648 (0.45-0.91); 0.50 / 0.08 / 0.37; 1.00 | - |
| nocue-64 | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; 1.00 | - |
| nocue-1000 | 0.128 (0.08-0.21); 0.07 / 0.93 / 0.00; 0.98 | 0.086 (0.03-0.15); 0.05 / 0.95 / 0.00; 0.99 | 0.007 (0.00-0.01); 0.00 / 1.00 / 0.00; 1.00 | - |

- **No.** With three buttons the first press goes to A no more often than to the third
  button, which never matters: base A 0.37, third 0.40; a2rucomp A 0.25, third 0.42. With
  four: A 0.23 and the two others 0.28 each (base). Over 12 seeds A less the third is -0.06
  ±0.07 (base) and -0.17 ±0.04 (a2rucomp). What the lifted cap does is keep the paid button
  from winning the unpaid stage; which of the others is pressed is left to chance. With two
  buttons "not B" is A, and that is the whole of the pass.
- **The critic does send credit back, and it shows**: A on 0.95 of stage-0 steps with two
  buttons and still 0.95 with three (the third 0.00), at cap 64. The cap lifted costs the
  critic 0.10 (0.964 to 0.861): the walk pulls on what it learned.
- **The one setting has a small real lean to A** on top: with three buttons A 0.49, third
  0.35 (seeds 5-12) and A 0.53, third 0.22 (seeds 1-4); over 12 seeds A less the third is
  +0.19 ±0.09 (9 of 12 above 0).
  Step by step (`scripts/capdefault.sh first`, three buttons, cap 1000, seeds 5-12, the last
  2,000 steps): on a stage-0 step that follows a stage-1 step the one setting presses A on
  0.54 of them, B on 0.16 and the third button on 0.25; the base rule 0.33, 0.39, 0.23;
  a2rucomp 0.10, 0.70, 0.17. After the third button on stage 0 the base rule presses it again
  on 0.59 of steps. So the one setting's lean to A is there on the first try and is not only
  "not the button just pressed". Where it comes from was not found; it is a quarter of the
  critic's.
- **Without the stage line nothing works** (0.13 or less at either cap): the sheet is not
  remembering the stage.

Seeds 1-4 (the tuning seeds), same cells:

| arm | base | a2rucomp | best |
|---|---|---|---|
| c64 | 0.404 (0.29-0.54); 0.26 / 0.72 / 0.00; 1.00 | 0.000 (0.00-0.00); 0.00 / 1.00 / 0.00; - | 0.195 (0.00-0.78); 0.16 / 0.83 / 0.00; 1.00 |
| c1000 | 0.718 (0.64-0.81); 0.56 / 0.42 / 0.00; 1.00 | 0.574 (0.50-0.66); 0.40 / 0.59 / 0.00; 1.00 | 0.994 (0.98-1.00); 0.99 / 0.01 / 0.00; 1.00 |
| k3-1000 | 0.491 (0.42-0.56); 0.33 / 0.18 / 0.45; 1.00 | 0.415 (0.37-0.50); 0.26 / 0.27 / 0.44; 1.00 | 0.658 (0.28-0.93); 0.53 / 0.20 / 0.22; 1.00 |

The same as 5-12: base 0.718, the one setting 0.994 (A on 0.99 of stage-0 steps with two
buttons, 0.53 with three).

The other rows only a critic passed or lifted (cartpole, noisy lines): the cap lifted passes
neither. Seeds 5-12: cartpole 17 to 23 steps a fall under the base rule, 13 to 16 and 17 to
18 (a coin 21, the critic 77); noisy lines 0.27, 0.32, 0.34 (the pass line 0.61). Seeds
1-4 the same (table in part 3): cartpole 19 to 22, 13 to 15, 16 to 17; noisy lines 0.29,
0.32, 0.37.

## 2. What MNIST online and reach were gaining at cap 64

`scripts/capdefault.sh rate`: the quick map's MNIST, far3 and far5 lives through the same
probe, seeds 5-8, with the rate each life really ran at (blobrun's end-of-life rate counts
sensor cells, which in the MNIST world are a quarter of the sheet; here they are left out).

A point about "the target rate" first. `sector_target_per_1024` is only the sector bias's
target. The per-cell walk has its own: a threshold steps up with chance 2^-4 on a spike and
down with chance 2^-9 a tick, so each cell settles where it fires on 2^-5 of the ticks, 32
per 1,024, whatever the sector's number says. Raising the sector's number alone does nothing
(arm s64: the rate stays at 31 to 34 and the scores at cap 1000's). An honest higher target
moves both: `--g.sector_target_per_1024 64 --g.threshold_down_shift 8` (t64), or 128 and 7
(t128). Only powers of two can be asked for, because the chances are shifts.

Each cell: late score; rate per 1,024 cell-ticks (hidden cells, motor cells); share of
hidden cells silent, share of motor cells stuck (firing on over a quarter of the ticks).

MNIST online (20,000 digits, the score on digits not seen before):

| arm | base | a2rucomp | best |
|---|---|---|---|
| c64 | 0.214; 93 (34, 210); 0.00, 0.26 | 0.840; 73 (63, 93); 0.01, 0.01 | 0.716; 44 (13, 105); 0.19, 0.00 |
| c128 | 0.225; 53 (32, 94); 0.00, 0.05 | 0.846; 56 (50, 65); 0.00, 0.00 | 0.745; 40 (22, 76); 0.10, 0.00 |
| c256 | 0.210; 39 (32, 53); 0.00, 0.00 | 0.826; 45 (39, 56); 0.00, 0.00 | 0.740; 37 (30, 52); 0.08, 0.00 |
| c1000 | 0.171; 31 (32, 30); 0.00, 0.00 | 0.740; 34 (32, 37); 0.00, 0.00 | 0.672; 33 (34, 30); 0.08, 0.00 |
| t64 | 0.181; 62 (63, 60); 0.00, 0.00 | 0.826; 63 (62, 63); 0.00, 0.00 | 0.758; 42 (35, 57); 0.08, 0.00 |
| t128 | 0.191; 119 (115, 126); 0.00, 0.00 | 0.834; 83 (76, 96); 0.01, 0.00 | 0.737; 46 (14, 109); 0.14, 0.00 |
| s64 | 0.168; 31 (32, 30); 0.00, 0.00 | 0.727; 34 (32, 37); 0.00, 0.00 | 0.670; 33 (35, 29); 0.08, 0.00 |
| bias64 | 0.224; 64 (14, 163); 0.00, 0.15 | 0.839; 57 (41, 89); 0.03, 0.02 | 0.712; 39 (12, 93); 0.15, 0.00 |
| hiddenfree | 0.219; 83 (24, 201); 0.00, 0.24 | 0.828; 48 (32, 80); 0.00, 0.03 | 0.686; 43 (14, 101); 0.16, 0.00 |
| motorfree | 0.164; 38 (42, 31); 0.00, 0.00 | 0.806; 67 (83, 35); 0.00, 0.00 | 0.679; 34 (36, 29); 0.08, 0.00 |
| m64t64 | 0.194; 108 (43, 238); 0.00, 0.33 | 0.830; 66 (61, 76); 0.00, 0.01 | 0.706; 45 (15, 106); 0.19, 0.00 |
| holdmotor | 0.109; 122 (22, 321); 0.00, 0.49 | 0.814; 48 (31, 81); 0.00, 0.04 | 0.668; 47 (10, 120); 0.58, 0.00 |

Reach, cue 3 tiles from the buttons (far3):

| arm | base | a2rucomp | best |
|---|---|---|---|
| c64 | 0.866; 32 (32, 34); 0.00, 0.00 | 0.921; 47 (47, 46); 0.01, 0.00 | 0.990; 27 (27, 33); 0.13, 0.00 |
| c128 | 0.831; 32 (32, 32); 0.00, 0.00 | 0.845; 38 (38, 32); 0.00, 0.00 | 0.986; 27 (27, 30); 0.13, 0.00 |
| c256 | 0.802; 32 (32, 32); 0.00, 0.00 | 0.754; 33 (33, 31); 0.00, 0.00 | 0.982; 27 (27, 30); 0.13, 0.00 |
| c1000 | 0.793; 32 (32, 32); 0.00, 0.00 | 0.721; 32 (32, 31); 0.00, 0.00 | 0.978; 27 (27, 30); 0.13, 0.00 |
| t64 | 0.907; 62 (62, 62); 0.00, 0.00 | 0.771; 59 (60, 55); 0.00, 0.00 | 0.990; 29 (28, 45); 0.13, 0.00 |
| t128 | 0.889; 108 (108, 107); 0.00, 0.00 | 0.927; 66 (65, 83); 0.07, 0.00 | 0.991; 30 (28, 51); 0.13, 0.00 |
| s64 | 0.817; 32 (32, 32); 0.00, 0.00 | 0.689; 32 (32, 32); 0.00, 0.00 | 0.979; 28 (27, 30); 0.13, 0.00 |
| bias64 | 0.860; 33 (32, 35); 0.00, 0.00 | 0.926; 37 (37, 41); 0.00, 0.00 | 0.989; 27 (27, 33); 0.13, 0.00 |
| hiddenfree | 0.861; 32 (32, 34); 0.00, 0.00 | 0.794; 32 (32, 34); 0.00, 0.00 | 0.986; 27 (27, 33); 0.13, 0.00 |
| motorfree | 0.807; 32 (32, 32); 0.00, 0.00 | 0.798; 47 (48, 30); 0.01, 0.00 | 0.983; 27 (27, 30); 0.13, 0.00 |
| m64t64 | 0.935; 62 (61, 67); 0.00, 0.00 | 0.790; 60 (60, 58); 0.00, 0.00 | 0.990; 29 (28, 45); 0.13, 0.00 |
| holdmotor | 0.879; 33 (30, 76); 0.00, 0.02 | 0.875; 35 (31, 88); 0.00, 0.01 | 0.986; 28 (26, 48); 0.13, 0.00 |

Reach, 5 tiles (far5):

| arm | base | a2rucomp | best |
|---|---|---|---|
| c64 | 0.555; 32 (32, 33); 0.00, 0.00 | 0.802; 46 (46, 34); 0.01, 0.00 | 0.936; 27 (27, 30); 0.13, 0.00 |
| c128 | 0.550; 32 (32, 31); 0.00, 0.00 | 0.722; 37 (37, 32); 0.00, 0.00 | 0.924; 27 (27, 29); 0.13, 0.00 |
| c256 | 0.553; 32 (32, 32); 0.00, 0.00 | 0.607; 33 (33, 32); 0.00, 0.00 | 0.911; 27 (27, 29); 0.13, 0.00 |
| c1000 | 0.553; 32 (32, 32); 0.00, 0.00 | 0.602; 32 (32, 31); 0.00, 0.00 | 0.908; 27 (27, 28); 0.13, 0.00 |
| t64 | 0.598; 62 (61, 61); 0.00, 0.00 | 0.640; 59 (59, 56); 0.00, 0.00 | 0.934; 30 (29, 38); 0.13, 0.00 |
| t128 | 0.547; 108 (108, 107); 0.00, 0.00 | 0.759; 66 (67, 59); 0.04, 0.00 | 0.955; 30 (29, 42); 0.13, 0.00 |
| s64 | 0.541; 32 (32, 32); 0.00, 0.00 | 0.586; 32 (32, 32); 0.00, 0.00 | 0.905; 28 (28, 29); 0.13, 0.00 |
| bias64 | 0.553; 33 (32, 33); 0.00, 0.00 | 0.799; 37 (37, 31); 0.01, 0.00 | 0.937; 28 (27, 30); 0.13, 0.00 |
| hiddenfree | 0.558; 32 (32, 33); 0.00, 0.00 | 0.601; 32 (32, 32); 0.00, 0.00 | 0.908; 27 (27, 30); 0.13, 0.00 |
| motorfree | 0.549; 32 (32, 32); 0.00, 0.00 | 0.752; 45 (46, 28); 0.01, 0.00 | 0.932; 27 (27, 29); 0.13, 0.00 |
| m64t64 | 0.596; 62 (61, 64); 0.00, 0.00 | 0.642; 59 (59, 57); 0.00, 0.00 | 0.938; 30 (29, 38); 0.13, 0.00 |
| holdmotor | 0.582; 33 (30, 71); 0.00, 0.01 | 0.670; 34 (31, 68); 0.00, 0.00 | 0.918; 28 (27, 36); 0.13, 0.00 |

Arms as in part 1, and: s64 the sector target at 64 alone; m64t64: hiddenfree with the
target at 64.

**MNIST online gains from motor cells that fire far over their share, and from little else.**

- Under the base rule the runaway at cap 64 is in the motor cells only: they fire at 210 per
  1,024 (6.5 times the target, 26 in 100 stuck, 77 in 100 at the cap) while the hidden cells
  sit at 34. The MNIST world's "3.1 times the target" is that. Capping only the motor cells
  (hiddenfree) gives the whole score back, 0.219 against 0.214; capping only the rest
  (motorfree) gives nothing, 0.164 against 0.171. More spikes everywhere do not: 0.181 at
  twice the target and 0.191 at four times.
- Under a2rucomp both kinds run at cap 64 (hidden 63, motor 93). The motor cells' cap alone
  gives back 0.828 of 0.840 with the hidden cells held at 32. An honest target of 64 gives
  back 0.826 (rate 63, under the 73 the cap-64 life ran at), and 128 gives 0.834.
- Under the one setting the cap of 64 is doing harm that the score hides: the motor cells
  run at 105, the sector bias sits at its limit on 94 in 100 sectors, and the hidden cells
  are starved to 13 per 1,024 with 19 in 100 silent. With the cap lifted and the target at
  64 the hidden cells fire at 35, the motor cells at 57, and the score is higher than at cap
  64: 0.758 against 0.716 (cap 1000 alone: 0.672). Two winners a tile allow about 32, so
  here too the walk cannot reach its target for the hidden cells; whether the gain is the
  motor cells' spikes or the hidden cells let go was not separated.
- A cap between the two keeps MNIST online and holds most of the rate: at 128, 0.225 / 0.846
  / 0.745 (base / a2rucomp / best) at 53 / 56 / 40 per 1,024; at 256, 0.210 / 0.826 / 0.740
  at 39 / 45 / 37, with the hidden cells at 32 / 39 / 30.
- "The rate held another way" at cap 64 (the sector bias given room) keeps the score (0.224
  / 0.839 / 0.712) but is not an honest hold: the motor cells keep running (163 / 89 / 93)
  and the bias takes the difference out of the hidden cells (14 / 41 / 12).

**Reach with competitive learning gains from the cap itself, on both kinds of cell, not
from the spikes.**

- far3 under a2rucomp: 0.921 at cap 64 (rate 47), 0.721 at cap 1000. With the rate held at
  37 by the sector bias and the cap still at 64: 0.926. With an honest target of 64 at cap
  1000 (rate 59, more spikes than the cap-64 life had): 0.771. So the spikes are not it.
- The cap on either kind alone gives back a third: motor cells capped 0.794, the rest capped
  0.798. On far5 it is the hidden cells' cap (0.752 against 0.601 for the motor cells').
- A target of 128 gives it back (far3 0.927, far5 0.759 of 0.802), but not honestly: four
  winners a tile allow at most 64 per 1,024, the walk cannot reach its target, the
  thresholds come down (2 in 100 hidden cells at 64 or more, against 16 to 19) and per-cell
  sharing is in effect off. That is the same thing the cap does for the cells it pins: a
  cell at the cap is one the walk can no longer hold to its share.
- A cap between loses in step: far3 0.845 at 128, 0.754 at 256; far5 0.722, 0.607.
- Under the base rule far3 loses 0.07 with the cap lifted, all of it the motor cells'
  (hiddenfree 0.861 of 0.866), and a target of 64 more than gives it back (0.907). Under
  the one setting reach barely moves (0.990 to 0.978; with the target at 64, 0.990).

So the answer to "does a higher honest target give the scores back": **a target of 64 gives
back MNIST online under a2rucomp (0.826 of 0.840) and more than gives it back under the one
setting (0.758 against 0.716), and reach under the base rule (0.907 against 0.866). It does
not give back reach with competitive learning (far3 0.771 of 0.921, far5 0.640 of 0.802) or
the base rule's MNIST (0.181 of 0.214).** What gives those back is the cap staying on the
cells in question: the motor cells for the base rule's MNIST, both kinds for reach with
competitive learning.

## 3. The quick map

`scripts/capdefault.sh map`. Late score; `*` passes (halfway from chance to perfect and
above its learning-off twin by 2 standard errors; the twin runs under the same cap); in
brackets the paired difference to the same column at cap 64, `+` / `-` at 2 standard errors.

Cap 64 against cap 128 and cap 1000, seeds 5-12 (the cap-64 and cap-1000 lives are from the
caches, the cap-1000 ones the rate-control job's; the cap-128 lives are new):

| row | base | at 128 | at 1000 | a2rucomp | at 128 | at 1000 | best | at 128 | at 1000 |
|---|---|---|---|---|---|---|---|---|---|
| echo2 | 1.000* | 1.000* (-0.000) | 0.973* (-0.027-) | 1.000* | 1.000* (+0.000) | 0.948* (-0.052-) | 1.000* | 1.000* (+0.000) | 1.000* (+0.000) |
| echo8 | 0.801* | 0.792* (-0.008) | 0.802* (+0.001) | 0.925* | 0.926* (+0.001) | 0.910* (-0.015-) | 0.976* | 0.988* (+0.012) | 0.988* (+0.011) |
| echo16 | 0.266 | 0.267 (+0.001) | 0.267 (+0.001) | 0.620* | 0.619* (-0.001) | 0.628* (+0.008) | 0.955* | 0.954* (-0.001) | 0.949* (-0.006-) |
| shuffled | 1.000* | 1.000* (+0.000) | 0.977* (-0.023-) | 1.000* | 1.000* (+0.000) | 0.953* (-0.047-) | 1.000* | 1.000* (+0.000) | 1.000* (+0.000) |
| far3 | 0.847* | 0.815* (-0.032-) | 0.797* (-0.050-) | 0.925* | 0.861* (-0.065-) | 0.721 (-0.205-) | 0.975* | 0.969* (-0.006-) | 0.962* (-0.013-) |
| far5 | 0.576 | 0.563 (-0.013) | 0.565 (-0.011-) | 0.841* | 0.728 (-0.113-) | 0.600 (-0.240-) | 0.933* | 0.922* (-0.010-) | 0.901* (-0.031-) |
| xor | 0.882* | 0.865* (-0.017) | 0.851* (-0.032-) | 0.997* | 0.986* (-0.011-) | 0.995* (-0.003) | 0.936* | 0.960* (+0.023) | 0.961* (+0.025) |
| parity3 | 0.561 | 0.557 (-0.004) | 0.548 (-0.013) | 0.831* | 0.914* (+0.083+) | 0.883* (+0.052+) | 0.642 | 0.648 (+0.006) | 0.641 (-0.001) |
| order | 0.676 | 0.686 (+0.010) | 0.677 (+0.001) | 0.733 | 0.874* (+0.141+) | 0.843* (+0.110+) | 0.994* | 0.996* (+0.002) | 0.996* (+0.001) |
| gap | 0.509 | 0.521 (+0.012) | 0.518 (+0.009) | 0.938* | 0.970* (+0.032) | 0.924* (-0.014) | 0.897* | 0.899* (+0.003) | 0.885* (-0.011) |
| delayed2 | 0.544 | 0.555 (+0.011) | 0.553 (+0.009) | 0.919* | 0.881* (-0.038) | 0.925* (+0.006) | 1.000* | 1.000* (+0.000) | 1.000* (+0.000) |
| hold1000 | 0.506 | 0.493 (-0.013) | 0.485 (-0.021) | 0.508 | 0.494 (-0.014) | 0.476 (-0.031) | 0.998* | 1.000* (+0.002) | 1.000* (+0.002) |
| bandit | 0.773* | 0.771* (-0.002) | 0.756* (-0.017-) | 1.000* | 0.998* (-0.002-) | 0.960* (-0.040-) | 0.995* | 0.988* (-0.007-) | 0.984* (-0.011-) |
| chain | 0.306 | 0.730* (+0.425+) | 0.720* (+0.415+) | 0.000 | 0.577 (+0.577+) | 0.525 (+0.525+) | 0.150 | 0.388 (+0.237) | 0.811* (+0.661+) |
| cartpole | 17 | 21 (+3.835+) | 23 (+5.477+) | 13 | 15 (+1.716+) | 16 (+3.336+) | 17 | 16 (-0.355) | 18 (+1.002) |
| catch | 0.298 | 0.325 (+0.027+) | 0.333 (+0.034+) | 0.366 | 0.378 (+0.013) | 0.372 (+0.006) | 0.671* | 0.635* (-0.036) | 0.681* (+0.011) |
| noisy | 0.268 | 0.276 (+0.007+) | 0.272 (+0.004) | 0.218 | 0.329 (+0.111+) | 0.321 (+0.104+) | 0.307 | 0.326 (+0.018) | 0.337 (+0.030) |
| copy2 | 0.638 | 0.652 (+0.014) | 0.636 (-0.003) | 0.927* | 0.903* (-0.025) | 0.886* (-0.041) | 0.897* | 0.907* (+0.009) | 0.903* (+0.005) |
| inv-scale | 0.699 | 0.664 (-0.035-) | 0.654 (-0.045-) | 0.719 | 0.502 (-0.218-) | 0.527 (-0.192-) | 0.602 | 0.590 (-0.012) | 0.586 (-0.016) |
| inv-position | 0.487 | 0.498 (+0.011) | 0.504 (+0.017+) | 0.536 | 0.538 (+0.002) | 0.537 (+0.002) | 0.471 | 0.464 (-0.007) | 0.458 (-0.013) |
| pairs | 0.273 | 0.283 (+0.010) | 0.265 (-0.008) | 0.358 | 0.381 (+0.023) | 0.427 (+0.069+) | 0.525 | 0.502 (-0.023-) | 0.525 (+0.000) |
| mnist | 0.214 | 0.230 (+0.016+) | 0.180 (-0.034-) | 0.838* | 0.844* (+0.006+) | 0.738* (-0.100-) | 0.714* | 0.745* (+0.031+) | 0.671* (-0.043-) |
| rows passed | 6 | 7 | 7 | 13 | 13 | 12 | 15 | 15 | 16 |

- **Rows passed: cap 64 6 / 13 / 15, cap 128 7 / 13 / 15, cap 1000 7 / 12 / 16** (base /
  a2rucomp / best).
- At 1000 the rows gained are the chain (base, best), which part 1 says is not what the row
  is for, and order under a2rucomp (0.733 to 0.843). The rows lost are far3 and far5 under
  a2rucomp.
- What falls at 1000 is on the output's side in every column: echo of 2 and shuffled echo
  stop being perfect under base and a2rucomp (1.000 to 0.95 to 0.98), the bandit loses 0.01
  to 0.04, MNIST online 0.03 to 0.10, reach 0.01 to 0.24. What rises is under competitive
  learning: order +0.11, parity of 3 +0.05 (it passed already), noisy lines +0.10 and pairs
  +0.07 (neither passes).
- **A cap of 128 keeps the output's side and gets the inside's gains**: echo of 2, shuffled
  and the bandit within 0.01 of cap 64, MNIST online +0.016 / +0.006 / +0.031, and under
  a2rucomp order 0.874 (passed), parity of 3 +0.083, noisy lines +0.111. It loses a third to
  two thirds of what 1000 loses on reach (under a2rucomp far3 -0.065, far5 -0.113, which
  falls under its line of 0.739; under base far3 -0.032) and all of held-out scale under
  a2rucomp (-0.218). The chain: passed under base (0.730, the same side effect), not under a2rucomp
  (0.577) or the one setting (0.388: there B's cells need a mean threshold of 130).

The other changes part 2 points to, seeds 5-8, each beside its own cap-64 column: c128
again, m64 (cap 1000 for every cell but the motor cells, which keep 64; run through the
probe, twins too), t64 (cap 1000 and the target at 64). Rows shown: those that move by 0.03
or more in some column, and exclusive-or, 16 cues, order and MNIST; every row counts in
"rows passed". All rows: `out/capdefault/table-map58.md`.

Base:

| row | cap 64 | 1000 | c128 | m64 | t64 |
|---|---|---|---|---|---|
| echo8 | 0.791* | 0.799* (+0.007) | 0.784* (-0.008) | - | 0.572* (-0.220-) |
| echo16 | 0.258 | 0.253 (-0.004) | 0.252 (-0.005) | - | 0.184 (-0.074-) |
| far3 | 0.866* | 0.793* (-0.074-) | 0.831* (-0.035-) | - | 0.907* (+0.041) |
| far5 | 0.555 | 0.553 (-0.002) | 0.550 (-0.006) | - | 0.598 (+0.043+) |
| xor | 0.891* | 0.866* (-0.025) | 0.866* (-0.024-) | - | 0.729 (-0.162-) |
| parity3 | 0.555 | 0.547 (-0.008) | 0.543 (-0.012) | - | 0.524 (-0.032-) |
| order | 0.669 | 0.651 (-0.018) | 0.672 (+0.003) | - | 0.727 (+0.057+) |
| chain | 0.237 | 0.712* (+0.475+) | 0.734* (+0.497+) | - | 0.276 (+0.039) |
| cartpole | 17 | 22 (+4.938+) | 21 (+3.379+) | - | 21 (+3.592+) |
| catch | 0.295 | 0.340 (+0.045+) | 0.327 (+0.031) | - | 0.303 (+0.008) |
| copy2 | 0.639 | 0.617 (-0.022) | 0.644 (+0.006) | - | 0.704 (+0.066+) |
| inv-scale | 0.706 | 0.669 (-0.037) | 0.652 (-0.054-) | - | 0.737* (+0.031) |
| pairs | 0.287 | 0.258 (-0.029) | 0.296 (+0.008) | - | 0.221 (-0.067) |
| mnist | 0.214 | 0.171 (-0.043-) | 0.225 (+0.011) | - | 0.181 (-0.034-) |
| rows passed | 6 | 7 | 7 | - | 6 |

a2rucomp:

| row | cap 64 | 1000 | c128 | m64 | t64 |
|---|---|---|---|---|---|
| echo2 | 1.000* | 0.933* (-0.067-) | 1.000* (+0.000) | 1.000* (+0.000) | - |
| echo16 | 0.596* | 0.614* (+0.018) | 0.607* (+0.011) | 0.605* (+0.009) | - |
| shuffled | 1.000* | 0.952* (-0.048-) | 1.000* (+0.000) | 0.987* (-0.013) | - |
| far3 | 0.921* | 0.721 (-0.200-) | 0.845* (-0.076-) | 0.794* (-0.127-) | - |
| far5 | 0.802* | 0.602 (-0.200-) | 0.722 (-0.080-) | 0.601 (-0.201-) | - |
| xor | 0.995* | 0.996* (+0.002) | 0.985* (-0.010) | 0.989* (-0.006-) | - |
| parity3 | 0.816* | 0.889* (+0.074+) | 0.917* (+0.101+) | 0.883* (+0.067+) | - |
| order | 0.696 | 0.841* (+0.145+) | 0.850* (+0.154+) | 0.808* (+0.113) | - |
| gap | 0.931* | 0.921* (-0.010) | 0.971* (+0.041) | 0.937* (+0.006) | - |
| delayed2 | 0.951* | 0.947* (-0.004) | 0.893* (-0.057) | 0.942* (-0.010) | - |
| hold1000 | 0.506 | 0.450 (-0.056) | 0.503 (-0.003) | 0.476 (-0.030) | - |
| bandit | 1.000* | 0.958* (-0.042-) | 0.997* (-0.003) | 0.997* (-0.003-) | - |
| chain | 0.000 | 0.523 (+0.523+) | 0.578 (+0.578+) | 0.468 (+0.468+) | - |
| cartpole | 13 | 16 (+3.372+) | 15 (+1.769+) | 14 (+1.110+) | - |
| catch | 0.349 | 0.363 (+0.014) | 0.381 (+0.032) | 0.394 (+0.045) | - |
| noisy | 0.270 | 0.336 (+0.066) | 0.347 (+0.077) | 0.337 (+0.067) | - |
| inv-scale | 0.732 | 0.522 (-0.210-) | 0.509 (-0.223-) | 0.648 (-0.085) | - |
| pairs | 0.400 | 0.413 (+0.013) | 0.400 (-0.000) | 0.450 (+0.050+) | - |
| mnist | 0.840* | 0.740* (-0.100-) | 0.846* (+0.006+) | 0.827* (-0.013-) | - |
| rows passed | 13 | 12 | 13 | 13 | - |

The one setting:

| row | cap 64 | 1000 | c128 | m64 | t64 |
|---|---|---|---|---|---|
| echo16 | 0.952* | 0.943* (-0.009-) | 0.951* (-0.001) | 0.951* (-0.001) | - |
| xor | 0.935* | 0.964* (+0.029) | 0.960* (+0.025) | 0.952* (+0.017) | - |
| order | 0.994* | 0.996* (+0.002) | 0.998* (+0.005) | 1.000* (+0.006) | - |
| chain | 0.298 | 0.926* (+0.629+) | 0.369 (+0.072) | 0.298 (+0.000) | - |
| cartpole | 16 | 17 (+1.142) | 16 (+0.304) | 16 (+0.225) | - |
| catch | 0.664* | 0.683* (+0.018) | 0.633* (-0.032) | 0.677* (+0.012+) | - |
| noisy | 0.324 | 0.365 (+0.041) | 0.349 (+0.025+) | 0.385 (+0.061) | - |
| inv-scale | 0.558 | 0.526 (-0.032-) | 0.503 (-0.055-) | 0.535 (-0.023) | - |
| mnist | 0.716* | 0.672* (-0.044-) | 0.745* (+0.030+) | 0.686* (-0.029-) | - |
| rows passed | 15 | 16 | 15 | 15 | - |

- **A target of 64 is not a default** (base column): 8 cues 0.791 to 0.572, exclusive-or
  0.891 to 0.729 (it stops passing), 16 cues -0.07, MNIST still down 0.03, the chain not
  passed; it gains far5, order, copy and the held-out rows a little.
  Only the base column was run with it.
- **Motor cells kept at 64, the rest lifted (m64)** splits what cap 1000 does. Under
  a2rucomp it keeps the output's rows (echo of 2 1.000, bandit 0.997, MNIST 0.827) and
  keeps the inside's gains (order 0.808, parity of 3 0.883), and it loses reach as cap 1000
  does (far3 0.794, far5 0.601) and does not pass the chain (0.468). Under the one setting it
  changes little: the bandit whole, the chain as at cap 64 (0.298), MNIST 0.686 (-0.029),
  noisy lines and parity of 3 up a little. The base column was not run with it on the map;
  part 2's four rows under the base rule read as at cap 64 (0.219, 0.861, 0.558, 0.325).
- Rows passed, seeds 5-8 (cap 64 / 1000 / c128 / m64 / t64):
  base 6 / 7 / 7 / not run / 6; a2rucomp 13 / 12 / 13 / 13 / not run; the one setting
  15 / 16 / 15 / 15 / not run.
- **No column keeps both MNIST online and reach with competitive learning except cap 64.**

Seeds 1-4, the rows only a critic passed or lifted, cap 64 against cap 1000:

| row | base | at 1000 | a2rucomp | at 1000 | best | at 1000 |
|---|---|---|---|---|---|---|
| chain | 0.404 | 0.718* (+0.313+) | 0.000 | 0.574 (+0.574+) | 0.195 | 0.994* (+0.800+) |
| cartpole | 19 | 22 (+2.930+) | 13 | 15 (+2.324+) | 16 | 17 (+1.213+) |
| noisy | 0.285 | 0.287 (+0.002) | 0.197 | 0.321 (+0.123+) | 0.331 | 0.366 (+0.036) |
| rows passed | 0 | 1 | 0 | 0 | 0 | 1 |

## 4. The inside-learning test at cap 64, 128 and 1000

`scripts/capdefault.sh inside`. After a life of 60,000 digits the sheet is frozen and a fresh
linear reader of the 3,072 non-sensor cells is fit on the 60,000 training digits and scored
on test digits: MNIST, and the pair test (`--pair 2 --pair-mix 1`: two half-size digits
interleaved, same class or not). Seeds 5-12. The recipes are the ones whose numbers the
findings quote, so that their runs can stand: on MNIST competitive learning is a life with
no reward (`--g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5`, the
compete2 job; its born sheet has the tile's winners at 4, "born-tw") and the one-hop rule is
a taught life told the answer (`--g.hop_on 1 --g.hop_shift 7`, the hop job); on the pair
test both are taught lives (the hop2 job's "same" recipe; the one-hop rule with
`--g.hop_shift 7 --g.hop_fired 0`). The cap's number is on the life and the read alike.
rate: the hidden cells' spikes per 1,000 cell-ticks while the reader's digits are shown.

| test | sheet | cap | score | rate | lived - born |
|---|---|---|---|---|---|
| mnist | born | 64 | 0.9251 | 33.1 |  |
| mnist | born-tw | 64 | 0.9264 | 32.8 |  |
| mnist | comp | 64 | 0.9523 | 63.6 | +0.0259 ±0.0013 |
| mnist | hop | 64 | 0.9355 | 37.8 | +0.0104 ±0.0009 |
| mnist | born | 128 | 0.9209 | 31.6 |  |
| mnist | born-tw | 128 | 0.9224 | 31.5 |  |
| mnist | comp | 128 | 0.9439 | 45.0 | +0.0215 ±0.0011 |
| mnist | hop | 128 | 0.9360 | 33.8 | +0.0150 ±0.0006 |
| mnist | born | 1000 | 0.9214 | 31.3 |  |
| mnist | born-tw | 1000 | 0.9217 | 31.3 |  |
| mnist | comp | 1000 | 0.9351 | 31.2 | +0.0134 ±0.0015 |
| mnist | hop | 1000 | 0.9349 | 31.1 | +0.0134 ±0.0011 |
| pair | born | 64 | 0.6774 | 32.7 |  |
| pair | comp | 64 | 0.7257 | 65.8 | +0.0483 ±0.0032 |
| pair | hop | 64 | 0.6914 | 40.7 | +0.0140 ±0.0021 |
| pair | born | 128 | 0.6723 | 31.4 |  |
| pair | comp | 128 | 0.7173 | 45.1 | +0.0451 ±0.0030 |
| pair | hop | 128 | 0.6951 | 33.9 | +0.0228 ±0.0026 |
| pair | born | 1000 | 0.6703 | 31.3 |  |
| pair | comp | 1000 | 0.7036 | 31.2 | +0.0333 ±0.0035 |
| pair | hop | 1000 | 0.6955 | 31.1 | +0.0252 ±0.0029 |

- **Competitive learning with the rate honestly held reads 0.935 / 0.704** (MNIST / pairs,
  at 31 spikes per 1,000 cell-ticks, the born sheet's rate), not 0.952 / 0.726, which were
  read at 64 to 66. It is still above its born sheet on every seed: +0.013 on MNIST (+0.026
  as it fires) and +0.033 on the pair test (+0.048).
- **The one-hop rule does not lose**: 0.9349 / 0.6955 at cap 1000 against 0.9355 / 0.6914,
  and its gain over born goes up (+0.013 / +0.025 from +0.010 / +0.014), because the born
  sheet reads a little lower with the cap lifted (0.9214 / 0.6703 against 0.9251 / 0.6774).
- **So with the rate held the two rules are level on MNIST** (competitive learning less the
  one-hop rule: +0.0002 ±0.0005, 4 of 8 seeds) and competitive learning leads on the pair
  test by 0.008 ±0.004 (6 of 8), where at cap 64 it led by 0.017 and 0.034. More than half
  of its lead came with the runaway.
- At 128 the reads sit between, with competitive learning at 45: 0.944 / 0.717.
- Copied runs: one seed of each of the 11 copied groups was run again with this build and
  read the same to four digits (`out/capdefault/table-inside.md`).

## 5. Speed, hardware, long lives

`scripts/capdefault.sh speed`:

| cap | learning off | learning on |
|---|---|---|
| 64 | 66939 to 69520 (67612) | 45976 to 47818 (47638) |
| 1000 | 67256 to 69328 (68577) | 46593 to 47967 (47762) |
| 1000, target 64 | 69712 to 71336 (70373) | 45496 to 45952 (45652) |

The cap is one compare in the walk and thresholds are 16-bit already (`birth.cpp` clips the
number at 30,000), so 64, 128 and 1000 are the same machine. Five rounds
taking turns, medians: 67,600 ticks a second at cap 64 and 68,600 at 1000 with learning off,
47,600 and 47,800 with it on; the rounds overlap. A target of 64 measures 70,400 and 45,700
there, but blobbench's sheet has nothing plugged in; a life at twice the spikes costs more
(the compete2 doc: 1.2 times for the doubled rate).

Nothing in `src/blob/` changed in this job. `blobprobe capsplit` is a probe
(`src/probe/cap_split.cpp`, one line each in `src/probe/main.cpp`); its caps by kind of cell
are a loop over the cells after each tick, which the engine would not need: in the walk the
cap is already read per cell, so a cap per kind is one more number picked by the motor mask.

Long lives (`scripts/capdefault.sh alive`: the rate-control job's first test, `blobprobe
ratehealth`; 160,000 steps for echo at 3 tiles and delayed echo of 2, 20,000 digits for
MNIST; base, base with competitive learning (comp) and the one setting; seeds 5 and 6; the
cap-64 and cap-1000 lives are that job's). Each cell: rate over target at the end of the
life; share of hidden thresholds at the cap, silent share of plain hidden cells; late score.

| setting | world | cap 64 | cap 128 | cap 256 | cap 1000 |
|---|---|---|---|---|---|
| base | far3 | 1.013; 0.005, 0.000; 0.909 | 1.005; 0.000, 0.000; 0.887 | 1.004; 0.000, 0.000; 0.836 | 1.004; 0.000, 0.000; 0.818 |
| base | delayed2 | 1.008; 0.002, 0.000; 0.560 | 1.004; 0.000, 0.000; 0.561 | 1.008; 0.000, 0.000; 0.552 | 1.008; 0.000, 0.000; 0.552 |
| base | mnist | 3.084; 0.098, 0.000; 0.206 | 1.650; 0.022, 0.000; 0.220 | 1.190; 0.001, 0.000; 0.212 | 1.006; 0.000, 0.000; 0.165 |
| comp | far3 | 1.523; 0.197, 0.012; 0.985 | 1.173; 0.062, 0.000; 0.881 | 1.018; 0.014, 0.000; 0.831 | 1.020; 0.000, 0.000; 0.767 |
| comp | delayed2 | 1.095; 0.035, 0.000; 0.986 | 1.023; 0.010, 0.000; 0.955 | 1.002; 0.000, 0.000; 0.938 | 1.007; 0.000, 0.000; 0.950 |
| comp | mnist | 2.562; 0.275, 0.145; 0.217 | 1.838; 0.183, 0.001; 0.258 | 1.335; 0.102, 0.000; 0.240 | 1.004; 0.000, 0.000; 0.177 |
| best | far3 | 0.853; 0.007, 0.001; 0.988 | 0.848; 0.000, 0.000; 0.992 | 0.848; 0.000, 0.000; 0.988 | 0.847; 0.000, 0.000; 0.986 |
| best | delayed2 | 0.936; 0.004, 0.001; 1.000 | 0.930; 0.000, 0.000; 1.000 | 0.927; 0.000, 0.000; 1.000 | 0.927; 0.000, 0.000; 1.000 |
| best | mnist | 1.364; 0.001, 0.144; 0.708 | 1.239; 0.000, 0.029; 0.734 | 1.156; 0.000, 0.001; 0.736 | 1.020; 0.000, 0.000; 0.657 |

- At 256 the rate holds on the toy worlds (1.00 to 1.02) and runs 1.16 to 1.34 in the MNIST
  world, where 64 runs 1.36 to 3.08 and 1000 holds 1.00 to 1.02. Nothing is silent or stuck
  that is not at 1000 too. At 128 the toy worlds run 1.00 to
  1.17 and the MNIST world 1.24 to 1.84, with 18 in 100 hidden thresholds at the cap under
  competitive learning: about half of the runaway gone, not a hold.
- The late scores of the long lives say the same as the quick map. Reach with competitive
  learning: 0.985 at cap 64, 0.881 at 128, 0.831 at 256, 0.767 at 1000. MNIST online under
  the one setting: 0.708, 0.734, 0.736, 0.657.

## What is real, what is suspect

Real:
- The chain's pass with the cap lifted is the motor cells' and is not credit: the cap split
  by kind, thresholds held as born, three and four buttons, the critic beside it, seeds 5-12
  and 1-4, three columns.
- MNIST online under the base rule is the motor cells' cap and nothing else (4 seeds, every
  seed within 0.02 of its arm's mean).
- The sector's target alone does nothing at cap 1000 (the walk has its own); an honest
  target of 64 holds where the tile's winners allow it.
- The map at cap 64, 128 and 1000 on seeds 5-12 (cap 64 and 1000 from the caches, the same
  lives the rate-control job's tables came from; cap 128 new).
- The probe is a faithful blobrun for these lives (22 rows, no difference in any window).

Suspect:
- Part 2 and the map's m64 and t64 columns are 4 seeds (5-8). The differences leaned on are
  large beside the seed spread (the ranges are in `out/capdefault/table-rate.md`), but a
  0.02 difference there is not a result.
- "Reach with competitive learning needs the cap itself" rests on two arms: cap 64 with the
  sector bias given room (rate 37, not 32) and the honest target of 64. Why pinned winners
  reach further was not looked at.
- The caps by kind of cell exist only in the probe (a loop after each tick). It is exactly
  a cap for cells the walk raises one step at a time, and was checked with every kind at 64
  against the real cap of 64; a kind-by-kind check against an engine with such a number
  cannot be made, because there is none.
- "Motor thresholds held as born" is not "no cap": the cells never adapt at all.
- "The hidden cells fire much alike on the two stages" is a cosine of spike counts over
  2,000 steps, not a reader.
- The one setting's lean to A on the first press (+0.19 ±0.09 over 12 seeds with three
  buttons) is not explained.
- The long lives at 128 and 256 are two seeds.
- The inside test's cap-64 and MNIST cap-1000 runs are the earlier jobs' files, copied where
  the flags are the same; one seed of each copied group was run again with this build
  (below the table).
- The answer weighs scores against health. Someone who puts the held rate first would pick
  128 or 1000 from the same tables; they are all here.

## Not done

- A cap for motor cells in the genome, or motor cells out of the sector's count (the core
  owner's files). The map with the motor cells at 64 and the rest at 1000 was run through
  the probe for a2rucomp and best, 4 seeds; the base column was not run.
- The map with a target of 64 for a2rucomp and best (dropped after the base column lost 8
  cues and exclusive-or); the map with the motor cells capped apart on seeds 9-12.
- The whole map on seeds 1-4 at cap 1000 (only the chain, cartpole and noisy rows).
- The inside test with the motor cells capped apart, and its shuffled and coin controls at
  cap 1000 for the pair test (born, competitive learning and the one-hop rule only).
- Why full sharing on the motor cells costs the output (the walk against the output rule's
  drive, thresholds of 100 to 400 on the motor cells): seen, not taken apart.
- Why competitive learning's reach wants pinned winners; the one setting's lean to A.
- The scoreboard and the suite at any cap but 64. Standard 40,000-step lives.
- Four buttons for the critic; the chain row's coin with more than two buttons.

## Script lines

```
scripts/capdefault.sh smoke --quick                  every part, tiny, seed 5 (ran end to end)
scripts/capdefault.sh chain                          part 1: the chain looked inside
scripts/capdefault.sh first                          the first press step by step, three buttons
scripts/capdefault.sh probecheck                     the probe's life against blobrun's, 22 rows
scripts/capdefault.sh rate                           part 2: MNIST online, far3, far5
scripts/capdefault.sh rate --arms c128,c256,holdmotor,m64t64    arms added after the first table
scripts/capdefault.sh map --seeds 5-8 --fixes t64,c128 --cols base        part 3, four seeds
scripts/capdefault.sh map --seeds 5-8 --fixes c128,m64 --cols a2rucomp,best
scripts/capdefault.sh map                            cap 64, 128, 1000 on seeds 5-12; seeds 1-4 rows
scripts/capdefault.sh inside                         part 4: the inside test at cap 64, 128, 1000
scripts/capdefault.sh speed                          part 5: blobbench, the caps taking turns
scripts/capdefault.sh alive --arms c256,c128         long lives (ran on stevejobs)
scripts/capdefault.sh table                          every table from what has run
bash out/capdefault/refcheck.sh                      the four reference lives against out/merge-ref
bash out/capdefault/refcap.sh 1000                   which of them would change at that cap (or 128)
python3 out/capdefault/tables.py; python3 out/capdefault/tables_map.py    this doc's tables
```

On stevejobs: `MINI_DIR=thonk-capdefault scripts/mini.sh bg cdalive
'THONK_MNIST=$HOME/thonk-hop2/data/mnist scripts/capdefault.sh alive --arms c256,c128 --jobs
2'`; the reach lives of part 2 and half of the cap-128 lives of seeds 9-12 ran there too.
Nothing is left running there.

Raw outputs: `out/capdefault/look/` (the probe's 932 runs), `trace/`, `inside/`,
`alive/`, `table-PART.md`, `doc/` (this doc's pieces; `python3 out/capdefault/assemble.py`
builds it). New quick-map lives: `out/oneset/cache.jsonl` of this worktree (blobrun's, run
here), `out/oneset/cache-mini2.jsonl` (blobrun's, run on stevejobs) and
`out/oneset/cache-capsplit.jsonl` (the probe's, for the column with a cap by kind of cell).
The main checkout's caches were copied beside them (`cache-main.jsonl` is its
`cache.jsonl`).

## Predictions, scored

| prediction | outcome |
|---|---|
| 1. the cap lifted on motor cells alone passes the chain, 0.60 or more (0.7); on the rest alone it does not, 0.40 or less (0.7) | right, both: 0.706 and 0.325 (base) |
| 2. motor thresholds held as born: 0.40 or less (0.65) | right: 0.000 in every column |
| 3. three buttons: A's share of the first presses that are not B under 0.65 (0.6) | right: 0.48 (base), 0.37 (a2rucomp), 0.58 (best); the critic 0.97 to 1.00 |
| 4. no stage line at cap 1000: 0.60 or more (0.35) | no: 0.13 or less in every column |
| 5. seeds 1-4: base 0.65 to 0.78 (0.8); the one setting 0.60 or more (0.6) | right: 0.718 (0.64 to 0.81 by seed), 0.994 |
| 6. cartpole and noisy lines pass on neither seed set (0.9) | right: seeds 1-4 cartpole 22 / 15 / 17 steps a fall (a coin 21), noisy lines 0.29 / 0.32 / 0.37 (line 0.61) |
| 7. not real credit over a step (0.75) | right for the pass; the one setting has a small lean to A that is not explained |
| 8. a2rucomp's MNIST back within 0.02 at the runaway rate's target (0.35); all three columns (0.25) | a2rucomp yes (0.826 of 0.840 at a target of 64); all three no (base 0.181 of 0.214; best is above, 0.758) |
| 9. cap 64 with the rate held by the bias keeps a2rucomp's MNIST within 0.02 (0.5) | yes, 0.839; but the bias does not hold the rate there (57 per 1,024, taken from the hidden cells) |
| 10. motor cells capped alone give a2rucomp's MNIST back within 0.02 (0.45) | yes: 0.828 |
| 11. far3 under a2rucomp 0.85 or more at an honest target of 64 or less (0.5) | no: 0.771. Only 128, which the tile's winners cannot reach |
| 12. the map at cap 1000 plus the fix: at least 7 / 13 / 16 rows with the four must rows held (0.3) | no fix was found. The nearest, a cap of 128, on seeds 5-12: 7 / 13 / 15 with the four must rows held (lowest: exclusive-or under base, -0.017); one row short under the one setting. A target of 64 costs the base column exclusive-or 0.16 and 16 cues 0.07 |
| 13. inside test at cap 1000, pair test: competitive learning 0.698, the one-hop rule 0.688, born 0.668 | close, all a little low: competitive learning 0.7036, the one-hop rule 0.6955, born 0.6703 |
| 14. blobbench within 3 in 100 (0.9) | right: the medians are within 1.5 in 100 either way, inside the rounds' spread |
| 15. I end up saying 1000 should be the default (0.6) | wrong: keep 64 |

What I had wrong going in: I took the cap for a fault with two costs to be paid back, and
looked for the payment. It is a setting of how hard per-cell sharing may push, and the two
"costs" are the two places where the sheet's learning wants a cell over its share. I also
expected the first press to go to "the other button" by sharing; it is plainer than that, a
coin once the paid button stops winning.

## If the default did change: what would have to be remade

Nothing here changed the default; `bash out/capdefault/refcheck.sh` says all four reference
lives are byte-identical. For whoever does change `threshold_cap` in `src/blob/genome.hpp`
(to 1000, or to a cap between), in one step:

1. **`out/merge-ref`**: `ref-echo.csv`, `ref-echo-sum.csv`, `ref-xor.csv` and
   `ref-xor-sum.csv` change, at 1000 and at 128 alike (`bash out/capdefault/refcap.sh 1000`
   and `128` run the four lives with that cap and compare: both full-engine lives differ
   within 4,000 steps, the plain echo life 0.543 to 0.432 late at 1000). `ref-min` and
   `ref-tab` do not: the minimal engine has its own cap (255, `src/minimal/minimal.hpp`) and
   the table engine has none. Remake the two full-engine lives with the lines of any
   `refcheck.sh`.
2. **Other readers of the number**: `src/blob/learn_lose_shift.cpp` clips the motor cells'
   shifted thresholds at `threshold_cap` (so lose-shift lives change too);
   `scripts/ladder2_model.py` has `THRESHOLD_CAP = 64` written in (the offline model of the
   sheet; the ladder jobs' models would no longer match the engine); `src/blob/birth.cpp`
   clips the number at 30,000 (thresholds are 16-bit; 1000 fits).
3. **Flags that become the default and the reverse**: every script that adds
   `--g.threshold_cap 1000` for a "rate held" arm (`compete2.py` CAP, `timerule_lib.py` CAP,
   `loops.py`, `loops_probe.py`, `hop2.py` targetcap, `ratecontrol.py` e) would then be
   adding nothing, and their "as it fires" arms would need `--g.threshold_cap 64` to mean
   what their docs say.
4. **Headline numbers measured at cap 64 with the full engine** (all of them; the ones the
   docs lead with): in `docs/findings.md` the quick map's rows passed (base 6, a2rucomp 13,
   the one setting 15, and every row score quoted from it: MNIST online 0.838 and 0.714,
   reach 0.925 / 0.841, the chain 0.15); the inside test's born sheet 0.924 / 0.673,
   competitive learning 0.952 / 0.726, the one-hop rule 0.9355 / 0.691, the teacher's wiring
   0.966 / 0.785 and the written-in wirings of the ladder jobs (all read under the engine's
   regulation); the scoreboard's full-engine columns (`scripts/scoreboard.sh`,
   `scripts/suite.sh`, blob and blob-readout); the MNIST results doc's online and test-set
   numbers; `docs/results-2026-11-26-one-setting.md` and `-one-setting-2.md` (both tables);
   the time-rule, loops and repeatability docs' cap-64 columns. The quick map's three columns
   at cap 1000 and at cap 128 are in the caches (seeds 5-12; the table in part 3), and the
   inside test's born, competitive-learning and one-hop rows at all three caps are in part
   4; the scoreboard, the other map columns and the inside test's other arms are not.
5. **`docs/design.md`** does not name the cap at all. Whichever value stands, the regulation
   section should say it and what it is for (part 2: the room above a cell's share).

## What should change elsewhere (for the coordinator; not edited here)

- `src/blob/genome.hpp`: `threshold_cap` stays 64 (my answer). If it moves it should move to
  128, with the list above (128 changes the two full-engine reference lives too). A number
  worth having either way: a cap for motor cells apart from the rest
  (`threshold_cap_motor`, picked by the motor mask in `regulate_thresholds`), so that "the
  output's cells may run over their share" and "the inside holds its rate" can be set
  apart. The probe's arms are that number.
- `scripts/variants/oneset.txt`, the chain row: more buttons, or a score of the first press.
  With two buttons a coin on the first press beside a learned second press scores 0.72 and
  passes (line 0.65). "Anything but the paid button" on the first press scores at most 2/k
  with k buttons: 1.0, 0.67, 0.50. At `--k 4` that is under the halfway line (about 0.55;
  the coin is about 0.10 by count, not calibrated here), and the columns here score 0.35 to
  0.50 there. The critic still scores 0.96 with three buttons (four not run). As it stands
  the row does not test what its comment says ("paid one step late").
- `docs/findings.md`, replacing the rate-control entry's last sentence ("The chain passes
  under the one setting with it lifted ... they had leaned on the runaway rate"):
  **the chain's pass with the cap lifted is not credit over a step.** The paid button's
  motor cells sit near the cap of 64 and out-vote the others on the unpaid stage, where no
  verdict corrects it; lifted, the walk levels the buttons there and the first press is a
  coin among the buttons that are not the paid one (three buttons: A 0.37, the third 0.40;
  the critic 0.95 and 0.00; 8 to 12 seeds). It is the motor cells' cap (lifted on them
  alone: 0.71; on the rest alone: 0.33), not the rate (32 per 1,024 at both caps). [real]
- `docs/findings.md`, new: **the cap of 64 is the room over a cell's share, and two things
  use it.** The output's motor cells: MNIST online's whole gain under the base rule comes
  back with only they capped (0.219 of 0.214), and they are where the MNIST world's "3.1
  times the target" is (210 per 1,024 against the hidden cells' 34). Competitive learning's
  winners: reach keeps 0.926 with the rate held at 37 and the cap on, and gets 0.771 at an
  honest 59 with the cap off. [likely: 4 seeds, the probe's caps by kind]
- `docs/findings.md`, new: **the target rate is two numbers.** `sector_target_per_1024`
  moves only the sector bias; the per-cell walk settles each cell at up chance over down
  chance (32 per 1,024). Raising the sector's number alone does nothing at cap 1000. An
  honest target of 64 (`--g.sector_target_per_1024 64 --g.threshold_down_shift 8`) gives
  back MNIST online under a2rucomp and the one setting and costs the base rule 8 cues 0.22
  and exclusive-or 0.16. [real for the mechanism; likely for the scores, 4 seeds]
- `docs/findings.md`, the one setting on MNIST: at cap 64 the motor cells run at 105 per
  1,024, the shared sector bias sits at its limit, and the hidden cells are starved to 13
  with 19 in 100 silent. Any of cap 128, cap 256 or a target of 64 lifts its MNIST online
  0.02 to 0.04. Motor cells should not be counted in the sector's rate. [likely, 4 seeds;
  cap 128 on 8]
- `docs/findings.md`, new: **the cap is a dial between two costs.** From 64 to 128 to 256
  to 1000: MNIST online under a2rucomp 0.840, 0.846, 0.826, 0.740; reach at 3 tiles under
  a2rucomp 0.921, 0.845, 0.754, 0.721; the MNIST world's rate in long lives 1.4 to 3.1
  times the target, 1.2 to 1.8, 1.2 to 1.3, 1.0. On the whole map 128 passes 7 / 13 / 15
  rows (64: 6 / 13 / 15; 1000: 7 / 12 / 16; seeds 5-12). [likely]
- `docs/findings.md`, the inside test's line for competitive learning: with the rate held
  (cap 1000) it reads 0.935 / 0.704 at 31 spikes per 1,000 cell-ticks, not 0.952 / 0.726
  (read at 64 to 66); the one-hop rule reads 0.935 / 0.696 there, so the two are level on
  MNIST and 0.008 ±0.004 apart on the pair test (8 seeds). [real]
- `docs/plan.md`: item 2m done with this doc. Queue: (a) the chain row with four buttons
  or a first-press score, and the map's chain numbers read again; (b) a cap for motor cells
  apart, or motor cells out of the sector's count (core owner), then the map with the
  inside at 1000 and the motor cells at 64 on all three columns (only a2rucomp and best, 4
  seeds, here); (c) why reach with competitive learning wants its winners pinned (far5 0.80
  against 0.60): a sharing rule for competitive learning that is weaker than the walk; (d)
  the one setting's lean to A on the chain's first press (0.54 against 0.25 with three
  buttons): what sends it.
- `docs/ideas.md`: 180 done (above). New: a cap by kind of cell; motor cells out of the
  sector's count; the chain with four buttons as the map's row; a weaker conscience for
  competitive learning (reach).
- `docs/predictions.md`: the predictions above, scored.
- `docs/notes-world.md` (or wherever probes are listed): `blobprobe capsplit`.

## Files that may collide with other branches

- Shared, one line each: `src/probe/main.cpp` (a declaration and a table line for
  `capsplit`).
- New: `src/probe/cap_split.cpp`, `scripts/capdefault.sh`, `scripts/capdefault.py`,
  `scripts/capdefault_more.py`.
- Not touched: everything in `src/blob/`, `scripts/oneset.py`, `scripts/ratecontrol*.py`,
  `scripts/inside.py`, `scripts/variants/` (imported or read only).
