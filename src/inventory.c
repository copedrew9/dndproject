/* inventory.c -- managing what a character carries, after they have it.
 *
 * The equipment step in gear.c settles a character's starting gear once.
 * This is the screen for everything afterwards: picking things up, putting
 * them down, deciding what is actually worn, and attuning to magic items.
 *
 * Nothing here sells anything. Haggling over the price of a used breastplate
 * is a conversation with the DM, not a menu.
 */
#include "dnd.h"
#include "data.h"
#include "build.h"
#include "game.h"
#include "inventory.h"
#include "reference.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>

/* One line describing a carried entry, for the menus below. */
static void entry_line(const Character *c, int i, char *out, size_t n)
{
    const InventoryEntry *e = &c->inventory[i];

    if (e->is_magic) {
        const MagicItem *m = &MAGIC_ITEMS[e->item_id];
        char name[64];
        if (e->plus) {
            snprintf(name, sizeof name, "%s [+%d]", m->name, e->plus);
        } else {
            snprintf(name, sizeof name, "%s", m->name);
        }
        snprintf(out, n, "%2d x %-30s %-14s%s%s", e->quantity, name,
                 m->rarity, e->attuned ? " (attuned)" : "",
                 e->equipped ? " (worn)" : "");
    } else {
        const ItemData *it = &ITEMS[e->item_id];
        snprintf(out, n, "%2d x %-30s %-14s%s", e->quantity, it->name,
                 CATEGORY_LABEL[it->category],
                 e->equipped ? " (worn)" : "");
    }
}

static void show_carried(const Character *c)
{
    int i;

    printf("\n  Carrying %d.%d lb of a %d lb capacity. Purse: %d pp, %d gp, "
           "%d ep, %d sp, %d cp\n",
           current_weight_tenths(c) / 10, current_weight_tenths(c) % 10,
           carrying_capacity(c), c->platinum, c->gold, c->electrum,
           c->silver, c->copper);
    if (current_weight_tenths(c) > carrying_capacity(c) * 10) {
        printf("  You are carrying more than your Strength allows.\n");
    }
    printf("  Attuned to %d of %d magic items. Armor Class %d.\n",
           attuned_count(c), MAX_ATTUNED, armour_class(c));

    if (c->item_count == 0) {
        printf("\n  Nothing carried.\n");
        return;
    }
    printf("\n");
    for (i = 0; i < c->item_count; i++) {
        char line[128];
        entry_line(c, i, line, sizeof line);
        printf("    %s\n", line);
    }
}

/* Builds a menu of the carried entries `wanted` accepts -- everything, when
   it is NULL. Returns how many there are; map[i] is the inventory index of
   menu entry i. The three screens that ask about carried things differ only
   in that filter and in what they say. */
static int carried_menu(const Character *c, int (*wanted)(const Character *,
                                                          int),
                        const char **opts, char lines[][128], int *map,
                        int max)
{
    int i, n = 0;

    for (i = 0; i < c->item_count && n < max; i++) {
        if (wanted && !wanted(c, i)) continue;
        entry_line(c, i, lines[n], 128);
        opts[n] = lines[n];
        map[n] = i;
        n++;
    }
    return n;
}

/* ------------------------------------------------------------ adding gear */

