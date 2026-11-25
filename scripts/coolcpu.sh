#!/usr/bin/env bash
# Keep the CPU at or under 90C by limiting how many cores the agents' jobs may use.
#   scripts/coolcpu.sh          run it (logs to out/coolcpu.log)
#   scripts/coolcpu.sh --off    give every job all cores back
cd "$(dirname "$0")/.."
exec nice -n 5 python3 -I scripts/coolcpu.py "$@"
