"""Label-free rule families in the model (idea 173): one life of one rule, then the read.

docs/results-2026-11-26-label-free.md. The model is idea 165's bottom rung (7b: the engine's
cell, reach, fixed signs, 1 and 4 at born counts, the engine's regulation), started from the
real born sheet's wiring. A life is one pass of digits, one batch of `batch` digits at a time:
the cells answer, regulation moves the thresholds, the rule moves the counters
(ladder4_rules.py), the wiring is re-picked. No labels anywhere in the life. Then, as idea
165 reads: freeze, let regulation settle on 3,000 digits, fit a fresh linear reader of the
3,072 cells on the training digits and score the test digits (and, on MNIST, the test digits
moved 1 and 2 pixels).

Controls, by `ctrl`: "life" (the rule on real digits), "shuffled" (the same rule on digits
whose pixels are moved by a permutation drawn afresh for every digit: nothing to learn),
"churn" (the rule's steps, as many up and down, on candidates drawn at random). Family
"born" is the sheet as born, read the same way.
"""

import copy
import json
import time

import torch

import ladder2_data as D
import ladder2_model as M
import ladder2_train as T
import ladder4_data as X
import ladder4_rules as R

FAMILIES = ["born", "f0", "f0w", "f1", "f1g", "f2", "f3", "f4", "f5"]


def learner_cells(sheet_path, sheet):
    """hidden excitatory cells: not motor, inhibitory or pacemaker (the engine's learners)"""
    kind = {}
    for line in open(sheet_path).read().splitlines()[1:]:
        r = line.split()
        kind[int(r[0])] = int(r[3])
    ok = [u for u, n in enumerate(sheet.cells) if kind[n] & (2 | 4 | 8) == 0]
    return torch.tensor(ok, dtype=torch.long)


def answer(model, x, W=None):
    """the cells' counts with the wiring as it is now (rung 7b's forward, no gradient); W:
    the signed wiring, if the caller keeps it"""
    with torch.no_grad():
        W = model.src_sign * model.levels if W is None else W
        drive_px = x @ W[:, :model.P].t()
        c = model.count(drive_px)
        for _ in range(model.steps - 1):
            c = model.count(drive_px + (c / M.TICKS) @ W[:, model.P:].t())
        return c


def active(model, x, c):
    """what a spike reads active: pixels on, and a cell source with chance count / 20"""
    return torch.cat([x, (c / M.TICKS).clamp(max=1.0)], dim=1)


def tile_cap(c, tile_of, tiles, cap):
    """the tile's winners in counts: at most cap x 20 spikes a digit in a tile, the cells
    with the most spikes keep theirs (the engine keeps the most driven cells each tick)"""
    if not cap:
        return c
    out = c.clone()
    for t in range(tiles):
        rows = torch.nonzero(tile_of == t).flatten()
        ct = c[:, rows]
        order = ct.argsort(dim=1, descending=True)
        sorted_c = torch.gather(ct, 1, order)
        room = (cap * M.TICKS - (sorted_c.cumsum(1) - sorted_c)).clamp(min=0)
        kept = torch.minimum(sorted_c, room)
        out[:, rows] = torch.zeros_like(ct).scatter_(1, order, kept)
    return out


