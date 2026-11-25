#!/usr/bin/env python3
"""Keep the CPU at or under a temperature by limiting how many cores the agents' jobs use.

The owner: "keep the cpu temp at 90 or below pls". Every job the agents start is nice 19
and descends from one claude process, so this pins that process tree (and every nice-19
process of this user) to the first N physical cores, both threads of each, and moves N
down when the chip is hot and up when it is cool. Nothing is stopped; jobs share fewer
cores and run slower. New jobs inherit the limit from the process that starts them.

    scripts/coolcpu.sh              run it (stays in the foreground, logs to out/coolcpu.log)
    scripts/coolcpu.sh --off        give every job all cores back and exit

Needs no root. Does not touch the clock cap.
"""

import argparse
import glob
import os
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG = os.path.join(ROOT, "out", "coolcpu.log")


def temperature_file():
    for d in glob.glob("/sys/class/hwmon/hwmon*"):
        try:
            if open(os.path.join(d, "name")).read().strip() in ("k10temp", "zenpower", "coretemp"):
                return os.path.join(d, "temp1_input")
        except OSError:
            pass
    return None


def core_threads():
    # physical core -> its logical cpus, in core order
    cores = {}
    for d in glob.glob("/sys/devices/system/cpu/cpu[0-9]*"):
        try:
            core = int(open(os.path.join(d, "topology", "core_id")).read())
        except OSError:
            continue
        cores.setdefault(core, []).append(int(os.path.basename(d)[3:]))
    return [sorted(cores[c]) for c in sorted(cores)]


def claude_ancestor():
    pid = os.getpid()
    while pid > 1:
        try:
            stat = open(f"/proc/{pid}/stat").read()
        except OSError:
            return None
        name = stat[stat.index("(") + 1:stat.rindex(")")]
        if name == "claude":
            return pid
        pid = int(stat[stat.rindex(")") + 2:].split()[1])
    return None


def targets(root):
    # every process under the claude process, and every nice-19 process of this user
    me = os.getuid()
    parent, nice = {}, {}
    for d in glob.glob("/proc/[0-9]*"):
        pid = int(os.path.basename(d))
        try:
            if os.stat(d).st_uid != me:
                continue
            stat = open(os.path.join(d, "stat")).read()
        except OSError:
            continue
        rest = stat[stat.rindex(")") + 2:].split()
        parent[pid] = int(rest[1])
        nice[pid] = int(rest[16])
    out = set()
    for pid in parent:
        p = pid
        while p in parent and p != root:
            p = parent[p]
        if p == root or nice[pid] == 19:
            out.add(pid)
    return out


def pin(pids, cpus):
    moved = 0
    for pid in pids:
        try:
            tasks = os.listdir(f"/proc/{pid}/task")
        except OSError:
            continue
        for tid in tasks:
            try:
                os.sched_setaffinity(int(tid), cpus)
                moved += 1
            except OSError:
                pass
    return moved


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--off", action="store_true")
    p.add_argument("--hot", type=int, default=86, help="at or over this, use one core fewer")
    p.add_argument("--cool", type=int, default=79, help="at or under this, use one core more")
    p.add_argument("--limit", type=int, default=90, help="at or over this, drop three cores")
    p.add_argument("--start", type=int, default=9)
    p.add_argument("--least", type=int, default=4)
    p.add_argument("--every", type=int, default=15, help="seconds between looks")
    args = p.parse_args()

    cores = core_threads()
    root = claude_ancestor()
    if root is None:
        sys.exit("coolcpu: not started from inside a claude session")
    if args.off:
        everything = {c for threads in cores for c in threads}
        pin(targets(root) | {root}, everything)
        print("coolcpu: every job has all cores back")
        return
    temp = temperature_file()
    if temp is None:
        sys.exit("coolcpu: no cpu temperature sensor found")
    os.makedirs(os.path.dirname(LOG), exist_ok=True)
    n = min(args.start, len(cores))
    while True:
        t = int(open(temp).read()) // 1000
        if t >= args.limit:
            n -= 3
        elif t >= args.hot:
            n -= 1
        elif t <= args.cool:
            n += 1
        n = max(args.least, min(len(cores), n))
        cpus = {c for threads in cores[:n] for c in threads}
        moved = pin(targets(root) | {root}, cpus)
        with open(LOG, "a") as f:
            f.write(f"{time.strftime('%m-%d %H:%M:%S')} {t}C cores {n} threads {moved}\n")
        time.sleep(args.every)


if __name__ == "__main__":
    main()
