"""Running one command of the search: one blobrun life or one blobprobe reading.

Every run is `nice -n 19`, single-threaded, with a time limit. A life writes a csv and a
-sum.csv into the search's tmp/ folder; the late and early scores are read from the
summary and the files are removed. A probe prints one line; the readings at 0..4 tiles are
kept. A run that fails or times out is recorded with its error and scores 0, so it is never
retried by accident; a run that was stopped (the search shutting down) is not recorded.
"""

import hashlib
import os
import socket
import subprocess
import threading
import time

running = set()  # Popen objects, so a stop can kill them
running_lock = threading.Lock()
stopping = threading.Event()


def kill_all():
    stopping.set()
    with running_lock:
        for p in list(running):
            try:
                p.kill()
            except OSError:
                pass


def real_command(cmd, root, rule_file):
    """swap the program name for its path and rule:<hash> tokens for files"""
    out = ["nice", "-n", "19", os.path.join(root, "build", cmd[0])]
    for token in cmd[1:]:
        out.append(rule_file(token[5:]) if token.startswith("rule:") else token)
    return out


def read_summary(path):
    """first engine driver's (early, late) from a -sum.csv"""
    values = {}
    with open(path) as f:
        next(f, None)
        for line in f:
            parts = line.strip().split(",")
            if len(parts) == 3 and parts[0] not in ("coin", "perfect"):
                values.setdefault(parts[0], {})[parts[1]] = float(parts[2])
    for driver in ("blob", "table", "learn_off", "table_off"):
        if driver in values and "late" in values[driver]:
            return values[driver].get("early", 0.0), values[driver]["late"]
    raise ValueError("no late score in " + path)


def read_probe(text, feature):
    for line in text.splitlines():
        parts = line.split()
        if parts and parts[0] == feature and len(parts) >= 4:
            return [float(x) for x in parts[2:]]
    raise ValueError("no %s line in the probe output" % feature)


def run(cmd, kind, root, tmp, rule_file, timeout, engine, commit):
    """kind: life, twin or probe. returns a result row, or None if stopped"""
    key = " ".join(cmd)
    stem = os.path.join(tmp, hashlib.sha1(key.encode()).hexdigest()[:16])
    real = real_command(cmd, root, rule_file)
    if kind != "probe":
        real += ["--out", stem + ".csv"]
    row = {"key": key, "kind": kind, "engine": engine, "host": socket.gethostname(),
           "commit": commit}
    started = time.time()
    if stopping.is_set():
        return None
    proc = subprocess.Popen(real, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            stdin=subprocess.DEVNULL, universal_newlines=True, cwd=root)
    with running_lock:
        running.add(proc)
    try:
        out, err = proc.communicate(timeout=timeout)
        code = proc.returncode
    except subprocess.TimeoutExpired:
        proc.kill()
        out, err = proc.communicate()
        code = None
    finally:
        with running_lock:
            running.discard(proc)
    row["seconds"] = round(time.time() - started, 2)
    if stopping.is_set():
        return None
    try:
        if code is None:
            raise ValueError("timed out after %d s" % timeout)
        if code != 0:
            raise ValueError("exit %d: %s" % (code, (err or out).strip()[-300:]))
        if kind == "probe":
            feature = cmd[cmd.index("--feature") + 1] if "--feature" in cmd else "which"
            row["values"] = read_probe(out, feature)
            row["value"] = row["values"][1]
        else:
            row["early"], row["value"] = read_summary(stem + "-sum.csv")
    except (ValueError, OSError, IndexError) as e:
        row["error"] = str(e)
        row["value"] = None
    for suffix in (".csv", "-sum.csv"):
        if os.path.exists(stem + suffix):
            os.remove(stem + suffix)
    return row
