#!/usr/bin/env bash
# Tick speed on one core at several sizes.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh
for tiles in 4 8 16 32; do
    nice -n 19 ./build/blobbench speed --tiles "$tiles" "$@"
done
