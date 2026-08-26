/* main.c -- a D&D 5th Edition character creator and builder.
 *
 * Creates a character following the six steps of Player's Handbook chapter 1
 * and saves everything to "<Charactername>.txt". A saved character can be
 * loaded again and levelled up.
 */
#include "dnd.h"
#include "data.h"
#include "build.h"
#include "reference.h"
#include "saveload.h"
#include "ui.h"
#include "data.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void save_and_report(const Character *c)
{
    char path[MAX_NAME + 8];

    if (save_character(c, path, sizeof path) == 0) {
        printf("\n  Saved to %s\n", path);
    } else {
        fprintf(stderr, "\n  Could not write the character file.\n");
    }
}

static void do_create(void)
{
    Character c;

    wizard_create(&c);

    ui_header("Your Character");
    print_sheet(&c);
    save_and_report(&c);
}

static void do_level_up(void)
{
    Character c;
    char name[MAX_NAME], path[MAX_NAME + 8];
    int r;

    ui_header("Load a Character");
    ui_line("  Character name (or a path to the .txt file)", name, sizeof name);
    if (!name[0]) {
        printf("  Nothing to load.\n");
        return;
    }

    if (strstr(name, ".txt")) {
        snprintf(path, sizeof path, "%s", name);
    } else {
        snprintf(path, sizeof path, "%s.txt", name);
    }

    r = load_character(path, &c);
    if (r == -1) {
        printf("  Could not open %s\n", path);
        return;
    }
    if (r == -2) {
        printf("  %s has no character data block; it was not written by this "
               "program.\n", path);
        return;
    }
    if (r < 0) {
        printf("  %s could not be read as a character.\n", path);
        return;
    }

    printf("  Loaded %s (level %d, %d hit points).\n", c.name, total_level(&c),
           hit_points_max(&c));

    for (;;) {
        wizard_level_up(&c);
        if (total_level(&c) >= MAX_LEVEL) break;
        if (!ui_yesno("\n  Gain another level?", 0)) break;
    }

    ui_header("Your Character");
    print_sheet(&c);
    save_and_report(&c);
}

static void do_view(void)
{
    Character c;
    char name[MAX_NAME], path[MAX_NAME + 8];

    ui_line("  Character name (or a path to the .txt file)", name, sizeof name);
    if (!name[0]) return;

    if (strstr(name, ".txt")) snprintf(path, sizeof path, "%s", name);
    else snprintf(path, sizeof path, "%s.txt", name);

    if (load_character(path, &c) != 0) {
        printf("  Could not read %s\n", path);
        return;
    }
    print_sheet(&c);
}

int main(int argc, char **argv)
{
    static const char *const menu[] = {
        "Create a new character",
        "Load a character and level up",
        "View a saved character",
        "Content settings (books and optional rules)",
        "Item reference (equipment, magic items, prices)",
        "Quit"
    };
    unsigned int seed = (unsigned int)time(NULL);
    int i;

    /* --seed makes dice reproducible, which the test script relies on. */
    for (i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
            seed = (unsigned int)strtoul(argv[++i], NULL, 10);
        } else if (!strcmp(argv[i], "--help")) {
            printf("Usage: %s [--seed N]\n\n"
                   "  Creates D&D 5th Edition characters following the "
                   "Player's Handbook\n"
                   "  and saves them to <Charactername>.txt.\n", argv[0]);
            return 0;
        }
    }
    rng_seed(seed);
    settings_defaults(&SETTINGS);

    for (;;) {
        int pick;

        printf("\n");
        ui_rule();
        printf("  D&D 5th Edition Character Creator\n");
        ui_rule();

        pick = ui_menu("  What would you like to do?", menu, NULL, 6);
        switch (pick) {
        case 0: do_create(); break;
        case 1: do_level_up(); break;
        case 2: do_view(); break;
        case 3: settings_menu(&SETTINGS); break;
        case 4: reference_menu(); break;
        default: return 0;
        }
    }
}
