#!/usr/bin/env python3
"""Check the BEAST rows in data/world.txt against the Monster Manual.

A beast is what a druid turns into and what a ranger's companion is, so
every number in the row gets read out at the table: a wrong Constitution is
a wrong pile of hit points for a whole campaign, and nothing in the program
would ever notice. tools/audit.py only checks that the names exist. This
checks the numbers beside them.

A Monster Manual stat block is regular enough to read back whole. It opens
with "Large beast, unaligned", then "Armor Class 13", "Hit Points 51
(6d10 + 18)", "Speed 60 ft.", then the six abilities as a label with its
"19 (+4)" underneath, then Senses and "Challenge 2 (450 XP)". Size, AC, hit
points, speed, the six scores, senses, the printed challenge and the eighths
the row stores it in are all compared.

What the dump does that this has to work around.

The stat block's name is a decorative heading and does not always survive:
the allosaurus, the plesiosaurus and the triceratops have nothing above
"Large beast, unaligned" but the tail of the previous column or a page
number. Small capitals come back mixed and letters are misread outright --
"AxE BEAK", "GrANT APE", "llGER" for the tiger. So a block is matched to a
row by its heading with case and punctuation thrown away, then by a heading
that differs in one letter, and finally, for a block whose heading is gone,
by the creature its own traits talk about: "the plesiosaurus can hold its
breath" can only be the plesiosaurus. Every step demands a unique match, and
a row that cannot be matched is reported unchecked rather than guessed at.

A stat block is set in columns and the dump interleaves them: Skills,
Senses, Languages and Challenge land between STR/DEX/CON and INT/WIS/CHA,
and on some pages the six labels come as a run of their own with the scores
in a second run underneath. Both shapes are read, but where a label has no
score under it, or a score is too mangled to be a number ("s r-3)" for the
deer's Charisma), or the sixth score never made it onto the page at all (the
giant rat, the giant boar and the giant wasp all lose their Charisma), the
whole set is left unchecked rather than half-read.

Digits are misread as letters in a way that is safe to undo in a numeric
field and nowhere else: "Armor Class l3", "Speed 1 0 ft." Only l->1 and
O->0 are undone, and only in a field that is meant to hold a number; a value
that still will not parse is left unchecked.

Words are set with a space inside them -- the rhinoceros has "passive Pe
rception 11" -- so the fixed words of a Senses line are matched with spaces
allowed between their letters. Senses is otherwise compared with the
punctuation thrown away, because the dump spaces "ft." at random; a mark in
the row that a Senses line never contains is reported separately, since that
comparison would swallow it.

Deliberately unchecked: everything below the Challenge line. Skills, damage
resistances, languages, traits and actions are not in the BEAST row, so
there is nothing here to compare them against. Nor is the hit dice
expression behind the hit points, which the row does not store either.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

DUMP = os.path.join(ROOT, "TextFiles", "MMtext.txt")

SIZES = ("Tiny", "Small", "Medium", "Large", "Huge", "Gargantuan")

# The first line of a stat block: "Large beast, unaligned", or the longer
# "Medium swarm of Tiny beasts, unaligned". The leading junk is the dump's
# stray quotes and bullets. A comma and a short line are what tell it from
# the prose that starts the same way -- "Tiny beasts such as rodents and
# birds that are nor-", "Medium or smaller creature. Hit:".
TYPE_LINE = re.compile(r"^[^A-Za-z0-9]{0,4}(%s)\s+([a-z][a-z'~]*)"
                       % "|".join(SIZES))
TYPE_MAX = 60

# No beast stat block runs anywhere near this long before its Challenge
# line. The cap stops a block whose successor was not recognised from
# swallowing the rest of the chapter.
MAX_BLOCK = 45

ABILITIES = ("STR", "DEX", "CON", "INT", "WIS", "CHA")
# The first two letters tell the six labels apart, which is what lets the
# jackal's "WI~" still be read as its Wisdom.
BY_PREFIX = dict((a[:2], a) for a in ABILITIES)

# "19 (+4)", "2. (-4)", "5, (~ 3)", "21 (+S)", "3 (L4)" -- the modifier is
# mangled every way there is, so only the score and the bracket after it are
# insisted on. Score lines are looked for directly under a label and nowhere
# else, so this cannot pick up a damage roll.
SCORE = re.compile(r"^[^A-Za-z0-9]{0,3}([0-9lO]{1,2})\s*[.,;]?\s*[\(\[]")
LABEL_AND_SCORE = re.compile(
    r"^[^A-Za-z0-9]{0,3}(%s)\s+([0-9lO]{1,2})\s*[.,;]?\s*[\(\[]"
    % "|".join(ABILITIES))

NUM = r"[0-9lO][0-9lO ]*"
AC = re.compile(r"Armor\s*Class\s+(%s)" % NUM)
HP = re.compile(r"Hit\s*Points\s+(%s)" % NUM)
SPEED = re.compile(r"^[^A-Za-z0-9]{0,3}Speed\s+(.+?)\s*$")
CHALLENGE = re.compile(r"Challenge\s+(\d+\s*/\s*\d+|\d+)\s*[\(\[]")


def spaced(word):
    """A word the typesetting may have split: 'Pe rception'."""
    return r"\s*".join(word)


# "Sen~es" is the one spelling of the label the dump gets wrong. The gap is
# capped so that a block whose Senses line was dropped cannot reach forward
# to a passive Perception belonging to something else.
SENSES = re.compile(r"Sen\S{0,2}es\s+(.{0,90}?%s\s+%s\s+[0-9lO]{1,3})"
                    % (spaced("passive"), spaced("Perception")))

# Lines that sit between a heading and its stat block without being either:
# blank lines, page markers, page numbers.
JUNK = re.compile(r"^(?:=== page \d+ ===|[^A-Za-z]*"
                  r"|[A-Za-z]?\d{1,3}[A-Za-z]?)$")


def norm(s):
    """Case, punctuation and spacing thrown away."""
    return re.sub(r"[^a-z0-9]", "", s.lower())


def undo_ocr(s):
    """l->1 and O->0, for a field that is meant to hold a number."""
    return s.replace("l", "1").replace("O", "0")


def number(s):
    """A book number, or None when what is left is not one."""
    s = undo_ocr(s).replace(" ", "")
    return int(s) if s.isdigit() else None


def flatten(s, book=False):
    """A book string and a row's field reduced to the same shape.

    Case, spacing and every mark but the comma go, because the dump spaces
    "ft." and the commas at random. On the book's side an l standing next to
    a digit is a misread 1 -- "Perception l3" -- but an l anywhere else is a
    letter, and "climb" must stay "climb".
    """
    s = s.lower().replace("ft.", "ft")
    s = re.sub(r"[^a-z0-9,]", "", s)
    if book:
        s = re.sub(r"(?<=[0-9])l|l(?=[0-9])", "1", s)
    return s.strip(",")


def eighths(cr):
    """'1/4' -> 2. The row stores the challenge as a whole number too."""
    if "/" in cr:
        top, _, bot = cr.partition("/")
        return int(top) * 8 // int(bot)
    return int(cr) * 8


def heading(lines, n):
    """The name above a stat block, or '' where the dump dropped it."""
    for k in range(n - 1, max(n - 4, -1), -1):
        line = lines[k].strip()
        if JUNK.match(line):
            continue
        return line
    return ""


def blocks(lines):
    """Every beast stat block in the dump.

    A block runs from its type line to the next creature's, whatever that
    creature is, so that a plant or a humanoid between two beasts still
    closes the block before it.
    """
    starts = []
    for n, line in enumerate(lines):
        m = TYPE_LINE.match(line)
        if not m or m.group(2) == "or":
            continue
        if "," not in line or len(line.strip()) > TYPE_MAX:
            continue
        starts.append((n, m.group(1), m.group(2)))
    out = []
    for i, (n, size, kind) in enumerate(starts):
        end = starts[i + 1][0] if i + 1 < len(starts) else len(lines)
        end = min(end, n + MAX_BLOCK)
        if kind == "beast":
            out.append({"at": n, "size": size, "lines": lines[n:end],
                        "head": norm(heading(lines, n))})
    return out


def one_letter_apart(a, b):
    """Headings differing in a single misread letter: GrANT APE."""
    if len(a) != len(b) or not a:
        return False
    return sum(1 for x, y in zip(a, b) if x != y) == 1


def claim(beasts, rows):
    """Match each row to its block, and say nothing where it is not sure."""
    found, taken = {}, set()

    def assign(name, hits):
        if len(hits) == 1 and hits[0] not in taken:
            found[name] = hits[0]
            taken.add(hits[0])

    def left(r):
        return r.str(0) not in found

    for r in rows:
        want = norm(r.str(0))
        assign(r.str(0), [i for i, b in enumerate(beasts)
                          if b["head"] == want])
    for r in rows:
        if left(r):
            want = norm(r.str(0))
            assign(r.str(0), [i for i, b in enumerate(beasts)
                              if i not in taken
                              and one_letter_apart(b["head"], want)])
    for r in rows:
        if left(r):
            want = norm(r.str(0))
            assign(r.str(0), [i for i, b in enumerate(beasts)
                              if i not in taken and b["head"].endswith(want)])
    for r in rows:
        if left(r):
            word = re.compile(r"\bthe %s\b" % re.escape(r.str(0).lower()))
            assign(r.str(0), [i for i, b in enumerate(beasts)
                              if i not in taken
                              and word.search(" ".join(b["lines"]).lower())])
    return found


def label_of(line):
    """The ability a short line names, or None."""
    s = line.strip()
    if len(s) > 6:
        return None
    letters = re.sub(r"[^A-Za-z]", "", s)
    caps = sum(1 for c in letters if c.isupper())
    if len(letters) not in (2, 3) or caps < 2:
        return None
    return BY_PREFIX.get(letters[:2].upper())


def read_abilities(block):
    """The six scores, or None where the two columns came apart.

    The labels come either one at a time above their own score or as a run
    of labels above a run of scores; both are read, and anything else is
    refused.
    """
    lines, got, i = block["lines"], {}, 0
    while i < len(lines):
        m = LABEL_AND_SCORE.match(lines[i])
        if m:
            labels, scores, i = [m.group(1)], [m.group(2)], i + 1
        elif label_of(lines[i]):
            labels, scores = [], []
            while i < len(lines) and label_of(lines[i]):
                labels.append(label_of(lines[i]))
                i += 1
            while i < len(lines) and SCORE.match(lines[i]):
                scores.append(SCORE.match(lines[i]).group(1))
                i += 1
        else:
            i += 1
            continue
        if len(labels) != len(scores):
            return None
        for label, raw in zip(labels, scores):
            value = number(raw)
            if value is None or label in got:
                return None
            got[label] = value
    return got if len(got) == len(ABILITIES) else None


def find(pattern, lines):
    """The first match of a pattern that has to stay on its own line."""
    for line in lines:
        m = pattern.search(line)
        if m:
            return m
    return None


def main():
    if not os.path.exists(DUMP):
        sys.exit("verify_beasts: no %s" % DUMP)

    lines = open(DUMP, encoding="utf-8", errors="replace").read().split("\n")
    beasts = blocks(lines)
    rows = [r for r in read_file("world.txt") if r.tag == "BEAST"]
    found = claim(beasts, rows)

    checked = bad = 0
    unchecked = []

    for r in rows:
        name = r.str(0)
        if name not in found:
            unchecked.append((name, "no stat block could be matched to it"))
            continue
        block = beasts[found[name]]
        text = " ".join(block["lines"])
        checked += 1
        problems = []

        def note(what, mine, book):
            problems.append("%s %s, book says %s" % (what, mine, book))

        def skip(why):
            unchecked.append((name, why))

        if block["size"] != r.str(1):
            note("size", r.str(1), block["size"])

        for what, pattern, field in (("AC", AC, 2), ("hp", HP, 3)):
            m = find(pattern, block["lines"])
            if m is None:
                skip("no %s line" % ("Armor Class" if field == 2
                                     else "Hit Points"))
                continue
            value = number(m.group(1))
            if value is None:
                skip("the %s is not a readable number: %r"
                     % (what, m.group(1).strip()))
            elif value != r.int(field):
                note(what, r.int(field), value)

        m = find(SPEED, block["lines"])
        if m is None:
            skip("no Speed line")
        elif flatten(m.group(1), book=True) != flatten(r.str(5)):
            note("speed", repr(r.str(5)), repr(m.group(1)))

        got = read_abilities(block)
        if got is None:
            skip("the ability columns came apart; scores unreadable")
        else:
            for n, key in enumerate(ABILITIES, start=6):
                if got[key] != r.int(n):
                    note(key, r.int(n), got[key])

        m = SENSES.search(text)
        if m is None:
            skip("no Senses line")
        else:
            book = " ".join(m.group(1).split())
            stray = re.search(r"[^A-Za-z0-9 ,.]", r.str(13))
            if flatten(m.group(1), book=True) != flatten(r.str(13)):
                note("senses", repr(r.str(13)), repr(book))
            elif stray:
                # The comparison above throws punctuation away, so a mark
                # the book never uses in a Senses line would slip past it.
                note("senses", repr(r.str(13)),
                     repr(book) + " (a stray %r in the row)"
                     % stray.group(0))

        m = CHALLENGE.search(text)
        if m is None:
            skip("no Challenge line")
        else:
            cr = m.group(1).replace(" ", "")
            if cr != r.str(12).replace(" ", ""):
                note("challenge", r.str(12), cr)
            elif eighths(cr) != r.int(4):
                note("challenge in eighths", r.int(4), eighths(cr))

        if problems:
            bad += 1
            print("  %s" % name)
            for p in problems:
                print("      %s" % p)

    if unchecked:
        print("\n  left unchecked:")
        for name, why in unchecked:
            print("      %s: %s" % (name, why))

    print("\n%d beasts checked against the Monster Manual, %d disagree"
          % (checked, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
