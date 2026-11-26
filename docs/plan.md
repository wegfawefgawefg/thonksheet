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

Running (four), as of 2026-11-26:
- Queued item 9b (idea 188): the closed loop on every row; good-or-bad only; parity of 3.
- Queued item 2n (idea 184): motor cells regulated apart; a chain row a coin cannot pass.
- Queued item 11c (idea 185): a harder real-data test with more room.
- Queued item 2j (idea 174): do the inner rules learn more from a steadier sheet.
- Queued item 2m (idea 180): the threshold cap lifted, decide the default.
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
- A2. *(done: docs/results-2026-11-26-one-setting-2.md; with no hand-built cells the
  runner-up rule and competitive learning pass 13 of 22, best on exclusive-or, parity of
  3, gap and copy; the runner-up rule goes into the one setting (MNIST 0.714 to 0.841,
  same 15 rows); competitive learning does not add to it. Left: competitive learning in
  the one setting at four winners, hold cells alone beside it, an inside read for these
  columns, why the chain dies under competitive learning)* The one setting with what
  merged after it, on the quick map.
- A. *(done, merged: docs/results-2026-11-26-one-setting.md, `scripts/oneset.sh best`;
  one setting passes 15 of 22 worlds on seeds 5 to 12, the output rule alone 6, the GRU
  16; the critic does not combine; 12 in 100 neurons end silent after a long life. Left:
  the silent 12, a 40,000-step pass, MNIST test-set accuracy for the setting, rich2 and
  the slow potential in combination, the chain without the critic)* One setting with everything that worked: the readout, one step of reverse replay,
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
2n. *(running, idea 184)* Motor cells regulated apart from hidden cells; the chain row
   replaced by one a coin cannot pass. Behind it: idea 176 (short delays first), idea
   181 (a loop lit at twice its share).
2m. *(done, merged: docs/results-2026-11-26-cap-default.md, `scripts/capdefault.sh`; keep
   64; the chain pass is a coin first press; motor cells starve hidden cells under the
   one setting)*
2k. *(done, merged: docs/results-2026-11-26-rate-control.md, `scripts/ratecontrol.sh`;
   nothing passes more than 2 of 5 tests; the rate control is part of the learning
   rule; the cap of 64 is a fault and lifting it passes the chain)*
