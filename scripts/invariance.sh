#!/usr/bin/env bash
# The invariance battery (docs/curriculum.md, "Invariance"; results:
# docs/results-2026-11-25-invariance-battery.md). Every row shows a shape (or tune, or 3D
# object) in versions of one change, rewards some versions and scores only the others. Three
# levels: (a) can a linear reader of the sheet as born carry it over, (b) does the evidence
# readout on a frozen sheet, (c) does the whole engine learning. Seeds 5-12, each beside its
# learning-off twin and coin flips; 2 runs at a time under nice.
#   scripts/invariance.sh blob                      full engine: levels a, b, c (about 2 hours)
#   scripts/invariance.sh blob --quick              2 seeds, short lives: a smoke check
#   scripts/invariance.sh minimal --engine minimal  another engine: levels a and c
#   scripts/invariance.sh table --table
#   scripts/invariance.sh myidea --g.some_number 3  a variant: flags go to every run, last
#   scripts/invariance.sh blob --levels a           one level only; --resume keeps finished runs
#   scripts/invariance.sh logpolar --levels a --bitmap-map logpolar     the log-polar map
# Writes out/scorecards/invariance-<name>.csv and prints the table.
#
# To see the tasks (no blob; prints every shape in every version, * marks held out):
#   scripts/invariance.sh show scale               any of position scale rotation bright clutter
#   scripts/invariance.sh show view --inv-motion 50     mixed view shift tempo pitch
# and on the sheet, in the viewer (owner only; it opens a window):
#   scripts/watch.sh inv-rotation --inv-motion 50
#   scripts/invariance.sh gru --engine gru          the fixed reference networks: level c only
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun blobprobe
if [ "${1:-}" = show ]; then
    kind="${2:-scale}"
    shift 2 || shift $#
    side=1
    [ "$kind" = shift ] || [ "$kind" = tempo ] && side=3
    exec ./build/blobprobe invariance --show 1 --task "inv-$kind" --ticks 20 --sensor-side "$side" "$@"
fi
exec python3 scripts/invariance.py "$@"
