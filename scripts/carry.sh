#!/usr/bin/env bash
# How far does a cue carry across the sheet? One tile's sensor cells are split into cue A
# and cue B; in each block of 20 ticks one of them is on, with no reward. Each column is
# the distance in tiles from the cue (0 is the cue's own tile); the number is how well A
# and B can be told apart from the neurons there (1.0 easy, under 0.3 lost). "reach" is
# the farthest distance still readable.
#   scripts/carry.sh            the standard set of variants
#   scripts/carry.sh --seed 2
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh
p() { nice -n 19 ./build/blobprobe carry "$@"; }
echo "distance in tiles:           0     1     2     3     4     5     6     7"
p --label "baseline, learning off" --g.learning_on 0 "$@"
p --label "baseline, learning on" "$@"
p --label "long life first (600k)" --settle 600000 "$@"
p --label "no pulse (slow sectors)" --g.sector_smooth_shift 6 "$@"
p --label "inhibit gain 2" --g.inhibit_gain 2 "$@"
p --label "inhibit gain 3" --g.inhibit_gain 3 "$@"
p --label "denser wiring at birth" --g.local_connect_shift 2 "$@"
p --label "more strong at birth" --g.strong_shift 1 "$@"
p --label "branch bonus 6" --g.branch_bonus 6 "$@"
p --label "less noise" --g.noise_shift 13 "$@"
p --label "lower activity target" --g.sector_target_per_1024 16 "$@"
p --label "higher activity target" --g.sector_target_per_1024 64 "$@"
p --label "half the neurons far" --g.far_percent 50 "$@"
p --label "all delays 1" --g.local_delay_min 1 --g.local_delay_max 1 "$@"
p --label "no prediction learning" --g.predict_shift 30 "$@"
p --label "no growth or pruning" --g.grow_shift 30 --g.prune_shift 30 "$@"
