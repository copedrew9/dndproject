/* data_classes.c -- PHB chapter 3 classes, subclasses and level progressions. */
#include "data.h"
#include <string.h>

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
static const Skill SK_ARTIFICER[] = {
    SKL_ARCANA, SKL_HISTORY, SKL_INVESTIGATION, SKL_MEDICINE, SKL_NATURE,
    SKL_PERCEPTION, SKL_SLEIGHT_OF_HAND };
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

/* Artificer (Tasha's): a half-caster that has spell slots from 1st level. */
static const unsigned char CK_ARTIFICER[21] =
    {0, 2,2,2,2,2,2,2,2,2,3,3,3,3,4,4,4,4,4,4,4};

const unsigned char INFUSIONS_KNOWN[MAX_LEVEL + 1] =
    {0, 0,4,4,4,4,6,6,6,6,8,8,8,8,10,10,10,10,12,12,12};
const unsigned char INFUSED_ITEMS[MAX_LEVEL + 1] =
    {0, 0,2,2,2,2,3,3,3,3,4,4,4,4,5,5,5,5,6,6,6};

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
  "An explorer's pack and four javelins",
  0, 0 },

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
  "Leather armor and a dagger",
  1, 0 },

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
  "A shield and a holy symbol",
  1, 0 },

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
  "Leather armor, an explorer's pack and a druidic focus",
  1, 0 },

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
  "(a) a dungeoneer's pack or (b) an explorer's pack",
  3, 0 },

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
  "10 darts",
  0, 0 },

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
  "Chain mail and a holy symbol",
  2, 0 },

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
  "A longbow and a quiver of 20 arrows",
  2, 0 },

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
  "Leather armor, two daggers and thieves' tools",
  3, 0 },

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
  "Two daggers",
  1, 0 },

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
  "Leather armor, any simple weapon and two daggers",
  1, 0 },

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
  "A spellbook",
  1, 0 },

{ "Artificer", 8, {ABL_CON, ABL_INT},
  "Light armor, medium armor, shields",
  "Simple weapons",
  "Thieves' tools, tinker's tools, one type of artisan's tools of your choice",
  SK_ARTIFICER, NSK(SK_ARTIFICER), 2,
  CAST_HALF, PREP_PREPARED, ABL_INT,
  3, "Artificer Specialist", 0, 0,
  {ABL_INT, ABL_INT}, {13, 0}, 1, 0,
  "Light armor, medium armor, shields, thieves' tools, tinker's tools", 5, 10,
  CK_ARTIFICER, NULL,
  "Highest score Intelligence, then Constitution or Dexterity.",
  "Two simple weapons of your choice|"
  "A light crossbow and 20 bolts|"
  "(a) studded leather armor or (b) scale mail|"
  "Thieves' tools and a dungeoneer's pack",
  1, 1 },
};
const int CLASS_COUNT = (int)(sizeof(CLASSES) / sizeof(CLASSES[0]));

/* ------------------------------------------------------------- lookups */

int subclass_by_name(const char *name)
{
    int i;
    for (i = 0; i < SUBCLASS_COUNT; i++) {
        if (strcmp(SUBCLASSES[i].name, name) == 0) return i;
    }
    return -1;
}

int subclass_is(int subclass_id, const char *name)
{
    if (subclass_id < 0 || subclass_id >= SUBCLASS_COUNT) return 0;
    return strcmp(SUBCLASSES[subclass_id].name, name) == 0;
}

int subclasses_of(int class_id, int *out, int max)
{
    int i, n = 0;
    for (i = 0; i < SUBCLASS_COUNT && n < max; i++) {
        if (SUBCLASSES[i].class_id == class_id) out[n++] = i;
    }
    return n;
}

int class_by_name(const char *name)
{
    int i;
    for (i = 0; i < CLASS_COUNT; i++) {
        if (strcmp(CLASSES[i].name, name) == 0) return i;
    }
    return -1;
}
