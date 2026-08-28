/* data_lookup.c -- finding things in the tables.
 *
 * The tables themselves are generated from data/ into src/gen_*.c. The
 * searches over them are code, not data, so they live here and are written
 * by hand.
 *
 * Everything is looked up by name rather than by index. That is what lets a
 * saved character survive a table growing, and what lets homebrew.c splice
 * the DM's own entries onto the end of a bank without anything else
 * noticing.
 */
#include "dnd.h"
#include "data.h"
#include "ui.h"
#include "data_spells.h"

#include <stdio.h>
#include <string.h>

/* --------------------------------------------------------------- classes */

int subclass_by_name(const char *name)
{
    int i;
    for (i = 0; i < SUBCLASS_COUNT; i++) {
        if (strcmp(SUBCLASSES[i].name, name) == 0) return i;
    }
    return -1;
}

int subclass_is(int subclass_id, const char *name)
{
    if (subclass_id < 0 || subclass_id >= SUBCLASS_COUNT) return 0;
    return strcmp(SUBCLASSES[subclass_id].name, name) == 0;
}

/* Subclasses are found by scanning for their owning class rather than by a
   contiguous index window, so a new one can be written anywhere in
   data/character.txt without renumbering anything. */
int subclasses_of(int class_id, int *out, int max)
{
    int i, n = 0;
    for (i = 0; i < SUBCLASS_COUNT && n < max; i++) {
        if (SUBCLASSES[i].class_id == class_id) out[n++] = i;
    }
    return n;
}

int class_by_name(const char *name)
{
    int i;
    for (i = 0; i < CLASS_COUNT; i++) {
        if (strcmp(CLASSES[i].name, name) == 0) return i;
    }
    return -1;
}

/* Ability Score Improvement levels. Every class uses 4/8/12/16/19; the
 * fighter gains two more (6 and 14) and the rogue one more (10). */
int asi_levels_for(int class_id, int out[], int max)
{
    static const int base[] = {4, 8, 12, 16, 19};
    int n = 0, i;

    for (i = 0; i < (int)(sizeof(base) / sizeof(base[0])); i++) {
        if (n < max) out[n++] = base[i];
    }
    if (class_id == CLS_FIGHTER) {
        if (n < max) out[n++] = 6;
        if (n < max) out[n++] = 14;
    } else if (class_id == CLS_ROGUE) {
        if (n < max) out[n++] = 10;
    }
    /* Insertion sort: the list is tiny and must come back ascending. */
    for (i = 1; i < n; i++) {
        int key = out[i], j = i - 1;
        while (j >= 0 && out[j] > key) { out[j + 1] = out[j]; j--; }
        out[j + 1] = key;
    }
    return n;
}

/* ------------------------------------------------------------- equipment */

int find_item(const char *name)
{
    int i;
    for (i = 0; i < ITEM_COUNT; i++) {
        if (strcmp(ITEMS[i].name, name) == 0) return i;
    }
    /* Fall back to a case-insensitive match so saved sheets survive small
       differences in capitalisation. */
    for (i = 0; i < ITEM_COUNT; i++) {
        if (same_fold(ITEMS[i].name, name)) return i;
    }
    return -1;
}

const char *item_notes(const char *name)
{
    int i;
    for (i = 0; i < ITEM_NOTE_COUNT; i++) {
        if (strcmp(ITEM_NOTES[i].item, name) == 0) return ITEM_NOTES[i].text;
    }
    return NULL;
}

int find_magic_item(const char *name)
{
    int i;
    for (i = 0; i < MAGIC_ITEM_COUNT; i++)
        if (strcmp(MAGIC_ITEMS[i].name, name) == 0) return i;
    return -1;
}

/* Whether a rule is armour or a shield, and so does nothing until it is
   worn. Most such rules are recognised by the base Armor Class they set,
   but "Armor, +1, +2, or +3" sets none -- the copy's plus is the whole of
   its effect -- so it has to be recognised by what it is instead: a rule
   that varies from copy to copy and is not a weapon. Without that it was
   never treated as worn at all, and a +3 breastplate sitting in the pack
   raised its owner's Armor Class by three. */
int magic_rule_is_worn(const MagicRule *r)
{
    return r && (r->armor_base || r->shield || r->worn
                 || (r->variable && !r->weapon));
}

/* A field the copy fills in: the "*" that means any damage type, or a list
   of the only ones the book allows. */
int magic_type_is_variant(const char *what)
{
    return what && (!strcmp(what, "*") || strchr(what, ',') != NULL);
}

int magic_variant_types(const MagicRule *r, const char **out, int max)
{
    static const char *const ANY[] = {
        "acid", "cold", "fire", "force", "lightning", "necrotic",
        "poison", "psychic", "radiant", "thunder"
    };
    /* The split is in place, so the list has to be copied first. One item
       is asked about at a time, so one buffer is enough. */
    static char buf[128];
    const char *what = NULL;
    char *cursor, *piece;
    int n = 0;

    if (r && magic_type_is_variant(r->resist)) what = r->resist;
    else if (r && magic_type_is_variant(r->immune)) what = r->immune;
    if (!what) return 0;

    if (!strcmp(what, "*")) {
        for (n = 0; n < (int)(sizeof ANY / sizeof ANY[0]) && n < max; n++) {
            out[n] = ANY[n];
        }
        return n;
    }

    snprintf(buf, sizeof buf, "%s", what);
    cursor = buf;
    while (n < max && (piece = next_csv(&cursor)) != NULL) {
        if (*piece) out[n++] = piece;
    }
    return n;
}

