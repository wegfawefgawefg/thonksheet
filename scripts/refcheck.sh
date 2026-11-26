#!/usr/bin/env bash
# The four reference lives: with every optional mechanism off, a change to the engine must
# leave these lives byte-identical (tests/ref/, made 2026-11-25). Run after any merge:
#   scripts/refcheck.sh          prints "echo same  xor same  min same  tab same"
# A line that says DIFFERS means a default path changed; find out why before going on.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -x build/blobrun ] || scripts/build.sh --target blobrun
T=$(mktemp -d)
trap 'rm -r "$T"' EXIT
life="--steps 4000 --window 1000 --seed 5"
./build/blobrun --task echo $life --out "$T/echo.csv" >/dev/null 2>&1
./build/blobrun --task both-not-either $life --g.learn_mode 5 --g.readout_evidence 1 \
    --g.readout_shift 4 --g.readout_balance 1 --motor-spread 128 --out "$T/xor.csv" >/dev/null 2>&1
./build/blobrun --task echo --engine minimal $life --out "$T/min.csv" >/dev/null 2>&1
./build/blobrun --task echo --table $life --out "$T/tab.csv" >/dev/null 2>&1
bad=0
for k in echo xor min tab; do
    if cmp -s "tests/ref/ref-$k.csv" "$T/$k.csv"; then
        printf '%s same  ' "$k"
    else
        printf '%s DIFFERS  ' "$k"
        bad=1
    fi
done
echo
exit $bad
