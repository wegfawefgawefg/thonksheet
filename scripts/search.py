#!/usr/bin/env python3
"""A resumable evolutionary search for one setting that works on every world of the quick map.

    scripts/search.py run [--name main] [--jobs 2] [--hours H] [--map FILE]
                          [--engines blob,minimal,table] [--starts FILE|none]
                          [--batch program:PATH[:ENGINES]] [--max-load L] [--busy-jobs N]
    scripts/search.py status [--name main]
    scripts/search.py report [--name main] [--out docs/search-leaderboard.md]
    scripts/search.py merge OTHER_FOLDER [--name main]
    scripts/search.py stop [--name main]        finish the runs in flight, then stop
    scripts/search.py check ENGINE...           are the space file's numbers and tags right

Everything lives in out/search/<name>/ (see scripts/search/store.py). The search reads its
spaces (scripts/search/space-<engine>.txt) and its quick map (the --map file, kept in
settings.json after the first run) every time it starts, so a new number or a new world is
picked up by stopping and starting it again. How it all works: docs/search.md.
Standard library only; runs on Python 3.9 (the second machine).
"""

import argparse
import concurrent.futures
import os
import signal
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.dont_write_bytecode = True  # no __pycache__ beside the scripts
sys.path.insert(0, os.path.join(ROOT, "scripts", "search"))

import batch      # noqa: E402
import checks     # noqa: E402
import evolve      # noqa: E402
import leaderboard  # noqa: E402
import quickmap    # noqa: E402
import runner      # noqa: E402
import space as sp  # noqa: E402
import store as st  # noqa: E402

HERE = os.path.join(ROOT, "scripts", "search")
DEFAULTS = {"map": "scripts/search/quick-map.txt", "engines": "blob,minimal,table",
            "starts": "scripts/search/starts.txt"}


def folder_of(name):
    return os.path.join(ROOT, "out", "search", name)


LOGFILE = []  # run.log of the search being run


def log(text):
    line = time.strftime("%Y-%m-%d %H:%M:%S ") + text
    print(line, flush=True)
    for path in LOGFILE:
        with open(path, "a") as f:
            f.write(line + "\n")


def pid_alive(folder):
    p = os.path.join(folder, "run.pid")
    if not os.path.exists(p):
        return None
    try:
        pid = int(open(p).read().strip())
        os.kill(pid, 0)
        return pid
    except (ValueError, OSError):
        return None


def load_all(store, args):
    """settings (command line over settings.json over defaults), spaces, map, starts"""
    settings = dict(DEFAULTS)
    settings.update(store.settings())
    for k in DEFAULTS:
        v = getattr(args, k, None)
        if v:
            settings[k] = v
    engines = settings["engines"].split(",")
    spaces = {}
    for e in engines:
        spaces[e] = sp.load_space(os.path.join(HERE, "space-%s.txt" % e), ROOT)
    qm = quickmap.load_quick_map(os.path.join(ROOT, settings["map"]))
    starts = []
    if settings["starts"] != "none":
        starts = read_starts(os.path.join(ROOT, settings["starts"]), spaces)
    return settings, engines, spaces, qm, starts


def all_engines(store, args):
    """for status and report: every engine with candidates in the folder (a folder may hold
    another machine's engines after a merge), unless --engines names them"""
    settings, engines, spaces, qm, starts = load_all(store, args)
    if not args.engines:
        for row in store.candidates.values():
            e = row["engine"]
            if e not in engines and os.path.exists(os.path.join(HERE, "space-%s.txt" % e)):
                engines.append(e)
                spaces[e] = sp.load_space(os.path.join(HERE, "space-%s.txt" % e), ROOT)
    return engines, spaces, qm


def read_starts(path, spaces):
    """lines `engine name: --flag value ...`; values must be numbers of that engine's space"""
    out = []
    for raw in open(path):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        head, _, flags = line.partition(":")
        engine, name = head.split()
        if engine not in spaces:
            continue
        space = spaces[engine]
        values = space.defaults()
        parts = flags.split()
        for i in range(0, len(parts), 2):
            n = space.by_flag.get(parts[i])
            if n is None:
                raise ValueError("%s: %s is not in space-%s.txt" % (path, parts[i], engine))
            if n.kind == "rule":
                values[n.flag] = sp.read_rule(parts[i + 1], ROOT, n.low)
            else:
                values[n.flag] = int(parts[i + 1])
        out.append((engine, name, values))
    return out


def git_commit():
    try:
        return subprocess.run(["git", "-C", ROOT, "rev-parse", "--short", "HEAD"],
                              capture_output=True, text=True).stdout.strip() or "unknown"
    except OSError:
        return "unknown"


