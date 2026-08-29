#!/usr/bin/env python3
"""Look for feats, invocations and infusions the books have and data/ lacks.

The same "other direction" as tools/verify_coverage.py and
tools/verify_equipment_coverage.py, for the three lists whose entries are
neither stat blocks nor priced rows: feats, eldritch invocations, artificer
infusions and the rest of the OPTION lists. tools/verify_feats.py and
tools/verify_options.py compare what we hold against the book; neither can
see something nobody typed in.

What these entries share is a line the books print in a fixed form and
almost nowhere else: "Prerequisite: ...". The name is the last non-blank
line above it. That is enough to enumerate what the books gate, and to
subtract what data/ holds under FEAT, OPTION and INFUSION.

It found Bond of the Talisman, the 12th-level warlock invocation Tashy's
adds alongside Protection and Rebuke of the Talisman -- the other two were
entered and this one was not, which is exactly the shape of gap that no
check in the other direction can report.

Matching is loose, because the text layer mangles headings: it sets spaces
inside words ("A g o n i z i n g B l a s t"), reads O as 0 and d as n
("0RCISH FURY", "Woon ELF MAGIC"). So comparison is on letters only,
lowercased. A heading that still does not match is reported rather than
counted, and the ones the dump has mangled past any rule are named in
MANGLED below, each with the entry it is -- and every name in that table
has to exist in data/, so the table cannot outlive what it points at.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file            # noqa: E402

BOOKS = [
    ("PHB", "PHBtext.txt"),
    ("XGE", "XANATHARtext.txt"),
    ("TCE", "TASHAtext.txt"),
]

# Headings the dump has mangled past any rule, and the entry each one is.
MANGLED = {
    "0RCISH FURY": "Orcish Fury",
    "Woon ELF MAGIC": "Wood Elf Magic",
}

# Lines that sit above a "Prerequisite:" without being the heading of one:
# a sentence the two-column layout has broken across the boundary, or the
# tail of the entry above. Each is prose, not a name.
NOT_A_HEADING = re.compile(
    r"^[a-z]"                       # a heading is not lower-case
    r"|\.$"                         # nor does one end in a full stop
    r"|^[^A-Za-z]*$"                # nor is one all punctuation
)


def letters(s):
    return re.sub(r"[^a-z]", "", s.lower())


def headings(path):
    """Every name the book prints immediately above a "Prerequisite:"."""
    lines = [l.strip() for l in
             open(path, encoding="utf-8", errors="replace")]
    found = set()
    for i, line in enumerate(lines):
        if not line.startswith("Prerequisite:"):
            continue
        for j in range(i - 1, max(0, i - 4) - 1, -1):
            if lines[j]:
                found.add(lines[j])
                break
    return found


def main():
    have = set()
    for r in read_file("character.txt"):
        if r.tag in ("FEAT", "INFUSION"):
            have.add(letters(r.str(0)))
        elif r.tag == "OPTION":
            have.add(letters(r.str(1)))

    for mangled, real in sorted(MANGLED.items()):
        if letters(real) not in have:
            print("  MANGLED names %s, which data/ does not have" % real)
            return 1
        have.add(letters(mangled))

    total = missing = 0
    for book, name in BOOKS:
        path = os.path.join(ROOT, "TextFiles", name)
        if not os.path.exists(path):
            print("  %s: %s is not here" % (book, name))
            continue
        gaps = []
        for head in sorted(headings(path)):
            total += 1
            if NOT_A_HEADING.search(head) or not (3 < len(head) < 40):
                continue
            if letters(head) in have:
                continue
            gaps.append(head)
        for g in gaps:
            print("  %s gates \"%s\", which data/ does not have" % (book, g))
        missing += len(gaps)

    print("\n%d gated entries in the books, %d not in data/"
          % (total, missing))
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
