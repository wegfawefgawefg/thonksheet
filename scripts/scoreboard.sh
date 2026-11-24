#!/usr/bin/env bash
# Join every scorecard in out/scorecards/ into docs/scoreboard.md, one column per engine or
# variant. Quick smoke runs (*-quick) are left out unless named.
#   scripts/scoreboard.sh                     all full scorecards
#   scripts/scoreboard.sh blob-main no-pulse  only these
set -euo pipefail
cd "$(dirname "$0")/.."
exec python3 scripts/scoreboard.py "$@"
