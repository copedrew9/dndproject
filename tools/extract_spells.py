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

DESC_START, DESC_END = 31590, 43119

SCHOOLS = [
    "abjuration", "conjuration", "divination", "enchantment",
    "evocation", "illusion", "necromancy", "transmutation",
]

CLASSES = ["bard", "cleric", "druid", "paladin", "ranger", "sorcerer",
           "warlock", "wizard"]

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


def load_lines():
    with open(SRC, encoding="utf-8", errors="replace") as fh:
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
                "name": name, "level": level, "school": school,
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
        for cls in CLASSES:
            if squashed == cls + "spells" and cls not in found:
                found[cls] = idx + 1              # 1-based line number
    missing = [c for c in CLASSES if c not in found]
    if missing:
        raise SystemExit("could not locate spell list heading(s): %s"
                         % ", ".join(missing))

    ordered = sorted(found.items(), key=lambda kv: kv[1])
    bounds = {}
    for i, (cls, start) in enumerate(ordered):
        bounds[cls] = (start, ordered[i + 1][1] if i + 1 < len(ordered) else b)
    return bounds



def extract_lists(lines):
    out = {c: [] for c in CLASSES}
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
            'extern const SpellData SPELLS[];',
            'extern const int SPELL_COUNT;',
            'extern const char *const SCHOOL_NAMES[SCHOOL_COUNT];', '',
            '#endif']

    src = ['/* Generated by tools/extract_spells.py -- do not edit by hand. */',
           '#include "data_spells.h"', '',
           'const char *const SCHOOL_NAMES[SCHOOL_COUNT] = {']
    for s in SCHOOLS:
        src.append('    "%s",' % s.capitalize())
    src += ['};', '', 'const SpellData SPELLS[] = {']
    for sp in ordered:
        bits = " | ".join("SPL_" + c.upper() for c in sorted(sp["classes"]))
        src.append('    { %s, %d, SCHOOL_%s, %d, %d,' % (
            cstr(sp["name"]), sp["level"], sp["school"].upper(),
            1 if sp["ritual"] else 0, 1 if sp["conc"] else 0))
        src.append('      %s, %s,' % (cstr(sp["time"]), cstr(sp["range"])))
        src.append('      %s,' % cstr(sp["components"]))
        src.append('      %s, %s },' % (cstr(sp["duration"]), bits or "0"))
    src += ['};', '',
            'const int SPELL_COUNT = (int)(sizeof(SPELLS) / sizeof(SPELLS[0]));',
            '']

    out = os.path.join(ROOT, "src")
    os.makedirs(out, exist_ok=True)
    with open(os.path.join(out, "data_spells.h"), "w") as fh:
        fh.write("\n".join(hdr) + "\n")
    with open(os.path.join(out, "data_spells.c"), "w") as fh:
        fh.write("\n".join(src) + "\n")


def main():
    lines = load_lines()
    spells = apply_repairs(extract_spells(lines[DESC_START - 1: DESC_END - 1]))
    lists = extract_lists(lines)
    exact, loose = build_index(spells)

    unmatched, level_votes = {}, {}
    for cls in CLASSES:
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

    for sp in spells.values():
        sp["conc"] = bool(re.match(r"^concentration", sp["duration"] or "", re.I))

    # Guard against a mis-detected section boundary silently moving spells
    # from one class list to another.
    cantrip_errors = []
    for cls in CLASSES:
        bit = cls
        got = sum(1 for sp in spells.values()
                  if bit in sp["classes"] and sp["level"] == 0)
        want = EXPECTED_CANTRIPS[cls]
        if got != want:
            cantrip_errors.append("%s: %d cantrips, expected %d"
                                  % (cls, got, want))

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

    if nleft or unresolved or bad or cantrip_errors:
        sys.stderr.write("EXTRACTION INCOMPLETE\n")
        return 1
    emit(ordered)
    sys.stderr.write("wrote src/data_spells.{h,c}\n")
    return 0


if __name__ == "__main__":
    sys.exit(main())
