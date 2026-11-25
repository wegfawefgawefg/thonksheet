"""Checks on a space file, run with `scripts/search.sh check ENGINE`.

1. Every number the space varies is taken by blobrun (an unknown name stops blobrun).
2. Every number with a `when` tag that is idle at the defaults changes nothing there: the
   life of the defaults and the life with that number moved must write the same csv.
3. Every number tagged `learn` leaves the learning-off twin exactly as it was: the twin of
   the defaults and the twin with that one number moved must write the same csv. A wrong
   `learn` tag would let candidates share a twin they should not, so this matters.
Short lives (2,000 steps), one seed, at most 2 at a time.
"""

import concurrent.futures
import hashlib
import os
import subprocess

import space as sp

LIFE = ["--task", "echo", "--steps", "2000", "--window", "200", "--no-baselines",
        "--alive-ticks", "0", "--rhythm-ticks", "0", "--seed", "3", "--task-seed", "3"]


def twin_csv(root, tmp, args):
    key = " ".join(args)
    out = os.path.join(tmp, "check-" + hashlib.sha1(key.encode()).hexdigest()[:12] + ".csv")
    cmd = ["nice", "-n", "19", os.path.join(root, "build", "blobrun")] + LIFE + args + [
        "--out", out]
    done = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE,
                          universal_newlines=True)
    if done.returncode != 0:
        return "error: " + done.stderr.strip()[-200:]
    text = open(out).read()
    for suffix in ("", "-sum.csv"):
        p = out if not suffix else out[:-4] + suffix
        if os.path.exists(p):
            os.remove(p)
    return text


def moved_value(n):
    if n.kind == "switch":
        return 1 - n.default
    if n.kind == "choice":
        return [c for c in n.choices if c != n.default][0]
    if n.kind in ("int", "shift"):
        return n.high if n.default != n.high else n.low
    return None


def check(space, root, tmp, jobs=2):
    os.makedirs(tmp, exist_ok=True)
    base = space.defaults()
    work = {"twin of the defaults": sp.twin_args(space, base)}
    life_work = {"life of the defaults": sp.life_args(space, base)}
    idle = []
    for n in space.numbers:
        v = moved_value(n)
        if v is None:
            continue
        values = dict(base)
        values[n.flag] = v
        is_idle = n.when is not None and not sp.live(space, base, n)
        if is_idle:
            idle.append("life with %s %s" % (n.flag, v))  # not tidied: the raw command
        if n.learn:
            work["twin with %s %s" % (n.flag, v)] = sp.twin_args(space, values)
        if is_idle or not n.learn:
            life_work["life with %s %s" % (n.flag, v)] = sp.life_args(space, values)
    every = dict(work)
    every.update(life_work)
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        names = list(every)
        outs = dict(zip(names, pool.map(lambda k: twin_csv(root, tmp, every[k]), names)))
    problems = []
    reference = outs["twin of the defaults"]
    if reference.startswith("error"):
        return ["the defaults do not run: " + reference]
    for name in work:
        if outs[name] != reference:
            problems.append("%s: the twin changed (%s); drop its learn tag" % (
                name, outs[name][:120] if outs[name].startswith("error") else "different csv"))
    for name in life_work:
        if outs[name].startswith("error"):
            problems.append("%s: %s" % (name, outs[name]))
        elif name in idle and outs[name] != outs["life of the defaults"]:
            problems.append("%s: changed the life although its `when` says it is idle" % name)
    print("%s: %d learn-tagged numbers checked against the twin, %d idle ones against the "
          "life, %d run once" % (space.engine, len(work) - 1, len(idle), len(life_work) - 1))
    return problems
