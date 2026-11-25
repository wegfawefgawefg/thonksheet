#!/usr/bin/env bash
# The rest of the task map (docs/task-map.md tiers 9 and 10, being shown the answer, damage),
# every world scored three ways: (a) readable from the sheet as born, by the linear reader,
# beside the same reader on the raw input (the shortcut check); (b) learned by the readout rule
# on a frozen sheet; (c) the minimal engine and table neurons with their default rules. Each
# beside its learning-off twin, coin flips and the scripted perfect player. Worlds:
# scripts/map_worlds.py; definitions: docs/curriculum.md, "The rest of the map".
#   scripts/map.sh map                         everything, seeds 5-12 (2 runs at a time)
#   scripts/map.sh map --quick                 seeds 5 and 6, short lives
#   scripts/map.sh map --only same-easy,copy-hard      some worlds
#   scripts/map.sh map --levels b              only the readout rule
#   scripts/map.sh map --print                 print the table from runs already there
#   scripts/map.sh map --resume                keep finished runs, do the rest
#   scripts/map.sh rich --g.local_delay_min 4  flags after the name go to levels (a) and (b)
# Results: out/map/<name>/map.csv and the table on screen.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun blobprobe
exec python3 scripts/map.py "$@"
