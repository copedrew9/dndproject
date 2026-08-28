#!/usr/bin/env python3
"""Check the class, subclass and feature rows in data/character.txt.

tools/audit.py checks that every name in data/ can be found in the book it
claims. This checks the numbers and the filing beside those names, for the
thirteen CLASS rows, the hundred and seven SUBCLASS rows and the FEATURE
rows hung off both -- the places where a wrong value is silent, because
nothing in the program can tell a 15th-level feature from an 18th-level one.

Three sources in the books are read, each because it is printed in a fixed
shape and nowhere else:

  * The "Class Features" block -- "Hit Dice: 1d12 per barbarian level",
    "Saving Throws: Strength, Constitution", the Armor/Weapons/Tools lines,
    "Skills: Choose two from ...", and the bulleted starting equipment.
    That settles the CLASS rows.
  * The class's own advancement table ("The Barbarian", "The Artificer"),
    whose Features column names the class features and the level they
    arrive at, and whose row for the subclass label ("Primal Path") is
    where the subclass is chosen.  That settles subclass_level and the
    FEATURE rows filed against a class rather than a subclass.
  * Each subclass feature's own section, which opens with the level in a
    fixed handful of forms: "At 3rd level", "Starting at 7th level", "By
    7th level", "When you choose this archetype at 3rd level", and, in
    Tasha's, the standing line "6th-level College of Eloquence feature".

Where a subclass is filed -- its class and its book -- is settled by which
class's section of which book its heading sits in.  Xanathar's, Tasha's
and the Sword Coast guide each divide their class options under a heading
that is just the class's name; the PHB has no such heading the text layer
keeps, so its chapters are bounded by their Hit Dice lines instead.

What the typesetting does, and what is done about it. Every comparison is
made on letters and digits alone, lowercased, because the text layer sets
headings in small capitals that come back spaced out ("I m p r o v e d C o
m b a t Su p e r i o r i t y"), sets words with a space inside them ("Sim
ple weapons", "com ponent pouch", "1d 12", "1 7th"), hyphenates at line
breaks, and turns 1 into l ("ld8").  Squashing repairs all of those at
once and costs nothing, since none of the values being read are prose.

Two-column pages interleave, so a feature's section can be cut in half by
an unrelated clause -- the Battle Master's Know Your Enemy has the
Champion's Superior Critical sitting in the middle of it.  A level is
therefore read only from the opening of a section, within its first
hundred and sixty squashed characters, which is a sentence of flavour and
no more, and a section is cut short at the next heading.  A feature whose
section does not open with a level at all is left unchecked rather than
guessed at; so is one whose heading the interleaving has carried out of
its own subclass's section, which is most of what the Sword Coast
Adventurer's Guide loses, its two columns being the worst interleaved of
the four books.  The sixteen Channel Divinity options are unchecked for a
different reason: "Channel Divinity: Sacred Weapon" is our composite name
and the books head those paragraphs with the option's name alone, or not
at all.

Deliberately not checked: the prose of a feature (only its level is read);
the caster, preparation, multiclassing, gold and quick-build columns, which
are our own shorthand rather than anything the book prints in a fixed
shape; and two proficiency lines the data paraphrases on purpose, named in
PARAPHRASED below with the reason.

Equipment is compared bullet for bullet, with the articles and
conjunctions dropped from both sides first, because the books' lists carry
an Oxford comma the data does not and Tasha's writes "your choice of
studded leather armor or scale mail" where the data writes it as the (a)/(b)
choice the rest of the file uses.  What survives that is the items
themselves, which is what is worth comparing.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

BOOKS = {
    "PHB":  "PHBtext.txt",
    "XGE":  "XANATHARtext.txt",
    "TCE":  "TASHAtext.txt",
    "SCAG": "SCAGtext.txt",
}

CLASSES = ["Barbarian", "Bard", "Cleric", "Druid", "Fighter", "Monk",
           "Paladin", "Ranger", "Rogue", "Sorcerer", "Warlock", "Wizard",
           "Artificer"]

ABILITIES = [("Strength", "STR"), ("Dexterity", "DEX"),
             ("Constitution", "CON"), ("Intelligence", "INT"),
             ("Wisdom", "WIS"), ("Charisma", "CHA")]

SKILLS = ["Acrobatics", "Animal Handling", "Arcana", "Athletics",
          "Deception", "History", "Insight", "Intimidation",
          "Investigation", "Medicine", "Nature", "Perception",
          "Performance", "Persuasion", "Religion", "Sleight of Hand",
          "Stealth", "Survival"]

COUNTS = {"one": 1, "two": 2, "three": 3, "four": 4, "five": 5}

# Proficiency lines the data states in its own words, with the reason. The
# book's wording is printed beside the row so a person can still read them
# against each other; they are not counted as disagreements.
PARAPHRASED = {
    ("Druid", "armour"):
        "the data shortens the book's parenthesis to \"nonmetal\"",
    ("Monk", "tools"):
        "the data says \"one artisan's tool\" for the book's \"one type of "
        "artisan's tools\"",
}

# Words dropped from both sides of an equipment bullet before comparing.
# Everything here is grammar rather than an item: the books' Oxford comma,
# the (a)/(b) lettering, and the "your choice of" the artificer's list uses
# where the rest of the file writes a lettered choice.
FILLER = re.compile(r"\b(a|an|the|any|your|choice|of|or|and|if|armor)\b", re.I)
MARKER = re.compile(r"\((?:[abc])\)")

ORDINAL = {1: "st", 2: "nd", 3: "rd"}


# ------------------------------------------------------------- the dumps

def suffix(n):
    return ORDINAL.get(n if n < 20 else n % 10, "th")


def squash(s):
    """Letters and digits only, lowercased.

    This is the only comparison the file makes. It repairs spaced small
    capitals, words set with a space inside them, hyphenation at line
    breaks and the books' curly punctuation in one go.
    """
    return re.sub(r"[^a-z0-9]", "", s.lower())


def loose(s):
    """A regex matching s however much punctuation and space is set in it."""
    return "[^A-Za-z0-9]*".join(re.escape(c) for c in s)


def hitdice(cls):
    """The one line each PHB and Tasha's class prints exactly once.

    The class's name is spelt out letter by letter because the text layer
    sets a space inside it often enough to matter: the monk's line arrives
    as "Hit Dice: 1d8 per m onk level".
    """
    return r"Hit Dice:\s*[l1]\s*d\s*(\d+)\s*per\s*%s\s*level" % loose(cls)


HEAD_CAPS = re.compile(r"^[A-Z0-9][A-Z0-9 '.,:!&()\-]{2,47}$")


def spaced_heading(line):
    """Whether a line is a heading set in spaced-out small capitals.

    The PHB sets them as "I m p r o v e d C o m b a t Su p e r i o r i t y";
    the other three books set them in plain capitals.
    """
    words = line.split()
    if len(words) < 3 or len(line) > 60:
        return False
    singles = sum(1 for w in words if len(w) == 1 and w.isalpha())
    return singles * 2 > len(words)


class Book(object):
    """One dump, with its lines and the offsets they start at."""

    def __init__(self, path):
        raw = open(path, encoding="utf-8", errors="replace").read()
        raw = raw.replace("­", "").replace("’", "'")
        self.lines = [re.sub(r"\s+", " ", l).strip() for l in raw.split("\n")]
        self.at, pos = [], 0
        for l in self.lines:
            self.at.append(pos)
            pos += len(l) + 1
        self.text = " ".join(self.lines)
        self.lsq = [squash(l) for l in self.lines]
        self.ends = [self.at[i] + len(l) for i, l in enumerate(self.lines)]
        self.index = {}
        for i, key in enumerate(self.lsq):
            if key:
                self.index.setdefault(key, []).append((self.at[i],
                                                       self.ends[i]))
        for i in range(len(self.lines) - 1):
            if self.lsq[i] and self.lsq[i + 1]:
                self.index.setdefault(self.lsq[i] + self.lsq[i + 1],
                                      []).append((self.at[i],
                                                  self.ends[i + 1]))
        self.seen = {}
        self.heads = sorted(self.at[i] for i, l in enumerate(self.lines)
                            if HEAD_CAPS.match(l) or spaced_heading(l))

    def next_head(self, pos, cap):
        """Where the section starting at pos ends."""
        end = pos + cap
        for h in self.heads:
            if pos < h < end:
                return h
        return end

    def find_lines(self, name):
        """Where a heading reading `name` starts, and where its text does.

        Four tolerances, all of them things the dumps do to headings and
        nothing else: a heading too long for its column is broken over two
        lines ("C h a n n e l D i v i n i t y : C h a r m A n i m a l s" /
        "a n d P l a n t s"); a section opening with a decorative initial
        loses that letter ("UICKENED HEALING"); Xanathar's puts the word
        "features" after a subclass name to head its table; and a single
        letter goes missing inside a word often enough to be worth
        allowing ("COLLEGE OF ELO UENCE").
        """
        want = squash(name)
        if len(want) < 4:
            return []
        if want in self.seen:
            return self.seen[want]
        keys = [want, want + "features", want[1:]]
        if len(want) > 8:
            keys += [want[:k] + want[k + 1:] for k in range(4, len(want))]
        out = []
        for key in keys:
            out += self.index.get(key, [])
        out = sorted(set(out))
        self.seen[want] = out
        return out


# --------------------------------------------------- where each class sits

def class_sections(book, name):
    """Where each class's options sit in one dump, by its own heading.

    Xanathar's, Tasha's and the Sword Coast guide each divide their class
    options under a heading that is only the class's name (or its plural),
    so those are read straight off. The PHB has no such heading in a form
    the text layer keeps, so its chapters are bounded by the one line each
    of them prints exactly once: "Hit Dice: 1dN per <class> level".
    """
    spans, marks = {}, []
    if name == "PHB":
        for cls in CLASSES:
            m = re.search(hitdice(cls), book.text, re.I)
            if m:
                marks.append((m.start(), cls))
    else:
        for cls in CLASSES:
            want = squash(cls)
            hits = [book.at[i] for i, l in enumerate(book.lines)
                    if l.isupper() and squash(l).rstrip("s") == want]
            if len(hits) >= 1:
                marks.append((hits[0], cls))
    marks.sort()
    for i, (pos, cls) in enumerate(marks):
        end = marks[i + 1][0] if i + 1 < len(marks) else pos + 60000
        spans[cls] = (pos, end)
    return spans


def which_class(spans, pos):
    for cls, (a, b) in spans.items():
        if a <= pos < b:
            return cls
    return None


# ------------------------------------------------------ the class features

LABELS = [("armour", r"\bA\s?r\s?m\s?or:\s*"), ("weapons", r"\bWeapons:\s*"),
          ("tools", r"\bTools:\s*"), ("saves", r"\bSaving Throws:\s*"),
          ("skills", r"\bSkills:\s*")]

STOP = re.compile(r"\bA\s?r\s?m\s?or:|\bWeapons:|\bTools:|\bSaving Throws:"
                  r"|\bSkills:|\bE\s?q\s?u\s?i\s?p|\bEQUIP|\bMULTI|===")

EQUIP_SENTENCE = loose("Youstartwiththefollowingequipmentinadditiontothe"
                       "equipmentgrantedbyyourbackground")


def class_block(book, cls):
    """The Class Features block for one class: hit die and each label."""
    m = re.search(hitdice(cls), book.text, re.I)
    if not m:
        return None
    out = {"hitdie": int(m.group(1)), "at": m.start()}
    for key, pat in LABELS:
        best = None
        for hit in re.finditer(pat, book.text):
            d = abs(hit.start() - m.start())
            if best is None or d < best[0]:
                best = (d, hit.end())
        if best is None or best[0] > 20000:
            continue
        tail = book.text[best[1]:best[1] + 400]
        cut = STOP.search(tail)
        out[key] = tail[:cut.start()] if cut else tail
    return out


def bullet_runs(book):
    """Every run of bulleted lines in a dump, as a list of bullets.

    The starting equipment cannot be read as the text that follows the
    sentence introducing it: the ranger's list sits a whole column away
    from its own sentence, with the Natural Explorer feature in between.
    So the runs are collected first and the nearest one is taken.
    """
    if getattr(book, "runs", None) is not None:
        return book.runs
    lines, n = book.lines, len(book.lines)
    runs, i = [], 0
    while i < n:
        if not lines[i].startswith("•"):
            i += 1
            continue
        j = i
        while j + 1 < n:
            k = j + 1
            if lines[k].startswith("•"):
                j = k
            elif lines[k] and (not lines[j].strip("• ")
                               or any(lines[m].startswith("•")
                                      for m in range(k + 1, min(k + 3, n)))):
                j = k                      # a bullet wrapped onto two lines
            else:
                break
        joined = " ".join(lines[i:j + 1])
        bullets = [b.strip() for b in joined.split("•")[1:] if b.strip()]
        runs.append((book.at[i], bullets))
        i = j + 1
    book.runs = runs
    return runs


def class_equipment(book, at):
    """The starting equipment list nearest a class's Hit Dice line.

    An equipment list is a short run of bullets none of which is a
    sentence; the bulleted lists a class's features carry -- the ranger's
    Natural Explorer, the barbarian's Rage -- all end in a full stop.
    """
    best = None
    for m in re.finditer(EQUIP_SENTENCE, book.text, re.I):
        d = abs(m.start() - at)
        if best is None or d < best[0]:
            best = (d, m.end())
    if best is None or best[0] > 20000:
        return None
    after = best[1]
    pick = None
    for pos, bullets in bullet_runs(book):
        if not 2 <= len(bullets) <= 6:
            continue
        if any(b.endswith(".") or len(b) > 130 for b in bullets):
            continue
        d = abs(pos - after)
        if d < 6000 and (pick is None or d < pick[0]):
            pick = (d, pos, bullets)
    if pick is None:
        return None
    _, pos, bullets = pick
    # Tasha's loses the bullet in front of the artificer's first line, so
    # a short piece of text between the sentence and the run is one too.
    if 0 <= pos - after < 130:
        lead = book.text[after:pos].strip(" :")
        if len(squash(lead)) > 6:
            bullets = [lead] + bullets
    return bullets


def item_words(s):
    """An equipment bullet with its grammar taken out and squashed."""
    return squash(FILLER.sub(" ", MARKER.sub(" ", s)))


# ------------------------------------------------------- the class tables

def class_table(book, cls):
    """Level -> the feature names printed in that row of the class table."""
    start = None
    for pat in ("The%sLevelProficiency" % cls, "The%sProficiency" % cls):
        m = re.search(loose(pat), book.text, re.I)
        if m:
            start = m.end()
            break
    if start is None:
        return {}
    region = book.text[start:start + 9000]
    rows, pos = {}, 0
    bounds = []
    for lvl in range(1, 21):
        digits = r"\s?".join(str(lvl))
        m = re.compile(r"%s\s*%s\s+\+\s?\d" % (digits, suffix(lvl))).search(
            region, pos)
        if not m:
            break
        bounds.append((lvl, m.start(), m.end()))
        pos = m.end()
    widest = max([b[1] - a[2] for a, b in zip(bounds, bounds[1:])] or [90])
    for i, (lvl, _, end) in enumerate(bounds):
        if i + 1 < len(bounds):
            stop = bounds[i + 1][1]
        else:
            # Nothing below the last row bounds it, so it is cut at the
            # next heading, at the running page marker, and at the width
            # of the widest row above it.
            stop = min(book.next_head(start + end, widest) - start,
                       end + widest, len(region))
            while stop < len(region) and region[stop].isalnum():
                stop += 1                  # never cut a word in half
        cell = region[end:max(stop, end)].split("===")[0]
        rows[lvl] = table_cell(cell)
    return rows


NUMERIC = re.compile(r"[—–\-]+|=+|\+?\d+|\d*d\d+|ft\.?|"
                     r"\d+(?:st|nd|rd|th)|\d+/\d+")


def table_cell(chunk):
    """The Features cell of one table row, split into feature names.

    Every other column is a number, a die or a dash, so the features are
    the longest run of words in the row. Numbers inside brackets belong to
    the feature -- "Extra Attack (2)", "Destroy Undead (CR 1/2)" -- so
    brackets are stitched shut before the row is split up.
    """
    out, depth = [], 0
    for ch in chunk:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth = max(0, depth - 1)
        out.append("\x01" if (ch == " " and depth) else ch)
    runs, cur = [], []
    for tok in "".join(out).split():
        if NUMERIC.fullmatch(tok.replace("\x01", " ")):
            if cur:
                runs.append(cur)
                cur = []
        else:
            cur.append(tok.replace("\x01", " "))
    if cur:
        runs.append(cur)
    if not runs:
        return []
    best = max(runs, key=lambda r: len(" ".join(r)))
    return [p.strip() for p in " ".join(best).split(",") if p.strip()]


WORDNUM = re.compile(r"\b(one|two|three)\b", re.I)


def cellkey(s):
    """Squashed, with the counts the tables spell out written as digits."""
    return squash(WORDNUM.sub(lambda m: str(COUNTS[m.group(1).lower()]), s))


# ------------------------------------------------- a feature's own section

OPENS = re.compile(r"^(\d+)(?:st|nd|rd|th)level")
INLINE = re.compile(r"(?:at|by|reach|reaching)(\d+)(?:st|nd|rd|th)level")


def feature_levels(book, pos, cap=1400):
    """The levels a feature's section gives, or None if it does not open
    with one.

    Only a section that states its level in its first hundred and sixty
    squashed characters is read -- a sentence of flavour and no more --
    because a two-column page can drop an unrelated clause into the middle
    of one, and a level further down would then be somebody else's.
    """
    end = book.next_head(pos + 1, cap)
    block = squash(book.text[pos:end])
    head = block[:160]
    opens = OPENS.match(block)
    if not opens and not INLINE.search(head):
        return None
    levels = set([int(opens.group(1))]) if opens else set()
    for hit in INLINE.finditer(block):
        levels.add(int(hit.group(1)))
    return levels or None


# ------------------------------------------------------------- the report

class Report(object):
    def __init__(self):
        self.checked = 0
        self.bad = 0
        self.skipped = []

    def ok(self, n=1):
        self.checked += n

    def fail(self, what, detail):
        self.bad += 1
        print("  %s" % what)
        print("      %s" % detail)

    def skip(self, what, why):
        self.skipped.append((what, why))


def check_classes(books, rows, rep):
    print("class rows")
    for r in rows:
        cls, book = r.str(0), r.str(1)
        b = books.get(book)
        if b is None:
            rep.skip(cls, "no dump for %s" % book)
            continue
        blk = class_block(b, cls)
        if blk is None:
            rep.skip(cls, "no \"Hit Dice: 1dN per %s level\" line"
                     % cls.lower())
            continue

        rep.ok()
        if blk["hitdie"] != r.int(2):
            rep.fail(cls, "hit die d%d, book says d%d"
                     % (r.int(2), blk["hitdie"]))

        if "saves" in blk:
            key = squash(blk["saves"])
            found = sorted(((key.find(a.lower()), s)
                            for a, s in ABILITIES if a.lower() in key))
            rep.ok()
            if len(found) == 2:
                want = [s for _, s in found]
                if want != [r.str(3), r.str(4)]:
                    rep.fail(cls, "saves %s/%s, book says %s/%s"
                             % (r.str(3), r.str(4), want[0], want[1]))
            else:
                rep.skip(cls + " saves", "the line reads %r" % blk["saves"])

        for key, field in (("armour", 5), ("weapons", 6), ("tools", 7)):
            if key not in blk:
                rep.skip("%s %s" % (cls, key), "the line is not in the dump")
                continue
            ours = r.str(field) or "None"
            theirs = blk[key]
            if (cls, key) in PARAPHRASED:
                rep.skip("%s %s" % (cls, key), "%s; the book prints %r"
                         % (PARAPHRASED[(cls, key)], theirs.strip()))
                continue
            rep.ok()
            if squash(ours) != squash(theirs):
                rep.fail(cls, "%s %r,\n          book says %r"
                         % (key, ours, theirs.strip()))

        if "skills" in blk:
            line = blk["skills"]
            key = squash(line)
            rep.ok()
            m = re.search(r"choose(any)?(one|two|three|four|five)", key)
            if not m:
                rep.skip(cls + " skills", "the line reads %r" % line)
            else:
                picks = COUNTS[m.group(2)]
                if picks != r.int(9):
                    rep.fail(cls, "%d skill picks, book says %d"
                             % (r.int(9), picks))
                if m.group(1):                      # "Choose any three"
                    theirs = set(SKILLS)
                else:
                    theirs = set(s for s in SKILLS if squash(s) in key)
                ours = set(x.strip() for x in r.str(8).split(",") if x.strip())
                rep.ok()
                if ours != theirs:
                    rep.fail(cls, "skills %s,\n          book says %s"
                             % (", ".join(sorted(ours)) or "-",
                                ", ".join(sorted(theirs)) or "-"))

        if not r.str(21):
            continue
        bullets = class_equipment(b, blk["at"])
        ours = r.str(21).split("|")
        if bullets is None:
            rep.skip(cls + " equipment", "the bulleted list is not in the dump")
            continue
        rep.ok()
        if len(bullets) != len(ours):
            rep.fail(cls, "%d equipment lines, book prints %d"
                     % (len(ours), len(bullets)))
            continue
        for mine, theirs in zip(ours, bullets):
            if item_words(mine) != item_words(theirs):
                rep.fail(cls, "equipment %r,\n          book says %r"
                         % (mine, theirs))


def check_subclass_filing(books, subs, rep):
    """Which class's section of which book each subclass heading sits in."""
    print("\nsubclass class and book")
    spans = dict((name, class_sections(b, name)) for name, b in books.items())
    for r in subs:
        cls, name, book = r.str(0), r.str(1), r.str(2)
        placed = {}
        for bname, b in books.items():
            for pos, _ in b.find_lines(name):
                got = which_class(spans[bname], pos)
                if got:
                    placed.setdefault((bname, got), 0)
                    placed[(bname, got)] += 1
        if not placed:
            rep.skip(name, "no heading for it in any dump's class sections")
            continue
        rep.ok()
        if (book, cls) in placed:
            continue
        where = sorted(placed, key=lambda k: -placed[k])[0]
        rep.fail("%s (%s, %s)" % (name, cls, book),
                 "its heading is in the %s section of the %s"
                 % (where[1], where[0]))


