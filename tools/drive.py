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
    buf = bytearray()
    fd = proc.stdout.fileno()
    while True:
        chunk = os.read(fd, 65536)
        if not chunk:
            return None
        buf += chunk
        # The program flushes and then blocks, so a prompt is a ': ' that
        # nothing follows. Reading in blocks rather than a byte at a time
        # makes no difference to that test -- a ': ' with more text behind it
        # is not at the end of the buffer -- and saves a syscall per byte,
        # which is most of what a run used to spend its time on.
        # "  > " is the other prompt the program writes: ui_text_block asks
        # for line after line that way until a blank one ends it, and a
        # reader that knows only ": " waits forever for a prompt the program
        # has already given.
        if buf.endswith(b": ") or buf.endswith(b"> "):
            ready, _, _ = select.select([fd], [], [], 0.05)
            if not ready:
                text = buf.decode("utf-8", "replace")
                transcript.append(text)
                return text.rsplit("\n", 1)[-1]
        if len(buf) > 4_000_000:
            raise RuntimeError("runaway output")


MENU_LINE = re.compile(r"^\s*(\d+)[.)]\s+(.*)$")


def menu_number(tail, wanted):
    """The number of the menu entry whose text contains `wanted`, or None.

    The menus grow as features are added, so their entries are found by
    what they say rather than by where they sit. Read out of the accumulated
    transcript for the same reason at_main_menu is: one read can split a
    menu from the prompt that follows it.
    """
    for line in tail.splitlines():
        m = MENU_LINE.match(line)
        if m and wanted.lower() in m.group(2).lower():
            return m.group(1)
    return None


def answer(prompt, rng, free_text_name, want=None, tail=""):
    """One reply. `want` is an optional {"class": name, "level": n} that
    aims the run rather than leaving it to the dice, so a caster can be
    built on purpose. Answering at random reaches a wizard about one run in
    thirteen and a wizard of a chosen level far less often, which is why
    nothing here ever exercised the spell steps twice in a row."""
    # A line of a text block. A blank line ends it, which is what this
    # harness wants: the notes screen is not what it is here to exercise.
    if prompt.endswith("> "):
        return ""
    m = RANGE_RE.search(prompt)
    if m:
        lo, hi = int(m.group(1)), int(m.group(2))
        if want:
            # The screen names the step, not the prompt line -- the prompt
            # is only "Choose, N info [1-13]: ". So the aim is taken from
            # the accumulated tail.
            low_tail = tail.lower()
            # Leave the books alone when aiming. They are all on by
            # default, and toggling them at random switches off the very
            # book the wanted race or subclass comes from -- a run aimed at
            # a tortle was offered the nine races of the Player's Handbook
            # and quietly built a dwarf.
            if "source books" in low_tail and "toggle" in prompt.lower():
                done = menu_number(tail, "Done")
                if done:
                    return done
            if want.get("class") and ("classes:" in low_tail
                                      or "which class" in low_tail):
                pick = menu_number(tail, want["class"])
                if pick and lo <= int(pick) <= hi:
                    return pick
            # A subclass is chosen from a menu the step names rather than
            # the prompt. Without this the third casters -- the Eldritch
            # Knight and the Arcane Trickster -- are reachable only by luck,
            # which is why nothing ever exercised their spell steps.
            if want.get("subclass"):
                pick = menu_number(tail, want["subclass"])
                if pick and lo <= int(pick) <= hi:
                    return pick
            if want.get("race") and ("races:" in low_tail
                                     or "which race" in low_tail):
                pick = menu_number(tail, want["race"])
                if pick and lo <= int(pick) <= hi:
                    return pick
            if want.get("level"):
                if lo == 1 and hi == 20 and "level" in prompt.lower():
                    return str(want["level"])
                # "How many levels in it?" -- put them all in the one class,
                # so aiming at a class does not produce a multiclass.
                if "how many levels" in low_tail:
                    return str(hi)
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


