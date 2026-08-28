#!/usr/bin/env python3
"""Check the added and expanded spell lists in data/character.txt.

Two tables name spells rather than describing them, and nothing read
either. tools/verify_spells.py checks the 477 spells themselves and
tools/verify_coverage.py looks for spells the books have that we do not,
but neither notices when a class is handed a spell it should not have, or
loses one it should.

  ADDSPELLS      the spells Tasha's adds to each of eight class lists
  OPTIONSPELLS   the spells a subclass gains at fixed levels: the Circle
                 of the Land's eight terrains and the Genie's four kinds

Both are compared with the spelling squashed to letters, because this is
where the extraction is at its worst. The Handbook's circle spell tables
set a space inside a word often enough that "freedom of movement" arrives
as "freedom o f movement" and "commune with nature" as "com m une with
nature"; matching on letters alone makes those the same word and would
still refuse "cone of cold" against "wall of stone".

Tasha's marks a spell that is new in that book with an asterisk and a
ritual with a tag, and the extraction turns both into whatever punctuation
it felt like -- "Intellect fortress'", "Dream of the blue veil''",
"Summonfe('". Everything from the first bracket or apostrophe is cut before
the name is compared, which is also what makes "Rary's telepathic bond
(ritual)" compare as the spell it is.

Five rows are reported unchecked rather than passed, all for the same
reason: a table set in more than one column, which the extraction takes a
line at a time across all of them.

The Genie's is four columns wide and split across a page break as well, so
"1st / detect evil and good / sanctuary / thunderwave / burning hands / fog
cloud" runs five spells together across four columns and the remaining rows
arrive after the headings that were meant to sit above them. The wizard's
is two columns, which is subtler and worth detecting rather than listing:
"Summon shadow" / "Booming blade (evoc.)" / "spawn* (conj.)" strands the
tail of every cell that wraps between two cells of the other column. A
stranded tail opens with a lowercase letter, which no cell of these tables
does, and that is what marks a list unreadable rather than wrong. The other
six read cleanly and are compared in full.

Seven faults injected -- a spell dropped from a list, a spell added, a
spell swapped for a similar one, a spell added to a list read through a
different path, a circle spell changed at two different levels, a level
changed -- are all caught. So is a wrong entry in MANGLED: pointing one at
a spell no row has fails loudly rather than quietly excusing a real
disagreement.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file            # noqa: E402

PHB = os.path.join(ROOT, "TextFiles", "PHBtext.txt")
TCE = os.path.join(ROOT, "TextFiles", "TASHAtext.txt")

# Subclasses whose table the extraction scrambles past reading.
NO_TABLE = {
    "The Genie": "its Expanded Spells table is four columns wide, and the "
                 "extraction interleaves the cells and splits them across a "
                 "page break",
}

FEATURE_LINE = re.compile(r"^\d+(?:st|nd|rd|th)-level \w+ feature$")

# Cells the extraction chews past matching on letters, and the spell each
# one is. The ranger's summon fey loses both its space and its last letter
# at the end of a line; the warlock's green-flame blade loses its hyphen
# and has its "fl" ligature read as "jl". Every spell named here has to be
# in a row of ours, so an entry cannot outlive the cell it excuses.
MANGLED = {
    "Summonfe": "Summon fey",
    "Greenjlame blade": "Green-flame blade",
}


def clean(s):
    s = s.replace("’", "'").replace("­", "")
    return re.sub(r"\s+", " ", s).strip()


def squash(s):
    """A spell name reduced to its letters.

    "freedom o f movement" and "freedom of movement" are the same spell,
    and the only difference between them is where the typesetting put a
    space.
    """
    return re.sub(r"[^a-z]", "", s.lower())


def spell_name(line):
    """A table cell as a spell name, or None if it is not one.

    A spell is set with only its first letter capitalised, which separates
    it from the level markers above it -- and those arrive mangled anyway,
    "l ST LEVEL" and "]TH LEVEL" and "STH LEVEL". It is also short: the
    longest in any of these lists is "Rary's telepathic bond (ritual)", so
    a line past forty characters is the prose that follows the table, not a
    spell in it.

    Only a trailing mark is cut. Tasha's flags a spell new to that book
    with an asterisk and a ritual with a tag, and the extraction turns both
    into whatever punctuation it felt like -- "Intellect fortress'",
    "Dream of the blue veil''", "Summonfe('". Cutting from the first
    apostrophe instead would leave "Rary's telepathic bond" as "Rary".
    """
    if not re.match(r"^[A-Z][a-z']", line) or len(line) > 40:
        return None
    name = re.split(r"\s*\(", line, 1)[0]
    return name.rstrip("'\u2019*.,;:\u00b7 ").strip() or None


LEVEL_MARK = re.compile(r"^[\dl\]\[SI](?:\s*[A-Z]){1,3}\s*LEVE\s*L$", re.I)


def is_level_mark(line):
    """Whether a line is one of the list's level headings.

    They are set in capitals and the extraction mauls them: "l ST LEVEL"
    for 1st, "]TH LEVEL" for 7th, "STH LEVEL" for 5th, "2 N D LEV EL" for
    2nd. What survives all of them is the word LEVEL at the end, with the
    spacing thrown away, and nothing but a marker on the line.
    """
    t = re.sub(r"\s+", "", line).upper()
    if t.startswith("CANTRIP"):
        return len(t) <= 16          # "CANTRIP(OLEVEL)"
    return len(t) <= 10 and t.endswith("LEVEL")


def added():
    """Tasha's additional spells, as {class: [spell, ...]}."""
    lines = [clean(l) for l in open(TCE, encoding="utf-8")]
    out = {}
    for i, l in enumerate(lines):
        m = re.match(r"^ADDITIONAL (\w+) SPELLS$", l)
        if not m:
            continue
        # The feature line is usually the next one, but the paladin's
        # heading has a rule under it that the extraction keeps as "----".
        j = i + 1
        while j < len(lines) and not re.search(r"[A-Za-z]", lines[j]):
            j += 1
        if j >= len(lines) or not FEATURE_LINE.match(lines[j]):
            continue

        # Collecting starts at the first level marker, which leaves the
        # paragraph of prose above it out; and a spell has to follow either
        # a marker or another spell, so the prose under the table is not
        # swept in. A later feature can be printed in the middle of the
        # list -- Wild Companion sits between the druid's 3rd and 4th level
        # spells -- so an interruption is stepped over, not stopped at.
        # The list runs from its first level marker to the first line that
        # is none of a marker, a spell, or an interruption. A later feature
        # can be printed in the middle of the table -- Wild Companion sits
        # between the druid's 3rd- and 4th-level spells -- and arrives as a
        # heading with its own "2nd-level druid feature" line under it, so
        # that pair is stepped over rather than stopped at.
        names, started, region, stopped = [], False, [], ""
        k = j + 1
        while k < min(j + 120, len(lines)):
            line = lines[k]
            if not line:
                k += 1
                continue
            if is_level_mark(line):
                started, k = True, k + 1
                continue
            if not started:
                k += 1
                continue
            if k + 1 < len(lines) and FEATURE_LINE.match(lines[k + 1]):
                k += 2
                continue
            got = spell_name(line)
            if not got:
                stopped = line
                break
            names.append(got)
            region.append(line)
            k += 1

        # Four of these lists are set in two columns, and the extraction
        # takes them a line at a time across both, so a cell that wraps
        # leaves its tail stranded between two cells of the other column:
        # the wizard's reads "Summon shadow" / "Booming blade (evoc.)" /
        # "spawn* (conj.)". A stranded tail opens with a lowercase letter,
        # which no cell of the table does, and that is what marks a list as
        # unreadable rather than wrong.
        stray = region + ([stopped] if stopped else [])
        wrapped = any(re.match(r"^[a-z]", t) and len(t) <= 40 for t in stray)
        out[m.group(1).capitalize()] = (names, wrapped)
    return out


