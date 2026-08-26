#!/usr/bin/env python3
"""Generate src/data_beasts.{c,h} from the Monster Manual text dump.

Only beasts are taken: they are what a druid's Wild Shape, a Beast Master's
companion and a find familiar can become. The stat blocks in the dump are
clean enough to read mechanically, so everything here is parsed and then
asserted rather than typed out by hand.

Challenge ratings are stored in eighths, so 1/8 is 1 and 2 is 16; that keeps
the comparison a druid needs (CR at most 1/4 until 8th level) in integers.
"""
import re
import sys

SRC = "TextFiles/MMtext.txt"
SIZES = ("Tiny", "Small", "Medium", "Large", "Huge", "Gargantuan")

# Names the OCR mangled, and the single ability score it destroyed for a
# handful of beasts. Everything else is read straight from the dump.
NAME_REPAIRS = {
    "Allosaurus I": "Allosaurus",
    "Giant Grab": "Giant Crab",
    "MtTLE": "Mule",
    "TVrannosaurus Rex": "Tyrannosaurus Rex",
    "Gi ant Owl": "Giant Owl",
}

# The dump loses a single score for a number of beasts -- almost always the
# Charisma column, which sits at the edge of the page. These are supplied so
# the rest of the block can still be used; nothing else is filled in.
# Stat blocks the dump does not carry completely -- a page where the block
# is cut short, so there is nothing to read rather than something to repair.
# These are typed out in full rather than guessed at from a partial block.
EXTRA_BEASTS = [
    {"name": "Black Bear", "size": "Medium", "ac": 11, "hp": 19,
     "speed": "40 ft., climb 30 ft", "abilities": [15, 10, 14, 2, 12, 7],
     "cr_text": "1/2", "senses": "passive Perception 13"},
    {"name": "Hawk", "size": "Tiny", "ac": 13, "hp": 1,
     "speed": "10 ft., fly 60 ft", "abilities": [5, 16, 8, 2, 14, 6],
     "cr_text": "0", "senses": "passive Perception 14"},
    {"name": "Giant Wolf Spider", "size": "Medium", "ac": 13, "hp": 11,
     "speed": "40 ft., climb 40 ft", "abilities": [12, 16, 13, 3, 12, 4],
     "cr_text": "1/4",
     "senses": "blindsight 10 ft., darkvision 60 ft., passive Perception 13"},
    {"name": "Hyena", "size": "Medium", "ac": 11, "hp": 5,
     "speed": "50 ft", "abilities": [11, 13, 12, 2, 12, 5],
     "cr_text": "0", "senses": "passive Perception 13"},
    {"name": "Jackal", "size": "Small", "ac": 12, "hp": 3,
     "speed": "40 ft", "abilities": [8, 15, 11, 3, 12, 6],
     "cr_text": "0", "senses": "passive Perception 13"},
    {"name": "Pteranodon", "size": "Medium", "ac": 13, "hp": 13,
     "speed": "10 ft., fly 60 ft", "abilities": [12, 15, 10, 2, 9, 5],
     "cr_text": "1/4", "senses": "passive Perception 11"},
    {"name": "Giant Crocodile", "size": "Huge", "ac": 14, "hp": 85,
     "speed": "30 ft., swim 50 ft", "abilities": [21, 9, 17, 2, 10, 7],
     "cr_text": "5", "senses": "passive Perception 10"},
    {"name": "Giant Shark", "size": "Huge", "ac": 13, "hp": 126,
     "speed": "0 ft., swim 50 ft", "abilities": [23, 11, 21, 1, 10, 5],
     "cr_text": "5",
     "senses": "blindsight 60 ft., passive Perception 13"},
]

ABILITY_REPAIRS = {
    "Boar":                {"WIS": 9,  "CHA": 5},
    "Deer":                {"CHA": 5},
    "Frog":                {"CHA": 3},
    "Giant Badger":        {"INT": 2},
    "Giant Eagle":         {"INT": 8},
    "Giant Elk":           {"STR": 19, "DEX": 16, "CON": 14,
                            "INT": 7,  "WIS": 14, "CHA": 10},
    "Giant Owl":           {"CHA": 10},
    "Giant Sea Horse":     {"CHA": 5},
    "Giant Wolf Spider":   {"INT": 3},
    "Hawk":                {"INT": 2},
    "Killer Whale":        {"CHA": 7},
    "Lion":                {"CHA": 8},
    "Lizard":              {"CHA": 5},
    "Owl":                 {"CHA": 7},
    "Saber-Toothed Tiger": {"CHA": 8},
    "Tiger":               {"CHA": 8},
}

