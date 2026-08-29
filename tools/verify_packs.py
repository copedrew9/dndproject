#!/usr/bin/env python3
"""Check what is in each equipment pack against the Player's Handbook.

The PHB prices the seven packs in prose rather than in a table -- "Burglar's
Pack (16 gp). Includes a backpack, a bag of 1,000 ball bearings, ..." -- and
three of them add a trailing sentence that is part of the contents and easy
to miss: "The pack also has 50 feet of hempen rope strapped to the side of
it."

data/equipment.txt holds each pack's contents twice: once as that prose, in
the pack row's own contents field, and once as PACKITEM rows naming real
ITEMs with quantities, which is what the program puts on a character who
takes one. This checks both against the book, and the two against each
other, so they cannot drift apart.

It also checks the arithmetic that makes unpacking safe: a pack's
weight_tenths must be exactly the sum of its parts. If it were not, taking
a pack would change what a character weighs -- the contents REPLACE the
pack, because carrying both would count everything twice.

The book's phrasing and ours differ on purpose ("5 days rations" for "5 days
of rations", "Rations (1 day)" x5 for both), so parts are matched through
ALIAS below rather than by string equality. A phrase that matches nothing is
reported rather than skipped.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file          # noqa: E402

DUMP = os.path.join(ROOT, "TextFiles", "PHBtext.txt")

# The book's phrase for one part -> the ITEM row it is. Everything not here
# is expected to match an ITEM name outright once the count is stripped.
ALIAS = {
    "backpack": "Backpack",
    "bag of 1,000 ball bearings": "Ball bearings (bag of 1,000)",
    "bag of 1000 ball bearings": "Ball bearings (bag of 1,000)",
    "10 feet of string": "String (10 feet)",
    "bell": "Bell",
    "candles": "Candle",
    "candle": "Candle",
    "crowbar": "Crowbar",
    "hammer": "Hammer",
    "pitons": "Piton",
    "hooded lantern": "Lantern, hooded",
    "flasks of oil": "Oil (flask)",
    "days rations": "Rations (1 day)",
    "days of rations": "Rations (1 day)",
    "tinderbox": "Tinderbox",
    "waterskin": "Waterskin",
    "50 feet of hempen rope": "Rope, hempen (50 feet)",
    "chest": "Chest",
    "cases for maps and scrolls": "Case, map or scroll",
    "set of fine clothes": "Clothes, fine",
    "fine clothes": "Clothes, fine",        # our contents line's wording
    "bottle of ink": "Ink (1 ounce bottle)",
    "ink pen": "Ink pen",
    "lamp": "Lamp",
    "sheets of paper": "Paper (one sheet)",
    "vial of perfume": "Perfume (vial)",
    "sealing wax": "Sealing wax",
    "soap": "Soap",
    "torches": "Torch",
    "bedroll": "Bedroll",
    "costumes": "Clothes, costume",
    "disguise kit": "Disguise kit",
    "mess kit": "Mess kit",
    "blanket": "Blanket",
    "alms box": "Alms box",
    "blocks of incense": "Block of incense",
    "censer": "Censer",
    "vestments": "Vestments",
    "book of lore": "Book of lore",
    "sheets of parchment": "Parchment (one sheet)",
    "little bag of sand": "Little bag of sand",
    "small knife": "Small knife",
}

# "The pack also has 50 feet of hempen rope strapped to the side of it."
ALSO = re.compile(r"pack also has (.+?) strapped to the side", re.I)
HEAD = re.compile(r"([A-Z][a-z]+'s Pack) \((\d+) gp\)\.\s*Includes\s+")

NUMBER = re.compile(r"^(\d[\d,]*)\s+(.*)$")
ARTICLE = re.compile(r"^(?:a|an|the)\s+", re.I)

# Parts whose own phrase contains " and ", which the comma-and-and split
# would otherwise cut in half.
GLUED = ["cases for maps and scrolls", "bag of 1,000 ball bearings",
         "bag of 1000 ball bearings"]


def tidy(s):
    return re.sub(r"\s+", " ", s.replace("\u2019", "'")).strip()


def one_part(phrase):
    """"5 days of rations" -> ("Rations (1 day)", 5), or (None, phrase)."""
    p = tidy(phrase).rstrip(".").strip()
    p = re.sub(r"^and\s+", "", p, flags=re.I)
    p = ARTICLE.sub("", p)
    qty = 1
    if p.lower() in ALIAS:
        return ALIAS[p.lower()], 1
    m = NUMBER.match(p)
    if m:
        qty = int(m.group(1).replace(",", ""))
        p = m.group(2)
        p = ARTICLE.sub("", p)
    if p.lower() in ALIAS:
        return ALIAS[p.lower()], qty
    return None, p


def split_parts(body):
    """The book's list, split without cutting a part that says "and"."""
    holes = {}
    for i, g in enumerate(GLUED):
        token = "\x01%d\x01" % i
        if g in body:
            body = body.replace(g, token)
            holes[token] = g
    pieces = re.split(r",\s*|\s+and\s+", body)
    out = []
    for piece in pieces:
        for token, real in holes.items():
            piece = piece.replace(token, real)
        out.append(piece)
    return out


