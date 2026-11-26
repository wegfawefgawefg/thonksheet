"""Label-free rule families in the model (idea 173): the counters and the local rules.

docs/results-2026-11-26-label-free.md. Every rule works on the wiring the way the engine
does (src/blob/tag_store.cpp, learn_compete.cpp): a counter per candidate input per cell
(signed, 6 bits: -31..31, starting at the born strength 0, 1 or 4), integer steps up or
down, the cell wired to its top counters at its born counts of excitatory and of strong
inputs. Only hidden excitatory cells learn, only their excitatory sources move (the engine's
learners); inhibitory inputs keep the born wiring. A rule says, for one digit, how a cell's
counters for its on-inputs (and for its silent ones) move, from what the cell can see. No
gradient passes between cells anywhere in this file.

The model has no ticks: a cell's count c (0..7 spikes a digit) stands for c spikes in 20
ticks. A pixel source is on all 20 ticks of a digit it is on; a cell source with count c is
on at the tick a spike reads it with chance c / 20. So "the sources active on this spike" is
A = pixels, plus c / 20 for cells. The engine moves a cell with chance 2^-shift a spike; here
a cell makes c 2^-shift moves a digit in expectation. The steps of a batch of digits are
summed as expectations and rounded by chance to whole steps (the variance is lower than the
engine's; the mean is the same); the wiring is re-picked after every batch.
"""

import torch

TOP = 31  # genome tag_bits 6: counters -31..31


class Store:
    """the learners' counters and the read-off (the engine's tag store). kept compact: each
    learner's usable candidates (excitatory sources in reach) in a padded row, `idx` their
    columns; the rules see full rows (`full`)"""

    def __init__(self, model, learners, seed):
        self.L = learners
        use = (model.cand & ~model.src_inh)[learners]               # excitatory, in reach
        self.N = use.shape[1]
        K = int(use.sum(1).max())
        order = use.float().argsort(dim=1, descending=True, stable=True)[:, :K]
        self.idx = order
        self.valid = use.gather(1, order)
        lev = model.levels[learners]
        self.C = lev.gather(1, order) * self.valid                  # born strengths
        self.k = model.kk[learners, 0]
        self.k4 = model.kk4[learners, 0]
        self.moved = torch.zeros(len(learners), dtype=torch.bool)  # rows past their first step
        self.gen = torch.Generator().manual_seed(4000 + seed)
        self.inh_levels = lev * model.src_inh                       # kept as born
        self.wired = (self.C > 0) & self.valid
        self.strong = (self.C >= 4) & self.valid
        self.steps = 0.0

    def full(self, v):
        """a compact row's values in the full columns (0 where not usable)"""
        return torch.zeros(len(self.L), self.N, dtype=v.dtype).scatter_(1, self.idx, v * self.valid)

    @property
    def use(self):
        return self.full(self.valid)

    def forbid(self, rows, cols):
        """candidates a learner may never wire (family 2's own target pixel)"""
        self.valid[rows] &= self.idx[rows] != cols.unsqueeze(1)

    def wired_full(self):
        return self.full(self.wired)

    def rewire(self, model):
        """the top counters at the born counts; ties: wired first, then strong, then chance.
        a row that has not moved yet keeps exactly its born wiring (its born silent sensor
        inputs are not in the model's columns and hold their slots until the first move)"""
        key = self.C + 0.4 * self.wired + 0.2 * self.strong + 0.1 * torch.rand(
            self.C.shape, generator=self.gen)
        key = key.masked_fill(~self.valid, float("-inf"))
        kmax = int(self.k.max())
        val, top = key.topk(kmax, dim=1)
        r = torch.arange(kmax).unsqueeze(0)
        lev = torch.where(r < self.k4.unsqueeze(1), 4.0,
                          torch.where(r < self.k.unsqueeze(1), 1.0, 0.0))
        floor = torch.where(self.moved, float("-inf"), 0.5).unsqueeze(1)
        lev = lev * (val > floor) * (val > float("-inf"))
        exc = torch.zeros_like(self.C).scatter_(1, top, lev)
        self.wired, self.strong = exc > 0, exc >= 4
        model.levels[self.L] = self.inh_levels + self.full(exc)

    def apply(self, d, churn=False):
        """expected steps d (compact) rounded by chance to whole steps; with churn, the same
        number of steps up and down land on candidates drawn at random"""
        step = torch.floor(d + torch.rand(d.shape, generator=self.gen)) * self.valid
        if churn:
            step = self.churn(step)
        self.steps += float(step.abs().sum())
        self.moved |= (step != 0).any(1)
        self.C = (self.C + step).clamp_(-TOP, TOP)

    def churn(self, step):
        """as many steps up and down per cell, toward candidates drawn at random (up: any
        usable source; down: usable sources whose counter is above zero)"""
        out = torch.zeros_like(step)
        for sign, pool in ((1.0, self.valid), (-1.0, self.valid & (self.C > 0))):
            n = (step * sign).clamp(min=0).sum(1).long()
            rows = torch.nonzero((n > 0) & pool.any(1)).flatten()
            if len(rows) == 0:
                continue
            m = int(n[rows].max())
            pick = torch.multinomial(pool[rows].float(), m, replacement=True, generator=self.gen)
            keep = (torch.arange(m).unsqueeze(0) < n[rows].unsqueeze(1)).float()
            out[rows] = out[rows].scatter_add(1, pick, sign * keep)
        return out


