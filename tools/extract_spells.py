#!/usr/bin/env python3
"""Extract PHB spell data from the OCR'd text dump into a C source table.

Reads TextFiles/PHBtext.txt and emits src/data_spells.c / src/data_spells.h
containing, for every PHB spell: name, level, school, ritual flag, casting
time, range, components, duration, concentration flag, and the bitmask of
classes whose spell list includes it.

Only mechanical stat-block fields are extracted -- never the descriptive
prose of a spell.
"""

import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
SRC = os.path.join(ROOT, "TextFiles", "PHBtext.txt")
SRC_XGE = os.path.join(ROOT, "TextFiles", "XANATHARtext.txt")
SRC_TCE = os.path.join(ROOT, "TextFiles", "TASHAtext.txt")

DESC_START, DESC_END = 31590, 43119

# Xanathar's Guide: the class spell lists, then the spell descriptions.
XGE_LISTS = (25690, 26344)
XGE_DESC = (26344, 30025)

# Tasha's Cauldron: a single summary table (spell rows, then class rows in
# the same order), then the spell descriptions.
TCE_TABLE = (12530, 12617)
TCE_DESC = (12617, 14097)

SCHOOLS = [
    "abjuration", "conjuration", "divination", "enchantment",
    "evocation", "illusion", "necromancy", "transmutation",
]

CLASSES = ["bard", "cleric", "druid", "paladin", "ranger", "sorcerer",
           "warlock", "wizard", "artificer"]

# Classes that appear in the PHB's own spell-list chapter; the artificer's
# list comes from Tasha's instead.
PHB_CLASSES = CLASSES[:8]

# The class spell lists sit between chapter 10 and the spell descriptions.
# Their headings are located by name rather than by hardcoded line numbers:
# the OCR mangles some of them (the sorcerer's reads "Sorc erer Sp ells") and
# a wrong boundary silently moves one class's spells onto another's list.
LISTS_REGION = (30100, DESC_START)

# Cantrip counts each PHB class list must yield. A mismatch means a section
# boundary is wrong, which is exactly the failure this guards against.
EXPECTED_CANTRIPS = {
    "bard": 11, "cleric": 7, "druid": 8, "paladin": 0, "ranger": 0,
    "sorcerer": 16, "warlock": 9, "wizard": 16,
}


def clean(s):
    """Normalise OCR noise in a line of text."""
    s = s.replace("’", "'").replace("‘", "'")
    s = s.replace("“", '"').replace("”", '"')
    s = s.replace("—", "-").replace("–", "-")
    s = s.replace("ﬁ", "fi").replace("ﬂ", "fl")
    return s.strip()


def strip_lead(s):
    """Drop stray leading punctuation the OCR sprinkles before list items."""
    return re.sub(r"^[.,'`\"•*\-]+\s*", "", s).strip()


def load_lines(path=SRC):
    with open(path, encoding="utf-8", errors="replace") as fh:
        return [clean(l) for l in fh.readlines()]


# --------------------------------------------------------------------------
# level / school line
# --------------------------------------------------------------------------

ORDINAL = {
    "1st": 1, "2nd": 2, "3rd": 3, "4th": 4, "5th": 5,
    "6th": 6, "7th": 7, "8th": 8, "9th": 9,
}

# OCR routinely renders "1st-level" as "Ist-Ievel" / "lst-level" etc.
OCR_ORD = {
    "ist": 1, "lst": 1, "1st": 1, "l st": 1,
    "2nd": 2, "2 nd": 2, "znd": 2,
    "3rd": 3, "3 rd": 3,
    "4th": 4, "5th": 5, "6th": 6, "7th": 7, "8th": 8, "9th": 9,
}


def parse_level_school(line):
    """Return (level, school, ritual) or None if this isn't a level/school line."""
    low = line.lower()
    school = None
    for s in SCHOOLS:
        if s in low:
            school = s
            break
    if school is None:
        return None

    ritual = "ritual" in low
    if "cantrip" in low:
        return 0, school, ritual

    # "<ordinal>-level <school>"
    m = re.search(r"([0-9a-z]{1,4})\s*[-–—]\s*[il1]?evel", low)
    if m:
        key = m.group(1).strip()
        if key in OCR_ORD:
            return OCR_ORD[key], school, ritual
        d = re.match(r"([1-9])", key)
        if d:
            return int(d.group(1)), school, ritual
    m = re.search(r"\b([1-9])(?:st|nd|rd|th)\b", low)
    if m:
        return int(m.group(1)), school, ritual
    # OCR renders 3, 5 and 8 all as "S" ("Sth-level"); the digit is not
    # recoverable from the glyph, so report the level as unknown and let the
    # class spell lists (which state levels explicitly) resolve it.
    if re.search(r"\bs\s*(?:th|rd|nd|st)\s*[-\u2013\u2014]\s*[il1]?evel", low):
        return None, school, ritual
    return None


