"""The inside's ladder: data, the sheet as born, and the fresh linear reader.

docs/results-2026-11-25-inside-ladder.md; scripts/ladder2.sh runs it. Backprop here is a
measuring stick, not the method: the project's method stays local rules on bits.

MNIST bits exactly as `blobmnist` shows them (src/world/mnist_data.cpp): t28 is every pixel
on at half ink; the pair test (`--pair 2 --pair-mix 1`) is two half-size digits (2x2
averaged, on at half ink) interleaved, the first on even columns, the second on odd ones,
each half-size row shown twice, answer 1 if the two are the same class. Splits as the tree:
test runs train on training digits 0-59,999 and score the 10,000 test digits; validation
runs train on 0-49,999 and score 50,000-59,999.
"""

import os

import numpy as np
import torch

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def idx_read(path):
    raw = np.fromfile(path, dtype=np.uint8)
    dims = raw[3]
    shape = [int.from_bytes(raw[4 + 4 * i:8 + 4 * i].tobytes(), "big") for i in range(dims)]
    return raw[4 + 4 * dims:].reshape(shape)


def mnist_raw():
    d = os.environ.get("THONK_MNIST", os.path.join(ROOT, "data", "mnist"))
    tr = idx_read(os.path.join(d, "train-images-idx3-ubyte")).reshape(-1, 784)
    trl = idx_read(os.path.join(d, "train-labels-idx1-ubyte"))
    te = idx_read(os.path.join(d, "t10k-images-idx3-ubyte")).reshape(-1, 784)
    tel = idx_read(os.path.join(d, "t10k-labels-idx1-ubyte"))
    return tr, trl.astype(np.int64), te, tel.astype(np.int64)


def half_bits(images):
    """each digit at half size: the 2x2 block's sum against half ink, 14 x 14"""
    x = images.reshape(-1, 14, 2, 14, 2).astype(np.int32).sum(axis=(2, 4))
    return (x >= 512).astype(np.uint8)


def pair_bits(half_a, half_b):
    """interleaved (mix 1): first digit on even columns, second on odd, rows twice"""
    n = half_a.shape[0]
    out = np.zeros((n, 28, 28), np.uint8)
    for r in range(2):
        out[:, r::2, 0::2] = half_a
        out[:, r::2, 1::2] = half_b
    return out.reshape(n, 784)


def partners(labels, rng):
    """for each digit a second one: half the time of the same class, half another"""
    n = len(labels)
    want = rng.integers(0, 2, n).astype(bool)
    by_class = [np.flatnonzero(labels == c) for c in range(10)]
    out = np.empty(n, np.int64)
    for c in range(10):
        mine = np.flatnonzero(labels == c)
        same = mine[want[mine]]
        other = mine[~want[mine]]
        pool = by_class[c]
        out[same] = pool[rng.integers(0, len(pool), len(same))]
        rest = np.concatenate([by_class[k] for k in range(10) if k != c])
        out[other] = rest[rng.integers(0, len(rest), len(other))]
    # never a digit with itself: draw again from its own class
    hit = np.flatnonzero(out == np.arange(n))
    while len(hit):
        for i in hit:
            pool = by_class[labels[i]]
            out[i] = pool[rng.integers(0, len(pool))]
        hit = np.flatnonzero(out == np.arange(n))
    return out


class Task:
    """one test: training bits (fresh pairs each pass for the pair test) and scored bits"""

    def __init__(self, name, val, seed, limit=0):
        tr, trl, te, tel = mnist_raw()
        if val:
            te, tel = tr[50000:60000], trl[50000:60000]
            tr, trl = tr[:50000], trl[:50000]
        if limit:  # quick runs: the first `limit` training digits
            tr, trl = tr[:limit], trl[:limit]
        self.name, self.pair = name, name == "pair"
        self.classes = 2 if self.pair else 10
        if self.pair:
            self.half_tr, self.trl = half_bits(tr), trl
            rng = np.random.default_rng(1000 + seed)
            half_te = half_bits(te)
            j = partners(tel, rng)
            self.x_te = torch.from_numpy(pair_bits(half_te, half_te[j])).float()
            self.y_te = torch.from_numpy((tel == tel[j]).astype(np.int64))
            self.rng = np.random.default_rng(2000 + seed)
        else:
            self.x_tr = torch.from_numpy((tr >= 128).astype(np.uint8)).float()
            self.y_tr = torch.from_numpy(trl)
            self.x_te = torch.from_numpy((te >= 128).astype(np.uint8)).float()
            self.y_te = torch.from_numpy(tel)

    def train_set(self):
        """the training bits for one pass (the pair test pairs the digits afresh)"""
        if not self.pair:
            return self.x_tr, self.y_tr
        j = partners(self.trl, self.rng)
        x = pair_bits(self.half_tr, self.half_tr[j])
        return torch.from_numpy(x).float(), torch.from_numpy((self.trl == self.trl[j]).astype(np.int64))


def batches(n, size, gen):
    order = torch.randperm(n, generator=gen)
    for i in range(0, n, size):
        yield order[i:i + size]


def reader(f_tr, y_tr, f_te, y_te, classes, seed, epochs=10):
    """a fresh linear reader: softmax regression on features centred and scaled as the
    tree's reader does (scale mode 2: by the feature's spread softened by the mean spread;
    a feature that never varies is dropped), Adam, batches of 256. test accuracy"""
    f_tr, f_te = f_tr.float(), f_te.float()
    mean = f_tr.mean(0)
    var = f_tr.var(0, unbiased=False)
    inv = torch.where(var > 1e-6, 1.0 / torch.sqrt(var + var.mean()), torch.zeros_like(var))
    gen = torch.Generator().manual_seed(seed)
    lin = torch.nn.Linear(f_tr.shape[1], classes)
    torch.nn.init.zeros_(lin.weight)
    torch.nn.init.zeros_(lin.bias)
    opt = torch.optim.Adam(lin.parameters(), lr=1e-3)
    sched = torch.optim.lr_scheduler.LambdaLR(opt, lambda e: 1.0 / (1 + e))
    for _ in range(epochs):
        for idx in batches(len(y_tr), 256, gen):
            loss = torch.nn.functional.cross_entropy(lin((f_tr[idx] - mean) * inv), y_tr[idx])
            opt.zero_grad()
            loss.backward()
            opt.step()
        sched.step()
    with torch.no_grad():
        right = 0
        for i in range(0, len(y_te), 2000):
            right += (lin((f_te[i:i + 2000] - mean) * inv).argmax(1) == y_te[i:i + 2000]).sum()
    return float(right) / len(y_te)


def rows_read(path):
    """the rows `blobmnist ceiling --rows-out` wrote: (train counts, labels), (test ...)"""
    out = []
    with open(path, "rb") as f:
        for _ in range(2):
            n, d = map(int, f.readline().split()[1:])
            raw = np.frombuffer(f.read(n * (d + 1)), np.uint8).reshape(n, d + 1)
            out.append((torch.from_numpy(raw[:, 1:].astype(np.float32)),
                        torch.from_numpy(raw[:, 0].astype(np.int64))))
    return out
