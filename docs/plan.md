# Plan

The work queue, in order. Status is updated as things land. Detail for the current effort is
in its own dated doc.

## Where we stand and the one question (2026-11-25, late)

The owner: "are we shooting in the dark. are we actually making any progress? that shifted
memory thing sounds more like a hack than a genuine general learning mechanic."

Honest position. What works here so far is known science under new names: a fixed random
sheet with a trained output is reservoir computing with a perceptron on top; the memory
engine is a lookup table (tabular learning), and falls to 0.20 on MNIST. A plain GRU
beats every sheet on every row. Nothing we have learns *inside*, and nothing generalises
to cases it was not shown. What two days did buy is the ability to tell: a bench, a
backprop reference, a lookup-table reference, real data, held-out cases, a search loop
and a GPU path. Until today a result could not be told from an easy test.

So the work narrows to one question, with one test:

**Can local rules make the inside learn features?** The test: after a life, freeze the
machine, give it a fresh output, and compare with a fresh output on (a) the machine as
born and (b) a plain random projection of the input. If the lived machine reads better
than both, on cases it was not shown, the inside learned something. On MNIST the numbers
to beat are 0.924 (sheet as born) and 0.943 (random projection); on the batteries, the
held-out columns.

Order of work:
1. Finish what is running; record it; cut what did nothing.
2. Every new idea is scored by that test first. Mechanisms that only tune the fixed sheet
   or the output wait.
3. Start from rules already known to learn features without backprop and put them in
   bits: feedback from the chosen action (running), competitive learning with local
   inhibition, local "is this real" goodness per layer, prediction of the input, random
   fixed feedback of the error. Then the stranger ones (credit by code, payment).
4. The memory stays as a reference and as a possible organ, not as the answer.

## The queue (2026-11-25)

The owner: "how many bots do you have going can you cut to like 4. a big queue of tasks is
better." So: four agents at a time, each a short scoped job from the top of this list;
when one hands back, its result is merged and recorded and the next item starts. Items
are ordered by the one question above. An item is struck when done, with where the result
is.

Running (four), as of 2026-11-25 afternoon:
- Queued item 3 (idea 162): every line is a target.
- Queued item 2c (idea 161): why our output cannot read what the inside learned.
- Item A below: one setting with everything that worked (`scripts/oneset.sh`).
- Queued item 2b (idea 159): the error sent one hop to every hidden cell.
Finished since: the vvm route (converges; idea 135) and tags with the margin gate (ideas
153, 154), both merged; their slots went to items B and C. The calculation battery
(ideas 133, 134; docs/results-2026-11-25-calculation.md), merged; its slot went to
queued items 1 and 2. Those are done and merged too (`scripts/inside.sh`; competitive
learning lifts the lived sheet to 0.952 on MNIST against 0.926 as born, idea 158,
docs/results-2026-11-25-compete.md); their slot went to item 2b.
Also running unattended, not an agent: the readout suite on stevejobs (`thonk-ref`, log
`out/readout.log` there; pull with `MINI_DIR=thonk-ref scripts/mini.sh pull`). The memory,
GRU and plain-net suites on the integrated code are done and on the scoreboard. The search
is stopped on both machines (resumable).

Done since the queue was written: the GPU (any world, evidence counters, the memory
engine, one call per table of designs); the integration of all eleven wound-up branches
and the GPU branch into main (it builds; four reference lives byte-identical; cartpole
now uses the fixed step by default, `--cart-integrator euler` for old numbers). Raw
outputs of every finished agent are under out/from-agents/.

Wound up, merged and recorded: the combined
core, reverse replay, nudges and balance, more worlds, the invariance battery, the
function battery, the dynamics family, the rest of the map, rule tables, long memory, the
critic. The unattended search is stopped on this PC and still runs on stevejobs.

Added after the wind-up, ahead of the rest (these are the first rules that move a wall,
so they come first):
- A. *(running)* One setting with everything that worked: the readout, one step of reverse replay,
  tiring neurons (or time and hold cells), winners in the tile, the critic. Does it hold
  together on the whole map, and does the replay runaway stop under it? Scored with the
  references beside it.
