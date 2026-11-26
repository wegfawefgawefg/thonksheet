# The threshold cap lifted: decide the default (idea 180, 2026-11-26)

Queue item 2m of `docs/plan.md`, following `docs/results-2026-11-26-rate-control.md`. Every
cell's threshold walks up on a spike and down slowly, capped at `threshold_cap` (64). The
rate-control job found that at 64 long lives run away and at 1000 they hold, that the
two-step chain passes with the cap lifted, and that MNIST online and reach with competitive
learning lose. This job asks why, and whether 1000 becomes the default.

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