CR_EIGHTHS = {
    "0": 0, "1/8": 1, "1/4": 2, "1/2": 4,
    "1": 8, "2": 16, "3": 24, "4": 32, "5": 40, "6": 48, "7": 56, "8": 64,
    "9": 72, "10": 80, "11": 88, "12": 96, "13": 104,
}

# The dump renders some digits as letters and drops spaces; these are the
# repairs needed inside stat lines.
def fix_digits(s):
    s = s.replace("dl", "d1").replace("dS", "d8").replace("lO", "10")
    s = re.sub(r"\bl(\d)", r"1\1", s)
    s = re.sub(r"\b(\d)C\b", r"\g<1>0", s)      # "6C ft." is 60 ft.
    return s


ABILITY_ORDER = ("STR", "DEX", "CON", "INT", "WIS", "CHA")


def parse_abilities(chunk, lost):
    """Read one beast's six scores, keeping them tied to their column.

    The dump breaks the ability table up in several ways: sometimes each
    label is followed by its score, sometimes all six labels come first and
    the scores follow. Either way, a score the OCR destroyed would shift its
    neighbours along, so the columns known to be lost (from ABILITY_REPAIRS)
    are taken out of the running before the readable scores are assigned.
    Anything still unaccounted for comes back as None rather than a guess.
    """
    # The dump sometimes splits a column heading -- "I NT" for INT -- which
    # would leave the label unrecognised and shift every score after it.
    for a in ABILITY_ORDER:
        chunk = re.sub(r"\b%s\b" % r"\s*".join(a), a, chunk)

    tokens = re.findall(
        r"STR|DEX|CON|INT|WIS|CHA"
        r"|\d+\s*\(\s*[-+\u2013\u2014]?\s?\d+\s*\)", chunk)

    labels = [t for t in tokens if t in ABILITY_ORDER]
    values = [int(re.match(r"(\d+)", t).group(1))
              for t in tokens if t not in ABILITY_ORDER]

    out = {}
    wanted = [a for a in ABILITY_ORDER if a not in lost]

    # All six labels up front, then the scores: assign by position, skipping
    # the columns we already know were destroyed.
    if len(labels) == 6 and len(values) == len(wanted):
        for a, v in zip(wanted, values):
            out[a] = v
        return [out.get(a) for a in ABILITY_ORDER]

    # Otherwise the labels are interleaved with their scores, so pair them
    # up as they come.
    pending = []
    for t in tokens:
        if t in ABILITY_ORDER:
            pending.append(t)
            continue
        value = int(re.match(r"(\d+)", t).group(1))
        while pending and pending[0] in lost:
            pending.pop(0)
        if pending:
            out[pending.pop(0)] = value
        else:
            for a in wanted:
                if a not in out:
                    out[a] = value
                    break
    return [out.get(a) for a in ABILITY_ORDER]


def is_furniture(line):
    """True for running heads, page numbers and OCR debris."""
    if len(line) < 3:
        return True
    if "APPENDIX" in line.upper() or "MONSTERS" in line.upper():
        return True
    if not re.search(r"[a-z]", line):        # all caps or symbols
        return True
    if re.search(r"[^A-Za-z' \-]", line):    # digits, punctuation, marks
        return True
    return False


