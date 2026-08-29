#!/usr/bin/env python3
"""Wander the whole program at random and check it never falls over.

tools/drive.py answers the creation wizard and stops there, which leaves the
other eight entries of the main menu -- settings, reference, inventory,
sidekicks, homebrew, notes, view, level up -- driven only by whatever the
wizard happens to reach on its way past. This drives all of them, in one
session, in an order the seed decides, so the state each leaves behind is
what the next one starts from: a character saved by the wizard is reloaded
by the inventory screen, the homebrew the DM just added is in the shop when
the next character buys gear, and the books switched off in the settings are
gone from every menu after that.

Three things it looks for.

  A crash. Any non-zero exit, any signal, a timeout, or output that will not
  stop. Sanitizer and valgrind diagnostics on stderr count as failures too,
  so this is the harness to run under `make asan`.

  A prompt that will not take an answer. Every prompt states its own bounds,
  so an answer inside them must be accepted; a menu that re-asks forever
  means the bounds are a lie. A run that spends more than `--max-prompts`
  prompts is a hang, not a slow run.

  A sheet that does not survive being written. After the session, every
  character file it produced is loaded again through the program's own view
  mode and the reprinted sheet compared with the stored one, which is the
  same check tools/roundtrip.py makes -- but on characters that have been
  through the inventory, homebrew and sidekick screens rather than only the
  wizard.

Free text is where a line-oriented save format is easiest to break, so a
share of the text answers are chosen to break it: the separator the format
is built on, the escape that protects it, a name longer than the field it
goes in, a line of control characters. A name that survives the round trip
above has survived being written into the data block and read back.

Everything is a function of the seed, so a failing run replays exactly, and
two builds fed the same seed produce the same transcript -- which is what
makes this an equivalence check as well as a stress test. `--hash` prints a
digest of each run's transcript; if a refactor changes one, it changed
behaviour.

Usage:
    stress.py [--runs N] [--seed N] [--ops N] [--valgrind] [--hash]
"""

import argparse
import hashlib
import os
import random
import re
import select
import shutil
import subprocess
import sys
import tempfile
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import roundtrip                                     # noqa: E402

MENU_LINE = re.compile(r"^\s*(\d+)[.)]\s+(.*)$")


def menu_number(tail, wanted):
    """The number of the menu entry whose text contains `wanted`, or None.

    By what it says rather than where it sits, so adding an option above it
    does not silently change which one this picks.
    """
    for line in tail.splitlines():
        m = MENU_LINE.match(line)
        if m and wanted.lower() in m.group(2).lower():
            return m.group(1)
    return None


RANGE_RE = re.compile(r"\[(-?\d+)-(-?\d+)\]:\s*$")
YESNO_RE = re.compile(r"\[(?:Y/n|y/N)\]:\s*$")
# A character sheet the program says it has written. The homebrew screen
# prints the same sentence about homebrew.txt, which is not a character:
# counting it made main_menu believe a character existed, so the session
# stopped creating one, and the run then failed with "no character file
# written" -- a harness bug reported as the program's.
SAVED_RE = re.compile(r"Saved to (.+\.txt)")
NOT_A_SHEET = ("homebrew.txt",)

# Printed immediately above the main menu and nowhere else, which is what
# makes it a safe way to recognise the main menu.
BANNER = "D&D 5th Edition Character Creator"

NAMES = ["Bruenor", "Lidda", "Tordek", "Mialee", "Jozan", "Vadania",
         "Krusk", "Nebin", "Ember", "Hennet", "Naull", "Alhandra"]

# Text chosen to break a '|' separated, line-oriented save format: the
# separator itself, the escape that protects it, a field longer than the
# struct member it is copied into, and bytes that are not text at all.
NASTY = [
    "a|b|c",
    "back\\slash",
    "pipe|and\\escape",
    "#END-DNDDATA",
    "#BEGIN-DNDDATA v1",
    "NAME|Fake",
    "  leading and trailing  ",
    "x" * 300,
    "Ünïcodé Ñame",
    "tab\there",
    "%s %d %n",
    "-1",
    "0",
    "99999999999999999999",
    "'; DROP TABLE",
    "\x01\x02\x03",
]


