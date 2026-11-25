# Findings

What we think we know, and how well. This is the product: principles, with the evidence
behind each. A finding moves up a grade only when a second, different experiment agrees.

Grades: **solid** (two or more independent results), **likely** (one clean result),
**hint** (one result, few seeds or one engine).

## Where it stands (2026-11-25, night, after eleven jobs were wound up)

Each line is one job's result; most are on few seeds or unfinished, and are graded below.

- **The inside can be taught, one step back.** Sending the output's error back along the
  spikes that just happened (reverse replay, one step) lifts exclusive-or 0.88 to 0.985,
  reach to five tiles 0.59 to 0.88, eight cues 0.84 to 0.92, and a fresh output on the
  frozen result reads better than on the sheet as born (0.997 against 0.874). It is the
  first rule here that passes the inside-learning test. It is task-specific, half of the
  gain comes from unsigned rewiring, and over long lives it feeds itself and runs hot.
- **Time yields to a slow variable in the neuron.** A threshold that jumps on firing and
  fades over about a hundred ticks takes delayed echo 0.55 to 0.91, order to 0.88, gap to
  0.68. Cells that hold without leaking, beside cells that mark time since an input came
  on, take delayed echo, gap and order to 0.98 or better and hold a cue 5,000 ticks.
- **Acting yields to a critic, up to a point.** A second group of cells that learns how
  well things are going, and teaches the buttons by its surprise. One setting, 17 worlds,
  seeds 5 to 12 (docs/results-2026-11-25-critic.md): above its learning-off twin on 15,
  above the readout alone on 15. Bandits 1.00, switching bandit 0.98, the two-step chain
  0.99 (GRU 0.01, memory 0.00), track 0.88, forage 0.84, arrival-only track and forage
  0.78 (credit one step back, reached), mountain car 0.97, catch 0.39 (GRU 0.72).
  **Cartpole does not balance:** 109 steps per fall at 40,000 steps and still about 110 at
  400,000 (coin 21, memory 307, GRU 895). The sheet can represent balancing: told the
  right press its buttons reach 865 to 1000. The limit is how little the critic's
  surprise says about each press. It costs 2 to 4 times the readout alone per tick.
- **Cartpole was broken.** Its reward judged the press before the one it followed, so no
  rule that credits the current press could learn it. Fixed (semi-implicit step). Every
  earlier cartpole number, the references' included, is on the faulty world.
- **Nothing generalises, and that includes the references.** On the invariance battery no
  engine carries a shape to a new place, turn, tempo or pitch; the MLP and GRU learn every
  taught version and fail the same held-out rows. The old place world was a trap.
- **The sheet carries pairs, not triples.** And, or, majority of 8 lines: learned. Parity
  of 2: yes. Parity of 3: only on the richest sheet. Parity of 4: no sheet.
- **Keeping is a capacity limit.** The output holds 8 to 14 pairs and has lost any pair by
  10,000 steps; freezing strong counters does not help.

**Two more, the morning after (2026-11-25, late).**

- **The inside learns, a second way, and this one is stable.** Tags from the chosen action
  (idea 153): exclusive-or 0.996, eight cues 0.95; a fresh output on the lived sheet reads
  0.998 against 0.855 as born; the sheet stays at its target rate through 160,000 steps.
  Two independent rules now pass the inside-learning test, and they are the same idea:
  one step of signed credit from the action's cells into hidden counters under a budget.
  Neither transfers to an unrelated task, and neither helps MNIST by its credit (the
  reward-free rule below does).
- **Merging the two gives the tags again, and the first clear transfer** (idea 155,
  docs/results-2026-11-25-inner-rule.md, `scripts/inner.sh`). The tags with the walk's
  spike-exact credit tie the tags on 11 rows (exclusive-or 0.991, eight cues 0.952) and
  take none of the walk's reach (echo at 5 tiles 0.757 against the walk's 0.876); on the
  tuning seeds the plain tags scored a little higher. What is new is in the tests around
  it, seeds 5 to 12, fresh output on the frozen sheet:
  - Related tasks transfer: after a life of exclusive-or, parity of 3 reads 0.856
    against 0.552 as born (better than after a parity-of-3 life, 0.693); after eight
    cues, other pairings of the same cues read 0.950 against 0.796. A new line the life
    never showed gains little (0.760 against 0.712).
  - A random-neuron control (the same amount of rewiring given to random cells that
    fired) shows where credit matters: it is the whole gain on eight cues and on echo at
    5 tiles (control 0.809 and 0.579, no gain), and only a quarter of it on exclusive-or
    (control 0.961 of 0.993).
  - On MNIST after 20,000 rewarded digits a linear reader gets 0.938 against 0.926 as
    born, but coin rewards give 0.936 and random neurons 0.939. So the gain is rewiring
    of busy cells at fixed counts, not credit: the same thing competitive learning does
    on purpose and better (0.952).
  - A life that alternates two tasks every 1,000 steps keeps both (exclusive-or 0.994,
    eight cues 0.883) and lifts a third it never saw (parity of 3, 0.726).
  Not run: the piece-by-piece table on report seeds, 160,000-step lives, a second hop.
- **The output's error, sent one hop to every hidden cell, teaches the inside a
  product** (idea 159, the owner's "hyper truncated bit backprop";
  docs/results-2026-11-25-hop.md, `scripts/hop.sh`, `src/blob/learn_hop.cpp`). The error
  per button line (-1, 0, +1) reaches each hidden cell through fixed random signed wires
  that are a hash and cost no memory; the cell moves hidden counters of the sources that
  were on while it was near its bar, at its born counts. Nothing goes backwards and
  nothing is stored per connection. Fresh output on the frozen lived sheet:
  - Toys, seeds 5 to 12: exclusive-or 0.998 (born 0.855, coin control 0.930); parity of 3
    0.816 (born 0.552, coin 0.587), 0.897 when only cells that came near and did not fire
    are marked. Told only good or bad: 0.983 and 0.667. Every seed above its coin. Parity
    of 4: no.
  - Calculation, cases never shown, seeds 5 and 6: xor4 0.633 to 0.871 (coin 0.667), add4
    0.565 to 0.660 (coin 0.554). These are products of lines that sit apart, which the
    born sheet lacks and no earlier rule reached.
  - MNIST, seeds 5 to 12: lived 0.9355 against born 0.9251, but the coin control (the same
    errors on random lines) gets 0.9346. The errors add 0.001 told the answer, 0.003 told
    good or bad, 0.003 with the rate held at target. With competitive learning on, nothing
    is added (0.9530 against 0.9519). The sheet's own online readout is not helped.
  - Cost: 7 in 100 on the tick, 2 to 5 in 100 on a taught MNIST life. The firing rate
    does not move.
  A reading, not yet tested: a toy answer is a conjunction of a few nearby lines, which
  one cell with 34 inputs can become; a digit class is a sum over hundreds of pixels and
  a cell hears about 6, so the error asks it to be something it cannot be. And a linear
  reader of the born sheet is already at 0.925, so MNIST leaves little for products of
  nearby pixels to add. The calculation battery's held-out columns are the sharper test
  of whether the inside learned.
