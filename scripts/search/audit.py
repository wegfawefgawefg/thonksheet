"""The parts of the report that keep the search fast and lean (docs/bench.md, "Keep the
loop fast"): what a candidate costs against the budget, candidates per hour per core,
worlds that tell us nothing, numbers that have never mattered.

Correlations are rank correlations (Spearman, ties averaged) over every candidate with a
score on the first selection seed; scores are taken before they are held between 0 and 1,
so a candidate that does worse than its learning-off twin still counts as worse.
"""

import statistics

MATTERS = 0.15       # |rank correlation| shown as an effect
NEVER = 0.10         # below this on every world and on fitness: no effect seen
NEVER_CHOICE = 0.02  # a choice whose best value is this close to the mean: no effect seen
ENOUGH = 200         # candidates before a number is flagged for removal
TRACKS = 0.90        # two worlds this correlated tell us the same thing
ALL_PASS = 0.95      # share of candidates passing a world that tells us nothing


def ranks(values):
    order = sorted(range(len(values)), key=lambda i: values[i])
    out = [0.0] * len(values)
    i = 0
    while i < len(order):
        j = i
        while j + 1 < len(order) and values[order[j + 1]] == values[order[i]]:
            j += 1
        for k in range(i, j + 1):
            out[order[k]] = (i + j) / 2.0
        i = j + 1
    return out


def spearman(a, b):
    if len(a) < 3:
        return 0.0
    ra, rb = ranks(a), ranks(b)
    ma, mb = statistics.mean(ra), statistics.mean(rb)
    num = sum((x - ma) * (y - mb) for x, y in zip(ra, rb))
    den = (sum((x - ma) ** 2 for x in ra) * sum((y - mb) ** 2 for y in rb)) ** 0.5
    return num / den if den else 0.0


def first_seed_table(scorer, engine, ids):
    """world name -> {id: gain on the first selection seed}"""
    seed = scorer.qm.select[0]
    out = {}
    for w in scorer.qm.worlds_for(engine):
        col = {}
        for cid in ids:
            g = scorer.gain(cid, w, seed)
            if g is not None:
                col[cid] = g
        out[w.name] = col
    return out


# what it costs

def cost_section(store, qm, engines, ids_by_engine, previous):
    lines = ["## What a candidate costs", ""]
    per = {}  # (engine, host, world) -> [seconds, runs]
    for row in store.results.values():
        if "world" not in row or row.get("engine") not in engines:
            continue
        k = (row["engine"], row.get("host", "?"), row["world"])
        t = per.setdefault(k, [0.0, 0])
        t[0] += row.get("seconds", 0.0)
        t[1] += 1
    hosts = sorted(set(k[1] for k in per))
    lines.append("Seconds of one core for one world on one seed (the life and its twin, or "
                 "one probe), mean over every run so far. Budget for one pass of the map: "
                 "**%s s**." % (("%g" % qm.budget) if qm.budget else "not set"))
    lines.append("")
    for engine in engines:
        worlds = qm.worlds_for(engine)
        if not worlds:
            continue
        lines.append("| %s | " % engine + " | ".join(hosts) + " |")
        lines.append("| --- |" + " --- |" * len(hosts))
        totals = {h: 0.0 for h in hosts}
        for w in worlds:
            cells = []
            for h in hosts:
                t = per.get((engine, h, w.name))
                if t:
                    cost = t[0] / t[1] * (1 if w.kind == "probe" else 2)
                    totals[h] += cost
                    cells.append("%.1f" % cost)
                else:
                    cells.append("-")
            lines.append("| %s | " % w.name + " | ".join(cells) + " |")
        cells = []
        for h in hosts:
            over = qm.budget and totals[h] > qm.budget
            cells.append("**%.0f%s**" % (totals[h], " OVER BUDGET" if over else ""))
        lines.append("| one pass | " + " | ".join(cells) + " |")
        lines.append("")
    lines.append("Candidates per hour per core (every candidate of the engine over every "
                 "second its runs took, any host, all rungs):")
    lines.append("")
    lines.append("| engine | candidates | core hours | per core hour | last report |")
    lines.append("| --- | --- | --- | --- | --- |")
    rates = {}
    for engine in engines:
        secs = sum(r.get("seconds", 0.0) for r in store.results.values()
                   if r.get("engine") == engine)
        n = len(ids_by_engine.get(engine, []))
        rate = n / (secs / 3600) if secs > 0 else 0.0
        rates[engine] = rate
        before = previous.get("rates", {}).get(engine)
        note = "-" if before is None else "%.0f%s" % (
            before, "  (slower)" if rate < 0.8 * before else "")
        lines.append("| %s | %d | %.1f | %.0f | %s |" % (engine, n, secs / 3600, rate, note))
    lines.append("")
    return lines, rates


# worlds that tell us nothing

