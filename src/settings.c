/* settings.c -- which books and optional rules a character may draw on.
 *
 * Every table carries the book it came from, so restricting the sources is
 * a matter of filtering the menus. The chosen settings are written into the
 * character file, which means loading a character to level it up offers the
 * same content it was built with.
 */
#include "data.h"
#include "ui.h"

#include <stdio.h>
#include <string.h>

Settings SETTINGS;

const char *const BOOK_NAME[BOOK_COUNT] = {
    "Player's Handbook",
    "Xanathar's Guide to Everything",
    "Tasha's Cauldron of Everything",
    "Dungeon Master's Guide",
    "Mordenkainen Presents: Monsters of the Multiverse",
    "Monster Manual",
    "Sword Coast Adventurer's Guide",
    "Homebrew (your own items and spells)",
};

const char *const BOOK_ABBREV[BOOK_COUNT] = {
    "PHB", "XGE", "TCE", "DMG", "MPMM", "MM", "SCAG", "Homebrew",
};

void settings_defaults(Settings *s)
{
    int i;
    for (i = 0; i < BOOK_COUNT; i++) s->book[i] = 1;
    s->custom_origins = 0;      /* the PHB's fixed racial traits by default */
    s->optional_features = 1;
    s->multiclassing = 1;
    s->feats = 1;
    s->experience = 1;
    s->manual_dice = 0;
    ui_set_manual_dice(s->manual_dice);
}

int book_enabled(SourceBook b)
{
    if (b < 0 || b >= BOOK_COUNT) return 1;
    /* The Player's Handbook cannot be switched off; nothing would be left. */
    if (b == BOOK_PHB) return 1;
    return SETTINGS.book[b] != 0;
}

/* --------------------------------------------------------------- the menu */

static void show(const Settings *s)
{
    int i;

    printf("\n  Source books\n");
    for (i = 0; i < BOOK_COUNT; i++) {
        printf("   %2d) [%c] %-32s %s\n", i + 1,
               (i == BOOK_PHB || s->book[i]) ? 'x' : ' ',
               BOOK_NAME[i],
               i == BOOK_PHB ? "(always on)" : "");
    }
    printf("\n  Optional rules\n");
    printf("   %2d) [%c] Tasha's Customizing Your Origin\n",
           BOOK_COUNT + 1, s->custom_origins ? 'x' : ' ');
    printf("   %2d) [%c] Tasha's optional class features\n",
           BOOK_COUNT + 2, s->optional_features ? 'x' : ' ');
    printf("   %2d) [%c] Multiclassing (PHB chapter 6)\n",
           BOOK_COUNT + 3, s->multiclassing ? 'x' : ' ');
    printf("   %2d) [%c] Feats (PHB chapter 6)\n",
           BOOK_COUNT + 4, s->feats ? 'x' : ' ');
    printf("   %2d) [%c] Experience on the sheet\n",
           BOOK_COUNT + 5, s->experience ? 'x' : ' ');
    printf("   %2d) [%c] Roll your own dice (the program asks instead)\n",
           BOOK_COUNT + 6, s->manual_dice ? 'x' : ' ');
    printf("   %2d) Done\n", BOOK_COUNT + 7);
}

void settings_menu(Settings *s)
{
    ui_header("Content Settings");
    ui_para("Choose which books this character may draw on, and which "
            "optional rules are in play. Restricting the books hides their "
            "races, classes, subclasses, spells, feats and equipment "
            "everywhere in the wizard.");

    for (;;) {
        int pick;

        show(s);
        pick = ui_int("  Toggle", 1, BOOK_COUNT + 7);

        if (pick == BOOK_COUNT + 7) break;
        if (pick <= BOOK_COUNT) {
            int b = pick - 1;
            if (b == BOOK_PHB) {
                printf("  The Player's Handbook is always available.\n");
                continue;
            }
            s->book[b] = !s->book[b];
            continue;
        }
        switch (pick - BOOK_COUNT) {
        case 1: s->custom_origins = !s->custom_origins; break;
        case 2: s->optional_features = !s->optional_features; break;
        case 3: s->multiclassing = !s->multiclassing; break;
        case 4: s->feats = !s->feats; break;
        case 5: s->experience = !s->experience; break;
        default: s->manual_dice = !s->manual_dice; break;
        }
    }
    ui_set_manual_dice(s->manual_dice);

    /* Tasha's rules need Tasha's. */
    if (!s->book[BOOK_TCE] && (s->custom_origins || s->optional_features)) {
        printf("\n  Tasha's Cauldron is switched off, so its custom origins "
               "and optional class features are unavailable.\n");
        s->custom_origins = 0;
        s->optional_features = 0;
    }
}

/* A one-line summary for the character sheet. */
void settings_summary(const Settings *s, char *out, size_t n)
{
    size_t used = 0;
    int i, first = 1;

    out[0] = '\0';
    for (i = 0; i < BOOK_COUNT; i++) {
        int w;
        if (i != BOOK_PHB && !s->book[i]) continue;
        w = snprintf(out + used, n - used, "%s%s", first ? "" : ", ",
                     BOOK_ABBREV[i]);
        if (w < 0 || (size_t)w >= n - used) return;
        used += (size_t)w;
        first = 0;
    }
    snprintf(out + used, n - used, "; origins %s, optional features %s, "
             "multiclassing %s, feats %s, experience %s, dice %s",
             s->custom_origins ? "custom" : "PHB",
             s->optional_features ? "on" : "off",
             s->multiclassing ? "on" : "off",
             s->feats ? "on" : "off",
             s->experience ? "on" : "off",
             s->manual_dice ? "your own" : "rolled");
}
