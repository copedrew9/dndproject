#!/usr/bin/env python3
"""Check the MPMM race numbers in data/character.txt against the book.

MPMM is the one book whose text in TextFiles/ is still OCR, and its races
were the last rows with nothing checking the numbers beside their names.
There are not many numbers -- these races have no fixed ability increases at
all, so what is left is size, walking speed and darkvision -- but they were
wrong three times out of thirty-three: the air genasi's speed and darkvision,
and the earth genasi's darkvision.

Every trait is a sentence of the form "Speed. Your walking speed is 30 feet."
inside a block headed "<RACE> TRAITS", which is enough to read them back.

Three things the dump does that this has to work around, and which are why a
first pass over it reported nineteen false alarms rather than three real
ones. Sub-races are headed by the variant, not the race: "AIR GENASI TRAITS",
not "GENASI TRAITS". Some pages interleave two columns, so the sentence
giving a darkvision radius can be cut in half by an unrelated clause -- the
triton's is -- which is why the radius is read as the first number after the
trait's name rather than the number before the word "feet". And the orc's
heading does not come through at all, so its traits run on into the block
above and lend the minotaur a darkvision it does not have; rather than guess,
a race whose block cannot be bounded is reported unchecked.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

DUMP = os.path.join(ROOT, "TextFiles", "MORDENKAIDENtext.txt")

HEADING = re.compile(r"[A-Z][A-Z ,'-]{2,30} TRAITS")

# Races whose block cannot be told from the one after it, because the
# heading between them is missing from the dump. Their numbers are the
# ones a person still has to read off the page.
#
# The orc is also the one race whose row deliberately disagrees with this
# dump: it follows the newer printing, which gives darkvision out to 120
# feet, lets Adrenaline Rush recharge on a short rest as well as a long one,
# and drops Powerful Build. Do not "correct" it back against the text here.
UNBOUNDED = {"Minotaur", "Orc"}


def flatten(text):
    """One long line, with the hyphenation of line breaks undone."""
    return re.sub(r"\s+", " ", text).replace("- ", "")


def blocks():
    """The text, and where every traits heading in it starts.

    The headings are looked up by name rather than collected into a table,
    because a running page header sits directly above one of them and would
    otherwise be read as part of it: the harengon's arrives as "RACES
    HARENGON TRAITS".
    """
    flat = flatten(open(DUMP, encoding="utf-8").read())
    return flat, sorted(m.start() for m in HEADING.finditer(flat))


def block_for(flat, starts, heading):
    at = flat.find(heading + " TRAITS")
    if at < 0:
        return None
    end = next((s for s in starts if s > at), len(flat))
    return flat[at:end]


def heading_for(name):
    """"Genasi, Air" is headed "AIR GENASI", not "GENASI, AIR"."""
    name = name.upper()
    if "," in name:
        race, _, variant = name.partition(",")
        return variant.strip() + " " + race.strip()
    return name


def main():
    if not os.path.exists(DUMP):
        sys.exit("verify_races: no %s" % DUMP)

    flat, starts = blocks()
    checked = skipped = missing = bad = 0

    for r in read_file("character.txt"):
        if r.tag != "RACE" or r.str(1) != "MPMM":
            continue
        if r.str(0) in UNBOUNDED:
            skipped += 1
            continue
        block = block_for(flat, starts, heading_for(r.str(0)))
        if block is None:
            print("  no traits block: %s" % r.str(0))
            missing += 1
            continue
        checked += 1
        problems = []

        m = re.search(r"Size\. You(?:r size)? (?:is|are) ([A-Za-z]+)", block)
        if m and m.group(1) != r.str(9):
            problems.append("size %s, book says %s" % (r.str(9), m.group(1)))

        m = re.search(r"walking speed is (\d+) feet", block)
        if m and int(m.group(1)) != r.int(8):
            problems.append("speed %d, book says %s" % (r.int(8), m.group(1)))

        # The radius is the first number after the trait's name. Looking
        # instead for the number before "feet" fails wherever the dump has
        # interleaved two columns and put another clause between the two.
        m = re.search(r"Darkvision\.[^0-9]{0,120}(\d+)", block)
        dark = int(m.group(1)) if m else 0
        if dark != r.int(10):
            problems.append("darkvision %d, book says %d" % (r.int(10), dark))

        if problems:
            bad += 1
            print("  %s" % r.str(0))
            for p in problems:
                print("      %s" % p)

    if skipped:
        print("  %d races left unchecked, their blocks running together in "
              "the dump: %s" % (skipped, ", ".join(sorted(UNBOUNDED))))
    print("\n%d MPMM races checked, %d disagree, %d not found"
          % (checked, bad, missing))
    return 1 if (bad or missing) else 0


if __name__ == "__main__":
    sys.exit(main())
