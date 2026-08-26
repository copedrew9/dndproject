/* data_classes.c -- PHB chapter 3 classes, subclasses and level progressions. */
#include "data.h"

/* ------------------------------------------------------ skill option lists */

static const Skill SK_BARBARIAN[] = {
    SKL_ANIMAL_HANDLING, SKL_ATHLETICS, SKL_INTIMIDATION, SKL_NATURE,
    SKL_PERCEPTION, SKL_SURVIVAL };
static const Skill SK_ANY[] = {
    SKL_ACROBATICS, SKL_ANIMAL_HANDLING, SKL_ARCANA, SKL_ATHLETICS,
    SKL_DECEPTION, SKL_HISTORY, SKL_INSIGHT, SKL_INTIMIDATION,
    SKL_INVESTIGATION, SKL_MEDICINE, SKL_NATURE, SKL_PERCEPTION,
    SKL_PERFORMANCE, SKL_PERSUASION, SKL_RELIGION, SKL_SLEIGHT_OF_HAND,
    SKL_STEALTH, SKL_SURVIVAL };
static const Skill SK_CLERIC[] = {
    SKL_HISTORY, SKL_INSIGHT, SKL_MEDICINE, SKL_PERSUASION, SKL_RELIGION };
static const Skill SK_DRUID[] = {
    SKL_ARCANA, SKL_ANIMAL_HANDLING, SKL_INSIGHT, SKL_MEDICINE, SKL_NATURE,
    SKL_PERCEPTION, SKL_RELIGION, SKL_SURVIVAL };
static const Skill SK_FIGHTER[] = {
    SKL_ACROBATICS, SKL_ANIMAL_HANDLING, SKL_ATHLETICS, SKL_HISTORY,
    SKL_INSIGHT, SKL_INTIMIDATION, SKL_PERCEPTION, SKL_SURVIVAL };
static const Skill SK_MONK[] = {
    SKL_ACROBATICS, SKL_ATHLETICS, SKL_HISTORY, SKL_INSIGHT, SKL_RELIGION,
    SKL_STEALTH };
static const Skill SK_PALADIN[] = {
    SKL_ATHLETICS, SKL_INSIGHT, SKL_INTIMIDATION, SKL_MEDICINE,
    SKL_PERSUASION, SKL_RELIGION };
static const Skill SK_RANGER[] = {
    SKL_ANIMAL_HANDLING, SKL_ATHLETICS, SKL_INSIGHT, SKL_INVESTIGATION,
    SKL_NATURE, SKL_PERCEPTION, SKL_STEALTH, SKL_SURVIVAL };
static const Skill SK_ROGUE[] = {
    SKL_ACROBATICS, SKL_ATHLETICS, SKL_DECEPTION, SKL_INSIGHT,
    SKL_INTIMIDATION, SKL_INVESTIGATION, SKL_PERCEPTION, SKL_PERFORMANCE,
    SKL_PERSUASION, SKL_SLEIGHT_OF_HAND, SKL_STEALTH };
static const Skill SK_SORCERER[] = {
    SKL_ARCANA, SKL_DECEPTION, SKL_INSIGHT, SKL_INTIMIDATION,
    SKL_PERSUASION, SKL_RELIGION };
static const Skill SK_WARLOCK[] = {
    SKL_ARCANA, SKL_DECEPTION, SKL_HISTORY, SKL_INTIMIDATION,
    SKL_INVESTIGATION, SKL_NATURE, SKL_RELIGION };
static const Skill SK_WIZARD[] = {
    SKL_ARCANA, SKL_HISTORY, SKL_INSIGHT, SKL_INVESTIGATION, SKL_MEDICINE,
    SKL_RELIGION };

#define NSK(a) (int)(sizeof(a) / sizeof((a)[0]))

/* ----------------------------------------------- cantrips / spells known */
/* Index by class level; index 0 is unused. */

static const unsigned char CK_BARD[21] =
    {0, 2,2,2,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4};
static const unsigned char SK_KNOWN_BARD[21] =
    {0, 4,5,6,7,8,9,10,11,12,14,15,15,16,18,19,19,20,22,22,22};

static const unsigned char CK_CLERIC[21] =
    {0, 3,3,3,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5};
static const unsigned char CK_DRUID[21] =
    {0, 2,2,2,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4};
