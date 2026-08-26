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

const ItemData BOOK_ITEMS[] = {
/* ------------------------------- light armour ------------------------------ */
{ "Padded armor", BOOK_PHB,        ITEM_LIGHT_ARMOR,  GP(5),   LB(8),  11, -1, 0, 1, "","","","" },
{ "Leather armor", BOOK_PHB,       ITEM_LIGHT_ARMOR,  GP(10),  LB(10), 11, -1, 0, 0, "","","","" },
{ "Studded leather armor", BOOK_PHB,ITEM_LIGHT_ARMOR, GP(45),  LB(13), 12, -1, 0, 0, "","","","" },

/* ------------------------------ medium armour ------------------------------ */
{ "Hide armor", BOOK_PHB,          ITEM_MEDIUM_ARMOR, GP(10),  LB(12), 12,  2, 0, 0, "","","","" },
{ "Chain shirt", BOOK_PHB,         ITEM_MEDIUM_ARMOR, GP(50),  LB(20), 13,  2, 0, 0, "","","","" },
{ "Scale mail", BOOK_PHB,          ITEM_MEDIUM_ARMOR, GP(50),  LB(45), 14,  2, 0, 1, "","","","" },
{ "Breastplate", BOOK_PHB,         ITEM_MEDIUM_ARMOR, GP(400), LB(20), 14,  2, 0, 0, "","","","" },
{ "Half plate", BOOK_PHB,          ITEM_MEDIUM_ARMOR, GP(750), LB(40), 15,  2, 0, 1, "","","","" },

/* ------------------------------- heavy armour ------------------------------ */
{ "Ring mail", BOOK_PHB,           ITEM_HEAVY_ARMOR,  GP(30),  LB(40), 14,  0,  0, 1, "","","","" },
{ "Chain mail", BOOK_PHB,          ITEM_HEAVY_ARMOR,  GP(75),  LB(55), 16,  0, 13, 1, "","","","" },
{ "Splint armor", BOOK_PHB,        ITEM_HEAVY_ARMOR,  GP(200), LB(60), 17,  0, 15, 1, "","","","" },
{ "Plate armor", BOOK_PHB,         ITEM_HEAVY_ARMOR,  GP(1500),LB(65), 18,  0, 15, 1, "","","","" },

/* ---------------------------------- shield --------------------------------- */
{ "Shield", BOOK_PHB,              ITEM_SHIELD,       GP(10),  LB(6),   2,  0,  0, 0, "","","","" },

/* ------------------------------ simple melee ------------------------------- */
{ "Club", BOOK_PHB,          ITEM_SIMPLE_MELEE, SP(1),  LB(2),  0,0,0,0, "1d4","bludgeoning","Light","" },
{ "Dagger", BOOK_PHB,        ITEM_SIMPLE_MELEE, GP(2),  LB(1),  0,0,0,0, "1d4","piercing","Finesse, light, thrown (range 20/60)","" },
{ "Greatclub", BOOK_PHB,     ITEM_SIMPLE_MELEE, SP(2),  LB(10), 0,0,0,0, "1d8","bludgeoning","Two-handed","" },
{ "Handaxe", BOOK_PHB,       ITEM_SIMPLE_MELEE, GP(5),  LB(2),  0,0,0,0, "1d6","slashing","Light, thrown (range 20/60)","" },
{ "Javelin", BOOK_PHB,       ITEM_SIMPLE_MELEE, SP(5),  LB(2),  0,0,0,0, "1d6","piercing","Thrown (range 30/120)","" },
{ "Light hammer", BOOK_PHB,  ITEM_SIMPLE_MELEE, GP(2),  LB(2),  0,0,0,0, "1d4","bludgeoning","Light, thrown (range 20/60)","" },
{ "Mace", BOOK_PHB,          ITEM_SIMPLE_MELEE, GP(5),  LB(4),  0,0,0,0, "1d6","bludgeoning","","" },
{ "Quarterstaff", BOOK_PHB,  ITEM_SIMPLE_MELEE, SP(2),  LB(4),  0,0,0,0, "1d6","bludgeoning","Versatile (1d8)","" },
{ "Sickle", BOOK_PHB,        ITEM_SIMPLE_MELEE, GP(1),  LB(2),  0,0,0,0, "1d4","slashing","Light","" },
{ "Spear", BOOK_PHB,         ITEM_SIMPLE_MELEE, GP(1),  LB(3),  0,0,0,0, "1d6","piercing","Thrown (range 20/60), versatile (1d8)","" },

/* ------------------------------ simple ranged ------------------------------ */
{ "Light crossbow", BOOK_PHB,ITEM_SIMPLE_RANGED, GP(25), LB(5),  0,0,0,0, "1d8","piercing","Ammunition (range 80/320), loading, two-handed","" },
{ "Dart", BOOK_PHB,          ITEM_SIMPLE_RANGED, CP(5),  2,      0,0,0,0, "1d4","piercing","Finesse, thrown (range 20/60)","" },
{ "Shortbow", BOOK_PHB,      ITEM_SIMPLE_RANGED, GP(25), LB(2),  0,0,0,0, "1d6","piercing","Ammunition (range 80/320), two-handed","" },
{ "Sling", BOOK_PHB,         ITEM_SIMPLE_RANGED, SP(1),  0,      0,0,0,0, "1d4","bludgeoning","Ammunition (range 30/120)","" },

/* ------------------------------ martial melee ------------------------------ */
{ "Battleaxe", BOOK_PHB,     ITEM_MARTIAL_MELEE, GP(10), LB(4),  0,0,0,0, "1d8","slashing","Versatile (1d10)","" },
{ "Flail", BOOK_PHB,         ITEM_MARTIAL_MELEE, GP(10), LB(2),  0,0,0,0, "1d8","bludgeoning","","" },
{ "Glaive", BOOK_PHB,        ITEM_MARTIAL_MELEE, GP(20), LB(6),  0,0,0,0, "1d10","slashing","Heavy, reach, two-handed","" },
{ "Greataxe", BOOK_PHB,      ITEM_MARTIAL_MELEE, GP(30), LB(7),  0,0,0,0, "1d12","slashing","Heavy, two-handed","" },
{ "Greatsword", BOOK_PHB,    ITEM_MARTIAL_MELEE, GP(50), LB(6),  0,0,0,0, "2d6","slashing","Heavy, two-handed","" },
{ "Halberd", BOOK_PHB,       ITEM_MARTIAL_MELEE, GP(20), LB(6),  0,0,0,0, "1d10","slashing","Heavy, reach, two-handed","" },
{ "Lance", BOOK_PHB,         ITEM_MARTIAL_MELEE, GP(10), LB(6),  0,0,0,0, "1d12","piercing","Reach, special","" },
{ "Longsword", BOOK_PHB,     ITEM_MARTIAL_MELEE, GP(15), LB(3),  0,0,0,0, "1d8","slashing","Versatile (1d10)","" },
{ "Maul", BOOK_PHB,          ITEM_MARTIAL_MELEE, GP(10), LB(10), 0,0,0,0, "2d6","bludgeoning","Heavy, two-handed","" },
{ "Morningstar", BOOK_PHB,   ITEM_MARTIAL_MELEE, GP(15), LB(4),  0,0,0,0, "1d8","piercing","","" },
{ "Pike", BOOK_PHB,          ITEM_MARTIAL_MELEE, GP(5),  LB(18), 0,0,0,0, "1d10","piercing","Heavy, reach, two-handed","" },
{ "Rapier", BOOK_PHB,        ITEM_MARTIAL_MELEE, GP(25), LB(2),  0,0,0,0, "1d8","piercing","Finesse","" },
{ "Scimitar", BOOK_PHB,      ITEM_MARTIAL_MELEE, GP(25), LB(3),  0,0,0,0, "1d6","slashing","Finesse, light","" },
{ "Shortsword", BOOK_PHB,    ITEM_MARTIAL_MELEE, GP(10), LB(2),  0,0,0,0, "1d6","piercing","Finesse, light","" },
{ "Trident", BOOK_PHB,       ITEM_MARTIAL_MELEE, GP(5),  LB(4),  0,0,0,0, "1d6","piercing","Thrown (range 20/60), versatile (1d8)","" },
{ "War pick", BOOK_PHB,      ITEM_MARTIAL_MELEE, GP(5),  LB(2),  0,0,0,0, "1d8","piercing","","" },
{ "Warhammer", BOOK_PHB,     ITEM_MARTIAL_MELEE, GP(15), LB(2),  0,0,0,0, "1d8","bludgeoning","Versatile (1d10)","" },
{ "Whip", BOOK_PHB,          ITEM_MARTIAL_MELEE, GP(2),  LB(3),  0,0,0,0, "1d4","slashing","Finesse, reach","" },

/* ----------------------------- martial ranged ------------------------------ */
{ "Blowgun", BOOK_PHB,       ITEM_MARTIAL_RANGED, GP(10), LB(1),  0,0,0,0, "1","piercing","Ammunition (range 25/100), loading","" },
{ "Hand crossbow", BOOK_PHB, ITEM_MARTIAL_RANGED, GP(75), LB(3),  0,0,0,0, "1d6","piercing","Ammunition (range 30/120), light, loading","" },
{ "Heavy crossbow", BOOK_PHB,ITEM_MARTIAL_RANGED, GP(50), LB(18), 0,0,0,0, "1d10","piercing","Ammunition (range 100/400), heavy, loading, two-handed","" },
{ "Longbow", BOOK_PHB,       ITEM_MARTIAL_RANGED, GP(50), LB(2),  0,0,0,0, "1d8","piercing","Ammunition (range 150/600), heavy, two-handed","" },
{ "Net", BOOK_PHB,           ITEM_MARTIAL_RANGED, GP(1),  LB(3),  0,0,0,0, "-","-","Special, thrown (range 5/15)","" },

/* ---------------------------------- packs ---------------------------------- */
{ "Burglar's pack", BOOK_PHB, ITEM_PACK, GP(16), LB(47), 0,0,0,0, "","","",
  "Backpack, bag of 1000 ball bearings, 10 feet of string, bell, 5 candles, "
  "crowbar, hammer, 10 pitons, hooded lantern, 2 flasks of oil, 5 days "
  "rations, tinderbox, waterskin, 50 feet of hempen rope" },
{ "Diplomat's pack", BOOK_PHB, ITEM_PACK, GP(39), LB(39), 0,0,0,0, "","","",
  "Chest, 2 cases for maps and scrolls, fine clothes, bottle of ink, ink "
  "pen, lamp, 2 flasks of oil, 5 sheets of paper, vial of perfume, sealing "
  "wax, soap" },
{ "Dungeoneer's pack", BOOK_PHB, ITEM_PACK, GP(12), LB(61), 0,0,0,0, "","","",
  "Backpack, crowbar, hammer, 10 pitons, 10 torches, tinderbox, 10 days "
  "rations, waterskin, 50 feet of hempen rope" },
{ "Entertainer's pack", BOOK_PHB, ITEM_PACK, GP(40), LB(38), 0,0,0,0, "","","",
  "Backpack, bedroll, 2 costumes, 5 candles, 5 days rations, waterskin, "
  "disguise kit" },
{ "Explorer's pack", BOOK_PHB, ITEM_PACK, GP(10), LB(59), 0,0,0,0, "","","",
  "Backpack, bedroll, mess kit, tinderbox, 10 torches, 10 days rations, "
  "waterskin, 50 feet of hempen rope" },
{ "Priest's pack", BOOK_PHB, ITEM_PACK, GP(19), LB(24), 0,0,0,0, "","","",
  "Backpack, blanket, 10 candles, tinderbox, alms box, 2 blocks of incense, "
  "censer, vestments, 2 days rations, waterskin" },
{ "Scholar's pack", BOOK_PHB, ITEM_PACK, GP(40), LB(10), 0,0,0,0, "","","",
  "Backpack, book of lore, bottle of ink, ink pen, 10 sheets of parchment, "
  "little bag of sand, small knife" },

/* ------------------------------ adventuring gear --------------------------- */
{ "Abacus", BOOK_PHB,              ITEM_GEAR, GP(2),  LB(2),  0,0,0,0,"","","","" },
{ "Acid (vial)", BOOK_PHB,         ITEM_GEAR, GP(25), LB(1),  0,0,0,0,"","","","" },
{ "Alchemist's fire (flask)", BOOK_PHB, ITEM_GEAR, GP(50), LB(1), 0,0,0,0,"","","","" },
{ "Arrows (20)", BOOK_PHB,         ITEM_GEAR, GP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Blowgun needles (50)", BOOK_PHB,ITEM_GEAR, GP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Crossbow bolts (20)", BOOK_PHB, ITEM_GEAR, GP(1),  LB(15), 0,0,0,0,"","","","" },
{ "Sling bullets (20)", BOOK_PHB,  ITEM_GEAR, CP(4),  LB(15), 0,0,0,0,"","","","" },
{ "Antitoxin (vial)", BOOK_PHB,    ITEM_GEAR, GP(50), 0,      0,0,0,0,"","","","" },
{ "Backpack", BOOK_PHB,            ITEM_GEAR, GP(2),  LB(5),  0,0,0,0,"","","","" },
{ "Ball bearings (bag of 1,000)", BOOK_PHB, ITEM_GEAR, GP(1), LB(2), 0,0,0,0,"","","","" },
{ "Barrel", BOOK_PHB,              ITEM_GEAR, GP(2),  LB(70), 0,0,0,0,"","","","" },
{ "Basket", BOOK_PHB,              ITEM_GEAR, SP(4),  LB(2),  0,0,0,0,"","","","" },
{ "Bedroll", BOOK_PHB,             ITEM_GEAR, GP(1),  LB(7),  0,0,0,0,"","","","" },
{ "Bell", BOOK_PHB,                ITEM_GEAR, GP(1),  0,      0,0,0,0,"","","","" },
{ "Blanket", BOOK_PHB,             ITEM_GEAR, SP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Block and tackle", BOOK_PHB,    ITEM_GEAR, GP(1),  LB(5),  0,0,0,0,"","","","" },
{ "Book", BOOK_PHB,                ITEM_GEAR, GP(25), LB(5),  0,0,0,0,"","","","" },
{ "Bucket", BOOK_PHB,              ITEM_GEAR, CP(5),  LB(2),  0,0,0,0,"","","","" },
{ "Caltrops (bag of 20)", BOOK_PHB,ITEM_GEAR, GP(1),  LB(2),  0,0,0,0,"","","","" },
{ "Candle", BOOK_PHB,              ITEM_GEAR, CP(1),  0,      0,0,0,0,"","","","" },
{ "Chain (10 feet)", BOOK_PHB,     ITEM_GEAR, GP(5),  LB(10), 0,0,0,0,"","","","" },
{ "Chalk (1 piece)", BOOK_PHB,     ITEM_GEAR, CP(1),  0,      0,0,0,0,"","","","" },
{ "Chest", BOOK_PHB,               ITEM_GEAR, GP(5),  LB(25), 0,0,0,0,"","","","" },
{ "Climber's kit", BOOK_PHB,       ITEM_GEAR, GP(25), LB(12), 0,0,0,0,"","","","" },
{ "Clothes, common", BOOK_PHB,     ITEM_GEAR, SP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Clothes, costume", BOOK_PHB,    ITEM_GEAR, GP(5),  LB(4),  0,0,0,0,"","","","" },
{ "Clothes, fine", BOOK_PHB,       ITEM_GEAR, GP(15), LB(6),  0,0,0,0,"","","","" },
{ "Clothes, traveler's", BOOK_PHB, ITEM_GEAR, GP(2),  LB(4),  0,0,0,0,"","","","" },
{ "Component pouch", BOOK_PHB,     ITEM_GEAR, GP(25), LB(2),  0,0,0,0,"","","","" },
{ "Crowbar", BOOK_PHB,             ITEM_GEAR, GP(2),  LB(5),  0,0,0,0,"","","","" },
{ "Crystal (arcane focus)", BOOK_PHB, ITEM_GEAR, GP(10), LB(1), 0,0,0,0,"","","","" },
{ "Orb (arcane focus)", BOOK_PHB,  ITEM_GEAR, GP(20), LB(30), 0,0,0,0,"","","","" },
{ "Rod (arcane focus)", BOOK_PHB,  ITEM_GEAR, GP(10), LB(2),  0,0,0,0,"","","","" },
{ "Staff (arcane focus)", BOOK_PHB,ITEM_GEAR, GP(5),  LB(4),  0,0,0,0,"","","","" },
{ "Wand (arcane focus)", BOOK_PHB, ITEM_GEAR, GP(10), LB(1),  0,0,0,0,"","","","" },
{ "Sprig of mistletoe (druidic focus)", BOOK_PHB, ITEM_GEAR, GP(1), 0, 0,0,0,0,"","","","" },
{ "Wooden staff (druidic focus)", BOOK_PHB, ITEM_GEAR, GP(5), LB(4), 0,0,0,0,"","","","" },
{ "Yew wand (druidic focus)", BOOK_PHB, ITEM_GEAR, GP(10), LB(1), 0,0,0,0,"","","","" },
{ "Amulet (holy symbol)", BOOK_PHB,ITEM_GEAR, GP(5),  LB(1),  0,0,0,0,"","","","" },
{ "Emblem (holy symbol)", BOOK_PHB,ITEM_GEAR, GP(5),  0,      0,0,0,0,"","","","" },
{ "Reliquary (holy symbol)", BOOK_PHB, ITEM_GEAR, GP(5), LB(2), 0,0,0,0,"","","","" },
{ "Fishing tackle", BOOK_PHB,      ITEM_GEAR, GP(1),  LB(4),  0,0,0,0,"","","","" },
{ "Flask or tankard", BOOK_PHB,    ITEM_GEAR, CP(2),  LB(1),  0,0,0,0,"","","","" },
{ "Grappling hook", BOOK_PHB,      ITEM_GEAR, GP(2),  LB(4),  0,0,0,0,"","","","" },
{ "Hammer", BOOK_PHB,              ITEM_GEAR, GP(1),  LB(3),  0,0,0,0,"","","","" },
{ "Hammer, sledge", BOOK_PHB,      ITEM_GEAR, GP(2),  LB(10), 0,0,0,0,"","","","" },
{ "Healer's kit", BOOK_PHB,        ITEM_GEAR, GP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Holy water (flask)", BOOK_PHB,  ITEM_GEAR, GP(25), LB(1),  0,0,0,0,"","","","" },
{ "Hourglass", BOOK_PHB,           ITEM_GEAR, GP(25), LB(1),  0,0,0,0,"","","","" },
{ "Hunting trap", BOOK_PHB,        ITEM_GEAR, GP(5),  LB(25), 0,0,0,0,"","","","" },
{ "Ink (1 ounce bottle)", BOOK_PHB,ITEM_GEAR, GP(10), 0,      0,0,0,0,"","","","" },
{ "Ink pen", BOOK_PHB,             ITEM_GEAR, CP(2),  0,      0,0,0,0,"","","","" },
{ "Jug or pitcher", BOOK_PHB,      ITEM_GEAR, CP(2),  LB(4),  0,0,0,0,"","","","" },
{ "Ladder (10-foot)", BOOK_PHB,    ITEM_GEAR, SP(1),  LB(25), 0,0,0,0,"","","","" },
{ "Lamp", BOOK_PHB,                ITEM_GEAR, SP(5),  LB(1),  0,0,0,0,"","","","" },
{ "Lantern, bullseye", BOOK_PHB,   ITEM_GEAR, GP(10), LB(2),  0,0,0,0,"","","","" },
{ "Lantern, hooded", BOOK_PHB,     ITEM_GEAR, GP(5),  LB(2),  0,0,0,0,"","","","" },
{ "Lock", BOOK_PHB,                ITEM_GEAR, GP(10), LB(1),  0,0,0,0,"","","","" },
{ "Magnifying glass", BOOK_PHB,    ITEM_GEAR, GP(100),0,      0,0,0,0,"","","","" },
{ "Manacles", BOOK_PHB,            ITEM_GEAR, GP(2),  LB(6),  0,0,0,0,"","","","" },
{ "Mess kit", BOOK_PHB,            ITEM_GEAR, SP(2),  LB(1),  0,0,0,0,"","","","" },
{ "Mirror, steel", BOOK_PHB,       ITEM_GEAR, GP(5),  LB(5),  0,0,0,0,"","","","" },
{ "Oil (flask)", BOOK_PHB,         ITEM_GEAR, SP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Paper (one sheet)", BOOK_PHB,   ITEM_GEAR, SP(2),  0,      0,0,0,0,"","","","" },
{ "Parchment (one sheet)", BOOK_PHB,ITEM_GEAR,SP(1),  0,      0,0,0,0,"","","","" },
{ "Perfume (vial)", BOOK_PHB,      ITEM_GEAR, GP(5),  0,      0,0,0,0,"","","","" },
{ "Pick, miner's", BOOK_PHB,       ITEM_GEAR, GP(2),  LB(10), 0,0,0,0,"","","","" },
{ "Piton", BOOK_PHB,               ITEM_GEAR, CP(5),  2,      0,0,0,0,"","","","" },
{ "Poison, basic (vial)", BOOK_PHB,ITEM_GEAR, GP(100),0,      0,0,0,0,"","","","" },
{ "Pole (10-foot)", BOOK_PHB,      ITEM_GEAR, CP(5),  LB(7),  0,0,0,0,"","","","" },
{ "Pot, iron", BOOK_PHB,           ITEM_GEAR, GP(2),  LB(10), 0,0,0,0,"","","","" },
{ "Potion of healing", BOOK_PHB,   ITEM_GEAR, GP(50), 5,      0,0,0,0,"","","","" },
{ "Pouch", BOOK_PHB,               ITEM_GEAR, SP(5),  LB(1),  0,0,0,0,"","","","" },
{ "Quiver", BOOK_PHB,              ITEM_GEAR, GP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Ram, portable", BOOK_PHB,       ITEM_GEAR, GP(4),  LB(35), 0,0,0,0,"","","","" },
{ "Rations (1 day)", BOOK_PHB,     ITEM_GEAR, SP(5),  LB(2),  0,0,0,0,"","","","" },
{ "Robes", BOOK_PHB,               ITEM_GEAR, GP(1),  LB(4),  0,0,0,0,"","","","" },
{ "Rope, hempen (50 feet)", BOOK_PHB, ITEM_GEAR, GP(1), LB(10), 0,0,0,0,"","","","" },
{ "Rope, silk (50 feet)", BOOK_PHB,ITEM_GEAR, GP(10), LB(5),  0,0,0,0,"","","","" },
{ "Sack", BOOK_PHB,                ITEM_GEAR, CP(1),  5,      0,0,0,0,"","","","" },
{ "Scale, merchant's", BOOK_PHB,   ITEM_GEAR, GP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Sealing wax", BOOK_PHB,         ITEM_GEAR, SP(5),  0,      0,0,0,0,"","","","" },
{ "Shovel", BOOK_PHB,              ITEM_GEAR, GP(2),  LB(5),  0,0,0,0,"","","","" },
{ "Signal whistle", BOOK_PHB,      ITEM_GEAR, CP(5),  0,      0,0,0,0,"","","","" },
{ "Signet ring", BOOK_PHB,         ITEM_GEAR, GP(5),  0,      0,0,0,0,"","","","" },
{ "Soap", BOOK_PHB,                ITEM_GEAR, CP(2),  0,      0,0,0,0,"","","","" },
{ "Spellbook", BOOK_PHB,           ITEM_GEAR, GP(50), LB(3),  0,0,0,0,"","","","" },
{ "Spikes, iron (10)", BOOK_PHB,   ITEM_GEAR, GP(1),  LB(5),  0,0,0,0,"","","","" },
{ "Spyglass", BOOK_PHB,            ITEM_GEAR, GP(1000),LB(1), 0,0,0,0,"","","","" },
{ "Tent, two-person", BOOK_PHB,    ITEM_GEAR, GP(2),  LB(20), 0,0,0,0,"","","","" },
{ "Tinderbox", BOOK_PHB,           ITEM_GEAR, SP(5),  LB(1),  0,0,0,0,"","","","" },
{ "Torch", BOOK_PHB,               ITEM_GEAR, CP(1),  LB(1),  0,0,0,0,"","","","" },
{ "Vial", BOOK_PHB,                ITEM_GEAR, GP(1),  0,      0,0,0,0,"","","","" },
{ "Waterskin", BOOK_PHB,           ITEM_GEAR, SP(2),  LB(5),  0,0,0,0,"","","","" },
{ "Whetstone", BOOK_PHB,           ITEM_GEAR, CP(1),  LB(1),  0,0,0,0,"","","","" },

/* ----------------------------------- tools --------------------------------- */
{ "Alchemist's supplies", BOOK_PHB,ITEM_TOOL, GP(50), LB(8),  0,0,0,0,"","","","" },
{ "Brewer's supplies", BOOK_PHB,   ITEM_TOOL, GP(20), LB(9),  0,0,0,0,"","","","" },
{ "Calligrapher's supplies", BOOK_PHB, ITEM_TOOL, GP(10), LB(5), 0,0,0,0,"","","","" },
{ "Carpenter's tools", BOOK_PHB,   ITEM_TOOL, GP(8),  LB(6),  0,0,0,0,"","","","" },
{ "Cartographer's tools", BOOK_PHB,ITEM_TOOL, GP(15), LB(6),  0,0,0,0,"","","","" },
{ "Cobbler's tools", BOOK_PHB,     ITEM_TOOL, GP(5),  LB(5),  0,0,0,0,"","","","" },
{ "Cook's utensils", BOOK_PHB,     ITEM_TOOL, GP(1),  LB(8),  0,0,0,0,"","","","" },
{ "Glassblower's tools", BOOK_PHB, ITEM_TOOL, GP(30), LB(5),  0,0,0,0,"","","","" },
{ "Jeweler's tools", BOOK_PHB,     ITEM_TOOL, GP(25), LB(2),  0,0,0,0,"","","","" },
{ "Leatherworker's tools", BOOK_PHB,ITEM_TOOL,GP(5),  LB(5),  0,0,0,0,"","","","" },
{ "Mason's tools", BOOK_PHB,       ITEM_TOOL, GP(10), LB(8),  0,0,0,0,"","","","" },
{ "Painter's supplies", BOOK_PHB,  ITEM_TOOL, GP(10), LB(5),  0,0,0,0,"","","","" },
{ "Potter's tools", BOOK_PHB,      ITEM_TOOL, GP(10), LB(3),  0,0,0,0,"","","","" },
{ "Smith's tools", BOOK_PHB,       ITEM_TOOL, GP(20), LB(8),  0,0,0,0,"","","","" },
{ "Tinker's tools", BOOK_PHB,      ITEM_TOOL, GP(50), LB(10), 0,0,0,0,"","","","" },
{ "Weaver's tools", BOOK_PHB,      ITEM_TOOL, GP(1),  LB(5),  0,0,0,0,"","","","" },
{ "Woodcarver's tools", BOOK_PHB,  ITEM_TOOL, GP(1),  LB(5),  0,0,0,0,"","","","" },
{ "Disguise kit", BOOK_PHB,        ITEM_TOOL, GP(25), LB(3),  0,0,0,0,"","","","" },
{ "Forgery kit", BOOK_PHB,         ITEM_TOOL, GP(15), LB(5),  0,0,0,0,"","","","" },
{ "Herbalism kit", BOOK_PHB,       ITEM_TOOL, GP(5),  LB(3),  0,0,0,0,"","","","" },
{ "Navigator's tools", BOOK_PHB,   ITEM_TOOL, GP(25), LB(2),  0,0,0,0,"","","","" },
{ "Poisoner's kit", BOOK_PHB,      ITEM_TOOL, GP(50), LB(2),  0,0,0,0,"","","","" },
{ "Thieves' tools", BOOK_PHB,      ITEM_TOOL, GP(25), LB(1),  0,0,0,0,"","","","" },
{ "Dice set", BOOK_PHB,            ITEM_TOOL, SP(1),  0,      0,0,0,0,"","","","" },
{ "Playing card set", BOOK_PHB,    ITEM_TOOL, SP(5),  0,      0,0,0,0,"","","","" },
{ "Bagpipes", BOOK_PHB,            ITEM_TOOL, GP(30), LB(6),  0,0,0,0,"","","","" },
{ "Drum", BOOK_PHB,                ITEM_TOOL, GP(6),  LB(3),  0,0,0,0,"","","","" },
{ "Dulcimer", BOOK_PHB,            ITEM_TOOL, GP(25), LB(10), 0,0,0,0,"","","","" },
{ "Flute", BOOK_PHB,               ITEM_TOOL, GP(2),  LB(1),  0,0,0,0,"","","","" },
{ "Lute", BOOK_PHB,                ITEM_TOOL, GP(35), LB(2),  0,0,0,0,"","","","" },
{ "Lyre", BOOK_PHB,                ITEM_TOOL, GP(30), LB(2),  0,0,0,0,"","","","" },
{ "Horn", BOOK_PHB,                ITEM_TOOL, GP(3),  LB(2),  0,0,0,0,"","","","" },
{ "Pan flute", BOOK_PHB,           ITEM_TOOL, GP(12), LB(2),  0,0,0,0,"","","","" },
{ "Shawm", BOOK_PHB,               ITEM_TOOL, GP(2),  LB(1),  0,0,0,0,"","","","" },
{ "Viol", BOOK_PHB,                ITEM_TOOL, GP(30), LB(1),  0,0,0,0,"","","","" },

/* ---------------------------------- mounts --------------------------------- */
{ "Camel", BOOK_PHB,               ITEM_MOUNT, GP(50),  0, 0,0,0,0,"","","","" },
{ "Donkey or mule", BOOK_PHB,      ITEM_MOUNT, GP(8),   0, 0,0,0,0,"","","","" },
{ "Draft horse", BOOK_PHB,         ITEM_MOUNT, GP(50),  0, 0,0,0,0,"","","","" },
{ "Elephant", BOOK_PHB,            ITEM_MOUNT, GP(200), 0, 0,0,0,0,"","","","" },
{ "Mastiff", BOOK_PHB,             ITEM_MOUNT, GP(25),  0, 0,0,0,0,"","","","" },
{ "Pony", BOOK_PHB,                ITEM_MOUNT, GP(30),  0, 0,0,0,0,"","","","" },
{ "Riding horse", BOOK_PHB,        ITEM_MOUNT, GP(75),  0, 0,0,0,0,"","","","" },
{ "Warhorse", BOOK_PHB,            ITEM_MOUNT, GP(400), 0, 0,0,0,0,"","","","" },
{ "Saddle, riding", BOOK_PHB,      ITEM_MOUNT, GP(10), LB(25), 0,0,0,0,"","","","" },
{ "Saddlebags", BOOK_PHB,          ITEM_MOUNT, GP(4),  LB(8),  0,0,0,0,"","","","" },
{ "Feed (per day)", BOOK_PHB,      ITEM_MOUNT, CP(5),  LB(10), 0,0,0,0,"","","","" },
{ "Saddle, exotic", BOOK_PHB,      ITEM_MOUNT, GP(60), LB(40), 0,0,0,0,"","","","" },
{ "Saddle, military", BOOK_PHB,    ITEM_MOUNT, GP(20), LB(30), 0,0,0,0,"","","","" },
{ "Saddle, pack", BOOK_PHB,        ITEM_MOUNT, GP(5),  LB(15), 0,0,0,0,"","","","" },
{ "Bit and bridle", BOOK_PHB,      ITEM_MOUNT, GP(2),  LB(1),  0,0,0,0,"","","","" },
{ "Stabling (per day)", BOOK_PHB,  ITEM_MOUNT, SP(5),  0,      0,0,0,0,"","","","" },

/* -------------------------------- vehicles --------------------------------- */
{ "Cart", BOOK_PHB,                ITEM_MOUNT, GP(15),   LB(200), 0,0,0,0,"","","","" },
{ "Sled", BOOK_PHB,                ITEM_MOUNT, GP(20),   LB(300), 0,0,0,0,"","","","" },
{ "Wagon", BOOK_PHB,               ITEM_MOUNT, GP(35),   LB(400), 0,0,0,0,"","","","" },
{ "Carriage", BOOK_PHB,            ITEM_MOUNT, GP(100),  LB(600), 0,0,0,0,"","","","" },
{ "Chariot", BOOK_PHB,             ITEM_MOUNT, GP(250),  LB(100), 0,0,0,0,"","","","" },
{ "Rowboat", BOOK_PHB,             ITEM_MOUNT, GP(50),   0, 0,0,0,0,"","","","" },
{ "Keelboat", BOOK_PHB,            ITEM_MOUNT, GP(3000), 0, 0,0,0,0,"","","","" },
{ "Longship", BOOK_PHB,            ITEM_MOUNT, GP(10000),0, 0,0,0,0,"","","","" },
{ "Sailing ship", BOOK_PHB,        ITEM_MOUNT, GP(10000),0, 0,0,0,0,"","","","" },
{ "Warship", BOOK_PHB,             ITEM_MOUNT, GP(25000),0, 0,0,0,0,"","","","" },
{ "Galley", BOOK_PHB,              ITEM_MOUNT, GP(30000),0, 0,0,0,0,"","","","" },
};
const int BOOK_ITEM_COUNT = (int)(sizeof(BOOK_ITEMS) / sizeof(BOOK_ITEMS[0]));

/* Until homebrew.c says otherwise, the bank is just the book. */
const ItemData *ITEMS = BOOK_ITEMS;
int ITEM_COUNT = (int)(sizeof(BOOK_ITEMS) / sizeof(BOOK_ITEMS[0]));

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
