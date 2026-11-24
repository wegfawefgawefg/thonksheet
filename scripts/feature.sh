#!/usr/bin/env bash
# Does the sheet hold features nobody designed in? No reward anywhere. Each line is one
# life; the numbers are how readable the feature is from a tile's neurons at 0, 1, 2, 3, 4
# tiles from the cue (1.0 easy, under 0.3 lost; an ideal reader, so an upper bound).
#   which   cue A or cue B (the plain case)
#   order   A then B, or B then A, read after both are off
#   xor     exactly one of A and B, against both or neither
#   gap     two blinks with a short or long gap, read after the second
#   scripts/feature.sh            the standard set
#   scripts/feature.sh --seed 2
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobprobe
p() { nice -n 19 ./build/blobprobe feature "$@"; }
echo "feature variant                    tiles: 0      1      2      3      4"
for f in which order xor gap; do
    p --feature $f --label "as born, learning off" --g.learning_on 0 "$@"
    p --feature $f --label "learning on, no reward" "$@"
    p --feature $f --label "after 600k ticks" --settle 600000 "$@"
    p --feature $f --label "all delays 1" --g.local_delay_min 1 --g.local_delay_max 1 "$@"
    p --feature $f --label "delays 4 to 8" --g.local_delay_min 4 --g.local_delay_max 8 "$@"
    p --feature $f --label "no growth or pruning" --g.grow_on 0 --g.prune_on 0 "$@"
    echo
done