class Life:
    def __init__(self, task, sheet, sheet_path, seed, opt):
        self.opt, self.task = opt, task
        self.model = M.Ladder("7b", sheet, task.classes, seed)
        self.model.born_wiring(sheet)
        self.model.rewire()
        x0, _ = task.train_set()
        T.init_thresholds(self.model, x0[:3000])
        self.gen = torch.Generator().manual_seed(seed)
        self.W = self.model.src_sign * self.model.levels
        self.regulate(x0[:10000])  # the born sheet's thresholds before the life
        self.learners = learner_cells(sheet_path, sheet)
        self.store = R.Store(self.model, self.learners, seed)
        self.tile_all = torch.from_numpy(sheet.cells // 64)
        self.tile_of = self.tile_all[self.learners]
        self.tiles = int(self.tile_all.max()) + 1
        self.avg = torch.ones(len(self.learners))  # family 5's running mean of count squared
        self.level = torch.zeros(self.tiles)                   # family 1g's tile levels
        self.target = None

    def regulate(self, x, rate=0.05, batch=256):
        for i in range(0, len(x), batch):
            self.model.regulate(answer(self.model, x[i:i + batch], self.W), rate=rate)

    def look(self, x):
        c = tile_cap(answer(self.model, x, self.W), self.tile_all, self.tiles, self.opt["cap"])
        return c, active(self.model, x, c)

    def step(self, fam, xb):
        o, L, S = self.opt, self.learners, self.store
        c, A = self.look(xb)
        shown = [c]
        if fam in ("f0", "f0w"):
            d = R.compete(c[:, L], A, S, o["shift"])
        elif fam in ("f1", "f1g"):
            xf = X.made_up(xb, self.gen)
            cf, Af = self.look(xf)
            shown.append(cf)
            gate = None
            if fam == "f1g":
                g_r = torch.zeros(len(xb), self.tiles).index_add_(1, self.tile_all, c)
                g_f = torch.zeros(len(xb), self.tiles).index_add_(1, self.tile_all, cf)
                lv = self.level.unsqueeze(0)
                gate = ((g_r <= lv).float()[:, self.tile_of], (g_f >= lv).float()[:, self.tile_of])
                self.level += 0.02 * (torch.cat([g_r, g_f]).mean(0) - self.level)
            d = R.real_or_made(c[:, L], A, cf[:, L], Af, S, o["shift"], gate)
        elif fam == "f2":
            y = xb[:, self.target.clamp(min=0)]
            d = R.predict_pixel(c[:, L], A, y, (self.target >= 0).float(), o["rate"], S)
        elif fam == "f3":
            xb2 = X.second_look(xb, self.gen)
            c2, A2 = self.look(xb2)
            shown.append(c2)
            d = R.two_looks(c[:, L], A, c2[:, L], A2, S, o["shift"])
        elif fam == "f4":
            d = R.compete(c[:, L], A, S, o["shift"]) + R.push_apart(
                c[:, L], A, S, self.tile_of, self.tiles, o["shift"], o["push"])
        elif fam == "f5":
            d = R.bcm(c[:, L], A, S, self.avg, o["shift"])
            self.avg += 0.01 * ((c[:, L] ** 2).mean(0) - self.avg)
        else:
            raise ValueError(fam)
        return d, torch.cat(shown)

    def live(self, fam, ctrl, stream, checkpoints):
        """the life: returns the state at each checkpoint (digits seen)"""
        snaps, B = {}, self.opt["batch"]
        cuts = sorted(set(range(0, len(stream), B)) | set(checkpoints) | {len(stream)})
        for i, j in zip(cuts[:-1], cuts[1:]):
            xb = stream[i:j]
            if ctrl == "shuffled":
                xb, _ = X.shuffled(xb, self.gen)
            d, shown = self.step(fam, xb)
            self.model.regulate(shown, rate=self.opt["reg"])
            self.store.apply(d, churn=ctrl == "churn")
            self.store.rewire(self.model)
            self.W = self.model.src_sign * self.model.levels
            if j in checkpoints:
                snaps[j] = self.snapshot()
        return snaps

    def snapshot(self):
        m = self.model
        return {"levels": m.levels.clone(), "theta": m.theta.detach().clone(),
                "bias": m.bias.clone(), "steps": self.store.steps}

    def restore(self, s):
        with torch.no_grad():
            self.model.levels.copy_(s["levels"])
            self.model.theta.copy_(s["theta"])
            self.model.bias.copy_(s["bias"])
        self.W = self.model.src_sign * self.model.levels


def read(life, sheet, born_levels, seed):
    """settle 3,000 digits with learning off, then the fresh linear reader (and on MNIST the
    same reader, refit identically, on the test digits moved 1 and 2 pixels)"""
    task, model = life.task, life.model
    x, y = task.train_set()
    life.regulate(x[:3000])
    feat = lambda z: torch.cat([answer(model, z[i:i + 1000], life.W)
                                for i in range(0, len(z), 1000)])
    f_tr, f_te = feat(x), feat(task.x_te)
    res = {"reader": D.reader(f_tr, y, f_te, task.y_te, task.classes, seed)}
    if not task.pair and life.opt["moved"]:
        for by, sets in X.moved_sets(task.x_te).items():
            f_m = torch.cat([feat(s) for s in sets])
            # the reader is deterministic for a seed: the same fit, scored on the moved digits
            res["moved%d" % by] = D.reader(f_tr, y, f_m, task.y_te.repeat(len(sets)),
                                           task.classes, seed)
    res["rate"] = float(f_te.mean()) * 1000 / M.TICKS
    res["active"] = float((f_te > 0).float().mean())
    res["silent_cells"] = float((f_te.sum(0) == 0).float().mean())
    L = life.learners
    lev = model.levels[L]
    use = life.store.use
    born = born_levels[L] > 0
    res["born_kept"] = float(((lev > 0) & born & use).sum() / max(1, int((born & use).sum())))
    res["pixels"] = float((lev[:, :784] > 0).sum(1).float().mean())
    res["likeness"], res["spread"] = X.pixel_likeness(lev, x)
    return res


def run_one(task_name, fam, ctrl, seed, sheet_path, out_path, opt, wiring_out=None):
    torch.set_num_threads(1)
    torch.manual_seed(seed)
    logf = open(out_path + ".log", "w")
    t0 = time.time()
    task = D.Task(task_name, False, seed, opt["limit"])
    sheet = M.SheetInfo(sheet_path)
    life = Life(task, sheet, sheet_path, seed, opt)
    born_levels = life.model.levels.clone()
    res = {"task": task_name, "family": fam, "arm": opt.get("arm", fam), "ctrl": ctrl,
           "seed": seed}
    res.update({k: opt[k] for k in ("shift", "rate", "push", "batch", "cap", "reg")})
    marks = sorted(set(min(n, opt["life"]) for n in opt["checkpoints"]))
    reads = {}
    if fam == "born":
        reads[0] = read(life, sheet, born_levels, seed)
    else:
        if fam == "f2":
            use = (life.model.cand & ~life.model.src_inh).numpy()
            tgt = X.target_pixels(sheet, use, task.train_set()[0], seed)[life.learners]
            # a cell may not wire its own target pixel
            j = torch.nonzero(tgt >= 0).flatten()
            life.store.forbid(j, tgt[j])
            life.target = tgt
            res["targets"] = int(len(j))
        stream = X.life_stream(task, opt["life"], seed)
        snaps = life.live(fam, ctrl, stream, set(marks))
        logf.write("life done %.0f s\n" % (time.time() - t0))
        logf.flush()
        for n in marks:
            life.restore(snaps[n])
            reads[n] = read(life, sheet, born_levels, seed)
            reads[n]["steps_per_cell"] = snaps[n]["steps"] / len(life.learners)
            logf.write("read %d: %s\n" % (n, json.dumps(reads[n])))
            logf.flush()
        life.restore(snaps[marks[-1]])
        if wiring_out:
            T.write_wiring(life.model, sheet, wiring_out)
    res["reads"] = {str(k): v for k, v in reads.items()}
    res["seconds"] = round(time.time() - t0)
    logf.write(json.dumps(res) + "\n")
    with open(out_path, "w") as f:
        f.write(json.dumps(res) + "\n")
    return res
