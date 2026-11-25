#!/usr/bin/env python3
"""Evolve rule tables (idea 120). Results: docs/results-2026-11-25-rule-tables.md

    scripts/rule_search.py run --name frozen-hand --mode frozen --init hand --gens 20
    scripts/rule_search.py status --name frozen-hand
    scripts/rule_search.py confirm --name frozen-hand --top 4
    scripts/rule_search.py show --name frozen-hand --top 3

Modes, all with the readout plug (128 motor cells per button):
  frozen  the table is the only learning, on button cells only (genome rule_on 3)
  whole   the table is the only learning, on every neuron (rule_on 1)
  inside  the hand-written error rule with evidence counters at the buttons, and the table
          on every other neuron (learn_mode 5 + rule_on 2); fitness is the gain over the
          same readout on a frozen sheet
Fitness: the mean over worlds and seeds of (late - twin) / (1 - twin), late the mean of
the last --late windows, twin the learning-off life. Worlds: echo with 4 cues, exclusive-or,
order, echo with cue and button 3 tiles apart. Selection seeds (default 1,2) and
confirmation seeds (default 5-12) are kept apart: `confirm` reruns the best tables, the
hand-written error rule and the frozen readout on seeds no table was chosen on.

Everything goes to out/rules/search/NAME/: state.json (the population), cache.jsonl (one
line per life, keyed by its command), tables/*.rule, log.txt.
"""

import argparse
import concurrent.futures
import hashlib
import json
import os
import random
import statistics
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import rule_table as rt  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BLOBRUN = os.path.join(ROOT, "build", "blobrun")
PLUG = ["--ticks", "20", "--sensor-side", "3", "--motor-side", "3", "--motor-spread", "128",
        "--alive-ticks", "500", "--rhythm-ticks", "64"]  # end-of-life measures kept short
WORLDS = {
    "echo4": ["--task", "echo", "--k", "4"],
    "xor": ["--task", "both-not-either"],
    "order": ["--task", "order"],
    "echo-far3": ["--task", "echo", "--distance", "3"],
}
READOUT = ["--g.learn_mode", "5", "--g.readout_evidence", "1", "--g.readout_shift", "4",
           "--g.readout_balance", "1"]
MODE_FLAGS = {
    "frozen": ["--g.rule_on", "3"],
    "whole": ["--g.rule_on", "1"],
    "inside": READOUT + ["--g.rule_on", "2"],
}
# which neurons a mode's table acts on: mutations stay there
MODE_BUTTON = {"frozen": 1, "whole": None, "inside": 0}


def parse_seeds(text):
    out = []
    for part in text.split(","):
        if "-" in part:
            lo, hi = part.split("-")
            out += range(int(lo), int(hi) + 1)
        else:
            out.append(int(part))
    return out


class Search:
    def __init__(self, name):
        self.dir = os.path.join(ROOT, "out", "rules", "search", name)
        os.makedirs(os.path.join(self.dir, "tables"), exist_ok=True)
        self.cache_path = os.path.join(self.dir, "cache.jsonl")
        self.cache = {}
        if os.path.exists(self.cache_path):
            for line in open(self.cache_path):
                row = json.loads(line)
                self.cache[row["key"]] = row

    def log(self, text):
        line = time.strftime("%H:%M:%S ") + text
        print(line, flush=True)
        with open(os.path.join(self.dir, "log.txt"), "a") as f:
            f.write(line + "\n")

    def table_path(self, rule):
        h = hashlib.sha1(rule.hex().encode()).hexdigest()[:12]
        path = os.path.join(self.dir, "tables", h + ".rule")
        if not os.path.exists(path):
            rt.save(rule, path, rt.describe(rule))
        return path, h

    def life(self, args):
        """one life; returns (late score per window list, seconds)"""
        key = " ".join(args)
        if key in self.cache:
            return self.cache[key]
        csv = os.path.join(self.dir, "lives", hashlib.sha1(key.encode()).hexdigest()[:16] + ".csv")
        os.makedirs(os.path.dirname(csv), exist_ok=True)
        start = time.time()
        cmd = ["nice", "-n", "19", BLOBRUN] + args + ["--no-baselines", "--out", csv]
        done = subprocess.run(cmd, cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                              text=True)
        if done.returncode != 0:
            raise RuntimeError(key + "\n" + done.stderr)
        lines = [l for l in open(csv).read().split("\n") if l]
        col = lines[0].split(",").index("blob")
        scores = [float(l.split(",")[col]) for l in lines[1:]]
        row = {"key": key, "windows": scores, "seconds": time.time() - start}
        return row

    def run_all(self, todo, jobs):
        todo = [a for a in dict.fromkeys(tuple(a) for a in todo) if " ".join(a) not in self.cache]
        if not todo:
            return
        with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
            for row in pool.map(lambda a: self.life(list(a)), todo):
                self.cache[row["key"]] = row
                with open(self.cache_path, "a") as f:
                    f.write(json.dumps(row) + "\n")


