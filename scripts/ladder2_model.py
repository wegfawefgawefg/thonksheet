"""The inside's ladder: the offline model of the sheet's cells, one constraint per rung.

docs/results-2026-11-25-inside-ladder.md. Backprop is the measuring stick here, not the
method. Every rung has 3,072 cells, one for each non-sensor cell of scripts/mnist.sh's sheet,
and the real cells' numbers come from the born sheet (`blobmnist ceiling --describe`). A cell
gives a spike count for the digit (0..7); a cell that hears other cells hears their count /
20 per tick, since a pixel line fires on all 20 ticks of a digit and a hidden cell only on
its spikes. The rungs, each adding to the one before:

  1    count units, full fan-in from the 784 pixels, float weights, threshold and gain learned
  2    fan-in cut to the cell's born count, the inputs chosen as the top of a learned score
       per candidate (the engine's counters: wire the top counters at the born count)
  3a   strengths all 1 (sign free)
  3b   strengths 1 and 4 at the cell's born counts (no cell is born with a 5)
  4    cells hear cells: every pixel and every other cell is a candidate, 2 settle steps
       (4s3: 3 settle steps)
  5    a fixed sign per source: pixels excite, a cell excites or inhibits everything it
       reaches as born (25% inhibitory, at the engine's inhibit_gain 5); excitatory and
       inhibitory inputs, and strong ones, at the cell's born counts of each kind
  6    candidates only within reach: the 9 tiles around the cell and its far tiles
  7a   activity held at the engine's rate: each threshold set by regulation (not by the
       teacher) to 0.66 spikes a digit, the born sheet's 33 per 1,000 cell-ticks
  7b   the engine's own cell and regulation: a leaky integrator with the cell's own leak
       and cooldown at constant drive a tick, so a count is 20 / ticks between spikes; its
       threshold is the cell's own (regulated, 1 to 64) plus its sector's bias (regulated
       on the sector's mean, -16 to 16), as src/blob/regulate.cpp bounds them

Gradients pass the non-differentiable steps straight through: a rounded count passes the
gradient of its unrounded value (with a small slope outside its range so a silent cell can
come back); a wiring (top-K of the scores, then strengths by rank) passes the gradient of the
weight to the score.
"""

import math

import numpy as np
import torch

COUNT_MAX = 7
TARGET = 0.66        # spikes a digit a cell: the born sheet's 33 per 1,000 cell-ticks x 20
TARGET_ACTIVE = 0.42  # share of cell-digits with a spike on the born sheet
THRESHOLD_FLOOR, THRESHOLD_CAP = 1, 64  # regulate.cpp, genome threshold_cap
BIAS_MAX = 16                           # genome sector_bias_max
TICKS = 20
INHIBIT_GAIN = 5

RUNGS = ["1", "2", "3a", "3b", "4", "4s3", "5", "6", "7a", "7b"]


def rung_index(r):
    return RUNGS.index(r)