def hebb(R, A, S):
    """per learner and usable candidate: the sum over digits of moves x source active"""
    return (R.t() @ A).gather(1, S.idx)


def moves(c, shift):
    """a cell moves with chance 2^-shift a spike: c 2^-shift moves a digit (at most one)"""
    return (c * 2.0 ** -shift).clamp(max=1.0)


def compete_moves(R, A, S, down=0.25):
    """R moves a digit per cell: up for the active sources, down by chance for silent ones
    whose counter is above zero"""
    up = hebb(R, A, S)
    silent = R.sum(0).unsqueeze(1) - up
    return up - down * silent * (S.C > 0)


def compete(c, A, S, shift):
    """family 0, the engine's competitive learning: a cell that fires moves (c 2^-shift times
    a digit); each move steps up every source active on the spike and steps down, with chance
    1/4, each silent source whose counter is above zero"""
    return compete_moves(moves(c, shift), A, S)


def real_or_made(c_real, A_real, c_fake, A_fake, S, shift, gate=None):
    """family 1, a local "is this real": a broadcast bit says whether the digit is real. on a
    real digit a cell that fires moves as in competitive learning (toward what is on); on a
    made-up one a cell that fires steps its active sources down. gate (optional, per digit and
    cell, real and made-up): only when its tile is not yet sure (a real digit that drew no
    more than the tile's running level of spikes, a made-up one that drew no less)"""
    Rr, Rf = moves(c_real, shift), moves(c_fake, shift)
    if gate is not None:
        Rr, Rf = Rr * gate[0], Rf * gate[1]
    return compete_moves(Rr, A_real, S) - hebb(Rf, A_fake, S)


def predict_pixel(c, A, y, has, rate, S):
    """family 2, predict what is just outside: each cell has a target pixel in its reach that
    it may not wire. its spike is its guess that the pixel is on. guessed off and it was on:
    the sources active on the digit step up; guessed on and it was off: they step down. right:
    nothing. (a delta rule on the counters, chance `rate` a wrong guess)"""
    err = (y - (c > 0).float()) * has
    return rate * hebb(err, A, S)


def two_looks(cA, AA, cB, AB, S, shift):
    """family 3, two looks agree (a trace of the cell's own last activity): the same digit is
    shown twice, the second look moved by a pixel and a little noisy. a cell that fired on one
    look moves toward the sources active on the other (up for those, down by chance for the
    silent ones with counters above zero), so it comes to answer both looks the same; which
    digits it answers stays the competition's business, as in family 0"""
    return compete_moves(moves(cA, shift), AB, S) + compete_moves(moves(cB, shift), AA, S)


def push_apart(c, A, S, tile_of, tiles, shift, rate):
    """family 4, push apart (anti-Hebbian in the tile): a source that is active and wired to
    two or more cells of a tile that fired on the digit is shared; each of those cells steps
    its counter for it down (chance `rate` a fired move). tile_of: each learner's tile"""
    F = (c > 0).float()
    R = moves(c, shift)
    W = S.wired_full().float()
    d = torch.zeros_like(S.C)
    for t in range(tiles):
        rows = torch.nonzero(tile_of == t).flatten()
        if len(rows) < 2:
            continue
        # how many fired cells of the tile wire each source, per digit
        G = F[:, rows] @ W[rows]                       # B x N
        shared = (G >= 2).float() * A                  # active and shared on the digit
        d[rows] = -rate * (R[:, rows].t() @ shared).gather(1, S.idx[rows]) * S.wired[rows]
    return d


def bcm(c, A, S, avg, shift):
    """family 5, a sliding threshold (BCM): a cell keeps a running mean of its count squared
    (avg; BCM's threshold). on a digit where it fires more than that, it moves toward the
    active sources as in family 0; where it fires but no more, it steps the active sources
    down. cells grow selective: a strong answer to a few digits, not a middling one to many"""
    R = moves(c, shift)
    above = (c > avg.unsqueeze(0)).float()
    below = ((c > 0) & (c <= avg.unsqueeze(0))).float()
    return compete_moves(R * above, A, S) - hebb(R * below, A, S)
