"""Label-free rule families (idea 173): does noise in the model's cells make the model read
wirings the way the real sheet does? (docs/results-2026-11-26-label-free.md, calibration)

The model reads competitive learning's wiring under born; the real sheet reads it above. This
probe gives the model's cells noise and reads again: born, and family 0 lived on real digits,
on shuffled digits and as churn (2,000-digit lives: the wiring's shape is settled by then;
10,000-digit fit). Noise level s > 0: Gaussian noise on every cell's drive, s times the drive
that reaches its threshold; s < 0: every count, fed on and read, a Poisson draw with mean the
count plus (-s - 1). It prints the reader, a cell's counts against themselves on a second
draw (the real sheet: 0.61), and the rate.

  ladder4.py noise TASK LEVELS SHEET [SEED]     e.g. noise mnist 0,0.25,0.5,-1 out/ladder4/main/sheet-5.txt
"""

import torch

import ladder2_data as D
import ladder2_model as M
import ladder4_data as X
import ladder4_life as L


def noisy_answer(model, x, W, s, gen):
    with torch.no_grad():
        drive_px = x @ W[:, :model.P].t()
        if s < 0:
            lam = -s - 1.0
            c = torch.poisson(model.count(drive_px) + lam, generator=gen).clamp(max=M.COUNT_MAX)
            for _ in range(model.steps - 1):
                d = drive_px + (c / M.TICKS) @ W[:, model.P:].t()
                c = torch.poisson(model.count(d) + lam, generator=gen).clamp(max=M.COUNT_MAX)
            return c
        T = (model.theta.detach() + model.bias[model.sector]).clamp(min=1.0)
        sd = s * T / torch.pow(2.0, model.leak)
        c = model.count(drive_px + sd * torch.randn(drive_px.shape, generator=gen))
        for _ in range(model.steps - 1):
            d = drive_px + (c / M.TICKS) @ W[:, model.P:].t()
            c = model.count(d + sd * torch.randn(d.shape, generator=gen))
        return c


def self_corr(a, b):
    a, b = a - a.mean(0), b - b.mean(0)
    den = a.std(0, unbiased=False) * b.std(0, unbiased=False)
    ok = den > 1e-6
    return float(((a * b).mean(0)[ok] / den[ok]).mean())


def probe(task_name, levels, sheet_path, seed=5):
    torch.set_num_threads(1)
    opt = dict(limit=10000, life=2000, checkpoints=[2000], shift=5, rate=0.125, push=0.25,
               batch=32, cap=0, reg=0.05, moved=False)
    for arm, ctrl in [("born", "life"), ("f0", "life"), ("f0", "shuffled"), ("f0", "churn")]:
        task = D.Task(task_name, False, seed, opt["limit"])
        sheet = M.SheetInfo(sheet_path)
        life = L.Life(task, sheet, sheet_path, seed, opt)
        if arm != "born":
            life.live(arm, ctrl, X.life_stream(task, opt["life"], seed), set())
        x, y = task.train_set()
        life.regulate(x[:3000])
        for s in levels:
            gen = torch.Generator().manual_seed(11)
            feat = lambda z: torch.cat([noisy_answer(life.model, z[i:i + 1000], life.W, s, gen)
                                        for i in range(0, len(z), 1000)])
            f_tr, f_te, f_te2 = feat(x), feat(task.x_te), feat(task.x_te)
            acc = D.reader(f_tr, y, f_te, task.y_te, task.classes, seed)
            print("%-5s %-9s noise %5.2f  reader %.4f  self %.3f  rate %.1f" % (
                arm, ctrl, s, acc, self_corr(f_te, f_te2), float(f_te.mean()) * 50), flush=True)
