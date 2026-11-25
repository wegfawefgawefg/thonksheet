"""The inside's ladder: one run of one rung on one test (docs/results-2026-11-25-inside-ladder.md).

kind "trained": the hidden cells and an output layer trained together by backprop on the
labels; "auto": the hidden cells trained with no labels, to give back the input bits through a
linear decoder (an autoencoder); "born": the rung's constraints with a wiring drawn at random
(rungs 6 and up: the real born sheet's wiring) and thresholds set by regulation only, nothing
trained. Every kind is then read the same way: freeze the cells, show the training digits
once, fit a fresh linear reader of the 3,072 cells' counts, score it on the test digits.
"""

import json
import math
import time

import numpy as np
import torch

import ladder2_data as D
import ladder2_model as M


def init_thresholds(model, x):
    """thresholds (and gains) where the cells start near the born sheet's activity"""
    with torch.no_grad():
        if isinstance(model, M.Net):
            return
        model.rewire()
        W = model.weights()
        drive = x @ W[:, :model.P].t()
        if model.ratecurve:
            top = drive * torch.pow(2.0, model.leak)
            model.theta.copy_(torch.quantile(top, 1 - M.TARGET_ACTIVE, dim=0).clamp(
                M.THRESHOLD_FLOOR, M.THRESHOLD_CAP))
            return
        q = torch.quantile(drive, 1 - M.TARGET_ACTIVE, dim=0)
        sd = drive.std(0).clamp(min=1e-3)
        model.theta.copy_(q)
        model.log_sigma.copy_(torch.log(sd / 2))


def regulate_only(model, task, gen, epochs=1):
    """a born arm: nothing learns, thresholds move toward the target count; with no rate
    curve one gain for all cells moves so the share of cells with a spike matches the born
    sheet's"""
    with torch.no_grad():
        for _ in range(epochs):
            x, _ = task.train_set()
            for idx in D.batches(len(x), 256, gen):
                c = model(x[idx])
                model.regulate(c, rate=0.1)
                if not model.ratecurve:
                    share = (c > 0).float().mean()
                    model.log_sigma.sub_(0.05 * float(share - M.TARGET_ACTIVE))


def train(model, task, epochs, lr, mode, gen, log):
    params = [p for n, p in model.named_parameters()
              if not (model_regulated(model) and n == "theta")]
    opt = torch.optim.Adam(params, lr=lr)
    steps = max(1, epochs * math.ceil(len(task.train_set()[1]) / 256))
    sched = torch.optim.lr_scheduler.LambdaLR(
        opt, lambda s: 0.5 * (1 + math.cos(math.pi * min(s, steps) / steps)))
    step = 0
    for epoch in range(epochs):
        x, y = task.train_set()
        t0, tot, right, n = time.time(), 0.0, 0, 0
        for idx in D.batches(len(x), 256, gen):
            if step % 4 == 0:
                model.rewire()
            c = model(x[idx])
            if mode == "auto":
                loss = torch.nn.functional.binary_cross_entropy_with_logits(
                    model.decoder(c), x[idx])
            else:
                out = model.head(c)
                loss = torch.nn.functional.cross_entropy(out, y[idx])
                right += int((out.argmax(1) == y[idx]).sum())
            opt.zero_grad()
            loss.backward()
            opt.step()
            sched.step()
            if model_regulated(model):
                model.regulate(c.detach())
            tot += float(loss.detach()) * len(idx)
            n += len(idx)
            step += 1
        log("epoch %d loss %.4f train right %.4f (%.0f s)" % (
            epoch + 1, tot / n, right / n, time.time() - t0))
    model.rewire()


def model_regulated(model):
    return getattr(model, "regulated", False)


def features(model, x):
    with torch.no_grad():
        return torch.cat([model(x[i:i + 1000]) for i in range(0, len(x), 1000)])


def head_score(model, f, y):
    with torch.no_grad():
        return float((model.head(f).argmax(1) == y).float().mean())


