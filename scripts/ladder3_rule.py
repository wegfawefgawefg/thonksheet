"""The rule's ladder: idea 165's bottom rung (7b) taught by rules that step from backprop down
to the engine's one-hop rule (docs/results-2026-11-25-rule-ladder.md; scripts/ladder3.sh).
Backprop is a measuring stick here, never the project's method.

The cells are ladder2_model's rung 7b (the engine's cell at constant drive, the engine's
regulation, 2 settle steps, top-counter wiring at born counts, fixed signs, reach). Here its
teacher is written out by hand, so each part of it can be swapped:

  err      bp: the error reaches a cell through the output's own weights (backprop);
           fa: through fixed random wires, one per error line (dense);
           sparse: each line reaches a cell with chance 1/2, +1 or -1 (the engine's hop wires)
  hops     2: backprop also through the cell-to-cell step; 1: the error stops at the cell
  quant    none: each line's error as a number; sign: +1, -1, or 0 ("fine": under 0.1)
  gate     slope: the cell's derivative (the model's surrogate); fired: the cell spiked;
           near: near the bar (within 2/8 under it) and did not spike; nearfired: either
  step     grad: the score moves by the cell's error times the input; count: one count up or
           down with chance 2^-shift on each input that was on, integer counters (-31..31,
           born at their strengths), by the sign of the cell's error (the engine's step)
  mark     what an input counts for in a step. grad steps: value (a pixel 1, a cell its count
           / 20, its share of the drive) or bit (1 if it was on at all); count steps: bit (one
           chance per verdict, the engine's mark) or ticks (one chance per tick it was on: a
           pixel 20, a cell its count)
  cellsign the cell's own error kept only as its sign (more or less), grad steps
  cap      count steps: the counters' bound (31, genome tag_bits 6; 127 is tag_bits 8)
  verdict  full: the output's errors (softmax less the answer); pressed: told good or bad for
           the pressed button only (bad: it should be lower); right: on bad, the pressed one
           lower and the right one higher; answer: every step, each line's target less its
           running rate (the engine's hop_teach 3)
  learners all: every non-sensor cell learns; engine: only hidden excitatory cells
  coin     the cells get the error of another digit (drawn from the last 256), the output
           still learns from its own

The teacher runs in blocks: online (one digit at a time, plain steps) the block is 64 digits:
the output learns digit by digit, the scores move by the sum of the block's steps, and the
cells rewire and regulate at the block's end (the wiring only changes at a rewire, so the
scores reached are those of digit-by-digit steps). Batch (Adam, idea 165's teacher) the block
is 256 and the cells rewire every 4 blocks.
"""

import math
import time

import numpy as np
import torch

import ladder2_model as M

COUNTER_TOP = 31  # genome tag_bits 6
NEAR_EIGHTHS = 2  # genome hop_near
FINE = 0.1        # quant sign: an error under this is "fine"
RATE_SHIFT = 8    # genome hop_rate_shift (verdict answer)


def cell_kinds(path):
    """kind bits of each non-sensor cell, in the reader's order (1 sensor, 2 motor, 4
    inhibitory, 8 pacemaker; 0 is a hidden excitatory cell, the hop rule's learners)"""
    kinds = [int(l.split()[3]) for l in open(path).read().splitlines()[1:]]
    kinds = np.array(kinds)
    return kinds[(kinds & 1) == 0]


def compact(sheet, mask_cols):
    """each cell's candidates of one kind as a padded row of column indices (pad = N)"""
    rows = [np.flatnonzero(mask_cols[u]) for u in range(sheet.H)]
    width = max(1, max(len(r) for r in rows))
    N = sheet.P + sheet.H
    idx = np.full((sheet.H, width), N, np.int64)
    for u, r in enumerate(rows):
        idx[u, :len(r)] = r
    return torch.from_numpy(idx)


