/* main.c -- a D&D 5th Edition character creator and builder.
 *
 * Creates a character following the six steps of Player's Handbook chapter 1
 * and saves everything to "<Charactername>.txt". A saved character can be
 * loaded again and levelled up.
 */
#include "dnd.h"
#include "data.h"
#include "build.h"
#include "details.h"
#include "homebrew.h"
#include "inventory.h"
#include "sidekick.h"
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

    /* Levelling up is usually the moment a character's gear has changed too,
       so offer the inventory before saving rather than making it a separate
       trip through the main menu. */
    if (ui_yesno("\n  Change what this character is carrying?", 0)) {
        manage_inventory(&c);
    }

    /* A sidekick gains a level whenever the party's average level does, so
       this is exactly when to ask. */
    if (c.sidekick_count
        && ui_yesno("\n  Level up their sidekicks too?", 1)) {
        manage_sidekicks(&c);
    }

    if (ui_yesno("\n  Add or change any notes?", 0)) {
        edit_details(&c);
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

/* Loads a saved character by name or path. Returns 0 on success. */
static int load_by_name(const char *prompt, Character *c)
{
    char name[MAX_NAME], path[MAX_NAME + 8];

    ui_line(prompt, name, sizeof name);
    if (!name[0]) return -1;

    if (strstr(name, ".txt")) snprintf(path, sizeof path, "%s", name);
    else snprintf(path, sizeof path, "%s.txt", name);

    if (load_character(path, c) != 0) {
        printf("  Could not read %s\n", path);
        return -1;
    }
    return 0;
}

static void do_details(void)
{
    Character c;

    ui_header("Notes and Character Details");
    if (load_by_name("  Character name (or a path to the .txt file)", &c))
        return;
    printf("  Loaded %s (level %d).\n", c.name, total_level(&c));

    edit_details(&c);
    if (ui_yesno("\n  Save the changes?", 1)) save_and_report(&c);
}

static void do_sidekicks(void)
{
    Character c;

    ui_header("Manage a Character's Sidekicks");
    if (load_by_name("  Character name (or a path to the .txt file)", &c))
        return;
    printf("  Loaded %s (level %d).\n", c.name, total_level(&c));

    manage_sidekicks(&c);
    if (ui_yesno("\n  Save the changes?", 1)) save_and_report(&c);
}

/* Loading a character purely to change what they carry, then saving. */
static void do_inventory(void)
{
    Character c;

    ui_header("Manage a Character's Gear");
    if (load_by_name("  Character name (or a path to the .txt file)", &c))
        return;
    printf("  Loaded %s (level %d).\n", c.name, total_level(&c));

    manage_inventory(&c);
    if (ui_yesno("\n  Save the changes?", 1)) save_and_report(&c);
}

int main(int argc, char **argv)
{
    static const char *const menu[] = {
        "Create a new character",
        "Load a character and level up",
        "View a saved character",
        "Content settings (books and optional rules)",
        "Item reference (equipment, magic items, prices)",
        "Manage a character's inventory",
        "Manage a character's sidekicks",
        "Homebrew (your own items and spells)",
        "Notes and character details",
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

    /* Fold the DM's own items and spells into the banks before anything
       reads them, so homebrew appears wherever a printed entry would. */
    {
        int added = homebrew_load();
        if (added) {
            printf("  Loaded %d homebrew entr%s.\n", added,
                   added == 1 ? "y" : "ies");
        }
    }

    for (;;) {
        int pick;

        printf("\n");
        ui_rule();
        printf("  D&D 5th Edition Character Creator\n");
        ui_rule();

        pick = ui_menu("  What would you like to do?", menu, NULL, 10);
        switch (pick) {
        case 0: do_create(); break;
        case 1: do_level_up(); break;
        case 2: do_view(); break;
        case 3: settings_menu(&SETTINGS); break;
        case 4: reference_menu(); break;
        case 5: do_inventory(); break;
        case 6: do_sidekicks(); break;
        case 7: homebrew_menu(); break;
        case 8: do_details(); break;
        default: return 0;
        }
    }
}
