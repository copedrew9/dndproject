/* data_classoptions.c -- the lists a class picks from as it levels.
 *
 * Fighting styles, subclasses and infusions already had tables of their own.
 * These are the rest: eldritch invocations, metamagic, battle master
 * maneuvers, pact boons, arcane shots, elemental disciplines, rune knight
 * runes, and the ranger's favoured enemies and terrains.
 *
 * Each list says how many the class knows at each level, so the wizard can
 * work out how many are still owed rather than asking every time. Where an
 * option has a prerequisite it is written out, and the menu marks it
 * unavailable rather than hiding it -- a player planning ahead wants to see
 * what is coming.
 */
#include "data.h"

/* ------------------------------------------------------ eldritch invocations */

static const ClassOption INVOCATIONS[] = {
{ "Agonizing Blast", BOOK_PHB, 0, "eldritch blast cantrip",
  "Add your Charisma modifier to the damage eldritch blast deals on a hit." },
{ "Armor of Shadows", BOOK_PHB, 0, "",
  "Cast mage armor on yourself at will, without a slot or components." },
{ "Ascendant Step", BOOK_PHB, 9, "",
  "Cast levitate on yourself at will, without a slot or components." },
{ "Beast Speech", BOOK_PHB, 0, "",
  "Cast speak with animals at will, without expending a spell slot." },
{ "Beguiling Influence", BOOK_PHB, 0, "",
  "You gain proficiency in the Deception and Persuasion skills." },
{ "Bewitching Whispers", BOOK_PHB, 7, "",
  "Cast compulsion once using a warlock spell slot, then take a long rest." },
{ "Book of Ancient Secrets", BOOK_PHB, 0, "Pact of the Tome",
  "Inscribe two 1st-level ritual spells from any class list in your Book of "
  "Shadows and cast them as rituals; you can add more as you find them." },
{ "Chains of Carceri", BOOK_PHB, 15, "Pact of the Chain",
  "Cast hold monster at will on a celestial, fiend or elemental, without a "
  "slot or components; once per long rest per creature." },
{ "Devil's Sight", BOOK_PHB, 0, "",
  "You see normally in darkness, magical and nonmagical, out to 120 feet." },
{ "Dreadful Word", BOOK_PHB, 7, "",
  "Cast confusion once using a warlock spell slot, then take a long rest." },
{ "Eldritch Sight", BOOK_PHB, 0, "",
  "Cast detect magic at will, without expending a spell slot." },
{ "Eldritch Spear", BOOK_PHB, 0, "eldritch blast cantrip",
  "Eldritch blast has a range of 300 feet." },
{ "Eyes of the Rune Keeper", BOOK_PHB, 0, "",
  "You can read all writing." },
{ "Fiendish Vigor", BOOK_PHB, 0, "",
  "Cast false life on yourself at will as a 1st-level spell, without a slot "
  "or components." },
{ "Gaze of Two Minds", BOOK_PHB, 0, "",
  "Touch a willing humanoid and perceive through its senses until the end of "
  "your next turn; you can extend it each turn." },
{ "Lifedrinker", BOOK_PHB, 12, "Pact of the Blade",
  "Your pact weapon deals extra necrotic damage equal to your Charisma "
  "modifier, minimum 1." },
{ "Mask of Many Faces", BOOK_PHB, 0, "",
  "Cast disguise self at will, without expending a spell slot." },
{ "Master of Myriad Forms", BOOK_PHB, 15, "",
  "Cast alter self at will, without expending a spell slot." },
{ "Minions of Chaos", BOOK_PHB, 9, "",
  "Cast conjure elemental once using a warlock spell slot, then take a long "
  "rest." },
{ "Mire the Mind", BOOK_PHB, 5, "",
  "Cast slow once using a warlock spell slot, then take a long rest." },
{ "Misty Visions", BOOK_PHB, 0, "",
  "Cast silent image at will, without a slot or material components." },
{ "One with Shadows", BOOK_PHB, 5, "",
  "In dim light or darkness, become invisible as an action until you move or "
  "take an action or reaction." },
{ "Otherworldly Leap", BOOK_PHB, 9, "",
  "Cast jump on yourself at will, without a slot or components." },
{ "Repelling Blast", BOOK_PHB, 0, "eldritch blast cantrip",
  "When you hit a creature with eldritch blast you can push it up to 10 feet "
  "away in a straight line." },
{ "Sculptor of Flesh", BOOK_PHB, 7, "",
  "Cast polymorph once using a warlock spell slot, then take a long rest." },
{ "Sign of Ill Omen", BOOK_PHB, 5, "",
  "Cast bestow curse once using a warlock spell slot, then take a long "
  "rest." },
{ "Thief of Five Fates", BOOK_PHB, 0, "",
  "Cast bane once using a warlock spell slot, then take a long rest." },
{ "Thirsting Blade", BOOK_PHB, 5, "Pact of the Blade",
  "You can attack with your pact weapon twice, instead of once, whenever you "
  "take the Attack action." },
{ "Visions of Distant Realms", BOOK_PHB, 15, "",
  "Cast arcane eye at will, without expending a spell slot." },
{ "Voice of the Chain Master", BOOK_PHB, 0, "Pact of the Chain",
  "You can communicate telepathically with your familiar and perceive "
  "through its senses at any distance on the same plane." },
{ "Whispers of the Grave", BOOK_PHB, 9, "",
  "Cast speak with dead at will, without expending a spell slot." },
{ "Witch Sight", BOOK_PHB, 15, "",
  "See the true form of any shapechanger or creature concealed by illusion "
  "or transmutation magic within 30 feet." },
/* Xanathar's Guide to Everything */
{ "Aspect of the Moon", BOOK_XGE, 0, "Pact of the Tome",
  "You no longer need to sleep and cannot be forced to sleep by any means; "
  "you can keep watch through a long rest." },
{ "Cloak of Flies", BOOK_XGE, 5, "",
  "As a bonus action, surround yourself with a 5-foot aura of buzzing flies: "
  "advantage on Intimidation, disadvantage on other Charisma checks, and "
  "poison damage equal to your Charisma modifier to creatures that end their "
  "turn in it." },
{ "Eldritch Mind", BOOK_TCE, 0, "",
  "You have advantage on Constitution saving throws made to maintain "
  "concentration on a spell." },
{ "Eldritch Smite", BOOK_XGE, 5, "Pact of the Blade",
  "Once per turn, when you hit with your pact weapon, expend a warlock spell "
  "slot to deal an extra 1d8 force damage per slot level, plus 1d8, and "
  "knock a Huge or smaller creature prone." },
{ "Far Scribe", BOOK_TCE, 5, "Pact of the Tome",
  "A new page appears in your Book of Shadows; write a creature's name on it "
  "and you can cast sending to that creature without a slot." },
{ "Ghostly Gaze", BOOK_XGE, 7, "",
  "As an action, gain darkvision to 30 feet and see through 1 foot of stone "
  "or 3 feet of wood for 1 minute, once per short rest." },
{ "Gift of the Depths", BOOK_XGE, 5, "",
  "You can breathe underwater, gain a swimming speed equal to your walking "
  "speed, and cast water breathing once per long rest without a slot." },
{ "Gift of the Ever-Living Ones", BOOK_XGE, 0, "Pact of the Chain",
  "Whenever you regain hit points while your familiar is within 100 feet, "
  "you regain the maximum possible." },
{ "Gift of the Protectors", BOOK_TCE, 9, "Pact of the Tome",
  "A new page appears in your Book of Shadows; each creature whose name is "
  "written on it drops to 1 hit point instead of 0, once per long rest." },
{ "Grasp of Hadar", BOOK_XGE, 0, "eldritch blast cantrip",
  "Once per turn, when eldritch blast hits a creature, you can move it 10 "
  "feet closer to you in a straight line." },
{ "Improved Pact Weapon", BOOK_XGE, 0, "Pact of the Blade",
  "Your pact weapon counts as a magic weapon with +1 to attack and damage "
  "rolls, can be a shortbow or longbow, and can serve as a spellcasting "
  "focus." },
{ "Investment of the Chain Master", BOOK_TCE, 5, "Pact of the Chain",
  "Your familiar gains a flying or swimming speed of 40 feet, can attack as "
  "a bonus action, and its attacks count as magical." },
{ "Lance of Lethargy", BOOK_XGE, 0, "eldritch blast cantrip",
  "Once per turn, when eldritch blast hits, you can reduce that creature's "
  "speed by 10 feet until the end of your next turn." },
{ "Maddening Hex", BOOK_XGE, 5, "hex or a warlock curse feature",
  "As a bonus action, deal psychic damage equal to your Charisma modifier to "
  "the cursed target and every creature within 5 feet of it." },
{ "Protection of the Talisman", BOOK_TCE, 7, "Pact of the Talisman",
  "The wearer of your talisman can add 1d4 to a failed saving throw, three "
  "times per long rest." },
{ "Rebuke of the Talisman", BOOK_TCE, 0, "Pact of the Talisman",
  "When the wearer of your talisman is hit, you can use your reaction to "
  "deal psychic damage equal to your proficiency bonus and push the attacker "
  "10 feet." },
{ "Relentless Hex", BOOK_XGE, 7, "hex or a warlock curse feature",
  "As a bonus action, teleport up to 30 feet to a space within 5 feet of the "
  "creature you have cursed." },
{ "Shroud of Shadow", BOOK_XGE, 15, "",
  "Cast invisibility at will, without expending a spell slot." },
{ "Tomb of Levistus", BOOK_XGE, 5, "",
  "As a reaction when you take damage, gain 10 temporary hit points per "
  "warlock level, but your speed is 0 and you take 10 cold damage; once per "
  "short rest." },
{ "Trickster's Escape", BOOK_XGE, 7, "",
  "Cast freedom of movement on yourself once per long rest, without a slot." },
{ "Undying Servitude", BOOK_TCE, 5, "",
  "Cast animate dead once per long rest without a slot." },
};

