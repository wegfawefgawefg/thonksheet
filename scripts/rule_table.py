#!/usr/bin/env python3
"""Rule tables (idea 120): read, write, build and print them as sentences.

    scripts/rule_table.py print rules/error.rule          the table as plain sentences
    scripts/rule_table.py hand rules/                     write the hand-written tables again
    scripts/rule_table.py diff a.rule b.rule              the entries where two tables differ

A rule is 1,032 hex digits: 8 header digits (four chance shifts, the window, the read-off,
the balance, 0) then one digit per entry, entry i for fact pattern i. An entry digit is
action * 4 + chance (action 0 nothing, 1 up, 2 down, 3 toward zero; chance picks a shift).
Fact f is bit f of the entry index; the facts are FACTS below and src/blob/learn_rule.hpp.
A rule file is that string, whitespace and # comments allowed.
"""

import os
import random
import sys

FACTS = ["src_fired", "src_causal", "src_inh", "src_conn", "src_top",
         "n_recent", "n_button", "n_mark", "reward", "bad"]
BIT = {name: i for i, name in enumerate(FACTS)}
ENTRIES = 1 << len(FACTS)
HEADER = 8
NOTHING, UP, DOWN, DECAY = 0, 1, 2, 3
READOFFS = ["evidence, wired at strength 4", "evidence, strength by counter",
            "direct: the connection's level is the counter"]


class Rule:
    def __init__(self, shifts=(4, 6, 2, 12), window=15, readoff=0, balance=0):
        self.shifts = list(shifts)
        self.window = window
        self.readoff = readoff
        self.balance = balance
        self.entry = [0] * ENTRIES

    def hex(self):
        head = [*self.shifts, self.window % 16, self.readoff, self.balance, 0]
        return "".join("%x" % d for d in head) + "".join("%x" % e for e in self.entry)

    def copy(self):
        r = Rule(self.shifts, self.window, self.readoff, self.balance)
        r.entry = list(self.entry)
        return r

    def set(self, action, chance, **facts):
        """every entry matching the named facts (1 or 0; the rest any) gets action, chance"""
        care = value = 0
        for name, v in facts.items():
            care |= 1 << BIT[name]
            value |= (1 << BIT[name]) if v else 0
        for i in range(ENTRIES):
            if i & care == value:
                self.entry[i] = action * 4 + chance
        return self


def parse(text):
    digits = []
    for line in text.splitlines():
        line = line.split("#", 1)[0]
        digits += [int(c, 16) for c in line if not c.isspace()]
    if len(digits) != HEADER + ENTRIES:
        raise ValueError(f"{len(digits)} hex digits, want {HEADER + ENTRIES}")
    r = Rule(digits[0:4], digits[4] or 15, min(digits[5], 2), digits[6] & 1)
    r.entry = digits[HEADER:]
    return r


def load(path):
    return parse(open(path).read())


def save(rule, path, comment=""):
    os.makedirs(os.path.dirname(os.path.abspath(path)), exist_ok=True)
    h = rule.hex()
    with open(path, "w") as f:
        for line in comment.strip().splitlines():
            f.write("# " + line + "\n" if line else "#\n")
        f.write(h[:HEADER] + "\n")
        for i in range(HEADER, len(h), 64):
            f.write(h[i:i + 64] + "\n")


def cubes(rule):
    """the non-nothing entries merged into (care, value, digit) cubes over all ten facts"""
    full = ENTRIES - 1
    live = {(full, i, rule.entry[i]) for i in range(ENTRIES) if rule.entry[i] >> 2}
    merged = True
    while merged:
        merged = False
        for bit in range(len(FACTS)):
            b = 1 << bit
            out, used = set(), set()
            for c in sorted(live):
                if c in used:
                    continue
                care, value, d = c
                if care & b and not value & b:
                    other = (care, value | b, d)
                    if other in live and other not in used:
                        used |= {c, other}
                        out.add((care & ~b, value, d))
                        merged = True
                        continue
            live = (live - used) | out
    return sorted(live, key=lambda c: (-(c[0] >> 8 & 3), c[1] >> 8, c[1], c[0]))


def _one(care, value, name, yes, no):
    b = 1 << BIT[name]
    if not care & b:
        return None
    return yes if value & b else no


