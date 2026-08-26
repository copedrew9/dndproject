/* data_sidekicks.c -- Tasha's sidekick classes.
 *
 * A sidekick is any stat block of challenge 1/2 or lower given levels in one
 * of three simple classes. The creature supplies the ability scores, hit
 * die, speed and attacks; the class supplies proficiency, features and, for
 * the Spellcaster, spells.
 *
 * The three tables in the book are printed as columns beside their prose.
 * The Expert's table survived the text dump intact, and the Warrior's and
 * Spellcaster's feature levels are all stated in the prose of the features
 * themselves, so those were read off rather than guessed. Two numbers did
 * not survive and are marked where they are used:
 *
 *   - The Spellcaster's Spells Known column is destroyed in both the text
 *     dump and the PDF's text layer (which is a subset-font encoding with no
 *     ToUnicode map). SPELLCASTER_SPELLS_KNOWN below is a reconstruction,
 *     and the program says so when it uses it.
 *   - The Spellcaster's Ability Score Improvement levels are given only in
 *     that same column. 4/8/12/16/19 is the standard progression and is what
 *     the surviving fragment is consistent with.
 */
#include "data.h"

const char *const SIDEKICK_CLASS_NAME[SK_CLASS_COUNT] = {
    "Expert", "Spellcaster", "Warrior"
};

const char *const SIDEKICK_CLASS_BLURB[SK_CLASS_COUNT] = {
    "A master of certain tasks or knowledge, favouring cunning over brawn: "
    "a scout, a musician, a librarian, a clever street kid or a burglar. "
    "Needs a language it can speak.",
    "Walks the paths of magic: a hedge wizard, a priest, a soothsayer or a "
    "magical performer. Needs a language it can speak.",
    "Grows in martial prowess: a soldier, a town guard, a battle-trained "
    "beast or anything else honed for combat."
};

/* ------------------------------------------------------------- the features */

