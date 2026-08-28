#!/usr/bin/env python3
"""Check the sidekick rows in data/world.txt against Tasha's.

Fifty rows -- three classes, three spellcasting roles and forty-four
features -- and until now nothing read them. `make audit` confirmed the
names appear somewhere in the dump, which a feature listed at the wrong
level passes without complaint. One was: the Spellcaster's last ability
score improvement sat at 19th level, and Tasha's puts it at 18th.

Tasha's prints each class twice over, as a level table and as a run of
feature descriptions, and this reads the descriptions. The tables are the
obvious source and the wrong one: the extraction takes all three in
different shapes -- the Expert's puts its twenty level labels first and
then the cells, the Spellcaster's keeps them in rows, the Warrior's splits
into two blocks -- and each would need its own reader and its own way of
being subtly wrong. Every feature description, by contrast, carries its own
level in a line of fixed shape directly under the heading: "COORDINATED
STRIKE" / "6th-level Expert feature". That is one shape for all three
classes and it is what the level is read from.

Two things the descriptions state in prose rather than in a heading:

  The ability score improvements. One description covers all of them and
  lists the rest of the levels in a sentence -- "At 4th level and again at
  8th, 10th, 12th, 16th, and 19th level" -- so that sentence is parsed, and
  every level in it has to have a row. The extraction sets a space inside
  some of those numbers ("1 2th"), which is closed up first.

  The upgrades. Where a feature improves later, the book says so at the
  foot of the feature it improves rather than giving the improvement a
  heading -- "The sidekick can use this feature twice between rests
  starting at 20th level". The project gives those rows names of its own,
  "Second Wind improves" and the like, which tools/audit.py already
  excuses; here the name before "improves" has to be a real feature of that
  class, and its description has to name the level the row claims.

The Spellcaster's three roles are checked against the Spellcasting table,
which is where the book settles them. Its own prose does not: the paragraph
on spellcasting focuses calls the middle role Priest, where the table and
the sentence that offers the choice both call it Healer. The table wins,
and the row says Healer.

Ten faults injected -- a feature moved to a level the book does not give
it, an improvement moved, a repeat moved, a feature renamed, a role
renamed, a role's ability changed, a class renamed, a row deleted -- are
all caught.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file            # noqa: E402

DUMP = os.path.join(ROOT, "TextFiles", "TASHAtext.txt")

CLASSES = ("Expert", "Spellcaster", "Warrior")

# "6th-level Expert feature", and the sidekick classes only.
LEVEL_LINE = re.compile(r"^(\d+)(?:st|nd|rd|th)-level (%s) feature$"
                        % "|".join(CLASSES))

# "At 4th level and again at 8th, 10th, 12th, 16th, and 19th level"
AGAIN = re.compile(r"again at ([\d\w,\s]+?) level", re.I)


def clean(s):
    s = s.replace("’", "'").replace("­", "")
    return re.sub(r"\s+", " ", s).strip()


def close_numbers(s):
    """Join up a number the extraction split: "1 2th" is 12th."""
    return re.sub(r"(?<=\d)\s+(?=\d)", "", s)


def pretty(head):
    return " ".join(w.capitalize() for w in head.split())


def book():
    """Tasha's sidekick features, as {class: {name: (level, text)}}."""
    lines = [clean(l) for l in open(DUMP, encoding="utf-8")]
    out = {c: {} for c in CLASSES}
    for i, l in enumerate(lines):
        m = LEVEL_LINE.match(close_numbers(l))
        if not m or i == 0:
            continue
        head = lines[i - 1]
        if not re.match(r"^[A-Z][A-Z '-]{2,40}$", head):
            continue
        # A description runs to the next feature's heading, and that can be
        # a long way: the Expert's level table is printed in the middle of
        # Expertise, between "the chosen proficiencies" and "At 15th level,
        # choose two more". A short window stops inside the table and loses
        # the sentence that grants the feature its second time.
        text, j = [], i + 1
        while j < len(lines) and j < i + 120:
            nxt = lines[j]
            if LEVEL_LINE.match(close_numbers(nxt)):
                break
            if re.match(r"^[A-Z][A-Z '-]{2,40}$", nxt) and \
                    j + 1 < len(lines) and \
                    LEVEL_LINE.match(close_numbers(lines[j + 1])):
                break
            text.append(nxt)
            j += 1
        out[m.group(2)][pretty(head)] = (int(m.group(1)),
                                         close_numbers(" ".join(text)))
    return out


def asi_levels(level, text):
    """Every level an Ability Score Improvement description names."""
    got = {level}
    m = AGAIN.search(text)
    if m:
        for n in re.findall(r"(\d+)(?:st|nd|rd|th)", m.group(1)):
            got.add(int(n))
    return got