def read_prompt(proc, transcript, limit, deadline, grace=0.05, steps=0):
    """Reads stdout until the program blocks on input, or EOF.

    Descriptive text is full of colons, so ending in ': ' is not on its own a
    prompt; the program flushes and then blocks, so a real prompt is a ': '
    after which nothing more arrives. Same test drive.py makes.
    """
    buf = bytearray()
    fd = proc.stdout.fileno()
    while True:
        left = (deadline - time.time()) if deadline else None
        if left is not None and left <= 0:
            raise RuntimeError("session ran past its deadline, %d prompts in"
                               % steps)
        # Waited for rather than read straight, so that the deadline is what
        # ends a session and not the program's willingness to speak. A prompt
        # this cannot recognise leaves the program waiting on input and this
        # waiting on output, and a plain read would then block for as long as
        # the machine stays up -- which it once did, for two hours, over a
        # prompt the program writes "  > " rather than ": ". The count in the
        # message above is what tells the two apart afterwards: a session that
        # stopped at nine prompts is stuck, one that stopped at nine thousand
        # was only long.
        ready, _, _ = select.select([fd], [], [],
                                    1.0 if left is None else min(left, 1.0))
        if not ready:
            continue
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
            ready, _, _ = select.select([fd], [], [], grace)
            if not ready:
                text = buf.decode("utf-8", "replace")
                transcript.append(text)
                return text.rsplit("\n", 1)[-1]
        if len(buf) > limit:
            raise RuntimeError("runaway output")