static void add_from_catalogue(Character *c)
{
    for (;;) {
        const char *cats[16];
        int i, cat;

        for (i = 0; i < ITEM_CATEGORY_COUNT; i++) cats[i] = CATEGORY_LABEL[i];
        cats[ITEM_CATEGORY_COUNT] = "Back";

        cat = ui_menu("  Category:", cats, NULL, ITEM_CATEGORY_COUNT + 1);
        if (cat == ITEM_CATEGORY_COUNT) return;

        {
            const char *opts[REF_MAX + 1];
            static char lines[REF_MAX][REF_LINE];
            int map[REF_MAX], n = 0, pick, qty;

            for (i = 0; i < ITEM_COUNT && n < REF_MAX; i++) {
                char price[32];
                if ((int)ITEMS[i].category != cat) continue;
                if (!book_enabled(ITEMS[i].book)) continue;
                format_price(ITEMS[i].cost_cp, price, sizeof price);
                snprintf(lines[n], REF_LINE, "%-30s %8s", ITEMS[i].name,
                         price);
                opts[n] = lines[n];
                map[n] = i;
                n++;
            }
            if (n == 0) continue;
            opts[n] = "Back";

            pick = ui_menu("  Item:", opts, NULL, n + 1);
            if (pick == n) continue;

            show_item_detail(map[pick]);
            if (!ui_yesno("  Add it?", 1)) continue;
            qty = ui_int("  How many", 1, MAX_QUANTITY);
            if (add_gear(c, map[pick], qty, 0) > 1) {
                printf("  Added %d x %s, unpacked into what is in it.\n",
                       qty, ITEMS[map[pick]].name);
            } else {
                printf("  Added %d x %s.\n", qty, ITEMS[map[pick]].name);
            }
            /* Back to the categories rather than the same long list, so
               there is always a short way out. */
            if (!ui_yesno("  Pick up something else?", 0)) return;
        }
    }
}

/* The magic item list, optionally narrowed before it is shown. `preset`
   is NULL when the player is searching for themselves, and a word when the
   caller already knows what they are after -- the potion screen passes
   "potion" so that nobody has to walk past a deck of many things to reach
   a potion of healing. */
