/* reference.c -- looking things up.
 *
 * The catalogue in data_equipment.c carries the numbers, data_itemtext.c the
 * prose, and data_magicitems.c the Dungeon Master's Guide items. This file
 * is what turns those tables into something a player can read: a detail view
 * for any item, and a browser that reaches it from the shop, from an
 * inventory, and from the main menu.
 *
 * Everything here is read-only. Nothing in this file changes a character.
 */
#include "dnd.h"
#include "data.h"
#include "reference.h"
#include "ui.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

const char *const CATEGORY_LABEL[] = {
    "Light armor", "Medium armor", "Heavy armor", "Shield",
    "Simple melee weapon", "Simple ranged weapon", "Martial melee weapon",
    "Martial ranged weapon", "Adventuring gear", "Tool", "Equipment pack",
    "Mount, tack and vehicle"
};

/* ------------------------------------------------------------ formatting */

void format_price(int cp, char *out, size_t n)
{
    if (cp == 0)            snprintf(out, n, "--");
    else if (cp % 100 == 0) snprintf(out, n, "%d gp", cp / 100);
    else if (cp % 10 == 0)  snprintf(out, n, "%d sp", cp / 10);
    else                    snprintf(out, n, "%d cp", cp);
}

static void format_weight(int tenths, char *out, size_t n)
{
    if (tenths == 0)          snprintf(out, n, "--");
    else if (tenths % 10 == 0) snprintf(out, n, "%d lb", tenths / 10);
    else                       snprintf(out, n, "%d.%d lb",
                                        tenths / 10, tenths % 10);
}

/* Case-insensitive compare; strcasecmp is not in C99. */
static int eq_nocase(const char *a, const char *b)
{
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}


/* --------------------------------------------------- ordinary item detail */

/* Explain each property a weapon lists, so "versatile (1d10)" means
   something to a player who has not memorised chapter 5. */
static void explain_properties(const char *properties)
{
    char buf[160];
    const char *p = properties;
    int shown = 0;

    if (!properties || !properties[0]) return;

    while (*p) {
        size_t k = 0;
        int i;

        while (*p == ' ' || *p == ',') p++;
        while (p[k] && p[k] != ',' && p[k] != '(') k++;
        if (k == 0) { if (*p) p++; continue; }
        if (k >= sizeof buf) k = sizeof buf - 1;
        memcpy(buf, p, k);
        buf[k] = '\0';
        while (k > 0 && buf[k - 1] == ' ') buf[--k] = '\0';
        p += k;
        /* skip any bracketed detail, such as the versatile die */
        if (*p == '(') { while (*p && *p != ')') p++; if (*p) p++; }

        for (i = 0; i < WEAPON_PROPERTY_COUNT; i++) {
            if (eq_nocase(WEAPON_PROPERTIES[i].item, buf)) {
                if (!shown) { printf("\n    Properties\n"); shown = 1; }
                printf("      %s\n", WEAPON_PROPERTIES[i].item);
                ui_wrap(WEAPON_PROPERTIES[i].text, 8);
                break;
            }
        }
    }
}

