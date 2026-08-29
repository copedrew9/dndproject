#!/usr/bin/env python3
"""Look for equipment in the PHB's tables that data/ does not have.

tools/verify_equipment.py checks one direction: every ITEM row we hold is
compared, name and numbers, against the book. That direction cannot see a
gap. An item nobody typed in is not a row whose cost disagrees, and not a
name that fails to resolve -- it is simply absent, and the only thing that
notices is a player looking for a glass bottle.

tools/verify_coverage.py closes the same loop for spells and magic items,
which open with a line the books print in a fixed form. An equipment row has
no such line: it is a name, a price, and whatever else its table carries. So
this reads the price rows instead -- every "name / cost / ..." the dump
holds, which is what verify_equipment already parses -- and subtracts
everything data/ knows by any of its names.

Five PHB items were missing when this was written: the glass bottle, the
crossbow bolt case, the map or scroll case, and two of the four gaming sets
(dragonchess and Three-Dragon Ante, which also left the Gaming set tool
group holding half of what the book gives it).

What is left over after the subtraction is not all gaps. The dump's price
rows include the food, drink, lodging and travel tables, whose cells are
priced the same way and are held in data/ as SERVICE rows under names that
read as sentences rather than as table cells -- "Meals, modest (per day)"
for a row the table prints as "Modest". Those are named in NOT_ITEMS below,
each with the row it belongs to, so that the list stays short enough to read
and a new name appearing in it is a real question rather than noise.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file            # noqa: E402
import verify_equipment as E                # noqa: E402

# Price rows the equipment tables do not own. Each is a cell of one of the
# book's other priced tables, where data/ holds the row under a fuller name.
NOT_ITEMS = {
    "Common (pitcher)": "Ale, the Food, Drink, and Lodging table",
    "Fine (bottle)": "Wine, the same table",
    "Mug": "Ale by the mug, the same table",
    "Gallon": "Ale by the gallon, the same table",
    "Goods": "the Trade Goods table's own header cell",
    "Within a city": "Coach cab, the Services table",
}

# The rows data/ holds outside the ITEM table, which are priced in the same
# tables and would otherwise read as gaps.
OTHER_TAGS = ("SERVICE", "LIFESTYLE", "SPELLSERVICE", "MAGICITEM", "TRINKET",
              "GEM")


def main():
    rows = E.book_rows()

    have = set()
    for r in read_file("equipment.txt"):
        if r.tag == "ITEM" or r.tag in OTHER_TAGS:
            have.add(E.key(r.str(0)))
    # A name data/ deliberately spells differently from the book.
    for book_name in E.ALIAS.values():
        have.add(E.key(book_name))
    for name in NOT_ITEMS:
        have.add(E.key(name))

    missing = sorted((name, cost) for k, (name, cost, _) in rows.items()
                     if k not in have)

    for name, cost in missing:
        print("  %-36s %d cp in the book, and not in data/" % (name, cost))

    print("\n%d priced rows in the book, %d not in data/"
          % (len(rows), len(missing)))
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
