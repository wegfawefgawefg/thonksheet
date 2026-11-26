"""The five tests of scripts/ratecontrol.py (idea 175): alive, the standard read, the latch,
the prediction sheet, the quick map. Each runs what is missing and returns numbers per seed;
ratecontrol.py makes the tables. Lives and probes come from timerule_lib, loops_probe,
repeat and oneset (imported, not changed)."""

import os
import re
import shutil
import subprocess

import loops_probe as lp
import oneset as O
import repeat as R
import timerule_lib as tl
from timerule_lib import PRED, ROOT, SUITE, delayed, label_key

MAIN = "/media/vega/mdot2b/Coding2/AI/thonksheet/out/from-agents"
PROBE = os.path.join(ROOT, "build", "blobprobe")
BENCH = os.path.join(ROOT, "build", "blobbench")
LIFE = ["--ticks", "20", "--sensor-side", "3", "--motor-side", "3"]
WORLDS = {"far3": ["--task", "echo", "--distance", "3"], "delayed2": delayed(2)}
MAP_COLS = ["base", "a2rucomp", "best"]
MUST = ["xor", "echo16", "order", "mnist"]


def settings():
    """the three settings of the alive test, from the quick map's columns"""
    _, cols = O.read_map(O.MORE)
    return {"base": cols["base"], "comp": cols["a2comp"], "best": cols["best"]}


def split(flags):
    """genome numbers (they go on the probe's own line) and the rest (into --life)"""
    genome, rest = [], []
    for i in range(0, len(flags) - 1, 2):
        (genome if flags[i].startswith("--g.") else rest).extend(flags[i:i + 2])
    return genome, rest


def run_jobs(jobs, opt):
    """jobs: (command, log). a finished run (its log there) is kept"""
    tl.run_all([(cmd, log, log) for cmd, log in jobs], opt)


# ---- 1. alive ----

def health_job(folder, sname, tag, world, setting, cand, seed, opt):
    steps = {"far3": 160000, "delayed2": 160000, "mnist": 20000}[tag]
    if opt["quick"]:
        steps = 1000 if tag == "mnist" else 2000
    flags = O.merged(LIFE, world, setting + cand)
    genome, rest = split(flags)
    life = " ".join(rest + ["--steps", str(steps), "--window", "1000"])
    log = os.path.join(folder, "%s-%s-s%d.txt" % (sname, tag, seed))
    cmd = (["nice", "-n", "19", PROBE, "ratehealth", "--seed", str(seed), "--life", life,
            "--measure-steps", "200" if opt["quick"] else "820"] + genome)
    return cmd, log


def parse_health(path):
    out = {}
    if not os.path.exists(path):
        return out
    text = open(path).read()
    m = re.search(r"^life: late ([0-9.]+)", text, re.M)
    out["late"] = float(m.group(1)) if m else None
    m = re.search(r"health: rate ([0-9.]+) per 1024 cell-ticks \(target (\d+)\), silent ([0-9.]+), "
                  r"stuck ([0-9.]+); hidden thresholds at cap ([0-9.]+), at floor ([0-9.]+); sector "
                  r"bias mean (-?[0-9.]+), at \+max ([0-9.]+), at -max ([0-9.]+)", text)
    if m:
        v = [float(x) for x in m.groups()]
        out.update(rate=v[0] / v[1], silent=v[2], stuck=v[3], cap=v[4], floor=v[5], bias=v[6],
                   top=v[7], low=v[8])
    for k, n, s, st in re.findall(r"kind (\w+): (\d+) cells, silent ([0-9.]+), stuck ([0-9.]+)", text):
        out["silent_" + k] = float(s)
        out["stuck_" + k] = float(st)
    m = re.search(r"change: rate before ([0-9.]+), in the gap ([0-9.]+), first bin back ([0-9.]+) "
                  r"\(jump ([0-9.]+)\)", text)
    if m:
        out.update(before=float(m.group(1)), gap=float(m.group(2)), back=float(m.group(3)),
                   jump=float(m.group(4)))
    return out


def stage0_job(folder, cand, seed, opt):
    ticks = "20000" if opt["quick"] else "200000"
    window = "10000" if opt["quick"] else "50000"
    log = os.path.join(folder, "stage0-s%d.txt" % seed)
    cmd = (["nice", "-n", "19", BENCH, "stage0", "--ticks", ticks, "--window", window, "--csv",
            "0", "--seed", str(seed)] + cand)
    return cmd, log


