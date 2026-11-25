#!/usr/bin/env bash
# Search for what makes a sheet rich to read: hundreds of random sheets, each judged by how
# readable order, exclusive-or, gap and "which cue" are from its neurons, with no learning.
#   scripts/richness.sh run --samples 300     draw and measure (adds to out/richness/samples.csv)
#   scripts/richness.sh report                which numbers matter, and the best sheets
#   scripts/richness.sh check --top 10        re-measure the best sheets on new seeds
set -euo pipefail
cd "$(dirname "$0")/.."
[ "${1:-}" = report ] || scripts/build.sh --target blobprobe
exec python3 scripts/richness_search.py "$@"