- B. *(done, merged: docs/results-2026-11-25-inner-rule.md; the merge ties the tags and
  is not better than its parents; first clear transfer to related tasks; MNIST 0.938 but
  coin rewards match it. Left over, low in the queue as item 22: pieces table on report
  seeds, 160,000-step lives, one `--life N --life-reward 0|1|coin` flag in place of
  `--live` and `--life`)* One inner rule from tags and reverse replay; then the
  inside-learning test on MNIST and held-out rows. (Was: reverse replay made safe: zero-sum steps per neuron or a threshold that rises with
  gain; the unsigned half separated from the signed half; the inside-learning test on
  MNIST (0.924 and 0.943 to beat) and on held-out rows.)
- C. *(done, merged: docs/results-2026-11-25-critic.md; above its twin on 15 of 17 worlds; cartpole stops at 110 steps per fall)* The critic on report seeds and long lives, on the fixed cartpole, with the wide
  critic.
- D. *(GRU, plain net and memory done; readout running on stevejobs)* Integration check:
  after the merges, re-run the suite for the readout, the GRU and the memory engine, so
  the scoreboard is on the fixed cartpole and has the new rows. What the reference table
  says about the tests is in `docs/findings.md`, "The references on every row".

Queued, in order:
1. *(done: `scripts/inside.sh`)* The inside-learning test as one script: after a life, freeze, wipe the output, fit a
   fresh output, and compare with the machine as born and a random projection, on MNIST
   and on the held-out columns. Every item below is scored with it.
