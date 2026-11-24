#!/usr/bin/env bash
# The suite (docs/bench.md): every test in the bench table for one engine or variant, on
# the report seeds 5 to 12, each beside its learning-off twin and coin flips, 2 runs at a
# time under nice. Writes out/scorecards/<name>.csv and prints it as a table.
#   scripts/suite.sh blob-main                       the blob as it is
#   scripts/suite.sh blob-main --quick               2 seeds, short lives: a smoke check
#   scripts/suite.sh no-pulse --g.sector_smooth_shift 6       a variant: genome numbers
#   scripts/suite.sh table --table                   another engine, through a blobrun flag
#   scripts/suite.sh blob-main --resume              keep runs already done, do the rest
# Everything after the name goes to every blobrun call. Then scripts/scoreboard.sh joins
# all scorecards into docs/scoreboard.md. A full run is about an hour on 2 jobs.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh --target blobrun
exec python3 scripts/suite.py "$@"
