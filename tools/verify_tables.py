#!/usr/bin/env python3
"""Check the numeric tables in data/character.txt against the book's own.

These are the rows that are nothing but numbers, and until now nothing
checked any of them: the experience needed for each level, the full caster's
spell slots, the warlock's Pact Magic columns, the PHB's Random Height and
Weight table, each artificer infusion's minimum level, and the cantrips and
spells each class knows at each of its twenty levels.

A table in the extracted text is not laid out as a table. The extraction
emits one cell per line, reading down the columns of the printed row, so the
wizard's 1st-level row arrives as eleven consecutive lines: "1st", "+2",
"Spellcasting, Arcane Recovery", "3", "2", "—", "—", ... That is a shape
worth reading properly rather than guessing at, so this walks the cells: a
row opens at a line that is nothing but an ordinal ("1st", "17th"), the
proficiency bonus follows it, then the features cell, then the row's numbers
in the order the header gives. An em dash is an empty cell, which is a zero.

What the layout does not settle is left alone rather than guessed at. The
features cell can run to two lines when the page breaks it, so a row is
taken as ended by the next ordinal rather than by a count of cells, and a
row whose numbers do not fill its columns is reported as unread instead of
being compared half-way.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file                          # noqa: E402

PHB = os.path.join(ROOT, "TextFiles", "PHBtext.txt")
TCE = os.path.join(ROOT, "TextFiles", "TASHAtext.txt")

ORDINAL = re.compile(r"^\s*(\d{1,2})\s*(?:st|nd|rd|th)\s*$")
NUMBER = re.compile(r"^\s*([\d,]+)\s*(?:lb\.?)?\s*$")
DASH = re.compile(r"^\s*[—–-]\s*$")
# Two cells the extraction ran together: the wizard's ninth row arrives with
# "1 —" where the book prints a 1 and then an empty column.
GLUED = re.compile(r"^\s*(\d+)\s+[—–-]\s*$")
# A modifier cell: dice, or the flat "x 1 lb." the halfling and the gnome
# have where every other race has a die.
DICE = re.compile(r"^\s*(?:x\s*)?\(?\+?(\d+d\d+|\d+)\)?\s*(?:lb\.?)?\s*$")
FEET = re.compile(r"^\s*(\d+)\s*['’]\s*(\d*)\s*[\"”]?\s*$")
PROF = re.compile(r"^\s*\+\d\s*$")


def lines_of(path):
    return open(path, encoding="utf-8", errors="replace").read().split("\n")


def squash(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


def find_heading(lines, heading, start=0):
    """The line number of a heading, matched with its letterspacing removed."""
    want = squash(heading)
    for i in range(start, len(lines)):
        if squash(lines[i]) == want:
            return i
    return -1


def rows_under(lines, at, stop_after=20):
    """The table under a heading, as {level: [cells]}.

    A row opens at an ordinal followed by a proficiency bonus, and runs to
    the next such pair. Both halves of that test are needed. The header
    itself is a run of ordinals -- "1st" to "9th" are the column labels over
    the spell slots -- and would otherwise be read as nine rows; and the
    warlock's Slot Level column is an ordinal in the middle of a row, which
    would otherwise open a new one.
    """
    out, level, cells = {}, None, []
    body = lines[at + 1:at + 1200]
    for i, line in enumerate(body):
        m = ORDINAL.match(line)
        nxt = next((l for l in body[i + 1:i + 3] if l.strip()), "")
        if m and PROF.match(nxt):
            if level is not None and cells:
                out.setdefault(level, cells)
            level = int(m.group(1))
            cells = []
            if level > stop_after:
                break
            continue
        if level is not None and line.strip():
            cells.append(line.strip())
    if level is not None and cells:
        out.setdefault(level, cells)
    return out


def cell_numbers(cells):
    """A row's numeric columns: everything after the proficiency bonus and
    the features cell.

    The features cell has to be stepped over by position rather than by what
    is in it, because a level that grants no feature prints an em dash there
    -- which is a zero everywhere else in the row, and reading it as one
    shifted every column after it by one place. A dash is a zero in the
    columns; an ordinal is the number it names, which is how the warlock's
    slot level column reads.
    """
    out, seen_features = [], False
    for c in cells:
        if not c:
            continue
        if PROF.match(c):
            continue                     # the proficiency bonus column
        if not seen_features:
            seen_features = True         # the features cell, dash or prose
            continue
        if DASH.match(c):
            out.append(0)
        elif GLUED.match(c):
            out.append(int(GLUED.match(c).group(1)))
            out.append(0)
        elif NUMBER.match(c):
            out.append(int(NUMBER.match(c).group(1).replace(",", "")))
        elif ORDINAL.match(c):
            out.append(int(ORDINAL.match(c).group(1)))
        elif out:
            break                        # past the table, into the prose
    return out


class Report:
    def __init__(self):
        self.checked = 0
        self.problems = []
        self.skipped = []

    def ok(self):
        self.checked += 1

    def bad(self, what, ours, theirs):
        self.checked += 1
        self.problems.append((what, ours, theirs))

    def unchecked(self, what, why):
        self.skipped.append((what, why))


def check_advancement(rows, lines, report):
    """Experience Points / Level / Proficiency, read as three cells a row."""
    at = find_heading(lines, "Character Advancement")
    if at < 0:
        report.unchecked("the experience table", "no heading in the dump")
        return
    # This table leads with the experience, so the ordinal-led reader does
    # not fit it: the cells are simply three to a row, in order.
    cells = [l.strip() for l in lines[at + 4:at + 4 + 70] if l.strip()]
    book = {}
    for i in range(0, len(cells) - 2, 3):
        xp, lvl, prof = cells[i], cells[i + 1], cells[i + 2]
        if not (NUMBER.match(xp) and lvl.isdigit() and prof.startswith("+")):
            break
        book[int(lvl)] = int(xp.replace(",", ""))
    for r in rows:
        level, xp = r.int(0), r.int(1)
        if level not in book:
            report.unchecked("experience for level %d" % level,
                             "the table stops short of it in the dump")
        elif book[level] != xp:
            report.bad("experience for level %d" % level, xp, book[level])
        else:
            report.ok()


def check_full_slots(rows, lines, report):
    """The wizard's table: cantrips known, then nine slot columns."""
    at = find_heading(lines, "The Wizard")
    if at < 0:
        report.unchecked("the full caster slot table", "no heading in the dump")
        return
    book = rows_under(lines, at)
    for r in rows:
        level = r.int(0)
        if level == 0:
            report.ok()                  # our own row; nobody is level 0
            continue
        cells = cell_numbers(book.get(level, []))
        if len(cells) < 10:
            report.unchecked("spell slots at level %d" % level,
                             "the row has %d numeric cells, not 10"
                             % len(cells))
            continue
        slots = cells[1:10]              # cells[0] is cantrips known
        ours = [r.int(i + 1) for i in range(9)]
        if slots != ours:
            report.bad("spell slots at level %d" % level,
                       " ".join(str(n) for n in ours),
                       " ".join(str(n) for n in slots))
        else:
            report.ok()


