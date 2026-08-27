#!/usr/bin/env python3
"""Check the Handbook and Sword Coast race rows in data/character.txt.

tools/verify_races.py reads the thirty-three MPMM races out of an OCR dump
and can only manage size, speed and darkvision. The nine Player's Handbook
races, the thirty SUBRACE rows and the ten draconic ancestries come from text
layers that were extracted rather than scanned, so this compares much more:
all six ability score increases, walking speed, size, darkvision radius, the
languages granted by name, how many further languages the race lets you
choose, how many skills it lets you choose, and -- for a subrace -- the
increases and the named traits it lays on top of its parent race. The ten
ANCESTRY rows are read straight out of the Draconic Ancestry table.

Both books set a trait as "<Name>. <sentence>", and every pattern here is
anchored on that shape rather than on loose substring matching, because the
prose around the traits is full of the same words: the drow sidebar sits in
the middle of the wood elf's traits, and the half-orc's opening page sits in
the middle of the half-elf's.

Four things the extraction does that this has to work around.

Headings are letterspaced -- "H a l f l i n g T r a i t s" -- and a couple
lose the spacing in odd places ("St o u t", "H a l f -E lf"), so a heading is
recognised by its own line with the whitespace taken out rather than by a
substring. That also keeps the index at the back of the book, which carries
"halfling traits, 28", from being mistaken for the heading.

Words are set with a space inside them: "Com m on", "W isdom", "base w alking
speed". Every literal that has to be read back is matched with the spacing
made optional, and a language is recognised by name from the LANGUAGE rows
rather than by splitting the sentence on commas.

The two-column pages interleave. The halfling's Languages trait ends up on
the page before its own traits block, so when a race's Languages sentence is
not inside its traits section the search widens to the run of pages from the
end of the previous race, and is used only if it turns up exactly one
sentence. The duergar's block in the Sword Coast guide is cut in half the
same way, with the tail of Duergar Magic and the whole of Sunlight
Sensitivity landing a page later, so that block deliberately runs on to the
heading after the one that interrupts it.

Sword Coast digits are the one place the extraction is not clean: "increases
by l." for "by 1." twice, and the heading "HALF-ORGS" for "HALF-ORCS". Both
are normalised. So is the Draconic Ancestry table's sixth row, whose dragon
arrives as "Cold" -- the row sits between Copper and Green, and reads Fire /
15 ft. cone (Dex. save), so it is the gold dragon's; its damage and breath
are compared, its name is not.

Left unchecked on purpose:

  * Standard Human and Variant Human. The Handbook has no block for the
    first -- it is the human as printed -- and the dump has lost the body of
    the Variant Human Traits sidebar, keeping only its opening sentence, so
    the two increases, the skill and the feat cannot be read back.
  * Traits are compared one way only: every trait a subrace row claims must
    be a named trait in the book, but a trait the book grants and the row
    omits is not reported. The options inside the rock gnome's Tinker
    ("Fire Starter.", "Music Box.") are set exactly like trait names, and
    telling them apart from real traits is guesswork.
  * The half-elf variants of the Sword Coast guide are a bulleted sidebar
    rather than a block of traits, so what is compared for them is what the
    sidebar states: that each is taken in place of Skill Versatility, that
    none of them touches an ability score, and that the option the row names
    is one the sidebar offers. Fleet of Foot's speed is the wood elf's, read
    from the Handbook.
  * "Mark of Asmodeus" is this project's name for the unmodified Handbook
    tiefling; the guide names no such trait. Its numbers are checked against
    the guide's statement that Faerunian tieflings have the Handbook's
    racial traits, and its name is not checked.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

from build_data import read_file      # noqa: E402

PHB = os.path.join(ROOT, "TextFiles", "PHBtext.txt")
SCAG = os.path.join(ROOT, "TextFiles", "SCAGtext.txt")

ABILITIES = ["STR", "DEX", "CON", "INT", "WIS", "CHA"]
ABILITY_AT = {"strength": 0, "dexterity": 1, "constitution": 2,
              "intelligence": 3, "wisdom": 4, "charisma": 5}
COUNT = {"one": 1, "two": 2, "three": 3, "four": 4}

# RACE columns.
R_NAME, R_BOOK = 0, 1
R_ASI = 2                       # ... through 7
R_SPEED, R_SIZE, R_DARK, R_LANGS = 8, 9, 10, 11
R_XLANG, R_XSKILL, R_CHOICE_N, R_CHOICE_AMT = 12, 13, 14, 15

# SUBRACE columns.
S_RACE, S_NAME, S_BOOK = 0, 1, 2
S_ASI = 3                       # ... through 8
S_SPEED, S_DARK, S_XLANG, S_CHOICE_N = 9, 10, 11, 12
S_XSKILL, S_REPLACES, S_TRAITS = 13, 15, 16     # 14 is bonus_feats


# ----------------------------------------------------------------- the text


def spaced(literal):
    """A pattern for a literal the extraction may have put spaces inside.

    "Com m on", "W isdom", "Artificer' s Lore": the space can fall anywhere,
    so every gap between two characters is made optional.
    """
    out = []
    for ch in literal:
        if ch == " ":
            out.append(r"\s+")
        elif ch == "'":
            out.append(r"\s*['‘’]\s*")
        else:
            out.append(re.escape(ch) + r"\s*")
    return "".join(out)


def flatten(text):
    """One long line: page markers gone, line-break hyphenation undone."""
    text = text.replace("’", "'").replace("‘", "'")
    text = re.sub(r"=== page \d+ ===", " ", text)
    text = re.sub(r"([a-z])-\s*\n\s*([a-z])", r"\1\2", text)
    text = re.sub(r"increases by [lI](?![A-Za-z])", "increases by 1", text)
    return re.sub(r"\s+", " ", text).strip()


def squash(s):
    return re.sub(r"\s+", "", s).lower().replace("’", "'")


# The headings that bound a race's or a subrace's block in the Handbook.
# "Draconic Ancestry" is deliberately absent: the dragonborn's Languages
# trait sits on the far side of that table and of the draconians sidebar.
PHB_HEADINGS = {
    "chapter2:races", "chapter3:classes",
    "dwarf", "elf", "halfling", "human", "dragonborn", "gnome",
    "half-elf", "half-orc", "tiefling",
    "dwarftraits", "elftraits", "halflingtraits", "humantraits",
    "dragonborntraits", "gnometraits", "half-elftraits", "half-orctraits",
    "tieflingtraits", "varianthumantraits",
    "hilldwarf", "mountaindwarf", "duergar", "highelf", "woodelf",
    "darkelf(drow)", "lightfoot", "stout", "forestgnome", "rockgnome",
}


class Handbook:
    """Chapter 2 of the Player's Handbook, cut into blocks by its headings."""

    def __init__(self, path):
        self.lines = open(path, encoding="utf-8").read().split("\n")
        heads = []
        for n, line in enumerate(self.lines):
            s = line.strip()
            if not s or " " not in s:
                continue
            key = squash(s)
            if key in PHB_HEADINGS:
                heads.append((n, key))
        first = [n for n, k in heads if k == "chapter2:races"]
        last = [n for n, k in heads if k == "chapter3:classes"]
        if not first or not last:
            self.heads = []
            return
        self.heads = [(n, k) for n, k in heads if first[0] <= n <= last[0]]
        self.at = [n for n, _ in self.heads]

    def find(self, key, low=0, high=None):
        """Where a heading sits, when it sits in the span asked for once."""
        high = len(self.lines) if high is None else high
        hits = [n for n, k in self.heads if k == key and low <= n < high]
        return hits[0] if len(hits) == 1 else None

    def after(self, n):
        later = [p for p in self.at if p > n]
        return later[0] if later else len(self.lines)

    def block(self, n):
        return flatten("\n".join(self.lines[n:self.after(n)]))

    def span(self, low, high):
        return flatten("\n".join(self.lines[low:high]))


