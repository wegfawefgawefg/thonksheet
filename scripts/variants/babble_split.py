#!/usr/bin/env python3
"""Ideas 60 and 61 helper: read sweep.py's cache and print the tables of the babble and
balance results doc. Runs nothing; run scripts/babble.sh first.

    python3 scripts/variants/babble_split.py --cache out/babble/sweep/cache.jsonl \
        --seeds 11-24 --variants scripts/variants/babble.txt -- --task shuffled-echo --k 2 ...

Default: shuffled echo split into straight and crossed lives (crossed: the task's draw put
cue 1 on button 0, as in lose_shift_split.py). --no-split: one table over all seeds.
--signals: how often good and bad came per step, as the task gave them and as the blob got
them. --stable: late-life stability, on all windows and on windows without babbled steps.
"Clean" is the score with babbled steps left out (right / tries tasks only). t is the
paired difference to the first variant over its standard error.
"""

import json
import statistics
import sys

M = (1 << 64) - 1
LEARNING_ONLY_PREFIX = "--g."


def splitmix(state):
    state = (state + 0x9E3779B97F4A7C15) & M
    z = state
    z = ((z ^ (z >> 30)) * 0xBF58476D1CE4E5B9) & M
    z = ((z ^ (z >> 27)) * 0x94D049BB133111EB) & M
    return state, z ^ (z >> 31)


def crossed(seed):
    state, _ = splitmix(seed)
    state, word = splitmix(state)
    return ((word >> 32) * 2) >> 32 == 0


def parse_seeds(text):
    out = []
    for part in text.split(","):
        if "-" in part:
            lo, hi = part.split("-")
            out += list(range(int(lo), int(hi) + 1))
        else:
            out.append(int(part))
    return out


def read_variants(path):
    out = []
    for line in open(path):
        line = line.strip()
        if line and not line.startswith("#"):
            name, _, args = line.partition(":")
            out.append((name.strip(), args.split()))
    return out


def twin_args(args, extra):
    # as sweep.py: plug flags stay, genome numbers here are all learning-only or none
    out = []
    for i in range(0, len(args) - 1, 2):
        if not args[i].startswith(LEARNING_ONLY_PREFIX):
            out += [args[i], args[i + 1]]
    return out + ["--g.learning_on", "0"] + extra


def mean_se(v, digits=3):
    if not v:
        return "-"
    se = statistics.stdev(v) / len(v) ** 0.5 if len(v) > 1 else 0.0
    if abs(statistics.mean(v)) >= 10:
        return f"{statistics.mean(v):.1f} ±{se:.1f}"
    return f"{statistics.mean(v):.{digits}f} ±{se:.{digits}f}"


def vs(values, base):
    d = [a - b for a, b in zip(values, base)]
    if len(d) < 2:
        return "", ""
    se = statistics.stdev(d) / len(d) ** 0.5
    m = statistics.mean(d)
    t = m / se if se > 0 else 0.0
    return (f"{m:+.1f}" if abs(m) >= 10 else f"{m:+.3f}"), f"{t:+.1f}"