static const unsigned char CK_WIZARD[21] =
    {0, 3,3,3,4,4,4,4,4,4,5,5,5,5,5,5,5,5,5,5,5};

static const unsigned char CK_SORCERER[21] =
    {0, 4,4,4,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6};
static const unsigned char SK_KNOWN_SORCERER[21] =
    {0, 2,3,4,5,6,7,8,9,10,11,12,12,13,13,14,14,15,15,15,15};

static const unsigned char CK_WARLOCK[21] =
    {0, 2,2,2,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4};
static const unsigned char SK_KNOWN_WARLOCK[21] =
    {0, 2,3,4,5,6,7,8,9,10,10,11,11,12,12,13,13,14,14,15,15};

static const unsigned char SK_KNOWN_RANGER[21] =
    {0, 0,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11};

/* Eldritch Knight and Arcane Trickster: third-casters. */
static const unsigned char CK_THIRD[21] =
    {0, 0,0,2,2,2,2,2,2,2,3,3,3,3,3,3,3,3,3,3,3};
static const unsigned char SK_KNOWN_THIRD[21] =
    {0, 0,0,3,4,4,4,5,6,6,7,8,8,9,10,10,11,11,11,12,13};

const unsigned char *const THIRD_CANTRIPS = CK_THIRD;
const unsigned char *const THIRD_SPELLS_KNOWN = SK_KNOWN_THIRD;

/* -------------------------------------------------------- spell slot tables */

/* FULL_SLOTS[caster level][spell level]; spell level 0 is unused. */
const unsigned char FULL_SLOTS[MAX_LEVEL + 1][10] = {
    {0,0,0,0,0,0,0,0,0,0},
    {0,2,0,0,0,0,0,0,0,0},   /*  1 */
    {0,3,0,0,0,0,0,0,0,0},   /*  2 */
    {0,4,2,0,0,0,0,0,0,0},   /*  3 */
    {0,4,3,0,0,0,0,0,0,0},   /*  4 */
    {0,4,3,2,0,0,0,0,0,0},   /*  5 */
    {0,4,3,3,0,0,0,0,0,0},   /*  6 */
    {0,4,3,3,1,0,0,0,0,0},   /*  7 */
    {0,4,3,3,2,0,0,0,0,0},   /*  8 */
    {0,4,3,3,3,1,0,0,0,0},   /*  9 */
    {0,4,3,3,3,2,0,0,0,0},   /* 10 */
    {0,4,3,3,3,2,1,0,0,0},   /* 11 */
    {0,4,3,3,3,2,1,0,0,0},   /* 12 */
    {0,4,3,3,3,2,1,1,0,0},   /* 13 */
    {0,4,3,3,3,2,1,1,0,0},   /* 14 */
    {0,4,3,3,3,2,1,1,1,0},   /* 15 */
    {0,4,3,3,3,2,1,1,1,0},   /* 16 */
    {0,4,3,3,3,2,1,1,1,1},   /* 17 */
    {0,4,3,3,3,3,1,1,1,1},   /* 18 */
    {0,4,3,3,3,3,2,1,1,1},   /* 19 */
    {0,4,3,3,3,3,2,2,1,1},   /* 20 */
};

/* Warlock Pact Magic: {number of slots, slot level}. */
const unsigned char PACT_SLOTS[MAX_LEVEL + 1][2] = {
    {0,0},
    {1,1},{2,1},{2,2},{2,2},{2,3},{2,3},{2,4},{2,4},{2,5},{2,5},
    {3,5},{3,5},{3,5},{3,5},{3,5},{3,5},{4,5},{4,5},{4,5},{4,5},
};

/* ------------------------------------------------------------- subclasses */

