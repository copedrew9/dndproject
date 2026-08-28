#!/usr/bin/env python3
"""Check the MAGICRULE rows in data/equipment.txt against the DMG.

A MAGICITEM row is prose the player reads; a MAGICRULE row is a number the
sheet computes with, and the two are checked by different files because
they fail differently. Prose that drifts from the book reads oddly. A wrong
MAGICRULE is silent: armour of resistance carrying armor_base=18 made a
leather copy of it read AC 18 instead of 15 and cost its wearer ten feet of
speed, on every sheet, with nothing on screen to say where it came from.
Nothing checked these rows until this file; `make audit` confirms the names
exist and stops there.

Each row is compared key by key against the entry's own text, which is
found the way tools/verify_magic_items.py finds an item's line: from the
row's name outward, taking the heading run that spells it and reading down
to the next entry's heading. Its MANGLED table is reused for the fifteen
headings the extraction chews past matching, and its INDEXED table for the
four pages whose margin index sits between a heading and the body under it
-- there the body belongs to the entry the table names, not to the heading
directly above it.

The armour rows are not read out of the entry at all. The DMG gives them as
"+1 plate" or "half plate", and the numbers the row carries -- the Armor
Class, the Strength the suit wants, whether it spoils Stealth -- are the
PHB's for that armour with the entry's bonus added. So the base armour is
taken from the item's own line, "Armor (plate)", the bonus from the body,
and the four numbers computed from the PHB table that
tools/verify_equipment.py already reads. A row that names armour the table
does not have is reported, not skipped.

What is checked and what is not:

  Checked against the entry's words: the Armor Class and saving-throw
  bonuses, the unarmoured base an item sets, a score an item fixes and
  which ability it fixes, walking, flying, swimming and climbing speeds,
  and every damage type resisted or ignored.

  Checked against the PHB table: the four armour numbers, and a shield's
  +2.

  Not checked, because they say how the program should read the row rather
  than what the book says: variable, weapon, worn, only_unarmored. Each is
  still tested for internal sense -- a row with only_unarmored has to say
  so in its entry, a variable row's bonus must not also be written out --
  and the count is reported.

Two rows have nothing to compare against, and are reported unchecked rather
than passed. Hand of Vecna is a row of ours: the DMG describes it inside EYE
AND HAND OF VECNA and gives the hand no heading of its own. Frost Brand's
heading is not in the extraction at all -- the only FROST BRAND in it is a
running header that landed over the figurine of wondrous power.

Bending one value at a time across all forty-one rows -- every number by
one, every damage type to another, every flag to a value that is neither
yes nor no -- gives 74 faults, of which this catches 71. The three it does
not are the two unchecked rows above, which it names on every run.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file            # noqa: E402
import verify_equipment as EQ               # noqa: E402
import verify_magic_items as MI             # noqa: E402

ABILITIES = ["Strength", "Dexterity", "Constitution",
             "Intelligence", "Wisdom", "Charisma"]

# Rows the DMG gives no entry of its own, with the reason.
NO_ENTRY = {
    "Hand of Vecna": "described inside EYE AND HAND OF VECNA, which gives "
                     "the hand no heading of its own",
    "Frost Brand":   "its heading is not in the extraction; the only FROST "
                     "BRAND there is a running header over the figurine of "
                     "wondrous power",
}

# The running headers the extraction drops into the flow. A body is cut at
# the first one so an entry cannot borrow the next page's words.
RUNNING = re.compile(r"CHAPTER\s+\d|APPENDIX\s+[A-Z]|^\d+\s*$")

# "Armor (plate)", "Armor (shield)", "Armor (light, medium, or heavy)".
ARMOUR_KIND = re.compile(r"rmor\s*\(([^)]*)\)", re.I)


def flat(s):
    """The entry's text with the extraction's spacing thrown away."""
    return re.sub(r"\s+", " ", s.replace("’", "'")).strip()


# "+1", "+ 1": the extraction opens a space after the sign often enough that
# the stone of good luck's whole entry reads "a + 1 bonus".
def plus(n):
    return r"\+\s*%d" % n


# What the DMG calls Armor Class, and what the page edge leaves of it: the
# staff of power's line arrives as "bonus to Armor Clas saving throws" and
# dragon scale mail's as "a +1 bonus to C".
AC_WORDS = r"(?:AC|C|[A-Za-z]?rmor\s+Clas{1,2}s?)"


def entries():
    """Every DMG magic item entry, as name -> (its line, the text under it).

    An entry opens with its line -- "Wondrous item, rare (requires
    attunement)" -- and that is what tells a real entry from a margin index
    that happens to repeat the name, and from a running header sitting over
    prose. tools/verify_magic_items.is_item_line decides it on the line's
    shape, which is the only thing the page edge leaves intact.
    """
    raw = [l.rstrip("\n") for l in open(MI.DUMP, encoding="utf-8")]
    lines = [l.strip() for l in raw]
    nb = [i for i, l in enumerate(lines) if l]

    names = {}
    for r in read_file("equipment.txt"):
        if r.tag == "MAGICITEM":
            names[MI.squash(r.str(0))] = r.str(0)
    for bad, good in MI.MANGLED.items():
        names[MI.squash(bad)] = good

    anchors = []
    for k in range(len(nb)):
        if not MI.headingish(lines[nb[k]]):
            continue
        for span in (1, 2, 3):
            if k + span > len(nb):
                break
            run = [lines[nb[x]] for x in range(k, k + span)]
            if not MI.headingish(run[-1]):
                break
            nm = names.get(MI.squash(" ".join(run)))
            if nm:
                anchors.append((k, k + span, nm))
                break
    starts = [a[0] for a in anchors]

    cand = {}
    for n, (k, end, nm) in enumerate(anchors):
        if end >= len(nb):
            continue
        line = MI.clause(lines, nb, end)
        if not line or not MI.is_item_line(line):
            continue
        stop = starts[n + 1] if n + 1 < len(anchors) else len(nb)
        text = []
        for x in range(end, stop):
            if RUNNING.search(lines[nb[x]]):
                break
            text.append(lines[nb[x]])
        cand.setdefault(nm, []).append((k, line, flat(" ".join(text))))

    # Four pages set a margin index between an entry's heading and its line,
    # which leaves the name at the foot of the index sitting over a line that
    # is not its own. verify_magic_items.INDEXED names the entry each of
    # those bodies really belongs to. Which of the name's bodies is the
    # misfiled one cannot be decided by position -- the cloak of the manta
    # ray's own entry is printed above its index and the shield of missile
    # attraction's below -- so it is decided the way verify_magic_items
    # decides it, by looking for the other entry's name in the run of
    # headings above the body. That is the index itself.
    for wrong, right in MI.INDEXED.items():
        want = MI.squash(right)
        keep = []
        for k, line, text in cand.get(wrong, []):
            blk = MI.heading_block(lines, nb, k)
            indexed = any(MI.squash(" ".join(reversed(blk[a:a + span]))) == want
                          for a in range(len(blk)) for span in (1, 2, 3)
                          if a + span <= len(blk))
            if indexed:
                cand.setdefault(right, []).append((k, line, text))
            else:
                keep.append((k, line, text))
        if wrong in cand:
            cand[wrong] = keep
    return {n: (v[0][1], v[0][2]) for n, v in cand.items() if v}


def armour_numbers(text):
    """The AC, Strength, Dex cap and Stealth the PHB gives an armour.

    Returned in the shape data/ writes them: dex is -1 for the full
    modifier, 0 for none and N for a cap.
    """
    row = EQ.book_rows().get(EQ.key(text or ""))
    if not row:
        return None
    cells = row[2]
    m = EQ.ARMOUR_AC.match(cells[0]) if cells else None
    if not m:
        return None
    base = int(m.group(1))
    dex = 0 if "Dex" not in cells[0] else (int(m.group(2)) if m.group(2)
                                           else -1)
    need = 0
    for c in cells[1:]:
        got = re.match(r"Str\s*(\d+)", c)
        if got:
            need = int(got.group(1))
    stealth = 1 if any("Disadvantage" in c for c in cells[1:]) else 0
    return base, dex, need, stealth


def settings(r):
    """A MAGICRULE's key=value fields as a dict, and the keys in order."""
    out = {}
    for f in r.f[1:]:
        f = f.strip()
        if not f:
            continue
        k, _, v = f.partition("=")
        out[k.strip()] = v.strip()
    return out


