#!/usr/bin/env python3
"""Motor cells regulated apart from hidden cells, and a chain row a coin cannot pass (idea
184; docs/results-2026-11-26-motor-apart.md). See motorapart.sh. The screen, the chain and
the lean run `blobprobe capsplit` (a quick-map life with health by kind of cell and what was
pressed); the fresh readers, the map, the inside test, the long lives and the speed check are
in motorapart_more.py."""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import capdefault as C  # noqa: E402  the probe's reader and the job pool
import oneset as O  # noqa: E402

ROOT = O.ROOT
PROBE = os.path.join(ROOT, "build", "blobprobe")
CAP = "--g.threshold_cap 1000"
# arm: (what, genome numbers). all at the default cap of 64 unless said
ARMS = {
    "std": ("the default: motor cells in the sector's count, under its bias", ""),
    "out1": ("motor cells out of the sector's count and target; they still feel its bias",
             "--g.motor_apart 1"),
    "out2": ("motor cells out of the sector's count and of its bias", "--g.motor_apart 2"),
    "own": ("a bias per kind of cell: the motor cells' own, toward 32 per 1,024",
            "--g.motor_apart 3"),
    "own128": ("a bias and a walk of their own for motor cells, both toward 128 per 1,024",
               "--g.motor_apart 3 --g.motor_target_per_1024 128 "
               "--g.threshold_down_shift_motor 7"),
    "walk128": ("motor cells out of the count, with their own walk: target 128, cap 1000",
                "--g.motor_apart 1 --g.threshold_down_shift_motor 7 "
                "--g.threshold_cap_motor 1000"),
    "walkonly": ("their own walk alone (target 128, cap 1000), still in the sector's count",
                 "--g.threshold_down_shift_motor 7 --g.threshold_cap_motor 1000"),
    "m64": ("the cap by kind: 1000 for the rest, 64 for motor cells (the cap doc's probe arm)",
            CAP + " --g.threshold_cap_motor 64"),
    "out1h": ("out1 with the cap by kind: the rest at 1000, motor cells at 64",
              "--g.motor_apart 1 " + CAP + " --g.threshold_cap_motor 64"),
    "c1000": ("cap 1000 for every cell", CAP),
}
COLS = ["base", "a2rucomp", "best"]
SCREEN = (["mnist", "echo2", "bandit", "xor", "far3"], COLS,
          ["std", "out1", "out2", "own", "own128", "walk128", "walkonly"], [5, 6, 7])
# the chain's forms (every one scored on the opening press; the probe prints chains paid too)
FORMS = {"k2": "", "k3": "--k 3", "k4": "--k 4"}
# (name, quick-map column, genome numbers)
CHAIN = [("base", "base", ""), ("base@1000", "base", CAP), ("a2rucomp", "a2rucomp", ""),
         ("a2rucomp@1000", "a2rucomp", CAP), ("best", "best", ""), ("best@1000", "best", CAP),
         ("critic", "critic", ""), ("critic@1000", "critic", CAP)]
# the one setting's pieces (scripts/variants/best.txt), for the lean
BASE = "--g.learn_mode 5 --g.readout_evidence 2 --g.readout_shift 4 --g.readout_balance 1 " \
       "--motor-spread 128"
PIECES = {"timehold": "--g.time_percent 10 --g.hold_percent 10",
          "margin": "--g.readout_good 1 --g.margin_percent 20 --g.margin_win 2",
          "tire": "--g.tire_jump 4 --g.tire_shift 7", "winners": "--g.tile_winners 2",
          "walk": "--g.replay_depth 1 --g.replay_bad_only 1"}


def lean_cols():
    """the one setting, it less each piece, the base rule, and it with each piece"""
    out = {"best": BASE + " " + " ".join(PIECES.values())}
    for p in PIECES:
        out["best-" + p] = BASE + " " + " ".join(v for k, v in PIECES.items() if k != p)
    out["base"] = BASE
    for p, v in PIECES.items():
        out["base+" + p] = BASE + " " + v
    return out


