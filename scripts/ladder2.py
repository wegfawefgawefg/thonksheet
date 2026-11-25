#!/usr/bin/env python3
"""The inside's ladder (idea 165; docs/results-2026-11-25-inside-ladder.md). See ladder2.sh.

  ladder2.py NAME [--quick] [--seeds 5,6,7] [--tasks mnist,pair] [--parts ...] [--jobs 3]
             [--epochs N] [--val] [--table-only] [--rungs 1,2,...] [--kinds lin,trained,born,auto]
  ladder2.py run TASK RUNG KIND SEED SHEET OUT [--val] [--epochs N] [--limit N] [--wiring F]
  ladder2.py match TASK SEED SHEET WIRING|born ROWS [ROWS2]   (ladder2_match.py)

Parts of a ladder (default all): bars (the tree's own baselines), model (every rung trained,
the born arms, the label-free arm), transplant (the bottom rung's wiring written into the
real sheet and read by `blobmnist ceiling`), match (the first seed's model cells against the
real sheet's, digit by digit: ladder2_match.py).
"""

import gzip
import json
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MNIST = os.path.join(ROOT, os.environ.get("LADDER_BUILD", "build"), "blobmnist")
PY = sys.executable
# scripts/mnist.sh's sheet and the inside-learning test's reader (scripts/inside.py)
SHEET = ("--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1 "
         "--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4").split()
READER = "--scale 2 --passes 10 --lr-1000 1 --views 1".split()
PAIR = "--pair 2 --pair-mix 1".split()  # same class or not, interleaved (the hop-real doc)
TRAINED = ["net", "1", "2", "3a", "3b", "4", "4s3", "5", "6", "7a", "7b"]
BORN = ["2", "3a", "3b", "4", "5", "6", "7b"]
BOTTOM = "7b"
# the slow rungs first, so the pool ends on short jobs
COST = {"4": 5, "4s3": 6, "5": 5, "6": 5, "7a": 5, "7b": 5}


def nice(cmd):
    return ["nice", "-n", "19"] + cmd


def run_job(job):
    """a job is (command, the file it makes): a model run writes its own json, and its
    output goes beside it; any other command's output is the file it makes"""
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


def sheet_file(outdir, seed):
    return os.path.join(outdir, "sheet-%d.txt" % seed)


def describe_job(outdir, seed):
    # the born sheet as the model needs it; one digit fitted, one scored: a second
    out = os.path.join(outdir, "describe-%d.out" % seed)
    cmd = nice([MNIST, "ceiling"] + SHEET + ["--fit", "1", "--test-count", "1", "--settle", "0",
                                             "--seed", str(seed), "--views", "1",
                                             "--describe", sheet_file(outdir, seed)])
    return cmd, out


def model_job(outdir, task, rung, kind, seed, opt):
    out = os.path.join(outdir, "%s-%s-%s-%d.json" % (task, rung, kind, seed))
    cmd = nice([PY, os.path.abspath(__file__), "run", task, rung, kind, str(seed),
                sheet_file(outdir, seed), out, "--epochs", str(opt["epochs"]),
                "--limit", str(opt["limit"])] + (["--val"] if opt["val"] else []))
    if rung == BOTTOM and kind == "trained":
        cmd += ["--wiring", wiring_file(outdir, task, seed)]
    return cmd, out


def transplant_jobs(outdir, task, seed, opt):
    """the bottom rung's wiring in the real sheet, and the born sheet read the same way"""
    base = [MNIST, "ceiling"] + SHEET + READER + ["--seed", str(seed), "--fit",
                                                 str(opt["limit"] or 60000), "--settle",
                                                 str(opt["settle"])]
    base += PAIR if task == "pair" else []
    base += ["--val", "1"] if opt["val"] else []
    return [(nice(base + ["--wiring-load", wiring_file(outdir, task, seed)]),
             os.path.join(outdir, "transplant-%s-%d.txt" % (task, seed))),
            (nice(base), os.path.join(outdir, "bornread-%s-%d.txt" % (task, seed)))]


def wiring_file(outdir, task, seed):
    return os.path.join(outdir, "wiring-%s-%d.txt" % (task, seed))


def wirings(outdir, opt, unpack):
    """the transplant wiring files are kept gzipped; unpacked while blobmnist reads them"""
    for task in opt["tasks"]:
        for s in opt["seeds"]:
            w = wiring_file(outdir, task, s)
            src, dst = (w + ".gz", w) if unpack else (w, w + ".gz")
            if os.path.exists(src) and (unpack or not os.path.exists(dst)):
                opener = (gzip.open, open) if unpack else (open, gzip.open)
                with opener[0](src, "rb") as a, opener[1](dst, "wb") as b:
                    shutil.copyfileobj(a, b)
            if not unpack and os.path.exists(w) and os.path.exists(w + ".gz"):
                os.remove(w)


