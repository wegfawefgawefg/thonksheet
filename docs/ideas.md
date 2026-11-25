# Ideas

The running list. The owner, 2026-11-24: "just keep listing ideas, trying them, it may take
100 ideas but surely we will find some core ones that actually work. we know its possible."

Rules for this file:
- Every idea gets a number that never changes, a tag for whether it is a known idea or new
  as far as we know, what it is aimed at, and a status.
- An idea is tried on the fixed ladder (same tasks, same seeds, the sweep tool) and its
  line gets the result and a pointer to the results doc. Nulls and failures stay on the
  list; they are results.
- Add ideas at the bottom whenever one comes up. Do not reorder.

Aims: **carry** (a cue reaching across tiles), **credit** (wiring the right thing on good
and bad), **room** (not saturating, holding more than one thing), **alive** (stable
activity), **speed**.

Status words: idea, building, tried (with result), kept, dropped.

## Credit rules

1. **Noise is the experiment.** [known rule; exact noise bit is ours] credit, room. Only
   neurons whose spike was noise get graded, against a running average of reward. *tried:
   loses. 0.81 on echo with 2 cues, 0.12 with 4, uneven across seeds; alongside good/bad it
   changes nothing. docs/results-2026-11-24-ablations-and-rivals.md*
2. **A budget per neuron.** [known] room. Every strengthening is paid for by a weakening
   elsewhere on the same neuron. *tried in the minimal engine: it is the piece that carries
   the result. Without it Echo drops 0.17 to 0.23 and the sheet saturates; with it every
   neuron holds exactly its 48 connections after 4M ticks. Not yet ported to the full
   engine. docs/results-2026-11-24-minimal-engine.md*
3. **Causal credit.** [known family] credit. Grade only a source that fired exactly its
   slot's delay before the target fired. *idea*
4. **Reward minus expectation.** [known] credit, room. Keep the current rule but act only on
   reward better or worse than its running average. *idea*
5. **A critic per sector.** [known family] credit. Each sector keeps its own average of the
   reward that follows its activity and grades itself against that. *idea*
6. **Grade the unexpected.** [rare] credit. Only pairs where the target firing was unusual
   for it get graded. *idea*
7. **Fast bit, slow bit.** [known theory] room. Use the two weight planes as a quick bit
   that flips easily and a slow bit that follows only if the quick one held. *idea*
8. **Undo on bad.** [new?] credit. Keep a tiny log of the last flips per neuron and revert
   them on bad, instead of weakening everything that was active. *idea*
9. **Winners only.** [known] credit, room. In each tile only the few most-driven neurons
   that fired get graded. *idea*
10. **Motor first.** [known-ish] credit. Only motor neurons learn until their score stops
    rising, then the rest unlock. *idea*

## Carrying a signal

11. **Every neuron predicts.** [known family] carry. Each neuron learns to guess its own
    next firing from its delayed inputs, with no reward. *idea*
12. **One spike makes one spike.** [known theory, rare as a regulator] carry, alive. Regulate
    each sector so a spike causes on average one spike downstream. *idea*
13. **Relays at birth.** [plain engineering] carry. A share of neurons born as delay-line
    relays of one source. *idea*
14. **Highways.** [design] carry. Born with a few top-strength connections along one
    direction per neuron, so activity has a way to flow. *idea*
15. **Learned delays.** [rare] carry. Move a slot's delay one tick, by chance, on an
    outcome. *idea*
16. **Chains.** [known] carry. Grow and strengthen A to B to C where they fire in order at
    their delays. *idea*
17. **No pulse.** [ours] alive, credit. Slow the sector regulator so activity stops
    pulsing. *tried: Echo late score 0.85 to 0.90 mean over 4 seeds, the failed seed
    recovers; too few seeds. docs/results-2026-11-24-rhythm.md*
18. **Open a gate near input.** [known in biology] carry. Sensor activity lowers inhibition
    around it for a moment. *idea*

## Structure and wiring

19. **Layers inside a tile.** [known] carry, room. Input, middle and output groups by bit
    position. *idea, asked for by the owner*
20. **Fewer slots per neuron.** [ours] speed, variety. Listen to 4 of the 9 local tiles.
    *idea*
21. **Fine locality inside a tile.** [known] carry. Prefer close cells at birth and growth.
    *idea*
22. **Start as a reflex, pull apart.** [new?] carry. Input and output areas overlap at
    first and are moved apart as the score holds. *idea*
23. **Grow when it matters.** [known-ish] credit. Growth chance goes up for a while after
    good. *idea*
24. **Drop what never mattered.** [plain] room. A connection not graded in many rewards is
    removed. *idea*
25. **Rebirth.** [known] room, alive. A neuron silent or stuck for long is reborn with fresh
    random wiring. *idea*

## Different neurons

26. **Table neurons.** [old idea, new on this sheet] speed, credit. A neuron reads 6 bits
    and looks up a 64-entry table of tiny counters. *tried: works, and holds more. Echo
    0.90 with 2 cues, 0.68 with 4 (the popcount engines get 0.31 to 0.41), 0.80 with cue
    and button two tile rows apart (the others fail there). Needed 8 inputs and 4-bit
    counters for four cues; the 6-input cell gets 0.93 and 0.53. No faster on a CPU core.
    Fails where the others fail: 8 cues, delayed echo, catch, cartpole.
    docs/results-2026-11-24-table-neurons.md*
27. **All-of neurons.** [known] credit. Fire only if every chosen input is on. *idea*
28. **Context gate.** [known in biology] carry, room. Input on the far slots decides whether
    local input is allowed to fire the neuron. *idea*
29. **Bursts as a second symbol.** [known, 2021] credit. Two spikes close together count for
    more downstream and mark the moment for learning. *idea*

## Selection

30. **Selection among tiles.** [new?] credit, carry. Copy the masks of tiles active before
    good outcomes over tiles that did poorly. *idea*
31. **Random flips.** [known] baseline. Flip bits, keep them if the score went up.
    *tried: slow. Into motor neurons 0.82 on echo; the only rule that improves cartpole
    (20 steps per fall against 10.6) but still no better than coin flips.*
32. **Twin tournament.** [known] baseline. Two blobs that differ by a few flips; the better
    one each window is copied over the other. *idea*
33. **A bandit per sector.** [new?] credit. Each sector alternates between two small mask
    variants and keeps the one followed by more reward. *idea*

## Memory

34. **Sleep.** [known] room. Input cut, weak weights decay, replay. *idea, liked by the
    owner*
35. **Make it permanent.** [known family] room. A connection strengthened by good many times
    is locked. *idea*

## Exploration

36. **Noise where surprise is.** [rare] credit. More spontaneous firing in sectors whose
    predictions are failing. *idea*
37. **Noise on motors only.** [plain test] credit. Compare with noise everywhere. *idea*
38. **Babbling.** [known in robotics] carry. Before any task, random motor firing while
    prediction learns what each motor does to the sensors. *idea*

## Teaching signals

39. **Show the answer.** [known] carry. After a wrong answer the world briefly fires the
    right motor cells. Tests association without the credit problem. *idea*
40. **More to sense.** [task design] credit. Extra sensor lines that say how close the goal
    is. *idea*

## Baselines and measurements

41. **Frozen blob, learned readout.** [known] baseline. *tried: ties the full rule on
    echo with 2 cues (0.914 against 0.940) and shuffled echo; stays at chance on 4 cues,
    where the full rule reaches 0.33. So inner learning pays only once the task is harder.*
