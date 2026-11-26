#!/bin/sh
# The README's viewer pictures, into docs/img/. Each is one frame from blobview's capture
# mode: no window, no display, software Vulkan (lavapipe, 2 threads), one run at a time, a
# few seconds each. The PNG is then cut to 256 colours (scripts/figures.py shrink) to stay
# small. Safe for agents: nothing is opened on any screen.
#
#   scripts/captures.sh                 all four
#   scripts/captures.sh random-input    docs/img/viewer-random-input.png
#   scripts/captures.sh delayed-echo    docs/img/viewer-delayed-echo.png
#   scripts/captures.sh xor-readout     docs/img/viewer-xor-readout.png
#   scripts/captures.sh probe-which     docs/img/viewer-probe-which.png
set -e
cd "$(dirname "$0")/.."
[ -x build/blobview ] || scripts/build.sh --target blobview >/dev/null
mkdir -p docs/img

# every task here: 20 ticks a step, cue and button groups of 3 by 3 cells
TASK="--ticks 20 --sensor-side 3 --motor-side 3"
# the one setting (scripts/variants/best.txt)
BEST="--g.learn_mode 5 --g.readout_evidence 2 --g.readout_shift 4 --g.readout_balance 1 \
--motor-spread 128 --g.time_percent 10 --g.hold_percent 10 --g.readout_good 1 \
--g.margin_percent 20 --g.margin_win 2 --g.tire_jump 4 --g.tire_shift 7 --g.tile_winners 2 \
--g.replay_depth 1 --g.replay_bad_only 1"
# the output rule alone (scripts/readout.sh's BEST, as docs/notes-viewer.md has it)
READOUT="--g.learn_mode 5 --g.readout_evidence 1 --g.readout_shift 4 --g.readout_balance 1 \
--motor-spread 128"

capture() {
    name="$1"
    shift
    env -u DISPLAY -u WAYLAND_DISPLAY VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json \
        LP_NUM_THREADS=2 SDL_AUDIO_DRIVER=dummy timeout 300 nice -n 19 ./build/blobview \
        --capture "docs/img/viewer-$name.png" "$@"
    python3 scripts/figures.py shrink "docs/img/viewer-$name.png"
}

one() {
    case "$1" in
        # the sheet with random bits on its sensor cells, tick 3,000, an excitatory cell
        # picked: its sources ringed, its numbers in the neuron panel
        random-input) capture random-input --input --until 3000 --select-cell 21 13 ;;
        # delayed echo (answer 2 steps after the cue) under the one setting, step 5,000 of
        # a life: the task marks on the sheet, the task panel with its score per window
        delayed-echo) capture delayed-echo --task delayed-echo --delay 2 $TASK --window 200 \
            $BEST --until 100000 ;;
        # exclusive-or under the output rule alone, step 10,000, a cell of button 1 picked:
        # the readout panel and the cell's evidence picture on the sheet
        xor-readout) capture xor-readout --task both-not-either $TASK --window 500 $READOUT \
            --until 200000 --select-button 1 ;;
        # the readability probe as the world: how well "which cue" can be read, tile by tile
        probe-which) capture probe-which --probe which --until 60000 ;;
        *) echo "captures: no picture '$1' (random-input, delayed-echo, xor-readout," \
            "probe-which)" >&2; exit 1 ;;
    esac
}

[ $# -gt 0 ] || set -- random-input delayed-echo xor-readout probe-which
for name in "$@"; do
    one "$name"
done
