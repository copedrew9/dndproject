/* data_magicrules.c -- the magic items that change a number on the sheet.
 *
 * Most magic items do something the sheet cannot compute: they grant a
 * spell, a resistance, a reaction, an effect that depends on the situation.
 * A few change Armor Class or saving throws flatly and unconditionally, and
 * those are the ones a character sheet should be doing the arithmetic for.
 *
 * This is a side table keyed by name rather than extra columns on
 * MAGIC_ITEMS, because it describes twenty-odd of the two hundred and
 * seventy entries and the rest would just carry zeroes.
 *
 * What is deliberately NOT here: anything conditional. The Defender's bonus
 * is split between attack and AC anew each turn; an Arrow-Catching Shield's
 * extra +2 applies only against ranged attacks; a Robe of Scintillating
 * Colors imposes disadvantage rather than changing AC. Those stay prose, and
 * the sheet says so.
 */
#include "data.h"

const MagicRule MAGIC_RULES[] = {

/* ------------------------------------------- worn, adding to what you have */
{ "Ring of Protection",   1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
{ "Cloak of Protection",  1, 1, 0, 0, 0, 0, 0, 0, 0, 0 },
{ "Staff of Power",       2, 2, 0, 0, 0, 0, 0, 0, 0, 0 },

/* Bracers of Defense work only with no armour and no shield. */
{ "Bracers of Defense",   2, 0, 0, 0, 0, 0, 0, 1, 0, 0 },

/* The Robe of the Archmagi sets the unarmoured base rather than adding. */
{ "Robe of the Archmagi", 0, 0, 0, 0, 0, 0, 0, 1, 15, 0 },

/* -------------------------------------------------- armour in its own right
 * base already includes the item's own bonus: Dwarven Plate is plate (18)
 * plus 2, so 20. dex is -1 for the full modifier, 0 for none, or a cap. */
{ "Demon Armor",                 0, 0, 19, 0, 15, 1, 0, 0, 0, 0 },
{ "Dragon Scale Mail",           0, 0, 15, 2,  0, 1, 0, 0, 0, 0 },
{ "Dwarven Plate",               0, 0, 20, 0, 15, 1, 0, 0, 0, 0 },
{ "Efreeti Chain",               0, 0, 19, 0,  0, 0, 0, 0, 0, 0 },
{ "Elven Chain",                 0, 0, 14, 2,  0, 0, 0, 0, 0, 0 },
{ "Glamoured Studded Leather",   0, 0, 13, -1, 0, 0, 0, 0, 0, 0 },
{ "Plate Armor of Etherealness", 0, 0, 18, 0, 15, 1, 0, 0, 0, 0 },
{ "Armor of Invulnerability",    0, 0, 18, 0, 15, 1, 0, 0, 0, 0 },
{ "Armor of Vulnerability",      0, 0, 18, 0, 15, 1, 0, 0, 0, 0 },

/* Armour whose bonus is chosen when the item is found. */
{ "Armor, +1, +2, or +3",        0, 0, 0, 0, 0, 0, 0, 0, 0, 1 },

/* ------------------------------------------------------------------ shields
 * A shield's own +2 is counted here, so these are the whole bonus. */
{ "Animated Shield",             0, 0, 0, 0, 0, 0, 2, 0, 0, 0 },
{ "Arrow-Catching Shield",       0, 0, 0, 0, 0, 0, 2, 0, 0, 0 },
{ "Shield of Missile Attraction",0, 0, 0, 0, 0, 0, 2, 0, 0, 0 },
{ "Spellguard Shield",           0, 0, 0, 0, 0, 0, 2, 0, 0, 0 },
{ "Shield, +1, +2, or +3",       0, 0, 0, 0, 0, 0, 2, 0, 0, 1 },
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
