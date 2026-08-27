#!/usr/bin/env python3
"""Check every name in data/ against the book dumps in TextFiles/.

The data files under data/ are hand-written and are the only source the
program is built from. TextFiles/ holds OCR dumps of the books, and is kept
for exactly this: looking each name up in the book it claims to come from
and reporting what cannot be found.

Matching has to be tolerant, because the dumps are lossy in known ways: they
confuse C with G and I with L, lose spaces inside words and around
punctuation, and render small capitals as mixed case. So names are compared
on their letters alone, with those pairs folded together. A name that still
cannot be found is either absent from the source or wrong in the data, and
either way a person needs to look at it.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file          # noqa: E402  (same record parser)

BOOKS = {
    "PHB":  "TextFiles/PHBtext.txt",
    "XGE":  "TextFiles/XANATHARtext.txt",
    "TCE":  "TextFiles/TASHAtext.txt",
    "DMG":  "TextFiles/DMGtext.txt",
    "MPMM": "TextFiles/MORDENKAIDENtext.txt",
    "MM":   "TextFiles/MMtext.txt",
}

# What to check: the file, the record tag, which field holds the name, which
# holds the source book (None when the record does not carry one, in which
# case the name is looked for in every dump), and a label for the report.
CHECKS = [
    ("character.txt", "RACE",       0, 1,    "races"),
    ("character.txt", "SUBRACE",    1, 2,    "subraces"),
    ("character.txt", "CLASS",      0, 1,    "classes"),
    ("character.txt", "SUBCLASS",   1, 2,    "subclasses"),
    ("character.txt", "FEATURE",    3, None, "class features"),
    ("character.txt", "OPTFEATURE", 3, 1,    "Tasha's optional features"),
    ("character.txt", "INFUSION",   0, None, "artificer infusions"),
    ("character.txt", "OPTION",     1, 2,    "class option lists"),
    ("character.txt", "BACKGROUND", 0, 1,    "backgrounds"),
    ("character.txt", "FEAT",       0, 1,    "feats"),
    ("equipment.txt", "ITEM",       0, 1,    "equipment"),
    ("equipment.txt", "ITEMNOTE",   0, None, "item descriptions"),
    ("equipment.txt", "MAGICITEM",  0, 1,    "magic items"),
    ("equipment.txt", "MAGICRULE",  0, None, "magic item rules"),
    ("equipment.txt", "LIFESTYLE",  0, None, "lifestyles"),
    ("spells.txt",    "SPELL",      0, 1,    "spells"),
    ("world.txt",     "DEITY",      0, None, "deities"),
    ("world.txt",     "BEAST",      0, None, "beasts"),
    ("world.txt",     "SIDEKICKFEATURE", 2, None, "sidekick features"),
]


def squash(text):
    """Letters only, lowercased, with the dump's known confusions folded."""
    t = re.sub(r"[^A-Za-z]", "", text).lower()
    t = t.replace("c", "g").replace("i", "l").replace("1", "l")
    # The dumps double or drop letters at line breaks; collapse runs.
    return re.sub(r"(.)\1+", r"\1", t)


def main():
    haystacks = {}
    for book, path in BOOKS.items():
        full = os.path.join(ROOT, path)
        try:
            haystacks[book] = squash(
                open(full, encoding="utf-8", errors="replace").read())
        except OSError:
            print("cannot read %s" % path, file=sys.stderr)
            return 1

    files = {}
    total, missing_total, report = 0, 0, []

    for filename, tag, name_at, book_at, label in CHECKS:
        if filename not in files:
            files[filename] = read_file(filename)
        rows = [r for r in files[filename] if r.tag == tag]
        misses = []
        for r in rows:
            total += 1
            name = r.str(name_at)
            book = r.str(book_at) if book_at is not None else None
            if book == "Homebrew":
                continue
            needle = squash(name)
            if len(needle) < 4:
                continue                      # too short to match usefully

            # Check the book it claims first, then every other dump: a name
            # attested anywhere is at least real, even if it is tagged to
            # the wrong book.
            order = ([book] if book in haystacks else []) + \
                    [b for b in haystacks if b != book]
            head = squash(re.split(r"[,(]", name)[0])
            found = None
            for b in order:
                hay = haystacks[b]
                if needle in hay or (len(head) >= 5 and head in hay):
                    found = b
                    break
            if found is None:
                misses.append((name, book or "-", "not found in any dump"))
            elif book and found != book:
                misses.append((name, book, "found in %s, not %s"
                               % (found, book)))
        missing_total += len(misses)
        report.append((label, len(rows), misses))

    for label, n, misses in report:
        print("%-26s %4d rows, %d not found in the dumps"
              % (label, n, len(misses)))
        for name, book, why in misses:
            print("    %-44s %-10s %s" % (name[:44], book, why))

    print("\n%d names checked, %d could not be found" % (total, missing_total))
    return 0


if __name__ == "__main__":
    sys.exit(main())
