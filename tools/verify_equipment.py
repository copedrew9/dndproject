#!/usr/bin/env python3
"""Check the equipment numbers in data/equipment.txt against the book.

tools/audit.py checks that every name in data/ appears in the book. This
checks the numbers beside the names, for the one table where getting them
wrong is silent and expensive: a wrong armour value is wrong on every sheet,
and nothing else would ever catch it.

It reads the Armor, Weapons, Adventuring Gear, Tools, Mounts, Tack, Vehicles
and Trade Goods tables out of TextFiles/PHBtext.txt and compares cost,
weight, damage, armour class, Strength requirement and stealth against the
ITEM rows. The names in data/ are chosen to read well in a shop menu, so a
few differ from the book's ("Heavy crossbow" for "Crossbow, heavy"); those
are mapped below rather than changed, and an unmapped name that cannot be
found is reported rather than skipped.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

DUMP = os.path.join(ROOT, "TextFiles", "PHBtext.txt")

# Where each table sits, by the heading the extraction puts on its own line.
# A table runs from its header row to the first line that is clearly prose.
TABLES = ["A r m o r", "W e a p o n s", "A d v e n t u r i n g G e a r",
          "T o o l s", "M o u n t s a n d O t h e r A n i m a l s",
          "T a c k , H a r n e s s , a n d D r a w n V e h i c l e s",
          "W a t e r b o r n e V e h i c l e s"]

COIN = {"cp": 1, "sp": 10, "ep": 50, "gp": 100, "pp": 1000}

# data/ name -> the book's name, where the two differ on purpose.
ALIAS = {
    "Hide armor": "Hide", "Splint armor": "Splint", "Plate armor": "Plate",
    "Ring mail armor": "Ring mail", "Chain mail armor": "Chain mail",
    "Scale mail armor": "Scale mail", "Padded armor": "Padded",
    "Leather armor": "Leather", "Studded leather armor": "Studded leather",
    "Light crossbow": "Crossbow, light",
    "Hand crossbow": "Crossbow, hand",
    "Heavy crossbow": "Crossbow, heavy",
    "Orb (arcane focus)": "Orb", "Rod (arcane focus)": "Rod",
    "Staff (arcane focus)": "Staff", "Wand (arcane focus)": "Wand",
    "Crystal (arcane focus)": "Crystal",
    "Sprig of mistletoe (druidic focus)": "Sprig of mistletoe",
    "Totem (druidic focus)": "Totem",
    "Wooden staff (druidic focus)": "Wooden staff",
    "Yew wand (druidic focus)": "Yew wand",
    "Amulet (holy symbol)": "Amulet", "Emblem (holy symbol)": "Emblem",
    "Reliquary (holy symbol)": "Reliquary",
    "Draft horse": "Horse, draft", "Riding horse": "Horse, riding",
    "Saddle, riding": "Riding", "Saddle, exotic": "Exotic",
    "Saddle, military": "Military", "Saddle, pack": "Pack",
}

# The packs are priced in prose rather than in a table.
PACK = re.compile(r"([A-Z][a-z]+)'s Pack \((\d+) gp\)")

# Which columns a category's table actually has. A mount's table gives a
# speed and a carrying capacity where a gear table gives a weight, so
# comparing weights there would compare a camel against what a camel can
# carry. A pack's contents are prose, so only its price is in a table.
ARMOURS = ("light-armour", "medium-armour", "heavy-armour", "shield")
WEAPONS = ("simple-melee", "simple-ranged", "martial-melee",
           "martial-ranged")
WEIGHED = ARMOURS + WEAPONS + ("gear", "tool")


def clean(s):
    """The extraction keeps the book's curly apostrophes and stray spaces."""
    s = s.replace("’", "'").replace("‘", "'")
    s = s.replace("—", "-").replace("–", "-")
    return re.sub(r"\s+", " ", s).strip()


def key(s):
    return re.sub(r"[^a-z0-9]", "", clean(s).lower())


