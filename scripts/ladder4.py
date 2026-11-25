#!/usr/bin/env python3
"""Label-free rule families in the model (idea 173; docs/results-2026-11-26-label-free.md).
See ladder4.sh.

  ladder4.py NAME [--quick] [--seeds 5,6,7] [--tasks mnist,pair] [--arms f0,f1,...]
             [--ctrls life,churn,shuffled] [--parts model,bound,transplant] [--jobs 3]
             [--life 60000] [--checkpoints 20000,60000] [--epochs 6] [--objectives ...]
             [--transplant-arms f0,...] [--settle 3000] [--table-only]
  ladder4.py run TASK ARM CTRL SEED SHEET OUT [--life N] [--checkpoints a,b] [--limit N]
             [--wiring F] [--no-moved]
  ladder4.py bound TASK OBJECTIVE SEED SHEET OUT [--epochs N] [--limit N]

An arm is a family and optional settings after dots: f0, f0.cap4, f2.rate0.25, f4.push0.5,
f0.shift3 (shift: a cell moves with chance 2^-shift a spike; rate: family 2's chance a wrong
guess; push: family 4's chance a shared source steps down; cap: the tile's winners, at most
this many spikes a tick in a tile; batch: digits between re-picks; reg: regulation's rate).
"""

import gzip
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
MNIST = os.path.join(ROOT, os.environ.get("LADDER_BUILD", "build"), "blobmnist")
PY = sys.executable
SHEET = ("--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1 "
         "--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4").split()
READER = "--scale 2 --passes 10 --lr-1000 1 --views 1".split()
PAIR = "--pair 2 --pair-mix 1".split()
DEFAULTS = {"shift": 5.0, "rate": 0.125, "push": 0.25, "cap": 0.0, "batch": 32.0, "reg": 0.05}
ARMS = ["f0", "f1", "f1g", "f2", "f3", "f4", "f5"]
OBJECTIVES = ["real", "pixel", "looks", "apart", "bcm"]


def arm_opt(arm):
    """the family and its settings from an arm's name"""
    parts = arm.split(".")
    opt = dict(DEFAULTS)
    for p in parts[1:]:
        key = p.rstrip("0123456789.")
        opt[key] = float(p[len(key):])
    opt["batch"] = int(opt["batch"])
    return parts[0], opt


def nice(cmd):
    return ["nice", "-n", "19"] + cmd