NOISE = re.compile(
    r"^(part\s|chapter\s|appendix\s|spell descriptions|spells\b|_|\^|~|\||[0-9\s.,'`^~|_-]*$)",
    re.I,
)


def is_noise(line):
    if not line:
        return True
    if NOISE.match(line):
        return True
    # OCR page furniture: mostly non-alphabetic soup
    letters = sum(c.isalpha() for c in line)
    return letters < 3


def name_looks_sane(name):
    if not name or len(name) > 48:
        return False
    if not re.match(r"^[A-Za-z]", name):
        return False
    # Reject running headers that slipped through.
    if re.search(r"^(spell|chapter|part|appendix)\b", name, re.I):
        return False
    letters = sum(c.isalpha() for c in name)
    return letters >= 3 and letters / len(name) > 0.55


def field_after(lines, i, label, limit=16):
    """Find 'Label: value' at or after line i, joining wrapped continuations."""
    pat = re.compile(r"^" + label + r"\s*[:;.]\s*(.*)$", re.I)
    for j in range(i, min(i + limit, len(lines))):
        m = pat.match(strip_lead(lines[j]))
        if not m:
            continue
        val = m.group(1).strip()
        # Join continuation lines: unclosed parenthesis, or trailing comma.
        k = j + 1
        while k < len(lines) and k < j + 4:
            if val.count("(") > val.count(")") or val.endswith(","):
                nxt = lines[k].strip()
                if not nxt or re.match(
                    r"^(casting time|range|components|duration)\b", nxt, re.I
                ):
                    break
                val = (val + " " + nxt).strip()
                k += 1
            else:
                break
        return re.sub(r"\s+", " ", val).strip(" .")
    return ""


def extract_spells(lines):
    spells = {}
    for i, line in enumerate(lines):
        if not re.match(r"^casting\s*time\s*[:;.]", strip_lead(line), re.I):
            continue

        # Walk back for the level/school line, then the name above it.
        ls_idx = None
        for k in range(i - 1, max(i - 9, 0), -1):
            got = parse_level_school(lines[k])
            if got:
                ls_idx = k
                parsed = got
                break
        if ls_idx is None:
            continue

        name = None
        for k in range(ls_idx - 1, max(ls_idx - 6, 0), -1):
            cand = strip_lead(lines[k])
            if is_noise(cand):
                continue
            if parse_level_school(cand):
                continue
            if name_looks_sane(cand):
                name = cand
            break
        if not name:
            continue

        level, school, ritual = parsed
        ct = field_after(lines, i, "casting\\s*time")
        rng = field_after(lines, i, "range")
        comp = field_after(lines, i, "components?")
        dur = field_after(lines, i, "duration")
        if not ct or not rng:
            continue

        conc = bool(re.match(r"^concentration", dur, re.I))
        key = name.lower()
        if key not in spells:
            spells[key] = {
                "name": name, "book": "phb", "level": level, "school": school,
                "ritual": ritual, "time": ct, "range": rng,
                "components": comp, "duration": dur, "conc": conc,
                "classes": set(),
            }
    return spells


# --------------------------------------------------------------------------
# per-class spell lists
# --------------------------------------------------------------------------

LEVEL_HDR = re.compile(
    r"^(cantrips?\b|[0-9a-z]{1,4}\s*(st|nd|rd|th)\s+level\b)", re.I
)


def find_list_bounds(lines):
    """Locates each class's spell-list heading and returns (start, end) lines."""
    found = {}
    a, b = LISTS_REGION
    for idx in range(a - 1, min(b - 1, len(lines))):
        squashed = re.sub(r"[^a-z]", "", lines[idx].lower())
        for cls in PHB_CLASSES:
            if squashed == cls + "spells" and cls not in found:
                found[cls] = idx + 1              # 1-based line number
    missing = [c for c in PHB_CLASSES if c not in found]
    if missing:
        raise SystemExit("could not locate spell list heading(s): %s"
                         % ", ".join(missing))

    ordered = sorted(found.items(), key=lambda kv: kv[1])
    bounds = {}
    for i, (cls, start) in enumerate(ordered):
        bounds[cls] = (start, ordered[i + 1][1] if i + 1 < len(ordered) else b)
    return bounds