void show_item_detail(int idx)
{
    const ItemData *it;
    const char *note;
    char price[32], weight[32];

    if (idx < 0 || idx >= ITEM_COUNT) return;
    it = &ITEMS[idx];

    format_price(it->cost_cp, price, sizeof price);
    format_weight(it->weight_tenths, weight, sizeof weight);

    printf("\n  %s\n", it->name);
    printf("    %s -- %s, %s (%s)\n",
           CATEGORY_LABEL[it->category], price, weight,
           BOOK_ABBREV[it->book]);

    if (it->base_ac) {
        if (it->category == ITEM_SHIELD) {
            printf("\n    Armor Class  +%d, on top of your own AC\n",
                   it->base_ac);
        } else if (it->dex_cap < 0) {
            printf("\n    Armor Class  %d + your full Dexterity modifier\n",
                   it->base_ac);
        } else if (it->dex_cap == 0) {
            printf("\n    Armor Class  %d, with no Dexterity bonus\n",
                   it->base_ac);
        } else {
            printf("\n    Armor Class  %d + Dexterity modifier (max +%d)\n",
                   it->base_ac, it->dex_cap);
        }
        if (it->str_req) {
            printf("    Strength     %d required, or your speed drops by "
                   "10 feet\n", it->str_req);
        }
        if (it->stealth_disadvantage) {
            printf("    Stealth      disadvantage on Dexterity (Stealth) "
                   "checks\n");
        }
    }

    if (it->damage[0]) {
        printf("\n    Damage       %s %s\n", it->damage, it->damage_type);
        if (it->properties[0]) printf("    Listed as    %s\n", it->properties);
        explain_properties(it->properties);
    }

    if (it->contents[0]) {
        printf("\n    Contents\n");
        ui_wrap(it->contents, 6);
    }

    note = item_notes(it->name);
    if (note) {
        printf("\n");
        ui_wrap(note, 4);
    } else if (!it->damage[0] && !it->base_ac && !it->contents[0]) {
        printf("\n    No special rules of its own; it is ordinary gear "
               "whose use is up to you and the DM.\n");
    }
}

/* ------------------------------------------------------- magic item detail */

void show_magic_item_detail(int idx)
{
    const MagicItem *m;

    if (idx < 0 || idx >= MAGIC_ITEM_COUNT) return;
    m = &MAGIC_ITEMS[idx];

    printf("\n  %s\n", m->name);
    printf("    %s, %s%s%s (%s)\n", m->type, m->rarity,
           m->attunement ? " -- " : "",
           m->attunement ? m->attunement : "",
           BOOK_ABBREV[m->book]);
    printf("\n");
    ui_wrap(m->text, 4);
}

/* ------------------------------------------------------------- the browser */

/* Show a list of catalogue items and let one be inspected, repeatedly. */
static void browse_items(const int *map, const char *const *labels, int n,
                         const char *prompt)
{
    if (n == 0) {
        printf("  Nothing matches.\n");
        return;
    }
    for (;;) {
        const char *opts[REF_MAX + 1];
        int i, pick;

        for (i = 0; i < n; i++) opts[i] = labels[i];
        opts[n] = "Back";
        pick = ui_menu(prompt, opts, NULL, n + 1);
        if (pick == n) return;
        show_item_detail(map[pick]);
    }
}

static int collect_items(int category, const char *filter, int *map,
                         char lines[][REF_LINE], const char **labels)
{
    int i, n = 0;

    for (i = 0; i < ITEM_COUNT && n < REF_MAX; i++) {
        char price[32], weight[32];
        if (category >= 0 && (int)ITEMS[i].category != category) continue;
        if (!book_enabled(ITEMS[i].book)) continue;
        if (filter && !contains_ci(ITEMS[i].name, filter)) continue;

        format_price(ITEMS[i].cost_cp, price, sizeof price);
        format_weight(ITEMS[i].weight_tenths, weight, sizeof weight);
        snprintf(lines[n], REF_LINE, "%-26s %8s %8s", ITEMS[i].name, price,
                 weight);
        labels[n] = lines[n];
        map[n] = i;
        n++;
    }
    return n;
}

static void browse_equipment(void)
{
    for (;;) {
        const char *cats[16];
        int i, pick;

        for (i = 0; i < 12; i++) cats[i] = CATEGORY_LABEL[i];
        cats[12] = "Search by name";
        cats[13] = "Back";

        pick = ui_menu("  Equipment:", cats, NULL, 14);
        if (pick == 13) return;

        {
            static char lines[REF_MAX][REF_LINE];
            const char *labels[REF_MAX];
            int map[REF_MAX], n;

            if (pick == 12) {
                char term[64];
                ui_line("  Search for", term, sizeof term);
                n = collect_items(-1, term, map, lines, labels);
            } else {
                n = collect_items(pick, NULL, map, lines, labels);
            }
            browse_items(map, labels, n, "  Item:");
        }
    }
}

