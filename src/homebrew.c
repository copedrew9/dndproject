/* homebrew.c -- the DM's own items and spells.
 *
 * Everything the books provide is a const table compiled into the program.
 * What a DM invents cannot be, so this file keeps a second, growable copy of
 * each bank: at startup it reads homebrew.txt, allocates a new array holding
 * the book's entries followed by the custom ones, and points ITEMS, SPELLS
 * or MAGIC_ITEMS at it. Every menu, shop and lookup in the program reads
 * those pointers, so a homebrew entry appears everywhere a printed one does
 * without any of that code knowing it exists.
 *
 * Custom entries are tagged BOOK_HOMEBREW, so the settings menu can switch
 * them off the same way it switches off a book, and so a sheet says where an
 * item came from.
 *
 * The file is line-oriented and '|' separated, like the character files, so
 * a DM can write entries by hand or share them.
 */
#include "dnd.h"
#include "data.h"
#include "data_spells.h"
#include "homebrew.h"
#include "reference.h"
#include "ui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HOMEBREW_FILE "homebrew.txt"
#define MAX_CUSTOM 256

static ItemData  custom_items[MAX_CUSTOM];
static MagicItem custom_magic[MAX_CUSTOM];
static SpellData custom_spells[MAX_CUSTOM];
static int n_items, n_magic, n_spells;

/* Homebrew strings come from a file rather than the binary, so each one is
   copied onto the heap and kept for the life of the program. */
static const char *keep(const char *s)
{
    size_t n = strlen(s) + 1;
    char *p = malloc(n);
    if (!p) return "";
    memcpy(p, s, n);
    return p;
}

int homebrew_item_count(void)  { return n_items; }
int homebrew_magic_count(void) { return n_magic; }
int homebrew_spell_count(void) { return n_spells; }

int homebrew_total(void) { return n_items + n_magic + n_spells; }

/* ------------------------------------------------------- rebuilding a bank */

/* Points a bank at a fresh array of the book's entries plus the custom ones.
   The old array is leaked deliberately: it may still be pointed at by an
   index held elsewhere, and the number of rebuilds in one session is small
   and bounded by how many entries a DM types in by hand. */
#define REBUILD(TYPE, BANK, COUNT, BOOK_BANK, BOOK_COUNT, CUSTOM, NCUSTOM)   \
    do {                                                                     \
        TYPE *fresh = malloc(sizeof(TYPE) * (size_t)((BOOK_COUNT) + (NCUSTOM))); \
        if (!fresh) break;                                                   \
        memcpy(fresh, (BOOK_BANK), sizeof(TYPE) * (size_t)(BOOK_COUNT));     \
        memcpy(fresh + (BOOK_COUNT), (CUSTOM), sizeof(TYPE) * (size_t)(NCUSTOM)); \
        (BANK) = fresh;                                                      \
        (COUNT) = (BOOK_COUNT) + (NCUSTOM);                                  \
    } while (0)

static void rebuild_banks(void)
{
    REBUILD(ItemData, ITEMS, ITEM_COUNT, BOOK_ITEMS, BOOK_ITEM_COUNT,
            custom_items, n_items);
    REBUILD(MagicItem, MAGIC_ITEMS, MAGIC_ITEM_COUNT, BOOK_MAGIC_ITEMS,
            BOOK_MAGIC_ITEM_COUNT, custom_magic, n_magic);
    REBUILD(SpellData, SPELLS, SPELL_COUNT, BOOK_SPELLS, BOOK_SPELL_COUNT,
            custom_spells, n_spells);
}

/* ---------------------------------------------------------------- the file */

static int split_fields(char *line, char **out, int max)
{
    int n = 0;
    char *p = line;

    out[n++] = p;
    while (*p && n < max) {
        if (*p == '|') { *p = '\0'; out[n++] = p + 1; }
        p++;
    }
    return n;
}

static void strip_newline(char *s)
{
    size_t n = strlen(s);
    while (n && (s[n - 1] == '\n' || s[n - 1] == '\r')) s[--n] = '\0';
}

