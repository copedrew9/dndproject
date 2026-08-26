#!/usr/bin/env python3
"""Check every name in the hand-encoded tables against the book dumps.

The generated tables (spells, beasts, life events) validate themselves when
they are built. Everything else was typed in, so this looks each name up in
the dump it should have come from and reports what it cannot find.

Matching has to be tolerant, because the dumps are lossy in known ways: they
confuse C with G and I with L, lose spaces inside words and around
punctuation, and render small capitals as mixed case. So names are compared
on their letters alone, with those pairs folded together. A name that still
cannot be found is either absent from the source or wrong in the code, and
either way a person needs to look at it.
"""
import re
import sys

BOOKS = {
    "BOOK_PHB":  "TextFiles/PHBtext.txt",
    "BOOK_XGE":  "TextFiles/XANATHARtext.txt",
    "BOOK_TCE":  "TextFiles/TASHAtext.txt",
    "BOOK_DMG":  "TextFiles/DMGtext.txt",
    "BOOK_MPMM": "TextFiles/MORDENKAIDENtext.txt",
    "BOOK_MM":   "TextFiles/MMtext.txt",
}

# Tables to check: file, and how to read a row's name and its book.
TABLES = [
    ("src/data_races.c",        "races and subraces"),
    ("src/data_classes.c",      "classes"),
    ("src/data_backgrounds.c",  "backgrounds"),
    ("src/data_feats.c",        "feats"),
    ("src/data_equipment.c",    "equipment"),
    ("src/data_magicitems.c",   "magic items"),
    ("src/data_magicrules.c",   "magic item rules"),
    ("src/data_infusions.c",    "artificer infusions"),
    ("src/data_classoptions.c", "class option lists"),
    ("src/data_deities.c",      "deities"),
    ("src/data_itemtext.c",     "item descriptions"),
    ("src/data_optional.c",     "Tasha's optional features"),
    ("src/data_sidekicks.c",    "sidekick features"),
    ("src/data_subclasses.c",   "subclasses"),
]

# Most tables put the source book second; some -- infusions, deities, item
# notes -- do not carry one at all, so the name is taken on its own and
# checked against every dump.
ROW_WITH_BOOK = re.compile(r'^\{\s*"((?:[^"\\]|\\.)*)"\s*,\s*(BOOK_[A-Z]+)', re.M)
ROW_ANY = re.compile(r'^\s*\{\s*"((?:[^"\\]|\\.)*)"', re.M)
# Subclasses lead with a level and a book; sidekick features with a class
# and a level, before the name.
ROW_LEVEL_BOOK = re.compile(
    r'^\s*\{\s*\d+\s*,\s*(BOOK_[A-Z]+)\s*,\s*"((?:[^"\\]|\\.)*)"', re.M)
ROW_ENUM_LEVEL = re.compile(
    r'^\s*\{\s*(SK_[A-Z]+)\s*,\s*\d+\s*,\s*"((?:[^"\\]|\\.)*)"', re.M)


def squash(text):
    """Letters only, lowercased, with the dump's known confusions folded."""
    t = re.sub(r"[^A-Za-z]", "", text).lower()
    t = t.replace("c", "g").replace("i", "l").replace("1", "l")
    # The dumps double or drop letters at line breaks; collapse runs.
    return re.sub(r"(.)\1+", r"\1", t)


def load(path):
    raw = open(path, encoding="utf-8", errors="replace").read()
    return squash(raw)


def main():
    haystacks = {}
    for book, path in BOOKS.items():
        try:
            haystacks[book] = load(path)
        except OSError:
            print("cannot read %s" % path, file=sys.stderr)
            return 1

    total, missing_total = 0, 0
    report = []

    for path, label in TABLES:
        try:
            src = open(path, encoding="utf-8").read()
        except OSError:
            continue
        rows = ROW_WITH_BOOK.findall(src)
        if not rows:
            rows = [(n, b) for b, n in ROW_LEVEL_BOOK.findall(src)]
        if not rows:
            rows = [(n, None) for _, n in ROW_ENUM_LEVEL.findall(src)]
        if not rows:
            rows = [(n, None) for n in ROW_ANY.findall(src)]
        misses = []
        for name, book in rows:
            total += 1
            if book == "BOOK_HOMEBREW":
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
                               % (found.replace("BOOK_", ""),
                                  book.replace("BOOK_", ""))))
        missing_total += len(misses)
        report.append((label, path, len(rows), misses))

    for label, path, n, misses in report:
        print("%-22s %3d rows, %d not found in the dumps" %
              (label, n, len(misses)))
        for name, book, why in misses:
            print("    %-44s %-10s %s" % (name[:44], book, why))

    print("\n%d names checked, %d could not be found" % (total, missing_total))
    return 0


if __name__ == "__main__":
    sys.exit(main())