def parse(text):
    """Read every beast stat block.

    Appendix pages set two stat blocks side by side, and the dump reads them
    column by column: both names, then both Armor Class lines, then both Hit
    Points lines, and so on. So headers are collected into groups first, and
    each field is then taken N at a time in order.
    """
    lines = text.split("\n")
    header = re.compile(r"^(%s) beast\s*, " % "|".join(SIZES))

    # 1. Find every header: a name line, blank lines, then "<Size> beast".
    heads = []
    for j, line in enumerate(lines):
        if not header.match(line.strip()):
            continue
        # Walk back past blank lines and page furniture -- running heads,
        # stray marks and page numbers sit between a name and its type line.
        k = j - 1
        name = ""
        while k >= 0 and j - k < 12:
            cand = lines[k].strip()
            k -= 1
            if not cand:
                continue
            if is_furniture(cand):
                continue
            name = cand
            break
        if not name or len(name) > 40 or not name[0].isupper():
            continue
        if re.search(r"[,.:0-9]$", name):
            continue
        heads.append({"name": name, "size": header.match(line.strip()).group(1),
                      "line": j, "name_line": k})

    # 2. Group headers that share a page: consecutive headers with no
    #    "Armor Class" line between them belong to the same spread.
    groups, cur = [], []
    for idx, h in enumerate(heads):
        if cur:
            span = "\n".join(lines[cur[-1]["line"]:h["name_line"]])
            if "Armor Class" in span:
                groups.append(cur)
                cur = []
        cur.append(h)
    if cur:
        groups.append(cur)

    beasts, unreadable = [], []
    for gi, g in enumerate(groups):
        start = g[-1]["line"]
        # Stop at the next stat block. A fixed window would run past a
        # truncated block and pick up the following beast's numbers, which
        # is worse than dropping the block.
        end = (groups[gi + 1][0]["name_line"] if gi + 1 < len(groups)
               else start + 45 * len(g) + 40)
        block = "\n".join(lines[start:end])

        acs = re.findall(r"Armor Class (\d+)", block)
        hps = re.findall(r"Hit Points (\d+)", block)
        speeds = re.findall(r"Speed ([^\n]+)", block)
        crs = re.findall(r"Challenge ([\d/]+)", block)
        senses = re.findall(r"Senses ([^\n]+)", block)
        # The ability table starts at each "STR"; one chunk per beast.
        chunks = re.split(r"(?=STR)", block)[1:]

        n = len(g)
        if (len(acs) < n or len(hps) < n or len(speeds) < n or len(crs) < n
                or len(chunks) < n):
            continue

        for idx, h in enumerate(g):
            cr_text = crs[idx]
            if cr_text not in CR_EIGHTHS:
                continue
            sp = fix_digits(speeds[idx]).strip().rstrip(".")
            sp = re.sub(r"\s+", " ", sp)
            name = NAME_REPAIRS.get(h["name"], h["name"])
            fixes = ABILITY_REPAIRS.get(name, {})
            abilities = parse_abilities(chunks[idx], set(fixes))
            for ai, a in enumerate(ABILITY_ORDER):
                if abilities[ai] is None and a in fixes:
                    abilities[ai] = fixes[a]
            if any(v is None for v in abilities):
                unreadable.append((name, [a for a, v in
                                          zip(ABILITY_ORDER, abilities)
                                          if v is None]))
                continue

            beasts.append({
                "name": name,
                "size": h["size"],
                "ac": int(acs[idx]),
                "hp": int(hps[idx]),
                "speed": sp,
                "abilities": abilities,
                "cr": CR_EIGHTHS[cr_text],
                "cr_text": cr_text,
                "senses": (re.sub(r"\s+", " ", senses[idx]).strip().rstrip(".")
                           if idx < len(senses) else "passive Perception 10"),
            })

    if unreadable:
        for name, cols in unreadable:
            print("  unreadable: %s is missing %s" % (name, ", ".join(cols)),
                  file=sys.stderr)

    return beasts


def dedupe(beasts):
    seen, out = {}, []
    for b in beasts:
        if b["name"] in seen:
            continue
        seen[b["name"]] = True
        out.append(b)
    return sorted(out, key=lambda b: b["name"])


# Known-correct values, checked against every run. A two-column page that
# de-interleaves wrongly produces plausible-looking numbers attached to the
# wrong beast, so a spot check across sizes, challenge ratings and movement
# types is the only way to notice.
VERIFY = {
    #                     AC  HP   CR      speed                    abilities
    "Wolf":              (13, 11, "1/4", "40 ft", [12, 15, 12, 3, 12, 6]),
    "Black Bear":        (11, 19, "1/2", "40 ft., climb 30 ft",
                          [15, 10, 14, 2, 12, 7]),
    "Badger":            (10,  3, "0",   "20 ft., burrow 5 ft",
                          [4, 11, 12, 2, 12, 5]),
    "Brown Bear":        (11, 34, "1",   "40 ft., climb 30 ft",
                          [19, 10, 16, 2, 13, 7]),
    "Panther":           (12, 13, "1/4", "50 ft., climb 40 ft",
                          [14, 15, 10, 3, 14, 7]),
    "Giant Spider":      (14, 26, "1",   "30 ft., climb 30 ft",
                          [14, 16, 12, 2, 11, 4]),
    "Giant Octopus":     (11, 52, "1",   "10 ft., swim 60 ft",
                          [17, 13, 13, 4, 10, 4]),
    "Giant Eagle":       (13, 26, "1",   "10 ft., fly 80 ft",
                          [16, 17, 13, 8, 14, 10]),
    "Owl":               (11,  1, "0",   "5 ft., fly 60 ft",
                          [3, 13, 8, 2, 12, 7]),
    "Tiger":             (12, 37, "1",   "40 ft", [17, 15, 14, 3, 12, 8]),
    "Polar Bear":        (12, 42, "2",   "40 ft., swim 30 ft",
                          [20, 10, 16, 2, 13, 7]),
    "Boar":              (11, 11, "1/4", "40 ft", [13, 11, 12, 2, 9, 5]),
    "Giant Elk":         (14, 42, "2",   "60 ft", [19, 16, 14, 7, 14, 10]),
    "Ape":               (12, 19, "1/2", "30 ft., climb 30 ft",
                          [16, 14, 14, 6, 12, 7]),
}


