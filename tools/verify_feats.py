#!/usr/bin/env python3
"""Check the FEAT and OPTFEATURE rows in data/character.txt against the books.

The 73 feats come from four books -- 42 from the PHB, 15 from Tasha's, the 15
racial feats from Xanathar's and Svirfneblin Magic from the Sword Coast guide
-- and two things beside each name can be read straight back off the page: the
"Prerequisite:" line the book prints under the name (a feat that has none
prints no such line), and the ability score the feat raises, which is always a
first bullet of the form "Increase your Strength or Constitution score by 1,
to a maximum of 20." Everything else a FEAT row carries is a summary in this
project's own words and is not checkable this way, so it is left alone; so is
req_race, which maps a printed prerequisite like "Dwarf or a Small race" onto
this project's race names and is not something the book states.

The 42 OPTFEATURE rows are Tasha's optional class features. Each is printed
under a subhead of the form "3rd-level barbarian feature", sometimes carrying
", which replaces the Natural Explorer feature", which gives the class, the
level and the replaced feature in one line.

What the dumps do to get in the way:

  * The PHB sets feat names in small caps, which arrive as single letters
    separated by spaces -- "D u a l W i e / l d / e / r" across four lines --
    so names are found in a copy of the text with every space removed, and a
    heading is told from a mention in prose by having to start and end a line.
  * Tasha's loses the decorative initial that opens a section: Quickened
    Healing arrives as "UICKENED HEALING". Xanathar's mis-scans two of its
    headings ("0RCISH FURY", "Woon ELF MAGIC"). A heading that cannot be found
    letter for letter is therefore looked for a second time by comparing whole
    lines for near-equality.
  * Xanathar's mis-scans the prerequisites themselves -- "Halffing",
    "Haffling", "Tiefiing", "Half-ore", "D warf or a S mall race" -- so
    prerequisites are compared by shape: an ability-and-score prerequisite
    against the ability and score the row records, a proficiency against the
    proficiency, and a race against the row's text closely enough to survive a
    misread letter. Anything further apart than that is reported.
  * Page 156 of the PHB interleaves its two columns: the Grappler heading is
    followed immediately by the Inspiring Leader heading and only then by
    Grappler's prerequisite, so neither name has its own text under it. A feat
    whose block runs into another feat's -- no text at all before the next
    heading, or two "Prerequisite" lines inside one block -- is reported
    unchecked rather than guessed at, and those two were read by hand.
  * Two of Tasha's optional features, Maneuver Options and Eldritch Invocation
    Options, are printed with no "Nth-level ... feature" line at all, because
    maneuvers and invocations are reached by more than one route. Their class
    is still checked, from the class section they sit in, and their replaced
    feature is still checked; their level is reported unchecked.
"""

import difflib
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

DUMPS = {
    "PHB": "PHBtext.txt",
    "XGE": "XANATHARtext.txt",
    "TCE": "TASHAtext.txt",
    "SCAG": "SCAGtext.txt",
}

# Where each book's feats live, as a pair of strings that bracket them. The
# feats are a run of pages in every book, and slicing to those pages keeps a
# feat's name from matching the table of contents or a sidebar.
FEAT_REGION = {
    "PHB": ("=== page 154 ===", "=== page 160 ==="),
    "XGE": ("The feats are presented below", "=== page 78 ==="),
    "TCE": ("New feats are presented here", "=== page 83 ==="),
    "SCAG": ("Deep GNOME FEAT", "GNOMISH DEITIES"),
}

# Tasha's optional class features run from the first class section to the
# feats, and the artificer's own features sit above them.
OPT_REGION = ("OPTIONAL CLASS FEATURES", "New feats are presented here")

CLASSES = ["barbarian", "bard", "cleric", "druid", "fighter", "monk",
           "paladin", "ranger", "rogue", "sorcerer", "warlock", "wizard"]

ABILITIES = ["STR", "DEX", "CON", "INT", "WIS", "CHA"]

# How each ability's name survives the scan. Xanathar's prints Intelligence
# as "I ntelJigence" once, so that one is matched loosely.
ABILITY_RE = {
    "STR": r"strength",
    "DEX": r"dexterity",
    "CON": r"constitution",
    "INT": r"intel.{0,3}gence",
    "WIS": r"wisdom",
    "CHA": r"charisma",
}

