#!/usr/bin/env python3
"""Check the dice in every spell description against the book's own entry.

data/spells.txt now carries a sentence or two on what each spell does, in
the project's own words. Wording is nobody's business but ours; the numbers
in it are the books'. A description that says a fireball deals 8d8 is not a
matter of style, and nothing else would ever catch it: tools/verify_spells.py
checks the row's fields and knows nothing about the prose, and the program
cannot tell a right number from a wrong one.

So every dice expression in a description -- 8d6, 1d4, 10d10 -- is looked
for in that spell's own entry in the book its row names, and so is every
saving throw it names: a description that says "a Dexterity save" has to be
describing a spell whose entry asks for one. That found maze, which asks for
no saving throw at all, and telekinesis, which is a contested check. Every
distance is checked the same way, so a 20-foot sphere cannot quietly become
a 30-foot one. The entries are
found the way verify_spells.py finds them, by their Casting Time line, and
this reuses that parser rather than writing a second one.

The extraction is lossy in a way that matters here: the digits 1, 0 and 5
come back as the letters l, O and S ("ld8", "SdlO", "ldl2"), and the PHB
writes bless as "roll a d4" rather than "1d4". Both are allowed for. What
is not allowed for is a die the entry does not mention at all, which is how
"Add 1d8 per slot" was found in a spell whose book says 1d6.
"""

import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from build_data import read_file
from verify_spells import BOOKS, OCR_HEADING, ROOT, entries, letters

DIE = re.compile(r"\b\d+d\d+\b")

ABILITIES = ("Strength", "Dexterity", "Constitution", "Intelligence",
             "Wisdom", "Charisma")

# "a 20-foot sphere", "30 feet long". The book writes the same measurement
# either way round, so both spellings are accepted for either.
FEET = re.compile(r"\b(\d+)[- ]foot\b")

# The other numbers a description states outright: what a spell heals, what
# its component costs, what it can lift.
QUANTITY = re.compile(
    r"\b(\d[\d,]*)\s+(?:temporary\s+)?(?:hit points|gp|pounds|charges)\b")

# How many lines past the last entry in a book to keep looking; a spell at
# the end of the chapter has no following entry to stop at.
TAIL = 200


def normalise(text):
    """The entry as the dice in it can be searched for.

    Spaces go (a die is set as "1d 6" often enough), and so do hyphens,
    which is how a word broken across two lines comes back ("Wis-dom saving
    throw"). The letters the extraction leaves in place of digits are put
    back. The substitution
    is only ever used to find a die that is already known, so a word that
    becomes nonsense on the way does no harm.
    """
    squashed = re.sub(r"[\s-]+", "", text).lower()
    return (squashed.replace("l", "1").replace("i", "1")
                    .replace("o", "0").replace("s", "5"))


def book_entries():
    """Every spell entry in every book, with its body text."""
    out = {}
    for tag, name in BOOKS.items():
        path = os.path.join(ROOT, "TextFiles", name)
        if not os.path.exists(path):
            sys.exit("verify_spell_text: no %s" % path)
        lines = open(path, encoding="utf-8").read().split("\n")
        ents = entries(path)
        for i, e in enumerate(ents):
            start = e["line"] - 1
            end = (ents[i + 1]["line"] - 1 if i + 1 < len(ents)
                   else min(len(lines), start + TAIL))
            e["body"] = normalise("\n".join(lines[start:end]))
        out[tag] = ents
    return out


def find(ents, name):
    """The one entry for this spell, or None."""
    k = OCR_HEADING.get(name) or letters(name)
    hits = [e for e in ents if k in e["names"]]
    if not hits:
        hits = [e for e in ents if k[1:] in e["names"]]
    return hits[0] if len(hits) == 1 else None


def main():
    ents = book_entries()
    rows = list(read_file("spells.txt"))
    book_of = dict((r.str(0), r.str(1)) for r in rows if r.tag == "SPELL")

    checked = dice_checked = bad = missing = 0
    saves_checked = [0]
    feet_checked = [0]
    quantities_checked = [0]
    for r in rows:
        if r.tag != "SPELLTEXT":
            continue
        name, text = r.str(0), r.str(1)
        tag = book_of.get(name)
        if tag is None:
            print("  %s: described but not a spell" % name)
            missing += 1
            continue
        dice = sorted(set(DIE.findall(text)))
        named_save = any(re.search(a + r"\s+save", text) for a in ABILITIES)
        if (not dice and not named_save and not FEET.search(text)
                and not QUANTITY.search(text)):
            continue
        entry = find(ents[tag], name)
        if entry is None:
            print("  %s (%s): no entry in the book text" % (name, tag))
            missing += 1
            continue

        checked += 1
        wrong = []
        for a in ABILITIES:
            if not re.search(a + r"\s+save", text):
                continue
            saves_checked[0] += 1
            if normalise(a + "sav") in entry["body"]:
                continue
            wrong.append("a " + a + " save")
        for v in sorted(set(FEET.findall(text))):
            feet_checked[0] += 1
            if (normalise(v + "foot") in entry["body"]
                    or normalise(v + "feet") in entry["body"]):
                continue
            wrong.append(v + " feet")
        for v in sorted(set(QUANTITY.findall(text))):
            quantities_checked[0] += 1
            if (normalise(v) in entry["body"]
                    or normalise(v.replace(",", "")) in entry["body"]):
                continue
            wrong.append(v)
        for d in dice:
            dice_checked += 1
            if d in entry["body"]:
                continue
            # "roll a d4 and add the number rolled" -- the PHB's way of
            # writing 1d4 in bless, guidance, resistance and bane.
            if d.startswith("1d") and ("a" + d[1:]) in entry["body"]:
                continue
            wrong.append(d)
        if wrong:
            bad += 1
            print("  %s (%s, %s:%d): %s in no part of the book's entry"
                  % (name, tag, BOOKS[tag], entry["line"], ", ".join(wrong)))

    print("\n%d dice, %d saving throws, %d distances and %d other numbers "
          "in %d spell descriptions checked against the books, %d disagree, "
          "%d not found"
          % (dice_checked, saves_checked[0], feet_checked[0],
             quantities_checked[0], checked, bad, missing))
    return 1 if (bad or missing) else 0


if __name__ == "__main__":
    sys.exit(main())
