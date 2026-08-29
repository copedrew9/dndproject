#!/usr/bin/env python3
"""Corrupt a shop file and check the program survives reading it.

<shop name>.txt is the third file the program parses rather than trusts. Like
a character sheet and homebrew.txt it is line-oriented, '|' separated, read
into fixed-size arrays and fixed-size struct members, and meant to be edited
by hand -- the shopbuilder writes a readable header above the data block for
exactly that reason. So it wants the same treatment the other two get.

Two ways in, because they are different code paths and only one of them needs
a character:

  the builder   main menu -> Shopbuilder -> Open a shop you saved
  the table     main menu -> Game mode -> a character -> Visit a shop

The second is the one that then walks the shop's lines, prices them against a
purse and looks each book line up in the item banks, so it is the one that
touches most of what a mutant can have broken.

Run it against a sanitizer build (`make asan`) to get the most out of it.

Usage:
    fuzz_shop.py [--runs N] [--seed N] [--binary PATH] [--keep DIR]
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

BEGIN = "#BEGIN-DNDSHOP v1"
END = "#END-DNDSHOP"

SHOP = "fuzzshop"

# A shop the builder itself would write: a book line, a line of the DM's own,
# free stock, unlimited stock, and a note carrying both escaped characters.
BASE = [
    "SHOP|" + SHOP + "|Ivrit the Younger|Smells of pitch\\p and of tar\\\\.",
    "LINE|2|6000|1|1|Chain mail|Off a dead man\\p ask no questions.",
    "LINE|8|2500|-1|0|A map to the old mine|It is not to scale.",
    "LINE|4|0|3|1|Dagger|",
    "LINE|9|150000|-1|1|Thieves' tools|",
]

# Values chosen to break a fixed-size, positional parser. The same list the
# character fuzzer uses, plus the numbers this parser reads as ranges: a
# category off the end of ITEM_CATEGORY_NAME, a stock below the -1 that means
# "plenty", a price past what one coin field holds.
POISON = [
    "", "0", "-1", "-2", "-2147483648", "2147483647", "99999999999999999999",
    "1e9", "0x10", " ", "\t", "|", "\\", "\\p", "\\n", "%s%n",
    "11", "12", "13", "999", "1000", "999999", "1000000",
    "x" * 500, "A" * 63, "A" * 64, "A" * 65,
    END, BEGIN, "SHOP|spoof", "nonexistent thing", "-",
]

# Rows written out by hand, for shapes a well-formed shop does not contain.
# A row the loader declines is the interesting case: whatever it does next
# must not assume the row went in.
INJECT = [
    "LINE|0|0|0|1|Chain mail|",                 # nothing left on the shelf
    "LINE|0|0|-1|1|no such item at all|",       # from_book, but not in a book
    "LINE|-1|-1|-2|-1|Dagger|",                 # every number out of range
    "LINE|99|99999999|99999999|99|Dagger|",
    "LINE|2|6000|1|1|" + "n" * 300 + "|" + "t" * 900,
    "LINE|2|6000|1|1|Chain mail",               # a field short
    "LINE",
    "LINE|||||",
    "SHOP|second shop|keeper|about",            # a second header
    "SHOP",
    "SHOP|" + "n" * 300 + "|" + "k" * 300 + "|" + "a" * 900,
    BEGIN,
    END,
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
    r = list(records)
    kind = rng.choice([
        "drop", "repeat", "poison-field", "drop-field", "add-field",
        "swap", "truncate", "junk", "blank", "tag", "inject", "header",
    ])
    if not r:
        return "empty", r
    i = rng.randrange(len(r))

    if kind == "drop":
        r.pop(i)
    elif kind == "repeat":
        # MAX_SHOP_LINES is 128, so a shop has to survive being told it holds
        # more than that.
        r[i:i] = [r[i]] * rng.choice([2, 17, 129, 130, 500])
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
            "LINE|" + "x" * 900, "SHOP|" + "y" * 900,
        ]))
    elif kind == "blank":
        r.insert(i, "")
    elif kind == "inject":
        r.insert(i, rng.choice(INJECT))
    elif kind == "tag":
        f = r[i].split("|")
        f[0] = rng.choice(["SHOP", "LINE", "ITEM", "NAME", "", "line", "shop"])
        r[i] = "|".join(f)
    elif kind == "header":
        # The markers themselves, which decide whether any of it is read.
        return kind, r          # handled by the caller, which owns them
    return kind, r


def wrap(head, records, tail, rng, kind):
    """The whole file, including the readable half above the data block."""
    if kind == "header":
        head = rng.choice([
            [SHOP, ""],                         # no marker at all
            [SHOP, "", BEGIN.upper()],
            [SHOP, "", BEGIN + " "],            # a marker with a space after
            [SHOP, "", " " + BEGIN],
            [SHOP, "", BEGIN, BEGIN],
        ])
        tail = rng.choice([[END], [], [END.lower()], ["#END"], [END, END]])
    return "\n".join(head + records + tail) + "\n"


# The input ends after the shop name rather than counting menu entries: the
# program reports the closed input on stderr and exits 1, which looks_bad()
# treats as the clean end it is.
CLOSED = "Input ended unexpectedly"


def run_builder(binary, workdir, timeout):
    """Shopbuilder -> Open a shop you saved."""
    return subprocess.run(
        [binary, "--seed", "1"],
        input=("10\n2\n%s\n" % SHOP).encode(),
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        cwd=workdir, timeout=timeout)


def run_visit(binary, workdir, character, timeout, qty):
    """Game mode -> a character -> Visit a shop, then buy the first thing on
    the list over and over as long as the answers last.

    The quantity is the second half of what is being tested: it decides the
    purse arithmetic, the ledger line, whether a book line reaches the
    inventory or a line of the DM's own reaches the valuables, and whether a
    limited stock runs out and leaves the list.
    """
    keys = "3\n%s\n8\n%s\n" % (character, SHOP) + ("1\n%d\n" % qty) * 8
    return subprocess.run(
        [binary, "--seed", "1"],
        input=keys.encode(),
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        cwd=workdir, timeout=timeout)


def rich(src, dst, rng):
    """The seed character with a purse, so that things are actually bought.

    Bruenor as saved carries ten gold, which buys nothing in the shop above
    and leaves the whole purchase path -- the coins, the ledger, the item
    reaching the inventory -- untouched by every run.
    """
    out = []
    for line in open(src, encoding="utf-8", errors="replace"):
        if line.startswith("COINS|"):
            line = "COINS|0|0|0|%d|0\n" % rng.choice([0, 1, 250, 99999,
                                                      999999])
        out.append(line)
    open(dst, "w", encoding="utf-8").write("".join(out))


MARKERS = ("AddressSanitizer", "UndefinedBehaviorSanitizer",
           "runtime error:", "Invalid read", "Invalid write",
           "*** stack smashing", "munmap_chunk", "free(): ",
           "malloc(): ", "Assertion")


def looks_bad(proc):
    """What counts as a failure, as opposed to a file the program refuses.

    The sanitizers are read before the exit status, not after. ASan exits 1,
    which is also what the program does when its input runs out, so an exit
    status checked first turns "global-buffer-overflow in shop_load" into
    "exit 1" and throws away the line that says where.
    """
    err = proc.stderr.decode("utf-8", "replace")
    for marker in MARKERS:
        if marker in err:
            for line in err.strip().split("\n"):
                if marker in line:
                    return line.strip()[:200]
    if proc.returncode < 0:
        return "killed by signal %d" % -proc.returncode
    if proc.returncode == 1 and CLOSED in err:
        return None                             # the input simply ran out
    if proc.returncode != 0:
        return "exit %d" % proc.returncode
    return None


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--runs", type=int, default=200)
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--binary", default="./dndcreator")
    ap.add_argument("--timeout", type=int, default=60)
    ap.add_argument("--keep", metavar="DIR",
                    help="write every mutant that failed into DIR")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    binary = os.path.abspath(args.binary)
    sheet = os.path.join(ROOT, "examples", "Bruenor.txt")
    if not os.path.exists(sheet):
        print("examples/Bruenor.txt is missing; the visit path needs it")
        return 1
    character = os.path.basename(sheet)[:-len(".txt")]

    head = [SHOP, "Kept by Ivrit the Younger", "", BEGIN]
    tail = [END]

    if args.keep:
        os.makedirs(args.keep, exist_ok=True)
    failures = 0
    kinds = {}

    for run in range(args.runs):
        rng = random.Random(args.seed + run)
        kind, mutated = mutate(BASE, rng)
        kinds[kind] = kinds.get(kind, 0) + 1
        text = wrap(head, mutated, tail, rng, kind)

        with tempfile.TemporaryDirectory() as wd:
            with open(os.path.join(wd, SHOP + ".txt"), "w",
                      encoding="utf-8") as fh:
                fh.write(text)
            rich(sheet, os.path.join(wd, os.path.basename(sheet)), rng)

            visiting = rng.random() < 0.6
            qty = rng.choice([1, 2, 99])
            runner = (lambda: run_visit(binary, wd, character, args.timeout,
                                        qty)) \
                if visiting else (lambda: run_builder(binary, wd, args.timeout))
            how = "visit" if visiting else "builder"

            try:
                proc = runner()
            except subprocess.TimeoutExpired:
                failures += 1
                print("seed %d (%s, %s): timed out"
                      % (args.seed + run, kind, how))
                if args.keep:
                    shutil.copy(os.path.join(wd, SHOP + ".txt"),
                                os.path.join(args.keep, "timeout-%d.txt"
                                             % (args.seed + run)))
                continue

            bad = looks_bad(proc)
            if bad:
                failures += 1
                print("seed %d (%s, %s): %s"
                      % (args.seed + run, kind, how, bad))
                if args.keep:
                    shutil.copy(os.path.join(wd, SHOP + ".txt"),
                                os.path.join(args.keep, "fail-%d.txt"
                                             % (args.seed + run)))
            elif args.verbose:
                print("seed %d (%s, %s): ok" % (args.seed + run, kind, how))

    print("\nmutations: %s"
          % ", ".join("%s %d" % kv for kv in sorted(kinds.items())))
    print("%d/%d shops read without incident."
          % (args.runs - failures, args.runs))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