# The sentence that grants an increase, read off the squashed text because
# of what the scans do to it: "m axim um", "maximum o f 20", "of20", "by l ,"
# and, in three of Tasha's feats, no word "score" at all.
ASI = re.compile(r"increase(?:your|one|the)(.{0,70}?)by[1l]toamaximumof20")
ANY_ASI = re.compile(r"oneabilityscoreofyourchoice|thechosenabilityscore")

PREREQ_LINE = re.compile(r"^\s*Prerequisites?\s*:\s*(.+?)\s*$")

LEVEL = re.compile(r"(\d+)(?:st|nd|rd|th)-level (%s) feature"
                   % "|".join(CLASSES), re.I)
REPLACES = re.compile(r"which replaces the (.+?) feature", re.I)


def flat(text):
    """One long line, with the hyphenation of a line break undone.

    The soft hyphen Tasha's uses to break a word is the same thing as the
    hyphen the other books use, once the line break beside it is gone.
    """
    text = text.replace("­", "-")
    text = text.replace("’", "'").replace("‘", "'")
    return re.sub(r"\s+", " ", text).replace("- ", "")


def squash(text):
    """Letters and digits only, folded to lower case.

    This is what the PHB's letter-spaced small caps and Xanathar's spaces
    inside words have in common with the name as it is written in data/.
    """
    return re.sub(r"[^a-z0-9]", "", text.lower())


def squash_map(text):
    """squash() over a whole region, with a map back to its offsets."""
    out, idx = [], []
    for i, ch in enumerate(text.lower()):
        if ch.isalnum():
            out.append(ch)
            idx.append(i)
    return "".join(out), idx


def region(book, first, last):
    text = open(os.path.join(ROOT, "TextFiles", DUMPS[book]),
                encoding="utf-8").read()
    start = text.find(first)
    if start < 0:
        sys.exit("verify_feats: %s: no %r" % (DUMPS[book], first))
    end = text.find(last, start)
    return text[start:end if end > 0 else len(text)]


def line_bounds(text, at):
    start = text.rfind("\n", 0, at) + 1
    end = text.find("\n", at)
    return start, (end if end >= 0 else len(text))


def alone_on_a_line(text, start, end):
    """Is text[start:end] the whole of the line or lines it sits on?

    Two stray characters are allowed at each end, which is what a lost or
    mis-scanned decorative initial leaves behind ("0RCISH FURY").
    """
    head = text[line_bounds(text, start)[0]:start]
    tail = text[end:line_bounds(text, end - 1)[1]]
    return len(head.strip()) <= 2 and len(tail.strip()) <= 2


def headings(text, name):
    """Every place in text where name is printed as a heading.

    Looked for letter by letter first, then -- for a heading the scan has
    misread, which no exact search can find -- by comparing whole lines.
    """
    want = squash(name)
    flatten, idx = squash_map(text)
    found = []
    for key in (want, want[1:]):          # the second loses a drop cap
        at = flatten.find(key)
        while at >= 0:
            start, end = idx[at], idx[at + len(key) - 1] + 1
            if alone_on_a_line(text, start, end):
                found.append((line_bounds(text, start)[0], end))
            at = flatten.find(key, at + 1)
        if found:
            break

    if not found:
        for m in re.finditer(r"^.+$", text, re.M):
            got = squash(m.group(0))
            if not got or abs(len(got) - len(want)) > 3:
                continue
            if difflib.SequenceMatcher(None, got, want).ratio() >= 0.85:
                found.append((m.start(), m.end()))

    # Tasha's prints the Eldritch Adept heading twice running. Where two
    # candidates sit on top of each other, the body follows the later one.
    out = []
    for pos in sorted(found):
        if out and pos[0] - out[-1][1] < 40:
            out[-1] = pos
        else:
            out.append(pos)
    return out


def blocks(text, names):
    """name -> (heading start, heading end, end of its text).

    A feat's text runs from its own heading to the next heading of any feat,
    which is how the interleaved columns give themselves away: a heading with
    nothing between it and the next one.
    """
    at = {}
    for name in names:
        for pos in headings(text, name):
            at.setdefault(pos, []).append(name)
    starts = sorted(at)
    out = {}
    for n, pos in enumerate(starts):
        end = starts[n + 1][0] if n + 1 < len(starts) else len(text)
        for name in at[pos]:
            out.setdefault(name, []).append((pos[0], pos[1], end))
    return out


