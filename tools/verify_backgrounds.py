#!/usr/bin/env python3
"""Check the backgrounds in data/character.txt against the PHB and the SCAG.

Two things are checked. The BACKGROUND rows carry the numbers and names a
character sheet is built from -- which skills, which tools, how many extra
languages, the feature's name, the starting gold -- and the BGTRAIT,
BGIDEAL, BGBOND and BGFLAW rows carry the suggested characteristics printed
under each background in the PHB. Both are hand-written, and both are the
kind of thing nothing else would ever notice was wrong.

The stat blocks are easy: every background in both books opens with a
"Skill Proficiencies:" line followed by the other labelled lines, and there
are exactly thirteen such lines in the PHB dump and twelve in the SCAG one,
in the same (alphabetical) order the rows are written in. The thirteenth
SCAG row, Investigator, is the City Watch variant, which the book gives as
one sentence -- "you have proficiency in Investigation rather than
Athletics" -- rather than as a block of its own; it is checked against City
Watch's block with that swap applied, and the sentence itself is required to
be present.

Several SCAG backgrounds leave one skill open ("Persuasion, plus one from
among Arcana, History, Nature, and Religion"). The skill line is parsed into
the skills that are fixed, how many are chosen, and what they are chosen
from, so that a row which quietly picked one of them would show up as a
fixed skill the book does not give. The faction agent's "one Intelligence,
Wisdom, or Charisma skill of your choice" is the one line that names no
skills at all; the thirteen it means are written out below, since they come
from chapter 7 rather than from the line being read.

Four things the dumps do that this has to work around.

Words are set with a space inside them -- "Perform ance", "Anim al
Handling", "a holy sym bol" -- so everything compared against the books is
compared with the spaces taken out, not word by word.

Small capitals come back mixed and letter-spaced: the PHB's feature
headings arrive as "F e a t u r e : S h e l t e r o f t h e Fa i t h f u l".
That is recognisable by shape, and it also marks where a labelled field
stops, which matters because the two-column pages drop an unrelated column
into the middle of one: the faction agent's equipment sits sixty lines below
its skill line, with a sidebar in between.

The equipment lines in data/ are deliberately shortened -- the PHB gives the
acolyte "a holy symbol (a gift to you when you entered the priesthood)" and
the row says "Holy symbol" -- so equipment is checked one way only: every
word a row names has to be in the book's list. What the row leaves out is
not reported, and neither is the far traveler's 10 gp trinket or the
sailor's, which the rows drop on purpose. The starting gold is a number and
is checked exactly, read as the amount in the "pouch containing N gp" the
list ends with.

The suggested-characteristics tables are the awkward part, and the reason
this does more work than it looks like it should. The tables are printed in
two columns and the extraction interleaves them, so they do not arrive in
the order the backgrounds are printed in: the hermit's bond and flaw tables
land below the noble's skill line, and the soldier's and urchin's bond
tables arrive the wrong way round. Reading the table under a background is
therefore not possible. Instead all fifty-two tables are collected, and each
background is given the table of its kind that its own rows collectively
match best. That assignment is not a close call -- every background matches
its own table at 0.72 to 1.00 and the next best table at 0.34 or less -- but
it is printed anyway so that it can be read.

The rows are paraphrases, and shorter than the book's ("I quote sacred texts
in almost every situation" for "I quote (or misquote) sacred texts and
proverbs in almost every situation"), and they are written in British
spelling where the book is American. So a row is matched to an entry by how
much of the row's vocabulary the entry accounts for, allowing for both. That
makes the score a blunt instrument: it says whether a row came from an entry
at all, not whether the rewrite is faithful. Only a row that matches nothing
in its table is reported, and only when the dump has that table complete --
the dump loses eight entries altogether, six of the criminal's personality
traits and two others, and a row that would have matched one of those is
reported as unchecked rather than guessed at. Rows that match an entry
loosely are listed separately, unjudged, because deciding whether a
paraphrase has gone too far is a person's job and not this script's.

The ideals are printed with the ideal's name first and an alignment in
brackets after ("Tradition. The ancient traditions ... (Lawful)"). The name
is checked. No BGIDEAL row stores an alignment at present, so the check for
one finds nothing to do; it is written anyway, so that the day a row carries
one it is compared rather than believed.
"""

import difflib
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

DUMPS = {"PHB": os.path.join(ROOT, "TextFiles", "PHBtext.txt"),
         "SCAG": os.path.join(ROOT, "TextFiles", "SCAGtext.txt")}

