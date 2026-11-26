#!/usr/bin/env python3
"""The README's plots, into docs/img/. The numbers are in scripts/figures_data.py, each with
the doc table or the archived run it was copied from. Needs python3 with matplotlib only.

  scripts/figures.py                 the four plots that need no run
  scripts/figures.py ladder          docs/img/rule-ladder.png
  scripts/figures.py headline        docs/img/headline.png
  scripts/figures.py life            docs/img/own-answer-life.png
  scripts/figures.py map             docs/img/task-map.png
  scripts/figures.py cells           docs/img/hidden-cells.png: runs two MNIST lives first
                                     (seed 5, born and closed loop, under a minute, nice 19;
                                     needs build/blobmnist and data/mnist), kept in
                                     out/figures/cells/
  scripts/figures.py shrink A.png .. a PNG down to 256 colours in place (scripts/captures.sh)
"""

import os
import subprocess
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402
from matplotlib.colors import LinearSegmentedColormap  # noqa: E402
from matplotlib.lines import Line2D  # noqa: E402
from matplotlib.patches import Patch, Rectangle  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import figures_data as data  # noqa: E402

IMG = os.path.join(ROOT, "docs", "img")
INK, MUTED, FAINT = "#0b0b0b", "#52514e", "#e4e3df"
BLUE, ORANGE, GREY = "#2a78d6", "#eb6834", "#a3a29c"
plt.rcParams.update({
    "font.family": "DejaVu Sans", "font.size": 9.5, "text.color": INK,
    "axes.edgecolor": FAINT, "axes.labelcolor": MUTED, "xtick.color": MUTED,
    "ytick.color": MUTED, "axes.spines.top": False, "axes.spines.right": False,
    "figure.facecolor": "white", "axes.facecolor": "white", "savefig.facecolor": "white",
})


