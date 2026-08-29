#!/usr/bin/env python3
"""Check every name in data/ against the book text in TextFiles/.

The data files under data/ are hand-written and are the only source the
program is built from. TextFiles/ holds the books' text, and is kept for
exactly this: looking each name up in the book it claims to come from and
reporting what cannot be found.

Matching stays tolerant even though the text is now clean, because the
typesetting still gets in the way: words are set with a space inside them,
small capitals come back as mixed case, a section opening with a decorative
initial loses that letter entirely, and a dozen ligatures do not survive. So
names are compared on their letters alone, with the pairs the older OCR used
to confuse still folded together -- it costs nothing and it is what makes a
miss meaningful. A name that still cannot be found is either absent from the
source or wrong in the data, and either way a person needs to look at it.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file          # noqa: E402  (same record parser)
from verify_equipment import ALIAS        # noqa: E402

BOOKS = {
    "PHB":  "TextFiles/PHBtext.txt",
    "XGE":  "TextFiles/XANATHARtext.txt",
    "TCE":  "TextFiles/TASHAtext.txt",
    "DMG":  "TextFiles/DMGtext.txt",
    "MPMM": "TextFiles/MORDENKAIDENtext.txt",
    "MM":   "TextFiles/MMtext.txt",
    "SCAG": "TextFiles/SCAGtext.txt",
}

# What to check: the file, the record tag, which field holds the name, which
# holds the source book (None when the record does not carry one, in which
# case the name is looked for in every dump), and a label for the report.
CHECKS = [
    ("character.txt", "LANGUAGE",   0, 1,    "languages"),
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


# Names that are ours rather than the book's, with the reason. These are
# reported apart from the real misses so that a genuine one stands out
# instead of arriving in a crowd of expected ones.
OURS = {
    # The Priest's Pack names "2 blocks of incense"; the singular a row has
    # to be named by is ours. The other six of these pack-only items appear
    # in the book's prose in the singular and resolve on their own.
    "Block of incense":
        "our singular for the Priest's Pack's \"2 blocks of incense\"",
    "Standard Human":
        "the PHB prints the plain human with no heading of its own",
    "Extra Elemental Discipline":
        "our name for the extra disciplines the Four Elements monk gains",
    "Inspiring Help improves": "our name for a feature's later upgrade",
    "Extra Attack improves": "our name for a feature's later upgrade",
    "Indomitable improves": "our name for a feature's later upgrade",
    "Second Wind improves": "our name for a feature's later upgrade",
    # SCAG allows Feral alongside any one of the three variants it does call
    # mutually exclusive, so those pairings are offered as subraces of their
    # own. The book names the halves, not the pairs.
    "Feral Devil's Tongue":
        "our name for two SCAG tiefling variants taken together",
    "Feral Hellfire":
        "our name for two SCAG tiefling variants taken together",
    "Feral Winged":
        "our name for two SCAG tiefling variants taken together",
}


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
    ours = {}
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
            if name in OURS:
                ours.setdefault(name, OURS[name])
                continue
            needle = squash(name)
            if len(needle) < 4:
                continue                      # too short to match usefully

            # What to look for. Besides the name itself: the name the book
            # gives it where ours reads better in a menu; the part after a
            # colon, for the features we name compositely ("Channel
            # Divinity: Sacred Weapon"); and the name without its first
            # letter, because a section opening with a decorative initial
            # leaves that letter out of the text layer entirely, which is
            # how Quickened Healing is set as "UICKENED HEALING".
            tries = [needle]
            if name in ALIAS:
                tries.append(squash(ALIAS[name]))
            if ": " in name:
                tries.append(squash(name.split(": ", 1)[1]))
            tries.append(needle[1:])

            # Check the book it claims first, then every other dump: a name
            # attested anywhere is at least real, even if it is tagged to
            # the wrong book.
            order = ([book] if book in haystacks else []) + \
                    [b for b in haystacks if b != book]
            head = squash(re.split(r"[,(]", name)[0])
            found = None
            for b in order:
                hay = haystacks[b]
                if any(t in hay for t in tries) \
                        or (len(head) >= 5 and head in hay):
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

    if ours:
        print("\nNames of our own, not the book's:")
        for name, why in sorted(ours.items()):
            print("    %-30s %s" % (name, why))

    print("\n%d names checked, %d could not be found" % (total, missing_total))
    return 0


if __name__ == "__main__":
    sys.exit(main())
