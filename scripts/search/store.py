"""Everything a search has done, in append-only files in its folder (out/search/<name>/).

    results.jsonl     one line per finished run, keyed by its exact command
    candidates.jsonl  one line per candidate: id, engine, numbers, how it was made
    decisions.jsonl   one line per race decision: id, rung, go or not, why
    settings.json     the quick-map file and engines the search was started with
    rules/            rule tables written out for runs (rule kind numbers)

Nothing is ever rewritten, so a crash loses at most the runs in flight. Two folders are
joined with merge(): lines the other has and this one lacks are appended.
"""

import json
import os
import time

FILES = ("results.jsonl", "candidates.jsonl", "decisions.jsonl")


def read_lines(path):
    rows = []
    if not os.path.exists(path):
        return rows
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rows.append(json.loads(line))
            except ValueError:
                pass  # a line cut short by a crash
    return rows


def append(path, row):
    with open(path, "a") as f:
        f.write(json.dumps(row, sort_keys=True) + "\n")


class Store:
    def __init__(self, folder):
        self.folder = folder
        os.makedirs(folder, exist_ok=True)
        os.makedirs(os.path.join(folder, "rules"), exist_ok=True)
        self.results = {}      # key -> row
        self.candidates = {}   # id -> row
        self.order = []        # candidate ids in the order they were made
        self.decisions = {}    # (id, rung) -> row; a go anywhere wins
        self.by_candidate = {}  # id -> set of result keys, filled by the scheduler
        for row in read_lines(self.path("results.jsonl")):
            self.results.setdefault(row["key"], row)
        for row in read_lines(self.path("candidates.jsonl")):
            if row["id"] not in self.candidates:
                self.candidates[row["id"]] = row
                self.order.append(row["id"])
        for row in read_lines(self.path("decisions.jsonl")):
            self.note_decision(row)

    def path(self, name):
        return os.path.join(self.folder, name)

    def note_decision(self, row):
        k = (row["id"], row["rung"])
        old = self.decisions.get(k)
        if old is None or (row["go"] and not old["go"]):
            self.decisions[k] = row

    def add_result(self, row):
        row.setdefault("when", time.strftime("%Y-%m-%d %H:%M:%S"))
        if row["key"] in self.results:
            return
        self.results[row["key"]] = row
        append(self.path("results.jsonl"), row)

    def add_candidate(self, row):
        if row["id"] in self.candidates:
            return False
        row.setdefault("when", time.strftime("%Y-%m-%d %H:%M:%S"))
        self.candidates[row["id"]] = row
        self.order.append(row["id"])
        append(self.path("candidates.jsonl"), row)
        return True

    def add_decision(self, cand_id, rung, go, why):
        row = {"id": cand_id, "rung": rung, "go": bool(go), "why": why,
               "when": time.strftime("%Y-%m-%d %H:%M:%S")}
        self.note_decision(row)
        append(self.path("decisions.jsonl"), row)

    def decision(self, cand_id, rung):
        return self.decisions.get((cand_id, rung))

    def settings(self):
        p = self.path("settings.json")
        return json.load(open(p)) if os.path.exists(p) else {}

    def save_settings(self, settings):
        with open(self.path("settings.json"), "w") as f:
            json.dump(settings, f, indent=1, sort_keys=True)

    def rule_path(self, digest, text):
        p = os.path.join(self.folder, "rules", digest + ".hex")
        if not os.path.exists(p):
            with open(p + ".part", "w") as f:
                f.write(text + "\n")
            os.replace(p + ".part", p)
        return p


def merge(into, other):
    """append to folder `into` every line of folder `other` it does not have yet"""
    mine = Store(into)
    theirs = Store(other)
    added = [0, 0, 0]
    for key, row in theirs.results.items():
        if key not in mine.results:
            mine.results[key] = row
            append(mine.path("results.jsonl"), row)
            added[0] += 1
    for cid in theirs.order:
        if cid not in mine.candidates:
            mine.add_candidate(theirs.candidates[cid])
            added[1] += 1
    for k, row in theirs.decisions.items():
        old = mine.decisions.get(k)
        if old is None or (row["go"] and not old["go"]):
            mine.note_decision(row)
            append(mine.path("decisions.jsonl"), row)
            added[2] += 1
    if not mine.settings() and theirs.settings():
        mine.save_settings(theirs.settings())
    return added