def parse_money(s):
    """'1,500 gp' -> 150000 copper. Returns None when it is not a price."""
    m = re.match(r"^([\d,]+)\s*(cp|sp|ep|gp|pp)$", clean(s))
    if not m:
        return None
    return int(m.group(1).replace(",", "")) * COIN[m.group(2)]


def parse_weight(s):
    """'1 1/2 lb.' -> 15 tenths of a pound. '-' -> 0."""
    s = clean(s)
    if s in ("-", ""):
        return 0
    m = re.match(r"^([\d,]*)\s*(?:(\d)/(\d))?\s*lb\.?", s.replace(" 0 ", "0 "))
    if not m:
        return None
    whole = int(m.group(1).replace(",", "").replace(" ", "")) if m.group(1) \
        else 0
    tenths = whole * 10
    if m.group(2):
        tenths += int(round(10.0 * int(m.group(2)) / int(m.group(3))))
    return tenths


def book_rows():
    """Every 'name, cost, ...' row the PHB equipment tables carry."""
    text = open(DUMP, encoding="utf-8").read()
    lines = [clean(l) for l in text.split("\n")]
    rows, i = {}, 0

    while i < len(lines):
        # A row is a name followed by a price. What comes after the price
        # depends on the table, so everything up to the next name is kept.
        cost = parse_money(lines[i + 1]) if i + 1 < len(lines) else None
        name = lines[i]
        if cost is None or not name or len(name) > 44 or name[0].isdigit():
            i += 1
            continue
        rest = []
        j = i + 2
        while j < len(lines) and j < i + 6:
            if parse_money(lines[j]) is not None:
                break
            rest.append(lines[j])
            j += 1
        rows.setdefault(key(name), (name, cost, rest))
        i += 1
    return rows


ARMOUR_AC = re.compile(r"^(\d+)(?:\s*\+\s*Dex modifier(?:\s*\(max (\d+)\))?)?$")
DAMAGE = re.compile(r"^(\d+d\d+|\d+)\s+(\w+)$")


# data/ category -> the name the book's own don/doff table gives it.
ARMOUR_CATEGORY = {
    "light-armour": "Light Armor", "medium-armour": "Medium Armor",
    "heavy-armour": "Heavy Armor", "shield": "Shield",
}


def donning(text):
    """The PHB's Getting Into and Out of Armor table, read out of the dump.

    Four rows of three cells, one cell to a line, under a header that is
    itself three lines. Read rather than written down here, for the same
    reason as everything else in this file.
    """
    lines = [l.strip() for l in text.split("\n")]
    for i in range(len(lines) - 14):
        if lines[i:i + 3] != ["Category", "Don", "Doff"]:
            continue
        out, at = {}, i + 3
        for _ in range(4):
            if at + 2 >= len(lines):
                break
            out[lines[at]] = (lines[at + 1], lines[at + 2])
            at += 3
        if len(out) == 4:
            return out
    return {}


def check_notes(items, notes, times):
    """The armour notes, against the table and against the item's own row.

    A note is prose, so what is checked is what is fact: the time to put the
    armour on and take it off, whether it says anything about Stealth, and
    which Dexterity rule its category follows. Half plate said it took five
    minutes to take off, which is heavy armour's figure, and nothing noticed
    for as long as nothing read the notes.
    """
    if not times:
        return 0, 0, ["the don and doff table is not in the dump"]
    checked = bad = 0
    problems = []
    for r in items:
        cat = ARMOUR_CATEGORY.get(r.str(2))
        if cat is None or r.str(1) != "PHB":
            continue
        note = notes.get(r.str(0))
        if note is None:
            problems.append("%s has no note" % r.str(0))
            continue
        checked += 1
        don, doff = times[cat]
        low = note.lower()
        mine = []
        # "1 action" is written "an action" in the notes, which is the same
        # thing said the way a sentence says it.
        if don.endswith("action"):
            if "action" not in low:
                mine.append("the note does not say it takes an action")
        else:
            if don.lower() not in low:
                mine.append("no %r, which is what %s takes to don" % (don, cat))
            after = low.split("doff", 1)
            if len(after) > 1 and doff.lower() not in after[1][:40]:
                mine.append("doffing is not %r, which is what %s takes"
                            % (doff, cat))
        says_stealth = "stealth" in low
        if says_stealth != (r.int(8) == 1):
            mine.append("the row's stealth column is %d and the note %s it"
                        % (r.int(8), "mentions" if says_stealth else "omits"))
        if mine:
            bad += 1
            problems.append("%s\n      %s" % (r.str(0), "\n      ".join(mine)))
    return checked, bad, problems


