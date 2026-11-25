"""Label-free rule families in the model (idea 173): each family's upper bound.

docs/results-2026-11-26-label-free.md. Backprop here is only a measuring stick: for a family
whose aim can be written as a loss, the bottom rung (7b, idea 165's model: the engine's cell,
reach, fixed signs, 1 and 4 at born counts, the engine's regulation) is trained by backprop on
that loss with no labels, and read by the same fresh reader. That is about the best any
optimiser gets from the objective under the cells' constraints. If the bound is no better than
born, the objective is no good; if the bound is good and the local rule is not, the local
step fails to reach it.

  real    family 1: per tile, its spikes say "real": a logistic of the tile's summed count
          against a learned level, real digits 1, made-up ones 0 (Forward-Forward's goodness)
  pixel   family 2: each cell predicts its own target pixel (in its reach, not wirable) from
          its count through its own scale and offset
  looks   family 3: two looks at a digit (moved a pixel, a little noise) give the same
          population answer and different digits different ones (InfoNCE, temperature 0.1)
  apart   family 4: give the pixels back through a linear decoder (the autoencoder) and keep
          the cells of a tile uncorrelated (mean squared correlation within tiles)
  bcm     family 5: BCM's objective per cell, E[c^3]/3 - E[c^2]^2/4, at the engine's rate
Family 0's bound is idea 165's autoencoder (ladder2 kind "auto"), read from its outputs.
"""

import json
import math
import time

import torch
import torch.nn.functional as Fn

import ladder2_data as D
import ladder2_model as M
import ladder2_train as T
import ladder4_data as X

OBJECTIVES = ["real", "pixel", "looks", "apart", "bcm"]


def tile_sum(c, tile, tiles):
    return torch.zeros(c.shape[0], tiles).index_add_(1, tile, c)


class Aims(torch.nn.Module):
    """the few numbers an objective has besides the cells (levels, scales, a decoder)"""

    def __init__(self, H, tiles, decoder):
        super().__init__()
        self.level = torch.nn.Parameter(torch.full((tiles,), 30.0))
        self.scale = torch.nn.Parameter(torch.ones(H))
        self.offset = torch.nn.Parameter(torch.zeros(H))
        self.tile_scale = torch.nn.Parameter(torch.full((1,), 0.1))
        self.decoder = torch.nn.Linear(H, 784) if decoder else None


def loss_of(obj, model, aims, xb, gen, ctx):
    if obj == "real":
        xf = X.made_up(xb, gen)
        c = model(torch.cat([xb, xf]))
        g = tile_sum(c, ctx["tile"], ctx["tiles"])
        logit = aims.tile_scale * (g - aims.level)
        y = torch.cat([torch.ones(len(xb)), torch.zeros(len(xf))]).unsqueeze(1).expand_as(logit)
        return Fn.binary_cross_entropy_with_logits(logit, y), c
    if obj == "pixel":
        c = model(xb)
        t = ctx["target"]
        has = (t >= 0).float()
        y = xb[:, t.clamp(min=0)]
        logit = aims.scale * c + aims.offset
        return (Fn.binary_cross_entropy_with_logits(logit, y, reduction="none") * has).sum(1).mean() \
            / has.sum(), c
    if obj == "looks":
        x2 = X.second_look(xb, gen)
        c = model(torch.cat([xb, x2]))
        z = c - c.mean(0)
        z = Fn.normalize(z, dim=1)
        a, b = z[:len(xb)], z[len(xb):]
        logits = a @ b.t() / 0.1
        y = torch.arange(len(xb))
        return 0.5 * (Fn.cross_entropy(logits, y) + Fn.cross_entropy(logits.t(), y)), c
    if obj == "apart":
        c = model(xb)
        rec = Fn.binary_cross_entropy_with_logits(aims.decoder(c), xb)
        z = (c - c.mean(0)) / (c.std(0) + 1e-3)
        zt = z.t().reshape(ctx["tiles"], -1, len(xb))         # cells are in tile order, 48 a tile
        corr = zt @ zt.transpose(1, 2) / len(xb)
        off = corr - torch.diag_embed(torch.diagonal(corr, dim1=1, dim2=2))
        return rec + (off ** 2).mean(), c
    if obj == "bcm":
        c = model(xb)
        return -((c ** 3).mean(0) / 3 - (c ** 2).mean(0) ** 2 / 4).mean(), c
    raise ValueError(obj)


def run_one(task_name, obj, seed, sheet_path, out_path, epochs, limit=0, lr=2e-3):
    torch.set_num_threads(1)
    torch.manual_seed(seed)
    gen = torch.Generator().manual_seed(seed)
    logf = open(out_path + ".log", "w")
    t0 = time.time()
    task = D.Task(task_name, False, seed, limit)
    sheet = M.SheetInfo(sheet_path)
    model = M.Ladder("7b", sheet, task.classes, seed)
    tile = torch.from_numpy(sheet.cells // 64)
    ctx = {"tile": tile, "tiles": int(tile.max()) + 1}
    x0, _ = task.train_set()
    if obj == "pixel":
        use = (model.cand & ~model.src_inh).numpy()
        t = X.target_pixels(sheet, use, x0, seed)
        j = torch.nonzero(t >= 0).flatten()
        model.cand[j, t[j]] = False
        ctx["target"] = t
    aims = Aims(sheet.H, ctx["tiles"], obj == "apart")
    T.init_thresholds(model, x0[:3000])
    params = [p for n, p in model.named_parameters() if n != "theta"] + list(aims.parameters())
    opt = torch.optim.Adam(params, lr=lr)
    steps = max(1, epochs * math.ceil(len(x0) / 256))
    sched = torch.optim.lr_scheduler.LambdaLR(
        opt, lambda s: 0.5 * (1 + math.cos(math.pi * min(s, steps) / steps)))
    step = 0
    for epoch in range(epochs):
        x, _ = task.train_set()
        tot, n, t1 = 0.0, 0, time.time()
        for idx in D.batches(len(x), 256, gen):
            if step % 4 == 0:
                model.rewire()
            loss, c = loss_of(obj, model, aims, x[idx], gen, ctx)
            opt.zero_grad()
            loss.backward()
            opt.step()
            sched.step()
            model.regulate(c.detach())
            tot += float(loss.detach()) * len(idx)
            n += len(idx)
            step += 1
        logf.write("epoch %d loss %.4f (%.0f s)\n" % (epoch + 1, tot / n, time.time() - t1))
        logf.flush()
    model.rewire()
    model.eval()
    x, y = task.train_set()
    f_tr, f_te = T.features(model, x), T.features(model, task.x_te)
    res = {"task": task_name, "objective": obj, "seed": seed, "epochs": epochs,
           "reader": D.reader(f_tr, y, f_te, task.y_te, task.classes, seed),
           "rate": float(f_te.mean()) * 1000 / M.TICKS, "active": float((f_te > 0).float().mean()),
           "silent_cells": float((f_te.sum(0) == 0).float().mean())}
    res.update(T.wiring_stats(model, sheet))
    res["seconds"] = round(time.time() - t0)
    logf.write(json.dumps(res) + "\n")
    with open(out_path, "w") as f:
        f.write(json.dumps(res) + "\n")
    return res