def extract_lists(lines):
    out = {c: [] for c in PHB_CLASSES}
    bounds = find_list_bounds(lines)
    for cls, (a, b) in bounds.items():
        cur = None
        for raw in lines[a - 1: b - 1]:
            s = strip_lead(raw)
            if not s:
                continue
            low = s.lower()
            if low.startswith("cantrip"):
                cur = 0
                continue
            m = re.match(r"^([0-9a-z]{1,4})\s*(?:st|nd|rd|th)\s+level\b", low)
            if m:
                key = m.group(1).strip()
                cur = OCR_ORD.get(key)
                if cur is None:
                    d = re.match(r"([1-9])", key)
                    cur = int(d.group(1)) if d else None
                continue
            if cur is None:
                continue
            if is_noise(s) or not name_looks_sane(s):
                continue
            if re.search(r"\bspells?\b$", low):
                continue
            out[cls].append((s.lower(), cur))
    return out


# --------------------------------------------------------------------------
# OCR repairs
# --------------------------------------------------------------------------
#
# Every entry below was verified by reading the surrounding text in
# TextFiles/PHBtext.txt; each records a specific, identified way the OCR
# damaged the page. Nothing here is guesswork.

# The OCR mangled these spell names beyond fuzzy matching.
NAME_REPAIRS = {
    "earth(^uake": "Earthquake",
    # The OCR reads C as G in some small-capital headings. These four were
    # found by comparing each description's name with the spelling on the
    # class spell lists, which are set in ordinary type and came out clean.
    "gall lightning": "Call Lightning",
    "prismatig wall": "Prismatic Wall",
    "vampirig touch": "Vampiric Touch",
    "wall of forge": "Wall of Force",
}

# The OCR dropped these stat-block lines entirely.
FIELD_REPAIRS = {
    "wind walk": {"duration": "8 hours"},
}

# On the Create or Destroy Water / Creation page the OCR interleaved the two
# columns, stacking both headers above a single stat block. The block belongs
# to Create or Destroy Water; Creation's own block appears later, orphaned
# from its header. Both spells are restated here from that page.
BLOCK_REPAIRS = {
    "creation": {
        "name": "Creation", "level": 5, "school": "illusion", "ritual": False,
        "time": "1 minute", "range": "30 feet",
        "components": "V, S, M (a tiny piece of matter of the same type of "
                      "the item you plan to create)",
        "duration": "Special", "conc": False,
    },
    "create or destroy water": {
        "name": "Create or Destroy Water", "level": 1,
        "school": "transmutation", "ritual": False,
        "time": "1 action", "range": "30 feet",
        "components": "V, S, M (a drop of water if creating water or a few "
                      "grains of sand if destroying it)",
        "duration": "Instantaneous", "conc": False,
    },
}

# Class-list entries the OCR corrupted into a different word.
LIST_ALIASES = {
    "destructive smite": "destructive wave",
}

# Class-list lines that are page furniture, not spells.
LIST_NOISE = {"sorc erer sp ells", "sorcerer spells", "wizard spells"}

# Xanathar's spells whose stat line renders the level as the ambiguous "S"
# glyph, and which appear under a list heading the OCR also mangled. Both were
# read off the page directly.
EXPANSION_LEVEL_REPAIRS = {
    "enervation": 5,
    "maddening darkness": 8,
}

# Listed on the wizard's 8th-level list but given no spell description
# anywhere in the PHB -- a known errata item. It cannot be offered as a
# choice without a stat block, so it is excluded and reported.
KNOWN_ABSENT = {"trap the soul"}


# --------------------------------------------------------------------------
# matching list entries to description entries
# --------------------------------------------------------------------------


def canon(name):
    """Alpha-only key. Tolerates the OCR's lost spaces ("zone oftruth")."""
    return re.sub(r"[^a-z]", "", name.lower())


def fuzzy(name):
    """Canonical key that also folds the OCR's C/G and I/L confusions."""
    return canon(name).replace("g", "c").replace("l", "i").replace("j", "i")


def build_index(spells):
    exact, loose = {}, {}
    for key, sp in spells.items():
        exact.setdefault(canon(sp["name"]), key)
        loose.setdefault(fuzzy(sp["name"]), key)
    return exact, loose


