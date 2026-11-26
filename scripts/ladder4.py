#!/usr/bin/env python3
"""Label-free rule families in the model (idea 173; docs/results-2026-11-26-label-free.md).
See ladder4.sh.

  ladder4.py NAME [--quick] [--seeds 5,6,7] [--tasks mnist,pair] [--arms f0,f1,...]
             [--ctrls life,churn,shuffled] [--parts model,bound,transplant,engine] [--jobs 3]
             [--life 60000] [--checkpoints 20000,60000] [--epochs 6] [--objectives ...]
             [--transplant-arms f0,...] [--settle 3000] [--read TAG "FLAGS"] [--table-only]
  ladder4.py run TASK ARM CTRL SEED SHEET OUT [--life N] [--checkpoints a,b] [--limit N]
             [--wiring F] [--no-moved]
  ladder4.py bound TASK OBJECTIVE SEED SHEET OUT [--epochs N] [--limit N]
  ladder4.py noise TASK LEVELS SHEET [SEED]       (ladder4_noise.py, the calibration probe)

An arm is a family and optional settings after dots: f0, f0.cap4, f2.rate0.25, f4.push0.5,
f0.shift3 (shift: a cell moves with chance 2^-shift a spike; rate: family 2's chance a wrong
guess; push: family 4's chance a shared source steps down; cap: the tile's winners, at most
this many spikes a tick in a tile; batch: digits between re-picks; reg: regulation's rate;
coin1: family 1 with "made up" digits that are other real digits, so the bit says nothing).
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
# the engine's own competitive learning lived in the real sheet (results-2026-11-25-compete.md):
# A at a quarter of the updates, and B5 (with the tile's winners in the life and the read)
ENGINE = {"compete": "--g.learn_mode 5 --g.compete_on 1 --g.compete_shift 5".split(),
          "compete-tw": ("--g.learn_mode 5 --g.compete_on 1 --g.compete_shift 5 "
                         "--g.tile_winners 4").split()}
ARMS = ["f0", "f1", "f1g", "f2", "f3", "f4", "f5"]
OBJECTIVES = ["auto", "real", "pixel", "looks", "apart", "bcm"]


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
                str(opt["limit"]), "--wiring", wiring_file(outdir, task, "B" + obj, "life", seed)])
    return cmd, out


def finished(outdir, task, arm, ctrl, seed):
    """the json of the run that wrote this wiring (a life, or a bound: arm B<objective>)"""
    if arm.startswith("B"):
        return os.path.join(outdir, "bound-%s-%s-%d.json" % (task, arm[1:], seed))
    return os.path.join(outdir, "life-%s-%s-%s-%d.json" % (task, arm, ctrl, seed))


def transplant_jobs(outdir, task, arm, ctrl, seed, opt):
    """a lived wiring in the real born sheet, and the born sheet read the same way"""
    base = [MNIST, "ceiling"] + SHEET + READER + ["--seed", str(seed), "--fit",
                                                 str(opt["limit"] or 60000), "--settle",
                                                 str(opt["settle"])]
    base += PAIR if task == "pair" else ["--shift", "1"]
    base += opt["read_flags"]
    tag = "@" + opt["read_tag"] if opt["read_tag"] else ""
    jobs = [(nice(base), os.path.join(outdir, "bornread-%s%s-%d.txt" % (task, tag, seed)))]
    w = wiring_file(outdir, task, arm, ctrl, seed)
    if os.path.exists(w) and os.path.exists(finished(outdir, task, arm, ctrl, seed)):
        jobs.append((nice(base + ["--wiring-load", w]), os.path.join(
            outdir, "transplant-%s-%s-%s%s-%d.txt" % (task, arm, ctrl, tag, seed))))
    return jobs


def engine_jobs(outdir, task, seed, opt):
    """the engine's rule lived in the real sheet for --life digits, and on shuffled digits"""
    jobs = []
    for name, flags in ENGINE.items():
        for ctrl in ("life", "shuffled"):
            cmd = [MNIST, "ceiling"] + SHEET + READER + flags + [
                "--seed", str(seed), "--fit", str(opt["limit"] or 60000), "--settle",
                str(opt["settle"]), "--life", str(opt["life"])]
            cmd += PAIR if task == "pair" else ["--shift", "1"]
            cmd += ["--life-shuffle", "1"] if ctrl == "shuffled" else []
            jobs.append((nice(cmd), os.path.join(outdir, "engine-%s-%s-%s-%d.txt" % (
                task, name, ctrl, seed))))
    return jobs


