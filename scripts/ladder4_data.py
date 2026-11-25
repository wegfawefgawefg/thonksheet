"""Label-free rule families in the model (idea 173): what the life is shown.

docs/results-2026-11-26-label-free.md; scripts/ladder4.sh runs it. The digits, the pair test
and the reader are idea 165's (ladder2_data.py, imported). This file adds what the label-free
rules need: the life's stream of digits (one pass, in an order drawn by the seed), the
shuffled-digit control (a fresh permutation of the pixels for every digit), made-up digits
for the "is this real" family, a second look at a digit for the "two looks" family, test
digits moved by 1 and 2 pixels, and a target pixel per cell for the "predict outside" family.
"""

import numpy as np
import torch

import ladder2_data as D


def life_stream(task, n, seed):
    """the first n digits of the life, as bits (n x 784); the pair test pairs afresh"""
    x, _ = task.train_set()
    g = torch.Generator().manual_seed(7000 + seed)
    order = torch.randperm(len(x), generator=g)
    reps = (n + len(x) - 1) // len(x)
    order = torch.cat([order] * reps)[:n]
    return x[order]


def shuffled(x, gen):
    """every digit's pixels moved by a permutation drawn for that digit: the same ink, no
    shape that comes back (the control that exposed competitive learning)"""
    perm = torch.rand(x.shape, generator=gen).argsort(dim=1)
    return torch.gather(x, 1, perm), perm


def moved(x, dx, dy):
    """digits moved by (dx, dy) pixels, what falls off the edge lost (as blobmnist --shift)"""
    img = x.view(-1, 28, 28)
    out = torch.zeros_like(img)
    ys = slice(max(dy, 0), 28 + min(dy, 0))
    xs = slice(max(dx, 0), 28 + min(dx, 0))
    ys0 = slice(max(-dy, 0), 28 + min(-dy, 0))
    xs0 = slice(max(-dx, 0), 28 + min(-dx, 0))
    out[:, ys, xs] = img[:, ys0, xs0]
    return out.reshape(-1, 784)


DIRS = [(1, 0), (-1, 0), (0, 1), (0, -1)]


def moved_sets(x):
    """the test digits moved 1 and 2 pixels, each in the 4 directions"""
    return {by: [moved(x, dx * by, dy * by) for dx, dy in DIRS] for by in (1, 2)}


def made_up(x, gen):
    """a made-up digit for each real one, a third each kind: the pixels shuffled within the
    digit; two digits spliced (top half of this one, bottom half of another); this digit's
    halves swapped (top to bottom)"""
    n = len(x)
    kind = torch.randint(0, 3, (n,), generator=gen)
    out = x.clone().view(n, 28, 28)
    sh, _ = shuffled(x, gen)
    other = x[torch.randperm(n, generator=gen)].view(n, 28, 28)
    img = x.view(n, 28, 28)
    spliced = torch.cat([img[:, :14], other[:, 14:]], dim=1)
    swapped = torch.cat([img[:, 14:], img[:, :14]], dim=1)
    out[kind == 0] = sh.view(n, 28, 28)[kind == 0]
    out[kind == 1] = spliced[kind == 1]
    out[kind == 2] = swapped[kind == 2]
    return out.reshape(n, 784)


def second_look(x, gen, noise=0.02):
    """the same digit again, moved one pixel in a direction drawn per digit, and a share of
    its pixels flipped"""
    n = len(x)
    d = torch.randint(0, 4, (n,), generator=gen)
    out = torch.empty_like(x)
    for k, (dx, dy) in enumerate(DIRS):
        sel = d == k
        if sel.any():
            out[sel] = moved(x[sel], dx, dy)
    flip = (torch.rand(x.shape, generator=gen) < noise).float()
    return (out + flip) % 2


def target_pixels(sheet, usable, x, seed, lo=0.1, hi=0.6):
    """a target pixel for each cell: drawn among the pixels in its reach that it was not born
    wired to and that are on in lo..hi of the digits; -1 if there is none"""
    rate = x.mean(0).numpy()
    rng = np.random.default_rng(9000 + seed)
    born = sheet.born[:, :784] > 0
    out = np.full(sheet.H, -1, np.int64)
    for u in range(sheet.H):
        ok = np.flatnonzero(usable[u, :784] & ~born[u] & (rate >= lo) & (rate <= hi))
        if len(ok):
            out[u] = ok[rng.integers(0, len(ok))]
    return torch.from_numpy(out)


def pixel_likeness(levels, x):
    """for cells with 2 or more pixel inputs: the mean correlation, over pairs, of the pixels a
    cell listens to, on real digits (the compete doc's measure); and their spread in pixels"""
    px = (levels[:, :784] > 0).numpy()
    xs = x[:10000].numpy()
    xc = xs - xs.mean(0)
    sd = np.sqrt((xc ** 2).mean(0)) + 1e-9
    corr = (xc.T @ xc) / len(xs) / np.outer(sd, sd)
    ys, xx = np.divmod(np.arange(784), 28)
    like, spread = [], []
    for u in range(len(px)):
        p = np.flatnonzero(px[u])
        if len(p) >= 2:
            c = corr[np.ix_(p, p)]
            like.append((c.sum() - np.trace(c)) / (len(p) * (len(p) - 1)))
            spread.append(np.mean(np.hypot(xx[p] - xx[p].mean(), ys[p] - ys[p].mean())))
    return (float(np.mean(like)) if like else 0.0, float(np.mean(spread)) if spread else 0.0)