2. *(done, passes: 0.952 against 0.926 born and 0.939 shuffled, 8 seeds; idea 158)* Competitive learning with local inhibition, no reward: units that win on an input
   move toward it (in bits: gain the input's bits, lose others, under a budget).
2a. Follow-ups to item 2, in this order, as one job after 2b and 2c are placed: a
   rate-matched control (half the margin is not yet separated from a doubled firing
   rate); a cheap read-off (the rule costs 1.9 times per tick); the rule in the time suite
   (half of each cell's inputs become pixels: does delayed echo or order pay?); a
   shuffled control for the calculation rows.
2c. *(running)* Why our output cannot read what the inside learned (idea 161): a
   linear reader gets 0.95 from the lived sheet, the sheet's own readout 0.72 and falling.
   Takes in item 9 (the weighted vote, the drift). This gap is now the largest number on
   the board.
2b. *(running, idea 159)* The error sent one hop, to every hidden cell (was item 5; owner,
   2026-11-25: "do we need some sort of hyper truncated bit backprop... maybe backprop
   isnt the issue but the whole requirement that it goes all the way back"). Each hidden
   cell has a few fixed random wires from the output's error lines (which button should
   have been higher, which lower); each of its connections keeps a fading mark of "I just
   helped this cell fire"; when the error arrives, marked connections move a hidden
   counter up or down by the sign on that cell's wire. No chain backwards, no second
   pass, no stored history. Tags are the one-bit-reward version of this that only reaches
   cells with a spike path into the action; this reaches every cell and carries a signed
   error per output. Run it both ways: told only good or bad, and told the right button.
   Scored by the inside-learning test, MNIST 0.924 to beat.
3. *(running, as idea 162: every line is a target; `scripts/predict.sh`,
   docs/results-2026-11-25-predict.md)* Predicting the input: every unit learns to predict a neighbour or the next tick from
   the rest; what is mispredicted is what gets learned. Widened by the owner, 2026-11-25
   ("in all of our envs we should extract rich signal ... predict current state, next
   state, future states"): every input line is a target at 1, 2, 4, 8 steps ahead, in
   every world, with the sheet's own last press fed back in; scored by prediction skill
   against "same as last step", by a fresh reader of things not on the lines at one
   instant (which way the ball moves, time since the cue), and by whether the task score
   rises with it on. Its errors join idea 159's error lines when that merges. Moved up (owner, 2026-11-25: "do
   we need pretraining step lol. its not like llms start off with general features"):
   there is no stage before the life. Every inner rule is always on, in the one life, and
   "a life, then freeze and read" is only how we measure. A language model's features
   come from predicting its input all the time, which is this item.
4. A local "is this real" score per region: real input against shuffled or self-made
   input, each region raising its activity for one and lowering it for the other.
6. Credit by code (idea 138).
7. Units pay the units that set them up (idea 139).
8. A memory that knows near from far (idea 152), scored on MNIST.
9. The output: a weighted vote (147) and the drift on long lives (146).
10. *(done as item C)* Finish the critic. What is left of it, as a later job: more
    information per press (a critic level per button), a cheaper step end (it costs 2 to
    4 times the readout alone), exploration for goal-only worlds, and the viewer's critic
    check (`src/view/sim_task.cpp:100` must only use the critic on the full engine).
11. The suite a lookup table cannot pass (149) and the held-out split (126): fold every
    battery's never-shown rows into the scoreboard as their own columns. The calculation
    battery is the first such suite (memory: seen 0.88 to 0.99, held out near the most
    common answer); what is left is the scoreboard columns.
11b. Tidy: `src/world/task.cpp` is 541 lines, over the 500-line rule; split it.
12. The seeing battery (136): shapes, tracking, counting, behind a cover.
13. The behaviour battery (145).
14. Two regions, one tuned to ring and one to mix (112), on delayed echo and hold.
15. A bank of predictors at history lengths 2, 4, 8, ... (137).
16. Copy, vary, keep a frozen copy; never erase (140, 141).
17. Look one step ahead (144), on the acting worlds.
18. Position tolerance learned from motion (idea 104) on the invariance battery.
19. A red-team agent: beat the best design by any means that keeps the rules (127). It
    also takes the rows no reference learns (analogy, how-many-hard, the shown-the-answer
    rows, fill-in, the pendulum, the sine band): for each, a learner that passes it or
    the reason none can, and the row fixed or struck.
20. Loop audit: cut mechanisms and suite rows that have earned nothing; one page.
21. Long memory, whatever its wind-up leaves unfinished: the forgetting curve to a
    million steps, the freezer.
22. Inner-rule leftovers (idea 155): `JOBS=3 scripts/inner.sh pieces`; `SEEDS=5-8 JOBS=2
    scripts/inner.sh long`; the random-neuron control on the transfer rows; fold
    `blobmnist ceiling --live` into `--life N --life-reward 0|1|coin`.

## Earlier: first blob (docs/effort-2026-11-24-first-blob.md)

1. Core: birth, tick, regulation, learning, speed test. *done*
2. Viewer: the sheet, raster, sector bars, histograms, neuron picker. *done, shown on a fake blob; not yet driving a task*
3. World plug, the stage 1 tasks, the headless runner with baselines. *done*
4. Put the three together. Stage 0: does it stay alive, with and without input.
5. **The gate: one hand-set blob visibly learns Echo.** *Holds on 3 seeds of 4 (0.84 to
   0.95 against about 0.5 with learning off), only with sensing and acting a tile apart.
   Numbers in the effort doc.*

## Next

6. Stage 1 in full, including shuffled echo and the timing tasks.
7. The idle check: teach, unplug for a long stretch, plug back in.
8. Quiet phases (sleep): downscaling and replay.
9. Stage 2 tasks, then stage 3 (catch, cartpole, snake first).
10. Tick speed: hand-written AVX-512 path, then many blobs across cores.

## Agreed 2026-11-24: freeze, run, cut, and rivals

Nothing has run yet and the design has 21 mechanisms, which is the too-many-variables
problem. So: no new mechanisms until something has learned; run Echo; then switch each
mechanism off in turn and delete what makes no difference. The elegant core to aim for is
three things: a neuron is a popcount over a mask against a threshold; a synapse is a tiny
counter moved by chance on an outcome; something keeps activity alive.

Rivals, all on the same tasks through the same plug. The brain-style rule has to beat them
to earn its complexity:

1. **Frozen blob, learned readout.** No learning inside; only the motor neurons' input masks
   learn. If the full blob cannot beat this, the inner learning is not helping.
2. **Random flips.** Flip a few mask bits, live a while, keep the flips if reward went up,
   undo them if not.
3. **Noise is the experiment.** The blob knows which spikes were noise. Only those neurons
   get graded: reward above its running average strengthens their recently active inputs,
   below weakens them. This is the owner's original snake method at the synapse.
4. **Table neurons.** A different substrate, small and separate: a neuron reads 6 input bits
   and looks up a 64-entry table of tiny counters. One lookup per tick, any function of its
   inputs, and it is the cell FPGAs are made of. Learning nudges the entry that was just used.

*Ablations and rivals 1-3 run (2026-11-24): `docs/results-2026-11-24-ablations-and-rivals.md`.
Table neurons not built.*

## The rule train (agreed 2026-11-24)

The sheet, the tasks, the viewer and the sweep stay fixed. The learning rule is the part
that changes: one rule, one file, picked by `learn_mode`, run up the same ladder on the same
seeds, one table. Each is marked known or new so we know when we are off the map.

Two separate problems, tested separately:
- **Carrying:** does a cue reach across N tiles at all? Measure how far a cue can be read
  from activity, at birth and after a life with no reward. Needs no reward rule.
- **Credit:** given the cue arrives, does good/bad wire it to the right button without
  saturating?

Rules, in the order to try them:
1. Noise is the experiment (known rule; exact noise bit is ours). Credit. *run 2026-11-24: loses to good/bad, adds nothing beside it*
2. A budget per neuron: every strengthening is paid for by a weakening elsewhere on the same
   neuron (known idea). Credit, aimed at saturation.
3. Causal credit: only a source that fired exactly its slot's delay before the target fired
   gets graded (known family). Credit, sharper.
4. Every neuron predicts: each neuron learns to guess its own next firing from its delayed
   inputs, no reward; reward only acts at the motor cells (known family). Carrying.
5. Selection among tiles: copy the masks of tiles that were active before good outcomes over
   tiles that did poorly (new, as far as we know). Both.
6. Learned delays: move a slot's delay by one tick, by chance, on an outcome (rare). Carrying.
7. Table neurons, a different neuron altogether (old idea, new on this sheet). Both.
Baselines kept alongside: frozen blob with learned readout, random flips.

## Asked for, queued behind the gate

- **Layers inside a tile.** Split the 64 bit positions into input, middle and output groups;
  birth and growth rules depend on the sender's and receiver's group. A constant mask, no
  cost per tick. Strength is a genome number, zero meaning today's flat tile, so it can be
  measured. Owner: "layers would be good i think, would force feature locality. adds
  sparseness for sure. reduces propagation bandwidth i think."
- **Scale.** 4,096 neurons is far too few for anything past the first reflex tasks; it is
  the size for checking the learning rule fast. Next: the tick across all cores (it is a
  stencil), then sheets of 100,000 to 1,000,000 neurons, then the GPU.
- **Throughput numbers in `blobbench`.** Print, per size: neuron updates per second,
  possible-synapse checks per second (704 per neuron per tick), real synaptic events per
  second (a spike arriving over an existing connection), signal bits per tick (entropy of
  the spike map), and stored bits (connections chosen plus their strengths). These are the
  numbers to set beside a float network's multiply-adds and weights. Owner: "the total
  throughput of the machine i think can be determined no? and matched to different neural
  networks?"
- **Fewer slots per neuron.** A neuron scans 704 possible sources to use at most 96. Let
  each neuron listen to a drawn subset of its 9 local tiles (say 4) plus the far slots:
  about half the work and half the memory per neuron, and more varied listening areas. A
  genome number; measure learning and speed against the full 9.
- **Fine locality inside a tile**, as a birth and growth preference by cell distance, also a
  genome number.

## Later

11. Blobs past 16,000 neurons; check the cache behaviour holds.
12. Genome search over shuffled tasks, scored late minus early.
13. Tick in a Vulkan compute shader, once the search needs thousands of blobs. Measure
    against the CPU first.
14. Sound-shaped input, larger sensor sheets, walking.
15. FPGA sketch of one tile, if the GPU version shows the stencil shape holds up.

## Open questions

- Does one good/bad broadcast plus prediction give purposeful play at all? Unknown. The
  gate above is how we find out.
- Do delays matter? Every slot has one now; compare against a genome with all delays at 1.
- What should idle do beyond staying safe: replay what, downscale how fast?