/* A warlock knows two invocations at 2nd level, and more as they advance. */
static const unsigned char INVOCATIONS_KNOWN[MAX_LEVEL + 1] = {
    0, 0, 2, 2, 2, 3, 3, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8, 8, 8
};

/* ----------------------------------------------------------------- pact boon */

static const ClassOption PACT_BOONS[] = {
{ "Pact of the Chain", BOOK_PHB, 0, "",
  "You learn find familiar and can cast it as a ritual. Your familiar can "
  "take the form of an imp, pseudodragon, quasit or sprite, and you can "
  "forgo one of your attacks to let it attack." },
{ "Pact of the Blade", BOOK_PHB, 0, "",
  "As an action you create a pact weapon of any melee form in your hand. You "
  "are proficient with it, it counts as magical, and you can bond an "
  "existing magic weapon to summon it the same way." },
{ "Pact of the Tome", BOOK_PHB, 0, "",
  "Your patron gives you a Book of Shadows holding three cantrips from any "
  "class's spell list, which you can cast at will and which do not count "
  "against your cantrips known." },
{ "Pact of the Talisman", BOOK_TCE, 0, "",
  "Your patron gives you an amulet. When the wearer fails an ability check, "
  "they can add 1d4 to it, a number of times per long rest equal to your "
  "proficiency bonus." },
};
static const unsigned char PACT_BOON_KNOWN[MAX_LEVEL + 1] = {
    0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1
};

