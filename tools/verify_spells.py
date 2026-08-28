#!/usr/bin/env python3
"""Check every SPELL row in data/spells.txt against the book it names.

A spell entry has the same shape on every page: the name, a line reading
"3rd-level evocation" or "Evocation cantrip" (with "(ritual)" appended for a
ritual), then Casting Time:, Range:, Components: and Duration: lines, the
duration reading "Concentration, up to 1 minute" where the spell needs
concentration. That shape carries eight of the eleven columns, so all eight
are read back out of the dump and compared: level, school, ritual, casting
time, range, components (the V/S/M letters and the material text), duration
and the concentration flag.

Every spell is checked against the book its row names -- 361 in the PHB, 95
in Xanathar's, 21 in Tasha's -- and each of those books holds exactly that
many entries, so a row that cannot be found is reported rather than skipped.

What the typesetting does, and what is done about it:

  * The PHB sets spell names in letter-spaced small caps ("A c i d S p l a s h")
    and Xanathar's in full caps, so names are matched on their letters alone,
    with the name read as the one, two or three lines above the level line.
  * Three headings in Xanathar's are damaged past that: an apostrophe-s comes
    back as "1S" ("AGANAZZAR / 1S SCORCHER", "MELF / 1S MINUTE METEORS") and
    "TIDAL WAVE" as "1)DAL WAVE". Those three are named in OCR_HEADING below
    rather than guessed at by fuzzy matching.
  * The level line loses characters too: "8 th-level", "8th-Ievel",
    "l st-/eve/", "Stli-level". The digit and the ordinal are read with the
    usual letter-for-digit confusions allowed; a level line that still will
    not parse leaves that one spell's level unchecked.
  * Words are set with a space inside them ("consum es", "o f", "sham rock")
    and line breaks hyphenate ("hu- manoid"), so the text fields are compared
    with the hyphenation undone and all spaces removed. Spacing alone is
    therefore never reported -- "V,S" against the book's "V, S" is not a
    disagreement here.
  * A page marker, a blank line or a running header can land between the
    Components: line and its continuation; Flame Arrows has the heading "F1NO
    GREATER STEED" sitting inside its entry. Blank lines, page markers and
    lines of three or more capitals with no lower case are dropped from a
    field's continuation lines. Three capitals rather than one, because a
    line break can leave a price on a line of its own -- astral projection's
    material has "1,000" alone on one -- and that is not an intrusion.
  * The dump misreads letters for each other in the fields themselves:
    Infestation's components arrive as "Y, S, M", Tasha's Otherworldly
    Guise's as "V, $, M", and Toll the Dead's casting time as "l action".
    Those confusions are undone on the book's side only -- never on the
    data's -- so that the same misreading sitting in data/ is still reported.
    A book value that is only a letter away is thus given the benefit of the
    doubt, which is the safe direction here.

Deliberately unchecked: the classes column. Which classes get a spell is set
out in the class spell lists of chapter 3 and in Xanathar's and Tasha's
additions to them, not in the spell entry, so nothing in the entry's shape
can settle it; it wants a verifier of its own against those lists. The name
column is checked only in so far as a row has to match a heading before
anything else can be compared, and a row whose heading cannot be found is
reported rather than passed over. Nothing here checks the spell's text.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

BOOKS = {"PHB": "PHBtext.txt",
         "XGE": "XANATHARtext.txt",
         "TCE": "TASHAtext.txt"}

SCHOOLS = ["abjuration", "conjuration", "divination", "enchantment",
           "evocation", "illusion", "necromancy", "transmutation"]
SCH = "|".join(SCHOOLS)

# "3rd-level evocation (ritual)". The prefix is left loose because the dump
# breaks it in a different way each time, and is parsed separately below.
LEVEL_LINE = re.compile(r"^(.{0,14}?)(%s)(\(ritual\))?$" % SCH)
CANTRIP_LINE = re.compile(r"^(%s)cantrip(\(ritual\))?$" % SCH)
ORDINAL = re.compile(r"^([0-9slio|])(?:st|nd|rd|t[hlni]{1,2})-?[li/]eve[li/]$")
FOR_DIGIT = {"s": "5", "l": "1", "i": "1", "|": "1", "o": "0"}

FIELD = re.compile(r"^\s*(Casting\s*Time|Range|Com\s*ponents?|Duration)"
                   r"\s*[:.]\s*", re.I)
PAGE = re.compile(r"^\s*(===\s*page\s*\d+\s*===|\d+)\s*$")
SHOUT = re.compile(r"^(?=(?:[^A-Za-z]*[A-Z]){3})[^a-z]*$")

# Headings the dump mangles past recognition, and what it leaves behind.
OCR_HEADING = {
    "Aganazzar's Scorcher": "aganazzar1sscorcher",
    "Melf's Minute Meteors": "melf1sminutemeteors",
    "Tidal Wave": "1dalwave",
}


def tidy(s):
    """Curly quotes and long dashes back to ASCII, whitespace collapsed."""
    s = s.replace("’", "'").replace("‘", "'")
    s = s.replace("“", '"').replace("”", '"')
    s = s.replace("—", "-").replace("–", "-").replace("­", "")
    return re.sub(r"\s+", " ", s).strip()


def squash(s):
    """The form the text fields are compared in: no case, no spaces.

    A line break inside a word leaves a hyphen behind, and the extraction
    puts spaces inside words that the page never had, so neither can be
    allowed to count. Punctuation is kept -- a comma where the book has a
    full stop is a real difference.
    """
    s = tidy(s).lower()
    s = re.sub(r"(?<=[a-z])- (?=[a-z])", "", s)
    s = re.sub(r"[.·;:]+$", "", s)
    return s.replace(" ", "")


def letters(s):
    return re.sub(r"[^a-z0-9]", "", tidy(s).lower())


def compact(s):
    """A level line with its spacing removed, for the shape patterns."""
    return re.sub(r"[^A-Za-z0-9()'/|]", "", tidy(s)).lower()


def undigit(s):
    """A lone l, I or | in a book value is a misread 1.

    Only ever applied to the book's side. Applying it to data/ as well would
    hide exactly the rows worth finding.
    """
    return re.sub(r"(?<![A-Za-z0-9])[lI|](?=[ ,]|$)", "1", s)


def is_level_line(line):
    c = compact(line)
    if CANTRIP_LINE.match(c):
        return True
    return bool(LEVEL_LINE.match(c)) and not c.startswith("(")


def read_level(line):
    """(level, school, ritual) from a level line; level None if unreadable."""
    c = compact(line)
    m = CANTRIP_LINE.match(c)
    if m:
        return 0, m.group(1), bool(m.group(2))
    m = LEVEL_LINE.match(c)
    o = ORDINAL.match(m.group(1))
    if not o:
        return None, m.group(2), bool(m.group(3))
    d = FOR_DIGIT.get(o.group(1), o.group(1))
    return int(d), m.group(2), bool(m.group(3))


def entries(path):
    """Every spell entry in one book, in the order it is printed."""
    lines = open(path, encoding="utf-8").read().split("\n")
    out = []
    for i, line in enumerate(lines):
        if not line.strip().startswith("Casting Time:"):
            continue
        lvl = None
        for j in range(i - 1, max(i - 7, -1), -1):
            if is_level_line(lines[j]):
                lvl = j
                break
        if lvl is None:
            continue

        marks = []
        for j in range(i, min(i + 18, len(lines))):
            m = FIELD.match(lines[j])
            if m:
                name = m.group(1).lower().replace(" ", "")
                marks.append((j, "components" if name.startswith("com")
                              else name, lines[j][m.end():]))
            if len(marks) == 4:
                break
        fields = {}
        for n, (j, name, first) in enumerate(marks):
            end = marks[n + 1][0] if n + 1 < len(marks) else j + 1
            parts = [first]
            for x in range(j + 1, end):
                t = lines[x]
                if not t.strip() or PAGE.match(t) or SHOUT.match(t):
                    continue
                parts.append(t)
            fields[name] = tidy(" ".join(parts))

        names, run = [], ""
        for j in range(lvl - 1, max(lvl - 4, -1), -1):
            run = letters(lines[j]) + run
            names.append(run)
        out.append({"names": names, "level": lines[lvl], "f": fields,
                    "line": i + 1})
    return out


def components(s, repair):
    """(the V/S/M letters, the material text) or (None, ...) if malformed.

    On the book's side the letters are picked out one by one, so that the
    stray full stop in "V,. S, M" and the misread "Y" and "$" do not matter.
    On data/'s side the whole prefix has to be a well formed list, because
    "Y, S" and "V. S, M" sitting in a data file are the point of the check.
    """
    head, paren, rest = s.partition("(")
    material = (paren + rest) if paren else ""
    if repair:
        seq = [{"y": "v", "$": "s"}.get(c, c)
               for c in tidy(head).lower() if c in "vsmy$"]
        return "".join(seq), material
    head = tidy(head).rstrip(",").strip()
    if not re.match(r"^[VSM](\s*,\s*[VSM])*$", head):
        return None, material
    return "".join(t.strip().lower() for t in head.split(",")), material


def shorten(s, n=96):
    return s if len(s) <= n else s[:n - 24] + " ... " + s[-20:]


def main():
    book = {}
    for tag, name in BOOKS.items():
        path = os.path.join(ROOT, "TextFiles", name)
        if not os.path.exists(path):
            sys.exit("verify_spells: no %s" % path)
        book[tag] = entries(path)

    checked = missing = bad = 0
    unread = []
    for r in read_file("spells.txt"):
        if r.tag != "SPELL":
            continue
        tag = r.str(1)
        if tag not in book:
            print("  %s: book %s has no text in TextFiles/" % (r.str(0), tag))
            missing += 1
            continue
        k = OCR_HEADING.get(r.str(0)) or letters(r.str(0))
        # A dropped decorative initial is only fallen back on when the whole
        # name matches nothing, or "Blight" would also answer to "Light".
        hits = [e for e in book[tag] if k in e["names"]]
        if not hits:
            hits = [e for e in book[tag] if k[1:] in e["names"]]
        if len(hits) != 1:
            print("  %s (%s): %s in %s" % (
                r.str(0), tag,
                "no entry" if not hits else "%d entries" % len(hits), tag))
            missing += 1
            continue

        e = hits[0]
        checked += 1
        problems = []

        level, school, ritual = read_level(e["level"])
        if level is None:
            unread.append(r.str(0))
        elif level != r.int(2):
            problems.append("level %d, book says %d" % (r.int(2), level))
        if school != r.str(3).lower():
            problems.append("school %s, book says %s"
                            % (r.str(3), school.title()))
        if ritual != bool(r.int(4)):
            problems.append("ritual %d, book says %s"
                            % (r.int(4), "(ritual)" if ritual else "no"))

        for name, col in (("castingtime", 6), ("range", 7)):
            want = undigit(e["f"].get(name, ""))
            if squash(want) != squash(r.str(col)):
                problems.append("%s %r,\n           book says %r"
                                % (name, r.str(col), shorten(want)))

        got, gotmat = components(r.str(8), repair=False)
        want, wantmat = components(e["f"].get("components", ""), repair=True)
        if got is None:
            problems.append("components %r are not the book's %s"
                            % (shorten(r.str(8).split("(")[0].strip()),
                               ", ".join(want.upper()) or "V, S, M"))
        elif got != want:
            problems.append("components %s, book says %s"
                            % (", ".join(got.upper()),
                               ", ".join(want.upper())))
        if squash(gotmat) != squash(wantmat):
            problems.append("material %r,\n           book says %r"
                            % (shorten(gotmat), shorten(wantmat)))

        dur = undigit(e["f"].get("duration", ""))
        if squash(dur) != squash(r.str(9)):
            problems.append("duration %r, book says %r"
                            % (r.str(9), shorten(dur)))
        conc = squash(dur).startswith("concentration")
        if conc != bool(r.int(5)):
            problems.append("concentration %d, book's duration is %r"
                            % (r.int(5), shorten(dur)))

        if problems:
            bad += 1
            print("  %s (%s, %s:%d)" % (r.str(0), tag, BOOKS[tag], e["line"]))
            for p in problems:
                print("      %s" % p)

    if unread:
        print("\n  %d level line%s too damaged to read, so that spell's "
              "level is unchecked: %s"
              % (len(unread), "" if len(unread) == 1 else "s",
                 ", ".join(sorted(unread))))
    print("\n%d spells checked against the books, %d disagree, %d not found"
          % (checked, bad, missing))
    return 1 if (bad or missing) else 0


if __name__ == "__main__":
    sys.exit(main())
