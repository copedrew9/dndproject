#!/usr/bin/env python3
"""Check the reference tables -- the odds and ends nothing else covers.

The service and lifestyle prices, the conditions, the weapon properties, the
tool groups, the trinkets and Xanathar's This Is Your Life tables. None of
them is a stat block or a spell, so the checks written for those never
reached them, and between them they are three hundred rows.

What is checked here is what is data. The prose beside these rows is not:
this program writes its own descriptions rather than reproducing the books'
(dnd.h says so at the top, and it is why a lifestyle's paragraph reads
nothing like the PHB's), so comparing the wording would report the project's
own house style as a hundred errors. The first draft of this script did
exactly that. What can be compared is the numbers, the names and the shape:

  Prices, against the book's own tables, in the book's own units. A price is
  only confirmed when the number AND its unit are found beside the name --
  which is the check that matters here, because both errors it found were a
  price in silver stored as though it were copper.

  Names, against the book that prints them: a condition, a weapon property,
  and every tool named in a tool group.

  Shape, for the life tables: each table's rows must cover its die exactly
  once, from 1 to the number of sides, with no gap and no overlap. A gap is
  a roll the program could make and then find nothing for.

The six spellcasting-service prices are ours, not the books': no table of
them appears in any of the seven, and they are named here rather than
reported as missing.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file                          # noqa: E402

PHB = os.path.join(ROOT, "TextFiles", "PHBtext.txt")

# Prices this program supplies because no book prints them.
OURS = {
    "SPELLSERVICE": "no book of the seven prints a table of spellcasting "
                    "fees; these are the program's own suggestion",
}

# The name in data/ is written for a menu; the book's table splits the same
# entry over a heading and a row ("Coach cab" / "Between towns"). This maps
# each row to the words the book's own line uses.
PRICE_NAME = {
    "Inn stay, squalid (per day)": ("Squalid", "7 cp"),
    "Inn stay, poor (per day)": ("Poor", "1 sp"),
    "Inn stay, modest (per day)": ("Modest", "5 sp"),
    "Inn stay, comfortable (per day)": ("Comfortable", "8 sp"),
    "Inn stay, wealthy (per day)": ("Wealthy", "2 gp"),
    "Inn stay, aristocratic (per day)": ("Aristocratic", "4 gp"),
    "Meals, squalid (per day)": ("Squalid", "3 cp"),
    "Meals, poor (per day)": ("Poor", "6 cp"),
    "Meals, modest (per day)": ("Modest", "3 sp"),
    "Meals, comfortable (per day)": ("Comfortable", "5 sp"),
    "Meals, wealthy (per day)": ("Wealthy", "8 sp"),
    "Meals, aristocratic (per day)": ("Aristocratic", "2 gp"),
    "Ale, gallon": ("Gallon", "2 sp"),
    "Ale, mug": ("Mug", "4 cp"),
    "Banquet (per person)": ("Banquet (per person)", "10 gp"),
    "Bread, loaf": ("Bread, loaf", "2 cp"),
    "Cheese, hunk": ("Cheese, hunk", "1 sp"),
    "Meat, chunk": ("Meat, chunk", "3 sp"),
    "Wine, common (pitcher)": ("Common (pitcher)", "2 sp"),
    "Wine, fine (bottle)": ("Fine (bottle)", "10 gp"),
    "Coach cab, between towns (per mile)": ("Between towns", "3 cp per mile"),
    "Coach cab, within a city": ("Within a city", "1 cp"),
    "Hireling, skilled (per day)": ("Skilled", "2 gp per day"),
    "Hireling, untrained (per day)": ("Untrained", "2 sp per day"),
    "Messenger (per mile)": ("Messenger", "2 cp per mile"),
    "Road or gate toll": ("Road or gate toll", "1 cp"),
    "Ship's passage (per mile)": ("Ship's passage", "1 sp per mile"),
    "Stabling (per day)": ("Stabling (per day)", "5 sp"),
}

LIFESTYLE_PRICE = {
    "Wretched": None,                 # the book prints a dash
    "Squalid": "1 sp", "Poor": "2 sp", "Modest": "1 gp",
    "Comfortable": "2 gp", "Wealthy": "4 gp", "Aristocratic": "10 gp",
}

UNIT = {"cp": 1, "sp": 10, "gp": 100}


def squash(s):
    return re.sub(r"[^a-z0-9]", "", s.lower())


def in_copper(price):
    """"2 sp per day" -> 20."""
    m = re.match(r"(\d+)\s*(cp|sp|gp)", price)
    return int(m.group(1)) * UNIT[m.group(2)] if m else None


class Report:
    def __init__(self):
        self.checked = 0
        self.problems = []
        self.skipped = []
        self.ours = []

    def ok(self):
        self.checked += 1

    def bad(self, what, why):
        self.checked += 1
        self.problems.append((what, why))

    def unchecked(self, what, why):
        self.skipped.append((what, why))


def check_prices(rows, table, book_sq, report, kind):
    """A price is confirmed when the book's own line for it says so.

    The expected line is written out in the table above rather than searched
    for, because the book splits these entries across a heading and a row
    and the two halves cannot be paired up from the text alone. What the
    script checks is that the line is really in the book, and that our
    number is what that line says in copper.
    """
    for r in rows:
        name, cp = r.str(0), r.int(1)
        if name not in table:
            report.unchecked("%s %s" % (kind, name),
                             "no line of the book's table is paired with it")
            continue
        label, price = table[name]
        if price is None:
            if cp != 0:
                report.bad("%s %s" % (kind, name),
                           "the book prints no price; ours is %d cp" % cp)
            else:
                report.ok()
            continue
        if squash(label) not in book_sq or squash(price) not in book_sq:
            report.unchecked("%s %s" % (kind, name),
                             "the dump has lost %r or %r" % (label, price))
            continue
        want = in_copper(price)
        if want != cp:
            report.bad("%s %s" % (kind, name),
                       "the book says %s, which is %d cp; ours is %d"
                       % (price, want, cp))
        else:
            report.ok()


def check_names(rows, field, book_sq, report, kind):
    for r in rows:
        name = r.str(field)
        if squash(name) in book_sq:
            report.ok()
        else:
            report.bad("%s %s" % (kind, name), "the book does not name it")


# One table's rows deliberately run past its die: the book rolls Childhood
# Home with the Family Lifestyle modifier added, so it prints results for 0
# and below and for 111 and above. This program rolls the die alone, and
# keeps the extremes so that a player choosing from the table rather than
# rolling can still pick them.
MODIFIED = {"Childhood home"}


def check_life_shape(rows, tables, report):
    """Every table's rows must cover its die exactly once."""
    seen = {}
    for r in rows:
        seen.setdefault(r.str(0), []).append((r.int(1), r.int(2)))
    for t in tables:
        name, die = t.str(0), t.str(1)
        # "d100", but also "3d6" and "3d4": a table rolled on several dice
        # starts at the number of them, not at 1.
        m = re.match(r"^(\d*)d(\d+)$", die)
        if not m:
            report.unchecked(name, "a die this cannot read: %r" % die)
            continue
        count = int(m.group(1) or 1)
        lowest, highest = count, count * int(m.group(2))
        if name in MODIFIED:
            lowest, highest = None, None
        ranges = sorted(seen.get(name, []))
        if not ranges:
            report.unchecked(name, "the table has no rows")
            continue
        at = lowest if lowest is not None else ranges[0][0]
        for lo, hi in ranges:
            if lo != at or hi < lo:
                report.bad(name, "its rows run to %d and then start at %d"
                                 % (at - 1, lo))
                break
            at = hi + 1
        else:
            if highest is not None and at - 1 != highest:
                report.bad(name, "its rows stop at %d, and %s reaches %d"
                                 % (at - 1, die, highest))
            else:
                report.ok()