def sentence(rule, cube):
    care, value, d = cube
    action, chance = d >> 2, d & 3
    shift = rule.shifts[chance]
    reward = _one(care, value, "reward", 1, 0)
    bad = _one(care, value, "bad", 1, 0)
    if reward == 1:
        when = {1: "when bad arrives", 0: "when good arrives", None: "when a reward arrives"}[bad]
    elif reward == 0:
        when = "when the neuron fires" + {1: ", while the last reward was bad",
                                           0: ", while the last reward was good",
                                           None: ""}[bad]
    else:
        when = "at a reward or when the neuron fires" + {1: " (last reward bad)",
                                                        0: " (last reward good)", None: ""}[bad]
    button = _one(care, value, "n_button", 1, 0)
    mark = _one(care, value, "n_mark", 1, 0)
    who = {
        (1, 1): "a cell of the pressed button",
        (1, 0): "a cell of another button",
        (1, None): "a button cell",
        (0, 1): "an inner neuron whose last spike was noise",
        (0, 0): "an inner neuron whose last spike was not noise",
        (0, None): "an inner neuron",
        (None, 1): "a pressed-button cell or a noise-fired inner neuron",
        (None, 0): "a neuron (not pressed, not noise-fired)",
        (None, None): "any neuron",
    }[(button, mark)]
    recent = _one(care, value, "n_recent", "that fired recently", "that did not fire recently")
    if recent:
        who += " " + recent
    direct = rule.readoff == 2
    verb = {UP: "strengthens its links from" if direct else "gains evidence for",
            DOWN: "weakens its links from" if direct else "loses evidence for",
            DECAY: "weakens its links from" if direct else "lets evidence fade toward zero for"}[action]
    kind = _one(care, value, "src_inh", "inhibitory", "excitatory")
    conn = _one(care, value, "src_conn", "connected", "unconnected")
    top = _one(care, value, "src_top", "strong" if direct else "positive-evidence",
               "weak" if direct else "non-positive-evidence")
    adjectives = [a for a in (conn, top, kind) if a]
    what = " ".join(adjectives + ["sources"])
    quals = []
    fired = _one(care, value, "src_fired", 1, 0)
    if fired == 1:
        quals.append("that fired" + (" (one chance per tick they fired)" if reward == 1 else
                                     " recently"))
    elif fired == 0:
        quals.append("that did not fire" + ("" if reward == 1 else " recently"))
    elif reward != 0:
        quals.append("(at a reward, one chance per tick a source fired, one for a quiet one)")
    causal = _one(care, value, "src_causal", "that fired exactly one delay before its last spike",
                  "that did not fire at its delay before its last spike")
    if causal:
        quals.append(causal)
    text = f"{when}, {who} {verb} {what}"
    if quals:
        text += " " + " and ".join(quals)
    return text + f", chance 1 in {2 ** shift}"


def describe(rule):
    lines = [f"read-off: {READOFFS[rule.readoff]}; window {rule.window} ticks; "
             f"balance {'on' if rule.balance else 'off'}; chances "
             + ", ".join(f"1/{2 ** s}" for s in rule.shifts)]
    live = sum(1 for e in rule.entry if e >> 2)
    lines.append(f"{live} of {ENTRIES} entries act")
    for c in cubes(rule):
        lines.append("- " + sentence(rule, c))
    return "\n".join(lines)


# the hand-written rules as tables
def hand_error():
    """learn_readout.cpp with evidence counters and balance (the best readout)"""
    r = Rule(shifts=(4, 6, 2, 12), window=15, readoff=0, balance=1)
    for mark, sign in ((1, DOWN), (0, UP)):
        other = UP if sign == DOWN else DOWN
        r.set(sign, 0, reward=1, bad=1, n_button=1, n_mark=mark, src_fired=1, src_inh=0)
        r.set(other, 0, reward=1, bad=1, n_button=1, n_mark=mark, src_fired=1, src_inh=1)
    return r


def hand_old():
    """learn_signal.cpp learn_reward: good strengthens co-active connected pairs, bad weakens
    the strong ones (never below 1), on the connections directly"""
    r = Rule(shifts=(2, 4, 6, 12), window=6, readoff=2, balance=0)
    r.set(UP, 0, reward=1, bad=0, n_recent=1, src_fired=1, src_conn=1)
    r.set(DOWN, 0, reward=1, bad=1, n_recent=1, src_fired=1, src_top=1)
    return r


def hand_grow():
    """learn_wiring.cpp grow_one: a neuron that fires wires in recently active sources it is
    not connected to (per pair, chance 1 in 4096; the cap is fan_in_max)"""
    r = Rule(shifts=(12, 4, 6, 2), window=15, readoff=2, balance=0)
    r.set(UP, 0, reward=0, src_fired=1, src_conn=0)
    return r


HAND = {
    "error": (hand_error, "the error rule of learn_readout.cpp with evidence counters and\n"
                          "balance (learn_mode 5, readout_evidence 1, readout_shift 4)"),
    "old": (hand_old, "the old good/bad rule (learn_signal.cpp learn_reward), on the\n"
                      "connections directly"),
    "grow": (hand_grow, "growth toward recently active sources (learn_wiring.cpp grow_one)"),
}


def random_sparse(rng, cubes_n=None, readoff=None):
    """a few random cubes, each over 3 to 7 facts, on a table that is otherwise nothing"""
    r = Rule(shifts=(rng.choice([2, 3, 4]), rng.choice([5, 6, 7]), rng.choice([1, 2]),
                     rng.choice([8, 10, 12])),
             window=rng.choice([6, 10, 15]), readoff=rng.choice([0, 1]) if readoff is None
             else readoff, balance=rng.choice([0, 1]))
    for _ in range(cubes_n or rng.randint(2, 5)):
        random_cube(rng, r, rng.randint(1, 3), rng.randint(0, 3))
    return r


def random_cube(rng, rule, action, chance, lo=3, hi=7):
    names = rng.sample(FACTS, rng.randint(lo, hi))
    rule.set(action, chance, **{n: rng.randint(0, 1) for n in names})


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    what = sys.argv[1]
    if what == "print":
        for path in sys.argv[2:]:
            print(f"## {path}\n{describe(load(path))}\n")
    elif what == "hand":
        out = sys.argv[2] if len(sys.argv) > 2 else "rules"
        for name, (make, comment) in HAND.items():
            save(make(), os.path.join(out, name + ".rule"), comment + "\n\n" + describe(make()))
            print(os.path.join(out, name + ".rule"))
    elif what == "diff":
        a, b = load(sys.argv[2]), load(sys.argv[3])
        n = sum(1 for x, y in zip(a.entry, b.entry) if x != y)
        print(f"{n} entries differ; headers {a.hex()[:HEADER]} {b.hex()[:HEADER]}")
    elif what == "random":
        rng = random.Random(int(sys.argv[2]) if len(sys.argv) > 2 else 1)
        print(random_sparse(rng).hex())
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