42. **Carrying test.** How many tiles away can a cue still be read from activity. *tried:
    cue identity is readable 3 tiles out at birth, 4 with growth and pruning on, 6 after a
    long life or with many far links. Echo fails inside that range, so from 2 to 4 tiles the
    problem is credit, not carrying. docs/results-2026-11-24-carry.md*
43. **Capacity test.** How many cue-button pairs one blob holds: 2, 4, 8, 16. *idea*
44. **Retention test.** Learn A, learn B, retest A. *idea*
45. **Throughput numbers.** Synapse checks, real events, signal bits and stored bits in the
    speed test. *idea*
46. **A reader as weak as the rule.** Repeat the carrying test with a readout that learns the
    way the blob does, to see how far the real rule can read. *idea*
47. **Many far links.** [plain] carry. Half the neurons send far instead of 15%. *tried in
    the carrying test: reach 4 to 6 tiles; not yet tried on a task*
48. **Pool before deciding.** [known] credit. A motor cell listens to many weak neurons
    through one hop of relay cells that each sum a neighbourhood, so thinly spread
    information gets gathered before the reward rule has to find it. *idea*
49. **Richness test.** [ours] The owner's thesis: a sheet rich enough in features (in time,
    shape and place) turns learning into selection, so deep credit is not needed. Test it
    directly: freeze the sheet, learn only the readout, on tasks whose answer needs a
    feature the input does not hand over (which of two cues came first; two cues both on
    but not either alone; the same shape in a new place; a gap of N ticks). Count how many
    of these a frozen sheet already supports, at birth and after a reward-free life, and how
    that grows with sheet size. *tried: the features are there. An ideal reader gets order (3.4),
    exclusive-or (1.7) and gap (1.5) from the tile next to the input at birth; the suite's
    frozen readout scores chance on the same tasks. Delays triple the time features. So
    richness holds and reading is the wall. docs/results-2026-11-24-features.md*
50. **The minimal engine.** [ours] A fresh, tiny engine with three things only: a neuron is
    a popcount over a mask against a threshold; a synapse changes by chance on an outcome,
    graded only where the spike was noise, under a per-neuron budget; each neuron keeps its
    own firing rate. Plus wiring by co-activity, switchable. Does it match the 21-mechanism
    engine? *tried: on Echo it beats it. 2 cues 0.94 against 0.87, 4 cues 0.41 against
    0.31, shuffled 0.87 against 0.76, with half the code, 40% of the memory and about 4x
    the speed, and it does not saturate or pulse. It loses on reach (nothing past one
    tile; the full engine gets 0.58 at 3 tiles), has no memory past one tick, and forgets
    everything when idle unless co-activity wiring is switched off without input.
    docs/results-2026-11-24-minimal-engine.md*
51. **The suite.** One script scores any engine on every property in `docs/bench.md` and
    writes a scorecard; a second joins them into `docs/scoreboard.md`. *building*
52. **Slow credit.** [known family] credit in time. A second "fired recently" bit that lasts
    hundreds of ticks (maps ORed together and cleared on a slow schedule), graded at a lower
    chance. Cartpole's reward comes every 25 steps and the rule only reaches back 6 ticks.
    *queued*
53. **Lose-shift.** [known behaviour, mechanism ours] credit. On bad, the motor cells that
    just fired get harder to fire for a while, so a different answer gets tried. Aimed at the
    lock seen in seed 3 and in crossed shuffled-echo lives, where the blob keeps pressing the
    wrong button while punished. *tried: does not fix it. Crossed lives score 0.561 with it
    and 0.559 without on fresh seeds. It lifts the worst lives and wrecks the ones that had
    learned, and a learned blob wanders off after an unlucky bad. Its mirror, win-stay,
    makes crossed lives lock far harder (0.135): the lock comes from the reward rule
    strengthening its own answer. docs/results-2026-11-24-lose-shift.md*
54. **The ablation sweep.** Each mechanism off in turn, 8 seeds. *tried: needed are
    inhibition, per-neuron threshold regulation, good, bad, growth, pruning. No measurable
    effect from delays, branch bonus, varied neurons, noise, far redraw. The surprise boost
    is on 92-99% of the time, so it is only a doubling of the learning chances.
    docs/results-2026-11-24-ablations-and-rivals.md*
55. **Grow the minimal engine one piece at a time.** [ours] The minimal engine is now the
    base. A mechanism from the full engine goes back in only if it earns its place on the
    test it is for: a far slot on reach, delays on the timing tasks, lose-shift on the lock,
    slow credit on cartpole. *tried: neither far slots nor delays earned a place. Both do
    their mechanical job: with far slots the cue is readable at the buttons 3 tiles away,
    and with delays the cue's trace lasts through the gap of delayed echo. The score does
    not move (0.37 at 3 tiles; 0.23 on delayed echo). Delays do help when the ask comes
    right after the cue (+0.22) and cost Echo 0.20. The reward rule cannot use a thin,
    spread-out signal. docs/results-2026-11-24-minimal-grow.md*
56. **Wire by co-activity only while something is plugged in.** [ours] alive. Fixes the
    minimal engine forgetting everything when idle (0.95 to 0.33 becomes 0.95 to 0.96).
    *tried, works; now the default*
57. **Why does credit by noise learn the exact opposite?** [ours] In the table engine the
    noise-graded rule ends at 0.00 on most seeds, and with its sign reversed it scores 1.00
    on shuffled echo on every seed. Find the cause, and try the reversed sign in the other
    two engines. *tried: a slip in the rule's arithmetic, not a principle. Only flipped
    neurons were graded, which is half of the textbook rule; the missing half swamps the
    signal 2,500 to 1 and points the wrong way. Fixed, the rule learns nothing in the table
    engine and 0.90 on Echo in the minimal one. Only 1 flip in 2,500 changes the answer.
    docs/results-2026-11-24-noise-sign.md*
58. **A readout that can read what is there.** [ours] On a frozen sheet, find the local,
    online rule at the motor cells that gets closest to the ideal reader on which-at-a-
    distance, order, exclusive-or and gap. Candidates: an error rule (on bad, the button
    that fired loses its active inputs and the other gains them); a budget on the readout;
    more motor cells per button, spread through the sheet so some sit where the feature is;
    pooling cells. *tried: it works. An error rule that moves hidden evidence counters, with
    128 motor cells per button, on a frozen sheet: exclusive-or 0.88, order 0.70 (0.84 with
    long delays), Echo at 3 tiles 0.85, 4 cues 1.00, 8 cues 0.84, shuffled Echo 1.00 on every
    seed. The old readout scored chance on all of these. Needs the world to mark which
    button was pressed. A budget, pooling and far slots at the readout did nothing; gap
    stays near chance. docs/results-2026-11-24-readout.md*
59. **Evidence before commitment.** [known family, new on this sheet] credit. A connection
    is one bit, so one lucky or unlucky reward flips it. Give each possible source a small
    hidden counter (4 to 8 bits): good adds for sources that were active before the neuron
    fired, bad subtracts; a connection exists while its counter is in the top of the
    neuron's budget. Weak evidence then adds up over many tries before anything is wired.
    Why: table neurons needed 4-bit counters to learn four cues (2-bit gave 0.56), and
    every probe says the signal is present but thin. *tried in the minimal engine: helps
    reach (0.39 to 0.60 at 3 tiles), holding more (4 cues 0.41 to 0.63 with counters on
    every neuron; 8 cues above chance) and exclusive-or (0.26 to 0.58). Does not help time.
    Costs the one-tile reflex 0.2 and, on every neuron, six times the reward-path work and
    memory. The same idea at the full engine's readout, with an error rule, is idea 58's
    winner. docs/results-2026-11-24-evidence.md*
