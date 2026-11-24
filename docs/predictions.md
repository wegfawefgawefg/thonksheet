# Predictions

Written down before the results come in, so we can tell learning from hindsight. Each one
has a rough confidence and gets marked right, wrong or unclear when its result lands. Add
new predictions before starting an experiment, never after.

## 2026-11-24, before the ablation sweep, the rivals and table neurons

1. **Most mechanisms do nothing measurable on Echo-level tasks.** Branch bonus, pacemakers,
   surprise boost, far links and prediction learning can each be switched off with no change
   in score. (75%)
2. **What does matter:** inhibition and regulation (staying alive), growth and pruning, and
   the good/bad rule itself. (75%)
3. **A frozen blob with a learned readout matches or beats the full blob on Echo.** The
   inner learning is not yet helping the task. (65%)
4. **Noise-is-the-experiment saturates less and does better on 4 cues, but learns slower**
   than the current rule, because far fewer events get graded. (55%)
5. **Random flips solve cartpole, slowly.** (60%) **Some online brain-style rule solves
   cartpole in this round.** (35%)
6. **Table neurons learn Echo faster and sharper than the popcount blob, and carry a cue
   worse** until their wiring is tuned. (50%)
7. **Delays matter for the timing tasks** (beat, delayed echo) **and not for Echo.** (70%)
8. **Removing the pulse helps a little and never hurts.** (60%)

## The larger guess

The core that ends up working will be three things: sparse activity that stays stable;
wiring by co-activity, which needs no reward and builds reach and features; and a sharp,
sparse credit rule close to the output, fed by something that pools thinly spread signals.
In other words a sheet that organises itself, with reinforcement mostly at the edge.

Where most ideas will fail: teaching neurons deep inside the sheet from reward alone. That
is the hard, unsolved part. The find worth the whole exercise would be any rule that makes
inner neurons measurably better for a task than unsupervised wiring already makes them.