def class_share(f, y, classes):
    """what the cells respond to: the share of a cell's count variance that the answer
    explains (between-answer over total), averaged over cells that vary"""
    var = f.var(0, unbiased=False)
    means = torch.stack([f[y == c].mean(0) for c in range(classes)])
    shares = torch.tensor([float((y == c).float().mean()) for c in range(classes)])
    between = (shares[:, None] * (means - f.mean(0)) ** 2).sum(0)
    keep = var > 1e-6
    return float((between[keep] / var[keep]).mean()) if keep.any() else 0.0


def wiring_stats(model, sheet):
    """what the cells hear: inputs, pixel inputs, inhibitory inputs, strong ones; how far a
    cell's pixels sit from their middle; the share of the born wiring kept"""
    if isinstance(model, M.Net) or model.levels is None:
        return {}
    L = model.levels.numpy()
    wired = L > 0
    px = wired[:, :784]
    out = {"inputs": float(wired.sum(1).mean()), "pixels": float(px.sum(1).mean()),
           "strong": float((L >= 4).sum(1).mean())}
    if model.recurrent:
        out["inh_inputs"] = float((wired & sheet.src_inh[None, :model.N]).sum(1).mean())
        out["born_kept"] = float((wired & (sheet.born[:, :model.N] > 0)).sum() /
                                 max(1, (sheet.born[:, :model.N] > 0).sum()))
        if model.ri >= M.rung_index("5"):
            inh = sheet.src_inh
            out["exc_from_pixels"] = float(px.sum() / max(1, (wired & ~inh[None, :model.N]).sum()))
    ys, xs = np.divmod(np.arange(784), 28)
    spread = []
    for u in range(len(px)):
        p = np.flatnonzero(px[u])
        if len(p) >= 2:
            spread.append(np.mean(np.hypot(xs[p] - xs[p].mean(), ys[p] - ys[p].mean())))
    out["pixel_spread"] = float(np.mean(spread)) if spread else 0.0
    return out


def write_wiring(model, sheet, path):
    """the transplant file for `blobmnist ceiling --wiring-load`: each cell's sources"""
    L = model.levels.numpy()
    with open(path, "w") as f:
        for u, nrn in enumerate(sheet.cells):
            js = np.flatnonzero(L[u] > 0)
            f.write("%d %s\n" % (nrn, " ".join("%d:%d" % (sheet.neuron_of_col[j], L[u, j])
                                               for j in js)))


def run_one(task_name, rung, kind, seed, val, epochs, sheet_path, out_path, wiring_out=None,
            lr=2e-3, limit=0):
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
    res = {"task": task_name, "rung": rung, "kind": kind, "seed": seed, "val": int(val),
           "epochs": epochs}
    if rung == "lin":
        x, y = task.train_set()
        res["reader"] = D.reader(x, y, task.x_te, task.y_te, task.classes, seed)
    else:
        if rung == "net":
            model = M.Net(sheet.H, task.classes, seed)
        else:
            model = M.Ladder(rung, sheet, task.classes, seed,
                             decoder=784 if kind == "auto" else 0)
            if kind == "born" and model.ri >= M.rung_index("6"):
                model.born_wiring(sheet)
        x, _ = task.train_set()
        init_thresholds(model, x[:3000])
        if kind == "born":
            model.rewire()
            regulate_only(model, task, gen)
        else:
            train(model, task, epochs, lr, kind, gen, log)
        model.eval()
        x, y = task.train_set()
        f_tr, f_te = features(model, x), features(model, task.x_te)
        if kind == "trained":
            res["own"] = head_score(model, f_te, task.y_te)
        res["reader"] = D.reader(f_tr, y, f_te, task.y_te, task.classes, seed)
        res["active"] = float((f_te > 0).float().mean())
        res["mean_count"] = float(f_te.mean())
        res["silent_cells"] = float((f_te.sum(0) == 0).float().mean())
        res["class_share"] = class_share(f_te, task.y_te, task.classes)
        res.update(wiring_stats(model, sheet))
        if wiring_out and rung != "net":
            write_wiring(model, sheet, wiring_out)
    res["seconds"] = round(time.time() - t0)
    log(json.dumps(res))
    with open(out_path, "w") as f:
        f.write(json.dumps(res) + "\n")
    return res
