#!/usr/bin/env python3
"""Check what gates the class option lists in data/character.txt.

The 143 OPTION rows are the choices a class offers from a list: eldritch
invocations, metamagic options, maneuvers, runes, elemental disciplines,
Arcane Shot options, pact boons, favored enemies and terrains. `make audit`
confirms their names are in the books. Nothing read the two fields that
decide whether the program offers one at all -- the level it unlocks at,
and what else the character must already have -- and both were wrong in
places. The Four Elements monk could not take Eternal Mountain Defense
until 17th level where the Player's Handbook gates it at 11th, so four
levels of the discipline list were simply unreachable.

Two shapes carry the gate, and both are read:

  The heading states it, run into the prose on the same line: "Breath of
  Winter (17th Level Required). You can spend 6 ki points", "Storm Rune
  (7th Level or Higher). Using this rune". So a line is taken as an
  option's heading when the text before its first bracket or full stop
  spells the option's name, and the bracket after that is the gate.

  A "Prerequisite:" line under the heading states it, and states anything
  else the option needs: "Prerequisite: 7th-level warlock, Pact of the
  Talisman feature", "Prerequisite: eldritch blast cantrip". The level is
  taken from the first "Nth-level" in it and the rest is what the row's
  prerequisite field has to say.

The window under a heading is six lines rather than two, because a picture
caption can sit between the two: protection of the talisman's heading is
followed by three lines of "A TIEFLING WARLOCK CALLS ON THE POWER OF HIS
TALISMAN" before its prerequisite. It stops early at the next option's
heading, so an ungated option cannot borrow the next one's gate.

A row with no gate at all is checked too, and in the direction that
matters: the book must not gate it either. A level requirement dropped from
a row is the failure that hands a 1st-level character a 15th-level
invocation, and it is invisible to any check that only looks at rows which
already claim one.

Eight rows are reported unchecked rather than passed. They are the ranger's
favored enemy categories -- Beasts, Fey, Oozes and the rest -- which the
Player's Handbook prints as cells of a table and never as a heading, so
there is no line for a gate to be on. None of them claims a gate either.

Six faults injected -- a gate moved, a gate dropped, a gate invented on an
option the book does not gate, a prerequisite changed to a different pact,
a prerequisite invented, an invocation's level bumped by one -- are all
caught.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file            # noqa: E402
import verify_magic_items as MI             # noqa: E402

BOOKS = {"PHB": "PHBtext.txt", "XGE": "XANATHARtext.txt",
         "TCE": "TASHAtext.txt", "SCAG": "SCAGtext.txt"}

# "(17th Level Required)", "(7th Level or Higher)", straight after the name.
GATE = re.compile(r"^\s*\((\d+)(?:st|nd|rd|th)\s+Level\s+"
                  r"(?:Required|or Higher)\)", re.I)

PREREQ = re.compile(r"Prerequisite[s]?:\s*(.*)", re.I)

# "7th-level warlock", "5th level" -- the level inside a prerequisite.
IN_PREREQ = re.compile(r"(\d+)(?:st|nd|rd|th)[-\s]level", re.I)

# Words that carry no weight when comparing a prerequisite to the book's.
NOISE = {"the", "a", "an", "of", "or", "and", "feature", "features",
         "warlock", "level", "cantrip"}


def load(book):
    path = os.path.join(ROOT, "TextFiles", BOOKS[book])
    return [re.sub(r"\s+", " ", l.replace("’", "'").replace("­", "")).strip()
            for l in open(path, encoding="utf-8")]


def heads(lines, name):
    """Every line whose text before its first bracket or stop is this name."""
    want = MI.squash(name)
    out = []
    for i, l in enumerate(lines):
        cut = re.split(r"[(.]", l, 1)[0]
        if cut and MI.squash(cut) == want:
            out.append((i, l[len(cut):]))
    return out


def gate_at(lines, i, tail, other):
    """The level and prerequisite the book states for a heading.

    Returns (level or None, prerequisite text or None). The prerequisite is
    looked for in the six lines under the heading, because a picture caption
    can sit between the two, and the walk stops at the next option's heading
    so nothing borrows a gate that is not its own.
    """
    m = GATE.search(tail)
    level = int(m.group(1)) if m else None
    text = None
    for j in range(i, min(i + 7, len(lines))):
        if j > i and any(j == k for k, _ in other):
            break
        p = PREREQ.search(lines[j])
        if p:
            said = p.group(1).strip()
            # The clause wraps, and the wrap is not the end of it.
            if j + 1 < len(lines) and not said.endswith("."):
                said = said + " " + lines[j + 1]
            text = said
            break
    return level, text


def stem(w):
    """Enough of a stem to let "curses" match "curse".

    The books and the rows do not agree on the part of speech: ours reads
    "a warlock curse feature" where Xanathar's reads "a warlock feature
    that curses".
    """
    if len(w) > 3 and w.endswith("s"):
        w = w[:-1]
    if len(w) > 3 and w.endswith("e"):
        w = w[:-1]
    return w


def words(s):
    return {stem(w) for w in re.findall(r"[a-z]+", s.lower())
            if w not in NOISE}


def main():
    lines = {b: load(b) for b in BOOKS}
    rows = [r for r in read_file("character.txt") if r.tag == "OPTION"]

    # Where a heading is, per book, so one option cannot read the next
    # option's prerequisite.
    all_heads = {}
    for b in BOOKS:
        all_heads[b] = []
        for r in rows:
            if r.str(2) == b:
                all_heads[b] += heads(lines[b], r.str(1))

    checked = bad = 0
    unchecked = []
    for r in rows:
        lst, name, book = r.str(0), r.str(1), r.str(2)
        level, prereq = r.int(3), r.str(4)
        if book not in BOOKS:
            unchecked.append((name, "%s is not one of the dumps" % book))
            continue
        found = heads(lines[book], name)
        if not found:
            unchecked.append((name, "the extraction has no line opening with "
                                    "that name in the %s" % book))
            continue

        says_level, says_prereq = None, None
        for i, tail in found:
            lv, tx = gate_at(lines[book], i, tail, all_heads[book])
            says_level = lv if lv is not None else says_level
            says_prereq = tx if tx is not None else says_prereq
        if says_level is None and says_prereq is not None:
            m = IN_PREREQ.search(says_prereq)
            if m:
                says_level = int(m.group(1))
        # An option the book gates by nothing is not unchecked: the row has
        # to claim nothing either. A level requirement dropped from a row is
        # what hands a 1st-level character a 15th-level invocation, and only
        # this direction catches it.
        if says_level is None and says_prereq is None:
            checked += 1
            if level or prereq:
                bad += 1
                print("  %-28s we gate it at %s%s%s, and the book prints no "
                      "gate at all"
                      % (name, "level %d" % level if level else "",
                         " and " if level and prereq else "",
                         repr(prereq) if prereq else ""))
            continue

        checked += 1
        problems = []
        want = says_level or 0
        if want != level:
            problems.append("we gate it at %s, the book at %s"
                            % (level or "no level", want or "no level"))
        if says_prereq is not None:
            rest = IN_PREREQ.sub("", says_prereq)
            missing = words(prereq) - words(rest)
            if missing:
                problems.append("our prerequisite %r says %s, and the book's "
                                "%r does not"
                                % (prereq, ", ".join(sorted(missing)),
                                   rest.strip(" ,")))
        elif prereq:
            problems.append("we require %r and the book prints no "
                            "prerequisite" % prereq)
        if problems:
            bad += 1
            print("  %-28s %s" % (name, problems[0]))
            for p in problems[1:]:
                print("  %-28s %s" % ("", p))

    if unchecked:
        print("\n  %d left unchecked, the extraction losing the line the "
              "gate would be on:" % len(unchecked))
        by = {}
        for n, why in unchecked:
            by.setdefault(why, []).append(n)
        for why, names in sorted(by.items()):
            print("      %-58s %d" % (why, len(names)))
    print("\n%d option gates checked against the books, %d disagree"
          % (checked, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
