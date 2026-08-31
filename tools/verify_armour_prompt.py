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


def build(binary, workdir, cls, category, item, answer, seed, race=None):
    """Builds one character who buys `item`, answering the proficiency
    question with `answer` ("y", "n" or None when none is expected).

    Returns (the question as it was asked or None, the saved sheet)."""
    proc = subprocess.Popen([binary, "--seed", str(seed)],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, bufsize=0, cwd=workdir)
    rng = random.Random(seed)
    want = {"class": cls, "level": 1}
    if race:
        want["race"] = race
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
            # The shop draws two menus that both stay in the window; the one
            # being answered is whichever was drawn last.
            shop = drive.menu_on_screen(tail, "Category:", "Item:")

            if ASK in prompt:
                if not YESNO.search(prompt):
                    raise SystemExit(
                        "verify_armour_prompt: the question does not end in "
                        "a yes/no bracket, so no harness can answer it:\n  %s"
                        % prompt)
                asked = prompt.strip()
                reply = answer or "n"
            elif "Is that right?" in tail and "Choose" in prompt:
                reply = drive.menu_number_in(tail, "Is that right?",
                                             "Save this character") or "1"
            elif "Buy or add equipment?" in prompt:
                reply = "n" if bought else "y"
            elif shop == "Category:" and not bought:
                reply = drive.menu_number_in(tail, "Category:", category)
            elif shop == "Item:" and not bought:
                reply = drive.menu_number_in(tail, "Item:", item)
                if not reply:
                    raise SystemExit("verify_armour_prompt: %s is not on "
                                     "the %s shelf" % (item, category))
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


def wear_from_inventory(binary, workdir, name, item, answer, seed):
    """Loads a saved character, wears `item` from the inventory screen and
    answers the proficiency question with `answer`.

    The wizard is not the only way armour goes on: this screen is, and a
    character loaded from a file reaches it without passing through the
    wizard at all. Returns (the question as asked or None, the saved sheet).
    """
    proc = subprocess.Popen([binary, "--seed", str(seed)],
                            stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, bufsize=0, cwd=workdir)
    rng = random.Random(seed)
    transcript = []
    asked = None
    phase = "menu"
    steps = 0

    try:
        while True:
            prompt = drive.read_prompt(proc, transcript)
            if prompt is None:
                break
            steps += 1
            if steps > 400:
                raise SystemExit("verify_armour_prompt: lost in the "
                                 "inventory screens")
            tail = "".join(transcript[-8:])[-4000:]
            main = ("What would you like to do?" in tail
                    and re.search(r"Choose \[1-\d+\]", prompt))
            reply = None

            if ASK in prompt:
                if not YESNO.search(prompt):
                    raise SystemExit(
                        "verify_armour_prompt: the inventory screen's "
                        "question does not end in a yes/no bracket:\n  %s"
                        % prompt)
                asked = prompt.strip()
                reply, phase = answer, "leave"
            elif phase == "menu" and main:
                reply = drive.menu_number_in(
                    tail, "What would you like to do?",
                    "Manage a character's inventory")
                phase = "name"
            elif phase == "name" and "Character name" in prompt:
                reply, phase = name, "inventory"
            elif phase == "inventory" and "Inventory:" in tail:
                reply = drive.menu_number_in(tail, "Inventory:",
                                             "Wear or take off armor")
                phase = "wear"
            elif phase == "wear" and "Wear or take off which?" in tail:
                reply = drive.menu_number_in(
                    tail, "Wear or take off which?", item)
                if not reply:
                    raise SystemExit("verify_armour_prompt: %s is not on the "
                                     "wear menu" % item)
                # On to leaving whether or not the question comes: if it
                # does not, the caller's "was it asked" check is the one
                # that should report it, not a run that wanders.
                phase = "leave"
            elif phase == "leave" and "Wear or take off which?" in tail:
                reply = drive.menu_number_in(
                    tail, "Wear or take off which?", "Done")
                phase = "done"
            elif phase == "done" and "Inventory:" in tail:
                reply = drive.menu_number_in(tail, "Inventory:", "Done")
                phase = "save"
            elif phase == "save" and "Save the changes?" in prompt:
                reply, phase = "y", "out"
            elif phase == "out" and main:
                m = re.search(r"Choose \[1-(\d+)\]", prompt)
                reply = m.group(1)              # Quit
            if reply is None:
                reply = drive.answer(prompt, rng, name, None, tail)
            proc.stdin.write((reply + "\n").encode())
            proc.stdin.flush()
    except BrokenPipeError:
        pass

    proc.communicate(timeout=120)
    if proc.returncode != 0:
        raise SystemExit("verify_armour_prompt: the program exited %d"
                         % proc.returncode)
    sheet = open(os.path.join(workdir, name + ".txt"), encoding="utf-8").read()
    return asked, sheet


