"""The credit rules not yet tried (idea 177; docs/results-2026-11-26-credit-rules.md).

The bench is the rule's ladder's (scripts/ladder3_rule.py): idea 165's model of the sheet's
3,072 cells at its bottom rung, a counter per candidate input per cell, the cell wired to its
top counters at its born counts, rewired and regulated every 64 digits, an output layer that
learns by its own delta rule. Backprop is not used by any rule here; it appears only as a
diagnostic (how often a rule's teaching bit has the sign backprop's would have).

Every rule ends the same way, as the ladder's rung T7x does: each cell gets a teaching bit for
the digit (more, less, or nothing) and every marked input's counter moves one count (chance
1, counters wide enough: "every-event"). The families differ in where the bit comes from:

  hop     the one-hop rule (T7x): the output's signed line errors on fixed random +-1 wires,
          gated by "near the bar or fired". With ladder_step it calls the ladder's own step,
          so it must give T7x's numbers (the check that this bench is that bench)
  phase   two phases: the output lines reach the cells on fixed +-1 wires as plain inputs.
          A free phase with the output's own activity on them, a clamped phase with the
          answer; no error is computed anywhere. events spike: every spike in the clamped
          phase counts its marked inputs up, every spike in the free phase counts them down.
          events sign: one count by the sign of the cell's count difference
  code    credit by code: each cell forces one more spike or vetoes one by its own code bit;
          runs 2: the output says whether the answer's line rose or fell against a clean
          run; runs 1: only whether the pressed button was right. A cell whose code took
          effect is told to do more of what it did if the outcome was good. n: cells
          perturbed a digit (0: all)
  pay     payment: the output pays back along its own read wires (the sign of each read
          weight) to the cells that fired; back: a paid cell passes the payment to the
          wired cells whose firing let it fire (back "one": to one of them, drawn by chance,
          so payment is conserved)
  target  a target for each cell from the answer alone on the fixed wires: fire, be silent,
          or no target; the cell moves toward it (near and silent: more; fired: less)
  tuned   difference targets with learned back wires: each cell keeps an up/down counter per
          answer line (did I fire when this was the answer), and the sign of that counter is
          its wire from that line; then as hop
  time    hop with the error arriving `delay` showings late and a mark on each input that
          lasts `last` showings (last 1: no trace)

coin: the teaching signal comes from another digit (hop, tuned, time: the ladder's ring of the
last 256 errors; the rest: another digit of the same block of 64).
"""

import time

import torch

import ladder2_model as M
import ladder3_rule as R

BLOCK = 64
CAP = 32767   # the ladder's every-event counters (16 bits; none passed 12 there)
RING = ("hop", "tuned", "time")


class Life:
    """what a rule keeps between blocks, and the diagnostics of the life"""

    def __init__(self, t, cfg):
        self.U = torch.zeros(t.H, t.K)  # tuned: an up/down counter per answer line per cell
        self.tail = None                # time: the last showings' marks, gates and bits
        self.srcs = None                # pay, back one: each cell's wired cell sources
        self.d = {k: 0.0 for k in ("agree", "taught", "more", "cells", "agree0", "taught0",
                                   "cells0", "events")}

    def note(self, dnet, hbp, seen):
        """how often the teaching bit has backprop's sign (the error through the output's
        own weights), over the life and over its first 4,096 digits"""
        if dnet is None:
            return
        on = dnet != 0
        same = float(((torch.sign(dnet) == torch.sign(hbp)) & on).sum())
        for tag in ("", "0") if seen < 4096 else ("",):
            self.d["agree" + tag] += same
            self.d["taught" + tag] += float(on.sum())
            self.d["cells" + tag] += float(on.numel())
        self.d["more"] += float((dnet < 0).sum())

    def summary(self):
        d = self.d
        return {"agree": d["agree"] / max(1.0, d["taught"]),
                "agree_first": d["agree0"] / max(1.0, d["taught0"]),
                "taught": d["taught"] / max(1.0, d["cells"]),
                "taught_first": d["taught0"] / max(1.0, d["cells0"]),
                "more_share": d["more"] / max(1.0, d["taught"]), "events": d["events"]}


def roll(g, *tensors):
    """every digit of the block gets another digit's rows"""
    n = len(tensors[0])
    k = int(torch.randint(1, n, (1,), generator=g)) if n > 1 else 0
    return [torch.roll(a, k, 0) for a in tensors]


def perturb(t, cfg, c):
    """credit by code: the cell's code bit forces one more spike or vetoes one"""
    if cfg["family"] != "code":
        return None
    q = cfg.get("n", 0) / t.H if cfg.get("n", 0) else 1.0
    sign = torch.where(torch.rand(c.shape, generator=t.g) < 0.5, 1.0, -1.0)
    code = sign * (torch.rand(c.shape, generator=t.g) < q)
    return (c + code).clamp(0, M.COUNT_MAX)