# ------------------------------------------------------------- prerequisites

ABILITY_PREREQ = re.compile(
    r"^(strength|dexterity|constitution|intelligence|wisdom|charisma)"
    r"(?:\s+or\s+(strength|dexterity|constitution|intelligence|wisdom|"
    r"charisma))?\s+(\d+)\s+or higher$", re.I)
PROF_PREREQ = re.compile(r"^proficiency with (?:an?\s+)?(.+?)s?$", re.I)
SPELL_PREREQ = re.compile(r"^(the ability to cast at least one spell|"
                          r"spellcasting or pact magic(?: feature)?)$", re.I)

NAMED = {"strength": "STR", "dexterity": "DEX", "constitution": "CON",
         "intelligence": "INT", "wisdom": "WIS", "charisma": "CHA"}


def shape(prereq):
    """A printed prerequisite as something two of them can be compared by."""
    prereq = prereq.strip().rstrip(".")
    m = ABILITY_PREREQ.match(prereq)
    if m:
        got = [NAMED[m.group(1).lower()]]
        if m.group(2):
            got.append(NAMED[m.group(2).lower()])
        return ("ability", tuple(got), int(m.group(3)))
    if SPELL_PREREQ.match(prereq):
        return ("spell",)
    m = PROF_PREREQ.match(prereq)
    if m:
        return ("prof", squash(m.group(1)))
    return ("other", squash(prereq))


def row_prereq(r):
    """The prerequisite a FEAT row states, in the same shape."""
    text = r.str(2).strip()
    if not text:
        return None
    if r.str(3) != "-":
        got = [r.str(3)] + ([r.str(4)] if r.str(4) != "-" else [])
        return ("ability", tuple(got), r.int(5))
    if r.int(7):
        return ("spell",)
    if r.str(6):
        return ("prof", squash(r.str(6)))
    return ("other", squash(text))


def same_prereq(want, got):
    """Do a row's prerequisite and the book's agree?

    Race names are compared for near-equality, because Xanathar's scan turns
    Halfling into Halffing, Tiefling into Tiefiing and Half-orc into Half-ore.
    """
    if want is None or got is None:
        return want == got
    if want[0] != got[0]:
        return False
    if want[0] == "other":
        return difflib.SequenceMatcher(None, want[1], got[1]).ratio() >= 0.8
    return want == got


def show(prereq):
    if prereq is None:
        return "none"
    if prereq[0] == "ability":
        return "%s %d or higher" % (" or ".join(prereq[1]), prereq[2])
    if prereq[0] == "spell":
        return "the ability to cast a spell"
    return "%s %s" % (prereq[0], prereq[1])


# ----------------------------------------------------------- ability increase


def book_asi(window):
    """Which abilities the printed feat raises: a set, "any", None, or "?".

    "?" means the block holds more than one increase, which is the dump
    having run two feats together rather than a feat granting two.
    """
    said = ASI.findall(squash(flat(window)))
    if not said:
        return None
    if len(said) > 1:
        return "?"
    if ANY_ASI.search(said[0]):
        return "any"
    got = [a for a in ABILITIES if re.search(ABILITY_RE[a], said[0])]
    return tuple(got) if got else "?"


def row_asi(r):
    fixed = [a for n, a in enumerate(ABILITIES) if r.int(8 + n)]
    if fixed:
        return tuple(fixed)
    if r.int(14):
        chosen = [s.strip() for s in r.str(15).split(",") if s.strip()]
        return tuple(chosen) if chosen else "any"
    return None


def show_asi(asi):
    if asi is None:
        return "no increase"
    if asi == "any":
        return "+1 to any one score"
    return "+1 to " + " or ".join(asi)


# ------------------------------------------------------------------ the feats