def check_subclass_level(rows, tables, rep):
    print("\nthe level a subclass is chosen at")
    for r in rows:
        cls, book, label, lvl = r.str(0), r.str(1), r.str(14), r.int(13)
        table = tables.get(cls)
        if not table:
            rep.skip(cls, "its class table is not in the dump")
            continue
        want = squash(label)
        at = [n for n, cells in table.items()
              if any(squash(c) == want for c in cells)]
        if not at:
            rep.skip("%s (%s)" % (cls, label),
                     "the table's rows do not name it")
            continue
        rep.ok()
        if lvl not in at:
            rep.fail("%s, %s" % (cls, label),
                     "chosen at %d, table prints it at %s"
                     % (lvl, ", ".join(str(a) for a in sorted(at))))


def check_class_features(feats, tables, rep):
    print("\nclass features against the class table")
    for r in feats:
        cls, lvl, name = r.str(0), r.int(2), r.str(3)
        table = tables.get(cls)
        if not table:
            rep.skip("%s %s" % (cls, name), "its class table is not in the "
                     "dump")
            continue
        want = cellkey(name)
        at = set()
        for n, cells in table.items():
            for c in cells:
                k = cellkey(c)
                if k == want or k.startswith(want):
                    at.add(n)
        if not at:
            rep.skip("%s %s" % (cls, name),
                     "the table's Features column does not name it")
            continue
        rep.ok()
        if lvl not in at:
            rep.fail("%s, %s" % (cls, name),
                     "level %d, table prints it at %s"
                     % (lvl, ", ".join(str(a) for a in sorted(at))))