def scag_block(lines, opening, closing):
    """A run of the Sword Coast guide between two of its capitalised heads."""
    a = b = None
    for n, line in enumerate(lines):
        s = squash(line.strip())
        if a is None and s == squash(opening):
            a = n
        elif a is not None and s == squash(closing):
            b = n
            break
    if a is None or b is None:
        return None
    return flatten("\n".join(lines[a:b]))


# ------------------------------------------------------- reading the traits


# A trait's name: up to four words, the first and last capitalised, with the
# small joining words a name like "Speak with Small Beasts" needs.
JOIN = r"(?:of|the|with|and|to|in|from|a)"
TRAIT_NAME = (r"[A-Z][A-Za-z']*(?:\s+(?:" + JOIN + r"|[A-Z][A-Za-z']*)){0,3}")
NEXT_TRAIT = re.compile(r"\.\s+" + TRAIT_NAME + r"\.\s+[A-Z]")

ASI_HEAD = re.compile(r"Ability\s+Score\s+Increase\.")
ASI_ONE = re.compile(r"[Yy]our\s+([A-Za-z ]{3,20}?)\s+score\s+increases\s+"
                     r"by\s+(\d)")
ASI_ALL = re.compile(r"ability\s+scores\s+each\s+increase\s+by\s+(\d)")
ASI_CHOICE = re.compile(r"(one|two|three)\s+other\s+ability\s+scores?\s+"
                        r"of\s+your\s+choice\s+increases?\s+by\s+(\d)")