def check_feats(problems, unchecked):
    checked = 0
    for book in ("PHB", "TCE", "XGE", "SCAG"):
        text = region(book, *FEAT_REGION[book])
        rows = [r for r in read_file("character.txt")
                if r.tag == "FEAT" and r.str(1) == book]
        where = blocks(text, [r.str(0) for r in rows])

        for r in rows:
            name = r.str(0)
            found = where.get(name)
            if not found:
                unchecked.append((name, "no heading for it in the %s dump"
                                  % book))
                continue
            if len(found) > 1:
                unchecked.append((name, "the %s dump prints this heading %d "
                                  "times" % (book, len(found))))
                continue
            _, start, end = found[0]
            window = text[start:end]
            if len(window.strip()) < 60:
                unchecked.append((name, "the dump runs this heading straight "
                                  "into the next one, leaving no text under "
                                  "it"))
                continue
            if len(re.findall(r"Prerequisite", window)) > 1:
                unchecked.append((name, "two prerequisites inside one block; "
                                  "the dump has interleaved the columns here"))
                continue

            printed = None
            for line in window.split("\n")[:6]:
                m = PREREQ_LINE.match(line)
                if m:
                    printed = shape(flat(m.group(1)))
                    break
            if printed is None and "Prerequisite" in window:
                unchecked.append((name, "a prerequisite the dump has moved "
                                  "away from its heading"))
                continue

            checked += 1
            said = row_prereq(r)
            if not same_prereq(said, printed):
                problems.append((name, "prerequisite", show(said),
                                 show(printed)))

            got = book_asi(flat(window))
            mine = row_asi(r)
            if got != mine and not (got and mine and set(got) == set(mine)):
                problems.append((name, "ability score increase",
                                 show_asi(mine), show_asi(got)))
    return checked


# ------------------------------------------ Tasha's optional class features


def sections(text):
    """(start, end, class) for each class's optional-features section."""
    marks = [m.start() for m in re.finditer(r"OPTIONAL CLASS FEATURE", text)]
    out = []
    for n, at in enumerate(marks):
        end = marks[n + 1] if n + 1 < len(marks) else len(text)
        m = re.search(r"you can gain as an? (%s)\." % "|".join(CLASSES),
                      flat(text[at:at + 900]))
        if m:
            out.append((at, end, m.group(1)))
    return out


def check_optfeatures(problems, unchecked):
    text = region("TCE", *OPT_REGION)
    rows = [r for r in read_file("character.txt") if r.tag == "OPTFEATURE"]
    where = blocks(text, sorted({r.str(3) for r in rows}))
    filed = sections(text)

    checked = 0
    for r in rows:
        name, klass, level, replaces = r.str(3), r.str(0), r.int(2), r.str(4)
        found = where.get(name)
        if not found:
            unchecked.append((name, "no heading for it in the Tasha's dump"))
            continue

        # Several classes print a Fighting Style Options or a Martial
        # Versatility of their own, so the copy that belongs to this row is
        # the one whose subhead names this row's class.
        seen = []
        for _, start, end in found:
            window = flat(text[start:min(end, start + 4000)])
            m = LEVEL.search(window[:200])
            seen.append((m, window, start))
        mine = [s for s in seen if s[0] and s[0].group(2).lower()
                == klass.lower()]

        if not mine:
            named = sorted({s[0].group(2).lower() for s in seen if s[0]})
            if named:
                checked += 1
                problems.append((name, "class", klass, " and ".join(named)))
                continue
            # No subhead at all: Maneuver Options and Eldritch Invocation
            # Options are printed without one. Fall back to the section.
            here = [c for start, end, c in filed
                    if any(start <= s[2] < end for s in seen)]
            if len(here) != 1:
                unchecked.append((name, "the dump cannot say which class "
                                  "section this heading sits in"))
                continue
            if here[0].lower() != klass.lower():
                checked += 1
                problems.append((name, "class", klass, here[0]))
                continue
            unchecked.append((name, "level: the book prints no "
                              "\"Nth-level %s feature\" line under this "
                              "heading" % here[0]))
            mine = seen                   # the replaced feature is still there

        checked += 1
        m, window = mine[0][0], mine[0][1]
        if m and int(m.group(1)) != level:
            problems.append((name, "level", str(level), m.group(1)))

        rep = REPLACES.search(window[:300])
        printed = rep.group(1).strip() if rep else ""
        if squash(printed) != squash(replaces):
            problems.append((name, "replaces", replaces or "nothing",
                             printed or "nothing"))
    return checked


def main():
    problems, unchecked = [], []
    checked = check_feats(problems, unchecked)
    checked += check_optfeatures(problems, unchecked)

    for name, field, said, printed in problems:
        print("  %s: %s %s, book says %s" % (name, field, said, printed))
    if unchecked:
        print()
        for name, why in unchecked:
            print("  unchecked: %s: %s" % (name, why))

    print("\n%d checked, %d disagree" % (checked, len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