int homebrew_load(void)
{
    char line[1024];
    FILE *f = fopen(HOMEBREW_FILE, "r");

    n_items = n_magic = n_spells = 0;
    if (!f) return 0;                   /* no homebrew is not an error */

    while (fgets(line, sizeof line, f)) {
        char *fields[20];
        int n;

        strip_newline(line);
        if (!line[0] || line[0] == '#') continue;
        n = split_fields(line, fields, 20);

        if (!strcmp(fields[0], "ITEM") && n >= 13 && n_items < MAX_CUSTOM) {
            ItemData *it = &custom_items[n_items++];
            memset(it, 0, sizeof *it);
            it->name = keep(fields[1]);
            it->book = BOOK_HOMEBREW;
            it->category = (ItemCategory)atoi(fields[2]);
            it->cost_cp = atoi(fields[3]);
            it->weight_tenths = atoi(fields[4]);
            it->base_ac = atoi(fields[5]);
            it->dex_cap = atoi(fields[6]);
            it->str_req = atoi(fields[7]);
            it->stealth_disadvantage = atoi(fields[8]);
            it->damage = keep(fields[9]);
            it->damage_type = keep(fields[10]);
            it->properties = keep(fields[11]);
            it->contents = keep(fields[12]);
        } else if (!strcmp(fields[0], "MAGICITEM") && n >= 6
                   && n_magic < MAX_CUSTOM) {
            MagicItem *m = &custom_magic[n_magic++];
            m->name = keep(fields[1]);
            m->book = BOOK_HOMEBREW;
            m->type = keep(fields[2]);
            m->rarity = keep(fields[3]);
            m->attunement = fields[4][0] ? keep(fields[4]) : NULL;
            m->text = keep(fields[5]);
        } else if (!strcmp(fields[0], "SPELL") && n >= 12
                   && n_spells < MAX_CUSTOM) {
            SpellData *sp = &custom_spells[n_spells++];
            sp->name = keep(fields[1]);
            sp->book = BOOK_HOMEBREW;
            sp->level = (unsigned char)atoi(fields[2]);
            sp->school = (unsigned char)atoi(fields[3]);
            sp->ritual = (unsigned char)atoi(fields[4]);
            sp->concentration = (unsigned char)atoi(fields[5]);
            sp->casting_time = keep(fields[6]);
            sp->range = keep(fields[7]);
            sp->components = keep(fields[8]);
            sp->duration = keep(fields[9]);
            sp->classes = (unsigned short)atoi(fields[10]);
        }
    }
    fclose(f);

    if (homebrew_total()) rebuild_banks();
    return homebrew_total();
}

int homebrew_save(void)
{
    FILE *f;
    int i;

    if (homebrew_total() == 0) {
        remove(HOMEBREW_FILE);
        return 0;
    }
    f = fopen(HOMEBREW_FILE, "w");
    if (!f) return -1;

    fprintf(f, "# Homebrew items and spells, read at startup.\n"
               "# ITEM|name|category|cost_cp|weight_tenths|base_ac|dex_cap|"
               "str_req|stealth|damage|damage_type|properties|contents\n"
               "# MAGICITEM|name|type|rarity|attunement|text\n"
               "# SPELL|name|level|school|ritual|concentration|casting_time|"
               "range|components|duration|classes|\n");

    for (i = 0; i < n_items; i++) {
        const ItemData *it = &custom_items[i];
        fprintf(f, "ITEM|%s|%d|%d|%d|%d|%d|%d|%d|%s|%s|%s|%s\n", it->name,
                (int)it->category, it->cost_cp, it->weight_tenths,
                it->base_ac, it->dex_cap, it->str_req,
                it->stealth_disadvantage, it->damage, it->damage_type,
                it->properties, it->contents);
    }
    for (i = 0; i < n_magic; i++) {
        const MagicItem *m = &custom_magic[i];
        fprintf(f, "MAGICITEM|%s|%s|%s|%s|%s\n", m->name, m->type, m->rarity,
                m->attunement ? m->attunement : "", m->text);
    }
    for (i = 0; i < n_spells; i++) {
        const SpellData *sp = &custom_spells[i];
        fprintf(f, "SPELL|%s|%d|%d|%d|%d|%s|%s|%s|%s|%d|\n", sp->name,
                sp->level, sp->school, sp->ritual, sp->concentration,
                sp->casting_time, sp->range, sp->components, sp->duration,
                sp->classes);
    }
    fclose(f);
    return 0;
}