60. **Experiments the size of a decision.** [known in RL, new on this sheet] credit. One
    neuron's noise changes the answer once in 2,500 flips. So explore at the level of the
    answer: now and then the world nudges one whole button group for a step (motor
    babbling), and the usual rules learn from what follows. Also aimed at the lock, since it
    forces the other answer to be tried. *tried in all three engines (`--babble`, fading form best). It unfreezes the minimal engine's lives stuck on wrong answers (crossed lives 0.67 to 0.77, four cues 0.40 to 0.47, eight cues 0.08 to 0.19, the first score above chance there); small or negative in the full and table engines; no engine's crossed lives pass 0.8. docs/results-2026-11-24-babble-and-balance.md*
61. **Make good and bad equally common.** [known, as a baseline] credit. Under cartpole's
    reward bad arrives on 73% of steps whatever is done, so it says little. Thin out
    whichever signal is commoner until the two balance, at the plug, so every engine gets
    it. *tried (`--balance 1`): evens good and bad exactly and helps nothing; harmful under the sparse cartpole reward. Same doc as 60*
62. **Credit walks backward through the recorded ticks.** [ours; reverse replay is known in
    biology] credit to the inside. The sheet keeps its last 16 spike maps. When the readout
    is wrong, walk them backward: the motor cells that should change name the sources that
    were active one delay earlier, those name theirs, and so on for a few steps, moving
    evidence counters along the paths that actually fired. A backward pass with no
    gradients, only along edges that carried a spike, costing about one tick per step back
    and only on a reward. Aimed at teaching neurons inside the sheet. *tried (`--g.replay_depth N`), and it is the first rule here that makes the inside better. One step back is best: exclusive-or 0.88 to 0.985, echo at 5 tiles 0.59 to 0.88, eight cues 0.84 to 0.92; order and gap unchanged. A fresh output on the frozen walked sheet reads exclusive-or at 0.997 against 0.874 on the sheet as born, so the inside learned; it does not carry to another task. Two cautions: random signs give about half the gain (0.95), and on reach tasks the walk feeds itself over long lives (rate 112 against a target of 32 by 160,000 steps). Deeper than one step hurts. docs/results-2026-11-25-reverse-replay.md*
