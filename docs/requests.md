# Requests

What the owner asked for, in his words, newest last. Add to this when he asks for something.
A request that becomes work also gets a line in `docs/plan.md`.

## 2026-11-23 (planning chat on claude.ai)

- "is there a nn that has all possible priors? ... a brain does this by starting with
  overconnection and then trimming over time"
- "should we do this without backprop"
- "we dont wanna redesign the arch for every game yeah? just the eyes and the body"
- "a grid of weights where theyre literally connected to all others. and we step it multiple
  times" (later changed to local reach, see 11-24)
- "stop with the tanh lol ... couldnt we just have a cooldown period in steps. also binary
  right"
- "we can do active balancing ... if in a step activity is too low, increase excitation"
- "we can also let neurons move their connections live"
- "in real neurons growth and exploration of new connections implicitly has feature
  locality"
- "why do we need a population. just one nn no? were trying to respect the hardware"
- "brain probably has sectors so activity regulation has locality. and maybe some features
  require higher activity to find like higher time resolution features"
- "we want all the rich mechanics of the brain but in a way thats super efficient and
  respects hardware. so were going for like bitmasks"
- Reward: "needs to be rich and efficient. and easy. simple"
- "instantiation needs to be varied in all params across the network"
- The stored per-synapse tag was rejected as "not simple". Credit now uses the per-neuron
  "fired recently" bit.

## 2026-11-24

- "the goal here is in some sort of minimally sufficient way but super hardware respecting
  efficient way to get a bunch of mechanisms that real brains have"
- "this thing should somehow just clock nonstop ... i want a persistent brain i just pump
  input into"
- "a blob that has locality, can rediscover new, has a sense of time, does gradient free
  learning, feature locality"
- Wants: quiet phases, skewed distributions, the second broadcast signal, delays ("so we can
  sense distance"), dendrite branches.
- "if we train it on this big curriculum of varied stimuli, games, goals, tasks,
  recognition, mirroring, etc then we should be able to find a generally learning blob"
- "we need a visualizer for this so i can see whats going on ... dying or too much activity
  ... too self similar too cyclic"
- "some games on motor control would be good. the classic rl envs too. counting, balance,
  flying, walking, jumping"
- "once we have sufficient mechanism to learn arbitrary patterns we just match it to the
  hardware in some super efficient way. even if its kind of a strange version of the
  biological equivalent ... i cant emphasize hardware efficiency enough"
- "im thinking c+- and vulkan"
- "why do we need all to all. dont neurons only need to reach nearby" -> local tiles.
- Far links: "i figured it would just have a distribution of these" -> drawn, not a table.
- "if we can do local compute and tiles we can put this on a fpga or into the gpu
  efficiently ... the goal here is to have architectural flexibility in all possible ways
  features can appear, in time, shape, spatial arrangement, for any somatosensory domain"
- "were gonna have to think about how this thing doesnt destroy itself by running without
  input" -> "Running with no input" in the design; idle check in the plan.
- Wants a full diagram: neurons and connections color coded, the mechanism list, and how
  each is made ideal for the machine -> `docs/diagram.html`.
- "ganglion is too common" as a name. Then: "lets call it thonksheet". Named.
- On the first diagram: "the sensor sheet looks like its barely connected to the other
  internal tiles ... im wondering about bandwidth per time" -> sensor cells are now a
  lattice spread through an input area ("Where input lands" in the design).
- "regarding the delays. i dont know if delay ends up being extremely important ...
  everything in the world is some form of time pattern" -> every slot has a delay, and a
  genome with all delays at 1 stays valid so it can be measured.
- The viewer was first modelled on foolswud. Owner: "foolswud is a tiny little thing. i would
  really rather we had focused on mans in my house something i put tons of work into" ->
  Mans In My House is the reference for structure, renderer and libraries. Never foolswud.
- "would motor neurons also be scattered around?" -> yes: motor cells are a lattice through an
  output area, like the sensors (`motor_stride`).
- "layers would be good i think" and "our neuron count is VERY LOW ... are we really running
  just one sheet?" -> both queued in `docs/plan.md` right behind the first learning gate.
- "is this even worth trying. we still havent run anything yet ... maybe theres a simpler more
  elegant design" -> freeze the design, run Echo, cut by ablation, add rivals. He agreed:
  "dumb rival sounds kinda cool", "simpler design okay".
- The general goal, confirmed ("this is correct yes"): find out what makes general learning
  possible in a brain-like machine and build the smallest efficient version; the sheet and
  Echo are a vehicle. Written at the top of `docs/bench.md`.
- The method: "we should be perhaps running more bots, giving them each less of this full
  machine. you ideate some things. they implement. they run a suite of tests. you analyze,
  see if anything comes of it, and keep exploring other experiments." -> `docs/bench.md`.
- "come up with a real suite / curriculum of these envs ... as we solve basic ones we need to
  graduate ... beat mnist ... the general goal here is, to find a set of rules that works on
  any task" -> `docs/task-map.md` (kinds of task, tiers, milestones, holes); ideas 115, 116.
- "i didnt say it needs to be never taught wtf? the whole point is it should be able to learn
  new things right? run nonstop. online learning for any task" -> frozen-sheet runs are a
  diagnostic only; `docs/bench.md` "Nothing frozen"; the findings headline reworded.
- "env ideas shape identification. square. triangle. lines dot. object tracking." -> idea 136,
  the seeing battery, queued behind the invariance battery.