/* --------------------------------------------------------------- the screen */

/* A name already in the bank would shadow or be shadowed by the book's own
   entry, since lookups go by name. */
static int spell_named(const char *name)
{
    int i;
    for (i = 0; i < SPELL_COUNT; i++) {
        if (!strcmp(SPELLS[i].name, name)) return 1;
    }
    return 0;
}

static int name_taken(const char *name)
{
    return find_item(name) >= 0 || find_magic_item(name) >= 0
        || spell_named(name);
}

static int ask_name(const char *prompt, char *out, size_t n)
{
    for (;;) {
        ui_line(prompt, out, n);
        if (!out[0]) return 0;
        if (!name_taken(out)) return 1;
        printf("  Something is already called that. Names have to be "
               "unique, because everything is looked up by name.\n");
        if (!ui_yesno("  Try another name?", 1)) return 0;
    }
}

static void add_custom_item(void)
{
    ItemData *it;
    char buf[MAX_TEXT];

    if (n_items >= MAX_CUSTOM) {
        printf("  That is as many custom items as this program holds.\n");
        return;
    }
    it = &custom_items[n_items];
    memset(it, 0, sizeof *it);

    if (!ask_name("  What is the item called", buf, sizeof buf)) return;
    it->name = keep(buf);
    it->book = BOOK_HOMEBREW;

    {
        const char *cats[13];
        int i;
        for (i = 0; i < 12; i++) cats[i] = CATEGORY_LABEL[i];
        it->category = (ItemCategory)ui_menu("  What kind of thing is it?",
                                             cats, NULL, 12);
    }

    it->cost_cp = ui_int("  Cost in copper pieces (1 gp is 100)", 0, 10000000);
    it->weight_tenths = ui_int("  Weight in tenths of a pound (1 lb is 10)",
                               0, 100000);

    if (it->category <= ITEM_SHIELD) {
        it->base_ac = ui_int("  Base Armor Class", 0, 25);
        printf("  Dexterity: -1 adds the full modifier, 0 adds none, or "
               "give a cap.\n");
        it->dex_cap = ui_int("  Dexterity handling", -1, 10);
        it->str_req = ui_int("  Strength required (0 for none)", 0, 20);
        it->stealth_disadvantage =
            ui_yesno("  Disadvantage on Stealth checks?", 0);
        it->damage = keep(""); it->damage_type = keep("");
        it->properties = keep(""); it->contents = keep("");
    } else if (it->category <= ITEM_MARTIAL_RANGED) {
        ui_line_default("  Damage dice", "1d6", buf, sizeof buf);
        it->damage = keep(buf);
        ui_line_default("  Damage type", "slashing", buf, sizeof buf);
        it->damage_type = keep(buf);
        ui_line("  Properties (as they would be printed, blank for none)",
                buf, sizeof buf);
        it->properties = keep(buf);
        it->contents = keep("");
    } else {
        it->damage = keep(""); it->damage_type = keep("");
        it->properties = keep("");
        ui_line("  Contents, if it is a pack or kit (blank for none)", buf,
                sizeof buf);
        it->contents = keep(buf);
    }

    n_items++;
    rebuild_banks();
    printf("  Added %s to the item bank.\n", it->name);
}