def main():
    if not os.path.exists(DUMP):
        sys.exit("verify_equipment: no %s" % DUMP)

    rows = book_rows()
    text = open(DUMP, encoding="utf-8").read().replace("\u2019", "'")
    packs = dict((m.group(1), int(m.group(2)) * 100)
                 for m in PACK.finditer(text))
    items = [r for r in read_file("equipment.txt") if r.tag == "ITEM"]
    checked = missing = bad = 0

    for r in items:
        name = r.str(0)
        if r.str(1) != "PHB":
            continue
        if r.str(2) == "pack":
            checked += 1
            who = name.split("'")[0]
            if who not in packs:
                print("  not priced in the text: %s" % name)
                missing += 1
            elif packs[who] != r.int(3):
                print("  %s\n      cost %d cp, book says %d"
                      % (name, r.int(3), packs[who]))
                bad += 1
            continue
        want = rows.get(key(ALIAS.get(name, name)))
        if want is None:
            print("  not in the tables: %s" % name)
            missing += 1
            continue
        checked += 1
        book_name, cost, rest = want
        category = r.str(2)
        problems = []

        if cost != r.int(3):
            problems.append("cost %d cp, book says %d" % (r.int(3), cost))

        # The weight is whichever following cell reads as one. A few cells
        # are missing from the text layer altogether (the bell, the sealing
        # wax), and those are left unchecked rather than read as zero.
        if category in WEIGHED:
            weight = None
            for cell in rest:
                if "lb" not in cell:
                    continue
                weight = parse_weight(cell)
                break
            if weight is not None and weight != r.int(4):
                problems.append("weight %d, book says %d (tenths of a pound)"
                                % (r.int(4), weight))

        # Armour: base AC, the Dexterity cap, Strength and stealth.
        for cell in rest:
            m = ARMOUR_AC.match(cell)
            if not m or category not in ARMOURS:
                continue
            base = int(m.group(1))
            cap = -1 if ("Dex" in cell and not m.group(2)) \
                else (int(m.group(2)) if m.group(2) else 0)
            if base != r.int(5):
                problems.append("AC %d, book says %d" % (r.int(5), base))
            if cap != r.int(6):
                problems.append("Dex cap %d, book says %d" % (r.int(6), cap))
            break
        for cell in rest:
            m = re.match(r"^Str (\d+)$", cell)
            if m and category in ARMOURS and int(m.group(1)) != r.int(7):
                problems.append("Str %d, book says %s" % (r.int(7), m.group(1)))
        if category in ARMOURS:
            stealth = 1 if any(c == "Disadvantage" for c in rest) else 0
            if stealth != r.int(8):
                problems.append("stealth %d, book says %d"
                                % (r.int(8), stealth))

        # Weapons: the damage die and its type.
        for cell in rest:
            m = DAMAGE.match(cell)
            if not m or category not in WEAPONS:
                continue
            if m.group(1) != r.str(9) or m.group(2) != r.str(10):
                problems.append("damage %s %s, book says %s %s"
                                % (r.str(9), r.str(10), m.group(1),
                                   m.group(2)))
            break

        if problems:
            bad += 1
            print("  %s" % name)
            for p in problems:
                print("      %s" % p)

    notes = dict((r.str(0), r.str(1))
                 for r in read_file("equipment.txt") if r.tag == "ITEMNOTE")
    n_checked, n_bad, n_problems = check_notes(items, notes, donning(text))
    for p in n_problems:
        print("  %s" % p)
    checked += n_checked
    bad += n_bad

    print("\n%d PHB items checked against the tables, %d disagree, "
          "%d not found" % (checked, bad, missing))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
