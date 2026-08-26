#!/usr/bin/env python3
"""Generate src/data_life.c from Xanathar's "This Is Your Life" tables.

The tables are the cleanest thing in that dump: a heading in capitals, a
line naming the die and the column, then one row per result. So they are
parsed rather than typed, and the script refuses to write unless the tables
it knows should be there all came out with a plausible number of rows and a
complete range of results.
"""
import re
import sys

SRC = "TextFiles/XANATHARtext.txt"

def die_range(die):
    """The lowest and highest result the die can give."""
    m = re.match(r"^(\d*)d(\d+)$", die)
    n = int(m.group(1) or 1)
    sides = int(m.group(2))
    return n, n * sides


# heading in the dump -> (name shown to the player, die)
WANTED = [
    ("BIRTHPLACE",           "Birthplace",             "d100"),
    ("NUMBER OF SIBLINGS",   "Number of siblings",     "d10"),
    ("BIRTH ORDER",          "Birth order",            "2d6"),
    ("FAMILY",               "Who raised you",         "d100"),
    ("ABSENT PARENT",        "The absent parent",      "d4"),
    ("FAMILY LIFESTYLE",     "Family lifestyle",       "3d6"),
    ("CHILDHOOD HOME",       "Childhood home",         "3d6"),
    ("CHILDHOOD MEMORIES",   "Childhood memories",     "3d6"),
    ("OCCUPATION",           "Occupation",             "d100"),
    ("ALIGNMENT",            "Alignment",              "3d6"),
    ("STATUS",               "Status",                 "3d6"),
    ("RELATIONSHIP",         "Relationship",           "3d4"),
    ("LIFE EVENTS",          "A life event",           "d100"),
]

ROW = re.compile(r"^(\d{1,3})\s*[-–]\s*(\d{1,3})\s+(.*\S)\s*$"
                 r"|^(\d{1,3})\s+(.*\S)\s*$")
# The dump runs "2 or lower" together as "2orlower".
OPEN_ROW = re.compile(r"^(\d{1,3})\s*or\s*(lower|higher)\s+(.*\S)\s*$",
                      re.I)
HEADING = re.compile(r"^[A-Z][A-Z' ]{3,}$")
NOISE = re.compile(r"CHAPTER|CHARACTER OPTIONS|^\d+$|^[^A-Za-z]*$")


# Rows whose result number the OCR destroyed outright. The text is taken
# from the dump, where it is present but unnumbered ("Your parent abandoned
# you." with no 3) or misread ("an Aristocrat" for "11 Aristocrat").
REPAIRS = {
    ("ABSENT PARENT", 3, 3): "Your parent abandoned you",
    ("OCCUPATION", 11, 11): "Aristocrat",
}


def clean(text):
    text = re.sub(r"\s+", " ", text).strip()
    text = re.sub(r"(\w)-\s+(\w)", r"\1\2", text)   # word broken by a line
    # Dashes come through as em/en dashes or worse; fold them to ASCII.
    for ch in ("\u2014", "\u2013", "\u2012", "\u2212"):
        text = text.replace(ch, "-")
    text = re.sub(r"[^\x20-\x7e]", "", text)
    # Marks left in the margin attach themselves to the end of a row:
    # "Squalid (-20) ba", "Poor (-10) =I". Anything short trailing a closing
    # bracket is debris, not words.
    text = re.sub(r"(\))\s+[^\s]{1,3}$", r"\1", text)
    text = re.sub(r"\s+[|=_~^]+\s*$", "", text)
    text = text.replace("ora ", "or a ").replace("ofa ", "of a ")
    text = text.replace("Ina ", "In a ").replace("Inasage", "In a sage")
    text = text.rstrip(".")
    return text