def match_part(outdir, opt):
    """the model's cells against the sheet's, digit by digit (ladder2_match.py), on the first
    seed: the born sheet (with a second run of it from another state) and the transplant.
    the spike-count rows are large and are removed when read"""
    s = opt["seeds"][0]
    for task in opt["tasks"]:
        base = [MNIST, "ceiling"] + SHEET + READER + ["--seed", str(s), "--fit",
                                                     str(opt["limit"] or 60000)]
        base += PAIR if task == "pair" else []
        rows = {k: os.path.join(outdir, "rows-%s-%s-%d.bin" % (k, task, s))
                for k in ("born", "born2", "transplant")}
        outs = {k: os.path.join(outdir, "match-%s-%s-%d.txt" % (k, task, s))
                for k in ("born", "transplant")}
        if all(os.path.exists(o) for o in outs.values()):
            continue
        w = wiring_file(outdir, task, s)
        jobs = [(nice(base + ["--settle", str(opt["settle"]), "--rows-out", rows["born"]]),
                 rows["born"] + ".txt"),
                (nice(base + ["--settle", str(opt["settle"] + 1), "--rows-out", rows["born2"]]),
                 rows["born2"] + ".txt"),
                (nice(base + ["--settle", str(opt["settle"]), "--wiring-load", w, "--rows-out",
                              rows["transplant"]]), rows["transplant"] + ".txt")]
        pool(jobs, opt["jobs"])
        me = [PY, os.path.abspath(__file__), "match", task, str(s), sheet_file(outdir, s)]
        pool([(nice(me + ["born", rows["born"], rows["born2"]]), outs["born"]),
              (nice(me + [w, rows["transplant"]]), outs["transplant"])], opt["jobs"])
        for k in rows:
            for f in (rows[k], rows[k] + ".txt"):
                if os.path.exists(f):
                    os.remove(f)


def bars_jobs(outdir, opt):
    jobs = []
    for task in opt["tasks"]:
        for passes in (1, 5):
            cmd = [MNIST, "baselines", "--mnist-code", "t28", "--net-passes", str(passes)]
            cmd += PAIR if task == "pair" else []
            cmd += ["--val", "1"] if opt["val"] else []
            jobs.append((nice(cmd), os.path.join(outdir, "bars-%s-%d.txt" % (task, passes))))
    return jobs


def pool(jobs, n):
    with ThreadPoolExecutor(n) as ex:
        list(ex.map(run_job, jobs))


def ladder(name, opt):
    outdir = os.path.join(ROOT, "out", "ladder2", name)
    os.makedirs(outdir, exist_ok=True)
    parts = opt["parts"]
    if not opt["table_only"]:
        pool([describe_job(outdir, s) for s in opt["seeds"]], opt["jobs"])
        jobs = []
        if "bars" in parts:
            jobs += bars_jobs(outdir, opt)
        if "model" in parts:
            for task in opt["tasks"]:
                for s in opt["seeds"]:
                    k = opt["kinds"]
                    jobs += [model_job(outdir, task, "lin", "trained", s, opt)] * ("lin" in k)
                    jobs += [model_job(outdir, task, r, "trained", s, opt)
                             for r in opt["rungs"] if "trained" in k]
                    jobs += [model_job(outdir, task, r, "born", s, opt)
                             for r in BORN if r in opt["rungs"] and "born" in k]
                    if BOTTOM in opt["rungs"] and "auto" in k:
                        jobs += [model_job(outdir, task, BOTTOM, "auto", s, opt)]
        jobs.sort(key=lambda j: -COST.get(j[1].split("-")[-3], 1) * (
            "trained" in j[1] or "auto" in j[1]))
        pool(jobs, opt["jobs"])
        if "transplant" in parts or "match" in parts:
            wirings(outdir, opt, unpack=True)
            if "transplant" in parts:
                tj = []
                for task in opt["tasks"]:
                    for s in opt["seeds"]:
                        if os.path.exists(wiring_file(outdir, task, s)):
                            tj += transplant_jobs(outdir, task, s, opt)
                pool(tj, opt["jobs"])
            if "match" in parts:
                match_part(outdir, opt)
            wirings(outdir, opt, unpack=False)
    import ladder2_table
    text = ladder2_table.table(outdir, opt)
    with open(os.path.join(outdir, "table.txt"), "w") as f:
        f.write(text + "\n")
    print(text)


def seeds_arg(v):
    lo_hi = v.split("-")
    if len(lo_hi) == 2:
        return list(range(int(lo_hi[0]), int(lo_hi[1]) + 1))
    return [int(x) for x in v.split(",")]


def main():
    args = sys.argv[1:]
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    if args[0] == "match":
        import json
        import ladder2_match
        a = args[1:]
        print(json.dumps(ladder2_match.match(a[0], int(a[1]), a[2], a[3], a[4],
                                             a[5] if len(a) > 5 else None)))
        return 0
    if args[0] == "run":
        import ladder2_train
        task, rung, kind, seed, sheet, out = args[1:7]
        rest = args[7:]
        flag = lambda n, d: rest[rest.index(n) + 1] if n in rest else d
        ladder2_train.run_one(task, rung, kind, int(seed), "--val" in rest,
                              int(flag("--epochs", 20)), sheet, out, flag("--wiring", None),
                              limit=int(flag("--limit", 0)))
        return 0
    name = args.pop(0)
    opt = {"seeds": [5, 6, 7], "tasks": ["mnist", "pair"], "parts": ["bars", "model", "transplant", "match"],
           "jobs": 3, "epochs": 12, "val": False, "table_only": False, "limit": 0,
           "rungs": list(TRAINED), "settle": 3000, "kinds": ["lin", "trained", "born", "auto"]}
    if "--quick" in args:
        opt.update(seeds=[5], epochs=1, limit=10000, settle=500)
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--seeds":
            opt["seeds"] = seeds_arg(args[i + 1])
        elif a in ("--tasks", "--parts", "--rungs", "--kinds"):
            opt[a[2:]] = args[i + 1].split(",")
        elif a in ("--jobs", "--epochs", "--limit", "--settle"):
            opt[a[2:]] = int(args[i + 1])
        elif a in ("--val", "--table-only", "--quick"):
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
