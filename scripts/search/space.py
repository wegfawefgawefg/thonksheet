"""The search space of one engine, read from scripts/search/space-<engine>.txt.

A space file has one line per number:

    flag  kind  range  default  [tags]

    --m.budget        int     16..128      48
    --m.noise_shift   shift   5..13        9
    --m.cowire_on     switch  0..1         1     learn
    --m.ev_on         choice  0,1,2        0     learn
    --g.tiles_w       fixed   8            8
    --ticks           choice  12,16,20,24  20    plug always
    --rule-file       rule    1024         none  learn seeds=rules/*.hex

kinds: switch (0 or 1), int (a small integer), shift (a chance of 2^-x; mutation moves it a
step), choice (one of a list), fixed (listed so the file is complete, never varied), rule
(a table of N 4-bit entries kept as N hex digits, entry 0 first; passed as a file).
tags: learn (the learning-off twin and the probes ignore it), plug (a plug setting, not
read by the probes), always (always on the command line, even at its default),
seeds=GLOB (rule only: files a fresh or mutated rule may start from), when=FLAG (the number
only acts while FLAG is off its default) or when=FLAG:a,b (only while FLAG is a or b).
A number that cannot act is held at its default, so candidates that differ only there are
one candidate, and mutation only touches numbers that can act. `search.sh check` tests that
each `when` is right at the defaults.

Header lines:
    engine NAME            the engine's name in the search
    flags  --engine minimal    put on every command for this engine
    twin   --g.learning_on 0   turns learning off for the twin
    order  --a --b         after drawing, --b is made at least --a

A candidate's values are a dict flag -> int (or hex string for a rule, "" for none). Only
values that differ from the default are put on a command line, sorted by flag, so adding a
line (at the engine's own default) changes no earlier command.
"""

import glob
import hashlib
import os
import random

KINDS = ("switch", "int", "shift", "choice", "fixed", "rule")


class Number:
    def __init__(self, flag, kind, low, high, choices, default, tags):
        self.flag = flag
        self.kind = kind
        self.low = low
        self.high = high
        self.choices = choices
        self.default = default
        self.learn = "learn" in tags
        self.plug = "plug" in tags
        self.always = "always" in tags
        self.seeds = []
        self.when = None  # (flag, list of values or None for "off its default")
        for t in tags:
            if t.startswith("seeds="):
                self.seeds.append(t[len("seeds="):])
            if t.startswith("when="):
                gate, _, values = t[len("when="):].partition(":")
                self.when = (gate, [int(x) for x in values.split(",")] if values else None)

    def varied(self):
        return self.kind != "fixed"


class Space:
    def __init__(self, engine):
        self.engine = engine
        self.flags = []
        self.twin = []
        self.numbers = []
        self.by_flag = {}
        self.orders = []
        self.root = ""

    def defaults(self):
        return {n.flag: n.default for n in self.numbers}


def parse_range(kind, text):
    if kind == "choice":
        return None, None, [int(x) for x in text.split(",")]
    if kind in ("fixed", "rule"):
        return int(text), int(text), None
    lo, hi = text.split("..")
    return int(lo), int(hi), None


def load_space(path, root):
    space = Space(os.path.basename(path)[len("space-"):-len(".txt")])
    space.root = root
    for raw in open(path):
        line = raw.split("#", 1)[0].strip()
        if not line:
            continue
        parts = line.split()
        head = parts[0]
        if head == "engine":
            space.engine = parts[1]
        elif head == "flags":
            space.flags = parts[1:]
        elif head == "twin":
            space.twin = parts[1:]
        elif head == "order":
            space.orders.append((parts[1], parts[2]))
        else:
            flag, kind = parts[0], parts[1]
            if kind not in KINDS:
                raise ValueError("%s: unknown kind %r in %r" % (path, kind, line))
            low, high, choices = parse_range(kind, parts[2])
            if kind == "rule":
                default = "" if parts[3] == "none" else read_rule(parts[3], root, low)
            else:
                default = int(parts[3])
            number = Number(flag, kind, low, high, choices, default, parts[4:])
            space.numbers.append(number)
            space.by_flag[flag] = number
    return space


# rules: one line of hex digits, entry 0 first, one 4-bit entry per digit

def read_rule(path, root, length):
    path = path.lstrip("@")
    text = open(os.path.join(root, path)).read().strip().lower()
    if len(text) != length or any(c not in "0123456789abcdef" for c in text):
        raise ValueError("%s: a rule must be %d hex digits on one line" % (path, length))
    return text


def rule_hash(text):
    return hashlib.sha1(text.encode()).hexdigest()[:16]


def rule_seed_files(number, root):
    files = []
    for pattern in number.seeds:
        files += sorted(glob.glob(os.path.join(root, pattern)))
    return files


def random_rule(rng, length):
    return "".join(rng.choice("0123456789abcdef") for _ in range(length))


# drawing, mutating, crossing

def live(space, values, number):
    """can this number act, given the others?"""
    if number.when is None:
        return True
    gate = space.by_flag.get(number.when[0])
    if gate is None:
        return True
    v = values.get(gate.flag, gate.default)
    return v != gate.default if number.when[1] is None else v in number.when[1]


