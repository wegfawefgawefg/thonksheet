#!/usr/bin/env python3
"""A harder real-data test with more room (idea 185; docs/results-2026-11-26-harder-data.md).
See scripts/harderdata.sh. Each NAME is a fixed recipe; one line runs it and prints its tables.

For each data set (MNIST's own file format; scripts/get_data.sh) and each test (class: the
ten classes; pair: are two interleaved half-size images the same class) this runs, on
scripts/mnist.sh's sheet and reader with nothing changed but the data folder:
  bars      the plain learners on the bits (blobmnist baselines): linear, a two-layer net
  born      the never-taught sheet, a fresh linear reader of its non-sensor cells
  teacher   the backprop teacher's bound (a measuring stick, never the method): idea 165's
            model of the cells at its bottom rung, trained, its wiring written into the real
            sheet and read (scripts/ladder2.py's rung 7b, the rule ladder's T0)
  compete   competitive learning, a life with no reward
  rule      the live every-event one-hop rule taught by an outside output's error
            (scripts/everyevent.py's arms out, outf3, outint2), each with its coin
  quiet     born and the teacher's wiring read the quiet way (--read-freeze 3 --ticks 80)
"""

import gzip
import os
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import everyevent as EE  # noqa: E402  the rule's arms and the reading of one run
import inside  # noqa: E402  the sheet and its reader

MNIST = os.path.join(ROOT, os.environ.get("HARDERDATA_BUILD", "build"), "blobmnist")
PY = sys.executable
SETS = ["mnist", "fashion", "kmnist", "kannada"]
TESTS = {"class": [], "pair": "--pair 2 --pair-mix 1".split()}
COMPETE = "--g.learn_mode 5 --g.compete_on 1 --g.tile_winners 4 --g.compete_shift 5"
QUIET = "--read-freeze 3 --ticks 80".split()
RULE = ["out", "outf3", "outint2"]
PARTS = ["bars", "born", "teacher", "compete", "rule"]

# name -> what it runs. life: digits of a lived arm's life (the file name carries it)
RECIPES = {
    # the room: born, the plain learners and the teacher's bound, no lives
    "room": {"parts": ["bars", "born", "teacher"], "life": 0, "coins": False},
    # short lives of every lived arm, no coins: which arms and sets matter
    "screen": {"parts": ["born", "compete", "rule"], "life": 20000, "coins": False},
    # the report: 60,000-digit lives with coins
    "full": {"parts": PARTS, "life": 60000, "coins": True},
    # the room read the quiet way
    "quiet": {"parts": ["quiet"], "life": 0, "coins": False},
    # every part once, one seed (use with --quick)
    "smoke": {"parts": PARTS + ["quiet"], "life": 60000, "coins": True, "sets": ["fashion"]},
}


def data_dir(name):
    """a set's folder: data/NAME; MNIST also from THONK_MNIST, as everywhere else"""
    if name == "mnist" and os.environ.get("THONK_MNIST"):
        return os.environ["THONK_MNIST"]
    return os.path.join("data", name)


def run(job):
    """a job is (command, the file it makes). a model run writes its own json and its output
    goes beside it; any other command's output is the file"""
    cmd, out, data = job
    if os.path.exists(out):
        return
    env = dict(os.environ, OMP_NUM_THREADS="1", MKL_NUM_THREADS="1",
               THONK_MNIST=os.path.join(ROOT, data) if not os.path.isabs(data) else data)
    own = out.endswith(".json")
    log = out[:-5] + ".out" if own else out + ".part"
    with open(log, "w") as f:
        f.write(" ".join(cmd[3:]) + "\n")
        f.flush()
        r = subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT, cwd=ROOT, env=env)
    if r.returncode == 0 and not own:
        os.replace(log, out)


def pool(jobs, n):
    with ThreadPoolExecutor(max_workers=n) as ex:
        list(ex.map(run, jobs))


def nice(cmd):
    return ["nice", "-n", "19"] + cmd