static void add_magic_filtered(Character *c, const char *preset)
{
    char term[64];

    if (preset) {
        snprintf(term, sizeof term, "%s", preset);
    } else {
        ui_line("  Search magic items for (blank for all)", term, sizeof term);
    }

    for (;;) {
        const char *opts[REF_MAX + 1];
        static char lines[REF_MAX][REF_LINE];
        int map[REF_MAX], n = 0, i, pick;

        for (i = 0; i < MAGIC_ITEM_COUNT && n < REF_MAX; i++) {
            const MagicItem *m = &MAGIC_ITEMS[i];
            if (!book_enabled(m->book)) continue;
            /* A potion of healing says so in its name; an oil of
               slipperiness and a philter of love do not, and all three are
               filed under Potion in the type. Matching either is what
               makes one search find the lot. */
            if (term[0] && !contains_ci(m->name, term)
                && !contains_ci(m->type, term)) continue;
            snprintf(lines[n], REF_LINE, "%-34s %-22s %s", m->name, m->rarity,
                     m->attunement ? "attunement" : "");
            opts[n] = lines[n];
            map[n] = i;
            n++;
        }
        if (n == 0) {
            /* Two different dead ends read alike otherwise: a search that
               matched nothing, and a bank with nothing in it because the
               book the magic items come from is switched off. The second
               is not the player's typing, and saying so saves them
               retyping it. */
            int any = 0;
            for (i = 0; i < MAGIC_ITEM_COUNT; i++) {
                if (book_enabled(MAGIC_ITEMS[i].book)) { any = 1; break; }
            }
            if (!any) {
                printf("  There are no magic items to choose from: the "
                       "books they come from are switched off in Content "
                       "Settings.\n");
            } else {
                printf("  Nothing matches.\n");
            }
            return;
        }
        opts[n] = "Back";

        pick = ui_menu("  Magic item:", opts, NULL, n + 1);
        if (pick == n) return;

        show_magic_item_detail(map[pick]);
        if (!ui_yesno("  Add it?", 1)) continue;

        {
            int qty = ui_int("  How many", 1, MAX_QUANTITY);
            int attune = 0, plus = 0;
            const MagicItem *m = &MAGIC_ITEMS[map[pick]];
            const MagicRule *r = magic_rule_for(m->name);

            /* An entry covering +1, +2 and +3 has to say which this copy is
               before the sheet can count it. */
            if (r && r->variable) {
                static const char *const which[] = { "+1", "+2", "+3" };
                plus = ui_menu("  Which bonus is this one?", which, NULL, 3)
                     + 1;
            }
            /* A belt of giant strength depends on its giant. */
            if (r && r->sets_ability && r->sets_to == 0) {
                static const char *const giants[] = {
                    "Hill giant (Strength 21)", "Stone giant (Strength 23)",
                    "Frost giant (Strength 23)", "Fire giant (Strength 25)",
                    "Cloud giant (Strength 27)", "Storm giant (Strength 29)"
                };
                static const int scores[] = { 21, 23, 23, 25, 27, 29 };
                plus = scores[ui_menu("  Which giant?", giants, NULL, 6)];
            }
            if (m->attunement) {
                if (attuned_count(c) >= MAX_ATTUNED) {
                    printf("  You are already attuned to %d items, so this "
                           "one is carried unattuned.\n", MAX_ATTUNED);
                } else {
                    attune = ui_yesno("  Attune to it now?", 1);
                }
            }
            add_magic_item(c, map[pick], qty, attune, plus);

            /* An entry that covers a whole class of weapon at once --
               "Weapon (any sword)" -- leaves the weapon to the copy, so
               the copy has to say which before it can have a line in the
               attacks block. An entry that names its weapon does not ask.
               The question is put in the book's own words, so a sword of
               sharpness asks for a sword that deals slashing damage rather
               than for any weapon at all. */
            {
                char narrows[MAX_NAME];
                if (magic_weapon_kind(m->type, narrows, sizeof narrows)
                        == MAGIC_WEAPON_CHOICE) {
                    const char *opts[64];
                    const char *which = narrows;
                    char ask[MAX_NAME + 32];
                    char answer[MAX_NAME];
                    int k, wn = 0;

                    /* "any sword" asks for a sword; a bare "any" asks for
                       a weapon. */
                    if (!strncmp(which, "any", 3) || !strncmp(which, "Any", 3)) {
                        which += 3;
                        while (*which == ' ') which++;
                    }
                    snprintf(ask, sizeof ask, "  Which %s is it?",
                             *which ? which : "weapon");

                    for (k = 0; k < c->item_count && wn < 64; k++) {
                        const ItemData *w;
                        if (c->inventory[k].is_magic) continue;
                        w = &ITEMS[c->inventory[k].item_id];
                        if (w->category < ITEM_SIMPLE_MELEE
                            || w->category > ITEM_MARTIAL_RANGED) continue;
                        opts[wn++] = w->name;
                    }
                    if (wn) {
                        ui_menu_custom(ask, opts, NULL, wn,
                                       "A weapon you are not carrying yet",
                                       answer, sizeof answer);
                    } else {
                        ui_line(ask, answer, sizeof answer);
                    }
                    snprintf(c->inventory[c->item_count - 1].variant,
                             sizeof c->inventory[0].variant, "%s", answer);
                }
            }

            /* Armour and rings of resistance are made against one kind of
               damage; which one is a property of the copy. Which kinds are
               on offer is a property of the item: most take any of the ten,
               but dragon scale mail takes the dragon's and armour of
               vulnerability is made against bludgeoning, piercing or
               slashing, and offering the ten there let a player build a
               suit the DMG does not allow. */
            {
                const char *types[12];
                int nt = magic_variant_types(r, types, 12);
                if (nt > 0) {
                    snprintf(c->inventory[c->item_count - 1].variant,
                             sizeof c->inventory[0].variant, "%s",
                             types[ui_menu("  Made against which damage?",
                                           types, NULL, nt)]);
                }
            }
            printf("  Added %d x %s.\n", qty, m->name);
            if (r && (r->armor_base || r->shield)) {
                printf("  It only counts towards Armor Class once you wear "
                       "it.\n");
            }
            if (!ui_yesno("  Pick up another magic item?", 0)) return;
        }
    }
}

/* ---------------------------------------------------------- removing gear */

