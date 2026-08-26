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
      "Speed is not reduced by wearing heavy armor" },

    { "Elf", BOOK_PHB, {0,2,0,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Elvish", 0, 0, 0, 0, 0, 0,
      2, 3,
      "Darkvision: 60 feet|"
      "Keen Senses: proficiency in Perception|"
      "Fey Ancestry: advantage on saves against being charmed, and magic "
      "cannot put you to sleep|"
      "Trance: 4 hours of meditation counts as 8 hours of sleep" },

    { "Halfling", BOOK_PHB, {0,2,0,0,0,0}, 25, SZ_SMALL, 0, "Common, Halfling", 0, 0, 0, 0, 0, 0,
      5, 2,
      "Lucky: reroll a 1 on an attack roll, ability check or saving throw|"
      "Brave: advantage on saves against being frightened|"
      "Halfling Nimbleness: move through the space of any creature at least "
      "one size larger" },

    { "Human", BOOK_PHB, {1,1,1,1,1,1}, 30, SZ_MEDIUM, 0, "Common", 1, 0, 0, 0, 0, 0,
      7, 2,
      "Ability Score Increase: +1 to every ability score|"
      "Extra Language: one language of your choice" },

    { "Dragonborn", BOOK_PHB, {2,0,0,0,0,1}, 30, SZ_MEDIUM, 0, "Common, Draconic", 0, 0, 0, 0, 0, 1,
      0, 0,
      "Draconic Ancestry: choose a dragon type|"
      "Breath Weapon: exhale energy in the shape set by your ancestry. "
      "DC 8 + Constitution modifier + proficiency bonus; 2d6 damage, "
      "rising to 3d6 at 6th, 4d6 at 11th and 5d6 at 16th level; recharges "
      "on a short or long rest|"
      "Damage Resistance: to the damage type of your ancestry" },

    { "Gnome", BOOK_PHB, {0,0,0,2,0,0}, 25, SZ_SMALL, 60, "Common, Gnomish", 0, 0, 0, 0, 0, 0,
      9, 2,
      "Darkvision: 60 feet|"
      "Gnome Cunning: advantage on Intelligence, Wisdom and Charisma saves "
      "against magic" },

    { "Half-Elf", BOOK_PHB, {0,0,0,0,0,2}, 30, SZ_MEDIUM, 60, "Common, Elvish", 1, 2, 2, 1, 0, 0,
      0, 0,
      "Darkvision: 60 feet|"
      "Fey Ancestry: advantage on saves against being charmed, and magic "
      "cannot put you to sleep|"
      "Skill Versatility: proficiency in two skills of your choice|"
      "Ability Score Increase: +1 to two abilities of your choice" },

    { "Half-Orc", BOOK_PHB, {2,0,1,0,0,0}, 30, SZ_MEDIUM, 60, "Common, Orc", 0, 0, 0, 0, 0, 0,
      0, 0,
      "Darkvision: 60 feet|"
      "Menacing: proficiency in Intimidation|"
      "Relentless Endurance: when dropped to 0 hit points, drop to 1 "
      "instead, once per long rest|"
      "Savage Attacks: roll one extra weapon damage die on a melee "
      "critical hit" },

    { "Tiefling", BOOK_PHB, {0,0,0,1,0,2}, 30, SZ_MEDIUM, 60, "Common, Infernal", 0, 0, 0, 0, 0, 0,
      0, 0,
      "Darkvision: 60 feet|"
      "Hellish Resistance: resistance to fire damage|"
      "Infernal Legacy: you know thaumaturgy; hellish rebuke at 3rd level "
      "and darkness at 5th level, each once per long rest, using Charisma" },
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
