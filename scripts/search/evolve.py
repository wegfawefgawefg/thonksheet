"""The search: which command to run next, and what to do when a candidate finishes a rung.

Racing: a new candidate runs rung 0 (the fast worlds, one seed). When it has finished a
rung, it is compared with every earlier candidate of its engine that finished the same
rung, on selection seeds only:
    rung 0 -> 1: in the best 40%
    rung 1 -> 2: in the best 25%, or the best so far on some world or some tier
    rung 2 -> 3: among the 3 best, or the best so far on some world or some tier
A candidate scoring nothing never goes on, nor one whose every raw score at that rung is
the same as an earlier candidate's (it behaves the same; mutations often change nothing). Rung 3 adds the confirmation seeds; those
scores are only reported, never used to choose. Decisions are written down, so a restart
picks up exactly where it was.

New candidates, per engine (the engine that has used the least machine time goes next):
20% fresh (the defaults with about half the numbers drawn anew), 25% crossover of two
parents (and maybe one change), the rest a mutant of one parent (1 to 4 numbers changed).
Parents are picked by tournament from the pool: the 12 best of the engine (furthest rung
first, then fitness there) and the best on each world.
"""

import quickmap
import scoring
import space as sp

GO_SHARE = {0: 0.40, 1: 0.25}
CONFIRM_TOP = 3
FEW = {0: 10, 1: 10, 2: 5}  # fewer peers than this: go on
POOL_TOP = 12
FRESH = 0.20
CROSS = 0.25