class Teacher:
    def __init__(self, sheet, sheet_path, model, cfg, seed, task_x):
        self.sheet, self.m, self.cfg = sheet, model, cfg
        self.g = torch.Generator().manual_seed(seed * 7919 + 1)
        H, P = sheet.H, sheet.P
        self.H, self.P, self.N = H, P, P + H
        self.idx = [compact(sheet, sheet.reach & ~sheet.src_inh[None, :]),
                    compact(sheet, sheet.reach & sheet.src_inh[None, :])]
        self.valid = [i < self.N for i in self.idx]
        self.gidx = [i.clamp(max=self.N - 1) for i in self.idx]  # gather from (H, N)
        self.vmask = [v.float() for v in self.valid]
        born = np.concatenate([sheet.born, np.zeros((H, 1), np.int8)], 1)
        self.born = [torch.from_numpy(born).float().gather(1, i) for i in self.idx]
        self.counts = cfg["step"] == "count"
        if cfg.get("start") == "random":  # idea 165's start (alone arms)
            self.S = [0.02 * torch.randn(i.shape, generator=self.g) for i in self.idx]
        elif self.counts:
            self.S = [b.clone() for b in self.born]
        else:
            self.S = [b * cfg.get("scale", 1.0) + 1e-3 * cfg.get("scale", 1.0) *
                      torch.rand(i.shape, generator=self.g) for b, i in zip(self.born, self.idx)]
        self.L = [torch.zeros_like(s) for s in self.S]
        self.sign = torch.from_numpy(np.where(sheet.src_inh, -float(M.INHIBIT_GAIN), 1.0)).float()
        K = model.head.out_features
        self.K = K
        if cfg["err"] == "fa":
            self.B = torch.randn(H, K, generator=self.g) / math.sqrt(K)
        elif cfg["err"] == "sparse":
            on = torch.rand(H, K, generator=self.g) < 0.5
            sgn = torch.where(torch.rand(H, K, generator=self.g) < 0.5, 1.0, -1.0)
            self.B = on * sgn
        learn = torch.ones(H, dtype=torch.bool)
        if cfg.get("learners") == "engine":
            learn = torch.from_numpy(cell_kinds(sheet_path) == 0)
        self.learn = learn.float()
        self.ring = torch.zeros(256, K)
        self.ring_n = 0
        self.rate = torch.zeros(K)
        self.steps_taken = 0.0
        self.rewire()
        self.init_thresholds(task_x)

    # the wiring
    def rewire(self):
        lv = []
        for i, (S, valid) in enumerate(zip(self.S, self.valid)):
            key = S
            if self.counts:  # at a tie the wired (and strong) stay first, the rest by chance
                key = S + 0.5 * (self.L[i] > 0) + 0.25 * (self.L[i] >= 4) + 0.1 * torch.rand(
                    S.shape, generator=self.g)
            lv.append(M.wiring_levels(key, valid, self.m.kk[:, i], self.m.kk4[:, i]))
        self.L = lv
        dense = torch.zeros(self.H, self.N + 1)
        for idx, l in zip(self.idx, self.L):
            dense.scatter_add_(1, idx, l)
        self.levels = dense[:, :self.N]
        self.W = self.levels * self.sign

    def init_thresholds(self, x):
        """thresholds where the cells start near the born sheet's activity (as ladder2_train's
        init_thresholds, on this wiring)"""
        m = self.m
        top = (x @ self.W[:, :self.P].t()) * torch.pow(2.0, m.leak)
        m.theta.data.copy_(torch.quantile(top, 1 - M.TARGET_ACTIVE, dim=0).clamp(
            M.THRESHOLD_FLOOR, M.THRESHOLD_CAP))

    # the cells: counts, the derivative's stand-in, and the bar
    def cells(self, x):
        m = self.m
        two_l = torch.pow(2.0, m.leak)
        T = m.theta.detach() + m.bias[m.sector]
        W = self.W
        dpx = x @ W[:, :self.P].t()
        c0 = m.rate_count(dpx).detach()
        drive = dpx + (c0 / M.TICKS) @ W[:, self.P:].t()
        c1 = m.rate_count(drive).detach()
        out = {"c0": c0, "c1": c1, "T": T}
        for name, d in (("s0", dpx), ("s1", drive)):
            z = (d * two_l - T) / T.abs().clamp(min=4.0) * 2.0
            inside = (z >= 0) & (z <= M.COUNT_MAX)
            out[name] = torch.where(inside, 1.0, 0.05) * two_l * 2.0 / T.abs().clamp(min=4.0)
        out["top"] = drive * two_l
        return out

    def gate(self, f):
        g = self.cfg["gate"]
        if g == "slope":
            return f["s1"]
        fired = f["c1"] > 0
        T = f["T"]
        near = torch.where(T > 0, f["top"] >= T - T * NEAR_EIGHTHS / 8.0, torch.ones_like(fired))
        if g == "fired":
            return fired.float()
        if g == "near":
            return (near & ~fired).float()
        return (near | fired).float()

    # the error lines of one digit, given its output (before the output learns from it)
    def line_errors(self, p, y):
        v, K = self.cfg["verdict"], self.K
        t = torch.zeros(K)
        t[y] = 1.0
        if v == "answer":
            e = self.rate - t  # positive: should have been lower
            self.rate += (t - self.rate) / (1 << RATE_SHIFT)
            return e
        if v in ("pressed", "right"):
            e = torch.zeros(K)
            k = int(p.argmax())
            if k != y:
                e[k] = 1.0
                if v == "right":
                    e[y] = -1.0
            return e
        e = p - t
        if self.cfg["quant"] == "sign":
            e = torch.sign(e) * (e.abs() >= FINE)
        return e

    def coin(self, e):
        """the coin control: this digit's error goes into the ring, another's comes out"""
        n = min(self.ring_n, 256)
        out = e
        if n > 0:
            out = self.ring[int(torch.randint(n, (1,), generator=self.g))].clone()
        self.ring[self.ring_n % 256] = e
        self.ring_n += 1
        return out

    def regulate(self, c, rate):
        self.m.regulate(c, rate=rate)