SIZE = re.compile(r"[Yy]our\s+size\s+is\s+([A-Z][a-z]+)")
SPEED = re.compile(r"base\s+w\s*alking\s+speed\s+(?:is|increases\s+to)\s+"
                   r"(\d+)\s+feet")
SUPERIOR = re.compile(r"Superior\s+Darkvision\.")
SUPERIOR_R = re.compile(r"Superior\s+Darkvision\.[^.]{0,140}?radius\s+of\s+"
                        r"(\d+)\s+feet")
PLAIN_DARK = re.compile(r"(?<!Superior )Darkvision\.\s+[A-Z]")
# The radius is the first number after the trait's name, not the number
# before the word "feet": the sentence that carries it is two sentences
# further on ("...dim conditions. You can see in dim light within 60 feet").
PLAIN_DARK_R = re.compile(r"(?<!Superior )Darkvision\.[^0-9]{0,300}?"
                          r"(\d+)\s+feet")
LANG_CLAUSE = re.compile(r"Languages\.\s+You\s+can\s+speak,\s+read,\s+and\s+"
                         r"write\s+([^.]{0,160})")
XLANG_CLAUSE = re.compile(r"Extra\s+Language\.\s+You\s+can\s+speak,\s+read,"
                          r"\s+and\s+write\s+([^.]{0,160})")
XLANG_CHOICE = re.compile(r"(one|two|three)\s+extra\s+languages?\s+of\s+"
                          r"your\s+choice")
XSKILL = re.compile(r"proficiency\s+in\s+(one|two|three)\s+skills?\s+of\s+"
                    r"your\s+choice")


def only(values):
    """The one value a block gives, or None when it gives none or several."""
    seen = list(dict.fromkeys(values))
    return seen[0] if len(seen) == 1 else None


def asi_clause(block):
    """The Ability Score Increase sentence, cut at the next trait's name."""
    m = ASI_HEAD.search(block)
    if not m:
        return None
    rest = block[m.end():m.end() + 320]
    stop = NEXT_TRAIT.search(rest)
    return rest[:stop.start()] if stop else rest


