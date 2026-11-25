#!/usr/bin/env python3
"""One setting on the whole quick map (docs/bench.md, "One setting for everything").

    scripts/oneset.py run NAME [--seeds 5-12] [--rows a,b] [--jobs 2] [-- flags ...]
    scripts/oneset.py table NAME[,NAME...] [--seeds 5-12] [--rows a,b]
    scripts/oneset.py calibrate [--rows a,b]

The rows are the `row NAME: flags` lines of scripts/variants/oneset.txt. A column is a
setting: the flags after `--`, or with none the `NAME: flags` line of oneset.txt or of
scripts/variants/best.txt. Every life runs beside its learning-off twin (the same flags with
the learning-only genome numbers dropped and --g.learning_on 0, as scripts/sweep.py does, so
columns that differ only in learning share twins). Lives are cached by their command in
out/oneset/cache*.jsonl (a copy pulled from stevejobs can sit beside it as cache-mini.jsonl).

Score: late = mean of the last tenth of the windows. chance = coin flips, perfect = the
scripted player, both from `calibrate` (seeds 5 and 6). A row passes when its late score is
at least halfway from chance to perfect and above its twin by more than 2 standard errors
(paired by seed). `table A,B,...` prints the columns side by side, each beside the first
with the paired difference; `+` and `-` mark differences of 2 standard errors or more.
"""

import argparse
import concurrent.futures
import glob
import hashlib
import json
import os
import statistics
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from sweep import parse_seeds, twin_args  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MAP = os.path.join(ROOT, "scripts", "variants", "oneset.txt")
MORE = [MAP, os.path.join(ROOT, "scripts", "variants", "best.txt")]
OUT = os.path.join(ROOT, "out", "oneset")
LIFE = ["--ticks", "20", "--sensor-side", "3", "--motor-side", "3", "--steps", "20000",
        "--window", "1000"]


def read_map(paths):
    rows, cols = [], {}
    for path in paths:
        if not os.path.exists(path):
            continue
        for line in open(path):
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            name, _, args = line.partition(":")
            name = name.strip()
            if name.startswith("row "):
                rows.append((name[4:].strip(), args.split()))
            else:
                cols[name] = args.split()
    return rows, cols


def merged(*parts):
    """flags in order, a later value for the same flag replacing the earlier one in place"""
    order, vals = [], {}
    for part in parts:
        i = 0
        while i < len(part) - 1:
            width = TWO_VALUES.get(part[i], 1)
            if part[i] not in vals:
                order.append(part[i])
            vals[part[i]] = part[i + 1:i + 1 + width]
            i += 1 + width
    out = []
    for k in order:
        out += [k] + vals[k]
    return out


TWO_VALUES = {"--catch-size": 2}


def life_cmd(row_args, col_args, seed):
    return merged(LIFE, row_args, ["--seed", str(seed), "--task-seed", str(seed)], col_args)


def load_cache():
    cache = {}
    for path in sorted(glob.glob(os.path.join(OUT, "cache*.jsonl"))):
        for line in open(path):
            row = json.loads(line)
            cache[row["key"]] = row
    return cache


def edge(scores):
    n = max(1, len(scores) // 10)
    return statistics.mean(scores[-n:])


def run_life(blobrun, args, baselines):
    key = ("calib " if baselines else "") + " ".join(args)
    stem = os.path.join(OUT, "runs", hashlib.sha1(key.encode()).hexdigest()[:16])
    cmd = ["nice", "-n", "19", blobrun] + args + ["--out", stem + ".csv"]
    if not baselines:
        cmd.append("--no-baselines")
    done = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, text=True)
    if done.returncode != 0:
        raise RuntimeError(key + "\n" + done.stderr)
    lines = [l for l in open(stem + ".csv").read().split("\n") if l]
    head = lines[0].split(",")
    cols = {name: [float(l.split(",")[i]) for l in lines[1:]] for i, name in enumerate(head)}
    row = {"key": key, "windows": cols["blob"], "perfect": edge(cols["perfect"])}
    if baselines:
        row["coin"] = edge(cols["coin"])
    # the sheet's health at the end of the life, when blobrun wrote it
    if os.path.exists(stem + "-sum.csv"):
        for line in open(stem + "-sum.csv"):
            parts = line.strip().split(",")
            if len(parts) == 3 and parts[0] == "blob" and parts[1] in HEALTH:
                row[parts[1]] = float(parts[2])
    return row