def score_step(t, x, f, d, d0, lr):
    """grad steps: S -= lr * cell error * input * sign, gathered into each kind's rows"""
    cells = (f["c0"] > 0).float() if t.cfg.get("mark") == "bit" else f["c0"] / M.TICKS
    a1 = torch.cat([x, cells], 1) * t.sign
    G = d.t() @ a1
    if d0 is not None:
        G[:, :t.P] += d0.t() @ x
    return [G.gather(1, i) * v for i, v in zip(t.gidx, t.vmask)]


def count_step(t, x, f, d, shift):
    """the engine's step: each input that was on moves its counter one toward the cell's
    teaching bit with chance 2^-shift (excitatory up for "more", inhibitory down)"""
    if t.cfg.get("mark") == "ticks":  # a chance per tick on: a pixel all 20, a cell its count
        on = torch.cat([x * M.TICKS, f["c0"]], 1)
    else:
        on = (torch.cat([x, f["c0"]], 1) > 0).float()
    more, less = (d < 0).float(), (d > 0).float()
    n_more, n_less = more.t() @ on, less.t() @ on
    p = 2.0 ** -shift
    out = []
    for i, (idx, v) in enumerate(zip(t.gidx, t.vmask)):
        nm, nl = n_more.gather(1, idx) * v, n_less.gather(1, idx) * v
        up, down = (nm, nl) if i == 0 else (nl, nm)
        k_up = torch.binomial(up, torch.full_like(up, p), generator=t.g)
        k_down = torch.binomial(down, torch.full_like(down, p), generator=t.g)
        out.append(k_up - k_down)
        t.steps_taken += float((k_up + k_down).sum())
    return out