def output_pass(t, cfg, c, cp, y, head_lr):
    """the output, digit by digit as ladder3_rule.teach has it: its answer before it learns
    from the digit, its signed line errors, then its own delta rule. Beside them what a
    family needs from the output at that moment"""
    n, fam = len(y), cfg["family"]
    V, b = t.m.head.weight, t.m.head.bias
    o = {"P": torch.zeros(n, t.K), "E": torch.zeros(n, t.K), "Hv": torch.zeros(n, t.H),
         "Hbp": torch.zeros(n, t.H), "out": torch.zeros(n), "right": 0}
    one = fam == "code" and cfg.get("runs") == 1
    for j in range(n):
        cj = cp[j] if one else c[j]
        z = V @ cj + b
        p = torch.softmax(z, 0)
        yj = int(y[j])
        good = int(z.argmax()) == yj
        o["right"] += good
        e_out = p.clone()
        e_out[yj] -= 1.0
        e = t.line_errors(p, yj)
        if cfg.get("coin") and fam in RING:
            e = t.coin(e)
        o["P"][j], o["E"][j] = p, e
        o["Hbp"][j] = e_out @ V
        if fam == "pay":
            o["Hv"][j] = e @ torch.sign(V)
        elif one:  # good or bad only
            o["out"][j] = 1.0 if good else -1.0
        elif fam == "code":  # did the answer's line rise or fall against the clean run
            p2 = torch.softmax(V @ cp[j] + b, 0)
            o["out"][j] = torch.sign(p2[yj] - p[yj])
        step = head_lr / (1.0 + float(cj @ cj))
        V -= step * torch.outer(e_out, cj)
        b -= step * e_out
    return o


def settle(t, x, feedback):
    """the two settle steps with the output lines' drive added to every cell"""
    m, W = t.m, t.W
    dpx = x @ W[:, :t.P].t() + feedback
    a0 = m.rate_count(dpx)
    a1 = m.rate_count(dpx + (a0 / M.TICKS) @ W[:, t.P:].t())
    return a0, a1


def wired_cells(t):
    """each cell's wired cell sources as a padded row of columns, and each wire's sign"""
    Wc = t.W[:, t.P:]
    nz = (Wc != 0).float()
    val, idx = nz.topk(int(nz.sum(1).max()), dim=1)
    return idx, torch.sign(Wc.gather(1, idx)) * val


def pay_one(t, lf, c0on, d1):
    """conserved payment: a paying cell pays one of its wired cell sources that fired"""
    if lf.srcs is None:
        lf.srcs = wired_cells(t)
    idx, sgn = lf.srcs
    live = c0on[:, idx] * (sgn != 0)
    pick = (live * (0.01 + torch.rand(live.shape, generator=t.g))).argmax(2)
    rows = torch.arange(t.H)[None, :].expand_as(pick)
    pay = d1 * (live.sum(2) > 0) * sgn[rows, pick]
    return torch.zeros_like(d1).scatter_add_(1, idx[rows, pick], pay)


def pass_back(t, lf, cfg, x, f, d1):
    """a taught cell passes its bit to the wired cells whose firing let it fire; they step
    their pixel inputs (their first settle step hears pixels only)"""
    c0on = (f["c0"] > 0).float()
    h0 = pay_one(t, lf, c0on, d1) if cfg["back"] == "one" else d1 @ t.W[:, t.P:]
    return torch.cat([x, torch.zeros_like(c0on)], 1), torch.sign(h0) * c0on * t.learn


def time_steps(lf, cfg, on, g, hs):
    """the error of the digit shown `delay` showings ago reaches the cell now; each input's
    mark (it was on while the cell was near or fired) lasts `last` showings"""
    K, L = cfg["delay"], cfg["last"]
    if lf.tail is None:
        lf.tail = [z[:0] for z in (on, g, hs)]
    ON, G, HS = [torch.cat([a, b]) for a, b in zip(lf.tail, (on, g, hs))]
    s = torch.arange(len(lf.tail[0]), len(ON))
    marks, ds = [], []
    for a in range(L):
        u, e = s - a, s - K
        ok = (u >= 0) & (e >= 0)
        marks.append(ON[u[ok]])
        ds.append(HS[e[ok]] * G[u[ok]])
    lf.tail = [z[-(K + L):] for z in (ON, G, HS)]
    return torch.cat(marks), torch.cat(ds)


