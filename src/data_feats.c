/* data_feats.c -- PHB chapter 6 feats.
 *
 * Prerequisites are recorded both as readable text and in machine-checkable
 * form so the wizard can refuse feats the character does not qualify for.
 */
#include "data.h"

#define NOREQ ABL_COUNT
#define NOASI {0,0,0,0,0,0}

const FeatData FEATS[] = {
{ "Alert", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "+5 to initiative, you cannot be surprised while conscious, and unseen "
  "creatures gain no advantage on attacks against you.", "" },

{ "Athlete", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,DEX",
  "Stand from prone using only 5 feet of movement, climb at your full speed, "
  "and make a running jump after only 5 feet of running.", "" },

{ "Actor", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, {0,0,0,0,0,1}, 0, "",
  "Advantage on Deception and Performance checks when passing as someone "
  "else, and you can mimic speech and sounds you have heard.", "" },

{ "Charger", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "After Dashing, use a bonus action to make one melee attack or shove, "
  "with +5 damage if you moved at least 10 feet in a straight line.", "" },

{ "Crossbow Expert", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Ignore the loading property of crossbows you are proficient with, avoid "
  "disadvantage from adjacent foes, and add a hand crossbow attack as a "
  "bonus action after a one-handed attack.", "" },

{ "Defensive Duelist", BOOK_PHB, "Dexterity 13 or higher", ABL_DEX, NOREQ, 13, "", 0,
  NOASI, 0, "",
  "When wielding a finesse weapon you are proficient with, add your "
  "proficiency bonus to AC against one melee attack as a reaction.", "" },

{ "Dual Wielder", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "+1 AC while wielding two weapons, two-weapon fighting with non-light "
  "weapons, and you can draw or stow two weapons at once.", "" },

{ "Dungeon Delver", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Advantage on checks and saves against traps, resistance to trap damage, "
  "and you search for traps at a normal travel pace.", "" },

{ "Durable", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, {0,0,1,0,0,0}, 0, "",
  "When you spend a Hit Die to regain hit points, the minimum regained is "
  "twice your Constitution modifier.", "" },

{ "Elemental Adept", BOOK_PHB, "The ability to cast at least one spell",
  NOREQ, NOREQ, 0, "", 1, NOASI, 0, "",
  "Choose a damage type: your spells ignore resistance to it, and treat any "
  "1 rolled on its damage dice as a 2.", "" },

{ "Grappler", BOOK_PHB, "Strength 13 or higher", ABL_STR, NOREQ, 13, "", 0, NOASI, 0, "",
  "Advantage on attacks against creatures you grapple, and you can attempt "
  "to restrain a grappled creature.", "" },

{ "Great Weapon Master", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "A critical hit or a kill grants a bonus-action melee attack; with a heavy "
  "weapon you may take -5 to hit for +10 damage.", "" },

{ "Healer", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Stabilise a creature and restore 1 hit point with a healer's kit, or "
  "restore 1d6 + 4 + the creature's Hit Dice once per rest.", "" },

{ "Heavily Armored", BOOK_PHB, "Proficiency with medium armor",
  NOREQ, NOREQ, 0, "medium armor", 0, {1,0,0,0,0,0}, 0, "",
  "You gain proficiency with heavy armor.", "" },

{ "Heavy Armor Master", BOOK_PHB, "Proficiency with heavy armor",
  NOREQ, NOREQ, 0, "heavy armor", 0, {1,0,0,0,0,0}, 0, "",
  "While wearing heavy armor, reduce nonmagical bludgeoning, piercing and "
  "slashing damage by 3.", "" },

{ "Inspiring Leader", BOOK_PHB, "Charisma 13 or higher", ABL_CHA, NOREQ, 13, "", 0,
  NOASI, 0, "",
  "Spend 10 minutes to grant six friendly creatures temporary hit points "
  "equal to your level + your Charisma modifier.", "" },

{ "Keen Mind", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, {0,0,0,1,0,0}, 0, "",
  "You always know which way is north and the hours until sunrise or sunset, "
  "and you recall anything seen or heard within the past month.", "" },

{ "Lightly Armored", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,DEX",
  "You gain proficiency with light armor.", "" },

{ "Linguist", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, {0,0,0,1,0,0}, 0, "",
  "Learn three languages and create written ciphers others cannot break "
  "without magic or a hard Intelligence check.", "" },

{ "Lucky", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Three luck points per long rest: spend one to roll an extra d20 on an "
  "attack, check or save, or on an attack made against you.", "" },

{ "Mage Slayer", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "React to a nearby spell with a melee attack, impose disadvantage on "
  "concentration saves you cause, and gain advantage against spells cast by "
  "creatures within 5 feet.", "" },

{ "Magic Initiate", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Choose bard, cleric, druid, sorcerer, warlock or wizard: learn two "
  "cantrips and one 1st-level spell, castable once per long rest.", "" },

{ "Martial Adept", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Learn two Battle Master manoeuvres and gain one d6 superiority die, "
  "regained on a short or long rest.", "" },

{ "Medium Armor Master", BOOK_PHB, "Proficiency with medium armor",
  NOREQ, NOREQ, 0, "medium armor", 0, NOASI, 0, "",
  "Medium armor no longer imposes disadvantage on Stealth, and you may add "
  "up to 3 rather than 2 from your Dexterity modifier.", "" },

{ "Mobile", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Speed increases by 10 feet, difficult terrain costs nothing when you "
  "Dash, and melee attacks provoke no opportunity attack from that target.", "" },

{ "Moderately Armored", BOOK_PHB, "Proficiency with light armor",
  NOREQ, NOREQ, 0, "light armor", 0, NOASI, 1, "STR,DEX",
  "You gain proficiency with medium armor and shields.", "" },

{ "Mounted Combatant", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Advantage against unmounted creatures smaller than your mount, redirect "
  "attacks aimed at your mount to yourself, and your mount takes no damage "
  "on successful Dexterity saves.", "" },

{ "Observant", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "INT,WIS",
  "Read lips, and gain +5 to passive Perception and passive Investigation.", "" },

{ "Polearm Master", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Bonus-action butt-end attack with a glaive, halberd or quarterstaff, and "
  "creatures entering your reach provoke opportunity attacks.", "" },

{ "Resilient", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "",
  "Increase one ability score by 1 and gain proficiency in its saving throws.", "" },

{ "Ritual Caster", BOOK_PHB, "Intelligence or Wisdom 13 or higher",
  ABL_INT, ABL_WIS, 13, "", 0, NOASI, 0, "",
  "Choose a class and gain a ritual book with two 1st-level ritual spells "
  "you can cast as rituals.", "" },

{ "Savage Attacker", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Once per turn, reroll a melee weapon's damage dice and use either total.", "" },

{ "Sentinel", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Opportunity attacks reduce a creature's speed to 0, Disengage does not "
  "help against you, and you may react when a nearby creature attacks "
  "someone else.", "" },

{ "Sharpshooter", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "No disadvantage at long range, ignore half and three-quarters cover, and "
  "you may take -5 to hit for +10 damage with ranged weapons.", "" },

{ "Shield Master", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Bonus-action shove with a shield, add your shield's AC bonus to Dexterity "
  "saves against effects targeting you, and take no damage on a success.", "" },

{ "Skilled", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Gain proficiency in any combination of three skills or tools.", "" },

{ "Skulker", BOOK_PHB, "Dexterity 13 or higher", ABL_DEX, NOREQ, 13, "", 0, NOASI, 0, "",
  "Hide when lightly obscured, missing a ranged attack does not reveal you, "
  "and dim light imposes no disadvantage on Perception.", "" },

{ "Spell Sniper", BOOK_PHB, "The ability to cast at least one spell",
  NOREQ, NOREQ, 0, "", 1, NOASI, 0, "",
  "Double the range of attack-roll spells, ignore half and three-quarters "
  "cover, and learn one attack cantrip.", "" },

{ "Tavern Brawler", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,CON",
  "Proficiency with improvised weapons, unarmed strikes deal 1d4, and a hit "
  "lets you grapple as a bonus action.", "" },

{ "Tough", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Your hit point maximum increases by twice your level, and by 2 each time "
  "you gain a level.", "" },

{ "War Caster", BOOK_PHB, "The ability to cast at least one spell",
  NOREQ, NOREQ, 0, "", 1, NOASI, 0, "",
  "Advantage on concentration saves, cast with hands full, and cast a "
  "one-action spell as an opportunity attack.", "" },

{ "Weapon Master", BOOK_PHB, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,DEX",
  "Gain proficiency with four weapons of your choice.", "" },

/* --------------------------------- Tasha's Cauldron of Everything feats --- */

{ "Artificer Initiate", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Learn one artificer cantrip and one 1st-level artificer spell, castable "
  "once per long rest without a slot, using Intelligence. You also gain "
  "proficiency with one kind of artisan's tools and can use them as a "
  "spellcasting focus for those spells.", "" },

{ "Chef", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "CON,WIS",
  "Proficiency with cook's utensils. Over an hour's short rest you can cook "
  "food for up to four plus your proficiency bonus creatures, each regaining "
  "an extra 1d8 hit points from Hit Dice; over a long rest you can bake "
  "treats that give 1d8 temporary hit points.", "" },

{ "Crusher", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,CON",
  "Once per turn, when you deal bludgeoning damage you can move the target "
  "5 feet. When you score a critical hit with bludgeoning damage, attacks "
  "against that creature have advantage until the start of your next turn.", "" },

{ "Eldritch Adept", BOOK_TCE, "Spellcasting or Pact Magic",
  NOREQ, NOREQ, 0, "", 1, NOASI, 0, "",
  "Learn one eldritch invocation of your choice that has no Pact Boon "
  "prerequisite. You can replace it whenever you gain a level.", "" },

{ "Fey Touched", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1,
  "INT,WIS,CHA",
  "Learn misty step and one 1st-level divination or enchantment spell. You "
  "can cast each once per long rest without a slot, and otherwise with "
  "slots as normal, using the ability this feat raised.", "" },

{ "Fighting Initiate", BOOK_TCE, "Proficiency with a martial weapon",
  NOREQ, NOREQ, 0, "Martial weapons", 0, NOASI, 0, "",
  "Learn one Fighting Style option of your choice. You can replace it "
  "whenever you gain a level.", "" },

{ "Gunner", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, {0,1,0,0,0,0}, 0, "",
  "Proficiency with firearms, you ignore the loading property of firearms, "
  "and being within 5 feet of a hostile creature does not impose "
  "disadvantage on your ranged attacks.", "" },

{ "Metamagic Adept", BOOK_TCE, "Spellcasting or Pact Magic",
  NOREQ, NOREQ, 0, "", 1, NOASI, 0, "",
  "Learn two Metamagic options from the sorcerer class and gain 2 sorcery "
  "points to spend on them, regained on a long rest. You can replace one "
  "option whenever you gain a level.", "" },

{ "Piercer", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,DEX",
  "Once per turn, when you hit with an attack dealing piercing damage you "
  "can reroll one damage die. A critical hit with piercing damage adds one "
  "extra damage die to the critical's total.", "" },

{ "Poisoner", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "Your poison damage ignores resistance. You gain proficiency with the "
  "poisoner's kit and can apply a potent poison to a weapon as a bonus "
  "action: DC 14, 2d8 poison damage and poisoned for 1 minute. You can "
  "craft 50 gp of that poison over a long rest for 50 gp of materials.", "" },

{ "Shadow Touched", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1,
  "INT,WIS,CHA",
  "Learn invisibility and one 1st-level illusion or necromancy spell. You "
  "can cast each once per long rest without a slot, and otherwise with "
  "slots as normal, using the ability this feat raised.", "" },

{ "Skill Expert", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "",
  "Gain proficiency in one skill of your choice, and choose one skill or "
  "tool you are proficient with to gain expertise in, doubling your "
  "proficiency bonus for it.", "" },

{ "Slasher", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1, "STR,DEX",
  "Once per turn, when you deal slashing damage you can reduce the target's "
  "speed by 10 feet until the start of your next turn. A critical hit with "
  "slashing damage gives the target disadvantage on attack rolls until the "
  "start of your next turn.", "" },

{ "Telekinetic", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1,
  "INT,WIS,CHA",
  "Learn mage hand, or gain a 30-foot increase to its range if you already "
  "know it, and you can cast it without components. As a bonus action you "
  "can shove a creature within 30 feet 5 feet toward or away from you, on a "
  "failed Strength save.", "" },

{ "Telepathic", BOOK_TCE, "", NOREQ, NOREQ, 0, "", 0, NOASI, 1,
  "INT,WIS,CHA",
  "You can speak telepathically to any creature within 60 feet that "
  "understands a language you know, and you can cast detect thoughts once "
  "per long rest without a slot, using the ability this feat raised.", "" },


/* ------------------------------------ Xanathar's racial feats -------------
 *
 * Each is limited to a race, and three of them to a particular subrace, so
 * req_race is matched against both the race and subrace names. Where a feat
 * is open to more than one, they are separated by '|'.
 */

{ "Bountiful Luck", BOOK_XGE, "Halfling", NOREQ, NOREQ, 0, "", 0, NOASI, 0, "",
  "When an ally within 30 feet rolls a 1 on a d20, you can use your reaction "
  "to make them reroll it. You cannot use your Lucky trait until the end of "
  "your next turn.", "Halfling" },

{ "Dragon Fear", BOOK_XGE, "Dragonborn", NOREQ, NOREQ, 0, "", 0, NOASI, 1,
  "STR,CON,CHA",
  "Instead of your breath weapon, you can roar: each creature of your choice "
  "within 30 feet makes a Wisdom save (DC 8 + proficiency + Charisma "
  "modifier) or is frightened of you for 1 minute.", "Dragonborn" },

{ "Dragon Hide", BOOK_XGE, "Dragonborn", NOREQ, NOREQ, 0, "", 0, NOASI, 1,
  "STR,CON,CHA",
  "Your scales harden: with no armor your AC is 13 + Dexterity modifier, and "
  "a shield still helps. Retractable claws let your unarmed strikes deal 1d4 "
  "slashing plus your Strength modifier.", "Dragonborn" },

{ "Drow High Magic", BOOK_XGE, "Elf (drow)", NOREQ, NOREQ, 0, "", 0, NOASI,
  0, "",
  "You learn detect magic and can cast it at will. You also learn levitate "
  "and dispel magic, each castable once per long rest without a slot. "
  "Charisma is your spellcasting ability for all three.",
  "Dark Elf (Drow)" },

{ "Dwarven Fortitude", BOOK_XGE, "Dwarf", NOREQ, NOREQ, 0, "",
  0, {0,0,1,0,0,0}, 0, "",
  "Whenever you take the Dodge action you can spend one Hit Die to heal "
  "yourself, rolling the die and adding your Constitution modifier.",
  "Dwarf" },

{ "Elven Accuracy", BOOK_XGE, "Elf or half-elf", NOREQ, NOREQ, 0, "", 0,
  NOASI, 1, "DEX,INT,WIS,CHA",
  "Whenever you have advantage on an attack roll using Dexterity, "
  "Intelligence, Wisdom or Charisma, you can reroll one of the dice once.",
  "Elf|Half-Elf" },

{ "Fade Away", BOOK_XGE, "Gnome", NOREQ, NOREQ, 0, "", 0, NOASI, 1,
  "DEX,INT",
  "Immediately after taking damage, you can use a reaction to become "
  "invisible until the end of your next turn or until you attack, deal "
  "damage or cast a spell. Once per short or long rest.", "Gnome" },

{ "Fey Teleportation", BOOK_XGE, "Elf (high)", NOREQ, NOREQ, 0, "", 0, NOASI,
  1, "INT,CHA",
  "You learn Sylvan, and you learn misty step, castable once per short or "
  "long rest without a slot as well as with any slot you have. Intelligence "
  "is your spellcasting ability for it.", "High Elf" },

{ "Flames of Phlegethos", BOOK_XGE, "Tiefling", NOREQ, NOREQ, 0, "", 0,
  NOASI, 1, "INT,CHA",
  "When you roll fire damage for a spell, you can reroll any 1s, but must "
  "use the new roll. Casting a spell that deals fire damage wreathes you in "
  "flames until the end of your next turn, shedding light and burning any "
  "creature that hits you in melee for 1d4 fire damage.", "Tiefling" },

{ "Infernal Constitution", BOOK_XGE, "Tiefling", NOREQ, NOREQ, 0, "",
  0, {0,0,1,0,0,0}, 0, "",
  "You have resistance to cold and poison damage, and advantage on saving "
  "throws against being poisoned.", "Tiefling" },

{ "Orcish Fury", BOOK_XGE, "Half-orc", NOREQ, NOREQ, 0, "", 0, NOASI, 1,
  "STR,CON",
  "When you hit with a simple or martial weapon you are proficient with, you "
  "can add one extra damage die, once per short or long rest. When you use "
  "Relentless Endurance you can also make one weapon attack as a reaction.",
  "Half-Orc" },

{ "Prodigy", BOOK_XGE, "Half-elf, half-orc or human", NOREQ, NOREQ, 0, "", 0,
  NOASI, 0, "",
  "You gain one skill proficiency, one tool proficiency and one language, "
  "and you gain expertise in one skill you are proficient with.",
  "Half-Elf|Half-Orc|Human" },

{ "Second Chance", BOOK_XGE, "Halfling", NOREQ, NOREQ, 0, "", 0, NOASI, 1,
  "DEX,CON,CHA",
  "When a creature you can see hits you with an attack, you can use your "
  "reaction to force it to reroll. Once per short or long rest, or after you "
  "roll initiative.", "Halfling" },

{ "Squat Nimbleness", BOOK_XGE, "Dwarf or a Small race", NOREQ, NOREQ, 0, "",
  0, NOASI, 1, "STR,DEX",
  "Your walking speed increases by 5 feet, you gain proficiency in Acrobatics "
  "or Athletics, and you have advantage on checks made to escape a grapple.",
  "Dwarf|Gnome|Halfling" },

{ "Wood Elf Magic", BOOK_XGE, "Elf (wood)", NOREQ, NOREQ, 0, "", 0, NOASI, 0,
  "",
  "You learn one druid cantrip, plus longstrider and pass without trace, each "
  "castable once per long rest without a slot. Wisdom is your spellcasting "
  "ability for them.", "Wood Elf" },

};
const int FEAT_COUNT = (int)(sizeof(FEATS) / sizeof(FEATS[0]));
