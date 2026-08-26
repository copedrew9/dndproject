/* data_optional.c -- Tasha's Cauldron of Everything, "Optional Class
 * Features" (chapter 1).
 *
 * These are opt-in: the DM decides whether they are available, and a player
 * takes them one at a time. Some replace a Player's Handbook feature, which
 * is recorded so the sheet says what was given up.
 *
 * The "Additional <Class> Spells" features widen a class's spell list. The
 * lists below were read off the book's own tables; the OCR prints them in
 * two interleaved columns, so each was checked line by line against the
 * source. One entry could not be recovered -- see the note on the druid.
 */
#include "data.h"

const OptionalFeature OPTIONAL_FEATURES[] = {
/* class, level, name, replaces, summary */

{ 0, 3, "Primal Knowledge", "",
  "Gain proficiency in one more barbarian skill, and while raging you can "
  "make Strength checks in place of Acrobatics, Intimidation, Perception, "
  "Stealth or Survival." },
{ 0, 7, "Instinctive Pounce", "",
  "When you enter your rage as a bonus action, move up to half your speed." },

{ 1, 1, "Additional Bard Spells", "",
  "Fourteen more spells are added to the bard spell list." },
{ 1, 2, "Magical Inspiration", "",
  "A creature can spend your Bardic Inspiration die to add it to a spell's "
  "damage or healing." },
{ 1, 4, "Bardic Versatility", "",
  "Whenever you gain an Ability Score Improvement, you may swap a cantrip "
  "or replace one Expertise skill." },

{ 2, 1, "Additional Cleric Spells", "",
  "Eight more spells are added to the cleric spell list." },
{ 2, 2, "Harness Divine Power", "",
  "Expend a use of Channel Divinity to regain one expended spell slot." },
{ 2, 4, "Cantrip Versatility", "",
  "Whenever you gain an Ability Score Improvement, you may replace one "
  "cantrip with another from your class list." },
{ 2, 8, "Blessed Strikes", "Divine Strike or Potent Spellcasting",
  "Once per turn, when you hit with a weapon or a cantrip, deal an extra "
  "1d8 radiant damage." },

{ 3, 1, "Additional Druid Spells", "",
  "Fifteen more spells are added to the druid spell list." },
{ 3, 2, "Wild Companion", "",
  "Expend a Wild Shape use to cast find familiar without material "
  "components; the familiar is a fey that lasts half your druid level in "
  "hours." },
{ 3, 4, "Cantrip Versatility", "",
  "Whenever you gain an Ability Score Improvement, you may replace one "
  "cantrip with another from your class list." },

{ 4, 1, "Fighting Style Options", "",
  "Blind Fighting, Interception, Superior Technique, Thrown Weapon Fighting "
  "and Unarmed Fighting join your list of fighting styles." },
{ 4, 3, "Maneuver Options", "",
  "The Battle Master gains further manoeuvres: ambush, bait and switch, "
  "brace, commander's strike, grappling strike, quick toss, tactical "
  "assessment and the rest." },
{ 4, 4, "Martial Versatility", "",
  "Whenever you gain an Ability Score Improvement, you may replace a "
  "fighting style or a Battle Master manoeuvre." },

{ 5, 2, "Dedicated Weapon", "",
  "After a short or long rest, name a simple or martial weapon that lacks "
  "the heavy and special properties; it counts as a monk weapon." },
{ 5, 3, "Ki-Fueled Attack", "",
  "After spending 1 ki or more on an action, make an unarmed strike or a "
  "monk weapon attack as a bonus action." },
{ 5, 4, "Quickened Healing", "",
  "Spend 2 ki as an action to regain a roll of your Martial Arts die plus "
  "your proficiency bonus in hit points." },
{ 5, 5, "Focused Aim", "",
  "Spend 1 to 3 ki after a miss to add 2 per point to the attack roll." },

{ 6, 2, "Additional Paladin Spells", "",
  "Five more spells are added to the paladin spell list." },
{ 6, 2, "Fighting Style Options", "",
  "Blessed Warrior, Blind Fighting and Interception join your list of "
  "fighting styles." },
{ 6, 3, "Harness Divine Power", "",
  "Expend a use of Channel Divinity to regain one expended spell slot." },
{ 6, 4, "Martial Versatility", "",
  "Whenever you gain an Ability Score Improvement, you may replace a "
  "fighting style." },

{ 7, 1, "Deft Explorer", "Natural Explorer",
  "Canny: expertise in one skill, two more languages; later Roving grants "
  "speed and climbing and swimming, and Tireless grants temporary hit "
  "points and reduced exhaustion." },
{ 7, 1, "Favored Foe", "Favored Enemy",
  "Mark a creature you hit for an extra 1d4 damage once per turn while you "
  "concentrate, proficiency-bonus times per long rest." },
{ 7, 2, "Additional Ranger Spells", "",
  "Fourteen more spells are added to the ranger spell list." },
{ 7, 2, "Fighting Style Options", "",
  "Blind Fighting, Druidic Warrior and Thrown Weapon Fighting join your "
  "list of fighting styles." },
{ 7, 2, "Spellcasting Focus", "",
  "You can use a druidic focus as a spellcasting focus for your ranger "
  "spells." },
{ 7, 3, "Primal Awareness", "Primeval Awareness",
  "You always have speak with animals, beast sense, speak with plants, "
  "locate creature and commune with nature prepared, each castable once "
  "per long rest without a slot." },
{ 7, 4, "Martial Versatility", "",
  "Whenever you gain an Ability Score Improvement, you may replace a "
  "fighting style." },
{ 7, 10, "Nature's Veil", "Hide in Plain Sight",
  "As a bonus action, become invisible until the end of your next turn, "
  "proficiency-bonus times per long rest." },

{ 8, 3, "Steady Aim", "",
  "As a bonus action, gain advantage on your next attack this turn; your "
  "speed becomes 0 until the end of the turn." },

{ 9, 1, "Additional Sorcerer Spells", "",
  "Twenty-one more spells are added to the sorcerer spell list." },
{ 9, 3, "Metamagic Options", "",
  "Seeking Spell and Transmuted Spell join your Metamagic choices, and you "
  "may swap one Metamagic option whenever you gain a sorcerer level." },
{ 9, 4, "Sorcerous Versatility", "",
  "Whenever you gain an Ability Score Improvement, you may swap a Metamagic "
  "option or a cantrip." },
{ 9, 5, "Magical Guidance", "",
  "Spend 1 sorcery point to reroll a failed ability check." },

{ 10, 1, "Additional Warlock Spells", "",
  "Twenty more spells are added to the warlock spell list." },
{ 10, 2, "Eldritch Invocation Options", "",
  "Further invocations become available, including Eldritch Smite, Gift of "
  "the Depths, Investment of the Chain Master, Rebuke of the Talisman and "
  "Undying Servitude." },
{ 10, 3, "Pact Boon Option", "",
  "The Pact of the Talisman: an amulet that lets its wearer add a d4 to a "
  "failed ability check." },
{ 10, 4, "Eldritch Versatility", "",
  "Whenever you gain an Ability Score Improvement, you may swap a cantrip, "
  "an invocation, or your Pact Boon." },

{ 11, 1, "Additional Wizard Spells", "",
  "Twenty-three more spells are added to the wizard spell list." },
{ 11, 3, "Cantrip Formulas", "",
  "After each long rest, replace one wizard cantrip you know with another "
  "from your spellbook's formulas." },
};
const int OPTIONAL_FEATURE_COUNT =
    (int)(sizeof(OPTIONAL_FEATURES) / sizeof(OPTIONAL_FEATURES[0]));

