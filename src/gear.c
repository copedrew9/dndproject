/* gear.c -- starting equipment, the shop, and personality (PHB chapters 4-5). */
#include "backstory.h"
#include "details.h"
#include "build.h"
#include "reference.h"
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

/* The Armor Class a suit would give this character if worn. */
static int armour_ac(const Character *c, const ItemData *it)
{
    int dex = ability_mod(c, ABL_DEX);

    return it->base_ac + ((it->dex_cap < 0) ? dex
                        : (it->dex_cap == 0) ? 0
                        : (dex < it->dex_cap ? dex : it->dex_cap));
}

/* Equips the best armour and a shield if the character owns any.
 *
 * "Best" used to mean the highest Armor Class and nothing else, which put
 * the plate a wizard bought out of curiosity straight onto the wizard:
 * armour you are not proficient with means disadvantage on every ability
 * check, save and attack that uses Strength or Dexterity, and no spells at
 * all. The program silently chose that, and the sheet showed a very good
 * Armor Class with none of the cost.
 *
 * So the best armour the character is proficient with goes on by itself,
 * and anything better than that which they cannot use is offered rather
 * than assumed -- once, naming what it is, and only here, where the player
 * is at the prompt. Loading a character equips what the file says and asks
 * nothing.
 */
static void auto_equip(Character *c)
{
    int i, best = -1, best_ac = -1, other = -1, other_ac = -1;
    int shields = 0, wear_shield = 1;

    for (i = 0; i < c->item_count; i++) {
        const ItemData *it;
        int ac;

        if (c->inventory[i].is_magic) continue;
        it = &ITEMS[c->inventory[i].item_id];
        if (it->category == ITEM_SHIELD) { shields = 1; continue; }
        if (it->category > ITEM_HEAVY_ARMOR) continue;

        ac = armour_ac(c, it);
        if (armour_proficient(c, it->category)) {
            if (ac > best_ac) { best_ac = ac; best = i; }
        } else if (ac > other_ac) {
            other_ac = ac; other = i;
        }
    }

    /* Only worth asking when the armour they cannot use is actually better
       than the armour they can -- or when it is all they have. */
    if (other >= 0 && (best < 0 || other_ac > best_ac)) {
        char ask[MAX_TEXT];

        snprintf(ask, sizeof ask,
                 "\n  Do you want to equip the %s? You are not proficient "
                 "with it and so will be hindered by it.",
                 ITEMS[c->inventory[other].item_id].name);
        if (ui_yesno(ask, 0)) {
            best = other;
        } else if (best >= 0) {
            printf("  Wearing the %s instead.\n",
                   ITEMS[c->inventory[best].item_id].name);
        }
    }

    if (shields && !armour_proficient(c, ITEM_SHIELD)) {
        wear_shield = ui_yesno(
            "\n  Do you want to equip the shield? You are not proficient "
            "with it and so will be hindered by it.", 0);
    }

    for (i = 0; i < c->item_count; i++) {
        const ItemData *it;
        if (c->inventory[i].is_magic) continue;
        it = &ITEMS[c->inventory[i].item_id];
        if (it->category <= ITEM_HEAVY_ARMOR) {
            c->inventory[i].equipped = (i == best);
        } else if (it->category == ITEM_SHIELD) {
            c->inventory[i].equipped = wear_shield;
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
        if (!book_enabled(ITEMS[i].book)) continue;
        snprintf(lines[n], sizeof lines[n], "%-20s %s %s", ITEMS[i].name,
                 ITEMS[i].damage, ITEMS[i].damage_type);
        opts[n] = lines[n];
        map[n] = i;
        n++;
    }
    if (n == 0) return;

    snprintf(prompt, sizeof prompt, "      Choose a %s:", label);
    pick = ui_menu(prompt, opts, NULL, n);
    add_gear(c, map[pick], 1, 0);
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

    /* "a warhammer (if proficient)" and "chain mail (if proficient)" are
       the cleric's, and the qualifier is about who may take the option,
       not about what the thing is called. Nothing stripped it, so neither
       matched an item and both fell through to being noted as text -- a
       cleric who picked the warhammer or the chain mail was handed
       nothing. Only this one qualifier is stripped, because item names
       legitimately end in brackets: "Rations (1 day)", "Oil (flask)",
       "Orb (arcane focus)". */
    {
        static const char QUALIFIER[] = " (if proficient)";
        size_t plen = strlen(p);
        size_t qlen = sizeof QUALIFIER - 1;
        if (plen > qlen && strcmp(p + plen - qlen, QUALIFIER) == 0) {
            static char trimmed[256];
            size_t keep = plen - qlen;
            if (keep >= sizeof trimmed) keep = sizeof trimmed - 1;
            memcpy(trimmed, p, keep);
            trimmed[keep] = '\0';
            p = trimmed;
        }
    }

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
            add_gear(c, id, ALIASES[i].qty ? qty : 1, 0);
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
        int parts = add_gear(c, id, qty, 0);
        if (parts > 1) {
            printf("      Added %d x %s, unpacked into %d things.\n",
                   qty, ITEMS[id].name, parts);
        } else {
            printf("      Added %d x %s.\n", qty, ITEMS[id].name);
        }
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

/* Where the next lettered alternative starts, or NULL.
 *
 * A marker is "(a)" -- one bracket, one lowercase letter, one bracket --
 * standing at the start of the line or after a space. The letter has to be
 * a single character because the alternatives are not the only brackets on
 * these lines: the paladin's armour reads "(c) chain mail (if proficient)",
 * and splitting on that one would cut an option in half.
 */
static const char *next_alternative(const char *s, const char *start)
{
    for (; *s; s++) {
        if (s[0] != '(' || s[1] < 'a' || s[1] > 'z' || s[2] != ')') continue;
        if (s != start && s[-1] != ' ') continue;
        return s;
    }
    return NULL;
}

/* How much of an alternative is the alternative, with the punctuation that
 * joins it to the next one taken off: "(a) a rapier, " and "(b) a longsword
 * or " are a rapier and a longsword. */
static size_t alternative_len(const char *p, size_t len)
{
    while (len && p[len - 1] == ' ') len--;
    if (len >= 2 && p[len - 2] == 'o' && p[len - 1] == 'r'
        && (len == 2 || p[len - 3] == ' ')) {
        len -= 2;
        while (len && p[len - 1] == ' ') len--;
    }
    if (len && p[len - 1] == ',') len--;
    return len;
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

    /* Split on the markers themselves rather than on " or (". Three of the
       PHB's lines offer three things and separate the first two with a
       comma -- "(a) a burglar's pack, (b) a dungeoneer's pack or (c) an
       explorer's pack" -- and splitting on the "or" alone put the burglar's
       and the dungeoneer's pack on one line as a single option, so a rogue
       could not choose between them. */
    p = next_alternative(buf, buf);
    while (p && n < 8) {
        const char *next = next_alternative(p + 3, buf);
        size_t len = alternative_len(p, next ? (size_t)(next - p) : strlen(p));
        if (len >= sizeof parts[0]) len = sizeof parts[0] - 1;
        memcpy(parts[n], p, len);
        parts[n][len] = '\0';
        opts[n] = parts[n];
        n++;
        p = next;
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

/* The gold a class starts with when the average is taken rather than
   rolled.
 *
 * The average of a d4 is 2.5, so the expected value is dice x 5 / 2 times
 * the class's multiplier. The division used to come first and truncate:
 * 5d4 x 10 came out at 120 gp where the Player's Handbook's own average
 * column says 125. Multiplying first gives every class the book's number.
 */
int average_starting_gold(const ClassData *cd)
{
    return (cd->gold_dice * 5 * cd->gold_mult) / 2;
}

static void buy_with_gold(Character *c)
{
    const ClassData *cd = &CLASSES[c->classes[0].class_id];
    int rolled;

    printf("\n  A %s begins with %dd4", cd->name, cd->gold_dice);
    if (cd->gold_mult != 1) printf(" x %d", cd->gold_mult);
    printf(" gp.\n");

    if (ui_yesno("  Roll for starting gold?", 1)) {
        rolled = ui_roll(cd->gold_dice, 4, "starting gold")
               * cd->gold_mult;
        printf("    Rolled %d gp.\n", rolled);
    } else {
        rolled = average_starting_gold(cd);
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
        int i, cat, purse_cp;

        purse_cp = c->gold * 100 + c->silver * 10 + c->copper
                 + c->electrum * 50 + c->platinum * 1000;

        printf("\n  Purse: %d gp %d sp %d cp   Carrying %d.%d of %d lb\n",
               c->gold, c->silver, c->copper,
               current_weight_tenths(c) / 10, current_weight_tenths(c) % 10,
               carrying_capacity(c));

        if (!ui_yesno("  Buy or add equipment?", 0)) return;

        for (i = 0; i < ITEM_CATEGORY_COUNT; i++) cats[i] = CATEGORY_NAME[i];
        cats[ITEM_CATEGORY_COUNT] = "Done shopping";
        cat = ui_menu("  Category:", cats, NULL, ITEM_CATEGORY_COUNT + 1);
        if (cat == ITEM_CATEGORY_COUNT) return;

        {
            const char *opts[256];
            static char lines[256][160];
            int map[256], n = 0, pick, qty;

            for (i = 0; i < ITEM_COUNT && n < 254; i++) {
                char price[32], weight[32];
                if ((int)ITEMS[i].category != cat) continue;
                if (!book_enabled(ITEMS[i].book)) continue;
                print_price(ITEMS[i].cost_cp, price, sizeof price);
                format_weight(ITEMS[i].weight_tenths, weight, sizeof weight);

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

            opts[n] = "Look an item up without buying";
            opts[n + 1] = "Back to the categories";
            pick = ui_menu("  Item:", opts, NULL, n + 2);
            if (pick == n + 1) continue;
            if (pick == n) {
                int look = ui_menu("  Look up:", opts, NULL, n);
                show_item_detail(map[look]);
                continue;
            }
            show_item_detail(map[pick]);
            if (!ui_yesno("  Buy it?", 1)) continue;
            qty = ui_int("  Quantity", 1, MAX_QUANTITY);

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
                if (add_gear(c, map[pick], qty, 0) > 1) {
                    printf("  Added %d x %s, unpacked.\n", qty,
                           ITEMS[map[pick]].name);
                } else {
                    printf("  Added %d x %s.\n", qty,
                           ITEMS[map[pick]].name);
                }
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

    if (c->item_count && ui_yesno("\n  Look over what you are carrying?", 0)) {
        inventory_reference(c);
    }

    printf("\n  Armor Class %d, carrying %d.%d lb of a %d lb capacity.\n",
           armour_class(c), current_weight_tenths(c) / 10,
           current_weight_tenths(c) % 10, carrying_capacity(c));
    if (current_weight_tenths(c) > carrying_capacity(c) * 10) {
        printf("  You are carrying more than your Strength allows.\n");
    }
}


/* "2d10" -> a roll of it; "1" -> 1. The weight column of the Random Height
   and Weight table is a flat multiplier for the halfling and the gnome. */
static int roll_notation(const char *dice)
{
    const char *d = strchr(dice, 'd');
    if (!d) return atoi(dice);
    return ui_roll(d == dice ? 1 : atoi(dice), atoi(d + 1),
                   "height and weight");
}

/* Offers the PHB's Random Height and Weight table where the books give a
   row for the race, and falls back to asking outright where they do not:
   the newer races describe their build in prose instead. */
static void choose_body(Character *c)
{
    const BodyData *b = body_for(RACES[c->race_id].name,
                                 c->subrace_id >= 0
                                     ? SUBRACES[c->subrace_id].name : NULL);

    c->age = ui_int("  Age", 1, 900);

    if (b && ui_yesno("  Roll height and weight on the book's table?", 1)) {
        /* The same roll that gives the inches above the base height also
           multiplies the weight dice, which is what keeps a tall character
           heavy and a short one light. */
        int modifier = roll_notation(b->height_dice);
        int extra = modifier * roll_notation(b->weight_dice);

        c->height_in = b->base_height + modifier;
        c->weight_lb = b->base_weight + extra;
        printf("    %d ft %d in, %d lb (base %d in + %d, base %d lb + %d)\n",
               c->height_in / 12, c->height_in % 12, c->weight_lb,
               b->base_height, modifier, b->base_weight, extra);
        return;
    }

    if (b) {
        printf("    The table gives %d ft %d in + %s inches, and %d lb plus"
               " that roll times %s lb.\n", b->base_height / 12,
               b->base_height % 12, b->height_dice, b->base_weight,
               b->weight_dice);
    }
    c->height_in = ui_int("  Height in inches", 20, 120);
    c->weight_lb = ui_int("  Weight in pounds", 10, 1500);
}
/* ------------------------------------------------------------- personality */

/* The background's suggestions are a NULL-terminated array; the shared
   helper wants a count, so this counts and hands over. */
static void pick_or_type(const char *label, const char *const *suggestions,
                         char *out, size_t n)
{
    char prompt[128];
    int count = 0;

    snprintf(prompt, sizeof prompt, "  %s:", label);
    if (!suggestions) {                 /* nothing to suggest; just ask */
        ui_line(prompt, out, n);
        return;
    }
    while (count < 9 && suggestions[count]) count++;
    ui_pick_or_type(prompt, suggestions, count, out, n);
}

/* ------------------------------------------------------------------ faith */

/* Does this deity suggest the given domain? The domain list is text so that
 * it reads well on the sheet, so the match is a substring test against the
 * subclass name with its "Domain" suffix removed. */
static int deity_suggests(const Deity *d, const char *domain_subclass)
{
    char bare[MAX_NAME];
    char *space;

    snprintf(bare, sizeof bare, "%s", domain_subclass);
    space = strstr(bare, " Domain");
    if (space) *space = '\0';
    return contains_ci(d->domains, bare);
}

/* Which domain, if any, this character has already taken. */
static const char *chosen_domain(const Character *c)
{
    int i;
    for (i = 0; i < c->class_count; i++) {
        if (c->classes[i].class_id != CLS_CLERIC) continue;
        if (c->classes[i].subclass_id < 0) continue;
        return SUBCLASSES[c->classes[i].subclass_id].name;
    }
    return NULL;
}

/* Classes whose power is somebody else's.
 *
 * A cleric serves a god, a paladin swears an oath, and a warlock strikes a
 * bargain with an extraplanar patron: for all three the source is part of
 * the character rather than a decoration on it, so naming it is not
 * optional. Everyone else may keep a faith and is asked instead. */
int class_must_name_a_patron(int class_id)
{
    return class_id == CLS_CLERIC || class_id == CLS_PALADIN
        || class_id == CLS_WARLOCK;
}

/* Offers the deity tables, and says whether one was actually named.
 *
 * When it is required the way out is not offered at all, rather than
 * offered and refused: a menu that cannot be declined always ends in a
 * deity, where a loop around a decline would spin forever against a
 * closed input. */
static int choose_deity(Character *c, int required)
{
    const char *pantheons[16];
    int pn = 0, i, pick;
    const char *domain = chosen_domain(c);

    /* Pantheons in the order they appear in the table, without repeats. */
    for (i = 0; i < DEITY_COUNT && pn < 15; i++) {
        int j, seen = 0;
        for (j = 0; j < pn; j++) {
            if (strcmp(pantheons[j], DEITIES[i].pantheon) == 0) seen = 1;
        }
        if (!seen) pantheons[pn++] = DEITIES[i].pantheon;
    }
    /* A deity table with nothing in it -- a homebrew file that replaced it,
       say -- leaves nothing to require. */
    if (pn == 0) return 0;
    if (!required) pantheons[pn] = "No deity in particular";

    if (domain) {
        printf("\n  Deities marked with a star suggest the %s.\n", domain);
    }
    pick = ui_menu("  Which pantheon?", pantheons, NULL, pn + !required);
    if (!required && pick == pn) return 0;

    {
        const char *opts[128];
        const char *det[128];
        static char labels[128][120];
        static char details[128][160];
        int map[128], n = 0, choice;

        for (i = 0; i < DEITY_COUNT && n < 128; i++) {
            const Deity *d = &DEITIES[i];
            if (strcmp(d->pantheon, pantheons[pick]) != 0) continue;

            snprintf(labels[n], sizeof labels[n], "%s%s, %s",
                     (domain && deity_suggests(d, domain)) ? "* " : "",
                     d->name, d->title);
            snprintf(details[n], sizeof details[n], "%s -- %s. Symbol: %s",
                     d->alignment, d->domains, d->symbol);
            opts[n] = labels[n];
            det[n] = details[n];
            map[n] = i;
            n++;
        }
        if (n == 0) return 0;

        choice = ui_menu("  Your deity:", opts, det, n);
        add_choice(c, "Deity", DEITIES[map[choice]].name);

        if (domain && !deity_suggests(&DEITIES[map[choice]], domain)) {
            printf("  %s does not suggest the %s, which is worth agreeing "
                   "with your DM.\n", DEITIES[map[choice]].name, domain);
        }
        return 1;
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

    choose_body(c);
    ui_line_default("  Eyes", "brown", c->eyes, sizeof c->eyes);
    ui_line_default("  Skin", "tan", c->skin, sizeof c->skin);
    ui_line_default("  Hair", "black", c->hair, sizeof c->hair);

    /* A character with a background of their own has no table to draw
       suggestions from, but still has a personality; the step used to be
       skipped entirely in that case. */
    bg = (c->background_id >= 0) ? &BACKGROUNDS[c->background_id] : NULL;

    if (bg) {
        printf("\n  Suggested characteristics for the %s background:\n",
               bg->name);
    } else {
        printf("\n  Your background is your own, so these are yours to "
               "write.\n");
    }
    pick_or_type("Personality trait", bg ? bg->traits : NULL, c->trait,
                 sizeof c->trait);
    pick_or_type("Ideal", bg ? bg->ideals : NULL, c->ideal, sizeof c->ideal);
    pick_or_type("Bond", bg ? bg->bonds : NULL, c->bond, sizeof c->bond);
    pick_or_type("Flaw", bg ? bg->flaws : NULL, c->flaw, sizeof c->flaw);

    /* A cleric, paladin or warlock has to name what they serve; everyone
       else may still keep a faith, and is asked. */
    {
        int devout = 0;
        const char *why = NULL;
        for (i = 0; i < c->class_count; i++) {
            int id = c->classes[i].class_id;
            if (!class_must_name_a_patron(id)) continue;
            devout = 1;
            why = CLASSES[id].name;
        }
        if (devout) {
            printf("\n  A %s draws their power from someone in particular, "
                   "so this one is not a choice.\n", why);
            choose_deity(c, 1);
        } else if (ui_yesno("\n  Choose a deity for your character?", 0)) {
            choose_deity(c, 0);
        }
    }

    ui_line("  Appearance (one line, optional)", c->appearance,
            sizeof c->appearance);

    /* Xanathar's tables, for a player who would rather build a past than
       write one. Either way the answer lands in the same line. */
    if (book_enabled(BOOK_XGE)
        && ui_yesno("\n  Work out where you came from, from Xanathar's "
                    "tables?", 0)) {
        build_backstory(c);
    }
    if (!c->backstory[0]) {
        ui_line("  Backstory (one line, optional)", c->backstory,
                sizeof c->backstory);
    }

    if (ui_yesno("\n  Add any notes about this character?", 0)) {
        edit_details(c);
    }
}
