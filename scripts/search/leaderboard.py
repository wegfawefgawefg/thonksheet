"""The leaderboard (docs/search-leaderboard.md for the main search).

Candidates are chosen and ordered by their selection-seed fitness (rung 2: every world,
every selection seed); the numbers shown beside them are from the confirmation seeds, which
played no part in choosing them. Only confirmed candidates (rung 3 finished) are listed.
"""

import json
import os
import time

import audit
import scoring
import space as sp


def confirmed(scorer, ids):
    out = []
    for cid in ids:
        d = scorer.store.decision(cid, 3)
        if d is not None:
            out.append(cid)
    return out


def life_worlds(qm, engine):
    return [w for w in qm.worlds_for(engine) if w.kind == "life"]


def fit_text(fit, count):
    return "%d of %d, mean %.2f" % (fit[0], count, fit[1])


def name_of(store, cid):
    row = store.candidates[cid]
    return row.get("name") or row["born"]


def best_table(scorer, ids, worlds_of, title_rows=10):
    """rows for the best `title_rows` by selection fitness on worlds_of(engine)"""
    ranked = []
    for cid in ids:
        worlds = worlds_of(scorer.engine(cid))
        sel = scorer.fitness_of_means(scorer.world_means(cid, worlds, scorer.qm.select), worlds)
        ranked.append((scoring.scalar(sel), cid, sel))
    ranked.sort(reverse=True)
    return ranked[:title_rows]


def report(store, qm, spaces, engines, name, folder):
    scorer = scoring.Scorer(store, qm, spaces)
    scorer.set_costs(engines)
    ids_by_engine = {e: [c for c in store.order if store.candidates[c]["engine"] == e]
                     for e in engines}
    all_ids = [c for e in engines for c in ids_by_engine[e]]
    done = confirmed(scorer, all_ids)
    prev_path = os.path.join(folder, "last-report.json")
    previous = json.load(open(prev_path)) if os.path.exists(prev_path) else {}

    lines = ["# Search leaderboard: %s" % name, "",
             "Written by `scripts/search.sh report` on %s. How to read it: docs/search.md." %
             time.strftime("%Y-%m-%d %H:%M"), "",
             "Every candidate is one setting used unchanged on every world. Chosen and "
             "ordered on selection seeds %s; every score shown is from confirmation seeds "
             "%s, which played no part in choosing. A world's score is (late - the larger of "
             "the learning-off twin and chance) / (perfect - chance), held to 0..1 (probes: "
             "the reading scaled the same way); passed at %.2f or more." % (
                 " ".join(map(str, qm.select)), " ".join(map(str, qm.confirm)), qm.passing),
             ""]
    lines += counts_section(scorer, engines, ids_by_engine, done)
    lines += changes_section(scorer, engines, done, previous)

    # overall, on the worlds every engine has (life worlds)
    lines += ["## Best overall", "",
              "On the life worlds (every engine runs them). Passed and mean are on the "
              "confirmation seeds.", ""]
    lines += candidate_rows(scorer, best_table(scorer, done, lambda e: life_worlds(qm, e)),
                            lambda e: life_worlds(qm, e))
    for engine in engines:
        ids = [c for c in done if scorer.engine(c) == engine]
        lines += ["## Best %s" % engine, "",
                  "On every world this engine runs (probes included for the full engine). "
                  "The engine's defaults for comparison.", ""]
        top = best_table(scorer, ids, lambda e: qm.worlds_for(e), 5)
        default = [c for c in ids if store.candidates[c]["born"] == "default"]
        if default and default[0] not in [t[1] for t in top]:
            top.append((0, default[0], None))
        lines += candidate_rows(scorer, top, lambda e: qm.worlds_for(e))
    lines += tier_section(scorer, engines, done)
    lines += world_best_section(scorer, engines, done)
    cost_lines, rates = audit.cost_section(store, qm, engines, ids_by_engine, previous)
    lines += cost_lines
    lines += audit.world_section(scorer, engines, ids_by_engine)
    lines += audit.number_section(scorer, engines, ids_by_engine)
    lines += settings_section(scorer, done)

    save_summary(prev_path, scorer, engines, ids_by_engine, done, rates)
    return "\n".join(lines) + "\n"


