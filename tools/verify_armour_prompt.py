#!/usr/bin/env python3
"""Buy armour a character cannot use, and check what the program does.

auto_equip() puts the best armour a character owns onto them, and "best"
used to mean the highest Armor Class whether or not they were proficient
with it -- so a wizard who bought a suit of plate wore it, and the sheet
showed the Armor Class without any of the cost. It now asks first.

This drives the real wizard to the shop, buys armour the character has no
proficiency with, answers the question both ways, and reads the saved sheet
to see what is actually worn. The sheet is the ground truth: it records each
item as ITEM|quantity|equipped|name, so "worn" is not a matter of what the
screen said.

A character who IS proficient with what they bought must not be asked at
all, which is the other half of the check.
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

ASK = "You are not proficient with it and so will be hindered by it."

# The prompt has to end in the bracket tools/drive.py looks for, or no
# harness would ever see it as a question.
YESNO = re.compile(r"\[(?:Y/n|y/N)\]:\s*$")


def build(binary, workdir, cls, category, item, answer, seed):
    """Builds one character who buys `item`, answering the proficiency
    question with `answer` ("y", "n" or None when none is expected).

    Returns (the question as it was asked or None, the saved sheet)."""
    proc = subprocess.Popen([binary, "--seed", str(seed)],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, bufsize=0, cwd=workdir)
    rng = random.Random(seed)
    want = {"class": cls, "level": 1}
    transcript = []
    asked = None
    bought = False
    steps = 0

    try:
        while True:
            prompt = drive.read_prompt(proc, transcript)
            if prompt is None:
                break
            steps += 1
            if steps > 3000:
                raise SystemExit("verify_armour_prompt: %s never finished" % cls)
            tail = "".join(transcript[-8:])[-4000:]

            if ASK in prompt:
                if not YESNO.search(prompt):
                    raise SystemExit(
                        "verify_armour_prompt: the question does not end in "
                        "a yes/no bracket, so no harness can answer it:\n  %s"
                        % prompt)
                asked = prompt.strip()
                reply = answer or "n"
            elif "Is that right?" in tail and "Choose" in prompt:
                reply = drive.menu_number(tail, "Save this character") or "1"
            elif "Buy or add equipment?" in prompt:
                reply = "n" if bought else "y"
            elif "Category:" in tail and "Done shopping" in tail and not bought:
                reply = drive.menu_number(tail, category)
            elif ("Item:" in tail and "Look up" not in prompt and not bought
                  and drive.menu_number(tail, item)):
                reply = drive.menu_number(tail, item)
            elif "Buy it?" in prompt and not bought:
                reply = "y"
            elif "Add it anyway" in prompt and not bought:
                reply = "y"
            elif prompt.lstrip().startswith("Quantity") and not bought:
                reply = "1"
                bought = True
            elif "What would you like to do" in tail:
                m = re.search(r"Choose \[1-(\d+)\]", prompt)
                if m:
                    saves = sum(t.count("Saved to") for t in transcript)
                    reply = "1" if saves == 0 else m.group(1)
                else:
                    reply = drive.answer(prompt, rng, "Armourtest", want, tail)
            else:
                reply = drive.answer(prompt, rng, "Armourtest", want, tail)

            proc.stdin.write((reply + "\n").encode())
            proc.stdin.flush()
    except BrokenPipeError:
        pass

    proc.communicate(timeout=300)
    whole = "".join(transcript)
    m = re.search(r"Saved to (\S+)", whole)
    if not m:
        raise SystemExit("verify_armour_prompt: %s was never saved" % cls)
    sheet = open(os.path.join(workdir, m.group(1)), encoding="utf-8").read()
    return asked, sheet


def worn(sheet, item):
    """(carried, equipped) for one item on a saved sheet."""
    for line in sheet.split("\n"):
        parts = line.split("|")
        if len(parts) >= 4 and parts[0] == "ITEM" and parts[3] == item:
            return True, parts[2] == "1"
    return False, False


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--binary", default="./dndcreator")
    args = ap.parse_args()
    binary = os.path.abspath(args.binary)

    with tempfile.TemporaryDirectory() as workdir:
        # A wizard is proficient with no armour at all, so the cheapest suit
        # in the shop is enough to raise the question.
        asked, sheet = build(binary, workdir, "Wizard", "Light armor",
                             "Padded armor", "n", 11)
        if not asked:
            raise SystemExit("a wizard wearing padded armor was never asked "
                             "about it")
        carried, equipped = worn(sheet, "Padded armor")
        if not carried:
            raise SystemExit("the wizard never bought the armor")
        if equipped:
            raise SystemExit("the wizard said no and is wearing it anyway")

        asked_yes, sheet = build(binary, workdir, "Wizard", "Light armor",
                                 "Padded armor", "y", 23)
        if not asked_yes:
            raise SystemExit("the question is not asked every time")
        carried, equipped = worn(sheet, "Padded armor")
        if not (carried and equipped):
            raise SystemExit("the wizard said yes and is not wearing it")

        # A fighter is proficient with all armour, so buying the same suit
        # must raise no question at all.
        quiet, sheet = build(binary, workdir, "Fighter", "Light armor",
                             "Padded armor", None, 37)
        if quiet:
            raise SystemExit("a fighter was asked about armour they can "
                             "wear:\n  %s" % quiet)

    print("armour a character cannot use is offered, not assumed")


if __name__ == "__main__":
    main()