def increases(block):
    """The six increases and any free choice, or None when unreadable."""
    clause = asi_clause(block)
    if clause is None:
        return None
    got = [0] * 6
    m = ASI_ALL.search(clause)
    if m:
        got = [int(m.group(1))] * 6
    for name, amount in ASI_ONE.findall(clause):
        at = ABILITY_AT.get(squash(name))
        if at is None:
            return None
        got[at] = int(amount)
    m = ASI_CHOICE.search(clause)
    choice = (COUNT[m.group(1)], int(m.group(2))) if m else (0, 0)
    return got, choice


def darkvision(block):
    """The radius in feet; 0 when the block has no darkvision trait."""
    if SUPERIOR.search(block):
        return only([int(v) for v in SUPERIOR_R.findall(block)])
    if PLAIN_DARK.search(block):
        return only([int(v) for v in PLAIN_DARK_R.findall(block)])
    return 0


def languages(clause, known):
    """The languages a Languages sentence names, and its choices."""
    found = []
    for name in known:
        pat = r"(?<![A-Za-z])" + spaced(name) + r"(?![A-Za-z])"
        if re.search(pat, clause):
            found.append(name)
    m = XLANG_CHOICE.search(clause)
    return found, (COUNT[m.group(1)] if m else 0)


def trait_names(field):
    """The names a data row's traits column gives, in order."""
    out = []
    for part in field.split("|"):
        part = part.strip()
        if part:
            out.append(part.split(":")[0].strip())
    return out


def names_trait(block, name):
    """Whether the book sets `name` as a trait's name in this block."""
    return re.search(r"(?<![A-Za-z])" + spaced(name) + r"\.\s", block) \
        is not None


# --------------------------------------------------------------- the checks


class Report:
    def __init__(self):
        self.checked = 0
        self.bad = 0
        self.skipped = []

    def entry(self, what, problems):
        self.checked += 1
        if problems:
            self.bad += 1
            print("  %s" % what)
            for p in problems:
                print("      %s" % p)

    def skip(self, what, why):
        self.skipped.append((what, why))


def compare_increases(got, row, base, problems):
    for n, ability in enumerate(ABILITIES):
        if got[n] != row.int(base + n):
            problems.append("%s +%d, book says +%d"
                            % (ability, row.int(base + n), got[n]))


def check_races(hb, rows, known, report):
    """The nine Handbook races."""
    order = ["Dwarf", "Elf", "Halfling", "Human", "Dragonborn", "Gnome",
             "Half-Elf", "Half-Orc", "Tiefling"]
    heads = {}
    for name in order:
        heads[name] = hb.find(squash(name) + "traits")

    for row in rows:
        name = row.str(R_NAME)
        at = heads.get(name)
        if at is None:
            report.skip(name, "no traits heading in the Handbook")
            continue
        block = hb.block(at)
        problems = []

        got = increases(block)
        if got is None:
            report.skip(name, "its Ability Score Increase sentence is "
                              "not readable")
            continue
        compare_increases(got[0], row, R_ASI, problems)
        if got[1][0] != row.int(R_CHOICE_N) or got[1][1] != row.int(
                R_CHOICE_AMT):
            problems.append("%d free increases of +%d, book says %d of +%d"
                            % (row.int(R_CHOICE_N), row.int(R_CHOICE_AMT),
                               got[1][0], got[1][1]))

        size = only(SIZE.findall(block))
        if size is None:
            problems.append("size %s, the book's sentence is not readable"
                            % row.str(R_SIZE))
        elif size != row.str(R_SIZE):
            problems.append("size %s, book says %s" % (row.str(R_SIZE), size))

        speed = only([int(v) for v in SPEED.findall(block)])
        if speed is None:
            problems.append("speed %d, the book's sentence is not readable"
                            % row.int(R_SPEED))
        elif speed != row.int(R_SPEED):
            problems.append("speed %d, book says %d" % (row.int(R_SPEED),
                                                        speed))

        dark = darkvision(block)
        if dark is None:
            problems.append("darkvision %d, the book's sentence is not "
                            "readable" % row.int(R_DARK))
        elif dark != row.int(R_DARK):
            problems.append("darkvision %d, book says %d"
                            % (row.int(R_DARK), dark))

        # The halfling's Languages trait is carried on to the previous page
        # by the column flow, so widen to the run of pages this race owns.
        clause = only(LANG_CLAUSE.findall(block))
        if clause is None:
            n = order.index(name)
            before = heads.get(order[n - 1]) if n else None
            after = heads.get(order[n + 1]) if n + 1 < len(order) else None
            low = hb.after(before) if before is not None else hb.at[0]
            high = after if after is not None else hb.after(at)
            clause = only(LANG_CLAUSE.findall(hb.span(low, high)))
        if clause is None:
            problems.append("languages %r, the book's sentence is not "
                            "readable" % row.str(R_LANGS))
        else:
            spoken, extra = languages(clause, known)
            want = [s.strip() for s in row.str(R_LANGS).split(",")]
            if sorted(spoken) != sorted(want):
                problems.append("languages %r, book says %r"
                                % (row.str(R_LANGS), ", ".join(spoken)))
            if extra != row.int(R_XLANG):
                problems.append("%d language choices, book says %d"
                                % (row.int(R_XLANG), extra))

        m = XSKILL.search(block)
        skills = COUNT[m.group(1)] if m else 0
        if skills != row.int(R_XSKILL):
            problems.append("%d skill choices, book says %d"
                            % (row.int(R_XSKILL), skills))

        report.entry(name, problems)


