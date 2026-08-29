/* shop.c -- building a shop, and spending money in one.
 *
 * Two screens that share a file. The DM builds a shop and writes it out;
 * the table loads it in game mode and buys from it. The file is the same
 * '|'-separated shape as everything else this program writes, and goes
 * through the same escaping, so a shop called "Bell, Book & Candle" is a
 * shop and not three fields.
 *
 * A shop holds lines rather than categories. The tempting design is to let
 * the DM tick "adventuring gear" and have the shop mean the whole of that
 * table -- but such a shop restocks itself whenever the tables change, and
 * can never hold the one thing that makes a shop worth visiting, which is
 * something the tables do not have. So every line is put there on purpose,
 * carries its own price, and may be the DM's own invention.
 */
#include "shop.h"
#include "build.h"
#include "game.h"
#include "data.h"
#include "reference.h"
#include "saveload.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SHOP_BEGIN "#BEGIN-DNDSHOP v1"

/* ------------------------------------------------------------ the file */

void shop_name_to_path(const char *name, char *out, size_t n)
{
    char safe[MAX_SHOP_NAME];

    /* The same manners a character sheet is named with, so that the two
       kinds of file cannot be told apart by how carelessly they are
       named. */
    sheet_filename(name, safe, sizeof safe);
    snprintf(out, n, "%s.txt", safe);
}

int shop_save(const Shop *s, char *path, size_t n)
{
    FILE *f;
    int i;

    shop_name_to_path(s->name, path, n);
    /* Shops and characters share a directory and a naming rule, so a shop
       called Ivrit and a character called Ivrit want the same file. The
       character was there first and is the harder thing to lose, so the
       shop gives way and says so. */
    if (file_is_character(path)) return -2;

    f = fopen(path, "w");
    if (!f) return -1;

    /* A header a person can read, then the block the program reads -- the
       same arrangement as a character sheet, and for the same reason: a DM
       wants to look at their own shop without running anything. */
    fprintf(f, "%s\n", s->name);
    if (s->keeper[0]) fprintf(f, "Kept by %s\n", s->keeper);
    if (s->about[0]) {
        fprintf(f, "\n");
        record_wrap(f, s->about, 2);
    }
    fprintf(f, "\n  %-34s %12s  %s\n", "Stock", "Price", "In stock");
    for (i = 0; i < s->line_count; i++) {
        char price[32];
        format_price(s->lines[i].price_cp, price, sizeof price);
        fprintf(f, "  %-34s %12s  ", s->lines[i].name, price);
        if (s->lines[i].stock < 0) fprintf(f, "as many as you like\n");
        else fprintf(f, "%d\n", s->lines[i].stock);
        if (s->lines[i].note[0]) fprintf(f, "      %s\n", s->lines[i].note);
    }

    fprintf(f, "\n%s\n", SHOP_BEGIN);
    fprintf(f, "SHOP|");
    record_put(f, s->name);
    fputc('|', f);
    record_put(f, s->keeper);
    fputc('|', f);
    record_put(f, s->about);
    fputc('\n', f);
    for (i = 0; i < s->line_count; i++) {
        const ShopLine *l = &s->lines[i];
        /* Whether the line came from the books is written down rather than
           worked out on reload. A DM who prices their own "Rope" would
           otherwise find it turned into the book's rope, with the book's
           weight and the book's entry, the next time the shop was opened. */
        fprintf(f, "LINE|%d|%d|%d|%d|", l->category, l->price_cp, l->stock,
                l->from_book ? 1 : 0);
        record_put(f, l->name);
        fputc('|', f);
        record_put(f, l->note);
        fputc('\n', f);
    }
    fprintf(f, "#END-DNDSHOP\n");
    fclose(f);
    return 0;
}

