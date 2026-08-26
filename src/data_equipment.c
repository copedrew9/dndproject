/* data_equipment.c -- PHB chapter 5 armour, weapons, gear, tools and packs.
 *
 * Costs are stored in copper pieces (1 gp = 100 cp, 1 ep = 50 cp,
 * 1 sp = 10 cp) and weights in tenths of a pound, so the whole table is
 * integer-exact.
 */
#include "data.h"
#include <string.h>

#define GP(n)  ((n) * 100)
#define SP(n)  ((n) * 10)
#define CP(n)  (n)
#define LB(n)  ((n) * 10)

/* base_ac / dex_cap / str_req / stealth are meaningful for armour only;
 * dex_cap -1 means the full Dexterity modifier applies, 0 means none. */

const ItemData ITEMS[] = {
/* ------------------------------- light armour ------------------------------ */
{ "Padded armor",        ITEM_LIGHT_ARMOR,  GP(5),   LB(8),  11, -1, 0, 1, "","","","" },
{ "Leather armor",       ITEM_LIGHT_ARMOR,  GP(10),  LB(10), 11, -1, 0, 0, "","","","" },
{ "Studded leather armor",ITEM_LIGHT_ARMOR, GP(45),  LB(13), 12, -1, 0, 0, "","","","" },

/* ------------------------------ medium armour ------------------------------ */
{ "Hide armor",          ITEM_MEDIUM_ARMOR, GP(10),  LB(12), 12,  2, 0, 0, "","","","" },
{ "Chain shirt",         ITEM_MEDIUM_ARMOR, GP(50),  LB(20), 13,  2, 0, 0, "","","","" },
{ "Scale mail",          ITEM_MEDIUM_ARMOR, GP(50),  LB(45), 14,  2, 0, 1, "","","","" },
{ "Breastplate",         ITEM_MEDIUM_ARMOR, GP(400), LB(20), 14,  2, 0, 0, "","","","" },
{ "Half plate",          ITEM_MEDIUM_ARMOR, GP(750), LB(40), 15,  2, 0, 1, "","","","" },

/* ------------------------------- heavy armour ------------------------------ */
{ "Ring mail",           ITEM_HEAVY_ARMOR,  GP(30),  LB(40), 14,  0,  0, 1, "","","","" },
{ "Chain mail",          ITEM_HEAVY_ARMOR,  GP(75),  LB(55), 16,  0, 13, 1, "","","","" },
{ "Splint armor",        ITEM_HEAVY_ARMOR,  GP(200), LB(60), 17,  0, 15, 1, "","","","" },
{ "Plate armor",         ITEM_HEAVY_ARMOR,  GP(1500),LB(65), 18,  0, 15, 1, "","","","" },

/* ---------------------------------- shield --------------------------------- */
{ "Shield",              ITEM_SHIELD,       GP(10),  LB(6),   2,  0,  0, 0, "","","","" },

/* ------------------------------ simple melee ------------------------------- */
{ "Club",          ITEM_SIMPLE_MELEE, SP(1),  LB(2),  0,0,0,0, "1d4","bludgeoning","Light","" },
{ "Dagger",        ITEM_SIMPLE_MELEE, GP(2),  LB(1),  0,0,0,0, "1d4","piercing","Finesse, light, thrown (range 20/60)","" },
{ "Greatclub",     ITEM_SIMPLE_MELEE, SP(2),  LB(10), 0,0,0,0, "1d8","bludgeoning","Two-handed","" },
{ "Handaxe",       ITEM_SIMPLE_MELEE, GP(5),  LB(2),  0,0,0,0, "1d6","slashing","Light, thrown (range 20/60)","" },
{ "Javelin",       ITEM_SIMPLE_MELEE, SP(5),  LB(2),  0,0,0,0, "1d6","piercing","Thrown (range 30/120)","" },
{ "Light hammer",  ITEM_SIMPLE_MELEE, GP(2),  LB(2),  0,0,0,0, "1d4","bludgeoning","Light, thrown (range 20/60)","" },
{ "Mace",          ITEM_SIMPLE_MELEE, GP(5),  LB(4),  0,0,0,0, "1d6","bludgeoning","","" },
{ "Quarterstaff",  ITEM_SIMPLE_MELEE, SP(2),  LB(4),  0,0,0,0, "1d6","bludgeoning","Versatile (1d8)","" },
{ "Sickle",        ITEM_SIMPLE_MELEE, GP(1),  LB(2),  0,0,0,0, "1d4","slashing","Light","" },
{ "Spear",         ITEM_SIMPLE_MELEE, GP(1),  LB(3),  0,0,0,0, "1d6","piercing","Thrown (range 20/60), versatile (1d8)","" },

/* ------------------------------ simple ranged ------------------------------ */
{ "Light crossbow",ITEM_SIMPLE_RANGED, GP(25), LB(5),  0,0,0,0, "1d8","piercing","Ammunition (range 80/320), loading, two-handed","" },
{ "Dart",          ITEM_SIMPLE_RANGED, CP(5),  2,      0,0,0,0, "1d4","piercing","Finesse, thrown (range 20/60)","" },
{ "Shortbow",      ITEM_SIMPLE_RANGED, GP(25), LB(2),  0,0,0,0, "1d6","piercing","Ammunition (range 80/320), two-handed","" },
{ "Sling",         ITEM_SIMPLE_RANGED, SP(1),  0,      0,0,0,0, "1d4","bludgeoning","Ammunition (range 30/120)","" },

/* ------------------------------ martial melee ------------------------------ */
{ "Battleaxe",     ITEM_MARTIAL_MELEE, GP(10), LB(4),  0,0,0,0, "1d8","slashing","Versatile (1d10)","" },
{ "Flail",         ITEM_MARTIAL_MELEE, GP(10), LB(2),  0,0,0,0, "1d8","bludgeoning","","" },
{ "Glaive",        ITEM_MARTIAL_MELEE, GP(20), LB(6),  0,0,0,0, "1d10","slashing","Heavy, reach, two-handed","" },
{ "Greataxe",      ITEM_MARTIAL_MELEE, GP(30), LB(7),  0,0,0,0, "1d12","slashing","Heavy, two-handed","" },
{ "Greatsword",    ITEM_MARTIAL_MELEE, GP(50), LB(6),  0,0,0,0, "2d6","slashing","Heavy, two-handed","" },
{ "Halberd",       ITEM_MARTIAL_MELEE, GP(20), LB(6),  0,0,0,0, "1d10","slashing","Heavy, reach, two-handed","" },
{ "Lance",         ITEM_MARTIAL_MELEE, GP(10), LB(6),  0,0,0,0, "1d12","piercing","Reach, special","" },
{ "Longsword",     ITEM_MARTIAL_MELEE, GP(15), LB(3),  0,0,0,0, "1d8","slashing","Versatile (1d10)","" },
{ "Maul",          ITEM_MARTIAL_MELEE, GP(10), LB(10), 0,0,0,0, "2d6","bludgeoning","Heavy, two-handed","" },
{ "Morningstar",   ITEM_MARTIAL_MELEE, GP(15), LB(4),  0,0,0,0, "1d8","piercing","","" },
{ "Pike",          ITEM_MARTIAL_MELEE, GP(5),  LB(18), 0,0,0,0, "1d10","piercing","Heavy, reach, two-handed","" },
{ "Rapier",        ITEM_MARTIAL_MELEE, GP(25), LB(2),  0,0,0,0, "1d8","piercing","Finesse","" },
{ "Scimitar",      ITEM_MARTIAL_MELEE, GP(25), LB(3),  0,0,0,0, "1d6","slashing","Finesse, light","" },
{ "Shortsword",    ITEM_MARTIAL_MELEE, GP(10), LB(2),  0,0,0,0, "1d6","piercing","Finesse, light","" },
{ "Trident",       ITEM_MARTIAL_MELEE, GP(5),  LB(4),  0,0,0,0, "1d6","piercing","Thrown (range 20/60), versatile (1d8)","" },
{ "War pick",      ITEM_MARTIAL_MELEE, GP(5),  LB(2),  0,0,0,0, "1d8","piercing","","" },
{ "Warhammer",     ITEM_MARTIAL_MELEE, GP(15), LB(2),  0,0,0,0, "1d8","bludgeoning","Versatile (1d10)","" },
{ "Whip",          ITEM_MARTIAL_MELEE, GP(2),  LB(3),  0,0,0,0, "1d4","slashing","Finesse, reach","" },

/* ----------------------------- martial ranged ------------------------------ */
{ "Blowgun",       ITEM_MARTIAL_RANGED, GP(10), LB(1),  0,0,0,0, "1","piercing","Ammunition (range 25/100), loading","" },
{ "Hand crossbow", ITEM_MARTIAL_RANGED, GP(75), LB(3),  0,0,0,0, "1d6","piercing","Ammunition (range 30/120), light, loading","" },
{ "Heavy crossbow",ITEM_MARTIAL_RANGED, GP(50), LB(18), 0,0,0,0, "1d10","piercing","Ammunition (range 100/400), heavy, loading, two-handed","" },
{ "Longbow",       ITEM_MARTIAL_RANGED, GP(50), LB(2),  0,0,0,0, "1d8","piercing","Ammunition (range 150/600), heavy, two-handed","" },
{ "Net",           ITEM_MARTIAL_RANGED, GP(1),  LB(3),  0,0,0,0, "-","-","Special, thrown (range 5/15)","" },

/* ---------------------------------- packs ---------------------------------- */
{ "Burglar's pack", ITEM_PACK, GP(16), LB(47), 0,0,0,0, "","","",
  "Backpack, bag of 1000 ball bearings, 10 feet of string, bell, 5 candles, "
  "crowbar, hammer, 10 pitons, hooded lantern, 2 flasks of oil, 5 days "
  "rations, tinderbox, waterskin, 50 feet of hempen rope" },
{ "Diplomat's pack", ITEM_PACK, GP(39), LB(39), 0,0,0,0, "","","",
  "Chest, 2 cases for maps and scrolls, fine clothes, bottle of ink, ink "
  "pen, lamp, 2 flasks of oil, 5 sheets of paper, vial of perfume, sealing "
  "wax, soap" },
{ "Dungeoneer's pack", ITEM_PACK, GP(12), LB(61), 0,0,0,0, "","","",
  "Backpack, crowbar, hammer, 10 pitons, 10 torches, tinderbox, 10 days "
  "rations, waterskin, 50 feet of hempen rope" },
{ "Entertainer's pack", ITEM_PACK, GP(40), LB(38), 0,0,0,0, "","","",
  "Backpack, bedroll, 2 costumes, 5 candles, 5 days rations, waterskin, "
  "disguise kit" },
{ "Explorer's pack", ITEM_PACK, GP(10), LB(59), 0,0,0,0, "","","",
  "Backpack, bedroll, mess kit, tinderbox, 10 torches, 10 days rations, "
  "waterskin, 50 feet of hempen rope" },
{ "Priest's pack", ITEM_PACK, GP(19), LB(24), 0,0,0,0, "","","",
  "Backpack, blanket, 10 candles, tinderbox, alms box, 2 blocks of incense, "
  "censer, vestments, 2 days rations, waterskin" },
{ "Scholar's pack", ITEM_PACK, GP(40), LB(10), 0,0,0,0, "","","",
  "Backpack, book of lore, bottle of ink, ink pen, 10 sheets of parchment, "
  "little bag of sand, small knife" },

/* ------------------------------ adventuring gear --------------------------- */
{ "Abacus",              ITEM_GEAR, GP(2),  LB(2),  0,0,0,0,"","","","" },
{ "Acid (vial)",         ITEM_GEAR, GP(25), LB(1),  0,0,0,0,"","","","" },
{ "Alchemist's fire (flask)", ITEM_GEAR, GP(50), LB(1), 0,0,0,0,"","","","" },
{ "Arrows (20)",         ITEM_GEAR, GP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Blowgun needles (50)",ITEM_GEAR, GP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Crossbow bolts (20)", ITEM_GEAR, GP(1),  LB(15), 0,0,0,0,"","","","" },
{ "Sling bullets (20)",  ITEM_GEAR, CP(4),  LB(15), 0,0,0,0,"","","","" },
{ "Antitoxin (vial)",    ITEM_GEAR, GP(50), 0,      0,0,0,0,"","","","" },
{ "Backpack",            ITEM_GEAR, GP(2),  LB(5),  0,0,0,0,"","","","" },
{ "Ball bearings (bag of 1,000)", ITEM_GEAR, GP(1), LB(2), 0,0,0,0,"","","","" },
{ "Barrel",              ITEM_GEAR, GP(2),  LB(70), 0,0,0,0,"","","","" },
{ "Basket",              ITEM_GEAR, SP(4),  LB(2),  0,0,0,0,"","","","" },
{ "Bedroll",             ITEM_GEAR, GP(1),  LB(7),  0,0,0,0,"","","","" },
{ "Bell",                ITEM_GEAR, GP(1),  0,      0,0,0,0,"","","","" },
{ "Blanket",             ITEM_GEAR, SP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Block and tackle",    ITEM_GEAR, GP(1),  LB(5),  0,0,0,0,"","","","" },
{ "Book",                ITEM_GEAR, GP(25), LB(5),  0,0,0,0,"","","","" },
{ "Bucket",              ITEM_GEAR, CP(5),  LB(2),  0,0,0,0,"","","","" },
{ "Caltrops (bag of 20)",ITEM_GEAR, GP(1),  LB(2),  0,0,0,0,"","","","" },
{ "Candle",              ITEM_GEAR, CP(1),  0,      0,0,0,0,"","","","" },
{ "Chain (10 feet)",     ITEM_GEAR, GP(5),  LB(10), 0,0,0,0,"","","","" },
{ "Chalk (1 piece)",     ITEM_GEAR, CP(1),  0,      0,0,0,0,"","","","" },
{ "Chest",               ITEM_GEAR, GP(5),  LB(25), 0,0,0,0,"","","","" },
{ "Climber's kit",       ITEM_GEAR, GP(25), LB(12), 0,0,0,0,"","","","" },
{ "Clothes, common",     ITEM_GEAR, SP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Clothes, costume",    ITEM_GEAR, GP(5),  LB(4),  0,0,0,0,"","","","" },
{ "Clothes, fine",       ITEM_GEAR, GP(15), LB(6),  0,0,0,0,"","","","" },
{ "Clothes, traveler's", ITEM_GEAR, GP(2),  LB(4),  0,0,0,0,"","","","" },
{ "Component pouch",     ITEM_GEAR, GP(25), LB(2),  0,0,0,0,"","","","" },
{ "Crowbar",             ITEM_GEAR, GP(2),  LB(5),  0,0,0,0,"","","","" },
{ "Crystal (arcane focus)", ITEM_GEAR, GP(10), LB(1), 0,0,0,0,"","","","" },
{ "Orb (arcane focus)",  ITEM_GEAR, GP(20), LB(30), 0,0,0,0,"","","","" },
{ "Rod (arcane focus)",  ITEM_GEAR, GP(10), LB(2),  0,0,0,0,"","","","" },
{ "Staff (arcane focus)",ITEM_GEAR, GP(5),  LB(4),  0,0,0,0,"","","","" },
{ "Wand (arcane focus)", ITEM_GEAR, GP(10), LB(1),  0,0,0,0,"","","","" },
{ "Sprig of mistletoe (druidic focus)", ITEM_GEAR, GP(1), 0, 0,0,0,0,"","","","" },
{ "Wooden staff (druidic focus)", ITEM_GEAR, GP(5), LB(4), 0,0,0,0,"","","","" },
{ "Yew wand (druidic focus)", ITEM_GEAR, GP(10), LB(1), 0,0,0,0,"","","","" },
{ "Amulet (holy symbol)",ITEM_GEAR, GP(5),  LB(1),  0,0,0,0,"","","","" },
{ "Emblem (holy symbol)",ITEM_GEAR, GP(5),  0,      0,0,0,0,"","","","" },
{ "Reliquary (holy symbol)", ITEM_GEAR, GP(5), LB(2), 0,0,0,0,"","","","" },
{ "Fishing tackle",      ITEM_GEAR, GP(1),  LB(4),  0,0,0,0,"","","","" },
{ "Flask or tankard",    ITEM_GEAR, CP(2),  LB(1),  0,0,0,0,"","","","" },
{ "Grappling hook",      ITEM_GEAR, GP(2),  LB(4),  0,0,0,0,"","","","" },
{ "Hammer",              ITEM_GEAR, GP(1),  LB(3),  0,0,0,0,"","","","" },
{ "Hammer, sledge",      ITEM_GEAR, GP(2),  LB(10), 0,0,0,0,"","","","" },
{ "Healer's kit",        ITEM_GEAR, GP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Holy water (flask)",  ITEM_GEAR, GP(25), LB(1),  0,0,0,0,"","","","" },
{ "Hourglass",           ITEM_GEAR, GP(25), LB(1),  0,0,0,0,"","","","" },
{ "Hunting trap",        ITEM_GEAR, GP(5),  LB(25), 0,0,0,0,"","","","" },
{ "Ink (1 ounce bottle)",ITEM_GEAR, GP(10), 0,      0,0,0,0,"","","","" },
{ "Ink pen",             ITEM_GEAR, CP(2),  0,      0,0,0,0,"","","","" },
{ "Jug or pitcher",      ITEM_GEAR, CP(2),  LB(4),  0,0,0,0,"","","","" },
{ "Ladder (10-foot)",    ITEM_GEAR, SP(1),  LB(25), 0,0,0,0,"","","","" },
{ "Lamp",                ITEM_GEAR, SP(5),  LB(1),  0,0,0,0,"","","","" },
{ "Lantern, bullseye",   ITEM_GEAR, GP(10), LB(2),  0,0,0,0,"","","","" },
{ "Lantern, hooded",     ITEM_GEAR, GP(5),  LB(2),  0,0,0,0,"","","","" },
{ "Lock",                ITEM_GEAR, GP(10), LB(1),  0,0,0,0,"","","","" },
{ "Magnifying glass",    ITEM_GEAR, GP(100),0,      0,0,0,0,"","","","" },
{ "Manacles",            ITEM_GEAR, GP(2),  LB(6),  0,0,0,0,"","","","" },
{ "Mess kit",            ITEM_GEAR, SP(2),  LB(1),  0,0,0,0,"","","","" },
{ "Mirror, steel",       ITEM_GEAR, GP(5),  LB(5),  0,0,0,0,"","","","" },
{ "Oil (flask)",         ITEM_GEAR, SP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Paper (one sheet)",   ITEM_GEAR, SP(2),  0,      0,0,0,0,"","","","" },
{ "Parchment (one sheet)",ITEM_GEAR,SP(1),  0,      0,0,0,0,"","","","" },
{ "Perfume (vial)",      ITEM_GEAR, GP(5),  0,      0,0,0,0,"","","","" },
{ "Pick, miner's",       ITEM_GEAR, GP(2),  LB(10), 0,0,0,0,"","","","" },
{ "Piton",               ITEM_GEAR, CP(5),  2,      0,0,0,0,"","","","" },
{ "Poison, basic (vial)",ITEM_GEAR, GP(100),0,      0,0,0,0,"","","","" },
{ "Pole (10-foot)",      ITEM_GEAR, CP(5),  LB(7),  0,0,0,0,"","","","" },
{ "Pot, iron",           ITEM_GEAR, GP(2),  LB(10), 0,0,0,0,"","","","" },
{ "Potion of healing",   ITEM_GEAR, GP(50), 5,      0,0,0,0,"","","","" },
{ "Pouch",               ITEM_GEAR, SP(5),  LB(1),  0,0,0,0,"","","","" },
{ "Quiver",              ITEM_GEAR, GP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Ram, portable",       ITEM_GEAR, GP(4),  LB(35), 0,0,0,0,"","","","" },
{ "Rations (1 day)",     ITEM_GEAR, SP(5),  LB(2),  0,0,0,0,"","","","" },
{ "Robes",               ITEM_GEAR, GP(1),  LB(4),  0,0,0,0,"","","","" },
{ "Rope, hempen (50 feet)", ITEM_GEAR, GP(1), LB(10), 0,0,0,0,"","","","" },
{ "Rope, silk (50 feet)",ITEM_GEAR, GP(10), LB(5),  0,0,0,0,"","","","" },
{ "Sack",                ITEM_GEAR, CP(1),  5,      0,0,0,0,"","","","" },
{ "Scale, merchant's",   ITEM_GEAR, GP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Sealing wax",         ITEM_GEAR, SP(5),  0,      0,0,0,0,"","","","" },
{ "Shovel",              ITEM_GEAR, GP(2),  LB(5),  0,0,0,0,"","","","" },
{ "Signal whistle",      ITEM_GEAR, CP(5),  0,      0,0,0,0,"","","","" },
{ "Signet ring",         ITEM_GEAR, GP(5),  0,      0,0,0,0,"","","","" },
{ "Soap",                ITEM_GEAR, CP(2),  0,      0,0,0,0,"","","","" },
{ "Spellbook",           ITEM_GEAR, GP(50), LB(3),  0,0,0,0,"","","","" },
{ "Spikes, iron (10)",   ITEM_GEAR, GP(1),  LB(5),  0,0,0,0,"","","","" },
{ "Spyglass",            ITEM_GEAR, GP(1000),LB(1), 0,0,0,0,"","","","" },
{ "Tent, two-person",    ITEM_GEAR, GP(2),  LB(20), 0,0,0,0,"","","","" },
{ "Tinderbox",           ITEM_GEAR, SP(5),  LB(1),  0,0,0,0,"","","","" },
{ "Torch",               ITEM_GEAR, CP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Vial",                ITEM_GEAR, GP(1),  0,      0,0,0,0,"","","","" },
{ "Waterskin",           ITEM_GEAR, SP(2),  LB(5),  0,0,0,0,"","","","" },
{ "Whetstone",           ITEM_GEAR, CP(1),  LB(1),  0,0,0,0,"","","","" },

/* ----------------------------------- tools --------------------------------- */
{ "Alchemist's supplies",ITEM_TOOL, GP(50), LB(8),  0,0,0,0,"","","","" },
{ "Brewer's supplies",   ITEM_TOOL, GP(20), LB(9),  0,0,0,0,"","","","" },
{ "Calligrapher's supplies", ITEM_TOOL, GP(10), LB(5), 0,0,0,0,"","","","" },
{ "Carpenter's tools",   ITEM_TOOL, GP(8),  LB(6),  0,0,0,0,"","","","" },
{ "Cartographer's tools",ITEM_TOOL, GP(15), LB(6),  0,0,0,0,"","","","" },
{ "Cobbler's tools",     ITEM_TOOL, GP(5),  LB(5),  0,0,0,0,"","","","" },
{ "Cook's utensils",     ITEM_TOOL, GP(1),  LB(8),  0,0,0,0,"","","","" },
{ "Glassblower's tools", ITEM_TOOL, GP(30), LB(5),  0,0,0,0,"","","","" },
{ "Jeweler's tools",     ITEM_TOOL, GP(25), LB(2),  0,0,0,0,"","","","" },
{ "Leatherworker's tools",ITEM_TOOL,GP(5),  LB(5),  0,0,0,0,"","","","" },
{ "Mason's tools",       ITEM_TOOL, GP(10), LB(8),  0,0,0,0,"","","","" },
{ "Painter's supplies",  ITEM_TOOL, GP(10), LB(5),  0,0,0,0,"","","","" },
{ "Potter's tools",      ITEM_TOOL, GP(10), LB(3),  0,0,0,0,"","","","" },
{ "Smith's tools",       ITEM_TOOL, GP(20), LB(8),  0,0,0,0,"","","","" },
{ "Tinker's tools",      ITEM_TOOL, GP(50), LB(10), 0,0,0,0,"","","","" },
{ "Weaver's tools",      ITEM_TOOL, GP(1),  LB(5),  0,0,0,0,"","","","" },
{ "Woodcarver's tools",  ITEM_TOOL, GP(1),  LB(5),  0,0,0,0,"","","","" },
{ "Disguise kit",        ITEM_TOOL, GP(25), LB(3),  0,0,0,0,"","","","" },
{ "Forgery kit",         ITEM_TOOL, GP(15), LB(5),  0,0,0,0,"","","","" },
{ "Herbalism kit",       ITEM_TOOL, GP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Navigator's tools",   ITEM_TOOL, GP(25), LB(2),  0,0,0,0,"","","","" },
{ "Poisoner's kit",      ITEM_TOOL, GP(50), LB(2),  0,0,0,0,"","","","" },
{ "Thieves' tools",      ITEM_TOOL, GP(25), LB(1),  0,0,0,0,"","","","" },
{ "Dice set",            ITEM_TOOL, SP(1),  0,      0,0,0,0,"","","","" },
{ "Playing card set",    ITEM_TOOL, SP(5),  0,      0,0,0,0,"","","","" },
{ "Bagpipes",            ITEM_TOOL, GP(30), LB(6),  0,0,0,0,"","","","" },
{ "Drum",                ITEM_TOOL, GP(6),  LB(3),  0,0,0,0,"","","","" },
{ "Dulcimer",            ITEM_TOOL, GP(25), LB(10), 0,0,0,0,"","","","" },
{ "Flute",               ITEM_TOOL, GP(2),  LB(1),  0,0,0,0,"","","","" },
{ "Lute",                ITEM_TOOL, GP(35), LB(2),  0,0,0,0,"","","","" },
{ "Lyre",                ITEM_TOOL, GP(30), LB(2),  0,0,0,0,"","","","" },
{ "Horn",                ITEM_TOOL, GP(3),  LB(2),  0,0,0,0,"","","","" },
{ "Pan flute",           ITEM_TOOL, GP(12), LB(2),  0,0,0,0,"","","","" },
{ "Shawm",               ITEM_TOOL, GP(2),  LB(1),  0,0,0,0,"","","","" },
{ "Viol",                ITEM_TOOL, GP(30), LB(1),  0,0,0,0,"","","","" },

/* ---------------------------------- mounts --------------------------------- */
{ "Camel",               ITEM_MOUNT, GP(50),  0, 0,0,0,0,"","","","" },
{ "Donkey or mule",      ITEM_MOUNT, GP(8),   0, 0,0,0,0,"","","","" },
{ "Draft horse",         ITEM_MOUNT, GP(50),  0, 0,0,0,0,"","","","" },
{ "Elephant",            ITEM_MOUNT, GP(200), 0, 0,0,0,0,"","","","" },
{ "Mastiff",             ITEM_MOUNT, GP(25),  0, 0,0,0,0,"","","","" },
{ "Pony",                ITEM_MOUNT, GP(30),  0, 0,0,0,0,"","","","" },
{ "Riding horse",        ITEM_MOUNT, GP(75),  0, 0,0,0,0,"","","","" },
{ "Warhorse",            ITEM_MOUNT, GP(400), 0, 0,0,0,0,"","","","" },
{ "Saddle, riding",      ITEM_MOUNT, GP(10), LB(25), 0,0,0,0,"","","","" },
{ "Saddlebags",          ITEM_MOUNT, GP(4),  LB(8),  0,0,0,0,"","","","" },
{ "Feed (per day)",      ITEM_MOUNT, CP(5),  LB(10), 0,0,0,0,"","","","" },
};
const int ITEM_COUNT = (int)(sizeof(ITEMS) / sizeof(ITEMS[0]));

int find_item(const char *name)
{
    int i;
    for (i = 0; i < ITEM_COUNT; i++) {
        if (strcmp(ITEMS[i].name, name) == 0) return i;
    }
    /* Fall back to a case-insensitive match so saved sheets survive small
       differences in capitalisation. */
    for (i = 0; i < ITEM_COUNT; i++) {
        const char *a = ITEMS[i].name, *b = name;
        while (*a && *b) {
            int ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
            int cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
            if (ca != cb) break;
            a++; b++;
        }
        if (!*a && !*b) return i;
    }
    return -1;
}