def teach(t, task, digits, opt, log):
    """one life of `digits` training digits (several passes if more than a set) under the
    teacher's rule; online or batch"""
    cfg, m = t.cfg, t.m
    online = cfg.get("mode", "online") == "online"
    block = opt.get("block", 64) if online else 256
    head = m.head
    if not online:
        opt_s = torch.optim.Adam(t.S + list(head.parameters()), lr=cfg.get("lr", 2e-3))
        total = max(1, math.ceil(digits / block))
        sched = torch.optim.lr_scheduler.LambdaLR(
            opt_s, lambda s: 0.5 * (1 + math.cos(math.pi * min(s, total) / total)))
    seen, nblock, right, nright = 0, 0, 0, 0
    t_start = time.time()
    while seen < digits:
        x_all, y_all = task.train_set()
        order = torch.randperm(len(y_all), generator=t.g)
        for i0 in range(0, len(order), block):
            if seen >= digits:
                break
            ix = order[i0:min(i0 + block, len(order), i0 + digits - seen)]
            x, y = x_all[ix], y_all[ix]
            f = t.cells(x)
            c = f["c1"]
            E = torch.zeros(len(ix), t.K)
            Hb = torch.zeros(len(ix), t.H) if cfg["err"] == "bp" else None
            with torch.no_grad():
                if online:
                    V, b = head.weight, head.bias
                    for j in range(len(ix)):
                        o = V @ c[j] + b
                        p = torch.softmax(o, 0)
                        yj = int(y[j])
                        right += int(o.argmax()) == yj
                        e_out = p.clone()
                        e_out[yj] -= 1.0
                        e = t.line_errors(p, yj)
                        if cfg.get("coin"):
                            e = t.coin(e)
                        E[j] = e
                        if Hb is not None:
                            Hb[j] = e @ V
                        # the output's own delta rule, a step normalised by the input's size
                        step = opt["head_lr"] / (1.0 + float(c[j] @ c[j]))
                        V -= step * torch.outer(e_out, c[j])
                        b -= step * e_out
                else:
                    o = c @ head.weight.t() + head.bias
                    p = torch.softmax(o, 1)
                    right += int((o.argmax(1) == y).sum())
                    e_out = p.clone()
                    e_out[torch.arange(len(ix)), y] -= 1.0
                    for j in range(len(ix)):
                        e = t.line_errors(p[j], int(y[j]))
                        E[j] = t.coin(e) if cfg.get("coin") else e
                    if Hb is not None:
                        Hb = E @ head.weight
                    head.weight.grad = e_out.t() @ c / len(ix)
                    head.bias.grad = e_out.mean(0)
            nright += len(ix)
            h = Hb if Hb is not None else E @ t.B.t()
            g = t.gate(f)
            d = g * h * t.learn
            if cfg.get("cellsign"):  # the cell is told only more or less
                d = torch.sign(d)
            d0 = None
            if cfg["hops"] == 2 and cfg["err"] == "bp":
                d0 = (d @ t.W[:, t.P:] / M.TICKS) * f["s0"] * t.learn
            if t.counts:
                moves = count_step(t, x, f, torch.sign(d), opt["shift"])
                for S, mv in zip(t.S, moves):
                    top = cfg.get("cap", COUNTER_TOP)
                    S.add_(mv).clamp_(-top, top)
            elif online:
                for S, G in zip(t.S, score_step(t, x, f, d, d0, opt["lr"])):
                    S.sub_(opt["lr"] * G)
            else:
                for S, G in zip(t.S, score_step(t, x, f, d, d0, 1.0)):
                    S.grad = G / len(ix)
                opt_s.step()
                sched.step()
            seen += len(ix)
            nblock += 1
            t.regulate(c, 0.05 * len(ix) / 256.0)
            if online or nblock % 4 == 0:
                t.rewire()
            if nblock % (4096 // block) == 0:
                log("digits %d output right %.4f (%.0f s)" % (seen, right / max(1, nright),
                                                               time.time() - t_start))
                right, nright = 0, 0
    t.rewire()
    return right / max(1, nright)