def signals(t, lf, cfg, x, y, f, o, cp):
    """the block's counter steps as (marks, d) pairs (d > 0: less, d < 0: more; a size
    above one is that many events), and the net teaching bit for the diagnostics"""
    fam, coin = cfg["family"], cfg.get("coin")
    if fam == "still":  # nothing learns: the born wiring as this bench writes it out
        return [], None
    c0, c1 = f["c0"], f["c1"]
    on = (torch.cat([x, c0], 1) > 0).float()
    fired = (c1 > 0).float()
    if fam in RING:
        # tuned: a line's wire is +1 if the cell fires more than its own average when that
        # line is the answer, -1 if less
        wires = torch.sign(lf.U - lf.U.mean(1, keepdim=True)) if fam == "tuned" else t.B
        gate = t.gate(f)
        d = torch.sign(gate * (o["E"] @ wires.t())) * t.learn
        if fam == "tuned":  # +3 if it fired when this line was the answer, -1 if not
            for j in range(len(y)):
                k = int(y[j])
                lf.U[:, k] = (lf.U[:, k] + torch.where(fired[j] > 0, 3.0, -1.0)).clamp(-127, 127)
        if fam == "time":
            return [time_steps(lf, cfg, on, gate, d)], None
        steps = [(on, d)]
    elif fam == "phase":
        P, T = o["P"], torch.nn.functional.one_hot(y, t.K).float()
        if coin:
            P, T = roll(t.g, P, T)
        a0, a1 = settle(t, x, cfg["gain"] * (P @ t.B.t()))
        b0, b1 = settle(t, x, cfg["gain"] * (T @ t.B.t()))
        d = torch.sign(a1 - b1) * t.learn
        on_c = (torch.cat([x, b0], 1) > 0).float()
        if cfg.get("events") == "sign":
            steps = [(on_c, d)]
        else:
            steps = [(on_c, -b1 * t.learn), ((torch.cat([x, a0], 1) > 0).float(), a1 * t.learn)]
    elif fam == "code":
        out = roll(t.g, o["out"])[0] if coin else o["out"]
        d = -(out[:, None] * (cp - c1)) * t.learn
        steps = [(on, d)]
    elif fam == "pay":
        hv = roll(t.g, o["Hv"])[0] if coin else o["Hv"]
        d = torch.sign(hv) * fired * t.learn
        steps = [(on, d)]
    elif fam == "target":
        tg = t.B[:, roll(t.g, y)[0] if coin else y].t()
        near = t.gate(f) * (1 - fired)
        d = ((tg < 0).float() * fired - (tg > 0).float() * near) * t.learn
        steps = [(on, d)]
    else:
        raise ValueError(fam)
    if cfg.get("back"):
        steps.append(pass_back(t, lf, cfg, x, f, d))
    return steps, d


def apply_step(t, lf, on, d, count):
    """every marked input's counter moves one count per event toward the cell's bit:
    excitatory inputs up for "more", inhibitory inputs down"""
    G = (-d).t() @ on
    for i, (idx, v) in enumerate(zip(t.gidx, t.vmask)):
        mv = G.gather(1, idx) * v
        t.S[i].add_(mv if i == 0 else -mv).clamp_(-t.cfg["cap"], t.cfg["cap"])
    if count:  # counter steps, sampled
        lf.d["events"] += float((d.abs().t() @ on).gather(1, t.gidx[0]).mul(t.vmask[0]).sum())


def live(t, task, digits, cfg, log, head_lr=0.3):
    """one life of `digits` training digits under the rule, online, in blocks of 64 as the
    ladder's teacher runs them"""
    lf = Life(t, cfg)
    seen = nblock = right = nright = 0
    t0 = time.time()
    while seen < digits:
        x_all, y_all = task.train_set()
        order = torch.randperm(len(y_all), generator=t.g)
        for i0 in range(0, len(order), BLOCK):
            if seen >= digits:
                break
            ix = order[i0:min(i0 + BLOCK, len(order), i0 + digits - seen)]
            x, y = x_all[ix], y_all[ix]
            f = t.cells(x)
            c = f["c1"]
            cp = perturb(t, cfg, c)
            o = output_pass(t, cfg, c, cp, y, head_lr)
            right += o["right"]
            nright += len(ix)
            if cfg.get("ladder_step"):  # the ladder's own step, for the check against T7x
                dnet = torch.sign(t.gate(f) * (o["E"] @ t.B.t()) * t.learn)
                for S, mv in zip(t.S, R.count_step(t, x, f, dnet, 0)):
                    S.add_(mv).clamp_(-CAP, CAP)
            else:
                steps, dnet = signals(t, lf, cfg, x, y, f, o, cp)
                for on, d in steps:
                    apply_step(t, lf, on, d, nblock % 16 == 0)
            lf.note(dnet, o["Hbp"], seen)
            seen += len(ix)
            nblock += 1
            t.regulate(c, 0.05 * len(ix) / 256.0)
            t.rewire()
            lf.srcs = None
            if nblock % 64 == 0:
                log("digits %d output right %.4f (%.0f s)" % (seen, right / max(1, nright),
                                                               time.time() - t0))
                right, nright = 0, 0
    t.rewire()
    lf.d["events"] *= 16
    return right / max(1, nright), lf
