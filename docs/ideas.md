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
    forces the other answer to be tried. *building*
61. **Make good and bad equally common.** [known, as a baseline] credit. Under cartpole's
    reward bad arrives on 73% of steps whatever is done, so it says little. Thin out
    whichever signal is commoner until the two balance, at the plug, so every engine gets
    it. *building*
62. **Credit walks backward through the recorded ticks.** [ours; reverse replay is known in
    biology] credit to the inside. The sheet keeps its last 16 spike maps. When the readout
    is wrong, walk them backward: the motor cells that should change name the sources that
    were active one delay earlier, those name theirs, and so on for a few steps, moving
    evidence counters along the paths that actually fired. A backward pass with no
    gradients, only along edges that carried a spike, costing about one tick per step back
    and only on a reward. Aimed at teaching neurons inside the sheet. *building*
63. **More worlds.** The rungs between Echo and cartpole are missing. A two-armed bandit
    (no cue; which button pays), a two-step chain (press A then B), a track (move toward a
    target with feedback each step), a small grid to forage in, mountain car, pendulum. And
    long lives: proper RL takes hundreds of thousands of steps on cartpole. *building*
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
    the speed at once? *building*

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
66. **Tiring synapses.** A source that just fired counts for less for a few ticks. *idea*
67. **Warming synapses.** A source that just fired counts for more for a few ticks. *idea*
68. **Tiring neurons.** A neuron's threshold jumps when it fires and drifts back. *idea*
69. **A slow second potential.** Each neuron keeps a second sum that leaks much more slowly
    and adds to the first. *idea*
70. **Rebound.** A neuron that was just inhibited is easier to fire for a few ticks. *idea*
71. **Up and down states.** A neuron has two thresholds and stays "up" until it falls below
    the lower one. *idea*

### What a neuron computes
72. **Any-branch neurons.** Fire if any one slot has enough active sources (an OR of ANDs),
    not on the sum over slots. *idea*
73. **Veto.** One inhibitory source on a branch cancels the whole branch. *idea*
74. **Coincidence only.** Fire only if sources at two different delays are both active. *idea*
75. **Winner in the tile.** After the tick, only the most-driven few neurons of each tile keep
    their spike. *idea*
76. **Table plus count.** A table over the neuron's four strongest sources, a popcount over
    the rest. *idea*

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
    the raw reward, is what teaches. *idea*
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
     three levels: is it readable, does the readout get it, does the whole thing. *building*
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
     functions of the last few cues; each scored as readable, read, and learned. *building*
110. **A shell of views.** [known] The owner: "you have a shape, and you rotate it, you still
     know what it is. you form some imagination of a nd shell." As far as is known a brain
     does not rotate a model; it ties together the views that follow each other as a thing
     turns. Same mechanism as idea 102, tested on a viewpoint task. *the task is being built*
111. **A place that holds.** [known: persistent activity] The default sheet keeps a cue
     readable for 20 ticks and has lost it by 60. Search for sheets, or regions of a sheet,
     that hold it: slow leak, dense local excitation with wider inhibition, up and down
     states. Aimed at the time wall. *running: a 500-sheet search on how long a cue is held*
112. **Different regions, different rules.** [known] The owner: "grid cells have a different
     winner take all kind of activity bias too. that we wouldnt want everywhere in the brain
     but in somewhere probably?" Give each sector its own genome numbers, so one region can
     be winner-take-all and hold a bump of activity (a map, a memory) while another mixes
     freely. The sectors already exist; the numbers are global today. *idea*
113. **Screen the neuron and synapse family.** Ideas 66 to 76, each at several strengths,
     through the feature and hold probes, then a combined search. *building*
114. **Screen the wiring family.** Ideas 19 to 21 and 77 to 83, likewise. *building*
115. **MNIST.** The owner: "wed be cool as fuck if we beat mnist." Digits onto the sensor
     sheet, ten buttons, good and bad only. Three numbers: what a linear reader gets from a
     never-taught sheet, what the evidence readout learns, and how long it takes. Targets: a
     linear classifier on pixels (about 92%), then a small two-layer network (about 97%).
     *building*
116. **The rest of the map.** Worlds for relation (same or different, odd one out, bigger
     than), number, parts and wholes (grammar), chance (answers right only most of the
     time), context (a cue that changes what another means), longer memory (copy, recall,
     n-back), no teacher (predict the next input, fill in a gap), being shown the answer,
     and damage to the sheet. `docs/task-map.md` lists the tiers. *building*
117. **The fixed-net reference.** One ordinary network (a small two-layer one, and a small
     recurrent one), one size, one recipe, behind the same plug, told only good and bad,
     run on the whole map with nothing changed between tasks. Not to beat it task by task:
     to see where an unchanged network stops, and to calibrate which worlds are learnable.
     *building*
118. **One setting for everything.** Score our own designs the same way: one set of numbers
     for the whole map. Today the readout runs use flags chosen per task (long delays for
     the time tasks, more motor cells here, smaller line groups there). *idea: the combined
     engine is the first to be held to it*

### Outside the sheet (added 2026-11-25)

The owner: "are we limiting ourselves with this sheet of connections idea or no". In some
ways yes. A sheet commits us to learning as rewiring, to locality in two dimensions, to one
rule everywhere, and to doing only what a brain could do, on a machine that can also copy
exactly, hash and look things up in one step.

123. **A memory, not a sheet.** [known: sparse distributed memory, hyperdimensional
     computing; new here] An engine whose learning is storing and recalling bit patterns by
     content: long random bit vectors, bound with XOR, compared with popcount, kept in a
     table found by hashing. One-shot, no wiring to change, nothing to saturate. A brain has
     an organ like this beside its sheets. *building*
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
     shown once and asked for much later; rare events. *building*
130. **A pattern freezer.** [known: consolidation, cascade synapses] A connection whose
     evidence has stayed strong for a long time gets locked: it stops decaying and takes
     much stronger contrary evidence to move. *building*
131. **A timeline with log spacing.** [known in theory; cheap here] Beside the ring of the
     last 16 ticks, keep a few slower maps: what fired in the last 32, 64, ... 4,096 ticks,
     each refreshed half as often as the one before. Twelve small maps reach back thousands
     of ticks, and a slot can listen to any of them. *building*
132. **The owner's earlier machines.** `binary-brain-v1` (June 2025: an always-on one-bit
     substrate with three-factor local rules and four broadcast bits; the direct ancestor of
     this project) and `vvm` (a state vector that looks up its next operation by content).
     Read them for ideas not yet tried here, above all vvm's "the state is the address of
     the next operation". *idea*
133. **The calculation battery.** Adding and multiplying in binary with the answer given as
     bits, comparing, sorting three numbers, an ALU (an operation code and two operands),
     running a short program on a register, turning and moving points in 2D and 3D. Needs
     an answer that is several bits at once, not one of a few buttons. *building*
134. **An answer that is a pattern.** Several independent yes/no outputs read at once, each
     graded. Fills the "whole pattern out" hole in the task map; needed for calculation,
     for copying, and later for generating sound or text. *building, with 133*