MOD = r"%s\s+\d+\s+\(([-+]\d+)\)"


def sheet_mod(sheet, ability):
    m = re.search(MOD % ability, sheet)
    if not m:
        raise SystemExit("verify_armour_prompt: no %s on the sheet" % ability)
    return int(m.group(1))


def armour_beats_bare_skin(binary, workdir, seeds):
    """A barbarian buys hide armour, and wears it only when it helps.

    Unarmored Defense is 10 + Dexterity + Constitution and hide armour is
    12 + up to 2 of Dexterity, so which is better depends on the character:
    the wizard used to put the hide on either way, because it compared
    armour only against other armour. Rolled scores differ from seed to
    seed, so every run is checked against its own numbers, and the sheet's
    own Armor Class has to agree with the choice.

    A human is asked for so that no natural armour, no shell and no feat is
    in the way of the arithmetic.
    """
    both = set()
    for seed in seeds:
        asked, sheet = build(binary, workdir, "Barbarian", "Medium armor",
                             "Hide armor", None, seed, race="Human")
        if asked:
            raise SystemExit("a barbarian was asked about hide armour they "
                             "are proficient with:\n  %s" % asked)
        carried, equipped = worn(sheet, "Hide armor")
        if not carried:
            raise SystemExit("seed %d: the barbarian never bought the hide "
                             "armor" % seed)
        if worn(sheet, "Shield")[1]:
            continue            # a shield is +2 either way; skip the seed

        dex = sheet_mod(sheet, "Dexterity")
        con = sheet_mod(sheet, "Constitution")
        bare = 10 + dex + con
        hide = 12 + (dex if dex < 2 else 2)
        want = hide > bare

        if equipped != want:
            raise SystemExit(
                "seed %d: bare skin is %d and the hide is %d, so it should "
                "be %s, and the sheet says %s"
                % (seed, bare, hide, "worn" if want else "off",
                   "worn" if equipped else "off"))
        m = re.search(r"Armor Class\s+(\d+)", sheet)
        if not m:
            raise SystemExit("seed %d: no Armor Class on the sheet" % seed)
        if int(m.group(1)) != max(bare, hide):
            raise SystemExit("seed %d: Armor Class %s where %d is the better "
                             "of %d and %d"
                             % (seed, m.group(1), max(bare, hide), bare, hide))
        both.add(want)

    # Both outcomes have to turn up, or the check is only watching one of
    # them and would pass over a program that always did the same thing.
    if len(both) < 2:
        raise SystemExit("these seeds only ever %s the hide armour, so the "
                         "choice is not being tested"
                         % ("wore" if True in both else "left off"))


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

        # The other way armour goes on: the inventory screen, reached from
        # the main menu with a character loaded from a file.
        asked, sheet = build(binary, workdir, "Wizard", "Light armor",
                             "Padded armor", "n", 41)
        name = re.search(r"^NAME\|(.+)$", sheet, re.M)
        if not name:
            raise SystemExit("the saved sheet has no name to load it by")
        name = name.group(1)

        asked, sheet = wear_from_inventory(binary, workdir, name,
                                           "Padded armor", "n", 53)
        if not asked:
            raise SystemExit("the inventory screen equipped armour the "
                             "character cannot use without asking")
        carried, equipped = worn(sheet, "Padded armor")
        if equipped:
            raise SystemExit("the inventory screen said no and wore it "
                             "anyway")

        asked, sheet = wear_from_inventory(binary, workdir, name,
                                           "Padded armor", "y", 67)
        carried, equipped = worn(sheet, "Padded armor")
        if not equipped:
            raise SystemExit("the inventory screen said yes and did not "
                             "wear it")

        # Armour a character can wear still has to beat wearing none.
        armour_beats_bare_skin(binary, workdir, (3, 8, 15, 22, 29, 34))

    print("armour a character cannot use is offered, not assumed, and "
          "armour that loses to bare skin stays off")


if __name__ == "__main__":
    main()
