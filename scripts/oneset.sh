#!/usr/bin/env bash
# One setting on the whole quick map: every world in scripts/variants/oneset.txt, seeds 5-12,
# each life beside its learning-off twin, one table (late score, twin, chance, perfect, how far
# from chance to perfect, and whether the row passes). Results: docs/results-2026-11-26-one-setting.md.
#   scripts/oneset.sh base                       a column named in oneset.txt or best.txt
#   scripts/oneset.sh best                       the one setting
#   scripts/oneset.sh mine --g.learn_mode 5 --g.readout_evidence 2 --motor-spread 128   any flags
#   scripts/oneset.sh base --seeds 1-4 --jobs 2  the tuning seeds
#   scripts/oneset.sh base --rows xor,order      some rows
#   scripts/oneset.sh table base,gru,memory,best side by side, each against the first
#   JOBS=6 scripts/oneset.sh best                more at once (stevejobs; 2 on this PC)
# A pass of one column is 22 worlds x 8 seeds x 2 (life and twin), 40,000 steps each
# (20,000 digits for MNIST): run it on stevejobs (MINI_DIR=thonk-oneset scripts/mini.sh push).
# Lives are cached in out/oneset/, so a rerun only runs what is missing.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -x build/blobrun ] || scripts/build.sh --target blobrun
[ -d data/mnist ] || [ -n "${THONK_MNIST:-}" ] || echo "no data/mnist: scripts/get_mnist.sh" >&2
name="${1:?usage: scripts/oneset.sh NAME [--seeds 5-12] [--rows a,b] [flags ...]}"
shift
if [ "$name" = table ]; then
    exec python3 scripts/oneset.py table "$@"
fi
opts=()
flags=()
while [ $# -gt 0 ]; do
    case "$1" in
    --seeds | --rows | --jobs) opts+=("$1" "$2"); shift 2 ;;
    --no-twin) opts+=("$1"); shift ;;
    *) flags+=("$1"); shift ;;
    esac
done
exec python3 scripts/oneset.py run "$name" --jobs "${JOBS:-2}" ${opts[@]+"${opts[@]}"} -- ${flags[@]+"${flags[@]}"}