def resolve(entry, exact, loose):
    c = canon(entry)
    if c in exact:
        return exact[c]
    return loose.get(fuzzy(entry))


def apply_repairs(spells):
    for bad, good in NAME_REPAIRS.items():
        if bad in spells:
            sp = spells.pop(bad)
            sp["name"] = good
            spells[good.lower()] = sp
    for key, fields in FIELD_REPAIRS.items():
        if key in spells:
            spells[key].update(fields)
    for key, block in BLOCK_REPAIRS.items():
        sp = spells.get(key, {"classes": set()})
        classes = sp.get("classes", set())
        sp = dict(block)
        sp["classes"] = classes
        spells[key] = sp
    return spells


# --------------------------------------------------------------------------
# emit C
# --------------------------------------------------------------------------

SCHOOL_ENUM = [s.upper() for s in SCHOOLS]


def cstr(s):
    return '"' + s.replace("\\", "\\\\").replace('"', '\\"') + '"'


def emit(ordered):
    hdr = ['/* Generated by tools/extract_spells.py -- do not edit by hand. */',
           '#ifndef DATA_SPELLS_H', '#define DATA_SPELLS_H', '',
           'typedef enum {']
    for e in SCHOOL_ENUM:
        hdr.append('    SCHOOL_%s,' % e)
    hdr += ['    SCHOOL_COUNT', '} School;', '',
            'typedef enum {']
    for c in CLASSES:
        hdr.append('    SPL_%s = 1u << %d,' % (c.upper(), CLASSES.index(c)))
    hdr += ['} SpellClassBit;', '',
            'typedef struct {',
            '    const char *name;',
            '    unsigned char book;         /* matches SourceBook in data.h:',
            '                                   0 PHB, 1 XGE, 2 TCE */',
            '    unsigned char level;        /* 0 = cantrip */',
            '    unsigned char school;       /* School */',
            '    unsigned char ritual;',
            '    unsigned char concentration;',
            '    const char *casting_time;',
            '    const char *range;',
            '    const char *components;',
            '    const char *duration;',
            '    unsigned short classes;     /* SpellClassBit mask */',
            '} SpellData;', '',
            '/* The bank is a pointer, not an array, so homebrew.c can',
            '   replace it with a larger one holding the book spells plus',
            '   whatever the DM has added. SPELLS[i] reads the same either',
            '   way, and BOOK_SPELLS is what the book itself provides. */',
            'extern const SpellData *SPELLS;',
            'extern int SPELL_COUNT;',
            'extern const SpellData BOOK_SPELLS[];',
            'extern const int BOOK_SPELL_COUNT;',
            'extern const char *const SCHOOL_NAMES[SCHOOL_COUNT];', '',
            '#endif']

    src = ['/* Generated by tools/extract_spells.py -- do not edit by hand. */',
           '#include "data_spells.h"', '',
           'const char *const SCHOOL_NAMES[SCHOOL_COUNT] = {']
    for s in SCHOOLS:
        src.append('    "%s",' % s.capitalize())
    src += ['};', '', 'const SpellData BOOK_SPELLS[] = {']
    for sp in ordered:
        bits = " | ".join("SPL_" + c.upper() for c in sorted(sp["classes"]))
        book = {"phb": 0, "xge": 1, "tce": 2}[sp.get("book", "phb")]
        src.append('    { %s, %d, %d, SCHOOL_%s, %d, %d,' % (
            cstr(sp["name"]), book, sp["level"], sp["school"].upper(),
            1 if sp["ritual"] else 0, 1 if sp["conc"] else 0))
        src.append('      %s, %s,' % (cstr(sp["time"]), cstr(sp["range"])))
        src.append('      %s,' % cstr(sp["components"]))
        src.append('      %s, %s },' % (cstr(sp["duration"]), bits or "0"))
    src += ['};', '',
            'const int BOOK_SPELL_COUNT =',
            '    (int)(sizeof(BOOK_SPELLS) / sizeof(BOOK_SPELLS[0]));',
            '',
            '/* Until homebrew.c says otherwise, the bank is just the book. */',
            'const SpellData *SPELLS = BOOK_SPELLS;',
            'int SPELL_COUNT =',
            '    (int)(sizeof(BOOK_SPELLS) / sizeof(BOOK_SPELLS[0]));',
            '']

    out = os.path.join(ROOT, "src")
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "data_spells.h"), "w") as fh:
        fh.write("\n".join(hdr) + "\n")
    with open(os.path.join(out, "data_spells.c"), "w") as fh:
        fh.write("\n".join(src) + "\n")