def circle():
    """The Circle of the Land tables, as {land: {level: [spell, ...]}}."""
    lines = [clean(l) for l in open(PHB, encoding="utf-8")]
    out = {}
    for i, l in enumerate(lines):
        # A land's name is a letter-spaced heading over "Druid Level".
        if i + 1 >= len(lines) or lines[i + 1] != "Druid Level":
            continue
        name = re.sub(r"\s+", "", l)
        if not name.isalpha():
            continue
        # Read this table and nothing else. The levels have to be taken
        # from inside the heading's own block: "3rd" and "5th" are how
        # every other table in the book labels a row too, so a reader that
        # stays open after the table ends fills the land with whatever
        # comes next in the chapter.
        rows, k = {}, i + 2
        while k + 1 < len(lines):
            m = re.match(r"^(\d+)(?:st|nd|rd|th)$", lines[k])
            if not m:
                if lines[k] in ("Circle Spells", ""):
                    k += 1
                    continue
                break
            rows[int(m.group(1))] = [s.strip()
                                     for s in lines[k + 1].split(",")]
            k += 2
        if rows:
            out[name.capitalize()] = rows
    return out


def compare(what, ours, theirs, problems):
    a = [squash(s) for s in ours]
    b = [squash(MANGLED.get(s, s)) for s in theirs]
    theirs = [MANGLED.get(s, s) for s in theirs]
    for s, k in zip(ours, a):
        if k not in b:
            problems.append((what, "we list %r and the book's table does not"
                             % s))
    for s, k in zip(theirs, b):
        if k not in a:
            problems.append((what, "the book's table lists %r and we do not"
                             % s))


