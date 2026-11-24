# Prior art

Written from memory on 2026-11-24, not from a fresh search. Check a claim before leaning on
it. Every piece of this design has been tried somewhere; the whole has not, as far as we know.

- **Hierarchical Temporal Memory (Numenta).** Binary neurons, dendrite branches with their
  own thresholds, synapses that grow toward recently active cells, always learning, no
  backprop. Closest on mechanisms. It learned sequences and anomalies but never got
  reward-driven behavior working and never matched deep networks.
- **Reward-gated spike-timing learning** (Izhikevich 2007 and after). A broadcast reward
  acting on recently used synapses. Our good/bad signal. Shown on small tasks.
- **Synaptic tagging** (Frey and Morris 1997). The biology behind "recently used, then a
  later signal decides".
- **Few-state synapses with random flips** (Amit and Fusi 1994; the cascade model, 2005).
  Why a low chance of change gives long memory, and why a hidden "hard to change" state
  helps.
- **Evolved learning rules** (Najarro and Risi 2020, and others). Evolution finds the
  plasticity rule; the network starts random and learns within one life. It drove a racing
  car and walked a quadruped. Small nets, floats, one task each. The best evidence for the
  genome idea.
- **Tsetlin machines and weightless networks.** Learning with bit operations and chance
  updates. Classification only.
- **Reservoir computing.** A fixed random recurrent net with a trained readout. Shows a
  free-running blob holds useful time information.
- **Dynamic sparse training** (deep rewiring, RigL). Prune and regrow at a fixed connection
  count, with backprop.
- **Streaming and continual reinforcement learning** (Sutton's group, Carmack's work). The
  same always-on, no-episode goal, with backprop.
- **Loss of plasticity** (Dohare et al. 2024). Networks trained forever stop being able to
  learn; replacing little-used units fixes it. Our growth and pruning play that role.
- **Neuromorphic chips** (Loihi, SpiNNaker). The same hardware-first idea on special
  hardware. We aim at ordinary CPUs and GPUs.

What nobody has shown: a bit-packed, free-running, self-wiring blob whose genome is searched
over a broad curriculum, learning arbitrary tasks. That is either the opening or the warning.