def life_args(opt, world, seed, extra):
    return (WORLDS[world] + PLUG + ["--steps", str(opt.steps), "--window", str(opt.window),
                                    "--seed", str(seed), "--task-seed", str(seed)] + extra)


def twin_args(opt, world, seed):
    return life_args(opt, world, seed, ["--g.learning_on", "0"])


def rule_args(opt, search, rule):
    path, _ = search.table_path(rule)
    return MODE_FLAGS[opt.mode] + ["--rule-file", os.path.relpath(path, ROOT)]


def late(row, n):
    w = row["windows"]
    return statistics.mean(w[-n:])


def scaled(search, opt, world, seed, extra):
    twin = late(search.cache[" ".join(twin_args(opt, world, seed))], opt.late)
    score = late(search.cache[" ".join(life_args(opt, world, seed, extra))], opt.late)
    return (score - twin) / (1 - twin) if twin < 1 else 0.0


def evaluate(search, opt, extras, seeds):
    """mean scaled score per extras entry (and per world), running what is missing"""
    todo = []
    for seed in seeds:
        for world in WORLDS:
            todo.append(twin_args(opt, world, seed))
            for extra in extras:
                todo.append(life_args(opt, world, seed, extra))
    search.run_all(todo, opt.jobs)
    out = []
    for extra in extras:
        per = {w: statistics.mean(scaled(search, opt, w, s, extra) for s in seeds)
               for w in WORLDS}
        out.append((statistics.mean(per.values()), per))
    return out


def baseline_extra(opt):
    """what the fitness is measured against: the frozen readout in inside mode, else none"""
    return READOUT if opt.mode == "inside" else None


# variation
def mutate(rule, rng, mode):
    r = rule.copy()
    button = MODE_BUTTON[mode]
    for _ in range(rng.choice([1, 1, 2, 3])):
        op = rng.random()
        live = rt.cubes(r)
        if op < 0.35 or not live:
            # a new random cube (or a cleared one)
            names = rng.sample([f for f in rt.FACTS if f != "n_button"], rng.randint(2, 6))
            facts = {n: rng.randint(0, 1) for n in names}
            if button is not None:
                facts["n_button"] = button
            action = 0 if rng.random() < 0.25 else rng.randint(1, 3)
            r.set(action, rng.randint(0, 3), **facts)
        elif op < 0.75:
            # change one existing cube: move it, widen it, narrow it, or change what it does
            care, value, d = rng.choice(live)
            facts = {rt.FACTS[i]: (value >> i) & 1 for i in range(len(rt.FACTS)) if care >> i & 1}
            kind = rng.random()
            if kind < 0.4:
                r.set(0, 0, **facts)  # moved: clear the old place
                f = rng.choice(rt.FACTS)
                if f != "n_button" or button is None:
                    facts[f] = 1 - facts.get(f, rng.randint(0, 1))
            elif kind < 0.6 and len(facts) > 1:
                facts.pop(rng.choice([f for f in facts if f != "n_button" or button is None]
                                     or list(facts)))
            elif kind < 0.8:
                f = rng.choice(rt.FACTS)
                facts[f] = rng.randint(0, 1)
                r.set(0, 0, **{k: v for k, v in facts.items() if k != f})
            else:
                d = (rng.randint(1, 3) << 2) | rng.randint(0, 3) if rng.random() < 0.5 else \
                    (d & ~3) | rng.randint(0, 3)
            if button is not None:
                facts["n_button"] = button
            r.set(d >> 2, d & 3, **facts)
        elif op < 0.9:
            k = rng.randrange(4)
            r.shifts[k] = max(0, min(15, r.shifts[k] + rng.choice([-2, -1, 1, 2])))
        elif op < 0.95:
            r.window = rng.choice([3, 6, 10, 15])
        else:
            if rng.random() < 0.5:
                r.readoff = rng.choice([0, 1, 2])
            else:
                r.balance ^= 1
    return r