2i. *(done, merged: docs/results-2026-11-26-loops.md, `scripts/loops.sh`; the loop is a
   fade set by the leak; a slower leak with the cap lifted holds three to four steps and
   costs the rows that need the old item gone; a hand-made latch works and is replaced by
   the next cue; no learned route. Left: rule off at cap 1000 on every row, the
   runner-up's controls, why side 2 with the standard lattice holds)* What ends the
   learned loop after two steps, and the smallest change that lets it hold.
2f. *(done, merged: docs/results-2026-11-25-timerule.md, `scripts/timerule.sh`; the
   delay decides: +0.33 at 2 steps, +0.02 at 4; the rule builds a loop per cue from
   co-activity that lasts about two steps and is kept when frozen; 10 of 16 memory rows
   move. Left: `blobprobe timewire` on gap, order and 1-back; a shuffled life with the
   readout learning in it; the rest of the suite with the rule on)* Why competitive
   learning lifts the time rows and how far it goes; reconcile with the prediction
   battery, where it did not.
2a. *(done, merged: docs/results-2026-11-25-compete2.md; MNIST margin over shuffled at
   matched rate +0.003; not churn; delayed echo 0.59 to 0.91 and gap 0.54 to 0.94 with
   the rule on; cheaper read-off, 1.17 times at target rate. Left: the whole suite with
   the rule on, `scripts/suite.sh compete-on ...`, line in that doc)* Follow-ups to item 2, in this order, as one job after 2b and 2c are placed: a
   rate-matched control (half the margin is not yet separated from a doubled firing
   rate); a cheap read-off (the rule costs 1.9 times per tick); the rule in the time suite
   (half of each cell's inputs become pixels: does delayed echo or order pay?); a
   shuffled control for the calculation rows.
2c. *(done, merged: docs/results-2026-11-25-output-gap.md; the gap is the rule's good
   half; the runner-up rule lifts MNIST online to 0.852 at 120,000 digits with no drift.
   Left, as item 9: the runner-up rule on the critic's actor, on the calculation
   battery's many outputs, on lives past 120,000, the exclusive-or cost, the sign, and
   making the output use the lived sheet's lead)* Why our output cannot read what the inside learned (idea 161): a
   linear reader gets 0.95 from the lived sheet, the sheet's own readout 0.72 and falling.
   Takes in item 9 (the weighted vote, the drift). This gap is now the largest number on
   the board.
9b. *(running, idea 188)* The closed loop on the whole quick map as one setting, an
   output rule for good-or-bad-only verdicts, why parity of 3 fell, one counter store.
   Behind it: idea 187 (the predictor cells given the same output, so the prediction
   loop closes inside the blob with no labels), then the closed loop on the delayed
   rows with marks that last (idea 177's time result).
   Also queued: idea 189 (the error carried in timing: bursts, the change in a cell's
   own firing, marks cashed by a late strong signal), on the model bench.
9a. *(done, merged: docs/results-2026-11-26-output-inside.md, `scripts/outputinside.sh`;
   the loop closed inside the blob: 0.956 to 0.961 / 0.759 to 0.776, its own answer
   0.925 to 0.936 right online; one line up, one line down)*
3c. *(done, merged: docs/results-2026-11-26-predict-every-event.md,
   `scripts/predevent.sh`; on delayed echo the cells learn the future from an outside
   predictor's errors; the sheet's own predictor cells are too poor; ideas 186 and 187
   queued behind 9a)*
11c. *(running, idea 185)* A harder real-data test with more room than MNIST and the
   pair test, so rules separate on 2 or 3 seeds.
2l. *(done, merged: docs/results-2026-11-26-every-event.md, `scripts/everyevent.sh`;
   live 0.951 / 0.770 with an outside output's error; the sheet's own output is the
   missing piece)*
2g. *(done, merged: docs/results-2026-11-25-rule-ladder.md, `scripts/ladder3.sh`; the
   counter step's small chance loses the features, the rest of the way down from
   backprop is nearly free; every-event wiring in the real sheet 0.961 / 0.761)* The rule's ladder: in idea 165's model at its bottom rung,
   swap the teacher one step at a time toward the engine's one-hop rule (fewer digits,
   one at a time, random fixed feedback wires, a few wires a cell, sign only, good or
   bad only, counter steps on marked inputs, the sheet's own output). The rung where the
   score falls is what our rule is missing.
2j. *(running, idea 174)* The inner rules on a steadier sheet: longer showings during the
   life, regulation slowed or by sector only, counter moves on a digit's whole count;
   scored by lived less shuffled at matched rate, read the standard way and the quiet way.
2h. *(done, merged: docs/results-2026-11-26-repeatable.md, `scripts/repeat.sh`; the cause
   is regulation moving during the read and one spike or none a digit; regulation frozen
   and 80 ticks a digit read born 0.954 / 0.752 and the teacher's wiring 0.975 / 0.826; a
   lower target rate and wider cells do not pay; the long step breaks order and copy as a
   life's setting. Left: a spiking model for the last 0.004 to 0.018, the dynamics arms on
   8 seeds, a teacher trained at a lower rate)* Why the sheet does not answer the same
   digit the same way twice, and the two cheap levers idea 165 left.
2e. *(done, merged: docs/results-2026-11-25-inside-ladder.md, `scripts/ladder2.sh`; the
   cells are not the limit: the teacher's wiring inside every constraint reads 0.966 on
   MNIST and 0.785 on the pair test in the real sheet, born 0.924 and 0.673, competitive
   learning 0.952 and 0.726. Left: tile winners as a rung, a spiking model to split the
   model-to-sheet step, a contrastive label-free objective, the cell-by-cell match on
   more seeds)* The inside's ladder: a backprop-trained net given our cells'
   constraints one rung at a time (fan-in 34, top-counter wiring, one-bit strengths, fixed
   signs, nearby candidates, 3 in 100 firing), on MNIST and the pair test; then that
   wiring loaded into the real sheet. Backprop as a measuring stick only. It decides
   whether the shortfall on real data is the rule or the cell.
2d. *(done, merged: docs/results-2026-11-25-hop-real.md; not MNIST being easy; errors
   from the answer on every step are the best lever, +0.006 over the coin; depth helps a
   little; the pair test `blobmnist --pair 1` is the new real-data yardstick, born 0.677,
   two-layer net 0.80 to 0.85)* What stops the one-hop error on real data: cell width, a
   second set of cells one hop from the error, a real-data task a straight line fails,
   parity of 4, shuffled-input controls on the toys.
2b. *(done, merged: docs/results-2026-11-25-hop.md; teaches the inside products on toys
   and held-out calculation cases far above its coin control; on MNIST the coin control
   gets nearly all the gain)* The error sent one hop, to every hidden cell (was item 5; owner,
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
4b. *(done, merged: docs/results-2026-11-26-label-free.md, `scripts/ladder4.sh`; "is
   this real" 0.952 / 0.738, level with competitive learning on MNIST and +0.011 on
   pairs; the objectives are the limit; left as idea 179, queued behind 2l)* Label-free rule families tried in idea 165's model at its
   bottom rung before any is built in bits: competitive learning as calibration, the
   local "is this real" score (item 4), predicting a nearby pixel, two looks agreeing,
   pushing apart; the best written into the real sheet.
3b. *(done, merged: docs/results-2026-11-25-predhop.md, `scripts/predhop.sh`; the hidden
   cells learn what just changed (paddle move 0.52 to 0.74, coin 0.67, shuffled 0.65),
   not more of the future than rewiring alone; predictors and task unmoved. Left:
   horizons past 1, a read on the life's own moves in place of coin moves, reads at
   matched rate, a world where the answer is shown as a line so the calculation battery
   can be tried)* Each input line's prediction error as an error line of the
   one-hop rule: every hidden cell taught by what the sheet failed to predict, no reward.
3. *(first version done, merged: `scripts/predict.sh`,
   docs/results-2026-11-25-predict.md; the cells predict where there is something to
   predict, far under a fitted reader; nothing in it could change the inside; item 3b is
   the real test)* Predicting the input: every unit learns to predict a neighbour or the next tick from
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
6. *(done, merged: docs/results-2026-11-26-credit-rules.md, `scripts/credit.sh`; payment
   along the read wires beats random wires; the rest do not)* The credit rules not yet tried, on idea 167's model bench: two phases
   (91), credit by code (138), payment passed backward (139), a target for each cell,
   and a mark that lasts several steps for credit over time. Behind 2g, whose bench
   and whose answer (which step of our rule loses the features) it needs.
8. A memory that knows near from far (idea 152), scored on MNIST.
9. The output, what idea 161 left: the runner-up rule (`readout_good 2`) on the whole
   suite and as the one default (it costs exclusive-or 0.09), on the critic's actor, on
   the calculation battery, past 120,000 digits; the sign of a connection; scaling each
   source by its own spread so the output can use the lived sheet's lead.
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