int shop_load(const char *path, Shop *s)
{
    FILE *f = fopen(path, "r");
    /* Every field is written escaped, and an escape can double a field's
       length, so the longest line this can meet is twice the longest record
       it writes. The SHOP record is the long one: a name, a keeper and a
       line about the place. */
    char line[2 * (MAX_SHOP_NAME + MAX_NAME + MAX_TEXT) + 64];
    int in_data = 0, found = 0, i;

    if (!f) return -1;
    memset(s, 0, sizeof *s);

    while (fgets(line, sizeof line, f)) {
        char *fields[16];
        int n;

        line[strcspn(line, "\r\n")] = '\0';
        if (!in_data) {
            if (!strcmp(line, SHOP_BEGIN)) in_data = 1;
            continue;
        }
        if (!strcmp(line, "#END-DNDSHOP")) break;
        if (!line[0]) continue;

        n = record_split(line, fields, 16);
        if (n < 1) continue;
        /* Written escaped, so read unescaped -- the same pass a character
           file makes, and for the same reason: a shop called "Bell, Book &
           Candle | Pawn" is one shop. */
        for (i = 0; i < n; i++) record_unescape(fields[i]);

        if (!strcmp(fields[0], "SHOP") && n >= 4) {
            snprintf(s->name, sizeof s->name, "%s", fields[1]);
            snprintf(s->keeper, sizeof s->keeper, "%s", fields[2]);
            snprintf(s->about, sizeof s->about, "%s", fields[3]);
            found = 1;
        } else if (!strcmp(fields[0], "LINE") && n >= 7
                   && s->line_count < MAX_SHOP_LINES) {
            ShopLine *l = &s->lines[s->line_count++];
            memset(l, 0, sizeof *l);
            l->category = record_int(fields[1], 0, ITEM_CATEGORY_COUNT - 1);
            l->price_cp = record_int(fields[2], 0, MAX_COINS);
            l->stock = record_int(fields[3], -1, MAX_STACK);
            l->from_book = record_int(fields[4], 0, 1);
            snprintf(l->name, sizeof l->name, "%s", fields[5]);
            snprintf(l->note, sizeof l->note, "%s", fields[6]);
            /* A book line is looked up again rather than stored by number,
               because the banks move: a book switched off or a homebrew
               item added shifts every index after it. A line whose item the
               banks no longer hold keeps its name and its price and stops
               being a book item, which is what the DM's own lines already
               are. */
            l->item_id = l->from_book ? find_item(l->name) : -1;
            if (l->item_id < 0) l->from_book = 0;
        }
    }
    fclose(f);
    return found ? 0 : -2;
}

/* --------------------------------------------------------------- prices */

/* Money is asked for in coins rather than in copper, because a DM pricing
   a sword thinks "fifteen gold" and not "fifteen hundred". A price already
   known -- the book's, or the one the line carries -- is offered first, so
   the common answer is to accept it. */
static int ask_coins(void)
{
    int gp = ui_int("    Gold pieces", 0, MAX_COINS / 100);
    int sp = ui_int("    Silver", 0, 99);
    int cp = ui_int("    Copper", 0, 99);
    long total = (long)gp * 100 + (long)sp * 10 + cp;

    /* The file holds a price in one field and reads it back through the
       same range check every other number gets, so a price larger than
       that field allows would come back smaller than it went in and the
       shop would change on reload. Nine thousand nine hundred and
       ninety-nine gold, ninety-nine silver and ninety-nine copper is a
       little over it. */
    if (total > MAX_COINS) {
        total = MAX_COINS;
        printf("    Held to %d copper, which is all one price can be.\n",
               MAX_COINS);
    }
    return (int)total;
}

static int ask_price(int def_cp)
{
    char shown[32];

    format_price(def_cp, shown, sizeof shown);
    printf("    The book says %s.\n", shown);
    if (ui_yesno("    Charge that?", 1)) return def_cp;
    return ask_coins();
}

static int ask_stock(void)
{
    /* A shop that never runs out is the ordinary case -- nobody counts the
       rope in a general store -- so it is the default, and a number is for
       the one sword on the wall. */
    if (ui_yesno("    Is there always more of it?", 1)) return -1;
    return ui_int("    How many are on the shelf", 0, MAX_STACK);
}

/* ---------------------------------------------------------- the builder */

static void shop_lines_report(const Shop *s)
{
    int i;

    if (!s->line_count) {
        printf("  Nothing on the shelves yet.\n");
        return;
    }
    printf("\n  %-32s %10s  %s\n", "Stock", "Price", "How many");
    for (i = 0; i < s->line_count; i++) {
        char price[32];
        format_price(s->lines[i].price_cp, price, sizeof price);
        printf("  %2d. %-28s %10s  ", i + 1, s->lines[i].name, price);
        if (s->lines[i].stock < 0) printf("plenty\n");
        else printf("%d\n", s->lines[i].stock);
        if (s->lines[i].note[0]) printf("      %s\n", s->lines[i].note);
    }
}

/* Whether the shop already sells this. A shop with two "Rope, hempen (50
   feet)" lines at different prices is not a richer shop, it is a bug the
   DM has to find at the table. */
static int already_stocked(const Shop *s, const char *name)
{
    int i;
    for (i = 0; i < s->line_count; i++) {
        if (same_fold(s->lines[i].name, name)) return 1;
    }
    return 0;
}