- **The output's gap is its good half, and it is mostly closed** (idea 161,
  docs/results-2026-11-25-output-gap.md, `scripts/outgap.sh`). A ladder of readers on the
  same recorded sheet, from a float reader fit offline down to the engine's motor cells
  (born sheet, 20,000 digits, seeds 5 to 12): offline 0.926; online float 0.905; integer
  weights told the right button 0.864; the engine's good-and-bad rule on the same reader
  0.577; motor cells' own fields 0.60 to 0.69; the engine 0.692. One step holds nearly all
  of it: 0.28 lost where "on every good the pressed button gains and the nine others
  lose". A button that wins keeps winning, and once accuracy is over one half a seldom
  pressed button is pushed down more often than up, so 3 or 4 buttons die on every seed.
  Hearing only nearby cells costs nothing; one-bit connections cost nothing on that rule.
  - The fix, `--g.readout_good 2 --g.margin_percent 20 --g.margin_win 2`: on a good that
    was close, the pressed button gains and only the runner-up loses. MNIST online
    0.822, 0.845, 0.852 at 20,000, 60,000, 120,000 digits (plain 0.692, 0.673, 0.649; the
    gate alone 0.811, 0.799, 0.759). No drift and no quiet button on any seed, and it is
    cheaper per digit (1.2 ms against 3.2). With a weighted vote on top
    (`--g.readout_evidence_strong 2`): 0.837, 0.854, 0.863.
  - Toy rows, seeds 5 to 12: 8 cues 0.842 to 0.940, 16 cues 0.305 to 0.605 (0.723 with
    the weighted vote), order 0.695 to 0.779; exclusive-or falls 0.883 to 0.793, the
    gate's known cost (with two buttons the rule is the gate). So it is not yet one
    default for every row.
  - Still open: the sign (a one-bit excitatory connection can only add evidence; a
    reader held to weights of zero or more loses 0.17; opponent cells did not get it
    back); the engine reads 0.09 better than the agent's copy of it under the gate and
    nobody knows why; and the output still does not use the lived sheet's lead (0.845
    lived against 0.852 born), which survives float readers and is lost at the first
    integer one.
- **Every line as a target: the cells can predict, and the inside is not yet taught by
  it** (idea 162, the owner's "extract rich signal ... predict current state, next state,
  future states"; docs/results-2026-11-25-predict.md, `scripts/predict.sh`). Output cells
  for every input line at 1, 2, 4, 8 steps ahead, taught by the line's own future, always
  on beside the task; 10 worlds, seeds 5 to 12.
  - One step ahead they beat "same as now", "most common" and the learning-off twin on
    every seed in chain (0.875 against 0.286, 0.643, 0.536) and predict-next (0.767
    against 0.404, 0.634, 0.504); echo with random cues sits at baseline (0.749 against
    0.754), as it must.
  - A fitted reader of the same frozen sheet does much better (predict-next 0.914,
    cartpole 0.924 against the cells' 0.871), and on cartpole, pendulum and mountain car
    the cells are under "same as now". The output rule is the limit, not the information.
    A reader of the input lines alone does as well as a reader of the sheet: for a
    straight-line reader the future is in the lines.
  - Only motor cells learn under the working rule, so prediction alone cannot change the
    inside. With competitive learning and the errors fed back as lines, hidden cells
    read far more (which way the paddle moved +0.27, time since the cue +0.20), but a
    time-shuffled life does as well. The gain is what the error lines carry in, not
    something learned about time.
  - Competitive learning costs memory of time: time since the cue 0.859 from born hidden
    cells, 0.645 after a life with it; beat phase 0.497 to 0.286.
  - Error lines as input, nothing else changed and the rate unchanged: last press 0.604
    to 0.745, beat phase 0.500 to 0.614, time since the cue 0.873 to 0.919.
  - The task: delayed echo +0.04 to +0.05 on 7 or 8 seeds of 8, beat +0.03 with error
    lines; catch, forage, chain a little worse. Cost: 0.49 to 0.87 of the speed.
  The idea itself, errors teaching the inside, is untested until those errors go down
  idea 159's wires (idea 164).
- **What stops the one-hop error on real data: not MNIST being easy; the kind of error
  matters most** (idea 163, docs/results-2026-11-25-hop-real.md, `scripts/hop2.sh`).
  - A real-data test a straight line fails: two half-size digits interleaved, "same class
    or not" (`blobmnist --pair 1`). Linear on the bits 0.510, born sheet 0.677, random
    projection 0.705, two-layer net 0.796 to 0.852. The one-hop rule lifts the sheet to
    0.691 and its coin control to 0.691 (seeds 5 to 12, rates matched): nothing beyond
    churn. Competitive learning 0.726 at twice the rate; both rules 0.731, the pair with
    the hop coin 0.731. So the MNIST result was not an artefact of an easy yardstick.
  - Errors computed from the answer alone on every step (each line's target less its
    running rate, `--g.hop_teach 3`) are the largest lever found: MNIST 0.9377 against
    its coin 0.9317, 8 seeds of 8 (the old errors at the same slow step +0.0015); the
    pair test +0.008 over its coin on 4 of 4. With the rate held at target the MNIST
    margin is +0.003 (4 seeds). Thousandths, but the first error-driven margin on real
    data that every seed shows.
  - Depth helps the errors count, width does not. With half the cells hearing no pixels
    the old errors beat their coin on the pair test by +0.007 on 8 of 8, though that
    sheet starts and ends lower. Cells with four times the pixel inputs are pulled back
    off the pixels by the rule (30.6 to 10.7 a cell) and end under their born score.
  - A fault in the first hop job's toy test is fixed (the inner rules kept running while
    the fresh output learned). Frozen, the toy result stands and is a little stronger:
    parity of 3 0.847, coin 0.618, shuffled-input life 0.607; held-out xor4 0.871
    against 0.692 shuffled and 0.667 coin. Parity of 4 fails in every arm.
- **Competitive learning, checked: little of the MNIST gain is about digits, and it
  lifts the time rows a great deal** (queue item 2a, docs/results-2026-11-25-compete2.md,
  `scripts/compete2.sh`).
  - MNIST at matched rate: see the note under "The inside learns features from the input
    alone": about +0.003 over a shuffled-digit life.
  - Time rows, the rule on for the whole life beside the readout, seeds 5 to 12, no
    reward reaching the inside: delayed echo (2 steps) 0.587 to 0.913, gap 0.544 to
    0.938, order 0.664 to 0.728; with the rate held at target 0.874, 0.929, 0.809. Every
    seed is up on delayed echo and gap. Random rewiring is under rule-off on all three
    (0.537, 0.503, 0.606). The plain sheet with this rule matches the richest searched
    sheet on delayed echo and gap (0.89, 0.89). It is the first inner rule to move a time
    row. A reading, not tested: a winner's counters gain for sources that were on at the
    delay of the slot that reads them, so a cell comes to detect "this cue now and that
    one some ticks ago".
  - That result does not yet agree with the prediction battery (idea 162), where on
    another sheet shape and a 4-step delay the same rule left delayed echo unchanged
    (0.485 to 0.486) and a fresh reader of the frozen hidden cells read time since the
    cue worse (0.859 to 0.645). Finding 50 is therefore narrowed below, and the
    difference (shape, delay, rule on beside the readout or frozen) is idea 166.
  - Held-out calculation columns on 8 seeds: xor4 +0.11 to +0.15, add4 +0.06 to +0.09
    over born and over churn on every seed. The shuffled control cannot test these
    (their inputs are independent coin flips).
  - With the one-hop rule, both together are worse than the one-hop rule alone (held-out
    xor4 0.794 against 0.878): the two rules share counters and competitive learning
    outvotes the error; at a quarter of its moves the pair is back to 0.836.
  - Cost after a cheaper read-off with identical wiring (lives byte-identical): 1.17
    times the rule-off tick at target rate (was 1.32), 1.64 at the rule's own rate (was
    1.93; the doubled spike rate alone is 1.2 of that).