const SubclassData SUBCLASSES[] = {
    /* --- barbarian (0-1) --- */
    { 0, "Path of the Berserker",
      "Fury in battle: frenzy grants a bonus attack at the cost of exhaustion.",
      "", "", "" },
    { 0, "Path of the Totem Warrior",
      "A spirit animal guides you, granting resilience or ferocity.",
      "", "Totem spirit", "Bear|Eagle|Wolf" },

    /* --- bard (2-3) --- */
    { 1, "College of Lore",
      "Knowledge and cutting words; extra skills and additional magical secrets.",
      "", "", "" },
    { 1, "College of Valor",
      "A battle skald: martial training and inspiration that aids attacks.",
      "", "", "" },

    /* --- cleric (4-10) --- */
    { 2, "Knowledge Domain",
      "The pursuit of learning; you read thoughts and borrow proficiencies.",
      "command, identify|augury, suggestion|nondetection, speak with dead|"
      "arcane eye, confusion|legend lore, scrying", "", "" },
    { 2, "Life Domain",
      "Healing and vitality; your cures are more potent than others'.",
      "bless, cure wounds|lesser restoration, spiritual weapon|"
      "beacon of hope, revivify|death ward, guardian of faith|"
      "mass cure wounds, raise dead", "", "" },
    { 2, "Light Domain",
      "Radiance and fire; you blind foes and shield allies with light.",
      "burning hands, faerie fire|flaming sphere, scorching ray|"
      "daylight, fireball|guardian of faith, wall of fire|"
      "flame strike, scrying", "", "" },
    { 2, "Nature Domain",
      "The natural world; you command beasts and plants.",
      "animal friendship, speak with animals|barkskin, spike growth|"
      "plant growth, wind wall|dominate beast, grasping vine|"
      "insect plague, tree stride", "", "" },
    { 2, "Tempest Domain",
      "Storm and thunder; you strike with maximised lightning and thunder.",
      "fog cloud, thunderwave|gust of wind, shatter|"
      "call lightning, sleet storm|control water, ice storm|"
      "destructive wave, insect plague", "", "" },
    { 2, "Trickery Domain",
      "Deception and stealth; you create illusory duplicates and bless allies "
      "with stealth.",
      "charm person, disguise self|mirror image, pass without trace|"
      "blink, dispel magic|dimension door, polymorph|"
      "dominate person, modify memory", "", "" },
    { 2, "War Domain",
      "Battle prowess; bonus attacks and divine strikes.",
      "divine favor, shield of faith|magic weapon, spiritual weapon|"
      "crusader's mantle, spirit guardians|freedom of movement, stoneskin|"
      "flame strike, hold monster", "", "" },

    /* --- druid (11-12) --- */
    { 3, "Circle of the Land",
      "A druid of a particular terrain, with extra spells and recovery.",
      "", "Land type",
      "Arctic|Coast|Desert|Forest|Grassland|Mountain|Swamp|Underdark" },
    { 3, "Circle of the Moon",
      "A shapeshifter: wild shape as a bonus action into fiercer forms.",
      "", "", "" },

    /* --- fighter (13-15) --- */
    { 4, "Champion",
      "Simple, relentless martial excellence; improved critical hits.",
      "", "", "" },
    { 4, "Battle Master",
      "Tactical manoeuvres fuelled by superiority dice.",
      "", "", "" },
    { 4, "Eldritch Knight",
      "A fighter who weaves wizard magic into swordplay (third-caster).",
      "", "", "" },

    /* --- monk (16-18) --- */
    { 5, "Way of the Open Hand",
      "Mastery of unarmed combat; manipulate a foe's ki.",
      "", "", "" },
    { 5, "Way of Shadow",
      "A ninja of stealth and darkness, stepping between shadows.",
      "", "", "" },
    { 5, "Way of the Four Elements",
      "Bend the elements to your will through ki-fuelled disciplines.",
      "", "", "" },

    /* --- paladin (19-21) --- */
    { 6, "Oath of Devotion",
      "The classic knight in shining armour: honesty, courage, duty.",
      "protection from evil and good, sanctuary|"
      "lesser restoration, zone of truth|beacon of hope, dispel magic|"
      "freedom of movement, guardian of faith|commune, flame strike", "", "" },
    { 6, "Oath of the Ancients",
      "A green knight preserving light and life in the world.",
      "ensnaring strike, speak with animals|moonbeam, misty step|"
      "plant growth, protection from energy|ice storm, stoneskin|"
      "commune with nature, tree stride", "", "" },
    { 6, "Oath of Vengeance",
      "A dark avenger who punishes wrongdoers at any cost.",
      "bane, hunter's mark|hold person, misty step|"
      "haste, protection from energy|banishment, dimension door|"
      "hold monster, scrying", "", "" },

    /* --- ranger (22-23) --- */
    { 7, "Hunter",
      "A monster slayer with tactics tuned to the prey you face.",
      "", "", "" },
    { 7, "Beast Master",
      "You bond with an animal companion that fights alongside you.",
      "", "", "" },

    /* --- rogue (24-26) --- */
    { 8, "Thief",
      "Fast hands, climbing and the use of magic items others cannot.",
      "", "", "" },
    { 8, "Assassin",
      "Disguise, poison and devastating strikes against the unready.",
      "", "", "" },
    { 8, "Arcane Trickster",
      "A rogue who enhances stealth and mischief with wizard magic "
      "(third-caster).",
      "", "", "" },

    /* --- sorcerer (27-28) --- */
    { 9, "Draconic Bloodline",
      "Dragon blood grants resilience, tougher skin and elemental affinity.",
      "", "Dragon ancestor",
      "Black (acid)|Blue (lightning)|Brass (fire)|Bronze (lightning)|"
      "Copper (acid)|Gold (fire)|Green (poison)|Red (fire)|Silver (cold)|"
      "White (cold)" },
    { 9, "Wild Magic",
      "Raw chaos: your magic sometimes surges beyond your control.",
      "", "", "" },

    /* --- warlock (29-31) --- */
    { 10, "The Archfey",
      "A patron of the Feywild; charm, escape and beguilement.",
      "faerie fire, sleep|calm emotions, phantasmal force|"
      "blink, plant growth|dominate beast, greater invisibility|"
      "dominate person, seeming", "", "" },
    { 10, "The Fiend",
      "A patron from the lower planes; fire, temptation and dark luck.",
      "burning hands, command|blindness/deafness, scorching ray|"
      "fireball, stinking cloud|fire shield, wall of fire|"
      "flame strike, hallow", "", "" },
    { 10, "The Great Old One",
      "An alien intelligence; telepathy and psychic domination.",
      "dissonant whispers, Tasha's hideous laughter|"
      "detect thoughts, phantasmal force|clairvoyance, sending|"
      "dominate beast, Evard's black tentacles|"
      "dominate person, telekinesis", "", "" },

    /* --- wizard (32-39) --- */
    { 11, "School of Abjuration",
      "Protective magic; an arcane ward absorbs damage for you.", "", "", "" },
    { 11, "School of Conjuration",
      "Summoning and teleportation; conjure objects and blink between spaces.",
      "", "", "" },
    { 11, "School of Divination",
      "Glimpse the future; portent replaces rolls with foreseen numbers.",
      "", "", "" },
    { 11, "School of Enchantment",
      "Charm and compulsion; bend minds to your will.", "", "", "" },
    { 11, "School of Evocation",
      "Elemental destruction, sculpted so allies are spared.", "", "", "" },
    { 11, "School of Illusion",
      "Deception made real; illusions you can reshape at will.", "", "", "" },
    { 11, "School of Necromancy",
      "Life and death; harvest life force and command the undead.", "", "", "" },
    { 11, "School of Transmutation",
      "Change matter and form; a transmuter's stone grants shifting benefits.",
      "", "", "" },
};
const int SUBCLASS_COUNT = (int)(sizeof(SUBCLASSES) / sizeof(SUBCLASSES[0]));