class Session:
    """One run of the program, and the state the answers depend on."""

    def __init__(self, rng, ops, nasty_odds, tour=False):
        self.rng = rng
        self.tour = tour        # visit every screen in turn, not at random
        self.next_screen = 1
        self.ops_left = ops
        self.nasty_odds = nasty_odds
        self.name = rng.choice(NAMES) + str(rng.randint(1, 9999))
        self.saved = []         # file names the program says it has written
        self.lines_typed = 0    # lines given to the text block in hand
        self.depth = 0          # menus answered since the last main menu

    def main_menu(self, size, transcript):
        """Which entry of the main menu to take next.

        Quit is always the last entry. The screens that ask for a saved
        character are only worth entering once there is one, so the first
        move is always to create.
        """
        if not self.saved:
            return "1"
        self.ops_left -= 1
        if self.ops_left <= 0:
            return str(size)
        if self.tour:
            # Every screen in turn, so that one session touches all of them.
            # Chosen at random, the rarer screens are simply missed: gcov
            # found the whole sidekick screen untouched by fifty sessions.
            if self.next_screen >= size:
                return str(size)
            pick = self.next_screen
            self.next_screen += 1
            return str(pick)
        # Weighted so the screens that carry state -- level up, game mode,
        # inventory, homebrew -- come up more often than the read-only
        # ones. The list runs to one short of the menu's size, and entries
        # past what this knows about are covered by --tour, which walks
        # every screen in turn.
        pick = self.rng.choices(
            ["1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11"],
            weights=[2, 4, 4, 2, 3, 2, 4, 3, 4, 3, 3])[0]
        return pick if int(pick) < size else "1"

    def text(self, prompt):
        low = prompt.lower()
        # One line of a text block. A blank line is what ends it, so a
        # session that only ever typed would never get out of one.
        if prompt.endswith("> "):
            if self.lines_typed >= 3 or self.rng.random() < 0.4:
                self.lines_typed = 0
                return ""
            self.lines_typed += 1
            return self.nasty_or("a line of the note")
        # The screens that open a saved file say so -- "(or a path to the
        # .txt file)" -- which is what tells them from the wizard asking a
        # new character for its name.
        if "path to the" in low:
            # Usually the character just saved; sometimes one that does not
            # exist, which is the error path.
            if self.saved and self.rng.random() < 0.85:
                return self.rng.choice(self.saved)[:-4]
            return "no-such-character"
        if "player name" in low:
            return self.nasty_or("Player")
        # "What is the sidekick called" is a name prompt that does not use
        # the word: answering it with a blank abandoned the whole sidekick
        # flow, which is why gcov found none of it had ever run.
        if ("name" in low and "spell" not in low) or "called" in low:
            return self.nasty_or(self.name)
        if "name a tool" in low:
            return "Smith's tools"
        if "type it" in low or "name one your table uses" in low:
            return self.nasty_or("Table's own option")
        # An empty answer is what accepts a default, and most of the free
        # text the program asks for has one, so it stays the common answer.
        if self.rng.random() < 0.25:
            return self.nasty_or("something")
        return ""

    def nasty_or(self, ordinary):
        if self.rng.random() < self.nasty_odds:
            return self.rng.choice(NASTY)
        return ordinary

    def answer(self, prompt, transcript):
        m = RANGE_RE.search(prompt)
        if m:
            lo, hi = int(m.group(1)), int(m.group(2))
            # Wide enough to hold the whole main menu, banner included.
            # It was 600, which fitted an eleven-entry menu and not a
            # twelve-entry one: the banner fell off the front, every main
            # menu was answered as though it were some other screen, and
            # sessions ended having never created a character -- reported
            # as "no character file written", which reads like the
            # program's fault and is not. Sized from the menu itself, with
            # room for the menu to grow again.
            tail = "".join(transcript[-6:])[-2000:]
            # The main menu is the one under the program's own banner, not
            # merely the one that asks "What would you like to do?". Two
            # screens ask that -- the other is the note editor -- and taking
            # the second for the first was a livelock: it reset the wind-down
            # below to zero every time a note was opened, spent one of the
            # session's operations on a submenu, and then answered that
            # submenu with a strategy written for a ten-entry main menu.
            # Seed 20 spent all twenty thousand of its prompts that way,
            # opening the notes screen, cancelling a note and starting again,
            # and make check reported it as a failure with no clue in it.
            if BANNER in tail and lo == 1:
                self.depth = 0          # a fresh screen, a fresh budget
                return self.main_menu(hi, transcript)
            # The wizard ends by asking whether to keep the character, and
            # only one of the five answers saves. main_menu above creates
            # until something has been saved, so answering this at random
            # would build characters until the session's prompt budget ran
            # out -- the same shape of livelock the note editor caused, and
            # just as unreadable in the failure report.
            if "Is that right?" in tail:
                pick = menu_number(tail, "Save this character")
                if pick:
                    return pick
            if lo == 1 and hi == 20 and "level" in prompt.lower():
                return str(self.rng.choice([1, 1, 2, 3, 5, 8, 11, 14, 17, 20]))
            # An answer outside the stated bounds must be refused and the
            # question asked again; that path is worth exercising too.
            if self.rng.random() < 0.05:
                return str(self.rng.choice([lo - 1, hi + 1, 0, -7, 10 ** 9]))
            # The last entry of a menu is almost always Done, Back or Quit.
            # Answering uniformly meant most visits to a screen left it again
            # at once: measured with gcov, fifty sessions never once reached
            # the sidekick screen's own flows or a homebrew add-flow. So the
            # way out is avoided at first -- and then, as the session goes on,
            # taken more and more often, because a walk that never takes it
            # does not end: the shop and the reference browser will offer
            # their lists for as long as anything keeps answering.
            self.depth += 1
            leave = 0.03 + self.depth / 400.0
            if leave > 0.9:
                leave = 0.9
            if hi > lo and self.rng.random() > leave:
                return str(self.rng.randint(lo, hi - 1))
            return str(self.rng.randint(lo, hi))
        if YESNO_RE.search(prompt):
            return self.rng.choice(["y", "n", "", "Y", "N", "maybe"])
        return self.text(prompt)