def subclass_spans(books, subs, feats):
    """Where each subclass's own section runs, inside its class's section.

    A subclass name can head more than one line: Tasha's lists the names
    of the subclasses it is about to give at the head of each class, and
    Xanathar's puts the name again on the table of the subclass's
    features. So every line that heads the name is a candidate, and the
    one taken is whichever leaves the most of the subclass's own features
    inside the section it opens.
    """
    spans = dict((name, class_sections(b, name)) for name, b in books.items())
    owned = {}
    for f in feats:
        if f.str(1):
            owned.setdefault((f.str(0), f.str(1)), []).append(f.str(3))

    cand, marks = {}, dict((name, []) for name in books)
    for r in subs:
        cls, name, book = r.str(0), r.str(1), r.str(2)
        b = books.get(book)
        span = spans[book].get(cls) if b is not None else None
        if not span:
            continue
        hits = [p for p, _ in b.find_lines(name) if span[0] <= p < span[1]]
        if hits:
            cand[(cls, name)] = (book, hits, span[1])
            marks[book] += [(p, (cls, name)) for p in hits]
    for book in marks:
        marks[book].sort()

    out = {}
    for key, (book, hits, limit) in cand.items():
        b = books[book]
        best = None
        for pos in hits:
            nxt = [p for p, k in marks[book] if p > pos and k != key]
            end = min(nxt[0], limit) if nxt else limit
            score = sum(1 for n in set(owned.get(key, []))
                        if any(pos <= q < end for q, _ in b.find_lines(n)))
            if best is None or score > best[0]:
                best = (score, pos, end)
        out[key] = (book, best[1], best[2])
    return out