- **The cells are not the limit; the rule is** (idea 165, queue item 2e,
  docs/results-2026-11-25-inside-ladder.md, `scripts/ladder2.sh`). Backprop used as a
  measuring stick only: an offline model of the sheet's 3,072 non-sensor cells, trained
  with the answers, takes on the cells' constraints one rung at a time; then the bottom
  rung's wiring is written into the real born sheet and read by the inside-learning
  test's reader. Seeds 5 to 12, test digits, MNIST / the pair test (same class or not):
  - In the real sheet the teacher's wiring reads **0.966 / 0.785**, at the engine's own
    rate (32 to 35 spikes per 1,000 cell-ticks), every seed above its born sheet (0.924 /
    0.673). It keeps every constraint: about 33 inputs a cell from its own
    neighbourhood, strengths 1 and 4 at the born counts, each sender's fixed sign, the
    engine's regulation. Competitive learning, the best local rule, reads 0.952 / 0.726
    at twice the rate. So a perfect teacher adds +0.042 / +0.113 where our best rule
    adds +0.026 / +0.049. I re-ran seed 5 from the merged build: 0.9666 and 0.7857, the
    same as reported.
  - In the model the whole ladder costs MNIST nothing (0.981 free, 0.980 at the bottom).
    The pair test falls 0.933 to 0.860: counts in place of floats 0.009, the fan-in
    0.024, fixed signs 0.012, holding every cell at the engine's rate 0.029; one-bit or
    1-and-4 strengths, cells hearing cells and reach cost nothing (3 seeds a rung).
  - The largest step is from the model to the real sheet: 0.013 on MNIST, 0.075 on
    pairs, and the born wiring loses 0.037 and 0.095 the same way. Shown the same
    digits from two starting states, a real cell's spike counts correlate only 0.61 with
    themselves (seed 5): about two fifths of what a cell does on a digit is the sheet's
    state, not the digit. A quiet gap between digits and noise off each explain 0.004
    of it; the source is not found (idea 168).
  - Left free, the teacher keeps the code sparse (1 to 4 in 100 cell-digits with a
    spike, 38 to 48 in 100 cells silent on pairs); regulation holds every cell at 0.66
    spikes a digit and that is the model's dearest rung.
  - The same bottom rung trained with no labels (give the pixels back through a linear
    decoder) reads 0.960 / 0.745: the born level. One label-free objective, not all.
  - What the target cells look like: 15 to 17 pixel inputs of 33 (born 7, competitive
    learning 17.8), spread 3.4 pixels from their middle (competitive learning 2.5, born
    4.0), 7.5 in 100 of the born inputs kept, a spike on 12 to 15 in 100 digits. The
    answer explains 13 in 100 of a cell's variance on MNIST (born 11.5) and 0.2 in 100
    on pairs: no cell is a class or a "same" detector; the reader builds the answer
    from many.
  - Suspect: the model's absolute pair numbers run 0.08 to 0.10 high (the transplant is
    the number to quote); the 0.02 to 0.03 rung steps rest on 3 seeds; the 12 passes of
    60,000 digits the teacher had are 36 times what our lives show a rule (idea 167).
- **The mechanisms hold together: one setting passes 15 of 22 worlds** (queue item A,
  docs/results-2026-11-26-one-setting.md, `scripts/oneset.sh best`,
  `scripts/variants/best.txt`). One setting, unchanged on a quick map of 22 worlds, seeds
  5 to 12; a row passes when it is at least halfway from chance to perfect and above its
  learning-off twin by 2 standard errors. The output rule alone passes 6, the GRU 16, the
  memory engine 16, the vm engine 14. The setting: the output rule with the close-vote
  gate, time and hold cells, tiring neurons, two winners per tile, one step of the
  backward walk on bad steps only. I re-ran three rows on two seeds from the merged
  build: far5 0.933, xor 0.910, delayed echo 1.000, as reported.
  - Against the GRU: 16 cues 0.955 (0.99), reach to 5 tiles 0.933 (1.00), exclusive-or
    0.936 (0.99), order 0.994, gap 0.897, delayed echo 1.000, a 1,000-tick hold 0.998
    (GRU 0.50), bandit 0.995, catch 0.671 (0.72), copy of 2 0.897 (1.00), MNIST online
    0.714 (0.73).
  - Still failing: cartpole (17 steps per fall, a coin 21, GRU 703), the chain (0.15;
    only the critic learns it), parity of 3 (0.64), noisy lines (0.31), keeping pairs
    (0.53), held-out scale and place (nothing passes those).
  - Each piece alone: time and hold cells are by far the largest (6 rows to 12), then
    rich2 (10), tiring neurons (9), the walk, tags, the slow potential and the close-vote
    gate (8 each). The critic alone still passes 6: it is the only thing that learns the
    chain (0.96) and it costs exclusive-or 0.27 and reach 0.19.
  - The critic does not combine. With it on, the world's good and bad never reach the
    walk or the hold cells; with glue that lets the verdict through, its surprise is
    noise on steps where nothing is at stake (exclusive-or -0.3, order -0.4).
  - What this is and is not: most of the 9 rows gained come from cell kinds built by
    hand (time and hold cells, tiring) and from the output rule. The inside-learning
    read under this setting is small: a fresh output on the lived sheet 0.933 against
    0.891 as born at 5 tiles (+0.042 ±0.012), 0.941 against 0.915 on exclusive-or
    (borderline), same task, no transfer or random-projection arm.
  - Health after 160,000 steps: no runaway (0.89 of the target rate), but 12 in 100
    neurons end silent (the base: none), undiagnosed; the setting runs at half the
    base's speed.
  - Made before the runner-up output rule, the one-hop rule, competitive learning and
    prediction lines were merged; lives are 20,000 steps, half the standard.
