#!/usr/bin/env python3
"""Check that reloading a saved character reproduces its sheet exactly.

Creates characters, then reads each file back through the program's "view"
mode and compares the sheet it prints against the sheet stored in the file.
Any difference means the machine-readable block lost something that the
level-up path would then get wrong.
"""

import os
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import drive                                        # noqa: E402

DIVIDER = "MACHINE-READABLE DATA"
MENU = "D&D 5th Edition Character Creator"


MENU_LINE = re.compile(r"^\s*(\d+)[.)]\s+(.*)$")


def menu_entry(text, wanted):
    """The number of the main-menu entry whose label contains `wanted`.

    Three harnesses drive the view screen by sending its menu number, and
    all three had that number written into them as "3". Adding game mode
    above it moved it to 4, and a hardcoded number does not fail loudly
    when a menu grows -- it silently drives a different screen and reports
    the empty result as a fault in the character being viewed.
    """
    for line in text.splitlines():
        m = MENU_LINE.match(line)
        if m and wanted.lower() in m.group(2).lower():
            return m.group(1)
    return None


def view_entry(binary, workdir=None):
    """Ask the program itself which entry views a saved character."""
    probe = subprocess.run([binary, "--seed", "1"], input=b"\n",
                           stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                           cwd=workdir, timeout=300)
    return menu_entry(probe.stdout.decode("utf-8", "replace"),
                      "View a saved character") or "3"


def sheet_of(text):
    """Everything above the machine-readable divider.

    When the sheet came from the program's own output it is followed by the
    main menu, which is not part of the sheet.
    """
    out = []
    for line in text.splitlines():
        if DIVIDER in line or MENU in line:
            break
        out.append(line.rstrip())
    # Trim the rule lines and blank padding that introduce whatever follows
    # the sheet -- the divider in the file, the menu in the program's output.
    while out and (not out[-1] or set(out[-1]) <= set("=- ")):
        out.pop()
    return out


def main():
    binary = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else "./dndcreator")
    failures = 0
    checked = 0

    for seed in (11, 22, 33, 44, 55, 66, 77, 88):
        with tempfile.TemporaryDirectory() as wd:
            import random
            rc, _, _, _ = drive.run_once(binary, seed, random.Random(seed),
                                         False, wd, False)
            if rc != 0:
                print("  run for seed %d exited %d" % (seed, rc))
                failures += 1
                continue

            view = view_entry(binary, wd)
            for fn in sorted(os.listdir(wd)):
                if not fn.endswith(".txt"):
                    continue
                path = os.path.join(wd, fn)
                saved = sheet_of(open(path, encoding="utf-8",
                                      errors="replace").read())

                # The view entry is asked for rather than assumed; the
                # trailing 5 falls off the end of the menu and quits.
                proc = subprocess.run(
                    [binary], input="%s\n%s\n5\n" % (view, fn),
                    capture_output=True, text=True, cwd=wd, errors="replace")
                start = proc.stdout.find("=" * 20)
                reloaded = sheet_of(proc.stdout[start:] if start >= 0 else "")

                checked += 1
                if saved == reloaded:
                    print("  ok       %s" % fn)
                else:
                    print("  MISMATCH %s" % fn)
                    failures += 1
                    for a, b in zip(saved, reloaded):
                        if a != b:
                            print("      saved:    %r" % a)
                            print("      reloaded: %r" % b)
                            break
                    if len(saved) != len(reloaded):
                        print("      line counts %d vs %d"
                              % (len(saved), len(reloaded)))

    print("\n%d/%d sheets round-tripped unchanged." % (checked - failures, checked))
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