static void remove_gear(Character *c)
{
    for (;;) {
        const char *opts[MAX_ITEMS + 1];
        static char lines[MAX_ITEMS][128];
        int map[MAX_ITEMS], n, pick, qty, have;

        n = carried_menu(c, NULL, opts, lines, map, MAX_ITEMS);
        if (n == 0) {
            printf("  Nothing to put down.\n");
            return;
        }
        opts[n] = "Done";

        pick = ui_menu("  Put down which?", opts, NULL, n + 1);
        if (pick == n) return;

        have = c->inventory[map[pick]].quantity;
        qty = (have == 1) ? 1 : ui_int("  How many", 1, have);
        remove_inventory_entry(c, map[pick], qty);
        printf("  Put down %d.\n", qty);
    }
}

/* Unpacking a pack a character is already carrying.
 *
 * A pack taken now comes apart the moment it is acquired, so this is for
 * the sheets written before that: an old character still holds one line
 * reading "Explorer's pack". It is an action rather than something the
 * loader does, because a saved sheet has to reload unchanged --
 * tools/roundtrip.py checks exactly that -- and a loader that quietly
 * expanded a pack would print a different sheet from the one it read. */
static void unpack_carried(Character *c)
{
    for (;;) {
        const char *opts[MAX_ITEMS + 1];
        static char lines[MAX_ITEMS][128];
        int map[MAX_ITEMS], n = 0, i, pick;

        for (i = 0; i < c->item_count && n < MAX_ITEMS; i++) {
            if (c->inventory[i].is_magic) continue;
            if (ITEMS[c->inventory[i].item_id].category != ITEM_PACK) continue;
            snprintf(lines[n], sizeof lines[n], "%d x %s",
                     c->inventory[i].quantity,
                     ITEMS[c->inventory[i].item_id].name);
            opts[n] = lines[n];
            map[n] = i;
            n++;
        }
        if (n == 0) {
            printf("  You are not carrying a pack that is still packed.\n");
            return;
        }
        opts[n] = "Done";

        pick = ui_menu("  Unpack which?", opts, NULL, n + 1);
        if (pick == n) return;

        {
            int at = map[pick];
            int id = c->inventory[at].item_id;
            int qty = c->inventory[at].quantity;
            int parts;

            /* Take the pack off first, so the contents are not competing
               with it for the last inventory slot. */
            remove_inventory_entry(c, at, qty);
            parts = add_pack(c, id, qty);
            if (parts <= 0) {
                /* Nothing known to be in it -- put it back rather than
                   losing it. */
                add_item(c, id, qty, 0);
                printf("  Nothing is recorded as being in %s.\n",
                       ITEMS[id].name);
                return;
            }
            printf("  %s came apart into %d things.\n", ITEMS[id].name,
                   parts);
        }
    }
}

/* ------------------------------------------------------ wearing and wielding */

/* Armour and shields come from two tables now, so these answer the two
   questions the equip screen needs without caring which. */
static int entry_is_shield(const Character *c, int i)
{
    if (c->inventory[i].is_magic) {
        const MagicRule *r =
            magic_rule_for(MAGIC_ITEMS[c->inventory[i].item_id].name);
        return r && r->shield;
    }
    return ITEMS[c->inventory[i].item_id].category == ITEM_SHIELD;
}

static const char *entry_name(const Character *c, int i)
{
    return c->inventory[i].is_magic
         ? MAGIC_ITEMS[c->inventory[i].item_id].name
         : ITEMS[c->inventory[i].item_id].name;
}

/* Only one suit of armour and one shield can be worn at a time, so equipping
 * one takes the other off rather than silently stacking two armour bonuses. */
/* Which kind of armour an entry is -- ITEM_LIGHT_ARMOR through
 * ITEM_SHIELD -- or -1 when it is not armour, or is armour whose own entry
 * does not say which kind.
 *
 * A magic suit carries no category column; what it carries is the Armor
 * Class it sets and the Dexterity it allows, which is the same thing said
 * another way. Full Dexterity is light, a cap is medium, none is heavy --
 * exactly how the equipment table encodes it. The three suits written as
 * "+1, +2, or +3" state no Armor Class at all and so cannot be placed;
 * they answer -1 and are left alone rather than guessed at. So does elven
 * chain, for the opposite reason: it is worn "even if you lack proficiency
 * with medium armor", so there is nothing to warn anyone about.
 */