# Where each Handbook subrace's block is, and the race whose span it sits in.
PHB_SUBRACES = {
    "Hill Dwarf": ("hilldwarf", "Dwarf"),
    "Mountain Dwarf": ("mountaindwarf", "Dwarf"),
    "High Elf": ("highelf", "Elf"),
    "Wood Elf": ("woodelf", "Elf"),
    "Dark Elf (Drow)": ("darkelf(drow)", "Elf"),
    "Lightfoot Halfling": ("lightfoot", "Halfling"),
    "Stout Halfling": ("stout", "Halfling"),
    "Forest Gnome": ("forestgnome", "Gnome"),
    "Rock Gnome": ("rockgnome", "Gnome"),
}

# The two the dump cannot settle.
PHB_UNCHECKED = {
    "Standard Human": "the Handbook has no such block -- it is the human "
                      "as printed, and the row adds nothing",
    "Variant Human": "the dump keeps only the opening sentence of the "
                     "Variant Human Traits sidebar; its increases, skill "
                     "and feat are gone",
}


def subrace_block(hb, key, race):
    """A subrace's block, looked for only inside its own race's pages.

    "Stout" is a heading twice: once for the halfling subrace and once for
    the "Short and Stout" spread that opens the chapter.
    """
    order = ["Dwarf", "Elf", "Halfling", "Human", "Dragonborn", "Gnome",
             "Half-Elf", "Half-Orc", "Tiefling"]
    n = order.index(race)
    low = hb.find(squash(order[n - 1]) + "traits") if n else 0
    high = hb.find(squash(order[n + 1]) + "traits") if n + 1 < len(order) \
        else len(hb.lines)
    if low is None or high is None:
        return None
    at = hb.find(key, low, high)
    return None if at is None else hb.block(at)