static void browse_magic_items(void)
{
    static const char *const kinds[] = {
        "Armor", "Weapon", "Potion", "Ring", "Rod", "Scroll", "Staff",
        "Wand", "Wondrous item"
    };
    static const char *const rarities[] = {
        "common", "uncommon", "rare", "very rare", "legendary", "artifact"
    };

    for (;;) {
        static const char *const modes[] = {
            "Browse by kind", "Browse by rarity", "Only items needing "
            "attunement", "Search by name", "Show every magic item", "Back"
        };
        int mode = ui_menu("  Magic items:", modes, NULL, 6);
        int want_kind = -1, want_rarity = -1, want_attune = 0;
        char term[64];

        term[0] = '\0';
        if (mode == 5) return;
        if (mode == 0) want_kind = ui_menu("  Kind:", kinds, NULL, 9);
        else if (mode == 1) want_rarity = ui_menu("  Rarity:", rarities, NULL, 6);
        else if (mode == 2) want_attune = 1;
        else if (mode == 3) ui_line("  Search for", term, sizeof term);

        for (;;) {
            static char lines[REF_MAX][REF_LINE];
            const char *opts[REF_MAX + 1];
            int map[REF_MAX], n = 0, i, pick;

            for (i = 0; i < MAGIC_ITEM_COUNT && n < REF_MAX; i++) {
                const MagicItem *m = &MAGIC_ITEMS[i];
                if (!book_enabled(m->book)) continue;
                if (want_kind >= 0 &&
                    strncmp(m->type, kinds[want_kind],
                            strlen(kinds[want_kind])) != 0) continue;
                if (want_rarity >= 0 &&
                    !contains_ci(m->rarity, rarities[want_rarity]))
                    continue;
                if (want_attune && !m->attunement) continue;
                if (term[0] && !contains_ci(m->name, term)) continue;

                snprintf(lines[n], REF_LINE, "%-34s %-22s %s", m->name,
                         m->rarity, m->attunement ? "attunement" : "");
                opts[n] = lines[n];
                map[n] = i;
                n++;
            }
            if (n == 0) { printf("  Nothing matches.\n"); break; }
            opts[n] = "Back";
            pick = ui_menu("  Magic item:", opts, NULL, n + 1);
            if (pick == n) break;
            show_magic_item_detail(map[pick]);
        }
    }
}

static void browse_properties(void)
{
    for (;;) {
        const char *opts[24];
        int i, pick;

        for (i = 0; i < WEAPON_PROPERTY_COUNT; i++)
            opts[i] = WEAPON_PROPERTIES[i].item;
        opts[WEAPON_PROPERTY_COUNT] = "Back";

        pick = ui_menu("  Weapon property:", opts, NULL,
                       WEAPON_PROPERTY_COUNT + 1);
        if (pick == WEAPON_PROPERTY_COUNT) return;
        printf("\n  %s\n", WEAPON_PROPERTIES[pick].item);
        ui_wrap(WEAPON_PROPERTIES[pick].text, 4);
    }
}

static void browse_lifestyles(void)
{
    int i;
    printf("\n  Lifestyle expenses, per day\n\n");
    for (i = 0; i < LIFESTYLE_COUNT; i++) {
        char price[32];
        format_price(LIFESTYLES[i].cost_cp_per_day, price, sizeof price);
        printf("  %-14s %-10s", LIFESTYLES[i].name,
               LIFESTYLES[i].cost_cp_per_day ? price : "free");
        printf("\n");
        ui_wrap(LIFESTYLES[i].text, 6);
        printf("\n");
    }
    printf("  A month between adventures costs thirty times the daily "
           "price.\n");
}

static void browse_prices(void)
{
    int i;
    printf("\n  Food, drink, lodging and services\n\n");
    for (i = 0; i < SERVICE_COUNT; i++) {
        char price[32];
        format_price(SERVICES[i].cost_cp, price, sizeof price);
        printf("    %-38s %10s\n", SERVICES[i].name, price);
    }
    printf("\n  Hiring a spellcaster (the caster's fee; you supply any "
           "costly material component)\n\n");
    for (i = 0; i < SPELLCASTING_SERVICE_COUNT; i++) {
        char price[32];
        format_price(SPELLCASTING_SERVICES[i].cost_cp, price, sizeof price);
        printf("    %-38s %10s\n", SPELLCASTING_SERVICES[i].name, price);
    }
}