def check_pact_slots(rows, lines, report):
    """The warlock's table: cantrips, spells known, slots, slot level."""
    at = find_heading(lines, "The Warlock")
    if at < 0:
        report.unchecked("the pact magic table", "no heading in the dump")
        return
    book = rows_under(lines, at)
    for r in rows:
        level = r.int(0)
        if level == 0:
            report.ok()
            continue
        cells = cell_numbers(book.get(level, []))
        if len(cells) < 4:
            report.unchecked("pact magic at level %d" % level,
                             "the row has %d numeric cells, not 4"
                             % len(cells))
            continue
        slots, slot_level = cells[2], cells[3]
        if (slots, slot_level) != (r.int(1), r.int(2)):
            report.bad("pact magic at level %d" % level,
                       "%d slots of level %d" % (r.int(1), r.int(2)),
                       "%d slots of level %d" % (slots, slot_level))
        else:
            report.ok()


# Each class table's spell columns: the heading it sits under, how many
# numeric cells a whole row of it has, and which of those cells each
# PROGRESSION row in data/character.txt should equal.
#
# The Sorcerer is the one that needs a note. Its table puts Sorcery Points
# between the proficiency bonus and the Features column, so on a level whose
# feature is an em dash the reader takes the points for the feature and the
# dash for a number, and the row comes back one cell long with a nought at
# the front. Its numbers are the last eleven either way, which is why that
# shape is read from the back rather than reported.
#
# The artificer's own table is not here: it is in Tasha's, and the extraction
# shreds its columns into a single run of cells that cannot be read as rows.
KNOWN_COLUMNS = [
    ("The Bard", 11, {"Bard cantrips": 0, "Bard spells known": 1}),
    ("The Cleric", 10, {"Cleric cantrips": 0}),
    ("The Druid", 10, {"Druid cantrips": 0}),
    ("The Ranger", 6, {"Ranger spells known": 0}),
    ("The Sorcerer", 11, {"Sorcerer cantrips": 0, "Sorcerer spells known": 1}),
    ("The Warlock", 5, {"Warlock cantrips": 0, "Warlock spells known": 1,
                        "eldritch invocations": 4}),
    ("The Wizard", 10, {"Wizard cantrips": 0}),
]