63. **More worlds.** The rungs between Echo and cartpole are missing. A two-armed bandit
    (no cue; which button pays), a two-step chain (press A then B), a track (move toward a
    target with feedback each step), a small grid to forage in, mountain car, pendulum. And
    long lives: proper RL takes hundreds of thousands of steps on cartpole. *built (`scripts/ladder.py`): bandit, switching bandit, chain, track, forage, mountain car, pendulum, cartpole variants. The readout learns what a one-step table can (bandit 0.77, the chain's cued second press 0.72, forage with warmer/colder 0.64, mountain car with an energy reward 0.94) and nothing that needs credit one step back. Found that cartpole's reward judged the press before the one it followed; fixed by the semi-implicit step, now the default. Million-step lives for the readout unrun. docs/results-2026-11-25-more-worlds.md*
64. **Does it survive growing?** Everything was tuned at 4,096 neurons. Run the same tasks at
    16,384 and 65,536. *tried on Echo with 2 cues, seeds 5 to 7, late score. Minimal engine:
    0.94, 0.93, 0.92 at 4,096 / 16,384 / 65,536 neurons, every seed within 0.91 to 0.97.
    Full engine: 0.84, 0.81, 0.82, with seeds anywhere from 0.63 to 0.99 at every size. So
    neither falls apart when the sheet grows 16 times around the same task; the full
    engine is erratic at every size. Only Echo, only 3 seeds, and the input and output
    areas stayed the same size.*
65. **Put the winners together.** [ours] The minimal engine as the base (budget, each neuron
    keeping its rate, inhibition, co-activity wiring only while plugged in), plus the things
    that earned a place elsewhere: the error rule that knows which button was pressed,
    evidence counters at the motor cells and inside, many motor cells per button, delays
    for the time tasks. Does the combination keep the reflex, the reach, the features and
    the speed at once? *tried, partly finished (`scripts/minimal.sh core`): the minimal engine with the error-rule readout on 128 cells a button, co-activity wiring everywhere, far slots and delays 1..8. Echo 0.99, 3 tiles 0.86, 5 tiles 0.76, eight cues 0.87, sixteen 0.53 (the full engine 0.31), delayed echo with the ask right after the cue 0.99; exclusive-or only 0.72, order and gap at chance. Co-activity wiring inside is worth 0.05 to 0.25 on every row that learns; the noise-graded reward rule and inner counters cost and were left out. docs/results-2026-11-25-core.md*

## The wider space (added 2026-11-25)

The owner: "you tested like what, 20 mechanics? shouldnt there be a fuckton of possible
ones?" There are. Three things widen the search from here.

- **Screen sheets without any learning rule.** Learning has turned out to be mostly selection
  at the output, so a mechanism inside the sheet can be judged in seconds by how readable
  the features are (`scripts/richness.sh`). Hundreds of variants an hour instead of a few a
  day.
- **Re-test the early nulls.** Delays, far links, the branch bonus and varied neurons were all
  judged "does nothing" through a readout that could not read. Those verdicts are void until
  re-run through the new readout.
- **Families.** Mechanisms grouped by what they change, a handful per agent, each scored by
  the richness screen first and the suite second.

### What a connection does over a few ticks
66. **Tiring synapses.** A source that just fired counts for less for a few ticks. *tried: harmful on the sensor lines, a small help on the sheet's own (which +0.6). docs/results-2026-11-25-family-dynamics.md*
67. **Warming synapses.** A source that just fired counts for more for a few ticks. *tried: null or harmful. Same doc as 66*
68. **Tiring neurons.** A neuron's threshold jumps when it fires and drifts back. *tried (`--g.tire_jump 4 --g.tire_shift 7`), and it is the first sheet change that makes delayed echo learnable: 0.55 to 0.91 two steps back, order 0.70 to 0.88, gap 0.52 to 0.68, at a cost to exclusive-or (0.88 to 0.78). Seeds 5-12, frozen sheet with the readout. Same doc as 66*
69. **A slow second potential.** Each neuron keeps a second sum that leaks much more slowly
    and adds to the first. *tried (`--g.slow_leak_shift 6 --g.slow_share_shift 4`): delayed echo 0.84, order 0.85, gap 0.69; exclusive-or 0.75. Same doc as 66*
70. **Rebound.** A neuron that was just inhibited is easier to fire for a few ticks. *tried: null. Same doc as 66*
71. **Up and down states.** A neuron has two thresholds and stays "up" until it falls below
    the lower one. *tried: null. Same doc as 66*

### What a neuron computes
72. **Any-branch neurons.** Fire if any one slot has enough active sources (an OR of ANDs),
    not on the sum over slots. *tried in the screen only: exclusive-or readable +0.2 to +0.5. Same doc as 66*
73. **Veto.** One inhibitory source on a branch cancels the whole branch. *tried: null to small loss. Same doc as 66*
74. **Coincidence only.** Fire only if sources at two different delays are both active. *tried: harmful; on every neuron it silences the sheet. Same doc as 66*
75. **Winner in the tile.** After the tick, only the most-driven few neurons of each tile keep
    their spike. *tried (winners 2): exclusive-or 0.88 to 0.98 through the readout, time tasks unchanged or worse. Same doc as 66*
76. **Table plus count.** A table over the neuron's four strongest sources, a popcount over
    the rest. *not built*

### Wiring
77. **Mirror links.** If A listens to B, B is likelier to listen to A (loops by design). *idea*
78. **Hubs.** A few neurons with ten times the connections, in and out. *idea*
79. **Streams.** Wiring biased along one direction of the sheet, so activity flows. *idea*
80. **Two sheets.** A second sheet that only the first one feeds, and that feeds back. *idea*
81. **Wire by timing.** Growth prefers sources that fired just before, not just recently. *idea*
82. **Prune what is redundant.** Drop a source that always fires with another one you
    already have. *idea*
83. **Topography in the far links.** Far links from neighbouring neurons land near each other.
    *idea*

### Global states and signals
84. **Novelty opens the gate.** Plasticity is on only for a while after surprise. *idea*
85. **Arousal.** After good or bad, the whole sheet's thresholds drop a little for a while.
    *idea*
86. **Rhythm as a clock.** Keep a slow pulse on purpose and let neurons prefer a phase of it.
    *idea*
87. **Replay in sleep.** When idle, replay the last rewarded stretches of the spike ring and
    learn from them again. *idea*
88. **Critical periods.** Each neuron's plasticity starts high and falls as it is used. *idea*
89. **Attention.** The readout's current evidence feeds back and raises the gain of the tiles
    it relies on. *idea*

### Credit
90. **A value cell.** A group of neurons learns to predict good and bad, and its change, not
    the raw reward, is what teaches. *tried with a critic (`--g.critic_on 1`, `scripts/critic.sh`; seeds 1-4 only): a second group of cells learns how well things are going, and its surprise teaches the buttons. Bandit 0.77 to 1.00, chain 0.78 to 0.97 (the GRU gets 0.01: the first row where a sheet beats the reference), catch 0.20 to 0.32, cartpole with the lean reward on the fixed world 20 to 89 steps per fall and still rising (the GRU 798). The surprise has to teach on good as well as bad, and the critic needs an opponent half. docs/results-2026-11-25-critic.md. Finished 2026-11-25 on seeds 5-12, 17 worlds, one setting with a slower actor (`readout_shift 5`): above its twin on 15; chain 0.99; arrival-only track and forage 0.78; cartpole lean stops at about 110 steps per fall through 400,000 steps (GRU 895). Kept. Next on it: more information per press (a level per button), a cheaper step end, exploration for goal-only worlds.*
91. **Error by the difference of two phases.** Run a few ticks with the answer clamped and a
    few without; learn from the difference in each neuron's activity. *idea*
92. **Teach the inside to help the readout.** An inner neuron gains evidence when the readout
    currently relies on it and was right. *idea*
93. **Counters that forget.** Evidence leaks toward zero, so old evidence fades. *idea*
94. **Two readouts that vote.** A fast one and a slow one; the slow one learns from the fast
    one's mistakes. *idea*

### Growth and development
95. **Start small.** A sheet that begins with few connections and adds them as tasks demand.
    *idea*
96. **Add neurons where surprise stays high.** *idea*
97. **Pre-wire by spontaneous waves.** Before any input, waves of activity cross the sheet and
    co-activity wiring lays down local structure (as in a developing retina). *idea*

### Automatic search
98. **Search the sheet's numbers for richness.** Random sheets, each scored by the feature
    probe as born. *tried: the default sheet is mediocre. The best of 600 read order and gap
    three to four times better and exclusive-or up to twice, and keep the lead on fresh
    seeds. Levers: longer delays (time), denser local wiring, the branch bonus (exclusive-
    or), more inhibitory neurons. docs/results-2026-11-25-richness-search.md*
99. **Search the switches in combination.** Every mechanism on or off, scored by the quick
    suite, to find pieces that only work together. *idea*
100. **Share what one patch learned.** [the day-one question: a net with all the priors]
     Nothing carries a feature to a new place (the suite's place task scores 0.25, under
     chance). Let evidence learned for a source at some offset from a cell leak to cells
     elsewhere for the source at the same offset, weakly, so that what is true everywhere
     ends up wired everywhere and what is local stays local. A convolution that is found,
     not built in. *idea*

### Invariance (added 2026-11-25)

The owner: "how does the brain do it? ... fundamental ways of getting the same kind of
effect cheaply? scale invariance, rotational invariance, position invariance, time
invariance, frequency invariance ... we should have envs for testing all of these and keep
trying more mechanics until we find something that can do all of them."

The brain does not share weights. As far as is known it gets invariance five ways, and each
is a mechanism to try. All are scored on the invariance battery (idea 101).

101. **The invariance battery.** A task per kind of change (place, scale, turn, thickness,
     time shift, tempo, pitch, clutter), rewarded on some versions and scored on versions
     never rewarded, with and without the pattern drifting through its versions, measured at
     three levels: is it readable, does the readout get it, does the whole thing. *built (`scripts/invariance.sh`, `blobprobe invariance`): position, scale, rotation, brightness, clutter, mixed, 3D view, time shift, tempo, pitch, with held-out versions never rewarded. Nothing carries a shape to a new place (0.49), turn (0.36), tempo (0.45) or pitch; small size is partly carried (0.69 to 0.77). The MLP and GRU learn every taught version and fail the same held-out rows. The old place world was a trap (a held-out picture overlaps the other shape twice as often as its own), which is why everything scored below chance on it. docs/results-2026-11-25-invariance-battery.md*
102. **What follows is the same thing.** [known: the trace rule] As a thing moves, the
     detectors it sets off one after another get wired to the same cells, because wiring
     uses a slow "fired recently". One local rule that gives place, size and turn alike,
     learned from motion. *idea*
103. **Cells that fire for any one of several.** [known: complex cells] A neuron that fires
     if any one of its slots is active enough pools the same detector over nearby places.
     Same as idea 72. *idea*
104. **Maps where the change is a slide.** [known: the eye and the ear] A log-polar map of
     the image turns growing and turning into sliding; frequency lines spaced by ratio turn
     a change of pitch into a slide; time constants spaced by ratio turn tempo into a slide.
     Then one answer to sliding answers all of them. *first test inside idea 101*
105. **Move the eye.** [known: fixation] Motor lines that shift what the sensor sheet sees.
     The thing gets centred by acting, and position stops mattering. *idea*
106. **Stages with growing reach.** [known] A second and third sheet, each cell pooling a
     patch of the one before, so by the top a pattern anywhere looks the same. *idea*
107. **The same rule sees the same world everywhere.** [known] No sharing is needed if every
     patch sees every pattern in time. Test: after a life of drifting patterns and wiring by
     co-activity, are the detectors in different tiles alike? *idea*
108. **Gain control.** [known] Each cell's input divided by how active its neighbourhood is,
     so thick and thin, bright and dim drive it alike. *idea*
Idea 100 (leak evidence to the same offset elsewhere) is the engineer's way; the brain is
not known to do it.
109. **The function battery.** The owner: "xor, 2 point, 3 point, 4 point pattern. 5 point
     pattern. sin, cos, theres lots of cheap envs we can do. and we should be able to learn
     all of em." Boolean functions of N cues (and, or, parity, majority, random truth
     tables), counting, functions of a number (step, tent, sine, cosine, sums, products),
     functions of the last few cues; each scored as readable, read, and learned. *built, partly run (`scripts/functions.sh`, `blobprobe reader`): and, or and majority of up to 8 lines are learned (0.96 to 1.00); parity stops at 2 lines (parity of 3: readable 0.58, learned 0.54) and no sheet carries parity of 4. A richer sheet climbs one rung (rich4: parity of 3 readable 0.86, learned 0.76); a bigger sheet does not. The number and past families are unrun. docs/results-2026-11-25-function-battery.md*
