#!/usr/bin/env python3
"""Corrupt the files the program reads, and check it survives reading them.

Two files come from outside the program and are parsed rather than trusted:
the machine-readable block of a saved character, which the README invites the
player to edit by hand, and homebrew.txt, which a DM writes. Both are
line-oriented and '|' separated, and both are read into fixed-size arrays and
fixed-size struct members. That is exactly the shape of parser that a missing
bound turns into a memory bug, and neither had anything trying to break it.

The mutations are the ones a hand-edit or a half-written file actually
produces: a field dropped, a record repeated more times than the array it
fills can hold, a number that is negative or absurd, a string longer than the
member it is copied into, an unbalanced separator, a block that stops in the
middle. Each mutant is fed back to the program, which must either read it or
say it cannot -- what it must not do is crash, hang, or write outside an array.

Run it against a sanitizer build (`make asan`) to get the most out of it:
a plain build will not notice an overrun of a struct member that lands in the
next one.

Usage:
    fuzz_files.py [--runs N] [--seed N] [--binary PATH] [--keep DIR]
"""

import argparse
import os
import random
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)

import roundtrip                                    # noqa: E402

BEGIN = "#BEGIN-DNDDATA v1"
END = "#END-DNDDATA"

# Values chosen to break a fixed-size, positional parser.
POISON = [
    "", "0", "-1", "-2147483648", "2147483647", "99999999999999999999",
    "1e9", "0x10", " ", "\t", "|", "\\", "\\p", "\\n", "%s%n",
    "x" * 500, "A" * 63, "A" * 64, "A" * 65,
    END, BEGIN, "NAME|spoof", "nonexistent thing", "-",
]


# Records written out by hand, for the shapes a corpus of ordinary characters
# does not contain. A row the loader declines to add is the interesting case:
# whatever it does next must not assume the row went in.
INJECT = [
    "MAGICITEM|0|0|Bag of Holding|0|1|",            # quantity 0: declined
    "MAGICITEM|-1|0|Bag of Holding|0|1|fire",       # negative quantity
    "MAGICITEM|1|1|Bag of Holding|9|1|" + "v" * 200,
    "MAGICITEM|1|0|no such magic item|0|1|",
    "ITEM|0|0|Longsword",
    "ITEM|-5|1|Longsword",
    "ITEM|1|1|no such item",
    "NOTE|title|" + "long " * 500,                  # a note past the line buffer
    "NOTE|" + "t" * 300 + "|body",
    "NOTE|pipes|a\\pb\\pc\\\\d\\ne",
    "SIDEKICK|Sid|no such beast|Warrior|9|0|10|10|10|10|10|10|5|30 ft",
    "SKCHOICE|Sid|label|value",
    "SKSPELL|Sid|Fireball",
    "SKAC|Sid|-99",
    "HPROLLS|-3|1|2|3",
    "HPROLLS|99|1|2|3",
    "CLASS|Fighter|99|-|-1",
    "CLASS|Fighter|-1|Champion|99",
    "SPELL|1|1|Wizard|no such spell",
    "COINS|-1|-1|-1|-1|-1",
    "BODY|-1|-1|-1|eyes|skin|hair",
    "CUSTOMBG|" + "n" * 100 + "|feat|" + "x" * 400 + "|" + "y" * 400,
    "SETTINGS|NOSUCHBOOK|1|1|1|1|1|1",
    "SETTINGS|9|9|9|9|9|9|9|9|9",
]


def split_block(text):
    """The lines before the data block, the records in it, and what follows."""
    lines = text.split("\n")
    try:
        start = lines.index(BEGIN)
    except ValueError:
        return lines, [], []
    try:
        stop = lines.index(END, start)
    except ValueError:
        stop = len(lines) - 1
    return lines[:start + 1], lines[start + 1:stop], lines[stop:]


def mutate(records, rng):
    """One structured corruption of the data block. Returns (name, records)."""
    if not records:
        return "empty", records
    r = list(records)
    i = rng.randrange(len(r))
    kind = rng.choice([
        "drop", "repeat", "poison-field", "drop-field", "add-field",
        "swap", "truncate", "junk", "blank", "tag", "inject",
    ])

    if kind == "drop":
        r.pop(i)
    elif kind == "repeat":
        # Enough copies to overrun any of the fixed arrays: MAX_ITEMS is 96,
        # MAX_SPELLS 128, MAX_PROFS 32, MAX_NOTES 16.
        r[i:i] = [r[i]] * rng.choice([2, 17, 33, 97, 129, 300])
    elif kind == "poison-field":
        f = r[i].split("|")
        if len(f) > 1:
            f[rng.randrange(1, len(f))] = rng.choice(POISON)
        r[i] = "|".join(f)
    elif kind == "drop-field":
        f = r[i].split("|")
        if len(f) > 1:
            del f[rng.randrange(1, len(f))]
        r[i] = "|".join(f)
    elif kind == "add-field":
        r[i] = r[i] + "|" + rng.choice(POISON)
    elif kind == "swap" and len(r) > 1:
        j = rng.randrange(len(r))
        r[i], r[j] = r[j], r[i]
    elif kind == "truncate":
        r = r[:i]
    elif kind == "junk":
        r.insert(i, rng.choice([
            "|||||", "NOTAREALTAG|1|2|3", "|", "\\", "#", " " * 200,
            "CLASS|" + "x" * 400, "SPELL|1|1|Wizard|" + "y" * 400,
        ]))
    elif kind == "blank":
        r.insert(i, "")
    elif kind == "inject":
        r.insert(i, rng.choice(INJECT))
    elif kind == "tag":
        f = r[i].split("|")
        f[0] = rng.choice(["NAME", "CLASS", "ITEM", "SPELL", "NOTE", "SKILL",
                           "SIDEKICK", "MAGICITEM", "HPROLLS", "COINS",
                           "BODY", "SETTINGS", "CHOICE"])
        r[i] = "|".join(f)
    return kind, r