def run_job(job):
    cmd, out = job
    if os.path.exists(out):
        return
    env = dict(os.environ, OMP_NUM_THREADS="1", MKL_NUM_THREADS="1")
    own = out.endswith(".json")
    log = out[:-5] + ".out" if own else out + ".part"
    with open(log, "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT, env=env)
    if r.returncode == 0 and not own:
        os.replace(log, out)


def pool(jobs, n):
    with ThreadPoolExecutor(n) as ex:
        list(ex.map(run_job, jobs))


def sheet_file(outdir, seed):
    return os.path.join(outdir, "sheet-%d.txt" % seed)


def describe_job(outdir, seed):
    cmd = nice([MNIST, "ceiling"] + SHEET + ["--fit", "1", "--test-count", "1", "--settle", "0",
                                             "--seed", str(seed), "--views", "1",
                                             "--describe", sheet_file(outdir, seed)])
    return cmd, os.path.join(outdir, "describe-%d.out" % seed)


def wiring_file(outdir, task, arm, ctrl, seed):
    return os.path.join(outdir, "wiring-%s-%s-%s-%d.txt" % (task, arm, ctrl, seed))


def life_job(outdir, task, arm, ctrl, seed, opt):
    out = os.path.join(outdir, "life-%s-%s-%s-%d.json" % (task, arm, ctrl, seed))
    cmd = nice([PY, os.path.abspath(__file__), "run", task, arm, ctrl, str(seed),
                sheet_file(outdir, seed), out, "--life", str(opt["life"]), "--checkpoints",
                ",".join(map(str, opt["checkpoints"])), "--limit", str(opt["limit"])])
    if arm != "born":
        cmd += ["--wiring", wiring_file(outdir, task, arm, ctrl, seed)]
    return cmd, out


def bound_job(outdir, task, obj, seed, opt):
    out = os.path.join(outdir, "bound-%s-%s-%d.json" % (task, obj, seed))
    cmd = nice([PY, os.path.abspath(__file__), "bound", task, obj, str(seed),
                sheet_file(outdir, seed), out, "--epochs", str(opt["epochs"]), "--limit",
                str(opt["limit"])])
    return cmd, out


def transplant_jobs(outdir, task, arm, ctrl, seed, opt):
    """a lived wiring in the real born sheet, and the born sheet read the same way"""
    base = [MNIST, "ceiling"] + SHEET + READER + ["--seed", str(seed), "--fit",
                                                 str(opt["limit"] or 60000), "--settle",
                                                 str(opt["settle"])]
    base += PAIR if task == "pair" else ["--shift", "1"]
    jobs = [(nice(base), os.path.join(outdir, "bornread-%s-%d.txt" % (task, seed)))]
    w = wiring_file(outdir, task, arm, ctrl, seed)
    if os.path.exists(w):
        jobs.append((nice(base + ["--wiring-load", w]),
                     os.path.join(outdir, "transplant-%s-%s-%s-%d.txt" % (task, arm, ctrl, seed))))
    return jobs


def wirings(outdir, unpack, names=None):
    """wiring files are kept gzipped; unpacked while blobmnist reads them"""
    for f in sorted(os.listdir(outdir)):
        if not f.startswith("wiring-"):
            continue
        w = os.path.join(outdir, f[:-3] if f.endswith(".gz") else f)
        if unpack and f.endswith(".gz") and (names is None or w in names):
            if not os.path.exists(w):
                with gzip.open(w + ".gz", "rb") as a, open(w, "wb") as b:
                    shutil.copyfileobj(a, b)
        if not unpack and not f.endswith(".gz"):
            if not os.path.exists(w + ".gz"):
                with open(w, "rb") as a, gzip.open(w + ".gz", "wb") as b:
                    shutil.copyfileobj(a, b)
            os.remove(w)


def ladder(name, opt):
    outdir = os.path.join(ROOT, "out", "ladder4", name)
    os.makedirs(outdir, exist_ok=True)
    if not opt["table_only"]:
        pool([describe_job(outdir, s) for s in opt["seeds"]], opt["jobs"])
        jobs = []
        if "model" in opt["parts"]:
            for task in opt["tasks"]:
                for s in opt["seeds"]:
                    jobs.append(life_job(outdir, task, "born", "life", s, opt))
                    jobs += [life_job(outdir, task, a, c, s, opt)
                             for a in opt["arms"] for c in opt["ctrls"]]
        if "bound" in opt["parts"]:
            jobs += [bound_job(outdir, t, o, s, opt) for t in opt["tasks"]
                     for o in opt["objectives"] for s in opt["seeds"]]
        jobs.sort(key=lambda j: "bound-" not in j[1])  # the long ones first
        pool(jobs, opt["jobs"])
        wirings(outdir, unpack=False)
        if "transplant" in opt["parts"]:
            tj, need = [], set()
            for task in opt["tasks"]:
                for s in opt["seeds"]:
                    for a in opt["transplant_arms"]:
                        for c in opt["ctrls"]:
                            need.add(wiring_file(outdir, task, a, c, s))
            wirings(outdir, unpack=True, names=need)
            for task in opt["tasks"]:
                for s in opt["seeds"]:
                    for a in opt["transplant_arms"]:
                        for c in opt["ctrls"]:
                            tj += [j for j in transplant_jobs(outdir, task, a, c, s, opt)
                                   if j not in tj]
            pool(tj, opt["jobs"])
            wirings(outdir, unpack=False)
    import ladder4_table
    text = ladder4_table.table(outdir, opt)
    with open(os.path.join(outdir, "table.txt"), "w") as f:
        f.write(text + "\n")
    print(text)


def seeds_arg(v):
    lo_hi = v.split("-")
    if len(lo_hi) == 2:
        return list(range(int(lo_hi[0]), int(lo_hi[1]) + 1))
    return [int(x) for x in v.split(",")]


def flag(rest, n, d):
    return rest[rest.index(n) + 1] if n in rest else d


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    if args[0] == "run":
        import ladder4_life
        task, arm, ctrl, seed, sheet, out = args[1:7]
        rest = args[7:]
        fam, o = arm_opt(arm)
        o.update(life=int(flag(rest, "--life", 60000)), limit=int(flag(rest, "--limit", 0)),
                 checkpoints=[int(v) for v in flag(rest, "--checkpoints", "60000").split(",")],
                 moved="--no-moved" not in rest, arm=arm)
        ladder4_life.run_one(task, fam, ctrl, int(seed), sheet, out, o, flag(rest, "--wiring", None))
        return 0
    if args[0] == "bound":
        import ladder4_bound
        task, obj, seed, sheet, out = args[1:6]
        rest = args[6:]
        ladder4_bound.run_one(task, obj, int(seed), sheet, out, int(flag(rest, "--epochs", 6)),
                              int(flag(rest, "--limit", 0)))
        return 0
    name = args.pop(0)
    opt = {"seeds": [5, 6, 7], "tasks": ["mnist", "pair"], "parts": ["model"], "jobs": 3,
           "arms": list(ARMS), "ctrls": ["life", "churn", "shuffled"], "life": 60000,
           "checkpoints": [20000, 60000], "epochs": 6, "objectives": list(OBJECTIVES),
           "transplant_arms": ["f0"], "settle": 3000, "limit": 0, "table_only": False}
    if "--quick" in args:
        opt.update(seeds=[5], life=2000, checkpoints=[1000, 2000], limit=10000, epochs=1,
                   settle=500, arms=["f0", "f3"], objectives=["looks"],
                   parts=["model", "bound", "transplant"])
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--seeds":
            opt["seeds"] = seeds_arg(args[i + 1])
        elif a in ("--tasks", "--parts", "--arms", "--ctrls", "--objectives", "--transplant-arms"):
            opt[a[2:].replace("-", "_")] = args[i + 1].split(",")
        elif a == "--checkpoints":
            opt["checkpoints"] = [int(v) for v in args[i + 1].split(",")]
        elif a in ("--jobs", "--life", "--epochs", "--settle", "--limit"):
            opt[a[2:]] = int(args[i + 1])
        elif a in ("--table-only", "--quick"):
            opt[a[2:].replace("-", "_")] = True
            i -= 1
        else:
            print("unknown option", a)
            return 1
        i += 2
    ladder(name, opt)
    return 0


if __name__ == "__main__":
    sys.exit(main())