/* --------------------------------------------------------------- metamagic */

static const ClassOption METAMAGIC[] = {
{ "Careful Spell", BOOK_PHB, 0, "",
  "1 sorcery point: choose up to your Charisma modifier in creatures; they "
  "automatically succeed on their saving throw against the spell." },
{ "Distant Spell", BOOK_PHB, 0, "",
  "1 sorcery point: double a spell's range, or give a touch spell a range of "
  "30 feet." },
{ "Empowered Spell", BOOK_PHB, 0, "",
  "1 sorcery point: reroll up to your Charisma modifier in damage dice. This "
  "can be used alongside another metamagic option." },
{ "Extended Spell", BOOK_PHB, 0, "",
  "1 sorcery point: double a spell's duration, to a maximum of 24 hours." },
{ "Heightened Spell", BOOK_PHB, 0, "",
  "3 sorcery points: one target has disadvantage on its first saving throw "
  "against the spell." },
{ "Quickened Spell", BOOK_PHB, 0, "",
  "2 sorcery points: cast a spell with a casting time of 1 action as a bonus "
  "action instead." },
{ "Subtle Spell", BOOK_PHB, 0, "",
  "1 sorcery point: cast the spell without somatic or verbal components." },
{ "Twinned Spell", BOOK_PHB, 0, "",
  "Sorcery points equal to the spell's level, or 1 for a cantrip: target a "
  "second creature with a spell that can target only one." },
{ "Seeking Spell", BOOK_TCE, 0, "",
  "2 sorcery points: reroll a missed spell attack roll, using the new "
  "result." },
{ "Transmuted Spell", BOOK_TCE, 0, "",
  "1 sorcery point: change a spell's acid, cold, fire, lightning, poison or "
  "thunder damage to another of those types." },
};
static const unsigned char METAMAGIC_KNOWN[MAX_LEVEL + 1] = {
    0, 0, 0, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4
};

