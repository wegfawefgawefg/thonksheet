"""The credit rules not yet tried: the arms, and one run of one arm on one test
(docs/results-2026-11-26-credit-rules.md). A run is read as the ladder reads: the cells
frozen, the training digits shown once, a fresh linear reader of the 3,072 counts scored on
the test (or validation) digits; and its wiring is written out for the real sheet.
"""

import json
import time

import torch

import credit_rule as C
import ladder2_data as D
import ladder2_model as M
import ladder2_train as L2T
import ladder3_rule as R
import ladder3_train as L3T

# every arm is the ladder's T7x (sparse +-1 wires, signed line errors with a "fine" zone,
# near or fired, every-event counters, every cell learns) with its teaching bit swapped
ARMS = {
    "still": dict(family="still"),                 # nothing learns: born as the bench writes it
    "hop": dict(family="hop", ladder_step=True),   # T7x again, through this file's loop
    "hopb": dict(family="hop", back="all"),        # T7x, then one pass back along real wires
    "P2": dict(family="phase", events="spike", gain=0.25, cap=(1 << 23) - 1),
    "P2s": dict(family="phase", events="sign", gain=0.25),
    "C2": dict(family="code", runs=2, n=0),
    "C2n192": dict(family="code", runs=2, n=192),
    "C2n12": dict(family="code", runs=2, n=12),
    "C1": dict(family="code", runs=1, n=0),
    "pay1": dict(family="pay"),
    "pay": dict(family="pay", back="all"),
    "payc": dict(family="pay", back="one"),
    "tgt": dict(family="target"),
    "dtp": dict(family="tuned"),
    "dtpb": dict(family="tuned", back="all"),
    # time: the error `delay` showings late, a mark that lasts `last` showings
    "d1n": dict(family="time", delay=1, last=1),
    "d1t2": dict(family="time", delay=1, last=2),
    "d1t4": dict(family="time", delay=1, last=4),
    "d3t4": dict(family="time", delay=3, last=4),
    "d0t4": dict(family="time", delay=0, last=4),
}
TUNABLE = {"phase": "gain"}  # what NAME@V sets


def arm_cfg(arm, value=None):
    coin = arm.endswith("coin")
    own = dict(ARMS[arm[:-4] if coin else arm])
    if value is not None and own["family"] in TUNABLE:
        own[TUNABLE[own["family"]]] = float(value)
    cfg = dict(L3T.ARMS["T7x"], **own)
    cfg["coin"] = coin
    return cfg


def run_one(task_name, arm, seed, val, sheet_path, out_path, wiring_out=None, limit=0,
            value=None, digits=0):
    torch.set_num_threads(1)
    torch.manual_seed(seed)
    logf = open(out_path + ".log", "w")

    def log(s):
        logf.write(s + "\n")
        logf.flush()

    t0 = time.time()
    task = D.Task(task_name, val, seed, limit)
    sheet = M.SheetInfo(sheet_path)
    model = M.Ladder("7b", sheet, task.classes, seed)
    cfg = arm_cfg(arm, value)
    x_all, _ = task.train_set()
    t = R.Teacher(sheet, sheet_path, model, cfg, seed, x_all[:3000])
    life = digits or len(x_all)
    with torch.no_grad():
        own_life, lf = C.live(t, task, life, cfg, log)
    model.levels = t.levels
    fn = lambda x: t.cells(x)["c1"]
    res = {"task": task_name, "arm": arm, "seed": seed, "val": int(val), "cfg": cfg,
           "digits": life, "own_life": own_life}
    res.update(lf.summary())
    x, y = task.train_set()
    f_tr, f_te = L3T.features(fn, x), L3T.features(fn, task.x_te)
    res["own"] = L2T.head_score(model, f_te, task.y_te)
    res["reader"] = D.reader(f_tr, y, f_te, task.y_te, task.classes, seed)
    res["active"] = float((f_te > 0).float().mean())
    res["mean_count"] = float(f_te.mean())
    res["silent_cells"] = float((f_te.sum(0) == 0).float().mean())
    res.update(L2T.wiring_stats(model, sheet))
    res["born_changed"], res["born_inputs"] = L3T.born_changed(t.levels.numpy(), sheet)
    res["counter_top"] = max(float(S.abs().max()) for S in t.S)
    if wiring_out:
        L2T.write_wiring(model, sheet, wiring_out)
    res["seconds"] = round(time.time() - t0)
    log(json.dumps(res))
    with open(out_path, "w") as f:
        f.write(json.dumps(res) + "\n")
    return res
