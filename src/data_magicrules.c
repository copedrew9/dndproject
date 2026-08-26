/* data_magicrules.c -- the magic items that change a number on the sheet.
 *
 * Most magic items do something the sheet cannot compute: they grant a
 * spell, a reaction, an effect that depends on the situation. A few change
 * Armor Class, a saving throw, an ability score, a speed or a resistance
 * flatly and unconditionally, and those are the ones a character sheet
 * should be doing the arithmetic for.
 *
 * This is a side table keyed by name rather than extra columns on
 * MAGIC_ITEMS, because it describes about thirty of the two hundred and
 * seventy entries and the rest would just carry zeroes. Rows use designated
 * initialisers so that each one says which fields it is actually setting.
 *
 * What is deliberately NOT here: anything conditional. The Defender's bonus
 * is split between attack and AC anew each turn; an Arrow-Catching Shield's
 * extra +2 applies only against ranged attacks; Boots of Speed double your
 * speed only while switched on. Those stay prose, and the sheet says so.
 *
 * A "*" in resist, or a sets_to of 0, means the copy carries the answer:
 * a ring of resistance resists whatever its gem is, and a belt of giant
 * strength grants whatever its giant's score is. Those are asked for when
 * the item is picked up and stored on the inventory entry.
 */
#include "data.h"

const MagicRule MAGIC_RULES[] = {

/* ------------------------------------------- worn, adding to what you have */
{ "Ring of Protection",   .ac_bonus = 1, .save_bonus = 1 },
{ "Cloak of Protection",  .ac_bonus = 1, .save_bonus = 1 },
{ "Staff of Power",       .ac_bonus = 2, .save_bonus = 2 },
{ "Bracers of Defense",   .ac_bonus = 2, .only_unarmored = 1 },
{ "Robe of the Archmagi", .only_unarmored = 1, .unarmored_base = 15 },

/* -------------------------------------------------- armour in its own right
 * armor_base already includes the item's own bonus: Dwarven Plate is plate
 * (18) plus 2, so 20. armor_dex is -1 for the full modifier, 0 for none. */
{ "Demon Armor",
  .armor_base = 19, .armor_dex = 0, .armor_str = 15, .armor_stealth = 1 },
{ "Dragon Scale Mail",
  .armor_base = 15, .armor_dex = 2, .armor_stealth = 1, .resist = "*" },
{ "Dwarven Plate",
  .armor_base = 20, .armor_dex = 0, .armor_str = 15, .armor_stealth = 1 },
{ "Efreeti Chain",
  .armor_base = 19, .armor_dex = 0, .immune = "fire" },
{ "Elven Chain",       .armor_base = 14, .armor_dex = 2 },
{ "Glamoured Studded Leather", .armor_base = 13, .armor_dex = -1 },
{ "Plate Armor of Etherealness",
  .armor_base = 18, .armor_dex = 0, .armor_str = 15, .armor_stealth = 1 },
{ "Armor of Invulnerability",
  .armor_base = 18, .armor_dex = 0, .armor_str = 15, .armor_stealth = 1,
  .resist = "nonmagical damage" },
{ "Armor of Vulnerability",
  .armor_base = 18, .armor_dex = 0, .armor_str = 15, .armor_stealth = 1,
  .resist = "*" },
{ "Armor of Resistance",
  .armor_base = 18, .armor_dex = 0, .armor_str = 15, .armor_stealth = 1,
  .resist = "*" },
{ "Armor, +1, +2, or +3", .variable = 1 },

/* ------------------------------------------------------------------ shields
 * shield is the whole bonus, the shield's own +2 included. */
{ "Animated Shield",              .shield = 2 },
{ "Arrow-Catching Shield",        .shield = 2 },
{ "Shield of Missile Attraction", .shield = 2 },
{ "Spellguard Shield",            .shield = 2 },
{ "Shield, +1, +2, or +3",        .shield = 2, .variable = 1 },

/* ------------------------------------------------- scores set, not raised */
{ "Amulet of Health",         .sets_ability = ABL_CON + 1, .sets_to = 19 },
{ "Headband of Intellect",    .sets_ability = ABL_INT + 1, .sets_to = 19 },
{ "Gauntlets of Ogre Power",  .sets_ability = ABL_STR + 1, .sets_to = 19 },
/* The belt's score depends on which giant it draws from, so the copy
   carries it. */
{ "Belt of Giant Strength",   .sets_ability = ABL_STR + 1, .sets_to = 0 },

/* ------------------------------------------------------------------ speeds */
{ "Boots of Striding and Springing", .sets_speed = 30 },
{ "Winged Boots",         .fly_speed = -1 },
{ "Wings of Flying",      .fly_speed = 60 },
{ "Cloak of the Manta Ray", .swim_speed = 60 },
{ "Ring of Swimming",     .swim_speed = 40 },
{ "Cloak of Arachnida",   .climb_speed = -1, .resist = "poison" },

/* ------------------------------------------------ resistance and immunity */
{ "Ring of Resistance",     .resist = "*" },
{ "Brooch of Shielding",    .resist = "force" },
{ "Boots of the Winterlands", .resist = "cold" },
{ "Ring of Warmth",         .resist = "cold" },
{ "Frost Brand",            .resist = "fire" },
{ "Periapt of Proof against Poison", .immune = "poison" },
};
const int MAGIC_RULE_COUNT =
    (int)(sizeof(MAGIC_RULES) / sizeof(MAGIC_RULES[0]));

const MagicRule *magic_rule_for(const char *name)
{
    int i;
    for (i = 0; i < MAGIC_RULE_COUNT; i++) {
        const char *a = MAGIC_RULES[i].item, *b = name;
        while (*a && *a == *b) { a++; b++; }
        if (!*a && !*b) return &MAGIC_RULES[i];
    }
    return NULL;
}