/* ----------------------------------------------- battle master maneuvers */

static const ClassOption MANEUVERS[] = {
{ "Commander's Strike", BOOK_PHB, 0, "",
  "Forgo one attack and spend a die to let an ally use its reaction to make "
  "one weapon attack, adding the die to its damage." },
{ "Disarming Attack", BOOK_PHB, 0, "",
  "Add the die to damage; the target makes a Strength save or drops one item "
  "of your choice." },
{ "Distracting Strike", BOOK_PHB, 0, "",
  "Add the die to damage; the next attack roll against the target by another "
  "creature has advantage." },
{ "Evasive Footwork", BOOK_PHB, 0, "",
  "When you move, add the die to your AC until you stop moving." },
{ "Feinting Attack", BOOK_PHB, 0, "",
  "As a bonus action, feint at a creature within 5 feet: your next attack "
  "against it this turn has advantage and adds the die to damage." },
{ "Goading Attack", BOOK_PHB, 0, "",
  "Add the die to damage; the target makes a Wisdom save or has disadvantage "
  "on attacks against anyone but you until your next turn ends." },
{ "Lunging Attack", BOOK_PHB, 0, "",
  "Increase your reach by 5 feet for one melee attack and add the die to its "
  "damage." },
{ "Maneuvering Attack", BOOK_PHB, 0, "",
  "Add the die to damage; one ally who can see or hear you can use its "
  "reaction to move half its speed without provoking from the target." },
{ "Menacing Attack", BOOK_PHB, 0, "",
  "Add the die to damage; the target makes a Wisdom save or is frightened of "
  "you until the end of your next turn." },
{ "Parry", BOOK_PHB, 0, "",
  "As a reaction when you take melee damage, reduce it by the die plus your "
  "Dexterity modifier." },
{ "Precision Attack", BOOK_PHB, 0, "",
  "Add the die to an attack roll, before or after rolling but before the "
  "outcome is known." },
{ "Pushing Attack", BOOK_PHB, 0, "",
  "Add the die to damage; a Large or smaller target makes a Strength save or "
  "is pushed 15 feet away." },
{ "Rally", BOOK_PHB, 0, "",
  "As a bonus action, an ally who can see or hear you gains temporary hit "
  "points equal to the die plus your Charisma modifier." },
{ "Riposte", BOOK_PHB, 0, "",
  "As a reaction when a creature misses you with a melee attack, make one "
  "attack against it and add the die to its damage." },
{ "Sweeping Attack", BOOK_PHB, 0, "",
  "When you hit with a melee attack, a second creature within 5 feet of the "
  "target takes damage equal to the die if your roll would have hit it." },
{ "Trip Attack", BOOK_PHB, 0, "",
  "Add the die to damage; a Large or smaller target makes a Strength save or "
  "is knocked prone." },
/* Tasha's Cauldron of Everything */
{ "Ambush", BOOK_TCE, 0, "",
  "Add the die to a Stealth check or to your initiative roll." },
{ "Bait and Switch", BOOK_TCE, 0, "",
  "Swap places with a willing ally within 5 feet; one of you adds the die to "
  "AC until the start of your next turn." },
{ "Brace", BOOK_TCE, 0, "",
  "As a reaction when a creature moves into your reach, make one attack and "
  "add the die to its damage." },
{ "Commanding Presence", BOOK_TCE, 0, "",
  "Add the die to an Intimidation, Performance or Persuasion check." },
{ "Grappling Strike", BOOK_TCE, 0, "",
  "As a bonus action after hitting, make a grapple check with the die added "
  "to it." },
{ "Quick Toss", BOOK_TCE, 0, "",
  "As a bonus action, draw and throw a thrown weapon, adding the die to its "
  "damage." },
{ "Tactical Assessment", BOOK_TCE, 0, "",
  "Add the die to a History, Insight or Investigation check." },
};
static const unsigned char MANEUVERS_KNOWN[MAX_LEVEL + 1] = {
    /* battle master levels 3, 7, 10 and 15 grant more; indexed by fighter
       level, since the archetype is taken at 3rd. */
    0, 0, 0, 3, 3, 3, 3, 5, 5, 5, 7, 7, 7, 7, 7, 9, 9, 9, 9, 9, 9
};