const SidekickFeature SIDEKICK_FEATURES[] = {

/* --- Expert --- */
{ SK_EXPERT, 1, "Bonus Proficiencies",
  "Proficiency in one saving throw of Dexterity, Intelligence or Charisma; "
  "five skills; light armor; and, if a humanoid or armed, all simple weapons "
  "and two tools." },
{ SK_EXPERT, 1, "Helpful",
  "The sidekick can take the Help action as a bonus action." },
{ SK_EXPERT, 2, "Cunning Action",
  "On its turn it can take the Dash, Disengage or Hide action as a bonus "
  "action." },
{ SK_EXPERT, 3, "Expertise",
  "Choose two of its skill proficiencies; its proficiency bonus is doubled "
  "for any ability check using them." },
{ SK_EXPERT, 4, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_EXPERT, 6, "Coordinated Strike",
  "When it uses Helpful to aid an ally attacking a creature, the target can "
  "be up to 30 feet away, and it deals an extra 2d6 damage the next time it "
  "hits that target before the end of the turn." },
{ SK_EXPERT, 7, "Evasion",
  "On a Dexterity save for half damage it takes none on a success and half "
  "on a failure. Not while incapacitated." },
{ SK_EXPERT, 8, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_EXPERT, 10, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_EXPERT, 11, "Inspiring Help",
  "When it takes the Help action, the creature helped adds 1d6 to the d20 "
  "roll, or to the damage of a hit if it forgoes the bonus to the attack." },
{ SK_EXPERT, 12, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_EXPERT, 14, "Reliable Talent",
  "On any ability check that includes its whole proficiency bonus, treat a "
  "d20 roll of 9 or lower as a 10." },
{ SK_EXPERT, 15, "Expertise",
  "Choose two more skill proficiencies to gain the doubled bonus." },
{ SK_EXPERT, 16, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_EXPERT, 18, "Sharp Mind",
  "Proficiency in one saving throw of Intelligence, Wisdom or Charisma." },
{ SK_EXPERT, 19, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_EXPERT, 20, "Inspiring Help improves",
  "The Inspiring Help bonus increases to 2d6." },

/* --- Spellcaster --- */
{ SK_SPELLCASTER, 1, "Bonus Proficiencies",
  "Proficiency in one saving throw of Wisdom, Intelligence or Charisma; two "
  "skills from Arcana, History, Insight, Investigation, Medicine, "
  "Performance, Persuasion and Religion; light armor; and, if a humanoid or "
  "armed, all simple weapons." },
{ SK_SPELLCASTER, 1, "Spellcasting",
  "Choose a role -- Mage (wizard list, Intelligence), Healer (cleric and "
  "druid, Wisdom) or Prodigy (bard and warlock, Charisma). Spell save DC is "
  "8 + proficiency + the ability modifier." },
{ SK_SPELLCASTER, 4, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_SPELLCASTER, 6, "Potent Cantrips",
  "Add its spellcasting ability modifier to the damage of any cantrip." },
{ SK_SPELLCASTER, 8, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_SPELLCASTER, 12, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_SPELLCASTER, 14, "Empowered Spells",
  "Choose a school of magic; add its spellcasting ability modifier to the "
  "damage or healing of any spell of that school cast with a slot." },
{ SK_SPELLCASTER, 16, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_SPELLCASTER, 19, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_SPELLCASTER, 20, "Focused Casting",
  "Taking damage cannot break its concentration on a spell." },

/* --- Warrior --- */
{ SK_WARRIOR, 1, "Bonus Proficiencies",
  "Proficiency in one saving throw of Strength, Dexterity or Constitution; "
  "two skills from Acrobatics, Animal Handling, Athletics, Intimidation, "
  "Nature, Perception and Survival; all armor; and, if a humanoid or armed, "
  "shields and all simple and martial weapons." },
{ SK_WARRIOR, 1, "Martial Role",
  "Attacker: +2 to all attack rolls. Defender: use its reaction to impose "
  "disadvantage on an attack by a creature within 5 feet aimed at someone "
  "else." },
{ SK_WARRIOR, 2, "Second Wind",
  "As a bonus action, regain 1d10 + its class level hit points, once per "
  "short or long rest." },
{ SK_WARRIOR, 3, "Improved Critical",
  "Its attack rolls score a critical hit on a 19 or 20." },
{ SK_WARRIOR, 4, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_WARRIOR, 6, "Extra Attack",
  "Attack twice instead of once when it takes the Attack action. It can use "
  "Extra Attack or Multiattack on a turn, not both." },
{ SK_WARRIOR, 7, "Battle Readiness",
  "Advantage on initiative rolls." },
{ SK_WARRIOR, 8, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_WARRIOR, 10, "Improved Defense",
  "Its Armor Class increases by 1." },
{ SK_WARRIOR, 11, "Indomitable",
  "Reroll a failed saving throw, using the new roll, once per long rest." },
{ SK_WARRIOR, 12, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_WARRIOR, 14, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_WARRIOR, 15, "Extra Attack improves",
  "The number of attacks increases to three." },
{ SK_WARRIOR, 16, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_WARRIOR, 18, "Indomitable improves",
  "Indomitable can be used twice between long rests." },
{ SK_WARRIOR, 19, "Ability Score Improvement",
  "Raise one ability by 2, or two abilities by 1, to a maximum of 20." },
{ SK_WARRIOR, 20, "Second Wind improves",
  "Second Wind can be used twice between rests." },
};
const int SIDEKICK_FEATURE_COUNT =
    (int)(sizeof(SIDEKICK_FEATURES) / sizeof(SIDEKICK_FEATURES[0]));

/* ------------------------------------------------------------ spellcasting */

const char *const SPELLCASTER_ROLE_NAME[SK_ROLE_COUNT] = {
    "Mage", "Healer", "Prodigy"
};
const char *const SPELLCASTER_ROLE_DESC[SK_ROLE_COUNT] = {
    "Wizard spell list, Intelligence, an arcane focus",
    "Cleric and druid spell lists, Wisdom, a holy symbol",
    "Bard and warlock spell lists, Charisma, an arcane focus or instrument"
};

/* Cantrips known: 2 at 1st, 3 at 4th, 4 at 10th. */
const unsigned char SPELLCASTER_CANTRIPS[MAX_LEVEL + 1] = {
    0, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
};

/* Spells known. The book's column did not survive either source; this is a
 * reconstruction anchored on the two things the prose does state -- one
 * 1st-level spell at 1st level, and exactly one new spell on reaching 5th.
 * sidekick.c tells the player this number is reconstructed so the DM can
 * overrule it. */
const unsigned char SPELLCASTER_SPELLS_KNOWN[MAX_LEVEL + 1] = {
    0, 1, 2, 3, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12
};