def ceiling(opt, test, seed, more, data):
    cmd = [MNIST, "ceiling"] + inside.SHEET + inside.READER + [
        "--fit", str(opt["fit"]), "--seed", str(seed)] + more + TESTS[test]
    cmd += ["--test-count", "2000"] if opt["quick"] else []
    return nice(cmd + ["--data", data])


def life_tag(opt):
    return "k%d" % (opt["life"] // 1000)


def lived_runs(opt):
    """(run name, life flags) of every lived arm asked for"""
    runs = []
    if "compete" in opt["parts"]:
        runs.append(("compete", COMPETE))
    if "rule" in opt["parts"]:
        for arm in RULE:
            flags, coin = EE.ARMS[arm]
            runs.append((arm, flags))
            if opt["coins"]:
                runs.append((arm + "-coin", flags + " " + coin))
    return runs


def wiring_file(outdir, test, seed):
    return os.path.join(outdir, "wiring-%s-%d.txt" % (test, seed))


def first_jobs(opt, name, outdir):
    """everything but the reads of the teacher's wiring"""
    data, jobs = data_dir(name), []
    f = lambda s: os.path.join(outdir, s)
    for test in opt["tests"]:
        if "bars" in opt["parts"]:
            for passes in (1, 5):
                cmd = [MNIST, "baselines", "--mnist-code", "t28", "--net-passes", str(passes)]
                jobs.append((nice(cmd + TESTS[test] + ["--data", data]),
                             f("%s-bars-%d.txt" % (test, passes)), data))
        for s in opt["seeds"]:
            if "born" in opt["parts"]:
                jobs.append((ceiling(opt, test, s, [], data), f("%s-born-%d.txt" % (test, s)), data))
            if "teacher" in opt["parts"]:
                # the model of the cells (ladder2.py): its task "mnist" is the ten classes
                task = "mnist" if test == "class" else "pair"
                base = [PY, os.path.join(ROOT, "scripts", "ladder2.py"), "run", task]
                tail = [str(s), f("sheet-%d.txt" % s)]
                size = ["--epochs", str(opt["epochs"]), "--limit", str(opt["limit"])]
                jobs.append((nice(base + ["7b", "trained"] + tail + [
                    f("%s-teacher-%d.json" % (test, s))] + size + [
                    "--wiring", wiring_file(outdir, test, s)]),
                    f("%s-teacher-%d.json" % (test, s)), data))
                jobs.append((nice(base + ["7b", "born"] + tail + [
                    f("%s-modelborn-%d.json" % (test, s))] + size),
                    f("%s-modelborn-%d.json" % (test, s)), data))
                if s == opt["seeds"][0]:  # the unconstrained net, one seed: a measuring stick
                    jobs.append((nice(base + ["net", "trained"] + tail + [
                        f("%s-net-%d.json" % (test, s))] + size),
                        f("%s-net-%d.json" % (test, s)), data))
            for run_name, flags in lived_runs(opt):
                more = ["--life", str(opt["life"])] + flags.split()
                jobs.append((ceiling(opt, test, s, more, data),
                             f("%s-%s-%s-%d.txt" % (test, run_name, life_tag(opt), s)), data))
    return jobs


def describe_jobs(opt, name, outdir):
    """the born sheet as the model needs it (one digit fitted, one scored)"""
    data = data_dir(name)
    return [(nice([MNIST, "ceiling"] + inside.SHEET + [
        "--fit", "1", "--test-count", "1", "--settle", "0", "--seed", str(s), "--views", "1",
        "--describe", os.path.join(outdir, "sheet-%d.txt" % s), "--data", data]),
        os.path.join(outdir, "describe-%d.out" % s), data) for s in opt["seeds"]]


def wiring_jobs(opt, name, outdir):
    """the teacher's wiring in the real sheet and the born sheet read the same way (after
    3,000 digits of regulation); with the quiet part, both again read the quiet way"""
    data, jobs = data_dir(name), []
    for test in opt["tests"]:
        for s in opt["seeds"]:
            w = wiring_file(outdir, test, s)
            if not os.path.exists(w):
                continue
            settle = ["--settle", str(opt["settle"])]
            reads = [("", [])] if "teacher" in opt["parts"] else []
            reads += [("-quiet", QUIET)] if "quiet" in opt["parts"] else []
            for tag, how in reads:
                jobs.append((ceiling(opt, test, s, settle + how + ["--wiring-load", w], data),
                             os.path.join(outdir, "%s-transplant%s-%d.txt" % (test, tag, s)), data))
                jobs.append((ceiling(opt, test, s, settle + how, data),
                             os.path.join(outdir, "%s-bornread%s-%d.txt" % (test, tag, s)), data))
    return jobs


def wirings(outdir, unpack):
    """wiring files are kept gzipped, unpacked while blobmnist reads them"""
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


def folder(opt, name):
    return os.path.join(ROOT, "out", "harderdata", "run-quick" if opt["quick"] else "run", name)


def parse(args, r):
    opt = {"quick": False, "table_only": False, "jobs": 3, "reverse": False,
           "seeds": [5, 6, 7], "tests": list(TESTS), "sets": r.get("sets", SETS[:3]),
           "parts": list(r["parts"]), "life": r["life"], "coins": r["coins"], "fit": 60000,
           "epochs": 12, "limit": 0, "settle": 3000}
    i = 0
    while i < len(args):
        a = args[i]
        if a in ("--quick", "--table-only", "--reverse"):
            opt[a[2:].replace("-", "_")] = True
            i += 1
            continue
        if i + 1 >= len(args):
            print("harderdata: %s needs a value" % a)
            return None
        v = args[i + 1]
        if a == "--seeds":
            lo_hi = v.split("-")
            opt["seeds"] = (list(range(int(lo_hi[0]), int(lo_hi[1]) + 1)) if len(lo_hi) == 2
                            else [int(x) for x in v.split(",")])
        elif a in ("--sets", "--tests", "--parts"):
            opt[a[2:]] = v.split(",")
        elif a in ("--jobs", "--life"):
            opt[a[2:]] = int(v)
        elif a == "--coins":
            opt["coins"] = v != "0"
        else:
            print("harderdata: unknown option %s" % a)
            return None
        i += 2
    if opt["quick"]:
        opt.update(seeds=opt["seeds"][:1], fit=5000, epochs=1, limit=10000, settle=500)
        opt["life"] = 3000 if opt["life"] else 0
    return opt


def main():
    args = sys.argv[1:]
    if not args or args[0] not in RECIPES:
        print(open(os.path.join(ROOT, "scripts", "harderdata.sh")).read().split("set -")[0])
        print("recipes: " + " ".join(RECIPES))
        return 1
    name = args.pop(0)
    opt = parse(args, RECIPES[name])
    if opt is None:
        return 1
    import harderdata_table as T
    if not opt["table_only"]:
        for s in opt["sets"]:
            os.makedirs(folder(opt, s), exist_ok=True)
        if "teacher" in opt["parts"]:
            pool([j for s in opt["sets"] for j in describe_jobs(opt, s, folder(opt, s))],
                 opt["jobs"])
        jobs = [j for s in opt["sets"] for j in first_jobs(opt, s, folder(opt, s))]
        # the long model runs first, so the pool ends on short jobs
        jobs.sort(key=lambda j: not j[1].endswith(".json"))
        pool(jobs[::-1] if opt["reverse"] else jobs, opt["jobs"])
        if "teacher" in opt["parts"] or "quiet" in opt["parts"]:
            for s in opt["sets"]:
                wirings(folder(opt, s), unpack=True)
            pool([j for s in opt["sets"] for j in wiring_jobs(opt, s, folder(opt, s))],
                 opt["jobs"])
            for s in opt["sets"]:
                wirings(folder(opt, s), unpack=False)
    text = T.tables(opt, [(s, folder(opt, s)) for s in opt["sets"]])
    print(text)
    top = os.path.dirname(folder(opt, "x"))
    os.makedirs(top, exist_ok=True)
    with open(os.path.join(top, "table-%s.txt" % name), "w") as f:
        f.write(text + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