def counts_section(scorer, engines, ids_by_engine, done):
    lines = ["## Tried so far", "",
             "| engine | candidates | reached rung 1 | rung 2 | confirmed | core hours |",
             "| --- | --- | --- | --- | --- | --- |"]
    for e in engines:
        ids = ids_by_engine[e]
        reach = [scorer.reached(c) for c in ids]
        secs = sum(r.get("seconds", 0.0) for r in scorer.store.results.values()
                   if r.get("engine") == e)
        lines.append("| %s | %d | %d | %d | %d | %.1f |" % (
            e, len(ids), sum(1 for r in reach if r >= 1), sum(1 for r in reach if r >= 2),
            sum(1 for c in done if scorer.engine(c) == e), secs / 3600))
    lines.append("")
    return lines


def candidate_rows(scorer, ranked, worlds_of):
    qm = scorer.qm
    lines = []
    if not ranked:
        return ["Nothing confirmed yet.", ""]
    worlds = []
    for _, cid, _ in ranked:
        for w in worlds_of(scorer.engine(cid)):
            if w not in worlds:
                worlds.append(w)
    lines.append("| candidate | engine | made as | selection | confirmation | " +
                 " | ".join(w.name for w in worlds) + " |")
    lines.append("| --- " * (5 + len(worlds)) + "|")
    for _, cid, sel in ranked:
        own = worlds_of(scorer.engine(cid))
        if sel is None:
            sel = scorer.fitness_of_means(scorer.world_means(cid, own, qm.select), own)
        conf_means = scorer.world_means(cid, own, qm.confirm)
        conf = scorer.fitness_of_means(conf_means, own)
        cells = []
        for w in worlds:
            v = conf_means.get(w.name)
            cells.append("-" if v is None else ("**%.2f**" % v if v >= qm.passing else "%.2f" % v))
        lines.append("| `%s` | %s | %s | %s | %s | %s |" % (
            cid, scorer.engine(cid), name_of(scorer.store, cid), fit_text(sel, len(own)),
            fit_text(conf, len(own)), " | ".join(cells)))
    lines.append("")
    return lines


def tier_section(scorer, engines, done):
    qm = scorer.qm
    lines = ["## Best per tier", "",
             "Per tier, the confirmed candidate best on that tier's worlds on selection "
             "seeds, with its confirmation score there.", "",
             "| tier | candidate | engine | confirmation in the tier |", "| --- | --- | --- | --- |"]
    for tier in sorted(set(w.tier for w in qm.worlds)):
        best = None
        for cid in done:
            tw = [w for w in qm.worlds_for(scorer.engine(cid)) if w.tier == tier]
            if not tw:
                continue
            sel = scorer.fitness_of_means(scorer.world_means(cid, tw, qm.select), tw)
            if best is None or sel > best[0]:
                best = (sel, cid, tw)
        if best:
            sel, cid, tw = best
            conf = scorer.fitness_of_means(scorer.world_means(cid, tw, qm.confirm), tw)
            lines.append("| %d | `%s` | %s | %s |" % (tier, cid, scorer.engine(cid),
                                                      fit_text(conf, len(tw))))
    lines.append("")
    return lines


def world_best_section(scorer, engines, done):
    qm = scorer.qm
    lines = ["## Best per world", "",
             "Per world and engine, the confirmed candidate with the best selection score "
             "there; its confirmation score, and its raw late score beside the twin's "
             "(confirmation seeds, mean).", "",
             "| world | engine | candidate | confirmation | late | twin |",
             "| --- | --- | --- | --- | --- | --- |"]
    for w in qm.worlds:
        for engine in engines:
            if not w.runs_on(engine):
                continue
            best = None
            for cid in done:
                if scorer.engine(cid) != engine:
                    continue
                sel = scorer.world_means(cid, [w], qm.select).get(w.name, 0.0)
                if best is None or sel > best[0]:
                    best = (sel, cid)
            if not best:
                continue
            cid = best[1]
            conf = scorer.world_means(cid, [w], qm.confirm).get(w.name)
            raws = [scorer.raw(cid, w, s) for s in qm.confirm]
            raws = [r for r in raws if r is not None]
            late = sum(r[0] for r in raws) / len(raws) if raws else None
            twins = [r[1] for r in raws if r[1] is not None]
            twin = sum(twins) / len(twins) if twins else None
            lines.append("| %s | %s | `%s` | %s | %s | %s |" % (
                w.name, engine, cid, "-" if conf is None else "%.2f" % conf,
                "-" if late is None else "%.3f" % late, "-" if twin is None else "%.3f" % twin))
    lines.append("")
    return lines


