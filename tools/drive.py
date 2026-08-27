#!/usr/bin/env python3
"""Drive the character creator through its prompts and answer them.

The program only ever asks for a number in a stated range, a yes/no, or a
line of free text, and every prompt ends in ": " without a newline. That is
enough to answer automatically, so this harness can build hundreds of
different characters and check that none of them crash.

Usage:
    drive.py [--seed N] [--runs N] [--binary PATH] [--valgrind] [--verbose]
"""

import argparse
import os
import random
import re
import select
import subprocess
import sys
import tempfile

RANGE_RE = re.compile(r"\[(-?\d+)-(-?\d+)\]:\s*$")
YESNO_RE = re.compile(r"\[(?:Y/n|y/N)\]:\s*$")

RECORD_TO = None

NAMES = ["Bruenor", "Lidda", "Tordek", "Mialee", "Jozan", "Vadania",
         "Krusk", "Nebin", "Ember", "Hennet", "Naull", "Alhandra"]


def read_prompt(proc, transcript):
    """Reads stdout until the program blocks on input, or EOF.

    Descriptive text is full of colons, so ending in ': ' is not on its own
    a prompt. The program flushes and then blocks, so a real prompt is a
    ': ' after which nothing more arrives.
    """
    buf = []
    fd = proc.stdout.fileno()
    while True:
        ch = proc.stdout.read(1)
        if not ch:
            return None
        buf.append(ch.decode("utf-8", "replace"))
        text = "".join(buf)
        if text.endswith(": "):
            # Nothing further within the grace period means it is waiting.
            ready, _, _ = select.select([fd], [], [], 0.05)
            if not ready:
                transcript.append(text)
                return text.rsplit("\n", 1)[-1]
        if len(buf) > 4_000_000:
            raise RuntimeError("runaway output")


def answer(prompt, rng, free_text_name):
    m = RANGE_RE.search(prompt)
    if m:
        lo, hi = int(m.group(1)), int(m.group(2))
        # Favour level 1-8 characters so runs stay quick, but reach high
        # levels sometimes.
        if lo == 1 and hi == 20 and "level" in prompt.lower():
            return str(rng.choice([1, 1, 2, 3, 5, 8, 11, 14, 17, 20]))
        return str(rng.randint(lo, hi))
    if YESNO_RE.search(prompt):
        return rng.choice(["y", "n", ""])
    low = prompt.lower()
    if "character name" in low or "player name" in low:
        return free_text_name
    if "name a tool" in low:
        return "Smith's tools"
    # Every list the builder offers ends with an entry that lets the answer
    # be typed instead. Answering it exercises that path; an empty reply
    # would just send the menu round again.
    if "type it" in low or "name one your table uses" in low:
        return "Table's own option"
    return ""


def run_once(binary, seed, rng, use_valgrind, workdir, verbose, levelup=False):
    cmd = [binary, "--seed", str(seed)]
    if use_valgrind:
        cmd = ["valgrind", "--error-exitcode=99", "--quiet",
               "--errors-for-leak-kinds=none"] + cmd

    # Unbuffered binary pipes: the prompt test below asks the OS whether more
    # output is pending, and Python's own buffering would hide it, making a
    # colon in the middle of a paragraph look like a prompt.
    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE,
                            bufsize=0, cwd=workdir)
    transcript = []
    replies = []
    name = rng.choice(NAMES) + str(seed)
    steps = 0

    try:
        # Main menu: create a character.
        while True:
            prompt = read_prompt(proc, transcript)
            if prompt is None:
                break
            steps += 1
            if steps > 4000:
                raise RuntimeError("too many prompts")

            # The main menu grows as features are added, so read its size
            # out of the prompt rather than hardcoding it.
            menu_size = re.search(r"Choose \[1-(\d+)\]", prompt)
            # A single read can split the menu from its prompt, so look at
            # the accumulated tail rather than only the last chunk.
            tail = "".join(transcript[-4:])[-600:]
            at_main_menu = (menu_size is not None
                            and "What would you like to do" in tail)
            if at_main_menu:
                saves = sum(t.count("Saved to") for t in transcript)
                wanted = 2 if levelup else 1
                if saves == 0:
                    reply = "1"                 # create
                elif saves < wanted:
                    reply = "2"                 # load and level up
                else:
                    reply = menu_size.group(1)  # quit is always last
            else:
                reply = answer(prompt, rng, name)

            replies.append(reply)
            proc.stdin.write((reply + "\n").encode())
            proc.stdin.flush()
    except BrokenPipeError:
        pass

    out, err = proc.communicate(timeout=300)
    transcript.append(out.decode("utf-8", "replace"))
    if RECORD_TO:
        with open(RECORD_TO, "w") as fh:
            fh.write("\n".join(replies) + "\n")
    return proc.returncode, "".join(transcript), err, name


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--runs", type=int, default=25)
    ap.add_argument("--binary", default="./dndcreator")
    ap.add_argument("--valgrind", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    ap.add_argument("--record", metavar="FILE",
                    help="write the answers given during the run to FILE")
    ap.add_argument("--keep", metavar="DIR",
                    help="copy each produced character sheet into DIR")
    ap.add_argument("--levelup", action="store_true",
                    help="after creating, reload the character and level it up")
    args = ap.parse_args()

    global RECORD_TO
    RECORD_TO = args.record
    binary = os.path.abspath(args.binary)
    failures = 0
    if args.keep:
        os.makedirs(args.keep, exist_ok=True)

    for i in range(args.runs):
        seed = args.seed + i
        rng = random.Random(seed)
        with tempfile.TemporaryDirectory() as workdir:
            try:
                rc, transcript, err, name = run_once(
                    binary, seed, rng, args.valgrind, workdir, args.verbose,
                    args.levelup)
            except Exception as exc:            # noqa: BLE001
                print("run %d (seed %d): harness error: %s" % (i, seed, exc))
                failures += 1
                continue

            produced = [f for f in os.listdir(workdir) if f.endswith(".txt")]

            if rc != 0:
                failures += 1
                print("run %d (seed %d): exit %d" % (i, seed, rc))
                if err and err.strip():
                    print("  stderr: %s" % err.decode("utf-8", "replace")[:2000])
                tail = transcript[-1500:]
                print("  tail: ...%s" % tail.replace("\n", "\n  "))
            elif not produced:
                failures += 1
                print("run %d (seed %d): no character file written" % (i, seed))
            else:
                if args.keep:
                    import shutil
                    for f in produced:
                        shutil.copy(os.path.join(workdir, f),
                                    os.path.join(args.keep, f))
                if args.verbose:
                    print("run %d (seed %d): ok -> %s" % (i, seed, produced[0]))

    print("\n%d/%d runs succeeded." % (args.runs - failures, args.runs))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