const MagicRule *magic_rule_for(const char *name)
{
    int i;
    for (i = 0; i < MAGIC_RULE_COUNT; i++) {
        if (strcmp(MAGIC_RULES[i].item, name) == 0) return &MAGIC_RULES[i];
    }
    return NULL;
}

/* ----------------------------------------------------------------- world */

int find_beast(const char *name)
{
    int i;
    for (i = 0; i < BEAST_COUNT_ACTUAL; i++)
        if (strcmp(BEASTS[i].name, name) == 0) return i;
    return -1;
}

/* Wild Shape withholds swimmers until 4th level and fliers until 8th, so
 * the menu has to read the speed line. */
int beast_swims(const BeastData *b)
{
    return strstr(b->speed, "swim") != NULL;
}

int beast_flies(const BeastData *b)
{
    return strstr(b->speed, "fly") != NULL;
}

/* --------------------------------------------------- names the data uses */

const char *const ITEM_CATEGORY_NAME[] = {
    "light-armour", "medium-armour", "heavy-armour", "shield",
    "simple-melee", "simple-ranged", "martial-melee", "martial-ranged",
    "gear", "tool", "pack", "mount"
};
const int ITEM_CATEGORY_COUNT =
    (int)(sizeof(ITEM_CATEGORY_NAME) / sizeof(ITEM_CATEGORY_NAME[0]));

const char *const SPELL_CLASS_NAME[] = {
    "bard", "cleric", "druid", "paladin", "ranger", "sorcerer", "warlock",
    "wizard", "artificer"
};
const int SPELL_CLASS_NAME_COUNT =
    (int)(sizeof(SPELL_CLASS_NAME) / sizeof(SPELL_CLASS_NAME[0]));

int same_fold(const char *a, const char *b)
{
    while (*a && *b) {
        int ca = (*a >= 'A' && *a <= 'Z') ? *a + 32 : *a;
        int cb = (*b >= 'A' && *b <= 'Z') ? *b + 32 : *b;
        if (ca != cb) return 0;
        a++; b++;
    }
    return !*a && !*b;
}

int item_category_by_name(const char *s)
{
    int i;
    for (i = 0; i < ITEM_CATEGORY_COUNT; i++)
        if (same_fold(ITEM_CATEGORY_NAME[i], s)) return i;
    return -1;
}

int school_by_name(const char *s)
{
    int i;
    for (i = 0; i < SCHOOL_COUNT; i++)
        if (same_fold(SCHOOL_NAMES[i], s)) return i;
    return -1;
}

/* "bard, wizard" -> the bit mask. An unrecognised name is skipped rather
   than refused: a homebrew line naming a class this build does not have
   should still give up the classes it does. */
unsigned spell_classes_by_name(const char *csv)
{
    unsigned mask = 0;
    char word[32];
    size_t n = 0;
    int i;

    for (;; csv++) {
        if (*csv && *csv != ',') {
            if (*csv == ' ' && n == 0) continue;
            if (n + 1 < sizeof word) word[n++] = *csv;
            continue;
        }
        while (n && word[n - 1] == ' ') n--;
        word[n] = '\0';
        for (i = 0; i < SPELL_CLASS_NAME_COUNT; i++)
            if (same_fold(SPELL_CLASS_NAME[i], word)) mask |= 1u << i;
        n = 0;
        if (!*csv) break;
    }
    return mask;
}

void spell_classes_text(unsigned mask, char *out, size_t n)
{
    size_t used = 0;
    int i;

    if (n) out[0] = '\0';
    for (i = 0; i < SPELL_CLASS_NAME_COUNT; i++) {
        size_t len;
        if (!(mask & (1u << i))) continue;
        len = strlen(SPELL_CLASS_NAME[i]);
        if (used + len + (used ? 1 : 0) + 1 > n) break;
        if (used) out[used++] = ',';
        memcpy(out + used, SPELL_CLASS_NAME[i], len);
        used += len;
        out[used] = '\0';
    }
}

/* The items in one of the PHB's tool groups, in the order they are printed.
   An unknown group name yields every tool, so a proficiency the books word
   in some other way still gets a list to choose from rather than a blank
   prompt. */
int tools_in_group(const char *group, const char **out, int max)
{
    int i, n = 0, any = 0;

    for (i = 0; i < TOOL_GROUP_COUNT && n < max; i++) {
        if (strcmp(TOOL_GROUPS[i].group, group) != 0) continue;
        out[n++] = TOOL_GROUPS[i].item;
        any = 1;
    }
    if (any) return n;

    for (i = 0; i < ITEM_COUNT && n < max; i++) {
        if (ITEMS[i].category == ITEM_TOOL) out[n++] = ITEMS[i].name;
    }
    return n;
}

/* The Random Height and Weight row for a character. A subrace row wins over
   its race's, and a race the table does not cover has none: the newer books
   give height and weight in prose instead of in that table. */
const BodyData *body_for(const char *race, const char *subrace)
{
    const BodyData *fallback = NULL;
    int i;

    for (i = 0; i < BODY_COUNT; i++) {
        if (strcmp(BODIES[i].race, race) != 0) continue;
        if (subrace && BODIES[i].subrace[0]
            && strcmp(BODIES[i].subrace, subrace) == 0) return &BODIES[i];
        if (!BODIES[i].subrace[0]) fallback = &BODIES[i];
    }
    return fallback;
}
