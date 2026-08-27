#!/usr/bin/env python3
"""Check the deity tables in data/world.txt against appendix B.

Appendix B is four columns -- deity and title, alignment, suggested domains,
symbol -- and the OCR that TextFiles/ used to hold carried them intact for
the Forgotten Realms table and for no other pantheon. The rest were written
out by hand, and comparing them once the text could be read properly found
real mistakes: Vecna's symbol, Incabulos's symbol, and six nonhuman gods
missing altogether.

The rows are produced by tools/extract_deities.py now, and this checks them,
comparing pantheon by pantheon so that the two Tyrs, the two Surturs and the
two Silvanuses are each checked against the right one.
"""

import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file        # noqa: E402
from extract_deities import rows        # noqa: E402

FIELDS = ["title", "pantheon", "alignment", "domains", "symbol"]


def main():
    book = {}
    for r in rows():
        book[(r[2], r[0])] = r

    checked = missing = bad = 0
    for r in read_file("world.txt"):
        if r.tag != "DEITY":
            continue
        want = book.get((r.str(2), r.str(0)))
        if want is None:
            print("  not in appendix B: %s (%s)" % (r.str(0), r.str(2)))
            missing += 1
            continue
        checked += 1
        problems = []
        for n, field in enumerate(FIELDS, start=1):
            if want[n] != r.str(n):
                problems.append("%s %r,\n           book says %r"
                                % (field, r.str(n), want[n]))
        if problems:
            bad += 1
            print("  %s (%s)" % (r.str(0), r.str(2)))
            for p in problems:
                print("      %s" % p)

    extra = len(book) - checked
    if extra > 0:
        print("\n  %d rows in appendix B are not in data/world.txt" % extra)
    print("\n%d deities checked against appendix B, %d disagree, "
          "%d not found" % (checked, bad, missing))
    return 1 if (bad or missing or extra) else 0


if __name__ == "__main__":
    sys.exit(main())
