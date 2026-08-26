/* data_races.c -- PHB chapter 2 races and subraces. */
#include "data.h"

/* Subraces are grouped by parent race; RACES indexes into this table. */
const SubraceData SUBRACES[] = {
    /* 0-1 dwarf */
    { "Hill Dwarf", BOOK_PHB, {0,0,0,0,1,0}, 0, 0, 0, 0, 0, 0, 0,
      "Dwarven Toughness: your hit point maximum increases by 1, and by 1 "
      "again every time you gain a level" },
    { "Mountain Dwarf", BOOK_PHB, {2,0,0,0,0,0}, 0, 0, 0, 0, 0, 0, 0,
      "Dwarven Armor Training: proficiency with light and medium armor" },

    /* 2-4 elf */
    { "High Elf", BOOK_PHB, {0,0,0,1,0,0}, 0, 0, 1, 0, 0, 0, 0,
      "Elf Weapon Training: longsword, shortsword, shortbow, longbow|"
      "Cantrip: one wizard cantrip, cast with Intelligence" },
    { "Wood Elf", BOOK_PHB, {0,0,0,0,1,0}, 35, 0, 0, 0, 0, 0, 0,
      "Elf Weapon Training: longsword, shortsword, shortbow, longbow|"
      "Fleet of Foot: base walking speed 35 feet|"
      "Mask of the Wild: hide when lightly obscured by natural phenomena" },
    { "Dark Elf (Drow)", BOOK_PHB, {0,0,0,0,0,1}, 0, 120, 0, 0, 0, 0, 0,
      "Superior Darkvision: 120 feet|"
      "Sunlight Sensitivity: disadvantage on attacks and sight-based "
      "Perception in direct sunlight|"
      "Drow Magic: dancing lights; faerie fire at 3rd level; darkness at "
      "5th level, each once per long rest, using Charisma|"
      "Drow Weapon Training: rapier, shortsword, hand crossbow" },

    /* 5-6 halfling */
    { "Lightfoot Halfling", BOOK_PHB, {0,0,0,0,0,1}, 0, 0, 0, 0, 0, 0, 0,
      "Naturally Stealthy: hide behind a creature at least one size larger" },
    { "Stout Halfling", BOOK_PHB, {0,0,1,0,0,0}, 0, 0, 0, 0, 0, 0, 0,
      "Stout Resilience: advantage on saves against poison, and resistance "
      "to poison damage" },

    /* 7-8 human */
    { "Standard Human", BOOK_PHB, {0,0,0,0,0,0}, 0, 0, 0, 0, 0, 0, 0, "" },
    { "Variant Human", BOOK_PHB, {0,0,0,0,0,0}, 0, 0, 0, 2, 1, 1, 1,
      "Ability Score Increase: +1 to two abilities of your choice|"
      "Skills: proficiency in one skill of your choice|"
      "Feat: one feat of your choice" },

    /* 9-10 gnome */
    { "Forest Gnome", BOOK_PHB, {0,1,0,0,0,0}, 0, 0, 0, 0, 0, 0, 0,
      "Natural Illusionist: you know the minor illusion cantrip, cast with "
      "Intelligence|"
      "Speak with Small Beasts: communicate simple ideas to Small or "
      "smaller beasts" },
    { "Rock Gnome", BOOK_PHB, {0,0,1,0,0,0}, 0, 0, 0, 0, 0, 0, 0,
      "Artificer's Lore: double proficiency on History checks about magic, "
      "alchemical or technological items|"
      "Tinker: proficiency with tinker's tools; build Tiny clockwork devices" },
};
const int SUBRACE_COUNT = (int)(sizeof(SUBRACES) / sizeof(SUBRACES[0]));