SKILLS = ["Acrobatics", "Animal Handling", "Arcana", "Athletics",
          "Deception", "History", "Insight", "Intimidation",
          "Investigation", "Medicine", "Nature", "Perception",
          "Performance", "Persuasion", "Religion", "Sleight of Hand",
          "Stealth", "Survival"]

# The faction agent's line names an ability rather than a skill. These are
# the skills of those three abilities, from chapter 7.
BY_ABILITY = ["Arcana", "History", "Investigation", "Nature", "Religion",
              "Animal Handling", "Insight", "Medicine", "Perception",
              "Survival", "Deception", "Intimidation", "Performance",
              "Persuasion"]

LABELS = ("Skill Proficiencies", "Tool Proficiencies", "Languages",
          "Equipment")
LABEL_RE = re.compile(r"^(%s):\s*(.*)$" % "|".join(LABELS))

TABLE_HEAD = re.compile(r"^(Personality Trait|Ideal|Bond|Flaw)$")
ROLL = ("d4", "d6", "d8", "d10", "d12", "d20")
NUMBER = re.compile(r"^(\d{1,2})$")

# The variant's whole rule, in one sentence.
INVESTIGATOR = "proficiency in Investigation rather than Athletics"

STOP = set(("a an the i my me of to in and or that is are for with it they "
            "them on as at be but from have has had no not so up out all any "
            "was were will would can could do does when who what their this "
            "those these you your one am s t").split())

BRITISH = [("ise", "ize"), ("ised", "ized"), ("ising", "izing"),
           ("isation", "ization"), ("our", "or"), ("ll", "l"),
           ("re", "er"), ("ce", "se")]


# ------------------------------------------------------------ normalising


def clean(s):
    for a, b in ((u"’", "'"), (u"‘", "'"), (u"“", '"'),
                 (u"”", '"'), (u"—", "-"), (u"–", "-")):
        s = s.replace(a, b)
    return s


def squeeze(s):
    """Letters and digits only. Undoes every space the typesetting added."""
    return re.sub(r"[^a-z0-9]", "", clean(s).lower())


def words(s):
    """The content words of a line, with the run-together spellings kept."""
    s = re.sub(r"[^a-z ]", " ", clean(s).lower())
    return [w for w in s.split() if len(w) > 2 and w not in STOP]


def variants(w):
    """A word and the American spellings of it, and its plural and singular."""
    out = set([w, stem(w)])
    for a, b in BRITISH:
        if w.endswith(a) and len(w) - len(a) >= 3:
            out.add(w[:-len(a)] + b)
        if a in w:
            out.add(w.replace(a, b))
    for v in list(out):
        out.add(v + "s")
        if v.endswith("s"):
            out.add(v[:-1])
    return out


def stem(w):
    for suf in ("ing", "ed", "es", "s"):
        if w.endswith(suf) and len(w) - len(suf) >= 4:
            return w[:-len(suf)]
    return w


def same_word(a, b):
    if a == b or stem(a) == stem(b):
        return True
    if variants(a) & variants(b):
        return True
    return difflib.SequenceMatcher(None, a, b).ratio() >= 0.82


def covered(row, book):
    """How much of a row's vocabulary the book's sentence accounts for."""
    ours, theirs = words(row), words(book)
    if not ours:
        return 1.0
    hit = sum(1 for w in ours if any(same_word(w, t) for t in theirs))
    return hit / float(len(ours))


def contained(row, book):
    """The words of a row that are nowhere in the book's line.

    Compared against the run-together text rather than word by word, because
    the extraction sets "symbol" as "sym bol" and "common" as "com m on".
    """
    flat = squeeze(book)
    return [w for w in words(row)
            if not any(v in flat for v in variants(w))]


def unlisted(row, book):
    """The items a row's list names that the book's list does not.

    An item is one comma-separated part of the row, and it counts as the
    book's if the book accounts for half its words. Half, because the rows
    are written in their own words and shortened -- the sage's letter poses
    "a question you have not yet answered" where the book's poses one "you
    have not yet been able to answer" -- and a rule strict enough to mind
    that reports every row in the file.
    """
    out = []
    for part in re.split(r",| and ", clean(row)):
        ours = words(part)
        if not ours:
            continue
        missing = contained(part, book)
        if len(missing) > len(ours) / 2.0:
            out.append(part.strip())
    return out


# --------------------------------------------------------- the stat blocks


def is_heading(line):
    """A line that ends a labelled field: a heading, or a page marker."""
    t = clean(line).strip()
    if not t or t.startswith("==="):
        return True
    if re.match(r"^[A-Z][A-Z0-9 ':,.-]{3,}$", t):        # SCAG's small caps
        return True
    parts = t.split()
    singles = sum(1 for p in parts if len(p) == 1)
    return len(parts) >= 4 and singles > len(parts) / 2   # the PHB's


