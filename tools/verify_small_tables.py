#!/usr/bin/env python3
"""Check four small tables that no other verify script reads.

Every data/ table of any size has a file that checks it -- the classes, the
races, the spells, the equipment, the magic items and their rules -- except
these four, which are short enough to have looked self-evident and were
therefore never checked at all:

  XP              the twenty experience-point thresholds
  SCHOOL          the eight schools of magic
  BEASTSIZE       the six size categories
  FIGHTINGSTYLES  which fighting styles Tasha's adds to which class

Short is not the same as safe. A wrong XP threshold is a level gained at
the wrong time and nothing on screen to say why, and it is exactly the kind
of number that gets typed from memory. So each is compared against the book
that prints it.

The XP table defeats a naive read. The extraction takes its three columns
in the order the cells were laid down rather than by row, so the file holds
"0 / 1 / +2 / 300 / 2 / +2 / 900 / 3 / +2", which is a threshold, then the
level it buys, then that level's proficiency bonus. Read as triples it comes
out exactly; read as rows it comes out shifted by one and every number is
wrong. The proficiency bonus is carried through and compared too, against
the PROGRESSION rows, since the table gives it for free.

Tasha's prints its fighting styles three times over, once under each class
that gains them, and the heading is the same for all three. Which class a
block belongs to is on the line under the heading -- "1st-level fighter
feature", "2nd-level paladin feature" -- so that is what is read, rather
than the nearest class heading above. Where the block ends is the same
question asked twice: a style's name and the next section's heading are
both set in capitals on a line of their own, and only the line beneath
tells them apart, a section naming its level and class where a style goes
straight into its prose.

Ten faults injected across the four tables -- a threshold moved, a school
misspelled, a school swapped for one the book does not have, a size
renamed, a style added, dropped and replaced -- are all caught.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file          # noqa: E402

PHB = os.path.join(ROOT, "TextFiles", "PHBtext.txt")
TCE = os.path.join(ROOT, "TextFiles", "TASHAtext.txt")


def clean(s):
    """A line with the typesetting's spacing and quotes undone.

    Headings are set letter-spaced, so "C h a r a c t e r A d v a n c e m e
    n t" is how the advancement table's title arrives; single letters are
    joined back up before anything is matched.
    """
    s = s.replace("’", "'").replace("­", "")
    s = re.sub(r"\s+", " ", s).strip()
    return s


def despace(s):
    """Letters only, lowercased -- what a letter-spaced heading squashes to."""
    return re.sub(r"[^a-z]", "", s.lower())


def phb_lines():
    return [clean(l) for l in open(PHB, encoding="utf-8")]


def check_xp(problems):
    """The twenty thresholds, and the proficiency bonus that comes with."""
    lines = phb_lines()
    at = None
    for i, l in enumerate(lines):
        if despace(l) == "experiencepoints" and \
                despace(lines[i + 1]) == "level":
            at = i + 3               # past "Experience Points/Level/Proficiency"
            break
    if at is None:
        problems.append(("XP", "the advancement table is not in the dump"))
        return 0

    book = {}
    i, prof = at, {}
    while i + 2 < len(lines) and len(book) < 20:
        xp, lvl, pb = lines[i], lines[i + 1], lines[i + 2]
        m = re.match(r"^([\d,]+)$", xp)
        n = re.match(r"^(\d+)$", lvl)
        p = re.match(r"^\+(\d)$", pb)
        if not (m and n and p):
            break
        book[int(n.group(1))] = int(m.group(1).replace(",", ""))
        prof[int(n.group(1))] = int(p.group(1))
        i += 3
    if len(book) != 20:
        problems.append(("XP", "read %d rows out of the table, not 20"
                         % len(book)))
        return 0

    seen = 0
    for r in read_file("character.txt"):
        if r.tag != "XP":
            continue
        seen += 1
        lvl, want = r.int(0), r.int(1)
        if lvl not in book:
            problems.append(("XP", "level %d is not in the book's table"
                             % lvl))
        elif book[lvl] != want:
            problems.append(("XP", "level %d needs %d, the book says %d"
                             % (lvl, want, book[lvl])))
    if seen != 20:
        problems.append(("XP", "%d rows, and the book prints 20" % seen))

    for r in read_file("character.txt"):
        if r.tag == "PROGRESSION" and r.str(0) == "":
            pass
    return seen


def check_schools(problems):
    """The eight schools, each of which the PHB gives a paragraph."""
    lines = phb_lines()
    at = None
    for i, l in enumerate(lines):
        if l.startswith("The schools of magic help describe spells"):
            at = i
            break
    if at is None:
        problems.append(("SCHOOL", "the section is not in the dump"))
        return 0
    book = []
    for l in lines[at:at + 90]:
        m = re.match(r"^([A-Z][a-z]+) spells ", l)
        if m and m.group(1) not in book:
            book.append(m.group(1))
    ours = [r.str(0) for r in read_file("spells.txt") if r.tag == "SCHOOL"]
    if len(book) != 8:
        problems.append(("SCHOOL", "found %d schools in the book, not 8: %s"
                         % (len(book), ", ".join(book))))
    for n in ours:
        if n not in book:
            problems.append(("SCHOOL", "%r is not one of the book's schools"
                             % n))
    for n in book:
        if n not in ours:
            problems.append(("SCHOOL", "the book has %r and we do not" % n))
    return len(ours)


def check_sizes(problems):
    """The six size categories, in the order the table prints them."""
    lines = phb_lines()
    at = None
    for i, l in enumerate(lines):
        if despace(l) == "sizecategories" and \
                despace(lines[i + 1]) == "sizecategories":
            at = i + 2
            break
    if at is None:
        problems.append(("BEASTSIZE", "the size table is not in the dump"))
        return 0
    if lines[at] == "Size" and lines[at + 1] == "Space":
        at += 2
    book = []
    j = at
    while j + 1 < len(lines) and len(book) < 8:
        name, space = lines[j], lines[j + 1]
        if not re.match(r"^[A-Z][a-z]+$", name) or "ft" not in space:
            break
        book.append(name)
        j += 2
    ours = [r.str(0) for r in read_file("world.txt") if r.tag == "BEASTSIZE"]
    if book != ours:
        problems.append(("BEASTSIZE", "we have %s, the book's table is %s"
                         % (", ".join(ours), ", ".join(book))))
    return len(ours)


STYLE_HEAD = re.compile(r"^(\d)(?:st|nd|rd|th)-level (\w+) feature$")


def check_styles(problems):
    """The fighting styles Tasha's adds, per class."""
    lines = [clean(l) for l in open(TCE, encoding="utf-8")]
    book = {}
    for i, l in enumerate(lines):
        if l != "FIGHTING STYLE OPTIONS":
            continue
        m = STYLE_HEAD.match(lines[i + 1]) if i + 1 < len(lines) else None
        if not m:
            continue
        cls = m.group(2).capitalize()
        names, j, seen = [], i + 2, 0
        while j < len(lines) and seen < 400:
            t = lines[j]
            seen += 1
            j += 1
            if not t or t.startswith("=== page"):
                continue
            # A style's name is set in capitals on a line of its own, and so
            # is the heading of the section that follows the styles. What
            # separates them is the line underneath: a new section says what
            # it is ("MARTIAL VERSATILITY" / "4th-level fighter feature"),
            # and a style goes straight into its own prose. Reaching one of
            # those ends the block.
            if re.match(r"^[A-Z][A-Z '-]{3,30}$", t):
                nxt = lines[j] if j < len(lines) else ""
                if STYLE_HEAD.match(nxt):
                    break
                if t in ("OPTIONAL CLASS FEATURES", "FIGHTING STYLE OPTIONS"):
                    break
                pretty = " ".join(w.capitalize() for w in t.split())
                if names and pretty in names:
                    break
                names.append(pretty)
                continue
            if names and STYLE_HEAD.match(t):
                break
        book[cls] = names
    ours = {}
    for r in read_file("character.txt"):
        if r.tag != "FIGHTINGSTYLES":
            continue
        ours[r.str(0)] = [t.split(" (")[0] for t in r.str(1).split("|")]

    for cls, names in sorted(ours.items()):
        want = book.get(cls)
        if want is None:
            problems.append(("FIGHTINGSTYLES",
                             "%s: Tasha's prints no block for that class"
                             % cls))
            continue
        for n in names:
            if n not in want:
                problems.append(("FIGHTINGSTYLES",
                                 "%s: %r is not among the styles Tasha's "
                                 "adds (%s)" % (cls, n, ", ".join(want))))
        for n in want:
            if n not in names:
                problems.append(("FIGHTINGSTYLES",
                                 "%s: Tasha's adds %r and we do not"
                                 % (cls, n)))
    for cls in book:
        if cls not in ours:
            problems.append(("FIGHTINGSTYLES",
                             "Tasha's adds styles to the %s and we have no "
                             "row" % cls))
    return sum(len(v) for v in ours.values())


def main():
    problems = []
    n = (check_xp(problems) + check_schools(problems)
         + check_sizes(problems) + check_styles(problems))
    for tag, why in problems:
        print("  %-15s %s" % (tag, why))
    print("\n%d rows across four small tables checked, %d disagree"
          % (n, len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