def check_roles(problems):
    """The three Spellcaster roles, against the Spellcasting table."""
    lines = [clean(l) for l in open(DUMP, encoding="utf-8")]
    named = None
    for l in lines:
        m = re.search(r"role: ([A-Z]\w+), ([A-Z]\w+), or ([A-Z]\w+)", l)
        if m:
            named = list(m.groups())
            break
    ours = [r.str(0) for r in read_file("world.txt")
            if r.tag == "SIDEKICKROLE"]
    if named is None:
        problems.append(("SIDEKICKROLE", "the book does not name the roles "
                                         "where this looks for them"))
        return len(ours)
    if sorted(ours) != sorted(named):
        problems.append(("SIDEKICKROLE", "we have %s, the book offers %s"
                         % (", ".join(ours), ", ".join(named))))
    # The table's own rows: role, spell list, ability.
    joined = " ".join(lines)
    for r in read_file("world.txt"):
        if r.tag != "SIDEKICKROLE":
            continue
        want = re.search(r"\b%s ((?:\w+ and )?\w+) (Intelligence|Wisdom|"
                         r"Charisma)\b" % re.escape(r.str(0)), joined)
        if not want:
            problems.append(("SIDEKICKROLE",
                             "%s: the Spellcasting table does not give it a "
                             "spell list and an ability" % r.str(0)))
            continue
        said = r.str(1).lower()
        for w in want.group(1).lower().split(" and "):
            if w not in said:
                problems.append(("SIDEKICKROLE",
                                 "%s: the table's list is %r and the row does "
                                 "not say %r" % (r.str(0), want.group(1), w)))
        if want.group(2).lower() not in said:
            problems.append(("SIDEKICKROLE",
                             "%s: the table's ability is %s and the row does "
                             "not say so" % (r.str(0), want.group(2))))
    return len(ours)


def check_classes(problems, known):
    ours = [r.str(0) for r in read_file("world.txt")
            if r.tag == "SIDEKICKCLASS"]
    for n in ours:
        if n not in known:
            problems.append(("SIDEKICKCLASS",
                             "%r is not one of Tasha's sidekick classes" % n))
    for n in known:
        if n not in ours:
            problems.append(("SIDEKICKCLASS",
                             "Tasha's has a %s class and we have no row" % n))
    return len(ours)


def main():
    feats = book()
    problems = []
    n = check_classes(problems, CLASSES) + check_roles(problems)

    rows = [r for r in read_file("world.txt") if r.tag == "SIDEKICKFEATURE"]
    for r in rows:
        cls, level, name = r.str(0), r.int(1), r.str(2)
        if cls not in feats:
            problems.append(("SIDEKICKFEATURE",
                             "%s is not a sidekick class" % cls))
            continue
        got = feats[cls].get(name)
        if got:
            if name == "Ability Score Improvement":
                if level not in asi_levels(*got):
                    problems.append(
                        ("SIDEKICKFEATURE",
                         "%s: an Ability Score Improvement at %d, which the "
                         "book's list of levels does not have" % (cls, level)))
            elif got[0] != level:
                # A feature can be granted twice under the one name: the
                # Expert's Expertise is a 3rd-level feature whose own
                # description ends "At 15th level, choose two more". A
                # second row at that later level is the book's, so long as
                # the description names it.
                if not re.search(r"\b%dth level" % level, got[1]):
                    problems.append(("SIDEKICKFEATURE",
                                     "%s %s: we put it at %d, the book at %d"
                                     % (cls, name, level, got[0])))
            continue
        base = name[:-len(" improves")] if name.endswith(" improves") else None
        if base is None:
            problems.append(("SIDEKICKFEATURE",
                             "%s: the book gives the %s no feature called %r"
                             % (cls, cls, name)))
            continue
        if base not in feats[cls]:
            problems.append(("SIDEKICKFEATURE",
                             "%s %r improves nothing the book gives the %s"
                             % (cls, base, cls)))
        elif not re.search(r"\b%dth level" % level, feats[cls][base][1]):
            problems.append(("SIDEKICKFEATURE",
                             "%s %s: the book's %s does not improve at %dth "
                             "level" % (cls, name, base, level)))

    # And the other way: a feature the book gives that no row carries.
    for cls in CLASSES:
        ours = {r.str(2) for r in rows if r.str(0) == cls}
        for name, (level, text) in sorted(feats[cls].items()):
            if name in ours:
                continue
            problems.append(("SIDEKICKFEATURE",
                             "%s: the book gives it %s at %d and we have no "
                             "row" % (cls, name, level)))

    for tag, why in problems:
        print("  %-16s %s" % (tag, why))
    print("\n%d sidekick rows checked against Tasha's, %d disagree"
          % (n + len(rows), len(problems)))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
