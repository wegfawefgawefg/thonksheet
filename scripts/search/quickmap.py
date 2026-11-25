"""The quick map: which worlds a candidate is scored on, read from scripts/search/quick-map.txt.

    budget   120          seconds of one core for one pass of every world on one seed
    life     --steps 10000 --window 200 --no-baselines --alive-ticks 0 --rhythm-ticks 0
    probe    --tries 1500 --settle 5000
    select   1 2 3
    confirm  5 6 7 8
    pass     0.3
    timeout  900
    # name   tier kind  zero  full  fast engines  flags
    echo2    1    life  0.50  1.00  fast all      --task echo --k 2
    f-xor    2    probe 0.30  6.00  -    blob     --feature xor

A life world is one blobrun life and its learning-off twin, on one seed; its score is the
late score less the larger of the twin's and chance (zero), over perfect (full) less chance,
held between 0 and 1. A probe world is one `blobprobe feature` reading of the sheet as born
(learning off), at the tile next to the input; its score is (reading - zero) / (full - zero),
held between 0 and 1. A world is passed when its mean score over the seeds is at least
`pass`. `fast` worlds are the first race. `engines` is all or a comma list.

Command order: the base flags, then the candidate's, then the world's, then the seed, so a
world's own flags (its task and how it lays its lines out) win.
"""

import os


class World:
    def __init__(self, name, tier, kind, zero, full, fast, engines, flags):
        self.name = name
        self.tier = tier
        self.kind = kind
        self.zero = zero
        self.full = full
        self.fast = fast
        self.engines = engines
        self.flags = flags

    def runs_on(self, engine):
        return self.engines == ["all"] or engine in self.engines


class QuickMap:
    def __init__(self):
        self.life = []
        self.probe = []
        self.select = [1, 2, 3]
        self.confirm = [5, 6, 7, 8]
        self.passing = 0.3
        self.timeout = 900
        self.budget = 0.0  # seconds of one core for one pass of the map on one seed
        self.worlds = []
        self.by_name = {}

    def worlds_for(self, engine, fast_only=False):
        return [w for w in self.worlds if w.runs_on(engine) and (w.fast or not fast_only)]


def load_quick_map(path):
    qm = QuickMap()
    for raw in open(path):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        head = parts[0]
        if head == "life":
            qm.life = parts[1:]
        elif head == "probe":
            qm.probe = parts[1:]
        elif head == "select":
            qm.select = [int(x) for x in parts[1:]]
        elif head == "confirm":
            qm.confirm = [int(x) for x in parts[1:]]
        elif head == "pass":
            qm.passing = float(parts[1])
        elif head == "budget":
            qm.budget = float(parts[1])
        elif head == "timeout":
            qm.timeout = int(parts[1])
        else:
            w = World(parts[0], int(parts[1]), parts[2], float(parts[3]), float(parts[4]),
                      parts[5] == "fast", parts[6].split(","), parts[7:])
            if w.kind not in ("life", "probe"):
                raise ValueError("%s: world kind is life or probe: %r" % (path, line))
            if w.name in qm.by_name:
                raise ValueError("%s: world %s twice" % (path, w.name))
            qm.worlds.append(w)
            qm.by_name[w.name] = w
    if set(qm.select) & set(qm.confirm):
        raise ValueError("%s: selection and confirmation seeds must not mix" % path)
    return qm


# commands. a command is a list starting with the program name (blobrun or blobprobe); its
# key in the cache is the list joined by spaces.

def seed_flags(seed):
    return ["--seed", str(seed), "--task-seed", str(seed)]


def life_command(qm, world, cand_args, seed):
    return ["blobrun"] + qm.life + cand_args + world.flags + seed_flags(seed)


def probe_command(qm, world, cand_args, seed):
    return (["blobprobe", "feature"] + qm.probe + cand_args + world.flags +
            ["--seed", str(seed), "--label", "x", "--g.learning_on", "0"])


def key_of(cmd):
    return " ".join(cmd)


def clamp01(x):
    return 0.0 if x < 0 else (1.0 if x > 1 else x)


def life_score(world, late, twin):
    floor = max(twin, world.zero)
    return clamp01((late - floor) / (world.full - world.zero))


def life_gain(world, late, twin):
    """the life score before it is held between 0 and 1"""
    return (late - max(twin, world.zero)) / (world.full - world.zero)


def probe_score(world, reading):
    return clamp01((reading - world.zero) / (world.full - world.zero))


def default_paths(root):
    here = os.path.join(root, "scripts", "search")
    return os.path.join(here, "quick-map.txt"), here
