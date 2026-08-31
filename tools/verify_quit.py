#!/usr/bin/env python3
"""Type q in the creation wizard and see where it lands you.

"q" is offered at every wizard prompt as "throw this character away". It
used to throw the character away and then start the wizard again from step
0, which is not what a player who types q is asking for: they want out. It
now returns to the main menu, and the one place that does mean start again
-- the confirm screen's own "Throw it away and start again" -- still does.

Both are checked here by driving the real program: after q the next thing on
screen must be the main menu, and no new character may have been started;
after choosing "Throw it away and start again" the wizard must begin again
without passing through the main menu.
"""

import argparse
import os
import random
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive

MAIN_MENU = "What would you like to do?"
FIRST_STEP = "Step 0: Which Books Are In Play"
RESTART = "Throwing that character away and starting again."


def drive_until(binary, workdir, seed, at_prompt):
    """Answers prompts until at_prompt(prompt, tail) returns a reply for a
    prompt it recognises; returns everything printed after that reply."""
    proc = subprocess.Popen([binary, "--seed", str(seed)],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, bufsize=0, cwd=workdir)
    rng = random.Random(seed)
    want = {"class": "Cleric", "level": 1}
    transcript = []
    mark = None
    pending = None
    steps = 0

    try:
        while True:
            prompt = drive.read_prompt(proc, transcript)
            if prompt is None:
                break
            pending = prompt
            steps += 1
            if steps > 2000:
                raise SystemExit("verify_quit: the wizard never finished")
            tail = "".join(transcript[-8:])[-4000:]

            if mark is None:
                special = at_prompt(prompt, tail)
                if special is not None:
                    mark = len("".join(transcript))
                    proc.stdin.write((special + "\n").encode())
                    proc.stdin.flush()
                    continue
            elif MAIN_MENU in tail or FIRST_STEP in tail:
                break               # far enough to see where it landed

            if MAIN_MENU in tail and re.search(r"Choose \[1-(\d+)\]", prompt):
                m = re.search(r"Choose \[1-(\d+)\]", prompt)
                reply = "1" if mark is None else m.group(1)
            else:
                reply = drive.answer(prompt, rng, "Quitter", want, tail)
            proc.stdin.write((reply + "\n").encode())
            proc.stdin.flush()
            pending = None
    except BrokenPipeError:
        pass

    quit_cleanly(proc, transcript, pending)
    if mark is None:
        raise SystemExit("verify_quit: never reached the prompt to test")
    return "".join(transcript)[mark:]


def quit_cleanly(proc, transcript, pending):
    """Leaves by the main menu's own Quit and checks the program agreed.

    Killing the process instead would hide the thing most worth catching: a
    crash, an assertion or a sanitizer abort after the escape. Every string
    this file checks would still be in the transcript, and the check would
    pass over a program that had fallen over.

    `pending` is the prompt the caller has already read and not answered.
    It has to be answered before anything else is read, or both sides wait
    for the other: the program for its reply, and this for output that is
    not coming."""
    rng = random.Random(0)
    try:
        for _ in range(60):
            if pending is not None:
                prompt, pending = pending, None
            else:
                prompt = drive.read_prompt(proc, transcript)
            if prompt is None:
                break
            tail = "".join(transcript[-6:])[-3000:]
            m = re.search(r"Choose \[1-(\d+)\]", prompt)
            if MAIN_MENU in tail and m:
                reply = m.group(1)              # Quit is always last
            elif "(b back, q quit)" in prompt:
                reply = "q"                     # out of the wizard first
            else:
                reply = drive.answer(prompt, rng, "Quitter", None, tail)
            proc.stdin.write((reply + "\n").encode())
            proc.stdin.flush()
    except BrokenPipeError:
        pass

    try:
        proc.communicate(timeout=60)
    except subprocess.TimeoutExpired:
        proc.kill()
        raise SystemExit("verify_quit: the program never exited")
    if proc.returncode != 0:
        raise SystemExit("verify_quit: the program exited %d after the "
                         "escape" % proc.returncode)


def typed_q(prompt, tail):
    """q at the third prompt that offers it.

    Which step that lands on depends on the seed and on how the prompts
    before it were answered, and that is the point: every escape-armed
    prompt has to behave the same way, so the check does not care which one
    it was."""
    if "(b back, q quit)" in prompt:
        typed_q.seen += 1
        if typed_q.seen == 3:
            return "q"
    return None


ROW = "Tasha's optional class features"


def last_row(text):
    """The state of the optional-features checkbox the last time it was
    drawn, "[x]" or "[ ]", or None if it was not."""
    state = None
    for line in text.split("\n"):
        if ROW in line:
            state = "[x]" if "[x]" in line else "[ ]" if "[ ]" in line else None
    return state