HOMEBREW_LINES = [
    "ITEM|Thing|gear|150|20|0|0|0|0|||",
    "MAGICITEM|Doohickey|wondrous item|rare|0||It does a thing.",
    "SPELL|Zap|1|Evocation|0|0|1 action|60 feet|V, S|Instantaneous|0|Wizard|"
    "It zaps.",
]


def mutate_homebrew(rng):
    """A homebrew file built out of plausible and implausible lines."""
    out = []
    for _ in range(rng.randint(1, 12)):
        line = rng.choice(HOMEBREW_LINES)
        if rng.random() < 0.8:
            f = line.split("|")
            f[rng.randrange(len(f))] = rng.choice(POISON)
            line = "|".join(f)
        out.append(line)
    if rng.random() < 0.3:
        out.append(rng.choice(["", "|", "#", "x" * 2000, "ITEM|"]))
    return "\n".join(out) + "\n"


# Entry 3 of the main menu views a saved character. The input ends after the
# file name rather than counting the menu's entries to find Quit: the program
# reports the closed input on stderr and exits 1, which looks_bad() below
# treats as the clean end it is.
CLOSED = "Input ended unexpectedly"


def run_view(binary, workdir, filename, timeout):
    """Reads the file back through the program's view mode."""
    return subprocess.run([binary, "--seed", "1"],
                          input=("3\n%s\n" % filename).encode(),
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          cwd=workdir, timeout=timeout)


def looks_bad(proc):
    """What counts as a failure, as opposed to a file the program refuses."""
    err = proc.stderr.decode("utf-8", "replace")
    if proc.returncode < 0:
        return "killed by signal %d" % -proc.returncode
    if proc.returncode == 1 and CLOSED in err:
        pass                                    # the input simply ran out
    elif proc.returncode != 0:
        return "exit %d" % proc.returncode
    for marker in ("AddressSanitizer", "UndefinedBehaviorSanitizer",
                   "runtime error:", "Invalid read", "Invalid write",
                   "*** stack smashing", "munmap_chunk", "free(): ",
                   "malloc(): ", "Assertion"):
        if marker in err:
            return err.strip().split("\n")[0][:200]
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=300)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--binary", default="./dndcreator")
    ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--corpus", metavar="DIR",
                    help="mutate the saved characters in DIR instead of the "
                         "two in examples/. Sheets kept by tools/stress.py "
                         "carry magic items, sidekicks and notes, which the "
                         "examples do not.")
    ap.add_argument("--keep", metavar="DIR",
                    help="write every mutant that failed into DIR")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    binary = os.path.abspath(args.binary)
    if args.corpus:
        seeds = sorted(os.path.join(args.corpus, f)
                       for f in os.listdir(args.corpus)
                       if f.endswith(".txt") and f != "homebrew.txt")
    else:
        seeds = [os.path.join(ROOT, "examples", f)
                 for f in ("Bruenor.txt", "Artificer.txt")]
    corpus = []
    for path in seeds:
        text = open(path, encoding="utf-8", errors="replace").read()
        head, records, tail = split_block(text)
        if not records:
            print("%s has no data block; skipping" % path)
            continue
        corpus.append((os.path.basename(path), head, records, tail))
    if not corpus:
        print("no seed characters to mutate")
        return 1

    if args.keep:
        os.makedirs(args.keep, exist_ok=True)
    failures = 0
    kinds = {}

    for run in range(args.runs):
        rng = random.Random(args.seed + run)
        name, head, records, tail = rng.choice(corpus)
        kind, mutated = mutate(records, rng)
        kinds[kind] = kinds.get(kind, 0) + 1
        text = "\n".join(head + mutated + tail)

        with tempfile.TemporaryDirectory() as wd:
            fn = "mutant.txt"
            with open(os.path.join(wd, fn), "w", encoding="utf-8") as fh:
                fh.write(text)
            if rng.random() < 0.4:
                with open(os.path.join(wd, "homebrew.txt"), "w",
                          encoding="utf-8") as fh:
                    fh.write(mutate_homebrew(rng))
            try:
                proc = run_view(binary, wd, fn, args.timeout)
            except subprocess.TimeoutExpired:
                failures += 1
                print("seed %d (%s of %s): timed out"
                      % (args.seed + run, kind, name))
                if args.keep:
                    shutil.copy(os.path.join(wd, fn),
                                os.path.join(args.keep,
                                             "timeout-%d.txt" % (args.seed + run)))
                continue

            bad = looks_bad(proc)
            if bad:
                failures += 1
                print("seed %d (%s of %s): %s"
                      % (args.seed + run, kind, name, bad))
                if args.keep:
                    shutil.copy(os.path.join(wd, fn),
                                os.path.join(args.keep,
                                             "fail-%d.txt" % (args.seed + run)))
                    hb = os.path.join(wd, "homebrew.txt")
                    if os.path.exists(hb):
                        shutil.copy(hb, os.path.join(
                            args.keep, "fail-%d-homebrew.txt" % (args.seed + run)))
            elif args.verbose:
                print("seed %d (%s of %s): ok" % (args.seed + run, kind, name))

    print("\nmutations: %s"
          % ", ".join("%s %d" % kv for kv in sorted(kinds.items())))
    print("%d/%d mutants read without incident." % (args.runs - failures,
                                                    args.runs))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