def save(fig, name):
    os.makedirs(IMG, exist_ok=True)
    path = os.path.join(IMG, name)
    fig.savefig(path, dpi=130)
    plt.close(fig)
    print("wrote docs/img/%s (%d KB)" % (name, os.path.getsize(path) // 1024))


def heading(fig, title, sub, left=0.02):
    fig.text(left, 0.965, title, fontsize=13.5, fontweight="bold", va="top")
    fig.text(left, 0.965 - 0.36 / fig.get_figheight(), sub, fontsize=9.5, color=MUTED, va="top")


def note(fig, text, left=0.02):
    fig.text(left, 0.015, text, fontsize=8.2, color=MUTED, va="bottom", linespacing=1.4)


def from_born(ax, rows, col, born, lim, title, colours):
    """one bar a row from the born level to the row's score; ticks for its controls"""
    ys = np.arange(len(rows))[::-1]
    for y, row, colour in zip(ys, rows, colours):
        v = row["v"][col]
        ax.add_patch(Rectangle((born, y - 0.29), v - born, 0.58, color=colour, lw=0))
        ax.text(v + (lim[1] - lim[0]) * 0.012, y, "%.3f" % v, va="center", fontsize=8.8)
        if row.get("coin"):
            ax.plot([row["coin"][col]], [y], marker="|", ms=13, mew=1.8, color=INK)
        if row.get("shuffled"):
            ax.plot([row["shuffled"][col]], [y], marker="o", ms=5.5, mew=1.3, mfc="white",
                    mec=INK)
    ax.axvline(born, color=INK, lw=1)
    ax.set_xlim(*lim)
    ax.set_ylim(-0.6, len(rows) - 0.4)
    ax.set_yticks([])
    ax.spines["left"].set_visible(False)
    ax.grid(axis="x", color=FAINT, lw=0.6)
    ax.set_axisbelow(True)
    ax.tick_params(length=0, labelsize=8.5)
    ax.set_title(title, fontsize=10.5, loc="left", pad=6)
    return ys


def row_labels(fig, ax, ys, rows, x):
    for y, row in zip(ys, rows):
        fig.text(x, ax.transData.transform((0, y))[1] / fig.bbox.height, row["name"],
                 va="center", fontsize=9.3)


def ladder():
    rows = data.LADDER
    fig = plt.figure(figsize=(11.2, 5.6))
    heading(fig, "From backprop down to a rule in bits",
            "In a model of the sheet's cells the teacher's error is cut down one step at a "
            "time; each rung adds to the ones above it. Score: a fresh linear reader, share "
            "right.")
    colours = [ORANGE if r["name"].startswith("T7 ") else BLUE for r in rows]
    lims = [(0.932, 0.986), (0.752, 0.874)]
    for col, (name, left) in enumerate([("MNIST", 0.39), ("pair test", 0.70)]):
        ax = fig.add_axes([left, 0.225, 0.27, 0.575])
        ys = from_born(ax, rows, col, data.LADDER_BORN[col], lims[col],
                       "%s (born arm %.3f)" % (name, data.LADDER_BORN[col]), colours)
        if col == 0:
            fig.canvas.draw()
            row_labels(fig, ax, ys, rows, 0.02)
    fig.legend(handles=[
        Patch(color=BLUE, label="gain over the born arm (bars start at the born arm)"),
        Patch(color=ORANGE, label="the counter step with a small chance: most of the loss"),
        Line2D([], [], marker="|", ms=11, mew=1.8, color=INK, ls="", label="a rung's coin control"),
    ], loc="lower left", bbox_to_anchor=(0.012, 0.105), ncol=3, frameon=False, fontsize=8.6,
        handlelength=1.4, columnspacing=1.8)
    note(fig, "A model of the sheet's 3,072 non-sensor cells with every constraint of the "
         "engine; test digits, mean of 3 seeds (T0: 8). Coin control: the cells get another "
         "digit's errors.\nWritten into the real sheet the two bottom wirings read 0.948 / "
         "0.723 and 0.961 / 0.761 (born 0.924 / 0.673). Backprop is the measuring stick here, "
         "never the method.\nSide arms left out. Numbers: "
         "docs/results-2026-11-25-rule-ladder.md")
    save(fig, "rule-ladder.png")


def headline():
    rows = data.HEADLINE
    fig = plt.figure(figsize=(11.2, 4.5))
    heading(fig, "What the inside learns from a 60,000-digit life",
            "A fresh linear reader of the real sheet's non-sensor cells after the life, share "
            "right on the test digits, mean of 8 seeds.")
    colours = [BLUE if "closed loop" in r["name"] else GREY for r in rows]
    lims = [(0.917, 0.974), (0.655, 0.802)]
    for col, (name, left) in enumerate([("MNIST", 0.39), ("pair test", 0.70)]):
        ax = fig.add_axes([left, 0.275, 0.27, 0.49])
        ys = from_born(ax, rows, col, data.HEADLINE_BORN[col], lims[col],
                       "%s (born sheet %.3f)" % (name, data.HEADLINE_BORN[col]), colours)
        if col == 0:
            fig.canvas.draw()
            row_labels(fig, ax, ys, rows, 0.02)
    fig.legend(handles=[
        Patch(color=BLUE, label="the closed loop: gain over the born sheet"),
        Patch(color=GREY, label="the others: the same, in grey"),
        Line2D([], [], marker="|", ms=11, mew=1.8, color=INK, ls="",
               label="coin control (the cells get another verdict's errors)"),
        Line2D([], [], marker="o", ms=5.5, mew=1.3, mfc="white", mec=INK, ls="",
               label="shuffled-digit life"),
    ], loc="lower left", bbox_to_anchor=(0.012, 0.115), ncol=4, frameon=False, fontsize=8.6,
        handlelength=1.4, columnspacing=1.6)
    note(fig, "The closed loop must be told the right answer; all of this is one family of "
         "data (MNIST digits). The old rule's pair number is 4 seeds. Competitive learning "
         "fires at about twice\nthe born rate; with the rate held it reads 0.935 / 0.704. The "
         "second setting has no shuffled life. Backprop is the measuring stick. Numbers: "
         "docs/results-2026-11-26-output-inside.md")
    save(fig, "headline.png")


def life():
    fig = plt.figure(figsize=(8.6, 5.0))
    heading(fig, "The blob's own answer during a 60,000-digit MNIST life",
            "Share right in each 6,000 digits. Every digit is seen once and answered before "
            "the output is taught.", left=0.03)
    ax = fig.add_axes([0.075, 0.235, 0.60, 0.575])
    x = np.arange(6, 61, 6)
    # the labels' heights: the two upper lines end 0.02 apart, so their labels are spread
    for (name, mean, low, high), colour, at in zip(data.LIFE, [BLUE, GREY, ORANGE],
                                                   [0.955, 0.875, 0.68]):
        ax.fill_between(x, low, high, color=colour, alpha=0.18, lw=0)
        ax.plot(x, mean, color=colour, lw=2, marker="o", ms=4.5, mec="white", mew=0.8)
        ax.plot([60.6, 62.4], [mean[-1], at], color=colour, lw=1, clip_on=False)
        ax.text(63, at, "%.3f" % mean[-1], va="center", fontsize=9, fontweight="bold")
        ax.text(68.3, at, name, va="center", fontsize=8.8, linespacing=1.25)
    ax.set_xlim(3, 61)
    ax.set_ylim(0.5, 1.0)
    ax.set_xticks(x)
    ax.set_yticks([0.5, 0.6, 0.7, 0.8, 0.9, 1.0])
    ax.grid(axis="y", color=FAINT, lw=0.6)
    ax.set_axisbelow(True)
    ax.tick_params(length=0, labelsize=8.5)
    ax.set_xlabel("digits seen, thousands")
    ax.set_ylabel("share right (the axis starts at 0.5; chance is 0.10)")
    note(fig, "Lines: mean over seeds (8 for the upper two, 3 for the old readout); bands: "
         "lowest to highest seed. The blob's own output is told the\nright line; the old "
         "readout is told good or bad. From the life logs behind "
         "docs/results-2026-11-26-output-inside.md and\n"
         "docs/results-2026-11-26-every-event.md", left=0.03)
    save(fig, "own-answer-life.png")


def task_map():
    cols, rows = data.MAP_COLUMNS, data.MAP_ROWS
    for name, sheet, scores, passed in cols:
        assert len(scores) == len(rows) and sum(s.endswith("*") for s in scores) == passed, name
    fig = plt.figure(figsize=(9.6, 8.6))
    heading(fig, "The task map at tabling: the quick map of 22 worlds",
            "Late score of a 20,000-step life, mean of 8 seeds. Filled: the row is passed. One "
            "setting per column, unchanged on every world.")
    ax = fig.add_axes([0.25, 0.095, 0.73, 0.765])
    n = len(rows)
    for c, (name, sheet, scores, passed) in enumerate(cols):
        x = c + (0.25 if not sheet else 0)
        ax.text(x + 0.5, -0.25, name, ha="center", va="bottom", fontsize=8.6, linespacing=1.2,
                fontweight="bold" if sheet else "normal")
        for r, s in enumerate(scores):
            won = s.endswith("*")
            fill = (BLUE if sheet else "#7d7c76") if won else "#f3f2ef"
            ax.add_patch(Rectangle((x + 0.03, r + 0.06), 0.94, 0.88, color=fill, lw=0))
            ax.text(x + 0.5, r + 0.5, s.rstrip("*"), ha="center", va="center", fontsize=8.6,
                    color="white" if won else MUTED, fontweight="bold" if won else "normal")
        ax.text(x + 0.5, n + 0.55, "%d of %d" % (passed, n), ha="center", va="center",
                fontsize=10.5, fontweight="bold")
    for r, name in enumerate(rows):
        ax.text(-0.12, r + 0.5, name, ha="right", va="center", fontsize=9)
    ax.text(-0.12, n + 0.55, "rows passed", ha="right", va="center", fontsize=9.5,
            fontweight="bold")
    ax.text(2.0, -1.95, "the sheet (local rules on bits)", ha="center", fontsize=9, color=MUTED)
    ax.text(5.75, -1.95, "references", ha="center", fontsize=9, color=MUTED)
    ax.set_xlim(0, len(cols) + 0.25)
    ax.set_ylim(n + 1.1, -2.2)
    ax.axis("off")
    note(fig, "A row passes when its late score is at least halfway from chance to perfect and "
         "above its own learning-off twin by more than 2 standard\nerrors. Cartpole is steps "
         "per fall (a coin gets 21). The GRU is trained by backprop. Numbers: "
         "docs/results-2026-11-26-one-setting.md\nand docs/results-2026-11-26-one-setting-2.md")
    save(fig, "task-map.png")


CELLS_DIR = os.path.join(ROOT, "out", "figures", "cells")


def mnist_dir():
    """where the engine looks for MNIST: $THONK_MNIST, else data/mnist"""
    path = os.environ.get("THONK_MNIST") or os.path.join(ROOT, "data", "mnist")
    if not os.path.exists(os.path.join(path, "t10k-images-idx3-ubyte")):
        sys.exit("figures: no MNIST in %s; run scripts/get_mnist.sh" % path)
    return path


def cell_runs():
    """the two sheets of the cells picture, seed 5: as born, and after a closed-loop life
    (scripts/outputinside.py's arm intope); each writes the reader's rows of spike counts"""
    os.makedirs(CELLS_DIR, exist_ok=True)
    if not os.path.exists(os.path.join(ROOT, "build", "blobmnist")):
        sys.exit("figures: no build/blobmnist; run scripts/build.sh")
    sheet = ("--mnist-code t28 --g.sensor_w 64 --g.sensor_h 64 --g.motor_x 1 --g.motor_y 1 "
             "--g.motor_w 64 --g.motor_h 64 --motor-spread 102 --g.local_connect_shift 4 "
             "--scale 2 --passes 10 --lr-1000 1 --views 1 --fit 2000 --seed 5")
    loop = ("--life 60000 --g.learn_mode 5 --life-teach 6 --g.hop_on 1 --g.hop_bits 12 "
            "--g.hop_shift 0 --g.out_on 1 --g.out_top 1 --motor-spread-even 1")
    runs = {"born": "--describe %s" % os.path.join(CELLS_DIR, "born-sheet.txt"), "loop": loop}
    for name, flags in runs.items():
        rows = os.path.join(CELLS_DIR, name + ".rows")
        if os.path.exists(rows):
            continue
        cmd = "nice -n 19 build/blobmnist ceiling %s %s --rows-out %s" % (sheet, flags, rows)
        print(cmd, flush=True)
        with open(os.path.join(CELLS_DIR, name + ".log"), "w") as log:
            subprocess.run(cmd.split(), cwd=ROOT, stdout=log, check=True)


def read_rows(path):
    """the test digits' rows of a --rows-out file: each digit's index, the cells' counts"""
    with open(path, "rb") as f:
        for part in range(2):  # train rows, then test rows
            head = f.readline().split()
            n, d = int(head[1]), int(head[2])
            rows = np.frombuffer(f.read(n * (d + 5)), dtype=np.uint8).reshape(n, d + 5)
    return rows[:, 0].astype(int) | (rows[:, 1].astype(int) << 8), rows[:, 5:]


def cells():
    mnist = mnist_dir()
    cell_runs()
    # the sheet as born: kind bits (1 sensor), the pixel a sensor cell stands for, the sources
    kind, pixel, sources = {}, {}, {}
    with open(os.path.join(CELLS_DIR, "born-sheet.txt")) as f:
        f.readline()
        for line in f:
            p = line.split()
            n = int(p[0])
            kind[n], pixel[n] = int(p[3]), int(p[4])
            sources[n] = [int(s.split(":")[0]) for s in p[10:]]
    inner = [n for n in sorted(kind) if not kind[n] & 1]  # the rows' columns, in this order
    born_pixels = {n: [pixel[s] for s in sources[n] if kind[s] & 1 and pixel[s] >= 0]
                   for n in inner}
    # ten hidden excitatory cells with 2 or more pixel inputs as born, evenly through the list
    wanted = [i for i, n in enumerate(inner) if kind[n] == 0 and len(born_pixels[n]) >= 2]
    picked = [wanted[k * len(wanted) // 10 + len(wanted) // 20] for k in range(10)]
    digits = np.fromfile(os.path.join(mnist, "t10k-images-idx3-ubyte"),
                         dtype=np.uint8, offset=16).reshape(-1, 784).astype(np.float32) / 255
    cmap = LinearSegmentedColormap.from_list("lessmore", [BLUE, "#f6f5f2", ORANGE])
    fig = plt.figure(figsize=(11.2, 3.75))
    heading(fig, "What a hidden cell fires for, as born and after a closed-loop life",
            "Ten hidden excitatory cells of one sheet. Each picture: the average of the test "
            "digits weighted by the cell's spikes, less the plain average digit.")
    for r, (run, label) in enumerate([("born", "as born"),
                                      ("loop", "after a 60,000-digit\nclosed-loop life")]):
        index, counts = read_rows(os.path.join(CELLS_DIR, run + ".rows"))
        shown = digits[index]
        fig.text(0.02, 0.60 - r * 0.33, label, fontsize=9.3, va="center", linespacing=1.3)
        for j, i in enumerate(picked):
            w = counts[:, i].astype(np.float32)
            picture = (w @ shown) / max(w.sum(), 1) - shown.mean(0)
            ax = fig.add_axes([0.165 + j * 0.0775, 0.455 - r * 0.33, 0.071, 0.29])
            image = ax.imshow(picture.reshape(28, 28), cmap=cmap, vmin=-0.35, vmax=0.35)
            if r == 0:
                ps = born_pixels[inner[i]]
                ax.plot([p % 28 for p in ps], [p // 28 for p in ps], ".", color=INK, ms=2.6)
            ax.set_xticks([])
            ax.set_yticks([])
            for side in ax.spines.values():
                side.set_visible(True)
                side.set_color(GREY)
    bar = fig.colorbar(image, cax=fig.add_axes([0.952, 0.2, 0.011, 0.5]), ticks=[-0.35, 0, 0.35])
    bar.ax.set_yticklabels(["less", "0", "more"], fontsize=8.2)
    bar.outline.set_visible(False)
    bar.ax.tick_params(length=0)
    note(fig, "Orange: the cell fires more for digits with ink there; blue: less. Dots, top row: "
         "the cell's own pixel inputs as born (the wiring after a life is not written out). "
         "Seed 5, 10,000 test\ndigits, 28 by 28 pixels; the cells were taken evenly through "
         "the list, not chosen by eye. The closed loop is told the right answer.")
    save(fig, "hidden-cells.png")


def shrink(paths):
    from PIL import Image
    for path in paths:
        before = os.path.getsize(path)
        Image.open(path).convert("RGB").quantize(colors=256, dither=Image.Dither.NONE).save(
            path, optimize=True)
        print("%s: %d KB to %d KB" % (path, before // 1024, os.path.getsize(path) // 1024))


PLOTS = {"ladder": ladder, "headline": headline, "life": life, "map": task_map, "cells": cells}

if __name__ == "__main__":
    args = sys.argv[1:]
    if args and args[0] == "shrink":
        shrink(args[1:])
    else:
        for name in args or ["ladder", "headline", "life", "map"]:
            if name not in PLOTS:
                sys.exit("figures: no plot '%s' (%s, shrink)" % (name, ", ".join(PLOTS)))
            PLOTS[name]()
