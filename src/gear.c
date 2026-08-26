/* gear.c -- starting equipment, the shop, and personality (PHB chapters 4-5). */
#include "build.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *const CATEGORY_NAME[] = {
    "Light armor", "Medium armor", "Heavy armor", "Shield",
    "Simple melee weapons", "Simple ranged weapons",
    "Martial melee weapons", "Martial ranged weapons",
    "Adventuring gear", "Tools", "Equipment packs", "Mounts and tack"
};

static void print_price(int cp, char *out, size_t n)
{
    if (cp >= 100 && cp % 100 == 0) snprintf(out, n, "%d gp", cp / 100);
    else if (cp >= 100)             snprintf(out, n, "%d.%02d gp", cp / 100, cp % 100);
    else if (cp >= 10 && cp % 10 == 0) snprintf(out, n, "%d sp", cp / 10);
    else                            snprintf(out, n, "%d cp", cp);
}

static void print_weight(int tenths, char *out, size_t n)
{
    if (tenths == 0)          snprintf(out, n, "--");
    else if (tenths % 10 == 0) snprintf(out, n, "%d lb", tenths / 10);
    else                       snprintf(out, n, "%d.%d lb", tenths / 10, tenths % 10);
}

/* Equips the best armour and a shield if the character owns any. */
static void auto_equip(Character *c)
{
    int i, best = -1, best_ac = -1;

    for (i = 0; i < c->item_count; i++) {
        const ItemData *it = &ITEMS[c->inventory[i].item_id];
        int ac;
        if (it->category > ITEM_HEAVY_ARMOR) continue;

        ac = it->base_ac
           + ((it->dex_cap < 0) ? ability_mod(c, ABL_DEX)
             : (it->dex_cap == 0) ? 0
             : (ability_mod(c, ABL_DEX) < it->dex_cap
                ? ability_mod(c, ABL_DEX) : it->dex_cap));
        if (ac > best_ac) { best_ac = ac; best = i; }
    }
    for (i = 0; i < c->item_count; i++) {
        const ItemData *it = &ITEMS[c->inventory[i].item_id];
        if (it->category <= ITEM_HEAVY_ARMOR) {
            c->inventory[i].equipped = (i == best);
        } else if (it->category == ITEM_SHIELD) {
            c->inventory[i].equipped = 1;
        }
    }
}

/* Turning the PHB's starting-equipment wording into real items.
 *
 * A class entry reads like "(a) a martial weapon and a shield or (b) two
 * martial weapons". Once an alternative is chosen it is split into phrases,
 * and each phrase becomes either a catalogue item (with a quantity) or, when
 * it names a whole category such as "any simple weapon", a menu to pick from.
 */

struct alias { const char *phrase; const char *item; int qty; };

/* Plurals and wordings that do not match a catalogue name directly. */
static const struct alias ALIASES[] = {
    { "handaxes",        "Handaxe",             1 },
    { "shortswords",     "Shortsword",          1 },
    { "javelins",        "Javelin",             1 },
    { "daggers",         "Dagger",              1 },
    { "darts",           "Dart",                1 },
    { "arrows",          "Arrows (20)",         0 },
    { "bolts",           "Crossbow bolts (20)", 0 },
    { "quiver of 20 arrows", "Arrows (20)",     1 },
    { "wooden shield",   "Shield",              1 },
    { "leather armor",   "Leather armor",       1 },
    { "chain mail",      "Chain mail",          1 },
    { "scale mail",      "Scale mail",          1 },
    { "spellbook",       "Spellbook",           1 },
    { "component pouch", "Component pouch",     1 },
    { "thieves' tools",  "Thieves' tools",      1 },
    { "priest's pack",   "Priest's pack",       1 },
};

/* Category phrases that open a menu, with the catalogue categories they
 * draw from (a second category of -1 means just the one). */
struct category { const char *phrase; int cat_a; int cat_b; const char *label; };