def main():
    argv = sys.argv[1:]
    split = argv.index("--")
    opts, task_args = argv[:split], argv[split + 1:]
    cache_path = opts[opts.index("--cache") + 1]
    seeds = parse_seeds(opts[opts.index("--seeds") + 1])
    variants = read_variants(opts[opts.index("--variants") + 1])
    no_split = "--no-split" in opts
    extra = ["--t.learning_on", "0"] if "--table" in task_args else []
    cache = {}
    for line in open(cache_path):
        row = json.loads(line)
        cache[row["key"]] = row

    def row(args, s):
        return cache.get(" ".join(task_args + ["--seed", str(s), "--task-seed", str(s)] + args))

    def get(r, field):
        if r is None:
            return None
        if field == "late_clean" and field not in r:
            return r["late"]  # nothing babbled: every step is clean
        return r.get(field)

    def column(args, field, ss, twin=False):
        a = twin_args(args, extra) if twin else args
        v = [get(row(a, s), field) for s in ss]
        return None if None in v else v

    if "--signals" in opts:
        print("| variant | late | learning off | good raw | bad raw | good sent | bad sent | "
              "babbled | late vs base | t |")
        print("|---|---|---|---|---|---|---|---|---|---|")
        base = None
        for name, args in variants:
            late = column(args, "late", seeds)
            if late is None:
                print(f"| {name} | missing runs |")
                continue
            base = base or late
            off = column(args, "late", seeds, twin=True)
            cells = []
            for k in ["good_raw_per_step", "bad_raw_per_step", "good_sent_per_step",
                      "bad_sent_per_step", "babbled_per_step"]:
                v = column(args, k, seeds)
                cells.append(f"{statistics.mean(v):.3f}" if v else "-")
            d, t = vs(late, base) if late is not base else ("", "")
            print(f"| {name} | {mean_se(late)} | {mean_se(off) if off else '-'} | "
                  f"{' | '.join(cells)} | {d} | {t} |")
        per_seed(variants, seeds, column)
        return

    if "--stable" in opts:
        print("| variant | windows | last tenth | lowest window, last tenth | lowest after first "
              "0.9 | windows under 0.8 after first 0.9 |")
        print("|---|---|---|---|---|---|")
        for name, args in variants:
            rows = [row(args, s) for s in seeds]
            if None in rows:
                print(f"| {name} | missing runs |")
                continue
            for field in ["windows", "clean"]:
                if field == "clean" and "clean" not in rows[0]:
                    continue
                tenth, low, low_after, dips, reached = [], [], [], [], 0
                for r in rows:
                    w = r[field]
                    edge = max(1, len(w) // 10)
                    tenth.append(statistics.mean(w[-edge:]))
                    low.append(min(w[-edge:]))
                    first = next((i for i, x in enumerate(w) if x >= 0.9), None)
                    if first is not None:
                        reached += 1
                        low_after.append(min(w[first:]))
                        dips.append(sum(1 for x in w[first:] if x < 0.8))
                label = "all" if field == "windows" else "unbabbled"
                print(f"| {name} | {label} | {mean_se(tenth)} | {mean_se(low)} (min "
                      f"{min(low):.2f}) | {mean_se(low_after)} ({reached}/{len(rows)}) | "
                      f"{statistics.mean(dips) if dips else 0:.1f} |")
        return

    if no_split:
        print("| variant | late | late, unbabbled | learning off | off, unbabbled | late vs base "
              "| t | unbabbled vs base | t | babbled | urged button won |")
        print("|---|---|---|---|---|---|---|---|---|---|---|")
        base = base_clean = None
        for name, args in variants:
            late = column(args, "late", seeds)
            clean = column(args, "late_clean", seeds)
            if late is None:
                print(f"| {name} | missing runs |")
                continue
            first = base is None
            base = base or late
            base_clean = base_clean or clean
            off = column(args, "late", seeds, twin=True)
            off_clean = column(args, "late_clean", seeds, twin=True)
            d, t = vs(late, base) if not first else ("", "")
            dc, tc = vs(clean, base_clean) if not first else ("", "")
            bab = column(args, "babbled_per_step", seeds)
            obe = column(args, "babble_obeyed", seeds)
            print(f"| {name} | {mean_se(late)} | {mean_se(clean)} | "
                  f"{mean_se(off) if off else '-'} | {mean_se(off_clean) if off_clean else '-'} "
                  f"| {d} | {t} | {dc} | {tc} | {statistics.mean(bab) if bab else 0:.3f} | "
                  f"{statistics.mean(obe) if obe else 0:.2f} |")
        per_seed(variants, seeds, column)
        return

    cross = [s for s in seeds if crossed(s)]
    straight = [s for s in seeds if not crossed(s)]
    print(f"straight seeds {straight}, crossed seeds {cross}\n")
    print("| variant | straight | straight, unbabbled | crossed | crossed, unbabbled | worst "
          "crossed | crossed vs base | t | unbabbled vs base | t | off: straight | off: crossed |")
    print("|---|---|---|---|---|---|---|---|---|---|---|---|")
    base = base_clean = None
    for name, args in variants:
        late = column(args, "late", seeds)
        clean = column(args, "late_clean", seeds)
        if late is None:
            print(f"| {name} | missing runs |")
            continue
        lat = dict(zip(seeds, late))
        cl = dict(zip(seeds, clean))
        first = base is None
        base = base or lat
        base_clean = base_clean or cl
        c = [lat[s] for s in cross]
        cc = [cl[s] for s in cross]
        d, t = vs(c, [base[s] for s in cross]) if not first else ("", "")
        dc, tc = vs(cc, [base_clean[s] for s in cross]) if not first else ("", "")
        off = column(args, "late", seeds, twin=True)
        if off:
            off = dict(zip(seeds, off))
            o_s = mean_se([off[s] for s in straight])
            o_c = mean_se([off[s] for s in cross])
        else:
            o_s = o_c = "-"
        print(f"| {name} | {mean_se([lat[s] for s in straight])} | "
              f"{mean_se([cl[s] for s in straight])} | {mean_se(c)} | {mean_se(cc)} | "
              f"{min(c) if c else 0:.2f} | {d} | {t} | {dc} | {tc} | {o_s} | {o_c} |")
    per_seed(variants, seeds, column)


def per_seed(variants, seeds, column):
    print("\nlate per seed, seeds " + " ".join(str(s) for s in seeds) + ":\n\n```")
    for name, args in variants:
        late = column(args, "late", seeds)
        if late:
            cells = " ".join(f"{v:.2f}" if v < 10 else f"{v:.1f}" for v in late)
            print(f"{name:<22} {cells}")
    print("```\n")


if __name__ == "__main__":
    main()
