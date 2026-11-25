"""The rest of the task map (docs/task-map.md tiers 9 and 10, and the holes): one entry per
world and setting, shared by scripts/map.py and scripts/suite.py. Definitions are in
docs/curriculum.md, "The rest of the map".

Each entry: (name, kind of task, what it asks, blobrun flags). Everything here runs on the
standard life (20 ticks a step, 3x3 groups, 40,000 steps); the sheet worlds lay their bitmap
at 2x2 groups (it does not fit at 3x3), which belongs to the world, not the design.
"""

SHEET = ["--sensor-side", "2"]

WORLDS = [
    # relation
    ("same-easy", "relation", "same or different, side by side, 4 one-line items",
     ["--task", "same", "--item-lines", "4", "--item-on", "1"]),
    ("same-hard", "relation", "same or different, side by side, 3-of-6 items, 6 never taught",
     ["--task", "same", "--item-lines", "6", "--item-on", "3", "--hold", "6"]),
    ("same-then-easy", "relation", "same or different, one after the other, 4 items",
     ["--task", "same-then", "--item-lines", "4", "--item-on", "1"]),
    ("same-then-hard", "relation", "same or different, one after the other, 6 never taught",
     ["--task", "same-then", "--item-lines", "6", "--item-on", "3", "--hold", "6"]),
    ("odd-easy", "relation", "odd one out of 3, 4 one-line items",
     ["--task", "odd-one", "--k", "3", "--item-lines", "4", "--item-on", "1"]),
    ("odd-hard", "relation", "odd one out of 4, 2-of-5 items, 4 never taught",
     ["--task", "odd-one", "--k", "4", "--item-lines", "5", "--item-on", "2", "--hold", "4"]),
    ("bigger-easy", "relation", "which of two is bigger, 6 levels",
     ["--task", "bigger", "--item-lines", "6"]),
    ("bigger-hard", "relation", "which of two is bigger, 10 levels, always neighbours",
     ["--task", "bigger", "--item-lines", "10", "--diff-max", "1"]),
    ("analogy-easy", "relation", "A:B as C:which, shift or mirror, 2 candidates",
     ["--task", "analogy", "--k", "2", "--item-lines", "6"]),
    ("analogy-hard", "relation", "A:B as C:which, shift, mirror or add, C never taught",
     ["--task", "analogy", "--k", "3", "--item-lines", "6", "--hold", "3"]),
    # number
    ("how-many-easy", "number", "how many dots, 1 to 3", ["--task", "how-many", "--k", "3"] + SHEET),
    ("how-many-hard", "number", "how many items, 1 to 5, mixed sizes, area matched",
     ["--task", "how-many", "--k", "5", "--varied", "1"] + SHEET),
    ("more-fewer-easy", "number", "more or fewer dots than last time, 1 to 5",
     ["--task", "more-fewer", "--k", "5"] + SHEET),
    ("more-fewer-hard", "number", "more or fewer than last time, mixed sizes",
     ["--task", "more-fewer", "--k", "5", "--varied", "1"] + SHEET),
    # parts and wholes
    ("grammar-easy", "parts", "Reber string allowed or not, up to 8 symbols",
     ["--task", "grammar", "--length", "8"]),
    ("grammar-hard", "parts", "Reber string allowed or not, up to 14 symbols",
     ["--task", "grammar", "--length", "14"]),
    ("brackets-easy", "parts", "brackets balanced or not, 8 symbols",
     ["--task", "brackets", "--length", "8"]),
    ("brackets-hard", "parts", "brackets balanced or not, 12 symbols",
     ["--task", "brackets", "--length", "12"]),
    ("parts-easy", "parts", "two parts of two kinds at fixed spots: the ordered pair",
     ["--task", "parts", "--k", "2"] + SHEET),
    ("parts-hard", "parts", "two parts of three kinds anywhere, touching: the pair",
     ["--task", "parts", "--k", "3", "--varied", "1"] + SHEET),
    # chance
    ("noisy-echo-easy", "chance", "echo, reward true 80 in 100", ["--task", "noisy-echo",
                                                                   "--right-pct", "80"]),
    ("noisy-echo-hard", "chance", "echo, reward true 60 in 100", ["--task", "noisy-echo",
                                                                   "--right-pct", "60"]),
    ("rare-cue-easy", "chance", "a cue one try in 10 (balanced score)", ["--task", "rare-cue",
                                                                          "--rare", "10"]),
    ("rare-cue-hard", "chance", "a cue one try in 40 (balanced score)", ["--task", "rare-cue",
                                                                          "--rare", "40"]),
    ("noisy-lines-easy", "chance", "2 patterns on 8 lines, 10 in 100 flipped",
     ["--task", "noisy-lines", "--k", "2", "--item-lines", "8", "--flip-pct", "10"]),
    ("noisy-lines-hard", "chance", "4 patterns on 8 lines, 25 in 100 flipped",
     ["--task", "noisy-lines", "--k", "4", "--item-lines", "8", "--flip-pct", "25"]),
    # context
    ("context-easy", "context", "context line flips the rule, shown with the cue",
     ["--task", "context", "--k", "2"]),
    ("context-hard", "context", "4 cues, the context a step before the cue",
     ["--task", "context", "--k", "4", "--context-early", "1"]),
    ("switch-easy", "context", "rule changes every 500 tries, no signal",
     ["--task", "switch-rule", "--switch-every", "500"]),
    ("switch-hard", "context", "rule changes every 50 tries, no signal",
     ["--task", "switch-rule", "--switch-every", "50"]),
    # longer memory
    ("copy-easy", "memory", "copy 2 cues of 2 back in order", ["--task", "copy", "--k", "2",
                                                               "--length", "2"]),
    ("copy-hard", "memory", "copy 2 to 6 cues of 4 back in order",
     ["--task", "copy", "--k", "4", "--length", "6"]),
    ("recall-easy", "memory", "the marked cue of 2, one other between",
     ["--task", "recall", "--k", "2", "--fill", "1"]),
    ("recall-hard", "memory", "the marked cue of 4, four others between",
     ["--task", "recall", "--k", "4", "--fill", "4"]),
    ("n-back-easy", "memory", "same as 1 step ago, 4 symbols", ["--task", "n-back", "--k", "4",
                                                                "--back", "1"]),
    ("n-back-hard", "memory", "same as 2 steps ago, 4 symbols", ["--task", "n-back", "--k", "4",
                                                                 "--back", "2"]),
    # no teacher
    ("predict-easy", "no teacher", "next of 4 symbols from the last one (7 in 8)",
     ["--task", "predict-next", "--k", "4", "--markov-order", "1"]),
    ("predict-hard", "no teacher", "next of 4 symbols from the last two (7 in 8)",
     ["--task", "predict-next", "--k", "4", "--markov-order", "2"]),
    ("fill-in-easy", "no teacher", "2 patterns on 8 lines, half blanked 1 step in 4",
     ["--task", "fill-in", "--k", "2", "--item-lines", "8"]),
    ("fill-in-hard", "no teacher", "6 patterns on 8 lines, half blanked 1 step in 4",
     ["--task", "fill-in", "--k", "6", "--item-lines", "8"]),
    ("odd-event-easy", "no teacher", "a cycle of 4 symbols broken 1 step in 8",
     ["--task", "odd-event", "--k", "4", "--rare", "8"]),
    ("odd-event-hard", "no teacher", "a cycle of 6 symbols broken 1 step in 8",
     ["--task", "odd-event", "--k", "6", "--rare", "8"]),
]

