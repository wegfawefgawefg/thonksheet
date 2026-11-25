#!/usr/bin/env python3
"""The rule's ladder (idea 167; docs/results-2026-11-25-rule-ladder.md). See ladder3.sh.

  ladder3.py NAME [--quick] [--seeds 5,6,7] [--tasks mnist,pair] [--arms T0b,T1a,...]
             [--coins 0|1] [--parts model,transplant,engine] [--transplant T6,T7]
             [--jobs 3] [--val] [--table-only]
  ladder3.py run TASK ARM SEED SHEET OUT [--val] [--limit N] [--wiring F]

An arm is a rung of the teacher (ladder3_train.ARMS, AUTO). NAME@V runs it at step size V
(the score's step for grad rungs, the counter's chance 2^-V for count rungs) instead of its
pick (PICKS below, from `scripts/ladder3.sh pick ...` on the validation slice). Arms from T3
down get a coin arm (NAMEcoin) unless --coins 0. "born" is idea 165's born arm (rung 7b with
the real born wiring, nothing trained). Parts: model (every arm in the offline model),
transplant (the named arms' wirings written into the real sheet with `blobmnist ceiling
--wiring-load`, beside the born sheet read the same way), engine (the engine's real one-hop
rule from the answer in the real sheet: scripts/hop2.sh's `target` arm).
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ladder2 as L2  # noqa: E402  the sheet's flags, the describe job, the job pool

ROOT = L2.ROOT
PY = sys.executable
LADDER = ["T0", "T0b", "T1a", "T1b", "T2", "T2h", "T3", "T4", "T5", "T6f", "T6n", "T6", "T7",
          "T8v", "T8r", "T8", "T10"]
ALONE = ["A0", "A2h", "A3", "A5", "A6", "A8"]
COINLESS = {"T0", "T0b", "T1a", "T1b", "T2", "T2h", "born"} | set(ALONE)
# step sizes picked on the validation slice, seed 1 (scripts/ladder3.sh pick ...; the doc's
# "Picking the step"): score step for grad rungs, 2^-shift for count rungs
PICKS = {"mnist": {}, "pair": {}}
DEFAULT_LR, DEFAULT_SHIFT = 10.0, 9
COST = {"T0": 12, "T0b": 12, "A0": 12, "A2h": 12, "A3": 12, "A5": 12, "A6": 12, "A8": 12}


def split_arm(arm):
    name, _, v = arm.partition("@")
    return name, v


def is_count(name):
    import ladder3_train as T
    base = name[:-4] if name.endswith("coin") else name
    return base in T.ARMS and T.ARMS[base]["step"] == "count"


def step_of(task, arm):
    name, v = split_arm(arm)
    base = name[:-4] if name.endswith("coin") else name
    if v:
        return float(v)
    return PICKS[task].get(base, DEFAULT_SHIFT if is_count(name) else DEFAULT_LR)


def model_job(outdir, task, arm, seed, opt, wiring=False):
    out = os.path.join(outdir, "%s-%s-%d.json" % (task, arm, seed))
    name, _ = split_arm(arm)
    if name == "born":  # idea 165's born arm, its own runner
        cmd = [PY, os.path.join(ROOT, "scripts", "ladder2.py"), "run", task, "7b", "born",
               str(seed), L2.sheet_file(outdir, seed), out, "--limit", str(opt["limit"])]
        return L2.nice(cmd + (["--val"] if opt["val"] else [])), out
    cmd = [PY, os.path.abspath(__file__), "run", task, arm, str(seed), L2.sheet_file(outdir, seed),
           out, "--limit", str(opt["limit"])] + (["--val"] if opt["val"] else [])
    if opt["digits"]:
        cmd += ["--digits", str(opt["digits"])]
    if wiring:
        cmd += ["--wiring", wiring_file(outdir, task, arm, seed)]
    return L2.nice(cmd), out


def wiring_file(outdir, task, arm, seed):
    return os.path.join(outdir, "wiring-%s-%s-%d.txt" % (task, arm, seed))


def transplant_jobs(outdir, task, arm, seed, opt):
    base = [L2.MNIST, "ceiling"] + L2.SHEET + L2.READER + [
        "--seed", str(seed), "--fit", str(opt["limit"] or 60000), "--settle", str(opt["settle"])]
    base += L2.PAIR if task == "pair" else []
    base += ["--val", "1"] if opt["val"] else []
    jobs = [(L2.nice(base + ["--wiring-load", wiring_file(outdir, task, arm, seed)]),
             os.path.join(outdir, "transplant-%s-%s-%d.txt" % (task, arm, seed)))]
    return jobs + [(L2.nice(base), os.path.join(outdir, "bornread-%s-%d.txt" % (task, seed)))]


def engine_jobs(outdir, task, seed, quick):
    """the engine's one-hop rule from the answer in the real sheet (hop2.sh `target`)"""
    import hop2
    flags = hop2.HOP + " --g.hop_shift 9"
    r = {"arms": [("engine", 3, flags), ("enginecoin", 3, flags + " --g.hop_coin 1")],
         "seeds": [seed], "test": hop2.SAME if task == "pair" else ""}
    jobs = hop2.mnist_jobs(r, outdir, quick)
    named = []
    for cmd, out in jobs:  # engine-5.txt -> engine-mnist-5.txt
        arm, seed_txt = os.path.basename(out).rsplit("-", 1)
        named.append((cmd, os.path.join(outdir, "%s-%s-%s" % (arm, task, seed_txt))))
    return named


