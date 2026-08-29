# dndproject

A D&D 5th Edition character creator and builder, written in C.

It walks the six steps of PHB chapter 1, asks for every choice those steps
require, and writes the finished character to `<Charactername>.txt`. A saved
character can be loaded again and levelled up.

Content comes from seven sourcebooks -- PHB, XGE, TCE, DMG, MPMM, MM and
SCAG -- and covers 42 races with 30 subraces, 13 classes, 107 subclasses,
477 spells, 73 feats, 26 backgrounds, 277 magic items, 88 beast stat blocks
and 195 deities. A settings menu decides which of those are in play.

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
3. **Game mode** — a character in play rather than in the making: hit
   points and death saves, rests and hit dice, spell slots and components,
   uses of things (bardic inspiration, ki, rage), conditions, money and a
   ledger of where it went, a visit to a shop, food and lodging off the
   book's own table, gear, and how worn that gear is.
4. **View a saved character** — prints the sheet.
5. **Content settings** — which books are in play, and whether custom
   origins, TCE's optional class features, multiclassing and feats are
   allowed, whether the sheet prints where the level sits on the experience
   table, and whether you roll your own dice. The settings are written into
   the character file, so a character loaded to level up offers the content
   it was built with -- and keeps its own answer on the experience line.
6. **Reference** — look up any item, magic item, weapon property, trinket,
   lifestyle, service or condition.
7. **Manage a character's inventory** — pick things up, put them down,
   change what is worn, attune to magic items, carry gems and the things no
   table has.
8. **Manage a character's sidekicks** — add one, level it up, or write it
   out as a sheet of its own.
9. **Homebrew** — add your own items, magic items and spells to the banks,
   or take them out again.
10. **Shopbuilder** — a shop for your table, saved to `<shop name>.txt`. A
    shop is what the DM puts in it and nothing else: each line carries its
    own price, so the town with a war on can charge what it likes for
    arrows, and a line can be something no book sells. Game mode loads one
    by name and buys from it.
11. **Notes and character details** — notes, personality, appearance and
    backstory, on a saved character.

## What it covers

- **Races** — all 9 from the PHB with their **30 subraces** — the 11 in that
  book, and 19 more from SCAG: the duergar, the svirfneblin, the ghostwise
  halfling, the tiefling's variants (Feral, Devil's Tongue, Hellfire, Winged
  and the pairings the book allows), and the half-elf's, which trade Skill
  Versatility for a trait of the elf parent — the dragonborn's
  draconic ancestry, the half-elf's floating ability increases and skill
  versatility, and the variant human's ability increases, skill and bonus
  feat; plus the **33 races of MPMM**, from aarakocra to yuan-ti. TCE's
  "Customizing Your Origin" is off by default, so PHB races keep their fixed
  increases; switching it on pools those points and lets you place them. The
  MPMM races have no fixed increases at all by design, so they always choose
  their own spread whatever that setting says.
- **Classes** — the 12 PHB classes plus the **artificer** from TCE, levels
  1–20, with **107 subclasses**: the 40 from the PHB, 31 from XGE, 26 from
  TCE, the artificer's four specialists, and 6 that only SCAG printed — the
  Path of the Battlerager, the Arcana Domain, the Purple Dragon Knight, the
  Way of the Long Death, the Oath of the Crown and the Undying patron. The
  five SCAG subclasses that XGE reprinted are not offered twice. Class and subclass features are
  listed at every level, along with the sub-choices (totem animal, storm aura,
  kensei weapons, starry form, rune, genie kind, Circle of the Land terrain,
  draconic bloodline).
- **The artificer** — a half-caster that, unusually, has spell slots from
  1st level and counts half its levels **rounded up** towards the multiclass
  spellcaster table. Magical Tinkering, Infuse Item, Flash of Genius,
  Spell-Storing Item and the rest, the four specialists, and all 16 infusions
  with their minimum levels and the known/infused counts per level.