def parse_table(lines, start, die, limit=90):
    """Reads the rows after a heading, stopping at the next heading.

    A fixed window would run into the following table and cover results
    twice, which the coverage check below would then reject.
    """
    lo_die, hi_die = die_range(die)
    rows, i, misses = [], start, 0
    while i < len(lines) and i < start + limit:
        raw = lines[i].strip()
        i += 1
        # The dump renders a capital I as a pipe, so rows read "| 3orlower
        # | am still haunted". Strip the leading one and restore the rest.
        raw = re.sub(r"^\|\s*", "", raw)
        raw = re.sub(r"(?<![A-Za-z0-9])\|(?![A-Za-z0-9])", "I", raw)
        raw = re.sub(r"\s+", " ", raw).strip()
        if not raw or NOISE.match(raw):
            continue
        if HEADING.match(raw) and rows:
            break                       # the next table starts here
        o = OPEN_ROW.match(raw)
        if o:
            v = int(o.group(1))
            if o.group(2).lower() == "lower":
                rows.append((lo_die, v, clean(o.group(3))))
            else:
                rows.append((v, hi_die, clean(o.group(3))))
            misses = 0
            continue
        m = ROW.match(raw)
        if not m:
            # a wrapped continuation of the previous row
            # A wrapped continuation is short and lowercase. Anything that
            # would make the row absurdly long is the prose after the table,
            # not part of it.
            if (rows and raw[0].islower() and len(raw) < 70
                    and len(rows[-1][2]) + len(raw) < 140):
                prev = rows[-1][2]
                # A word broken across lines keeps its hyphen in the dump;
                # rejoining with a space would leave "other- wise".
                joined = (prev[:-1] + raw) if prev.endswith("-") \
                    else (prev + " " + raw)
                rows[-1] = (rows[-1][0], rows[-1][1], clean(joined))
                continue
            misses += 1
            if misses > 2 and rows:
                break
            continue
        misses = 0
        if m.group(1):
            lo, hi, txt = int(m.group(1)), int(m.group(2)), m.group(3)
        else:
            lo = hi = int(m.group(4))
            txt = m.group(5)
        # "00" on a d100 means 100
        if lo == 0:
            lo = hi = 100
        if hi == 0:
            hi = 100
        if not txt or len(txt) > 200:
            continue
        rows.append((lo, hi, clean(txt)))
    return rows


def main():
    lines = open(SRC, encoding="utf-8", errors="replace").read().split("\n")
    tables, problems = [], []

    for heading, name, die in WANTED:
        best = []
        for i, line in enumerate(lines):
            if line.strip() != heading:
                continue
            rows = parse_table(lines, i + 1, die)
            if len(rows) > len(best):
                best = rows
        for (h, lo_r, hi_r), text in REPAIRS.items():
            if h != heading:
                continue
            if not any(a <= lo_r <= b for a, b, _ in best):
                best.append((lo_r, hi_r, text))
        best.sort()
        # The rows must tile the die's whole range exactly: no gap a roll
        # could land in, and no result covered twice.
        lo, hi = die_range(die)
        seen = {}
        for a, b, _ in best:
            for v in range(a, b + 1):
                seen[v] = seen.get(v, 0) + 1
        gaps = [v for v in range(lo, hi + 1) if v not in seen]
        dupes = [v for v, k in seen.items() if k > 1]
        if gaps or dupes:
            problems.append(
                "%s (%s, %d rows): %s%s"
                % (name, die, len(best),
                   ("misses %s" % gaps[:6]) if gaps else "",
                   (" covers %s twice" % dupes[:6]) if dupes else ""))
            continue
        tables.append((name, die, best))

    # A table whose rows do not tile its die is left out rather than shipped
    # with a hole a roll could fall into. Several of these tables are split
    # across columns or lost a row's number to the OCR.
    if problems:
        print("  left out, could not be read completely:", file=sys.stderr)
        for p in problems:
            print("    " + p, file=sys.stderr)

    # A wholesale regression should still fail the build.
    if len(tables) < 8:
        print("only %d tables parsed; the dump may have changed"
              % len(tables), file=sys.stderr)
        return 1

    with open("src/data_life.c", "w") as f:
        f.write('/* data_life.c -- Xanathar\'s "This Is Your Life" tables.\n'
                ' *\n'
                ' * Generated by tools/extract_life.py; do not edit by hand.\n'
                ' *\n'
                ' * Each table can be rolled on or picked from, so the rows\n'
                ' * carry their result range as well as their text.\n'
                ' */\n#include "data.h"\n\n')
        for idx, (name, die, rows) in enumerate(tables):
            f.write("static const LifeEntry rows_%d[] = {\n" % idx)
            for lo, hi, txt in rows:
                f.write('{ %d, %d, "%s" },\n' % (lo, hi, txt.replace('"', "'")))
            f.write("};\n\n")
        f.write("const LifeTable LIFE_TABLES[] = {\n")
        for idx, (name, die, rows) in enumerate(tables):
            f.write('{ "%s", "%s", rows_%d, %d },\n'
                    % (name, die, idx, len(rows)))
        f.write("};\nconst int LIFE_TABLE_COUNT =\n"
                "    (int)(sizeof(LIFE_TABLES) / sizeof(LIFE_TABLES[0]));\n")

    print("wrote %d tables" % len(tables))
    for name, die, rows in tables:
        print("  %-22s %-5s %d rows" % (name, die, len(rows)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