def world_section(scorer, engines, ids_by_engine):
    qm = scorer.qm
    lines = ["## Worlds: what each one tells us", "",
             "Share of candidates passing each world on the first selection seed, and the "
             "world its scores track most closely (rank correlation over candidates).", ""]
    flags = []
    for engine in engines:
        ids = ids_by_engine.get(engine, [])
        table = first_seed_table(scorer, engine, ids)
        worlds = qm.worlds_for(engine)
        if not ids or not worlds:
            continue
        lines.append("| %s | candidates | passing | best ever | tracks most closely |" % engine)
        lines.append("| --- | --- | --- | --- | --- |")
        for w in worlds:
            col = table[w.name]
            seed = qm.select[0]
            scores = [scorer.score(c, w, seed) for c in col]
            passing = sum(1 for s in scores if s is not None and s >= qm.passing)
            share = passing / len(col) if col else 0.0
            best = max(scores) if scores else 0.0
            near, near_r = "-", 0.0
            for other in worlds:
                if other.name == w.name:
                    continue
                both = [c for c in col if c in table[other.name]]
                r = spearman([col[c] for c in both], [table[other.name][c] for c in both])
                if abs(r) > abs(near_r):
                    near, near_r = other.name, r
            lines.append("| %s | %d | %.0f%% | %.2f | %s %+.2f |" % (
                w.name, len(col), 100 * share, best, near, near_r))
            if len(col) >= 50:
                if share >= ALL_PASS:
                    flags.append("%s (%s): %.0f%% of candidates pass it" % (
                        w.name, engine, 100 * share))
                if passing == 0:
                    flags.append("%s (%s): no candidate has passed it (best %.2f)" % (
                        w.name, engine, best))
                if abs(near_r) >= TRACKS:
                    flags.append("%s (%s): tracks %s (%+.2f)" % (w.name, engine, near, near_r))
        lines.append("")
    lines.append("Candidates to cut from the quick map (judged after 50 candidates):")
    lines.append("")
    lines += ["- " + f for f in flags] if flags else ["- none flagged yet"]
    lines.append("")
    return lines


# numbers that matter, and numbers that never have

def number_effects(scorer, space, ids, table):
    """flag -> (effects {world: r}, n varied, summary). choice numbers: group means."""
    out = {}
    for n in space.numbers:
        if not n.varied() or n.kind == "rule":
            continue
        values = {c: scorer.values(c).get(n.flag, n.default) for c in ids}
        varied = sum(1 for v in values.values() if v != n.default)
        effects = {}
        for wname, col in table.items():
            have = [c for c in col if c in values]
            if len(have) < 20:
                continue
            if n.kind == "choice":
                groups = {}
                for c in have:
                    groups.setdefault(values[c], []).append(col[c])
                mean = statistics.mean(col[c] for c in have)
                big = [statistics.mean(g) - mean for g in groups.values() if len(g) >= 10]
                effects[wname] = max(big, key=abs) if big else 0.0
            else:
                effects[wname] = spearman([values[c] for c in have], [col[c] for c in have])
        out[n.flag] = (effects, varied, n.kind == "choice")
    return out


def number_section(scorer, engines, ids_by_engine):
    lines = ["## Which numbers matter", "",
             "Rank correlation of each number with each world's score on the first "
             "selection seed (|r| %.2f or more shown). For a choice (a mode), the largest "
             "difference between one value's mean score and the mean of all. Mutants of "
             "good parents are over-represented, so these are hints, not measurements." %
             MATTERS, ""]
    never_all = []
    for engine in engines:
        ids = ids_by_engine.get(engine, [])
        if len(ids) < 20:
            lines.append("%s: %d candidates, too few yet." % (engine, len(ids)))
            lines.append("")
            continue
        space = scorer.spaces[engine]
        table = first_seed_table(scorer, engine, ids)
        effects = number_effects(scorer, space, ids, table)
        worlds = [w.name for w in scorer.qm.worlds_for(engine)]
        lines.append("| %s (%d candidates) | " % (engine, len(ids)) + " | ".join(worlds) + " |")
        lines.append("| --- |" + " --- |" * len(worlds))
        never = []
        for flag, (eff, varied, choice) in effects.items():
            limit = 0.05 if choice else MATTERS
            cells = []
            shown = False
            for w in worlds:
                v = eff.get(w)
                if v is not None and abs(v) >= limit:
                    cells.append("%+.2f" % v)
                    shown = True
                else:
                    cells.append(".")
            if shown:
                lines.append("| %s | " % flag + " | ".join(cells) + " |")
            quiet = all(abs(v) < (NEVER_CHOICE if choice else NEVER) for v in eff.values())
            if eff and quiet:
                never.append((flag, varied))
        lines.append("")
        if never:
            lines.append("%s, no effect seen on any world (times it was off its default):" %
                         engine)
            lines.append("")
            lines.append(", ".join("`%s` (%d)" % fv for fv in never))
            lines.append("")
            never_all += [(engine, f, v) for f, v in never if len(ids) >= ENOUGH]
    lines.append("Numbers to cut from the space files (no effect after %d or more "
                 "candidates of their engine):" % ENOUGH)
    lines.append("")
    lines += (["- %s `%s` (off its default %d times)" % x for x in never_all]
              if never_all else ["- none flagged yet"])
    lines.append("")
    return lines