def check_known(rows, lines, report):
    """The cantrips and spells each class knows, level by level.

    Two hundred numbers that nothing checked: how many cantrips a cleric
    has at 10th, how many spells a bard knows at 19th, how many invocations
    a warlock has at 12th. They are what the level-up counts out, so a wrong
    one is a character with the wrong number of spells.
    """
    ours = {}
    for r in rows:
        ours[r.str(0)] = [int(x) for x in r.str(1).split(",")]

    for heading, width, columns in KNOWN_COLUMNS:
        at = find_heading(lines, heading)
        if at < 0:
            report.unchecked(heading, "no heading in the dump")
            continue
        book = rows_under(lines, at)
        for level in range(1, 21):
            cells = cell_numbers(book.get(level, []))
            if len(cells) == width + 1 and heading == "The Sorcerer":
                cells = cells[-width:]
            if len(cells) != width:
                report.unchecked("%s at level %d" % (heading, level),
                                 "the row has %d numeric cells, not %d"
                                 % (len(cells), width))
                continue
            for name, column in columns.items():
                if name not in ours:
                    report.unchecked(name, "no row of that name in data/")
                elif ours[name][level] != cells[column]:
                    report.bad("%s at level %d" % (name, level),
                               ours[name][level], cells[column])
                else:
                    report.ok()


# The table names the races the way the book does, not the way the data does.
BODY_NAME = {
    "Human": "Human", "Hill Dwarf": "Dwarf, hill",
    "Mountain Dwarf": "Dwarf, mountain", "High Elf": "Elf, high",
    "Wood Elf": "Elf, wood", "Dark Elf (Drow)": "Elf, drow",
    "Halfling": "Halfling", "Dragonborn": "Dragonborn", "Gnome": "Gnome",
    "Half-Elf": "Half-elf", "Half-Orc": "Half-orc", "Tiefling": "Tiefling",
}


def check_body(rows, lines, report):
    """Random Height and Weight: five cells a row, the height in feet."""
    at = find_heading(lines, "Random Height and Weight")
    if at < 0:
        report.unchecked("the height and weight table",
                         "no heading in the dump")
        return
    cells = [l.strip() for l in lines[at + 1:at + 90] if l.strip()]
    book = {}
    i = 0
    while i + 4 < len(cells):
        name, height, hmod, weight, wmul = cells[i:i + 5]
        if FEET.match(height) and DICE.match(hmod) and NUMBER.match(weight) \
                and DICE.match(wmul):
            f = FEET.match(height)
            inches = int(f.group(1)) * 12 + int(f.group(2) or 0)
            book[squash(name)] = (inches, DICE.match(hmod).group(1),
                                  int(NUMBER.match(weight).group(1)),
                                  DICE.match(wmul).group(1))
            i += 5
        else:
            i += 1

    for r in rows:
        who = r.str(1) or r.str(0)
        key = squash(BODY_NAME.get(who, who))
        if key not in book:
            report.unchecked(who, "its row is not in the dump's table")
            continue
        ours = (r.int(2), r.str(3), r.int(4), r.str(5))
        if book[key] != ours:
            report.bad("%s height and weight" % who,
                       "%d in, %s, %d lb, %s" % ours,
                       "%d in, %s, %d lb, %s" % book[key])
        else:
            report.ok()


def check_infusions(rows, tce_text, report):
    """Each infusion's minimum artificer level, from its prerequisite line."""
    sq = squash(tce_text)
    for r in rows:
        name, prereq, level = r.str(0), r.str(1), r.int(2)
        if squash(name) not in sq:
            report.unchecked(name, "the heading is not in the dump")
            continue
        if not prereq:
            # Nothing printed under the name: the infusion is available as
            # soon as Infuse Item is, at 2nd level.
            if level != 2:
                report.bad(name, "minimum level %d" % level,
                           "no prerequisite printed, so 2nd level")
            else:
                report.ok()
            continue
        if squash("prerequisite:" + prereq) not in sq:
            report.bad(name, "prerequisite %r" % prereq,
                       "no such line under the heading")
        elif not prereq.startswith(str(level)):
            report.bad(name, "minimum level %d" % level,
                       "its prerequisite reads %r" % prereq)
        else:
            report.ok()


def main():
    rows = read_file("character.txt")
    by_tag = {}
    for r in rows:
        by_tag.setdefault(r.tag, []).append(r)

    phb = lines_of(PHB)
    tce_text = open(TCE, encoding="utf-8", errors="replace").read()
    report = Report()

    check_advancement(by_tag.get("XP", []), phb, report)
    check_full_slots(by_tag.get("FULLSLOTS", []), phb, report)
    check_pact_slots(by_tag.get("PACTSLOTS", []), phb, report)
    check_body(by_tag.get("BODY", []), phb, report)
    check_infusions(by_tag.get("INFUSION", []), tce_text, report)
    check_known(by_tag.get("PROGRESSION", []), phb, report)

    for what, ours, theirs in report.problems:
        print("  %s" % what)
        print("      ours %s, the book %s" % (ours, theirs))
    if report.skipped:
        print("\n  %d left unchecked, the dump not settling them:"
              % len(report.skipped))
        for what, why in report.skipped:
            print("      %-34s %s" % (what, why))

    print("\n%d table rows checked, %d disagree"
          % (report.checked, len(report.problems)))
    return 1 if report.problems else 0


if __name__ == "__main__":
    sys.exit(main())