int entry_armour_category(const Character *c, int i)
{
    if (c->inventory[i].is_magic) {
        const MagicRule *r =
            magic_rule_for(MAGIC_ITEMS[c->inventory[i].item_id].name);

        if (!r) return -1;
        if (r->shield) return ITEM_SHIELD;
        if (r->armor_base <= 0 || r->armor_prof) return -1;
        return r->armor_dex < 0 ? ITEM_LIGHT_ARMOR
             : r->armor_dex > 0 ? ITEM_MEDIUM_ARMOR
                                : ITEM_HEAVY_ARMOR;
    }
    {
        int cat = ITEMS[c->inventory[i].item_id].category;
        return cat <= ITEM_SHIELD ? cat : -1;
    }
}

/* Armour and shields, magical or not. */
static int is_wearable(const Character *c, int i)
{
    if (c->inventory[i].is_magic) {
        const MagicRule *r =
            magic_rule_for(MAGIC_ITEMS[c->inventory[i].item_id].name);
        return magic_rule_is_worn(r);
    }
    return ITEMS[c->inventory[i].item_id].category <= ITEM_SHIELD;
}

static void equip_gear(Character *c)
{
    for (;;) {
        const char *opts[MAX_ITEMS + 1];
        static char lines[MAX_ITEMS][128];
        int map[MAX_ITEMS], n, i, pick;

        n = carried_menu(c, is_wearable, opts, lines, map, MAX_ITEMS);
        if (n == 0) {
            printf("  You have no armor or shield to wear.\n");
            return;
        }
        opts[n] = "Done";

        printf("\n  Armor Class %d.\n", armour_class(c));
        pick = ui_menu("  Wear or take off which?", opts, NULL, n + 1);
        if (pick == n) return;

        {
            int idx = map[pick];
            int is_shield = entry_is_shield(c, idx);
            int wearing = c->inventory[idx].equipped;
            const char *what = entry_name(c, idx);

            if (wearing) {
                c->inventory[idx].equipped = 0;
                printf("  Took off %s.\n", what);
            } else {
                /* The wizard asks before putting armour on that the
                   character cannot use; this screen is the other way to
                   put it on, and asks the same question. Choosing it from
                   a menu is a decision either way, but not always an
                   informed one: what "not proficient" costs -- every
                   Strength and Dexterity check, save and attack at
                   disadvantage, and no spellcasting at all -- is nowhere
                   on this screen. */
                int cat = entry_armour_category(c, idx);

                if (cat >= 0 && !armour_proficient(c, cat)) {
                    char ask[MAX_TEXT];

                    snprintf(ask, sizeof ask,
                             "\n  Do you want to equip the %s? You are not "
                             "proficient with it and so will be hindered "
                             "by it.", what);
                    if (!ui_yesno(ask, 0)) {
                        printf("  Left it where it was.\n");
                        continue;
                    }
                }

                /* One suit of armour and one shield at a time, whether
                   either of them is magical or not. */
                for (i = 0; i < c->item_count; i++) {
                    if (i == idx) continue;
                    if (!c->inventory[i].equipped) continue;
                    if (entry_is_shield(c, i) == is_shield) {
                        c->inventory[i].equipped = 0;
                    }
                }
                c->inventory[idx].equipped = 1;
                printf("  Wearing %s.\n", what);
            }
            printf("  Armor Class is now %d.\n", armour_class(c));
        }
    }
}

/* ---------------------------------------------------------------- attunement */

static int needs_attunement(const Character *c, int i)
{
    return c->inventory[i].is_magic
        && MAGIC_ITEMS[c->inventory[i].item_id].attunement != NULL;
}