static void add_custom_magic_item(void)
{
    MagicItem *m;
    char buf[MAX_TEXT];

    if (n_magic >= MAX_CUSTOM) {
        printf("  That is as many custom magic items as this program "
               "holds.\n");
        return;
    }
    m = &custom_magic[n_magic];

    if (!ask_name("  What is the magic item called", buf, sizeof buf)) return;
    m->name = keep(buf);
    m->book = BOOK_HOMEBREW;

    ui_line_default("  Type (as a stat line would read it)", "Wondrous item",
                    buf, sizeof buf);
    m->type = keep(buf);

    {
        static const char *const rarities[] = {
            "common", "uncommon", "rare", "very rare", "legendary",
            "artifact"
        };
        m->rarity = keep(rarities[ui_menu("  Rarity:", rarities, NULL, 6)]);
    }

    if (ui_yesno("  Does it need attunement?", 0)) {
        ui_line_default("  Attunement requirement", "requires attunement",
                        buf, sizeof buf);
        m->attunement = keep(buf);
    } else {
        m->attunement = NULL;
    }

    ui_line("  What does it do", buf, sizeof buf);
    m->text = keep(buf[0] ? buf : "No description was given.");

    n_magic++;
    rebuild_banks();
    printf("  Added %s to the magic item bank.\n", m->name);
}

static void add_custom_spell(void)
{
    SpellData *sp;
    char buf[MAX_TEXT];

    if (n_spells >= MAX_CUSTOM) {
        printf("  That is as many custom spells as this program holds.\n");
        return;
    }
    sp = &custom_spells[n_spells];
    memset(sp, 0, sizeof *sp);

    if (!ask_name("  What is the spell called", buf, sizeof buf)) return;
    sp->name = keep(buf);
    sp->book = BOOK_HOMEBREW;

    sp->level = (unsigned char)ui_int("  Spell level (0 for a cantrip)", 0, 9);

    {
        const char *schools[SCHOOL_COUNT];
        int i;
        for (i = 0; i < SCHOOL_COUNT; i++) schools[i] = SCHOOL_NAMES[i];
        sp->school = (unsigned char)ui_menu("  School:", schools, NULL,
                                            SCHOOL_COUNT);
    }

    sp->ritual = (unsigned char)ui_yesno("  Can it be cast as a ritual?", 0);
    sp->concentration =
        (unsigned char)ui_yesno("  Does it need concentration?", 0);

    ui_line_default("  Casting time", "1 action", buf, sizeof buf);
    sp->casting_time = keep(buf);
    ui_line_default("  Range", "60 feet", buf, sizeof buf);
    sp->range = keep(buf);
    ui_line_default("  Components", "V, S", buf, sizeof buf);
    sp->components = keep(buf);
    ui_line_default("  Duration", "Instantaneous", buf, sizeof buf);
    sp->duration = keep(buf);

    /* Which lists it appears on decides who can ever learn it. */
    {
        static const char *const names[9] = {
            "Bard", "Cleric", "Druid", "Paladin", "Ranger", "Sorcerer",
            "Warlock", "Wizard", "Artificer"
        };
        static const unsigned short bits[9] = {
            SPL_BARD, SPL_CLERIC, SPL_DRUID, SPL_PALADIN, SPL_RANGER,
            SPL_SORCERER, SPL_WARLOCK, SPL_WIZARD, SPL_ARTIFICER
        };
        int i;
        printf("\n  Which class spell lists is it on? A spell on no list "
               "can never be learned.\n");
        sp->classes = 0;
        for (i = 0; i < 9; i++) {
            char prompt[64];
            snprintf(prompt, sizeof prompt, "    %s", names[i]);
            if (ui_yesno(prompt, 0)) sp->classes |= bits[i];
        }
        if (sp->classes == 0) {
            printf("  No list chosen, so nobody could learn it. Not "
                   "added.\n");
            return;
        }
    }

    n_spells++;
    rebuild_banks();
    printf("  Added %s to the spell bank.\n", sp->name);
}

/* ------------------------------------------------------------ removing one */

/* Removing shifts the array down, which changes the indices of everything
   after it -- fine, because saved characters store names, not indices. */
