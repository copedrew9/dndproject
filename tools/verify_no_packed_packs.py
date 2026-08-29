#!/usr/bin/env python3
"""No route into an inventory may leave a pack packed.

Taking a pack puts what is in it on the sheet, not the pack. That is easy to
get right at one acquisition site and easy to miss at another: the
starting-equipment resolver has an alias table that added the item and
returned before reaching the unpacking, and the Priest's pack is the one
pack with an entry in it -- so a cleric whose equipment line happened to
match the alias carried "1 x Priest's pack" while all six of the other
packs came apart correctly. One seed of one class did not show it; this
does.

Every class is built at two levels over several seeds, and every saved
sheet is read back for an ITEM row whose name is a pack. There should never
be one.
"""

import argparse
import os
import random
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import drive                                    # noqa: E402
from build_data import read_file                # noqa: E402


def packs_in(sheet):
    out = []
    for line in sheet.split("\n"):
        if not line.startswith("ITEM|"):
            continue
        f = line.split("|")
        if len(f) > 3 and f[3].lower().endswith(" pack"):
            out.append(f[3])
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seeds", type=int, default=6)
    ap.add_argument("--binary", default="./dndcreator")
    args = ap.parse_args()

    binary = os.path.abspath(args.binary)
    classes = [r.str(0) for r in read_file("character.txt") if r.tag == "CLASS"]
    pack_names = set(r.str(0) for r in read_file("equipment.txt")
                     if r.tag == "ITEM" and r.str(2) == "pack")

    checked = bad = 0
    for cls in classes:
        for level in (1, 5):
            for k in range(args.seeds):
                seed = 1000 + k * 97 + len(cls) + level
                rng = random.Random(seed)
                with tempfile.TemporaryDirectory() as wd:
                    try:
                        rc, tr, err, name = drive.run_once(
                            binary, seed, rng, False, wd, False, False,
                            0, 0, 0, {"class": cls, "level": level})
                    except Exception as exc:      # noqa: BLE001
                        print("  %s %d seed %d: harness error: %s"
                              % (cls, level, seed, exc))
                        bad += 1
                        continue
                    sheets = [f for f in os.listdir(wd) if f.endswith(".txt")]
                    if not sheets:
                        continue
                    sheet = open(os.path.join(wd, sheets[0]),
                                 encoding="utf-8", errors="replace").read()
                    checked += 1
                    left = [p for p in packs_in(sheet) if p in pack_names]
                    if left:
                        print("  %s %d seed %d still carries %s"
                              % (cls, level, seed, ", ".join(left)))
                        bad += 1

    print("\n%d sheets checked, %d still carrying a packed pack"
          % (checked, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
