# dndproject

A D&D 5th Edition character creator and builder, written in C.

It walks the six steps of *Player's Handbook* chapter 1, asks for every choice
those steps require, and writes the finished character to
`<Charactername>.txt`. A saved character can be loaded again and levelled up.

Content comes from the *Player's Handbook*, *Xanathar's Guide to Everything*,
*Tasha's Cauldron of Everything*, the *Dungeon Master's Guide*, *Monsters of
the Multiverse* and the *Monster Manual*: 42 races, 13 classes, 101
subclasses, 477 spells, 57 feats, 270 magic items, 88 beast stat blocks and
186 deities. A settings menu decides which of those books are in play.

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
4. **Content settings** — which books are in play, and whether custom
   origins, Tasha's optional class features, multiclassing and feats are
   allowed. The settings are written into the character file, so a character
   loaded to level up offers the content it was built with.
5. **Item reference** — look up any item, magic item, weapon property,
   trinket, lifestyle or service.

## What it covers

- **Races** — all 9 from the PHB with their subraces, the dragonborn's
  draconic ancestry, the half-elf's floating ability increases and skill
  versatility, and the variant human's ability increases, skill and bonus
  feat; plus the **33 races of *Monsters of the Multiverse***, from aarakocra
  to yuan-ti. Tasha's "Customizing Your Origin" is off by default, so PHB
  races keep their fixed increases; switching it on pools those points and
  lets you place them. The Multiverse races have no fixed increases at all by
  design, so they always choose their own spread whatever that setting
  says.
- **Classes** — the 12 PHB classes plus the **artificer** from Tasha's,
  levels 1–20, with **101 subclasses**: the 40 from the PHB, 31 from
  Xanathar's, 26 from Tasha's, and the artificer's four specialists. Class
  and subclass features are listed at every level, along with the sub-choices
  (totem animal, storm aura, kensei weapons, starry form, rune, genie kind,
  Circle of the Land terrain, draconic bloodline).
- **The artificer** — a half-caster that, unusually, has spell slots from
  1st level and counts half its levels **rounded up** towards the multiclass
  spellcaster table. Magical Tinkering, Infuse Item, Flash of Genius,
  Spell-Storing Item and the rest, the four specialists, and all 16 infusions
  with their minimum levels and the known/infused counts per level.
- **Tasha's optional class features** — all 42, offered as opt-in choices at
  the class level that grants them, and recorded on the sheet with whatever
  PHB feature they replace. Favored Foe, Deft Explorer, Primal Awareness,
  Wild Companion, Harness Divine Power, Blessed Strikes, Steady Aim,
  Dedicated Weapon and the others. Taking "Additional *Class* Spells" really
  does widen that class's spell list in the spell picker, and taking
  "Fighting Style Options" adds Blind Fighting, Interception and the rest to
  the fighting style menu.
- **Ability scores** — standard array, 4d6 drop lowest, 27-point buy, or
  entered by hand.