def gz(outdir, unpack):
    """wiring files are kept gzipped, unpacked while blobmnist reads them"""
    import gzip
    import shutil
    for name in sorted(os.listdir(outdir)):
        p = os.path.join(outdir, name)
        if unpack and name.startswith("wiring-") and name.endswith(".gz"):
            if not os.path.exists(p[:-3]):
                with gzip.open(p, "rb") as a, open(p[:-3], "wb") as b:
                    shutil.copyfileobj(a, b)
        elif not unpack and name.startswith("wiring-") and name.endswith(".txt"):
            if not os.path.exists(p + ".gz"):
                with open(p, "rb") as a, gzip.open(p + ".gz", "wb") as b:
                    shutil.copyfileobj(a, b)
            os.remove(p)


def arms_with_coins(opt):
    out = []
    for a in opt["arms"]:
        out.append(a)
        name, v = split_arm(a)
        if opt["coins"] and name not in COINLESS and not name.endswith("coin"):
            out.append(name + "coin" + ("@" + v if v else ""))
    return out


def ladder(name, opt):
    outdir = os.path.join(ROOT, "out", "ladder3", name)
    os.makedirs(outdir, exist_ok=True)
    if not opt["table_only"]:
        L2.pool([L2.describe_job(outdir, s) for s in opt["seeds"]], opt["jobs"])
        jobs = []
        arms = arms_with_coins(opt)
        if "model" in opt["parts"]:
            for task in opt["tasks"]:
                for s in opt["seeds"]:
                    for a in arms:
                        jobs.append(model_job(outdir, task, a, s, opt,
                                              wiring=split_arm(a)[0] in opt["transplant"]))
        jobs.sort(key=lambda j: -COST.get(split_arm(os.path.basename(j[1]).split("-")[1])[0], 1))
        if "engine" in opt["parts"]:
            for task in opt["tasks"]:
                jobs = engine_jobs(outdir, task, opt["seeds"][0], opt["quick"]) + jobs
        L2.pool(jobs, opt["jobs"])
        if "transplant" in opt["parts"]:
            gz(outdir, unpack=True)
            tj = []
            for task in opt["tasks"]:
                for s in opt["seeds"]:
                    for a in opt["transplant"]:
                        if os.path.exists(wiring_file(outdir, task, a, s)):
                            tj += transplant_jobs(outdir, task, a, s, opt)
            L2.pool(list(dict((j[1], j) for j in tj).values()), opt["jobs"])
        gz(outdir, unpack=False)
    import ladder3_table
    text = ladder3_table.table(outdir, opt)
    with open(os.path.join(outdir, "table.txt"), "w") as f:
        f.write(text + "\n")
    print(text)


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    if args[0] == "run":
        import ladder3_train
        task, arm, seed, sheet, out = args[1:6]
        rest = args[6:]
        flag = lambda n, d: rest[rest.index(n) + 1] if n in rest else d
        name, _ = split_arm(arm)
        step = step_of(task, arm)
        ladder3_train.run_one(task, name, int(seed), "--val" in rest, sheet, out,
                              flag("--wiring", None), limit=int(flag("--limit", 0)),
                              lr=None if is_count(name) else step,
                              shift=int(step) if is_count(name) else None,
                              digits=int(flag("--digits", 0)))
        return 0
    name = args.pop(0)
    opt = {"seeds": [5, 6, 7], "tasks": ["mnist", "pair"], "parts": ["model"], "jobs": 3,
           "val": False, "table_only": False, "limit": 0, "digits": 0, "arms": list(LADDER),
           "coins": True, "transplant": [], "settle": 3000, "quick": False}
    if "--quick" in args:
        opt.update(seeds=[5], limit=10000, digits=10000, settle=500, quick=True,
                   arms=["born", "T1b", "T2", "T3", "T7", "T8", "A0"],
                   parts=["model", "transplant", "engine"], transplant=["T8"])
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--seeds":
            opt["seeds"] = L2.seeds_arg(args[i + 1])
        elif a in ("--tasks", "--parts", "--arms", "--transplant"):
            opt[a[2:]] = [x for x in args[i + 1].split(",") if x]
        elif a in ("--jobs", "--limit", "--settle", "--digits"):
            opt[a[2:]] = int(args[i + 1])
        elif a == "--coins":
            opt["coins"] = args[i + 1] != "0"
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