static const struct category CATEGORIES[] = {
    { "any martial melee weapon", ITEM_MARTIAL_MELEE, -1, "martial melee weapon" },
    { "any martial weapon",  ITEM_MARTIAL_MELEE, ITEM_MARTIAL_RANGED, "martial weapon" },
    { "martial weapon",      ITEM_MARTIAL_MELEE, ITEM_MARTIAL_RANGED, "martial weapon" },
    { "martial weapons",     ITEM_MARTIAL_MELEE, ITEM_MARTIAL_RANGED, "martial weapon" },
    { "any simple melee weapon", ITEM_SIMPLE_MELEE, -1, "simple melee weapon" },
    { "simple melee weapons",ITEM_SIMPLE_MELEE, -1, "simple melee weapon" },
    { "any simple weapon",   ITEM_SIMPLE_MELEE, ITEM_SIMPLE_RANGED, "simple weapon" },
    { "simple weapon",       ITEM_SIMPLE_MELEE, ITEM_SIMPLE_RANGED, "simple weapon" },
    { "simple weapons of your choice", ITEM_SIMPLE_MELEE, ITEM_SIMPLE_RANGED, "simple weapon" },
    { "martial weapons of your choice", ITEM_MARTIAL_MELEE, ITEM_MARTIAL_RANGED, "martial weapon" },
};

static void lowercase(const char *src, char *dst, size_t n)
{
    size_t i;
    for (i = 0; src[i] && i + 1 < n; i++) {
        int ch = (unsigned char)src[i];
        dst[i] = (char)((ch >= 'A' && ch <= 'Z') ? ch + 32 : ch);
    }
    dst[i] = '\0';
}

/* Strips a leading count, returning it; "two handaxes" gives 2. */
static int leading_quantity(const char **p)
{
    static const struct { const char *word; int n; } WORDS[] = {
        { "two ", 2 }, { "three ", 3 }, { "four ", 4 }, { "five ", 5 },
        { "ten ", 10 },
    };
    const char *s = *p;
    size_t i;

    if (*s >= '0' && *s <= '9') {
        int n = 0;
        while (*s >= '0' && *s <= '9') n = n * 10 + (*s++ - '0');
        while (*s == ' ') s++;
        *p = s;
        return n;
    }
    for (i = 0; i < sizeof WORDS / sizeof WORDS[0]; i++) {
        size_t len = strlen(WORDS[i].word);
        if (strncmp(s, WORDS[i].word, len) == 0) {
            *p = s + len;
            return WORDS[i].n;
        }
    }
    return 1;
}

/* Offers every item in one or two catalogue categories. */
static void pick_from_category(Character *c, int cat_a, int cat_b,
                               const char *label)
{
    const char *opts[128];
    static char lines[128][96];
    int map[128], n = 0, i, pick;
    char prompt[96];

    for (i = 0; i < ITEM_COUNT && n < 128; i++) {
        if ((int)ITEMS[i].category != cat_a
            && (cat_b < 0 || (int)ITEMS[i].category != cat_b)) continue;
        snprintf(lines[n], sizeof lines[n], "%-20s %s %s", ITEMS[i].name,
                 ITEMS[i].damage, ITEMS[i].damage_type);
        opts[n] = lines[n];
        map[n] = i;
        n++;
    }
    if (n == 0) return;

    snprintf(prompt, sizeof prompt, "      Choose a %s:", label);
    pick = ui_menu(prompt, opts, NULL, n);
    add_item(c, map[pick], 1, 0);
    printf("      Added %s.\n", ITEMS[map[pick]].name);
}

/* Handles the "one of a kind" phrases the PHB uses for spellcasting foci
 * and instruments. Returns 1 when the phrase was recognised. */
