#!/usr/bin/env bash
# Why does the blob pulse? Change one thing at a time and measure the period.
# Each line is one life: 30,000 ticks to settle, then 65,536 ticks measured.
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh
p() { nice -n 19 ./build/blobprobe "$@"; }
echo "== with random input"
p --label "baseline"
p --label "idle (no input)" --input 0
p --label "sheet 16x16 tiles" --tiles 16
p --label "sheet 4x4 tiles" --tiles 4 --g.sensor_w 16 --g.sensor_h 16 --g.motor_x 0 --g.motor_y 16 --g.motor_w 16 --g.motor_h 16
p --label "all local delays 1" --g.local_delay_min 1 --g.local_delay_max 1
p --label "local delays 4..8" --g.local_delay_min 4 --g.local_delay_max 8
p --label "sector smoothing 4 ticks" --g.sector_smooth_shift 2
p --label "sector smoothing 64 ticks" --g.sector_smooth_shift 6
p --label "sector bias off" --g.sector_bias_max 0
p --label "cooldown 1..1" --g.cooldown_min 1 --g.cooldown_max 1
p --label "cooldown 6..8" --g.cooldown_min 6 --g.cooldown_max 8
p --label "no pacemakers" --g.pacemakers_per_1024 0
p --label "inhibit gain 2" --g.inhibit_gain 2
p --label "inhibit gain 8" --g.inhibit_gain 8
p --label "learning off" --g.learning_on 0