/* ---------------------------------------------------------------- classes */

const ClassData CLASSES[] = {
{ "Barbarian", 12, {ABL_STR, ABL_CON},
  "Light armor, medium armor, shields",
  "Simple weapons, martial weapons", "",
  SK_BARBARIAN, NSK(SK_BARBARIAN), 2,
  CAST_NONE, PREP_NONE, ABL_STR,
  3, "Primal Path", 0, 2,
  {ABL_STR, ABL_STR}, {13, 0}, 1, 0,
  "Shields, simple weapons, martial weapons", 2, 10,
  NULL, NULL,
  "Highest score Strength, then Constitution.",
  "(a) a greataxe or (b) any martial melee weapon|"
  "(a) two handaxes or (b) any simple weapon|"
  "An explorer's pack and four javelins" },

{ "Bard", 8, {ABL_DEX, ABL_CHA},
  "Light armor",
  "Simple weapons, hand crossbows, longswords, rapiers, shortswords",
  "Three musical instruments of your choice",
  SK_ANY, NSK(SK_ANY), 3,
  CAST_FULL, PREP_KNOWN, ABL_CHA,
  3, "Bard College", 2, 2,
  {ABL_CHA, ABL_CHA}, {13, 0}, 1, 0,
  "Light armor, one skill of your choice, one musical instrument", 5, 10,
  CK_BARD, SK_KNOWN_BARD,
  "Highest score Charisma, then Dexterity.",
  "(a) a rapier, (b) a longsword or (c) any simple weapon|"
  "(a) a diplomat's pack or (b) an entertainer's pack|"
  "(a) a lute or (b) any other musical instrument|"
  "Leather armor and a dagger" },

{ "Cleric", 8, {ABL_WIS, ABL_CHA},
  "Light armor, medium armor, shields",
  "All simple weapons", "",
  SK_CLERIC, NSK(SK_CLERIC), 2,
  CAST_FULL, PREP_PREPARED, ABL_WIS,
  1, "Divine Domain", 4, 7,
  {ABL_WIS, ABL_WIS}, {13, 0}, 1, 0,
  "Light armor, medium armor, shields", 5, 10,
  CK_CLERIC, NULL,
  "Highest score Wisdom, then Constitution or Strength.",
  "(a) a mace or (b) a warhammer (if proficient)|"
  "(a) scale mail, (b) leather armor or (c) chain mail (if proficient)|"
  "(a) a light crossbow and 20 bolts or (b) any simple weapon|"
  "(a) a priest's pack or (b) an explorer's pack|"
  "A shield and a holy symbol" },

{ "Druid", 8, {ABL_INT, ABL_WIS},
  "Light armor, medium armor, shields (druids will not wear metal armor)",
  "Clubs, daggers, darts, javelins, maces, quarterstaffs, scimitars, "
  "sickles, slings, spears",
  "Herbalism kit",
  SK_DRUID, NSK(SK_DRUID), 2,
  CAST_FULL, PREP_PREPARED, ABL_WIS,
  2, "Druid Circle", 11, 2,
  {ABL_WIS, ABL_WIS}, {13, 0}, 1, 0,
  "Light armor, medium armor, shields (nonmetal)", 2, 10,
  CK_DRUID, NULL,
  "Highest score Wisdom, then Constitution.",
  "(a) a wooden shield or (b) any simple weapon|"
  "(a) a scimitar or (b) any simple melee weapon|"
  "Leather armor, an explorer's pack and a druidic focus" },

{ "Fighter", 10, {ABL_STR, ABL_CON},
  "All armor, shields",
  "Simple weapons, martial weapons", "",
  SK_FIGHTER, NSK(SK_FIGHTER), 2,
  CAST_NONE, PREP_NONE, ABL_INT,
  3, "Martial Archetype", 13, 3,
  {ABL_STR, ABL_DEX}, {13, 13}, 2, 1,
  "Light armor, medium armor, shields, simple weapons, martial weapons", 5, 10,
  NULL, NULL,
  "Highest score Strength or Dexterity, then Constitution.",
  "(a) chain mail or (b) leather armor, longbow and 20 arrows|"
  "(a) a martial weapon and a shield or (b) two martial weapons|"
  "(a) a light crossbow and 20 bolts or (b) two handaxes|"
  "(a) a dungeoneer's pack or (b) an explorer's pack" },

{ "Monk", 8, {ABL_STR, ABL_DEX},
  "None",
  "Simple weapons, shortswords",
  "One artisan's tool or one musical instrument of your choice",
  SK_MONK, NSK(SK_MONK), 2,
  CAST_NONE, PREP_NONE, ABL_WIS,
  3, "Monastic Tradition", 16, 3,
  {ABL_DEX, ABL_WIS}, {13, 13}, 2, 0,
  "Simple weapons, shortswords", 5, 1,
  NULL, NULL,
  "Highest score Dexterity, then Wisdom.",
  "(a) a shortsword or (b) any simple weapon|"
  "(a) a dungeoneer's pack or (b) an explorer's pack|"
  "10 darts" },

{ "Paladin", 10, {ABL_WIS, ABL_CHA},
  "All armor, shields",
  "Simple weapons, martial weapons", "",
  SK_PALADIN, NSK(SK_PALADIN), 2,
  CAST_HALF, PREP_PREPARED, ABL_CHA,
  3, "Sacred Oath", 19, 3,
  {ABL_STR, ABL_CHA}, {13, 13}, 2, 0,
  "Light armor, medium armor, shields, simple weapons, martial weapons", 5, 10,
  NULL, NULL,
  "Highest score Strength, then Charisma.",
  "(a) a martial weapon and a shield or (b) two martial weapons|"
  "(a) five javelins or (b) any simple melee weapon|"
  "(a) a priest's pack or (b) an explorer's pack|"
  "Chain mail and a holy symbol" },

{ "Ranger", 10, {ABL_STR, ABL_DEX},
  "Light armor, medium armor, shields",
  "Simple weapons, martial weapons", "",
  SK_RANGER, NSK(SK_RANGER), 3,
  CAST_HALF, PREP_KNOWN, ABL_WIS,
  3, "Ranger Archetype", 22, 2,
  {ABL_DEX, ABL_WIS}, {13, 13}, 2, 0,
  "Light armor, medium armor, shields, simple weapons, martial weapons, "
  "one skill from the class list", 5, 10,
  NULL, SK_KNOWN_RANGER,
  "Highest score Dexterity, then Wisdom.",
  "(a) scale mail or (b) leather armor|"
  "(a) two shortswords or (b) two simple melee weapons|"
  "(a) a dungeoneer's pack or (b) an explorer's pack|"
  "A longbow and a quiver of 20 arrows" },

{ "Rogue", 8, {ABL_DEX, ABL_INT},
  "Light armor",
  "Simple weapons, hand crossbows, longswords, rapiers, shortswords",
  "Thieves' tools",
  SK_ROGUE, NSK(SK_ROGUE), 4,
  CAST_NONE, PREP_NONE, ABL_INT,
  3, "Roguish Archetype", 24, 3,
  {ABL_DEX, ABL_DEX}, {13, 0}, 1, 0,
  "Light armor, one skill from the class list, thieves' tools", 4, 10,
  NULL, NULL,
  "Highest score Dexterity, then Intelligence or Charisma.",
  "(a) a rapier or (b) a shortsword|"
  "(a) a shortbow and quiver of 20 arrows or (b) a shortsword|"
  "(a) a burglar's pack, (b) a dungeoneer's pack or (c) an explorer's pack|"
  "Leather armor, two daggers and thieves' tools" },

{ "Sorcerer", 6, {ABL_CON, ABL_CHA},
  "None",
  "Daggers, darts, slings, quarterstaffs, light crossbows", "",
  SK_SORCERER, NSK(SK_SORCERER), 2,
  CAST_FULL, PREP_KNOWN, ABL_CHA,
  1, "Sorcerous Origin", 27, 2,
  {ABL_CHA, ABL_CHA}, {13, 0}, 1, 0,
  "None", 3, 10,
  CK_SORCERER, SK_KNOWN_SORCERER,
  "Highest score Charisma, then Constitution.",
  "(a) a light crossbow and 20 bolts or (b) any simple weapon|"
  "(a) a component pouch or (b) an arcane focus|"
  "(a) a dungeoneer's pack or (b) an explorer's pack|"
  "Two daggers" },

{ "Warlock", 8, {ABL_WIS, ABL_CHA},
  "Light armor",
  "Simple weapons", "",
  SK_WARLOCK, NSK(SK_WARLOCK), 2,
  CAST_PACT, PREP_KNOWN, ABL_CHA,
  1, "Otherworldly Patron", 29, 3,
  {ABL_CHA, ABL_CHA}, {13, 0}, 1, 0,
  "Light armor, simple weapons", 4, 10,
  CK_WARLOCK, SK_KNOWN_WARLOCK,
  "Highest score Charisma, then Constitution.",
  "(a) a light crossbow and 20 bolts or (b) any simple weapon|"
  "(a) a component pouch or (b) an arcane focus|"
  "(a) a scholar's pack or (b) a dungeoneer's pack|"
  "Leather armor, any simple weapon and two daggers" },

{ "Wizard", 6, {ABL_INT, ABL_WIS},
  "None",
  "Daggers, darts, slings, quarterstaffs, light crossbows", "",
  SK_WIZARD, NSK(SK_WIZARD), 2,
  CAST_FULL, PREP_SPELLBOOK, ABL_INT,
  2, "Arcane Tradition", 32, 8,
  {ABL_INT, ABL_INT}, {13, 0}, 1, 0,
  "None", 4, 10,
  CK_WIZARD, NULL,
  "Highest score Intelligence, then Constitution or Dexterity.",
  "(a) a quarterstaff or (b) a dagger|"
  "(a) a component pouch or (b) an arcane focus|"
  "(a) a scholar's pack or (b) an explorer's pack|"
  "A spellbook" },
};
const int CLASS_COUNT = (int)(sizeof(CLASSES) / sizeof(CLASSES[0]));
