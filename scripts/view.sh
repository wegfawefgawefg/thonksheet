#!/usr/bin/env bash
# Open the viewer. With no arguments: the real blob with random input.
#   scripts/view.sh                 real blob, random bits on the sensor cells
#   scripts/view.sh --real          real blob, nothing plugged in
#   scripts/view.sh --fake mixed    the viewer's own fake blob (dying, saturating, cycling)
# Agents must not run this: it opens a window. Use scripts/offscreen.sh or --capture.
set -euo pipefail
cd "$(dirname "$0")/.."
if [ -n "${CLAUDECODE:-}" ] && [ -z "${THONK_ON_SCREEN:-}" ]; then
    echo "view.sh opens a window on the real display; agents use scripts/offscreen.sh" >&2
    exit 1
fi
scripts/build.sh
if [ $# -eq 0 ]; then set -- --real --input; fi
exec ./build/blobview "$@"
