"""Candidates, their commands, and their scores, read from a store with a quick map.

Rungs of the race (each includes the ones before it):
    0  the fast worlds, first selection seed (marked `fast` in the map; if none are marked
       for an engine, its 3 cheapest life worlds by measured time, fixed when a run starts)
    1  every world, first selection seed
    2  every world, every selection seed
    3  every world, every selection seed and every confirmation seed
Fitness on a set of worlds and seeds: per world the mean score over the seeds; the
headline is how many worlds reach `pass`, the tie-break the mean over worlds.
"""

import quickmap
import space as sp


class Scorer:
    def __init__(self, store, qm, spaces):
        self.store = store
        self.qm = qm
        self.spaces = spaces  # engine -> Space
        self.args = {}        # id -> (life, twin, probe) flag lists
        self.scores = {}      # (id, world, seed) -> score, once known
        self.fast = {}        # engine -> fast worlds, set by set_costs
        self.cheap = {}       # engine -> worlds, cheapest first

    def set_costs(self, engines, refresh_fast=True):
        """order worlds cheapest first from the seconds results have taken; pick the fast
        worlds. A world never measured counts as dear, in file order."""
        spent = {}
        for row in self.store.results.values():
            if "world" in row and row.get("seconds") is not None:
                k = (row.get("engine"), row["world"])
                t = spent.setdefault(k, [0.0, 0])
                t[0] += row["seconds"]
                t[1] += 1
        self.cost = {}
        for engine in engines:
            worlds = self.qm.worlds_for(engine)
            for w in worlds:
                t = spent.get((engine, w.name))
                per_run = t[0] / t[1] if t else None
                runs = 1 if w.kind == "probe" else 2
                self.cost[(engine, w.name)] = per_run * runs if per_run is not None else None
            order = sorted(range(len(worlds)), key=lambda i: (
                self.cost[(engine, worlds[i].name)] is None,
                self.cost[(engine, worlds[i].name)] or 0, i))
            self.cheap[engine] = [worlds[i] for i in order]
            marked = [w for w in self.cheap[engine] if w.fast]
            lives = [w for w in self.cheap[engine] if w.kind == "life"]
            if refresh_fast or engine not in self.fast:
                self.fast[engine] = marked or lives[:3] or self.cheap[engine][:3]

    def engine(self, cid):
        return self.store.candidates[cid]["engine"]

    def values(self, cid):
        return self.store.candidates[cid]["values"]

    def space(self, cid):
        return self.spaces[self.engine(cid)]

    def flag_lists(self, cid):
        if cid not in self.args:
            s, v = self.space(cid), self.values(cid)
            self.args[cid] = (sp.life_args(s, v), sp.twin_args(s, v), sp.probe_args(s, v))
        return self.args[cid]

    def commands(self, cid, world, seed):
        """[(kind, cmd)] for one world and seed"""
        life, twin, probe = self.flag_lists(cid)
        if world.kind == "probe":
            return [("probe", quickmap.probe_command(self.qm, world, probe, seed))]
        return [("life", quickmap.life_command(self.qm, world, life, seed)),
                ("twin", quickmap.life_command(self.qm, world, twin, seed))]

    def worlds(self, cid, rung):
        engine = self.engine(cid)
        if engine not in self.cheap:
            self.set_costs([engine] + list(self.cheap))
        return list(self.fast[engine]) if rung == 0 else list(self.cheap[engine])

    def seeds(self, rung):
        if rung <= 1:
            return self.qm.select[:1]
        if rung == 2:
            return list(self.qm.select)
        return list(self.qm.select) + list(self.qm.confirm)

    def needed(self, cid, rung):
        """(kind, cmd, world name) of this rung (and those below) with no result yet,
        cheapest world first"""
        out = []
        for s in self.seeds(rung):
            for w in self.worlds(cid, rung):
                for kind, cmd in self.commands(cid, w, s):
                    if quickmap.key_of(cmd) not in self.store.results:
                        out.append((kind, cmd, w.name))
        return out

    def score(self, cid, world, seed):
        """a world's score on one seed, None if not run yet; a failed run scores 0"""
        k = (cid, world.name, seed)
        if k in self.scores:
            return self.scores[k]
        rows = []
        for kind, cmd in self.commands(cid, world, seed):
            row = self.store.results.get(quickmap.key_of(cmd))
            if row is None:
                return None
            rows.append(row)
        if any(r.get("value") is None for r in rows):
            value = 0.0
        elif world.kind == "probe":
            value = quickmap.probe_score(world, rows[0]["value"])
        else:
            value = quickmap.life_score(world, rows[0]["value"], rows[1]["value"])
        self.scores[k] = value
        return value

    def gain(self, cid, world, seed):
        """the score before it is held between 0 and 1 (for correlations), None if missing"""
        got = self.raw(cid, world, seed)
        if got is None:
            return None
        if world.kind == "probe":
            return (got[0] - world.zero) / (world.full - world.zero)
        return quickmap.life_gain(world, got[0], got[1])

    def raw(self, cid, world, seed):
        """(late or reading, twin late or None), for the report"""
        rows = [self.store.results.get(quickmap.key_of(c)) for _, c in
                self.commands(cid, world, seed)]
        if any(r is None or r.get("value") is None for r in rows):
            return None
        return rows[0]["value"], (rows[1]["value"] if len(rows) > 1 else None)

    def world_means(self, cid, worlds, seeds):
        """world name -> mean score over the seeds that have run; missing worlds left out"""
        out = {}
        for w in worlds:
            got = [self.score(cid, w, s) for s in seeds]
            got = [g for g in got if g is not None]
            if got:
                out[w.name] = sum(got) / len(got)
        return out

    def fitness_of_means(self, means, worlds):
        """(passes, mean) over the given worlds; a missing world counts 0"""
        if not worlds:
            return 0, 0.0
        passes = sum(1 for w in worlds if means.get(w.name, 0.0) >= self.qm.passing)
        mean = sum(means.get(w.name, 0.0) for w in worlds) / len(worlds)
        return passes, mean

    def rung_fitness(self, cid, rung):
        worlds = self.worlds(cid, rung)
        means = self.world_means(cid, worlds, self.seeds(min(rung, 2)))
        return self.fitness_of_means(means, worlds)

    def confirm_means(self, cid):
        return self.world_means(cid, self.qm.worlds_for(self.engine(cid)), self.qm.confirm)

    def reached(self, cid):
        """highest rung entered: one past each go"""
        r = 0
        while r < 3:
            d = self.store.decision(cid, r)
            if d is None or not d["go"]:
                break
            r += 1
        return r


def scalar(fit):
    passes, mean = fit
    return passes + 0.999 * mean
