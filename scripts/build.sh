#!/usr/bin/env bash
# Build everything, gently.
set -euo pipefail
cd "$(dirname "$0")/.."
[ -f build/CMakeCache.txt ] || cmake -S . -B build >/dev/null
nice -n 19 cmake --build build -j4 "$@" | grep -E 'error|warning|Linking' || true