def parse_stage0(path):
    """the last window of each phase: rate, sector low and high, dead sectors, saturated and
    silent ticks"""
    out = {}
    if not os.path.exists(path):
        return out
    for line in open(path):
        p = line.split()
        if len(p) > 8 and p[0] in ("idle", "random", "onoff") and p[1].isdigit():
            out[p[0]] = {"rate": int(p[2]), "lo": int(p[3]), "hi": int(p[4]), "dead": int(p[5]),
                         "satur": int(p[7]), "silent": int(p[8]), "thr": float(p[12])}
    return out


def alive(name, cand, opt):
    """{setting: {world: [health per seed]}}, and stage 0 per seed"""
    folder = os.path.join(opt["out"], "alive", name)
    os.makedirs(folder, exist_ok=True)
    jobs, where = [], {}
    mnist_row = dict(O.read_map(O.MORE)[0])["mnist"]
    for sname, setting in settings().items():
        for tag, world in list(WORLDS.items()) + [("mnist", mnist_row)]:
            for s in opt["seeds"]:
                cmd, log = health_job(folder, sname, tag, world, setting, cand, s, opt)
                jobs.append((cmd, log))
                where.setdefault((sname, tag), []).append(log)
    for s in opt["seeds"]:
        cmd, log = stage0_job(folder, cand, s, opt)
        jobs.append((cmd, log))
        where.setdefault(("stage0", ""), []).append(log)
    # the long lives first, so the pool ends on short runs
    jobs.sort(key=lambda j: "mnist" in j[1] or "stage0" in j[1])
    run_jobs(jobs, opt)
    res = {k: [parse_health(p) for p in v] for k, v in where.items() if k[0] != "stage0"}
    res["stage0"] = [parse_stage0(p) for p in where["stage0", ""]]
    return res


# ---- 2. the standard read ----

def reuse_repeat(outdir):
    """the base and frozen reads of the repeatability job (the same code, the born sheet)"""
    src = os.path.join(MAIN, "repeat", "arms")
    for f in os.listdir(src) if os.path.isdir(src) else []:
        if re.match(r"(base|freeze)-born-(mnist|pair)-\d+\.txt$", f) and not os.path.exists(
                os.path.join(outdir, f)):
            shutil.copyfile(os.path.join(src, f), os.path.join(outdir, f))


def read(name, cand, opt):
    """{task: [{reader, corr, rate} per seed]} of `blobmnist ceiling --twice 1`"""
    outdir = os.path.join(opt["out"], "repeat")
    os.makedirs(outdir, exist_ok=True)
    if not opt["quick"]:
        reuse_repeat(outdir)
    # the standard control is the repeatability job's base arm
    arm = "base" if name == "std" else "rc-" + name
    if arm not in R.ARM:
        R.ARMS.append((arm, "rate control " + name, cand))
        R.ARM[arm] = R.ARMS[-1]
    q = opt["quick"]
    ropt = {"seeds": opt["seeds"], "tasks": ["mnist", "pair"], "kinds": ["born"],
            "arms": [arm], "settle": 200 if q else 3000,
            "size": ["--fit", "3000", "--test-count", "1000"] if q else ["--fit", "60000"]}
    if not opt["table_only"]:
        R.pool(R.jobs_for(outdir, ropt), opt["jobs"])
    return {t: [R.parse(R.run_file(outdir, arm, "born", t, s)) for s in opt["seeds"]]
            for t in ("mnist", "pair")}


# ---- 3. the latch ----

def reuse_loops(opt):
    """the loops job's traces and lives, and the timerule job's lives (the same code)"""
    if opt["quick"] or opt["table_only"]:
        return
    for sub in ("trace", "pred"):
        src, dst = os.path.join(MAIN, "loops", sub), os.path.join(opt["out"], sub)
        os.makedirs(dst, exist_ok=True)
        for f in os.listdir(src) if os.path.isdir(src) else []:
            if f.endswith(".txt") and not os.path.exists(os.path.join(dst, f)):
                shutil.copyfile(os.path.join(src, f), os.path.join(dst, f))
    lives = os.path.join(opt["out"], "lives")
    os.makedirs(lives, exist_ok=True)
    for src in (os.path.join(MAIN, "timerule", "lives"), os.path.join(MAIN, "loops", "lives")):
        for f in os.listdir(src) if os.path.isdir(src) else []:
            if f.endswith("-sum.csv") and not os.path.exists(os.path.join(lives, f)):
                shutil.copyfile(os.path.join(src, f), os.path.join(lives, f))