static int pick_special(Character *c, const char *low)
{
    struct spec { const char *phrase; const char *const *names; int count;
                  const char *label; };
    static const char *const HOLY[] = {
        "Amulet (holy symbol)", "Emblem (holy symbol)",
        "Reliquary (holy symbol)" };
    static const char *const ARCANE[] = {
        "Crystal (arcane focus)", "Orb (arcane focus)", "Rod (arcane focus)",
        "Staff (arcane focus)", "Wand (arcane focus)" };
    static const char *const DRUIDIC[] = {
        "Sprig of mistletoe (druidic focus)", "Wooden staff (druidic focus)",
        "Yew wand (druidic focus)" };
    static const char *const INSTRUMENTS[] = {
        "Bagpipes", "Drum", "Dulcimer", "Flute", "Lute", "Lyre", "Horn",
        "Pan flute", "Shawm", "Viol" };
    static const struct spec SPECS[] = {
        { "holy symbol", HOLY, 3, "holy symbol" },
        { "arcane focus", ARCANE, 5, "arcane focus" },
        { "druidic focus", DRUIDIC, 3, "druidic focus" },
        { "musical instrument", INSTRUMENTS, 10, "musical instrument" },
    };
    size_t i;

    for (i = 0; i < sizeof SPECS / sizeof SPECS[0]; i++) {
        char prompt[96];
        int pick;
        if (!strstr(low, SPECS[i].phrase)) continue;
        snprintf(prompt, sizeof prompt, "      Choose a %s:", SPECS[i].label);
        pick = ui_menu(prompt, SPECS[i].names, NULL, SPECS[i].count);
        add_item_by_name(c, SPECS[i].names[pick], 1, 0);
        printf("      Added %s.\n", SPECS[i].names[pick]);
        return 1;
    }
    return 0;
}

/* Resolves one phrase, such as "two handaxes" or "a shield". */
static void add_phrase(Character *c, const char *phrase)
{
    char low[256];
    const char *p;
    int qty, id;
    size_t i;

    lowercase(phrase, low, sizeof low);
    p = low;
    while (*p == ' ') p++;
    if (strncmp(p, "a ", 2) == 0) p += 2;
    else if (strncmp(p, "an ", 3) == 0) p += 3;
    else if (strncmp(p, "the ", 4) == 0) p += 4;
    while (*p == ' ') p++;
    if (!*p) return;

    qty = leading_quantity(&p);
    if (strncmp(p, "any ", 4) == 0) p += 4;

    for (i = 0; i < sizeof CATEGORIES / sizeof CATEGORIES[0]; i++) {
        if (strcmp(p, CATEGORIES[i].phrase) == 0) {
            int k;
            for (k = 0; k < qty; k++) {
                pick_from_category(c, CATEGORIES[i].cat_a,
                                   CATEGORIES[i].cat_b, CATEGORIES[i].label);
            }
            return;
        }
    }
    if (pick_special(c, p)) return;

    for (i = 0; i < sizeof ALIASES / sizeof ALIASES[0]; i++) {
        if (strcmp(p, ALIASES[i].phrase) != 0) continue;
        id = find_item(ALIASES[i].item);
        if (id >= 0) {
            /* qty 0 in the table means the entry already bundles the count. */
            add_item(c, id, ALIASES[i].qty ? qty : 1, 0);
            printf("      Added %s.\n", ITEMS[id].name);
        }
        return;
    }

    id = find_item(p);
    if (id < 0) {
        /* Try the singular: "javelins" -> "javelin". */
        size_t len = strlen(p);
        if (len > 1 && p[len - 1] == 's') {
            char singular[256];
            memcpy(singular, p, len - 1);
            singular[len - 1] = '\0';
            id = find_item(singular);
        }
    }
    if (id >= 0) {
        add_item(c, id, qty, 0);
        printf("      Added %d x %s.\n", qty, ITEMS[id].name);
    } else {
        add_choice(c, "Equipment", phrase);
        printf("      Noted: %s\n", phrase);
    }
}