def run_once(binary, seed, ops, nasty_odds, use_valgrind, workdir,
             max_prompts, limit, seconds, tour=False, grace=0.05):
    cmd = [binary, "--seed", str(seed)]
    if use_valgrind:
        cmd = ["valgrind", "--error-exitcode=99", "--quiet",
               "--errors-for-leak-kinds=none"] + cmd

    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            bufsize=0, cwd=workdir)
    rng = random.Random(seed)
    session = Session(rng, ops, nasty_odds, tour)
    transcript = []
    replies = []
    steps = 0
    # A session that wanders into the longest menus can take a while,
    # especially under the sanitizers; one that has stopped making progress
    # takes forever. The deadline turns the second into a reported failure
    # rather than a suite that never returns.
    deadline = time.time() + seconds if seconds else 0

    try:
        while True:
            prompt = read_prompt(proc, transcript, limit, deadline, grace,
                                 steps)
            if prompt is None:
                break
            steps += 1
            if steps > max_prompts:
                raise RuntimeError("more than %d prompts: not making progress"
                                   % max_prompts)
            for m in SAVED_RE.finditer("".join(transcript[-3:])):
                f = os.path.basename(m.group(1))
                if f not in session.saved and f not in NOT_A_SHEET:
                    session.saved.append(f)
            reply = session.answer(prompt, transcript)
            replies.append(reply)
            proc.stdin.write((reply + "\n").encode())
            proc.stdin.flush()
    except BrokenPipeError:
        pass
    except RuntimeError as exc:
        proc.kill()
        proc.communicate()
        # Carried out on the exception because main cannot see either of
        # these otherwise: the session that failed never returned them.
        exc.transcript = "".join(transcript)
        exc.replies = replies
        raise

    out, err = proc.communicate(timeout=600)
    transcript.append(out.decode("utf-8", "replace"))
    return proc.returncode, "".join(transcript), err.decode("utf-8", "replace"), replies


# Menu entry 3 views a saved character. Rather than counting the menu's
# entries to find Quit -- a number that moves every time the program grows a
# screen -- the input simply ends after the file name: the program says so on
# stderr and exits 1, which is a clean end, not a failure.
CLOSED = "Input ended unexpectedly"


def view_sheet(binary, workdir, filename):
    """Loads a saved sheet through the program's own view mode.

    Returns what it printed and whether it ended cleanly.
    """
    # The main menu's entry for viewing is found by what it says rather
    # than by a fixed number. It was "3" until game mode was added above
    # it, and a hardcoded number does not fail loudly when a menu grows --
    # it quietly drives a different screen and reports the empty result as
    # a round-trip failure in the character.
    entry = roundtrip.view_entry(binary, workdir)
    proc = subprocess.run([binary, "--seed", "1"],
                          input=("%s\n%s\n" % (entry, filename)).encode(),
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          cwd=workdir, timeout=300)
    err = proc.stderr.decode("utf-8", "replace")
    ok = proc.returncode == 0 or (proc.returncode == 1 and CLOSED in err)
    return proc.stdout.decode("utf-8", "replace"), 0 if ok else proc.returncode


def is_character_file(path):
    """A character file carries the machine-readable block. A sidekick's
    sheet, written out from the sidekick screen, is a printout with no block
    and is not meant to be loaded.

    The whole line has to be the marker, which is the test the program
    itself makes (strcmp against DATA_BEGIN in src/saveload.c). Matching a
    prefix instead read a sidekick NAMED "#BEGIN-DNDDATA v1" as a character
    file, because its printout opens with its own name, and then reported
    the printout's missing data block as a round-trip failure.
    """
    with open(path, encoding="utf-8", errors="replace") as fh:
        return any(line.rstrip("\r\n") == "#BEGIN-DNDDATA v1" for line in fh)