/* --------------------------------------------------------- arcane archer */

static const ClassOption ARCANE_SHOTS[] = {
{ "Banishing Arrow", BOOK_XGE, 0, "",
  "The target is banished to a harmless demiplane until the end of your next "
  "turn on a failed Charisma save, and its speed is 0." },
{ "Beguiling Arrow", BOOK_XGE, 0, "",
  "2d6 psychic damage; on a failed Wisdom save the target is charmed by an "
  "ally of your choice until the end of your next turn." },
{ "Bursting Arrow", BOOK_XGE, 0, "",
  "The target and every creature within 10 feet of it take 2d6 force "
  "damage." },
{ "Enfeebling Arrow", BOOK_XGE, 0, "",
  "2d6 necrotic damage; on a failed Constitution save the target's weapon "
  "damage is halved until the end of your next turn." },
{ "Grasping Arrow", BOOK_XGE, 0, "",
  "2d6 poison damage, the target's speed drops by 10 feet, and brambles deal "
  "2d6 slashing damage if it moves." },
{ "Piercing Arrow", BOOK_XGE, 0, "",
  "The arrow passes through creatures in a 30-foot line, dealing its damage "
  "plus 1d6 piercing on a failed Dexterity save." },
{ "Seeking Arrow", BOOK_XGE, 0, "",
  "The arrow curves to find a creature you name that you have seen, ignoring "
  "cover, and reveals its location." },
{ "Shadow Arrow", BOOK_XGE, 0, "",
  "2d6 psychic damage; on a failed Wisdom save the target cannot see beyond "
  "5 feet until the end of your next turn." },
};
static const unsigned char ARCANE_SHOTS_KNOWN[MAX_LEVEL + 1] = {
    0, 0, 0, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 6, 6, 6
};

/* ------------------------------------------------- way of the four elements */