110. **A shell of views.** [known] The owner: "you have a shape, and you rotate it, you still
     know what it is. you form some imagination of a nd shell." As far as is known a brain
     does not rotate a model; it ties together the views that follow each other as a thing
     turns. Same mechanism as idea 102, tested on a viewpoint task. *the task is being built*
111. **A place that holds.** [known: persistent activity] The default sheet keeps a cue
     readable for 20 ticks and has lost it by 60. Search for sheets, or regions of a sheet,
     that hold it: slow leak, dense local excitation with wider inhibition, up and down
     states. Aimed at the time wall. *tried by search over the existing numbers: 60 ticks can
     be held (readable at 3.4 to 3.9 on fresh seeds, default 0.23) by a sheet that keeps
     ringing, at a third of the sharpness; nothing holds 150. Longer needs a mechanism and a
     region of its own. docs/results-2026-11-25-richness-search.md*
112. **Different regions, different rules.** [known] The owner: "grid cells have a different
     winner take all kind of activity bias too. that we wouldnt want everywhere in the brain
     but in somewhere probably?" Give each sector its own genome numbers, so one region can
     be winner-take-all and hold a bump of activity (a map, a memory) while another mixes
     freely. The sectors already exist; the numbers are global today. *idea*
113. **Screen the neuron and synapse family.** Ideas 66 to 76, each at several strengths,
     through the feature and hold probes, then a combined search. *building*
114. **Screen the wiring family.** Ideas 19 to 21 and 77 to 83, likewise. *screen done on the
     sheet as born (4 seeds, ideal reader); the combined search and the readout runs are
     still going. Movers: inhibition kept local (exclusive-or 1.76 to 2.84, gap 1.60 to
     4.62, at a cost to which-cue and order) and mirror links (small steady gains, carry
     +0.34). Reach only: two sheets (reach 3.25 to 5.0 tiles) and gentle streams. Harmful:
     layers (19), fine locality (21), fewer slots (20). Nothing: hubs (78), far-link
     topography (83). One early sign that growth and pruning erase birth wiring within
     100,000 ticks. docs/results-2026-11-25-family-wiring.md on its branch*
115. **MNIST.** The owner: "wed be cool as fuck if we beat mnist." Digits onto the sensor
     sheet, ten buttons, good and bad only. Three numbers: what a linear reader gets from a
     never-taught sheet, what the evidence readout learns, and how long it takes. Targets: a
     linear classifier on pixels (about 92%), then a small two-layer network (about 97%).
     *done, first pass (`scripts/mnist.sh`). A linear reader of the never-taught sheet:
     0.924 on the test set (linear on pixels 0.922, two-layer net 0.955, a random
     projection of the same bits 0.943). The sheet's own readout from good and bad: 0.71,
     reached in 20,000 digits (about a minute), then it drifts down (0.687 at 60,000, 0.635
     after three passes). It was at chance until: good also moves the right button, a fix
     for counters at the clamp silencing buttons, wired strength 1, 102 cells a button, the
     digit spread over the whole sheet. Moved 2 pixels, everything (ours and the plain
     learners) keeps about two thirds of its score. docs/results-2026-11-25-mnist.md*
116. **The rest of the map.** Worlds for relation (same or different, odd one out, bigger
     than), number, parts and wholes (grammar), chance (answers right only most of the
     time), context (a cue that changes what another means), longer memory (copy, recall,
     n-back), no teacher (predict the next input, fill in a gap), being shown the answer,
     and damage to the sheet. `docs/task-map.md` lists the tiers. *built, partly run (`scripts/map.sh`): 21 worlds for relation, number, parts and wholes, chance, context, longer memory and no teacher, plus being shown the answer and damage. The readout learns thin and noisy association (rare cue 1.00, noisy lines 0.99, context line 0.93, a rule that switches 0.94) and holds a cue for one step (copy of 2 0.66, 1-back 0.71); nothing held two steps, no relation on untaught items, no grammar, analogy or count. Being shown the answer teaches no engine; the readout loses nothing to 20 in 100 of the sheet silenced. docs/results-2026-11-25-rest-of-map.md*
117. **The fixed-net reference.** One ordinary network (a small two-layer one, and a small
     recurrent one), one size, one recipe, behind the same plug, told only good and bad,
     run on the whole map with nothing changed between tasks. Not to beat it task by task:
     to see where an unchanged network stops, and to calibrate which worlds are learnable.
     *done (`blobrun --engine mlp|gru`, `scripts/reference.sh`). A plain recurrent network
     (GRU, 128 units, backprop through 8 steps, good and bad only) learns every row any of
     our engines learns, at the same or a higher score: echo to 16 cues 1.00, exclusive-or
     0.97, delayed echo, beat, order and gap all 1.00, catch 0.72. Ours leads only in
     speed on association (level reached in 1,000 steps against 2,000 to 3,000). Both
     networks fail the place never rewarded (0.16, below chance), holds past the GRU's
     8-step window, and cartpole from good and bad (below a coin; told the answer they
     balance). docs/results-2026-11-25-fixed-net-reference.md*
118. **One setting for everything.** Score our own designs the same way: one set of numbers
     for the whole map. Today the readout runs use flags chosen per task (long delays for
     the time tasks, more motor cells here, smaller line groups there). *idea: the combined
     engine is the first to be held to it*