def check_subrace(name, block, row, report, speed_hint=None):
    """The increases, overrides and named traits a subrace block gives."""
    problems = []
    got = increases(block)
    if got is None:
        got = ([0] * 6, (0, 0))     # a block with no increase grants none
    compare_increases(got[0], row, S_ASI, problems)
    if got[1][0] != row.int(S_CHOICE_N):
        problems.append("%d free increases, book says %d"
                        % (row.int(S_CHOICE_N), got[1][0]))

    speed = only([int(v) for v in SPEED.findall(block)])
    if speed is None:
        speed = speed_hint if speed_hint is not None else 0
    if speed != row.int(S_SPEED):
        problems.append("speed override %d, book says %d"
                        % (row.int(S_SPEED), speed))

    dark = darkvision(block)
    if dark is None:
        problems.append("darkvision override %d, the book's sentence is "
                        "not readable" % row.int(S_DARK))
    elif dark != row.int(S_DARK):
        problems.append("darkvision override %d, book says %d"
                        % (row.int(S_DARK), dark))

    extra = 0
    for clause in XLANG_CLAUSE.findall(block):
        m = XLANG_CHOICE.search(clause)
        if m:
            extra += COUNT[m.group(1)]
    if extra != row.int(S_XLANG):
        problems.append("%d language choices, book says %d"
                        % (row.int(S_XLANG), extra))

    m = XSKILL.search(block)
    skills = COUNT[m.group(1)] if m else 0
    if skills != row.int(S_XSKILL):
        problems.append("%d skill choices, book says %d"
                        % (row.int(S_XSKILL), skills))

    for trait in trait_names(row.str(S_TRAITS)):
        if not names_trait(block, trait):
            problems.append("claims a trait the book does not name: %s"
                            % trait)

    report.entry(name, problems)


def check_phb_subraces(hb, rows, report):
    wood = subrace_block(hb, "woodelf", "Elf")
    for row in rows:
        name = row.str(S_NAME)
        if name in PHB_UNCHECKED:
            report.skip(name, PHB_UNCHECKED[name])
            continue
        where = PHB_SUBRACES.get(name)
        if where is None:
            report.skip(name, "no block of that name in the Handbook")
            continue
        block = subrace_block(hb, where[0], where[1])
        if block is None:
            report.skip(name, "its heading is not in the dump")
            continue
        check_subrace(name, block, row, report)
    return wood


# ------------------------------------------------- the Sword Coast subraces


# "HALF-ORGS" is how the extraction reads the HALF-ORCS heading.
SCAG_BLOCKS = {
    "Duergar (Gray Dwarf)": ("DUERGAR SUBRACE TRAITS", "DWARVEN DEITIES"),
    "Ghostwise Halfling": ("GHOSTWISE HALFLINGS", "KEEPERS OF THE HOME"),
    "Svirfneblin (Deep Gnome)": ("SVIRFNEBLIN SUBRACE TRAITS",
                                 "GNOMISH DEITIES"),
}
HALF_ELF_SIDEBAR = ("HALF-ELF VARIANTS", "HALF-ORGS")
TIEFLING_SIDEBAR = ("TIEFLING VARIANTS", "AASIMAR")
ASMODEUS = ("THE MARK OF ASMODEUS", "FIRST FAMILY")

REPLACES_VERSATILITY = re.compile(
    r"in\s+place\s+of\s+the\s+Skill\s+Versatility\s+trait")
REPLACES_ASI = re.compile(
    r"replaces\s+the\s+Ability\s+Score\s+Increase\s+trait")
AS_PRINTED = re.compile(r"racial\s+traits\s+of\s+tieflings\s+in\s+the\s+"
                        r"Player's\s+Handbook")