static const ClassOption DISCIPLINES[] = {
{ "Elemental Attunement", BOOK_PHB, 0, "",
  "As an action, briefly control an elemental force: create a harmless "
  "sensory effect, light or snuff a small flame, chill or warm a pound of "
  "matter, or shape earth, fire, water or fog in a 1-foot cube." },
{ "Breath of Winter", BOOK_PHB, 17, "",
  "Spend 6 ki points to cast cone of cold." },
{ "Clench of the North Wind", BOOK_PHB, 6, "",
  "Spend 3 ki points to cast hold person." },
{ "Eternal Mountain Defense", BOOK_PHB, 17, "",
  "Spend 5 ki points to cast stoneskin on yourself." },
{ "Fangs of the Fire Snake", BOOK_PHB, 0, "",
  "Spend 1 ki point when you use the Attack action to reach 10 feet further "
  "and deal fire damage, and 1 more for an extra 1d10." },
{ "Fist of Four Thunders", BOOK_PHB, 0, "",
  "Spend 2 ki points to cast thunderwave." },
{ "Fist of Unbroken Air", BOOK_PHB, 0, "",
  "Spend 2 ki points to strike at 30 feet for 3d10 bludgeoning, pushing and "
  "knocking prone on a failed Strength save." },
{ "Flames of the Phoenix", BOOK_PHB, 11, "",
  "Spend 4 ki points to cast fireball." },
{ "Gong of the Summit", BOOK_PHB, 6, "",
  "Spend 3 ki points to cast shatter." },
{ "Mist Stance", BOOK_PHB, 11, "",
  "Spend 4 ki points to cast gaseous form on yourself." },
{ "Ride the Wind", BOOK_PHB, 11, "",
  "Spend 4 ki points to cast fly on yourself." },
{ "River of Hungry Flame", BOOK_PHB, 17, "",
  "Spend 5 ki points to cast wall of fire." },
{ "Rush of the Gale Spirits", BOOK_PHB, 0, "",
  "Spend 2 ki points to cast gust of wind." },
{ "Shape the Flowing River", BOOK_PHB, 0, "",
  "Spend 1 ki point to freeze, thaw or shape a 30-foot area of water or ice "
  "within 120 feet." },
{ "Sweeping Cinder Strike", BOOK_PHB, 0, "",
  "Spend 2 ki points to cast burning hands." },
{ "Water Whip", BOOK_PHB, 0, "",
  "Spend 2 ki points to lash a creature within 30 feet for 3d10 bludgeoning, "
  "pulling it or knocking it prone." },
{ "Wave of Rolling Earth", BOOK_PHB, 17, "",
  "Spend 6 ki points to cast wall of stone." },
};
static const unsigned char DISCIPLINES_KNOWN[MAX_LEVEL + 1] = {
    0, 0, 0, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 6, 7, 7, 7, 7
};

/* ------------------------------------------------------------- rune knight */

static const ClassOption RUNES[] = {
{ "Cloud Rune", BOOK_TCE, 0, "",
  "Advantage on Sleight of Hand and Deception checks. As a reaction, when a "
  "creature you can see hits another, redirect the attack to a different "
  "creature within 30 feet, once per short rest." },
{ "Fire Rune", BOOK_TCE, 0, "",
  "Double your proficiency bonus on tool checks. When you hit with a weapon "
  "attack, deal an extra 2d6 fire damage and restrain the target, once per "
  "short rest." },
{ "Frost Rune", BOOK_TCE, 0, "",
  "Advantage on Animal Handling and Intimidation checks. As a bonus action, "
  "gain +2 to Strength and Constitution checks and saves for 10 minutes, "
  "once per short rest." },
{ "Stone Rune", BOOK_TCE, 0, "",
  "Advantage on Insight checks and darkvision to 120 feet. As a reaction, "
  "charm a creature within 30 feet on a failed Wisdom save, once per short "
  "rest." },
{ "Hill Rune", BOOK_TCE, 7, "",
  "Advantage on saving throws against poison and resistance to poison "
  "damage. As a bonus action, gain resistance to bludgeoning, piercing and "
  "slashing damage for 1 minute, once per short rest." },
{ "Storm Rune", BOOK_TCE, 7, "",
  "Advantage on Arcana checks and you cannot be surprised while conscious. "
  "As a bonus action, enter a prophetic state for 1 minute, altering rolls "
  "made by others, once per short rest." },
};
static const unsigned char RUNES_KNOWN[MAX_LEVEL + 1] = {
    0, 0, 0, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5
};