static void remove_custom(void)
{
    const char *opts[MAX_CUSTOM * 3 + 1];
    static char lines[MAX_CUSTOM * 3][96];
    int kind[MAX_CUSTOM * 3], slot[MAX_CUSTOM * 3];
    int n = 0, i, pick;

    for (i = 0; i < n_items; i++) {
        snprintf(lines[n], sizeof lines[n], "Item        %s",
                 custom_items[i].name);
        opts[n] = lines[n]; kind[n] = 0; slot[n] = i; n++;
    }
    for (i = 0; i < n_magic; i++) {
        snprintf(lines[n], sizeof lines[n], "Magic item  %s",
                 custom_magic[i].name);
        opts[n] = lines[n]; kind[n] = 1; slot[n] = i; n++;
    }
    for (i = 0; i < n_spells; i++) {
        snprintf(lines[n], sizeof lines[n], "Spell       %s",
                 custom_spells[i].name);
        opts[n] = lines[n]; kind[n] = 2; slot[n] = i; n++;
    }
    if (n == 0) {
        printf("  There is no homebrew to remove.\n");
        return;
    }
    opts[n] = "Back";

    pick = ui_menu("  Remove which?", opts, NULL, n + 1);
    if (pick == n) return;

    printf("  Characters already saved keep whatever they were given; this "
           "only takes it out of the bank.\n");
    if (!ui_yesno("  Remove it?", 0)) return;

    if (kind[pick] == 0) {
        for (i = slot[pick]; i < n_items - 1; i++)
            custom_items[i] = custom_items[i + 1];
        n_items--;
    } else if (kind[pick] == 1) {
        for (i = slot[pick]; i < n_magic - 1; i++)
            custom_magic[i] = custom_magic[i + 1];
        n_magic--;
    } else {
        for (i = slot[pick]; i < n_spells - 1; i++)
            custom_spells[i] = custom_spells[i + 1];
        n_spells--;
    }
    rebuild_banks();
    printf("  Removed.\n");
}

static void list_homebrew(void)
{
    int i;

    if (homebrew_total() == 0) {
        printf("\n  Nothing has been added yet.\n");
        return;
    }
    printf("\n  %d item%s, %d magic item%s and %d spell%s, held in %s.\n",
           n_items, n_items == 1 ? "" : "s",
           n_magic, n_magic == 1 ? "" : "s",
           n_spells, n_spells == 1 ? "" : "s", HOMEBREW_FILE);

    for (i = 0; i < n_items; i++) {
        printf("    Item        %-28s %s\n", custom_items[i].name,
               CATEGORY_LABEL[custom_items[i].category]);
    }
    for (i = 0; i < n_magic; i++) {
        printf("    Magic item  %-28s %s%s\n", custom_magic[i].name,
               custom_magic[i].rarity,
               custom_magic[i].attunement ? ", attunement" : "");
    }
    for (i = 0; i < n_spells; i++) {
        printf("    Spell       %-28s %s %s\n", custom_spells[i].name,
               custom_spells[i].level ? "level" : "cantrip",
               SCHOOL_NAMES[custom_spells[i].school]);
    }
}

void homebrew_menu(void)
{
    int changed = 0;

    ui_header("Homebrew");
    ui_para("Items and spells of your own, kept alongside the printed ones. "
            "Anything added here shows up wherever the books' entries do -- "
            "in the shop, the spell picker, the item reference -- and is "
            "marked as homebrew. Switching Homebrew off in the content "
            "settings hides it all without deleting anything.");

    for (;;) {
        static const char *const modes[] = {
            "List what has been added",
            "Add an item",
            "Add a magic item",
            "Add a spell",
            "Remove something",
            "Done"
        };
        int before = homebrew_total();

        switch (ui_menu("  Homebrew:", modes, NULL, 6)) {
        case 0: list_homebrew();         break;
        case 1: add_custom_item();       break;
        case 2: add_custom_magic_item(); break;
        case 3: add_custom_spell();      break;
        case 4: remove_custom();         break;
        default:
            if (changed) {
                if (homebrew_save() == 0) {
                    printf("  Saved to %s\n", HOMEBREW_FILE);
                } else {
                    printf("  Could not write %s\n", HOMEBREW_FILE);
                }
            }
            return;
        }
        if (homebrew_total() != before) changed = 1;
    }
}