static void attune_items(Character *c)
{
    for (;;) {
        const char *opts[MAX_ITEMS + 1];
        static char lines[MAX_ITEMS][128];
        int map[MAX_ITEMS], n, pick;

        n = carried_menu(c, needs_attunement, opts, lines, map, MAX_ITEMS);
        if (n == 0) {
            printf("  You carry nothing that needs attunement.\n");
            return;
        }
        opts[n] = "Done";

        printf("\n  Attuned to %d of %d.\n", attuned_count(c), MAX_ATTUNED);
        pick = ui_menu("  Attune or unattune which?", opts, NULL, n + 1);
        if (pick == n) return;

        {
            int idx = map[pick];
            const MagicItem *m = &MAGIC_ITEMS[c->inventory[idx].item_id];

            if (c->inventory[idx].attuned) {
                c->inventory[idx].attuned = 0;
                printf("  No longer attuned to %s.\n", m->name);
            } else if (attuned_count(c) >= MAX_ATTUNED) {
                printf("  You are already attuned to %d items. Unattune "
                       "from one first.\n", MAX_ATTUNED);
            } else {
                c->inventory[idx].attuned = 1;
                printf("  Attuned to %s. It %s\n", m->name, m->attunement);
            }
        }
    }
}

static void add_magic(Character *c)
{
    add_magic_filtered(c, NULL);
}

/* ------------------------------------------------- gems and oddments */

/* Potions are magic items, and there are 277 of those. Somebody who wants
   a potion of healing should not have to walk past a deck of many things
   to reach it, so the search that add_magic() already takes is simply
   pre-filled -- everything the DMG files under Potion or Oil. */
static void add_potion(Character *c)
{
    printf("\n  Potions and oils, from the Dungeon Master's Guide. They "
           "are magic items like any other; this is the same list with "
           "everything else left out.\n");
    add_magic_filtered(c, "potion");
}

/* Selling is worth its own step because the value is already known: a gem
   is worth what its table says, and the books give no haggling rule for
   one. What the table does with that is the table's business. */
static void sell_valuable(Character *c, int at)
{
    Valuable *v = &c->valuables[at];
    int many = v->quantity;
    long cp, cp_total;

    if (v->value_cp == 0) {
        printf("  %s has no price on it; agree one with your DM and use "
               "the money screen.\n", v->name);
        return;
    }
    if (many > 1) many = ui_int("  Sell how many", 1, v->quantity);
    cp = (long)v->value_cp * many;
    cp_total = cp;              /* the message says what was paid in full */

    /* A sheet holds each coin count in its own field, so a sale that would
       not fit is refused rather than rounded down out of sight. */
    if (purse_in_copper(c) + cp > MAX_PURSE_CP) {
        printf("  You are already carrying all the coin you can; nobody "
               "here can pay you for that.\n");
        return;
    }

    /* Paid in whatever coin divides it, biggest first, which is how a
       merchant would count it out. */
    c->gold += (int)(cp / 100); cp %= 100;
    c->silver += (int)(cp / 10); cp %= 10;
    c->copper += (int)cp;
    {
        char price[32];
        format_price((int)cp_total, price, sizeof price);
        printf("  Sold %d x %s for %s.\n", many, v->name, price);
    }

    v->quantity -= many;
    if (v->quantity <= 0) {
        int i;
        for (i = at; i + 1 < c->valuable_count; i++) {
            c->valuables[i] = c->valuables[i + 1];
        }
        c->valuable_count--;
    }
}