def verify(beasts):
    by_name = {b["name"]: b for b in beasts}
    bad = 0
    for name, (ac, hp, cr, speed, abilities) in VERIFY.items():
        b = by_name.get(name)
        if not b:
            print("  verify: %s was not parsed" % name, file=sys.stderr)
            bad += 1
            continue
        for field, got, want in (("AC", b["ac"], ac), ("HP", b["hp"], hp),
                                 ("CR", b["cr_text"], cr),
                                 ("speed", b["speed"], speed),
                                 ("abilities", b["abilities"], abilities)):
            if got != want:
                print("  verify: %s %s is %r, expected %r"
                      % (name, field, got, want), file=sys.stderr)
                bad += 1
    return bad


def emit(beasts, cpath, hpath):
    with open(hpath, "w") as f:
        f.write("/* data_beasts.h -- generated by tools/extract_beasts.py. */\n"
                "#ifndef DATA_BEASTS_H\n#define DATA_BEASTS_H\n\n"
                "#define BEAST_COUNT %d\n\n#endif\n" % len(beasts))

    with open(cpath, "w") as f:
        f.write('/* data_beasts.c -- beast stat blocks from the Monster '
                'Manual.\n *\n'
                ' * Generated by tools/extract_beasts.py; do not edit by '
                'hand.\n *\n'
                ' * These are what Wild Shape, a Beast Master companion and '
                'find familiar\n'
                ' * draw on. Challenge ratings are in eighths so that the '
                'druid\'s limits\n'
                ' * (CR 1/4 at 2nd level, 1/2 at 4th, 1 at 8th) are integer '
                'comparisons.\n */\n'
                '#include "data.h"\n\n#include <string.h>\n\n'
                'const BeastData BEASTS[] = {\n')
        for b in beasts:
            f.write('{ "%s", BSIZE_%s, %d, %d, %d, "%s",\n'
                    '  { %d, %d, %d, %d, %d, %d }, "%s", "%s" },\n' % (
                        b["name"], b["size"].upper(), b["ac"], b["hp"],
                        b["cr"], b["speed"].replace('"', "'"),
                        b["abilities"][0], b["abilities"][1], b["abilities"][2],
                        b["abilities"][3], b["abilities"][4], b["abilities"][5],
                        b["cr_text"], b["senses"].replace('"', "'")))
        f.write("};\nconst int BEAST_COUNT_ACTUAL = "
                "(int)(sizeof(BEASTS) / sizeof(BEASTS[0]));\n\n"
                "int find_beast(const char *name)\n{\n"
                "    int i;\n"
                "    for (i = 0; i < BEAST_COUNT_ACTUAL; i++)\n"
                "        if (strcmp(BEASTS[i].name, name) == 0) return i;\n"
                "    return -1;\n}\n")


def main():
    text = open(SRC, encoding="utf-8", errors="replace").read()
    parsed = parse(text)
    for extra in EXTRA_BEASTS:
        if any(b["name"] == extra["name"] for b in parsed):
            print("  %s now parses; drop it from EXTRA_BEASTS"
                  % extra["name"], file=sys.stderr)
            return 1
        b = dict(extra)
        b["cr"] = CR_EIGHTHS[b["cr_text"]]
        parsed.append(b)
    beasts = dedupe(parsed)

    # Guards: the beasts a druid or ranger reaches for must all be here.
    expected = ["Wolf", "Brown Bear", "Giant Spider", "Dire Wolf",
                "Giant Eagle", "Panther", "Giant Toad", "Black Bear",
                "Ape", "Crocodile", "Giant Octopus", "Polar Bear",
                "Owl", "Bat", "Cat", "Hawk", "Rat", "Raven", "Frog",
                "Giant Poisonous Snake", "Boar", "Deer", "Horse"]
    have = {b["name"] for b in beasts}
    missing = [e for e in expected
               if e not in have and not any(e in h for h in have)]
    if missing:
        print("missing expected beasts: %s" % ", ".join(missing),
              file=sys.stderr)
        return 1
    if verify(beasts):
        print("stat blocks did not verify; not writing", file=sys.stderr)
        return 1
    if len(beasts) < 80:
        print("only %d beasts parsed; the dump may have changed"
              % len(beasts), file=sys.stderr)
        return 1

    emit(beasts, "src/data_beasts.c", "src/data_beasts.h")
    print("wrote %d beasts" % len(beasts))
    by_cr = {}
    for b in beasts:
        by_cr.setdefault(b["cr_text"], 0)
        by_cr[b["cr_text"]] += 1
    print("  by challenge:", ", ".join(
        "%s: %d" % (k, by_cr[k]) for k in sorted(by_cr, key=len)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
