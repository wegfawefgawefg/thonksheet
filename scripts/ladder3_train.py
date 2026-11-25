"""The rule's ladder: one run of one arm on one test (docs/results-2026-11-25-rule-ladder.md).

An arm is a rung of the teacher (ARMS below). T0 to T1b are idea 165's own teacher
(ladder2_train.train, autograd, Adam, batches of 256) on rung 7b; the rest are written out by
hand (ladder3_rule.py). Every arm is read as idea 165 reads: freeze the cells, show all the
training digits once, fit a fresh linear reader of the 3,072 cells' counts, score it on the
test (or validation) digits.
"""

import json
import time

import numpy as np
import torch

import ladder2_data as D
import ladder2_model as M
import ladder2_train as L2T
import ladder3_rule as R

# idea 165's teacher (autograd): passes, digits a pass, start
AUTO = {"T0": (12, 0, "random"), "T0b": (12, 0, "born"), "T1a": (1, 0, "born"),
        "T1b": (1, 20000, "born")}

ONLINE = dict(mode="online", err="bp", hops=2, quant="none", gate="slope", step="grad",
              verdict="full", learners="all")
# the ladder, each rung adding to the one named first
CHAIN = [("T2", None, {}), ("T2h", "T2", {"hops": 1}), ("T3", "T2h", {"err": "fa"}),
         ("T4", "T3", {"err": "sparse"}), ("T5", "T4", {"quant": "sign"}),
         ("T6", "T5", {"gate": "nearfired"}), ("T6f", "T5", {"gate": "fired"}),
         ("T6n", "T5", {"gate": "near"}), ("T7", "T6", {"step": "count"}),
         ("T8", "T7", {"verdict": "answer"}), ("T8v", "T7", {"verdict": "pressed"}),
         ("T8r", "T7", {"verdict": "right"}), ("T10", "T8", {"learners": "engine"}),
         # one step alone from the top: idea 165's teacher (batches of 256, Adam, 12 passes,
         # random start), written out by hand, with one thing changed
         ("A0", None, {"mode": "batch", "start": "random"}),
         ("A2h", "A0", {"hops": 1}), ("A3", "A0", {"err": "fa", "hops": 1}),
         ("A5", "A0", {"quant": "sign"}), ("A6", "A0", {"gate": "fired"}),
         ("A8", "A0", {"verdict": "answer"})]
ARMS = {}
for name, parent, change in CHAIN:
    ARMS[name] = dict(ARMS[parent] if parent else ONLINE, **change)
COIN_FROM = "T3"  # every rung from here down has a coin arm: NAMEcoin


def arm_cfg(arm):
    coin = arm.endswith("coin")
    cfg = dict(ARMS[arm[:-4] if coin else arm])
    cfg["coin"] = coin
    return cfg


class Part:
    """the first `n` digits of each pass (the pair test pairs them afresh each pass)"""

    def __init__(self, task, n):
        self.task, self.n = task, n

    def train_set(self):
        x, y = self.task.train_set()
        return (x[:self.n], y[:self.n]) if self.n else (x, y)


def features(fn, x):
    with torch.no_grad():
        return torch.cat([fn(x[i:i + 1000]) for i in range(0, len(x), 1000)])


def born_changed(levels, sheet):
    """born inputs a cell no longer wires (of those the model can see)"""
    born = sheet.born[:, :levels.shape[1]] > 0
    kept = (levels > 0) & born
    return float((born.sum(1) - kept.sum(1)).mean()), float(born.sum(1).mean())


def run_one(task_name, arm, seed, val, sheet_path, out_path, wiring_out=None, limit=0,
            lr=None, shift=None, head_lr=0.3, digits=0):
    torch.set_num_threads(1)
    torch.manual_seed(seed)
    gen = torch.Generator().manual_seed(seed)
    logf = open(out_path + ".log", "w")

    def log(s):
        logf.write(s + "\n")
        logf.flush()

    t0 = time.time()
    task = D.Task(task_name, val, seed, limit)
    sheet = M.SheetInfo(sheet_path)
    model = M.Ladder("7b", sheet, task.classes, seed)
    res = {"task": task_name, "arm": arm, "seed": seed, "val": int(val)}
    x_all, _ = task.train_set()
    n_set = len(x_all)
    if arm in AUTO:
        passes, n, start = AUTO[arm]
        if start == "born":
            with torch.no_grad():
                b = torch.from_numpy(sheet.born[:, :model.N].astype(np.float32))
                model.u.copy_(0.02 * b + 0.002 * torch.randn(b.shape, generator=gen))
        L2T.init_thresholds(model, x_all[:3000])
        L2T.train(model, Part(task, min(n, n_set) if n else 0), passes, 2e-3, "trained", gen,
                  log)
        model.eval()
        fn = model
        res.update(passes=passes, digits=passes * (min(n, n_set) if n else n_set))
    else:
        cfg = arm_cfg(arm)
        t = R.Teacher(sheet, sheet_path, model, cfg, seed, x_all[:3000])
        opt = {"lr": lr if lr is not None else 0.1, "shift": shift if shift is not None else 9,
               "head_lr": head_lr}
        if cfg["mode"] == "batch":
            opt["lr"] = 2e-3
            cfg["lr"] = 2e-3
        life = digits or (12 * n_set if cfg["mode"] == "batch" else n_set)
        with torch.no_grad():
            res["own_life"] = R.teach(t, task, life, opt, log)
        model.levels = t.levels
        fn = lambda x: t.cells(x)["c1"]
        res.update(cfg=cfg, digits=life, lr=opt["lr"] if not t.counts else None,
                   shift=opt["shift"] if t.counts else None, head_lr=head_lr,
                   counter_steps=t.steps_taken)
    x, y = task.train_set()
    f_tr, f_te = features(fn, x), features(fn, task.x_te)
    res["own"] = L2T.head_score(model, f_te, task.y_te)
    res["reader"] = D.reader(f_tr, y, f_te, task.y_te, task.classes, seed)
    res["active"] = float((f_te > 0).float().mean())
    res["mean_count"] = float(f_te.mean())
    res["silent_cells"] = float((f_te.sum(0) == 0).float().mean())
    res["class_share"] = L2T.class_share(f_te, task.y_te, task.classes)
    levels = model.levels
    res.update(L2T.wiring_stats(model, sheet))
    res["born_changed"], res["born_inputs"] = born_changed(levels.numpy(), sheet)
    if wiring_out:
        L2T.write_wiring(model, sheet, wiring_out)
    res["seconds"] = round(time.time() - t0)
    log(json.dumps(res))
    with open(out_path, "w") as f:
        f.write(json.dumps(res) + "\n")
    return res