# --------------------------------------------------------------------------
# Xanathar's Guide and Tasha's Cauldron
# --------------------------------------------------------------------------
#
# Both books render spell names in small capitals, which the OCR turns into
# mixed case and sometimes doubles a letter ("ABI-DALZzIM'S"). Their own
# class lists and summary tables, however, carry properly cased names along
# with each spell's school. So the lists supply the name, class and school,
# the description block supplies the stat lines, and the school is required
# to agree before the two are joined.


def collapse(name):
    """Canonical key with runs of a repeated letter collapsed to one.

    Small-capital OCR turns "Dalzim" into "DALZzIM"; collapsing doubles makes
    the two forms comparable.
    """
    k = fuzzy(name)
    out = []
    for ch in k:
        if not out or out[-1] != ch:
            out.append(ch)
    return "".join(out)


def edit_distance(a, b):
    """Levenshtein distance, used only to rescue OCR letter substitutions."""
    if a == b:
        return 0
    prev = list(range(len(b) + 1))
    for i, ca in enumerate(a, 1):
        cur = [i]
        for j, cb in enumerate(b, 1):
            cur.append(min(prev[j] + 1, cur[j - 1] + 1,
                           prev[j - 1] + (ca != cb)))
        prev = cur
    return prev[-1]


def match_description(descs, name, school):
    """Finds the description entry for a listed spell, requiring the school
    to agree. Returns its key, or None."""
    want_school = school.lower() if school else None
    candidates = []
    for key, sp in descs.items():
        if want_school and sp["school"] != want_school:
            continue
        candidates.append((key, sp))

    for keyfn in (canon, fuzzy, collapse):
        target = keyfn(name)
        hits = [k for k, sp in candidates if keyfn(sp["name"]) == target]
        if len(hits) == 1:
            return hits[0]

    # The small-capital OCR also substitutes letters outright ("Cuaos BoLt"
    # for "Chaos Bolt"). Having already narrowed to one school, the nearest
    # name by edit distance identifies it, provided it is clearly nearest.
    target = collapse(name)
    scored = sorted((edit_distance(target, collapse(sp["name"])), k)
                    for k, sp in candidates)
    if scored:
        best, key = scored[0]
        runner_up = scored[1][0] if len(scored) > 1 else 99
        if best <= max(2, len(target) // 5) and best < runner_up:
            return key
    return None


def list_level_header(line):
    """Recognises a spell-list level heading. Returns 0 for cantrips, 1 for
    a first-level heading, -1 for any other heading, or None if the line is
    not a heading at all.

    Only 'cantrips' and '1st' need to be read exactly, since they are what
    marks the start of a class's list; the OCR mangles the rest ("47H LEVEL",
    "OTH LEVEL") and the real level comes from the matched description.
    """
    squashed = re.sub(r"[^a-z0-9]", "", line.lower())
    if not squashed.endswith("level") and "level" not in squashed:
        return None
    if not re.search(r"level$|level\)?$|0level$", squashed):
        return None

    # "CantTRips" -- the small-capital OCR doubles letters.
    if "cantrip" in collapse_runs(squashed):
        return 0
    head = squashed[:-len("level")] if squashed.endswith("level") else squashed
    head = head.replace("0", "")
    if head in ("1st", "ist", "lst", "1s"):
        return 1
    return -1


def collapse_runs(text):
    out = []
    for ch in text:
        if not out or out[-1] != ch:
            out.append(ch)
    return "".join(out)


LIST_ENTRY = re.compile(r"^(.+?)\s*\(([a-z]+)(?:,\s*(ritual))?\)\s*$", re.I)


def parse_xge_lists(lines):
    """Parses Xanathar's class spell lists.

    The headings sometimes appear in a run before their blocks rather than
    inline, so headings and blocks are collected separately and paired by
    order. A block ends when a level heading repeats, which is how one
    class's list is told from the next.
    """
    a, b = XGE_LISTS
    headings, blocks = [], []
    block = None
    seen_first = False
    pending = ""

    for raw in lines[a - 1: b - 1]:
        line = strip_lead(raw)
        if not line:
            continue

        squashed = re.sub(r"[^a-z]", "", line.lower())
        if squashed.endswith("spells") and squashed[:-6] in CLASSES:
            headings.append(squashed[:-6])
            pending = ""
            continue

        lvl = list_level_header(line)
        if lvl is not None:
            if lvl == 0 or (lvl == 1 and seen_first) or block is None:
                block = []
                blocks.append(block)
                seen_first = False
            if lvl == 1:
                seen_first = True
            pending = ""
            continue

        if block is None:
            continue

        # A long name wraps, leaving the "(school)" on the following line.
        candidate = (pending + " " + line).strip() if pending else line
        m = LIST_ENTRY.match(candidate)
        if not m:
            pending = candidate if "(" not in candidate else ""
            continue
        pending = ""

        name, school, ritual = m.group(1).strip(), m.group(2).lower(), m.group(3)
        if school not in SCHOOLS:
            continue
        block.append((name, school, bool(ritual)))

    return headings, blocks


def parse_tce_table(lines):
    """Parses Tasha's summary table.

    It is printed as two column groups: every spell's level, name, school and
    concentration flag, then every spell's ritual flag and class list in the
    same order. Zipping them recovers the assignments.
    """
    a, b = TCE_TABLE
    rows, classlists = [], []
    ordinal = re.compile(r"^(0|[1-9IilL]st|2nd|3rd|[4-9]th)\s+(.+?)\s+"
                         r"([A-Za-z]+)\s+(Yes|No)\s*$")
    classrow = re.compile(r"^(Yes|No)\s+([A-Z][A-Za-z]+(?:,\s*[A-Z][A-Za-z]+)*)\s*$")

    for raw in lines[a - 1: b - 1]:
        line = re.sub(r"\s+", " ", strip_lead(raw)).replace("|", "").strip()
        if not line:
            continue
        m = ordinal.match(line)
        if m:
            ord_txt = m.group(1)
            if ord_txt == "0":
                lvl = 0
            elif ord_txt[0].isdigit():
                lvl = int(ord_txt[0])
            else:
                lvl = 1                 # "Ist" is the OCR's 1st
            school = m.group(3).lower()
            if school in SCHOOLS:
                rows.append((m.group(2).strip(), lvl, school))
            continue
        m = classrow.match(line)
        if m:
            names = [c.strip().lower() for c in m.group(2).split(",")]
            if all(c in CLASSES for c in names):
                classlists.append((bool(m.group(1) == "Yes"), names))

    return rows, classlists


# Tasha's prints the artificer spell list in interleaved columns that the OCR
# shreds, so it is written out here instead. Only the names are recorded --
# each spell's level and school come from its own description -- and every
# name is checked against the database below.
ARTIFICER_SPELLS = [
    # cantrips
    "Acid Splash", "Booming Blade", "Create Bonfire", "Dancing Lights",
    "Fire Bolt", "Frostbite", "Green-Flame Blade", "Guidance", "Light",
    "Lightning Lure", "Mage Hand", "Magic Stone", "Mending", "Message",
    "Poison Spray", "Prestidigitation", "Ray of Frost", "Resistance",
    "Shocking Grasp", "Spare the Dying", "Sword Burst", "Thorn Whip",
    "Thunderclap",
    # 1st
    "Absorb Elements", "Alarm", "Catapult", "Cure Wounds", "Detect Magic",
    "Disguise Self", "Expeditious Retreat", "Faerie Fire", "False Life",
    "Feather Fall", "Grease", "Identify", "Jump", "Longstrider",
    "Purify Food and Drink", "Sanctuary", "Snare", "Tasha's Caustic Brew",
    # 2nd
    "Aid", "Alter Self", "Arcane Lock", "Blur", "Continual Flame",
    "Darkvision", "Enhance Ability", "Enlarge/Reduce", "Heat Metal",
    "Invisibility", "Lesser Restoration", "Levitate", "Magic Mouth",
    "Magic Weapon", "Protection from Poison", "Pyrotechnics", "Rope Trick",
    "See Invisibility", "Skywrite", "Spider Climb", "Web",
    # 3rd
    "Blink", "Catnap", "Create Food and Water", "Dispel Magic",
    "Elemental Weapon", "Flame Arrows", "Fly", "Glyph of Warding", "Haste",
    "Intellect Fortress", "Protection from Energy", "Revivify",
    "Tiny Servant", "Water Breathing", "Water Walk",
    # 4th
    "Arcane Eye", "Elemental Bane", "Fabricate", "Freedom of Movement",
    "Leomund's Secret Chest", "Mordenkainen's Faithful Hound",
    "Mordenkainen's Private Sanctum", "Otiluke's Resilient Sphere",
    "Stone Shape", "Stoneskin", "Summon Construct",
    # 5th
    "Animate Objects", "Bigby's Hand", "Creation", "Greater Restoration",
    "Skill Empowerment", "Transmute Rock", "Wall of Stone",
]

# Spells per level the artificer list must end up with.
ARTIFICER_BY_LEVEL = {0: 23, 1: 18, 2: 21, 3: 15, 4: 11, 5: 7}


def add_expansion_spells(spells, problems):
    """Merges the Xanathar's and Tasha's spells into the PHB set."""

    # ---- Xanathar's Guide -------------------------------------------------
    xge_lines = load_lines(SRC_XGE)
    xge_desc = extract_spells(xge_lines[XGE_DESC[0] - 1: XGE_DESC[1] - 1])
    headings, blocks = parse_xge_lists(xge_lines)

    if len(headings) != len(blocks):
        problems.append("Xanathar's: %d class headings but %d list blocks"
                        % (len(headings), len(blocks)))
        return

    added = {}
    for cls, block in zip(headings, blocks):
        for name, school, ritual in block:
            key = match_description(xge_desc, name, school)
            if key is None:
                problems.append("Xanathar's: no description for %r (%s)"
                                % (name, school))
                continue
            sp = xge_desc[key]
            entry = added.get(key)
            if entry is None:
                entry = dict(sp)
                entry["name"] = titlecase(name)
                entry["book"] = "xge"
                entry["school"] = school
                entry["ritual"] = ritual or sp["ritual"]
                entry["classes"] = set()
                added[key] = entry
            entry["classes"].add(cls)

    for key, entry in added.items():
        if entry["level"] is None:
            entry["level"] = EXPANSION_LEVEL_REPAIRS.get(entry["name"].lower())
        if entry["level"] is None:
            problems.append("Xanathar's: unresolved level for %r"
                            % entry["name"])
            continue
        merge_spell(spells, entry, problems, "Xanathar's")

    if len(added) != len(xge_desc):
        missing = sorted(xge_desc[k]["name"] for k in xge_desc if k not in added)
        problems.append("Xanathar's: %d of %d spells are on no class list: %s"
                        % (len(missing), len(xge_desc), ", ".join(missing)))

    # ---- Tasha's Cauldron -------------------------------------------------
    tce_lines = load_lines(SRC_TCE)
    tce_desc = extract_spells(tce_lines[TCE_DESC[0] - 1: TCE_DESC[1] - 1])
    rows, classlists = parse_tce_table(tce_lines)

    if len(rows) != len(classlists):
        problems.append("Tasha's: %d spell rows but %d class rows"
                        % (len(rows), len(classlists)))
    elif len(rows) != len(tce_desc):
        problems.append("Tasha's: %d table rows but %d descriptions"
                        % (len(rows), len(tce_desc)))
    else:
        for (name, level, school), (ritual, classes) in zip(rows, classlists):
            key = match_description(tce_desc, name, school)
            if key is None:
                problems.append("Tasha's: no description for %r (%s)"
                                % (name, school))
                continue
            entry = dict(tce_desc[key])
            entry["name"] = name
            entry["book"] = "tce"
            entry["level"] = level
            entry["school"] = school
            entry["ritual"] = ritual
            entry["classes"] = set(classes)
            merge_spell(spells, entry, problems, "Tasha's")

    # ---- the artificer list ----------------------------------------------
    by_name = {canon(sp["name"]): sp for sp in spells.values()}
    counts = {}
    for name in ARTIFICER_SPELLS:
        sp = by_name.get(canon(name))
        if sp is None:
            problems.append("artificer list: no spell named %r" % name)
            continue
        sp["classes"].add("artificer")
        counts[sp["level"]] = counts.get(sp["level"], 0) + 1

    for level, want in ARTIFICER_BY_LEVEL.items():
        got = counts.get(level, 0)
        if got != want:
            problems.append("artificer list: %d spells of level %d, expected %d"
                            % (got, level, want))


def titlecase(name):
    """Title-cases a spell name the way the books print it."""
    small = {"of", "the", "from", "and", "or", "in", "on", "to", "a", "an"}
    parts = name.split()
    out = []
    for i, w in enumerate(parts):
        low = w.lower()
        if i > 0 and low in small:
            out.append(low)
        elif "'" in w:
            head, _, tail = w.partition("'")
            out.append(head[:1].upper() + head[1:].lower() + "'" + tail.lower())
        elif "-" in w:
            out.append("-".join(p[:1].upper() + p[1:].lower()
                                for p in w.split("-")))
        else:
            out.append(w[:1].upper() + w[1:].lower())
    return " ".join(out)


def merge_spell(spells, entry, problems, book):
    """Adds an expansion spell, or merges it into an existing entry."""
    key = entry["name"].lower()
    existing = spells.get(key)
    if existing is None:
        # A spell reprinted from another book may already be present under a
        # slightly different key; look for it before adding a duplicate.
        for k, sp in spells.items():
            if canon(sp["name"]) == canon(entry["name"]):
                existing = sp
                break
    if existing is not None:
        existing["classes"] |= entry["classes"]
        return
    spells[key] = entry


def main():
    lines = load_lines()
    spells = apply_repairs(extract_spells(lines[DESC_START - 1: DESC_END - 1]))
    lists = extract_lists(lines)
    exact, loose = build_index(spells)

    unmatched, level_votes = {}, {}
    for cls in PHB_CLASSES:
        entries = lists[cls]
        skip = set()
        for idx, (nm, lvl) in enumerate(entries):
            if idx in skip or nm in LIST_NOISE or nm in KNOWN_ABSENT:
                continue
            nm = LIST_ALIASES.get(nm, nm)
            key = resolve(nm, exact, loose)
            if key is None and idx + 1 < len(entries):
                # The OCR wraps long names across two lines
                # ("Mordenkainen's" / "Magnificent Mansion").
                key = resolve(nm + " " + entries[idx + 1][0], exact, loose)
                if key is not None:
                    skip.add(idx + 1)
            if key is None:
                unmatched.setdefault(cls, []).append(nm)
                continue
            spells[key]["classes"].add(cls)
            level_votes.setdefault(key, set()).add(lvl)

    unresolved = []
    for key, sp in spells.items():
        votes = level_votes.get(key, set())
        if sp["level"] is None:
            if len(votes) == 1:
                sp["level"] = votes.pop()
            else:
                unresolved.append((sp["name"], sorted(votes)))
        elif votes and len(votes) == 1 and sp["level"] not in votes:
            sp["level"] = sorted(votes)[0]

    # The cantrip guard checks the PHB's own section boundaries, so take the
    # counts before the expansions contribute cantrips of their own.
    cantrip_errors = []
    for cls in PHB_CLASSES:
        got = sum(1 for sp in spells.values()
                  if cls in sp["classes"] and sp["level"] == 0)
        want = EXPECTED_CANTRIPS[cls]
        if got != want:
            cantrip_errors.append("%s: %d cantrips, expected %d"
                                  % (cls, got, want))

    # ---------------------------------------------------------- expansions
    problems = []
    add_expansion_spells(spells, problems)

    for sp in spells.values():
        sp["conc"] = bool(re.match(r"^concentration", sp["duration"] or "", re.I))

    ordered = sorted(spells.values(), key=lambda s: s["name"].lower())

    bad = [s["name"] for s in ordered
           if s["level"] is None
           or not all(s[f] for f in ("time", "range", "duration"))]
    nleft = sum(len(v) for v in unmatched.values())
    sys.stderr.write("spells: %d  unmatched list entries: %d  "
                     "unresolved levels: %d  incomplete: %d\n"
                     % (len(ordered), nleft, len(unresolved), len(bad)))
    for cls, names in sorted(unmatched.items()):
        for n in names:
            sys.stderr.write("    ? %s: %s\n" % (cls, n))
    for nm, v in unresolved:
        sys.stderr.write("    ! unresolved level: %s votes=%s\n" % (nm, v))
    for nm in bad:
        sys.stderr.write("    ! incomplete: %s\n" % nm)
    for nm in sorted(KNOWN_ABSENT):
        sys.stderr.write("    note: %r listed on a class list but has no "
                         "PHB description; excluded\n" % nm)

    for e in cantrip_errors:
        sys.stderr.write("    ! %s\n" % e)

    for e in problems:
        sys.stderr.write("    ! %s\n" % e)

    if nleft or unresolved or bad or cantrip_errors or problems:
        sys.stderr.write("EXTRACTION INCOMPLETE\n")
        return 1
    emit(ordered)
    sys.stderr.write("wrote src/data_spells.{h,c}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
