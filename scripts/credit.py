#!/usr/bin/env python3
"""The credit rules not yet tried (idea 177; docs/results-2026-11-26-credit-rules.md). See
credit.sh.

  credit.py NAME [--quick] [--seeds 5,6,7] [--tasks mnist,pair] [--arms P2,pay,...]
            [--coins 0|1] [--parts model,transplant] [--jobs 3] [--val] [--table-only]
            [--digits N] [--limit N] [--settle 3000] [--read TAG "FLAGS"]
  credit.py run TASK ARM SEED SHEET OUT [--val] [--limit N] [--digits N] [--wiring F]

An arm is a rule (credit_train.ARMS), or one of the ladder's own: T7x (the one-hop rule with
every-event counters, run by the ladder's code) and born (idea 165's born arm). NAME@V runs a
two-phase arm at gain V instead of its pick (PICKS). Every arm but born and those in NOCOIN
gets a coin arm (NAMEcoin: its teaching signal from another digit) unless --coins 0. Parts:
model (the life in the offline model, its own reader, its wiring written out), transplant
(every wiring written into the real sheet with `blobmnist ceiling --wiring-load`, beside the
born sheet read the same way; --read TAG "FLAGS" reads with extra sheet flags into files
tagged TAG, as --read cap "--g.threshold_cap 1000" for a read at the target firing rate).
The real sheet's number is the one to rank by.
"""

import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import ladder2 as L2  # noqa: E402  the job pool, the describe job
import ladder3 as L3  # noqa: E402  the ladder's model and transplant jobs

ROOT = L2.ROOT
PY = sys.executable
LADDER_ARMS = ("T7x", "born")
DEFAULT = ["born", "still", "T7x", "P2", "P2s", "C2", "C1", "C2n192", "C2n12", "pay1", "pay",
           "payc", "tgt", "dtp", "hopb", "dtpb", "d1n", "d1t2", "d1t4", "d3t4", "d0t4"]
# no coin arm: the scaling arms, the conserved payment, and the time arms (the no-trace arm
# d1n is their control: an error from another digit on this digit's marks)
NOCOIN = {"born", "still", "C2n192", "C2n12", "payc", "d1n", "d1t2", "d1t4", "d3t4", "d0t4"}
# the two-phase gain (drive a tick that an output line at full rate adds to a cell), picked
# on the validation slice by the real sheet's read (the doc's "Picking")
PICKS = {"mnist": {"P2s": 0.0625, "P2": 0.0625}, "pair": {"P2s": 0.0625, "P2": 0.0625}}


def split_arm(arm):
    name, _, v = arm.partition("@")
    return name, v


def base_name(arm):
    name, _ = split_arm(arm)
    return name[:-4] if name.endswith("coin") else name


def model_job(outdir, task, arm, seed, opt):
    if base_name(arm) in LADDER_ARMS:
        return L3.model_job(outdir, task, arm, seed, opt, wiring=arm != "born")
    out = os.path.join(outdir, "%s-%s-%d.json" % (task, arm, seed))
    cmd = [PY, os.path.abspath(__file__), "run", task, arm, str(seed),
           L2.sheet_file(outdir, seed), out, "--limit", str(opt["limit"]), "--wiring",
           L3.wiring_file(outdir, task, arm, seed)] + (["--val"] if opt["val"] else [])
    if opt["digits"]:
        cmd += ["--digits", str(opt["digits"])]
    return L2.nice(cmd), out


def with_coins(opt):
    out = []
    for a in opt["arms"]:
        out.append(a)
        name, v = split_arm(a)
        if opt["coins"] and name not in NOCOIN and not name.endswith("coin"):
            out.append(name + "coin" + ("@" + v if v else ""))
    return out


def retag(out, tag):
    d, name = os.path.split(out)
    for kind in ("transplant", "bornread"):
        if name.startswith(kind + "-"):
            name = kind + tag + name[len(kind):]
    return os.path.join(d, name)


