#!/usr/bin/env bash
# The long search: try many settings of every engine, each one setting used unchanged on
# every world of the quick map, keep what works. Resumable; nothing is ever run twice.
# How it works and how to read it: docs/search.md. Results: out/search/<name>/.
#
# On this PC (always nice -n 19; default 2 jobs; Ctrl-C stops it and loses only runs in flight):
#   scripts/search.sh run                        run the main search until stopped
#   scripts/search.sh run --jobs 2 --hours 8     stop by itself after 8 hours
#   scripts/search.sh status                     how far it is, the best so far
#   scripts/search.sh report                     write docs/search-leaderboard.md
#   scripts/search.sh stop                       finish the runs in flight, then stop
#   scripts/search.sh check minimal              is a space file right (names, learn tags)
#   scripts/search.sh merge out/search/other     add another folder's results to main
# Any of them takes --name NAME for a search other than main. A first run may also take
# --map FILE, --engines blob,minimal,table and --starts FILE|none; they are kept.
#
# On the second machine (stevejobs, in its own folder ~/thonk-search; at most 8 jobs; it
# keeps running when the ssh session ends):
#   scripts/search.sh mini start [--jobs 8] [--hours 48]   copy the code there, build, start
#   scripts/search.sh mini status                          load, progress, best so far
#   scripts/search.sh mini stop                            finish runs in flight, then stop
#   scripts/search.sh mini kill                            stop now (runs in flight lost)
#   scripts/search.sh mini pull                            bring its results here and merge
#   scripts/search.sh mini log                             the last lines of its log
# To add a world or a number to a running search: edit scripts/search/quick-map.txt or a
# space file here, then `mini stop`, wait until `mini status` says not running, `mini start`.
set -euo pipefail
cd "$(dirname "$0")/.."

name=main
args=("$@")
for ((i = 0; i < ${#args[@]}; i++)); do
    if [ "${args[$i]}" = "--name" ]; then name="${args[$((i + 1))]}"; fi
done

case "${1:-}" in
run)
    shift
    [ -x build/blobrun ] || scripts/build.sh
    exec nice -n 19 python3 scripts/search.py run "$@"
    ;;
status | report | stop | check | merge)
    exec python3 scripts/search.py "$@"
    ;;
mini)
    shift
    dir="${MINI_DIR:-thonk-search}"
    remote="export PATH=\$HOME/Library/Python/3.9/bin:\$PATH; cd ~/$dir"
    what="${1:-}"
    shift || true
    case "$what" in
    start)
        jobs=8
        hours=48
        rest=()
        while [ $# -gt 0 ]; do
            case "$1" in
            --jobs) jobs="$2"; shift 2 ;;
            --hours) hours="$2"; shift 2 ;;
            --name) shift 2 ;;
            *) rest+=("$1"); shift ;;
            esac
        done
        if [ "$jobs" -gt 8 ]; then
            echo "at most 8 jobs on the second machine"
            exit 1
        fi
        if ssh mini "$remote 2>/dev/null && [ -f out/search/$name/run.pid ] && kill -0 \$(cat out/search/$name/run.pid) 2>/dev/null"; then
            echo "search $name is already running there; mini stop first"
            exit 1
        fi
        ssh mini uptime
        MINI_DIR="$dir" scripts/mini.sh push
        commit="$(git rev-parse --short HEAD 2>/dev/null || echo unknown)"
        ssh mini "$remote && mkdir -p out/search/$name && nohup caffeinate -i python3 scripts/search.py run --name $name --jobs $jobs --hours $hours --commit $commit ${rest[*]:-} >> out/search/$name/nohup.log 2>&1 < /dev/null & echo started search $name there, $jobs jobs, $hours hours"
        ;;
    status)
        ssh mini "uptime; $remote && python3 scripts/search.py status --name $name"
        ;;
    log)
        ssh mini "$remote && tail -n 30 out/search/$name/run.log"
        ;;
    stop)
        ssh mini "$remote && python3 scripts/search.py stop --name $name"
        ;;
    kill)
        ssh mini "$remote && kill -TERM \$(cat out/search/$name/run.pid) && echo sent"
        ;;
    pull)
        mkdir -p "out/search/mini-$name"
        rsync -a --exclude tmp/ "mini:~/$dir/out/search/$name/" "out/search/mini-$name/"
        python3 scripts/search.py merge "out/search/mini-$name" --name "$name"
        ;;
    *)
        sed -n '17,24p' "$0"
        exit 1
        ;;
    esac
    ;;
*)
    sed -n '2,28p' "$0"
    exit 1
    ;;
esac