static void stock_from_books(Shop *s)
{
    for (;;) {
        const char *cats[16];
        int i, cat;

        if (s->line_count >= MAX_SHOP_LINES) {
            printf("  That is as much as one shop can hold.\n");
            return;
        }
        for (i = 0; i < ITEM_CATEGORY_COUNT; i++) cats[i] = CATEGORY_LABEL[i];
        cats[ITEM_CATEGORY_COUNT] = "Back";

        cat = ui_menu("  Which shelf?", cats, NULL, ITEM_CATEGORY_COUNT + 1);
        if (cat == ITEM_CATEGORY_COUNT) return;

        {
            const char *opts[REF_MAX + 1];
            static char lines[REF_MAX][REF_LINE];
            int map[REF_MAX], n = 0, pick;

            for (i = 0; i < ITEM_COUNT && n < REF_MAX; i++) {
                char price[32];
                if ((int)ITEMS[i].category != cat) continue;
                if (!book_enabled(ITEMS[i].book)) continue;
                format_price(ITEMS[i].cost_cp, price, sizeof price);
                /* Saying so here saves the DM adding a line and being told
                   afterwards that it was already there. */
                snprintf(lines[n], REF_LINE, "%-30s %8s%s", ITEMS[i].name,
                         price,
                         already_stocked(s, ITEMS[i].name) ? "  (stocked)"
                                                           : "");
                opts[n] = lines[n];
                map[n] = i;
                n++;
            }
            if (n == 0) {
                printf("  Nothing on that shelf, with the books you have "
                       "switched on.\n");
                continue;
            }
            opts[n] = "Back";

            pick = ui_menu("  Which item?", opts, NULL, n + 1);
            if (pick == n) continue;

            if (already_stocked(s, ITEMS[map[pick]].name)) {
                printf("  %s is already in this shop.\n",
                       ITEMS[map[pick]].name);
                continue;
            }
            {
                ShopLine *l = &s->lines[s->line_count++];
                memset(l, 0, sizeof *l);
                snprintf(l->name, sizeof l->name, "%s", ITEMS[map[pick]].name);
                l->from_book = 1;
                l->item_id = map[pick];
                l->category = (int)ITEMS[map[pick]].category;
                l->price_cp = ask_price(ITEMS[map[pick]].cost_cp);
                l->stock = ask_stock();
                ui_line("    A word about it (blank for none)", l->note,
                        sizeof l->note);
                printf("  %s is on the shelf.\n", l->name);
            }
            if (s->line_count >= MAX_SHOP_LINES) return;
            if (!ui_yesno("  Stock something else?", 1)) return;
        }
    }
}

static void stock_your_own(Shop *s)
{
    const char *cats[16];
    ShopLine *l;
    char name[MAX_NAME];
    int i;

    if (s->line_count >= MAX_SHOP_LINES) {
        printf("  That is as much as one shop can hold.\n");
        return;
    }
    printf("\n  For what the books do not sell: a map to the old mine, a "
           "cure for a curse, a pie.\n");
    ui_line("  What is it", name, sizeof name);
    if (!name[0]) return;
    if (already_stocked(s, name)) {
        printf("  This shop already sells that.\n");
        return;
    }

    for (i = 0; i < ITEM_CATEGORY_COUNT; i++) cats[i] = CATEGORY_LABEL[i];

    l = &s->lines[s->line_count++];
    memset(l, 0, sizeof *l);
    snprintf(l->name, sizeof l->name, "%s", name);
    l->from_book = 0;
    l->item_id = -1;
    l->category = ui_menu("  Which shelf does it go on?", cats, NULL,
                          ITEM_CATEGORY_COUNT);
    l->price_cp = ask_coins();
    l->stock = ask_stock();
    ui_line("    A word about it (blank for none)", l->note, sizeof l->note);
    printf("  %s is on the shelf.\n", l->name);
}