def look_file(opt, part, row, col, arm, seed):
    return os.path.join(opt["out"], part, "%s-%s-%s-s%d.txt" % (row, col, arm, seed))


def look_job(opt, part, row_flags, col_flags, genome, world, name, seed, quick_steps="2000"):
    """one quick-map life through the probe. name: (row, column, arm) for the file"""
    flags = O.merged(O.LIFE, row_flags, world.split(), col_flags + genome.split())
    if opt["quick"]:
        flags = O.merged(flags, ["--steps", quick_steps, "--window", "200"])
    own, life = [], []
    i = 0
    while i < len(flags) - 1:
        width = O.TWO_VALUES.get(flags[i], 1)
        (own if flags[i].startswith("--g.") else life).extend(flags[i:i + 1 + width])
        i += 1 + width
    cmd = (["nice", "-n", "19", PROBE, "capsplit", "--seed", str(seed), "--life", " ".join(life)]
           + (["--look-steps", "400"] if opt["quick"] else []) + own)
    return cmd, look_file(opt, part, name[0], name[1], name[2], seed)


def parse(path):
    """capdefault's reading of a probe run, with this job's two lines"""
    p = C.parse_look(path)
    if not p:
        return None
    text = open(path).read()
    m = re.search(r"^sector bias at -max ([0-9.]+); motor bias mean (-?[0-9.]+), at \+max "
                  r"([0-9.]+)", text, re.M)
    if m:
        p["biaslow"], p["mbias"], p["mbiastop"] = (float(x) for x in m.groups())
    m = re.search(r"^chain opened (\d+) times: A at once ([0-9.]+), B ([0-9.]+), another button "
                  r"([0-9.]+), nothing ([0-9.]+); chains paid x 2 / steps ([0-9.]+)", text, re.M)
    if m:
        p["opened"] = int(m.group(1))
        p["openA"], p["openB"], p["openC"], p["open0"], p["paid"] = (
            float(x) for x in m.groups()[1:])
    return p


def pick(opt, key, default):
    return opt.get(key) or default


def seeds_for(opt, default):
    return [5] if opt["quick"] else (opt["seeds"] or default)


# ---- part 1: the screen ----

def screen_jobs(opt):
    rows, cols = O.read_map(O.MORE)
    rows = dict(rows)
    rnames, cnames, arms, seeds = SCREEN
    rnames, cnames, arms = pick(opt, "rows", rnames), pick(opt, "cols", cnames), pick(
        opt, "arms", arms)
    if opt["quick"]:
        rnames, cnames, arms = rnames[:2], cnames[:1], arms[:3]
    jobs = []
    for row in rnames:
        for col in cnames:
            for arm in arms:
                for s in seeds_for(opt, seeds):
                    jobs.append(look_job(opt, "look", rows[row], cols[col], ARMS[arm][1], "",
                                         (row, col, arm), s, "600" if row == "mnist" else "2000"))
    jobs.sort(key=lambda j: "mnist" not in j[1])
    return jobs