def crossover(a, b, rng):
    r = a.copy()
    for g in range(rt.ENTRIES >> 5):
        if rng.random() < 0.5:
            r.entry[g * 32:(g + 1) * 32] = b.entry[g * 32:(g + 1) * 32]
    if rng.random() < 0.5:
        r.shifts, r.window, r.readoff, r.balance = list(b.shifts), b.window, b.readoff, b.balance
    return r


def initial(opt, rng):
    if opt.init == "hand":
        rules = [make() for make, _ in rt.HAND.values()]
    else:
        rules = []
    while len(rules) < opt.pop:
        if opt.init == "hand":
            rules.append(mutate(rng.choice(rules[:3]), rng, opt.mode))
        else:
            r = rt.random_sparse(rng)
            button = MODE_BUTTON[opt.mode]
            if button is not None:
                # keep only the part the mode's neurons can use
                for i in range(rt.ENTRIES):
                    if (i >> rt.BIT["n_button"]) & 1 != button:
                        r.entry[i] = 0
            rules.append(r)
    return rules


def cmd_run(opt):
    search = Search(opt.name)
    state_path = os.path.join(search.dir, "state.json")
    rng = random.Random(opt.rng)
    seeds = parse_seeds(opt.seeds)
    if os.path.exists(state_path):
        state = json.load(open(state_path))
        pop = [(rt.parse(p["hex"]), p["fitness"]) for p in state["pop"]]
        gen = state["gen"]
        rng.seed(opt.rng * 1000 + gen)
        search.log(f"resume at generation {gen}")
    else:
        pop, gen = [(r, None) for r in initial(opt, rng)], 0
        json.dump({"opt": vars(opt), "gen": 0, "pop": [], "history": []}, open(state_path, "w"))
    base = baseline_extra(opt)
    base_fit = evaluate(search, opt, [base], seeds)[0][0] if base else 0.0
    if base:
        search.log(f"baseline (frozen readout) on selection seeds: {base_fit:.3f}")
    while gen < opt.gens:
        need = [r for r, f in pop if f is None]
        results = evaluate(search, opt, [rule_args(opt, search, r) for r in need], seeds)
        fits = iter(results)
        pop = [(r, f if f is not None else next(fits)[0] - base_fit) for r, f in pop]
        pop.sort(key=lambda p: -p[1])
        state = json.load(open(state_path))
        state["gen"] = gen + 1
        state["pop"] = [{"hex": r.hex(), "fitness": f, "id": search.table_path(r)[1]}
                        for r, f in pop]
        state["history"].append([gen, pop[0][1], statistics.mean(f for _, f in pop)])
        json.dump(state, open(state_path, "w"))
        search.log(f"gen {gen}: best {pop[0][1]:+.3f} ({search.table_path(pop[0][0])[1]}), "
                   f"mean {statistics.mean(f for _, f in pop):+.3f}")
        gen += 1
        if gen >= opt.gens:
            break
        # next generation: the elite stay, children fill the rest
        elite = pop[:opt.elite]
        seen = {r.hex() for r, _ in elite}
        children = []
        while len(children) < opt.pop - opt.elite:
            def pick():
                return max(rng.sample(pop, 3), key=lambda p: p[1])[0]
            child = crossover(pick(), pick(), rng) if rng.random() < 0.4 else pick()
            child = mutate(child, rng, opt.mode)
            if child.hex() not in seen:
                seen.add(child.hex())
                children.append((child, None))
        pop = elite + children