/* The spells each "Additional <Class> Spells" feature adds to a class list.
 * Only names are stored; every spell's level and school come from its own
 * entry in the spell database, and every name here is checked at start-up
 * by the self-test.
 *
 * Known gap: the druid table's single 7th-level entry is destroyed in the
 * OCR (it reads "ay" and "erred" across two lines) and the source PDFs are
 * page scans with no recoverable text, so it is omitted rather than guessed.
 */
const AdditionalSpells ADDITIONAL_SPELLS[] = {
{ 1,
  "color spray, command, aid, enlarge/reduce, mirror image, "
  "intellect fortress, mass healing word, phantasmal killer, "
  "rary's telepathic bond, heroes' feast, dream of the blue veil, "
  "prismatic spray, antipathy/sympathy, prismatic wall" },

{ 2,
  "aura of vitality, spirit shroud, aura of life, aura of purity, "
  "summon celestial, sunbeam, sunburst, power word heal" },

{ 3,
  "protection from evil and good, augury, continual flame, enlarge/reduce, "
  "summon beast, aura of vitality, elemental weapon, revivify, summon fey, "
  "divination, fire shield, summon elemental, cone of cold, flesh to stone, "
  "incendiary cloud" },

{ 6,
  "gentle repose, prayer of healing, warding bond, spirit shroud, "
  "summon celestial" },

{ 7,
  "entangle, searing smite, aid, enhance ability, gust of wind, "
  "magic weapon, summon beast, elemental weapon, meld into stone, revivify, "
  "summon fey, dominate beast, summon elemental, greater restoration" },

{ 9,
  "booming blade, green-flame blade, lightning lure, mind sliver, "
  "sword burst, grease, tasha's caustic brew, flame blade, flaming sphere, "
  "magic weapon, tasha's mind whip, intellect fortress, vampiric touch, "
  "fire shield, bigby's hand, flesh to stone, otiluke's freezing sphere, "
  "tasha's otherworldly guise, dream of the blue veil, demiplane, "
  "blade of disaster" },

{ 10,
  "booming blade, green-flame blade, lightning lure, mind sliver, "
  "sword burst, intellect fortress, spirit shroud, summon fey, "
  "summon shadowspawn, summon undead, summon aberration, mislead, "
  "planar binding, teleportation circle, summon fiend, "
  "tasha's otherworldly guise, dream of the blue veil, blade of disaster, "
  "gate, weird" },

{ 11,
  "booming blade, green-flame blade, lightning lure, mind sliver, "
  "sword burst, tasha's caustic brew, augury, enhance ability, "
  "tasha's mind whip, intellect fortress, speak with dead, spirit shroud, "
  "summon fey, summon shadowspawn, summon undead, divination, "
  "summon aberration, summon construct, summon elemental, summon fiend, "
  "tasha's otherworldly guise, dream of the blue veil, blade of disaster" },
};
const int ADDITIONAL_SPELLS_COUNT =
    (int)(sizeof(ADDITIONAL_SPELLS) / sizeof(ADDITIONAL_SPELLS[0]));

/* Fighting styles Tasha's adds, per class. */
const char *const TASHA_FIGHTER_STYLES =
    "Blind Fighting (blindsight 10 feet)|"
    "Interception (reduce damage to a nearby ally by 1d10 + proficiency)|"
    "Superior Technique (one Battle Master manoeuvre and a superiority die)|"
    "Thrown Weapon Fighting (draw as part of the attack, +2 damage)|"
    "Unarmed Fighting (d6 or d8 unarmed strikes, and grapple damage)";
const char *const TASHA_PALADIN_STYLES =
    "Blessed Warrior (two cleric cantrips, cast with Charisma)|"
    "Blind Fighting (blindsight 10 feet)|"
    "Interception (reduce damage to a nearby ally by 1d10 + proficiency)";
const char *const TASHA_RANGER_STYLES =
    "Blind Fighting (blindsight 10 feet)|"
    "Druidic Warrior (two druid cantrips, cast with Wisdom)|"
    "Thrown Weapon Fighting (draw as part of the attack, +2 damage)";