def cmd_run(args):
    folder = folder_of(args.name)
    os.makedirs(os.path.join(folder, "tmp"), exist_ok=True)
    if pid_alive(folder):
        print("a search is already running in %s (pid %d)" % (folder, pid_alive(folder)))
        return 1
    with open(os.path.join(folder, "run.pid"), "w") as f:
        f.write(str(os.getpid()))
    LOGFILE.append(os.path.join(folder, "run.log"))
    stop_file = os.path.join(folder, "STOP")
    if os.path.exists(stop_file):
        os.remove(stop_file)
    store = st.Store(folder)
    settings, engines, spaces, qm, starts = load_all(store, args)
    store.save_settings(settings)
    commit = args.commit or git_commit()
    search = evolve.Search(store, qm, spaces, engines, sp.make_rng(args.seed), log)
    search.add_starts(starts)
    topped = search.top_up()
    max_load = args.max_load or float(os.cpu_count() or 2)
    log("search %s: engines %s, %d worlds, jobs %d, hours %s, commit %s; %d candidates, "
        "%d runs known, %d resumed, %d topped up" % (
            args.name, ",".join(engines), len(qm.worlds), args.jobs, args.hours or "no limit",
            commit, len(store.candidates), len(store.results), len(search.active), topped))

    def on_signal(signum, frame):
        log("signal %d: killing runs in flight and stopping" % signum)
        runner.kill_all()

    signal.signal(signal.SIGTERM, on_signal)
    signal.signal(signal.SIGINT, on_signal)

    def rule_file(digest):
        return os.path.join(folder, "rules", digest + ".hex")

    started = time.time()
    deadline = started + args.hours * 3600 if args.hours else None
    last_status = started
    done = 0
    pending = {}  # future -> list of (id, kind, cmd, world, engine)
    tmp = os.path.join(folder, "tmp")
    ev = batch.make(args.batch)
    pool = concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs + 1)

    def submit_one(job):
        cid, kind, cmd, world = job
        for digest, text in sp.rules_in(search.scorer.space(cid), search.scorer.values(cid)).items():
            store.rule_path(digest, text)
        search.started(cmd)
        return job + (search.scorer.engine(cid),)

    def cpu_run(job):
        row = runner.run(job[2], job[1], ROOT, tmp, rule_file, qm.timeout, job[4], commit)
        if row is not None:
            row["world"] = job[3]
        return [row]

    while True:
        stopping = (runner.stopping.is_set() or os.path.exists(stop_file) or
                    (deadline is not None and time.time() > deadline))
        cpu_busy = sum(1 for jobs in pending.values() if not jobs[0][5])
        batch_busy = any(jobs[0][5] for jobs in pending.values())
        if ev and not stopping and not batch_busy:
            jobs = search.next_commands(ev.size, max_active=args.jobs * 3 + 4 + ev.size,
                                        accept=ev.accepts)
            if jobs:
                jobs = [submit_one(j) + (True,) for j in jobs]
                fut = pool.submit(ev.run, [j[:5] for j in jobs], ROOT, tmp, rule_file,
                                  qm.timeout, commit)
                pending[fut] = jobs
        while not stopping and cpu_busy < args.jobs:
            if cpu_busy >= args.busy_jobs and os.getloadavg()[0] > max_load:
                break  # the machine is busy with other work: wait
            refuse = (lambda e, k: not ev.accepts(e, k)) if ev else None
            jobs = search.next_commands(1, max_active=args.jobs * 3 + 4, accept=refuse)
            if not jobs:
                break
            job = submit_one(jobs[0]) + (False,)
            pending[pool.submit(cpu_run, job)] = [job]
            cpu_busy += 1
        if not pending:
            if stopping:
                break
            time.sleep(1)
            continue
        finished, _ = concurrent.futures.wait(list(pending), timeout=5,
                                              return_when=concurrent.futures.FIRST_COMPLETED)
        for fut in finished:
            jobs = pending.pop(fut)
            try:
                rows = fut.result()
            except Exception as e:  # a bug in a runner must not end a week-long search
                log("run failed in python: %r: %s" % (e, " ".join(jobs[0][2])[:200]))
                rows = [None] * len(jobs)
            if jobs[0][5] and all(r is None for r in rows) and not runner.stopping.is_set():
                log("the batch evaluator failed on a whole batch; the CPU runs everything now")
                ev = None
            for job, row in zip(jobs, rows):
                search.finished(job[2], row)
                if row is not None:
                    done += 1
                    if row.get("error"):
                        log("error: %s: %s" % (row["error"][:200], row["key"][:200]))
                    if done % 500 == 0:
                        search.top_up()
        if time.time() - last_status > 600:
            last_status = time.time()
            log(status_line(store, engines, started, done))
    pool.shutdown(wait=True)
    log("stopped. " + status_line(store, engines, started, done))
    os.remove(os.path.join(folder, "run.pid"))
    return 0


