/* data_feats.c -- PHB chapter 6 feats.
 *
 * Prerequisites are recorded both as readable text and in machine-checkable
 * form so the wizard can refuse feats the character does not qualify for.
 */
#include "data.h"

#define NOREQ ABL_COUNT
#define NOASI {0,0,0,0,0,0}

const FeatData FEATS[] = {
{ "Alert", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "+5 to initiative, you cannot be surprised while conscious, and unseen "
  "creatures gain no advantage on attacks against you." },

{ "Athlete", "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,DEX",
  "Stand from prone using only 5 feet of movement, climb at your full speed, "
  "and make a running jump after only 5 feet of running." },

{ "Actor", "", NOREQ, NOREQ, 0, "", 0, {0,0,0,0,0,1}, 0, "",
  "Advantage on Deception and Performance checks when passing as someone "
  "else, and you can mimic speech and sounds you have heard." },

{ "Charger", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "After Dashing, use a bonus action to make one melee attack or shove, "
  "with +5 damage if you moved at least 10 feet in a straight line." },

{ "Crossbow Expert", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Ignore the loading property of crossbows you are proficient with, avoid "
  "disadvantage from adjacent foes, and add a hand crossbow attack as a "
  "bonus action after a one-handed attack." },

{ "Defensive Duelist", "Dexterity 13 or higher", ABL_DEX, NOREQ, 13, "", 0,
  NOASI, 0, "",
  "When wielding a finesse weapon you are proficient with, add your "
  "proficiency bonus to AC against one melee attack as a reaction." },

{ "Dual Wielder", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "+1 AC while wielding two weapons, two-weapon fighting with non-light "
  "weapons, and you can draw or stow two weapons at once." },

{ "Dungeon Delver", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Advantage on checks and saves against traps, resistance to trap damage, "
  "and you search for traps at a normal travel pace." },

{ "Durable", "", NOREQ, NOREQ, 0, "", 0, {0,0,1,0,0,0}, 0, "",
  "When you spend a Hit Die to regain hit points, the minimum regained is "
  "twice your Constitution modifier." },

{ "Elemental Adept", "The ability to cast at least one spell",
  NOREQ, NOREQ, 0, "", 1, NOASI, 0, "",
  "Choose a damage type: your spells ignore resistance to it, and treat any "
  "1 rolled on its damage dice as a 2." },

{ "Grappler", "Strength 13 or higher", ABL_STR, NOREQ, 13, "", 0, NOASI, 0, "",
  "Advantage on attacks against creatures you grapple, and you can attempt "
  "to restrain a grappled creature." },

{ "Great Weapon Master", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "A critical hit or a kill grants a bonus-action melee attack; with a heavy "
  "weapon you may take -5 to hit for +10 damage." },

{ "Healer", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Stabilise a creature and restore 1 hit point with a healer's kit, or "
  "restore 1d6 + 4 + the creature's Hit Dice once per rest." },

{ "Heavily Armored", "Proficiency with medium armor",
  NOREQ, NOREQ, 0, "medium armor", 0, {1,0,0,0,0,0}, 0, "",
  "You gain proficiency with heavy armor." },

{ "Heavy Armor Master", "Proficiency with heavy armor",
  NOREQ, NOREQ, 0, "heavy armor", 0, {1,0,0,0,0,0}, 0, "",
  "While wearing heavy armor, reduce nonmagical bludgeoning, piercing and "
  "slashing damage by 3." },

{ "Inspiring Leader", "Charisma 13 or higher", ABL_CHA, NOREQ, 13, "", 0,
  NOASI, 0, "",
  "Spend 10 minutes to grant six friendly creatures temporary hit points "
  "equal to your level + your Charisma modifier." },

{ "Keen Mind", "", NOREQ, NOREQ, 0, "", 0, {0,0,0,1,0,0}, 0, "",
  "You always know which way is north and the hours until sunrise or sunset, "
  "and you recall anything seen or heard within the past month." },

{ "Lightly Armored", "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,DEX",
  "You gain proficiency with light armor." },

{ "Linguist", "", NOREQ, NOREQ, 0, "", 0, {0,0,0,1,0,0}, 0, "",
  "Learn three languages and create written ciphers others cannot break "
  "without magic or a hard Intelligence check." },

{ "Lucky", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Three luck points per long rest: spend one to roll an extra d20 on an "
  "attack, check or save, or on an attack made against you." },

{ "Mage Slayer", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "React to a nearby spell with a melee attack, impose disadvantage on "
  "concentration saves you cause, and gain advantage against spells cast by "
  "creatures within 5 feet." },

{ "Magic Initiate", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Choose bard, cleric, druid, sorcerer, warlock or wizard: learn two "
  "cantrips and one 1st-level spell, castable once per long rest." },

{ "Martial Adept", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Learn two Battle Master manoeuvres and gain one d6 superiority die, "
  "regained on a short or long rest." },

{ "Medium Armor Master", "Proficiency with medium armor",
  NOREQ, NOREQ, 0, "medium armor", 0, NOASI, 0, "",
  "Medium armor no longer imposes disadvantage on Stealth, and you may add "
  "up to 3 rather than 2 from your Dexterity modifier." },

{ "Mobile", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Speed increases by 10 feet, difficult terrain costs nothing when you "
  "Dash, and melee attacks provoke no opportunity attack from that target." },

{ "Moderately Armored", "Proficiency with light armor",
  NOREQ, NOREQ, 0, "light armor", 0, NOASI, 1, "STR,DEX",
  "You gain proficiency with medium armor and shields." },

{ "Mounted Combatant", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Advantage against unmounted creatures smaller than your mount, redirect "
  "attacks aimed at your mount to yourself, and your mount takes no damage "
  "on successful Dexterity saves." },

{ "Observant", "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "INT,WIS",
  "Read lips, and gain +5 to passive Perception and passive Investigation." },

{ "Polearm Master", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Bonus-action butt-end attack with a glaive, halberd or quarterstaff, and "
  "creatures entering your reach provoke opportunity attacks." },

{ "Resilient", "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "",
  "Increase one ability score by 1 and gain proficiency in its saving throws." },

{ "Ritual Caster", "Intelligence or Wisdom 13 or higher",
  ABL_INT, ABL_WIS, 13, "", 0, NOASI, 0, "",
  "Choose a class and gain a ritual book with two 1st-level ritual spells "
  "you can cast as rituals." },

{ "Savage Attacker", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Once per turn, reroll a melee weapon's damage dice and use either total." },

{ "Sentinel", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Opportunity attacks reduce a creature's speed to 0, Disengage does not "
  "help against you, and you may react when a nearby creature attacks "
  "someone else." },

{ "Sharpshooter", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "No disadvantage at long range, ignore half and three-quarters cover, and "
  "you may take -5 to hit for +10 damage with ranged weapons." },

{ "Shield Master", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Bonus-action shove with a shield, add your shield's AC bonus to Dexterity "
  "saves against effects targeting you, and take no damage on a success." },

{ "Skilled", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Gain proficiency in any combination of three skills or tools." },

{ "Skulker", "Dexterity 13 or higher", ABL_DEX, NOREQ, 13, "", 0, NOASI, 0, "",
  "Hide when lightly obscured, missing a ranged attack does not reveal you, "
  "and dim light imposes no disadvantage on Perception." },

{ "Spell Sniper", "The ability to cast at least one spell",
  NOREQ, NOREQ, 0, "", 1, NOASI, 0, "",
  "Double the range of attack-roll spells, ignore half and three-quarters "
  "cover, and learn one attack cantrip." },

{ "Tavern Brawler", "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,CON",
  "Proficiency with improvised weapons, unarmed strikes deal 1d4, and a hit "
  "lets you grapple as a bonus action." },

{ "Tough", "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Your hit point maximum increases by twice your level, and by 2 each time "
  "you gain a level." },

{ "War Caster", "The ability to cast at least one spell",
  NOREQ, NOREQ, 0, "", 1, NOASI, 0, "",
  "Advantage on concentration saves, cast with hands full, and cast a "
  "one-action spell as an opportunity attack." },

{ "Weapon Master", "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,DEX",
  "Gain proficiency with four weapons of your choice." },
};
const int FEAT_COUNT = (int)(sizeof(FEATS) / sizeof(FEATS[0]));