def check_scag_subraces(lines, rows, report, wood_speed):
    sidebar = scag_block(lines, *HALF_ELF_SIDEBAR)
    variants = scag_block(lines, *TIEFLING_SIDEBAR)
    asmodeus = scag_block(lines, *ASMODEUS)

    # The tiefling sidebar is a run of named traits; cut it into them.
    pieces = {}
    if variants:
        marks = [(m.start(), m.group(1)) for m in
                 re.finditer(r"(?:^|(?<=\.\s))(" + TRAIT_NAME + r")\.\s+[A-Z]",
                             variants)]
        for n, (at, label) in enumerate(marks):
            end = marks[n + 1][0] if n + 1 < len(marks) else len(variants)
            pieces.setdefault(label, variants[at:end])

    for row in rows:
        name = row.str(S_NAME)
        race = row.str(S_RACE)

        if name in SCAG_BLOCKS:
            block = scag_block(lines, *SCAG_BLOCKS[name])
            if block is None:
                report.skip(name, "its block is not in the guide's dump")
            else:
                check_subrace(name, block, row, report)
            continue

        if race == "Half-Elf":
            if sidebar is None or not REPLACES_VERSATILITY.search(sidebar):
                report.skip(name, "the half-elf variants sidebar is not "
                                  "in the guide's dump")
                continue
            problems = []
            if ASI_ONE.search(sidebar) or ASI_HEAD.search(sidebar):
                report.skip(name, "the sidebar mentions an ability score "
                                  "increase, which it should not")
                continue
            compare_increases([0] * 6, row, S_ASI, problems)
            want_skills = 0 if name == "Skill Versatility" else -2
            if row.int(S_XSKILL) != want_skills:
                problems.append("%d skill choices, the sidebar has this "
                                "taken in place of Skill Versatility, so "
                                "%d" % (row.int(S_XSKILL), want_skills))
            want_speed = wood_speed if name == "Fleet of Foot" else 0
            if row.int(S_SPEED) != want_speed:
                problems.append("speed override %d, book says %d"
                                % (row.int(S_SPEED), want_speed))
            if row.int(S_DARK) != 0:
                problems.append("darkvision override %d, the sidebar "
                                "grants none" % row.int(S_DARK))
            if not re.search(spaced(name), sidebar, re.I):
                problems.append("the sidebar does not offer: %s" % name)
            report.entry(name, problems)
            continue

        if race == "Tiefling":
            if name == "Mark of Asmodeus":
                if asmodeus is None or not AS_PRINTED.search(asmodeus):
                    report.skip(name, "the guide's paragraph on Faerunian "
                                      "tieflings is not in the dump")
                    continue
                problems = []
                compare_increases([0] * 6, row, S_ASI, problems)
                for at, what in ((S_SPEED, "speed override"),
                                 (S_DARK, "darkvision override"),
                                 (S_XLANG, "language choices"),
                                 (S_XSKILL, "skill choices"),
                                 (S_REPLACES, "replaced race increases")):
                    if row.int(at) != 0:
                        problems.append("%s %d, the guide has these "
                                        "tieflings with the Handbook's "
                                        "traits unchanged"
                                        % (what, row.int(at)))
                report.entry(name, problems)
                continue

            wanted = trait_names(row.str(S_TRAITS))
            missing = [t for t in wanted if t not in pieces]
            if missing:
                report.skip(name, "the variants sidebar does not name %s"
                                  % ", ".join(missing))
                continue
            # A variant states its increase inside its own trait rather than
            # under an Ability Score Increase heading, and only Feral has
            # one; the rows that pair Feral with another variant get both.
            got, replaces, unreadable = [0] * 6, 0, []
            for trait in wanted:
                piece = pieces[trait]
                for ability, amount in ASI_ONE.findall(piece):
                    at = ABILITY_AT.get(squash(ability))
                    if at is None:
                        unreadable.append(trait)
                    else:
                        got[at] = int(amount)
                if REPLACES_ASI.search(piece):
                    replaces = 1
            if unreadable:
                report.skip(name, "an increase %s grants names no ability "
                                  "this recognises"
                                  % ", ".join(sorted(set(unreadable))))
                continue
            problems = []
            compare_increases(got, row, S_ASI, problems)
            if row.int(S_REPLACES) != replaces:
                problems.append("replaces the race's increases: %d, book "
                                "says %d" % (row.int(S_REPLACES), replaces))
            # A flying speed is not a walking speed, so nothing overrides it.
            if row.int(S_SPEED) != 0:
                problems.append("speed override %d, the sidebar changes no "
                                "walking speed" % row.int(S_SPEED))
            if row.int(S_DARK) != 0:
                problems.append("darkvision override %d, the sidebar grants "
                                "none" % row.int(S_DARK))
            report.entry(name, problems)
            continue

        report.skip(name, "no block for it in the guide")