def best_of(search, top):
    state = json.load(open(os.path.join(search.dir, "state.json")))
    opt = argparse.Namespace(**state["opt"])
    seen, out = set(), []
    for p in state["pop"]:
        if p["hex"] not in seen:
            seen.add(p["hex"])
            out.append((rt.parse(p["hex"]), p["fitness"], p["id"]))
    return opt, out[:top], state


def cmd_status(opt):
    search = Search(opt.name)
    sopt, best, state = best_of(search, opt.top)
    print(f"{opt.name}: mode {sopt.mode}, init {sopt.init}, generation {state['gen']} of "
          f"{sopt.gens}, {len(search.cache)} lives run")
    for gen, b, m in state["history"]:
        print(f"  gen {gen:3d}  best {b:+.3f}  mean {m:+.3f}")
    for r, f, i in best:
        print(f"  {i}  {f:+.3f}")


def cmd_show(opt):
    search = Search(opt.name)
    _, best, _ = best_of(search, opt.top)
    for r, f, i in best:
        print(f"## {i} (selection fitness {f:+.3f})\n{rt.describe(r)}\n")


def cmd_confirm(opt):
    search = Search(opt.name)
    sopt, best, _ = best_of(search, opt.top)
    sopt.jobs = opt.jobs
    seeds = parse_seeds(opt.seeds)
    if opt.steps:
        sopt.steps, sopt.window, sopt.late = opt.steps, opt.window, opt.late
    extras = [rule_args(sopt, search, r) for r, _, _ in best]
    names = [i for _, _, i in best]
    hand = rt.hand_error()
    hand_path, _ = search.table_path(hand)
    extras += [READOUT, ["--g.rule_on", "3", "--rule-file", os.path.relpath(hand_path, ROOT)]]
    names += ["hand error rule (learn_mode 5)", "error rule as a table, buttons only"]
    results = evaluate(search, sopt, extras, seeds)
    base = results[-2][0] if sopt.mode == "inside" else 0.0
    lines = [f"## {opt.name}: confirmation, seeds {opt.seeds}, {sopt.steps} steps\n",
             "| table | selection | confirmation | gain over frozen readout | "
             + " | ".join(WORLDS) + " |",
             "|---|---|---|---|" + "---|" * len(WORLDS)]
    sel = {i: f for _, f, i in best}
    for name, (fit, per) in zip(names, results):
        s = f"{sel[name]:+.3f}" if name in sel else ""
        lines.append(f"| {name} | {s} | {fit:.3f} | {fit - results[-2][0]:+.3f} | "
                     + " | ".join(f"{per[w]:.3f}" for w in WORLDS) + " |")
    text = "\n".join(lines) + "\n"
    print(text)
    tag = f"confirm-{opt.seeds}-{sopt.steps}.md"
    open(os.path.join(search.dir, tag), "w").write(text)
    _ = base


def main():
    ap = argparse.ArgumentParser()
    sub = ap.add_subparsers(dest="what", required=True)
    run = sub.add_parser("run")
    run.add_argument("--name", required=True)
    run.add_argument("--mode", choices=list(MODE_FLAGS), required=True)
    run.add_argument("--init", choices=["hand", "random"], required=True)
    run.add_argument("--gens", type=int, default=20)
    run.add_argument("--pop", type=int, default=16)
    run.add_argument("--elite", type=int, default=4)
    run.add_argument("--steps", type=int, default=6000)
    run.add_argument("--window", type=int, default=500)
    run.add_argument("--late", type=int, default=3, help="windows averaged for the late score")
    run.add_argument("--seeds", default="1,2")
    run.add_argument("--jobs", type=int, default=2)
    run.add_argument("--rng", type=int, default=1)
    for name in ("status", "show", "confirm"):
        p = sub.add_parser(name)
        p.add_argument("--name", required=True)
        p.add_argument("--top", type=int, default=4)
        p.add_argument("--jobs", type=int, default=2)
        p.add_argument("--seeds", default="5-12")
        p.add_argument("--steps", type=int, default=0, help="0: the search's own life length")
        p.add_argument("--window", type=int, default=1000)
        p.add_argument("--late", type=int, default=4)
    opt = ap.parse_args()
    {"run": cmd_run, "status": cmd_status, "show": cmd_show, "confirm": cmd_confirm}[opt.what](opt)


if __name__ == "__main__":
    main()
