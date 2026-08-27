#!/usr/bin/env python3
"""Look for content in the books that is missing from data/.

tools/audit.py checks one direction: every name in data/ has to appear in
the book it claims. This checks the other, which is the direction a gap
hides in -- an item nobody wrote down is not a name that fails to resolve,
it is simply absent, and nothing notices.

Both things it looks for open with a line the books print in a fixed form
and nowhere else: a magic item's "Wondrous item, rare (requires
attunement)", and a spell's "3rd-level evocation" or "Evocation cantrip".
The name is the heading above that line. That is enough to enumerate what
the book has and subtract what we have.

It is deliberately loose about what counts as a match, for the same reason
audit.py is: headings are set in small capitals, some open with a
decorative initial that the text layer drops, and two-column pages
interleave. So a heading that does not match is reported rather than
counted, and the report is short enough to read. What it found when it was
written was seven magic items nobody had entered -- the Axe of the Dwarvish
Lords, the Eye and Hand of Vecna, a moonblade, and four smaller ones.

Matching is loose in three ways, all of them to absorb the dump rather
than to be generous about what counts as entered. Letters only, lowercased,
with the pairs the text layer confuses folded together (c for g, i and 1
for l, 0 for o) and runs of a letter collapsed, which is what audit.py
does. And a heading that is a long enough prefix or suffix of a name we
have is taken as that name, because the dump routinely keeps half of one:
"Ron OF ALERTNESS" is the rod, and "MANTA RAY" is the cloak. That last
rule can in principle hide a real gap whose name ends the same way as
something we have, so it is held to seven letters and no fewer.

What is left after that is a handful of headings the dump mangles past any
rule -- it reads "Rod" as "Ron" throughout, and drops enough of "MELF'S"
to leave "1S" -- and those are named one by one in MANGLED below, with the
entry each of them is. Every name in that table has to exist in data/, so
the table cannot outlive what it points at.

Anything still unmatched is reported. It is either an entry nobody wrote
down or a heading nobody has looked at yet, and only reading it tells you
which.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

SCHOOLS = ("abjuration|conjuration|divination|enchantment|evocation|"
           "illusion|necromancy|transmutation")

SPELL_LINE = re.compile(
    r"^\s*((%s)\s+cantrip|[1-9](st|nd|rd|th)-level\s+(%s))" % (SCHOOLS, SCHOOLS),
    re.I)

ITEM_LINE = re.compile(
    r"^(Armor|Weapon|Wondrous item|Wand|Staff|Rod|Ring|Potion|Scroll|"
    r"Ammunition)\b.*\b"
    r"(common|uncommon|very rare|rare|legendary|artifact|varies)\b", re.I)

# Below this, a prefix or suffix match is more likely to be a coincidence
# than a mangled heading.
PART = 7

# Headings this dump mangles past any rule, and what each one is. Checked
# against the data files, so an entry here that stops naming anything is a
# failure rather than a heading silently going unexamined.
MANGLED = {
    "IMMOVABLE Ron":         "Immovable Rod",
    "R o n OF SECURITY":     "Rod of Security",
    "Ron OF ALERTNESS":      "Rod of Alertness",
    "Ron oF LoRDLY MIGHT":   "Rod of Lordly Might",
    "TENTACLE Ron":          "Tentacle Rod",
    "TALISMAN OF PuRE Goon": "Talisman of Pure Good",
    "1S MINUTE METEORS":     "Melf's Minute Meteors",
    "1S SCORCHER":           "Aganazzar's Scorcher",
}


def squash(text):
    """Letters only, lowercased, with the dump's confusions folded.

    The same folding audit.py uses, and for the same reason: the text layer
    confuses c with g and i with l, sets some capitals as digits, and
    doubles or drops letters at line breaks.
    """
    t = re.sub(r"[^A-Za-z0-9]", "", text).lower()
    t = t.replace("c", "g").replace("i", "l").replace("1", "l")
    t = t.replace("0", "o")
    return re.sub(r"(.)\1+", r"\1", re.sub(r"[^a-z]", "", t))


def headings(path, opener):
    """Every heading in the file that sits above one of the opener lines."""
    lines = open(path, encoding="utf-8", errors="replace").read().split("\n")
    out = []
    for i, line in enumerate(lines):
        if not opener.match(line.strip()):
            continue
        j = i - 1
        while j >= 0 and not lines[j].strip():
            j -= 1
        if j < 0:
            continue
        head = lines[j].strip()
        if len(head) > 48 or head.startswith("===") or head.endswith("."):
            continue
        out.append(head)
    return out


def entered(key, have, heading):
    """Whether a heading names something in the data files."""
    if heading in MANGLED:
        return True
    if key in have:
        return True
    if len(key) < PART:
        return False
    return any(o.startswith(key) or o.endswith(key) for o in have)


def check(label, path, opener, ours):
    if not os.path.exists(path):
        print("  %-14s no %s" % (label, path))
        return 0
    have = {squash(o) for o in ours}
    found = headings(path, opener)
    missing = sorted({h for h in found if not entered(squash(h), have, h)})
    print("  %-14s %4d entries in the book, %d we do not have"
          % (label, len(found), len(missing)))
    for m in missing:
        print("        %s" % m)
    return len(missing)


def main():
    spells = [r.str(0) for r in read_file("spells.txt") if r.tag == "SPELL"]
    items = [r.str(0) for r in read_file("equipment.txt")
             if r.tag == "MAGICITEM"]

    print("spells")
    unknown = 0
    for book, name in (("PHB", "PHBtext.txt"), ("XGE", "XANATHARtext.txt"),
                       ("TCE", "TASHAtext.txt")):
        unknown += check(book, os.path.join(ROOT, "TextFiles", name),
                         SPELL_LINE, spells)

    print("\nmagic items")
    unknown += check("DMG", os.path.join(ROOT, "TextFiles", "DMGtext.txt"),
                     ITEM_LINE, items)

    # A mangled heading is only excused while the entry it stands for is
    # still there to excuse it.
    names = set(spells) | set(items)
    stale = sorted(v for v in MANGLED.values() if v not in names)
    for v in stale:
        print("\n  MANGLED names %r, which is in neither data file" % v)

    print("\n%d headings unmatched, %d dump manglings named and excused"
          % (unknown, len(MANGLED)))
    return 1 if (unknown or stale) else 0


if __name__ == "__main__":
    sys.exit(main())
