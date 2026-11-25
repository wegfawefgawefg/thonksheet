#!/usr/bin/env bash
# Run things on stevejobs (the borrowed Mac mini, 14 cores) instead of this PC.
#   scripts/mini.sh push                 copy the source there (to ~/thonksheet) and build it
#   MINI_DIR=thonk-myjob scripts/mini.sh push      the same, into its own folder (agents)
#   scripts/mini.sh run 'scripts/suite.sh core --engine minimal'     run one command there
#   scripts/mini.sh bg NAME 'command'    start it in the background there; log in out/NAME.log
#   scripts/mini.sh status               what is running there and the load
#   scripts/mini.sh pull                 bring its out/ folder back into out/mini/ here
# The viewer is not built there. Someone else also uses that machine: keep to 10 jobs.
set -euo pipefail
cd "$(dirname "$0")/.."
dir="${MINI_DIR:-thonksheet}"
remote="export PATH=\$HOME/Library/Python/3.9/bin:\$PATH; cd ~/$dir"
case "${1:-}" in
push)
    rsync -a --delete --exclude /build --exclude '/build-*' --exclude /out --exclude /.git \
        --exclude /.claude --exclude /captures --exclude /third_party --exclude /.cache \
        ./ "mini:~/$dir/"
    ssh mini "$remote && { [ -f build/CMakeCache.txt ] || cmake -S . -B build -DTHONK_NO_VIEW=1 >/dev/null; } && cmake --build build -j8 2>&1 | grep -E 'error|warning: unused|Linking' | tail -20"
    ;;
run)
    shift
    ssh mini "$remote && $*"
    ;;
bg)
    name="$2"
    shift 2
    ssh mini "$remote && mkdir -p out && nohup sh -c '$*' > out/$name.log 2>&1 < /dev/null & echo started $name"
    ;;
status)
    ssh mini 'uptime; ps -axo pcpu,etime,command | grep -E "blobrun|blobprobe|minimalprobe|tablebench|suite.py|sweep.py|richness" | grep -v grep | cut -c1-150 | head -20'
    ;;
pull)
    mkdir -p out/mini
    rsync -a "mini:~/$dir/out/" out/mini/
    ;;
*)
    sed -n '2,9p' "$0"
    exit 1
    ;;
esac