def main():
    by_tag = {}
    for r in read_file("equipment.txt") + read_file("world.txt"):
        by_tag.setdefault(r.tag, []).append(r)

    book = open(PHB, encoding="utf-8", errors="replace").read()
    book_sq = squash(book)
    report = Report()

    check_prices(by_tag.get("SERVICE", []), PRICE_NAME, book_sq, report,
                 "service")
    check_prices([r for r in by_tag.get("LIFESTYLE", [])],
                 {r.str(0): (r.str(0), LIFESTYLE_PRICE.get(r.str(0)))
                  for r in by_tag.get("LIFESTYLE", [])},
                 book_sq, report, "lifestyle")
    check_names(by_tag.get("CONDITION", []), 0, book_sq, report, "condition")
    check_names(by_tag.get("WEAPONPROP", []), 0, book_sq, report, "property")
    check_names(by_tag.get("TOOLGROUP", []), 1, book_sq, report, "tool")
    check_life_shape(by_tag.get("LIFEROW", []), by_tag.get("LIFETABLE", []),
                     report)

    trinkets = by_tag.get("TRINKET", [])
    if len(trinkets) != 100:
        report.bad("the trinket table",
                   "has %d rows; the book's has 100" % len(trinkets))
    else:
        report.ok()

    for tag, why in OURS.items():
        report.ours.append((tag, len(by_tag.get(tag, [])), why))

    for what, why in report.problems:
        print("  %s" % what)
        print("      %s" % why)
    for tag, n, why in report.ours:
        print("\n  %d %s rows are ours rather than a book's:\n      %s"
              % (n, tag, why))
    if report.skipped:
        print("\n  %d left unchecked:" % len(report.skipped))
        for what, why in report.skipped:
            print("      %-44s %s" % (what[:44], why))

    print("\n%d reference rows checked, %d disagree"
          % (report.checked, len(report.problems)))
    return 1 if report.problems else 0


if __name__ == "__main__":
    sys.exit(main())