static void add_gem(Character *c)
{
    const char *opts[80];
    static char lines[80][110];
    int map[80], n = 0, i, pick;
    int last = -1;

    for (i = 0; i < GEM_COUNT && n < 78; i++) {
        /* The book prints six tables and the value is the only thing
           separating them, so the value leads the line. */
        if (GEMS[i].value_gp != last) {
            last = GEMS[i].value_gp;
        }
        snprintf(lines[n], sizeof lines[n], "%5d gp  %-18s %s",
                 GEMS[i].value_gp, GEMS[i].name, GEMS[i].description);
        opts[n] = lines[n];
        map[n] = i;
        n++;
    }
    if (n == 0) return;
    opts[n] = "Back";

    pick = ui_menu("  Which stone?", opts, NULL, n + 1);
    if (pick == n) return;

    if (c->valuable_count >= MAX_VALUABLES) {
        printf("  That is as many odd things as this program carries.\n");
        return;
    }
    {
        const GemData *g = &GEMS[map[pick]];
        Valuable *v = &c->valuables[c->valuable_count++];
        memset(v, 0, sizeof *v);
        snprintf(v->name, sizeof v->name, "%s", g->name);
        v->value_cp = g->value_gp * 100;
        v->quantity = ui_int("  How many", 1, MAX_QUANTITY);
        snprintf(v->note, sizeof v->note, "%s", g->description);
        printf("  Added %d x %s.\n", v->quantity, g->name);
    }
}

static void add_oddment(Character *c)
{
    Valuable *v;
    char name[MAX_NAME];

    if (c->valuable_count >= MAX_VALUABLES) {
        printf("  That is as many odd things as this program carries.\n");
        return;
    }
    printf("\n  For the things no table has: a letter of marque, a signet "
           "ring, somebody's locket.\n");
    ui_line("  What is it", name, sizeof name);
    if (!name[0]) return;

    v = &c->valuables[c->valuable_count++];
    memset(v, 0, sizeof *v);
    snprintf(v->name, sizeof v->name, "%s", name);
    v->quantity = ui_int("  How many", 1, MAX_QUANTITY);
    if (ui_yesno("  Is it worth anything in particular?", 0)) {
        int gp = ui_int("  How many gold pieces each", 0, MAX_COINS / 100);
        int cp = ui_int("  And how many copper", 0, 99);
        v->value_cp = gp * 100 + cp;
    }
    ui_line("  A line about it (blank for none)", v->note, sizeof v->note);
    printf("  Added %d x %s.\n", v->quantity, v->name);
}

static void valuables_menu(Character *c)
{
    for (;;) {
        const char *opts[MAX_VALUABLES + 4];
        static char lines[MAX_VALUABLES][110];
        int i, n = 0, pick;

        for (i = 0; i < c->valuable_count; i++) {
            const Valuable *v = &c->valuables[i];
            if (v->value_cp) {
                snprintf(lines[n], sizeof lines[n], "%3d x %-24s %d gp each",
                         v->quantity, v->name, v->value_cp / 100);
            } else {
                snprintf(lines[n], sizeof lines[n], "%3d x %-24s no price",
                         v->quantity, v->name);
            }
            opts[n] = lines[n];
            n++;
        }
        opts[n] = "Add a gemstone";
        opts[n + 1] = "Add something of your own";
        opts[n + 2] = "Back";

        printf("\n  Gems and oddments -- treasure rather than equipment: "
               "nothing here is worn, wielded or weighed.\n");
        pick = ui_menu("  Carried:", opts, NULL, n + 3);
        if (pick == n + 2) return;
        if (pick == n) { add_gem(c); continue; }
        if (pick == n + 1) { add_oddment(c); continue; }

        {
            static const char *const act[] = { "Sell it", "Drop it", "Back" };
            int i2;
            switch (ui_menu("  Do what with it?", act, NULL, 3)) {
            case 0: sell_valuable(c, pick); break;
            case 1:
                for (i2 = pick; i2 + 1 < c->valuable_count; i2++) {
                    c->valuables[i2] = c->valuables[i2 + 1];
                }
                c->valuable_count--;
                break;
            default: break;
            }
        }
    }
}

/* --------------------------------------------------- what the player is told */

static int is_magic_item(const Character *c, int i)
{
    return c->inventory[i].is_magic;
}

/* Hiding and revealing what a magic item is.
 *
 * A DM hands over a rod without saying it is a rod of lordly might, and
 * says so later. Two things can be withheld separately, because they are
 * withheld for different reasons and revealed at different moments: the
 * whole entry, until somebody identifies the thing, and the curse alone,
 * until it bites. Neither changes what the item does. The bonus still
 * reaches Armor Class and the attack line, because the character really is
 * carrying it -- what changes is only what the sheet is willing to say.
 */