def check_subclass_features(books, feats, spans, rep):
    print("\nsubclass features against their own sections")
    for r in feats:
        cls, sub, lvl, name = r.str(0), r.str(1), r.int(2), r.str(3)
        span = spans.get((cls, sub))
        if span is None:
            rep.skip("%s / %s" % (sub, name), "its subclass section could "
                     "not be found")
            continue
        book, start, end = span
        b = books[book]
        hits = [q for p, q in b.find_lines(name) if start <= p < end]
        if not hits:
            hits = [q for p, q in b.find_lines(name.split(": ", 1)[-1])
                    if start <= p < end]
        if not hits:
            rep.skip("%s / %s" % (sub, name),
                     "no heading for it inside the subclass's section")
            continue
        seen = [feature_levels(b, p) for p in hits]
        seen = [s for s in seen if s]
        if not seen:
            rep.skip("%s / %s" % (sub, name),
                     "its section does not open with a level")
            continue
        union = set()
        for s in seen:
            union |= s
        rep.ok()
        if lvl not in union:
            rep.fail("%s / %s (%s, %s)" % (sub, name, cls, book),
                     "level %d, the book says %s"
                     % (lvl, ", ".join(str(x) for x in sorted(union))))


def main():
    books = {}
    for name, path in BOOKS.items():
        full = os.path.join(ROOT, "TextFiles", path)
        if not os.path.exists(full):
            sys.exit("verify_classes: no %s" % full)
        books[name] = Book(full)

    recs = read_file("character.txt")
    classes = [r for r in recs if r.tag == "CLASS"]
    subs = [r for r in recs if r.tag == "SUBCLASS"]
    feats = [r for r in recs if r.tag == "FEATURE"]

    tables = {}
    for r in classes:
        b = books.get(r.str(1))
        if b is not None:
            tables[r.str(0)] = class_table(b, r.str(0))

    rep = Report()
    check_classes(books, classes, rep)
    check_subclass_filing(books, subs, rep)
    check_subclass_level(classes, tables, rep)
    check_class_features([f for f in feats if not f.str(1)], tables, rep)
    spans = subclass_spans(books, subs, feats)
    check_subclass_features(books, [f for f in feats if f.str(1)], spans, rep)

    if rep.skipped:
        print("\nleft unchecked, the dump not settling them:")
        for what, why in rep.skipped:
            print("    %-46s %s" % (what[:46], why))

    print("\n%d checked, %d disagree (%d left unchecked)"
          % (rep.checked, rep.bad, len(rep.skipped)))
    return 1 if rep.bad else 0


if __name__ == "__main__":
    sys.exit(main())
