#!/bin/sh
# Runs the windowed blobview inside a private Xvfb, never on the real display. For testing
# the window path (input, resize, present); for pictures of what it draws, --capture is
# simpler and needs no X at all.
#
#   scripts/offscreen.sh [options] [-- blobview arguments]
#     --lavapipe        software Vulkan (LP_NUM_THREADS=2) with validation; the default
#     --gpu             the real GPU instead (keep runs short)
#     --shot out.png    a screenshot of the Xvfb screen after --after seconds
#     --after S         (default 6)
#     --click "X Y"     a left click at X Y (window pixels) before the shot; repeatable
#     --key K           a key (xdotool name, e.g. space, e, t) before the shot; repeatable
#     --resize "W H"    resize the window before the shot; repeatable
#     --seconds S       how long blobview may run (default 12); it is killed after
#     --size WxH        the Xvfb screen and window (default 1600x1000)
#
# Example: scripts/offscreen.sh --shot captures/window.png --click "330 300" -- --fake mixed
set -e
cd "$(dirname "$0")/.."

lavapipe=1
shot=""
after=6
seconds=12
size=1600x1000
actions=""
while [ $# -gt 0 ]; do
    case "$1" in
        --lavapipe) lavapipe=1 ;;
        --gpu) lavapipe=0 ;;
        --shot) shift; shot="$1" ;;
        --after) shift; after="$1" ;;
        --click) shift; actions="$actions|click $1" ;;
        --key) shift; actions="$actions|key $1" ;;
        --resize) shift; actions="$actions|resize $1" ;;
        --seconds) shift; seconds="$1" ;;
        --size) shift; size="$1" ;;
        --) shift; break ;;
        *) break ;;
    esac
    shift
done
width=${size%x*}
height=${size#*x}

# a display number nobody is using
display=""
for n in 91 92 93 94 95 96 97 98 99; do
    if [ ! -e "/tmp/.X11-unix/X$n" ] && [ ! -e "/tmp/.X$n-lock" ]; then
        display=":$n"
        break
    fi
done
[ -n "$display" ] || { echo "offscreen: no free display number" >&2; exit 1; }

Xvfb "$display" -screen 0 "${width}x${height}x24" -nolisten tcp >/dev/null 2>&1 &
xvfb_pid=$!
trap 'kill $xvfb_pid 2>/dev/null || true' EXIT INT TERM
sleep 1

# only the private display from here on; no audio
export DISPLAY="$display"
unset WAYLAND_DISPLAY
export SDL_VIDEO_DRIVER=x11
export SDL_AUDIO_DRIVER=dummy
validation=""
if [ "$lavapipe" = 1 ]; then
    export VK_ICD_FILENAMES=/usr/share/vulkan/icd.d/lvp_icd.json
    export LP_NUM_THREADS=2
    validation="--validation"
fi

timeout "$seconds" nice -n 19 ./build/blobview --size "$width" "$height" $validation "$@" &
view_pid=$!

if [ -n "$shot" ] || [ -n "$actions" ]; then
    sleep "$after"
    # the clicks and keys, in order, to the blobview window
    echo "$actions" | tr '|' '\n' | while read -r kind a b; do
        case "$kind" in
            click) xdotool mousemove "$a" "$b" click 1; sleep 0.5 ;;
            resize) xdotool search --name thonksheet windowsize "$a" "$b"; sleep 1.5 ;;
            key) xdotool search --name thonksheet windowactivate --sync key "$a" 2>/dev/null ||
                     xdotool key "$a"; sleep 0.5 ;;
        esac
    done
    sleep 1
    if [ -n "$shot" ]; then
        import -display "$display" -window root "$shot"
        echo "offscreen: wrote $shot"
    fi
fi
wait $view_pid || true