/* ------------------------------------------------ ranger favoured enemies */

static const ClassOption FAVORED_ENEMIES[] = {
{ "Aberrations", BOOK_PHB, 0, "", "" },
{ "Beasts", BOOK_PHB, 0, "", "" },
{ "Celestials", BOOK_PHB, 0, "", "" },
{ "Constructs", BOOK_PHB, 0, "", "" },
{ "Dragons", BOOK_PHB, 0, "", "" },
{ "Elementals", BOOK_PHB, 0, "", "" },
{ "Fey", BOOK_PHB, 0, "", "" },
{ "Fiends", BOOK_PHB, 0, "", "" },
{ "Giants", BOOK_PHB, 0, "", "" },
{ "Monstrosities", BOOK_PHB, 0, "", "" },
{ "Oozes", BOOK_PHB, 0, "", "" },
{ "Plants", BOOK_PHB, 0, "", "" },
{ "Undead", BOOK_PHB, 0, "", "" },
{ "Two races of humanoid", BOOK_PHB, 0, "",
  "Instead of a creature type, choose two humanoid races, such as gnolls and "
  "orcs." },
};
static const unsigned char FAVORED_ENEMIES_KNOWN[MAX_LEVEL + 1] = {
    0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3
};

static const ClassOption FAVORED_TERRAINS[] = {
{ "Arctic", BOOK_PHB, 0, "", "" },
{ "Coast", BOOK_PHB, 0, "", "" },
{ "Desert", BOOK_PHB, 0, "", "" },
{ "Forest", BOOK_PHB, 0, "", "" },
{ "Grassland", BOOK_PHB, 0, "", "" },
{ "Mountain", BOOK_PHB, 0, "", "" },
{ "Swamp", BOOK_PHB, 0, "", "" },
{ "Underdark", BOOK_PHB, 0, "", "" },
};
static const unsigned char FAVORED_TERRAINS_KNOWN[MAX_LEVEL + 1] = {
    0, 1, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3
};

/* ------------------------------------------------------------- the registry */

#define LIST(cls, sub, label, plural, arr, known, rep)                        \
    { cls, sub, label, plural, arr,                                           \
      (int)(sizeof(arr) / sizeof((arr)[0])), known, rep }

const OptionList OPTION_LISTS[] = {
LIST("Warlock", "", "Eldritch Invocation", "eldritch invocations",
     INVOCATIONS, INVOCATIONS_KNOWN, 0),
LIST("Warlock", "", "Pact Boon", "pact boons",
     PACT_BOONS, PACT_BOON_KNOWN, 0),
LIST("Sorcerer", "", "Metamagic option", "metamagic options",
     METAMAGIC, METAMAGIC_KNOWN, 0),
LIST("Fighter", "Battle Master", "Maneuver", "maneuvers",
     MANEUVERS, MANEUVERS_KNOWN, 0),
LIST("Fighter", "Arcane Archer", "Arcane Shot option", "Arcane Shot options",
     ARCANE_SHOTS, ARCANE_SHOTS_KNOWN, 0),
LIST("Fighter", "Rune Knight", "Rune", "runes", RUNES, RUNES_KNOWN, 0),
LIST("Monk", "Way of the Four Elements", "Elemental Discipline",
     "elemental disciplines", DISCIPLINES, DISCIPLINES_KNOWN, 0),
LIST("Ranger", "", "Favored Enemy", "favored enemies",
     FAVORED_ENEMIES, FAVORED_ENEMIES_KNOWN, 1),
LIST("Ranger", "", "Favored Terrain", "favored terrains",
     FAVORED_TERRAINS, FAVORED_TERRAINS_KNOWN, 0),
};
const int OPTION_LIST_COUNT =
    (int)(sizeof(OPTION_LISTS) / sizeof(OPTION_LISTS[0]));