- **TCE's optional class features** — all 42, offered as opt-in choices at
  the class level that grants them, and recorded on the sheet with whatever
  PHB feature they replace. Favored Foe, Deft Explorer, Primal Awareness,
  Wild Companion, Harness Divine Power, Blessed Strikes, Steady Aim,
  Dedicated Weapon and the others. Taking "Additional *Class* Spells" really
  does widen that class's spell list in the spell picker, and taking
  "Fighting Style Options" adds Blind Fighting, Interception and the rest to
  the fighting style menu.
- **Ability scores** — standard array, 4d6 drop lowest, 27-point buy, or
  entered by hand.
- **Your own dice** — a setting that stops the program rolling anything.
  With it on, every roll it would have made is asked for instead: ability
  scores (four dice at a time, and it tells you which one it dropped), hit
  points on levelling up, starting gold, height and weight, a trinket, a
  sidekick's hit points and every backstory table. Bounds are shown and
  enforced, so a mistyped total is caught rather than written down.
- **Multiclassing** — full PHB chapter 6 rules: ability prerequisites are
  checked, later classes grant only the reduced multiclass proficiencies, and
  spell slots use the multiclass spellcaster table when more than one class
  grants Spellcasting (and that class's own table when only one does).
- **Ability Score Improvements** — at each ASI level, raise scores or take one
  of **73 feats** (42 from the PHB, 15 from TCE, 15 racial feats from XGE,
  and SCAG's Svirfneblin Magic),
  with prerequisites enforced — including the racial ones, so a halfling is
  offered Bountiful Luck and Second Chance and a human is not.
- **Backstory** — XGE's "This Is Your Life" tables: birthplace, siblings, who
  raised you, family lifestyle, childhood home and memories, occupation and
  more. Every table can be rolled on *or* read down and picked from, and any
  of them skipped, so a player who already knows where their character came
  from does not have to roll and then argue with the result.
- **Spellcasting** — **477 spells**: 361 from the PHB, 95 from XGE and 21
  from TCE, each with level, school, ritual flag, casting time, range,
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
- **Packs come apart.** Taking an explorer's pack puts the bedroll, the
  rations, the waterskin and the rest on the sheet as things you can drop,
  sell, eat and count -- not one line reading "Explorer's pack". The
  contents replace the pack rather than joining it, which is only safe
  because a pack's weight is exactly the sum of its parts: all seven are,
  to the tenth of a pound, and `tools/verify_packs.py` checks both that
  and the contents themselves against the book. Seven things a pack names
  and the book prices nowhere -- an alms box, a censer, vestments, a book
  of lore -- have rows of their own so they can be carried.
- **Item information** — every item says what it does, not just what it costs
  and weighs. Armour carries its AC formula, Strength requirement, Stealth
  penalty and don/doff times; weapons explain each property they list;
  and items with no stat line at all — thieves' tools, a healer's kit, a
  hunting trap — carry what they contain, what they are used for and which
  ability checks they help with. **277 magic items** from the *Dungeon
  Master's Guide* are browsable by kind, rarity, attunement or name --
  every one it prints, artifacts included, and
  the trinket table, lifestyle expenses and the prices of food, lodging,
  services and hired spellcasting are all to hand.
- **Languages** — the PHB's 16, plus the **18 regional tongues of Faerun**
  that SCAG gives its human ethnicities, from Alzhedo to Uluik. They come and
  go with SCAG like everything else, so a table not playing in the Realms
  never sees them.
- **Class option lists** — eldritch invocations (with their prerequisites),
  pact boons, metamagic, Battle Master maneuvers, Arcane Shot options,
  elemental disciplines, Rune Knight runes, and the ranger's favoured enemies
  and terrains are all offered as menus and recorded on the sheet. Each list
  knows how many the class has by each level, so a character levelled in
  stages is asked exactly once for each.
- **Beasts** — **88 stat blocks** from the MM. A druid's Wild Shape lists the
  forms actually available at their level and circle, a Beast Master picks a
  companion from the beasts that qualify, and a Pact of the Chain warlock
  picks a familiar.
- **Deities** — the **195 gods of appendix B** across nine pantheons, with
  alignment, suggested domains and symbol. A cleric who has already chosen a
  domain sees which deities suggest it.
- **Inventory** — the equipment step settles a character's starting gear;
  after that, gear is managed on its own screen, reachable from the main
  menu and offered at every level-up. Pick things up from the catalogue or
  from the magic item list, put things down, choose which armour and shield
  are actually worn (Armor Class follows), attune to at most three magic
  items, and set the purse. Nothing is ever sold: haggling over the price of
  a used breastplate is a conversation with the DM, not a menu.
- **Sidekicks** — TCE's Expert, Spellcaster and Warrior at levels 1–20, built
  on any beast of challenge 1/2 or lower from the MM tables or on a stat block
  typed in by hand. Each level's choices are actually made — saving throw
  proficiency, martial role, spellcasting role and its spells, and every
  ability score improvement. A sidekick is saved inside its owner's file and
  can also be written out as its own sheet for whoever is running it at the
  table.
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
- **Backgrounds** — all 13 from the PHB, with their skills, tools, languages,
  feature, and suggested traits, ideals, bonds and flaws; plus SCAG's 13,
  from City Watch (and its investigator variant) to Waterdhavian Noble. Five
  of those name one skill and leave the other to you -- "Persuasion, plus one
  from among Arcana, History, Nature, and Religion" -- and the wizard asks
  rather than choosing for you. SCAG sends you to a PHB background
  for traits and ideals rather than printing its own tables, so those are
  left open and the wizard asks for them. Or build one of your own by the
  PHB's own rules (p.125): any two skills, two tools or languages between
  them, and any feature — one borrowed from a printed background or written
  yourself.
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

Anything the player typed is written with the separator escaped: a `|` in a
name, a note or a tool a table invented is stored as `\p`, a backslash as
`\\`, and a newline inside a note as `\n`. Without that a single `|` shifted
every field after it and the character came back as whatever preceded it.
homebrew.txt uses the same convention, and a field carrying none of the three
is stored exactly as typed, so a file written before any of this was escaped
still reads correctly.

Storing names is also what lets homebrew work at all, since a custom entry's
position in a bank is not stable between runs. The cost is that a name the
banks no longer hold — homebrew the DM has since removed, or a book switched
off — cannot be restored. The loader says so on stderr rather than dropping
it silently.

## Where the data comes from

Every table the program uses is hand-written, in four files under `data/`:

```
data/character.txt   races, subraces, classes, subclasses, features,
                     backgrounds, feats, invocations, the experience
                     table and the height and weight table
data/equipment.txt   armour, weapons, gear, tools, packs, magic items,
                     trinkets, lifestyles, prices and the tool groups
data/spells.txt      every spell
data/world.txt       gods, beasts, sidekicks, conditions, the
                     background tables
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

`TextFiles/` holds the text of the six sourcebooks, which is what every check
below runs against. Nothing is built from it, and no sourcebook is
distributed here: `tools/pdf_text.py` pulls the text layer out of a PDF you
supply, once, and its output is what is checked in.

    python3 tools/pdf_text.py /path/to/your/pdfs

That text used to be OCR of scanned pages, and the difference is the
difference between guessing and knowing. The OCR confused C with G and I with
L, lost spaces inside words, read two-column stat blocks across the gutter,
and dropped the PHB equipment table entirely -- not just the numbers, the
names. Everything below became checkable when it was replaced -- and most of
what the checks have found since is that same OCR's fingerprints, left in
rows typed up while it was still the only text to hand: "| action" for
"1 action", "crystal bail" for "crystal ball", a casting time that stops
where the page's line did.

Three checks run over it:

```sh
make audit      # every name in data/ appears in the book it claims
make verify     # the numbers beside those names are the book's too
make check      # the above, plus the test suite
```

`tools/audit.py` looks up all 2,739 names -- races, classes, subclasses,
features, feats, equipment, magic items, spells, deities, beasts -- in the
dump of the book each claims to come from. All of them are found. Matching
stays tolerant, because the typesetting still gets in the way: words are set
with a space inside them, small capitals come back as mixed case, and a
section opening with a decorative initial loses that letter altogether, which
is why *Quickened Healing* is set as "UICKENED HEALING". Six names are ours
rather than the book's -- "Standard Human", for a human the PHB prints under
no heading of its own, five composite feature names, and three pairings of
SCAG tiefling variants -- and the audit says so rather than reporting them as
missing.

The audit runs one way: every name in `data/` has to appear in its book. A
gap hides in the other direction, because an entry nobody wrote down is not
a name that fails to resolve -- it is simply absent, and nothing notices.
`tools/verify_coverage.py` closes that: it enumerates the books by the fixed
line each entry opens with -- a magic item's "Wondrous item, rare (requires
attunement)", a spell's "3rd-level evocation" -- and subtracts what `data/`
has. It found seven magic items nobody had entered, the Axe of the Dwarvish
Lords and the Eye and Hand of Vecna among them. It now reports none, for 434
spells and 255 magic items.

Names are the cheap check. Thirteen scripts do the expensive one, comparing the
numbers and the words beside those names -- every spell's stat line, every
magic item's kind and rarity, every beast's stat block, every class and
subclass feature's level, every background's table of suggestions, every
feat's prerequisite, and the races and equipment and gods that were checked
before. Between them they found 65 rows the books settle differently:

- **All 214 PHB equipment rows** -- cost, weight, damage die and type,
  armour class, the Dexterity cap, Strength requirement and stealth --
  against the book's own Armor, Weapons, Adventuring Gear, Tools, Mounts,
  Tack and Waterborne Vehicles tables, with the packs priced from the prose.
  This found four items ten times too heavy: crossbow bolts, sling bullets, a
  steel mirror and an orb, each stored in pounds where the column reads a
  fraction.
- **All 477 spells** -- level, school, ritual, casting time, range, the
  V/S/M letters, the material component's text, duration and concentration,
  against the fixed shape a spell entry has in the book. Level, school,
  ritual and concentration were right for every one. Thirty-six rows were
  not, all of it damage from the days when the book text was OCR rather than
  a text layer: nine components punctuated "V. S, M"; eleven values cut off
  where the page's line ended, five reaction casting times and six material
  components left with an unclosed bracket; eight letters read for digits or
  stray characters left in ("| action", "up to I minute", "Self
  (lO-foot-radius"); and three misread words -- "gum arable" for gum arabic,
  "crystal bail" for crystal ball. Shield's casting time was "1 reaction,
  which you take when you are", stopping where the page did.
- **All 88 beast stat blocks** -- size, armour class, hit points, speed, the
  six ability scores, senses and challenge. Nine disagreed. The triceratops
  was challenge 1/4 rather than 5, which put it within reach of a 2nd-level
  druid's Wild Shape; the giant frog carried another creature's ability
  scores and the giant sea horse's were a column out of step; three senses
  lines read "bitndsight", "Hindsight" and a stray "^".
- **274 of the 277 magic items** -- the kind, rarity and attunement clause
  each entry opens with. Five were wrong: Whelm is a warhammer, not a maul;
  Blackrazor, the Staff of the Adder and the Tome of the Stilled Tongue each
  lost or widened an attunement restriction; and the Hammer of Thunderbolts
  requires no attunement on its line, the book attaching it to the Giant's
  Bane property alone. Two rows differ from the book on purpose and say so
  rather than being counted as errors.
- **936 rows of class and subclass data** -- hit dice, saving throws,
  proficiencies, skill choices, which class and book each subclass belongs
  to, and the level each feature is gained at, read out of the feature's own
  opening line. Five features were filed at the wrong level: the Battle
  Master's Improved Combat Superiority and Relentless were swapped, so were
  the Path of Wild Magic's Magic Awareness and Bolstering Magic, and the
  College of Eloquence's Universal Speech sat at 14th rather than 6th.
- **All 26 backgrounds and their 261 suggested characteristics** -- skills,
  tools, languages, feature, equipment, and every trait, ideal, bond and
  flaw against the table it comes from. Three sentences were in no table in
  the book: two under guild artisan, and a folk hero flaw that is the
  acolyte's.
- **All 73 feats and the 42 optional class features**, and **the 9 PHB races,
  30 subraces and 10 draconic ancestries** -- prerequisites, ability
  increases, speeds, sizes, darkvision, languages. Nothing wrong in either,
  which is worth as much as a finding: the feat checker was run against 36
  deliberately corrupted copies of its own rows first, and caught all 36.
- **The tables that are nothing but numbers** -- the experience needed for
  each level, the full caster's spell slots, the warlock's Pact Magic
  columns, the height and weight table, the artificer's infusion levels, and
  the cantrips and spells each class knows at each of its twenty levels:
  306 rows, read back out of the book. All right, which took writing a
  reader for the shape the extraction leaves a table in: one cell to a line,
  an em dash for an empty column, and the header's own "1st" to "9th"
  sitting above the rows like nine more of them. Two rows the extraction
  garbles are named as unread rather than guessed at.
- **The reference rows** -- prices, conditions, weapon properties, tool
  groups, trinkets and the This Is Your Life tables. Two prices were in
  silver in the book and stored as copper, a tenth of what the book asks.
  The Childhood Home table was filed as 3d6 where Xanathar's rolls d100, and
  since a 3d6 cannot reach past 18 and that table's first row runs to 20,
  every character the program ever rolled a childhood for grew up in a
  rundown shack. Two more of its rows were a page header and a footnote the
  extraction had left in the table.
- **All 195 deities of appendix B** -- title, alignment, suggested domains and
  symbol, compared pantheon by pantheon so that the two Tyrs, the two Surturs
  and the two Silvanuses are each checked against the right table. The
  hand-written rows had Vecna's symbol wrong, Incabulos's wrong, and the Norse
  Tyr's title, alignment, domains and symbol all four; six nonhuman gods were
  missing outright. `tools/extract_deities.py` reads the tables and writes the
  rows now, and the verifier keeps them honest.

What the books themselves do not settle:

- **MPMM** is the one book whose text is still OCR, no clean copy of it
  having been to hand. `tools/verify_races.py` checks 31 of its 33 races
  against it anyway -- size, walking speed, darkvision -- which caught the
  air genasi's speed and darkvision and the earth genasi's darkvision. The
  minotaur and the orc are the two it leaves alone: the orc's heading is
  missing from the dump, so its traits run on into the minotaur's block and
  neither can be told from the other. Reading those two off the page by
  hand is what turned up the minotaur carrying Imposing Presence, which
  belongs to a different minotaur entirely, in place of Labyrinthine Recall.

## Testing

`tools/drive.py` drives the program through its prompts and answers them,
building whole characters at random. Because every prompt asks for a number in
a stated range, a yes/no, or a line of text, and the program blocks after
flushing, the harness can synchronise on real prompts and answer anything.

```sh
make check                                  # the suite below
make asan                                   # the same, with the sanitizers
make sweep                                  # every race x every class, built
python3 tools/drive.py --runs 30            # 30 random characters
python3 tools/drive.py --runs 10 --levelup  # create, save, reload, level up
python3 tools/drive.py --runs 8 --valgrind  # under valgrind
python3 tools/drive.py --runs 5 --keep out  # keep the sheets to inspect
python3 tools/drive.py --class Wizard --level 3 --race Tortle
python3 tools/drive.py --class Rogue --subclass "Arcane Trickster"
```

`--class`, `--subclass`, `--race` and `--level` aim a run instead of leaving
it to the dice. Without them a wizard of a chosen level comes up about one
run in a hundred and an Eldritch Knight far less often, which is why the
spell bugs in both went years without being exercised. An aimed run also
leaves the source books alone, since toggling them at random switches off
the very book the wanted race comes from.

`make sweep` builds every one of the 42 races crossed with every one of the
13 classes through the wizard itself -- a thousand characters, most of an
hour. `tools/combos.c` covers the same ground in memory two hundred
thousand times over and far faster; what the sweep reaches that it cannot
is the prompts, which is where a pack that arrived unopened, a cleric
handed nothing for an option it offered, and a race whose menu entry was a
substring of another's were all found.

`--record FILE` writes the answers a run gave, so a failure can be replayed
against the binary directly.

`tools/stress.py` drives the rest of the program. The wizard is one of nine
things the main menu offers, and drive.py only ever answers that one; this
walks all of them in a single session, in an order the seed decides, so what
each screen leaves behind is what the next one starts from -- the character
the wizard saved is reloaded by the inventory screen, the homebrew just added
is in the shop when the next character buys gear, the books switched off in
the settings are gone from every menu after that. It looks for three things:
a crash, a prompt that will not accept an answer inside its own stated
bounds, and a sheet that does not survive being written and read back. A
share of the free text it types is chosen to break the save format -- the
separator, the escape, the block's own markers, a name longer than the field
it goes in.

```sh
python3 tools/stress.py --runs 20 --ops 6   # 20 sessions, six screens each
python3 tools/stress.py --runs 8 --hash     # a digest per session
```

Because every answer is a function of the seed, two builds fed the same seed
produce the same transcript, and `--hash` turns the harness into a check that
a change to the code changed nothing: the digests either match or they do not.

`tools/fuzz_files.py` corrupts the files the program reads rather than the
answers it is given -- the data block of a saved character, which this README
invites you to edit by hand, and homebrew.txt, which a DM writes. It drops
fields, repeats a record more times than the array it fills can hold, poisons
numbers, overruns fixed members, and cuts the block off mid-way, then feeds
each mutant back. It is worth the most against a sanitizer build, which is
where it found the loader writing one entry before the start of the
inventory, and a level of 99 in a hand-edited file reading past the end of
the experience table.

A sanitizer build has to be told to stop, though. Without
`-fno-sanitize-recover` the undefined-behaviour checker prints its
complaint and carries on, the program exits zero, and every harness above
reports success -- which is how five signed overflows on numbers read out
of a character file survived a whole round of this. `make asan` sets it
now, and the numeric columns of both files are held to the ranges the
program's own prompts ask for.

`tools/selftest.c` (`make test`) asserts the rules engine directly: ability
modifiers, proficiency bonus at every level, the PHB's own Bruenor example,
Armor Class for each unarmoured option and armour category, the single-class
and multiclass spell slot tables, Pact Magic, skills and expertise, and the
integrity of the data tables.

It also sweeps the whole of the data. Every race, every subrace, every class
at every one of the twenty levels, every subclass, every spell, every item,
every magic item and every beast -- about fourteen hundred characters -- is
written to a file, read back and written again, and the two files have to be
identical. Text the format cannot take at face value goes through every field
that holds it, a note of two thousand characters has to come back whole, and
the two item tables have to stay apart.

`tools/combos.c` (`make combos`) crosses those sweeps. A bug that needs a
particular race in a particular class at a particular level lives in the
space between them, and nothing was looking there. It builds characters in
memory rather than through the wizard, which is what makes the numbers
affordable -- one costs microseconds -- so it can walk the whole cross
product:

```
every race and subrace x every class x every level     1-20
every race and subrace x every subclass x every level  it can be held at
every subclass x every option it offers                totem, land, rune...
every pair of classes x four level splits, and every three classes together
every background x every class; every feat x every class and every race
every spell x every class; every magic item x every class; every item carried
every armour x seven Dexterities x two Strengths x shield or none
every weapon x every class x three levels
every beast x every kind of sidekick x every level
a character sitting on every array limit at once
all 8,192 combinations of the seven books and the six optional rules
```

That is 202,000 characters, measured in about a second, of which 2,278 go
through the file and back. What it checks at each point is not that a number
is a particular value -- the self-test does that against the book -- but that
it is possible at all, and that the numbers the PHB states as formulas follow
them: a carrying capacity of Strength x 15, a passive Perception of 10 plus
the skill, an initiative of the Dexterity modifier, an unarmoured Armor Class
at least what the class allows, and never more spell slots than the caster
level permits. Breaking any of those in the engine on purpose -- Strength x
14, say -- makes it fail within the first few hundred combinations.

Three of the sweeps check an exact number rather than a range, by working it
out a second way from the tables the character was built from: the speed,
from the race and what is worn; the Armor Class, from the armour's own row
and the Dexterity it admits; and each attack, from the class's proficiency
line and the weapon's category. Those three found five of this round's bugs
between them.

Run under `make asan` it is the same walk with the address and
undefined-behaviour sanitizers watching every table index, which is what it
is really for.

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
data/world.txt         gods, beasts, sidekicks, conditions, tables
homebrew.txt           the DM's own entries, read at run time
tools/build_data.py    data/ -> src/gen_*.c
tools/dump_data.c      src/gen_*.c -> data/, for `make dataverify`
tools/pdf_text.py      the books' PDFs -> TextFiles/, run once
tools/audit.py         checks every name in data/ against TextFiles/
tools/verify_equipment.py  checks the PHB equipment numbers
tools/verify_deities.py    checks appendix B, column by column
tools/verify_races.py      checks the MPMM race numbers
tools/verify_spells.py     checks every spell's stat line
tools/verify_magic_items.py  checks the DMG items' kind, rarity, attunement
tools/verify_beasts.py     checks the MM stat blocks
tools/verify_classes.py    checks the class tables and feature levels
tools/verify_backgrounds.py  checks the backgrounds and their tables
tools/verify_feats.py      checks feat prerequisites and Tasha's features
tools/verify_races_phb.py  checks the PHB and SCAG races
tools/verify_tables.py     checks the tables that are only numbers
tools/verify_reference.py  checks prices, conditions and the life tables
tools/verify_coverage.py   looks for book content data/ is missing
tools/extract_deities.py   writes the DEITY rows from appendix B
tools/stress.py            walks every menu, not just the wizard
tools/combos.c             every combination of race, class, subclass, level
tools/fuzz_files.py        corrupts the files the program reads

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

Seven sourcebooks: PHB, XGE, TCE, DMG (magic items), MPMM (races), MM (beasts)
and SCAG (subraces, subclasses, backgrounds, languages and a feat), plus
whatever the DM adds as homebrew. Any of them
except the PHB can be switched off in the settings menu, which hides their
races, classes, subclasses, spells, feats and items everywhere in the wizard.

The item, magic item and spell banks are pointers rather than fixed arrays,
so `src/homebrew.c` can replace each with the book's entries followed by the
DM's. `ITEMS[i]` reads the same either way, which is why adding homebrew
touched none of the eighty-odd places that read those tables.

TCE's sidekicks come from that book's chapter 4.

### What the sheet works out for you

The point of writing a sheet is not having to redo arithmetic at the table,
so the sheet carries the numbers a player reaches for most:

- **An attacks block.** For every weapon carried: what it hits at and what it
  does. Finesse takes the better of Strength and Dexterity, a ranged weapon
  takes Dexterity, proficiency is added when the character has it and the
  line says so when they do not, the Archery fighting style adds its +2, and a
  monk's unarmed strike and monk weapons use the Martial Arts die for their
  level. A `+1`, `+2` or `+3` weapon names which weapon it is when you pick it
  up, and its bonus lands on both rolls. What stays off the block is anything
  conditional -- Dueling, Great Weapon Fighting, Sneak Attack, Rage -- and the
  sheet says so rather than quietly leaving it out.
- **Where the level sits on the advancement table**, so the distance to the
  next one is on the page rather than in someone's head. Tables that hand
  out levels rather than experience can switch the line off in the settings.
- **The encumbrance thresholds** from the variant rule, and whether the
  character is over them.

Every roll above can be handed back to the player: switch on "roll your own
dice" in the settings and the program asks for the result instead of
generating one, which is what a table that rolls in the open wants.

Height and weight are rolled on the PHB's own table where the books give the
race a row: the roll that sets the inches above the base height is the one
that multiplies the weight dice, which is what keeps a tall character heavy.
The races the table never listed say so and ask instead.

The reference screen carries **appendix A's conditions** alongside the
equipment, because that is the page a table turns to once play has started.

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