def fix_orders(space, values):
    """pairs put in order; numbers that cannot act set to their defaults"""
    for lo, hi in space.orders:
        if lo in values and hi in values and values[hi] < values[lo]:
            values[lo], values[hi] = values[hi], values[lo]
    for n in space.numbers:
        if n.when is not None and not live(space, values, n):
            values[n.flag] = n.default
    return values


def draw_one(number, rng, root):
    if number.kind == "switch":
        return rng.randint(0, 1)
    if number.kind == "choice":
        return rng.choice(number.choices)
    if number.kind in ("int", "shift"):
        return rng.randint(number.low, number.high)
    if number.kind == "rule":
        seeds = rule_seed_files(number, root)
        pick = rng.random()
        if seeds and pick < 0.5:
            return read_rule(rng.choice(seeds), root, number.low)
        if pick < 0.75:
            return random_rule(rng, number.low)
        return number.default
    return number.default


def fresh(space, rng, share=0.5):
    """the defaults, with about `share` of the varied numbers drawn anew"""
    values = space.defaults()
    for n in space.numbers:
        if n.varied() and rng.random() < share:
            values[n.flag] = draw_one(n, rng, space.root)
    return fix_orders(space, values)


def nudge(number, value, rng, root):
    if number.kind == "switch":
        return 1 - value
    if number.kind == "choice":
        others = [c for c in number.choices if c != value]
        return rng.choice(others) if others else value
    if number.kind == "rule":
        if not value:
            return draw_one(number, rng, root) or random_rule(rng, number.low)
        entries = list(value)
        for _ in range(rng.randint(1, 8)):
            entries[rng.randrange(len(entries))] = rng.choice("0123456789abcdef")
        return "".join(entries)
    if rng.random() < 0.25:
        return rng.randint(number.low, number.high)
    if number.kind == "shift":
        step = rng.choice((-1, 1)) * (1 if rng.random() < 0.8 else 2)
    else:
        size = max(1, (number.high - number.low) // 6)
        step = rng.choice((-1, 1)) * rng.randint(1, size)
    return min(number.high, max(number.low, value + step))


def mutate(space, values, rng, changes=None):
    """change a few numbers: 1 to 4, mostly 1 or 2"""
    out = dict(space.defaults())
    out.update(values)
    varied = [n for n in space.numbers if n.varied() and live(space, out, n)]
    if changes is None:
        changes = 1
        while changes < 4 and rng.random() < 0.45:
            changes += 1
    for n in rng.sample(varied, min(changes, len(varied))):
        out[n.flag] = nudge(n, out[n.flag], rng, space.root)
    return fix_orders(space, out)


def crossover(space, a, b, rng):
    """each number from either parent; a rule entry by entry"""
    out = {}
    for n in space.numbers:
        va = a.get(n.flag, n.default)
        vb = b.get(n.flag, n.default)
        if n.kind == "rule" and va and vb and len(va) == len(vb):
            out[n.flag] = "".join(rng.choice(pair) for pair in zip(va, vb))
        else:
            out[n.flag] = va if rng.random() < 0.5 else vb
    return fix_orders(space, out)


# command lines

def changed(space, values):
    """(flag, value) pairs that go on a command line, sorted by flag"""
    pairs = []
    for flag in sorted(values):
        n = space.by_flag.get(flag)
        if n is None:
            continue
        v = values[flag]
        if v != n.default or n.always:
            pairs.append((n, v))
    return pairs


def arg_list(space, values, keep):
    """flags for a command; keep(number) says which numbers belong. a rule shows as
    rule:<hash> in the key; the runner swaps in a file path."""
    out = list(space.flags)
    for n, v in changed(space, values):
        if not keep(n):
            continue
        if n.kind == "rule":
            if v:
                out += [n.flag, "rule:" + rule_hash(v)]
        else:
            out += [n.flag, str(v)]
    return out


def life_args(space, values):
    return arg_list(space, values, lambda n: True)


def twin_args(space, values):
    return arg_list(space, values, lambda n: not n.learn) + list(space.twin)


def probe_args(space, values):
    return arg_list(space, values, lambda n: not n.learn and not n.plug)


def candidate_id(space, values):
    key = space.engine + " " + " ".join(life_args(space, values))
    return hashlib.sha1(key.encode()).hexdigest()[:12]


def rules_in(space, values):
    """hash -> hex text for every rule a candidate carries"""
    out = {}
    for n in space.numbers:
        v = values.get(n.flag, n.default)
        if n.kind == "rule" and v:
            out[rule_hash(v)] = v
    return out


def describe(space, values):
    """the non-default numbers as a short readable string"""
    parts = []
    for n, v in changed(space, values):
        if n.always and v == n.default:
            continue
        parts.append("%s %s" % (n.flag, ("rule:" + rule_hash(v)) if n.kind == "rule" else v))
    return " ".join(parts) if parts else "(defaults)"


def make_rng(seed=None):
    return random.Random(seed if seed is not None else int.from_bytes(os.urandom(8), "little"))
