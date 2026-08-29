#!/usr/bin/env python3
"""Check the GEM rows in data/equipment.txt against the DMG's treasure tables.

Six tables, one per value -- 10, 50, 100, 500, 1,000 and 5,000 gold -- and
what a gem is worth is the whole of what it does, so the value is the thing
worth checking. The name and the description are checked too, because a
stone in the wrong table reads perfectly well and is wrong by a factor of
five hundred.

The tables fight the reader in three ways, and all three are worked around
here rather than in the data:

  The die column and the row numbers are set apart from the stones, and the
  extraction interleaves them, so "Moss agate (translucent pink or
  yellow-white with" is followed by "9 10 11 12" and then by four
  descriptions in a row. Numbers standing alone inside a description are
  dropped rather than read.

  The column headings -- "d12", "Stone", "Description" -- lose their own
  first letters to the same interleaving and end up glued to the first
  stone of each table, as "Escription Black sapphire" and "Ne Description
  Black opal". Leading words that are header debris are stripped until a
  real name is left.

  Some initials arrive lowercase: jade, jet and jacinth. Comparing on
  letters with the case folded reads them as the stones they are.

Every gem in the data has to be in the book's table for its value, and
every gem in the book has to be in the data -- a missing row is the failure
that a check written only one way round would not see.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file            # noqa: E402

DUMP = os.path.join(ROOT, "TextFiles", "DMGtext.txt")

# The heading of each table, and what a stone in it is worth.
TABLES = [("10 GP GEMSTONES", 10), ("50 GP GEMSTONES", 50),
          ("100 GP GEMSTONES", 100), ("500 GP GEMSTONES", 500),
          ("1,000 GP GEMSTONES", 1000), ("5,000 GP GEMSTONES", 5000)]

# "Azurite (opaque mottled deep blue)" -- a name, then what it looks like.
STONE = re.compile(r"([A-Za-z][A-Za-z' -]{2,24})\s*\(([^)]{4,140})\)")

# What is left of "d12 Stone Description" after the columns interleave.
# The headings lose letters off the front as well as the back, so the
# leftovers turn up as "Escription", "escription" and "Ne" -- anything that
# is a tail of one of the heading words counts, whatever its case.
HEADINGS = ("description", "stone")


def is_debris(word):
    w = word.lower()
    if re.fullmatch(r"d\s*l?\d*", w):
        return True
    if len(w) <= 2 and w.isalpha():
        return True
    return any(h.endswith(w) and len(w) >= 3 for h in HEADINGS)


def squash(s):
    return re.sub(r"[^a-z]", "", s.lower())


def tidy(name, desc):
    words = name.strip().split()
    while len(words) > 1 and is_debris(words[0]):
        words.pop(0)
    name = " ".join(words)
    desc = re.sub(r"\s*\b\d{1,2}\b\s*", " ", desc)      # stray row numbers
    desc = desc.replace("·", "").replace(" ,", ",")
    return name, re.sub(r"\s+", " ", desc).strip()


def book():
    """Every gem the DMG prints, as {squashed name: (value, description)}."""
    lines = [l.rstrip("\n") for l in open(DUMP, encoding="utf-8")]
    starts = []
    for i, l in enumerate(lines):
        for head, value in TABLES:
            if l.strip() == head:
                starts.append((i, value, head))
    starts.sort()

    out = {}
    for n, (i, value, head) in enumerate(starts):
        end = starts[n + 1][0] if n + 1 < len(starts) else i + 90
        blob = re.sub(r"\s+", " ", " ".join(lines[i + 1:end]))
        for raw_name, raw_desc in STONE.findall(blob):
            name, desc = tidy(raw_name, raw_desc)
            if not name or squash(name) in ("stone", "description"):
                continue
            out[squash(name)] = (value, desc, name, head)
    return out


def main():
    have = book()
    rows = [r for r in read_file("equipment.txt") if r.tag == "GEM"]
    problems = []
    seen = set()

    for r in rows:
        name, value, desc = r.str(0), r.int(1), r.str(2)
        key = squash(name)
        seen.add(key)
        if key not in have:
            problems.append("%s is not in any of the book's gem tables"
                            % name)
            continue
        want_value, want_desc, book_name, head = have[key]
        if want_value != value:
            problems.append("%s is worth %d gp, and the book prints it in "
                            "the %s table" % (name, value, head))
        if squash(desc) != squash(want_desc):
            problems.append("%s reads %r, the book %r"
                            % (name, desc, want_desc))

    for key, (value, desc, book_name, head) in sorted(have.items()):
        if key not in seen:
            problems.append("the book has %s at %d gp and we do not"
                            % (book_name, value))

    for p in problems:
        print("  %s" % p)
    print("\n%d gemstones checked against the DMG, %d disagree"
          % (len(rows), len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
