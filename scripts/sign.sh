#!/usr/bin/env bash
# Idea 57: why credit by noise learns the exact opposite, in all three engines.
# Results: docs/results-2026-11-24-noise-sign.md. Two runs at a time, nice'd; logs land in
# out/sign/.
#   scripts/sign.sh probe          the table sheet's diagnosis on seeds 1-4 (tableprobe):
#                                  as built, reversed, the oracles, the random-sample rules
#   scripts/sign.sh probe-one 1    one diagnosed life (seed 1), with the first 40 rewards
#   scripts/sign.sh scores table   every version on echo k=2, k=4, shuffled echo and cue and
#   scripts/sign.sh scores blob    buttons two tile rows apart, seeds 5-12, with twins
#   scripts/sign.sh scores minimal
#   scripts/sign.sh summary        the score tables from what has been run
set -euo pipefail
cd "$(dirname "$0")/.."
scripts/build.sh
common=(--ticks 20 --sensor-side 3 --motor-side 3 --steps 40000 --window 1000)
k4=(--t.inputs 4 --t.bad_rise_shift 0 --t.reward_shift 0) # the sheet the noise rules ran on

task_flags() { # the four tasks
    case $1 in
    echo2) echo --task echo --k 2 ;;
    echo4) echo --task echo --k 4 ;;
    shuffled) echo --task shuffled-echo --k 2 ;;
    apart2) echo --task echo --k 2 --g.sensor_h 8 --g.motor_y 16 --g.motor_h 8 ;;
    esac
}

# version name, engine flags, and whether this version also runs the learning-off twin
versions() {
    case $1 in
    table)
        echo "default|--table|1"
        echo "plain_k4|--table --t.inputs 4 --t.bad_rise_shift 0|1"
        echo "noise_as_built|--table ${k4[*]} --t.learn_mode 1|0"
        echo "noise_reversed|--table ${k4[*]} --t.learn_mode 2|0"
        echo "noise_fixed|--table ${k4[*]} --t.learn_mode 3|0"
        echo "noise_fixed_n9|--table ${k4[*]} --t.learn_mode 3 --t.noise_shift 9|1"
        ;;
    blob)
        echo "default||1"
        echo "noise_as_built|--g.learn_mode 3 --g.noise_learn_shift 4|0"
        echo "noise_reversed|--g.learn_mode 3 --g.noise_learn_shift 4 --g.noise_learn_sign -1|0"
        echo "noise_fixed|--g.learn_mode 3 --g.noise_learn_shift 4 --g.noise_learn_pair 1|0"
        ;;
    minimal)
        echo "default|--engine minimal|1"
        echo "noise_reversed|--engine minimal --m.noise_sign -1|0"
        echo "noise_fixed|--engine minimal --m.noise_pair 1|0"
        echo "plain|--engine minimal --m.grade_mode 1|0"
        ;;
    esac
}

scores() { # scores ENGINE
    local engine=$1
    versions "$engine" | while IFS='|' read -r name flags twin; do
        for task in echo2 echo4 shuffled apart2; do
            local dir="out/sign/$engine/$name/$task"
            mkdir -p "$dir"
            local base=(--no-baselines)
            [ "$twin" = 1 ] && base=()
            for s in $(seq 5 12); do
                [ -s "$dir/s$s.log" ] && grep -q "^csv:" "$dir/s$s.log" && continue
                # shellcheck disable=SC2046,SC2086
                nice -n 19 ./build/blobrun $(task_flags "$task") "${common[@]}" $flags \
                    "${base[@]}" --seed "$s" --out "$dir/s$s.csv" >"$dir/s$s.log" 2>&1 &
                while [ "$(jobs -rp | wc -l)" -ge 2 ]; do wait -n; done
            done
        done
        wait
    done
}

probe() { # probe LABEL TASK flags...: tableprobe on seeds 1-4
    local label=$1 task=$2
    shift 2
    local dir="out/sign/probe/$label"
    mkdir -p "$dir"
    for s in 1 2 3 4; do
        nice -n 19 ./build/tableprobe --task "$task" "${common[@]}" --seed "$s" "$@" \
            >"$dir/s$s.log" 2>&1 &
        while [ "$(jobs -rp | wc -l)" -ge 2 ]; do wait -n; done
    done
    wait
    printf "%-16s" "$label"
    for s in 1 2 3 4; do
        grep "window score" "$dir/s$s.log" | sed 's/.*score \([0-9.]*\).*/\1/' | tail -4 |
            awk '{a += $1} END {printf " %.3f", a / NR}'
    done
    echo
}

case "${1:-}" in
probe)
    e=(--k 2 "${k4[@]}")
    probe as_built echo "${e[@]}" --t.learn_mode 1
    probe reversed echo "${e[@]}" --t.learn_mode 2
    probe fixed echo "${e[@]}" --t.learn_mode 3
    probe motor_only echo "${e[@]}" --t.learn_mode 1 --p.motor_only 1
    probe inner_only echo "${e[@]}" --t.learn_mode 1 --p.classes 8
    probe fired_only echo "${e[@]}" --t.learn_mode 1 --p.ways 1
    probe silenced_only echo "${e[@]}" --t.learn_mode 1 --p.ways 2
    probe cue_baseline echo "${e[@]}" --t.learn_mode 1 --p.cue_baseline 1
    probe causal_only echo "${e[@]}" --t.learn_mode 1 --p.causal_only 1
    probe sample_plain echo "${e[@]}" --t.learn_mode 1 --p.pair 1 --p.flips 0
    probe sample_anti echo "${e[@]}" --t.learn_mode 2 --p.pair 1 --p.flips 0
    ;;
probe-one)
    exec nice -n 19 ./build/tableprobe --task echo --k 2 "${common[@]}" "${k4[@]}" \
        --t.learn_mode 1 --seed "${2:-1}" --p.detail 40
    ;;
scores)
    scores "${2:-table}"
    ;;
summary)
    for e in table blob minimal; do
        [ -d "out/sign/$e" ] && python3 scripts/sign_summary.py "$e"
    done
    ;;
*)
    sed -n '2,13p' "$0"
    exit 1
    ;;
esac