def stat_blocks(lines):
    """Every 'Skill Proficiencies:' line with the fields that follow it.

    A field runs from its label to the next label. Equipment is allowed to
    run on past a blank line, because the extraction splits one in half; the
    others stop at the first, because the two-column pages put a paragraph
    of something else directly below them.
    """
    starts = [i for i, l in enumerate(lines)
              if l.startswith("Skill Proficiencies:")]
    out = []
    for k, s in enumerate(starts):
        end = starts[k + 1] if k + 1 < len(starts) else min(len(lines),
                                                            s + 120)
        fields, i = {}, s
        while i < end:
            m = LABEL_RE.match(clean(lines[i]).strip())
            if not m:
                i += 1
                continue
            name, text, j = m.group(1), [m.group(2)], i + 1
            blanks = 0
            while j < end and len(text) < 9:
                t = clean(lines[j]).strip()
                if LABEL_RE.match(t) or t.startswith("==="):
                    break
                if not t:
                    blanks += 1
                    if name != "Equipment" or blanks > 2:
                        break
                    j += 1
                    continue
                if is_heading(t) or t in ROLL:
                    break
                text.append(t)
                j += 1
            fields.setdefault(name, " ".join(text).strip())
            i = j
        out.append({"start": s, "end": end, "fields": fields})
    return out


def feature_name(lines, block):
    """The heading of the feature inside a block, or None.

    The PHB letter-spaces it, the SCAG sets it in capitals, and both books
    print variant features too -- those are skipped, being always below the
    background's own.
    """
    for i in range(block["start"], block["end"]):
        t = clean(lines[i]).strip()
        flat = squeeze(t)
        if not flat.startswith("feature"):
            continue
        if squeeze(t[:20]).startswith("variant"):
            continue
        name = t.split(":", 1)[1] if ":" in t else ""
        return re.sub(r"\s+", " ", name).strip()
    return None


CHOICES = [
    ("choosetwofromamong", 2, None),
    ("chooseonefromamong", 1, None),
    ("plusyourchoiceofonefromamong", 1, None),
    ("plusonefromamong", 1, None),
    ("andoneintelligencewisdomorcharismaskillofyourchoice", 1, BY_ABILITY),
]


def read_skills(text):
    """A skill line, as (the fixed skills, how many are chosen, from what)."""
    flat = squeeze(text)
    at, after, take, fixed_list = len(flat), len(flat), 0, None
    for marker, n, listed in CHOICES:
        p = flat.find(marker)
        if 0 <= p < at:
            at, take, fixed_list = p, n, listed
            after = p + len(marker)
    found = []
    for name in SKILLS:
        p = 0
        while True:
            p = flat.find(squeeze(name), p)
            if p < 0:
                break
            found.append((p, name))
            p += 1
    found.sort()
    if take == 0:
        return [n for _, n in found], 0, []
    fixed = [n for p, n in found if p < at]
    if fixed_list is not None:
        choices = [n for n in fixed_list if n not in fixed]
    else:
        choices = [n for p, n in found if p >= after]
    return fixed, take, choices


def read_languages(text):
    if text is None:
        return 0
    flat = squeeze(text)
    for word, n in (("three", 3), ("two", 2)):
        if flat.startswith(word) or ("any" + word) in flat:
            return n
    return 1


GOLD = re.compile(r"containing\D{0,60}?(\d[\d,]*)\s*gp")
ANY_GOLD = re.compile(r"(\d[\d,]*)\s*gp")


def read_gold(text):
    m = GOLD.search(clean(text))
    if not m:
        found = ANY_GOLD.findall(clean(text))
        if not found:
            return None
        m = None
        return int(found[-1].replace(",", ""))
    return int(m.group(1).replace(",", ""))


# -------------------------------------------- the suggested characteristics


def tables(lines):
    """Every numbered suggested-characteristics table in a dump."""
    out, i = [], 0
    while i < len(lines):
        head = TABLE_HEAD.match(clean(lines[i]).strip())
        if not head:
            i += 1
            continue
        kind, entries, j = head.group(1), [], i + 1
        while j < len(lines):
            t = clean(lines[j]).strip()
            if t in ROLL:
                j += 1
                continue
            if not NUMBER.match(t):
                break
            n, text, j = int(t), [], j + 1
            while j < len(lines):
                u = clean(lines[j]).strip()
                if (not u or NUMBER.match(u) or TABLE_HEAD.match(u)
                        or u in ROLL or u.startswith("===")
                        or is_heading(u)):
                    break
                text.append(u)
                j += 1
            if text:
                entries.append((n, " ".join(text)))
        if entries:
            out.append({"kind": kind, "line": i + 1, "entries": entries})
        i = max(j, i + 1)
    return out


