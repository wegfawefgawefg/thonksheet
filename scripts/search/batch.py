"""The seam for an evaluator that runs many lives in one go (blobgpu: thousands of
minimal-engine lives at once on the GPU), beside the one-process-per-life runner.

An evaluator says which commands it takes (accepts(engine, kind)) and runs a list of them,
returning one result row per command, keyed by the command's own key, with the same fields
the runner gives (value = the late score, early, seconds, error). The search hands it up to
`size` commands at a time, in a slot of its own beside the --jobs CPU slots, and records the
rows exactly as if blobrun had run them. So an evaluator must give blobrun's numbers for the
same command; one that does not (another random stream, say) belongs in a search of its own
name, so its scores never mix with blobrun's.

    --batch serial                    runs its batch one life at a time through the runner
                                      (to exercise the seam; no speed gain)
    --batch program:PATH[:ENGINES[:SIZE]]   an external program: PATH --in A.csv --out B.csv
                                      (ENGINES a + list, default minimal; SIZE lives per
                                      batch, default 1024)

The csv a program gets: header `key,args`, then one line per life, args being blobrun's
flags after the program name, space-separated, with rule files as paths. The csv it gives
back: header `key,early,late,error`, one line per key, error empty when it ran.
"""

import csv
import os
import subprocess
import time

import runner


class SerialBatch:
    name = "serial"

    def __init__(self, engines=("minimal",), size=16):
        self.engines = set(engines)
        self.size = size

    def accepts(self, engine, kind):
        return engine in self.engines and kind in ("life", "twin")

    def run(self, jobs, root, tmp, rule_file, timeout, commit):
        rows = []
        for cid, kind, cmd, world, engine in jobs:
            row = runner.run(cmd, kind, root, tmp, rule_file, timeout, engine, commit)
            if row is not None:
                row["world"] = world
                row["evaluator"] = self.name
            rows.append(row)
        return rows


class ProgramBatch:
    def __init__(self, path, engines=("minimal",), size=1024):
        self.path = path
        self.name = os.path.basename(path)
        self.engines = set(engines)
        self.size = size
        self.count = 0

    def accepts(self, engine, kind):
        return engine in self.engines and kind in ("life", "twin")

    def run(self, jobs, root, tmp, rule_file, timeout, commit):
        self.count += 1
        stem = os.path.join(tmp, "batch-%d-%d" % (os.getpid(), self.count))
        with open(stem + "-in.csv", "w", newline="") as f:
            w = csv.writer(f)
            w.writerow(["key", "args"])
            for cid, kind, cmd, world, engine in jobs:
                real = runner.real_command(cmd, root, rule_file)[4:]
                w.writerow([" ".join(cmd), " ".join(real)])
        started = time.time()
        if runner.stopping.is_set():
            return [None] * len(jobs)
        proc = subprocess.Popen(["nice", "-n", "19", self.path, "--in", stem + "-in.csv",
                                 "--out", stem + "-out.csv"], cwd=root,
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                stdin=subprocess.DEVNULL, universal_newlines=True)
        with runner.running_lock:
            runner.running.add(proc)
        try:
            proc.stdout_text = proc.communicate()[0]
        finally:
            with runner.running_lock:
                runner.running.discard(proc)
        if runner.stopping.is_set():
            return [None] * len(jobs)
        seconds = (time.time() - started) / max(1, len(jobs))
        got = {}
        if proc.returncode == 0 and os.path.exists(stem + "-out.csv"):
            for r in csv.DictReader(open(stem + "-out.csv")):
                got[r["key"]] = r
        rows = []
        for cid, kind, cmd, world, engine in jobs:
            key = " ".join(cmd)
            row = {"key": key, "kind": kind, "engine": engine, "world": world,
                   "commit": commit, "evaluator": self.name, "seconds": round(seconds, 3)}
            r = got.get(key)
            if r is None:
                # the whole batch failed: leave these unrecorded so the CPU path can run them
                rows.append(None)
                continue
            if r.get("error"):
                row["error"], row["value"] = r["error"], None
            else:
                row["early"], row["value"] = float(r["early"]), float(r["late"])
            rows.append(row)
        if proc.returncode != 0:
            print("batch %s failed (exit %d): %s" % (self.name, proc.returncode,
                                                     proc.stdout_text[-300:]), flush=True)
        for suffix in ("-in.csv", "-out.csv"):
            if os.path.exists(stem + suffix):
                os.remove(stem + suffix)
        return rows


def make(spec):
    """--batch value -> evaluator, or None"""
    if not spec:
        return None
    if spec == "serial":
        return SerialBatch()
    if spec.startswith("program:"):
        parts = spec.split(":")
        engines = parts[2].split("+") if len(parts) > 2 else ["minimal"]
        size = int(parts[3]) if len(parts) > 3 else 1024
        return ProgramBatch(parts[1], engines, size)
    raise ValueError("--batch is serial or program:PATH[:ENGINES[:SIZE]]")