static void conceal_items(Character *c)
{
    for (;;) {
        const char *opts[MAX_ITEMS + 1];
        static char lines[MAX_ITEMS][128];
        int map[MAX_ITEMS], n, pick;

        n = carried_menu(c, is_magic_item, opts, lines, map, MAX_ITEMS);
        if (n == 0) {
            printf("  You carry no magic items.\n");
            return;
        }
        opts[n] = "Done";

        printf("\n  A hidden item still does what it does; the sheet just "
               "does not say what.\n");
        pick = ui_menu("  Hide or reveal which?", opts, NULL, n + 1);
        if (pick == n) return;

        {
            InventoryEntry *e = &c->inventory[map[pick]];
            const MagicItem *m = &MAGIC_ITEMS[e->item_id];
            const char *what[4];
            int k = 0, act;
            int can_curse = (m->curse != NULL);

            what[k++] = e->concealed ? "Reveal what it is"
                                     : "Hide everything but the name";
            if (can_curse) {
                what[k++] = e->curse_hidden ? "Reveal the curse"
                                            : "Hide the curse";
            }
            what[k++] = "Back";

            printf("\n  %s: %s%s\n", m->name,
                   e->concealed ? "not yet identified" : "fully written out",
                   can_curse ? (e->curse_hidden ? ", curse hidden"
                                                : ", curse shown")
                             : ", no curse");
            act = ui_menu("  Do what?", what, NULL, k);
            if (act == 0) {
                e->concealed = !e->concealed;
                printf("  %s %s.\n", m->name,
                       e->concealed ? "now prints as a name and nothing else"
                                    : "is written out in full again");
            } else if (can_curse && act == 1) {
                e->curse_hidden = !e->curse_hidden;
                printf("  The curse on %s is now %s.\n", m->name,
                       e->curse_hidden ? "withheld" : "shown");
            }
        }
    }
}

/* ------------------------------------------------------------------- coins */

static void adjust_coins(Character *c)
{
    printf("\n  Enter the new totals; a treasure hoard or a night's "
           "spending both land here.\n");
    c->platinum = ui_int("  Platinum", 0, MAX_COINS);
    c->gold     = ui_int("  Gold",     0, MAX_COINS);
    c->electrum = ui_int("  Electrum", 0, MAX_COINS);
    c->silver   = ui_int("  Silver",   0, MAX_COINS);
    c->copper   = ui_int("  Copper",   0, MAX_COINS);
}

/* --------------------------------------------------------------- the screen */

void manage_inventory(Character *c)
{
    ui_header("Inventory");
    ui_para("Everything this character carries. Items picked up in play go "
            "here, along with what is worn and which magic items are "
            "attuned. Selling and haggling are left to the table.");

    for (;;) {
        static const char *const modes[] = {
            "Look at what you are carrying",
            "Pick up equipment",
            "Pick up a magic item",
            "Pick up a potion",
            "Gems and oddments",
            "Put something down",
            "Unpack a pack you are still carrying",
            "Wear or take off armor and shields",
            "Attune to magic items",
            "Hide or reveal what a magic item is",
            "Set your coins",
            "Done"
        };

        show_carried(c);
        switch (ui_menu("  Inventory:", modes, NULL, 12)) {
        case 0: inventory_reference(c);  break;
        case 1: add_from_catalogue(c);   break;
        case 2: add_magic(c);            break;
        case 3: add_potion(c);           break;
        case 4: valuables_menu(c);       break;
        case 5: remove_gear(c);          break;
        case 6: unpack_carried(c);       break;
        case 7: equip_gear(c);           break;
        case 8: attune_items(c);         break;
        case 9: conceal_items(c);        break;
        case 10: adjust_coins(c);        break;
        default: return;
        }
    }
}