static void change_a_line(Shop *s)
{
    const char *opts[MAX_SHOP_LINES + 1];
    static char lines[MAX_SHOP_LINES][REF_LINE];
    int i, pick;

    if (!s->line_count) {
        printf("  Nothing on the shelves yet.\n");
        return;
    }
    for (i = 0; i < s->line_count; i++) {
        char price[32];
        format_price(s->lines[i].price_cp, price, sizeof price);
        snprintf(lines[i], REF_LINE, "%-30s %8s", s->lines[i].name, price);
        opts[i] = lines[i];
    }
    opts[s->line_count] = "Back";

    pick = ui_menu("  Which line?", opts, NULL, s->line_count + 1);
    if (pick == s->line_count) return;

    {
        static const char *const what[] = {
            "Change the price", "Change how many there are",
            "Change the word about it", "Take it off the shelf", "Back"
        };
        ShopLine *l = &s->lines[pick];

        switch (ui_menu("  What about it?", what, NULL, 5)) {
        case 0:
            l->price_cp = ask_coins();
            break;
        case 1:
            l->stock = ask_stock();
            break;
        case 2:
            ui_line("    A word about it (blank for none)", l->note,
                    sizeof l->note);
            break;
        case 3:
            printf("  %s is off the shelf.\n", l->name);
            for (i = pick; i + 1 < s->line_count; i++) {
                s->lines[i] = s->lines[i + 1];
            }
            s->line_count--;
            break;
        default:
            break;
        }
    }
}

void shopbuilder_menu(void)
{
    static Shop s;          /* far too big for the stack at 128 lines */
    int open = 0;
    /* Whether anything has changed since the last save, so that closing a
       shop just written asks nothing and closing an hour's work asks
       once. */
    int dirty = 0;

    ui_header("Shopbuilder");
    printf("  A shop is what you put in it and nothing else -- a smith who "
           "sells three swords sells three swords, not the whole weapons "
           "table. Each line carries its own price, so the town with a war "
           "on can charge what it likes for arrows.\n");

    for (;;) {
        static const char *const top[] = {
            "Start a new shop", "Open a shop you saved", "Back"
        };
        char path[MAX_SHOP_NAME + 8];

        switch (ui_menu("\n  Shopbuilder:", top, NULL, 3)) {
        case 0:
            memset(&s, 0, sizeof s);
            ui_line("  What is the shop called", s.name, sizeof s.name);
            if (!s.name[0]) continue;
            ui_line("  Who keeps it (blank for nobody in particular)",
                    s.keeper, sizeof s.keeper);
            ui_line("  A line about the place (blank for none)", s.about,
                    sizeof s.about);
            open = 1;
            dirty = 1;
            break;
        case 1: {
            char name[MAX_SHOP_NAME];
            ui_line("  Which shop (the name you saved it under)", name,
                    sizeof name);
            if (!name[0]) continue;
            shop_name_to_path(name, path, sizeof path);
            if (shop_load(path, &s) != 0) {
                printf("  Could not read %s\n", path);
                continue;
            }
            printf("  Opened %s, %d line%s.\n", s.name, s.line_count,
                   s.line_count == 1 ? "" : "s");
            open = 1;
            dirty = 0;
            break;
        }
        default:
            return;
        }

        while (open) {
            static const char *const what[] = {
                "Stock something from the books",
                "Stock something of your own",
                "Change or remove a line",
                "Look at the shelves",
                "Save the shop",
                "Close it"
            };

            printf("\n  %s", s.name);
            if (s.keeper[0]) printf(", kept by %s", s.keeper);
            printf(" -- %d line%s\n", s.line_count,
                   s.line_count == 1 ? "" : "s");

            switch (ui_menu("  The shop:", what, NULL, 6)) {
            case 0: stock_from_books(&s); dirty = 1; break;
            case 1: stock_your_own(&s); dirty = 1; break;
            case 2: change_a_line(&s); dirty = 1; break;
            case 3: shop_lines_report(&s); break;
            case 4:
                switch (shop_save(&s, path, sizeof path)) {
                case 0:
                    printf("  Written to %s\n", path);
                    dirty = 0;
                    break;
                case -2:
                    printf("  %s is a character's sheet. Call the shop "
                           "something else.\n", path);
                    break;
                default:
                    printf("  Could not write the shop.\n");
                    break;
                }
                break;
            default:
                /* Closing without saving loses the work, which is worth one
                   question rather than a silent surprise. */
                if (dirty && !ui_yesno("  Close it without saving?", 0)) {
                    break;
                }
                open = 0;
                break;
            }
        }
    }
}

/* ----------------------------------------------------------- the visit */

/* What the buyer walks away with. An item the books know goes into the
   inventory proper, with its weight and its stat line; a line the DM
   invented has neither, and goes in with the gems and the oddments, where
   a thing that is only a name and a price already lives. */
