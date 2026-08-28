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

RANGE_RE = re.compile(r"\[(-?\d+)-(-?\d+)\]:\s*$")
YESNO_RE = re.compile(r"\[(?:Y/n|y/N)\]:\s*$")
SAVED_RE = re.compile(r"Saved to (.+\.txt)")

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


def read_prompt(proc, transcript, limit, deadline):
    """Reads stdout until the program blocks on input, or EOF.

    Descriptive text is full of colons, so ending in ': ' is not on its own a
    prompt; the program flushes and then blocks, so a real prompt is a ': '
    after which nothing more arrives. Same test drive.py makes.
    """
    buf = bytearray()
    fd = proc.stdout.fileno()
    while True:
        if deadline and time.time() > deadline:
            raise RuntimeError("session ran past its deadline")
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
        if len(buf) > limit:
            raise RuntimeError("runaway output")


class Session:
    """One run of the program, and the state the answers depend on."""

    def __init__(self, rng, ops, nasty_odds):
        self.rng = rng
        self.ops_left = ops
        self.nasty_odds = nasty_odds
        self.name = rng.choice(NAMES) + str(rng.randint(1, 9999))
        self.saved = []         # file names the program says it has written
        self.lines_typed = 0    # lines given to the text block in hand

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
        # Weighted so the screens that carry state -- level up, inventory,
        # homebrew -- come up more often than the read-only ones.
        pick = self.rng.choices(
            ["1", "2", "3", "4", "5", "6", "7", "8", "9"],
            weights=[2, 4, 2, 3, 2, 4, 3, 4, 3])[0]
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
        if "name" in low and "spell" not in low:
            return self.nasty_or(self.name)
        if "name a tool" in low:
            return "Smith's tools"
        if "type it" in low or "name one your table uses" in low:
            return self.nasty_or("Table's own option")
        # A blank line is what ends a text block and what accepts a default,
        # so it has to stay the common answer.
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
            tail = "".join(transcript[-4:])[-600:]
            if "What would you like to do" in tail and lo == 1:
                return self.main_menu(hi, transcript)
            if lo == 1 and hi == 20 and "level" in prompt.lower():
                return str(self.rng.choice([1, 1, 2, 3, 5, 8, 11, 14, 17, 20]))
            # An answer outside the stated bounds must be refused and the
            # question asked again; that path is worth exercising too.
            if self.rng.random() < 0.05:
                return str(self.rng.choice([lo - 1, hi + 1, 0, -7, 10 ** 9]))
            return str(self.rng.randint(lo, hi))
        if YESNO_RE.search(prompt):
            return self.rng.choice(["y", "n", "", "Y", "N", "maybe"])
        return self.text(prompt)


def run_once(binary, seed, ops, nasty_odds, use_valgrind, workdir,
             max_prompts, limit, seconds):
    cmd = [binary, "--seed", str(seed)]
    if use_valgrind:
        cmd = ["valgrind", "--error-exitcode=99", "--quiet",
               "--errors-for-leak-kinds=none"] + cmd

    proc = subprocess.Popen(cmd, stdin=subprocess.PIPE,
                            stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                            bufsize=0, cwd=workdir)
    rng = random.Random(seed)
    session = Session(rng, ops, nasty_odds)
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
            prompt = read_prompt(proc, transcript, limit, deadline)
            if prompt is None:
                break
            steps += 1
            if steps > max_prompts:
                raise RuntimeError("more than %d prompts: not making progress"
                                   % max_prompts)
            for m in SAVED_RE.finditer("".join(transcript[-3:])):
                f = os.path.basename(m.group(1))
                if f not in session.saved:
                    session.saved.append(f)
            reply = session.answer(prompt, transcript)
            replies.append(reply)
            proc.stdin.write((reply + "\n").encode())
            proc.stdin.flush()
    except BrokenPipeError:
        pass
    except RuntimeError:
        proc.kill()
        proc.communicate()
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
    proc = subprocess.run([binary, "--seed", "1"],
                          input=("3\n%s\n" % filename).encode(),
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          cwd=workdir, timeout=300)
    err = proc.stderr.decode("utf-8", "replace")
    ok = proc.returncode == 0 or (proc.returncode == 1 and CLOSED in err)
    return proc.stdout.decode("utf-8", "replace"), 0 if ok else proc.returncode


def check_roundtrip(binary, workdir, produced):
    """Every sheet the session wrote must reprint exactly as it was stored.

    The same comparison tools/roundtrip.py makes, and it borrows that
    script's sheet_of() so the two cannot drift apart.
    """
    problems = []
    for f in produced:
        path = os.path.join(workdir, f)
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
                    args.seconds)
            except Exception as exc:            # noqa: BLE001
                print("seed %d: harness error: %s" % (seed, exc))
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