def check_roundtrip(binary, workdir, produced):
    """Every sheet the session wrote must reprint exactly as it was stored.

    The same comparison tools/roundtrip.py makes, and it borrows that
    script's sheet_of() so the two cannot drift apart.
    """
    problems = []
    for f in produced:
        path = os.path.join(workdir, f)
        if not is_character_file(path):
            continue
        stored = roundtrip.sheet_of(open(path, encoding="utf-8",
                                         errors="replace").read())
        shown, rc = view_sheet(binary, workdir, f)
        if rc != 0:
            problems.append("%s: view exited %d" % (f, rc))
            continue
        start = shown.find("=" * 20)
        reloaded = roundtrip.sheet_of(shown[start:] if start >= 0 else "")
        if stored != reloaded:
            first = next((("stored %r != reloaded %r" % (a[:60], b[:60]))
                          for a, b in zip(stored, reloaded) if a != b),
                         "line counts %d vs %d" % (len(stored), len(reloaded)))
            problems.append("%s: sheet changed on reload: %s" % (f, first))
    return problems


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--seed", type=int, default=1)
    ap.add_argument("--runs", type=int, default=20)
    ap.add_argument("--ops", type=int, default=6,
                    help="how many main-menu screens a session visits")
    ap.add_argument("--binary", default="./dndcreator")
    ap.add_argument("--valgrind", action="store_true")
    ap.add_argument("--nasty", type=float, default=0.25,
                    help="share of free-text answers chosen to break the "
                         "save format")
    ap.add_argument("--max-prompts", type=int, default=20000)
    ap.add_argument("--grace", type=float, default=0.05,
                    help="how long to wait, after a ': ', for more output "
                         "before calling it a prompt. The program writes its "
                         "whole screen and then blocks, so the gap it has to "
                         "beat is microseconds; the default is a thousandfold "
                         "margin, and most of a long session's wall clock.")
    ap.add_argument("--tour", action="store_true",
                    help="visit every main-menu screen in turn rather than "
                         "at random, so one session touches all of them")
    ap.add_argument("--seconds", type=int, default=300,
                    help="give up on a session after this long; 0 waits")
    ap.add_argument("--output-limit", type=int, default=40_000_000)
    ap.add_argument("--hash", action="store_true",
                    help="print a digest of each transcript, for comparing "
                         "two builds")
    ap.add_argument("--no-roundtrip", action="store_true")
    ap.add_argument("--keep", metavar="DIR")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()

    binary = os.path.abspath(args.binary)
    if args.keep:
        os.makedirs(args.keep, exist_ok=True)
    failures = 0

    for i in range(args.runs):
        seed = args.seed + i
        with tempfile.TemporaryDirectory() as workdir:
            try:
                rc, transcript, err, replies = run_once(
                    binary, seed, args.ops, args.nasty, args.valgrind,
                    workdir, args.max_prompts, args.output_limit,
                    args.seconds, args.tour, args.grace)
            except Exception as exc:            # noqa: BLE001
                # The failure that says least about itself used to print the
                # least: a deadline or a runaway is exactly where the last few
                # answers and the tail of the transcript say whether the
                # session was stuck or merely long, and neither was shown.
                print("seed %d: harness error: %s" % (seed, exc))
                if args.verbose:
                    print("  replies: %r"
                          % (getattr(exc, "replies", [])[-40:],))
                    print("  tail: ...%s"
                          % getattr(exc, "transcript", "")[-1200:])
                failures += 1
                continue

            problems = []
            if rc != 0:
                problems.append("exit %d%s" % (rc, " (signal %d)" % -rc
                                               if rc < 0 else ""))
            bad_err = [l for l in err.split("\n")
                       if ("Sanitizer" in l or "ERROR:" in l
                           or "Invalid " in l or "definitely lost" in l)]
            if bad_err:
                problems.append("stderr: %s" % bad_err[0][:200])

            produced = sorted(f for f in os.listdir(workdir)
                              if f.endswith(".txt") and f != "homebrew.txt")
            if not produced:
                problems.append("no character file written")
            elif not args.no_roundtrip and not problems:
                problems += check_roundtrip(binary, workdir, produced)

            if args.keep:
                for f in produced:
                    shutil.copy(os.path.join(workdir, f),
                                os.path.join(args.keep, "%d-%s" % (seed, f)))

            if args.hash:
                digest = hashlib.sha256(transcript.encode()).hexdigest()[:16]
                print("seed %-6d %s  %s" % (seed, digest,
                                            ",".join(produced)[:60]))

            if problems:
                failures += 1
                print("seed %d: %s" % (seed, "; ".join(problems)))
                if args.verbose:
                    print("  replies: %r" % (replies[-40:],))
                    print("  tail: ...%s" % transcript[-1200:])
            elif args.verbose:
                print("seed %d: ok (%d sheets)" % (seed, len(produced)))

    print("\n%d/%d sessions survived." % (args.runs - failures, args.runs))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