static void browse_trinkets(void)
{
    static const char *const modes[] = {
        "Roll one at random", "Show the whole table", "Back"
    };
    int mode = ui_menu("  Trinkets:", modes, NULL, 3);
    int i;

    if (mode == 2) return;
    if (mode == 0) {
        i = roll_die(100) - 1;
        printf("\n  %d: ", i + 1);
        ui_wrap(TRINKETS[i], 6);
        return;
    }
    printf("\n");
    for (i = 0; i < TRINKET_COUNT; i++) {
        printf("  %3d ", i + 1);
        ui_wrap(TRINKETS[i], 6);
    }
}

/* The conditions of appendix A, which is the page a table turns to most
   often once play has started. */
static void browse_conditions(void)
{
    for (;;) {
        const char *opts[32];
        int i, n = 0;

        for (i = 0; i < CONDITION_COUNT && n < 31; i++) {
            opts[n++] = CONDITIONS[i].name;
        }
        opts[n] = "Back";
        i = ui_menu("  Condition:", opts, NULL, n + 1);
        if (i == n) return;

        printf("\n  %s\n", CONDITIONS[i].name);
        {
            char buf[1024];
            const char *parts[16];
            int k, count = split_pipe(CONDITIONS[i].effects, buf, sizeof buf,
                                      parts, 16);
            for (k = 0; k < count; k++) {
                char line[MAX_TEXT + 8];
                snprintf(line, sizeof line, "- %s", parts[k]);
                ui_wrap(line, 4);
            }
        }
        printf("\n");
    }
}
void reference_menu(void)
{
    ui_header("Reference");
    ui_para("Everything the books say about a piece of equipment: what it "
            "costs and weighs, what its stat line means, and what it does "
            "when it has no stat line at all. And the conditions, for when "
            "something has just gone wrong.");

    for (;;) {
        static const char *const modes[] = {
            "Equipment, weapons and armor",
            "Magic items",
            "Weapon properties explained",
            "Trinkets",
            "Lifestyle expenses",
            "Food, lodging, services and spellcasting",
            "Conditions",
            "Back"
        };
        switch (ui_menu("  Look up:", modes, NULL, 8)) {
        case 0: browse_equipment();   break;
        case 1: browse_magic_items(); break;
        case 2: browse_properties();  break;
        case 3: browse_trinkets();    break;
        case 4: browse_lifestyles();  break;
        case 5: browse_prices();      break;
        case 6: browse_conditions();  break;
        default: return;
        }
    }
}

/* --------------------------------------------------- a character's own kit */

void inventory_reference(const Character *c)
{
    for (;;) {
        static char lines[REF_MAX][REF_LINE];
        const char *opts[REF_MAX + 1];
        int map[REF_MAX], n = 0, i, pick;

        for (i = 0; i < c->item_count && n < REF_MAX; i++) {
            if (c->inventory[i].is_magic) {
                const MagicItem *m = &MAGIC_ITEMS[c->inventory[i].item_id];
                snprintf(lines[n], REF_LINE, "%2d x %-26s%s",
                         c->inventory[i].quantity, m->name,
                         c->inventory[i].attuned ? "  (attuned)" : "");
            } else {
                const ItemData *it = &ITEMS[c->inventory[i].item_id];
                snprintf(lines[n], REF_LINE, "%2d x %-26s%s",
                         c->inventory[i].quantity, it->name,
                         c->inventory[i].equipped ? "  (worn)" : "");
            }
            opts[n] = lines[n];
            /* Remember the inventory slot, not the table index, so the
               detail view knows which table to read. */
            map[n] = i;
            n++;
        }
        if (n == 0) {
            printf("  You are not carrying anything yet.\n");
            return;
        }
        opts[n] = "Done";
        pick = ui_menu("  Which item?", opts, NULL, n + 1);
        if (pick == n) return;
        if (c->inventory[map[pick]].is_magic) {
            show_magic_item_detail(c->inventory[map[pick]].item_id);
        } else {
            show_item_detail(c->inventory[map[pick]].item_id);
        }
    }
}
