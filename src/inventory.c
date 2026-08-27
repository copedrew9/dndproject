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

/* Builds the menu of everything carried. Returns the number of entries. */
static int carried_menu(const Character *c, const char **opts,
                        char lines[][128], int *map, int max)
{
    int i, n = 0;

    for (i = 0; i < c->item_count && n < max; i++) {
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
            qty = ui_int("  How many", 1, 99);
            add_item(c, map[pick], qty, 0);
            printf("  Added %d x %s.\n", qty, ITEMS[map[pick]].name);
            /* Back to the categories rather than the same long list, so
               there is always a short way out. */
            if (!ui_yesno("  Pick up something else?", 0)) return;
        }
    }
}

static void add_magic(Character *c)
{
    char term[64];

    ui_line("  Search magic items for (blank for all)", term, sizeof term);

    for (;;) {
        const char *opts[REF_MAX + 1];
        static char lines[REF_MAX][REF_LINE];
        int map[REF_MAX], n = 0, i, pick;

        for (i = 0; i < MAGIC_ITEM_COUNT && n < REF_MAX; i++) {
            const MagicItem *m = &MAGIC_ITEMS[i];
            if (!book_enabled(m->book)) continue;
            if (term[0] && !contains_ci(m->name, term)) continue;
            snprintf(lines[n], REF_LINE, "%-34s %-22s %s", m->name, m->rarity,
                     m->attunement ? "attunement" : "");
            opts[n] = lines[n];
            map[n] = i;
            n++;
        }
        if (n == 0) {
            printf("  Nothing matches.\n");
            return;
        }
        opts[n] = "Back";

        pick = ui_menu("  Magic item:", opts, NULL, n + 1);
        if (pick == n) return;

        show_magic_item_detail(map[pick]);
        if (!ui_yesno("  Add it?", 1)) continue;

        {
            int qty = ui_int("  How many", 1, 99);
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

            /* The book's magic weapon entry covers every weapon at once,
               so the copy has to say which one it is before the attacks
               block can add its bonus. */
            if (r && r->weapon) {
                const char *opts[64];
                int k, wn = 0;
                char answer[MAX_NAME];

                for (k = 0; k < c->item_count && wn < 64; k++) {
                    const ItemData *w;
                    if (c->inventory[k].is_magic) continue;
                    w = &ITEMS[c->inventory[k].item_id];
                    if (w->category < ITEM_SIMPLE_MELEE
                        || w->category > ITEM_MARTIAL_RANGED) continue;
                    opts[wn++] = w->name;
                }
                if (wn) {
                    ui_menu_custom("  Which weapon is it?", opts, NULL, wn,
                                   "A weapon you are not carrying yet",
                                   answer, sizeof answer);
                } else {
                    ui_line("  Which weapon is it?", answer, sizeof answer);
                }
                snprintf(c->inventory[c->item_count - 1].variant,
                         sizeof c->inventory[0].variant, "%s", answer);
            }

            /* Armour and rings of resistance are made against one kind of
               damage; which one is a property of the copy. */
            if (r && ((r->resist && !strcmp(r->resist, "*"))
                      || (r->immune && !strcmp(r->immune, "*")))) {
                static const char *const types[] = {
                    "acid", "cold", "fire", "force", "lightning", "necrotic",
                    "poison", "psychic", "radiant", "thunder"
                };
                snprintf(c->inventory[c->item_count - 1].variant,
                         sizeof c->inventory[0].variant, "%s",
                         types[ui_menu("  Made against which damage?", types,
                                       NULL, 10)]);
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

        n = carried_menu(c, opts, lines, map, MAX_ITEMS);
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
static void equip_gear(Character *c)
{
    for (;;) {
        const char *opts[MAX_ITEMS + 1];
        static char lines[MAX_ITEMS][128];
        int map[MAX_ITEMS], n = 0, i, pick;

        for (i = 0; i < c->item_count && n < MAX_ITEMS; i++) {
            if (c->inventory[i].is_magic) {
                /* Magic armour and shields are worn like any other. */
                const MagicRule *r =
                    magic_rule_for(MAGIC_ITEMS[c->inventory[i].item_id].name);
                if (!r || (!r->armor_base && !r->shield)) continue;
            } else {
                ItemCategory cat = ITEMS[c->inventory[i].item_id].category;
                if (cat > ITEM_SHIELD) continue;  /* armour and shields only */
            }
            entry_line(c, i, lines[n], sizeof lines[n]);
            opts[n] = lines[n];
            map[n] = i;
            n++;
        }
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

static void attune_items(Character *c)
{
    for (;;) {
        const char *opts[MAX_ITEMS + 1];
        static char lines[MAX_ITEMS][128];
        int map[MAX_ITEMS], n = 0, i, pick;

        for (i = 0; i < c->item_count && n < MAX_ITEMS; i++) {
            if (!c->inventory[i].is_magic) continue;
            if (!MAGIC_ITEMS[c->inventory[i].item_id].attunement) continue;
            entry_line(c, i, lines[n], sizeof lines[n]);
            opts[n] = lines[n];
            map[n] = i;
            n++;
        }
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

/* ------------------------------------------------------------------- coins */

static void adjust_coins(Character *c)
{
    printf("\n  Enter the new totals; a treasure hoard or a night's "
           "spending both land here.\n");
    c->platinum = ui_int("  Platinum", 0, 999999);
    c->gold     = ui_int("  Gold",     0, 999999);
    c->electrum = ui_int("  Electrum", 0, 999999);
    c->silver   = ui_int("  Silver",   0, 999999);
    c->copper   = ui_int("  Copper",   0, 999999);
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
            "Put something down",
            "Wear or take off armor and shields",
            "Attune to magic items",
            "Set your coins",
            "Done"
        };

        show_carried(c);
        switch (ui_menu("  Inventory:", modes, NULL, 8)) {
        case 0: inventory_reference(c);  break;
        case 1: add_from_catalogue(c);   break;
        case 2: add_magic(c);            break;
        case 3: remove_gear(c);          break;
        case 4: equip_gear(c);           break;
        case 5: attune_items(c);         break;
        case 6: adjust_coins(c);         break;
        default: return;
        }
    }
}