119. **The unattended search.** A loop that draws designs (genome numbers and mechanism
     switches), scores each on a quick map of worlds, keeps the best and varies them, on
     this PC and stevejobs, with a held-out set of worlds it never sees. A combination
     search: it cannot invent a mechanism, only find which of ours go together. *built and
     running (`scripts/search.sh`, docs/search.md, docs/search-leaderboard.md): candidates
     raced through four rungs, confirmation on seeds 5-8 that selection never sees,
     survives being killed, a seam for the GPU, rule tables as a kind of number. On a small
     case (3 worlds, 163 candidates) it found settings passing 2 of 3 where the defaults
     pass 1, by trading echo with 2 cues for 4. The long run: full engine on this PC
     (from the search agent's worktree, which must stay), minimal and table on stevejobs,
     48 hours from 2026-11-25 04:46 UTC. Nothing beats the defaults yet. Speed: minimal 51
     candidates per core hour, table 6, full engine 1.*
120. **The rule as a table.** The learning rule itself as a small lookup table (what a
     connection does given a few local bits: fired, source fired, good, bad, acted), so the
     search can vary rules and not only numbers. *built (`--g.rule_on`, `--rule-file`, `scripts/rules.sh`): a rule is a table over ten local yes-or-no facts, 1,024 entries. The error rule, the old good/bad rule and growth are all written as tables and score like the originals. A search over inside rules ran 11 generations and its best table gains +0.12 on its own selection seeds; nothing is confirmed on fresh seeds. docs/results-2026-11-25-rule-tables.md*
121. **Many lives on the GPU.** *done for the minimal engine and the Echo family
     (`scripts/gpu.sh`, src/gpu/, shaders/gpu/). One workgroup steps one life; bit-exact
     with the CPU engine (`blobgpu --check`, nine cases; full-length lives identical as
     text). 9.5 million life-ticks a second at 1,000 lives or more, the work of about 44
     unloaded cores; 16,384 lives and their twins (26 billion ticks) in 40 minutes with no
     fault on the card. Echo over 16,384 seeds: 0.947 (sd 0.021). Not ported: evidence
     counters, far slots, delays, worlds outside the Echo family.
     docs/results-2026-11-25-gpu.md*
122. (number not used)

### Outside the sheet (added 2026-11-25)

The owner: "are we limiting ourselves with this sheet of connections idea or no". In some
ways yes. A sheet commits us to learning as rewiring, to locality in two dimensions, to one
rule everywhere, and to doing only what a brain could do, on a machine that can also copy
exactly, hash and look things up in one step.

123. **A memory, not a sheet.** [known: sparse distributed memory, hyperdimensional
     computing; new here] An engine whose learning is storing and recalling bit patterns by
     content: long random bit vectors, bound with XOR, compared with popcount, kept in a
     table found by hashing. One-shot, no wiring to change, nothing to saturate. A brain has
     an organ like this beside its sheets. *done (`blobrun --engine memory`,
     `scripts/memory.sh`). The input is cut into events; the address is now plus the last
     four events and how long each lasted; 256 hashed tables of small per-button counters.
     One setting, seeds 5-12: echo to 16 cues 0.98, delayed echo 0.99 (also across 4,000
     silent ticks), beat, gap, order, exclusive-or all 0.99, catch 0.99, cartpole 109 and
     279 steps per fall (coin 23, GRU 12 to 14), about 1,000 random pairs held, a pair
     learned in under one error on average. It is a lookup table of what it has seen, and
     it fails where that is not enough: place 0.57 (a coin per layout), two distracting
     events in a gap, 2,000 pairs. Reading a sheet's spikes instead of the raw input made
     every row worse. docs/results-2026-11-25-memory-engine.md*
124. **Organs.** [known] Not one sheet with one rule: a sheet that mixes, a memory that
     stores at once, a selector that picks actions and explores, a bank of clocks for time.
     Each cheap, each with its own rule, behind one plug. *idea, after 123*
125. **Use what the machine can do and a brain cannot.** [ours] Exact copying of what one
     patch learned to all the others, hashing, sorting by evidence, global lookup. The
     hardware rule was "even if it is a strange version of the biological equivalent".
     Idea 100 is the first of these. *idea*
126. **Worlds the search never sees.** [method] A held-out set of worlds used only to
     confirm a design, never to choose one, so "works on any task" is tested on tasks the
     design was not fitted to. *idea: split the map once the batteries land*
127. **Someone to argue the other side.** [method] Now and then an agent whose only job is
     to find the cheapest thing that beats our best design on the map, by any means, and to
     say what that implies. *idea*

### Long memory and calculation (added 2026-11-25)

The owner: "idk how the brain locks in patterns from 20 years ago and keeps them ... i
imagined a sort of pattern freezer ... we probably need envs for patterns across long time
horizons ... holographic log time memory ... content addressable bank." And: "should we add
various alu, multiplication compute style tasks? executing programs, 3d transforms etc.
would be cool if we can arbitrarily learn calculations."

129. **Long-horizon worlds.** A stream of new cue-answer pairs for a whole life with old
     ones re-asked at ages of a hundred to a million steps (a forgetting curve); a thing
     shown once and asked for much later; rare events. *built (`blobrun --long`, `scripts/longlife.sh`): hold, lifelong pairs, once, rare, drift. Baseline: every engine has lost a pair by 10,000 steps and holds 8 to 14 at once. docs/results-2026-11-25-long-memory.md*
130. **A pattern freezer.** [known: consolidation, cascade synapses] A connection whose
     evidence has stayed strong for a long time gets locked: it stops decaying and takes
     much stronger contrary evidence to move. *tried in three forms on two engines: null to harmful. Forgetting here is a capacity limit (the output holds 10 to 14 pairs), not counters drifting. Decay keeps the output learning for a whole life and wrecks eight cues; no rate suits both*
131. **A timeline with log spacing.** [known in theory; cheap here] Beside the ring of the
     last 16 ticks, keep a few slower maps: what fired in the last 32, 64, ... 4,096 ticks,
     each refreshed half as often as the one before. Twelve small maps reach back thousands
     of ticks, and a slot can listen to any of them. *null as slow maps on table neurons; works as time cells (cells that fire in log-spaced bins of the ticks since an input line last came on) beside cells that hold without leaking (`--g.time_percent 10 --g.hold_percent 10`): delayed echo 0.53 to 1.00, gap 0.98, order 0.99, eight cues 0.96, a hold of 5,000 ticks 0.98, echo and exclusive-or unchanged. Two to four seeds; the cells take their input straight from sensor lines, which is a built-in prior*
132. **The owner's earlier machines.** `binary-brain-v1` (June 2025: an always-on one-bit
     substrate with three-factor local rules and four broadcast bits; the direct ancestor of
     this project) and `vvm` (a state vector that looks up its next operation by content).
     Read them for ideas not yet tried here, above all vvm's "the state is the address of
     the next operation". *idea*
133. **The calculation battery.** Adding and multiplying in binary with the answer given as
     bits, comparing, sorting three numbers, an ALU (an operation code and two operands),
     running a short program on a register, turning and moving points in 2D and 3D. Needs
     an answer that is several bits at once, not one of a few buttons. *built 2026-11-25:
     `--task calc`, `scripts/calc.sh`, docs/results-2026-11-25-calculation.md. 7 of 56
     learned with held-out cases; nothing that needs a product of distant lines. Next: a
     layout with the operands' bits side by side; more motor cells per output.*
134. **An answer that is a pattern.** Several independent yes/no outputs read at once, each
     graded. Fills the "whole pattern out" hole in the task map; needed for calculation,
     for copying, and later for generating sound or text. *built with 133: two motor
     groups per bit, voted like two buttons. A verdict per bit teaches; one verdict for
     the whole answer holds two bits (finding 42).*
