#!/usr/bin/env python3
"""Rewrite the DEITY rows in data/world.txt from appendix B.

Appendix B is four columns -- deity and title, alignment, suggested domains,
symbol. Those columns were written out by hand, and comparing them against
the extracted text found fifty-five rows that disagreed with the book: some
only in spelling, but some outright, with Tyr carrying the wrong title,
alignment, domains and symbol all four.

Rather than patch them one at a time, this reads the tables and writes the
rows. Run it once; `make verify` then keeps them honest.

The headings are set in spaced-out capitals, which is what marks a table's
start, and a row is recognised by its alignment cell: the line before it is
the deity and its title, the line after the domains, and everything up to the
next deity is the symbol.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)

DUMP = os.path.join(ROOT, "TextFiles", "PHBtext.txt")
WORLD = os.path.join(ROOT, "data", "world.txt")

ALIGNMENTS = {"LG", "NG", "CG", "LN", "N", "CN", "LE", "NE", "CE"}
DOMAINS = {"Knowledge", "Life", "Light", "Nature", "Tempest", "Trickery",
           "War", "Death", "None"}

# The three moons of Krynn have no clerics, and the book says so in the
# domain column rather than leaving it blank.
NO_CLERICS = "no clerics"

# The column headers, which repeat above every sub-heading ("The Gods of
# Neutrality", "The Dark Six") and would otherwise be read as part of the
# symbol of the row above.
HEADER_CELLS = {"Deity", "Alignment", "Suggested Domains", "Symbol"}

# Which pantheon a row belongs to is decided by the deity that opens each
# table, not by its heading: the Celtic table has no heading in the text
# layer at all, and Dragonlance and Eberron split theirs across sub-headings
# ("The Gods of Good", "The Dark Six"). A table runs from its first deity to
# the next table's. Each anchor is checked to appear exactly once, in this
# order, so a mis-parse cannot silently re-file a whole pantheon.
TABLES = [
    ("Auril",        "Forgotten Realms"),
    ("Beory",        "Greyhawk"),
    ("Paladine",     "Dragonlance"),
    ("Arawai",       "Eberron"),
    ("Bahamut",      "Nonhuman"),
    ("The Daghdha",  "Celtic"),
    ("Zeus",         "Greek"),
    ("Re-Horakhty",  "Egyptian"),
    ("Odin",         "Norse"),
]


def clean(s):
    """Undo what the typesetting does to a line of the text layer.

    The books set some words with a space inside them, so "god o f war" and
    "Deities o f Greyhawk" both occur; and they use curly quotes and dashes.
    """
    s = s.replace("’", "'").replace("‘", "'")
    s = s.replace("—", "-").replace("–", "-")
    s = re.sub(r"\bo f\b", "of", s)
    return re.sub(r"\s+", " ", s).strip()


def squash(s):
    return re.sub(r"[^a-z]", "", s.lower())


def rows():
    """Every deity in appendix B, in the order the tables print them."""
    lines = [clean(l) for l in open(DUMP, encoding="utf-8")]
    out = []

    for i, line in enumerate(lines):
        if line not in ALIGNMENTS or i < 2:
            continue

        head = lines[i - 1]
        if not head or len(head) > 70:
            continue
        # A long name wraps onto a second line, leaving the comma that
        # separates a deity from its title on the line above:
        #     "Hathor, goddess of love, music, and " / "motherhood"
        if "," not in head and "," in lines[i - 2]:
            head = lines[i - 2] + " " + head
        if "," not in head:
            continue
        if i + 1 >= len(lines):
            continue
        # Three domains do not fit the column, so they wrap too, leaving a
        # trailing comma behind: "Death, Tempest," / "Trickery".
        domains = lines[i + 1]
        skip = 0
        if domains.endswith(",") and i + 2 < len(lines):
            domains = domains + " " + lines[i + 2]
            skip = 1
        if domains == NO_CLERICS:
            domains = "None"
        elif any(p.strip() not in DOMAINS for p in domains.split(",")):
            continue

        symbol, j = [], i + 2 + skip
        while j < len(lines) and j < i + 6:
            nxt = lines[j]
            # Stop at a blank, at the next row's alignment, at the deity
            # line that precedes one, and at a spaced-out heading.
            if not nxt or nxt in ALIGNMENTS or nxt in HEADER_CELLS:
                break
            # A sub-heading is a line whose next line is the Alignment
            # column header.
            if j + 1 < len(lines) and lines[j + 1] == "Alignment":
                break
            if len(re.findall(r"\b\w\b", nxt)) >= 4:
                break
            if j + 1 < len(lines) and lines[j + 1] in ALIGNMENTS:
                break
            if j + 2 < len(lines) and lines[j + 2] in ALIGNMENTS \
                    and "," in nxt:
                break
            symbol.append(nxt)
            j += 1

        name, _, title = head.partition(",")
        # The deity column sets a capital I as a lowercase l, which turns
        # Iuz into luz. Nothing in appendix B starts with a lowercase letter.
        name = name.strip()
        if name[:1].islower():
            name = "I" + name[1:]
        out.append([name, title.strip(), None, line, domains,
                    " ".join(symbol)])

    # File each row under the table it falls in.
    starts = []
    for anchor, pantheon in TABLES:
        where = [n for n, r in enumerate(out) if r[0] == anchor]
        if len(where) != 1:
            sys.exit("%r opens no table, or more than one (%d matches)"
                     % (anchor, len(where)))
        starts.append((where[0], pantheon))
    if starts != sorted(starts):
        sys.exit("the tables did not come out in the printed order")

    for n, (at, pantheon) in enumerate(starts):
        end = starts[n + 1][0] if n + 1 < len(starts) else len(out)
        for r in out[at:end]:
            r[2] = pantheon
    return [tuple(r) for r in out if r[2]]


def main():
    if not os.path.exists(DUMP):
        sys.exit("extract_deities: no %s" % DUMP)

    found = rows()
    counts = {}
    for r in found:
        counts[r[2]] = counts.get(r[2], 0) + 1
    for _, p in TABLES:
        print("%-18s %3d" % (p, counts.get(p, 0)))
    print("%-18s %3d" % ("total", len(found)))

    if len(found) < 180:
        sys.exit("only %d deities parsed; refusing to write" % len(found))

    text = open(WORLD, encoding="utf-8").read()
    block = "\n".join("DEITY|%s|%s|%s|%s|%s|%s" % r for r in found)
    new, n = re.subn(r"(?m)^DEITY\|.*(?:\n^DEITY\|.*)*$", block, text, count=1)
    if n != 1:
        sys.exit("could not find the DEITY block in data/world.txt")
    open(WORLD, "w", encoding="utf-8").write(new)
    print("\nwrote %d DEITY rows into data/world.txt" % len(found))


if __name__ == "__main__":
    main()