def settings_section(scorer, done):
    lines = ["## The settings", "",
             "Every candidate named above, as flags (only what differs from the engine's "
             "defaults; a rule table is in out/search/<name>/rules/<hash>.hex).", ""]
    for cid in done:
        lines.append("- `%s` %s %s: `%s`" % (
            cid, scorer.engine(cid), name_of(scorer.store, cid),
            sp.describe(scorer.space(cid), scorer.values(cid))))
    lines.append("")
    return lines


def changes_section(scorer, engines, done, previous):
    lines = ["## Since the last report", ""]
    if not previous:
        return lines + ["First report.", ""]
    qm = scorer.qm
    lines.append("Last report: %s." % previous.get("when", "?"))
    for e in engines:
        before = previous.get("candidates", {}).get(e, 0)
        now = sum(1 for c in scorer.store.order if scorer.engine(c) == e)
        lines.append("- %s: %d new candidates (%d in all)." % (e, now - before, now))
    old_best = previous.get("best_per_world", {})
    for key, (cid, value) in best_per_world(scorer, engines, done).items():
        old = old_best.get(key)
        if old is None or old[0] != cid:
            lines.append("- new best on %s: `%s` (confirmation %.2f, was %s)" % (
                key, cid, value, "nothing" if old is None else "%.2f" % old[1]))
    lines.append("")
    return lines


def best_per_world(scorer, engines, done):
    qm = scorer.qm
    out = {}
    for w in qm.worlds:
        for e in engines:
            if not w.runs_on(e):
                continue
            best = None
            for cid in done:
                if scorer.engine(cid) == e:
                    sel = scorer.world_means(cid, [w], qm.select).get(w.name, 0.0)
                    if best is None or sel > best[0]:
                        best = (sel, cid)
            if best:
                conf = scorer.world_means(best[1], [w], qm.confirm).get(w.name, 0.0)
                out["%s %s" % (w.name, e)] = (best[1], conf)
    return out


def save_summary(path, scorer, engines, ids_by_engine, done, rates):
    summary = {"when": time.strftime("%Y-%m-%d %H:%M"),
               "candidates": {e: len(ids_by_engine[e]) for e in engines},
               "rates": rates,
               "best_per_world": best_per_world(scorer, engines, done)}
    with open(path, "w") as f:
        json.dump(summary, f, indent=1, sort_keys=True)


def short_status(store, qm, spaces, engines):
    scorer = scoring.Scorer(store, qm, spaces)
    scorer.set_costs(engines)
    lines = []
    for e in engines:
        ids = [c for c in store.order if store.candidates[c]["engine"] == e]
        done = confirmed(scorer, ids)
        reach = [scorer.reached(c) for c in ids]
        lines.append("%s: %d candidates, %d reached rung 2, %d confirmed" % (
            e, len(ids), sum(1 for r in reach if r >= 2), len(done)))
        worlds = qm.worlds_for(e)
        for _, cid, sel in best_table(scorer, done, lambda x: qm.worlds_for(x), 3):
            conf = scorer.fitness_of_means(scorer.world_means(cid, worlds, qm.confirm), worlds)
            lines.append("    %s (%s): selection %s; confirmation %s" % (
                cid, name_of(store, cid), fit_text(sel, len(worlds)),
                fit_text(conf, len(worlds))))
    lines.append("%d runs recorded" % len(store.results))
    return "\n".join(lines)