HEALTH = {"rate_over_target", "share_silent", "share_stuck", "share_strength_5",
          "share_threshold_cap", "ticks_per_s"}


def run_all(todo, jobs, blobrun, cache_path):
    if not todo:
        return
    print("%d lives to run, %d at a time" % (len(todo), jobs), file=sys.stderr)
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futs = [pool.submit(run_life, blobrun, args, calib) for args, calib in todo]
        for n, fut in enumerate(concurrent.futures.as_completed(futs)):
            row = fut.result()
            with open(cache_path, "a") as f:
                f.write(json.dumps(row) + "\n")
            print("  %d/%d" % (n + 1, len(todo)), file=sys.stderr, flush=True)


def column_flags(name, flags, cols):
    if flags:
        return flags
    if name not in cols:
        sys.exit("no column %s in %s and no flags given" % (name, " or ".join(MORE)))
    return cols[name]


def pick_rows(rows, only):
    if not only:
        return rows
    want = only.split(",")
    return [r for r in rows if r[0] in want]


def calib_of(cache, row_args):
    coins, perfects = [], []
    for seed in (5, 6):
        key = "calib " + " ".join(life_cmd(row_args, ["--engine", "memory"], seed))
        if key in cache:
            coins.append(cache[key]["coin"])
            perfects.append(cache[key]["perfect"])
    if not coins:
        return None, None
    return statistics.mean(coins), statistics.mean(perfects)


def scores(cache, row_args, col_args, seeds, twin):
    out = []
    for s in seeds:
        args = life_cmd(row_args, twin_args(col_args) if twin else col_args, s)
        row = cache.get(" ".join(args))
        out.append(edge(row["windows"]) if row else None)
    return out


def paired(a, b):
    d = [x - y for x, y in zip(a, b)]
    m = statistics.mean(d)
    se = statistics.stdev(d) / len(d) ** 0.5 if len(d) > 1 else 0.0
    return m, se


def num(v):
    return "-" if v is None else ("%.0f" % v if abs(v) >= 10 else "%.3f" % v)


def judge(late, off, chance, perfect):
    """(normalised score, passes)"""
    m = statistics.mean(late)
    norm = None if chance is None else (m - chance) / (perfect - chance)
    d, se = paired(late, off)
    above = d > 0 and (se == 0 or d / se > 2)
    return norm, norm is not None and norm >= 0.5 and above


def print_one(name, rows, col_args, cache, seeds):
    print("## %s\n\n`%s`, seeds %s\n" % (name, " ".join(col_args), ",".join(map(str, seeds))))
    print("| row | late | twin | late - twin | chance | perfect | of the way | pass |")
    print("|---|---|---|---|---|---|---|---|")
    passed = 0
    for rname, rargs in rows:
        late = scores(cache, rargs, col_args, seeds, False)
        off = scores(cache, rargs, col_args, seeds, True)
        if None in late or None in off:
            print("| %s | missing %d of %d |" % (rname, (late + off).count(None), 2 * len(seeds)))
            continue
        chance, perfect = calib_of(cache, rargs)
        norm, ok = judge(late, off, chance, perfect)
        passed += ok
        d, se = paired(late, off)
        sd = statistics.stdev(late) if len(late) > 1 else 0.0
        print("| %s | %s ±%s | %s | %+.3f ±%.3f | %s | %s | %s | %s |" % (
            rname, num(statistics.mean(late)), num(sd), num(statistics.mean(off)), d, se,
            num(chance), num(perfect), "-" if norm is None else "%.2f" % norm,
            "yes" if ok else ""))
    print("\npasses %d of %d rows\n" % (passed, len(rows)))