def cand_arm(name, cand):
    """the rule-on arm (the tile's winners and competitive learning) under the candidate"""
    arm = "rc-" + name
    tl.ARMS[arm] = (tl.ARMS["on"] + " " + " ".join(cand)).strip()
    return arm


# the std rows of the latch: the loops job's (lived at cap 64), and the life at cap 1000
STD_LATCH = [("loop inputs all strong", "on", ["--loop-strong", "1"]),
             ("loop inputs all strong, cap 1000", "on",
              ["--loop-strong", "1", "--after", "threshold_cap 1000"])]


def latch(name, cand, opt):
    reuse_loops(opt)
    arm = cand_arm(name, cand)
    sheets = [("%s: loop inputs all strong" % name, SUITE, arm, ["--loop-strong", "1"] + cand)]
    if name == "std":
        sheets = [(n, SUITE, a, f) for n, a, f in STD_LATCH]
    elif name == "e":
        sheets[0] = (sheets[0][0], SUITE, "oncap", ["--loop-strong", "1"] + cand)
    traces = lp.trace_grid(os.path.join(opt["out"], "trace"), sheets, opt)
    rows = [("delayed echo, %d steps" % d, delayed(d), SUITE) for d in (2, 3, 4, 8)]
    life_arm = {"std": "on", "e": "oncap"}.get(name, arm)
    echo = tl.grid(os.path.join(opt["out"], "lives"), rows, [life_arm], opt, by_args=True)
    return {"traces": {n: traces[n] for n, _, _, _ in sheets},
            "echo": {label: echo[label, life_arm] for label, _, _ in rows}}


# ---- 4. the prediction sheet ----

def pred(name, cand, opt):
    reuse_loops(opt)
    arm = {"std": "on", "e": "oncap"}.get(name) or cand_arm(name, cand)
    sheet = ("side 2, wide lattice, " + ("on" if name == "std" else
                                          "oncap" if name == "e" else arm), dict(PRED, delay=2),
             arm, [] if name in ("std", "e") else cand)
    res = lp.trace_grid(os.path.join(opt["out"], "pred"), [sheet], opt)
    return res[sheet[0]]


# ---- 5. the quick map ----

def map_name(name, col):
    return "rc-%s-%s" % (name, col)


def quick_map(name, cand, opt):
    """runs the three columns with the candidate's flags through scripts/oneset.py (its cache in
    out/oneset/); returns {col: {row: (late per seed, passes)}} for the column and with it"""
    _, cols = O.read_map(O.MORE)
    seeds = ",".join(map(str, opt["seeds"]))
    rows = ["echo2", "xor"] if opt["quick"] else None
    for col in MAP_COLS:
        if name == "std" or opt["table_only"]:
            continue
        cmd = ["python3", os.path.join(ROOT, "scripts", "oneset.py"), "run", map_name(name, col),
               "--seeds", seeds, "--jobs", str(opt["jobs"])] + (["--rows", ",".join(rows)]
                                                                 if rows else [])
        cmd += ["--"] + cols[col] + cand
        subprocess.run(cmd, cwd=ROOT, stdout=subprocess.DEVNULL)
    cache = O.load_cache()
    maprows = O.pick_rows(O.read_map(O.MORE)[0], ",".join(rows) if rows else "")
    out = {}
    for col in MAP_COLS:
        for tag, flags in (("without", cols[col]), ("with", cols[col] + cand)):
            for rname, rargs in maprows:
                late = O.scores(cache, rargs, flags, opt["seeds"], False)
                twin = O.scores(cache, rargs, flags, opt["seeds"], True)
                if None in late or None in twin:
                    out[col, tag, rname] = (late, None)
                    continue
                chance, perfect = O.calib_of(cache, rargs)
                out[col, tag, rname] = (late, O.judge(late, twin, chance, perfect)[1])
    return out, [r for r, _ in maprows]


def label_of(name):
    return label_key(name)