def screen(opt):
    if not opt["table_only"]:
        C.pool(screen_jobs(opt), opt["jobs"])
    rnames, cnames, _, seeds = SCREEN
    seeds = seeds_for(opt, seeds)
    lines = ["Seeds %s. late: the life's online score (the quick map's). Rates per 1,024 "
             "cell-ticks over the last 2,000 steps. hidden silent: share of plain hidden cells "
             "with no spike in them; motor stuck: share of motor cells firing on over a quarter "
             "of the ticks; bias: share of sectors with the bias at its upper / lower limit; "
             "motor bias: the motor cells' own, mean (own arms; else 0)." %
             ",".join(map(str, seeds)), ""]
    for row in pick(opt, "rows", rnames):
        lines += ["", "%s:" % row, "",
                  "| column | arm | late (lowest-highest) | hidden rate | motor rate | hidden "
                  "silent | motor stuck | motor at 64+ | bias at +max / -max | motor bias | seeds |",
                  "|---|---|---|---|---|---|---|---|---|---|---|"]
        for col in pick(opt, "cols", cnames):
            for arm in pick(opt, "arms", list(ARMS)):
                got = [p for p in (parse(look_file(opt, "look", row, col, arm, s)) for s in seeds)
                       if p]
                if not got:
                    continue
                late = [p["late"] for p in got]
                k = lambda kind, key, d=3: C.fmt(C.mean(  # noqa: E731
                    [p.get(kind, {}).get(key) for p in got]), d)
                lines.append("| %s | %s | %s (%s-%s) | %s | %s | %s | %s | %s | %s / %s | %s | %d |"
                             % (col, arm, C.fmt(C.mean(late)), C.fmt(min(late), 2),
                                C.fmt(max(late), 2), k("hidden", "rate", 1), k("motor", "rate", 1),
                                k("hidden", "silent"), k("motor", "stuck"), k("motor", "high", 2),
                                C.fmt(C.mean([p.get("biastop") for p in got]), 2),
                                C.fmt(C.mean([p.get("biaslow") for p in got]), 2),
                                C.fmt(C.mean([p.get("mbias") for p in got]), 1), len(got)))
    lines += ["", "Arms: " + "; ".join("%s: %s (`%s`)" % (a, ARMS[a][0], ARMS[a][1] or "no flags")
                                       for a in ARMS)]
    return "\n".join(lines)


# ---- part 3: the chain's forms ----

def chain_jobs(opt, settings, forms, seeds):
    rows, cols = O.read_map(O.MORE)
    rows = dict(rows)
    jobs = []
    for form in forms:
        for name, col, genome in settings:
            for s in seeds:
                jobs.append(look_job(opt, "chain", rows["chain"], cols[col], genome,
                                     FORMS[form] + " --chain-first 1", (form, name, "c"), s))
    return jobs


def chain_rows(opt, part, form, names, seeds, lines):
    for name in names:
        got = [parse(look_file(opt, part, form, name, "c", s)) for s in seeds]
        have = [p for p in got if p and "openA" in p]
        if not have:
            continue
        m = lambda key: C.fmt(C.mean([p[key] for p in have]), 2)  # noqa: E731
        a = [p["openA"] for p in have]
        d = [p["openA"] - p["openC"] for p in have]
        se = (sum((x - C.mean(d)) ** 2 for x in d) / (len(d) - 1) / len(d)) ** 0.5 \
            if len(d) > 1 else 0
        lines.append("| %s | %s (%s-%s) | %s / %s / %s | %+.2f ±%.2f | %s | %s | %s |" % (
            name, C.fmt(C.mean(a)), C.fmt(min(a), 2), C.fmt(max(a), 2), m("openB"), m("openC"),
            m("open0"), C.mean(d), se, m("paid"), C.fmt(C.mean([p["opened"] for p in have]), 0),
            " ".join(C.fmt(p["openA"], 2) if p and "openA" in p else "-" for p in got)))


CHAIN_HEAD = ["| setting | A at once (lowest-highest) | B / another button / nothing at once | A "
              "less another | chains paid x 2 / steps | chains opened | A at once per seed |",
              "|---|---|---|---|---|---|---|"]


def chain(opt):
    seeds = seeds_for(opt, list(range(5, 13)))
    forms = ["k3"] if opt["quick"] else pick(opt, "rows", list(FORMS))
    settings = CHAIN[:2] if opt["quick"] else [c for c in CHAIN if not opt.get("cols") or
                                                c[0] in opt["cols"]]
    if not opt["table_only"]:
        C.pool(chain_jobs(opt, settings, forms, seeds), opt["jobs"])
    lines = ["The chain, each form, seeds %s, the last 2,000 steps. A at once: of the stage-0 "
             "steps that open a chain (the step after a stage-1 step), the share on which A was "
             "pressed; this is the score of `--chain-first 1`. A less another: A's share less "
             "the share of the buttons that are neither A nor B, with its standard error. "
             "chains paid x 2 / steps: the old row's score on the same life." %
             ",".join(map(str, seeds))]
    for form in forms:
        k = int(FORMS[form].split()[-1]) if FORMS[form] else 2
        lines += ["", "%s (`--task chain %s --chain-first 1`): a coin on the opening press %.3f; "
                  "\"anything but B\" or \"each button in turn\" at most %.3f; the halfway line "
                  "from the coin to perfect %.3f." % (
                      form, FORMS[form], 1 / k, 1 / (k - 1), (1 / k + 1) / 2), ""] + CHAIN_HEAD
        chain_rows(opt, "chain", form, [c[0] for c in settings], seeds, lines)
    return "\n".join(lines)