135. **The state is the address of what to do next.** [the owner's vvm, 2026; kin to
     Kanerva's memory and to production systems] A different route to a general learner,
     not a sheet: one state (a long bit vector) and a bank of operations found by content.
     Each step the state looks up the few operations nearest to it, one is chosen and
     applied to the state, and the result is the next state and the next address. No
     program counter, no separate memory: thinking is the state walking through the bank.
     His version was trained by gradient descent and never learned. Here the bank would
     learn by local rules: evidence on which operation helped from which state, and new
     operations written in one go. Aimed at holding, at multi-step calculation, at acting.
     The owner: "i suspect it can actually work. but idk how. its a whole other route to a
     general learner i think." *done (`blobrun --engine vm`, `scripts/vm.sh`, src/vm/), and it converges. His original, run unchanged: the nearest-op lookup sits on one op forever whatever the input; his refractory lockout of 4 or 8 ticks gives loops of exactly 5 or 9 ops (his "loop 7-15"); the op added each tick is as big as the state, there is no "do nothing" op, and the input barely affects which op is picked. The bit version: the input is written into the state every tick, an operation only sets or clears a few bits and leaves the rest, entries are found by nearness, and a new input makes its own detector with no reward. One setting, seeds 5-12: delayed echo 1.00 across 20 silent steps (GRU 0.49, best sheet 0.55), 16 cues 1.00, a second task with the first kept, hold with distractors on the cue lines 0.97 (memory 0.63, GRU 0.50), computed carry 0.88, parity of 3 and 4 lines 0.85 and 0.75 (GRU 0.68 and 0.52), majority on never-rewarded patterns 0.78 (memory 0.45), 3% input noise 0.94 (memory 0.50). It fails: any sense of elapsed time (gap 0.48, beat 0.42), credit one step back (chain 0.00), several items in order (copy of 6 0.27, brackets at a coin), 32 cues, a shape in a new place (0.21), catch and cartpole. docs/results-2026-11-25-vm-route.md*
136. **Seeing battery.** The owner: "shape identification. square. triangle. lines. dot.
     object tracking." Name the shape (dot, line, square, triangle, cross, ring) wherever
     it is and whatever its size; which way a line points; how many things there are; where
     a moving thing is now (which part of the sheet), and where it will be in a few steps;
     follow one thing among others that look the same; a thing that passes behind a cover
     and comes out the other side. Answers by button, by pointing (a motor line per region),
     and by moving a marker onto it. *queued: starts when the invariance battery lands,
     since it reuses that battery's shape drawing*

### From the outside catalogue (added 2026-11-25)

`docs/outside-catalogue.md` has 149 mechanisms from fields that were never about learning
machines, each translated to bits. These are the ones to try first, queued in this order
and started as machine time frees up (both machines are full).

137. **A bank of predictors at history lengths 2, 4, 8, 16 ...** [from branch predictors in
     processors (TAGE); new here] Hashed tables of small counters, each indexed by the last
     2, 4, 8, ... steps of what happened; the longest history that has an opinion wins.
     Built for exactly our shape of hardware, and aimed at the gap and at sequences. With a
     pacemaker count (a gap kept as a stored number of ticks, not as activity). *queued*
138. **Credit by code.** [from simultaneous perturbation in control engineering] Every unit
     nudges its behaviour up or down following its own fixed pseudo-random code, all at
     once; each unit then correlates the outcome with its own code. One experiment the size
     of the whole decision, credit that is still per unit, a few counters each. Aimed at
     teaching the inside. *queued*
139. **Units pay the units that set them up.** [bucket brigade; Baum's economy] A unit that
     earns passes part of its earnings to the units whose firing let it fire; wealth is
     conserved, and the poor are replaced. Credit that flows backward as payment. Aimed at
     teaching the inside and at chains of actions. *queued*
140. **Copy, vary, and keep a frozen copy.** [the immune system; gene duplication] A unit
     pulled two ways is duplicated so one copy can change and the other stay; units that
     fit badly are copied with more changes; proven units are set aside unchanged. Aimed at
     keeping while adding. *queued*
141. **Never erase: unlearning is new learning tagged with its context.** [extinction in
     animals] *queued, with 140*
142. **Run it again.** [Braitenberg's vehicle 13] After a rare strong outcome, loop the last
     few recorded spike maps through learning several times. *queued, small*
143. **Tax the common answer.** [ecology: kill the winner] The more often an answer is
     given, the harder it is to give. Aimed at the lock. *queued, small*
144. **Look one step ahead.** [Braitenberg's vehicle 14] Counters for "after this, that" and
     for how good "that" was; choose by the predicted next state. The project has no
     lookahead at all. *queued, with the critic*
145. **The behaviour battery.** Twenty-two small worlds from a century of animal learning
     that any learner should pass and the task map lacks: habituation, blocking,
     overshadowing, latent inhibition, extinction with recovery and renewal, partial
     reward, second-order conditioning, reversal, learning sets, matching, interval timing,
     latent learning, spacing, superstition. *queued*
Also from Braitenberg, for whoever builds a holding region: holding activity while wiring by
co-activity runs ties the held units together and loses their order.
146. **Why the readout drifts down on a long life.** On MNIST it peaks at 20,000 digits and
     then loses a point every 20,000 more. Not the counter clamp. Suspects: the motor
     cells' threshold regulation; sources dropped and re-wired at the budget's edge. The
     same thing may be hiding in every long life. *answered 2026-11-25 by idea 161: the
     good half of the rule pushing the nine other buttons down; `readout_good 2` removes
     it to 120,000 digits.*
147. **A weighted vote.** Wired strength set by where the counter stands (1, 4, 5 are already
     in the weight planes), so a motor cell is a weighted vote and not a count. Aimed at the
     25 points between the sheet's readout (0.71) and what a linear reader gets from the
     same sheet (0.92). *built 2026-11-25 as `--g.readout_evidence_strong 2` (top quarter
     at strength 4): on the runner-up rule MNIST +0.011 to +0.015 and 16 cues 0.605 to
     0.723, exclusive-or 0.793 to 0.725. A trade, not a free gain.*
148. **Search richness on digits.** The rich sheets found for order and gap read digits
     worse than the default. Score the search on the digit ceiling. *queued*
149. **A suite a lookup table cannot pass.** The memory engine passes tiers 1, 2, 4, 7 and 8
     by storing what it saw. So those rows test storing. Every family needs rows where the
     asked case was never shown: held-out inputs (the calculation and function batteries
     have them), noise on the input lines, real data, more distracting events than any
     fixed depth. Score "generalises" apart from "remembers" on the scoreboard. *next: fold
     into the batteries as they land; with idea 126*
150. **What the memory needs from a partner.** Not mixing: a code that is the same for
     things that should be treated the same (a shape wherever it is, a digit moved 2
     pixels). A sheet, or anything, that learns such a code with no reward, feeding the
     memory. This is the invariance problem restated as one organ's job. *queued behind the
     invariance battery*
151. **The memory engine on the GPU, and any world.** The memory engine is table lookups and
     small adds, the easiest thing here to run by the thousand. Port it, with a host-world
     mode (engine on the card, world on the CPU, one exchange per step) so every world on
     the map runs, not only Echo. Then the search can vary the memory's shape (what goes in
     the address) on held-out worlds at ten million ticks a second. *done (`scripts/gpu.sh settings in.csv out.csv --tasks ... --twin`, either engine). Any world runs with the engine on the card and the world on the CPU, exchanging once a step; the minimal engine's evidence counters, far slots and delays are ported; all of it bit-exact with the CPU (44 of 44 task kinds for the minimal engine, 43 of 44 for the memory engine). Life-ticks a second: host worlds about 7 million, evidence counters 0.6 to 5 million, the memory engine 12 to 16 million (32 to 58 cores' worth). 64 memory designs on 8 worlds with twins: 3.6 seconds. No fault on the card. docs/results-2026-11-25-gpu.md*
152. **A memory that knows near from far.** The memory engine on MNIST, one pass from good
     and bad, each digit new when first answered: 0.20 with its defaults, 0.32 at best
     (address from the current picture only, 1,024 tables; chance 0.10; the sheet's readout
     gets 0.71). Its codes are parities of 16 random cells, so one changed pixel changes
     the whole address: it has exact recall and no sense of "nearly the same". Tables that
     read small sets of raw input bits (an n-tuple memory; the table neurons are close to
     this) keep nearness. Give the memory such an address and score it on MNIST and on the
     held-out rows. *queued: small*
153. **Tags from the chosen action.** [AGREL, AuGMEnT, BrainProp, in bits] After the answer,
     the chosen button's cells mark the inner neurons that fed them, minus those that fed
     the other buttons; the verdict then moves only marked neurons' evidence counters,
     under fixed counts of excitatory, inhibitory and strong connections. *done
     (`--g.tag_mode 2 --g.tag_inh 0 --g.tag_min 4`, `scripts/tags.sh`). Seeds 5-12:
     exclusive-or 0.88 to 0.996, eight cues 0.84 to 0.95, echo at 5 tiles 0.59 to 0.78;
     order and gap unchanged; echo at 2 tiles worse (0.79). A fresh output on the lived
     sheet reads exclusive-or at 0.998 against 0.855 as born, and eight cues at 0.97
     against 0.81: the inside learned, for the task it was rewarded on, with no transfer.
     Unlike reverse replay it does not run hot: rate 33 against a target of 32 after
     160,000 steps, because each neuron keeps its counts. The contrast between buttons is
     what makes it work; feedback from the chosen button alone hurts. Same mechanism as
     idea 62 in another form. On MNIST it adds nothing. docs/results-2026-11-25-tags-and-margin.md*
154. **Learn on good only when the vote was close.** [Brader, Senn and Fusi 2007] *done
     (`--g.readout_evidence 2 --g.readout_good 1 --g.margin_percent 20 --g.margin_win 2`).
     Sixteen cues 0.31 to 0.60 (pairs held 8 to 16), order 0.70 to 0.78, eight cues learned
     in 2,000 steps instead of 17,000, MNIST 0.71 to 0.81, and no locked lives; costs
     exclusive-or (0.80) and reach. It does not stop the slow decline on long lives (MNIST
     0.81 to 0.70 by 180,000 digits; buttons die). Baldassi's deepening form gained little
     and wrecked order. Same doc as 153*
155. **One inner rule from the two that work.** Reverse replay's spike-exact credit and
     growable reach, with the tags' contrast between buttons, their threshold, no learning
     by inhibitory neurons and fixed counts per neuron. Scored on the rows where each one
     alone loses (order, echo at 2 tiles, long lives), on MNIST's ceiling, and on held-out
     rows. *done 2026-11-25: `--g.tag_mode 2 --g.tag_inh 0 --g.tag_min 4 --g.tag_exact 1`,
     `scripts/inner.sh`, docs/results-2026-11-25-inner-rule.md. It ties the plain tags and
     gains none of the walk's reach, so the merge is not kept as the default. Its tests
     gave more than the rule: transfer to related tasks (parity of 3 after exclusive-or
     0.856 against 0.552), a random-neuron control, and MNIST 0.938 that coin rewards
     match. Left: pieces on report seeds, long lives, a form of the walk's reach at fixed
     counts.*
156. **The vm's four walls.** Time (give the state a clock: a counter field in the address,
     or the time cells of idea 131 as input); credit one step back (the critic's surprise
     as the verdict); several items in order (marks that do not halve each other: a slot
     per position); a learned gate on what to keep. *queued*
157. **Why the output declines on a long life.** With the gate, buttons die one by one. A
     floor per button, or balancing the good half by how often each button is right.
     Replaces idea 146. *answered by idea 161: the runner-up rule; no button goes quiet.*
158. **Competitive learning with local inhibition, no reward.** [Rumelhart and Zipser
     1985; Kohonen; in bits] A hidden cell that fires among its tile's winners moves
     hidden counters toward the sources that were just on and rewires its top counters at
     its born counts. *done 2026-11-25: `--g.compete_on 1 --g.tile_winners 4
     --g.compete_shift 5`, `scripts/inside.sh`, docs/results-2026-11-25-compete.md. MNIST
     lived 0.952 against born 0.926 and shuffled control 0.939, 8 seeds; half the margin
     is not yet separated from a doubled firing rate; the sheet's own readout does not
     use it. Next: a cheap read-off (it costs 1.9x per tick), the rate, the time suite.*
159. **The error sent one hop, to every hidden cell.** [direct feedback alignment, Nokland
     2016; e-prop, Bellec 2020; in bits] Fixed random wires from the output's error lines
     to each hidden cell, a fading mark on each connection that just helped its cell
     fire, a hidden counter moved by the sign on the wire when the error arrives. The
     owner's "hyper truncated bit backprop". *done 2026-11-25: `--g.hop_on 1
     --g.hop_teach 1|2`, `scripts/hop.sh`, docs/results-2026-11-25-hop.md. Teaches the
     inside products on the toys (parity of 3 0.55 to 0.82, coin 0.59) and on held-out
     calculation cases (xor4 0.63 to 0.87, coin 0.67). On MNIST its coin control gets
     nearly all of its gain. Next: idea 163.*
160. **The shuffled-input control.** Any life that claims to teach the inside is repeated
     on inputs shuffled afresh each time and read on real inputs. *done, part of
     `scripts/inside.sh`; a rate-matched control is still to add.*
161. **Why our output cannot read what the inside learned.** A linear reader gets 0.95
     from the lived sheet and the sheet's own readout 0.72, falling on long lives. The
     motor cells hear only nearby cells, with one-bit connections and a vote. Candidates:
     a weighted vote (147), motor cells that hear the whole sheet, the drift (146, 157).
     *done 2026-11-25: docs/results-2026-11-25-output-gap.md, `scripts/outgap.sh`,
     `blobmnist ladder`. The gap is the rule's good half (0.28 of it). Fix:
     `--g.readout_good 2 --g.margin_percent 20 --g.margin_win 2`, MNIST online 0.852 at
     120,000 digits (was 0.649), no drift. Open: the sign, the lived sheet's lead, the
     exclusive-or cost, long lives past 120,000, the rule on the critic and on calc.*
162. **Every line is a target.** [predictive coding; Sutton's many predictions at many
     horizons ("Horde"); in bits] Owner, 2026-11-25: "in all of our envs we should extract
     rich signal. thonksheet shouldnt just output policy right. shouldnt it predict current
     state, next state, future states ... a real brain extracts much more info from the
     environment. and that info aids the goal and feature learning." Good or bad is at
     most one bit a step; the input is hundreds of lines a step and each one, a step
     later, is a free right answer. So: the sheet predicts every input line 1, 2, 4, 8
     steps ahead, always on, in every world; its own last press comes back in as input so
     the next state can depend on it; each line's error teaches its predictor, and the
     errors go to the inside (as lines, and one hop to every hidden cell once idea 159 is
     in). The critic is then just the predictor of the reward line. Takes in queue item 3
     and idea 137. *first version done 2026-11-25: `blobrun --predict 1,2,4,8
     --predict-errors 1 --predict-press 1`, `scripts/predict.sh`,
     docs/results-2026-11-25-predict.md. The cells predict chain and predict-next one step
     ahead well above every baseline; a fitted reader does much better (the output gap);
     nothing in it can change a hidden cell, and with competitive learning the gain equals
     a time-shuffled control. Error lines as input make the recent past readable. Next:
     idea 164.*
163. **What stops the one-hop error on real data.** Three suspects from idea 159's MNIST
     result: one cell hears about 6 pixels and cannot become what the error asks for
     (try wider cells, and a second set of cells that hear the taught ones, each still
     one hop from the error); MNIST is nearly solved by a straight-line reader, so it
     cannot show a product being learned (score on a real-data task a straight line
     fails); the errors come from a readout that is mostly right and has dead buttons.
     Also parity of 4, and shuffled-input controls for the toys. *building*
164. **Prediction errors down the one-hop wires.** Idea 162's point, now possible: each
     input line's signed prediction error (`predict_errors()`) becomes an error line of
     idea 159's rule, so every hidden cell is taught by what the sheet failed to predict,
     with no reward at all. Hundreds of error lines a step in place of a handful from
     the buttons. Scored by the inside read on the control worlds against a time-shuffled
     life and the coin control, by the held-out calculation columns, and by whether the
     task improves. The critic's surprise is then one more such line. *building*
