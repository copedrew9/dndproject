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
6. **Manage a character's inventory** — pick things up, put them down,
   change what is worn, attune to magic items.
7. **Manage a character's sidekicks** — add one, level it up, or write it
   out as a sheet of its own.
8. **Homebrew** — add your own items, magic items and spells to the banks,
   or take them out again.
9. **Notes and character details** — notes, personality, appearance and
   backstory, on a saved character.

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
  of **72 feats** (42 from the PHB, 15 from Tasha's, 15 racial feats from
  Xanathar's), with prerequisites enforced — including the racial ones, so a
  halfling is offered Bountiful Luck and Second Chance and a human is not.
- **Backstory** — Xanathar's "This Is Your Life" tables: birthplace,
  siblings, who raised you, family lifestyle, childhood home and memories,
  occupation and more. Every table can be rolled on *or* read down and picked
  from, and any of them skipped, so a player who already knows where their
  character came from does not have to roll and then argue with the result.
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
- **Inventory** — the equipment step settles a character's starting gear;
  after that, gear is managed on its own screen, reachable from the main
  menu and offered at every level-up. Pick things up from the catalogue or
  from the magic item list, put things down, choose which armour and shield
  are actually worn (Armor Class follows), attune to at most three magic
  items, and set the purse. Nothing is ever sold: haggling over the price of
  a used breastplate is a conversation with the DM, not a menu.
- **Sidekicks** — Tasha's Expert, Spellcaster and Warrior at levels 1–20,
  built on any beast of challenge 1/2 or lower from the *Monster Manual*
  tables or on a stat block typed in by hand. Each level's choices are
  actually made — saving throw proficiency, martial role, spellcasting role
  and its spells, and every ability score improvement. A sidekick is saved
  inside its owner's file and can also be written out as its own sheet for
  whoever is running it at the table.
- **Homebrew** — a DM can add items, magic items and spells of their own.
  Anything added appears wherever a printed entry would — in the shop, the
  spell picker, the item reference, on the sheet — and is marked as
  homebrew.

  Every field with a known set of answers is a menu rather than something to
  type: a spell's level, school, casting time, range and duration; its
  components, as checkboxes that assemble `V, S, M (a pinch of soot)`; and
  the class lists it belongs to, as a checklist, because a spell on no list
  can never be learned. An item's price is asked in gold, silver or copper
  rather than in the copper it is stored as, and its weight in pounds rather
  than tenths. Armour asks how Dexterity applies — full modifier, capped at
  +2, or none — instead of asking for the `-1` the table actually holds. A
  weapon's properties are ticked off the real list of eleven, and the three
  that carry a number (ammunition, thrown, versatile) ask for it. A magic
  item's kind, rarity and attunement clause are all menus, and the two
  attunement shapes that name a class or an alignment offer those too.

  What is left to type is only what is genuinely free: names, descriptions,
  a material component, what is inside a pack.

  Entries live in `homebrew.txt` beside the character files, in the same
  `|`-separated format, so they can be written by hand or shared. Homebrew
  is a source book like any other in the settings menu, so switching it off
  hides everything without deleting it.
- **Backgrounds** — all 13, with their skills, tools, languages, feature, and
  suggested traits, ideals, bonds and flaws. Or build one of your own by the
  Player's Handbook's own rules (p.125): any two skills, two tools or
  languages between them, and any feature — one borrowed from a printed
  background or written yourself.
- **Notes and prose** — a character carries notes, each with a title and a
  body that may run to paragraphs: a contact, a debt, a patron's demands,
  the history behind a family sword. Type as many lines as you like; a blank
  line ends the note. They can be added, read, extended, rewritten, retitled
  and removed at any time from the main menu, long after the character was
  made. Personality, appearance and backstory sit on the same screen, so
  those can change in play too. Traits, ideals, bonds and flaws offer the
  background's suggestions and always let you write your own — including for
  a character whose background is their own, which the wizard used to skip
  past entirely.

  Notes are stored in the character file as one record each, with their
  newlines and separators escaped so a note full of paragraphs cannot break
  the line-oriented format.

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

Storing names is also what lets homebrew work at all, since a custom entry's
position in a bank is not stable between runs. The cost is that a name the
banks no longer hold — homebrew the DM has since removed, or a book switched
off — cannot be restored. The loader says so on stderr rather than dropping
it silently.

## Where the data comes from

Every table the program uses is hand-written, in four files under `data/`:

```
data/character.txt   races, subraces, classes, subclasses, features,
                     backgrounds, feats, invocations and the rest
data/equipment.txt   armour, weapons, gear, tools, packs, magic items,
                     trinkets, lifestyles, prices and the tool groups
data/spells.txt      every spell
data/world.txt       gods, beasts, sidekicks, the background tables
```

They are line-oriented and `|` separated, the same shape as the character
files, with a comment above each block naming its columns:

```
RACE|Dwarf|PHB|0|0|2|0|0|0|25|Medium|60|Common, Dwarvish|...
ITEM|Longsword|PHB|martial-melee|1500|30|0|0|0|0|1d8|slashing|Versatile (1d10)|
SPELL|Fireball|PHB|3|Evocation|0|0|1 action|150 feet|V, S, M (...)|...
```

Cross-references are by name — a feature names its class, a subrace names its
race, a magic item rule names its item — so a row can be added anywhere in a
file without renumbering anything.

`tools/build_data.py` turns those files into the `src/gen_*.c` tables the
program compiles:

```sh
make data       # regenerate src/gen_*.c from data/
```

The generated files are checked in, so building the program needs nothing but
a C compiler. The generator resolves every cross-reference and refuses to
write anything if a name does not resolve, quoting the file and line — that is
the checking the compiler used to do when the tables themselves were C.

Because the tables are generated, the reverse is possible too, and is worth
doing: `tools/dump_data.c` links against the compiled tables and prints them
back out in the same format. `make dataverify` dumps into a scratch directory
and compares. If the two ever disagree, a row was lost or changed in
translation.

```sh
make dataverify # data/ and the compiled tables must match byte for byte
```

That round trip is also how the data got here. It used to live in nineteen
hand-written C files; retyping thirteen hundred verified rows would have
introduced errors, so the first dump produced `data/` from the tables as they
stood, and the round trip proved nothing was lost.

`homebrew.txt` is the one data file read at run time rather than compiled in,
because what a DM invents cannot be a `const` table. It uses the same record
format and the same names for a category, a school and a class list, and it
ships with a commented example of an item, a magic item and a spell —
uncomment a line to bring it into play. The explanation is rewritten every
time the file is saved, so it survives whatever the Homebrew menu does to the
entries below it.

### Checking it against the books

`TextFiles/` holds OCR'd text of the sourcebooks. Nothing is built from it;
it is kept so the data can be checked against it. `tools/audit.py`
(`make audit`) looks up every name in `data/` in the dump of the book it
claims to come from and reports what it cannot find:

```sh
make audit      # 2618 names checked against six book dumps
```

Matching is deliberately tolerant, because the dumps are lossy in known ways:
they confuse C with G and I with L, lose spaces inside words, and render small
capitals as mixed case. So names are compared on their letters alone with
those pairs folded together. That is enough to find real mistakes — it caught
*Undying Servitude* tagged to Xanathar's when it is Tasha's, which meant the
feature was hidden with the wrong book switched off.

What the audit reports as missing is worth stating plainly, because it is
where the data is weakest:

- **The PHB equipment table** is absent from that dump altogether — not just
  the numbers, the names. Every cost, weight, damage die and armour value in
  `data/equipment.txt` was typed from the book and cannot be cross-checked
  against `TextFiles/`.
- **Appendix B's deity columns** survive for the Forgotten Realms table and
  for no other pantheon, so the alignment, domain and symbol columns of the
  other eight were written from the book.
- **Two "This Is Your Life" tables** (Alignment, and Life Events) are split
  across columns in a way that cannot be read back, so they are absent rather
  than guessed at.
- **The Spellcaster sidekick's *Spells Known* column** is unreadable in the
  OCR *and* in the PDF's own text layer, which is a subset-font encoding with
  no ToUnicode map. The numbers in `data/world.txt` are a reconstruction
  anchored on what the surrounding prose does state; the program says so on
  screen when it uses them, so the DM can overrule it.
- ***Trap the Soul*** appears on the PHB's wizard list with no description
  anywhere in the book, and one 7th-level entry in Tasha's "Additional Druid
  Spells" is destroyed in the dump. Both are left out rather than invented.

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
data/character.txt     races, classes, subclasses, features, backgrounds
data/equipment.txt     gear, weapons, armour, magic items, prices, tools
data/spells.txt        every spell
data/world.txt         gods, beasts, sidekicks, background tables
homebrew.txt           the DM's own entries, read at run time
tools/build_data.py    data/ -> src/gen_*.c
tools/dump_data.c      src/gen_*.c -> data/, for `make dataverify`
tools/audit.py         checks every name in data/ against TextFiles/

src/dnd.h              core types and the Character struct
src/data.h             game data table types
src/data_spells.h      the spell record and its enums
src/gen_character.c    generated from data/character.txt
src/gen_equipment.c    generated from data/equipment.txt
src/gen_spells.c       generated from data/spells.txt
src/gen_world.c        generated from data/world.txt
src/data_lookup.c      the searches over those tables
src/character.c        derived statistics
src/backstory.c        the This Is Your Life tables, rolled or chosen
src/details.c          notes, personality and the other prose
src/settings.c         which books and optional rules are in play
src/reference.c        the item lookup browser
src/inventory.c        adding, dropping, wearing and attuning
src/sidekick.c         creating and levelling sidekicks
src/homebrew.c         the DM's own items and spells, and the banks
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
Manual* (beasts), plus whatever the DM adds as homebrew. Any of them except
the PHB can be switched off in the settings menu, which hides their races,
classes, subclasses, spells, feats and items everywhere in the wizard.

The item, magic item and spell banks are pointers rather than fixed arrays,
so `src/homebrew.c` can replace each with the book's entries followed by the
DM's. `ITEMS[i]` reads the same either way, which is why adding homebrew
touched none of the eighty-odd places that read those tables.

Tasha's sidekicks come from that book's chapter 4.

### Choosing from the books, and beside them

Every list a class draws on as it levels is offered as a menu with the book's
entries and a line of explanation beside each: eldritch invocations, pact
boons, metamagic options, Battle Master maneuvers, Arcane Shot options,
elemental disciplines, rune knight runes, favoured enemies and favoured
terrains, fighting styles and artificer infusions.

Under each of those, and under the tool lists below, there is one more entry
for something the books do not print. A DM who has written a new invocation,
or a table using a maneuver from somewhere else, can put it on the sheet
without waiting for the program to learn about it; it is recorded, saved and
printed exactly as a printed one is. Where an entry is out of reach the menu
says why rather than hiding it, and where every printed entry is already
taken the question is still asked rather than dropped.

Tool proficiencies are read out of the prose the books use. "One type of
artisan's tools" offers the seventeen artisan's tools, "one type of gaming
set" the gaming sets, "three musical instruments of your choice" asks three
times from the ten instruments; the monk's "one artisan's tool or one
musical instrument" offers both groups at once. Which items make up a group
is the `TOOLGROUP` block in `data/equipment.txt`, so a homebrew tool can join
one. A proficiency line that names a definite tool grants it without asking.

The feats that hand you a further choice ask for it when the feat is taken,
rather than leaving it to be remembered: Eldritch Adept draws on the
invocation list (limited to those with no prerequisite unless you are a
warlock), Metamagic Adept takes two metamagic options, Fighting Initiate a
fighting style, Skill Expert a skill and an expertise, Artificer Initiate a
cantrip, a 1st-level spell and a set of artisan's tools, Fey Touched and
Shadow Touched their spells. A spell a feat grants is filed under no class,
so it never counts against a class's cantrips or spells known.

One choice is still free text, because the books give no list this program
holds: which magic item an artificer's Replicate Magic Item infusion copies,
that being its own table by artificer level.

Magic items that change a computed number flatly and unconditionally are
computed. The `MAGICRULE` lines in `data/equipment.txt` hold those, keyed by
the item's name:

- **Armor Class and saving throws** — a ring or cloak of protection, bracers
  of defense, a staff of power, magic armour and shields, and the +1/+2/+3
  entries once you say which one a copy is.
- **Ability scores** — an amulet of health, headband of intellect or
  gauntlets of ogre power set a score outright, and a belt of giant strength
  sets it to whatever its giant's is.
- **Speeds** — boots of striding and springing set a floor under the walking
  speed, and winged boots, wings of flying, a cloak of the manta ray, a ring
  of swimming and a cloak of arachnida add a flying, swimming or climbing
  speed the sheet prints alongside it.
- **Resistances and immunities** — collected into one line, including the
  damage type a ring of resistance or armour of resistance was made against,
  which is asked for when the item is picked up.

The sheet lists exactly which items the numbers above already account for, so
nothing is counted twice. An item does nothing until it is attuned if it
needs attunement, and armour and shields do nothing until worn.

Anything situational stays prose and is applied at the table: the Defender's
bonus is re-split between attack and AC every turn, an arrow-catching
shield's extra +2 applies only against ranged attacks, a robe of
scintillating colors imposes disadvantage rather than changing a number.