def print_many(names, rows, cols, cache, seeds):
    flags = [column_flags(n, [], cols) for n in names]
    print("| row | " + " | ".join(names) + " |")
    print("|---" * (len(names) + 1) + "|")
    passes = [0] * len(names)
    for rname, rargs in rows:
        chance, perfect = calib_of(cache, rargs)
        base = scores(cache, rargs, flags[0], seeds, False)
        cells = []
        for i, f in enumerate(flags):
            late = scores(cache, rargs, f, seeds, False)
            off = scores(cache, rargs, f, seeds, True)
            if None in late or None in off:
                cells.append("missing")
                continue
            norm, ok = judge(late, off, chance, perfect)
            passes[i] += ok
            text = num(statistics.mean(late)) + ("*" if ok else "")
            if i > 0 and None not in base:
                d, se = paired(late, base)
                mark = "+" if d > 2 * se and d > 0 else ("-" if d < -2 * se and d < 0 else "")
                fmt = "%+.0f" if abs(d) >= 10 else "%+.3f"
                text += (" (" + fmt + "%s)") % (d, mark)
            cells.append(text)
        print("| %s | %s |" % (rname, " | ".join(cells)))
    print("| rows passed | " + " | ".join(str(p) for p in passes) + " |")
    print("\n`*` passes (halfway from chance to perfect and above its twin by 2 standard errors);"
          " in brackets the paired difference to %s, `+`/`-` at 2 standard errors.\n" % names[0])


def main():
    argv = sys.argv[1:]
    split = argv.index("--") if "--" in argv else len(argv)
    ap = argparse.ArgumentParser()
    ap.add_argument("what", choices=["run", "table", "calibrate"])
    ap.add_argument("name", nargs="?", default="")
    ap.add_argument("--seeds", default="5-12")
    ap.add_argument("--rows", default="")
    ap.add_argument("--jobs", type=int, default=2)
    ap.add_argument("--blobrun", default=os.path.join(ROOT, "build", "blobrun"))
    ap.add_argument("--no-twin", action="store_true")
    opt = ap.parse_args(argv[:split])
    flags = argv[split + 1:]
    rows, cols = read_map(MORE)
    rows = pick_rows(rows, opt.rows)
    seeds = parse_seeds(opt.seeds)
    os.makedirs(os.path.join(OUT, "runs"), exist_ok=True)
    cache_path = os.path.join(OUT, "cache.jsonl")
    cache = load_cache()

    if opt.what == "calibrate" or opt.what == "run":
        todo = []
        for rname, rargs in rows:
            for s in (5, 6):
                args = life_cmd(rargs, ["--engine", "memory"], s)
                if "calib " + " ".join(args) not in cache:
                    todo.append((args, True))
        if opt.what == "run":
            col = column_flags(opt.name, flags, cols)
            for rname, rargs in rows:
                for s in seeds:
                    for twin in ([False] if opt.no_twin else [False, True]):
                        args = life_cmd(rargs, twin_args(col) if twin else col, s)
                        if " ".join(args) not in cache and (args, False) not in todo:
                            todo.append((args, False))
        # the slow worlds first, so the last lives to finish are short ones
        todo.sort(key=lambda t: ("mnist" not in " ".join(t[0]), "--delay 50" not in " ".join(t[0])))
        run_all(todo, opt.jobs, opt.blobrun, cache_path)
        cache = load_cache()
        if opt.what == "run":
            print_one(opt.name, rows, column_flags(opt.name, flags, cols), cache, seeds)
        return 0
    names = opt.name.split(",")
    if len(names) == 1:
        print_one(names[0], rows, column_flags(names[0], flags, cols), cache, seeds)
    else:
        print_many(names, rows, cols, cache, seeds)
    return 0


if __name__ == "__main__":
    sys.exit(main())