static void hand_it_over(Character *c, const ShopLine *l, int qty)
{
    if (l->from_book && l->item_id >= 0) {
        /* A pack bought in a shop is unpacked like any other, so what the
           player carries is the gear and not the word. */
        if (add_pack(c, l->item_id, qty) <= 0) {
            add_item(c, l->item_id, qty, 0);
        }
        return;
    }
    {
        int i;
        for (i = 0; i < c->valuable_count; i++) {
            if (same_fold(c->valuables[i].name, l->name)) {
                c->valuables[i].quantity += qty;
                return;
            }
        }
        if (c->valuable_count >= MAX_VALUABLES) {
            printf("  You are carrying as many odd things as you can keep "
                   "track of; %s went to a friend.\n", l->name);
            return;
        }
        {
            Valuable *v = &c->valuables[c->valuable_count++];
            memset(v, 0, sizeof *v);
            snprintf(v->name, sizeof v->name, "%s", l->name);
            v->value_cp = l->price_cp;
            v->quantity = qty;
            snprintf(v->note, sizeof v->note, "%s", l->note);
        }
    }
}

static void buy_from(Character *c, Shop *s)
{
    for (;;) {
        const char *opts[MAX_SHOP_LINES + 1];
        static char lines[MAX_SHOP_LINES][REF_LINE];
        static char info[MAX_SHOP_LINES][MAX_TEXT];
        const char *infos[MAX_SHOP_LINES + 1];
        int map[MAX_SHOP_LINES], n = 0, i, pick;
        long have = purse_in_copper(c);

        for (i = 0; i < s->line_count && n < MAX_SHOP_LINES; i++) {
            char price[32];
            const ShopLine *l = &s->lines[i];
            if (l->stock == 0) continue;    /* sold out; not on the list */
            format_price(l->price_cp, price, sizeof price);
            snprintf(lines[n], REF_LINE, "%-30s %10s%s", l->name, price,
                     l->price_cp > have ? "  (beyond you)" : "");
            snprintf(info[n], sizeof info[n], "%s",
                     l->note[0] ? l->note : l->name);
            opts[n] = lines[n];
            infos[n] = info[n];
            map[n] = i;
            n++;
        }
        if (n == 0) {
            printf("  The shelves are bare.\n");
            return;
        }
        opts[n] = "Done";
        infos[n] = "Leave the shop.";

        {
            char purse[32];
            format_price((int)have, purse, sizeof purse);
            printf("\n  You have %s.\n", purse);
        }
        pick = ui_menu_info("  Buy what?", opts, NULL, n + 1, infos);
        if (pick == n) return;

        {
            ShopLine *l = &s->lines[map[pick]];
            int most = MAX_QUANTITY;
            int qty;
            long cost;

            if (l->from_book && l->item_id >= 0) show_item_detail(l->item_id);
            else if (l->note[0]) printf("\n  %s\n", l->note);

            if (l->price_cp > 0) {
                long afford = have / l->price_cp;
                if (afford < most) most = (int)afford;
            }
            if (l->stock > 0 && l->stock < most) most = l->stock;
            if (most < 1) {
                printf("  You cannot afford even one.\n");
                continue;
            }
            qty = ui_int("  How many", 0, most);
            if (qty == 0) continue;

            cost = (long)l->price_cp * qty;
            purse_from_copper(c, have - cost);
            {
                char note[MAX_NAME];
                snprintf(note, sizeof note, "%s x%d at %s", l->name, qty,
                         s->name);
                remember(c, (int)-cost, note);
            }
            hand_it_over(c, l, qty);
            if (l->stock > 0) l->stock -= qty;
            /* format_price writes "--" for nothing, which is the right
               thing in a price column and the wrong thing in a sentence. */
            if (cost == 0) {
                printf("  Took %d x %s, for nothing.\n", qty, l->name);
            } else {
                char price[32];
                format_price((int)cost, price, sizeof price);
                printf("  Bought %d x %s for %s.\n", qty, l->name, price);
            }
        }
    }
}

void shop_visit(Character *c)
{
    static Shop s;
    char name[MAX_SHOP_NAME], path[MAX_SHOP_NAME + 8];

    ui_line("  Which shop (the name the DM saved it under)", name,
            sizeof name);
    if (!name[0]) return;

    shop_name_to_path(name, path, sizeof path);
    if (shop_load(path, &s) != 0) {
        printf("  Could not read %s -- ask your DM what the shop is "
               "called.\n", path);
        return;
    }

    printf("\n  %s", s.name);
    if (s.keeper[0]) printf(", kept by %s", s.keeper);
    printf("\n");
    if (s.about[0]) ui_wrap(s.about, 2);

    /* What is bought comes off the shelf for the rest of the visit, and the
       shelf is stocked again next time: the file is not written back, so
       one player emptying a shop does not empty it for the table. What a
       shop really has left is the DM's to say, in the builder. */
    buy_from(c, &s);
}