- **Multiclassing** — full PHB chapter 6 rules: ability prerequisites are
  checked, later classes grant only the reduced multiclass proficiencies, and
  spell slots use the multiclass spellcaster table when more than one class
  grants Spellcasting (and that class's own table when only one does).
- **Ability Score Improvements** — at each ASI level, raise scores or take one
  of **57 feats** (42 from the PHB, 15 from Tasha's), with prerequisites
  enforced.
- **Spellcasting** — **477 spells**: 361 from the PHB, 95 from Xanathar's and
  21 from Tasha's, each with level, school, ritual flag, casting time, range,
  components, duration and concentration. Cantrips and spells known/prepared
  are enforced per class, domain, oath, circle, patron and specialist spells
  are granted automatically at the right levels, and Pact Magic is tracked
  separately.
- **Equipment** — the class and background starting packages (resolved into
  real items, including "choose any martial weapon" style entries), or roll
  for starting gold and buy from the full PHB catalogue of armour, weapons,
  gear, tools, packs, tack and vehicles, with weight tracked against carrying
  capacity. The shop shows an item's full detail before asking you to buy it,
  and can look items up without buying.
- **Item information** — every item says what it does, not just what it costs
  and weighs. Armour carries its AC formula, Strength requirement, Stealth
  penalty and don/doff times; weapons explain each property they list;
  and items with no stat line at all — thieves' tools, a healer's kit, a
  hunting trap — carry what they contain, what they are used for and which
  ability checks they help with. **270 magic items** from the *Dungeon
  Master's Guide* are browsable by kind, rarity, attunement or name, and
  the trinket table, lifestyle expenses and the prices of food, lodging,
  services and hired spellcasting are all to hand.
- **Class option lists** — eldritch invocations (with their prerequisites),
  pact boons, metamagic, Battle Master maneuvers, Arcane Shot options,
  elemental disciplines, Rune Knight runes, and the ranger's favoured enemies
  and terrains are all offered as menus and recorded on the sheet. Each list
  knows how many the class has by each level, so a character levelled in
  stages is asked exactly once for each.
- **Beasts** — **88 stat blocks** from the *Monster Manual*. A druid's Wild
  Shape lists the forms actually available at their level and circle, a Beast
  Master picks a companion from the beasts that qualify, and a Pact of the
  Chain warlock picks a familiar.
- **Deities** — the **186 gods of appendix B** across nine pantheons, with
  alignment, suggested domains and symbol. A cleric who has already chosen a
  domain sees which deities suggest it.
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

The spell tables are **extracted** from all three books by
`tools/extract_spells.py`, which parses every spell stat block and each
book's class spell lists into `src/data_spells.{c,h}`:

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

Xanathar's and Tasha's set their spell names in small capitals, which the OCR
turns into mixed case and sometimes garbles outright ("Cuaos BoLt" for *Chaos
Bolt*). Their own class lists and summary tables carry clean names and each
spell's school, so those supply the name and the description block supplies
only the stat lines — and the school has to agree before the two are joined.
Tasha's prints its new spells as one table of levels and schools followed by a
second of ritual flags and classes, which are zipped back together.

Two quirks are preserved rather than papered over. *Trap the Soul* appears on
the PHB's wizard spell list but has no spell description anywhere in the book,
so it is excluded and the script says so. And a single 7th-level entry in
Tasha's "Additional Druid Spells" table is destroyed in the OCR — the source
PDFs are page scans with no recoverable text — so it is omitted rather than
guessed, and `src/data_optional.c` says so where the list lives.

The beast stat blocks are **extracted** from the *Monster Manual* by
`tools/extract_beasts.py`:

```sh
make beasts     # regenerate src/data_beasts.{c,h}
```

That dump is much cleaner than the others, but its appendix pages set two
stat blocks side by side and the OCR reads them column by column: both names,
then both Armor Class lines, then both Hit Points lines. So the script groups
the headers on a page first and takes each field N at a time, and it bounds
each block at the next header rather than a fixed number of lines — a wider
window silently picks up the *following* beast's challenge rating, which is
how the black bear came out as CR 0. Ability scores are matched to their
column heading rather than counted off in order, so a destroyed score (a
charisma that reads as "m", a heading split as "I NT") cannot shift its
neighbours along. Twelve known-correct stat blocks are checked on every run,
and the script refuses to write output if any of them disagrees. The handful
of blocks the dump carries incompletely are typed out in the script and
flagged if they ever start parsing.

The remaining tables — races, classes, subclasses, features, backgrounds,
feats, equipment, magic items and deities — are **hand-encoded** in
`src/data_*.c`. The OCR shreds the book's tables (columns get separated from
their labels, so the armour table's AC values end up detached from the armour
names), which makes extraction unreliable for exactly the data that must be
exact. The *Dungeon Master's Guide* is the worst of them: its magic item
entries interleave across columns so that an item's name sits above another
item's description. Appendix B is the same — the dump carries complete
alignment, domain and symbol columns for the Forgotten Realms table and for
no other pantheon. Those were written out and checked against the text
instead; the OCR was used to cross-check the roster for completeness rather
than to supply the values.

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

`examples/` holds two finished sheets built by this program:
`Bruenor.txt`, the PHB's own worked example of a 1st-level mountain dwarf
fighter, and `Artificer.txt`, an 8th-level alchemist showing the infusions,
half-caster slots and specialist spells.

## Layout

```
src/dnd.h              core types and the Character struct
src/data.h             game data table types
src/character.c        derived statistics
src/data_races.c       races and subraces
src/data_classes.c     classes, spell slot and spells-known tables
src/data_subclasses.c  all 101 subclasses and their granted spells
src/data_optional.c    Tasha's optional class features and added spell lists
src/data_infusions.c   artificer infusions
src/data_features.c    class and subclass features by level
src/data_backgrounds.c backgrounds
src/data_feats.c       feats
src/data_equipment.c   armour, weapons, gear, tools, packs, mounts, vehicles
src/data_itemtext.c    what each item does, and the weapon properties
src/data_magicitems.c  the DMG magic item catalogue
src/data_gearlists.c   trinkets, lifestyles, services, sizes
src/data_classoptions.c invocations, metamagic, maneuvers, runes and the rest
src/data_deities.c     the gods of appendix B
src/data_spells.c/.h   generated by tools/extract_spells.py
src/data_beasts.c/.h   generated by tools/extract_beasts.py
src/settings.c         which books and optional rules are in play
src/reference.c        the item lookup browser
src/build.c            creation wizard, steps 1-4
src/progression.c      levels, subclasses, ASIs, feats, spells, level-up
src/gear.c             equipment and personality
src/saveload.c         the character file
src/ui.c               prompts, menus, dice
src/main.c             menu and entry point
```

## Scope

Six books: the *Player's Handbook*, *Xanathar's Guide to Everything*,
*Tasha's Cauldron of Everything*, the *Dungeon Master's Guide* (magic items),
*Mordenkainen Presents: Monsters of the Multiverse* (races) and the *Monster
Manual* (beasts). Any of them except the PHB can be switched off in the
settings menu, which hides their races, classes, subclasses, spells, feats
and items everywhere in the wizard.

A few choices are still recorded as free text rather than picked from a list,
because the books do not give one: the specific artisan's tools a background
grants, and which magic item an artificer's Replicate Magic Item infusion
copies.
