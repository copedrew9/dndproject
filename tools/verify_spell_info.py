#!/usr/bin/env python3
"""Ask a real spell prompt about a spell and check what comes back.

Every menu answers "N info", and until now a spell menu answered it with
"There is nothing more to say about 3." -- the one menu in the program where
the line on screen (a name and a school) is least of what the choice turns
on. This drives the wizard as far as its spell steps, types "1 info" at each
of them, and checks that the reply carries the whole spell: what it does,
how long it takes to cast, its range, components and duration, whether it is
a ritual, whether it needs concentration, and whose list it is on.

It runs the program the way a player does, so it covers the menu plumbing --
the info list reaching ui_int -- and not only the string that builds it.
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

# The prompts pick_spells writes, by the word it is given for what is being
# chosen. "replacement" belongs to the swap on level-up.
SPELL_PROMPTS = ("Choose cantrip", "Choose spell", "Choose replacement")

WANT = [
    ("Wizard", 5, None),
    ("Cleric", 3, None),
    ("Warlock", 3, None),
    ("Bard", 4, None),
    ("Rogue", 3, "Arcane Trickster"),
]

MUST_HAVE = ("Casting time:", "Range:", "Duration:", "Components:",
             "On the spell list of:")


def a_spell_prompt(tail):
    return any(p in tail for p in SPELL_PROMPTS)


def run(binary, cls, level, subclass, seed, workdir):
    """Builds one character, asking every spell prompt about its first
    entry. Returns how many answers were checked."""
    proc = subprocess.Popen([binary, "--seed", str(seed)],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, bufsize=0, cwd=workdir)
    rng = random.Random(seed)
    want = {"class": cls, "level": level}
    if subclass:
        want["subclass"] = subclass
    transcript = []
    asked = 0
    checked = 0
    steps = 0
    pending = False

    try:
        while True:
            prompt = drive.read_prompt(proc, transcript)
            if prompt is None:
                break
            steps += 1
            if steps > 4000:
                raise RuntimeError("too many prompts")
            tail = "".join(transcript[-6:])[-4000:]

            if pending:
                # The reply to "1 info" is printed and the same prompt comes
                # round again, so what arrived is the answer.
                pending = False
                said = transcript[-1]
                missing = [w for w in MUST_HAVE if w not in said]
                if missing:
                    raise SystemExit(
                        "%s %d: a spell prompt answered 'info' without %s\n%s"
                        % (cls, level, ", ".join(missing), said[-800:]))
                if ("Can be cast as a ritual." not in said
                        and "Cannot be cast as a ritual." not in said):
                    raise SystemExit("%s %d: no word on rituals\n%s"
                                     % (cls, level, said[-800:]))
                if ("Requires concentration." not in said
                        and "Needs no concentration." not in said):
                    raise SystemExit("%s %d: no word on concentration\n%s"
                                     % (cls, level, said[-800:]))
                # The description is the line after the class list, and is
                # the whole point of the exercise.
                body = said.split("On the spell list of:")[-1]
                body = body.split("\n", 1)[-1]
                body = body.split("Choose")[0].strip()
                if len(body) < 40:
                    raise SystemExit(
                        "%s %d: the spell has no description\n%s"
                        % (cls, level, said[-800:]))
                checked += 1

            if a_spell_prompt(tail) and ", N info" in prompt and asked < 6:
                asked += 1
                pending = True
                proc.stdin.write(b"1 info\n")
                proc.stdin.flush()
                continue

            menu_size = re.search(r"Choose \[1-(\d+)\]", prompt)
            if menu_size is not None and "What would you like to do" in tail:
                saves = sum(t.count("Saved to") for t in transcript)
                reply = "1" if saves == 0 else menu_size.group(1)
            elif "Is that right?" in tail:
                reply = drive.menu_number(tail, "Save this character") or "1"
            else:
                reply = drive.answer(prompt, rng, "Infoseeker", want, tail)
            proc.stdin.write((reply + "\n").encode())
            proc.stdin.flush()
    except BrokenPipeError:
        pass

    proc.communicate(timeout=300)
    return checked


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", default="./dndcreator")
    args = ap.parse_args()

    binary = os.path.abspath(args.binary)
    total = 0
    with tempfile.TemporaryDirectory() as workdir:
        for seed, (cls, level, subclass) in enumerate(WANT, start=1):
            got = run(binary, cls, level, subclass, seed * 17, workdir)
            if got == 0:
                raise SystemExit(
                    "%s %d never reached a spell prompt to ask about"
                    % (cls, level))
            total += got
    print("%d spell prompts answered 'info' in full" % total)


if __name__ == "__main__":
    main()