def number(kv, name):
    try:
        return int(kv.get(name, 0))
    except ValueError:
        return 0


def damage_types(field):
    return [t.strip() for t in field.split(",") if t.strip()]


def variant(field):
    """Whether the copy carries the damage type rather than the entry.

    The same rule the program uses, in src/data_lookup.c: a "*" means any
    of the ten, and a comma-separated list means one of those named.
    """
    return field == "*" or "," in field


CHOICE = re.compile(r"one (?:type of damage|damage type|of the following "
                    r"damage types)|resistance to one", re.I)


def check(kv, line, body):
    """A row against its entry, as (disagreements, things left unchecked)."""
    bad, open_ = [], []

    def says(pattern, why):
        if not re.search(pattern, body, re.I):
            bad.append(why)

    ac = number(kv, "ac_bonus")
    if ac:
        says(r"%s bonus to %s" % (plus(ac), AC_WORDS),
             "ac_bonus=%d: the entry gives no +%d bonus to AC" % (ac, ac))
    sv = number(kv, "save_bonus")
    if sv:
        says(r"%s bonus to [^.]{0,70}saving throws" % plus(sv),
             "save_bonus=%d: the entry gives no +%d bonus to saving throws"
             % (sv, sv))
    # Both of these are checked in both directions. A bonus a row claims and
    # the entry does not give is the obvious fault; a bonus the entry gives
    # and the row drops is the silent one, and it is the one that was there
    # -- the stone of good luck raised saving throws and quietly did nothing
    # for the ability checks the same sentence grants.
    cb = number(kv, "check_bonus")
    grants = re.search(r"\+\s*(\d) bonus to [^.]{0,40}ability checks",
                       body, re.I)
    if cb and not grants:
        bad.append("check_bonus=%d: the entry gives no +%d bonus to ability "
                   "checks" % (cb, cb))
    elif cb and int(grants.group(1)) != cb:
        bad.append("check_bonus=%d, and the entry gives +%s"
                   % (cb, grants.group(1)))
    elif grants and not cb:
        bad.append("the entry gives a +%s bonus to ability checks and the "
                   "row carries no check_bonus" % grants.group(1))

    curses = re.search(r"vulnerability to two of the three damage type",
                       body, re.I)
    if number(kv, "vulnerable_others"):
        if not curses:
            bad.append("vulnerable_others=1: the entry does not curse the "
                       "wearer with vulnerability to the two types it was "
                       "not made against")
        if not variant(kv.get("resist") or ""):
            bad.append("vulnerable_others=1 needs a resist list to take the "
                       "other types from, and the row has none")
    elif curses:
        bad.append("the entry curses the wearer with vulnerability to two "
                   "of its three types, and the row does not say so")

    ub = number(kv, "unarmored_base")
    if ub:
        says(r"base Armor Class is %d" % ub,
             "unarmored_base=%d: the entry sets no base Armor Class of %d"
             % (ub, ub))
    if number(kv, "only_unarmored") and not re.search(
            r"no armor|aren't wearing armor", body, re.I):
        bad.append("only_unarmored=1: the entry does not say it needs you to "
                   "be wearing no armour")

    ab = number(kv, "sets_ability")
    if ab:
        if not 1 <= ab <= 6:
            bad.append("sets_ability=%d is not an ability" % ab)
        else:
            which, to = ABILITIES[ab - 1], number(kv, "sets_to")
            if to:
                says(r"%s (?:score )?(?:is|changes to|becomes) %d"
                     % (which, to),
                     "sets_ability=%d sets_to=%d: the entry does not set %s "
                     "to %d" % (ab, to, which, to))
            else:
                says(r"%s score changes to a score granted" % which,
                     "sets_to=0 says the copy carries the score, but the "
                     "entry does not say the item grants one")

    sp = number(kv, "sets_speed")
    if sp:
        says(r"walking speed becomes %d feet" % sp,
             "sets_speed=%d: the entry sets no walking speed of %d feet"
             % (sp, sp))
    for name, word in (("fly_speed", "flying"), ("swim_speed", "swimming"),
                       ("climb_speed", "climbing")):
        v = number(kv, name)
        if not v:
            continue
        if v < 0:
            says(r"%s speed equal to your walking speed" % word,
                 "%s=-1: the entry gives no %s speed equal to your walking "
                 "speed" % (name, word))
        else:
            says(r"%s speed of %d feet" % (word, v),
                 "%s=%d: the entry gives no %s speed of %d feet"
                 % (name, v, word, v))

    for name, verb in (("resist", r"resistance to"),
                       ("immune", r"immun\w+ to")):
        field = kv.get(name)
        if not field:
            continue
        if variant(field):
            if not CHOICE.search(body):
                bad.append("%s=%r says the copy carries the type, but the "
                           "entry offers no choice of type" % (name, field))
                continue
            if field == "*":
                continue
            # The types are in a table, and the table is set tight enough
            # that the extraction puts a space inside a word: dragon scale
            # mail's reads "3lue Lightn ing" and "Si lver Cold". So they are
            # looked for with the spacing thrown away, as
            # verify_magic_items compares an attunement clause.
            tight = re.sub(r"\s+", "", body).lower()
            missing = [t for t in damage_types(field)
                       if re.sub(r"\s+", "", t).lower() not in tight]
            if missing:
                bad.append("%s names %s, which the entry does not"
                           % (name, ", ".join(missing)))
            continue
        for t in damage_types(field):
            says(r"%s [^.]{0,50}%s" % (verb, re.escape(t.split()[0])),
                 "%s lists %r, which the entry does not give" % (name, t))

    for name in ("only_unarmored", "worn", "variable", "weapon",
                 "vulnerable_others"):
        if name in kv and kv[name] not in ("0", "1"):
            bad.append("%s=%s: the flag is a yes or a no, and nothing else "
                       "reads as either" % (name, kv[name]))

    shield = number(kv, "shield")
    if shield:
        row = EQ.book_rows().get(EQ.key("Shield"))
        cell = row[2][0] if row else None
        if cell != "+%d" % shield:
            bad.append("shield=%d: the PHB gives a shield %s"
                       % (shield, cell))

    base = number(kv, "armor_base")
    if base:
        parts = MI.split_clause(line)
        kind = parts[1] if parts else None
        want = armour_numbers(kind)
        got = re.search(r"%s bonus to %s" % (plus(1) + r"|" + plus(2)
                                             + r"|" + plus(3), AC_WORDS),
                        body)
        step = int(re.search(r"\d", got.group(0)).group(0)) if got else 0
        if want is None:
            bad.append("armor_base=%d: the line names %r, which is not one "
                       "armour in the PHB table" % (base, kind))
        else:
            have = (base, number(kv, "armor_dex"), number(kv, "armor_str"),
                    number(kv, "armor_stealth"))
            exp = (want[0] + step, want[1], want[2], want[3])
            if have != exp:
                bad.append("armour numbers %r, but %s%s is %r"
                           % (have, ("+%d " % step) if step else "",
                              kind, exp))
    return bad, open_


def main():
    rules = [r for r in read_file("equipment.txt") if r.tag == "MAGICRULE"]
    found = entries()

    checked = wrong = flags = 0
    unchecked = []
    for r in rules:
        name = r.str(0)
        if name in NO_ENTRY:
            unchecked.append((name, NO_ENTRY[name]))
            continue
        if name not in found:
            unchecked.append((name, "no entry of its own in the extraction"))
            continue
        kv = settings(r)
        line, body = found[name]
        checked += 1
        bad, open_ = check(kv, line, body)
        for why in open_:
            unchecked.append((name, why))
        if number(kv, "variable") or number(kv, "weapon") or \
                number(kv, "worn"):
            flags += 1
        if bad:
            wrong += 1
            print("  %s" % name)
            for b in bad:
                print("      %s" % b)

    if unchecked:
        print("\n  %d left unchecked:" % len(unchecked))
        for n, why in unchecked:
            print("      %-32s %s" % (n, why))
    print("\n%d magic item rules checked against the DMG, %d disagree "
          "(%d carry a flag the book cannot settle)" % (checked, wrong, flags))
    return 1 if wrong else 0


if __name__ == "__main__":
    sys.exit(main())