def run_once(binary, seed, rng, use_valgrind, workdir, verbose, levelup=False,
             magic=0, quit_at=0, back_at=0, want=None):
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
    got = 0
    tries = 0
    escapes = 0

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
            # Wide enough for the whole main menu with room to grow. It
            # was 600, which the menu came within a hundred characters of:
            # tools/stress.py had the same window and a twelfth entry
            # pushed its own marker off the front, so every main menu was
            # answered as though it were another screen.
            tail = "".join(transcript[-6:])[-2000:]
            at_main_menu = (menu_size is not None
                            and "What would you like to do" in tail)
            # The wizard now ends by asking whether to keep the character,
            # and only one of the five answers saves. Answering it at random
            # leaves saves == 0, which sends the main-menu branch below
            # straight back into "create a character" -- so a harness that
            # did not know about this screen would build characters until it
            # hit the prompt cap and report a timeout rather than a bug.
            # tools/roundtrip.py calls run_once directly and inherits this.
            if "Is that right?" in tail:
                pick = menu_number(tail, "Save this character")
                if pick:
                    replies.append(pick)
                    proc.stdin.write((pick + "\n").encode())
                    proc.stdin.flush()
                    continue

            # b and q are only offered where the wizard can honour them,
            # and the prompt says so. Typing one at the Nth such prompt
            # exercises the jump out of a half-built character: back has to
            # put the previous step's answers back, and quit has to leave
            # nothing of the old character behind. Neither is reachable by
            # answering menus at random, so neither was ever covered.
            if (quit_at or back_at) and "(b back, q quit)" in prompt:
                escapes += 1
                word = None
                if quit_at and escapes == quit_at:
                    word = "q"
                elif back_at and escapes == back_at:
                    word = "b"
                if word:
                    replies.append(word)
                    proc.stdin.write((word + "\n").encode())
                    proc.stdin.flush()
                    continue

            # Magic items are only reachable through the inventory, and
            # the inventory is only offered after a level-up -- creation
            # hands out a class's starting equipment and nothing else. So a
            # corpus built from plain creation runs contains no magic item
            # at all, and every check made against it is silent about them.
            # --magic walks the one path that reaches them.
            if magic and not at_main_menu:
                if "Change what this character is carrying" in prompt:
                    replies.append("y")
                    proc.stdin.write(b"y\n")
                    proc.stdin.flush()
                    continue
                if "Pick up another magic item" in prompt:
                    got += 1
                    reply = "y" if got < magic else "n"
                    replies.append(reply)
                    proc.stdin.write((reply + "\n").encode())
                    proc.stdin.flush()
                    continue
                if "Inventory:" in tail:
                    # The books menu now runs first and the harness answers
                    # it at random, so a session can switch off the book the
                    # magic items come from -- and then no amount of asking
                    # produces one. Give up after a few tries rather than
                    # asking until the prompt cap and reporting a timeout.
                    if got < magic and tries < magic * 4:
                        entry = "Pick up a magic item"
                        tries += 1
                    else:
                        entry = "Done"
                    pick = menu_number(tail, entry)
                    if pick:
                        replies.append(pick)
                        proc.stdin.write((pick + "\n").encode())
                        proc.stdin.flush()
                        continue

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
                reply = answer(prompt, rng, name, want, tail)

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
    ap.add_argument("--quit-at", type=int, default=0, metavar="N",
                    help="type q at the Nth prompt that offers it, then "
                         "build the restarted character to the end")
    ap.add_argument("--back-at", type=int, default=0, metavar="N",
                    help="type b at the Nth prompt that offers it")
    ap.add_argument("--magic", type=int, default=0, metavar="N",
                    help="pick up N magic items on the way through; implies "
                         "--levelup, since the inventory is only offered "
                         "after a level-up")
    ap.add_argument("--class", dest="klass", metavar="NAME",
                    help="build this class every run rather than one the "
                         "dice chose, so a caster can be reached on purpose")
    ap.add_argument("--level", type=int, default=0, metavar="N",
                    help="build at this level every run")
    ap.add_argument("--subclass", metavar="NAME",
                    help="take this subclass wherever it is offered")
    ap.add_argument("--race", metavar="NAME",
                    help="build this race every run")
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
                want = None
                if args.klass or args.level or args.subclass or args.race:
                    want = {"class": args.klass, "level": args.level,
                            "subclass": args.subclass, "race": args.race}
                rc, transcript, err, name = run_once(
                    binary, seed, rng, args.valgrind, workdir, args.verbose,
                    args.levelup or bool(args.magic), args.magic,
                    args.quit_at, args.back_at, want)
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
