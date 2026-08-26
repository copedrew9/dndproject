# dndproject

A D&D 5th Edition character creator and builder, written in C.

It walks the six steps of *Player's Handbook* chapter 1, asks for every choice
those steps require, and writes the finished character to
`<Charactername>.txt`. A saved character can be loaded again and levelled up.

## Building and running

```sh
make            # builds ./dndcreator
./dndcreator    # interactive
```

`./dndcreator --seed N` makes the dice reproducible, which the test harness
relies on.

The main menu offers:

1. **Create a new character** — the full wizard.
2. **Load a character and level up** — reads a saved `.txt` and advances it.
3. **View a saved character** — prints the sheet.

## What it covers

Everything here is *Player's Handbook* content.

- **Races** — all 9, with their subraces, the dragonborn's draconic ancestry,
  the half-elf's floating ability increases and skill versatility, and the
  variant human's ability increases, skill and bonus feat.
- **Classes** — all 12, levels 1–20, with every PHB subclass (40 of them),
  class features at each level, and the subclass sub-choices (totem animal,
  Circle of the Land terrain, draconic bloodline).
- **Ability scores** — standard array, 4d6 drop lowest, 27-point buy, or
  entered by hand.
- **Multiclassing** — full PHB chapter 6 rules: ability prerequisites are
  checked, later classes grant only the reduced multiclass proficiencies, and
  spell slots use the multiclass spellcaster table when more than one class
  grants Spellcasting (and that class's own table when only one does).
- **Ability Score Improvements** — at each ASI level, raise scores or take one
  of the 42 PHB feats, with prerequisites enforced.
- **Spellcasting** — all 361 PHB spells with level, school, ritual flag,
  casting time, range, components, duration and concentration. Cantrips and
  spells known/prepared are enforced per class, domain/oath/circle spells are
  granted automatically, and Pact Magic is tracked separately.
- **Equipment** — the class and background starting packages (resolved into
  real items, including "choose any martial weapon" style entries), or roll
  for starting gold and buy from the full PHB catalogue of armour, weapons,
  gear, tools, packs and mounts, with weight tracked against carrying capacity.
- **Backgrounds** — all 13, with their skills, tools, languages, feature, and
  suggested traits, ideals, bonds and flaws.

Derived numbers are computed rather than typed in: proficiency bonus, ability
modifiers, saving throws, skills (including expertise and the bard's Jack of
All Trades), Armor Class (armour, shields, and the barbarian, monk and
draconic-sorcerer unarmoured options), initiative, speed, passive Perception,
hit points, carrying capacity, spell save DC and spell attack bonus.

## The character file

`<Charactername>.txt` has two halves.

The top is a formatted sheet meant to be read at the table: ability scores,
combat numbers, skills, proficiencies, racial traits, class features, spells
with their full stat lines, equipment and personality.

Below a marked divider is a machine-readable block between
`#BEGIN-DNDDATA v1` and `#END-DNDDATA`. Each line is a `|`-separated record
the loader parses to rebuild the character exactly, which is what makes
levelling up a saved character possible. It stores *names* rather than table
indices, so a file stays valid if the game data is extended, and it can be
edited by hand as long as the field names and separators survive.

## Where the data comes from

`TextFiles/` holds OCR'd text of the sourcebooks.

The spell tables are **extracted** from `TextFiles/PHBtext.txt` by
`tools/extract_spells.py`, which parses all 361 spell stat blocks and the
eight class spell lists into `src/data_spells.{c,h}`:

```sh
make spells     # regenerate src/data_spells.{c,h}
```

The OCR is lossy, so the extractor cleans up after it: it folds the C/G and
I/L confusions and lost spaces when matching list entries to descriptions,
resolves the level of spells whose stat line reads "Sth-level" (the glyph is
ambiguous between 3, 5 and 8) from the class lists, which state levels
explicitly, and locates each class list by heading rather than by line number
because one of them reads "Sorc erer Sp ells". A short, documented repair
table in the script fixes the handful of pages the OCR damaged beyond
matching; every entry in it was checked against the surrounding text. The
script refuses to write output unless all 361 spells parse completely, every
class-list entry matches a spell, and each class's cantrip count is exactly
what the PHB gives it.

One quirk is preserved rather than papered over: *Trap the Soul* appears on
the PHB's wizard spell list but has no spell description anywhere in the book,
so it is excluded and the script says so.

The remaining tables — races, classes, subclasses, features, backgrounds,
feats and equipment — are **hand-encoded** in `src/data_*.c`. The OCR shreds
the book's tables (columns get separated from their labels, so the armour
table's AC values end up detached from the armour names), which makes
extraction unreliable for exactly the data that must be exact. They were
written out and checked against the text instead.

## Testing

`tools/drive.py` drives the program through its prompts and answers them,
building whole characters at random. Because every prompt asks for a number in
a stated range, a yes/no, or a line of text, and the program blocks after
flushing, the harness can synchronise on real prompts and answer anything.

```sh
make check                                  # the suite below
python3 tools/drive.py --runs 30            # 30 random characters
python3 tools/drive.py --runs 10 --levelup  # create, save, reload, level up
python3 tools/drive.py --runs 8 --valgrind  # under valgrind
python3 tools/drive.py --runs 5 --keep out  # keep the sheets to inspect
```

`--record FILE` writes the answers a run gave, so a failure can be replayed
against the binary directly.

`tools/selftest.c` (`make test`) asserts the rules engine directly: ability
modifiers, proficiency bonus at every level, the PHB's own Bruenor example,
Armor Class for each unarmoured option and armour category, the single-class
and multiclass spell slot tables, Pact Magic, skills and expertise, and the
integrity of the data tables.

`tools/roundtrip.py` saves characters and reads them back through the
program's view mode, checking the reprinted sheet is identical to the stored
one — which is what makes levelling up a saved character trustworthy.

`examples/Bruenor.txt` is a finished sheet: the PHB's own worked example,
a 1st-level mountain dwarf fighter, built by this program.

## Layout

```
src/dnd.h              core types and the Character struct
src/data.h             game data table types
src/character.c        derived statistics
src/data_races.c       races and subraces
src/data_classes.c     classes, subclasses, spell slot and known tables
src/data_features.c    class and subclass features by level
src/data_backgrounds.c backgrounds
src/data_feats.c       feats
src/data_equipment.c   armour, weapons, gear, tools, packs, mounts
src/data_spells.c/.h   generated by tools/extract_spells.py
src/build.c            creation wizard, steps 1-4
src/progression.c      levels, subclasses, ASIs, feats, spells, level-up
src/gear.c             equipment and personality
src/saveload.c         the character file
src/ui.c               prompts, menus, dice
src/main.c             menu and entry point
```

## Scope

PHB content only — the *Xanathar's*, *Tasha's* and *Mordenkainen's* text dumps
in `TextFiles/` are not used. Choices the PHB leaves to the player and the DM
without a fixed list (eldritch invocations, Battle Master manoeuvres,
metamagic, favoured enemies and terrains, the specific artisan's tools a
background grants) are prompted for and recorded as free text rather than
enumerated.
