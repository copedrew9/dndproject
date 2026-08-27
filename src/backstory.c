/* backstory.c -- Xanathar's "This Is Your Life".
 *
 * The book's tables are meant to be rolled on, but a player who already
 * knows where their character came from should not have to roll and then
 * argue with the result. Every table here can be rolled or read down and
 * picked from, and any of them can be skipped.
 *
 * What is written onto the character is the answers, joined into the
 * backstory line the sheet already carries.
 */
#include "dnd.h"
#include "data.h"
#include "backstory.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Rolls the table's own die, written as "d100", "3d6" and so on. */
static int roll_table_die(const char *die)
{
    const char *d = strchr(die, 'd');
    int n = (d && d != die) ? (die[0] - '0') : 1;
    int sides = d ? atoi(d + 1) : 20;

    if (sides < 2) sides = 20;
    return ui_roll(n, sides, "this table");
}

static const LifeEntry *entry_for(const LifeTable *t, int roll)
{
    int i;
    for (i = 0; i < t->count; i++) {
        if (roll >= t->rows[i].lo && roll <= t->rows[i].hi) return &t->rows[i];
    }
    return t->count ? &t->rows[t->count - 1] : NULL;
}

/* Offers one table three ways: roll it, pick from it, or leave it out. */
static const char *ask_table(const LifeTable *t)
{
    static const char *const modes[] = {
        "Roll on the table", "Choose from the table", "Skip this one"
    };
    char prompt[96];
    int mode;

    snprintf(prompt, sizeof prompt, "  %s (%s):", t->name, t->die);
    mode = ui_menu(prompt, modes, NULL, 3);

    if (mode == 2) return NULL;

    if (mode == 0) {
        int roll = roll_table_die(t->die);
        const LifeEntry *e = entry_for(t, roll);
        if (!e) return NULL;
        printf("    Rolled %d: %s\n", roll, e->text);
        return e->text;
    }

    {
        const char *opts[128];
        static char labels[128][160];
        int i, n = t->count > 128 ? 128 : t->count;

        for (i = 0; i < n; i++) {
            /* Show the range too, so a player picking can still see what
               they would have had to roll. */
            if (t->rows[i].lo == t->rows[i].hi) {
                snprintf(labels[i], sizeof labels[i], "%3d     %s",
                         t->rows[i].lo, t->rows[i].text);
            } else {
                snprintf(labels[i], sizeof labels[i], "%3d-%-3d %s",
                         t->rows[i].lo, t->rows[i].hi, t->rows[i].text);
            }
            opts[i] = labels[i];
        }
        return t->rows[ui_menu("  Which one?", opts, NULL, n)].text;
    }
}

void build_backstory(Character *c)
{
    static const char *const modes[] = {
        "Go through the tables one at a time",
        "Roll the whole thing at once",
        "Leave it; I will write my own"
    };
    char out[MAX_TEXT];
    size_t used = 0;
    int i, mode, wrote = 0;

    ui_header("This Is Your Life");
    ui_para("Xanathar's tables for where a character came from: their "
            "parents, their birthplace, who raised them and what they did "
            "before adventuring. Roll for any of it, or pick the answer you "
            "already have in mind.");

    mode = ui_menu("  How would you like to do this?", modes, NULL, 3);
    if (mode == 2) return;

    out[0] = '\0';
    for (i = 0; i < LIFE_TABLE_COUNT; i++) {
        const LifeTable *t = &LIFE_TABLES[i];
        const char *answer;
        int w;

        if (mode == 1) {
            int roll = roll_table_die(t->die);
            const LifeEntry *e = entry_for(t, roll);
            answer = e ? e->text : NULL;
            if (answer) printf("  %-22s %s\n", t->name, answer);
        } else {
            answer = ask_table(t);
        }
        if (!answer) continue;

        w = snprintf(out + used, sizeof out - used, "%s%s: %s",
                     wrote ? ". " : "", t->name, answer);
        if (w < 0 || (size_t)w >= sizeof out - used) break;
        used += (size_t)w;
        wrote++;
    }

    if (!wrote) return;

    printf("\n");
    ui_wrap(out, 2);
    if (ui_yesno("\n  Keep this as your backstory?", 1)) {
        snprintf(c->backstory, sizeof c->backstory, "%s", out);
    }
}