FULL = {"Personality Trait": 8, "Ideal": 6, "Bond": 6, "Flaw": 6}
KINDS = [("BGTRAIT", "Personality Trait"), ("BGIDEAL", "Ideal"),
         ("BGBOND", "Bond"), ("BGFLAW", "Flaw")]

MATCHED = 0.35        # below this, the row came from nothing in the table
CLEAN = 0.70          # above this, the paraphrase is close enough to leave

def assign(backgrounds, rows, kind, tabs):
    """Give each background the table of this kind its rows match best."""
    score = {}
    for bg in backgrounds:
        mine = rows.get((kind, bg), [])
        for t, tab in enumerate(tabs):
            score[(bg, t)] = sum(max(covered(d, e) for _, e in tab["entries"])
                                 for d in mine) / float(len(mine))
    taken, out = set(), {}
    for (bg, t), _ in sorted(score.items(), key=lambda kv: -kv[1]):
        if bg in out or t in taken:
            continue
        out[bg], _ = t, taken.add(t)
    return out, score


# ------------------------------------------------------------------- main


def main():
    for book, path in DUMPS.items():
        if not os.path.exists(path):
            sys.exit("verify_backgrounds: no %s" % path)

    lines = dict((b, open(p, encoding="utf-8").read().split("\n"))
                 for b, p in DUMPS.items())
    blocks = dict((b, stat_blocks(lines[b])) for b in DUMPS)

    rows = [r for r in read_file("character.txt") if r.tag == "BACKGROUND"]
    checked = bad = 0
    unchecked = []

    for book in ("PHB", "SCAG"):
        want = [r for r in rows if r.str(1) == book and "Variant" not in
                r.str(0)]
        if len(want) != len(blocks[book]):
            sys.exit("verify_backgrounds: %s has %d blocks for %d rows -- the "
                     "dump and data/ no longer line up"
                     % (book, len(blocks[book]), len(want)))

    variant_ok = any(INVESTIGATOR in clean(l) for l in lines["SCAG"])
    order = dict((b, 0) for b in DUMPS)

    print("backgrounds")
    for r in rows:
        book = r.str(1)
        name = r.str(0)
        is_variant = "Variant" in name
        if is_variant:
            block = blocks[book][order[book] - 1]
            if not variant_ok:
                unchecked.append((name, "the sentence giving the variant its "
                                        "skill is not in the dump"))
                continue
        else:
            block = blocks[book][order[book]]
            order[book] += 1
        f = block["fields"]
        problems = []
        checked += 1

        # Skills, and whether a choice was left open or quietly taken.
        fixed, take, choices = read_skills(f.get("Skill Proficiencies", ""))
        if is_variant:
            fixed = ["Investigation" if s == "Athletics" else s
                     for s in fixed]
        ours = [s for s in (r.str(2), r.str(3)) if s and s != "-"]
        if ours != fixed:
            problems.append("skills %s, book gives %s"
                            % (", ".join(ours) or "none",
                               ", ".join(fixed) or "none"))
        if r.int(4) != take:
            problems.append("%d skills chosen, book lets you choose %d"
                            % (r.int(4), take))
        theirs = [c for c in r.str(5).split("|") if c]
        if take and sorted(theirs) != sorted(choices):
            problems.append("chosen from %s,\n           book says %s"
                            % (", ".join(theirs) or "nothing",
                               ", ".join(choices)))

        # Tools: the row's wording is its own, so only what it names is read.
        tools, book_tools = r.str(6), f.get("Tool Proficiencies")
        if bool(tools) != bool(book_tools):
            problems.append("tools %r, book says %r"
                            % (tools, book_tools or ""))
        elif tools:
            miss = contained(tools, book_tools)
            if miss:
                problems.append("tools %r,\n           book does not mention "
                                "%s" % (tools, ", ".join(miss)))

        langs = read_languages(f.get("Languages"))
        if r.int(7) != langs:
            problems.append("%d extra languages, book gives %d"
                            % (r.int(7), langs))

        equip = f.get("Equipment", "")
        miss = unlisted(r.str(8), equip)
        if miss:
            problems.append("equipment lists %s,\n           which the book's "
                            "does not" % "; ".join(repr(m) for m in miss))
        gold = read_gold(equip)
        if gold is None:
            unchecked.append((name, "no starting gold in the equipment line"))
        elif gold != r.int(9):
            problems.append("%d gp to start, book says %d gp"
                            % (r.int(9), gold))

        feature = feature_name(lines[book], block)
        if feature is None:
            unchecked.append((name, "no feature heading in the dump"))
        elif squeeze(feature) != squeeze(r.str(10)):
            problems.append("feature %r, the book's heading reads %r with "
                            "its letter-spacing taken out"
                            % (r.str(10), squeeze(feature)))

        if problems:
            bad += 1
            print("  %s (%s)" % (name, book))
            for p in problems:
                print("      %s" % p)

    # ---- the suggested characteristics, all of which are in the PHB
    chars = {}
    for r in read_file("character.txt"):
        if r.tag in dict(KINDS):
            chars.setdefault((r.tag, r.str(0)), []).append(r.str(1))
    backgrounds = [r.str(0) for r in rows if r.str(1) == "PHB"]
    all_tables = tables(lines["PHB"])

    print("\nsuggested characteristics")
    loose = []
    for tag, kind in KINDS:
        tabs = [t for t in all_tables if t["kind"] == kind]
        if len(tabs) != len(backgrounds):
            unchecked.append((kind, "the dump has %d %s tables for %d "
                                    "backgrounds" % (len(tabs), kind,
                                                     len(backgrounds))))
            continue
        which, score = assign(backgrounds, chars, tag, tabs)
        weakest = min((score[(bg, which[bg])], bg) for bg in backgrounds)
        rival = max(max(v for (b, t), v in score.items()
                        if b == bg and t != which[bg]) for bg in backgrounds)
        print("  %-17s matched to their tables by content: the weakest fit "
              "is %.2f (%s), the strongest rival %.2f"
              % (kind.lower() + "s", weakest[0], weakest[1].lower(), rival))
        for bg in backgrounds:
            mine = score[(bg, which[bg])]
            best_rival = max(v for (b, t), v in score.items()
                             if b == bg and t != which[bg])
            if mine < 2 * best_rival:
                unchecked.append(("%s %s" % (bg, kind.lower()),
                                  "the best table for it has %d of the %d "
                                  "entries left and is not clearly this "
                                  "background's (%.2f against %.2f)"
                                  % (len(tabs[which[bg]]["entries"]),
                                     FULL[kind], mine, best_rival)))
                continue
            tab = tabs[which[bg]]
            full = len(tab["entries"]) == FULL[kind]
            names = [e.split(".")[0].strip() for _, e in tab["entries"]]
            for text in chars[(tag, bg)]:
                best, at, entry = max((covered(text, e), n, e)
                                      for n, e in tab["entries"])
                if best < MATCHED and not full:
                    unchecked.append(("%s %s" % (bg, kind.lower()),
                                      "the dump has only %d of the %d entries "
                                      "in this table, and this row matches "
                                      "none of them"
                                      % (len(tab["entries"]), FULL[kind])))
                    continue
                checked += 1
                if best < MATCHED:
                    bad += 1
                    print("  %s, %s" % (bg, kind.lower()))
                    print("      %r" % text)
                    print("      is none of the %d in the book's table "
                          "(%s:%d)" % (len(tab["entries"]),
                                       os.path.basename(DUMPS["PHB"]),
                                       tab["line"]))
                    continue
                if best < CLEAN:
                    loose.append((bg, kind, text, at, entry))
                if tag != "BGIDEAL":
                    continue
                # An ideal is named, and the book brackets an alignment.
                label = text.split(":")[0].strip() if ":" in text else ""
                if label and not any(same_word(label.lower(), n.lower())
                                     for n in names):
                    bad += 1
                    print("  %s ideal %r is named for no ideal in the "
                          "book's table" % (bg, label))
                m = re.search(r"\(([A-Za-z]+)\)\s*$", text)
                if m:
                    b = re.search(r"\(([A-Za-z]+)\)", entry)
                    if b and m.group(1) != b.group(1):
                        bad += 1
                        print("  %s ideal %r alignment %s, book says %s"
                              % (bg, label, m.group(1), b.group(1)))

    if loose:
        print("\n  %d rows are paraphrased far enough from the entry they "
              "came from that only a person can say whether the rewrite is "
              "fair. They are not counted as disagreeing:" % len(loose))
        for bg, kind, text, at, entry in loose:
            print("      %s %s: %r" % (bg, kind.lower(), text))
            print("          book %d. %r" % (at, entry))

    if unchecked:
        print("\n  %d left unchecked:" % len(unchecked))
        for what, why in unchecked:
            print("      %s -- %s" % (what, why))

    print("\n%d checked, %d disagree" % (checked, bad))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