/* Splits "a martial weapon and a shield" into its phrases. */
static void add_all_phrases(Character *c, const char *text)
{
    char buf[512];
    char *p, *start;

    strncpy(buf, text, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';

    start = buf;
    p = buf;
    while (*p) {
        char *sep = NULL;
        if (strncmp(p, " and ", 5) == 0) sep = p;
        else if (*p == ',') sep = p;

        if (sep) {
            int skip = (*sep == ',') ? 1 : 5;
            *sep = '\0';
            add_phrase(c, start);
            p = sep + skip;
            while (*p == ' ') p++;
            start = p;
            continue;
        }
        p++;
    }
    if (*start) add_phrase(c, start);
}

/* Reads a starting-equipment line, offering the lettered alternatives when
 * the PHB gives a choice, then resolving whatever was chosen into items. */
static void resolve_equipment_line(Character *c, const char *line)
{
    char buf[512];
    const char *opts[8];
    char parts[8][160];
    int n = 0;
    const char *p;

    if (strstr(line, "(a)") == NULL) {
        printf("    You also start with: ");
        ui_wrap(line, 0);
        add_all_phrases(c, line);
        return;
    }

    strncpy(buf, line, sizeof buf - 1);
    buf[sizeof buf - 1] = '\0';

    p = buf;
    while (p && *p && n < 8) {
        const char *next = strstr(p, " or (");
        size_t len = next ? (size_t)(next - p) : strlen(p);
        if (len >= sizeof parts[0]) len = sizeof parts[0] - 1;
        memcpy(parts[n], p, len);
        parts[n][len] = '\0';
        opts[n] = parts[n];
        n++;
        p = next ? next + 4 : NULL;
    }

    if (n <= 1) {
        add_all_phrases(c, line);
        return;
    }
    {
        int pick = ui_menu("    Choose:", opts, NULL, n);
        const char *t = strchr(parts[pick], ')');
        t = t ? t + 1 : parts[pick];
        while (*t == ' ') t++;
        add_all_phrases(c, t);
    }
}

static void take_starting_package(Character *c)
{
    int i;

    for (i = 0; i < c->class_count; i++) {
        char buf[1024];
        const char *lines[12];
        int n, k;

        if (i > 0) {
            printf("\n  (Multiclassing grants no starting equipment for %s.)\n",
                   CLASSES[c->classes[i].class_id].name);
            break;
        }
        printf("\n  %s starting equipment:\n",
               CLASSES[c->classes[i].class_id].name);
        n = split_pipe(CLASSES[c->classes[i].class_id].equipment,
                       buf, sizeof buf, lines, 12);
        for (k = 0; k < n; k++) resolve_equipment_line(c, lines[k]);
    }

    if (c->background_id >= 0) {
        printf("\n  %s background equipment:\n",
               BACKGROUNDS[c->background_id].name);
        printf("    ");
        ui_wrap(BACKGROUNDS[c->background_id].equipment, 4);
        add_choice(c, "Background equipment",
                   BACKGROUNDS[c->background_id].equipment);
        c->gold += BACKGROUNDS[c->background_id].gold;
        printf("    Plus %d gp.\n", BACKGROUNDS[c->background_id].gold);
    }
}

static void buy_with_gold(Character *c)
{
    const ClassData *cd = &CLASSES[c->classes[0].class_id];
    int rolled;

    printf("\n  A %s begins with %dd4", cd->name, cd->gold_dice);
    if (cd->gold_mult != 1) printf(" x %d", cd->gold_mult);
    printf(" gp.\n");

    if (ui_yesno("  Roll for starting gold?", 1)) {
        rolled = roll_dice(cd->gold_dice, 4) * cd->gold_mult;
        printf("    Rolled %d gp.\n", rolled);
    } else {
        /* The average of d4 is 2.5; use the rounded expected value. */
        rolled = (cd->gold_dice * 5 / 2) * cd->gold_mult;
        printf("    Taking the average: %d gp.\n", rolled);
    }
    c->gold += rolled;

    if (c->background_id >= 0) {
        printf("\n  Your background also grants its equipment and %d gp.\n",
               BACKGROUNDS[c->background_id].gold);
        ui_wrap(BACKGROUNDS[c->background_id].equipment, 4);
        add_choice(c, "Background equipment",
                   BACKGROUNDS[c->background_id].equipment);
        c->gold += BACKGROUNDS[c->background_id].gold;
    }
}

/* The shop, usable both after taking a package and after buying with gold. */
static void shop(Character *c)
{
    for (;;) {
        const char *cats[16];
        char catlabel[16][48];
        int i, cat, purse_cp;

        purse_cp = c->gold * 100 + c->silver * 10 + c->copper
                 + c->electrum * 50 + c->platinum * 1000;

        printf("\n  Purse: %d gp %d sp %d cp   Carrying %d.%d of %d lb\n",
               c->gold, c->silver, c->copper,
               current_weight_tenths(c) / 10, current_weight_tenths(c) % 10,
               carrying_capacity(c));

        if (!ui_yesno("  Buy or add equipment?", 0)) return;

        for (i = 0; i < 12; i++) {
            snprintf(catlabel[i], sizeof catlabel[i], "%s", CATEGORY_NAME[i]);
            cats[i] = catlabel[i];
        }
        cats[12] = "Done shopping";
        cat = ui_menu("  Category:", cats, NULL, 13);
        if (cat == 12) return;

        {
            const char *opts[256];
            static char lines[256][160];
            int map[256], n = 0, pick, qty;

            for (i = 0; i < ITEM_COUNT && n < 256; i++) {
                char price[32], weight[32];
                if ((int)ITEMS[i].category != cat) continue;
                print_price(ITEMS[i].cost_cp, price, sizeof price);
                print_weight(ITEMS[i].weight_tenths, weight, sizeof weight);

                if (ITEMS[i].damage[0]) {
                    snprintf(lines[n], sizeof lines[n],
                             "%-24s %8s %8s  %s %s%s%s",
                             ITEMS[i].name, price, weight,
                             ITEMS[i].damage, ITEMS[i].damage_type,
                             ITEMS[i].properties[0] ? " -- " : "",
                             ITEMS[i].properties);
                } else if (ITEMS[i].base_ac) {
                    char acbuf[48];
                    if (ITEMS[i].category == ITEM_SHIELD) {
                        snprintf(acbuf, sizeof acbuf, "AC +%d", ITEMS[i].base_ac);
                    } else if (ITEMS[i].dex_cap < 0) {
                        snprintf(acbuf, sizeof acbuf, "AC %d + Dex",
                                 ITEMS[i].base_ac);
                    } else if (ITEMS[i].dex_cap == 0) {
                        snprintf(acbuf, sizeof acbuf, "AC %d", ITEMS[i].base_ac);
                    } else {
                        snprintf(acbuf, sizeof acbuf, "AC %d + Dex (max %d)",
                                 ITEMS[i].base_ac, ITEMS[i].dex_cap);
                    }
                    snprintf(lines[n], sizeof lines[n],
                             "%-24s %8s %8s  %s%s%s",
                             ITEMS[i].name, price, weight, acbuf,
                             ITEMS[i].str_req ? ", Str " : "",
                             ITEMS[i].str_req ? (ITEMS[i].str_req == 13 ? "13"
                                                                        : "15")
                                              : "");
                } else {
                    snprintf(lines[n], sizeof lines[n], "%-24s %8s %8s",
                             ITEMS[i].name, price, weight);
                }
                opts[n] = lines[n];
                map[n] = i;
                n++;
            }
            if (n == 0) continue;

            pick = ui_menu("  Item:", opts, NULL, n);
            qty = ui_int("  Quantity", 1, 99);

            {
                int cost = ITEMS[map[pick]].cost_cp * qty;
                if (cost > purse_cp) {
                    printf("  That costs more than you have.\n");
                    if (!ui_yesno("  Add it anyway (the DM may allow it)?", 0)) {
                        continue;
                    }
                } else {
                    int left = purse_cp - cost;
                    c->platinum = 0; c->electrum = 0;
                    c->gold = left / 100; left %= 100;
                    c->silver = left / 10;
                    c->copper = left % 10;
                }
                add_item(c, map[pick], qty, 0);
                printf("  Added %d x %s.\n", qty, ITEMS[map[pick]].name);
            }
        }
    }
}

void choose_equipment(Character *c)
{
    static const char *const modes[] = {
        "Take the equipment your class and background grant",
        "Take starting gold instead and buy your own"
    };
    int m;

    ui_header("Step 5: Choose Equipment");
    ui_para("Your class and background determine your starting equipment. "
            "Instead of that gear you may take starting gold and buy what you "
            "like. Either way you can visit the shop afterwards.");

    m = ui_menu("How would you like to equip yourself?", modes, NULL, 2);
    if (m == 0) take_starting_package(c);
    else        buy_with_gold(c);

    shop(c);
    auto_equip(c);

    printf("\n  Armor Class %d, carrying %d.%d lb of a %d lb capacity.\n",
           armour_class(c), current_weight_tenths(c) / 10,
           current_weight_tenths(c) % 10, carrying_capacity(c));
    if (current_weight_tenths(c) > carrying_capacity(c) * 10) {
        printf("  You are carrying more than your Strength allows.\n");
    }
}

/* ------------------------------------------------------------- personality */

static void pick_or_type(const char *label, const char *const *suggestions,
                         char *out, size_t n)
{
    const char *opts[10];
    int count = 0, pick;

    while (count < 9 && suggestions[count]) {
        opts[count] = suggestions[count];
        count++;
    }
    opts[count] = "Write my own";

    {
        char prompt[128];
        snprintf(prompt, sizeof prompt, "  %s:", label);
        pick = ui_menu(prompt, opts, NULL, count + 1);
    }
    if (pick == count) {
        ui_line("  Enter your own", out, n);
    } else {
        strncpy(out, opts[pick], n - 1);
        out[n - 1] = '\0';
    }
}

void choose_personality(Character *c)
{
    const BackgroundData *bg;
    const char *aligns[ALIGN_COUNT];
    int i;

    ui_header("Step 4 (continued): Describe Your Character");

    for (i = 0; i < ALIGN_COUNT; i++) aligns[i] = ALIGNMENT_NAME[i];
    c->alignment = (Alignment)ui_menu("  Alignment:", aligns, NULL, ALIGN_COUNT);

    c->age = ui_int("  Age", 1, 900);
    c->height_in = ui_int("  Height in inches", 20, 100);
    c->weight_lb = ui_int("  Weight in pounds", 10, 600);
    ui_line_default("  Eyes", "brown", c->eyes, sizeof c->eyes);
    ui_line_default("  Skin", "tan", c->skin, sizeof c->skin);
    ui_line_default("  Hair", "black", c->hair, sizeof c->hair);

    if (c->background_id < 0) return;
    bg = &BACKGROUNDS[c->background_id];

    printf("\n  Suggested characteristics for the %s background:\n", bg->name);
    pick_or_type("Personality trait", bg->traits, c->trait, sizeof c->trait);
    pick_or_type("Ideal", bg->ideals, c->ideal, sizeof c->ideal);
    pick_or_type("Bond", bg->bonds, c->bond, sizeof c->bond);
    pick_or_type("Flaw", bg->flaws, c->flaw, sizeof c->flaw);

    ui_line("  Appearance (one line, optional)", c->appearance,
            sizeof c->appearance);
    ui_line("  Backstory (one line, optional)", c->backstory,
            sizeof c->backstory);
}