def pack(outdir, only=None):
    """wiring files are kept gzipped: pack those whose life has finished (its json is
    written after its wiring), or only the given ones"""
    for f in sorted(os.listdir(outdir)):
        if not (f.startswith("wiring-") and f.endswith(".txt")):
            continue
        w = os.path.join(outdir, f)
        task, rest = f[len("wiring-"):-4].split("-", 1)
        arm_ctrl, seed = rest.rsplit("-", 1)
        arm, ctrl = arm_ctrl.rsplit("-", 1)
        done = finished(outdir, task, arm, ctrl, int(seed))
        if (only is not None and w not in only) or not os.path.exists(done):
            continue
        if not os.path.exists(w + ".gz"):
            with open(w, "rb") as a, gzip.open(w + ".gz", "wb") as b:
                shutil.copyfileobj(a, b)
        os.remove(w)


def unpack(names):
    """unpacked while blobmnist reads them; returns those this call unpacked"""
    made = set()
    for w in names:
        if not os.path.exists(w) and os.path.exists(w + ".gz"):
            with gzip.open(w + ".gz", "rb") as a, open(w, "wb") as b:
                shutil.copyfileobj(a, b)
            made.add(w)
    return made


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
        if jobs:  # only what this call made: a transplant call elsewhere may be reading
            pack(outdir, only={wiring_file(outdir, t, a, c, s) for t in opt["tasks"]
                               for s in opt["seeds"] for a in opt["arms"] + ["B" + o for o in
                                                                            opt["objectives"]]
                               for c in opt["ctrls"] + ["life"]})
        if "engine" in opt["parts"]:
            pool([j for t in opt["tasks"] for sd in opt["seeds"]
                  for j in engine_jobs(outdir, t, sd, opt)], opt["jobs"])
        if "transplant" in opt["parts"]:
            tj, need = [], set()
            for task in opt["tasks"]:
                for s in opt["seeds"]:
                    for a in opt["transplant_arms"]:
                        for c in opt["ctrls"]:
                            need.add(wiring_file(outdir, task, a, c, s))
            made = unpack(need)
            for task in opt["tasks"]:
                for s in opt["seeds"]:
                    for a in opt["transplant_arms"]:
                        for c in opt["ctrls"]:
                            tj += [j for j in transplant_jobs(outdir, task, a, c, s, opt)
                                   if j not in tj]
            pool(tj, opt["jobs"])
            pack(outdir, only=made)
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
    if args[0] == "noise":
        import ladder4_noise
        ladder4_noise.probe(args[1], [float(v) for v in args[2].split(",")], args[3],
                            int(args[4]) if len(args) > 4 else 5)
        return 0
    if args[0] == "bound":
        import ladder4_bound
        task, obj, seed, sheet, out = args[1:6]
        rest = args[6:]
        ladder4_bound.run_one(task, obj, int(seed), sheet, out, int(flag(rest, "--epochs", 6)),
                              int(flag(rest, "--limit", 0)), flag(rest, "--wiring", None))
        return 0
    name = args.pop(0)
    opt = {"seeds": [5, 6, 7], "tasks": ["mnist", "pair"], "parts": ["model"], "jobs": 3,
           "arms": list(ARMS), "ctrls": ["life", "churn", "shuffled"], "life": 60000,
           "checkpoints": [20000, 60000], "epochs": 6, "objectives": list(OBJECTIVES),
           "transplant_arms": ["f0"], "settle": 3000, "read_tag": "", "read_flags": [], "limit": 0, "table_only": False}
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
        elif a == "--read":
            # extra sheet flags for the transplant reads, and a tag for their files
            opt["read_tag"], opt["read_flags"] = args[i + 1], args[i + 2].split()
            i += 1
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