def main():
    problems, unchecked = [], []
    book_added, book_circle = added(), circle()

    rows = [r for r in read_file("character.txt") if r.tag == "ADDSPELLS"]
    for r in rows:
        cls = r.str(0)
        got = book_added.get(cls)
        if got is None:
            unchecked.append((cls, "Tasha's prints no Additional %s Spells "
                                   "list this can find" % cls))
            continue
        want, wrapped = got
        if wrapped:
            unchecked.append((cls, "its list is set in two columns, and the "
                                   "extraction takes them a line at a time "
                                   "across both, stranding the tail of every "
                                   "cell that wraps"))
            continue
        compare("ADDSPELLS %s" % cls,
                [s.strip() for s in r.str(1).split(",")], want, problems)

    opts = [r for r in read_file("character.txt") if r.tag == "OPTIONSPELLS"]
    for r in opts:
        sub, which = r.str(0), r.str(1)
        if sub in NO_TABLE:
            unchecked.append(("%s %s" % (sub, which), NO_TABLE[sub]))
            continue
        want = book_circle.get(which)
        if want is None:
            unchecked.append(("%s %s" % (sub, which),
                              "no table under that heading"))
            continue
        levels = [int(x) for x in r.str(2).split(",")]
        groups = r.str(3).split("|")
        if len(levels) != len(groups):
            problems.append(("%s %s" % (sub, which),
                             "%d levels and %d groups of spells"
                             % (len(levels), len(groups))))
            continue
        for lvl, group in zip(levels, groups):
            if lvl not in want:
                problems.append(("%s %s" % (sub, which),
                                 "the book's table has no %dth level" % lvl))
                continue
            compare("%s %s at %d" % (sub, which, lvl),
                    [s.strip() for s in group.split(",")], want[lvl],
                    problems)

    listed = set()
    for r in read_file("character.txt"):
        if r.tag == "ADDSPELLS":
            listed |= {squash(x) for x in r.str(1).split(",")}
    for cell, spell in sorted(MANGLED.items()):
        if squash(spell) not in listed:
            problems.append(("MANGLED", "%r is excused as %r, and no row has "
                             "that spell" % (cell, spell)))

    for what, why in problems:
        print("  %-26s %s" % (what, why))
    if unchecked:
        print("\n  %d left unchecked:" % len(unchecked))
        for what, why in unchecked:
            print("      %-22s %s" % (what, why))
    print("\n%d spell lists checked against the books, %d disagree"
          % (len(rows) + len(opts) - len(unchecked), len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