- **Why competitive learning lifts the time rows: it builds loops that keep a cue lit
  for about two steps** (idea 166, queue item 2f, docs/results-2026-11-25-timerule.md,
  `scripts/timerule.sh`). Seeds 5 to 12 unless said; I re-ran the reproduction table on
  two seeds from the merged build (delayed echo 0.61 to 0.97, gap 0.56 to 0.89, the
  prediction battery's row flat).
  - The two earlier results do not disagree. On the suite's sheet the gain on delayed
    echo is +0.33 at 2 steps, +0.07 at 3, +0.02 at 4, nothing at 8 or 16. The prediction
    battery asked at 4 steps. Its sheet also fails at 2 steps for a second reason:
    sensor groups of side 2 together with the sensor lattice over the top half, where
    the rule drives the rate from 29 to 50 and the gain goes (either change alone keeps
    two thirds).
  - The frozen inside carries all of it. Fresh output on the frozen lived sheet against
    born: delayed echo +0.36, gap +0.42, every seed. A life with the frames in shuffled
    order gives +0.33 and +0.42; a life with no reward +0.30 and +0.38; churn nothing.
    So it is learned from which cells are on together, not from order in time.
  - What the cells wired (4 seeds, delayed echo): the hidden cells that hold a cue take
    28 of their 49 excitatory inputs from other cells holding the same cue (born 3,
    churn 3). Slot delays stay about 2 ticks. These are cell assemblies. A fresh reader
    of the hidden cells reads the cue at the ask step at 0.998 against 0.722 born. The
    earlier guess (a cell learns "this now and that some ticks ago" through slot
    delays) is wrong.
  - How far: 10 of 16 memory and time rows move by 0.05 or more, all ones a trace of
    one or two steps answers: 1-back 0.726 to 0.978 (GRU 0.97), same or different in
    turn 0.53 to 0.83, copy of 2 0.69 to 0.89, gap 0.54 to 0.94, 2-back 0.50 to 0.61,
    recall past one 0.70 to 0.80. Beat falls (0.48 to 0.45, rate 58), sequence stays.
  - With the runner-up output rule most rows gain a little more (copy of up to 3 +0.11,
    delayed echo at 3 steps +0.10); 1-back loses 0.14 on every seed (that rule's
    two-button cost).
  - This is the first memory here that is learned inside by a local rule with no
    reward, and not built by hand as hold cells are. Open (idea 172): what makes the
    loop fade after two steps, and why the rate runs up on the other sheet.
- **Prediction errors sent to every hidden cell teach it what just changed, and not
  more of the future** (idea 164, queue item 3b, docs/results-2026-11-25-predhop.md,
  `scripts/predhop.sh`; the owner's "every line is a target", first real test). Each
  input line's one-step prediction error becomes an error line of the one-hop rule; no
  reward reaches the inside. 10 worlds, seeds 5 to 12, hidden cells read by a fresh
  reader after the life, firing rates within 2 of born. I re-ran catch and forage on
  two seeds from the merged build (paddle 0.752, coin 0.665, shuffled 0.595, born 0.526).
  - What changed is learned. Which way the paddle moved (catch): born 0.521, rule off
    0.549, prediction errors 0.742, coin control 0.672, time-shuffled life 0.649 (7 of 8
    seeds over each control). Forage's last move: 0.275, 0.324, coin 0.253, shuffled
    0.256 (8 of 8). A cell taught by "I said it stays and it moved" comes to carry the
    move.
  - The future is not. A fresh reader of the hidden cells for every line 1 and 8 steps
    on moves under 0.01 from born on 8 of 10 worlds; where it moves (predict-next 0.822
    to 0.870) the coin gets 0.861 and the shuffled life 0.864. Time since the cue 0.859
    to 0.956, coin 0.963. Those gains are the rewiring, not the errors (finding 47).
  - The loop does not close: the predictor cells' own skill rises no more than under
    the coin (+0.015 against +0.008), and the task score is within 0.03 of rule-off on 9
    of 10 worlds (chain -0.075).
  - Errors wired only to nearby cells are worse than random wires (paddle 0.620).
    Beside competitive learning the paddle read is best (0.842) and time since the cue
    falls (0.811).
  - Cost: 0.76 to 0.86 of the speed. Only horizon 1 is built.
  - Suspect: the read drives the world with coin presses, so on acting worlds the next
    lines depend on a press nothing can know, and "the future is in the lines" is
    partly the test's doing; the coin control spreads errors over all lines where real
    errors sit on the few that change; one setting, picked on seeds 1 and 2.
- **The owner's vvm converges once four things change** (idea 135): the input is written
  into the state, an operation leaves most bits alone, lookup is by nearness, and new
  inputs make their own detectors. It holds across 20 silent steps (1.00; the GRU 0.49),
  survives distractors on the cue lines (0.97; memory 0.63, GRU 0.50), does parity of 3
  and 4 lines (0.85, 0.75; GRU 0.68, 0.52), and answers never-rewarded patterns (0.78;
  memory 0.45). It has no sense of time, no credit one step back, and cannot keep several
  items in order.
- **Learning on good only when the vote was close** doubles the pairs held (16 cues 0.31
  to 0.60) and lifts MNIST's online readout 0.71 to 0.81, without the lock.

**The inside learns features from the input alone (2026-11-25, late;
`docs/results-2026-11-25-compete.md`, `scripts/inside.sh`).** Competitive learning with no
reward: a hidden cell that fires among its tile's winners moves hidden counters toward the
sources that were just on, and rewires its top counters at the counts it was born with.
After a life of 60,000 MNIST digits, frozen, a fresh linear reader gets 0.952 on the test
digits, on every one of seeds 5 to 12 (0.950 to 0.954). The sheet as born: 0.926. The
control, the same life on digits with their pixels shuffled afresh each time: 0.939. A
random projection: 0.943. A two-layer net by backprop, one pass: 0.955. It is the first
result here where the lived inside beats the born inside on real data, and it used no
reward and no error.
- What the cells learned: the pixels one cell listens to go from scattered (correlation
  between them 0.05, spread 4.1 pixels) to patches that light together (0.26, 2.5
  pixels). The shuffled life leaves them scattered.
- Held-out calculation cases move too (2 seeds, no shuffled control): xor of 4 bits 0.60
  to 0.67 becomes 0.73 to 0.80, add of 4 bits 0.52 to 0.61 becomes 0.61 to 0.69.
- The old co-activity wiring passes the same test, which had not been measured: 0.937
  (born 0.924, shuffled 0.926; 2 seeds). The budget and counters double its gain.
- **Caveat one, the rate.** The learned cells fire at twice their target (63 against 33
  spikes per 1,000 cell-ticks). Held at target, lived still beats born by 0.013, but
  beats the shuffled control by 0.012 without tile winners and only 0.002 with them (2
  seeds each). A higher rate alone does not help (the born sheet pushed to 63 reads
  0.910). So about half of the headline gain is structure and half is not yet separated
  from rate.
  *Settled on 8 seeds (docs/results-2026-11-25-compete2.md, `scripts/compete2.sh mnist`):
  with every sheet held at its target rate, lived 0.9351, shuffled 0.9322, born 0.9217,
  random rewiring ("churn") 0.9201. Lived less shuffled is +0.003 (5 of 8 seeds; +0.002
  without tile winners, 6 of 8). So what is about digit shapes is about 0.003; the rest of
  the 0.026 headline is the doubled rate (64 against 33) and a drift toward active inputs
  that shuffled digits give as well. It is not churn: moving the same counters toward
  random sources reads at or under born, and lived beats churn by 0.015 at matched rate on
  every seed.*
- **Caveat two, our own output cannot use it.** With the rule on from the first digit of
  the rewarded online life, the sheet's own readout scores the same as with it off (0.72
  at 20,000 digits, falling to 0.66 to 0.68 by 120,000). A linear reader gets 0.95 from
  the same cells. The gap between what the sheet holds and what our output rule reads is
  now the largest number on the board.
- Cost: 1.9 times slower per tick at the cheaper setting (a quarter of the updates, same
  score), because each update re-picks a cell's wiring from 704 counters.

**The references on every row (2026-11-25, `docs/scoreboard.md`).** The GRU, the plain
two-layer net and the memory engine now have the whole suite, eight seeds, on the
integrated code. What that table says about the tests, before it says anything about us:

- **Rows no reference learns, so they do not yet measure anything:** analogy (0.50, 0.33),
  how many of 1 to 5 with mixed sizes (0.35), being shown the answer with no reward (all
  nine rows at chance), fill-in, the pendulum (0.22 with the scripted player at 0.86), the
  band of a sine at an x never rewarded (0.36 against 0.24), and the held-out rows for
  place, turn, size and mixed. Either they need a kind of learner none of the three is, or
  they are badly made. Each needs a learner that passes it, or a note saying why none can.
- **The lookup table is a lookup table.** Memory matches or beats the GRU wherever the
  exact case comes back (prediction 0.85 against 0.50, catch 0.99 against 0.72, n-back,
  switching, parity of 4 lines 0.99 against 0.50). It is at chance the moment the case is
  only near one it saw: a dimmer shape 0.50 (GRU 0.96), more clutter 0.50 (0.89), items
  never taught 0.65 and 0.27 (0.87 and 0.48), reward true 60 in 100 0.73 (1.00).
- **Where recurrence is the whole difference:** every row with a gap (delayed echo, order,
  gap, same-then, copy, recall, n-back, brackets) the GRU learns and the plain net sits at
  0.50. Our sheet without time cells sits with the plain net.
- **The chain is ours.** GRU 0.01, memory 0.00, plain net 0.45, the readout alone 0.72;
  with the critic 0.97 on the tuning seeds. It is the one row where a rule of ours beats
  all three references, and it is the row that needs credit one step back.

**Calculation (2026-11-25, `docs/results-2026-11-25-calculation.md`).** Answers that are
bit patterns, 28 kinds at 56 sizes, a quarter of each one's cases never rewarded. With a
verdict per bit the working rule learns what is a weighted sum of its input lines (copy to
8 bits, not, compare, max, the carry-out of a sum up to 6 bits) and nothing that needs a
product of distant lines (sum bits, exclusive-or, parity of 4, the ALUs, programs of two
steps). Of 56: 7 learned with held-out cases too, 3 on seen cases only, 16 the sheet
carries and the rule does not reach, 30 the sheet does not carry. Where it learns, held
out stays within 0.05 to 0.16 of seen, so it is a rule and not a table. The memory engine
is the reverse: 0.88 to 0.99 on seen cases, the most common answer on held-out ones (add
0.11, exclusive-or 0.05). The GRU and plain net, held out, get only comparison. This
battery is the suite a lookup cannot pass.

## The headline (2026-11-25)

**The output can now learn online, from good and bad alone, things no input line carries.**
In a test with the inside of the sheet held fixed as born (a way to find where learning
happens, not a goal), a rule at the motor cells learns exclusive-or of two cues (0.88), which cue came first (0.70,
0.84 with long delays), Echo with the cue three tiles from the button (0.85), four cues
(1.00) and eight (0.84), and shuffled Echo with no locked lives (1.00 on eight seeds of
eight). The same sheet with the old readout scored chance on all of these. The owner's
thesis, that a rich enough sheet turns learning into selection, holds for everything a
plain linear reader can get out of the sheet. docs/results-2026-11-24-readout.md

**What this is not.** The goal is a brain that learns everywhere, all the time, on any task
(the owner, 2026-11-25: "i didnt say it needs to be never taught ... the whole point is it
should be able to learn new things right? run nonstop. online learning for any task"). The
readout does learn that way: it never stops, it picks up a second task without losing the
first. The inside does not yet. Held fixed, it can only offer what it was born with, which
is why a shape in a new place, a long gap and sixteen cues fail. Letting the old inner rule
run alongside helps reach and hurts the feature tasks. Making the inside learn usefully,
online, is the open problem and the next target.

The full suite on that rule (`blob-readout` in docs/scoreboard.md, 8 seeds, sheet frozen)
agrees and adds: a second task is learned after the first (0.99) with the first still there
(0.98 at once); eight cue-button pairs held, and sixteen at 0.31 where chance is 0.06;
nothing lost after being unplugged (1.00 before, 1.00 after); Echo five tiles away 0.59;
catch 0.32 where chance is 0.25. Still failing: delayed echo (0.55), beat, gap (0.52),
cartpole (below a coin), and a shape in a place it was never rewarded (0.25, below chance).

**Where we stand against ordinary networks (2026-11-25).** A plain recurrent network (a
GRU of 128 units trained by backprop through its last 8 steps), with one fixed setting,
behind the same plug and told only good and bad, learns every world any of our engines
learns, at the same or a higher score, and several that none of ours learn: sixteen cues
(1.00 against our 0.31), delayed echo, beat, order and gap (all 1.00), catch (0.72 against
0.32). Our only lead is speed on plain association (1,000 steps against 2,000 to 3,000).
So nothing here beats standard methods yet, and the bar for "learns it" on these worlds is
1.00, not "above chance". What the reference cannot do is as useful: a shape in a place
never rewarded (0.16, below chance, same as ours), a hold longer than its window (fails at
11 steps), and cartpole from good and bad alone (worse than a coin; shown the right press
it balances for 1,000 steps). Those three are not our private walls. They are where a
fixed ordinary network also stops, and where it would normally be redesigned per task.
docs/results-2026-11-25-fixed-net-reference.md

**A memory passes most of the suite (2026-11-25).** An engine that only stores and recalls
(the input cut into events; now plus the last four events as the address; hashed tables of
small counters; all integers, 187,000 ticks a second) passes, with one setting, every row
the GRU passes and two it does not: echo to 16 cues, delayed echo across 4,000 silent
ticks, beat, gap, order, exclusive-or, catch (0.99), cartpole (109 and 279 steps per fall
where the GRU and every sheet sit below a coin's 23). It learns a pair in about one error
and holds about 1,000. It is the first thing here to beat the backprop reference anywhere.
Two cautions. It is a lookup table of what it has seen, so those rows turn out to test
storing, not understanding; where the asked case was never shown (a shape in a new place:
0.57, a coin per layout) it has nothing. And reading a sheet's spikes instead of the raw
input made it worse on every row: mixing is not what a memory wants from a partner.
docs/results-2026-11-25-memory-engine.md
On MNIST, where nothing repeats exactly, the same engine gets 0.20 (0.32 at best; chance
0.10; the sheet's readout 0.71): its address is a hash, so nearly-the-same looks like
entirely-different.

**The rich sheet as one setting for the whole suite (2026-11-25).** The sheet tuned for
time features (rich2), unchanged across all rows: delayed echo 0.89 (default sheet 0.55,
the first sheet to learn it), order 0.86, exclusive-or 0.94, gap 0.89; and it pays for it
on plain association: echo 0.89 where the default gets 1.00, eight cues 0.69 against
0.84, reach 2.3 tiles against 3.4. One sheet cannot yet be best at both.
docs/scoreboard.md

**MNIST, first pass (2026-11-25).** The never-taught sheet, read offline by a linear
reader, gets 0.924 on the test digits: level with a linear classifier on the pixels (0.922),
under a random projection of the same bits (0.943) and a small two-layer network (0.955).
The sheet's own readout, learning online from good and bad, gets 0.71 in about a minute
(20,000 digits) and then drifts down. So for digits the sheet adds about a point to its
input, and the online readout loses twenty more. The readout is the gap.
docs/results-2026-11-25-mnist.md

## Principles that hold so far

1. **A neuron must pay for what it keeps.** [solid] A fixed budget of connections, where
   gaining one costs another, is the piece that carries learning in the minimal engine
   (Echo drops 0.17 to 0.23 without it and the sheet saturates). The full engine, which has
   no budget, ends with 87 to 95% of connections at top strength. In the table engine only
   the entry just used moves, which is the same discipline by another route, and it does not
   saturate either.
2. **Inhibition and each neuron keeping its own rate are what keep a sheet alive.** [solid]
   Removing either is the largest loss in the ablation table (-0.46, -0.32). The minimal
   engine stays on target for 4M ticks with only these two. Regulating by sector is not
   needed and is what causes the pulse.
3. **Wiring toward what fires together helps, and needs no reward.** [solid] It extends how
   far a cue can be read from 3 tiles to 4, and to 6 over a long life, with no reward at
   all; it is worth about 0.15 on Echo in the minimal engine and -0.34 when removed from
   the full one.
4. **But it must stop when nothing is plugged in.** [likely] Left running idle it rewires
   the sheet to noise and everything learned is lost (0.95 to 0.33). Gated on input being
   attached, nothing is lost (0.95 to 0.96).
5. **Memory that is specific to the situation holds more.** [likely] Table neurons, where a
   neuron stores a separate answer for each pattern of its inputs, learn four cues (0.68)
   where both popcount engines manage 0.31 to 0.41, and reach one tile further. The cost
   is no speed gain on a CPU. In the suite they are also the only engine that learns a
   second task after the first (two new cues and buttons: 0.96, against 0.13 and 0.11) and
   still has the first afterwards (0.61 at once, 0.90 after a short retest).
6. **The sheet makes its own variety.** [likely] Switching injected noise off changes
   nothing in the full engine. Grading only injected-noise spikes learns Echo in the minimal
   engine (0.90 when built correctly) but nothing in the table engine.
7. **Most of what we first built does nothing on reflex tasks.** [solid] Delays, the
   dendrite-branch bonus, varied neurons, far-link redraw and injected noise can each be
   removed with no measurable change, and a four-mechanism engine beats the
   twenty-one-mechanism one on Echo. Delays and far links cannot be judged yet: no task
   that needs them is learned by anything.

8. **The sheet is already rich; reading it is the hard part.** [solid: four seeds, tight] With nothing
   learned and no reward, order, exclusive-or and gap length can all be read from the
   neurons a tile or two from the input by an ideal reader (1.3 to 3.4, where 1.0 is easy).
   The same features score at chance when a frozen sheet's readout has to learn them. This
   is the owner's richness thesis, half confirmed: the features are there; selection is
   what we cannot yet do.
9. **Delays give a sense of time.** [solid: four seeds, tight] They do nothing measurable on Echo. On
   time features they are most of the story: gap length reads 0.43 with all delays at 1 and
   4.0 with delays of 4 to 8 ticks; order goes 1.5 to 5.4.

10. **Far links are what give reach.** [likely] The full engine learns a little with cue
    and button three tiles apart (0.58); with its far links removed it does not (0.45).
11. **More bits of evidence per thing learned helps.** [hint] Table neurons needed 4-bit
    counters to learn four cues (2-bit gave 0.56 on two). The popcount engines, with one or
    two bits per connection, hold less. Not yet tested head to head.

12. **One neuron's experiment almost never changes the answer.** [likely] In the table
    engine only 1 graded noise flip in 2,500 changed which button won. Those that did all
    moved their counters the right way, and grading only them reaches 1.00 on the seeds
    where a single flip can matter. So credit from single-neuron noise is correct but far
    too rare; an experiment has to be the size of a decision.
13. **A rule that grades against its own average must count everything it averaged over.**
    [solid] Grading only the neurons noise flipped is half of the textbook rule. The
    missing half (the unflipped neurons, the other way) is thousands of times larger than
    the real signal, so the rule as first built learned the exact opposite (0.00 on six
    seeds of eight; 1.00 with its sign reversed). This was a slip, not a principle.

14. **Evidence before commitment.** [solid for reach and for holding more: two engines] A connection that
    flips on one reward cannot add up a thin signal. Give each possible source a hidden
    counter (6 bits), move it a step on each reward, and wire the sources with the most
    evidence. On a frozen sheet this takes exclusive-or from 0.57 to 0.79 and Echo at three
    tiles from 0.57 to 0.74 over the same rule without counters, and it keeps improving at
    the end of a life where the plain rule has stopped.
    The second test, in the minimal engine with the whole sheet learning: Echo at three
    tiles 0.39 to 0.60, four cues 0.41 to 0.63, exclusive-or 0.26 to 0.58, eight cues above
    chance for the first time in that engine. Two limits. The credit feeding the counters
    has to be broad (every neuron that spiked; noise-only credit does nothing). And without
    the error rule of finding 15 it costs the plain reflex 0.2 (0.94 to 0.73). Width is not
    the lever (6 bits best, 4 close, 8 worse); summing evidence at all is.
15. **The output must know which action was taken.** [likely] The rule that works is an
    error rule: on bad, the cells of the button that was pressed lose their recently active
    inputs and the cells of the other buttons gain them; on good nothing is strengthened.
    A cell that only knows whether it fired itself gets roughly half the benefit. The
    world has to tell the motor cells which button won. That is one bit per cell, and it
    is the one part of the rule that is not strictly local.
16. **A decision needs many cells.** [solid: echo and MNIST] 32 or more motor cells per button, instead of
    9, adds 0.02 to 0.13. On MNIST 51 to 102 cells a button took 0.62 to 0.71. It is the count that matters, to beat spike noise, more than
    where the cells sit.
17. **Do not strengthen your own answer on good.** [likely] The rule above never does, and
    it has no locked lives: shuffled Echo is 1.00 on every seed, crossed ones included,
    where the old rule averaged 0.56. *The literature says this is too strong* (Baldassi
    and others 2007, binary synapses with hidden states like ours): never on a comfortable
    good, but on a good that was barely won, deepen the hidden evidence of what helped.
    Without that case learning time grows exponentially with size; with it, about twice as
    many patterns are held. Being tested (the tags-and-margin run).
    And with ten buttons it is simply wrong: on MNIST the readout stays near chance (0.14)
    unless the right button also gains on good (0.41 with it, same settings). Blaming the
    pressed button says little when nine others could have been right.

18. **Size does not break it, so far.** [hint] Echo scores the same with the sheet grown 16
    times around the same task (minimal engine 0.94, 0.93, 0.92 at 4,096, 16,384 and 65,536
    neurons). Three seeds, one task, input and output areas unchanged.

19. **Counters inside the sheet help it hold more.** [hint] In the minimal engine, counters
    on the motor cells alone leave four cues at 0.39; counters on every neuron lift it to
    0.63, every seed up. This is the first sign of a reward rule making neurons inside the
    sheet more useful. It costs memory (82 to 493 bytes per neuron) and reward-time work.

20. **Richness can be tuned, and ours was not.** [likely] Among 600 random sheets the best
    read order and gap three to four times better than the default and exclusive-or up to
    twice as well, on fresh seeds. No single number does it (best rank correlation 0.3):
    longer delays for time, denser local wiring, the dendrite-branch bonus for
    exclusive-or, more inhibitory neurons. Richer sheets do learn better through the
    readout, some of them by a lot: the gap task, which nothing had learned (0.52), reaches
    0.93 on one of them, and that same sheet gets order 0.85 and exclusive-or 0.94 with
    nothing changed between the three. The fast screen is a rough guide only: two sheets
    that read better in the probe learned worse in the task.
21. **A mechanism can only be judged on a task that needs it, through a readout that can
    read.** [solid] Delays, far links and the branch bonus all measured as doing nothing on
    Echo. Delays turned out to carry time, far links reach, and the branch bonus
    exclusive-or.

22. **Holding and mixing want different sheets.** [hint] A sheet tuned to keep ringing (few
    and gentle inhibitory neurons, dense local wiring, a high activity target) keeps a cue
    readable for 60 ticks, three times the default's limit, and tells cues apart a third
    as well while they are on. Nothing found by tuning holds 150 ticks. So one region to
    ring and one to mix, and a real mechanism for anything longer.

23. **Where inhibition reaches is a lever; tidy structure is not.** [hint: four seeds, sheet
    as born, ideal reader] Inhibition kept inside its own tile nearly triples how readable
    a gap is (1.60 to 4.62) and lifts exclusive-or (1.76 to 2.84), at a cost to telling
    cues apart and to order. Links that go both ways help a little everywhere. Layers
    inside a tile, finer locality, fewer neighbour slots, hubs and topographic far links
    make the sheet poorer or change nothing. A second sheet fed by the first, and a gentle
    flow in one direction, add reach only.
24. **Wiring laid down at birth does not last.** [hint: one seed] With learning on, growth
    and pruning took mirror wiring from 0.80 reciprocal back to 0.15 within 100,000 ticks.
    A wiring idea has to live in the growth rule to survive a life.

25. **A random sheet is not much of a feature maker for real input.** [hint] For digits the
    frozen sheet holds its input plus about a point; a plain random projection of the same
    bits adds three. Finding 8 ("already rich") is about time and combination features of
    a few lines; it does not carry to pictures. What raised the digit ceiling was layout
    (the digit spread over the whole sheet: 0.856 to 0.924), not any genome number, and
    the sheets tuned rich for order and gap read digits worse.
26. **Counters that fill up silence a cell.** [likely] With ten buttons more than a cell's
    budget of counters sat at the top value; the rewiring step then wired none of them and
    4 to 6 buttons went quiet for good. Fixed (`readout_evidence 2`). Sixteen-cue echo
    (0.31) may be the same fault and is being re-run.

27. **Storing by content removes four walls at once.** [likely: one engine] Holding (by
    events, not ticks: silence costs nothing), many pairs, learning in one go, and a second
    task without loss all come free when what is learned is filed under what was seen.
    What it costs: no answer for anything unseen, a fixed depth of past events (one
    distracting event is survived, two are not), and slots (about 1,000 pairs in 5 MB).
28. **Most of our worlds test remembering.** [solid: the memory engine passes them] A suite
    for "any task" needs, in every family, cases that were never shown.

29. **Credit one step back reaches the inside; further back does not.** [likely: one
    engine, 8 seeds, with a fresh-output check] See "Where it stands". The step back must
    use evidence counters on inner neurons (principle 14 again) and only walk from spikes.
    Open: the runaway on long lives; the half that unsigned rewiring explains.
    Transfer (idea 155): clear to related tasks on the same lines, none shown to
    unrelated ones. A random-neuron control says credit is the whole gain on reach and
    on eight cues and a quarter of it on exclusive-or. The walk's reach does not come
    from spike-exact credit: the tags given it do not gain it.
    Credit need not follow the wiring back at all (idea 159): a fixed random hop from
    the error to every cell teaches products of a few nearby lines, ties the tags on
    exclusive-or, and reaches cells with no spike path into the button.
30. **Holding needs a slow variable inside the neuron, not a ringing sheet.** [likely: two
    independent mechanisms, 8 seeds each for the first] Tiring neurons, a slow second
    potential, or non-leaking cells each make delayed echo learnable. The ringing sheet
    from finding 22 holds too but the output reads it worse. Each costs exclusive-or about
    0.1; a winner-in-the-tile rule lifts exclusive-or to 0.98 and may pay that back.
    Confirmed on 8 seeds in the one setting: tiring neurons with two winners per tile
    take exclusive-or 0.80 to 0.93. Time and hold cells are the largest single piece on
    the quick map (6 rows passed to 12) and also lift reach (5 tiles +0.34), because a
    hold cell keeps the cue's identity on for the whole trial.
31. **An actor needs a critic, and the critic's surprise must teach on good as well as
    bad.** [likely: 17 worlds, seeds 5 to 12; a slower actor helps (cartpole 89 to 107,
    chain 0.97 to 0.99); a wider critic costs (chain 0.84, cartpole 33); the close-vote
    gate that helps raw good does not carry over to a critic's good (bandit 0.84, chain
    0.79); an actor trace shorter than 4 steps costs] Finding 17 holds only when the world's own verdict is the
    teacher. A critic made of spiking cells needs an opponent half so common swings cancel.
    It does not combine (item A): with the critic on, the world's verdict reaches no
    other rule, and its surprise is noise where nothing is at stake. A critic that could
    join the rest would have to stay quiet unless its level moved more than its usual
    noise. The critic is parked (the owner, 2026-11-25).
32. **Wiring by co-activity is the inner rule that pays beside an error-rule output.**
    [likely: third result for principle 3] In the minimal engine it adds 0.05 to 0.25 on
    every row that learns and loses none; the noise-graded reward rule and inner counters
    there cost. With it, sixteen cues reach 0.53.
33. **The sheet's features are roughly pairwise.** [likely] Parity of three lines is
    carried only by the richest sheet tried and parity of four by none; size does not
    help, because new tiles grow away from the input.
34. **The output learns thin, noisy and context-dependent association, and nothing held two
    steps.** [hint] Rare cue 1.00, noisy lines 0.99, a context line 0.93, a rule that
    switches 0.94; copy of two and 1-back about 0.7; grammar, brackets, 2-back, analogy,
    counting and every relation on untaught items at chance. Being shown the answer
    teaches nothing. Silencing a fifth of the sheet costs the output nothing.
35. **A whole-button nudge unfreezes a rule stuck on wrong answers.** [likely, one engine]
    In the minimal engine; little or nothing in the others. Evening out good and bad does
    nothing.
36. **A learning rule can be written as a table of ten local facts.** [solid as a
    construction] The error rule, the old rule and growth all are, and score alike. No
    evolved table is confirmed yet.

37. **Credit inward must be a contrast between actions.** [likely] Feedback from the chosen
    button alone hurts; the chosen button's wiring minus the others' is what teaches.
38. **A neuron that learns inside must keep its counts.** [hint] Fixed numbers of
    excitatory, inhibitory and strong connections per neuron is what keeps tags from
    running the sheet hot, where reverse replay (which lets strength grow) runs away.
    Inhibitory neurons should not learn from these credits.
39. **Finding 17, restated: do not strengthen on good unconditionally.** [likely] Only when
    the vote was close. It holds for the inside too: "on good, the pressed line should
    have been higher" sent down the one-hop wires costs exclusive-or 0.98 to 0.92.
40. **A state that addresses its own next step holds, once an operation can leave bits
    alone and the input writes into the state.** [likely: one engine, 8 seeds] Matching by
    nearness and waiting for the input to settle is what lets it survive noise and answer
    unseen patterns, where a hashed lookup cannot.
41. **The frozen sheet is a linear-plus-pairs feature bank.** [likely: one engine, 56
    worlds] On the calculation battery a reader of the sheet as born equals a reader of
    the raw input lines within a few hundredths, except for exclusive-ors of lines that
    sit together (0.54 to 0.66, 0.72 to 0.91). With finding 33 this is what "rich" means
    for this sheet today: little beyond its input, and nothing at a distance. So carry
    stops at the first sum bit (an exclusive-or of lines N apart), and a program stops at
    two instructions though the sheet still holds the previous one at 0.96.
    This describes the sheet as born, not what its cells can hold: see finding 52.
42. **One verdict cannot teach many outputs with an error rule that moves only on bad.**
    [likely; predicted from the rule's drift balance before the runs] One good or bad for
    the whole answer equals a verdict per bit at one output bit, loses a little at two,
    and sits at the learning-off twin from three bits on (0.39 to 0.55 where per-bit
    grading reaches 0.94). Moving on good as well partly rescues 3 and 4 bits, not 6 or 8.
    A verdict per two-valued output is being shown the answer, bit by bit. A wide answer
    needs that, or a rule that splits one verdict among many outputs.
43. **A reward-free rule teaches the inside when it keeps its counts.** [likely: 8 seeds,
    one data set] Competition in the tile, hidden counters, and rewiring at the born
    counts: lived 0.952 against born 0.926 on MNIST. The same idea without a budget or
    counters (co-activity wiring) gets half the gain. Second result for finding 38.
    Third result (idea 155): a reward-driven rule lifts the MNIST reader by the same
    route whatever the reward says (0.938 real rewards, 0.936 coin, 0.939 random
    neurons). Rewiring busy cells at fixed counts is the active part.
44. **Every inside-learning claim needs a shuffled-input control and a rate-matched one.**
    [likely] A life on shuffled digits lifts the reader by 0.014 with nothing to learn
    (cells drift toward being a random projection of the input), and a rule that leaves
    cells firing faster changes what a reader sees. Only what is left after both is
    learned structure. For a reward-driven rule add two more: coin rewards, and the
    same rewiring given to random cells that fired (idea 155 built both: `tag_random`,
    `tag_shuffle`, `--live-coin`). For an error-driven rule, the same errors sent to random
    lines (`hop_coin`): on MNIST that control explains nearly all of the gain.
45. **An error-driven inner rule must move slowly when verdicts are many.** [hint: one
    rule, one world] At a step chance of 1 in 8, 20,000 MNIST verdicts replaced 90 in 100
    of the born inputs and the lived sheet fell under born; at 1 in 128 it is above.
46. **"Near the bar and did not fire" is the useful mark for an error-driven rule.**
    [hint] Parity of 3: 0.90 against 0.82 with the cells that fired included. In bits it
    is one word per tile per tick.
47. **Rewiring busy cells toward what was just active lifts a linear reader whatever
    signal gates it.** (Toward random sources it does not: churn reads at or under born,
    docs/results-2026-11-25-compete2.md.)
    [likely: three rules, 8 seeds each on MNIST] Tags with coin rewards 0.936, one-hop
    error on random lines 0.935, real signals 0.938 and 0.936, born 0.925. A teaching
    signal has shown its worth only where it beats that: the toys and the held-out
    calculation columns, not MNIST.
48. **With many buttons, a good should move only the competitors.** [likely: MNIST 8
    seeds and the toy rows] On a close good the pressed button gains and the runner-up
    loses; nothing else moves. Pushing every other button down on every good drains the
    seldom pressed ones, and moving on comfortable goods makes winners richer. This
    joins principles 17 and 39: 17 was right for 2 buttons and wrong for 10; 39 (only
    when the vote was close) is half of it; who pays is the other half. The long-life
    drift (ideas 146, 157) was this, not the counter clamp and not the budget.
49. **Surprise fed back in as input makes the recent past readable.** [likely: 8 seeds,
    several worlds, rate unchanged] "I said on and it was off" lines are change
    detectors; a sheet that hears them carries what just happened and how long ago.
    Sent to the hidden cells as a teacher instead (idea 164), the same errors make the
    cells carry what changed (which way the paddle moved 0.52 to 0.74, coin 0.67), and
    nothing more about the future than the rewiring alone gives. [likely: 8 seeds, coin
    and time-shuffled controls, two worlds of ten]
50. **Competitive learning makes loops that hold what was on, for about two steps.**
    [likely: 8 seeds, 16 rows, frozen, shuffled, reward-free and churn controls; wiring
    looked at on 4 seeds] Cells that fire together on a cue wire to each other (28 of 49
    inputs against 3 as born), and the loop keeps the cue readable one to two steps
    after it is gone: delayed echo at 2 steps 0.59 to 0.91, 1-back 0.73 to 0.98, gap
    0.54 to 0.94. Nothing at 4 steps or more, nothing for rhythm. It is learned from
    co-activity, not from time order (a shuffled life gives the same). (This replaces
    the first reading, "a rule that turns cells toward the input costs them the recent
    past", which was one sheet shape at a 4-step delay where the rule drove the rate to
    50 and the loops barely formed; there the trace does get shorter than born.)
51. **The one-hop error teaches a cell what a few of its inputs can jointly decide, and
    little else so far.** [likely: toys and calc 8 and 2 seeds with coin and shuffled
    controls; two real-data tests, 8 seeds] Products of 2 or 3 nearby lines: yes, far
    above every control. A digit class, "same class or not", parity of 4: no more than
    churn. Better errors (from the answer, every step) and cells one more step from the
    pixels each buy thousandths. What is missing is not the error reaching the cell.
52. **The cells can hold what a trained net holds; the teaching rule is what falls
    short.** [likely: one engine, two real-data tests, 8 seeds, re-run from main] A
    wiring inside every constraint of the engine reads 0.966 on MNIST and 0.785 on the
    pair test in the real sheet at the engine's own rate (born 0.924 and 0.673; the best
    local rule 0.952 and 0.726 at twice the rate). No change to the cell is needed to
    get there. The trained wiring is a concrete target for a rule; finding 47's
    "rewiring at born counts" has +0.014 and +0.06 of room above it.
53. **The sheet does not answer the same input the same way twice.** [hint: seed 5,
    two tests] From another starting state a cell's spike count on the same digit
    correlates 0.6 with itself. A noise-free model of the same wiring reads 0.04 (MNIST)
    to 0.08 to 0.10 (pairs) higher. This is paid by every reader and every rule that
    counts spikes, and may be part of why a decision needs many cells (finding 16).
54. **The best code is sparse, and holding every cell at one rate forbids it.** [hint:
    3 seeds, in the model] Left free the teacher silences two fifths of the cells and
    has 1 to 4 in 100 cell-digits active; one target rate for every cell costs 0.029 on
    pairs, more than any wiring constraint.

## What the literature adds (docs/mechanism-catalogue.md, docs/outside-catalogue.md)

- Finding 14's hidden counter is a known device (permanence in HTM; hidden states in binary
  synapses), and the best number of hidden states is small. Our 6 bits fits.
- Finding 15's "which action was taken" bit is what the attention-gated rules (AGREL,
  AuGMEnT, BrainProp) send back from the chosen action to tag the neurons that fed it. It
  is the most direct known route from our readout to the inside. Being tested.
- Finding 13 has a stronger form: reward must be centred per situation, which takes a
  critic, not one running average. An actor told only good and bad failed cartpole in 1983
  too; it took a critic. Being tested.
- Finding 12 stands (perturbing single neurons scales badly), with one open door:
  perturbing connections, or every unit at once by its own code (idea 138).
- Finding 1's budget has to act fast, and a trained readout wants a bigger budget than the
  sheet (fits finding 16).
- A warning for any holding region (Braitenberg): activity held while co-activity wiring
  runs ties the held units together and loses their order.

## Walls

*As of the night of 2026-11-25 the list below is partly out of date: "Time", "Reach",
"Acting in a world" and "inside learning" each have a first mechanism that moves them (see
"Where it stands"). The walls still fully standing are: carrying anything to a case never
shown (a new place, turn, tempo; held-out relations and numbers); features of three or
more things at once; holding two things in order; keeping more than about a dozen pairs;
real data past a linear reader. And one new one: a rule that teaches the inside
consistently is a positive loop in a recurrent sheet, and rate regulation does not hold
it.*


- **Reading a thin signal: broken at the output, standing inside.** An error rule moving
  evidence counters at the motor cells reads most of what a linear reader can (see the
  headline). It has not been tried on neurons inside the sheet. When the whole sheet learns
  alongside it, reach gets better (0.97 at three tiles) and the feature tasks get worse
  (exclusive-or 0.88 down to 0.63): the old inner rule fights the new readout.
- **Reach.** Three tiles is now learned (0.85 frozen, 0.97 with the sheet learning). Past
  about five tiles nothing arrives to be read.
- **Time.** Nothing learns delayed echo, now also with evidence counters and with delays
  on: answers rise only because it presses more, and among presses it is at chance. Order
  is learned by the new readout (0.70 to 0.84); a gap in time before the question is not.
- **Locking on.** A sheet settles on one answer and keeps giving it while punished: Echo
  seed 3 and crossed shuffled-echo lives. The cause is the reward rule strengthening its
  own answer: making that stronger (win-stay) deepens the lock (0.56 to 0.14), and making
  the last answer harder to give (lose-shift) only trades stuck lives for unstable ones.
  Crossed lives need a two-tile path, so this is the thin-signal wall again. Cartpole is
  not a lock: the blob leans to one side (0.58 of presses) without being stuck.
  Any rule graded against its own average also freezes once every reward equals the
  average: an all-wrong life stops moving.
- **A new place.** A shape learned in some places is answered wrongly in a place never
  rewarded (0.25 where chance is 0.49). The reference networks fail it the same way (0.16),
  so the wall is real and general, though the world itself needs a cell-by-cell check
  (everything scores below chance, which smells of overlapping shapes). On MNIST a
  digit moved 2 pixels drops every learner to about two thirds of its score (linear 0.91
  to 0.60, two-layer net 0.96 to 0.65, sheet ceiling 0.92 to 0.66). Nothing in the sheet shares what one patch learned
  with another. This is the missing prior the owner started from on day one (a convolution
  shares; this sheet does not).
- **Acting in a world (updated 2026-11-25, late).** With the critic: cartpole is learned
  to about 110 steps per fall and stops there, though the sheet's buttons reach 865 to
  1000 when told the right press, so the wall is the teaching signal per press, not what
  the sheet can represent. Credit one step back is reached on the cued chain and on
  arrival-only track and forage. Goal-only mountain car (0.00) is an exploration problem:
  the critic never sees a reward. Upright-only pendulum (0.08) is a reader problem: even
  told the right press the buttons get 0.08 to 0.14. The chain with no stage cue (0.09) is
  a memory problem: nothing holds the last press. Before the critic:
  Nothing of ours learns catch or cartpole. On cartpole learning
  makes the full engine worse than a coin, and the reference networks too: the "lean"
  reward grades the press before the one it follows (a fault in the world, being fixed),
  and the sparse reward needs credit carried 25 steps back. The GRU learns catch (0.72).
  The memory engine learns catch (0.99) and cartpole (109 lean, 279 sparse) once credit
  reaches two steps back, which confirms the lean reward's one-step lag from a second side.
- **More than four things.** Eight cues are now learned (0.84) by the new readout on the
  full engine. Sixteen is untested with it. An answer of 8 or 9 bits, with 14 to 16 motor
  cells a group, learns below its learning-off twin (add of 8 bits 0.38 against 0.45):
  groups fall silent. `readout_evidence 3` removes the drop and costs copy of 8 bits 0.78
  to 0.66. And reach sets which output bits can be learned: in copy of 8 bits the bit
  whose line is nearest the motors gets 0.90, the furthest 0.50.