def bench(name, opt):
    outdir = os.path.join(ROOT, "out", "credit", name)
    os.makedirs(outdir, exist_ok=True)
    arms = with_coins(opt)
    if not opt["table_only"]:
        L2.pool([L2.describe_job(outdir, s) for s in opt["seeds"]], opt["jobs"])
        if "model" in opt["parts"]:
            jobs = [model_job(outdir, task, a, s, opt) for s in opt["seeds"]
                    for task in opt["tasks"] for a in arms]
            L2.pool(jobs, opt["jobs"])
        if "transplant" in opt["parts"]:
            L3.gz(outdir, unpack=True)
            tj = []
            for s in opt["seeds"]:
                for task in opt["tasks"]:
                    for a in arms:
                        done = os.path.join(outdir, "%s-%s-%d.json" % (task, a, s))
                        if a != "born" and os.path.exists(done) and os.path.exists(
                                L3.wiring_file(outdir, task, a, s)):
                            tj += L3.transplant_jobs(outdir, task, a, s, opt)
            if opt["read_tag"]:  # read with extra sheet flags, into files of their own
                tj = [(cmd + opt["read_flags"], retag(out, opt["read_tag"])) for cmd, out in tj]
            L2.pool(list(dict((j[1], j) for j in tj).values()), opt["jobs"])
        L3.gz(outdir, unpack=False)
    import credit_table
    text = credit_table.table(outdir, opt)
    tag = "-" + opt["read_tag"] if opt["read_tag"] else ""
    with open(os.path.join(outdir, "table%s.md" % tag), "w") as f:
        f.write(credit_table.markdown(outdir, opt) + "\n")
    with open(os.path.join(outdir, "table%s.txt" % tag), "w") as f:
        f.write(text + "\n")
    print(text)


def main():
    args = sys.argv[1:]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 1
    if args[0] == "run":
        import credit_train
        task, arm, seed, sheet, out = args[1:6]
        rest = args[6:]
        flag = lambda n, d: rest[rest.index(n) + 1] if n in rest else d
        name, v = split_arm(arm)
        value = v or PICKS[task].get(base_name(arm))
        credit_train.run_one(task, name, int(seed), "--val" in rest, sheet, out,
                             flag("--wiring", None), limit=int(flag("--limit", 0)),
                             value=value, digits=int(flag("--digits", 0)))
        return 0
    name = args.pop(0)
    opt = {"seeds": [5, 6, 7], "tasks": ["mnist", "pair"], "parts": ["model", "transplant"],
           "jobs": 3, "val": False, "table_only": False, "limit": 0, "digits": 0,
           "arms": list(DEFAULT), "coins": True, "settle": 3000, "quick": False,
           "read_tag": "", "read_flags": []}
    if "--quick" in args:
        opt.update(seeds=[5], limit=10000, digits=10000, settle=500, quick=True, coins=False,
                   arms=["born", "T7x", "hop", "P2", "P2s", "C2", "C1", "pay", "payc", "tgt",
                         "dtp", "hopb", "d1t2", "paycoin"])
    i = 0
    while i < len(args):
        a = args[i]
        if a == "--seeds":
            opt["seeds"] = L2.seeds_arg(args[i + 1])
        elif a in ("--tasks", "--parts", "--arms"):
            opt[a[2:]] = [x for x in args[i + 1].split(",") if x]
        elif a in ("--jobs", "--limit", "--settle", "--digits"):
            opt[a[2:]] = int(args[i + 1])
        elif a == "--coins":
            opt["coins"] = args[i + 1] != "0"
        elif a == "--read":  # --read TAG "FLAGS"
            opt["read_tag"], opt["read_flags"] = args[i + 1], args[i + 2].split()
            i += 1
        elif a in ("--val", "--table-only", "--quick"):
            opt[a[2:].replace("-", "_")] = True
            i -= 1
        else:
            print("unknown option", a)
            return 1
        i += 2
    bench(name, opt)
    return 0


if __name__ == "__main__":
    sys.exit(main())