class SheetInfo:
    """the born sheet from `blobmnist ceiling --describe`, in the model's columns: sources
    0..783 are the pixels, 784.. the non-sensor cells in neuron order (the reader's order)"""

    def __init__(self, path):
        lines = open(path).read().splitlines()
        n, tw, th = map(int, lines[0].split()[1:])
        rows = [l.split() for l in lines[1:]]
        kind = np.array([int(r[3]) for r in rows])
        pixel = np.array([int(r[4]) for r in rows])
        self.leak = np.array([int(r[6]) for r in rows])
        xy = np.array([[int(r[1]), int(r[2])] for r in rows])
        self.cool = np.array([int(r[7]) for r in rows])
        far = np.array([[int(r[8]), int(r[9])] for r in rows])
        sensor = (kind & 1) > 0
        self.inh_neuron = (kind & 4) > 0
        self.cells = np.flatnonzero(~sensor)  # neuron of each cell
        # sector of each cell: 2 x 2 tiles (genome sector_tiles)
        sw = (tw + 1) // 2
        self.sector = (xy[self.cells, 1] // 16) * sw + xy[self.cells, 0] // 16
        H, P = len(self.cells), 784
        self.H, self.P = H, P
        col = np.full(n, -1)
        col[pixel >= 0] = pixel[pixel >= 0]
        col[self.cells] = P + np.arange(H)
        self.col = col
        self.neuron_of_col = np.zeros(P + H, np.int64)
        self.neuron_of_col[pixel[pixel >= 0]] = np.flatnonzero(pixel >= 0)
        self.neuron_of_col[P:] = self.cells
        # kind of each source column: inhibitory cells inhibit
        self.src_inh = np.zeros(P + H, bool)
        self.src_inh[P:] = self.inh_neuron[self.cells]
        # born wiring and counts per cell (sources that are silent sensor cells count too)
        self.born = np.zeros((H, P + H), np.int8)
        self.k_all = np.zeros(H, np.int64)
        self.k_strong = np.zeros(H, np.int64)
        self.k_kind = np.zeros((H, 2), np.int64)
        self.k_strong_kind = np.zeros((H, 2), np.int64)
        for u, nrn in enumerate(self.cells):
            for item in rows[nrn][10:]:
                s, w = map(int, item.split(":"))
                inh = int(self.inh_neuron[s] and not sensor[s])
                self.k_all[u] += 1
                self.k_kind[u, inh] += 1
                if w >= 4:
                    self.k_strong[u] += 1
                    self.k_strong_kind[u, inh] += 1
                if col[s] >= 0:
                    self.born[u, col[s]] = w
        # reach: the 9 local tiles and the far tiles (far slots carry excitatory sources)
        tile = np.arange(n) // 64
        tx, ty = tile % tw, tile // tw
        self.reach = np.zeros((H, P + H), bool)
        src_cols = np.flatnonzero(col >= 0)
        for u, nrn in enumerate(self.cells):
            t = nrn // 64
            near = (np.abs(tx[src_cols] - t % tw) <= 1) & (np.abs(ty[src_cols] - t // tw) <= 1)
            for f in far[nrn]:
                if f >= 0:
                    near |= (tile[src_cols] == f) & ~self.inh_neuron[src_cols]
            self.reach[u, col[src_cols[near]]] = True
            self.reach[u, P + u] = False


def wiring_levels(score, cand, k, k_strong, floor=float("-inf")):
    """the top k[u] candidates of each row by score: the top k_strong[u] at 4, the rest at 1;
    none whose score is not above floor"""
    s = score.masked_fill(~cand, float("-inf"))
    kmax = int(k.max())
    if kmax == 0:
        return torch.zeros_like(score)
    val, idx = s.topk(kmax, dim=1)
    r = torch.arange(kmax).unsqueeze(0)
    lev = torch.where(r < k_strong.unsqueeze(1), 4.0, torch.where(r < k.unsqueeze(1), 1.0, 0.0))
    lev = lev * (val > floor)
    return torch.zeros_like(score).scatter_(1, idx, lev)


def through(forward, backward):
    """forward's value, backward's gradient"""
    return backward + (forward - backward).detach()


class Ladder(torch.nn.Module):
    def __init__(self, rung, sheet, classes, seed, decoder=0):
        super().__init__()
        self.rung, self.ri = rung, rung_index(rung)
        g = torch.Generator().manual_seed(seed)
        H, P = sheet.H, sheet.P
        self.H, self.P = H, P
        self.recurrent = self.ri >= rung_index("4")
        self.steps = 3 if rung == "4s3" else 2 if self.recurrent else 1
        N = P + H if self.recurrent else P
        self.N = N
        cand = torch.ones(H, N, dtype=torch.bool)
        if self.recurrent:
            cand[:, P:] &= ~torch.eye(H, dtype=torch.bool)
        if self.ri >= rung_index("6"):
            cand &= torch.from_numpy(sheet.reach[:, :N])
        self.register_buffer("cand", cand)
        self.register_buffer("k", torch.from_numpy(sheet.k_all))
        self.register_buffer("k4", torch.from_numpy(sheet.k_strong))
        self.register_buffer("kk", torch.from_numpy(sheet.k_kind))
        self.register_buffer("kk4", torch.from_numpy(sheet.k_strong_kind))
        inh = torch.from_numpy(sheet.src_inh[:N])
        self.register_buffer("src_inh", inh)
        self.register_buffer("src_sign", torch.where(inh, -float(INHIBIT_GAIN), 1.0))
        # float rungs: the usual scale; wired rungs: small, so the teacher sets the ranks fast
        scale = 1.0 / math.sqrt(P) if self.ri < rung_index("3a") else 0.02
        self.u = torch.nn.Parameter(torch.randn(H, N, generator=g) * scale)
        self.theta = torch.nn.Parameter(torch.zeros(H))
        self.log_sigma = torch.nn.Parameter(torch.zeros(H))
        self.regulated = self.ri >= rung_index("7a")
        self.ratecurve = rung == "7b"
        self.register_buffer("leak", torch.from_numpy(sheet.leak[sheet.cells]).float())
        self.register_buffer("cool", torch.from_numpy(sheet.cool[sheet.cells]).float())
        self.register_buffer("sector", torch.from_numpy(sheet.sector).long())
        self.register_buffer("bias", torch.zeros(int(sheet.sector.max()) + 1))
        self.head = torch.nn.Linear(H, classes)
        self.decoder = torch.nn.Linear(H, decoder) if decoder else None
        self.levels = None
        self.floor = float("-inf")

    def born_wiring(self, sheet):
        """the scores that wire exactly the born sheet (rungs 6 and below 7b's born arm)"""
        with torch.no_grad():
            b = torch.from_numpy(sheet.born[:, :self.N].astype(np.float32))
            self.u.copy_(b + 0.01 * torch.rand_like(b) * (b > 0))
        self.floor = 0.5

    def rewire(self):
        """the wiring the scores pick now (strength levels, without signs)"""
        with torch.no_grad():
            if self.ri < rung_index("2"):
                self.levels = None
            elif self.ri < rung_index("5"):
                mag = self.u.abs()
                if self.ri < rung_index("3b"):
                    self.levels = wiring_levels(mag, self.cand, self.k, torch.zeros_like(self.k),
                                                self.floor)
                else:
                    self.levels = wiring_levels(mag, self.cand, self.k, self.k4, self.floor)
            else:
                exc = wiring_levels(self.u, self.cand & ~self.src_inh, self.kk[:, 0],
                                    self.kk4[:, 0], self.floor)
                inh = wiring_levels(self.u, self.cand & self.src_inh, self.kk[:, 1],
                                    self.kk4[:, 1], self.floor)
                self.levels = exc + inh

    def weights(self):
        u = self.u
        if self.ri < rung_index("2"):
            return u
        wired = self.levels > 0
        if self.ri < rung_index("3a"):
            return through(u * wired, u)
        if self.ri < rung_index("5"):
            return through(torch.sign(u) * self.levels, u)
        return self.src_sign * through(self.levels, u)

    def count(self, drive):
        """a cell's spike count for the digit from its drive a tick"""
        if self.ratecurve:
            return self.rate_count(drive)
        sigma = torch.exp(self.log_sigma)
        theta = self.theta.detach() if self.regulated else self.theta
        z = (drive - theta) / sigma
        soft = z.clamp(0, COUNT_MAX)
        leaky = soft + 0.05 * (z - soft)
        return through(torch.round(soft), leaky)

    def rate_count(self, drive):
        """the engine's cell at constant drive D a tick: potential p <- p - p/2^L + D rises
        toward D 2^L; it fires when it reaches the threshold T and starts again from 0, and
        it cannot fire for `cool` ticks after a spike. count = 20 / ticks between spikes"""
        two_l = torch.pow(2.0, self.leak)
        a = 1.0 - 1.0 / two_l
        top = drive * two_l
        T = self.theta.detach() + self.bias[self.sector]
        rested = self.cool + 1.0
        # a positive bar: reached after n ticks from a reset, if the potential's top passes it
        ratio = (T / top.clamp(min=1e-3)).clamp(0, 1 - 1e-4)
        n = torch.log1p(-ratio) / torch.log(a)
        rate_pos = torch.where(top > T, TICKS / torch.maximum(n.clamp(min=1.0), rested),
                               torch.zeros_like(top))
        # a bar at or under 0 (a sector bias under minus the threshold): a reset potential is
        # already over it, so the cell fires whenever rested, unless inhibition pulls the
        # potential under the bar before it has rested
        over = top * (1 - torch.pow(a, rested)) >= T
        rate_neg = torch.where(over, TICKS / rested, torch.zeros_like(top))
        rate = torch.where(T > 0, rate_pos, rate_neg)
        soft = rate.clamp(0, COUNT_MAX)
        # the gradient: as if the count rose by one per threshold's worth of drive over the bar
        z = (top - T) / T.abs().clamp(min=4.0) * 2.0
        leaky = z.clamp(0, COUNT_MAX) + 0.05 * (z - z.clamp(0, COUNT_MAX))
        return through(torch.floor(soft), leaky)

    def forward(self, x):
        W = self.weights()
        Wp = W[:, :self.P]
        drive_px = x @ Wp.t()
        c = torch.zeros_like(drive_px)
        for s in range(self.steps):
            drive = drive_px
            if self.recurrent and s > 0:
                drive = drive + (c / TICKS) @ W[:, self.P:].t()
            c = self.count(drive)
        return c

    def regulate(self, c, rate=0.05):
        """per cell, move the threshold toward the target count (rungs 7a, 7b, born arms);
        7b also moves each sector's bias toward the target on the sector's mean, both within
        the engine's bounds"""
        with torch.no_grad():
            err = c.mean(0) - TARGET
            if self.ratecurve:
                self.theta.add_(rate * 4.0 * err * self.theta).clamp_(THRESHOLD_FLOOR,
                                                                      THRESHOLD_CAP)
                n = torch.zeros_like(self.bias).index_add_(0, self.sector, torch.ones_like(err))
                serr = torch.zeros_like(self.bias).index_add_(0, self.sector, err) / n
                self.bias.add_(rate * 20.0 * serr).clamp_(-BIAS_MAX, BIAS_MAX)
            else:
                self.theta.add_(rate * err * torch.exp(self.log_sigma))


class Net(torch.nn.Module):
    """rung 0: the unconstrained net, 3,072 relu units on the pixels, float weights"""

    def __init__(self, H, classes, seed):
        super().__init__()
        torch.manual_seed(seed)
        self.hid = torch.nn.Linear(784, H)
        self.head = torch.nn.Linear(H, classes)
        self.decoder = None

    def rewire(self):
        pass

    def forward(self, x):
        return torch.relu(self.hid(x))