# -------------------------------------------------------- draconic ancestry


BREATH = re.compile(r"^(?:\d+ by \d+ ft\. line|\d+ ft\. cone) "
                    r"\((?:Dex|Con)\. save\)$")

# The sixth row's dragon comes through as "Cold". It sits between Copper and
# Green and reads Fire / 15 ft. cone (Dex. save), so it is the gold dragon's.
MISREAD = {"Cold": "Gold"}


def ancestry_table(hb):
    """The Draconic Ancestry table, as (dragon, damage, breath) rows."""
    at = hb.find("dragonborntraits")
    if at is None:
        return {}
    lines = [l.strip() for l in hb.lines[at:hb.after(at)]]
    lines = [l for l in lines if l and not l.startswith("=== page")]
    rows, order = {}, []
    for n, line in enumerate(lines):
        if not BREATH.match(line) or n < 2:
            continue
        dragon, damage = lines[n - 2].strip(), lines[n - 1].strip()
        if not re.match(r"^[A-Z][a-z]+$", dragon) or \
                not re.match(r"^[A-Z][a-z]+$", damage):
            continue
        order.append(dragon)
        rows[MISREAD.get(dragon, dragon)] = (damage, line)
    # The misread only stands where the row keeps its alphabetical place.
    for wrong, right in MISREAD.items():
        if wrong in order:
            n = order.index(wrong)
            fixed = [MISREAD.get(d, d) for d in order]
            if fixed != sorted(fixed) or fixed[n] != right:
                rows.pop(right, None)
    return rows


def check_ancestries(hb, rows, report):
    table = ancestry_table(hb)
    if len(table) != 10:
        for row in rows:
            report.skip(row.str(0), "the Draconic Ancestry table did not "
                                    "come out of the dump whole")
        return
    for row in rows:
        want = table.get(row.str(0))
        if want is None:
            report.skip(row.str(0), "not a row of the Draconic Ancestry "
                                    "table")
            continue
        problems = []
        if want[0] != row.str(1):
            problems.append("damage %s, book says %s" % (row.str(1), want[0]))
        if want[1] != row.str(2):
            problems.append("breath %s, book says %s" % (row.str(2), want[1]))
        report.entry(row.str(0), problems)


# --------------------------------------------------------------------- main


def main():
    for path in (PHB, SCAG):
        if not os.path.exists(path):
            sys.exit("verify_races_phb: no %s" % path)

    hb = Handbook(PHB)
    if not hb.heads:
        sys.exit("verify_races_phb: chapter 2 is not in the Handbook's dump")
    scag = open(SCAG, encoding="utf-8").read().split("\n")

    recs = read_file("character.txt")
    known = [r.str(0) for r in recs if r.tag == "LANGUAGE"]
    races = [r for r in recs if r.tag == "RACE" and r.str(R_BOOK) == "PHB"]
    subs = [r for r in recs if r.tag == "SUBRACE"]
    ancestries = [r for r in recs if r.tag == "ANCESTRY"]

    report = Report()
    check_races(hb, races, known, report)
    wood = check_phb_subraces(
        hb, [r for r in subs if r.str(S_BOOK) == "PHB"], report)
    wood_speed = only([int(v) for v in SPEED.findall(wood)]) if wood else 0
    check_scag_subraces(
        scag, [r for r in subs if r.str(S_BOOK) == "SCAG"], report,
        wood_speed or 0)
    check_ancestries(hb, ancestries, report)

    if report.skipped:
        print("\n  %d left unchecked:" % len(report.skipped))
        for what, why in report.skipped:
            print("      %s -- %s" % (what, why))

    print("\n%d checked, %d disagree" % (report.checked, report.bad))
    return 1 if report.bad else 0


if __name__ == "__main__":
    sys.exit(main())