# ---- part 4: the one setting's lean to the right first press ----

def lean(opt):
    """three buttons, cap 1000 (where the paid button no longer wins the unpaid stage): the
    opening press under the one setting, it less each piece, and the base rule with each"""
    rows, _ = O.read_map(O.MORE)
    rows = dict(rows)
    cols = lean_cols()
    names = list(cols)[:2] if opt["quick"] else pick(opt, "cols", list(cols))
    seeds = seeds_for(opt, list(range(1, 13)))
    jobs = [look_job(opt, "lean", rows["chain"], cols[n].split(), CAP,
                     "--k 3 --chain-first 1", ("k3", n, "c"), s) for n in names for s in seeds]
    if not opt["table_only"]:
        C.pool(jobs, opt["jobs"])
    lines = ["Three buttons, cap 1000 (`--task chain --k 3 --chain-first 1 %s`), seeds %s, the "
             "last 2,000 steps; columns as the chain table. A coin among the buttons that are "
             "not B opens with A on 0.50." % (CAP, ",".join(map(str, seeds))), ""] + CHAIN_HEAD
    chain_rows(opt, "lean", "k3", names, seeds, lines)
    lines += ["", "Pieces: " + "; ".join("%s `%s`" % kv for kv in PIECES.items())]
    return "\n".join(lines)


def main():
    argv = sys.argv[1:]
    if not argv or argv[0].startswith("-"):
        sys.stderr.write(open(os.path.join(ROOT, "scripts", "motorapart.sh")).read().split(
            "set -")[0])
        return 1
    opt = {"seeds": None, "jobs": 3, "quick": False, "table_only": False, "arms": None,
           "rows": None, "cols": None, "skip_std": False}
    what, i = argv[0], 1
    while i < len(argv):
        a = argv[i]
        if a in ("--quick", "--table-only", "--skip-std"):
            opt[a[2:].replace("-", "_")] = True
        elif a in ("--seeds", "--jobs", "--arms", "--rows", "--cols"):
            v = argv[i + 1]
            opt[a[2:]] = (C.seeds_of(v) if a == "--seeds" else int(v) if a == "--jobs" else
                          [x for x in v.split(",") if x])
            i += 1
        else:
            print("motorapart: unknown option %s" % a)
            return 1
        i += 1
    import motorapart_more as M
    parts = {"screen": lambda: screen(opt), "reader": lambda: M.reader(opt),
             "chain": lambda: chain(opt), "lean": lambda: lean(opt),
             "map": lambda: M.quick_map(opt), "inside": lambda: M.inside_test(opt),
             "alive": lambda: M.alive(opt), "speed": lambda: M.speed(opt)}
    names = list(parts) if what in ("smoke", "table") else what.split(",")
    if what == "smoke":
        opt["quick"] = True
    if what == "table":
        opt["table_only"] = True
    opt["out"] = os.path.join(ROOT, "out", "motorapart" + ("-quick" if opt["quick"] else ""))
    os.makedirs(opt["out"], exist_ok=True)
    if any(n not in parts for n in names):
        print("motorapart: parts are %s (or smoke, table)" % ", ".join(parts))
        return 1
    for n in names:
        text = parts[n]()
        tag = n + ("-" + "-".join(opt["arms"]) if opt["arms"] and n in ("map", "inside") else "")
        with open(os.path.join(opt["out"], "table-%s.md" % tag), "w") as f:
            f.write(text + "\n")
        print("## %s\n%s\n" % (n, text), flush=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
