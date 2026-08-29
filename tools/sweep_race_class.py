#!/usr/bin/env python3
"""Every race x every class, built through the program itself.

tools/combos.c crosses these in memory, which is what makes two hundred
thousand of them affordable. This crosses them through the wizard, which is
slower by three orders of magnitude and reaches what the other cannot: the
prompts. The racial skill choice, the subclass menu, the equipment
packages and the spell pickers only exist on this side, and so do the bugs
in them -- a pack that reached the sheet unopened, a cleric handed nothing
for an option it offered, a race whose menu entry was a substring of
another's.

Every build is checked for a clean exit, no sanitizer output, a sheet
actually written, and the race on that sheet being the one that was asked
for.

Usage:
    sweep_race_class.py [levels] [a|b|all]

    levels   comma separated, default "1,5"
    a|b      run the first or second half of the races, so two can run at
             once on a machine with cores to spare

Set DNDBIN to test a binary other than ./dndcreator -- useful for pointing
it at a sanitizer build without rebuilding the tree.
"""
import sys, os, random, tempfile, re
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive

BIN = os.environ.get("DNDBIN", os.path.abspath("./dndcreator"))
RACES, CLASSES = [], []
from build_data import read_file
os.chdir(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
for r in read_file("character.txt"):
    if r.tag == "RACE":  RACES.append(r.str(0))
    if r.tag == "CLASS": CLASSES.append(r.str(0))

LEVELS = [int(x) for x in (sys.argv[1].split(",") if len(sys.argv) > 1 else ["1","5"])]
HALF = sys.argv[2] if len(sys.argv) > 2 else "all"
if HALF == "a":   RACES = RACES[:len(RACES)//2]
elif HALF == "b": RACES = RACES[len(RACES)//2:]
bad = ok = 0
for race in RACES:
    for cls in CLASSES:
        for lvl in LEVELS:
            seed = (abs(hash((race, cls, lvl))) % 90000) + 1000
            rng = random.Random(seed)
            with tempfile.TemporaryDirectory() as wd:
                try:
                    rc, tr, err, name = drive.run_once(
                        BIN, seed, rng, False, wd, False, False, 0, 0, 0,
                        {"class": cls, "level": lvl, "race": race})
                except Exception as exc:
                    print("FAIL %-14s %-10s L%-2d harness: %s" % (race, cls, lvl, exc))
                    bad += 1
                    continue
                sheets = [f for f in os.listdir(wd) if f.endswith(".txt")]
                blob = "".join(tr)
                problems = []
                if rc != 0:
                    problems.append("exit %d" % rc)
                errtxt = err.decode("utf-8","replace") if isinstance(err, bytes) else err
                for marker in ("Sanitizer", "runtime error:", "Assertion"):
                    if marker in errtxt: problems.append(errtxt.strip().split("\n")[0][:100])
                if not sheets:
                    problems.append("no sheet written")
                else:
                    sheet = open(os.path.join(wd, sheets[0])).read()
                    m = re.search(r"Race: (.+?)\s{2,}", sheet)
                    got = m.group(1).split(" / ")[0].strip() if m else "?"
                    if got != race:
                        problems.append("built %s not %s" % (got, race))
                    if "Armor Class" not in sheet:
                        problems.append("no armour class")
                if problems:
                    print("FAIL %-14s %-10s L%-2d  %s" % (race, cls, lvl, "; ".join(problems)))
                    bad += 1
                else:
                    ok += 1
                if (ok + bad) % 25 == 0:
                    print("  ... %d built, %d failed" % (ok, bad), flush=True)
print("\n%d built, %d failed, over %d races x %d classes x %d levels"
      % (ok, bad, len(RACES), len(CLASSES), len(LEVELS)))
sys.exit(1 if bad else 0)