def settings_survive(binary, workdir, seed, how):
    """Toggles a rule in the wizard's own step 0, leaves by `how`, and reads
    the main menu's settings screen back.

    Step 0 settles which books this character draws on. The restart path has
    always put them back; both ways of leaving have to as well, or a
    character that was thrown away takes the rest of the session's content
    settings with it. `how` is "q" for a typed escape, or "confirm" for the
    confirm screen's own "Leave without saving" -- the same decision reached
    at the other end of the wizard.
    """
    proc = subprocess.Popen([binary, "--seed", str(seed)],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, bufsize=0, cwd=workdir)
    rng = random.Random(seed)
    want = {"class": "Cleric", "level": 1}
    transcript = []
    phase = "create"
    seen = None
    pending = None
    steps = 0

    try:
        while True:
            prompt = drive.read_prompt(proc, transcript)
            if prompt is None:
                break
            pending = prompt
            steps += 1
            if steps > 2000:
                raise SystemExit("verify_quit: the settings run never "
                                 "finished")
            tail = "".join(transcript[-8:])[-4000:]
            books = "Optional rules" in tail
            menu = MAIN_MENU in tail and re.search(r"Choose \[1-\d+\]", prompt)
            reply = None

            if phase == "create" and menu:
                reply, phase = "1", "toggle"
            elif phase == "toggle" and books:
                reply = drive.menu_number(tail, ROW)
                if not reply:
                    raise SystemExit("verify_quit: no optional-features row")
                phase = "leave"
            elif phase == "leave" and books:
                # The row is off now; leaving must put it back on. Only the
                # last drawing of the row counts: the window holds the
                # screen before the toggle as well as the one after it, so
                # matching anywhere in it would pass whatever happened.
                if last_row(tail) != "[ ]":
                    raise SystemExit("verify_quit: the toggle did nothing")
                if how == "q":
                    reply, phase = "q", "settings"
                else:
                    # Out of the books screen and on with the character, as
                    # far as the confirm screen at the other end.
                    reply = drive.menu_number(tail, "Done") or "15"
            elif (phase == "leave" and how == "confirm"
                  and "Is that right?" in tail and "Choose" in prompt):
                reply = drive.menu_number(tail, "Leave without saving")
                if not reply:
                    raise SystemExit("verify_quit: no leave entry")
                phase = "settings"
            elif phase == "settings" and menu:
                reply = drive.menu_number(tail, "Content settings")
                if not reply:
                    raise SystemExit("verify_quit: no settings entry")
                phase = "read"
            elif phase == "read" and books:
                seen = tail
                break

            if reply is None:
                reply = drive.answer(prompt, rng, "Settler", want, tail)
            proc.stdin.write((reply + "\n").encode())
            proc.stdin.flush()
            pending = None
    except BrokenPipeError:
        pass

    quit_cleanly(proc, transcript, pending)
    if seen is None:
        raise SystemExit("verify_quit: never got back to the settings screen")
    if last_row(seen) != "[x]":
        raise SystemExit("leaving by %s took the session's content settings "
                         "with it:\n%s" % (how, seen[-800:]))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", default="./dndcreator")
    args = ap.parse_args()
    binary = os.path.abspath(args.binary)

    with tempfile.TemporaryDirectory() as workdir:
        typed_q.seen = 0
        after = drive_until(binary, workdir, 5, typed_q)
        if MAIN_MENU not in after:
            raise SystemExit("q did not come back to the main menu:\n%s"
                             % after[-600:])
        if RESTART in after:
            raise SystemExit("q still restarts the wizard")
        if FIRST_STEP in after.split(MAIN_MENU)[0]:
            raise SystemExit("q started another character before the menu")

        # The confirm screen's own entry still means start again.
        def restart_entry(prompt, tail):
            if "Is that right?" in tail and "Choose" in prompt:
                return drive.menu_number(tail, "Throw it away and start again")
            return None

        after = drive_until(binary, workdir, 9, restart_entry)
        if RESTART not in after:
            raise SystemExit("the confirm screen's restart no longer "
                             "restarts:\n%s" % after[-600:])
        if MAIN_MENU in after.split(FIRST_STEP)[0]:
            raise SystemExit("the confirm screen's restart went to the main "
                             "menu instead")

        settings_survive(binary, workdir, 13, "q")
        settings_survive(binary, workdir, 21, "confirm")

    print("q leaves to the main menu, puts the books back, and the confirm "
          "screen still restarts")


if __name__ == "__main__":
    main()