def status_line(store, engines, started, done):
    counts = {e: 0 for e in engines}
    for row in store.candidates.values():
        if row["engine"] in counts:
            counts[row["engine"]] += 1
    hours = (time.time() - started) / 3600
    return "%d runs this session in %.2f h; candidates %s; %d runs known" % (
        done, hours, ", ".join("%s %d" % kv for kv in counts.items()), len(store.results))


def cmd_status(args):
    folder = folder_of(args.name)
    if not os.path.exists(folder):
        print("no search named %s" % args.name)
        return 1
    pid = pid_alive(folder)
    print("search %s: %s" % (args.name, "running, pid %d" % pid if pid else "not running"))
    store = st.Store(folder)
    engines, spaces, qm = all_engines(store, args)
    print(leaderboard.short_status(store, qm, spaces, engines))
    logfile = os.path.join(folder, "run.log")
    if os.path.exists(logfile):
        print("last lines of %s:" % os.path.relpath(logfile, ROOT))
        for line in open(logfile).readlines()[-6:]:
            print("  " + line.rstrip()[:160])
    return 0


def cmd_report(args):
    folder = folder_of(args.name)
    store = st.Store(folder)
    engines, spaces, qm = all_engines(store, args)
    out = args.out or (os.path.join(ROOT, "docs", "search-leaderboard.md") if args.name == "main"
                       else os.path.join(folder, "leaderboard.md"))
    text = leaderboard.report(store, qm, spaces, engines, args.name, folder)
    with open(out, "w") as f:
        f.write(text)
    print("wrote %s" % os.path.relpath(out, ROOT))
    return 0


def cmd_merge(args):
    added = st.merge(folder_of(args.name), os.path.abspath(args.other))
    print("merged %s into %s: %d runs, %d candidates, %d decisions new" % (
        args.other, args.name, added[0], added[1], added[2]))
    if pid_alive(folder_of(args.name)):
        print("note: a search is running on %s; it sees the merged lines when it restarts"
              % args.name)
    return 0


def cmd_check(args):
    problems = []
    for engine in args.engines_to_check:
        space = sp.load_space(os.path.join(HERE, "space-%s.txt" % engine), ROOT)
        problems += checks.check(space, ROOT, os.path.join(ROOT, "out", "search", "check"))
    print("\n".join(problems) if problems else "no problems")
    return 1 if problems else 0


def cmd_stop(args):
    folder = folder_of(args.name)
    open(os.path.join(folder, "STOP"), "w").close()
    pid = pid_alive(folder)
    print("asked search %s to stop after its runs in flight%s" % (
        args.name, " (pid %d)" % pid if pid else "; it is not running"))
    return 0


def main():
    p = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    sub = p.add_subparsers(dest="cmd")
    r = sub.add_parser("run")
    r.add_argument("--jobs", type=int, default=2)
    r.add_argument("--hours", type=float, default=0)
    r.add_argument("--max-load", type=float, default=0,
                   help="do not start a run while the 1-minute load is above this "
                        "(default: the number of cores)")
    r.add_argument("--busy-jobs", type=int, default=1,
                   help="jobs kept going even while the load is over --max-load (default 1)")
    r.add_argument("--commit", default="")
    r.add_argument("--batch", default="", help="a batch evaluator beside the CPU jobs: "
                   "serial, or program:PATH[:ENGINES] (scripts/search/batch.py)")
    r.add_argument("--seed", type=int, default=None, help="random seed for the search itself")
    r.set_defaults(fn=cmd_run)
    s = sub.add_parser("status")
    s.set_defaults(fn=cmd_status)
    q = sub.add_parser("report")
    q.add_argument("--out", default="")
    q.set_defaults(fn=cmd_report)
    m = sub.add_parser("merge")
    m.add_argument("other")
    m.set_defaults(fn=cmd_merge)
    c = sub.add_parser("check")
    c.add_argument("engines_to_check", nargs="+")
    c.set_defaults(fn=cmd_check)
    t = sub.add_parser("stop")
    t.set_defaults(fn=cmd_stop)
    for x in (r, s, q, m, t):
        x.add_argument("--name", default="main")
    for x in (r, s, q):
        x.add_argument("--map", default="")
        x.add_argument("--engines", default="")
        x.add_argument("--starts", default="")
    args = p.parse_args()
    if not args.cmd:
        print(__doc__)
        return 1
    return args.fn(args)


if __name__ == "__main__":
    sys.exit(main())
