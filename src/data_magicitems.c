/* data_magicitems.c -- the Dungeon Master's Guide magic item catalogue.
 *
 * Each entry carries what a character sheet needs to state: the item's
 * category and rarity, whether it must be attuned, and what it actually
 * does. Descriptions are written to be usable at the table rather than
 * quoted at length; where an item's effect is a spell, the spell is named
 * so it can be looked up in the spell tables.
 *
 * Attunement is a separate field rather than being buried in the rarity
 * line, because a character may attune to only three items at once and the
 * sheet has to be able to count them.
 */
#include "data.h"

#include <string.h>

const MagicItem MAGIC_ITEMS[] = {

{ "Adamantine Armor", BOOK_DMG, "Armor (medium or heavy, not hide)",
  "uncommon", NULL,
  "Reinforced with adamantine. While you wear it, any critical hit against "
  "you becomes a normal hit." },

{ "Alchemy Jug", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "A ceramic jug weighing 12 lb. As an action, name a liquid and it fills "
  "with that liquid, then pours up to 2 gallons a minute: acid (8 oz.), "
  "basic poison (1/2 oz.), beer (4 gal.), honey (1 gal.), mayonnaise "
  "(2 gal.), oil (1 qt.), vinegar (2 gal.), fresh water (8 gal.), salt "
  "water (12 gal.) or wine (1 gal.). Once it starts, it makes no other "
  "liquid until the next dawn." },

{ "Ammunition, +1, +2, or +3", BOOK_DMG, "Weapon (any ammunition)",
  "uncommon (+1), rare (+2), very rare (+3)", NULL,
  "A bonus to attack and damage rolls made with this piece of ammunition, "
  "set by its rarity. Once it hits a target it is no longer magical." },

{ "Amulet of Health", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "Your Constitution score is 19 while you wear this amulet. It has no "
  "effect if your Constitution is already 19 or higher." },

{ "Amulet of Proof against Detection and Location", BOOK_DMG,
  "Wondrous item", "uncommon", "requires attunement",
  "While wearing it you are hidden from divination magic: you cannot be "
  "targeted by it, or perceived through magical scrying sensors." },

{ "Amulet of the Planes", BOOK_DMG, "Wondrous item", "very rare",
  "requires attunement",
  "As an action, name a plane of existence you are familiar with and make "
  "a DC 15 Intelligence check. On a success you cast plane shift; on a "
  "failure you and each creature travelling with you take 4d6 psychic "
  "damage and the amulet sends you to a random plane." },

{ "Animated Shield", BOOK_DMG, "Armor (shield)", "very rare",
  "requires attunement",
  "As a bonus action, speak the command word and the shield leaps into the "
  "air to hover and protect you for 1 minute, as though you were wielding "
  "it, leaving both your hands free. It returns after 1 minute, when you "
  "use a bonus action to end the effect, or when you are incapacitated or "
  "die." },

{ "Apparatus of Kwalish", BOOK_DMG, "Wondrous item", "legendary", NULL,
  "A sealed iron barrel, 500 lb., that opens into a crab-like vehicle "
  "seating two. It is AC 20, 200 hit points, immune to poison and psychic "
  "damage, has a walking speed of 30 ft. and a swimming speed of 30 ft., "
  "and can breathe underwater indefinitely for its occupants. Ten levers "
  "inside control the legs, claws, windows, lamp and propulsion." },

{ "Armor, +1, +2, or +3", BOOK_DMG, "Armor (light, medium or heavy)",
  "rare (+1), very rare (+2), legendary (+3)", NULL,
  "You have a bonus to AC while wearing this armor, set by its rarity." },

{ "Armor of Invulnerability", BOOK_DMG, "Armor (plate)", "legendary",
  "requires attunement",
  "You have resistance to nonmagical damage while you wear it. As an "
  "action you can become immune to nonmagical damage for 10 minutes or "
  "until you take it off; once used, this cannot be used again until the "
  "next dawn." },

{ "Armor of Resistance", BOOK_DMG, "Armor (light, medium or heavy)",
  "rare", "requires attunement",
  "You have resistance to one type of damage while you wear this armor. "
  "The type is chosen when the armor is created, from acid, cold, fire, "
  "force, lightning, necrotic, poison, psychic, radiant or thunder." },

{ "Armor of Vulnerability", BOOK_DMG, "Armor (plate)", "rare",
  "requires attunement",
  "You have resistance to one of bludgeoning, piercing or slashing damage "
  "while you wear it. Cursed: once attuned, you have vulnerability to the "
  "other two damage types, and cannot remove the armor until you are "
  "targeted by remove curse or similar magic. The curse is not apparent "
  "until it first takes effect." },

{ "Arrow-Catching Shield", BOOK_DMG, "Armor (shield)", "rare",
  "requires attunement",
  "You gain +2 AC against ranged attacks while wielding it, on top of the "
  "shield's normal bonus. Whenever an attacker makes a ranged attack "
  "against a target within 5 feet of you, you can use your reaction to "
  "become the target instead." },

{ "Arrow of Slaying", BOOK_DMG, "Weapon (arrow)", "very rare", NULL,
  "Made to kill one kind of creature. If that kind takes damage from the "
  "arrow, it makes a DC 17 Constitution save, taking an extra 6d10 "
  "piercing damage on a failure or half as much on a success. Once it "
  "deals its extra damage, it becomes a nonmagical arrow." },

{ "Bag of Beans", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A heavy cloth bag containing 3d4 dry beans. Emptying the bag out at "
  "once makes them explode: each creature within 10 feet makes a DC 15 "
  "Dexterity save, taking 5d4 fire damage on a failure or half on a "
  "success. Planting a bean and watering it produces a random effect one "
  "minute later, from a treant or a geyser to a gem-laden shrub." },

{ "Bag of Devouring", BOOK_DMG, "Wondrous item", "very rare", NULL,
  "Looks like a bag of holding but is the gullet of an extradimensional "
  "creature. Anything put wholly inside is swallowed; a creature inside "
  "must make a DC 15 Strength check to escape as an action, and there is "
  "a cumulative 50 percent chance each turn that it is eaten and lost. "
  "The bag weighs 15 lb." },

{ "Bag of Holding", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "The interior holds up to 500 lb., not exceeding 64 cubic feet. The bag "
  "always weighs 15 lb. Retrieving an item is an action. Overloading, "
  "piercing or tearing it destroys it and scatters the contents in the "
  "Astral Plane. Placing it inside an extradimensional space destroys "
  "both and opens a gate to the Astral Plane." },

{ "Bag of Tricks", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "Reaching in draws out a fuzzy ball; throwing it up to 20 feet turns it "
  "into a creature which acts on your turn as you command. It vanishes at "
  "0 hit points or after 1 hour. You can use the bag three times, and it "
  "recharges at dawn. Grey, rust and tan bags each produce a different "
  "list of beasts, from a weasel up to a lion or tiger." },

{ "Bead of Force", BOOK_DMG, "Wondrous item", "rare", NULL,
  "As an action, throw it up to 60 feet. It detonates on impact and is "
  "destroyed: each creature within 10 feet makes a DC 15 Dexterity save, "
  "taking 5d4 force damage on a failure, and a failed save also traps the "
  "creature in a sphere of force for 1 minute." },

{ "Belt of Dwarvenkind", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "Your Constitution increases by 2, to a maximum of 20. You have "
  "advantage on Charisma (Persuasion) checks with dwarves, darkvision to "
  "60 feet, and can speak, read and write Dwarvish. There is a 50 percent "
  "chance each day that you gain advantage on saves against poison and "
  "resistance to poison damage that day." },

{ "Belt of Giant Strength", BOOK_DMG, "Wondrous item",
  "rare to legendary", "requires attunement",
  "Your Strength score becomes the belt's score, if it is not already "
  "higher: hill giant 21 (rare), stone or frost giant 23 (very rare), "
  "fire giant 25 (very rare), cloud giant 27 (legendary), storm giant 29 "
  "(legendary)." },

{ "Berserker Axe", BOOK_DMG, "Weapon (any axe)", "rare",
  "requires attunement",
  "+1 to attack and damage rolls, and your hit point maximum increases by "
  "1 for each level you have. Cursed: while attuned, you cannot part with "
  "it, and whenever you take damage in combat you must make a DC 15 "
  "Wisdom save or go berserk, attacking the nearest creature each turn "
  "until there is nothing left to attack." },

{ "Boots of Elvenkind", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "Your steps make no sound, whatever the surface. You have advantage on "
  "Dexterity (Stealth) checks that rely on moving silently." },

{ "Boots of Levitation", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "You can cast levitate on yourself at will." },

{ "Boots of Speed", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "As a bonus action you can click the heels together to double your "
  "walking speed, and opportunity attacks against you are at "
  "disadvantage. Clicking again ends the effect. It lasts up to 10 "
  "minutes of walking, and recovers 2 hours of use for every 6 hours the "
  "boots are not used." },

{ "Boots of Striding and Springing", BOOK_DMG, "Wondrous item",
  "uncommon", "requires attunement",
  "Your walking speed becomes 30 feet unless it is already higher, and is "
  "not reduced by carrying weight up to your capacity. You can jump three "
  "times the normal distance, though never further than your remaining "
  "movement." },

{ "Boots of the Winterlands", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You have resistance to cold damage, difficult terrain of ice or snow "
  "costs no extra movement, and you can tolerate temperatures as low as "
  "-50 degrees Fahrenheit without extra protection." },

{ "Bowl of Commanding Water Elementals", BOOK_DMG, "Wondrous item",
  "rare", NULL,
  "While the bowl is filled with water, you can use an action to speak the "
  "command word and summon a water elemental as though you had cast "
  "conjure elemental. Once used, it cannot be used again until the next "
  "dawn. The bowl is about 1 foot across, 3 lb., and holds 3 gallons." },

{ "Bracers of Archery", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You have proficiency with the longbow and shortbow, and gain +2 to "
  "damage rolls on ranged attacks made with them." },

{ "Bracers of Defense", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "You gain +2 AC while wearing no armor and no shield." },

{ "Brazier of Commanding Fire Elementals", BOOK_DMG, "Wondrous item",
  "rare", NULL,
  "While a fire burns in the brazier, you can use an action to speak the "
  "command word and summon a fire elemental as though you had cast "
  "conjure elemental. Once used, it cannot be used again until the next "
  "dawn. The brazier weighs 5 lb." },

{ "Brooch of Shielding", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You have resistance to force damage and immunity to damage from the "
  "magic missile spell." },

{ "Broom of Flying", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "Speak the command word and it hovers beneath you; it has a flying "
  "speed of 50 feet and can carry up to 400 lb., though its speed drops "
  "to 30 feet above 200 lb. Spoken to from up to 1 mile away, it flies to "
  "you by the shortest route." },

{ "Candle of Invocation", BOOK_DMG, "Wondrous item", "very rare",
  "requires attunement",
  "Dedicated to a deity and its alignment. It burns for 4 hours, and can "
  "be snuffed out and relit. While it burns in your presence and your "
  "alignment matches, you gain advantage on all attack rolls, saving "
  "throws and ability checks. Lighting it for the first time lets you "
  "cast gate as an action." },

{ "Cape of the Mountebank", BOOK_DMG, "Wondrous item", "rare", NULL,
  "You can cast dimension door from it as an action. Once used, it cannot "
  "be used again until the next dawn. When you disappear you leave behind "
  "a cloud of smoke, and appear in another, both spaces lightly obscured "
  "until the end of your next turn." },

{ "Carpet of Flying", BOOK_DMG, "Wondrous item", "very rare", NULL,
  "Speak the command word to make it hover and fly. Its size, capacity "
  "and flying speed vary: 3 by 5 feet carries 200 lb. at 80 feet, 4 by 6 "
  "feet carries 400 lb. at 60 feet, 5 by 7 feet carries 600 lb. at 40 "
  "feet, and 6 by 9 feet carries 800 lb. at 30 feet. Above capacity it "
  "flies at half speed and carries at most twice its capacity." },

{ "Censer of Controlling Air Elementals", BOOK_DMG, "Wondrous item",
  "rare", NULL,
  "While incense burns in it, you can use an action to speak the command "
  "word and summon an air elemental as though you had cast conjure "
  "elemental. Once used, it cannot be used again until the next dawn. The "
  "censer weighs 1 lb." },

{ "Chime of Opening", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A hollow metal tube about 1 foot long. Striking it and pointing it at "
  "an object within 120 feet that can be opened -- a door, a lid, a lock, "
  "a set of shackles -- opens it, though it works on only one at a time. "
  "It has ten charges and is destroyed when the last is spent." },

{ "Circlet of Blasting", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "You can cast scorching ray from it as an action, with a +5 to hit for "
  "each ray. Once used, it cannot be used again until the next dawn." },

{ "Cloak of Arachnida", BOOK_DMG, "Wondrous item", "very rare",
  "requires attunement",
  "You have resistance to poison damage and a climbing speed equal to "
  "your walking speed, you can move up, down and across vertical surfaces "
  "and upside down along ceilings while leaving your hands free, you "
  "cannot be caught in webs of any sort and can move through them as if "
  "they were difficult terrain, and you can cast web once per dawn with a "
  "save DC of 13, unaffected by your own." },

{ "Cloak of Displacement", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "It projects an illusion that you are standing slightly away from your "
  "true position, so attack rolls against you have disadvantage. The "
  "property stops working until the start of your next turn whenever you "
  "take damage, and while you are incapacitated, restrained or otherwise "
  "unable to move." },

{ "Cloak of Elvenkind", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "With the hood up, Wisdom (Perception) checks made to see you have "
  "disadvantage and you have advantage on Dexterity (Stealth) checks made "
  "to hide. Pulling the hood up or down is a bonus action." },

{ "Cloak of Invisibility", BOOK_DMG, "Wondrous item", "legendary",
  "requires attunement",
  "As an action you can pull the hood over your head and become "
  "invisible. It has 2 hours of use, spent in increments of 1 minute; for "
  "every 12 hours it is not used it regains 1 hour." },

{ "Cloak of Protection", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You gain +1 to AC and to all saving throws." },

{ "Cloak of the Bat", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "You have advantage on Dexterity (Stealth) checks. In dim light or "
  "darkness you can grip the edges and gain a flying speed of 40 feet. As "
  "an action in dim light or darkness you can polymorph into a bat, "
  "keeping your Intelligence, Wisdom and Charisma." },

{ "Cloak of the Manta Ray", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "With the hood up you can breathe underwater and gain a swimming speed "
  "of 60 feet. Pulling the hood up or down is a bonus action." },

{ "Crystal Ball", BOOK_DMG, "Wondrous item",
  "very rare to legendary", "requires attunement",
  "About 6 inches across. While touching it you can cast scrying. The "
  "greater versions add: reading the surface thoughts of creatures seen "
  "through it (crystal ball of mind reading), casting detect magic or "
  "detect thoughts through the sensor (crystal ball of true seeing adds "
  "true seeing), or telepathic communication with a creature seen through "
  "it (crystal ball of telepathy)." },

{ "Cube of Force", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "A 36-charge cube; pressing a face spends charges to raise a barrier "
  "15 feet on a side around you, stopping gases and wind, living matter, "
  "nonliving matter, spells, or all of these. It regains 1d20 charges "
  "daily at dawn, and crumbles to dust if it starts a day with no "
  "charges." },

{ "Cubic Gate", BOOK_DMG, "Wondrous item", "legendary", NULL,
  "A 3-inch cube with a face for each of six planes, including the "
  "Material Plane. It has three charges and regains 1d3 daily at dawn. "
  "Pressing a face once casts gate to that plane; pressing it twice casts "
  "plane shift." },

{ "Dagger of Venom", BOOK_DMG, "Weapon (dagger)", "rare", NULL,
  "+1 to attack and damage rolls. As an action you can coat it in a thick "
  "black poison for 1 minute; a creature hit by it while it is coated "
  "must succeed on a DC 15 Constitution save or take 2d10 poison damage "
  "and be poisoned for 1 minute. Once used, it cannot be used again until "
  "the next dawn." },

{ "Dancing Sword", BOOK_DMG, "Weapon (any sword)", "very rare",
  "requires attunement",
  "As a bonus action you can toss it into the air; it hovers, flies up to "
  "30 feet and attacks one creature within 5 feet of it, using your "
  "attack roll and ability modifier. It can attack for up to 4 of your "
  "turns before falling, and returns to your hand as a bonus action." },

{ "Decanter of Endless Water", BOOK_DMG, "Wondrous item", "uncommon",
  NULL,
  "As an action, speak one of three command words: stream produces 1 "
  "gallon, fountain 5 gallons, and geyser a 30-foot line that knocks a "
  "creature prone on a failed DC 13 Strength save and can push a Large or "
  "smaller container. It can be used until the start of your next turn "
  "each time." },

{ "Deck of Illusions", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "A box of 34 parchment cards. Drawing one at random and throwing it "
  "makes an illusion of one or more creatures, which lasts until "
  "dispelled. The illusion looks and sounds real but can deal no damage "
  "and is revealed by a successful Intelligence (Investigation) check "
  "against your spell save DC, or by physical interaction." },

{ "Deck of Many Things", BOOK_DMG, "Wondrous item", "legendary", NULL,
  "A deck of 13 or 22 cards. You declare how many you will draw and must "
  "draw them all within 1 hour; each card takes effect at once. Effects "
  "range from gaining a keep, a wish or an ability point to losing all "
  "wealth, being imprisoned, or having your soul taken. Drawn cards "
  "vanish, and the Fool and Jester allow another draw." },

{ "Defender", BOOK_DMG, "Weapon (any sword)", "legendary",
  "requires attunement",
  "+3 to attack and damage rolls. Each turn you may choose to move any "
  "part of that bonus to your AC instead, keeping the split until the "
  "start of your next turn." },

{ "Demon Armor", BOOK_DMG, "Armor (plate)", "very rare",
  "requires attunement",
  "You gain +1 AC, can understand and speak Abyssal, and the gauntlets "
  "turn unarmed strikes into magical claw attacks: +1 to hit, dealing "
  "1d8 slashing damage plus your Strength modifier. Cursed: you cannot "
  "take it off, and you have disadvantage on attack rolls against demons "
  "and on saves against their spells and abilities." },

{ "Dimensional Shackles", BOOK_DMG, "Wondrous item", "rare", NULL,
  "You can put them on an incapacitated creature; they adjust to fit any "
  "size from Small to Large. While worn the creature cannot planeshift, "
  "teleport, or use extradimensional travel, and cannot be moved by such "
  "means. Escaping requires an action and a DC 30 Strength (Athletics) "
  "check, and only you or a creature you allow can remove them." },

{ "Dragon Scale Mail", BOOK_DMG, "Armor (scale mail)", "very rare",
  "requires attunement",
  "You gain +1 AC, advantage on saves against the Frightful Presence and "
  "breath weapons of dragons, and resistance to one damage type set by "
  "the kind of dragon whose scales made it. As an action you can learn "
  "the distance and direction to the closest dragon of that kind within "
  "30 miles, once per dawn." },

{ "Dragon Slayer", BOOK_DMG, "Weapon (any sword)", "rare", NULL,
  "+1 to attack and damage rolls. When you hit a dragon with it, the "
  "dragon takes an extra 3d6 damage of the weapon's type. For this "
  "purpose a dragon is any creature of the dragon type, including "
  "dragon turtles and wyverns." },

{ "Driftglobe", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "A glass sphere the size of a large marble, 1 lb. You can cast light or "
  "daylight from it; the daylight use recharges at dawn. While it is "
  "shedding light you can speak a command word to make it rise and hover "
  "up to 5 feet off the ground, following you at up to 30 feet away." },

{ "Dust of Disappearance", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "As an action you can throw the dust into the air; you and each "
  "creature and object within 10 feet become invisible for 2d4 minutes. "
  "The duration is the same for all, and a creature's invisibility ends "
  "early if it attacks or casts a spell." },

{ "Dust of Dryness", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "A pinch thrown into water turns a 15-foot cube of it into a marble-"
  "sized pellet; breaking the pellet releases the water. Thrown at a "
  "water-based creature it makes a DC 13 Constitution save or takes 10d6 "
  "necrotic damage. A packet holds 1d6+4 pinches." },

{ "Dust of Sneezing and Choking", BOOK_DMG, "Wondrous item", "uncommon",
  NULL,
  "Found as dust of disappearance; identification reveals it as a fake. "
  "Thrown into the air, each creature within 30 feet that needs to "
  "breathe makes a DC 15 Constitution save or becomes unable to breathe "
  "while sneezing uncontrollably, incapacitated and suffocating, until "
  "the effect is ended by lesser restoration or similar magic." },

{ "Dwarven Plate", BOOK_DMG, "Armor (plate)", "very rare", NULL,
  "You gain +2 AC. If an effect would move you against your will along "
  "the ground, you can use your reaction to reduce that movement by up to "
  "10 feet." },

{ "Dwarven Thrower", BOOK_DMG, "Weapon (warhammer)", "very rare",
  "requires attunement by a dwarf",
  "+3 to attack and damage rolls. It gains the thrown property with a "
  "range of 20/60 feet and returns to your hand after a thrown attack. "
  "A thrown hit deals an extra 1d8 bludgeoning damage, or 2d8 against a "
  "giant." },

{ "Efreeti Bottle", BOOK_DMG, "Wondrous item", "very rare", NULL,
  "Opening it as an action releases an efreeti in a cloud of smoke. Roll "
  "d100: it may attack you, grant three wishes, serve you for 1 hour, or "
  "serve you for 101 days. Once opened, it cannot be opened again for "
  "24 hours." },

{ "Efreeti Chain", BOOK_DMG, "Armor (chain mail)", "legendary",
  "requires attunement",
  "You gain +3 AC, immunity to fire damage, the ability to understand and "
  "speak Primordial, and a walking speed of 30 feet that works over "
  "molten rock and lava as though it were solid ground." },

{ "Elemental Gem", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "Breaking it as an action summons an elemental as though you had cast "
  "conjure elemental, and destroys the gem. The kind depends on the gem: "
  "blue sapphire an air elemental, yellow topaz an earth elemental, red "
  "corundum a fire elemental, emerald a water elemental." },

{ "Elven Chain", BOOK_DMG, "Armor (chain shirt)", "rare", NULL,
  "You gain +1 AC, and you are considered proficient with this armor even "
  "if you lack proficiency with medium armor." },

{ "Eversmoking Bottle", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "Smoke pours from it, filling a 60-foot radius with heavily obscuring "
  "fog that spreads 10 feet more each round to 120 feet, until stoppered. "
  "A wind of at least 10 miles an hour disperses it in 1 minute of "
  "moderate wind, or 1 round of strong wind." },

{ "Eyes of Charming", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "Crystal lenses with three charges, regaining all at dawn. As an action "
  "you can spend one charge to cast charm person on a humanoid within 30 "
  "feet, with a save DC of 13, provided you and it can see each other." },

{ "Eyes of Minute Seeing", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "You have advantage on Intelligence (Investigation) checks that rely on "
  "sight while searching an area or studying an object within 1 foot of "
  "you." },

{ "Eyes of the Eagle", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You have advantage on Wisdom (Perception) checks that rely on sight. "
  "In clear conditions you can make out details of even extremely distant "
  "creatures and objects as small as 2 feet across." },

{ "Quaal's Feather Token", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A tiny feather with one use. Anchor holds a ship in place for 1 day. "
  "Bird becomes a roc that carries you for 3 days. Fan makes a wind "
  "strong enough to fill sails. Swan boat becomes a boat for 24 hours. "
  "Tree becomes an oak. Whip becomes a floating whip that attacks at +9 "
  "for 1 hour." },

{ "Figurine of Wondrous Power", BOOK_DMG, "Wondrous item",
  "rare to very rare", NULL,
  "A statuette that becomes a living creature on the command word, "
  "friendly to you and obedient. It reverts to a figurine when its time "
  "runs out, when reduced to 0 hit points, or on a command. Kinds include "
  "the bronze griffon, ebony fly, golden lions, ivory goats, marble "
  "elephant, obsidian steed, onyx dog, serpentine owl and silver raven." },

{ "Flame Tongue", BOOK_DMG, "Weapon (any sword)", "rare",
  "requires attunement",
  "As a bonus action you can speak the command word and flames erupt from "
  "the blade, shedding bright light in a 40-foot radius and dim light for "
  "another 40. While alight, the sword deals an extra 2d6 fire damage on "
  "a hit. A second command word puts the flames out." },

{ "Folding Boat", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A wooden box 12 inches long which, on a command word, becomes a 10-"
  "foot boat for four or a 24-foot ship for fifteen, with a mast and "
  "oars. A third command word folds it back, provided nothing is aboard." },

{ "Frost Brand", BOOK_DMG, "Weapon (any sword)", "very rare",
  "requires attunement",
  "It deals an extra 1d6 cold damage on a hit. While you hold it you have "
  "resistance to fire damage. In freezing temperatures the blade sheds "
  "bright light in a 10-foot radius and dim light for another 10. When "
  "drawn you can extinguish all nonmagical flames within 30 feet, once "
  "per hour." },

{ "Gauntlets of Ogre Power", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "Your Strength score is 19 while you wear them. They have no effect if "
  "your Strength is already 19 or higher." },

{ "Gem of Brightness", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "A prism with 50 charges. One charge makes it shed bright light in a "
  "30-foot radius for 10 minutes. Two charges fire a beam at a creature "
  "within 60 feet, blinding it for 1 minute unless it succeeds on a DC 15 "
  "Constitution save. It becomes nonmagical when the last charge is "
  "spent." },

{ "Gem of Seeing", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "Three charges, regaining 1d3 daily at dawn. As an action, spend a "
  "charge and look through it: for 10 minutes you have truesight to 120 "
  "feet when you peer through the gem." },

{ "Giant Slayer", BOOK_DMG, "Weapon (any axe or sword)", "rare", NULL,
  "+1 to attack and damage rolls. When you hit a giant with it, the giant "
  "takes an extra 2d6 damage of the weapon's type and must succeed on a "
  "DC 15 Strength save or fall prone. For this purpose a giant is any "
  "creature of the giant type, including ettins and trolls." },

{ "Glamoured Studded Leather", BOOK_DMG, "Armor (studded leather)",
  "rare", NULL,
  "You gain +1 AC. As a bonus action you can make the armor look like "
  "normal clothing or another kind of armor; it keeps its bulk and weight "
  "but the illusion holds until you use the bonus action again or take "
  "the armor off." },

{ "Gloves of Missile Snaring", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "When you are hit by a ranged weapon attack, you can use your reaction "
  "to reduce the damage by 1d10 plus your Dexterity modifier. If you "
  "reduce it to 0 and the missile is small enough to hold in one hand and "
  "a hand is free, you catch it." },

{ "Gloves of Swimming and Climbing", BOOK_DMG, "Wondrous item",
  "uncommon", "requires attunement",
  "Climbing and swimming cost no extra movement, and you gain +5 to "
  "Strength (Athletics) checks made to climb or swim." },

{ "Gloves of Thievery", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "They are invisible while worn, and grant +5 to Dexterity (Sleight of "
  "Hand) checks and to Dexterity checks made to pick locks." },

{ "Goggles of Night", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "You have darkvision out to 60 feet. If you already have darkvision, "
  "wearing them increases its range by 60 feet." },

{ "Hammer of Thunderbolts", BOOK_DMG, "Weapon (maul)", "legendary",
  "requires attunement",
  "+1 to attack and damage rolls. If you have a belt of giant strength "
  "and gauntlets of ogre power attuned, your Strength score increases by "
  "4 to a maximum of 30 while you hold the hammer. On a hit against a "
  "giant, the giant must succeed on a DC 17 Constitution save or die. The "
  "hammer has five charges for a thunderclap that can kill." },

{ "Heward's Handy Haversack", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A backpack with a central pouch and two side pouches, each an "
  "extradimensional space: side pouches hold 20 lb. in 2 cubic feet, the "
  "central pouch 80 lb. in 8 cubic feet. It always weighs 5 lb., and "
  "retrieving an item is an action, with the desired item always on top." },

{ "Hat of Disguise", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You can cast disguise self from it at will. The spell ends if the hat "
  "is removed." },

{ "Headband of Intellect", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "Your Intelligence score is 19 while you wear it. It has no effect if "
  "your Intelligence is already 19 or higher." },

{ "Helm of Brilliance", BOOK_DMG, "Wondrous item", "very rare",
  "requires attunement",
  "Set with diamonds, rubies, fire opals and opals, each of which powers "
  "a spell: daylight, fireball, prismatic spray, and scorching ray. While "
  "it holds at least one opal you can use an action to make it shed "
  "bright light in a 30-foot radius, and while it holds a ruby you have "
  "resistance to fire damage." },

{ "Helm of Comprehending Languages", BOOK_DMG, "Wondrous item",
  "uncommon", NULL,
  "You can cast comprehend languages from it at will." },

{ "Helm of Telepathy", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You can cast detect thoughts from it, once per dawn, with a save DC of "
  "13. While a creature is affected, you can communicate telepathically "
  "with it as long as you are within 60 feet, and once per dawn you can "
  "cast suggestion on it." },

{ "Helm of Teleportation", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "Three charges, regaining 1d3 daily at dawn. As an action you can spend "
  "one charge to cast teleport." },

{ "Holy Avenger", BOOK_DMG, "Weapon (any sword)", "legendary",
  "requires attunement by a paladin",
  "+3 to attack and damage rolls. When you hit a fiend or undead with it, "
  "that creature takes an extra 2d10 radiant damage. While you hold the "
  "drawn sword it creates a 10-foot aura, or 30 feet at 17th level, in "
  "which you and friendly creatures have advantage on saving throws "
  "against spells and other magical effects." },

{ "Horn of Blasting", BOOK_DMG, "Wondrous item", "rare", NULL,
  "As an action you can blow it: each creature in a 30-foot cone makes a "
  "DC 15 Constitution save, taking 5d6 thunder damage and being deafened "
  "for 1 minute on a failure, or half damage on a success. Each use after "
  "the first in the same day has a 20 percent cumulative chance of "
  "exploding for 10d6 fire damage to the blower." },

{ "Horn of Valhalla", BOOK_DMG, "Wondrous item",
  "rare to legendary", NULL,
  "Blowing it summons 3d4+3 berserkers who fight for 1 hour or until "
  "dismissed. The silver horn has no requirement, the brass requires "
  "proficiency with all simple weapons, the bronze proficiency with all "
  "medium armor, and the iron proficiency with all martial weapons; "
  "blowing one without meeting its requirement turns the berserkers on "
  "you. Once used, it cannot be used again for 7 days." },

{ "Horseshoes of a Zephyr", BOOK_DMG, "Wondrous item", "very rare",
  NULL,
  "A set of four. The horse they are fitted to can move normally while "
  "hovering 4 inches above the ground, leaving no tracks and ignoring "
  "difficult terrain, and can travel for up to 12 hours a day without "
  "becoming exhausted." },

{ "Horseshoes of Speed", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A set of four. The horse they are fitted to has its walking speed "
  "increased by 30 feet." },

{ "Immovable Rod", BOOK_DMG, "Rod", "uncommon", NULL,
  "A flat iron rod with a button. Pressing the button fixes it in place "
  "magically, holding up to 8,000 lb.; more than that and it moves. A "
  "creature can use an action and a DC 30 Strength check to move it 10 "
  "feet. Pressing the button again ends the effect." },

{ "Daern's Instant Fortress", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A 1-inch adamantine cube. As an action you can speak the command word "
  "and it grows into a 20-foot square tower, 30 feet high, with arrow "
  "slits, battlements and a door that opens only for you. Creatures in "
  "the space take 10d10 bludgeoning damage on a failed DC 15 Dexterity "
  "save, or half on a success, and are pushed clear." },

{ "Ioun Stone", BOOK_DMG, "Wondrous item", "varies",
  "requires attunement",
  "As a bonus action you can toss it into the air, where it orbits your "
  "head at 1d3 feet and grants its benefit while it does. Kinds include "
  "absorption, agility, awareness, fortitude, greater absorption, "
  "insight, intellect, leadership, mastery, protection, regeneration, "
  "reserve, strength and sustenance. Another creature can take a stone "
  "with a successful attack or Sleight of Hand check." },

{ "Iron Bands of Bilarro", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A rusty iron sphere 3 inches across, 1 lb. As an action you can throw "
  "it at a Huge or smaller creature within 60 feet as a ranged attack "
  "with a range of 60 feet; on a hit the bands wrap the target and "
  "restrain it, which it can escape with a DC 20 Strength check as an "
  "action. Once used, it cannot be used again until the next dawn." },

{ "Iron Flask", BOOK_DMG, "Wondrous item", "legendary", NULL,
  "As an action you can point the open flask at a creature on another "
  "plane within 60 feet; it must succeed on a DC 17 Wisdom save or be "
  "trapped inside. Opening the flask releases a trapped creature, which "
  "serves you for 1 hour if you have never released one before, and is "
  "otherwise hostile. The flask holds only one creature at a time." },

{ "Javelin of Lightning", BOOK_DMG, "Weapon (javelin)", "uncommon",
  NULL,
  "As a bonus action you can speak the command word before you throw it; "
  "it becomes a lightning bolt 5 feet wide and 120 feet long, dealing "
  "4d6 lightning damage on a failed DC 13 Dexterity save or half on a "
  "success, and its own damage on a hit. The property cannot be used "
  "again until the next dawn." },

{ "Lantern of Revealing", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "While lit, it sheds bright light in a 30-foot radius and dim light for "
  "another 30. Invisible creatures and objects in that bright light are "
  "visible. As an action you can lower the hood, reducing the light to a "
  "5-foot radius of dim light. It burns for 6 hours on 1 pint of oil." },

{ "Luck Blade", BOOK_DMG, "Weapon (any sword)", "legendary",
  "requires attunement",
  "+1 to attack and damage rolls, and you gain +1 to saving throws while "
  "you carry it. Once per dawn you can reroll one attack roll, ability "
  "check or saving throw. It also holds 1d4-1 wishes, which can be spent "
  "to cast wish; when the last is used the sword keeps its other "
  "properties." },

{ "Mace of Disruption", BOOK_DMG, "Weapon (mace)", "rare",
  "requires attunement",
  "On a hit against a fiend or undead, the target takes an extra 2d6 "
  "radiant damage; if that leaves it with 25 hit points or fewer it must "
  "succeed on a DC 15 Wisdom save or be destroyed, and on a success it "
  "becomes frightened of you until the end of your next turn. While you "
  "hold it, the mace sheds bright light in a 20-foot radius." },

{ "Mace of Smiting", BOOK_DMG, "Weapon (mace)", "rare", NULL,
  "+1 to attack and damage rolls, or +3 against constructs. On a hit "
  "against a construct it deals an extra 2d6 bludgeoning damage, and a "
  "roll of 20 destroys the construct outright if it has 25 hit points or "
  "fewer after the damage." },

{ "Mace of Terror", BOOK_DMG, "Weapon (mace)", "rare",
  "requires attunement",
  "Three charges, regaining 1d3 daily at dawn. As an action you can spend "
  "a charge to release a wave of terror: each creature of your choice "
  "within 30 feet must succeed on a DC 15 Wisdom save or be frightened of "
  "you for 1 minute, unable to move closer to you and able to repeat the "
  "save at the end of each of its turns." },

{ "Mantle of Spell Resistance", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "You have advantage on saving throws against spells while you wear it." },

{ "Manual of Bodily Health", BOOK_DMG, "Wondrous item", "very rare",
  NULL,
  "Reading it over 48 hours across at least six days raises your "
  "Constitution score and your Constitution maximum by 2. The manual then "
  "loses its magic for a century." },

{ "Manual of Gainful Exercise", BOOK_DMG, "Wondrous item", "very rare",
  NULL,
  "Reading it over 48 hours across at least six days raises your Strength "
  "score and your Strength maximum by 2. The manual then loses its magic "
  "for a century." },

{ "Manual of Golems", BOOK_DMG, "Wondrous item", "very rare", NULL,
  "A tome describing how to make one kind of golem -- clay, flesh, iron "
  "or stone. Only a character with the listed minimum level and spell "
  "slots can use it, and the work takes months and thousands of gold "
  "pieces. The manual is destroyed when the golem is finished." },

{ "Manual of Quickness of Action", BOOK_DMG, "Wondrous item",
  "very rare", NULL,
  "Reading it over 48 hours across at least six days raises your "
  "Dexterity score and your Dexterity maximum by 2. The manual then loses "
  "its magic for a century." },

{ "Nolzur's Marvelous Pigments", BOOK_DMG, "Wondrous item", "very rare", NULL,
  "1d4 pots and a brush. Painting a two-dimensional object on a surface "
  "makes the real thing, up to 1,000 cubic feet, using 10 minutes and one "
  "pot per 100 square feet. Objects made this way are real but ordinary; "
  "no creatures or magic items can be created." },

{ "Mariner's Armor", BOOK_DMG, "Armor (light, medium or heavy)",
  "uncommon", NULL,
  "While you wear it you have a swimming speed equal to your walking "
  "speed, and whenever you start your turn underwater with 0 hit points "
  "the armor causes you to rise 60 feet toward the surface." },

{ "Medallion of Thoughts", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "Three charges, regaining 1d3 daily at dawn. As an action you can spend "
  "a charge to cast detect thoughts, with a save DC of 13." },

{ "Mirror of Life Trapping", BOOK_DMG, "Wondrous item", "very rare",
  NULL,
  "A 4-foot-wide mirror with twelve extradimensional cells. Any creature "
  "other than you that sees its reflection while within 30 feet must "
  "succeed on a DC 15 Charisma save or be trapped in a cell. You can free "
  "a trapped creature by speaking the command word, or shatter the mirror "
  "to free them all." },

{ "Mithral Armor", BOOK_DMG, "Armor (medium or heavy, not hide)",
  "uncommon", NULL,
  "Light and flexible. If the armor normally imposes disadvantage on "
  "Dexterity (Stealth) checks or has a Strength requirement, the mithral "
  "version does not." },

{ "Necklace of Adaptation", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You can breathe normally in any environment, and you have advantage on "
  "saving throws made against harmful gases and vapors -- cloudkill and "
  "stinking cloud effects, inhaled poisons, and the breath weapons of "
  "some dragons." },

{ "Necklace of Fireballs", BOOK_DMG, "Wondrous item", "rare", NULL,
  "It holds 1d6+3 beads. As an action you can detach one and throw it up "
  "to 60 feet; it bursts as a 3rd-level fireball, DC 15. Two or more "
  "thrown together add a level for each bead beyond the first." },

{ "Necklace of Prayer Beads", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement by a cleric, druid or paladin",
  "It has 1d4+2 magic beads, each holding a spell that you can cast as a "
  "bonus action using the bead: bless, cure wounds (2nd level), "
  "greater restoration, planar ally, wind walk, or a smite spell. Each "
  "bead recharges at dawn." },

{ "Nine Lives Stealer", BOOK_DMG, "Weapon (any sword)", "very rare",
  "requires attunement",
  "+1 to attack and damage rolls. It has 1d8+1 charges. When you score a "
  "critical hit against a creature with fewer than 100 hit points, it "
  "must succeed on a DC 15 Constitution save or die, spending a charge. "
  "With no charges left the sword keeps its bonus." },

{ "Oathbow", BOOK_DMG, "Weapon (longbow)", "very rare",
  "requires attunement",
  "Nocking an arrow and saying \"Swift defeat to my enemies\" names your "
  "sworn enemy. Against it your attacks with this bow never suffer "
  "disadvantage and deal an extra 3d6 piercing damage, or 3d10 against a "
  "creature immune to piercing damage. The oath ends when the enemy dies "
  "or when you swear a new one, and while sworn you have disadvantage on "
  "attacks with other weapons." },

{ "Oil of Etherealness", BOOK_DMG, "Potion", "rare", NULL,
  "Applying it to the body, which takes 10 minutes, gives the effect of "
  "the etherealness spell for 1 hour. One vial covers one Medium or "
  "smaller creature and its carried gear." },

{ "Oil of Sharpness", BOOK_DMG, "Potion", "very rare", NULL,
  "One vial coats one slashing or piercing weapon, or up to five pieces "
  "of ammunition, taking 1 minute to apply. For 1 hour the coated weapon "
  "is magical and gains +3 to attack and damage rolls." },

{ "Oil of Slipperiness", BOOK_DMG, "Potion", "uncommon", NULL,
  "Applied to the body, which takes 10 minutes, it gives the effect of "
  "the freedom of movement spell for 8 hours; poured on the ground it "
  "covers a 10-foot square as a grease spell for 8 hours." },

{ "Pearl of Power", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement by a spellcaster",
  "As an action you can speak the command word and regain one expended "
  "spell slot of 3rd level or lower. Once used, it cannot be used again "
  "until the next dawn." },

{ "Periapt of Health", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "You are immune to contracting any disease while you wear it. If you "
  "are already infected, the disease's effects are suppressed while you "
  "wear it." },

{ "Periapt of Proof against Poison", BOOK_DMG, "Wondrous item", "rare",
  NULL,
  "Any poison in your system is neutralized when you put it on, you are "
  "immune to poison damage, and you have immunity to the poisoned "
  "condition." },

{ "Periapt of Wound Closure", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You stabilize whenever you are dying at the start of your turn, and "
  "whenever you roll a Hit Die to regain hit points the die's result is "
  "doubled." },

{ "Philter of Love", BOOK_DMG, "Potion", "uncommon", NULL,
  "For 1 hour after drinking it, you are charmed by the first creature "
  "you see within 10 minutes of drinking, and if it is of a species and "
  "gender you are normally attracted to you regard it as your true love." },

{ "Pipes of Haunting", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "Three charges, regaining 1d3 daily at dawn. Playing them as an action "
  "and spending a charge makes each creature within 30 feet succeed on a "
  "DC 15 Wisdom save or become frightened of you for 1 minute, repeating "
  "the save at the end of each of its turns. You must be proficient with "
  "wind instruments to use them." },

{ "Pipes of the Sewers", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "Three charges, regaining 1d3 daily at dawn. While you play them, "
  "ordinary rats and giant rats within 1/2 mile come to you. As an action "
  "you can spend a charge to command those within 30 feet, and while you "
  "keep playing they follow your telepathic commands. You must be "
  "proficient with wind instruments to use them." },

{ "Plate Armor of Etherealness", BOOK_DMG, "Armor (plate)",
  "legendary", "requires attunement",
  "As an action you can speak the command word to gain the effect of the "
  "etherealness spell for 10 minutes, or until you remove the armor or "
  "use an action to end it. Once used, it cannot be used again until the "
  "next dawn." },

{ "Portable Hole", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A 6-foot circle of black cloth that unfolds into an extradimensional "
  "hole 10 feet deep. Folding it closed with a creature inside gives that "
  "creature 10 minutes of air before it suffocates. Placing it inside a "
  "bag of holding destroys both and opens a gate to the Astral Plane." },

{ "Potion of Animal Friendship", BOOK_DMG, "Potion", "uncommon", NULL,
  "For 1 hour after drinking it you can cast animal friendship at will, "
  "with a save DC of 13." },

{ "Potion of Clairvoyance", BOOK_DMG, "Potion", "rare", NULL,
  "Drinking it gives you the effect of the clairvoyance spell." },

{ "Potion of Climbing", BOOK_DMG, "Potion", "common", NULL,
  "For 1 hour after drinking it you gain a climbing speed equal to your "
  "walking speed, and you have advantage on Strength (Athletics) checks "
  "made to climb." },

{ "Potion of Diminution", BOOK_DMG, "Potion", "rare", NULL,
  "Drinking it gives you the reduce effect of the enlarge/reduce spell "
  "for 1d4 hours, without concentration." },

{ "Potion of Fire Breath", BOOK_DMG, "Potion", "uncommon", NULL,
  "For 1 hour after drinking it, or until you use it three times, you can "
  "use a bonus action to exhale fire at a target within 30 feet, dealing "
  "4d6 fire damage on a failed DC 13 Dexterity save or half on a "
  "success." },

{ "Potion of Flying", BOOK_DMG, "Potion", "very rare", NULL,
  "For 1 hour after drinking it you gain a flying speed equal to your "
  "walking speed and can hover; if you are still aloft when it ends you "
  "fall, unless you can stop the fall." },

{ "Potion of Gaseous Form", BOOK_DMG, "Potion", "rare", NULL,
  "Drinking it gives you the effect of the gaseous form spell for 1 hour, "
  "without concentration, or until you end the effect as a bonus "
  "action." },

{ "Potion of Giant Strength", BOOK_DMG, "Potion",
  "uncommon to legendary", NULL,
  "For 1 hour after drinking it your Strength score becomes the potion's "
  "score, if it is not already higher: hill giant 21 (uncommon), stone or "
  "frost giant 23 (rare), fire giant 25 (rare), cloud giant 27 (very "
  "rare), storm giant 29 (legendary)." },

{ "Potion of Growth", BOOK_DMG, "Potion", "uncommon", NULL,
  "Drinking it gives you the enlarge effect of the enlarge/reduce spell "
  "for 1d4 hours, without concentration." },

{ "Potion of Healing", BOOK_DMG, "Potion", "common to very rare", NULL,
  "Drinking it as an action restores hit points: healing 2d4+2 (common, "
  "50 gp), greater healing 4d4+4 (uncommon, 100 gp), superior healing "
  "8d4+8 (rare, 500 gp), supreme healing 10d4+20 (very rare, 5,000 gp)." },

{ "Potion of Heroism", BOOK_DMG, "Potion", "rare", NULL,
  "For 1 hour after drinking it you gain 10 temporary hit points and are "
  "under the effect of the bless spell, without concentration." },

{ "Potion of Invisibility", BOOK_DMG, "Potion", "very rare", NULL,
  "You become invisible for 1 hour, along with anything you wear or "
  "carry. The effect ends early if you attack or cast a spell." },

{ "Potion of Invulnerability", BOOK_DMG, "Potion", "rare", NULL,
  "For 1 minute after drinking it you have resistance to all damage." },

{ "Potion of Longevity", BOOK_DMG, "Potion", "very rare", NULL,
  "Drinking it reduces your age by 1d6+6 years, to a minimum of 13. Each "
  "time you drink it after the first there is a cumulative 10 percent "
  "chance, starting at 10 percent, that it instead ages you by 1d6+6 "
  "years." },

{ "Potion of Mind Reading", BOOK_DMG, "Potion", "rare", NULL,
  "Drinking it gives you the effect of the detect thoughts spell, with a "
  "save DC of 13." },

{ "Potion of Poison", BOOK_DMG, "Potion", "uncommon", NULL,
  "It looks, smells and tastes like a healing potion, and identify "
  "reveals only that. Drinking it deals 3d6 poison damage on a failed DC "
  "13 Constitution save and poisons you for 1 hour, or half damage with "
  "no poisoning on a success. While poisoned this way you take 3d6 poison "
  "damage at the start of each of your turns." },

{ "Potion of Resistance", BOOK_DMG, "Potion", "uncommon", NULL,
  "For 1 hour after drinking it you have resistance to one type of "
  "damage, chosen when the potion is made from among acid, cold, fire, "
  "force, lightning, necrotic, poison, psychic, radiant and thunder." },

{ "Potion of Speed", BOOK_DMG, "Potion", "very rare", NULL,
  "For 1 minute after drinking it you gain the effect of the haste spell, "
  "without concentration." },

{ "Potion of Vitality", BOOK_DMG, "Potion", "very rare", NULL,
  "Drinking it removes any exhaustion and cures any disease or poison "
  "affecting you. For the next 24 hours you regain the maximum number of "
  "hit points for any Hit Die you spend." },

{ "Potion of Water Breathing", BOOK_DMG, "Potion", "uncommon", NULL,
  "You can breathe underwater for 1 hour after drinking it." },

{ "Keoghtom's Ointment", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "A jar of 1d4+1 doses. As an action, one dose can be swallowed or "
  "applied to the skin: it restores 2d8+2 hit points, cures any disease "
  "or poison, and ends the blinded and deafened conditions." },

{ "Ring of Animal Influence", BOOK_DMG, "Ring", "rare", NULL,
  "Three charges, regaining all daily at dawn. You can spend a charge to "
  "cast animal friendship (save DC 13), fear on beasts only (save DC 13), "
  "or speak with animals." },

{ "Ring of Djinni Summoning", BOOK_DMG, "Ring", "legendary",
  "requires attunement",
  "You can speak its command word as an action to summon a particular "
  "djinni from the Elemental Plane of Air. It appears in an unoccupied "
  "space within 120 feet, is friendly to you and your companions, obeys "
  "your commands, and vanishes after 1 hour, when it drops to 0 hit "
  "points, or when you dismiss it. Once used, it cannot be used again "
  "until the next dawn." },

{ "Ring of Elemental Command", BOOK_DMG, "Ring", "legendary",
  "requires attunement",
  "Linked to one of the four Elemental Planes. You have advantage on "
  "attack rolls against elementals of that plane and they have "
  "disadvantage against you, you gain resistances and movement suited to "
  "the element, and the ring holds spells drawn from its plane. It has "
  "five charges, regaining 1d4+1 daily at dawn, and grows in power as you "
  "slay elementals of its plane." },

{ "Ring of Evasion", BOOK_DMG, "Ring", "rare", "requires attunement",
  "Three charges, regaining 1d3 daily at dawn. When you fail a Dexterity "
  "saving throw you can use your reaction to spend a charge and succeed "
  "instead." },

{ "Ring of Feather Falling", BOOK_DMG, "Ring", "rare",
  "requires attunement",
  "When you fall more than 10 feet you descend 60 feet per round and take "
  "no damage from the fall." },

{ "Ring of Free Action", BOOK_DMG, "Ring", "rare",
  "requires attunement",
  "Difficult terrain costs you no extra movement, and magic can neither "
  "reduce your speed nor cause you to be paralyzed or restrained." },

{ "Ring of Invisibility", BOOK_DMG, "Ring", "legendary",
  "requires attunement",
  "As an action you can turn invisible, along with anything you wear or "
  "carry, until you use an action to become visible, until you attack or "
  "cast a spell, or until you take the ring off." },

{ "Ring of Jumping", BOOK_DMG, "Ring", "uncommon",
  "requires attunement",
  "You can cast jump from it as a bonus action, targeting only yourself." },

{ "Ring of Mind Shielding", BOOK_DMG, "Ring", "uncommon",
  "requires attunement",
  "You are immune to magic that reads your thoughts, determines whether "
  "you are lying, or determines your alignment or creature type, and such "
  "magic reports whatever you choose. You cannot be targeted by magic "
  "that detects your location, and as an action you can make the ring "
  "invisible. If you die while wearing it, your soul enters it." },

{ "Ring of Protection", BOOK_DMG, "Ring", "rare",
  "requires attunement",
  "You gain +1 to AC and to all saving throws." },

{ "Ring of Regeneration", BOOK_DMG, "Ring", "very rare",
  "requires attunement",
  "You regain 1d6 hit points every 10 minutes, provided you have at least "
  "1 hit point. If you lose a body part, it grows back and returns to "
  "full function after 1d6+1 days, provided you have at least 1 hit point "
  "the whole time." },

{ "Ring of Resistance", BOOK_DMG, "Ring", "rare",
  "requires attunement",
  "You have resistance to one damage type, set by the gem in the ring: "
  "acid (pearl), cold (tourmaline), fire (garnet), force (sapphire), "
  "lightning (citrine), necrotic (jet), poison (amethyst), psychic "
  "(jade), radiant (topaz), thunder (spinel)." },

{ "Ring of Shooting Stars", BOOK_DMG, "Ring", "very rare",
  "requires attunement outdoors at night",
  "Six charges, regaining 1d6 daily at dawn. In dim light or darkness you "
  "can cast dancing lights and light at will, and spend charges for "
  "faerie fire, ball lightning, or shooting stars that deal 5d4 fire "
  "damage to creatures they strike." },

{ "Ring of Spell Storing", BOOK_DMG, "Ring", "rare",
  "requires attunement",
  "It stores spells of a total of 5 levels, cast into it by any creature. "
  "The stored spell uses the original caster's slot level, save DC, "
  "attack bonus and spellcasting ability. Any creature can cast a stored "
  "spell out of the ring, which frees that many levels of storage." },

{ "Ring of Spell Turning", BOOK_DMG, "Ring", "legendary",
  "requires attunement",
  "You have advantage on saving throws against any spell that targets "
  "only you, not an area. If you succeed on the save against a spell of "
  "7th level or lower, the spell has no effect on you and is turned back "
  "on the caster, using the caster's own slot level and DC." },

{ "Ring of Swimming", BOOK_DMG, "Ring", "uncommon", NULL,
  "You have a swimming speed of 40 feet while wearing it." },

{ "Ring of Telekinesis", BOOK_DMG, "Ring", "very rare",
  "requires attunement",
  "You can cast telekinesis at will, but you can target only objects that "
  "are not being worn or carried." },

{ "Ring of the Ram", BOOK_DMG, "Ring", "rare", "requires attunement",
  "Three charges, regaining 1d3 daily at dawn. As an action you can spend "
  "up to three charges to make a spectral ram attack a creature within 60 "
  "feet: +7 to hit, dealing 2d10 force damage per charge and pushing the "
  "target 5 feet per charge. It can also break objects and force doors." },

{ "Ring of Three Wishes", BOOK_DMG, "Ring", "legendary", NULL,
  "Three charges. You can spend one charge to cast the wish spell. The "
  "ring becomes nonmagical when the last charge is spent." },

{ "Ring of Warmth", BOOK_DMG, "Ring", "uncommon",
  "requires attunement",
  "You have resistance to cold damage, and you and everything you wear "
  "and carry are unharmed by temperatures as low as -50 degrees "
  "Fahrenheit." },

{ "Ring of Water Walking", BOOK_DMG, "Ring", "uncommon", NULL,
  "You can stand on and move across any liquid surface as though it were "
  "solid ground." },

{ "Ring of X-ray Vision", BOOK_DMG, "Ring", "rare",
  "requires attunement",
  "As an action you can see into and through solid matter for 1 minute, "
  "to a range of 30 feet, though 1 foot of stone, 1 inch of common metal, "
  "a thin sheet of lead or 3 feet of wood blocks it. Using it again "
  "before a long rest costs you a level of exhaustion on a failed DC 15 "
  "Constitution save." },

{ "Robe of Eyes", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "You have advantage on Wisdom (Perception) checks that rely on sight, "
  "darkvision to 120 feet, and you can see invisible creatures and into "
  "the Ethereal Plane within 120 feet. You cannot be blinded, but a light "
  "or daylight spell cast on the robe blinds you for 1 minute." },

{ "Robe of Scintillating Colors", BOOK_DMG, "Wondrous item",
  "very rare", "requires attunement",
  "Three charges, regaining 1d3 daily at dawn. As an action you can spend "
  "a charge to make the robe display shifting colors for 1 minute: you "
  "shed bright light in a 30-foot radius, creatures that can see you have "
  "disadvantage on attack rolls against you, and any creature within 30 "
  "feet that starts its turn able to see you must succeed on a DC 15 "
  "Wisdom save or be stunned until the effect ends." },

{ "Robe of Stars", BOOK_DMG, "Wondrous item", "very rare",
  "requires attunement",
  "You gain +1 to saving throws against spells. Six stars on it can each "
  "be used as an action to cast magic missile at 5th level, recovering "
  "daily at dusk. You can also use an action to enter the Astral Plane "
  "along with everything you are wearing and carrying, and return to the "
  "space you left or the nearest unoccupied space." },

{ "Robe of Useful Items", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "Patches on the robe become real objects. It begins with two each of "
  "dagger, bullseye lantern (filled and lit), steel mirror, 10-foot pole, "
  "hempen rope and sack, plus 4d4 more rolled at random from a table "
  "ranging from a bag of 100 gp to a portable ram or a 24-foot sailing "
  "boat. Detaching a patch is an action." },

{ "Robe of the Archmagi", BOOK_DMG, "Wondrous item", "legendary",
  "requires attunement by a sorcerer, warlock or wizard",
  "Your base AC is 15 plus your Dexterity modifier while you wear no "
  "armor, your spell save DC and spell attack bonus each increase by 2, "
  "and you have advantage on saving throws against spells and other "
  "magical effects." },

{ "Rod of Absorption", BOOK_DMG, "Rod", "very rare",
  "requires attunement",
  "While holding it you can use your reaction to absorb a spell targeting "
  "only you, cancelling it and storing its level, up to 50 levels over "
  "the rod's life. A spellcaster holding the rod can convert stored "
  "levels into spell slots of up to 5th level for spells on their own "
  "list. The rod becomes nonmagical once it can absorb no more." },

{ "Rod of Alertness", BOOK_DMG, "Rod", "very rare",
  "requires attunement",
  "You gain advantage on Wisdom (Perception) checks and on rolls for "
  "initiative. While holding it you can cast detect evil and good, detect "
  "magic, detect poison and disease, or see invisibility. As an action "
  "you can plant it in the ground for a 600-foot aura of light and a "
  "+1 bonus to AC and saving throws for allies within 10 feet." },

{ "Rod of Lordly Might", BOOK_DMG, "Rod", "legendary",
  "requires attunement",
  "A +3 mace with six buttons: it becomes a flame tongue, a +3 battleaxe, "
  "a +3 spear, a climbing pole up to 50 feet, a battering ram that grants "
  "+10 to open doors, or an indicator of magnetic north and your depth. "
  "It also has three drain-life and paralysis powers, recharging at "
  "dawn." },

{ "Rod of Resurrection", BOOK_DMG, "Rod", "legendary",
  "requires attunement by a cleric, druid or paladin",
  "Five charges, regaining 1 daily at dawn. You can spend one charge to "
  "cast heal, or five to cast resurrection. If the last charge is spent, "
  "roll a d20: on a 1 the rod vanishes in a burst of radiance." },

{ "Rod of Rulership", BOOK_DMG, "Rod", "rare", "requires attunement",
  "As an action you can command obedience: each creature of your choice "
  "within 120 feet that can see and hear you must succeed on a DC 15 "
  "Wisdom save or be charmed by you for 8 minutes, or until you or your "
  "companions harm it. Once used, it cannot be used again until the next "
  "dawn." },

{ "Rod of Security", BOOK_DMG, "Rod", "very rare", NULL,
  "As an action you can transport yourself and up to 199 other willing "
  "creatures to a paradise on a demiplane, where time passes slowly and "
  "creatures do not age. You can stay up to 200 days divided by the "
  "number of creatures. Once used, it cannot be used again for 10 days." },

{ "Rod of the Pact Keeper", BOOK_DMG, "Rod",
  "uncommon (+1), rare (+2), very rare (+3)",
  "requires attunement by a warlock",
  "You gain a bonus to spell attack rolls and to the saving throw DCs of "
  "your warlock spells. Once per long rest you can regain one expended "
  "warlock spell slot as an action." },

{ "Rope of Climbing", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "A 60-foot silk rope weighing 3 lb. and holding up to 3,000 lb. Holding "
  "one end you can speak the command word as an action to make it animate "
  "and move as you direct, knotting or unknotting itself. Knots make it "
  "50 feet long and grant advantage on checks to climb it. It has AC 20, "
  "20 hit points and regains 1 hit point every 5 minutes." },

{ "Rope of Entanglement", BOOK_DMG, "Wondrous item", "rare", NULL,
  "A 30-foot rope. As an action you can command it to entangle a creature "
  "within 20 feet; the target must succeed on a DC 15 Dexterity save or "
  "be restrained. Escaping requires a DC 15 Strength or Dexterity check "
  "as an action. The rope has AC 20 and 20 hit points, regaining 1 hit "
  "point every 5 minutes." },

{ "Scarab of Protection", BOOK_DMG, "Wondrous item", "legendary",
  "requires attunement",
  "You gain advantage on saving throws against spells. It has 12 charges: "
  "when you fail a saving throw against a necromancy spell or a harmful "
  "effect from an undead, you can use your reaction to spend a charge and "
  "succeed instead. It turns to powder when the last charge is spent." },

{ "Scimitar of Speed", BOOK_DMG, "Weapon (scimitar)", "very rare",
  "requires attunement",
  "+2 to attack and damage rolls, and you can make one attack with it as "
  "a bonus action on each of your turns." },

{ "Shield, +1, +2, or +3", BOOK_DMG, "Armor (shield)",
  "uncommon (+1), rare (+2), very rare (+3)", NULL,
  "While holding it you have a bonus to AC on top of the shield's normal "
  "bonus, set by its rarity." },

{ "Shield of Missile Attraction", BOOK_DMG, "Armor (shield)", "rare",
  "requires attunement",
  "You have resistance to damage from ranged weapon attacks. Cursed: you "
  "are attuned until targeted by remove curse or similar magic, and "
  "whenever a ranged weapon attack is made against a target within 10 "
  "feet of you, the curse makes you the target instead." },

{ "Slippers of Spider Climbing", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You can move up, down and across vertical surfaces and upside down "
  "along ceilings while leaving your hands free. You have a climbing "
  "speed equal to your walking speed, but the slippers do not work on "
  "slippery surfaces such as ice or oil." },

{ "Sovereign Glue", BOOK_DMG, "Wondrous item", "legendary", NULL,
  "A viscous, milky-white liquid, found in a vial holding 1d6+1 ounces. "
  "One ounce covers a 1-foot square and sets in 1 minute, bonding "
  "permanently. Only universal solvent, oil of etherealness or a wish "
  "spell can undo the bond." },

{ "Spell Scroll", BOOK_DMG, "Scroll",
  "common (cantrip) to legendary (9th level)", NULL,
  "A single spell written on the scroll. If the spell is on your class's "
  "list you can cast it from the scroll without a slot or material "
  "components; otherwise you must make a DC 10 + spell level ability "
  "check with your spellcasting ability, and the scroll is destroyed "
  "either way. The scroll's save DC is 13 + half the spell's level "
  "rounded down." },

{ "Spellguard Shield", BOOK_DMG, "Armor (shield)", "very rare",
  "requires attunement",
  "You have advantage on saving throws against spells and other magical "
  "effects, and spell attack rolls against you have disadvantage." },

{ "Sphere of Annihilation", BOOK_DMG, "Wondrous item", "legendary",
  NULL,
  "A 2-foot black sphere that annihilates all matter it touches. As an "
  "action you can control it with a DC 25 Intelligence (Arcana) check, "
  "moving it 5 feet per 5 points by which you beat the DC; on a failure "
  "it moves 10 feet toward you. A creature it touches takes 4d10 force "
  "damage on a failed DC 13 Dexterity save." },

{ "Staff of Charming", BOOK_DMG, "Staff", "rare",
  "requires attunement by a bard, cleric, druid, sorcerer, warlock or "
  "wizard",
  "Ten charges, regaining 1d8+2 daily at dawn. You can spend a charge to "
  "cast charm person, command or comprehend languages using your save DC, "
  "or use your reaction to absorb an enchantment spell targeting only "
  "you. Spending the last charge risks the staff crumbling to dust." },

{ "Staff of Fire", BOOK_DMG, "Staff", "very rare",
  "requires attunement by a druid, sorcerer, warlock or wizard",
  "You have resistance to fire damage while you hold it. Ten charges, "
  "regaining 1d6+4 daily at dawn: burning hands (1 charge), fireball (3), "
  "wall of fire (4). Spending the last charge risks the staff being "
  "consumed in flame." },

{ "Staff of Frost", BOOK_DMG, "Staff", "very rare",
  "requires attunement by a druid, sorcerer, warlock or wizard",
  "You have resistance to cold damage while you hold it. Ten charges, "
  "regaining 1d6+4 daily at dawn: cone of cold (5 charges), fog cloud "
  "(1), ice storm (4), wall of ice (4). Spending the last charge risks "
  "the staff turning to water." },

{ "Staff of Healing", BOOK_DMG, "Staff", "rare",
  "requires attunement by a bard, cleric or druid",
  "Ten charges, regaining 1d6+4 daily at dawn: cure wounds at up to 4th "
  "level (1 charge per level), lesser restoration (2), mass cure wounds "
  "(5). Spending the last charge risks the staff vanishing in a flash of "
  "light." },

{ "Staff of Power", BOOK_DMG, "Staff", "very rare",
  "requires attunement by a sorcerer, warlock or wizard",
  "A quarterstaff granting +2 to attack, damage and AC, and +2 to saving "
  "throws. Twenty charges, regaining 2d8+4 daily at dawn, spent on cone "
  "of cold, fireball, globe of invulnerability, hold monster, levitate, "
  "lightning bolt, magic missile, ray of enfeeblement or wall of force. "
  "You can break the staff for a retributive strike." },

{ "Staff of Striking", BOOK_DMG, "Staff", "very rare",
  "requires attunement",
  "A quarterstaff granting +3 to attack and damage rolls. Ten charges, "
  "regaining 1d6+4 daily at dawn; on a hit you can spend up to three "
  "charges to deal an extra 1d6 force damage per charge." },

{ "Staff of Swarming Insects", BOOK_DMG, "Staff", "rare",
  "requires attunement by a bard, cleric, druid, sorcerer, warlock or "
  "wizard",
  "Ten charges, regaining 1d6+4 daily at dawn: giant insect (4 charges), "
  "insect plague (5), or a swarm of insects around you for 1 charge that "
  "deals 2d10 piercing damage and blocks sight. Spending the last charge "
  "risks the staff being devoured by insects." },

{ "Staff of the Adder", BOOK_DMG, "Staff", "uncommon",
  "requires attunement",
  "As a bonus action you can turn the head into a poisonous snake for 1 "
  "minute; you can attack with it at +9 to hit, reach 5 feet, dealing 1d6 "
  "piercing damage and 3d6 poison damage. The head has AC 15 and 20 hit "
  "points, and destroying it destroys the staff." },

{ "Staff of the Magi", BOOK_DMG, "Staff", "legendary",
  "requires attunement by a sorcerer, warlock or wizard",
  "A quarterstaff granting +2 to attack and damage rolls and advantage on "
  "saving throws against spells. Fifty charges, regaining 4d6+2 daily at "
  "dawn, spent on a long list of spells up to conjure elemental, plane "
  "shift and passwall. It can also absorb spells targeting only you, and "
  "can be broken for a retributive strike." },

{ "Staff of the Python", BOOK_DMG, "Staff", "uncommon",
  "requires attunement by a cleric, druid or warlock",
  "As an action you can throw it to the ground within 10 feet, where it "
  "becomes a giant constrictor snake under your control. As a bonus "
  "action you can turn it back into a staff; if the snake is reduced to 0 "
  "hit points, the staff is destroyed." },

{ "Staff of the Woodlands", BOOK_DMG, "Staff", "rare",
  "requires attunement by a druid",
  "A quarterstaff granting +2 to attack and damage rolls and +2 to spell "
  "attack rolls. Ten charges, regaining 1d6+4 daily at dawn: animal "
  "friendship, awaken, barkskin, locate animals or plants, speak with "
  "animals, speak with plants, wall of thorns. You can also plant it to "
  "become an awakened tree." },

{ "Staff of Thunder and Lightning", BOOK_DMG, "Staff", "very rare",
  "requires attunement",
  "A quarterstaff granting +2 to attack and damage rolls, plus five "
  "properties each usable once per dawn: extra 2d6 lightning damage on a "
  "hit, a thunderclap for 2d6 thunder damage, a lightning bolt for 9d6, a "
  "thunderclap for 2d6 and stunning, or both at once." },

{ "Staff of Withering", BOOK_DMG, "Staff", "rare",
  "requires attunement by a cleric, druid or warlock",
  "A quarterstaff with three charges, regaining 1d3 daily at dawn. On a "
  "hit you can spend a charge to deal an extra 2d10 necrotic damage; the "
  "target must succeed on a DC 15 Constitution save or have disadvantage "
  "on ability checks and saving throws using Strength or Constitution for "
  "1 hour." },

{ "Stone of Controlling Earth Elementals", BOOK_DMG, "Wondrous item",
  "rare", NULL,
  "While the stone touches the ground you can use an action to speak the "
  "command word and summon an earth elemental as though you had cast "
  "conjure elemental. Once used, it cannot be used again until the next "
  "dawn. It weighs 5 lb." },

{ "Stone of Good Luck (Luckstone)", BOOK_DMG, "Wondrous item",
  "uncommon", "requires attunement",
  "You gain +1 to ability checks and saving throws while it is on your "
  "person." },

{ "Sun Blade", BOOK_DMG, "Weapon (longsword)", "rare",
  "requires attunement",
  "A hilt that makes a blade of pure radiance on the command word, "
  "finesse, dealing radiant damage. You gain +2 to attack and damage "
  "rolls, and an extra 1d8 against undead. It sheds bright light in a "
  "15-foot radius and dim light for another 15, counting as sunlight. You "
  "are proficient with it if you have longsword proficiency." },

{ "Sword of Life Stealing", BOOK_DMG, "Weapon (any sword)", "rare",
  "requires attunement",
  "When you roll a 20 on an attack roll with it, the target takes an "
  "extra 3d6 necrotic damage if it is not a construct or undead, and you "
  "gain temporary hit points equal to that extra damage." },

{ "Sword of Sharpness", BOOK_DMG,
  "Weapon (any sword that deals slashing damage)", "very rare",
  "requires attunement",
  "On a hit you can speak the command word to deal an extra 4d6 slashing "
  "damage. A roll of 20 severs one of the target's limbs or appendages, "
  "if it has any. The sword also sheds bright light in a 10-foot radius "
  "on command." },

{ "Sword of Wounding", BOOK_DMG, "Weapon (any sword)", "rare",
  "requires attunement",
  "A creature hit by it takes 1d4 necrotic damage at the start of each of "
  "its turns, and can only end this by succeeding on a DC 15 Constitution "
  "save at the end of its turn. Hit points lost this way can be regained "
  "only by a short or long rest." },

{ "Talisman of Pure Good", BOOK_DMG, "Wondrous item", "legendary",
  "requires attunement by a creature of good alignment",
  "Seven charges. As an action you can target an evil creature within 120 "
  "feet: it must succeed on a DC 20 Dexterity save or be destroyed and "
  "cast into a fiery crevasse. A good cleric or paladin using it as a "
  "holy symbol gains +2 to spell attack rolls. It crumbles when the last "
  "charge is spent." },

{ "Talisman of the Sphere", BOOK_DMG, "Wondrous item", "legendary",
  "requires attunement",
  "While you hold it, you have advantage on Intelligence (Arcana) checks "
  "made to control a sphere of annihilation, and you double the distance "
  "the sphere moves on a successful check." },

{ "Talisman of Ultimate Evil", BOOK_DMG, "Wondrous item", "legendary",
  "requires attunement by a creature of evil alignment",
  "Six charges. As an action you can target a good creature within 120 "
  "feet: it must succeed on a DC 20 Dexterity save or be destroyed and "
  "cast into a fiery crevasse. An evil cleric or paladin using it as a "
  "holy symbol gains +2 to spell attack rolls. It crumbles when the last "
  "charge is spent." },

{ "Tome of Clear Thought", BOOK_DMG, "Wondrous item", "very rare",
  NULL,
  "Reading it over 48 hours across at least six days raises your "
  "Intelligence score and your Intelligence maximum by 2. The tome then "
  "loses its magic for a century." },

{ "Tome of Leadership and Influence", BOOK_DMG, "Wondrous item",
  "very rare", NULL,
  "Reading it over 48 hours across at least six days raises your Charisma "
  "score and your Charisma maximum by 2. The tome then loses its magic "
  "for a century." },

{ "Tome of Understanding", BOOK_DMG, "Wondrous item", "very rare",
  NULL,
  "Reading it over 48 hours across at least six days raises your Wisdom "
  "score and your Wisdom maximum by 2. The tome then loses its magic for "
  "a century." },

{ "Trident of Fish Command", BOOK_DMG, "Weapon (trident)", "uncommon",
  "requires attunement",
  "Three charges, regaining 1d3 daily at dawn. You can spend a charge to "
  "cast dominate beast on a beast with an innate swimming speed, with a "
  "save DC of 15." },

{ "Universal Solvent", BOOK_DMG, "Wondrous item", "legendary", NULL,
  "A tube of clear slime. As an action you can apply it to a 1-foot "
  "square of adhesive, dissolving it instantly -- including sovereign "
  "glue." },

{ "Vicious Weapon", BOOK_DMG, "Weapon (any)", "rare", NULL,
  "When you roll a 20 on an attack roll with this weapon, the target "
  "takes an extra 7 damage of the weapon's type." },

{ "Vorpal Sword", BOOK_DMG,
  "Weapon (any sword that deals slashing damage)", "legendary",
  "requires attunement",
  "+3 to attack and damage rolls, and it ignores resistance to slashing "
  "damage. When you roll a 20 on an attack against a creature with at "
  "least one head, you cut one head off, killing it if it needs the head "
  "to live and has no heads left." },

{ "Wand of Binding", BOOK_DMG, "Wand", "rare",
  "requires attunement by a spellcaster",
  "Seven charges, regaining 1d6+1 daily at dawn. You can spend charges to "
  "cast hold monster (5 charges) or hold person (2), or spend 5 charges "
  "as a reaction to gain advantage on a save against paralysis or "
  "restraint. Spending the last charge risks the wand crumbling to "
  "ashes." },

{ "Wand of Enemy Detection", BOOK_DMG, "Wand", "rare",
  "requires attunement",
  "Seven charges, regaining 1d6+1 daily at dawn. As an action you can "
  "spend a charge to learn, for 1 minute, the direction of the nearest "
  "creature hostile to you within 60 feet, even if it is hidden, "
  "invisible or disguised. Spending the last charge risks the wand "
  "crumbling to ashes." },

{ "Wand of Fear", BOOK_DMG, "Wand", "rare", "requires attunement",
  "Seven charges, regaining 1d6+1 daily at dawn. One charge commands a "
  "creature within 60 feet to flee or grovel on a failed DC 15 Wisdom "
  "save; three charges frighten each creature in a 60-foot cone. "
  "Spending the last charge risks the wand crumbling to ashes." },

{ "Wand of Fireballs", BOOK_DMG, "Wand", "rare",
  "requires attunement by a spellcaster",
  "Seven charges, regaining 1d6+1 daily at dawn. You can spend one or "
  "more charges to cast fireball at 3rd level, adding a level for each "
  "charge beyond the first. Spending the last charge risks the wand "
  "crumbling to ashes." },

{ "Wand of Lightning Bolts", BOOK_DMG, "Wand", "rare",
  "requires attunement by a spellcaster",
  "Seven charges, regaining 1d6+1 daily at dawn. You can spend one or "
  "more charges to cast lightning bolt at 3rd level, adding a level for "
  "each charge beyond the first. Spending the last charge risks the wand "
  "crumbling to ashes." },

{ "Wand of Magic Detection", BOOK_DMG, "Wand", "uncommon", NULL,
  "Three charges. While holding it you can spend a charge to cast detect "
  "magic. It regains 1d3 charges daily at dawn." },

{ "Wand of Magic Missiles", BOOK_DMG, "Wand", "uncommon", NULL,
  "Seven charges, regaining 1d6+1 daily at dawn. You can spend one or "
  "more charges to cast magic missile at 1st level, adding a level for "
  "each charge beyond the first. Spending the last charge risks the wand "
  "crumbling to ashes." },

{ "Wand of Paralysis", BOOK_DMG, "Wand", "rare",
  "requires attunement by a spellcaster",
  "Seven charges, regaining 1d6+1 daily at dawn. As an action you can "
  "spend a charge to fire a thin blue ray at a creature within 60 feet, "
  "which must succeed on a DC 15 Constitution save or be paralyzed for 1 "
  "minute, repeating the save at the end of each of its turns." },

{ "Wand of Polymorph", BOOK_DMG, "Wand", "very rare",
  "requires attunement by a spellcaster",
  "Seven charges, regaining 1d6+1 daily at dawn. You can spend a charge "
  "to cast polymorph. Spending the last charge risks the wand crumbling "
  "to ashes." },

{ "Wand of Secrets", BOOK_DMG, "Wand", "uncommon", NULL,
  "Three charges, regaining 1d3 daily at dawn. As an action you can spend "
  "a charge; if a secret door or trap is within 30 feet, the wand pulses "
  "and points to the nearest one." },

{ "Wand of the War Mage, +1, +2, or +3", BOOK_DMG, "Wand",
  "uncommon (+1), rare (+2), very rare (+3)",
  "requires attunement by a spellcaster",
  "You gain a bonus to spell attack rolls, set by the wand's rarity, and "
  "you ignore half cover when making a spell attack." },

{ "Wand of Web", BOOK_DMG, "Wand", "uncommon",
  "requires attunement by a spellcaster",
  "Seven charges, regaining 1d6+1 daily at dawn. You can spend a charge "
  "to cast web with a save DC of 15. Spending the last charge risks the "
  "wand crumbling to ashes." },

{ "Wand of Wonder", BOOK_DMG, "Wand", "rare",
  "requires attunement by a spellcaster",
  "Seven charges, regaining 1d6+1 daily at dawn. Spending a charge aims "
  "it at a target and rolls on a table of a hundred results, from casting "
  "slow or fireball to summoning a rhinoceros, turning the target blue, "
  "or making it rain butterflies. Spending the last charge risks the wand "
  "crumbling to ashes." },

{ "Weapon, +1, +2, or +3", BOOK_DMG, "Weapon (any)",
  "uncommon (+1), rare (+2), very rare (+3)", NULL,
  "You have a bonus to attack and damage rolls made with this magic "
  "weapon, set by its rarity." },

{ "Weapon of Warning", BOOK_DMG, "Weapon (any)", "uncommon",
  "requires attunement",
  "You and your companions within 30 feet gain advantage on initiative "
  "rolls while you carry it. You and any of them cannot be surprised "
  "while sleeping, unless incapacitated by something other than normal "
  "sleep, and the weapon wakes you when it senses danger." },

{ "Well of Many Worlds", BOOK_DMG, "Wondrous item", "legendary", NULL,
  "A circle of black cloth that opens a two-way portal to a random plane "
  "when spread out; folding it closes the portal. It can be opened no "
  "more than once per dawn." },

{ "Wind Fan", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "You can use an action to cast gust of wind from it, with a save DC of "
  "13. Each use after the first carries a cumulative 20 percent chance "
  "that the fan tears into useless, nonmagical pieces; the chance resets "
  "at dawn." },

{ "Winged Boots", BOOK_DMG, "Wondrous item", "uncommon",
  "requires attunement",
  "You gain a flying speed equal to your walking speed and can hover. "
  "They hold 4 hours of flight, spent in increments of 1 minute, and "
  "regain 2 hours for every 12 hours they are not used. If you are aloft "
  "when the time runs out, you descend 30 feet per round until you land." },

{ "Wings of Flying", BOOK_DMG, "Wondrous item", "rare",
  "requires attunement",
  "As an action you can speak the command word to turn the cloak into a "
  "pair of bat or bird wings, giving you a flying speed of 60 feet for up "
  "to 1 hour. They recover 1 hour of flight for every 12 hours they are "
  "not used." },

{ "Elixir of Health", BOOK_DMG, "Potion", "rare", NULL,
  "Drinking it cures any disease afflicting you and removes the blinded, "
  "deafened, paralyzed and poisoned conditions." },

{ "Instrument of the Bards", BOOK_DMG, "Wondrous item",
  "uncommon to very rare", "requires attunement by a bard",
  "A masterwork instrument that grants +2 to the saving throw DCs of your "
  "bard spells while you play it, and lets you cast a fixed list of spells "
  "from it, each once per dawn. The seven kinds, in ascending power, are "
  "the Doss lute, Fochlucan bandore, Mac-Fuirmidh cittern, Cli lyre, "
  "Anstruth harp, Ollamh harp and Canaith mandolin. A creature not a bard "
  "that plays one takes 2d4 psychic damage." },

{ "Saddle of the Cavalier", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "While in this saddle on a mount, you cannot be dismounted against your "
  "will if you remain conscious, and attack rolls against the mount have "
  "disadvantage while you are mounted on it." },

{ "Scroll of Protection", BOOK_DMG, "Scroll", "rare", NULL,
  "Reading it as an action for 1 minute protects you against one type of "
  "creature -- aberrations, beasts, celestials, elementals, fey, fiends "
  "or undead. For 5 minutes, creatures of that type cannot willingly come "
  "within 5 feet of you, and have disadvantage on attack rolls against "
  "you, unless they succeed on a DC 15 Charisma save; even then they "
  "cannot charm, frighten or possess you." },

{ "Sending Stones", BOOK_DMG, "Wondrous item", "uncommon", NULL,
  "A matched pair. While touching one you can use an action to cast "
  "sending from it, with the other stone as the target, wherever it is on "
  "the same plane. Once used, the pair cannot be used again until the "
  "next dawn." },

{ "Sword of Answering", BOOK_DMG, "Weapon (longsword)", "legendary",
  "requires attunement by a creature of the same alignment as the sword",
  "+3 to attack and damage rolls. Whenever a creature within 5 feet of "
  "you hits you with an attack, you can use your reaction to make one "
  "attack against that creature with this sword. There are nine such "
  "swords, one for each alignment." },

{ "Sword of Vengeance", BOOK_DMG, "Weapon (any sword)", "uncommon",
  "requires attunement",
  "+1 to attack and damage rolls. Cursed: while attuned you have "
  "disadvantage on attack rolls made with any other weapon, and whenever "
  "a hostile creature damages you while the sword is on your person you "
  "must succeed on a DC 15 Wisdom save or attack that creature until "
  "either of you dies." },

{ "Tentacle Rod", BOOK_DMG, "Rod", "rare", "requires attunement",
  "As an action you can make three tentacle attacks against a creature "
  "within 15 feet, each at +9 to hit for 1d6 bludgeoning damage. If all "
  "three hit, the target has its speed halved, has disadvantage on "
  "Dexterity saves and cannot use reactions for 1 minute, and can repeat "
  "a DC 15 Constitution save at the end of each of its turns. A creature "
  "made by a drow." },

{ "Tome of the Stilled Tongue", BOOK_DMG, "Wondrous item", "legendary",
  "requires attunement by a spellcaster",
  "Vecna's own spellbook, with a severed tongue in its front cover. It "
  "acts as a spellbook, and any spell you write in it costs half the "
  "usual gold and time. It holds five charges, regaining 1d4+1 daily at "
  "dawn, which can be spent to cast a spell inscribed on its pages "
  "without expending a slot." },

/* ------------------------------------------------------------- artifacts */

{ "Blackrazor", BOOK_DMG, "Weapon (greatsword)", "legendary",
  "requires attunement",
  "A sentient blade that devours souls. +3 to attack and damage rolls. "
  "When you kill a creature with it that is not a construct or undead, "
  "the sword absorbs its soul and you gain temporary hit points equal to "
  "the creature's hit point maximum, which fade after 24 hours; while you "
  "have them you gain advantage on all attacks, saves and checks. It also "
  "grants immunity to being charmed or frightened, and hungers to feed." },

{ "Book of Exalted Deeds", BOOK_DMG, "Wondrous item", "artifact",
  "requires attunement by a creature of good alignment",
  "Studying it for 80 hours over at least 10 days raises your Wisdom by 2 "
  "and your Wisdom maximum by 2, and grants a bless-like aura and other "
  "boons of good. Reading it as an evil creature deals 24d6 radiant "
  "damage." },

{ "Book of Vile Darkness", BOOK_DMG, "Wondrous item", "artifact",
  "requires attunement",
  "Studying it for 80 hours over at least 10 days grants a permanent "
  "boon: an ability score increase, a lich transformation, or a dark "
  "gift, along with a curse. Reading it as a good creature deals 24d6 "
  "psychic damage." },

{ "Eye and Hand of Vecna", BOOK_DMG, "Wondrous item", "artifact",
  "requires attunement",
  "Grafted onto your own body in place of your eye or hand. The Eye "
  "grants truesight to 120 feet and spells including clairvoyance and "
  "true seeing; the Hand grants a chilling touch for 5d8 cold damage, a "
  "Strength of 20, and spells up to finger of death. Wearing both grants "
  "further powers, and both carry Vecna's curse." },

{ "Orb of Dragonkind", BOOK_DMG, "Wondrous item", "artifact",
  "requires attunement",
  "One of five orbs made to control dragons. It grants AC 16, resistance "
  "to one damage type, +1 to spell attack rolls, and the ability to speak "
  "with and dominate dragons within 1 mile. It has charges for spells "
  "including cure wounds, daylight, death ward and detect magic, and "
  "carries a curse toward evil." },

{ "Sword of Kas", BOOK_DMG, "Weapon (longsword)", "artifact",
  "requires attunement",
  "The blade Vecna's lieutenant turned against him. +3 to attack and "
  "damage rolls, an extra 2d10 slashing damage against undead, and a "
  "roll of 20 forces a DC 15 Constitution save or the target takes an "
  "extra 4d12 necrotic damage and dies at 0 hit points. It is sentient "
  "and drives its wielder to destruction." },

{ "Wand of Orcus", BOOK_DMG, "Wand", "artifact",
  "requires attunement",
  "The skull-topped mace of the demon prince of undeath. A magic mace "
  "granting +3 to attack and damage rolls, an extra 2d12 necrotic damage "
  "on a hit, +3 to AC and saving throws, and immunity to necrotic and "
  "poison damage. It can animate the dead and summon undead, and it is "
  "cursed to draw Orcus's attention." },

{ "Wave", BOOK_DMG, "Weapon (trident)", "legendary",
  "requires attunement by a creature that worships a god of the sea",
  "A sentient trident. +3 to attack and damage rolls, it returns to your "
  "hand when thrown, and it grants you the ability to breathe underwater "
  "and a swimming speed equal to your walking speed. A hit forces a DC 15 "
  "Constitution save or the target takes an extra 3d6 necrotic damage and "
  "its hit point maximum is reduced by the same amount; you regain that "
  "many hit points." },

{ "Whelm", BOOK_DMG, "Weapon (maul)", "legendary",
  "requires attunement by a dwarf",
  "A sentient maul. +3 to attack and damage rolls, an extra 1d8 damage "
  "against giants, and it can be thrown 20 feet, returning to your hand. "
  "It grants tremorsense to 30 feet and detect evil and good, and a "
  "shock wave on a hit that stuns a creature on a failed DC 15 "
  "Constitution save. It makes its dwarven wielder reluctant to leave "
  "underground." },

};

const int MAGIC_ITEM_COUNT =
    (int)(sizeof(MAGIC_ITEMS) / sizeof(MAGIC_ITEMS[0]));

int find_magic_item(const char *name)
{
    int i;
    for (i = 0; i < MAGIC_ITEM_COUNT; i++)
        if (strcmp(MAGIC_ITEMS[i].name, name) == 0) return i;
    return -1;
}