const RaceData RACES[] = {
    { "Dwarf", BOOK_PHB, {0,0,2,0,0,0}, 25, SZ_MEDIUM, 60, "Common, Dwarvish", 0, 0, 0, 0, 0, 0,
      0, 2,
      "Darkvision: 60 feet|"
      "Dwarven Resilience: advantage on saves against poison, and "
      "resistance to poison damage|"
      "Dwarven Combat Training: battleaxe, handaxe, light hammer, warhammer|"
      "Tool Proficiency: smith's, brewer's or mason's tools|"
      "Stonecunning: double proficiency on History checks about stonework|"
      "Speed is not reduced by wearing heavy armor", 0 },

    { "Elf", BOOK_PHB, {0,2,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Elvish", 0, 0, 0, 0, 0, 0,
      2, 3,
      "Darkvision: 60 feet|"
      "Keen Senses: proficiency in Perception|"
      "Fey Ancestry: advantage on saves against being charmed, and magic "
      "cannot put you to sleep|"
      "Trance: 4 hours of meditation counts as 8 hours of sleep", 0 },

    { "Halfling", BOOK_PHB, {0,2,0,0,0,0}, 25, SZ_SMALL, 0, "Common, Halfling", 0, 0, 0, 0, 0, 0,
      5, 2,
      "Lucky: reroll a 1 on an attack roll, ability check or saving throw|"
      "Brave: advantage on saves against being frightened|"
      "Halfling Nimbleness: move through the space of any creature at least "
      "one size larger", 0 },

    { "Human", BOOK_PHB, {1,1,1,1,1,1}, 30, SZ_MEDIUM, 0, "Common", 1, 0, 0, 0, 0, 0,
      7, 2,
      "Ability Score Increase: +1 to every ability score|"
      "Extra Language: one language of your choice", 0 },

    { "Dragonborn", BOOK_PHB, {2,0,0,0,0,1}, 30, SZ_MEDIUM, 0, "Common, Draconic", 0, 0, 0, 0, 0, 1,
      0, 0,
      "Draconic Ancestry: choose a dragon type|"
      "Breath Weapon: exhale energy in the shape set by your ancestry. "
      "DC 8 + Constitution modifier + proficiency bonus; 2d6 damage, "
      "rising to 3d6 at 6th, 4d6 at 11th and 5d6 at 16th level; recharges "
      "on a short or long rest|"
      "Damage Resistance: to the damage type of your ancestry", 0 },

    { "Gnome", BOOK_PHB, {0,0,0,2,0,0}, 25, SZ_SMALL, 60, "Common, Gnomish", 0, 0, 0, 0, 0, 0,
      9, 2,
      "Darkvision: 60 feet|"
      "Gnome Cunning: advantage on Intelligence, Wisdom and Charisma saves "
      "against magic", 0 },

    { "Half-Elf", BOOK_PHB, {0,0,0,0,0,2}, 30, SZ_MEDIUM, 60, "Common, Elvish", 1, 2, 2, 1, 0, 0,
      0, 0,
      "Darkvision: 60 feet|"
      "Fey Ancestry: advantage on saves against being charmed, and magic "
      "cannot put you to sleep|"
      "Skill Versatility: proficiency in two skills of your choice|"
      "Ability Score Increase: +1 to two abilities of your choice", 0 },

    { "Half-Orc", BOOK_PHB, {2,0,1,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Orc", 0, 0, 0, 0, 0, 0,
      0, 0,
      "Darkvision: 60 feet|"
      "Menacing: proficiency in Intimidation|"
      "Relentless Endurance: when dropped to 0 hit points, drop to 1 "
      "instead, once per long rest|"
      "Savage Attacks: roll one extra weapon damage die on a melee "
      "critical hit", 0 },

    { "Tiefling", BOOK_PHB, {0,0,0,1,0,2}, 30, SZ_MEDIUM, 60, "Common, Infernal", 0, 0, 0, 0, 0, 0,
      0, 0,
      "Darkvision: 60 feet|"
      "Hellish Resistance: resistance to fire damage|"
      "Infernal Legacy: you know thaumaturgy; hellish rebuke at 3rd level "
      "and darkness at 5th level, each once per long rest, using Charisma", 0 },

/* --------------------- Mordenkainen Presents: Monsters of the Multiverse ---
 *
 * These races carry no fixed ability score increases: every one of them
 * chooses its own spread, which is why each sets origin_choice. Where a
 * race grants innate spells, the spell is named so it can be looked up.
 */
{ "Aarakocra", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common", 2, 0, 0, 0, 0, 0,
  0, 0,
  "Flight: a flying speed of 30 feet, but not while wearing medium or "
  "heavy armor|"
  "Talons: unarmed strikes deal 1d6 slashing damage|"
  "Wind Caller: cast gust of wind once per long rest without a slot, from "
  "5th level, using a spellcasting ability of your choice", 1 },

{ "Aasimar", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common", 2, 0, 0, 0, 0, 0,
  0, 0,
  "Celestial Resistance: resistance to necrotic and radiant damage|"
  "Darkvision: 60 feet|"
  "Healing Hands: heal a creature you touch for d4 per level, once per "
  "long rest|"
  "Light Bearer: you know the light cantrip, cast with Charisma|"
  "Celestial Revelation: from 3rd level, transform as a bonus action for 1 "
  "minute -- Heavenly Wings, Inner Radiance or Necrotic Shroud -- once per "
  "long rest", 1 },

{ "Bugbear", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common", 2, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Long-Limbed: your reach is 5 feet longer for melee attacks on your turn|"
  "Powerful Build: you count as one size larger for carrying capacity|"
  "Sneaky: proficiency in Stealth, and you can move through the space of a "
  "creature one size larger|"
  "Surprise Attack: once per turn, deal an extra 2d6 damage to a creature "
  "that has not taken a turn yet in combat", 1 },

{ "Centaur", BOOK_MPMM, {0,0,0,0,0,0}, 40, SZ_MEDIUM, 0, "Common, Sylvan", 1, 0, 0, 0, 0, 0,
  0, 0,
  "Fey: your creature type is fey|"
  "Charge: after moving 30 feet in a straight line, a melee weapon attack "
  "deals an extra 1d6 damage|"
  "Equine Build: you count as one size larger for carrying capacity, and "
  "climbing costs extra movement|"
  "Hooves: unarmed strikes deal 1d6 bludgeoning damage|"
  "Natural Affinity: proficiency in one of Animal Handling, Medicine, "
  "Nature or Survival", 1 },

{ "Changeling", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common", 2, 0, 0, 0, 0, 0,
  0, 0,
  "Fey: your creature type is fey|"
  "Shapechanger: as an action, change your appearance and voice to any "
  "Medium or Small humanoid you have seen; your statistics do not change|"
  "Changeling Instincts: proficiency in two of Deception, Insight, "
  "Intimidation, Performance or Persuasion", 1 },

{ "Deep Gnome", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_SMALL, 120, "Common, Gnomish", 1, 0, 0, 0, 0, 0,
  0, 0,
  "Superior Darkvision: 120 feet|"
  "Gnomish Magic Resistance: advantage on saving throws against spells|"
  "Gnomish Magic: cast disguise self once per long rest from 3rd level, "
  "and nondetection on yourself from 5th level, without a slot", 1 },

{ "Duergar", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 120, "Common, Dwarvish", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Superior Darkvision: 120 feet|"
  "Dwarven Resilience: advantage on saves against poison, and resistance "
  "to poison damage|"
  "Duergar Magic: cast enlarge/reduce on yourself from 3rd level and "
  "invisibility on yourself from 5th level, each once per long rest|"
  "Psionic Fortitude: advantage on saves against being charmed or "
  "stunned", 1 },

{ "Eladrin", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Elvish", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Fey: your creature type is fey|"
  "Darkvision: 60 feet|"
  "Fey Ancestry: advantage on saves to avoid or end the charmed condition|"
  "Fey Step: teleport 30 feet as a bonus action, a number of times equal "
  "to your proficiency bonus per long rest; from 3rd level the season you "
  "are in adds an effect|"
  "Keen Senses: proficiency in Perception|"
  "Trance: four hours of meditation counts as a long rest, and you may "
  "change your season", 1 },

{ "Fairy", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_SMALL, 0, "Common", 2, 0, 0, 0, 0, 0,
  0, 0,
  "Fey: your creature type is fey|"
  "Fairy Flight: a flying speed equal to your walking speed|"
  "Fairy Magic: you know druidcraft; cast faerie fire from 3rd level and "
  "enlarge/reduce from 5th level, each once per long rest", 1 },

{ "Firbolg", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common, Elvish, Giant", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Firbolg Magic: cast detect magic and disguise self, each once per short "
  "rest, using a spellcasting ability of your choice|"
  "Hidden Step: turn invisible as a bonus action until the end of your "
  "next turn, a number of times equal to your proficiency bonus per long "
  "rest|"
  "Powerful Build: you count as one size larger for carrying capacity|"
  "Speech of Beast and Leaf: beasts and plants can understand you, and you "
  "have advantage on Charisma checks made to influence them", 1 },

{ "Genasi, Air", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common, Primordial", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Unending Breath: you can hold your breath indefinitely while not "
  "incapacitated|"
  "Lightning Resistance: resistance to lightning damage|"
  "Air Genasi Magic: you know shocking grasp; cast feather fall from 3rd "
  "level and levitate from 5th level, each once per long rest", 1 },

{ "Genasi, Earth", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common, Primordial", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Earth Walk: difficult terrain made of earth or stone costs no extra "
  "movement|"
  "Merge with Stone: cast pass without trace once per long rest|"
  "Earth Genasi Magic: you know blade ward", 1 },

{ "Genasi, Fire", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Primordial", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Fire Resistance: resistance to fire damage|"
  "Reach to the Blaze: you know produce flame; cast burning hands from 3rd "
  "level once per long rest", 1 },

{ "Genasi, Water", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Primordial", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Acid Resistance: resistance to acid damage|"
  "Amphibious: you can breathe air and water|"
  "Swim: a swimming speed equal to your walking speed|"
  "Call to the Wave: you know acid splash; cast create or destroy water "
  "from 3rd level once per long rest", 1 },

{ "Githyanki", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common, Gith", 1, 0, 0, 0, 0, 0,
  0, 0,
  "Astral Knowledge: after each long rest, gain proficiency in one skill "
  "of your choice until your next long rest|"
  "Githyanki Psionics: you know mage hand, cast without components; from "
  "3rd level jump and from 5th level misty step, each once per long rest|"
  "Psychic Resilience: resistance to psychic damage", 1 },

{ "Githzerai", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common, Gith", 1, 0, 0, 0, 0, 0,
  0, 0,
  "Githzerai Psionics: you know mage hand, cast without components; from "
  "3rd level shield and from 5th level detect thoughts, each once per long "
  "rest|"
  "Mental Discipline: resistance to psychic damage|"
  "Psychic Resilience: advantage on saves against being charmed or "
  "frightened", 1 },

{ "Goblin", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_SMALL, 60, "Common, Goblin", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Fury of the Small: once per short rest, deal extra damage equal to your "
  "proficiency bonus to a creature larger than you|"
  "Fey: your creature type is fey|"
  "Nimble Escape: Disengage or Hide as a bonus action", 1 },

{ "Goliath", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common, Giant", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Little Giant: proficiency in Athletics, and you count as one size "
  "larger for carrying capacity|"
  "Mountain Born: resistance to cold damage, and you are acclimated to "
  "high altitude|"
  "Stone's Endurance: as a reaction, reduce damage by 1d12 plus your "
  "Constitution modifier, a number of times equal to your proficiency "
  "bonus per long rest", 1 },

{ "Harengon", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common", 2, 0, 0, 0, 0, 0,
  0, 0,
  "Fey: your creature type is fey|"
  "Hare-Trigger: you can add your proficiency bonus to initiative rolls|"
  "Leporine Senses: proficiency in Perception|"
  "Lucky Footwork: as a reaction to a failed Dexterity save, add 1d4 to "
  "it, unless you are prone|"
  "Rabbit Hop: as a bonus action, jump a number of feet equal to five "
  "times your proficiency bonus, without provoking", 1 },

{ "Hobgoblin", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Goblin", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Fey: your creature type is fey|"
  "Fey Gift: as a bonus action, grant a creature within 30 feet temporary "
  "hit points, or one of three other benefits, a number of times equal to "
  "your proficiency bonus per long rest|"
  "Fortune from the Many: as a reaction to a failed attack or check, add a "
  "bonus equal to the number of allies within 30 feet, up to your "
  "proficiency bonus", 1 },

{ "Kenku", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common", 2, 0, 0, 0, 0, 0,
  0, 0,
  "Expert Duplication: advantage on checks made to copy writing or "
  "craftwork you can see|"
  "Kenku Recall: proficiency in two skills of your choice, and you can "
  "roll one of those checks with advantage a number of times equal to your "
  "proficiency bonus per long rest|"
  "Mimicry: you can mimic sounds you have heard; a listener may see "
  "through it with an Insight check against your Deception", 1 },

{ "Kobold", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_SMALL, 60, "Common, Draconic", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Draconic Cry: as a bonus action, enemies within 10 feet grant advantage "
  "on attacks against them until the start of your next turn, a number of "
  "times equal to your proficiency bonus per long rest|"
  "Kobold Legacy: choose Craftiness (proficiency in one skill), Defiance "
  "(advantage on saves against being frightened) or Draconic Sorcery (one "
  "sorcerer cantrip)", 1 },

{ "Lizardfolk", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common, Draconic", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Bite: unarmed strikes deal 1d6 piercing damage|"
  "Cunning Artisan: craft a shield, club, javelin or darts from a corpse "
  "over a short rest|"
  "Hold Breath: you can hold your breath for 15 minutes|"
  "Hungry Jaws: as a bonus action, make a bite attack and gain temporary "
  "hit points, once per short rest|"
  "Natural Armor: AC 13 plus your Dexterity modifier when unarmored|"
  "Nature's Intuition: proficiency in two of Animal Handling, Nature, "
  "Perception, Stealth or Survival", 1 },

{ "Minotaur", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common, Minotaur", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Horns: unarmed strikes deal 1d6 piercing damage|"
  "Goring Rush: after taking the Dash action, make a horn attack as a "
  "bonus action|"
  "Hammering Horns: after hitting with a melee attack, shove a creature "
  "within 5 feet up to 10 feet as a bonus action|"
  "Imposing Presence: proficiency in Intimidation or Persuasion", 1 },

{ "Orc", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Orc", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Adrenaline Rush: Dash as a bonus action and gain temporary hit points "
  "equal to your proficiency bonus, a number of times equal to your "
  "proficiency bonus per short rest|"
  "Powerful Build: you count as one size larger for carrying capacity|"
  "Relentless Endurance: drop to 1 hit point instead of 0, once per long "
  "rest", 1 },

{ "Satyr", BOOK_MPMM, {0,0,0,0,0,0}, 35, SZ_MEDIUM, 0, "Common, Sylvan", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Fey: your creature type is fey|"
  "Ram: unarmed strikes deal 1d6 bludgeoning damage|"
  "Magic Resistance: advantage on saves against spells|"
  "Mirthful Leaps: add a d8 to the distance of any jump|"
  "Reveler: proficiency in Performance and Persuasion, and with one "
  "musical instrument", 1 },

{ "Sea Elf", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Elvish, Aquan", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Fey Ancestry: advantage on saves to avoid or end the charmed condition|"
  "Child of the Sea: you can breathe air and water, and have a swimming "
  "speed of 30 feet|"
  "Friend of the Sea: you can communicate simple ideas to beasts with a "
  "swimming speed|"
  "Keen Senses: proficiency in Perception|"
  "Trance: four hours of meditation counts as a long rest", 1 },

{ "Shadar-kai", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Elvish", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Fey Ancestry: advantage on saves to avoid or end the charmed condition|"
  "Necrotic Resistance: resistance to necrotic damage|"
  "Blessing of the Raven Queen: teleport 30 feet as a bonus action, a "
  "number of times equal to your proficiency bonus per long rest; from 3rd "
  "level you also gain resistance to all damage until your next turn|"
  "Keen Senses: proficiency in Perception|"
  "Trance: four hours of meditation counts as a long rest", 1 },

{ "Shifter", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Shifting: as a bonus action, gain temporary hit points equal to twice "
  "your proficiency bonus for 1 minute, a number of times equal to your "
  "proficiency bonus per long rest|"
  "Shifting Feature: choose Beasthide, Longtooth, Swiftstride or "
  "Wildhunt; each adds an effect while shifted", 1 },

{ "Tabaxi", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common", 2, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Cat's Claws: a climbing speed equal to your walking speed, and unarmed "
  "strikes deal 1d6 slashing damage|"
  "Cat's Talent: proficiency in Perception and Stealth|"
  "Feline Agility: double your speed for a turn; you cannot do so again "
  "until you move 0 feet on one of your turns", 1 },

{ "Tortle", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 0, "Common, Aquan", 1, 0, 0, 0, 0, 0,
  0, 0,
  "Claws: unarmed strikes deal 1d6 slashing damage|"
  "Hold Breath: you can hold your breath for 1 hour|"
  "Natural Armor: AC 17, unaffected by Dexterity, and you gain no benefit "
  "from wearing armor|"
  "Shell Defense: withdraw into your shell as an action for +4 AC and "
  "advantage on Strength and Constitution saves, but prone and unable to "
  "move|"
  "Survival Instinct: proficiency in Survival", 1 },

{ "Triton", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Primordial", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Amphibious: you can breathe air and water|"
  "Swim: a swimming speed of 30 feet|"
  "Control Air and Water: cast fog cloud; from 3rd level gust of wind and "
  "from 5th level wall of water, each once per long rest|"
  "Emissary of the Sea: you can communicate simple ideas to beasts and "
  "elementals that can breathe water|"
  "Guardian of the Depths: resistance to cold damage", 1 },

{ "Yuan-ti", BOOK_MPMM, {0,0,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Abyssal, Draconic", 0, 0, 0, 0, 0, 0,
  0, 0,
  "Darkvision: 60 feet|"
  "Innate Spellcasting: you know poison spray and animal friendship "
  "(snakes only, at will); cast suggestion once per long rest from 3rd "
  "level|"
  "Magic Resistance: advantage on saves against spells|"
  "Poison Resilience: immunity to poison damage and the poisoned "
  "condition", 1 },

};
const int RACE_COUNT = (int)(sizeof(RACES) / sizeof(RACES[0]));

const AncestryData ANCESTRIES[] = {
    { "Black",  "Acid",      "5 by 30 ft. line (Dex. save)" },
    { "Blue",   "Lightning", "5 by 30 ft. line (Dex. save)" },
    { "Brass",  "Fire",      "5 by 30 ft. line (Dex. save)" },
    { "Bronze", "Lightning", "5 by 30 ft. line (Dex. save)" },
    { "Copper", "Acid",      "5 by 30 ft. line (Dex. save)" },
    { "Gold",   "Fire",      "15 ft. cone (Dex. save)" },
    { "Green",  "Poison",    "15 ft. cone (Con. save)" },
    { "Red",    "Fire",      "15 ft. cone (Dex. save)" },
    { "Silver", "Cold",      "15 ft. cone (Con. save)" },
    { "White",  "Cold",      "15 ft. cone (Con. save)" },
};
const int ANCESTRY_COUNT = (int)(sizeof(ANCESTRIES) / sizeof(ANCESTRIES[0]));

const char *const LANGUAGES[] = {
    "Common", "Dwarvish", "Elvish", "Giant", "Gnomish", "Goblin",
    "Halfling", "Orc",
    "Abyssal", "Celestial", "Deep Speech", "Draconic", "Infernal",
    "Primordial", "Sylvan", "Undercommon",
};
const int LANGUAGE_COUNT = (int)(sizeof(LANGUAGES) / sizeof(LANGUAGES[0]));