# being shown the answer and damage: life protocols on existing worlds. window 200 so the
# first window after the change is short, as retention and idle have it
PROTOCOLS = [
    ("shown-echo2", "shown", "echo k=2: shown the answer, then tested (no reward)",
     ["--task", "echo", "--shown", "6"]),
    ("shown-echo4", "shown", "echo k=4: shown the answer, then tested (no reward)",
     ["--task", "echo", "--k", "4", "--shown", "6"]),
    ("shown-xor", "shown", "both-not-either: shown the answer, then tested (no reward)",
     ["--task", "both-not-either", "--shown", "6"]),
    ("damage5", "damage", "echo k=2, then 5 in 100 cells silenced", ["--task", "echo",
                                                                    "--damage", "5"]),
    ("damage20", "damage", "echo k=2, then 20 in 100 cells silenced", ["--task", "echo",
                                                                      "--damage", "20"]),
    ("damage50", "damage", "echo k=2, then 50 in 100 cells silenced", ["--task", "echo",
                                                                      "--damage", "50"]),
    ("damage-tile", "damage", "echo k=2, then the cue tile silenced", ["--task", "echo",
                                                                      "--damage-tile", "1"]),
    ("damage20-xor", "damage", "both-not-either, then 20 in 100 cells silenced",
     ["--task", "both-not-either", "--damage", "20"]),
]

# the keys each protocol writes, and which one is its headline
PROTOCOL_KEYS = {"shown": ["teach_end", "test_start", "test_mean"],
                 "damage": ["before", "after_start", "after_end"]}