def book_packs():
    """{pack name: {item: quantity}} out of the book's own sentences."""
    text = open(DUMP, encoding="utf-8", errors="replace").read()
    text = tidy(text)
    out, unknown = {}, []

    starts = [(m.start(), m.end(), m.group(1)) for m in HEAD.finditer(text)]
    for k, (_s0, e0, name) in enumerate(starts):
        stop = starts[k + 1][0] if k + 1 < len(starts) else e0 + 600
        chunk = text[e0:stop]
        # The contents run to the first full stop; the "also has" sentence
        # that may follow is part of them, and lives inside this chunk only.
        body = chunk.split(".")[0]
        parts = {}
        pieces = split_parts(body)
        extra = ALSO.search(chunk)
        if extra:
            pieces = pieces + [extra.group(1)]
        for piece in pieces:
            piece = tidy(piece)
            if not piece:
                continue
            item, qty = one_part(piece)
            if item is None:
                unknown.append((name, qty))
                continue
            parts[item] = parts.get(item, 0) + qty
        out[name] = parts
    return out, unknown


def main():
    if not os.path.exists(DUMP):
        sys.exit("verify_packs: no %s" % DUMP)

    rows = [r for r in read_file("equipment.txt")]
    weight = {}
    packs = []
    for r in rows:
        if r.tag != "ITEM":
            continue
        weight[r.str(0)] = r.int(4)
        if r.str(2) == "pack":
            packs.append(r.str(0))

    ours = {}
    for r in rows:
        if r.tag != "PACKITEM":
            continue
        ours.setdefault(r.str(0), {})
        ours[r.str(0)][r.str(1)] = ours[r.str(0)].get(r.str(1), 0) + r.int(2)

    book, unknown = book_packs()
    for name, phrase in unknown:
        print("  %s: could not read the part %r" % (name, phrase))

    bad = checked = 0
    for pack in sorted(packs):
        # "Burglar's pack" in data/, "Burglar's Pack" in the book.
        b = book.get(pack[:1].upper() + pack[1:].replace(" pack", " Pack"))
        mine = ours.get(pack)
        if mine is None:
            print("  %s has no PACKITEM rows" % pack)
            bad += 1
            continue
        if b is None:
            print("  %s is not a pack the book prints" % pack)
            bad += 1
            continue
        checked += 1

        for item, qty in sorted(b.items()):
            if mine.get(item, 0) != qty:
                print("  %s: the book gives %d x %s, we give %d"
                      % (pack, qty, item, mine.get(item, 0)))
                bad += 1
        for item, qty in sorted(mine.items()):
            if item not in b:
                print("  %s: we give %d x %s and the book does not"
                      % (pack, qty, item))
                bad += 1

        # The arithmetic that makes replacing the pack with its parts safe.
        total = 0
        for item, qty in mine.items():
            if item not in weight:
                print("  %s names %r, which is no ITEM" % (pack, item))
                bad += 1
                continue
            total += weight[item] * qty
        if total != weight[pack]:
            print("  %s weighs %.1f lb and its parts weigh %.1f -- taking "
                  "one would change what a character carries"
                  % (pack, weight[pack] / 10.0, total / 10.0))
            bad += 1

        # And the prose field has to say the same thing, since that is what
        # the reference screen and the saved sheet show. Read with the same
        # machinery as the book's own sentence, so this is an equality check
        # and not a guess at substrings.
        prose = ""
        for r in rows:
            if r.tag != "ITEM" or r.str(0) != pack:
                continue
            prose = r.str(12)      # the contents field; the tag is not
                                   # counted, so this is the 14th column
        said = {}
        for piece in split_parts(prose):
            piece = tidy(piece)
            if not piece:
                continue
            item, qty = one_part(piece)
            if item is None:
                print("  %s: the contents line says %r, which is no part"
                      % (pack, qty))
                bad += 1
                continue
            said[item] = said.get(item, 0) + qty
        if said != mine:
            for item in sorted(set(said) | set(mine)):
                if said.get(item, 0) != mine.get(item, 0):
                    print("  %s: the contents line says %d x %s, the rows "
                          "say %d" % (pack, said.get(item, 0), item,
                                      mine.get(item, 0)))
                    bad += 1

    print("\n%d packs checked against the Player's Handbook, %d disagree"
          % (checked, bad))
    return 1 if (bad or unknown) else 0


if __name__ == "__main__":
    sys.exit(main())