class Search:
    def __init__(self, store, qm, spaces, engines, rng, log):
        self.store = store
        self.qm = qm
        self.spaces = spaces
        self.engines = engines
        self.rng = rng
        self.log = log
        self.scorer = scoring.Scorer(store, qm, spaces)
        self.scorer.set_costs(engines)
        self.active = {}    # id -> rung being run, in the order they started
        self.inflight = set()
        self.means = {}     # (id, rung) -> world means at that rung, once complete
        self.peers = {}     # (engine, rung) -> ids with a decision at that rung
        self.pool = {}
        self.signatures = {}  # (engine, rung) -> {raw scores: id}, filled as needed
        self.decided_since_pool = 0
        self.turn = 0
        self.seconds = {e: 0.0 for e in engines}
        for row in store.results.values():
            if row.get("engine") in self.seconds:
                self.seconds[row["engine"]] += row.get("seconds", 0.0)
        for (cid, rung) in store.decisions:
            if cid in store.candidates and self.engine_ok(cid):
                self.peers.setdefault((self.scorer.engine(cid), rung), []).append(cid)
        for cid in store.order:
            if not self.engine_ok(cid):
                continue
            r = self.scorer.reached(cid)
            if store.decision(cid, r) is None:
                self.active[cid] = r
        self.rebuild_pool()

    def engine_ok(self, cid):
        return self.store.candidates[cid]["engine"] in self.engines

    # candidates

    def add(self, engine, values, born, parents=(), name=None):
        space = self.spaces[engine]
        values = sp.fix_orders(space, dict(values))
        cid = sp.candidate_id(space, values)
        row = {"id": cid, "engine": engine, "values": values, "born": born,
               "parents": list(parents)}
        if name:
            row["name"] = name
        if self.store.add_candidate(row):
            self.active[cid] = 0
            return cid
        return None

    def add_starts(self, starts):
        """the defaults of each engine and the named starts; they always go on"""
        for engine in self.engines:
            self.add(engine, self.spaces[engine].defaults(), "default", name="default")
        for engine, name, values in starts:
            if engine in self.engines:
                self.add(engine, values, "start", name=name)

    def pick_engine(self, engines):
        return min(engines, key=lambda e: (self.seconds[e], self.rng.random()))

    def tournament(self, pool):
        picks = [self.rng.randrange(len(pool)) for _ in range(3)]
        return pool[min(picks)]

    def new_candidate(self, accept=None):
        engines = [e for e in self.engines if accept is None or accept(e, "life")]
        if not engines:
            return None
        engine = self.pick_engine(engines)
        space = self.spaces[engine]
        pool = self.pool.get(engine, [])
        for _ in range(30):
            roll = self.rng.random()
            if not pool or roll < FRESH:
                values, born, parents = sp.fresh(space, self.rng), "fresh", []
            elif roll < FRESH + CROSS and len(pool) >= 2:
                a, b = self.tournament(pool), self.tournament(pool)
                values = sp.crossover(space, self.scorer.values(a), self.scorer.values(b),
                                      self.rng)
                if a == b or self.rng.random() < 0.5:
                    values = sp.mutate(space, values, self.rng, changes=1)
                born, parents = "cross", [a, b]
            else:
                a = self.tournament(pool)
                values = sp.mutate(space, self.scorer.values(a), self.rng)
                born, parents = "mutant", [a]
            cid = self.add(engine, values, born, parents)
            if cid:
                return cid
        return None

    # what to run

    def next_commands(self, count, max_active, accept=None):
        """up to `count` (id, kind, cmd, world) not yet run nor in flight, oldest candidate
        first, making new candidates while there is room; accept(engine, kind) filters
        (a batch evaluator takes only what it can run)"""
        self.settle()
        out = []
        taken = set()

        def take(cid, rung):
            engine = self.scorer.engine(cid)
            for kind, cmd, world in self.scorer.needed(cid, rung):
                if accept and not accept(engine, kind):
                    continue
                k = quickmap.key_of(cmd)
                if len(out) < count and k not in self.inflight and k not in taken:
                    taken.add(k)
                    out.append((cid, kind, cmd, world))

        # turn about: the oldest candidate first (so confirmations finish), then the lowest
        # rung first (so new candidates race), so neither waits for the other
        self.turn += 1
        if self.turn % 2:
            for cid, rung in list(self.active.items()):
                if rung <= 1:
                    take(cid, rung)
            while len(out) < count and len(self.active) < max_active:
                cid = self.new_candidate(accept)
                if cid is None:
                    break
                take(cid, 0)
        for cid, rung in list(self.active.items()):
            take(cid, rung)
        while len(out) < count and len(self.active) < max_active:
            cid = self.new_candidate(accept)
            if cid is None:
                break
            take(cid, 0)
        return out

    def started(self, cmd):
        self.inflight.add(quickmap.key_of(cmd))

    def finished(self, cmd, row):
        self.inflight.discard(quickmap.key_of(cmd))
        if row is not None:
            self.store.add_result(row)
            if row.get("engine") in self.seconds:
                self.seconds[row["engine"]] += row.get("seconds", 0.0)

    def settle(self):
        """decide every active candidate whose rung has finished"""
        moved = True
        while moved:
            moved = False
            for cid, rung in list(self.active.items()):
                if self.scorer.needed(cid, rung):
                    continue
                moved = True
                d = self.store.decision(cid, rung)
                if d is None:
                    go, why = self.decide(cid, rung)
                    self.store.add_decision(cid, rung, go, why)
                    self.peers.setdefault((self.scorer.engine(cid), rung), []).append(cid)
                    self.decided_since_pool += 1
                    self.log_decision(cid, rung, go, why)
                else:
                    go = d["go"]  # a top-up finished: its scores changed
                    for r in range(4):
                        self.means.pop((cid, r), None)
                if go and rung < 3:
                    self.active[cid] = rung + 1
                else:
                    del self.active[cid]
        if self.decided_since_pool >= 10:
            self.rebuild_pool()

    # deciding

    def rung_means(self, cid, rung):
        k = (cid, rung)
        if k in self.means:
            return self.means[k]
        worlds = self.scorer.worlds(cid, rung)
        means = self.scorer.world_means(cid, worlds, self.scorer.seeds(min(rung, 2)))
        if self.store.decision(cid, rung) is not None:
            self.means[k] = means  # finished rungs only; a top-up clears it
        return means

    def leads(self, cid, rung, peers):
        """name of a world or tier this candidate is the best at, or None"""
        worlds = self.scorer.worlds(cid, rung)
        mine = self.rung_means(cid, rung)
        theirs = [self.rung_means(p, rung) for p in peers]
        for w in worlds:
            v = mine.get(w.name, 0.0)
            if v > 0 and all(v > t.get(w.name, 0.0) for t in theirs):
                return "best on " + w.name
        for tier in sorted(set(w.tier for w in worlds)):
            tw = [w for w in worlds if w.tier == tier]
            fit = self.scorer.fitness_of_means(mine, tw)
            if fit[1] > 0 and all(fit > self.scorer.fitness_of_means(t, tw) for t in theirs):
                return "best in tier %d" % tier
        return None

    def signature(self, cid, rung):
        raw = []
        for s in self.scorer.seeds(min(rung, 2)):
            for w in self.scorer.worlds(cid, rung):
                raw.append((w.name, s, self.scorer.raw(cid, w, s)))
        return tuple(raw)

    def same_as(self, cid, rung, peers):
        """an earlier candidate with exactly the same raw scores at this rung, or None"""
        k = (self.scorer.engine(cid), rung)
        if k not in self.signatures:
            self.signatures[k] = {}
            for p in peers:
                self.signatures[k].setdefault(self.signature(p, rung), p)
        sig = self.signature(cid, rung)
        twin = self.signatures[k].get(sig)
        if twin is None:
            self.signatures[k][sig] = cid
        return twin

    def decide(self, cid, rung):
        born = self.store.candidates[cid]["born"]
        if born in ("default", "start"):
            return True, born
        if rung == 3:
            return True, "confirmed"
        engine = self.scorer.engine(cid)
        worlds = self.scorer.worlds(cid, rung)
        mine = scoring.scalar(self.scorer.fitness_of_means(self.rung_means(cid, rung), worlds))
        peers = [p for p in self.peers.get((engine, rung), []) if p != cid]
        if mine <= 0:
            return False, "scored nothing"
        same = self.same_as(cid, rung, peers)
        if same:
            return False, "same scores as " + same
        if len(peers) < FEW[rung]:
            return True, "few to compare with (%d)" % len(peers)
        better = sum(1 for p in peers if scoring.scalar(
            self.scorer.fitness_of_means(self.rung_means(p, rung), worlds)) > mine)
        place = "%d of %d better" % (better, len(peers))
        if rung in GO_SHARE and better < GO_SHARE[rung] * len(peers):
            return True, place
        if rung == 2 and better < CONFIRM_TOP:
            return True, place
        if rung >= 1:
            lead = self.leads(cid, rung, peers)
            if lead:
                return True, lead + "; " + place
        return False, place

    def log_decision(self, cid, rung, go, why):
        if rung >= 1 or go:
            fit = self.scorer.rung_fitness(cid, rung)
            self.log("%s %s rung %d %s (%s): %d passed, mean %.3f" % (
                self.scorer.engine(cid), cid, rung, "go" if go else "stop", why, fit[0],
                fit[1]))

    # parents, and keeping the best up to date with the map

    def rebuild_pool(self):
        self.decided_since_pool = 0
        for engine in self.engines:
            ranked = []
            for cid in self.store.order:
                if self.scorer.engine(cid) != engine:
                    continue
                r = min(self.scorer.reached(cid), 2)
                if r < 1:
                    continue
                fit = scoring.scalar(self.scorer.fitness_of_means(
                    self.rung_means(cid, r), self.scorer.worlds(cid, r)))
                ranked.append((r, fit, cid))
            ranked.sort(reverse=True)
            pool = [cid for _, _, cid in ranked[:POOL_TOP]]
            # the best on each world, on the first selection seed
            for w in self.qm.worlds_for(engine):
                best, best_v = None, 0.0
                for _, _, cid in ranked:
                    v = self.scorer.score(cid, w, self.qm.select[0])
                    if v is not None and v > best_v:
                        best, best_v = cid, v
                if best and best not in pool:
                    pool.append(best)
            self.pool[engine] = pool

    def top_up(self):
        """bring the starts and the pool up to date with the map (a world added since);
        re-measure which worlds are cheapest"""
        self.scorer.set_costs(self.engines, refresh_fast=False)
        keep = []
        for cid in self.store.order:
            if self.engine_ok(cid) and self.store.candidates[cid]["born"] in ("default", "start"):
                keep.append(cid)
        for engine in self.engines:
            keep += self.pool.get(engine, [])
        count = 0
        for cid in keep:
            if cid in self.active:
                continue
            r = self.scorer.reached(cid)
            if self.scorer.needed(cid, r):
                self.active[cid] = r
                count += 1
        return count
