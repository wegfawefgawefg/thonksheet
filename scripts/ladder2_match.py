"""The inside's ladder: the model's cells against the real sheet's, digit by digit.

`blobmnist ceiling --rows-out F` writes every cell's spike count for every digit it was shown
(and which digit, and on the pair test which partner). Here the model is given the same
wiring (the born sheet's, or a transplant's) with thresholds set by regulation only, shown
the same digits, and compared: the fresh reader on each, fit on the same training digits; how
well a cell's count in the model predicts its count in the sheet; and, from a second run of
the sheet on the same digits from another starting state, how well the sheet predicts itself.

  ladder2.py match TASK SEED SHEET WIRING|born ROWS [ROWS2]
"""

import numpy as np
import torch

import ladder2_data as D
import ladder2_model as M
import ladder2_train as T


def rows_read(path):
    out = []
    with open(path, "rb") as f:
        for _ in range(2):
            n, d = map(int, f.readline().split()[1:])
            raw = np.frombuffer(f.read(n * (d + 5)), np.uint8).reshape(n, d + 5)
            image = raw[:, 0].astype(np.int64) + 256 * raw[:, 1]
            partner = raw[:, 2].astype(np.int64) + 256 * raw[:, 3]
            out.append((torch.from_numpy(raw[:, 5:].astype(np.float32)),
                        torch.from_numpy(raw[:, 4].astype(np.int64)), image, partner))
    return out


def wiring_levels_from(path, sheet):
    L = np.zeros((sheet.H, sheet.P + sheet.H), np.float32)
    row = {n: u for u, n in enumerate(sheet.cells)}
    for line in open(path):
        p = line.split()
        if not p or p[0] == "t":
            continue
        u = row[int(p[0])]
        for item in p[1:]:
            s, w = map(int, item.split(":"))
            if sheet.col[s] >= 0:
                L[u, sheet.col[s]] = w
    return torch.from_numpy(L)


def bits(task, images, partners, test, val):
    tr, _, te, _ = D.mnist_raw()
    src = te if test else tr
    if task == "pair":
        h = D.half_bits(src)
        return torch.from_numpy(D.pair_bits(h[images], h[partners])).float()
    return torch.from_numpy((src[images] >= 128).astype(np.uint8)).float()


def cell_corr(a, b):
    a = a - a.mean(0)
    b = b - b.mean(0)
    den = torch.sqrt((a * a).sum(0) * (b * b).sum(0))
    keep = den > 1e-6
    return float(((a * b).sum(0)[keep] / den[keep]).mean())


def match(task, seed, sheet_path, wiring, rows_path, rows2_path=None, val=False):
    torch.set_num_threads(1)
    sheet = M.SheetInfo(sheet_path)
    (ftr, ytr, itr, ptr), (fte, yte, ite, pte) = rows_read(rows_path)
    classes = 2 if task == "pair" else 10
    model = M.Ladder("7b", sheet, classes, seed)
    if wiring == "born":
        model.born_wiring(sheet)
        model.rewire()
    else:
        model.levels = wiring_levels_from(wiring, sheet)
    model.rewire = lambda: None
    xtr = bits(task, itr, ptr, False, val)
    xte = bits(task, ite, pte, True, val)
    T.init_thresholds(model, xtr[:3000])
    gen = torch.Generator().manual_seed(seed)
    with torch.no_grad():
        for idx in D.batches(len(xtr), 256, gen):
            model.regulate(model(xtr[idx]), rate=0.1)
    mtr, mte = T.features(model, xtr), T.features(model, xte)
    out = {"sheet_reader": D.reader(ftr, ytr, fte, yte, classes, seed),
           "model_reader": D.reader(mtr, ytr, mte, yte, classes, seed),
           "cell_corr_model_sheet": cell_corr(mte, fte),
           "sheet_mean": float(fte.mean()), "model_mean": float(mte.mean()),
           "sheet_active": float((fte > 0).float().mean()),
           "model_active": float((mte > 0).float().mean())}
    if rows2_path:
        (_, _, _, _), (f2, _, i2, _) = rows_read(rows2_path)
        if np.array_equal(i2, ite):
            out["cell_corr_sheet_sheet"] = cell_corr(f2, fte)
            out["sheet_reader_run2"] = D.reader(*rows_read(rows2_path)[0][:2],
                                                f2, yte, classes, seed)
    return out
