#!/usr/bin/env python3
"""Check the MAGICITEM headers in data/equipment.txt against the DMG.

tools/verify_coverage.py enumerates the line every magic item in the DMG
opens with -- "Wondrous item, rare (requires attunement)", "Weapon (any
sword), uncommon", "Armor (plate), legendary (requires attunement by a
paladin)" -- and uses it only to find items nobody entered. That line
carries four facts, and this compares them against the row rather than
counting it: the kind of item, the parenthetical that narrows it, the
rarity, and the attunement clause with whatever restriction it puts on who
may attune.

It works from the rows outward rather than from the book inward, which is
the opposite of verify_coverage.py and is the whole reason it can be
precise. Several pages set a margin index -- a list of the item names on
the page -- and the extraction drops it into the flow directly above an
entry, so a line read backwards from can have three headings over it and
only one of them owns it. Starting from the row's own name and taking the
line directly under it gets the right one, and a name that ends up over two
different lines is a conflict this reports rather than guesses at.

Four pages defeat even that, because the margin index sits between an entry
heading and its line: the index over the crystal ball's line reads CHIME OF
OPENING / CRYSTAL BALL / CLOAK OF THE MANTA RAY, so the cloak appears to own
a line that says "very rare or legendary" when the cloak is uncommon. Those
four are named in INDEXED below, each read off the body under the line ("The
typical crystal ball, a very rare item, is about 6 inches in diameter"), and
the line is handed to the entry that the body belongs to.

Three things the typesetting does that the comparison has to absorb:

  Headings lose their opening letter to the decorative initial, and the
  extraction reads a capital D as ":J" and a capital R as "R o n". Fifteen
  headings arrive that way and are named in MANGLED, which is checked
  against the data files so a stale entry cannot hide an item.

  The left edge of pages 165-211 is eaten, so the line itself arrives as
  ".J.rmor (plate), very rare", "Veapon (trident), uncommon" or "IVand,
  rare". The rarity and the attunement survive that; the kind does not. So
  the kind is compared only where the book's word for it is one of the ten
  the DMG uses, and where it is not, the kind is left unchecked for that
  item and the rest of the line is still compared.

  Words are set with a space inside them ("requires attunem ent") and a
  letter is sometimes left stranded ("by a cleric, druid, r paladin"), so
  the attunement clause is compared on its words with the spacing thrown
  away and single letters dropped.

Deliberately not reported as disagreements:

  A rarity the book gives as a range or a list -- "rare (silver or brass),
  very rare (bronze), or legendary (iron)", "very rare or legendary",
  "uncommon (+1), rare (+2), or very rare (+3)" -- against a row that writes
  it "rare to legendary" or "uncommon (+1), rare (+2), very rare (+3)". A
  row written as a range is compared on the ends of the book's list.

  A rarity the book refuses to give at all -- "rarity varies", "rarity by
  figurine", "varies" -- where the real range is in a table further down the
  entry. Those rows are accepted as long as they say a range or "varies",
  and counted apart at the end, because the ends of the range are not on the
  line and nothing here has read the table.

Three rows end up with nothing to compare against, and are reported
unchecked rather than counted or guessed at. Eye of Vecna and Hand of Vecna
are rows of ours: the DMG describes both inside one entry headed EYE AND
HAND OF VECNA and gives no line to either half. Frost Brand's own heading
is not in the dump at all -- the only FROST BRAND in it is a running header
that landed in the middle of the figurine of wondrous power, over a line
reading "an Intelligence of 8 and can speak Common".
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

DUMP = os.path.join(ROOT, "TextFiles", "DMGtext.txt")

# The ten words the DMG opens an item's line with. A line whose first word
# is not one of these has lost it to the page edge, and the kind is left
# unchecked rather than guessed at.
KINDS = {"wondrousitem", "ammunition", "armor", "weapon", "potion",
         "scroll", "staff", "wand", "ring", "rod"}

RARITY = re.compile(r"very\s+rare|uncommon|rare|common|legendary|artifact|"
                    r"varies", re.I)

# The book refuses to give a rarity on the line and puts it in a table
# inside the entry.
VARIES = re.compile(r"rarity\s+varies|rarity\s+by\b|^\s*varies\s*$", re.I)

# "requires attunement", and every way the page edge chews it: the shield's
# is set "atttinement", Blackrazor's "attunemenc", the sword of vengeance's
# "attunemer". Only the word is matched here; what restricts the attunement
# is whatever follows it.
ATTUNED = re.compile(r"requires\s+att\w*", re.I)

# Marks the extraction leaves in the middle of a line, which fall between a
# rarity's two words often enough to matter ("very· rare").
NOISE = re.compile(r"[·•]")

# Headings the extraction mangles past matching on letters, and the entry
# each one is. Every name here has to exist in data/, so the table cannot
# outlive what it points at.
# Two rows where ours deliberately differs from the book's line, and why.
# Reported under a heading of their own rather than as disagreements, the way
# audit.py names the six entries that are ours rather than the book's.
KEPT = {
    "Sword of Kas":
        "the DMG's line reads \"Wondrous item, artifact\", but the entry "
        "under it describes a longsword and gives a +3 to attack and damage "
        "rolls with it. Filed as the weapon it is, so it can be wielded, "
        "reach the attacks block and carry a weapon's bonus.",
    "Gem of Brightness":
        "the line in the dump carries no attunement clause. A clause at a "
        "line end is exactly what the extraction loses, so ours is kept "
        "rather than dropped on the strength of one damaged line.",
}

MANGLED = {
    ":JEMON ARMOR":            "Demon Armor",
    "~IM ENSIONAL SHACKLES":   "Dimensional Shackles",
    ":JRAGON SCALE MAIL":      "Dragon Scale Mail",
    "IMMOVABLE Ron":           "Immovable Rod",
    "Ron OF ALERTNESS":        "Rod of Alertness",
    "Ron oF LoRDLY MIGHT":     "Rod of Lordly Might",
    "On OF RESURRECTION":      "Rod of Resurrection",
    "On OF RULERSHIP":         "Rod of Rulership",
    "R o n OF SECURITY":       "Rod of Security",
    "R on OF THE PACT KEEPER": "Rod of the Pact Keeper",
    "TENTACLE Ron":            "Tentacle Rod",
    "TALISMAN OF PuRE Goon":   "Talisman of Pure Good",
    "AND OF LIGHTNING BOLTS":  "Wand of Lightning Bolts",
    "AND OF MAGIC DETECTION":  "Wand of Magic Detection",
    "AND OF MAGIC MISSILES":   "Wand of Magic Missiles",
}

# Pages whose margin index lands between an entry's heading and its line.
# The name on the left is the one the index leaves sitting directly over a
# line that is not its own; the name on the right is the entry the body
# under that line belongs to. The line is only moved when the entry's own
# heading is somewhere in the block of headings above it, so a wrong guess
# here fails loudly rather than quietly.
INDEXED = {
    "Armor of Invulnerability":     "Armor, +1, +2, or +3",
    "Cloak of the Manta Ray":       "Crystal Ball",
    "Ring of Water Walking":        "Ring of Spell Turning",
    "Shield of Missile Attraction": "Shield, +1, +2, or +3",
}

# Rows the DMG gives no line of its own, with the reason.
NO_LINE = {
    "Eye of Vecna":  "described inside EYE AND HAND OF VECNA, no line of its own",
    "Hand of Vecna": "described inside EYE AND HAND OF VECNA, no line of its own",
}


def squash(text):
    """Letters only, lowercased, with the dump's confusions folded.

    The folding tools/audit.py and tools/verify_coverage.py use, for the
    same reason: the extraction confuses c with g and i with l, sets some
    capitals as digits, and doubles or drops letters at line breaks.
    """
    t = re.sub(r"[^A-Za-z0-9]", "", text).lower()
    t = t.replace("c", "g").replace("i", "l").replace("1", "l")
    t = t.replace("0", "o")
    return re.sub(r"(.)\1+", r"\1", re.sub(r"[^a-z]", "", t))


def headingish(line):
    """Whether a line could be a heading rather than prose.

    Headings are set in small capitals, so they come back mostly upper
    case, and they are short and do not end a sentence. Everything the
    running page headers and the margin indexes put in the flow passes
    this too, which is the point: the block above a line has to be walked
    to its top before the entry's own heading can be found in it.
    """
    if not line or len(line) > 48 or line.endswith(".") or \
            line.startswith("==="):
        return False
    letters = [c for c in line if c.isalpha()]
    if len(letters) < 2:
        return False
    return sum(1 for c in letters if c.isupper()) / len(letters) >= 0.55


def clause(lines, nb, k):
    """The item line beginning at non-blank slot k, joined up, or None.

    The line wraps, and where it wraps is not where a clause ends: "Weapon
    (any sword that deals slashing damage)," breaks before its rarity, and
    "Staff, very rare (requires attunement by a druid," breaks inside the
    attunement. So the next line is taken while a bracket is still open,
    while the text ends on a comma or on a word a rarity continues, and
    where the next line opens the attunement.
    """
    if k >= len(nb) or headingish(lines[nb[k]]):
        return None
    text = lines[nb[k]]
    for _ in range(2):
        nxt = lines[nb[k + 1]] if k + 1 < len(nb) else ""
        if not nxt or headingish(nxt):
            break
        if not (text.count("(") > text.count(")")
                or re.search(r"(,|\bor|\bvery)$", text.rstrip(" .·"))
                or nxt.startswith("(")
                or re.match(r"requires\s+att", nxt, re.I)):
            break
        text += " " + nxt
        k += 1
    text = re.sub(r"\s+", " ", NOISE.sub("", text)).strip()
    return text if is_item_line(text) else None


def is_item_line(text):
    """Whether a line has the shape the DMG opens an item with.

    Shape rather than wording, because the wording is half eaten on some
    pages. What has to hold is a kind of at most three words, a comma
    outside any bracket, and a rarity after that comma. Prose fails it:
    "an Intelligence of 8 and can speak Common. It also" sits under a
    running header reading FROST BRAND and carries the word "Common", and
    nothing but the shape tells the two apart.
    """
    parts = split_clause(text)
    if parts is None:
        return False
    kind, _, rarity, _ = parts
    return (len(kind) <= 20 and len(kind.split()) <= 3
            and bool(rarities(rarity)))


def top_comma(s):
    """Where the first comma outside brackets is, or -1."""
    depth = 0
    for i, ch in enumerate(s):
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth = max(0, depth - 1)
        elif ch == "," and depth == 0:
            return i
    return -1


def split_clause(text):
    """An item line as (kind, parenthetical, rarity, attunement)."""
    i = top_comma(text)
    if i < 0:
        return None
    head, rest = text[:i], text[i + 1:]
    m = re.search(r"\(([^()]*)\)\s*$", head)
    paren = m.group(1) if m else ""
    kind = head[:m.start()] if m else head
    a = ATTUNED.search(rest)
    rarity = rest[:a.start()] if a else rest
    return (kind.strip(), paren.strip(),
            rarity.strip().rstrip("(").strip().strip(",").strip(),
            rest[a.start():] if a else "")


def attunement(text, bounded):
    """(is it required, what restricts it) for an attunement clause.

    The restriction is what the book puts between the word and the closing
    bracket. The sword of vengeance's bracket never closes -- its line runs
    "uncommon (requires attunemer." and then straight into the entry -- so
    on the book's side an unclosed restriction is returned as unreadable
    rather than as a paragraph of prose.
    """
    m = ATTUNED.search(text or "")
    if not m:
        return False, ""
    rest = text[m.end():]
    # "requires attunem ent": the word itself is set with a space in it, so
    # the fragment left over is not the start of a restriction. It is only
    # swallowed when the word and the fragment spell the word exactly,
    # which leaves "attunemenc by a creature of non-lawful alignment" alone.
    frag = re.match(r"\s*[A-Za-z]{1,5}\b", rest)
    if frag and words(m.group(0) + frag.group(0), set()) \
            == "requiresattunement":
        rest = rest[frag.end():]
    if not bounded:
        return True, rest
    return True, (rest[:rest.index(")")] if ")" in rest else None)


def rarities(text):
    """The rarities named on a line, in order. ['varies'] for a refusal."""
    if VARIES.search(text.strip()):
        return ["varies"]
    return [re.sub(r"\s+", " ", m.group(0).lower())
            for m in RARITY.finditer(text)]


def words(text, drop):
    """A clause as one string of letters, with the spacing thrown away.

    "requires attunem ent by a cleric, druid, r paladin" and "requires
    attunement by a cleric, druid or paladin" have to come out the same:
    the space inside a word and the stranded letter are the extraction's,
    and the comma before "or" is the book's house style, not ours.
    """
    out = []
    for w in re.split(r"[^A-Za-z]+", text.lower()):
        if len(w) > 1 and w not in drop:
            out.append(w)
    return "".join(out)


def att_key(text):
    """What an attunement clause restricts attuning to, as one string.

    The function words go, so that the book's "by a creature with the same
    alignment as the sword" and our "by a creature of the same alignment as
    the sword" are the same restriction and not a finding. What is left is
    the classes, the races and the alignments, which is what a restriction
    is made of and where a real difference would show.
    """
    return words(text, {"an", "the", "or", "of", "by", "with", "that"})


def paren_key(text):
    return words(text, {"or", "but"})


def blocks(lines, nb, names):
    """Every item line in the book, filed under the heading over it.

    A heading may take up to three lines -- "AMULET OF PROOF AGAINST
    DETECTION / A ND LOCATION" -- so runs of that length are tried, and a
    run that spells one of our names claims the line under it.
    """
    out = {}
    for k in range(len(nb)):
        if not headingish(lines[nb[k]]):
            continue
        for span in (1, 2, 3):
            if k + span > len(nb):
                break
            run = [lines[nb[x]] for x in range(k, k + span)]
            if not headingish(run[-1]):
                break
            key = squash(" ".join(run))
            if key not in names:
                continue
            text = clause(lines, nb, k + span)
            if text:
                out.setdefault(names[key], []).append((k + span, text))
    return out


def heading_block(lines, nb, k):
    """The run of headings sitting above the line at non-blank slot k."""
    out, j = [], k - 1
    while j >= 0 and headingish(lines[nb[j]]) and len(out) < 8:
        out.append(lines[nb[j]])
        j -= 1
    return out


def unindex(lines, nb, found, names_of):
    """Hand a margin index's line to the entry whose body follows it.

    Only ever moves a line whose block of headings actually holds the other
    entry's name, so the table cannot quietly move the wrong line.
    """
    moved = []
    for wrong, right in INDEXED.items():
        if wrong not in found or right in found:
            continue
        want = squash(right)
        keep = []
        for k, text in found[wrong]:
            blk = heading_block(lines, nb, k)
            hit = any(squash(" ".join(reversed(blk[a:a + span]))) == want
                      for a in range(len(blk)) for span in (1, 2, 3)
                      if a + span <= len(blk))
            if hit:
                found.setdefault(right, []).append((k, text))
                moved.append((wrong, right))
            else:
                keep.append((k, text))
        found[wrong] = keep
    return moved


def main():
    if not os.path.exists(DUMP):
        sys.exit("verify_magic_items: no %s" % DUMP)

    lines = [l.strip() for l in
             open(DUMP, encoding="utf-8", errors="replace").read().split("\n")]
    nb = [i for i, l in enumerate(lines) if l]

    rows = [r for r in read_file("equipment.txt") if r.tag == "MAGICITEM"]
    names = {}
    for r in rows:
        names[squash(r.str(0))] = r.str(0)
    for heading, name in MANGLED.items():
        names[squash(heading)] = name

    stale = sorted(set(list(MANGLED.values()) + list(INDEXED)
                       + list(INDEXED.values()) + list(NO_LINE))
                   - set(names.values()))

    found = blocks(lines, nb, names)
    unindex(lines, nb, found, names)

    checked = bad = 0
    unchecked, ranged, nokind, noatt, kept = [], [], [], [], []

    for r in rows:
        name, book = r.str(0), r.str(1)
        if book != "DMG":
            unchecked.append((name, "not a DMG item"))
            continue
        got = found.get(name, [])
        texts = sorted({t for _, t in got})
        if not got:
            unchecked.append((name, NO_LINE.get(
                name, "no line under its heading in the dump")))
            continue
        if len(texts) > 1:
            unchecked.append((name, "two different lines under its name: %s"
                              % " / ".join(repr(t) for t in texts)))
            continue
        parts = split_clause(texts[0])
        if parts is None:
            unchecked.append((name, "line does not split: %r" % texts[0]))
            continue
        b_kind, b_paren, b_rarity, b_att = parts

        checked += 1
        problems = []

        # Kind. Where the page edge has eaten the word, say so instead of
        # reporting the row against a word that is not there.
        d_head, _, d_paren = r.str(2).partition(" (")
        d_paren = d_paren.rstrip(")")
        theirs = b_kind + (" (%s)" % b_paren if b_paren else "")
        if words(b_kind, set()) not in KINDS:
            nokind.append(name)
        elif (words(b_kind, set()) != words(d_head, set())
                or paren_key(d_paren) != paren_key(b_paren)):
            problems.append("kind %r, book says %r" % (r.str(2), theirs))

        # Rarity.
        d_rarity = r.str(3)
        book_r, ours_r = rarities(b_rarity), rarities(d_rarity)
        if book_r == ["varies"]:
            if ours_r != ["varies"] and " to " not in d_rarity:
                problems.append("rarity %r, book gives none on the line (%r)"
                                % (d_rarity, b_rarity))
            else:
                ranged.append(name)
        elif " to " in d_rarity:
            if book_r and (book_r[0], book_r[-1]) != (ours_r[0], ours_r[-1]):
                problems.append("rarity %r, book says %r"
                                % (d_rarity, b_rarity))
        elif ours_r != book_r:
            problems.append("rarity %r, book says %r" % (d_rarity, b_rarity))

        # Attunement, and whatever it restricts attuning to.
        b_needs, b_who = attunement(b_att, True)
        d_needs, d_who = attunement(r.str(4), False)
        if b_needs != d_needs:
            problems.append("attunement %r, book says %r"
                            % (r.str(4) or "none",
                               "requires attunement" if b_needs else "none"))
        elif b_who is None:
            noatt.append(name)
        elif att_key(d_who) != att_key(b_who):
            problems.append("attunement %r, book says %r"
                            % (r.str(4), ("requires attunement "
                                          + b_who.strip()).strip()))

        if problems and name in KEPT:
            kept.append((name, problems[0], KEPT[name]))
        elif problems:
            bad += 1
            print("  %s" % name)
            for p in problems:
                print("      %s" % p)

    if kept:
        print("\n  %d rows differ from the book on purpose:" % len(kept))
        for name, what, why in kept:
            print("      %s -- %s" % (name, what))
            print("          %s" % why)
    if unchecked:
        print("\n  %d rows left unchecked:" % len(unchecked))
        for name, why in unchecked:
            print("      %-32s %s" % (name, why))
    for name in stale:
        print("\n  a table names %r, which is not a magic item row" % name)

    print("\n  %d of the lines refuse the rarity and put it in a table "
          "inside the entry, so only the row's shape was compared"
          % len(ranged))
    print("  %d of the lines have lost their opening word to the page edge, "
          "so the kind was not compared" % len(nokind))
    if noatt:
        print("  %d of the lines never close the attunement's bracket, so "
              "only that it is required was compared: %s"
              % (len(noatt), ", ".join(noatt)))
    print("\n%d checked, %d disagree" % (checked, bad))
    return 1 if (bad or stale) else 0


if __name__ == "__main__":
    sys.exit(main())
